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

package android.hdmicec.cts.tv;

import android.hdmicec.cts.BaseHdmiCecCtsTest;
import android.hdmicec.cts.CecOperand;
import android.hdmicec.cts.HdmiCecConstants;
import android.hdmicec.cts.LogicalAddress;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;


import static com.google.common.truth.Truth.assertWithMessage;

/** HDMI CEC tests for system standby features (Section 11.1.3) */
@RunWith(DeviceJUnit4ClassRunner.class)
public class HdmiCecTvStandbyTest extends BaseHdmiCecCtsTest {

    private static final LogicalAddress TV_DEVICE = LogicalAddress.TV;

    public HdmiCecTvStandbyTest() {
        super(HdmiCecConstants.CEC_DEVICE_TYPE_TV);
    }

    @Rule
    public RuleChain ruleChain =
            RuleChain.outerRule(CecRules.requiresCec(this))
                    .around(CecRules.requiresLeanback(this))
                    .around(CecRules.requiresDeviceType(this, TV_DEVICE))
                    .around(hdmiCecClient);

    private static final String HDMI_CONTROL_DEVICE_AUTO_OFF =
            "hdmi_control_auto_device_off_enabled";

    /**
     * Test 11.1.3-1
     *
     * <p>Tests that the DUT broadcasts a {@code <Standby>} message correctly and goes into standby
     * when standby is initiated.
     */
    @Test
    public void cect_11_1_3_1_BroadcastStandby() throws Exception {
        ITestDevice device = getDevice();
        device.waitForBootComplete(HdmiCecConstants.REBOOT_TIMEOUT);
        boolean wasOn = setHdmiControlDeviceAutoOff(true);
        try {
            device.executeShellCommand("input keyevent KEYCODE_SLEEP");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.BROADCAST, CecOperand.STANDBY);
            String wakeState = device.executeShellCommand("dumpsys power | grep mWakefulness=");
            assertWithMessage("Device is not in standby.")
                    .that(wakeState.trim())
                    .isEqualTo("mWakefulness=Asleep");
        } finally {
            device.executeShellCommand("input keyevent KEYCODE_WAKEUP");
            setHdmiControlDeviceAutoOff(wasOn);
        }
    }

    private boolean setHdmiControlDeviceAutoOff(boolean turnOn) throws Exception {
        ITestDevice device = getDevice();
        String val =
                device.executeShellCommand("settings get global " + HDMI_CONTROL_DEVICE_AUTO_OFF)
                        .trim();
        String valToSet = turnOn ? "1" : "0";
        device.executeShellCommand(
                "settings put global " + HDMI_CONTROL_DEVICE_AUTO_OFF + " " + valToSet);
        device.executeShellCommand("settings get global " + HDMI_CONTROL_DEVICE_AUTO_OFF);
        return val.equals("1");
    }
}
