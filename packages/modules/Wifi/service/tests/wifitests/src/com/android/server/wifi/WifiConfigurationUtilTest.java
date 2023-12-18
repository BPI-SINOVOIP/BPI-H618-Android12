/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.server.wifi;

import static android.net.wifi.WifiConfiguration.INVALID_NETWORK_ID;
import static android.net.wifi.WifiEnterpriseConfig.OCSP_NONE;
import static android.net.wifi.WifiEnterpriseConfig.OCSP_REQUIRE_CERT_STATUS;

import static com.android.server.wifi.WifiConfigurationUtil.addSecurityTypeToNetworkId;
import static com.android.server.wifi.WifiConfigurationUtil.convertWifiInfoSecurityTypeToWifiConfiguration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.pm.UserInfo;
import android.net.IpConfiguration;
import android.net.MacAddress;
import android.net.wifi.ScanResult;
import android.net.wifi.SecurityParams;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.WifiScanner;
import android.os.PatternMatcher;
import android.util.Pair;

import androidx.test.filters.SmallTest;

import com.android.modules.utils.build.SdkLevel;

import org.junit.Test;

import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Unit tests for {@link com.android.server.wifi.WifiConfigurationUtil}.
 */
@SmallTest
public class WifiConfigurationUtilTest extends WifiBaseTest {
    static final int CURRENT_USER_ID = 0;
    static final int CURRENT_USER_MANAGED_PROFILE_USER_ID = 10;
    static final int OTHER_USER_ID = 11;
    static final int TEST_UID = 10000;
    static final String TEST_PACKAGE = "com.test";
    static final String TEST_SSID = "\"test_ssid\"";
    static final String TEST_SSID_1 = "\"test_ssid_1\"";
    static final String TEST_BSSID = "aa:aa:11:22:cc:dd";
    static final String TEST_BSSID_1 = "11:22:11:22:cc:dd";
    static final List<UserInfo> PROFILES = Arrays.asList(
            new UserInfo(CURRENT_USER_ID, "owner", 0),
            new UserInfo(CURRENT_USER_MANAGED_PROFILE_USER_ID, "managed profile", 0));

    /**
     * Verify that new WifiEnterpriseConfig is detected.
     */
    @Test
    public void testEnterpriseConfigAdded() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(
                null, eapConfig.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig eap change is detected.
     */
    @Test
    public void testEnterpriseConfigEapChangeDetected() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS);
        EnterpriseConfig peapConfig = new EnterpriseConfig(WifiEnterpriseConfig.Eap.PEAP);

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig.enterpriseConfig,
                peapConfig.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig phase2 method change is detected.
     */
    @Test
    public void testEnterpriseConfigPhase2ChangeDetected() {
        EnterpriseConfig eapConfig =
                new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                        .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2);
        EnterpriseConfig papConfig =
                new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                        .setPhase2(WifiEnterpriseConfig.Phase2.PAP);

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig.enterpriseConfig,
                papConfig.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig added Certificate is detected.
     */
    @Test
    public void testCaCertificateAddedDetected() {
        EnterpriseConfig eapConfigNoCerts = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password");

        EnterpriseConfig eapConfig1Cert = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(
                eapConfigNoCerts.enterpriseConfig, eapConfig1Cert.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig Certificate change is detected.
     */
    @Test
    public void testDifferentCaCertificateDetected() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});

        EnterpriseConfig eapConfigNewCert = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT1});

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig.enterpriseConfig,
                eapConfigNewCert.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig added Certificate changes are detected.
     */
    @Test
    public void testCaCertificateChangesDetected() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});

        EnterpriseConfig eapConfigAddedCert = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0, FakeKeys.CA_CERT1});

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig.enterpriseConfig,
                eapConfigAddedCert.enterpriseConfig));
    }

    /**
     * Verify that WifiEnterpriseConfig does not detect changes for identical configs.
     */
    @Test
    public void testWifiEnterpriseConfigNoChanges() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0, FakeKeys.CA_CERT1});

        // Just to be clear that check is not against the same object
        EnterpriseConfig eapConfigSame = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0, FakeKeys.CA_CERT1});

        assertFalse(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig.enterpriseConfig,
                eapConfigSame.enterpriseConfig));
    }

    /**
     * Verify that the validate method successfully validates good WifiConfigurations with ASCII
     * values.
     */
    @Test
    public void testValidatePositiveCases_AsciiSsidString() {
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createOpenNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createPskNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createWepNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createEapNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createOweNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createSaeNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                WifiConfigurationTestUtil.createEapSuiteBNetwork(),
                WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method successfully validates good WifiConfigurations with hex
     * values.
     */
    @Test
    public void testValidatePositiveCases_HexSsidString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        config.SSID = "abcd1234555a";
        config.preSharedKey = "abcd123455151234556788990034556667332345667322344556676743233445";
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method validates WifiConfiguration with "any" in the BSSID field.
     */
    @Test
    public void testValidatePositiveCases_AnyBssidString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        config.BSSID = "any";
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                config, WifiConfigurationUtil.VALIDATE_FOR_UPDATE));
    }

    /**
     * Verify that the validate method validates WifiConfiguration with masked psk string only for
     * an update.
     */
    @Test
    public void testValidatePositiveCases_MaskedPskString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = WifiConfigurationUtil.PASSWORD_MASK;
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                config, WifiConfigurationUtil.VALIDATE_FOR_UPDATE));
    }

    /**
     * Verify that the validate method validates WifiConfiguration with null ssid only for an
     * update.
     */
    @Test
    public void testValidatePositiveCases_OnlyUpdateIgnoresNullSsid() {
        WifiConfiguration config = new WifiConfiguration();
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                config, WifiConfigurationUtil.VALIDATE_FOR_UPDATE));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad ssid length.
     */
    @Test
    public void testValidateNegativeCases_BadAsciiSsidLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.SSID = "\"abcdfefeeretretyetretetetetetrertertrsreqwrwe\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.SSID = "\"\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad ssid length.
     */
    @Test
    public void testValidateNegativeCases_BadUtf8SsidLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.SSID = "\"가하아너너ㅓ저저ㅓ어아아다자저ㅓ더타아어어러두어\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.SSID = "\"\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed ssid
     * string.
     */
    @Test
    public void testValidateNegativeCases_MalformedAsciiSsidString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.SSID = "\"ab";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad ssid length.
     */
    @Test
    public void testValidateNegativeCases_BadHexSsidLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.SSID = "abcdfe012345632423343543453456464545656464545646454ace34534545634535";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.SSID = "";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed ssid
     * string.
     */
    @Test
    public void testValidateNegativeCases_MalformedHexSsidString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.SSID = "hello";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad psk length.
     */
    @Test
    public void testValidateNegativeCases_BadAsciiPskLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "\"abcdffeeretretyetreteteteabe34tetrertertrsraaaaaaaaaaa345eqwrweewq"
                + "weqe\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.preSharedKey = "\"454\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad sae length.
     */
    @Test
    public void testValidateNegativeCases_BadAsciiSaeLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createSaeNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "\"abcdffeeretretyetreteteteabe34tetrertertrsraaaaaaaaaaa345eqwrweewq"
                + "weqe\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.preSharedKey = "\"\"";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed psk
     * string.
     */
    @Test
    public void testValidateNegativeCases_MalformedAsciiPskString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "\"abcdfefeeretrety";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed sae
     * string.
     */
    @Test
    public void testValidateNegativeCases_MalformedAsciiSaeString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createSaeNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "\"abcdfefeeretrety";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad psk length.
     */
    @Test
    public void testValidateNegativeCases_BadHexPskLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "abcd123456788990013453445345465465476546";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.preSharedKey = "";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed psk
     * string.
     */
    @Test
    public void testValidateNegativeCases_MalformedHexPskString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "adbdfgretrtyrtyrty";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed sae
     * string.
     */
    @Test
    public void testValidateNegativeCases_MalformedHexSaeString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createSaeNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.preSharedKey = "adbdfgretrtyrtyrty";
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method validates WifiConfiguration with masked wep key only for
     * an update.
     */
    @Test
    public void testValidatePositiveCases_MaskedWepKeysString() {
        WifiConfiguration config = WifiConfigurationTestUtil.createWepNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.wepKeys = new String[]{ WifiConfigurationUtil.PASSWORD_MASK,
                WifiConfigurationUtil.PASSWORD_MASK,
                WifiConfigurationUtil.PASSWORD_MASK,
                WifiConfigurationUtil.PASSWORD_MASK};
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(
                config, WifiConfigurationUtil.VALIDATE_FOR_UPDATE));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad wep length.
     */
    @Test
    public void testValidateNegativeCases_BadWepKeysLength() {
        WifiConfiguration config = WifiConfigurationTestUtil.createWepNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.wepKeys = new String[] {"\"abcd\""};
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        config.wepKeys = new String[] {"456"};
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        // Error scenario in b/169638868.
        config.wepKeys = new String[] {""};
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad wep tx key idx.
     */
    @Test
    public void testValidateNegativeCases_BadWepTxKeysIndex() {
        WifiConfiguration config = WifiConfigurationTestUtil.createWepNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        // Should be < wepKeys.length
        config.wepTxKeyIndex = config.wepKeys.length;
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad key mgmt values.
     */
    @Test
    public void testValidateNegativeCases_BadKeyMgmtPskEap() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.IEEE8021X);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad key mgmt values.
     */
    @Test
    public void testValidateNegativeCases_BadKeyMgmtOpenPsk() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad key mgmt values.
     */
    @Test
    public void testValidateNegativeCases_BadKeyMgmt() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.IEEE8021X);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad key mgmt values.
     */
    @Test
    public void testValidateNegativeCases_BadSuiteBKeyMgmt() {
        WifiConfiguration config = WifiConfigurationTestUtil.createEapSuiteBNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedKeyManagement.clear(WifiConfiguration.KeyMgmt.IEEE8021X);
        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad ipconfiguration
     * values.
     */
    @Test
    public void testValidateNegativeCases_BadIpconfiguration() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        IpConfiguration ipConfig =
                WifiConfigurationTestUtil.createStaticIpConfigurationWithPacProxy();
        config.setIpConfiguration(ipConfig);
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        ipConfig.setStaticIpConfiguration(null);
        config.setIpConfiguration(ipConfig);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad KeyMgmt value.
     */
    @Test
    public void testValidateNegativeCases_InvalidKeyMgmt() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.FT_EAP + 1);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad KeyMgmt value.
     */
    @Test
    public void testValidateNegativeCases_InvalidKeyMgmtWithPreSharedKey() {
        WifiConfiguration config = WifiConfigurationTestUtil.createPskNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.setSecurityParams(WifiConfiguration.SECURITY_TYPE_OSEN);
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        // Verify we reset the KeyMgmt
        assertTrue(config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_PSK));
        assertFalse(config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.OSEN));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad Protocol value.
     */
    @Test
    public void testValidateNegativeCases_InvalidProtocol() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedProtocols.set(4);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad AuthAlgorithm
     * value.
     */
    @Test
    public void testValidateNegativeCases_InvalidAuthAlgorithm() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.LEAP + 3);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad GroupCipher
     * value.
     */
    @Test
    public void testValidateNegativeCases_InvalidGroupCipher() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.GTK_NOT_USED + 4);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with bad PairwiseCipher
     * value.
     */
    @Test
    public void testValidateNegativeCases_InvalidPairwiseCipher() {
        WifiConfiguration config = WifiConfigurationTestUtil.createOpenNetwork();
        assertTrue(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP + 4);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method successfully validates good WifiNetworkSpecifier with
     * only ssid pattern set.
     */
    @Test
    public void testValidateNetworkSpecifierPositiveCases_SsidPattern() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_LITERAL),
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertTrue(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method successfully validates good WifiNetworkSpecifier with
     * only bssid pattern set.
     */
    @Test
    public void testValidateNetworkSpecifierPositiveCases_BssidPattern() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(".*", PatternMatcher.PATTERN_SIMPLE_GLOB),
                Pair.create(MacAddress.fromString(TEST_BSSID), MacAddress.BROADCAST_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertTrue(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method successfully validates good WifiNetworkSpecifier with
     * both ssid & bssid patterns set.
     */
    @Test
    public void testValidateNetworkSpecifierPositiveCases_BothSsidPatternAndBssidPattern() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_LITERAL),
                Pair.create(MacAddress.fromString(TEST_BSSID), MacAddress.BROADCAST_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertTrue(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method validates a WifiNetworkSpecifier that specifies ssid, bssid,
     * and band. Note that such requests will currently still be rejected by WifiNetworkFactory, but
     * requesting specific bands may be supported in future releases.
     */
    @Test
    public void testValidateNetworkSpecifierPositiveCases_SsidPatternAndBssidPatternAndBand() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_LITERAL),
                Pair.create(MacAddress.fromString(TEST_BSSID), MacAddress.BROADCAST_ADDRESS),
                ScanResult.WIFI_BAND_5_GHZ,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertTrue(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }


    /**
     * Verify that the validate method fails to validate WifiNetworkSpecifier with no
     * ssid/bssid info.
     */
    @Test
    public void testValidateNetworkSpecifierNegativeCases_NoSsidBssid() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(".*", PatternMatcher.PATTERN_SIMPLE_GLOB),
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertFalse(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method fails to validate WifiNetworkSpecifier with invalid SSID
     * match pattern.
     */
    @Test
    public void testValidateNetworkSpecifierNegativeCases_MatchNoneSsidPattern() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher("", PatternMatcher.PATTERN_LITERAL),
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertFalse(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method fails to validate WifiNetworkSpecifier with illegal
     * pattern.
     */
    @Test
    public void testValidateNetworkSpecifierNegativeCases_MatchNoneBssidPattern() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_LITERAL),
                Pair.create(MacAddress.BROADCAST_ADDRESS, MacAddress.BROADCAST_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertFalse(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method fails to validate WifiNetworkSpecifier with illegal
     * pattern.
     */
    @Test
    public void testValidateNetworkSpecifierNegativeCases_InvalidBssidPattern() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_LITERAL),
                Pair.create(MacAddress.fromString(TEST_BSSID), WifiManager.ALL_ZEROS_MAC_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenNetwork());
        assertFalse(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method fails to validate WifiNetworkSpecifier with SSID pattern
     * for hidden network.
     */
    @Test
    public void testValidateNetworkSpecifierNegativeCases_NoSsidPatternForHiddenNetwork() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_PREFIX),
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS),
                ScanResult.UNSPECIFIED,
                WifiConfigurationTestUtil.createOpenHiddenNetwork());
        assertFalse(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify that the validate method fails to validate WifiNetworkSpecifier with an invalid band.
     */
    @Test
    public void testValidateNetworkSpecifierNegativeCases_InvalidBand() {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier(
                new PatternMatcher(TEST_SSID, PatternMatcher.PATTERN_LITERAL),
                Pair.create(MacAddress.fromString(TEST_BSSID), MacAddress.BROADCAST_ADDRESS),
                42,  // invalid
                WifiConfigurationTestUtil.createOpenNetwork());
        assertFalse(WifiConfigurationUtil.validateNetworkSpecifier(specifier));
    }

    /**
     * Verify the instance of {@link android.net.wifi.WifiScanner.PnoSettings.PnoNetwork} created
     * for an open network using {@link WifiConfigurationUtil#createPnoNetwork(
     * WifiConfiguration)}.
     */
    @Test
    public void testCreatePnoNetworkWithOpenNetwork() {
        WifiConfiguration network = WifiConfigurationTestUtil.createOpenNetwork();
        WifiScanner.PnoSettings.PnoNetwork pnoNetwork =
                WifiConfigurationUtil.createPnoNetwork(network);
        assertEquals(network.SSID, pnoNetwork.ssid);
        assertEquals(
                WifiScanner.PnoSettings.PnoNetwork.FLAG_A_BAND
                        | WifiScanner.PnoSettings.PnoNetwork.FLAG_G_BAND, pnoNetwork.flags);
        assertEquals(WifiScanner.PnoSettings.PnoNetwork.AUTH_CODE_OPEN, pnoNetwork.authBitField);
    }

    /**
     * Verify the instance of {@link android.net.wifi.WifiScanner.PnoSettings.PnoNetwork} created
     * for an open hidden network using {@link WifiConfigurationUtil#createPnoNetwork(
     * WifiConfiguration)}.
     */
    @Test
    public void testCreatePnoNetworkWithOpenHiddenNetwork() {
        WifiConfiguration network = WifiConfigurationTestUtil.createOpenHiddenNetwork();
        WifiScanner.PnoSettings.PnoNetwork pnoNetwork =
                WifiConfigurationUtil.createPnoNetwork(network);
        assertEquals(network.SSID, pnoNetwork.ssid);
        assertEquals(
                WifiScanner.PnoSettings.PnoNetwork.FLAG_A_BAND
                        | WifiScanner.PnoSettings.PnoNetwork.FLAG_G_BAND
                        | WifiScanner.PnoSettings.PnoNetwork.FLAG_DIRECTED_SCAN, pnoNetwork.flags);
        assertEquals(WifiScanner.PnoSettings.PnoNetwork.AUTH_CODE_OPEN, pnoNetwork.authBitField);
    }

    /**
     * Verify the instance of {@link android.net.wifi.WifiScanner.PnoSettings.PnoNetwork} created
     * for a PSK network using {@link WifiConfigurationUtil#createPnoNetwork(WifiConfiguration)
     * }.
     */
    @Test
    public void testCreatePnoNetworkWithPskNetwork() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork();
        WifiScanner.PnoSettings.PnoNetwork pnoNetwork =
                WifiConfigurationUtil.createPnoNetwork(network);
        assertEquals(network.SSID, pnoNetwork.ssid);
        assertEquals(
                WifiScanner.PnoSettings.PnoNetwork.FLAG_A_BAND
                        | WifiScanner.PnoSettings.PnoNetwork.FLAG_G_BAND, pnoNetwork.flags);
        assertEquals(WifiScanner.PnoSettings.PnoNetwork.AUTH_CODE_PSK, pnoNetwork.authBitField);
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns true when two WifiConfiguration
     * objects have the same parameters.
     */
    @Test
    public void testIsSameNetworkReturnsTrueOnSameNetwork() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        WifiConfiguration network1 = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        assertTrue(WifiConfigurationUtil.isSameNetwork(network, network1));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns true when two WifiConfiguration
     * objects have the same parameters but different network selection BSSID's.
     */
    @Test
    public void testIsSameNetworkReturnsTrueOnSameNetworkWithDifferentBSSID() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        network.getNetworkSelectionStatus().setNetworkSelectionBSSID(TEST_BSSID);
        WifiConfiguration network1 = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        network1.getNetworkSelectionStatus().setNetworkSelectionBSSID(TEST_BSSID_1);
        assertTrue(WifiConfigurationUtil.isSameNetwork(network, network1));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns false when two WifiConfiguration
     * objects have the different SSIDs.
     */
    @Test
    public void testIsSameNetworkReturnsFalseOnDifferentSSID() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        WifiConfiguration network1 = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID_1);
        assertFalse(WifiConfigurationUtil.isSameNetwork(network, network1));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns false when two WifiConfiguration
     * objects have the different security type.
     */
    @Test
    public void testIsSameNetworkReturnsFalseOnDifferentSecurityType() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        WifiConfiguration network1 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        assertFalse(WifiConfigurationUtil.isSameNetwork(network, network1));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns false when two WifiConfiguration
     * objects have the different EAP identity.
     */
    @Test
    public void testIsSameNetworkReturnsFalseOnDifferentEapIdentity() {
        WifiConfiguration network1 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        WifiConfiguration network2 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        network1.enterpriseConfig.setIdentity("Identity1");
        network2.enterpriseConfig.setIdentity("Identity2");
        assertFalse(WifiConfigurationUtil.isSameNetwork(network1, network2));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns false when two WifiConfiguration
     * objects have the different EAP anonymous identity.
     */
    @Test
    public void testIsSameNetworkReturnsFalseOnDifferentEapAnonymousIdentity() {
        WifiConfiguration network1 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        WifiConfiguration network2 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        network1.enterpriseConfig.setAnonymousIdentity("Identity1");
        network2.enterpriseConfig.setAnonymousIdentity("Identity2");
        assertFalse(WifiConfigurationUtil.isSameNetwork(network1, network2));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns true when two WifiConfiguration
     * objects have the different EAP anonymous(pseudonym) identity in EAP-SIM.
     */
    @Test
    public void testIsSameNetworkReturnsTrueOnDifferentEapAnonymousIdentityInEapSim() {
        WifiConfiguration network1 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        WifiConfiguration network2 = WifiConfigurationTestUtil.createEapNetwork(TEST_SSID);
        network1.enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.SIM);
        network2.enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.SIM);
        network1.enterpriseConfig.setAnonymousIdentity("Identity1");
        network2.enterpriseConfig.setAnonymousIdentity("Identity2");
        assertTrue(WifiConfigurationUtil.isSameNetwork(network1, network2));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns true when two WifiConfiguration
     * objects have the same candidate security params.
     */
    @Test
    public void testIsSameNetworkReturnsTrueOnSameNetworkWithSameCandidateSecurityParams() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        network.getNetworkSelectionStatus().setCandidateSecurityParams(
                SecurityParams.createSecurityParamsBySecurityType(
                        WifiConfiguration.SECURITY_TYPE_PSK));
        WifiConfiguration network1 = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        network1.getNetworkSelectionStatus().setCandidateSecurityParams(
                SecurityParams.createSecurityParamsBySecurityType(
                        WifiConfiguration.SECURITY_TYPE_PSK));
        assertTrue(WifiConfigurationUtil.isSameNetwork(network, network1));
    }

    /**
     * Verify that WifiConfigurationUtil.isSameNetwork returns false when two WifiConfiguration
     * objects have the different candidate security params.
     */
    @Test
    public void testIsSameNetworkReturnsTrueOnSameNetworkWithDifferentCandidateSecurityParams() {
        WifiConfiguration network = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        network.getNetworkSelectionStatus().setCandidateSecurityParams(
                SecurityParams.createSecurityParamsBySecurityType(
                        WifiConfiguration.SECURITY_TYPE_PSK));
        WifiConfiguration network1 = WifiConfigurationTestUtil.createPskNetwork(TEST_SSID);
        network1.getNetworkSelectionStatus().setCandidateSecurityParams(
                SecurityParams.createSecurityParamsBySecurityType(
                        WifiConfiguration.SECURITY_TYPE_SAE));
        assertFalse(WifiConfigurationUtil.isSameNetwork(network, network1));
    }

    /**
     * Verify the instance of {@link android.net.wifi.WifiScanner.PnoSettings.PnoNetwork} created
     * for a EAP network using {@link WifiConfigurationUtil#createPnoNetwork(WifiConfiguration)
     * }.
     */
    @Test
    public void testCreatePnoNetworkWithEapNetwork() {
        WifiConfiguration network = WifiConfigurationTestUtil.createEapNetwork();
        WifiScanner.PnoSettings.PnoNetwork pnoNetwork =
                WifiConfigurationUtil.createPnoNetwork(network);
        assertEquals(network.SSID, pnoNetwork.ssid);
        assertEquals(
                WifiScanner.PnoSettings.PnoNetwork.FLAG_A_BAND
                        | WifiScanner.PnoSettings.PnoNetwork.FLAG_G_BAND, pnoNetwork.flags);
        assertEquals(WifiScanner.PnoSettings.PnoNetwork.AUTH_CODE_EAPOL, pnoNetwork.authBitField);
    }

    /**
     * Verify that the generalized
     * {@link com.android.server.wifi.WifiConfigurationUtil.WifiConfigurationComparator}
     * can be used to sort a List given a 'compareNetworkWithSameStatus' method.
     */
    @Test
    public void testPnoListComparator() {
        List<WifiConfiguration> networks = new ArrayList<>();
        final WifiConfiguration enabledNetwork1 = WifiConfigurationTestUtil.createEapNetwork();
        enabledNetwork1.getNetworkSelectionStatus().setNetworkSelectionStatus(
                WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLED);
        final WifiConfiguration enabledNetwork2 = WifiConfigurationTestUtil.createEapNetwork();
        enabledNetwork2.getNetworkSelectionStatus().setNetworkSelectionStatus(
                WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLED);
        final WifiConfiguration tempDisabledNetwork1 = WifiConfigurationTestUtil.createEapNetwork();
        tempDisabledNetwork1.getNetworkSelectionStatus().setNetworkSelectionStatus(
                WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_TEMPORARY_DISABLED);
        final WifiConfiguration tempDisabledNetwork2 = WifiConfigurationTestUtil.createEapNetwork();
        tempDisabledNetwork2.getNetworkSelectionStatus().setNetworkSelectionStatus(
                WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_TEMPORARY_DISABLED);
        WifiConfiguration permDisabledNetwork = WifiConfigurationTestUtil.createEapNetwork();
        permDisabledNetwork.getNetworkSelectionStatus().setNetworkSelectionStatus(
                WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_PERMANENTLY_DISABLED);

        // Add all the networks to the list.
        networks.add(tempDisabledNetwork1);
        networks.add(enabledNetwork1);
        networks.add(permDisabledNetwork);
        networks.add(tempDisabledNetwork2);
        networks.add(enabledNetwork2);

        // Prefer |enabledNetwork1| over |enabledNetwork2| and |tempDisabledNetwork1| over
        // |tempDisabledNetwork2|.
        WifiConfigurationUtil.WifiConfigurationComparator comparator =
                new WifiConfigurationUtil.WifiConfigurationComparator() {
                    @Override
                    public int compareNetworksWithSameStatus(
                            WifiConfiguration a, WifiConfiguration b) {
                        if (a == enabledNetwork1 && b == enabledNetwork2) {
                            return -1;
                        } else if (b == enabledNetwork1 && a == enabledNetwork2) {
                            return 1;
                        } else if (a == tempDisabledNetwork1 && b == tempDisabledNetwork2) {
                            return -1;
                        } else if (b == tempDisabledNetwork1 && a == tempDisabledNetwork2) {
                            return 1;
                        }
                        return 0;
                    }
                };
        Collections.sort(networks, comparator);

        // Now ensure that the networks were sorted correctly.
        assertEquals(enabledNetwork1, networks.get(0));
        assertEquals(enabledNetwork2, networks.get(1));
        assertEquals(tempDisabledNetwork1, networks.get(2));
        assertEquals(tempDisabledNetwork2, networks.get(3));
        assertEquals(permDisabledNetwork, networks.get(4));
    }

    /**
     * Verifies that when the existing configuration is null and macRandomizationSetting in the
     * newConfig is the default value, then hasMacRandomizationSettingsChanged returns false.
     */
    @Test
    public void testHasMacRandomizationSettingsChangedNullExistingConfigDefaultNewConfig() {
        WifiConfiguration newConfig = new WifiConfiguration();
        assertFalse(WifiConfigurationUtil.hasMacRandomizationSettingsChanged(null, newConfig));
    }

    /**
     * Verifies that when the existing configuration is null and macRandomizationSetting in the
     * newConfig is not the default value, then hasMacRandomizationSettingsChanged returns true.
     */
    @Test
    public void testHasMacRandomizationSettingsChangedNullExistingConfigModifiedNewConfig() {
        WifiConfiguration newConfig = new WifiConfiguration();
        newConfig.macRandomizationSetting = WifiConfiguration.RANDOMIZATION_NONE;
        assertTrue(WifiConfigurationUtil.hasMacRandomizationSettingsChanged(null, newConfig));
    }

    /**
     * Verifies that when macRandomizationSetting in the newConfig is different from existingConfig
     * hasMacRandomizationSettingsChanged returns true.
     */
    @Test
    public void testHasMacRandomizationSettingsChangedFieldsDifferent() {
        WifiConfiguration existingConfig = new WifiConfiguration();
        WifiConfiguration newConfig = new WifiConfiguration();
        newConfig.macRandomizationSetting = WifiConfiguration.RANDOMIZATION_NONE;
        assertTrue(WifiConfigurationUtil.hasMacRandomizationSettingsChanged(
                existingConfig, newConfig));
    }

    /**
     * Verifies that when macRandomizationSetting in the newConfig is the same as existingConfig
     * hasMacRandomizationSettingsChanged returns false.
     */
    @Test
    public void testHasMacRandomizationSettingsChangedFieldsSame() {
        WifiConfiguration existingConfig = new WifiConfiguration();
        existingConfig.macRandomizationSetting = WifiConfiguration.RANDOMIZATION_NONE;
        WifiConfiguration newConfig = new WifiConfiguration();
        newConfig.macRandomizationSetting = WifiConfiguration.RANDOMIZATION_NONE;
        assertFalse(WifiConfigurationUtil.hasMacRandomizationSettingsChanged(
                existingConfig, newConfig));
    }

    private static class EnterpriseConfig {
        public String eap;
        public String phase2;
        public String identity;
        public String password;
        public X509Certificate[] caCerts;
        public WifiEnterpriseConfig enterpriseConfig;
        public String wapiCertSuite;

        EnterpriseConfig(int eapMethod) {
            enterpriseConfig = new WifiEnterpriseConfig();
            enterpriseConfig.setEapMethod(eapMethod);
            eap = WifiEnterpriseConfig.Eap.strings[eapMethod];
        }

        public EnterpriseConfig setPhase2(int phase2Method) {
            enterpriseConfig.setPhase2Method(phase2Method);
            phase2 = "auth=" + WifiEnterpriseConfig.Phase2.strings[phase2Method];
            return this;
        }

        public EnterpriseConfig setIdentity(String identity, String password) {
            enterpriseConfig.setIdentity(identity);
            enterpriseConfig.setPassword(password);
            this.identity = identity;
            this.password = password;
            return this;
        }

        public EnterpriseConfig setCaCerts(X509Certificate[] certs) {
            enterpriseConfig.setCaCertificates(certs);
            caCerts = certs;
            return this;
        }

        public EnterpriseConfig setWapiCertSuite(String certSuite) {
            enterpriseConfig.setWapiCertSuite(certSuite);
            wapiCertSuite = certSuite;
            return this;
        }
    }

    /**
     * Verify WifiEnterpriseConfig CA Certificate alias changes are detected.
     */
    @Test
    public void testCaCertificateAliasChangesDetected() {
        EnterpriseConfig eapConfig1 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password");
        eapConfig1.enterpriseConfig.setCaCertificateAlias("ALIAS_1");

        EnterpriseConfig eapConfig2 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password");
        eapConfig2.enterpriseConfig.setCaCertificateAlias("ALIAS_2");

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig1.enterpriseConfig,
                eapConfig2.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig Client Certificate alias changes are detected.
     */
    @Test
    public void testClientCertificateAliasChangesDetected() {
        EnterpriseConfig eapConfig1 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TLS);
        eapConfig1.enterpriseConfig.setCaCertificateAlias("ALIAS_1");
        eapConfig1.enterpriseConfig.setClientCertificateAlias("CLIENT_ALIAS_1");

        EnterpriseConfig eapConfig2 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS);
        eapConfig2.enterpriseConfig.setCaCertificateAlias("ALIAS_1");
        eapConfig2.enterpriseConfig.setClientCertificateAlias("CLIENT_ALIAS_2");

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig1.enterpriseConfig,
                eapConfig2.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig OCSP changes are detected.
     */
    @Test
    public void testOcspChangesDetected() {
        EnterpriseConfig eapConfig1 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});
        eapConfig1.enterpriseConfig.setOcsp(OCSP_NONE);

        EnterpriseConfig eapConfig2 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});
        eapConfig2.enterpriseConfig.setOcsp(OCSP_REQUIRE_CERT_STATUS);

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig1.enterpriseConfig,
                eapConfig2.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig subject match changes are detected.
     */
    @Test
    public void testSubjectMatchChangesDetected() {
        EnterpriseConfig eapConfig1 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});
        eapConfig1.enterpriseConfig.setAltSubjectMatch("domain1.com");

        EnterpriseConfig eapConfig2 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.TTLS)
                .setPhase2(WifiEnterpriseConfig.Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[]{FakeKeys.CA_CERT0});
        eapConfig1.enterpriseConfig.setAltSubjectMatch("domain2.com");

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(eapConfig1.enterpriseConfig,
                eapConfig2.enterpriseConfig));
    }

    /**
     * Verify that new WifiEnterpriseConfig is detected.
     */
    @Test
    public void testEnterpriseConfigWapiCertChanged() {
        EnterpriseConfig eapConfig1 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.WAPI_CERT)
                .setWapiCertSuite("WapiCertSuite1");
        EnterpriseConfig eapConfig2 = new EnterpriseConfig(WifiEnterpriseConfig.Eap.WAPI_CERT)
                .setWapiCertSuite("WapiCertSuite2");

        assertTrue(WifiConfigurationUtil.hasEnterpriseConfigChanged(
                eapConfig1.enterpriseConfig, eapConfig2.enterpriseConfig));
    }

    /**
     * Verify that a WAPI config is not considered an OPEN config.
     */
    @Test
    public void testWapiConfigNotOpenConfig() {
        WifiConfiguration wapiPskConfig = new WifiConfiguration();
        wapiPskConfig.setSecurityParams(WifiConfiguration.SECURITY_TYPE_WAPI_CERT);
        assertFalse(WifiConfigurationUtil.isConfigForOpenNetwork(wapiPskConfig));

        WifiConfiguration wapiCertConfig = new WifiConfiguration();
        wapiCertConfig.setSecurityParams(WifiConfiguration.SECURITY_TYPE_WAPI_CERT);
        assertFalse(WifiConfigurationUtil.isConfigForOpenNetwork(wapiCertConfig));
    }


    /**
     * Verify that the validate method fails to validate WifiConfiguration with malformed
     * enterprise configuration
     */
    @Test
    public void testValidateNegativeCases_MalformedEnterpriseConfig() {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = "\"someNetwork\"";
        config.setSecurityParams(WifiConfiguration.SECURITY_TYPE_EAP);
        // EAP method is kept as Eap.NONE - should not crash, but return invalid ID
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.setSecurityParams(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));

        config.setSecurityParams(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
    }

    /**
     * Verify that the validate method fails to validate WifiConfiguration with enterprise
     * configuration that is missing the identity and/or password.
     */
    @Test
    public void testValidateNegativeCases_NoIdentityOrPasswordEnterpriseConfig() {
        WifiConfiguration config = WifiConfigurationTestUtil.createEapNetwork();
        config.enterpriseConfig.setIdentity(null);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertFalse(WifiConfigurationUtil.validate(config,
                WifiConfigurationUtil.VALIDATE_FOR_UPDATE));

        config = WifiConfigurationTestUtil.createEapNetwork();
        config.enterpriseConfig.setPassword(null);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(config,
                WifiConfigurationUtil.VALIDATE_FOR_UPDATE));

        config = WifiConfigurationTestUtil.createWpa3EnterpriseNetwork(TEST_SSID);
        config.enterpriseConfig.setIdentity(null);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertFalse(WifiConfigurationUtil.validate(config,
                WifiConfigurationUtil.VALIDATE_FOR_UPDATE));

        config = WifiConfigurationTestUtil.createWpa3EnterpriseNetwork(TEST_SSID);
        config.enterpriseConfig.setPassword(null);
        assertFalse(WifiConfigurationUtil.validate(config, WifiConfigurationUtil.VALIDATE_FOR_ADD));
        assertTrue(WifiConfigurationUtil.validate(config,
                WifiConfigurationUtil.VALIDATE_FOR_UPDATE));
    }

    /**
     * Verify the behavior of convertWifiInfoSecurityTypeToWifiConfiguration
     */
    @Test
    public void testConvertWifiInfoSecurityTypeToWifiConfiguration() {
        assertEquals(WifiConfiguration.SECURITY_TYPE_OPEN,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_OPEN));
        assertEquals(WifiConfiguration.SECURITY_TYPE_WEP,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_WEP));
        assertEquals(WifiConfiguration.SECURITY_TYPE_PSK,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_PSK));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_EAP));
        assertEquals(WifiConfiguration.SECURITY_TYPE_SAE,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_SAE));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT,
                convertWifiInfoSecurityTypeToWifiConfiguration(
                        WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT));
        assertEquals(WifiConfiguration.SECURITY_TYPE_OWE,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_OWE));
        assertEquals(WifiConfiguration.SECURITY_TYPE_WAPI_PSK,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_WAPI_PSK));
        assertEquals(WifiConfiguration.SECURITY_TYPE_WAPI_CERT,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_WAPI_CERT));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE,
                convertWifiInfoSecurityTypeToWifiConfiguration(
                        WifiInfo.SECURITY_TYPE_EAP_WPA3_ENTERPRISE));
        assertEquals(WifiConfiguration.SECURITY_TYPE_OSEN,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_OSEN));
        assertEquals(WifiConfiguration.SECURITY_TYPE_PASSPOINT_R1_R2,
                convertWifiInfoSecurityTypeToWifiConfiguration(
                        WifiInfo.SECURITY_TYPE_PASSPOINT_R1_R2));
        assertEquals(WifiConfiguration.SECURITY_TYPE_PASSPOINT_R3,
                convertWifiInfoSecurityTypeToWifiConfiguration(
                        WifiInfo.SECURITY_TYPE_PASSPOINT_R3));
        assertEquals(-1, convertWifiInfoSecurityTypeToWifiConfiguration(13));
        assertEquals(-1,
                convertWifiInfoSecurityTypeToWifiConfiguration(WifiInfo.SECURITY_TYPE_UNKNOWN));
    }

    /**
     * Verify that adding and removing the security type for network ID behaves correctly
     */
    @Test
    public void testAddAndRemoveSecurityTypeForNetworkId() {
        List<Integer> securityList = Arrays.asList(
                WifiConfiguration.SECURITY_TYPE_OPEN,
                WifiConfiguration.SECURITY_TYPE_WEP,
                WifiConfiguration.SECURITY_TYPE_PSK,
                WifiConfiguration.SECURITY_TYPE_EAP,
                WifiConfiguration.SECURITY_TYPE_SAE,
                WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT,
                WifiConfiguration.SECURITY_TYPE_OWE,
                WifiConfiguration.SECURITY_TYPE_WAPI_PSK,
                WifiConfiguration.SECURITY_TYPE_WAPI_CERT,
                WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE,
                WifiConfiguration.SECURITY_TYPE_OSEN,
                WifiConfiguration.SECURITY_TYPE_PASSPOINT_R1_R2,
                WifiConfiguration.SECURITY_TYPE_PASSPOINT_R3
        );

        final int netId = 1;
        if (!SdkLevel.isAtLeastS()) {
            // INVALID_NET_ID should remain the same from either adding or removing
            assertEquals(INVALID_NETWORK_ID, WifiConfigurationUtil.addSecurityTypeToNetworkId(
                    INVALID_NETWORK_ID, WifiConfiguration.SECURITY_TYPE_OPEN));
            assertEquals(INVALID_NETWORK_ID, WifiConfigurationUtil.removeSecurityTypeFromNetworkId(
                    INVALID_NETWORK_ID));
            // Add and then remove should result in the original netId
            for (@WifiConfiguration.SecurityType int securityType : securityList) {
                assertEquals(netId, WifiConfigurationUtil.removeSecurityTypeFromNetworkId(
                        WifiConfigurationUtil.addSecurityTypeToNetworkId(
                                netId, securityType)));
            }
            // Multiple removes should result in the same netId as a single remove
            for (@WifiConfiguration.SecurityType int securityType : securityList) {
                assertEquals(WifiConfigurationUtil.removeSecurityTypeFromNetworkId(netId),
                        WifiConfigurationUtil.removeSecurityTypeFromNetworkId(
                                WifiConfigurationUtil.removeSecurityTypeFromNetworkId(
                                        WifiConfigurationUtil.addSecurityTypeToNetworkId(
                                                netId, securityType))));
            }
            // A unique net id should be created for each security type added
            assertEquals(securityList.size(), securityList.stream()
                    .map(security -> addSecurityTypeToNetworkId(netId, security))
                    .distinct()
                    .count());
        } else {
            // Add should do nothing for SDK level S and above.
            for (@WifiConfiguration.SecurityType int securityType : securityList) {
                assertEquals(netId, addSecurityTypeToNetworkId(netId, securityType));
            }
        }
    }
}
