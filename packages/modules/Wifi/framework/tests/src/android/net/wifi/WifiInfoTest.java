/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.net.wifi;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.net.NetworkCapabilities;
import android.os.Parcel;
import android.telephony.SubscriptionManager;

import androidx.test.filters.SmallTest;

import com.android.modules.utils.build.SdkLevel;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link android.net.wifi.WifiInfo}.
 */
@SmallTest
public class WifiInfoTest {
    private static final long TEST_TX_SUCCESS = 1;
    private static final long TEST_TX_RETRIES = 2;
    private static final long TEST_TX_BAD = 3;
    private static final long TEST_RX_SUCCESS = 4;
    private static final String TEST_PACKAGE_NAME = "com.test.example";
    private static final String TEST_FQDN = "test.com";
    private static final String TEST_PROVIDER_NAME = "test";
    private static final int TEST_WIFI_STANDARD = ScanResult.WIFI_STANDARD_11AC;
    private static final int TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS = 866;
    private static final int TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS = 1200;
    private static final String TEST_SSID = "Test123";
    private static final String TEST_BSSID = "12:12:12:12:12:12";
    private static final int TEST_RSSI = -60;
    private static final int TEST_NETWORK_ID = 5;
    private static final int TEST_NETWORK_ID2 = 6;
    private static final int TEST_SUB_ID = 1;

    private WifiInfo makeWifiInfoForNoRedactions(
            List<ScanResult.InformationElement> informationElements) {
        WifiInfo info = new WifiInfo();
        info.txSuccess = TEST_TX_SUCCESS;
        info.txRetries = TEST_TX_RETRIES;
        info.txBad = TEST_TX_BAD;
        info.rxSuccess = TEST_RX_SUCCESS;
        info.setSSID(WifiSsid.createFromAsciiEncoded(TEST_SSID));
        info.setBSSID(TEST_BSSID);
        info.setNetworkId(TEST_NETWORK_ID);
        info.setTrusted(true);
        info.setOemPaid(true);
        info.setOemPrivate(true);
        info.setCarrierMerged(true);
        info.setOsuAp(true);
        info.setFQDN(TEST_FQDN);
        info.setProviderFriendlyName(TEST_PROVIDER_NAME);
        info.setRequestingPackageName(TEST_PACKAGE_NAME);
        info.setWifiStandard(TEST_WIFI_STANDARD);
        info.setMaxSupportedTxLinkSpeedMbps(TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS);
        info.setMaxSupportedRxLinkSpeedMbps(TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS);
        info.setSubscriptionId(TEST_SUB_ID);
        info.setInformationElements(informationElements);
        info.setIsPrimary(true);
        info.setMacAddress(TEST_BSSID);
        return info;
    }

    private void assertNoRedaction(WifiInfo info,
            List<ScanResult.InformationElement> informationElements) {
        assertEquals(TEST_TX_SUCCESS, info.txSuccess);
        assertEquals(TEST_TX_RETRIES, info.txRetries);
        assertEquals(TEST_TX_BAD, info.txBad);
        assertEquals(TEST_RX_SUCCESS, info.rxSuccess);
        assertEquals("\"" + TEST_SSID + "\"", info.getSSID());
        assertEquals(TEST_BSSID, info.getBSSID());
        assertEquals(TEST_NETWORK_ID, info.getNetworkId());
        assertTrue(info.isTrusted());
        assertTrue(info.isOsuAp());
        assertTrue(info.isPasspointAp());
        assertEquals(TEST_PACKAGE_NAME, info.getRequestingPackageName());
        assertEquals(TEST_FQDN, info.getPasspointFqdn());
        assertEquals(TEST_PROVIDER_NAME, info.getPasspointProviderFriendlyName());
        assertEquals(TEST_WIFI_STANDARD, info.getWifiStandard());
        assertEquals(TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS, info.getMaxSupportedTxLinkSpeedMbps());
        assertEquals(TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS, info.getMaxSupportedRxLinkSpeedMbps());
        assertEquals(TEST_BSSID, info.getMacAddress());
        assertEquals(2, info.getInformationElements().size());
        assertEquals(informationElements.get(0).id,
                info.getInformationElements().get(0).id);
        assertEquals(informationElements.get(0).idExt,
                info.getInformationElements().get(0).idExt);
        assertArrayEquals(informationElements.get(0).bytes,
                info.getInformationElements().get(0).bytes);
        assertEquals(informationElements.get(1).id,
                info.getInformationElements().get(1).id);
        assertEquals(informationElements.get(1).idExt,
                info.getInformationElements().get(1).idExt);
        assertArrayEquals(informationElements.get(1).bytes,
                info.getInformationElements().get(1).bytes);
        if (SdkLevel.isAtLeastS()) {
            assertTrue(info.isOemPaid());
            assertTrue(info.isOemPrivate());
            assertTrue(info.isCarrierMerged());
            assertEquals(TEST_SUB_ID, info.getSubscriptionId());
            assertTrue(info.isPrimary());
        }
    }

    /**
     *  Verify redaction of WifiInfo with REDACT_NONE.
     */
    @Test
    public void testWifiInfoRedactNoRedactions() throws Exception {
        List<ScanResult.InformationElement> informationElements = generateIes();
        WifiInfo writeWifiInfo = makeWifiInfoForNoRedactions(informationElements);

        // Make a copy which allows parcelling of location sensitive data.
        WifiInfo redactedWifiInfo = writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_NONE);

        Parcel parcel = Parcel.obtain();
        redactedWifiInfo.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);

        assertNoRedaction(redactedWifiInfo, informationElements);
        assertNoRedaction(readWifiInfo, informationElements);

        if (SdkLevel.isAtLeastS()) {
            // equals() was only introduced in S.
            assertEquals(readWifiInfo, redactedWifiInfo);
        }
    }

    private WifiInfo makeWifiInfoForLocationSensitiveRedaction() {
        WifiInfo info = new WifiInfo();
        info.txSuccess = TEST_TX_SUCCESS;
        info.txRetries = TEST_TX_RETRIES;
        info.txBad = TEST_TX_BAD;
        info.rxSuccess = TEST_RX_SUCCESS;
        info.setSSID(WifiSsid.createFromAsciiEncoded(TEST_SSID));
        info.setBSSID(TEST_BSSID);
        info.setNetworkId(TEST_NETWORK_ID);
        info.setTrusted(true);
        info.setOemPaid(true);
        info.setOemPrivate(true);
        info.setCarrierMerged(true);
        info.setOsuAp(true);
        info.setFQDN(TEST_FQDN);
        info.setProviderFriendlyName(TEST_PROVIDER_NAME);
        info.setRequestingPackageName(TEST_PACKAGE_NAME);
        info.setWifiStandard(TEST_WIFI_STANDARD);
        info.setMaxSupportedTxLinkSpeedMbps(TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS);
        info.setMaxSupportedRxLinkSpeedMbps(TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS);
        info.setSubscriptionId(TEST_SUB_ID);
        info.setInformationElements(generateIes());
        info.setIsPrimary(true);
        info.setMacAddress(TEST_BSSID);
        return info;
    }

    private void assertLocationSensitiveRedaction(WifiInfo info) {
        assertNotNull(info);
        assertEquals(TEST_TX_SUCCESS, info.txSuccess);
        assertEquals(TEST_TX_RETRIES, info.txRetries);
        assertEquals(TEST_TX_BAD, info.txBad);
        assertEquals(TEST_RX_SUCCESS, info.rxSuccess);
        assertEquals(WifiManager.UNKNOWN_SSID, info.getSSID());
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, info.getBSSID());
        assertEquals(WifiConfiguration.INVALID_NETWORK_ID, info.getNetworkId());
        assertTrue(info.isTrusted());
        assertTrue(info.isOsuAp());
        assertFalse(info.isPasspointAp()); // fqdn & friendly name is masked.
        assertEquals(TEST_PACKAGE_NAME, info.getRequestingPackageName());
        assertNull(info.getPasspointFqdn());
        assertNull(info.getPasspointProviderFriendlyName());
        assertEquals(TEST_WIFI_STANDARD, info.getWifiStandard());
        assertEquals(TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS, info.getMaxSupportedTxLinkSpeedMbps());
        assertEquals(TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS, info.getMaxSupportedRxLinkSpeedMbps());
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, info.getMacAddress());
        assertNull(info.getInformationElements());
        if (SdkLevel.isAtLeastS()) {
            assertTrue(info.isOemPaid());
            assertTrue(info.isOemPrivate());
            assertTrue(info.isCarrierMerged());
            assertEquals(TEST_SUB_ID, info.getSubscriptionId());
            assertTrue(info.isPrimary());
        }
    }

    /**
     *  Verify redaction of WifiInfo with REDACT_FOR_ACCESS_FINE_LOCATION.
     */
    @Test
    public void testWifiInfoRedactLocationSensitiveInfo() throws Exception {
        WifiInfo writeWifiInfo = makeWifiInfoForLocationSensitiveRedaction();

        WifiInfo redactedWifiInfo =
                writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_FOR_ACCESS_FINE_LOCATION);

        Parcel parcel = Parcel.obtain();
        redactedWifiInfo.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);

        assertLocationSensitiveRedaction(redactedWifiInfo);
        assertLocationSensitiveRedaction(readWifiInfo);

        if (SdkLevel.isAtLeastS()) {
            // equals() was only introduced in S.
            assertEquals(redactedWifiInfo, readWifiInfo);
        }
    }

    /**
     *  Verify redaction of WifiInfo with REDACT_FOR_LOCAL_MAC_ADDRESS.
     */
    @Test
    public void testWifiInfoRedactLocalMacAddressInfo() throws Exception {
        WifiInfo writeWifiInfo = new WifiInfo();
        writeWifiInfo.setMacAddress(TEST_BSSID);

        WifiInfo redactedWifiInfo =
                writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_FOR_LOCAL_MAC_ADDRESS);

        Parcel parcel = Parcel.obtain();
        redactedWifiInfo.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);

        assertNotNull(redactedWifiInfo);
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, redactedWifiInfo.getMacAddress());

        assertNotNull(readWifiInfo);
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, readWifiInfo.getMacAddress());

        if (SdkLevel.isAtLeastS()) {
            // equals() was only introduced in S.
            assertEquals(redactedWifiInfo, readWifiInfo);
        }
    }

    private void assertIsPrimaryThrowsSecurityException(WifiInfo info) {
        try {
            // Should generate a security exception if caller does not have network settings
            // permission.
            info.isPrimary();
            fail();
        } catch (SecurityException e) { /* pass */ }
    }

    /**
     *  Verify redaction of WifiInfo with REDACT_FOR_NETWORK_SETTINGS.
     */
    @Test
    public void testWifiInfoRedactNetworkSettingsInfo() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());

        WifiInfo writeWifiInfo = new WifiInfo();
        writeWifiInfo.setIsPrimary(true);

        WifiInfo redactedWifiInfo =
                writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_FOR_NETWORK_SETTINGS);

        Parcel parcel = Parcel.obtain();
        redactedWifiInfo.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);

        assertNotNull(redactedWifiInfo);
        assertIsPrimaryThrowsSecurityException(redactedWifiInfo);
        assertNotNull(readWifiInfo);
        assertIsPrimaryThrowsSecurityException(readWifiInfo);

        if (SdkLevel.isAtLeastS()) {
            // equals() was only introduced in S.
            assertEquals(redactedWifiInfo, readWifiInfo);
        }
    }

    @Test
    public void testWifiInfoGetApplicableRedactions() throws Exception {
        long redactions = new WifiInfo().getApplicableRedactions();
        assertEquals(NetworkCapabilities.REDACT_FOR_ACCESS_FINE_LOCATION
                | NetworkCapabilities.REDACT_FOR_LOCAL_MAC_ADDRESS
                | NetworkCapabilities.REDACT_FOR_NETWORK_SETTINGS, redactions);
    }

    private WifiInfo makeWifiInfoForLocationSensitiveAndLocalMacAddressRedaction() {
        WifiInfo info = new WifiInfo();
        info.setSSID(WifiSsid.createFromAsciiEncoded(TEST_SSID));
        info.setBSSID(TEST_BSSID);
        info.setNetworkId(TEST_NETWORK_ID);
        info.setFQDN(TEST_FQDN);
        info.setProviderFriendlyName(TEST_PROVIDER_NAME);
        info.setInformationElements(generateIes());
        info.setMacAddress(TEST_BSSID);
        return info;
    }

    private void assertLocationSensitiveAndLocalMacAddressRedaction(WifiInfo info) {
        assertNotNull(info);
        assertEquals(WifiManager.UNKNOWN_SSID, info.getSSID());
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, info.getBSSID());
        assertEquals(WifiConfiguration.INVALID_NETWORK_ID, info.getNetworkId());
        assertNull(info.getPasspointFqdn());
        assertNull(info.getPasspointProviderFriendlyName());
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, info.getMacAddress());
        assertNull(info.getInformationElements());
    }

    @Test
    public void testWifiInfoRedactLocationAndLocalMacAddressSensitiveInfo()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());

        WifiInfo writeWifiInfo = makeWifiInfoForLocationSensitiveAndLocalMacAddressRedaction();

        WifiInfo redactedWifiInfo =
                writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_FOR_ACCESS_FINE_LOCATION
                        | NetworkCapabilities.REDACT_FOR_LOCAL_MAC_ADDRESS);

        Parcel parcel = Parcel.obtain();
        redactedWifiInfo.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);

        assertLocationSensitiveAndLocalMacAddressRedaction(redactedWifiInfo);
        assertLocationSensitiveAndLocalMacAddressRedaction(readWifiInfo);

        if (SdkLevel.isAtLeastS()) {
            // equals() was only introduced in S.
            assertEquals(redactedWifiInfo, readWifiInfo);
        }
    }

    /**
     *  Verify parcel write/read with null information elements.
     */
    @Test
    public void testWifiInfoParcelWriteReadWithNullInfoElements() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());

        WifiInfo writeWifiInfo = new WifiInfo();
        writeWifiInfo.setInformationElements(null);

        // Make a copy which allows parcelling of location sensitive data.
        WifiInfo writeWifiInfoCopy = writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_NONE);

        Parcel parcel = Parcel.obtain();
        writeWifiInfoCopy.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);
        assertNull(readWifiInfo.getInformationElements());
    }

    /**
     *  Verify parcel write/read with empty information elements.
     */
    @Test
    public void testWifiInfoParcelWriteReadWithEmptyInfoElements() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());

        WifiInfo writeWifiInfo = new WifiInfo();
        writeWifiInfo.setInformationElements(new ArrayList<>());

        // Make a copy which allows parcelling of location sensitive data.
        WifiInfo writeWifiInfoCopy = writeWifiInfo.makeCopy(NetworkCapabilities.REDACT_NONE);

        Parcel parcel = Parcel.obtain();
        writeWifiInfoCopy.writeToParcel(parcel, 0);
        // Rewind the pointer to the head of the parcel.
        parcel.setDataPosition(0);
        WifiInfo readWifiInfo = WifiInfo.CREATOR.createFromParcel(parcel);
        assertTrue(readWifiInfo.getInformationElements().isEmpty());
    }

    @Test
    public void testWifiInfoCopyConstructor() throws Exception {
        WifiInfo writeWifiInfo = new WifiInfo();
        writeWifiInfo.txSuccess = TEST_TX_SUCCESS;
        writeWifiInfo.txRetries = TEST_TX_RETRIES;
        writeWifiInfo.txBad = TEST_TX_BAD;
        writeWifiInfo.rxSuccess = TEST_RX_SUCCESS;
        writeWifiInfo.setTrusted(true);
        writeWifiInfo.setOemPaid(true);
        writeWifiInfo.setOemPrivate(true);
        writeWifiInfo.setCarrierMerged(true);
        writeWifiInfo.setOsuAp(true);
        writeWifiInfo.setFQDN(TEST_FQDN);
        writeWifiInfo.setProviderFriendlyName(TEST_PROVIDER_NAME);
        writeWifiInfo.setRequestingPackageName(TEST_PACKAGE_NAME);
        writeWifiInfo.setWifiStandard(TEST_WIFI_STANDARD);
        writeWifiInfo.setMaxSupportedTxLinkSpeedMbps(TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS);
        writeWifiInfo.setMaxSupportedRxLinkSpeedMbps(TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS);
        writeWifiInfo.setSubscriptionId(TEST_SUB_ID);
        writeWifiInfo.setIsPrimary(true);

        WifiInfo readWifiInfo = new WifiInfo(writeWifiInfo);

        assertEquals(TEST_TX_SUCCESS, readWifiInfo.txSuccess);
        assertEquals(TEST_TX_RETRIES, readWifiInfo.txRetries);
        assertEquals(TEST_TX_BAD, readWifiInfo.txBad);
        assertEquals(TEST_RX_SUCCESS, readWifiInfo.rxSuccess);
        assertTrue(readWifiInfo.isTrusted());
        assertTrue(readWifiInfo.isOsuAp());
        assertTrue(readWifiInfo.isPasspointAp());
        assertEquals(TEST_PACKAGE_NAME, readWifiInfo.getRequestingPackageName());
        assertEquals(TEST_FQDN, readWifiInfo.getPasspointFqdn());
        assertEquals(TEST_PROVIDER_NAME, readWifiInfo.getPasspointProviderFriendlyName());
        assertEquals(TEST_WIFI_STANDARD, readWifiInfo.getWifiStandard());
        assertEquals(TEST_MAX_SUPPORTED_TX_LINK_SPEED_MBPS,
                readWifiInfo.getMaxSupportedTxLinkSpeedMbps());
        assertEquals(TEST_MAX_SUPPORTED_RX_LINK_SPEED_MBPS,
                readWifiInfo.getMaxSupportedRxLinkSpeedMbps());
        if (SdkLevel.isAtLeastS()) {
            assertTrue(readWifiInfo.isOemPaid());
            assertTrue(readWifiInfo.isOemPrivate());
            assertTrue(readWifiInfo.isCarrierMerged());
            assertEquals(TEST_SUB_ID, readWifiInfo.getSubscriptionId());
            assertTrue(readWifiInfo.isPrimary());
        }
    }

    /**
     *  Verify values after reset()
     */
    @Test
    public void testWifiInfoResetValue() throws Exception {
        WifiInfo wifiInfo = new WifiInfo();
        wifiInfo.reset();
        assertEquals(WifiInfo.LINK_SPEED_UNKNOWN, wifiInfo.getMaxSupportedTxLinkSpeedMbps());
        assertEquals(WifiInfo.LINK_SPEED_UNKNOWN, wifiInfo.getMaxSupportedRxLinkSpeedMbps());
        assertEquals(WifiInfo.LINK_SPEED_UNKNOWN, wifiInfo.getTxLinkSpeedMbps());
        assertEquals(WifiInfo.LINK_SPEED_UNKNOWN, wifiInfo.getRxLinkSpeedMbps());
        assertEquals(WifiInfo.INVALID_RSSI, wifiInfo.getRssi());
        assertEquals(WifiManager.UNKNOWN_SSID, wifiInfo.getSSID());
        assertEquals(null, wifiInfo.getBSSID());
        assertEquals(-1, wifiInfo.getNetworkId());
        if (SdkLevel.isAtLeastS()) {
            assertFalse(wifiInfo.isOemPaid());
            assertFalse(wifiInfo.isOemPrivate());
            assertFalse(wifiInfo.isCarrierMerged());
            assertEquals(SubscriptionManager.INVALID_SUBSCRIPTION_ID, wifiInfo.getSubscriptionId());
            assertFalse(wifiInfo.isPrimary());
        }
    }

    /**
     * Test that the WifiInfo Builder returns the same values that was set, and that
     * calling build multiple times returns different instances.
     */
    @Test
    public void testWifiInfoBuilder() throws Exception {
        WifiInfo.Builder builder = new WifiInfo.Builder()
                .setSsid(TEST_SSID.getBytes(StandardCharsets.UTF_8))
                .setBssid(TEST_BSSID)
                .setRssi(TEST_RSSI)
                .setNetworkId(TEST_NETWORK_ID);

        WifiInfo info1 = builder.build();

        assertEquals("\"" + TEST_SSID + "\"", info1.getSSID());
        assertEquals(TEST_BSSID, info1.getBSSID());
        assertEquals(TEST_RSSI, info1.getRssi());
        assertEquals(TEST_NETWORK_ID, info1.getNetworkId());

        WifiInfo info2 = builder
                .setNetworkId(TEST_NETWORK_ID2)
                .build();

        // different instances
        assertNotSame(info1, info2);

        // assert that info1 didn't change
        assertEquals("\"" + TEST_SSID + "\"", info1.getSSID());
        assertEquals(TEST_BSSID, info1.getBSSID());
        assertEquals(TEST_RSSI, info1.getRssi());
        assertEquals(TEST_NETWORK_ID, info1.getNetworkId());

        // assert that info2 changed
        assertEquals("\"" + TEST_SSID + "\"", info2.getSSID());
        assertEquals(TEST_BSSID, info2.getBSSID());
        assertEquals(TEST_RSSI, info2.getRssi());
        assertEquals(TEST_NETWORK_ID2, info2.getNetworkId());
    }

    @Test
    public void testWifiInfoEquals() throws Exception {
        WifiInfo.Builder builder = new WifiInfo.Builder()
                .setSsid(TEST_SSID.getBytes(StandardCharsets.UTF_8))
                .setBssid(TEST_BSSID)
                .setRssi(TEST_RSSI)
                .setNetworkId(TEST_NETWORK_ID);

        WifiInfo info1 = builder.build();
        WifiInfo info2 = builder.build();
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1, info2);
        } else {
            // On R devices, reference equality.
            assertNotEquals(info1, info2);
        }

        info1.setSubscriptionId(TEST_SUB_ID);
        assertNotEquals(info1, info2);

        info2.setSubscriptionId(TEST_SUB_ID);
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1, info2);
        } else {
            // On R devices, reference equality.
            assertNotEquals(info1, info2);
        }

        info1.setSSID(WifiSsid.createFromHex(null));
        assertNotEquals(info1, info2);

        info2.setSSID(WifiSsid.createFromHex(null));
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1, info2);
        } else {
            // On R devices, reference equality.
            assertNotEquals(info1, info2);
        }
    }

    @Test
    public void testWifiInfoEqualsWithInfoElements() throws Exception {
        WifiInfo.Builder builder = new WifiInfo.Builder()
                .setSsid(TEST_SSID.getBytes(StandardCharsets.UTF_8))
                .setBssid(TEST_BSSID)
                .setRssi(TEST_RSSI)
                .setNetworkId(TEST_NETWORK_ID);

        WifiInfo info1 = builder.build();
        WifiInfo info2 = builder.build();
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1, info2);
        } else {
            // On R devices, reference equality.
            assertNotEquals(info1, info2);
        }

        info1.setInformationElements(generateIes());
        info2.setInformationElements(generateIes());

        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1, info2);
        } else {
            // On R devices, reference equality.
            assertNotEquals(info1, info2);
        }
    }

    @Test
    public void testWifiInfoHashcode() throws Exception {
        WifiInfo.Builder builder = new WifiInfo.Builder()
                .setSsid(TEST_SSID.getBytes(StandardCharsets.UTF_8))
                .setBssid(TEST_BSSID)
                .setRssi(TEST_RSSI)
                .setNetworkId(TEST_NETWORK_ID);

        WifiInfo info1 = builder.build();
        WifiInfo info2 = builder.build();
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1.hashCode(), info2.hashCode());
        } else {
            // On R devices, system generated hashcode.
            assertNotEquals(info1.hashCode(), info2.hashCode());
        }

        info1.setSubscriptionId(TEST_SUB_ID);
        assertNotEquals(info1.hashCode(), info2.hashCode());

        info2.setSubscriptionId(TEST_SUB_ID);
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1.hashCode(), info2.hashCode());
        } else {
            // On R devices, system generated hashcode.
            assertNotEquals(info1.hashCode(), info2.hashCode());
        }

        info1.setSSID(WifiSsid.createFromHex(null));
        assertNotEquals(info1.hashCode(), info2.hashCode());

        info2.setSSID(WifiSsid.createFromHex(null));
        if (SdkLevel.isAtLeastS()) {
            assertEquals(info1.hashCode(), info2.hashCode());
        } else {
            // On R devices, system generated hashcode.
            assertNotEquals(info1.hashCode(), info2.hashCode());
        }
    }

    @Test
    public void testWifiInfoCurrentSecurityType() throws Exception {
        WifiInfo.Builder builder = new WifiInfo.Builder()
                .setSsid(TEST_SSID.getBytes(StandardCharsets.UTF_8))
                .setBssid(TEST_BSSID)
                .setRssi(TEST_RSSI)
                .setNetworkId(TEST_NETWORK_ID)
                .setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_SAE);

        WifiInfo info = new WifiInfo();
        assertEquals(WifiInfo.SECURITY_TYPE_UNKNOWN, info.getCurrentSecurityType());

        info = builder.build();
        assertEquals(WifiInfo.SECURITY_TYPE_SAE, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_OPEN).build();
        assertEquals(WifiInfo.SECURITY_TYPE_OPEN, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_WEP).build();
        assertEquals(WifiInfo.SECURITY_TYPE_WEP, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_PSK).build();
        assertEquals(WifiInfo.SECURITY_TYPE_PSK, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_EAP).build();
        assertEquals(WifiInfo.SECURITY_TYPE_EAP, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_OWE).build();
        assertEquals(WifiInfo.SECURITY_TYPE_OWE, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_WAPI_PSK).build();
        assertEquals(WifiInfo.SECURITY_TYPE_WAPI_PSK, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_WAPI_CERT).build();
        assertEquals(WifiInfo.SECURITY_TYPE_WAPI_CERT, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(
                WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE).build();
        assertEquals(WifiInfo.SECURITY_TYPE_EAP_WPA3_ENTERPRISE, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(
                WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT).build();
        assertEquals(WifiInfo.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT,
                info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(
                WifiConfiguration.SECURITY_TYPE_PASSPOINT_R1_R2).build();
        assertEquals(WifiInfo.SECURITY_TYPE_PASSPOINT_R1_R2, info.getCurrentSecurityType());

        info = builder.setCurrentSecurityType(WifiConfiguration.SECURITY_TYPE_PASSPOINT_R3).build();
        assertEquals(WifiInfo.SECURITY_TYPE_PASSPOINT_R3, info.getCurrentSecurityType());

        info.clearCurrentSecurityType();
        assertEquals(WifiInfo.SECURITY_TYPE_UNKNOWN, info.getCurrentSecurityType());
    }

    private static List<ScanResult.InformationElement> generateIes() {
        List<ScanResult.InformationElement> informationElements = new ArrayList<>();
        ScanResult.InformationElement informationElement = new ScanResult.InformationElement();
        informationElement.id = ScanResult.InformationElement.EID_HT_OPERATION;
        informationElement.idExt = 0;
        informationElement.bytes = new byte[]{0x11, 0x22, 0x33};
        informationElements.add(informationElement);

        informationElement = new ScanResult.InformationElement();
        informationElement.id = ScanResult.InformationElement.EID_EXTENSION_PRESENT;
        informationElement.idExt = ScanResult.InformationElement.EID_EXT_HE_OPERATION;
        informationElement.bytes = new byte[]{0x44, 0x55, 0x66};
        informationElements.add(informationElement);

        return informationElements;
    }
}
