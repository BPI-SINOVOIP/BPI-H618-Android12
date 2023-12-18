/*
 * Copyright 2020 The Android Open Source Project
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

package android.hdmicec.cts;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/** Helper class to call intents in the HdmiCecControlManagerHelper app */
public class HdmiControlManagerUtility {
    /** The package name of the APK. */
    private static final String PACKAGE = "android.hdmicec.app";

    /** The class name of the main activity in the APK. */
    private static final String CLASS = "HdmiControlManagerHelper";

    /** The command to launch the main activity. */
    private static final String START_COMMAND =
            String.format("am start -n %s/%s.%s -a ", PACKAGE, PACKAGE, CLASS);

    /** The command to clear the main activity. */
    private static final String CLEAR_COMMAND = String.format("pm clear %s", PACKAGE);

    /**
     * Method to make a device the active source. Will only work if the DUT is TV.
     *
     * @param device Reference to the DUT
     * @param logicalAddress The logical address of the device that should be made the active source
     */
    public static void setActiveSource(ITestDevice device, int logicalAddress)
            throws DeviceNotAvailableException {
        // Clear activity
        device.executeShellCommand(CLEAR_COMMAND);
        // Start the APK and wait for it to complete.
        device.executeShellCommand(
                START_COMMAND
                        + "android.hdmicec.app.DEVICE_SELECT --ei "
                        + "\"la\" "
                        + logicalAddress);
    }
}
