/* jcifs smb client library in Java
 * Copyright (C) 2000  "Michael B. Allen" <jcifs at samba dot org>
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

import java.util.Vector;
import java.util.Enumeration;
import java.net.InetAddress;
import java.net.UnknownHostException;
import jcifs.Config;
import jcifs.UniAddress;
import jcifs.netbios.NbtAddress;

/**
 * The class represents a user's session established with an SMB/CIFS server.
 * This class is used internally to the jCIFS library however applications may
 * wish to authenticate aribrary user credentials with the <tt>logon</tt>
 * method. It is noteworthy that jCIFS does not support DCE/RPC at this time and
 * therefore does not use the NETLOGON procedure. Instead, it simply performs a
 * "tree connect" to IPC$ using the supplied credentials. This is only a subset
 * of the NETLOGON procedure but is achives the same effect.
 *
 * Note that it is possible to change the resource against which clients are
 * authenticated to be something other than <tt>IPC$</tt> using the
 * <tt>jcifs.smb.client.logonShare</tt> property. This can be used to provide
 * simple group based access control. For example, one could setup the NTLM HTTP
 * Filter with the <tt>jcifs.smb.client.domainController</tt> init parameter set
 * to the name of the server used for authentication. On that host, create a
 * share called JCIFSAUTH and adjust the access control list for that share to
 * permit only the clients that should have access to the target website.
 * Finally, set the <tt>jcifs.smb.client.logonShare</tt> to JCIFSAUTH. This
 * should restrict access to only those clients that have access to the
 * JCIFSAUTH share. The access control on that share can be changed without
 * changing init parameters or reinitializing the webapp.
 */

public final class SmbSession {

    private static final String LOGON_SHARE = Config.getProperty("jcifs.smb.client.logonShare",
            null);
    private static final int LOOKUP_RESP_LIMIT = Config.getInt("jcifs.netbios.lookupRespLimit", 3);
    private static final String DOMAIN = Config.getProperty("jcifs.smb.client.domain", null);
    private static final String USERNAME = Config.getProperty("jcifs.smb.client.username", null);
    private static final int CACHE_POLICY = Config.getInt("jcifs.netbios.cachePolicy", 60 * 10) * 60; /*
                                                                                                       * 10
                                                                                                       * hours
                                                                                                       */

    static NbtAddress[] dc_list = null;
    static long dc_list_expiration;
    static int dc_list_counter;

    private static NtlmChallenge interrogate(NbtAddress addr) throws SmbException {
        UniAddress dc = new UniAddress(addr);
        SmbTransport trans = SmbTransport.getSmbTransport(dc, 0);
        if (USERNAME == null) {
            trans.connect();
            if (SmbTransport.log.level >= 3)
                SmbTransport.log.println("Default credentials (jcifs.smb.client.username/password)"
                        + " not specified. SMB signing may not work propertly."
                        + "  Skipping DC interrogation.");
        } else {
            SmbSession ssn = trans.getSmbSession(NtlmPasswordAuthentication.DEFAULT);
            ssn.getSmbTree(LOGON_SHARE, null).treeConnect(null, null);
        }
        return new NtlmChallenge(trans.server.encryptionKey, dc);
    }

    public static NtlmChallenge getChallengeForDomain() throws SmbException, UnknownHostException {
        if (DOMAIN == null) {
            throw new SmbException("A domain was not specified");
        }
        synchronized(DOMAIN) {
            long now = System.currentTimeMillis();
            int retry = 1;

            do {
                if (dc_list_expiration < now) {
                    NbtAddress[] list = NbtAddress.getAllByName(DOMAIN, 0x1C, null, null);
                    dc_list_expiration = now + CACHE_POLICY * 1000L;
                    if (list != null && list.length > 0) {
                        dc_list = list;
                    } else { /* keep using the old list */
                        dc_list_expiration = now + 1000 * 60 * 15; /* 15 min */
                        if (SmbTransport.log.level >= 2) {
                            SmbTransport.log.println("Failed to retrieve DC list from WINS");
                        }
                    }
                }

                int max = Math.min(dc_list.length, LOOKUP_RESP_LIMIT);
                for (int j = 0; j < max; j++) {
                    int i = dc_list_counter++ % max;
                    if (dc_list[i] != null) {
                        try {
                            return interrogate(dc_list[i]);
                        } catch (SmbException se) {
                            if (SmbTransport.log.level >= 2) {
                                SmbTransport.log.println("Failed validate DC: " + dc_list[i]);
                                if (SmbTransport.log.level > 2)
                                    se.printStackTrace(SmbTransport.log);
                            }
                        }
                        dc_list[i] = null;
                    }
                }

                /*
                 * No DCs found, for retieval of list by expiring it and retry.
                 */
                dc_list_expiration = 0;
            } while (retry-- > 0);

            dc_list_expiration = now + 1000 * 60 * 15; /* 15 min */
        }

        throw new UnknownHostException("Failed to negotiate with a suitable domain controller for "
                + DOMAIN);
    }

    public static byte[] getChallenge(UniAddress dc) throws SmbException, UnknownHostException {
        return getChallenge(dc, 0);
    }

    public static byte[] getChallenge(UniAddress dc, int port) throws SmbException,
            UnknownHostException {
        SmbTransport trans = SmbTransport.getSmbTransport(dc, port);
        trans.connect();
        return trans.server.encryptionKey;
    }

    /**
     * Authenticate arbitrary credentials represented by the
     * <tt>NtlmPasswordAuthentication</tt> object against the domain controller
     * specified by the <tt>UniAddress</tt> parameter. If the credentials are
     * not accepted, an <tt>SmbAuthException</tt> will be thrown. If an error
     * occurs an <tt>SmbException</tt> will be thrown. If the credentials are
     * valid, the method will return without throwing an exception. See the last
     * <a href="../../../faq.html">FAQ</a> question.
     *
     * See also the <tt>jcifs.smb.client.logonShare</tt> property.
     */
    public static void logon(UniAddress dc, NtlmPasswordAuthentication auth) throws SmbException {
        logon(dc, 0, auth);
    }

    public static void logon(UniAddress dc, int port, NtlmPasswordAuthentication auth)
            throws SmbException {
        SmbTree tree = SmbTransport.getSmbTransport(dc, port).getSmbSession(auth)
                .getSmbTree(LOGON_SHARE, null);
        if (LOGON_SHARE == null) {
            tree.treeConnect(null, null);
        } else {
            Trans2FindFirst2 req = new Trans2FindFirst2("\\", "*", SmbFile.ATTR_DIRECTORY);
            Trans2FindFirst2Response resp = new Trans2FindFirst2Response();
            tree.send(req, resp);
        }
    }

    private int uid;
    Vector trees;
    private boolean sessionSetup;
    // Transport parameters allows trans to be removed from CONNECTIONS
    private UniAddress address;
    private int port, localPort;
    private InetAddress localAddr;

    SmbTransport transport = null;
    NtlmPasswordAuthentication auth;
    long expiration;

    SmbSession(UniAddress address, int port, InetAddress localAddr, int localPort,
            NtlmPasswordAuthentication auth) {
        this.address = address;
        this.port = port;
        this.localAddr = localAddr;
        this.localPort = localPort;
        this.auth = auth;
        trees = new Vector();
    }

    synchronized SmbTree getSmbTree(String share, String service) {
        SmbTree t;

        if (share == null) {
            share = "IPC$";
        }
        for (Enumeration e = trees.elements(); e.hasMoreElements();) {
            t = (SmbTree) e.nextElement();
            if (t.matches(share, service)) {
                return t;
            }
        }
        t = new SmbTree(this, share, service);
        trees.addElement(t);
        return t;
    }

    boolean matches(NtlmPasswordAuthentication auth) {
        return this.auth == auth || this.auth.equals(auth);
    }

    synchronized SmbTransport transport() {
        if (transport == null) {
            transport = SmbTransport.getSmbTransport(address, port, localAddr, localPort);
        }
        return transport;
    }

    void send(ServerMessageBlock request, ServerMessageBlock response) throws SmbException {
        if (response != null) {
            response.received = false;
        }

        synchronized(transport.setupDiscoLock) {
            expiration = System.currentTimeMillis() + SmbTransport.SO_TIMEOUT;
            sessionSetup(request, response);
            if (response != null && response.received) {
                return;
            }
            request.uid = uid;
            request.auth = auth;
            try {
                transport.send(request, response);
            } catch (SmbException se) {
                if (request instanceof SmbComTreeConnectAndX) {
                    logoff(true);
                }
                request.digest = null;
                throw se;
            }
        }
    }

    void sessionSetup(ServerMessageBlock andx, ServerMessageBlock andxResponse) throws SmbException {
        SmbException ex = null;

        synchronized(transport()) {
            if (sessionSetup) {
                return;
            }

            transport.connect();

            /*
             * Session Setup And X Request / Response
             */

            if (transport.log.level >= 4)
                transport.log.println("sessionSetup: accountName=" + auth.username
                        + ",primaryDomain=" + auth.domain);

            SmbComSessionSetupAndX request = new SmbComSessionSetupAndX(this, andx);
            SmbComSessionSetupAndXResponse response = new SmbComSessionSetupAndXResponse(
                    andxResponse);

            /*
             * Create SMB signature digest if necessary Only the first
             * SMB_COM_SESSION_SETUP_ANX with non-null or blank password
             * initializes signing.
             */
            if (transport.isSignatureSetupRequired(auth)) {
                if (auth.hashesExternal
                        && NtlmPasswordAuthentication.DEFAULT_PASSWORD != NtlmPasswordAuthentication.BLANK) {
                    /*
                     * preauthentication
                     */
                    transport.getSmbSession(NtlmPasswordAuthentication.DEFAULT)
                            .getSmbTree(LOGON_SHARE, null).treeConnect(null, null);
                } else {
                    request.digest = new SigningDigest(transport, auth);
                }
            }

            request.auth = auth;

            try {
                transport.send(request, response);
            } catch (SmbAuthException sae) {
                throw sae;
            } catch (SmbException se) {
                ex = se;
            }

            if (response.isLoggedInAsGuest && "GUEST".equalsIgnoreCase(auth.username) == false) {
                throw new SmbAuthException(NtStatus.NT_STATUS_LOGON_FAILURE);
            }

            uid = response.uid;
            sessionSetup = true;

            if (request.digest != null) {
                /* success - install the signing digest */
                transport.digest = request.digest;
            }

            if (ex != null)
                throw ex;
        }
    }

    void logoff(boolean inError) {
        synchronized(transport()) {
            if (sessionSetup == false) {
                return;
            }

            for (Enumeration e = trees.elements(); e.hasMoreElements();) {
                SmbTree t = (SmbTree) e.nextElement();
                t.treeDisconnect(inError);
            }

            if (!inError && transport.server.security != ServerMessageBlock.SECURITY_SHARE) {

                /*
                 * Logoff And X Request / Response
                 */

                SmbComLogoffAndX request = new SmbComLogoffAndX(null);
                request.uid = uid;
                try {
                    transport.send(request, null);
                } catch (SmbException se) {
                }
            }

            sessionSetup = false;
        }
    }

    public String toString() {
        return "SmbSession[accountName=" + auth.username + ",primaryDomain=" + auth.domain
                + ",uid=" + uid + ",sessionSetup=" + sessionSetup + "]";
    }
}
