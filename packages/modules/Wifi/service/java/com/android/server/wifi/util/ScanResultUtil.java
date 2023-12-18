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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.net.wifi.ScanResult;
import android.net.wifi.SecurityParams;
import android.net.wifi.WifiConfiguration;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.server.wifi.ScanDetail;
import com.android.server.wifi.hotspot2.NetworkDetail;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
/**
 * Scan result utility for any {@link ScanResult} related operations.
 * Currently contains:
 *   > Helper method for converting a ScanResult to a ScanDetail.
 *     Only fields that are supported in ScanResult are copied.
 *   > Helper methods to identify the encryption of a ScanResult.
 */
public class ScanResultUtil {
    private static final String TAG = "ScanResultUtil";
    private ScanResultUtil() { /* not constructable */ }

    /**
     * This method should only be used when the informationElements field in the provided scan
     * result is filled in with the IEs from the beacon.
     */
    public static ScanDetail toScanDetail(ScanResult scanResult) {
        ScanResult.InformationElement[] ieArray = (null != scanResult.informationElements)
            ? scanResult.informationElements
            : new ScanResult.InformationElement[0];
        NetworkDetail networkDetail = new NetworkDetail(scanResult.BSSID,
                ieArray, scanResult.anqpLines, scanResult.frequency);
        return new ScanDetail(scanResult, networkDetail);
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a PSK network or not.
     * This checks if the provided capabilities string contains PSK encryption type or not.
     */
    public static boolean isScanResultForPskNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("PSK");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a WAPI-PSK network or not.
     * This checks if the provided capabilities string contains PSK encryption type or not.
     */
    public static boolean isScanResultForWapiPskNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("WAPI-PSK");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a WAPI-CERT
     * network or not.
     * This checks if the provided capabilities string contains PSK encryption type or not.
     */
    public static boolean isScanResultForWapiCertNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("WAPI-CERT");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a EAP network or not.
     * This checks these conditions:
     * - Enable EAP/SHA1, EAP/SHA256 AKM, FT/EAP, or EAP-FILS.
     * - Not a WPA3 Enterprise only network.
     * - Not a WPA3 Enterprise transition network.
     */
    public static boolean isScanResultForEapNetwork(ScanResult scanResult) {
        return (scanResult.capabilities.contains("EAP/SHA1")
                        || scanResult.capabilities.contains("EAP/SHA256")
                        || scanResult.capabilities.contains("FT/EAP")
                        || scanResult.capabilities.contains("EAP-FILS"))
                && !isScanResultForWpa3EnterpriseOnlyNetwork(scanResult)
                && !isScanResultForWpa3EnterpriseTransitionNetwork(scanResult);
    }

    private static boolean isScanResultForPmfMandatoryNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("[MFPR]");
    }

    private static boolean isScanResultForPmfCapableNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("[MFPC]");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a Passpoint R1/R2
     * network or not.
     * Passpoint R1/R2 requirements:
     * - WPA2 Enterprise network.
     * - interworking bit is set.
     * - HotSpot Release presents.
     */
    public static boolean isScanResultForPasspointR1R2Network(ScanResult scanResult) {
        if (!isScanResultForEapNetwork(scanResult)) return false;

        ScanDetail detail = toScanDetail(scanResult);
        if (!detail.getNetworkDetail().isInterworking()) return false;
        return null != detail.getNetworkDetail().getHSRelease();
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a Passpoint R3
     * network or not.
     * Passpoint R3 requirements:
     * - Must be WPA2 Enterprise network, WPA3 Enterprise network,
     *   or WPA3 Enterprise 192-bit mode network.
     * - interworking bit is set.
     * - HotSpot Release presents.
     * - PMF is mandatory.
     */
    public static boolean isScanResultForPasspointR3Network(ScanResult scanResult) {
        if (!isScanResultForEapNetwork(scanResult)
                && !isScanResultForWpa3EnterpriseOnlyNetwork(scanResult)
                && !isScanResultForEapSuiteBNetwork(scanResult)) {
            return false;
        }
        if (!isScanResultForPmfMandatoryNetwork(scanResult)) return false;

        ScanDetail detail = toScanDetail(scanResult);
        if (!detail.getNetworkDetail().isInterworking()) return false;
        return null != detail.getNetworkDetail().getHSRelease();
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to
     * a WPA3 Enterprise transition network or not.
     *
     * See Section 3.3 WPA3-Enterprise transition mode in WPA3 Specification
     * - Enable at least EAP/SHA1 and EAP/SHA256 AKM suites.
     * - Not enable WPA1 version 1, WEP, and TKIP.
     * - Management Frame Protection Capable is set.
     * - Management Frame Protection Required is not set.
     */
    public static boolean isScanResultForWpa3EnterpriseTransitionNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("EAP/SHA1")
                && scanResult.capabilities.contains("EAP/SHA256")
                && scanResult.capabilities.contains("RSN")
                && !scanResult.capabilities.contains("WEP")
                && !scanResult.capabilities.contains("TKIP")
                && !isScanResultForPmfMandatoryNetwork(scanResult)
                && isScanResultForPmfCapableNetwork(scanResult);
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to
     * a WPA3 Enterprise only network or not.
     *
     * See Section 3.2 WPA3-Enterprise only mode in WPA3 Specification
     * - Enable at least EAP/SHA256 AKM suite.
     * - Not enable EAP/SHA1 AKM suite.
     * - Not enable WPA1 version 1, WEP, and TKIP.
     * - Management Frame Protection Capable is set.
     * - Management Frame Protection Required is set.
     */
    public static boolean isScanResultForWpa3EnterpriseOnlyNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("EAP/SHA256")
                && !scanResult.capabilities.contains("EAP/SHA1")
                && scanResult.capabilities.contains("RSN")
                && !scanResult.capabilities.contains("WEP")
                && !scanResult.capabilities.contains("TKIP")
                && isScanResultForPmfMandatoryNetwork(scanResult)
                && isScanResultForPmfCapableNetwork(scanResult);
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a WPA3-Enterprise 192-bit
     * mode network or not.
     * This checks if the provided capabilities comply these conditions:
     * - Enable SUITE-B-192 AKM.
     * - Not enable EAP/SHA1 AKM suite.
     * - Not enable WPA1 version 1, WEP, and TKIP.
     * - Management Frame Protection Required is set.
     */
    public static boolean isScanResultForEapSuiteBNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("SUITE_B_192")
                && scanResult.capabilities.contains("RSN")
                && !scanResult.capabilities.contains("WEP")
                && !scanResult.capabilities.contains("TKIP")
                && isScanResultForPmfMandatoryNetwork(scanResult);
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to a WEP network or not.
     * This checks if the provided capabilities string contains WEP encryption type or not.
     */
    public static boolean isScanResultForWepNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("WEP");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to OWE network.
     * This checks if the provided capabilities string contains OWE or not.
     */
    public static boolean isScanResultForOweNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("OWE");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to OWE transition network.
     * This checks if the provided capabilities string contains OWE_TRANSITION or not.
     */
    public static boolean isScanResultForOweTransitionNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("OWE_TRANSITION");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to SAE network.
     * This checks if the provided capabilities string contains SAE or not.
     */
    public static boolean isScanResultForSaeNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("SAE");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to PSK-SAE transition
     * network. This checks if the provided capabilities string contains both PSK and SAE or not.
     */
    public static boolean isScanResultForPskSaeTransitionNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("PSK") && scanResult.capabilities.contains("SAE");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to FILS SHA256 network.
     * This checks if the provided capabilities string contains FILS-SHA256 or not.
     */
    public static boolean isScanResultForFilsSha256Network(ScanResult scanResult) {
        return scanResult.capabilities.contains("FILS-SHA256");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to FILS SHA384 network.
     * This checks if the provided capabilities string contains FILS-SHA384 or not.
     */
    public static boolean isScanResultForFilsSha384Network(ScanResult scanResult) {
        return scanResult.capabilities.contains("FILS-SHA384");
    }

    /**
     *  Helper method to check if the provided |scanResult| corresponds to an unknown amk network.
     *  This checks if the provided capabilities string contains ? or not.
     */
    public static boolean isScanResultForUnknownAkmNetwork(ScanResult scanResult) {
        return scanResult.capabilities.contains("?");
    }

    /**
     * Helper method to check if the provided |scanResult| corresponds to an open network or not.
     * This checks if the provided capabilities string does not contain either of WEP, PSK, SAE
     * EAP, or unknown encryption types or not.
     */
    public static boolean isScanResultForOpenNetwork(ScanResult scanResult) {
        return (!(isScanResultForWepNetwork(scanResult) || isScanResultForPskNetwork(scanResult)
                || isScanResultForEapNetwork(scanResult) || isScanResultForSaeNetwork(scanResult)
                || isScanResultForWpa3EnterpriseTransitionNetwork(scanResult)
                || isScanResultForWpa3EnterpriseOnlyNetwork(scanResult)
                || isScanResultForWapiPskNetwork(scanResult)
                || isScanResultForWapiCertNetwork(scanResult)
                || isScanResultForEapSuiteBNetwork(scanResult)
                || isScanResultForUnknownAkmNetwork(scanResult)));
    }

    /**
     * Helper method to quote the SSID in Scan result to use for comparing/filling SSID stored in
     * WifiConfiguration object.
     */
    @VisibleForTesting
    public static String createQuotedSSID(String ssid) {
        return "\"" + ssid + "\"";
    }

    /**
     * Creates a network configuration object using the provided |scanResult|.
     * This is used to create ephemeral network configurations.
     */
    public static @Nullable WifiConfiguration createNetworkFromScanResult(ScanResult scanResult) {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = createQuotedSSID(scanResult.SSID);
        List<SecurityParams> list = generateSecurityParamsListFromScanResult(scanResult);
        if (list.isEmpty()) {
            return null;
        }
        config.setSecurityParams(list);
        return config;
    }

    /**
     * Generate security params from the scan result.
     * @param scanResult the scan result to be checked.
     * @return a list of security params. If no known security params, return an empty list.
     */
    public static @NonNull List<SecurityParams> generateSecurityParamsListFromScanResult(
            ScanResult scanResult) {
        List<SecurityParams> list = new ArrayList<>();

        // Open network & its upgradable types
        if (ScanResultUtil.isScanResultForOweTransitionNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_OPEN));
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_OWE));
            return list;
        } else if (ScanResultUtil.isScanResultForOweNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_OWE));
            return list;
        } else if (ScanResultUtil.isScanResultForOpenNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_OPEN));
            return list;
        }

        // WEP network which has no upgradable type
        if (ScanResultUtil.isScanResultForWepNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_WEP));
            return list;
        }

        // WAPI PSK network which has no upgradable type
        if (ScanResultUtil.isScanResultForWapiPskNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_WAPI_PSK));
            return list;
        }

        // WAPI CERT network which has no upgradable type
        if (ScanResultUtil.isScanResultForWapiCertNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_WAPI_CERT));
            return list;
        }

        // WPA2 personal network & its upgradable types
        if (ScanResultUtil.isScanResultForPskNetwork(scanResult)
                && ScanResultUtil.isScanResultForSaeNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_PSK));
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_SAE));
            return list;
        } else if (ScanResultUtil.isScanResultForPskNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_PSK));
            return list;
        } else if (ScanResultUtil.isScanResultForSaeNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_SAE));
            return list;
        }

        // WPA3 Enterprise 192-bit mode, WPA2/WPA3 enterprise network & its upgradable types
        if (ScanResultUtil.isScanResultForEapSuiteBNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT));
        } else if (ScanResultUtil.isScanResultForWpa3EnterpriseTransitionNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_EAP));
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE));
        } else if (ScanResultUtil.isScanResultForWpa3EnterpriseOnlyNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE));
        } else if (ScanResultUtil.isScanResultForEapNetwork(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_EAP));
        }
        // An Enterprise network might be a Passpoint network as well.
        // R3 network might be also a valid R1/R2 network.
        if (isScanResultForPasspointR1R2Network(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_PASSPOINT_R1_R2));
        }
        if (isScanResultForPasspointR3Network(scanResult)) {
            list.add(SecurityParams.createSecurityParamsBySecurityType(
                    WifiConfiguration.SECURITY_TYPE_PASSPOINT_R3));
        }
        return list;
    }

    /**
     * Dump the provided scan results list to |pw|.
     */
    public static void dumpScanResults(PrintWriter pw, List<ScanResult> scanResults, long nowMs) {
        if (scanResults != null && scanResults.size() != 0) {
            pw.println("    BSSID              Frequency      RSSI           Age(sec)     SSID "
                    + "                                Flags");
            for (ScanResult r : scanResults) {
                long timeStampMs = r.timestamp / 1000;
                String age;
                if (timeStampMs <= 0) {
                    age = "___?___";
                } else if (nowMs < timeStampMs) {
                    age = "  0.000";
                } else if (timeStampMs < nowMs - 1000000) {
                    age = ">1000.0";
                } else {
                    age = String.format("%3.3f", (nowMs - timeStampMs) / 1000.0);
                }
                String ssid = r.SSID == null ? "" : r.SSID;
                String rssiInfo = "";
                if (ArrayUtils.size(r.radioChainInfos) == 1) {
                    rssiInfo = String.format("%5d(%1d:%3d)       ", r.level,
                            r.radioChainInfos[0].id, r.radioChainInfos[0].level);
                } else if (ArrayUtils.size(r.radioChainInfos) == 2) {
                    rssiInfo = String.format("%5d(%1d:%3d/%1d:%3d)", r.level,
                            r.radioChainInfos[0].id, r.radioChainInfos[0].level,
                            r.radioChainInfos[1].id, r.radioChainInfos[1].level);
                } else {
                    rssiInfo = String.format("%9d         ", r.level);
                }
                if ((r.flags & ScanResult.FLAG_PASSPOINT_NETWORK)
                        == ScanResult.FLAG_PASSPOINT_NETWORK) {
                    r.capabilities += "[PASSPOINT]";
                }
                pw.printf("  %17s  %9d  %18s   %7s    %-32s  %s\n",
                        r.BSSID,
                        r.frequency,
                        rssiInfo,
                        age,
                        String.format("%1.32s", ssid),
                        r.capabilities);
            }
        }
    }

    /**
     * Check if ScarResult list is valid.
     */
    public static boolean validateScanResultList(List<ScanResult> scanResults) {
        if (scanResults == null || scanResults.isEmpty()) {
            Log.w(TAG, "Empty or null ScanResult list");
            return false;
        }
        for (ScanResult scanResult : scanResults) {
            if (!validate(scanResult)) {
                Log.w(TAG, "Invalid ScanResult: " + scanResult);
                return false;
            }
        }
        return true;
    }

    private static boolean validate(ScanResult scanResult) {
        return scanResult != null && scanResult.SSID != null
                && scanResult.capabilities != null && scanResult.BSSID != null;
    }
}
