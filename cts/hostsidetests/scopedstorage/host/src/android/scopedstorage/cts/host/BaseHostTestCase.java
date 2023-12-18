/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.scopedstorage.cts.host;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.NativeDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;


abstract class BaseHostTestCase extends BaseHostJUnit4Test {
    private int mCurrentUserId = NativeDevice.INVALID_USER_ID;
    private static final String ERROR_MESSAGE_TAG = "[ERROR]";

    protected String executeShellCommand(String cmd, Object... args) throws Exception {
        return getDevice().executeShellCommand(String.format(cmd, args));
    }

    protected CommandResult executeShellV2Command(String cmd, Object... args) throws Exception {
        return getDevice().executeShellV2Command(String.format(cmd, args));
    }

    protected boolean isPackageInstalled(String packageName, String userId) throws Exception {
        return getDevice().isPackageInstalled(packageName, userId);
    }

    // TODO (b/174775905) remove after exposing the check from ITestDevice.
    protected boolean isHeadlessSystemUserMode() throws DeviceNotAvailableException {
        String result = getDevice()
                .executeShellCommand("getprop ro.fw.mu.headless_system_user").trim();
        return "true".equalsIgnoreCase(result);
    }

    protected boolean isAtLeastS() throws DeviceNotAvailableException {
        return getDevice().getApiLevel() >= 31 /* BUILD.VERSION_CODES.S */;
    }

    protected static void eventually(ThrowingRunnable r, long timeoutMillis) {
        long start = System.currentTimeMillis();

        while (true) {
            try {
                r.run();
                return;
            } catch (Throwable e) {
                if (System.currentTimeMillis() - start < timeoutMillis) {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException ignored) {
                        throw new RuntimeException(e);
                    }
                } else {
                    throw new RuntimeException(e);
                }
            }
        }
    }

    protected int getCurrentUserId() throws Exception {
        setCurrentUserId();

        return mCurrentUserId;
    }

    protected boolean isSuccessful(CommandResult result) {
        if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
            return false;
        }
        String stdout = result.getStdout();
        if (stdout.contains(ERROR_MESSAGE_TAG)) {
            return false;
        }
        String stderr = result.getStderr();
        return (stderr == null || stderr.trim().isEmpty());
    }

    private void setCurrentUserId() throws Exception {
        if (mCurrentUserId != NativeDevice.INVALID_USER_ID) return;

        ITestDevice device = getDevice();
        mCurrentUserId = device.getCurrentUser();
        CLog.i("Current user: %d");
    }

    protected interface ThrowingRunnable {
        /**
         * Similar to {@link Runnable#run} but has {@code throws Exception}.
         */
        void run() throws Exception;
    }
}
