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
import static org.junit.Assume.assumeTrue;

import android.hdmicec.cts.BaseHdmiCecCtsTest;
import android.hdmicec.cts.CecMessage;
import android.hdmicec.cts.CecOperand;
import android.hdmicec.cts.HdmiCecConstants;
import android.hdmicec.cts.LogicalAddress;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Rule;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;
import org.junit.Test;

/** HDMI CEC test to verify device vendor specific commands (Section 11.2.9) */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class HdmiCecVendorCommandsTest extends BaseHdmiCecCtsTest {

    private static final int INCORRECT_VENDOR_ID = 0x0;

    @Rule
    public RuleChain ruleChain =
        RuleChain
            .outerRule(CecRules.requiresCec(this))
            .around(CecRules.requiresLeanback(this))
            .around(hdmiCecClient);

    /**
     * Test 11.2.9-1
     * <p>Tests that the device responds to a {@code <GIVE_DEVICE_VENDOR_ID>} from various source
     * devices with a {@code <DEVICE_VENDOR_ID>}.
     */
    @Test
    public void cect_11_2_9_1_GiveDeviceVendorId() throws Exception {
        for (LogicalAddress logicalAddress : LogicalAddress.values()) {
            // Skip the logical address of this device
            if (hasLogicalAddress(logicalAddress)) {
                continue;
            }
            hdmiCecClient.sendCecMessage(logicalAddress, CecOperand.GIVE_DEVICE_VENDOR_ID);
            String message = hdmiCecClient.checkExpectedOutput(CecOperand.DEVICE_VENDOR_ID);
            assertThat(CecMessage.getParams(message)).isNotEqualTo(INCORRECT_VENDOR_ID);
        }
    }

    /**
     * Test 11.2.9-2
     * <p>Tests that the device broadcasts a {@code <DEVICE_VENDOR_ID>} message after successful
     * initialisation and address allocation.
     */
    @Test
    public void cect_11_2_9_2_DeviceVendorIdOnInit() throws Exception {
        ITestDevice device = getDevice();
        device.reboot();
        String message = hdmiCecClient.checkExpectedOutput(CecOperand.DEVICE_VENDOR_ID);
        assertThat(CecMessage.getParams(message)).isNotEqualTo(INCORRECT_VENDOR_ID);
    }
}
