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

package android.hdmicec.cts.common;

import static com.google.common.truth.Truth.assertThat;

import android.hdmicec.cts.BaseHdmiCecCtsTest;
import android.hdmicec.cts.CecOperand;
import android.hdmicec.cts.HdmiCecConstants;
import android.hdmicec.cts.LogicalAddress;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * HDMI CEC test to verify the device handles standby correctly (Section 11.1.3, 11.2.3)
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class HdmiCecSystemStandbyTest extends BaseHdmiCecCtsTest {

    private static final String HDMI_CONTROL_DEVICE_AUTO_OFF =
            "hdmi_control_auto_device_off_enabled";

    public List<LogicalAddress> mLogicalAddresses = new ArrayList<>();
    public boolean wasOn;

    @Rule
    public RuleChain ruleChain =
            RuleChain
                    .outerRule(CecRules.requiresCec(this))
                    .around(CecRules.requiresLeanback(this))
                    .around(hdmiCecClient);

    @Before
    public void initialTestSetup() throws Exception {
        defineLogicalAddressList();
        wasOn = setHdmiControlDeviceAutoOff(false);
    }

    @After
    public void resetDutState() throws Exception {
        /* Wake up the device */
        getDevice().executeShellCommand("input keyevent KEYCODE_WAKEUP");
        setHdmiControlDeviceAutoOff(wasOn);
    }

    /**
     * Test 11.1.3-2, 11.2.3-2<br>
     * Tests that the device goes into standby when a {@code <STANDBY>} message is broadcast.
     */
    @Test
    public void cect_HandleBroadcastStandby() throws Exception {
        getDevice().reboot();
        TimeUnit.SECONDS.sleep(5);
        for (LogicalAddress source : mLogicalAddresses) {
            if (!hasLogicalAddress(source)) {
                hdmiCecClient.sendCecMessage(source, LogicalAddress.BROADCAST, CecOperand.STANDBY);
                checkDeviceAsleepAfterStandbySent();
            }
        }
    }

    /**
     * Test 11.1.3-3, 11.2.3-3<br>
     * Tests that the device goes into standby when a {@code <STANDBY>} message is sent to it.
     */
    @Test
    public void cect_HandleAddressedStandby() throws Exception {
        getDevice().reboot();
        for (LogicalAddress source : mLogicalAddresses) {
            if (!hasLogicalAddress(source)) {
                hdmiCecClient.sendCecMessage(source, CecOperand.STANDBY);
                checkDeviceAsleepAfterStandbySent();
            }
        }
    }

    /**
     * Test 11.2.3-4<br>
     * Tests that the device does not broadcast a {@code <STANDBY>} when going into standby mode.
     */
    @Test
    public void cect_11_2_3_4_NoBroadcastStandby() throws Exception {
        /*
         * CEC CTS does not specify for TV a no broadcast on standby test. On Android TVs, there is
         * a feature to turn off this standby broadcast and this test tests the same.
         */
        ITestDevice device = getDevice();
        device.executeShellCommand("input keyevent KEYCODE_SLEEP");
        hdmiCecClient.checkOutputDoesNotContainMessage(LogicalAddress.BROADCAST,
                CecOperand.STANDBY);
    }

    private void defineLogicalAddressList() throws Exception {
        /* TODO: b/174279917 Add LogicalAddress.BROADCAST to this list as well. */
        mLogicalAddresses.add(LogicalAddress.TV);
        mLogicalAddresses.add(LogicalAddress.RECORDER_1);
        mLogicalAddresses.add(LogicalAddress.TUNER_1);
        mLogicalAddresses.add(LogicalAddress.PLAYBACK_1);
        mLogicalAddresses.add(LogicalAddress.AUDIO_SYSTEM);

        if (hasDeviceType(HdmiCecConstants.CEC_DEVICE_TYPE_TV)) {
            //Add logical addresses 13, 14 only for TV panel tests.
            mLogicalAddresses.add(LogicalAddress.RESERVED_2);
            mLogicalAddresses.add(LogicalAddress.SPECIFIC_USE);
        }
    }

    private boolean setHdmiControlDeviceAutoOff(boolean turnOn) throws Exception {
        ITestDevice device = getDevice();
        String val = device.executeShellCommand("settings get global " +
                HDMI_CONTROL_DEVICE_AUTO_OFF).trim();
        String valToSet = turnOn ? "1" : "0";
        device.executeShellCommand("settings put global "
                + HDMI_CONTROL_DEVICE_AUTO_OFF + " " + valToSet);
        device.executeShellCommand("settings get global " + HDMI_CONTROL_DEVICE_AUTO_OFF);
        return val.equals("1");
    }

    private void checkDeviceAsleepAfterStandbySent() throws Exception {
        ITestDevice device = getDevice();
        TimeUnit.SECONDS.sleep(5);
        String wakeState = device.executeShellCommand("dumpsys power | grep mWakefulness=");
        assertThat(wakeState.trim()).isEqualTo("mWakefulness=Asleep");
        device.executeShellCommand("input keyevent KEYCODE_WAKEUP");
        TimeUnit.SECONDS.sleep(5);
    }
}
