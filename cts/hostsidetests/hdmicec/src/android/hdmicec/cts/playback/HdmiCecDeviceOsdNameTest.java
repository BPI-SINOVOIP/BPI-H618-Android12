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

/** HDMI CEC tests related to the device reporting the device OSD name (Section 11.2.11) */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class HdmiCecDeviceOsdNameTest extends BaseHdmiCecCtsTest {

    private static final LogicalAddress PLAYBACK_DEVICE = LogicalAddress.PLAYBACK_1;

    public HdmiCecDeviceOsdNameTest() {
        super(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    }

    @Rule
    public RuleChain ruleChain =
        RuleChain
            .outerRule(CecRules.requiresCec(this))
            .around(CecRules.requiresLeanback(this))
            .around(CecRules.requiresDeviceType(this, PLAYBACK_DEVICE))
            .around(hdmiCecClient);

    /**
     * Test 11.2.11-1a
     * Tests that the device responds to a <GIVE_OSD_NAME> with a <SET_OSD_NAME> that has the
     * correct device name in the parameters.
     */
    @Test
    public void cect_11_2_11_1a_GiveOsdNameTest() throws Exception {
        /* The params for <SET_OSD_NAME> only allow for 14 characters */
        final int nameLength = 14;
        ITestDevice device = getDevice();
        String deviceName = device.executeShellCommand("settings get global device_name").trim();
        if (deviceName.length() > nameLength) {
            deviceName = deviceName.substring(0, nameLength).trim();
        }
        hdmiCecClient.sendCecMessage(LogicalAddress.TV, CecOperand.GIVE_OSD_NAME);
        String message = hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.SET_OSD_NAME);
        assertThat(CecMessage.getAsciiString(message)).isEqualTo(deviceName);
    }

    /**
     * Test 11.2.11-1b
     * Test updates the device_name in global properties and checks that the device responds to a
     * <GIVE_OSD_NAME> with a <SET_OSD_NAME> that has the updated device name in the parameters.
     */
    @Test
    public void cect_11_2_11_1b_UpdateAndGiveOsdNameTest() throws Exception {
        final String testName = "test_name";
        ITestDevice device = getDevice();
        String originalName = device.executeShellCommand("settings get global device_name").trim();
        try {
            device.executeShellCommand("settings put global device_name '" + testName + "'");
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, CecOperand.GIVE_OSD_NAME);
            String message = hdmiCecClient.checkExpectedOutput(LogicalAddress.TV,
                    CecOperand.SET_OSD_NAME);
            assertThat(CecMessage.getAsciiString(message)).isEqualTo(testName);
        } finally {
            device.executeShellCommand("settings put global device_name '" + originalName + "'");
        }
    }

    /**
     * Test 11.2.11-2
     * Tests that the device does not respond to a <GIVE_OSD_NAME> from an unregistered device.
     */
    @Test
    public void cect_11_2_11_2_UnregisteredDeviceGiveOsdNameTest() throws Exception {
        hdmiCecClient.sendCecMessage(LogicalAddress.PLAYBACK_1, CecOperand.GIVE_OSD_NAME);
        hdmiCecClient.checkOutputDoesNotContainMessage(LogicalAddress.PLAYBACK_1,
                CecOperand.SET_OSD_NAME);
        hdmiCecClient.sendCecMessage(LogicalAddress.BROADCAST, CecOperand.GIVE_OSD_NAME);
        hdmiCecClient.checkOutputDoesNotContainMessage(LogicalAddress.BROADCAST,
                CecOperand.SET_OSD_NAME);
    }
}
