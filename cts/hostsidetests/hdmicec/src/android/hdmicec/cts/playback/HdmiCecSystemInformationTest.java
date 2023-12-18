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

import static org.junit.Assume.assumeTrue;

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

/** HDMI CEC system information tests (Section 11.2.6) */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class HdmiCecSystemInformationTest extends BaseHdmiCecCtsTest {

    @Rule
    public RuleChain ruleChain =
            RuleChain
                    .outerRule(CecRules.requiresCec(this))
                    .around(CecRules.requiresLeanback(this))
                    .around(CecRules.requiresDeviceType(this, LogicalAddress.PLAYBACK_1))
                    .around(hdmiCecClient);

    public HdmiCecSystemInformationTest() {
        super(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    }

    /**
     * Test 11.2.6-7
     * Tests that the device sends a <FEATURE_ABORT> in response to a <GET_MENU_LANGUAGE>
     */
    @Test
    public void cect_11_2_6_7_GetMenuLanguage() throws Exception {
        hdmiCecClient.sendCecMessage(LogicalAddress.TV, CecOperand.GET_MENU_LANGUAGE);
        String message = hdmiCecClient.checkExpectedOutput(LogicalAddress.TV, CecOperand.FEATURE_ABORT);
        int abortedOpcode = CecMessage.getParams(message,
                CecOperand.GET_MENU_LANGUAGE.toString().length());
        assertThat(CecOperand.getOperand(abortedOpcode)).isEqualTo(CecOperand.GET_MENU_LANGUAGE);
    }

    /**
     * Test 11.2.6-3
     * Tests that the device handles a <SET_MENU_LANGUAGE> with a valid language correctly.
     */
    @Test
    public void cect_11_2_6_3_SetValidMenuLanguage() throws Exception {
        assumeTrue(isLanguageEditable());
        final String locale = getSystemLocale();
        final String originalLanguage = extractLanguage(locale);
        final String language = originalLanguage.equals("spa") ? "eng" : "spa";
        final String newLanguage = originalLanguage.equals("spa") ? "en" : "es";
        try {
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, LogicalAddress.BROADCAST,
                    CecOperand.SET_MENU_LANGUAGE, CecMessage.convertStringToHexParams(language));
            TimeUnit.SECONDS.sleep(5);
            assertThat(extractLanguage(getSystemLocale())).isEqualTo(newLanguage);
        } finally {
            setSystemLocale(locale);
        }
    }

    /**
     * Test 11.2.6-4
     * Tests that the device ignores a <SET_MENU_LANGUAGE> with an invalid language.
     */
    @Test
    public void cect_11_2_6_4_SetInvalidMenuLanguage() throws Exception {
        assumeTrue(isLanguageEditable());
        final String locale = getSystemLocale();
        final String originalLanguage = extractLanguage(locale);
        final String language = "spb";
        try {
            hdmiCecClient.sendCecMessage(LogicalAddress.TV, LogicalAddress.BROADCAST,
                    CecOperand.SET_MENU_LANGUAGE, CecMessage.convertStringToHexParams(language));
            assertThat(extractLanguage(getSystemLocale())).isEqualTo(originalLanguage);
        } finally {
            setSystemLocale(locale);
        }
    }

    /**
     * Test 11.2.6-5
     * Tests that the device ignores a <SET_MENU_LANGUAGE> with a valid language that comes from a
     * source device which is not TV.
     */
    @Test
    public void cect_11_2_6_5_SetValidMenuLanguageFromInvalidSource() throws Exception {
        assumeTrue(isLanguageEditable());
        final String locale = getSystemLocale();
        final String originalLanguage = extractLanguage(locale);
        final String language = originalLanguage.equals("spa") ? "eng" : "spa";
        try {
            hdmiCecClient.sendCecMessage(LogicalAddress.RECORDER_1, LogicalAddress.BROADCAST,
                    CecOperand.SET_MENU_LANGUAGE, CecMessage.convertStringToHexParams(language));
            assertThat(extractLanguage(getSystemLocale())).isEqualTo(originalLanguage);
        } finally {
            setSystemLocale(locale);
        }
    }
}
