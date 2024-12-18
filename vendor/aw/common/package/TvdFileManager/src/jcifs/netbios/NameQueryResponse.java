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

package jcifs.netbios;

import java.util.ArrayList;

import jcifs.util.LocalNetwork;

class NameQueryResponse extends NameServicePacket {

    NameQueryResponse() {
        recordName = new Name();
    }

    int writeBodyWireFormat(byte[] dst, int dstIndex) {
        return 0;
    }

    int readBodyWireFormat(byte[] src, int srcIndex) {
        return readResourceRecordWireFormat(src, srcIndex);
    }

    int writeRDataWireFormat(byte[] dst, int dstIndex) {
        return 0;
    }

    int readRDataWireFormat(byte[] src, int srcIndex) {
        if (resultCode != 0 || opCode != QUERY) {
            return 0;
        }
        boolean groupName = ((src[srcIndex] & 0x80) == 0x80) ? true : false;
        int nodeType = (src[srcIndex] & 0x60) >> 5;
        srcIndex += 2;
        int address = readInt4(src, srcIndex);
        if (address != 0) {
            // a host may has more than one ip address,so we must check
            // them.Because the client can only
            // connect to the ip in same mask.
            int curIp = LocalNetwork.getCurIpv4();
            int mask = LocalNetwork.getCurNetMask();
            boolean isSame = LocalNetwork.sameNetSegment(curIp, address, mask);
            if (isSame) {
                addrEntry[addrIndex] = new NbtAddress(recordName, address, groupName, nodeType);
            } else {
                addrEntry[addrIndex] = null;
            }
        } else {
            addrEntry[addrIndex] = null;
        }

        return 6;
    }

    @Override
    int readResourceRecordWireFormat(byte[] src, int srcIndex) {
        int ret = super.readResourceRecordWireFormat(src, srcIndex);
        // remove unavailable addrEntries in record data.
        addrIndex = 0;
        for (addrIndex = 0; addrIndex < addrEntry.length; addrIndex++) {
            if (addrEntry[addrIndex] != null) {
                tmp.add(addrEntry[addrIndex]);
            }
        }
        addrEntry = new NbtAddress[tmp.size()];
        for (addrIndex = 0; addrIndex < tmp.size(); addrIndex++) {
            addrEntry[addrIndex] = tmp.get(addrIndex);
        }
        return ret;
    }

    public String toString() {
        return new String("NameQueryResponse[" + super.toString() + ",addrEntry=" + addrEntry + "]");
    }
}
