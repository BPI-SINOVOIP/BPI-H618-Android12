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
package com.android.cts.deviceandprofileowner;

import static com.android.cts.devicepolicy.TestCertificates.getCaCert;
import static com.android.cts.devicepolicy.TestCertificates.getTestKey;
import static com.android.cts.devicepolicy.TestCertificates.getUserCert;

import static org.junit.Assert.assertNotEquals;

import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSuggestion;
import android.text.TextUtils;

import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Collections;

/**
 * Tests that require the WiFi feature.
 */
public class WifiTest extends BaseDeviceAdminTest {
    /** Mac address returned when the caller doesn't have access. */
    private static final String DEFAULT_MAC_ADDRESS = "02:00:00:00:00:00";

    public static final ComponentName ADMIN = new ComponentName(
            BasicAdminReceiver.class.getPackage().getName(),
            BasicAdminReceiver.class.getName());

    private static final String TEST_ALIAS = "test_alias";
    private static final String TEST_SSID = "\"SomeNet\"";

    private WifiManager mWm;
    private int mNetId = -1;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mWm = mContext.getSystemService(WifiManager.class);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mNetId != -1) {
            mWm.removeNetwork(mNetId);
        }
        // Remove all suggestions if any were added.
        mWm.removeNetworkSuggestions(Collections.emptyList());
        mDevicePolicyManager.removeKeyPair(ADMIN, TEST_ALIAS);

        super.tearDown();
    }

    public void testGetWifiMacAddress() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WIFI)) {
            // wifi not supported.
            return;
        }
        final String macAddress = mDevicePolicyManager.getWifiMacAddress(ADMIN);

        assertFalse("Device owner should be able to get the real MAC address",
                DEFAULT_MAC_ADDRESS.equals(macAddress));
        assertFalse("getWifiMacAddress() returned an empty string.  WiFi not enabled?",
                TextUtils.isEmpty(macAddress));
    }

    public void testCannotGetWifiMacAddress() {
        try {
            mDevicePolicyManager.getWifiMacAddress(ADMIN);
            fail("Profile owner shouldn't be able to get the MAC address");
        } catch (SecurityException expected) {
        }
    }

    public void testAddNetworkWithKeychainKey_granted() throws Exception {
        prepareTestKeyPair(/* allowForWifi= */ true);
        final WifiConfiguration config = makeTestWifiConfig();

        mNetId = mWm.addNetwork(config);

        assertNotEquals(-1, mNetId);
    }

    public void testAddNetworkWithKeychainKey_notGranted() throws Exception {
        prepareTestKeyPair(/* allowForWifi= */ false);
        final WifiConfiguration config = makeTestWifiConfig();

        mNetId = mWm.addNetwork(config);

        assertEquals(-1, mNetId);
    }

    public void testAddNetworkSuggestionWithKeychainKey_granted() throws Exception {
        prepareTestKeyPair(/* allowForWifi= */ true);
        final WifiNetworkSuggestion suggestion = makeWifiNetworkSuggestion();

        assertEquals(WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS,
                mWm.addNetworkSuggestions(Collections.singletonList(suggestion)));
    }

    public void testAddNetworkSuggestionWithKeychainKey_notGranted() throws Exception {
        prepareTestKeyPair(/* allowForWifi= */ false);
        final WifiNetworkSuggestion suggestion = makeWifiNetworkSuggestion();

        assertEquals(WifiManager.STATUS_NETWORK_SUGGESTIONS_ERROR_ADD_INVALID,
                mWm.addNetworkSuggestions(Collections.singletonList(suggestion)));
    }

    private WifiEnterpriseConfig makeWifiEnterpriseConfig() throws Exception {
        WifiEnterpriseConfig enterpriseConfig = new WifiEnterpriseConfig();
        enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.TLS);
        enterpriseConfig.setDomainSuffixMatch("some-domain.com");
        enterpriseConfig.setIdentity("user");
        enterpriseConfig.setCaCertificate((X509Certificate) getCaCert());
        enterpriseConfig.setClientKeyPairAlias(TEST_ALIAS);
        return enterpriseConfig;
    }

    private WifiNetworkSuggestion makeWifiNetworkSuggestion() throws Exception {
        return new WifiNetworkSuggestion.Builder()
                .setSsid(TEST_SSID)
                .setWpa2EnterpriseConfig(makeWifiEnterpriseConfig())
                .build();
    }

    private void prepareTestKeyPair(boolean allowForWifi) throws Exception {
        assertTrue(mDevicePolicyManager.installKeyPair(ADMIN, getTestKey(),
                new Certificate[]{getUserCert()}, TEST_ALIAS,
                /* requestAccess= */ false));
        if (allowForWifi) {
            assertTrue(mDevicePolicyManager.grantKeyPairToWifiAuth(TEST_ALIAS));
        }
    }

    private WifiConfiguration makeTestWifiConfig() throws Exception {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = TEST_SSID;
        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_EAP);
        config.enterpriseConfig = makeWifiEnterpriseConfig();
        return config;
    }
}
