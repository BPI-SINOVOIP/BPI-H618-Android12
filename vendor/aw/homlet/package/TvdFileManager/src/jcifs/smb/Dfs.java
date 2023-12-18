/* jcifs smb client library in Java
 * Copyright (C) 2008  "Michael B. Allen" <jcifs at samba dot org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

package jcifs.smb;

import java.util.*;
import java.io.*;

import jcifs.UniAddress;
import jcifs.util.*;
import jcifs.Config;

public class Dfs {

    static class CacheEntry {
        long expiration;
        HashMap map;

        CacheEntry(long ttl) {
            if (ttl == 0)
                ttl = Dfs.TTL;
            expiration = System.currentTimeMillis() + ttl * 1000L;
            map = new HashMap();
        }
    }

    static LogStream log = LogStream.getInstance();
    static final boolean strictView = Config.getBoolean("jcifs.smb.client.dfs.strictView", false);
    static final long TTL = Config.getLong("jcifs.smb.client.dfs.ttl", 300);
    static final boolean DISABLED = Config.getBoolean("jcifs.smb.client.dfs.disabled", true);

    protected static CacheEntry FALSE_ENTRY = new Dfs.CacheEntry(0L);

    protected CacheEntry _domains = null; /* aka trusted domains cache */
    protected CacheEntry referrals = null;

    public HashMap getTrustedDomains(NtlmPasswordAuthentication auth) throws SmbAuthException {
        if (DISABLED)
            return null;

        if (_domains != null && System.currentTimeMillis() > _domains.expiration) {
            _domains = null;
        }
        if (_domains != null)
            return _domains.map;
        try {
            UniAddress addr = UniAddress.getByName(auth.domain, true);
            SmbTransport trans = SmbTransport.getSmbTransport(addr, 0);
            DfsReferral[] dr = trans.getDfsReferrals(auth, "", 0);
            CacheEntry entry = new CacheEntry(Dfs.TTL * 10L);
            for (int di = 0; di < dr.length; di++) {
                String domain = dr[di].server;
                entry.map.put(domain, new HashMap());
            }
            _domains = entry;
            return _domains.map;
        } catch (IOException ioe) {
            if (log.level >= 3)
                ioe.printStackTrace(log);
            if (strictView && ioe instanceof SmbAuthException) {
                throw (SmbAuthException) ioe;
            }
        }
        return null;
    }

    public boolean isTrustedDomain(String domain, NtlmPasswordAuthentication auth)
            throws SmbAuthException {
        HashMap domains = getTrustedDomains(auth);
        if (domains == null)
            return false;
        domain = domain.toLowerCase();
        return domains.get(domain) != null;
    }

    public SmbTransport getDc(String domain, NtlmPasswordAuthentication auth)
            throws SmbAuthException {
        if (DISABLED)
            return null;

        try {
            UniAddress addr = UniAddress.getByName(domain, true);
            SmbTransport trans = SmbTransport.getSmbTransport(addr, 0);
            DfsReferral[] dr = trans.getDfsReferrals(auth, "\\" + domain, 1);
            if (dr.length == 1) {
                addr = UniAddress.getByName(dr[0].server);
                return SmbTransport.getSmbTransport(addr, 0);
            }
        } catch (IOException ioe) {
            if (log.level >= 3)
                ioe.printStackTrace(log);
            if (strictView && ioe instanceof SmbAuthException) {
                throw (SmbAuthException) ioe;
            }
        }
        return null;
    }

    public DfsReferral getReferral(SmbTransport trans, String domain, String root, String path,
            NtlmPasswordAuthentication auth) throws SmbAuthException {
        if (DISABLED)
            return null;

        try {
            String p = "\\" + domain + "\\" + root;
            if (path != null)
                p += path;
            DfsReferral[] dr = trans.getDfsReferrals(auth, p, 1);
            if (dr.length == 1)
                return dr[0];
        } catch (IOException ioe) {
            if (log.level >= 3)
                ioe.printStackTrace(log);
            if (strictView && ioe instanceof SmbAuthException) {
                throw (SmbAuthException) ioe;
            }
        }
        return null;
    }

    public synchronized DfsReferral resolve(String domain, String root, String path,
            NtlmPasswordAuthentication auth) throws SmbAuthException {
        DfsReferral dr = null;
        long now = System.currentTimeMillis();

        if (DISABLED || root.equals("IPC$")) {
            return null;
        }
        HashMap domains = getTrustedDomains(auth);
        if (domains != null) {
            domain = domain.toLowerCase();
            HashMap roots = (HashMap) domains.get(domain);
            if (roots != null) {
                SmbTransport trans = null;

                root = root.toLowerCase();

                CacheEntry links = (CacheEntry) roots.get(root);
                if (links != null && now > links.expiration) {
                    roots.remove(root);
                    links = null;
                }
                if (links == null) {
                    if ((trans = getDc(domain, auth)) == null)
                        return null;

                    dr = getReferral(trans, domain, root, path, auth);
                    if (dr != null) {
                        dr.pathConsumed -= 1 + domain.length() + 1 + root.length();
                        links = new CacheEntry(0L);
                        if (path == null)
                            links.map.put("\\", dr);
                        roots.put(root, links);
                    } else if (path == null) {
                        roots.put(root, Dfs.FALSE_ENTRY);
                    }
                } else if (links == Dfs.FALSE_ENTRY) {
                    links = null;
                }

                if (links != null) {
                    String link = "\\";
                    if (path != null) {
                        int i = path.indexOf("\\", 1);
                        link = i > 0 ? path.substring(1, i) : path.substring(1);
                    }

                    dr = (DfsReferral) links.map.get(link);
                    if (dr != null && now > dr.expiration) {
                        dr = null;
                        links.map.remove(link);
                    }
                    if (dr == null) {
                        if (trans == null)
                            if ((trans = getDc(domain, auth)) == null)
                                return null;
                        dr = getReferral(trans, domain, root, path, auth);
                        if (dr != null) {
                            dr.pathConsumed -= 1 + domain.length() + 1 + root.length();
                            dr.link = link;
                            links.map.put(link, dr);
                        }
                    }
                }
            }
        }

        if (dr == null && path != null) {
            if (referrals != null && now > referrals.expiration) {
                referrals = null;
            }
            if (referrals == null) {
                referrals = new CacheEntry(0);
            }
            String key = "\\" + domain + "\\" + root;
            if (path.equals("\\") == false)
                key += path;
            key = key.toLowerCase();

            Iterator iter = referrals.map.keySet().iterator();
            while (iter.hasNext()) {
                String _key = (String) iter.next();
                int _klen = _key.length();
                boolean match = false;

                if (_klen == key.length()) {
                    match = _key.equals(key);
                } else if (_klen < key.length()) {
                    match = _key.regionMatches(0, key, 0, _klen) && key.charAt(_klen) == '\\';
                }

                if (match)
                    dr = (DfsReferral) referrals.map.get(_key);
            }
        }

        return dr;
    }

    synchronized void insert(String path, DfsReferral dr) {
        int s1, s2;
        String server, share, key;

        if (DISABLED)
            return;

        s1 = path.indexOf('\\', 1);
        s2 = path.indexOf('\\', s1 + 1);
        server = path.substring(1, s1);
        share = path.substring(s1 + 1, s2);

        key = path.substring(0, dr.pathConsumed).toLowerCase();
        /*
         * Subtract the server and share from the pathConsumed so that it
         * refects the part of the relative path consumed and not the entire
         * path.
         */
        dr.pathConsumed -= 1 + server.length() + 1 + share.length();

        if (referrals != null && (System.currentTimeMillis() + 10000) > referrals.expiration) {
            referrals = null;
        }
        if (referrals == null) {
            referrals = new CacheEntry(0);
        }
        referrals.map.put(key, dr);
    }
}
