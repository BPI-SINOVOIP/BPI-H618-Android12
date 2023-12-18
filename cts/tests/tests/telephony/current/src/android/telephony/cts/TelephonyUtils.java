/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.telephony.cts;

import android.app.Instrumentation;
import android.os.ParcelFileDescriptor;
import android.telecom.TelecomManager;
import android.telephony.TelephonyManager;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.util.function.BooleanSupplier;

public class TelephonyUtils {

    /**
     * See {@link TelecomManager#ENABLE_GET_CALL_STATE_PERMISSION_PROTECTION}
     */
    public static final String ENABLE_GET_CALL_STATE_PERMISSION_PROTECTION_STRING =
            "ENABLE_GET_CALL_STATE_PERMISSION_PROTECTION";

    private static final String COMMAND_ADD_TEST_EMERGENCY_NUMBER =
            "cmd phone emergency-number-test-mode -a ";

    private static final String COMMAND_REMOVE_TEST_EMERGENCY_NUMBER =
            "cmd phone emergency-number-test-mode -r ";

    private static final String COMMAND_END_BLOCK_SUPPRESSION = "cmd phone end-block-suppression";

    private static final String COMMAND_FLUSH_TELEPHONY_METRICS =
            "/system/bin/dumpsys activity service TelephonyDebugService --metricsproto";

    private static final String COMMAND_AM_COMPAT = "am compat ";

    public static final String CTS_APP_PACKAGE = "android.telephony.cts";
    public static final String CTS_APP_PACKAGE2 = "android.telephony2.cts";

    private static final char[] HEX_DIGITS = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'A', 'B', 'C', 'D', 'E', 'F' };

    public static void addTestEmergencyNumber(Instrumentation instr, String testNumber)
            throws Exception {
        executeShellCommand(instr, COMMAND_ADD_TEST_EMERGENCY_NUMBER + testNumber);
    }

    public static void removeTestEmergencyNumber(Instrumentation instr, String testNumber)
            throws Exception {
        executeShellCommand(instr, COMMAND_REMOVE_TEST_EMERGENCY_NUMBER + testNumber);
    }

    public static void endBlockSuppression(Instrumentation instr) throws Exception {
        executeShellCommand(instr, COMMAND_END_BLOCK_SUPPRESSION);
    }

    public static void flushTelephonyMetrics(Instrumentation instr) throws Exception {
        executeShellCommand(instr, COMMAND_FLUSH_TELEPHONY_METRICS);
    }

    public static void enableCompatCommand(Instrumentation instr, String pkgName,
            String commandName) throws Exception {
        executeShellCommand(instr, COMMAND_AM_COMPAT + "enable  --no-kill " + commandName + " "
                + pkgName);
    }

    public static void disableCompatCommand(Instrumentation instr, String pkgName,
            String commandName) throws Exception {
        executeShellCommand(instr, COMMAND_AM_COMPAT + "disable  --no-kill " + commandName + " "
                + pkgName);
    }

    public static void resetCompatCommand(Instrumentation instr, String pkgName,
            String commandName) throws Exception {
        executeShellCommand(instr, COMMAND_AM_COMPAT + "reset  --no-kill " + commandName + " "
                + pkgName);
    }

    public static boolean isSkt(TelephonyManager telephonyManager) {
        return isOperator(telephonyManager, "45005");
    }

    public static boolean isKt(TelephonyManager telephonyManager) {
        return isOperator(telephonyManager, "45002")
                || isOperator(telephonyManager, "45004")
                || isOperator(telephonyManager, "45008");
    }

    private static boolean isOperator(TelephonyManager telephonyManager, String operator) {
        String simOperator = telephonyManager.getSimOperator();
        return simOperator != null && simOperator.equals(operator);
    }

    public static String parseErrorCodeToString(int errorCode,
            Class<?> containingClass, String prefix) {
        for (Field field : containingClass.getDeclaredFields()) {
            if (field.getName().startsWith(prefix)) {
                if (field.getType() == Integer.TYPE) {
                    field.setAccessible(true);
                    try {
                        if (field.getInt(null) == errorCode) {
                            return field.getName();
                        }
                    } catch (IllegalAccessException e) {
                        continue;
                    }
                }
            }
        }
        return String.format("??%d??", errorCode);
    }

    /**
     * Executes the given shell command and returns the output in a string. Note that even
     * if we don't care about the output, we have to read the stream completely to make the
     * command execute.
     */
    public static String executeShellCommand(Instrumentation instrumentation,
            String command) throws Exception {
        final ParcelFileDescriptor pfd =
                instrumentation.getUiAutomation().executeShellCommand(command);
        BufferedReader br = null;
        try (InputStream in = new FileInputStream(pfd.getFileDescriptor())) {
            br = new BufferedReader(new InputStreamReader(in, StandardCharsets.UTF_8));
            String str = null;
            StringBuilder out = new StringBuilder();
            while ((str = br.readLine()) != null) {
                out.append(str);
            }
            return out.toString();
        } finally {
            if (br != null) {
                closeQuietly(br);
            }
            closeQuietly(pfd);
        }
    }

    private static void closeQuietly(AutoCloseable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (RuntimeException rethrown) {
                throw rethrown;
            } catch (Exception ignored) {
            }
        }
    }

    public static boolean pollUntilTrue(BooleanSupplier s, int times, int timeoutMs) {
        boolean successful = false;
        for (int i = 0; i < times; i++) {
            successful = s.getAsBoolean();
            if (successful) break;
            try {
                Thread.sleep(timeoutMs);
            } catch (InterruptedException e) { }
        }
        return successful;
    }

    public static String toHexString(byte[] array) {
        int length = array.length;
        char[] buf = new char[length * 2];

        int bufIndex = 0;
        for (byte b : array) {
            buf[bufIndex++] = HEX_DIGITS[(b >>> 4) & 0x0F];
            buf[bufIndex++] = HEX_DIGITS[b & 0x0F];
        }

        return new String(buf);
    }

    private static int toByte(char c) {
        if (c >= '0' && c <= '9') return (c - '0');
        if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
        if (c >= 'a' && c <= 'f') return (c - 'a' + 10);

        throw new RuntimeException("Invalid hex char '" + c + "'");
    }

    public static byte[] hexStringToByteArray(String hexString) {
        int length = hexString.length();
        byte[] buffer = new byte[length / 2];

        for (int i = 0; i < length; i += 2) {
            buffer[i / 2] =
                    (byte) ((toByte(hexString.charAt(i)) << 4) | toByte(hexString.charAt(i + 1)));
        }

        return buffer;
    }
}
