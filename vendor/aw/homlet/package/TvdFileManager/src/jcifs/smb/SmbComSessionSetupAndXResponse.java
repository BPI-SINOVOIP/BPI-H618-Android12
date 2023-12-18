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

import java.io.UnsupportedEncodingException;

class SmbComSessionSetupAndXResponse extends AndXServerMessageBlock {

    private String nativeOs = "";
    private String nativeLanMan = "";
    private String primaryDomain = "";

    boolean isLoggedInAsGuest;

    SmbComSessionSetupAndXResponse(ServerMessageBlock andx) {
        super(andx);
    }

    int writeParameterWordsWireFormat(byte[] dst, int dstIndex) {
        return 0;
    }

    int writeBytesWireFormat(byte[] dst, int dstIndex) {
        return 0;
    }

    int readParameterWordsWireFormat(byte[] buffer, int bufferIndex) {
        isLoggedInAsGuest = (buffer[bufferIndex] & 0x01) == 0x01 ? true : false;
        return 2;
    }

    int readBytesWireFormat(byte[] buffer, int bufferIndex) {
        int start = bufferIndex;

        nativeOs = readString(buffer, bufferIndex);
        bufferIndex += stringWireLength(nativeOs, bufferIndex);
        nativeLanMan = readString(buffer, bufferIndex);
        bufferIndex += stringWireLength(nativeLanMan, bufferIndex);

        if (useUnicode) {
            int len;

            if (((bufferIndex - headerStart) % 2) != 0) {
                bufferIndex++;
            }

            len = 0;
            while (buffer[bufferIndex + len] != (byte) 0x00) {
                len += 2;
                if (len > 256) {
                    throw new RuntimeException("zero termination not found");
                }
            }
            try {
                primaryDomain = new String(buffer, bufferIndex, len, "UnicodeLittle");
            } catch (UnsupportedEncodingException uee) {
                if (log.level > 1)
                    uee.printStackTrace(log);
            }
            bufferIndex += len;
        } else {
            primaryDomain = readString(buffer, bufferIndex);
            bufferIndex += stringWireLength(primaryDomain, bufferIndex);
        }

        return bufferIndex - start;
    }

    public String toString() {
        String result = new String("SmbComSessionSetupAndXResponse[" + super.toString()
                + ",isLoggedInAsGuest=" + isLoggedInAsGuest + ",nativeOs=" + nativeOs
                + ",nativeLanMan=" + nativeLanMan + ",primaryDomain=" + primaryDomain + "]");
        return result;
    }
}
