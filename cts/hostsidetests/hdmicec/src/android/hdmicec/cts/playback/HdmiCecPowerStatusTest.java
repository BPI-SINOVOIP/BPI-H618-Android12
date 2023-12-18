/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hdmicec.cts.playback;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.hdmicec.cts.BaseHdmiCecCtsTest;
import android.hdmicec.cts.CecMessage;
import android.hdmicec.cts.CecOperand;
import android.hdmicec.cts.HdmiCecConstants;
import android.hdmicec.cts.LogicalAddress;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;

/**
 * HDMI CEC tests verifying power status related messages of the device (CEC 2.0 CTS Section 7.6)
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class HdmiCecPowerStatusTest extends BaseHdmiCecCtsTest {

    public HdmiCecPowerStatusTest() {
        super(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    }

    @Rule
    public RuleChain ruleChain =
            RuleChain
                    .outerRule(CecRules.requiresCec(this))
                    .around(CecRules.requiresLeanback(this))
                    .around(CecRules.requiresDeviceType(this, LogicalAddress.PLAYBACK_1))
                    .around(hdmiCecClient);

    /**
     * Test HF4-6-7
     * Same as Test HF4-6-9
     *
     * Tests that a device comes out of the Standby state when it receives a {@code <Set Stream
     * Path>} message with its Physical Address as operand.
     *
     * Only applies if the DUT has Primary Device "Playback Device", "Recording Device", or "Tuner".
     */
    @Test
    public void cect_hf4_6_7_setStreamPath_powerOn() throws Exception {
        ITestDevice device = getDevice();

        try {
            device.executeShellCommand("input keyevent KEYCODE_SLEEP");

            TimeUnit.SECONDS.sleep(HdmiCecConstants.MAX_SLEEP_TIME_SECONDS);

            String wakeStateBefore = device.executeShellCommand(
                    "dumpsys power | grep mWakefulness=");
            assertThat(wakeStateBefore.trim()).isEqualTo("mWakefulness=Asleep");

            hdmiCecClient.sendCecMessage(
                    LogicalAddress.TV,
                    LogicalAddress.BROADCAST,
                    CecOperand.SET_STREAM_PATH,
                    CecMessage.formatParams(getDumpsysPhysicalAddress(),
                            HdmiCecConstants.PHYSICAL_ADDRESS_LENGTH));

            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            String wakeStateAfter = device.executeShellCommand(
                    "dumpsys power | grep mWakefulness=");
            assertWithMessage("Device should wake up on <Set Stream Path>")
                    .that(wakeStateAfter.trim()).isEqualTo("mWakefulness=Awake");
        } finally {
            device.executeShellCommand("input keyevent KEYCODE_WAKEUP");
        }
    }
}