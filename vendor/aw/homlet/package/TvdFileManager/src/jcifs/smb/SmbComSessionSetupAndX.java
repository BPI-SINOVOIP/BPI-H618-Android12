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

import java.util.Arrays;
import jcifs.Config;

class SmbComSessionSetupAndX extends AndXServerMessageBlock {

    private static final int BATCH_LIMIT = Config.getInt(
            "jcifs.smb.client.SessionSetupAndX.TreeConnectAndX", 1);
    private static final boolean DISABLE_PLAIN_TEXT_PASSWORDS = Config.getBoolean(
            "jcifs.smb.client.disablePlainTextPasswords", true);

    private byte[] accountPassword, unicodePassword;
    private int passwordLength, unicodePasswordLength;
    private int sessionKey;
    private String accountName, primaryDomain;

    SmbSession session;
    NtlmPasswordAuthentication auth;

    SmbComSessionSetupAndX(SmbSession session, ServerMessageBlock andx) throws SmbException {
        super(andx);
        command = SMB_COM_SESSION_SETUP_ANDX;
        this.session = session;
        this.auth = session.auth;
        if (auth.hashesExternal
                && Arrays.equals(auth.challenge, session.transport.server.encryptionKey) == false) {
            throw new SmbAuthException(SmbException.NT_STATUS_ACCESS_VIOLATION);
        }
    }

    int getBatchLimit(byte command) {
        return command == SMB_COM_TREE_CONNECT_ANDX ? BATCH_LIMIT : 0;
    }

    int writeParameterWordsWireFormat(byte[] dst, int dstIndex) {
        int start = dstIndex;

        if (session.transport.server.security == SECURITY_USER
                && (auth.hashesExternal || auth.password.length() > 0)) {
            if (session.transport.server.encryptedPasswords) {
                accountPassword = auth.getAnsiHash(session.transport.server.encryptionKey);
                passwordLength = accountPassword.length;
                unicodePassword = auth.getUnicodeHash(session.transport.server.encryptionKey);
                unicodePasswordLength = unicodePassword.length;
                // prohibit HTTP auth attempts for the null session
                if (unicodePasswordLength == 0 && passwordLength == 0) {
                    throw new RuntimeException("Null setup prohibited.");
                }
            } else if (DISABLE_PLAIN_TEXT_PASSWORDS) {
                throw new RuntimeException("Plain text passwords are disabled");
            } else if (useUnicode) {
                // plain text
                String password = auth.getPassword();
                accountPassword = new byte[0];
                passwordLength = 0;
                unicodePassword = new byte[(password.length() + 1) * 2];
                unicodePasswordLength = writeString(password, unicodePassword, 0);
            } else {
                // plain text
                String password = auth.getPassword();
                accountPassword = new byte[(password.length() + 1) * 2];
                passwordLength = writeString(password, accountPassword, 0);
                unicodePassword = new byte[0];
                unicodePasswordLength = 0;
            }
        } else {
            // no password in session setup
            passwordLength = unicodePasswordLength = 0;
        }

        sessionKey = session.transport.sessionKey;

        writeInt2(session.transport.snd_buf_size, dst, dstIndex);
        dstIndex += 2;
        writeInt2(session.transport.maxMpxCount, dst, dstIndex);
        dstIndex += 2;
        writeInt2(session.transport.VC_NUMBER, dst, dstIndex);
        dstIndex += 2;
        writeInt4(sessionKey, dst, dstIndex);
        dstIndex += 4;
        writeInt2(passwordLength, dst, dstIndex);
        dstIndex += 2;
        writeInt2(unicodePasswordLength, dst, dstIndex);
        dstIndex += 2;
        dst[dstIndex++] = (byte) 0x00;
        dst[dstIndex++] = (byte) 0x00;
        dst[dstIndex++] = (byte) 0x00;
        dst[dstIndex++] = (byte) 0x00;
        writeInt4(session.transport.capabilities, dst, dstIndex);
        dstIndex += 4;

        return dstIndex - start;
    }

    int writeBytesWireFormat(byte[] dst, int dstIndex) {
        int start = dstIndex;

        accountName = useUnicode ? auth.username : auth.username.toUpperCase();
        primaryDomain = auth.domain.toUpperCase();

        if (session.transport.server.security == SECURITY_USER
                && (auth.hashesExternal || auth.password.length() > 0)) {
            System.arraycopy(accountPassword, 0, dst, dstIndex, passwordLength);
            dstIndex += passwordLength;
            if (session.transport.server.encryptedPasswords == false && useUnicode) {
                /*
                 * Align Unicode plain text password manually
                 */
                if (((dstIndex - headerStart) % 2) != 0) {
                    dst[dstIndex++] = (byte) '\0';
                }
            }
            System.arraycopy(unicodePassword, 0, dst, dstIndex, unicodePasswordLength);
            dstIndex += unicodePasswordLength;
        }

        dstIndex += writeString(accountName, dst, dstIndex);
        dstIndex += writeString(primaryDomain, dst, dstIndex);
        dstIndex += writeString(session.transport.NATIVE_OS, dst, dstIndex);
        dstIndex += writeString(session.transport.NATIVE_LANMAN, dst, dstIndex);

        return dstIndex - start;
    }

    int readParameterWordsWireFormat(byte[] buffer, int bufferIndex) {
        return 0;
    }

    int readBytesWireFormat(byte[] buffer, int bufferIndex) {
        return 0;
    }

    public String toString() {
        String result = new String("SmbComSessionSetupAndX[" + super.toString() + ",snd_buf_size="
                + session.transport.snd_buf_size + ",maxMpxCount=" + session.transport.maxMpxCount
                + ",VC_NUMBER=" + session.transport.VC_NUMBER + ",sessionKey=" + sessionKey
                + ",passwordLength=" + passwordLength + ",unicodePasswordLength="
                + unicodePasswordLength + ",capabilities=" + session.transport.capabilities
                + ",accountName=" + accountName + ",primaryDomain=" + primaryDomain + ",NATIVE_OS="
                + session.transport.NATIVE_OS + ",NATIVE_LANMAN=" + session.transport.NATIVE_LANMAN
                + "]");
        return result;
    }
}
