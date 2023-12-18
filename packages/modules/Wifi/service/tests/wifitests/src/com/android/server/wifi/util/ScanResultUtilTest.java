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

package com.android.server.wifi.util;

import static org.junit.Assert.*;

import android.net.wifi.ScanResult;
import android.net.wifi.ScanResult.InformationElement;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiSsid;

import androidx.test.filters.SmallTest;

import com.android.server.wifi.ScanDetail;
import com.android.server.wifi.WifiBaseTest;
import com.android.server.wifi.hotspot2.NetworkDetail;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link com.android.server.wifi.util.ScanResultUtil}.
 */
@SmallTest
public class ScanResultUtilTest extends WifiBaseTest {

    @Test
    public void convertScanResult() {
        final String ssid = "SOME SsId";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, "", -78, 2450, 1025, 22, 33, 20, 0, 0, true);

        input.informationElements = new InformationElement[] {
            createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        ScanDetail output = ScanResultUtil.toScanDetail(input);

        validateScanDetail(input, output);
    }

    @Test
    public void convertScanResultWithAnqpLines() {
        final String ssid = "SOME SsId";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, "some caps", -78, 2450, 1025, 22, 33, 20, 0, 0, true);

        input.informationElements = new InformationElement[] {
            createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };
        input.anqpLines = Arrays.asList("LINE 1", "line 2", "Line 3");

        ScanDetail output = ScanResultUtil.toScanDetail(input);

        validateScanDetail(input, output);
    }

    @Test
    public void convertScanResultWithoutWifiSsid() {
        final String ssid = "Another SSid";
        ScanResult input = new ScanResult(ssid, "ab:cd:01:ef:45:89", 1245, 0, "other caps",
                -78, 2450, 1025, 22, 33, 20, 0, 0, true);
        input.informationElements = new InformationElement[] {
            createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        ScanDetail output = ScanResultUtil.toScanDetail(input);

        validateScanDetail(input, output);
    }

    @Test
    public void testNetworkCreationFromScanResult() {
        final String ssid = "Another SSid";
        ScanResult scanResult = new ScanResult(ssid, "ab:cd:01:ef:45:89", 1245, 0, "",
                -78, 2450, 1025, 22, 33, 20, 0, 0, true);
        scanResult.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };
        WifiConfiguration config;

        scanResult.capabilities = "";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_OPEN,
                config.getDefaultSecurityParams().getSecurityType());

        scanResult.capabilities = "WEP";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_WEP,
                config.getDefaultSecurityParams().getSecurityType());

        scanResult.capabilities = "PSK";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_PSK,
                config.getDefaultSecurityParams().getSecurityType());

        // WPA2 Enterprise network with none MFP capability.
        scanResult.capabilities = "[EAP/SHA1]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP,
                config.getDefaultSecurityParams().getSecurityType());

        // WPA2 Enterprise network with MFPC.
        scanResult.capabilities = "[EAP/SHA1][MFPC]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP,
                config.getDefaultSecurityParams().getSecurityType());

        // WPA2 Enterprise network with MFPR.
        scanResult.capabilities = "[EAP/SHA1][MFPR]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP,
                config.getDefaultSecurityParams().getSecurityType());

        // WPA3 Enterprise transition network
        scanResult.capabilities = "[RSN-EAP/SHA1+EAP/SHA256][MFPC]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP,
                config.getDefaultSecurityParams().getSecurityType());
        assertTrue(config.isSecurityType(WifiConfiguration.SECURITY_TYPE_EAP));
        assertTrue(config.isSecurityType(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE));

        // WPA3 Enterprise only network
        scanResult.capabilities = "[RSN-EAP/SHA256][MFPC][MFPR]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE,
                config.getDefaultSecurityParams().getSecurityType());

        // Neither a valid WPA3 Enterprise transition network nor WPA3 Enterprise only network
        // Fallback to WPA2 Enterprise
        scanResult.capabilities = "[RSN-EAP/SHA1+EAP/SHA256][MFPC][MFPR]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP,
                config.getDefaultSecurityParams().getSecurityType());

        // WPA3 Enterprise only network
        scanResult.capabilities = "[RSN-SUITE_B_192][MFPR]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT,
                config.getDefaultSecurityParams().getSecurityType());

        scanResult.capabilities = "WAPI-PSK";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_WAPI_PSK,
                config.getDefaultSecurityParams().getSecurityType());

        scanResult.capabilities = "WAPI-CERT";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertEquals(config.SSID, ScanResultUtil.createQuotedSSID(ssid));
        assertEquals(WifiConfiguration.SECURITY_TYPE_WAPI_CERT,
                config.getDefaultSecurityParams().getSecurityType());
    }

    /**
     * Test that a PSK-SHA256+SAE network is detected as transition mode
     */
    @Test
    public void testPskSha256SaeTransitionModeCheck() {
        final String ssid = "WPA3-Transition";
        String caps = "[WPA2-FT/PSK-CCMP][RSN-FT/PSK+PSK-SHA256+SAE+FT/SAE-CCMP][ESS][WPS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertTrue(ScanResultUtil.isScanResultForPskSaeTransitionNetwork(input));
    }

    /**
     * Test that a PSK+SAE network is detected as transition mode
     */
    @Test
    public void testPskSaeTransitionModeCheck() {
        final String ssid = "WPA3-Transition";
        String caps = "[WPA2-FT/PSK+PSK+SAE+FT/SAE-CCMP][ESS][WPS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertTrue(ScanResultUtil.isScanResultForPskSaeTransitionNetwork(input));
    }

    /**
     * Test that a PSK network is not detected as transition mode
     */
    @Test
    public void testPskNotInTransitionModeCheck() {
        final String ssid = "WPA2-Network";
        String caps = "[WPA2-FT/PSK+PSK][ESS][WPS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertFalse(ScanResultUtil.isScanResultForPskSaeTransitionNetwork(input));
    }

    /**
     * Test that an SAE network is not detected as transition mode
     */
    @Test
    public void testSaeNotInTransitionModeCheck() {
        final String ssid = "WPA3-Network";
        String caps = "[WPA2-FT/SAE+SAE][ESS][WPS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertFalse(ScanResultUtil.isScanResultForPskSaeTransitionNetwork(input));
    }

    /**
     * Test that provided network supports FT/EAP AKM.
     */
    @Test
    public void testFtEapAkmSupportedNetwork() {
        final String ssid = "FT-EAP-AP";
        String caps = " [WPA2-FT/EAP-CCMP][RSN-FT/EAP-CCMP][ESS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertTrue(ScanResultUtil.isScanResultForEapNetwork(input));
    }

    /**
     * Test that provided network supports FILS SHA256 AKM.
     */
    @Test
    public void testFilsSha256AkmSupportedNetwork() {
        final String ssid = "FILS-AP";
        String caps = "[WPA2-EAP-FILS-SHA256-CCMP]"
                + "[RSN-EAP-FILS-SHA256-CCMP][ESS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertTrue(ScanResultUtil.isScanResultForEapNetwork(input));
        assertTrue(ScanResultUtil.isScanResultForFilsSha256Network(input));
    }

    /**
     * Test that provided network supports FILS SHA384 AKM.
     */
    @Test
    public void testFilsSha384AkmSupportedNetwork() {
        final String ssid = "FILS-AP";
        String caps = "[WPA2-EAP-FILS-SHA384-CCMP]"
                + "[RSN-EAP-FILS-SHA384-CCMP][ESS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertTrue(ScanResultUtil.isScanResultForEapNetwork(input));
        assertTrue(ScanResultUtil.isScanResultForFilsSha384Network(input));
    }

    /**
     * Test that an EAP network is not detected as a Pasppoint network.
     */
    @Test
    public void testEapNetworkNotPasspointNetwork() {
        final String ssid = "EAP-NETWORK";
        String caps = "[EAP/SHA1][ESS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertTrue(ScanResultUtil.isScanResultForEapNetwork(input));
        assertFalse(ScanResultUtil.isScanResultForPasspointR1R2Network(input));
        assertFalse(ScanResultUtil.isScanResultForPasspointR3Network(input));
    }

    private void verifyPasspointNetwork(
            String caps, boolean isInterworking, NetworkDetail.HSRelease hsRel,
            boolean isR1R2Network, boolean isR3Network) {
        final String ssid = "PASSPOINT-NETWORK";
        List<InformationElement> ies = new ArrayList<>();
        ies.add(createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8)));
        if (isInterworking) {
            ies.add(createIE(InformationElement.EID_INTERWORKING, new byte[] {(byte) 0x10}));
        }
        if (null != hsRel) {
            byte releaseByte = (byte) 0xff;
            switch (hsRel) {
                case R1:
                    releaseByte = (byte) 0x0;
                    break;
                case R2:
                    releaseByte = (byte) 0x10;
                    break;
                case R3:
                    releaseByte = (byte) 0x20;
                    break;
            }
            ies.add(
                    createIE(InformationElement.EID_VSA, new byte[] {
                            (byte) 0x50, (byte) 0x6f, (byte) 0x9a, (byte) 0x10, releaseByte}));
        }

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);
        input.informationElements = new InformationElement[ies.size()];
        input.informationElements = ies.toArray(input.informationElements);

        assertTrue(ScanResultUtil.isScanResultForEapNetwork(input));
        assertEquals(isR1R2Network, ScanResultUtil.isScanResultForPasspointR1R2Network(input));
        assertEquals(isR3Network, ScanResultUtil.isScanResultForPasspointR3Network(input));
    }

    /**
     * Test that a Passpoint R1 network is detected correctly.
     */
    @Test
    public void testPasspointR1NetworkCheck() {
        String caps = "[EAP/SHA1][ESS]";
        verifyPasspointNetwork(caps, true, NetworkDetail.HSRelease.R1, true, false);
    }

    /**
     * Test that a Passpoint R2 network is detected correctly.
     */
    @Test
    public void testPasspointR2NetworkCheck() {
        String caps = "[EAP/SHA1][ESS]";
        verifyPasspointNetwork(caps, true, NetworkDetail.HSRelease.R2, true, false);
    }

    /**
     * Test that a Passpoint R3 network is detected correctly.
     */
    @Test
    public void testPasspointR3NetworkCheck() {
        String caps = "[EAP/SHA1][ESS][MFPR]";
        // R3 network is also a valid R1/R2 network.
        verifyPasspointNetwork(caps, true, NetworkDetail.HSRelease.R3, true, true);
    }

    /**
     * Test that an EAP network with HS release, but no interwork set.
     */
    @Test
    public void testEapNetworkWithoutInterworkNotAPasspointNetwork() {
        String caps = "[EAP/SHA1][ESS]";
        verifyPasspointNetwork(caps, false, NetworkDetail.HSRelease.R3, false, false);
    }

    /**
     * Test that an EAP network with interwork set, but no HS release.
     */
    @Test
    public void testEapNetworkWithoutHsReleaseNotAPasspointNetwork() {
        String caps = "[EAP/SHA1][ESS]";
        verifyPasspointNetwork(caps, true, null, false, false);
    }

    /**
     * Test that an unknown AMK network is not detected as an open network.
     */
    @Test
    public void testUnknownAkmNotOpenNetwork() {
        final String ssid = "UnknownAkm-Network";
        String caps = "[RSN-?-TKIP+CCMP][ESS][WPS]";

        ScanResult input = new ScanResult(WifiSsid.createFromAsciiEncoded(ssid), ssid,
                "ab:cd:01:ef:45:89", 1245, 0, caps, -78, 2450, 1025, 22, 33, 20, 0,
                0, true);

        input.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };

        assertFalse(ScanResultUtil.isScanResultForOpenNetwork(input));
    }

    /**
     * Verify ScanResultList validation.
     */
    @Test
    public void testValidateScanResultList() {
        List<ScanResult> scanResults = new ArrayList<>();
        assertFalse(ScanResultUtil.validateScanResultList(null));
        assertFalse(ScanResultUtil.validateScanResultList(scanResults));
        scanResults.add(null);
        assertFalse(ScanResultUtil.validateScanResultList(scanResults));
        ScanResult scanResult = new ScanResult();
        scanResults.clear();
        scanResults.add(scanResult);
        assertFalse(ScanResultUtil.validateScanResultList(scanResults));
        scanResult.SSID = "test";
        scanResult.capabilities = "[RSN-PSK-CCMP]";
        scanResult.BSSID = "ab:cd:01:ef:45:89";
        assertTrue(ScanResultUtil.validateScanResultList(scanResults));
    }

    /**
     * Verify that unknown AKM in the scan result
     */
    @Test
    public void testUnknownAkmForSecurityParamsGeneration() {
        final String ssid = "Another SSid";
        ScanResult scanResult = new ScanResult(ssid, "ab:cd:01:ef:45:89", 1245, 0, "",
                -78, 2450, 1025, 22, 33, 20, 0, 0, true);
        scanResult.informationElements = new InformationElement[] {
                createIE(InformationElement.EID_SSID, ssid.getBytes(StandardCharsets.UTF_8))
        };
        WifiConfiguration config;

        scanResult.capabilities = "[RSN-?-CCMP]";
        config = ScanResultUtil.createNetworkFromScanResult(scanResult);
        assertNull(config);
    }

    private static InformationElement createIE(int id, byte[] bytes) {
        InformationElement ie = new InformationElement();
        ie.id = id;
        ie.bytes = bytes;
        return ie;
    }

    private static void validateScanDetail(ScanResult input, ScanDetail output) {
        assertNotNull("NetworkDetail was null", output.getNetworkDetail());
        assertNotNull("ScanResult was null", output.getScanResult());
        assertEquals("NetworkDetail SSID", input.SSID,
                output.getNetworkDetail().getSSID());
        assertEquals("ScanResult SSID", input.SSID,
                output.getScanResult().SSID);
        assertEquals("ScanResult wifiSsid", input.wifiSsid,
                output.getScanResult().wifiSsid);
        assertEquals("getSSID", input.SSID, output.getSSID());
        assertEquals("NetworkDetail BSSID", input.BSSID,
                output.getNetworkDetail().getBSSIDString());
        assertEquals("getBSSIDString", input.BSSID, output.getBSSIDString());
        assertEquals("ScanResult frequency", input.frequency,
                output.getScanResult().frequency);
        assertEquals("ScanResult level", input.level,
                output.getScanResult().level);
        assertEquals("ScanResult capabilities", input.capabilities,
                output.getScanResult().capabilities);
        assertEquals("ScanResult timestamp", input.timestamp,
                output.getScanResult().timestamp);
        assertArrayEquals("ScanResult information elements", input.informationElements,
                output.getScanResult().informationElements);
        assertEquals("ScanResult anqp lines", input.anqpLines,
                output.getScanResult().anqpLines);
    }

}
