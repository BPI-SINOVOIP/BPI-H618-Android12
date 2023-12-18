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

package android.hdmicec.cts.playback;

import static com.google.common.truth.Truth.assertThat;

import android.hdmicec.cts.BaseHdmiCecCtsTest;
import android.hdmicec.cts.CecMessage;
import android.hdmicec.cts.CecOperand;
import android.hdmicec.cts.HdmiCecClientWrapper;
import android.hdmicec.cts.HdmiCecConstants;
import android.hdmicec.cts.LogicalAddress;
import android.hdmicec.cts.RequiredPropertyRule;
import android.hdmicec.cts.RequiredFeatureRule;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Rule;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;
import org.junit.Test;

import static android.hdmicec.cts.HdmiCecConstants.TIMEOUT_SAFETY_MS;

import java.util.concurrent.TimeUnit;

/**
 * HDMI CEC test to verify TV power toggle behavior
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class HdmiCecTvPowerToggleTest extends BaseHdmiCecCtsTest {

    private static final int ON = 0x0;
    private static final int OFF = 0x1;

    private static final LogicalAddress PLAYBACK_DEVICE = LogicalAddress.PLAYBACK_1;
    private static final String POWER_CONTROL_MODE =
            "hdmi_control_send_standby_on_sleep";
    @Rule
    public RuleChain ruleChain =
            RuleChain
                    .outerRule(CecRules.requiresCec(this))
                    .around(CecRules.requiresLeanback(this))
                    .around(CecRules.requiresDeviceType(this, LogicalAddress.PLAYBACK_1))
                    .around(hdmiCecClient);

    public HdmiCecTvPowerToggleTest() {
        super(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    }

    private String setPowerControlMode(String valToSet) throws Exception {
        ITestDevice device = getDevice();
        String val = device.executeShellCommand("settings get global " +
                POWER_CONTROL_MODE).trim();
        device.executeShellCommand("settings put global "
                + POWER_CONTROL_MODE + " " + valToSet);
        return val;
    }

    /**
     * Tests that KEYCODE_TV_POWER functions as a TV power toggle.
     * Device is awake and not active source. TV is on.
     */
    @Test
    public void cectTvPowerToggleTest_awake_noActiveSource_tvOn() throws Exception {
        ITestDevice device = getDevice();
        // Make sure the device is not booting up/in standby
        device.waitForBootComplete(HdmiCecConstants.REBOOT_TIMEOUT);
        String previousPowerControlMode = setPowerControlMode("to_tv");
        try {
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 0");
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 1");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(OFF));
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, LogicalAddress.BROADCAST,
                    CecOperand.ACTIVE_SOURCE, CecMessage.formatParams("0000"));
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            hdmiCecClient.clearClientOutput();
            device.executeShellCommand("input keyevent KEYCODE_TV_POWER");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(ON));
            // Verify that device is asleep and <Standby> was sent to TV.
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.STANDBY);
            String wakeState = device.executeShellCommand("dumpsys power | grep mWakefulness=");
            assertThat(wakeState.trim()).isEqualTo("mWakefulness=Asleep");
        } finally {
            setPowerControlMode(previousPowerControlMode);
        }
    }

    /**
     * Tests that KEYCODE_TV_POWER functions as a TV power toggle.
     * Device is awake and active source. TV is on.
     */
    @Test
    public void cectTvPowerToggleTest_awake_activeSource_tvOn() throws Exception {
        ITestDevice device = getDevice();
        // Make sure the device is not booting up/in standby
        device.waitForBootComplete(HdmiCecConstants.REBOOT_TIMEOUT);
        String previousPowerControlMode = setPowerControlMode("to_tv");
        try {
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 0");
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 1");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(OFF));
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            device.executeShellCommand("input keyevent KEYCODE_HOME");
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            hdmiCecClient.clearClientOutput();
            device.executeShellCommand("input keyevent KEYCODE_TV_POWER");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(ON));
            // Verify that device is asleep and <Standby> was sent to TV.
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.STANDBY);
            String wakeState = device.executeShellCommand("dumpsys power | grep mWakefulness=");
            assertThat(wakeState.trim()).isEqualTo("mWakefulness=Asleep");
        } finally {
            setPowerControlMode(previousPowerControlMode);
        }
    }

    /**
     * Tests that KEYCODE_TV_POWER functions as a TV power toggle.
     * Device is asleep. TV is on.
     */
    @Test
    public void cectTvPowerToggleTest_asleep_tvOn() throws Exception {
        ITestDevice device = getDevice();
        // Make sure the device is not booting up/in standby
        device.waitForBootComplete(HdmiCecConstants.REBOOT_TIMEOUT);
        String previousPowerControlMode = setPowerControlMode("to_tv");
        try {
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 0");
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 1");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(OFF));
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            device.executeShellCommand("input keyevent KEYCODE_SLEEP");
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            hdmiCecClient.clearClientOutput();
            device.executeShellCommand("input keyevent KEYCODE_TV_POWER");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(ON));
            // Verify that device is asleep and <Standby> was sent to TV.
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.STANDBY);
            String wakeState = device.executeShellCommand("dumpsys power | grep mWakefulness=");
            assertThat(wakeState.trim()).isEqualTo("mWakefulness=Asleep");
        } finally {
            setPowerControlMode(previousPowerControlMode);
        }
    }

    /**
     * Tests that KEYCODE_TV_POWER functions as a TV power toggle.
     * Device is asleep. TV is off.
     */
    @Test
    public void cectTvPowerToggleTest_asleep_tvOff() throws Exception {
        ITestDevice device = getDevice();
        // Make sure the device is not booting up/in standby
        device.waitForBootComplete(HdmiCecConstants.REBOOT_TIMEOUT);
        String previousPowerControlMode = setPowerControlMode("to_tv");
        try {
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 0");
            device.executeShellCommand("cmd hdmi_control cec_setting set hdmi_cec_enabled 1");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(OFF));
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            device.executeShellCommand("input keyevent KEYCODE_SLEEP");
            TimeUnit.SECONDS.sleep(HdmiCecConstants.DEVICE_WAIT_TIME_SECONDS);
            hdmiCecClient.clearClientOutput();
            device.executeShellCommand("input keyevent KEYCODE_TV_POWER");
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.GIVE_POWER_STATUS);
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, PLAYBACK_DEVICE,
                    CecOperand.REPORT_POWER_STATUS, CecMessage.formatParams(OFF));
            // Verify that device is awake and <Text View On> and <Active Source> were sent.
            hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.TEXT_VIEW_ON);
            hdmiCecClient.checkExpectedOutput(LogicalAddress.BROADCAST, CecOperand.ACTIVE_SOURCE);
            String wakeState = device.executeShellCommand("dumpsys power | grep mWakefulness=");
            assertThat(wakeState.trim()).isEqualTo("mWakefulness=Awake");
        } finally {
            setPowerControlMode(previousPowerControlMode);
        }
    }
}