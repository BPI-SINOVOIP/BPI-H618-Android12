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
import android.hdmicec.cts.HdmiControlManagerUtility;
import android.hdmicec.cts.LogicalAddress;

import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import static com.google.common.truth.Truth.assertWithMessage;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/** HDMI CEC test to test One Touch Play features (Section 11.1.1) */
@RunWith(DeviceJUnit4ClassRunner.class)
public class HdmiCecTvOneTouchPlayTest extends BaseHdmiCecCtsTest {

    private static final LogicalAddress TV_DEVICE = LogicalAddress.TV;
    private static final int WAIT_TIME_MS = 300;
    List<LogicalAddress> testDevices = new ArrayList<>();

    public HdmiCecTvOneTouchPlayTest() {
        /* Start the client as recorder, tuner and playback devices */
        super(HdmiCecConstants.CEC_DEVICE_TYPE_TV, "-t", "r", "-t", "t", "-t", "p");
        testDevices.add(LogicalAddress.RECORDER_1);
        testDevices.add(LogicalAddress.TUNER_1);
        testDevices.add(LogicalAddress.PLAYBACK_1);
    }

    @Rule
    public RuleChain ruleChain =
            RuleChain.outerRule(CecRules.requiresCec(this))
                    .around(CecRules.requiresLeanback(this))
                    .around(CecRules.requiresDeviceType(this, TV_DEVICE))
                    .around(hdmiCecClient);

    /**
     * Test 11.1.1-1
     *
     * <p>Tests that the DUT responds to {@code <Image View On>} message correctly when the message
     * is sent from logical addresses 0x1, 0x3 and 0x4
     */
    @Test
    public void cect_11_1_1_1_RespondToImageViewOn() throws Exception {
        for (LogicalAddress testDevice : testDevices) {
            hdmiCecClient.sendCecMessage(testDevice, LogicalAddress.TV, CecOperand.IMAGE_VIEW_ON);
            TimeUnit.MILLISECONDS.sleep(WAIT_TIME_MS);
            hdmiCecClient.broadcastActiveSource(testDevice, hdmiCecClient.getPhysicalAddress());
            hdmiCecClient.checkOutputDoesNotContainMessage(testDevice, CecOperand.FEATURE_ABORT);
            assertWithMessage(
                            "Device has not registered expected logical address as active source.")
                    .that(getDumpsysActiveSourceLogicalAddress())
                    .isEqualTo(testDevice);
        }
    }

    /**
     * Test 11.1.1-2
     *
     * <p>Tests that the DUT responds to {@code <Text View On>} message correctly when the message
     * is sent from logical addresses 0x1, 0x3 and 0x4
     */
    @Test
    public void cect_11_1_1_2_RespondToTextViewOn() throws Exception {
        for (LogicalAddress testDevice : testDevices) {
            hdmiCecClient.sendCecMessage(testDevice, LogicalAddress.TV, CecOperand.TEXT_VIEW_ON);
            TimeUnit.MILLISECONDS.sleep(WAIT_TIME_MS);
            hdmiCecClient.broadcastActiveSource(testDevice, hdmiCecClient.getPhysicalAddress());
            hdmiCecClient.checkOutputDoesNotContainMessage(testDevice, CecOperand.FEATURE_ABORT);
            assertWithMessage(
                            "Device has not registered expected logical address as active source.")
                    .that(getDumpsysActiveSourceLogicalAddress())
                    .isEqualTo(testDevice);
        }
    }

    /**
     * Test 11.1.1-5
     *
     * <p>Tests that the DUT broadcasts an {@code <Active Source>} message when changing to an
     * internal source from previously displaying an external source.
     */
    @Test
    public void cect_11_1_1_5_DutBroadcastsActiveSourceWhenChangingToInternal() throws Exception {
        // Ensure that an external source is the active source.
        try {
            /*
             * Check for the broadcasted <ACTIVE_SOURCE> message from Recorder_1, which was sent as
             * a response to <SET_STREAM_PATH> message from the TV.
             */
            String message =
                    hdmiCecClient.checkExpectedMessageFromClient(
                            LogicalAddress.RECORDER_1, CecOperand.ACTIVE_SOURCE);
        } catch (Exception e) {
            /*
             * In case the TV does not send <Set Stream Path> to CEC adapter, or the client does
             * not make recorder active source, broadcast an <Active Source> message from the
             * adapter.
             */
            hdmiCecClient.broadcastActiveSource(
                    LogicalAddress.RECORDER_1, hdmiCecClient.getPhysicalAddress());
        }
        // Make the TV device the active source.
        HdmiControlManagerUtility.setActiveSource(
                getDevice(), LogicalAddress.TV.getLogicalAddressAsInt());
        hdmiCecClient.checkExpectedOutput(LogicalAddress.BROADCAST, CecOperand.ACTIVE_SOURCE);
    }
}
