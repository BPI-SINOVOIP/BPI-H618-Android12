/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.cts.deviceowner;

import static com.android.compatibility.common.util.WifiConfigCreator.ACTION_CREATE_WIFI_CONFIG;
import static com.android.compatibility.common.util.WifiConfigCreator.ACTION_REMOVE_WIFI_CONFIG;
import static com.android.compatibility.common.util.WifiConfigCreator.ACTION_UPDATE_WIFI_CONFIG;
import static com.android.compatibility.common.util.WifiConfigCreator.EXTRA_NETID;
import static com.android.compatibility.common.util.WifiConfigCreator.EXTRA_PASSWORD;
import static com.android.compatibility.common.util.WifiConfigCreator.EXTRA_SECURITY_TYPE;
import static com.android.compatibility.common.util.WifiConfigCreator.EXTRA_SSID;
import static com.android.compatibility.common.util.WifiConfigCreator.SECURITY_TYPE_NONE;
import static com.android.compatibility.common.util.WifiConfigCreator.SECURITY_TYPE_WPA;

import static com.google.common.truth.Truth.assertWithMessage;

import android.content.Intent;
import android.net.wifi.WifiConfiguration;
import android.provider.Settings;
import android.util.Log;

import java.util.List;
import java.util.stream.Collectors;

/**
 * Testing WiFi configuration lockdown by Device Owner
 */
public final class WifiConfigLockdownTest extends BaseDeviceOwnerTest {
    private static final String TAG = "WifiConfigLockdownTest";
    private static final String ORIGINAL_DEVICE_OWNER_SSID = "DOCTSTest";
    private static final String CHANGED_DEVICE_OWNER_SSID = "DOChangedCTSTest";
    private static final String ORIGINAL_REGULAR_SSID = "RegularCTSTest";
    private static final String CHANGED_REGULAR_SSID = "RegularChangedCTSTest";
    private static final String ORIGINAL_PASSWORD = "originalpassword";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevicePolicyManager.setGlobalSetting(getWho(),
                Settings.Global.WIFI_DEVICE_OWNER_CONFIGS_LOCKDOWN, "1");
        mWifiConfigCreator.addNetwork(ORIGINAL_DEVICE_OWNER_SSID, true, SECURITY_TYPE_WPA,
                ORIGINAL_PASSWORD);
        startRegularActivity(ACTION_CREATE_WIFI_CONFIG, -1, ORIGINAL_REGULAR_SSID,
                SECURITY_TYPE_WPA, ORIGINAL_PASSWORD);
    }

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.setGlobalSetting(getWho(),
                Settings.Global.WIFI_DEVICE_OWNER_CONFIGS_LOCKDOWN, "0");
        List<WifiConfiguration> configs = mWifiConfigCreator.getConfiguredNetworks();
        logConfigs("tearDown()", configs);
        for (WifiConfiguration config : configs) {
            if (areMatchingSsids(ORIGINAL_DEVICE_OWNER_SSID, config.SSID) ||
                    areMatchingSsids(CHANGED_DEVICE_OWNER_SSID, config.SSID) ||
                    areMatchingSsids(ORIGINAL_REGULAR_SSID, config.SSID) ||
                    areMatchingSsids(CHANGED_REGULAR_SSID, config.SSID)) {
                Log.d(TAG, "Removing " + config.networkId);
                mWifiManager.removeNetwork(config.networkId);
            }
        }
        super.tearDown();
    }

    public void testDeviceOwnerCanUpdateConfig() throws Exception {
        List<WifiConfiguration> configs = mWifiConfigCreator.getConfiguredNetworks();
        logConfigs("testDeviceOwnerCanUpdateConfig()", configs);
        int updateCount = 0;
        for (WifiConfiguration config : configs) {
            Log.d(TAG, "testDeviceOwnerCanUpdateConfig(): testing " + config.SSID);
            if (areMatchingSsids(ORIGINAL_DEVICE_OWNER_SSID, config.SSID)) {
                int netId = mWifiConfigCreator.updateNetwork(config,
                        CHANGED_DEVICE_OWNER_SSID, true, SECURITY_TYPE_NONE, null);
                Log.d(TAG, "netid after updateNetwork(REGULAR_SSID):" + netId);
                assertWithMessage("netid after updateNetwork(%s, DO_SSID)", config.SSID)
                        .that(netId).isNotEqualTo(-1);
                ++updateCount;
            }
            if (areMatchingSsids(ORIGINAL_REGULAR_SSID, config.SSID)) {
                int netId = mWifiConfigCreator.updateNetwork(config,
                        CHANGED_REGULAR_SSID, true, SECURITY_TYPE_NONE, null);
                Log.d(TAG, "netid after updateNetwork(REGULAR_SSID):" + netId);
                assertWithMessage("netid after updateNetwork(%s, REGULAR_SSID)", config.SSID)
                        .that(netId).isNotEqualTo(-1);
                ++updateCount;
            }
        }
        // There might be auto-upgrade configs returned.
        assertWithMessage("number of updated configs (the DO created one and the regular one)")
                .that(updateCount).isAtLeast(2);
    }

    public void testRegularAppCannotUpdateDeviceOwnerConfig() throws Exception {
        List<WifiConfiguration> configs = mWifiConfigCreator.getConfiguredNetworks();
        logConfigs("testRegularAppCannotUpdateDeviceOwnerConfig()", configs);
        int updateCount = 0;
        for (WifiConfiguration config : configs) {
            Log.d(TAG, "testRegularAppCannotUpdateDeviceOwnerConfig(): testing " + config.SSID);
            if (areMatchingSsids(ORIGINAL_DEVICE_OWNER_SSID, config.SSID)) {
                startRegularActivity(ACTION_UPDATE_WIFI_CONFIG, config.networkId,
                        CHANGED_DEVICE_OWNER_SSID, SECURITY_TYPE_NONE, null);
                ++updateCount;
            }
        }
        // There might be auto-upgrade configs returned.
        assertWithMessage("number of updated configs (the DO created one)")
                .that(updateCount).isAtLeast(1);

        // Assert nothing has changed
        configs = mWifiConfigCreator.getConfiguredNetworks();
        int notChangedCount = 0;
        for (WifiConfiguration config : configs) {
            Log.d(TAG, "testRegularAppCannotUpdateDeviceOwnerConfig(): testing " + config.SSID);
            assertWithMessage("matching ssids for %s / %s", CHANGED_DEVICE_OWNER_SSID, config.SSID)
                    .that(areMatchingSsids(CHANGED_DEVICE_OWNER_SSID, config.SSID)).isFalse();
            if (areMatchingSsids(ORIGINAL_DEVICE_OWNER_SSID, config.SSID)) {
                ++notChangedCount;
            }
        }
        // There might be auto-upgrade configs returned.
        assertWithMessage("number of unchanged configs").that(notChangedCount).isAtLeast(1);
    }

    public void testRegularAppCannotRemoveDeviceOwnerConfig() throws Exception {
        List<WifiConfiguration> configs = mWifiConfigCreator.getConfiguredNetworks();
        logConfigs("testRegularAppCannotUpdateDeviceOwnerConfig()", configs);
        int removeCount = 0;
        for (WifiConfiguration config : configs) {
            Log.d(TAG, "testRegularAppCannotRemoveDeviceOwnerConfig(): testing " + config.SSID);
            if (areMatchingSsids(ORIGINAL_DEVICE_OWNER_SSID, config.SSID)) {
                startRegularActivity(ACTION_REMOVE_WIFI_CONFIG, config.networkId,
                        null, SECURITY_TYPE_NONE, null);
                ++removeCount;
            }
        }

        // There might be auto-upgrade configs returned.
        assertWithMessage("number of removed configs (the DO created one)")
                .that(removeCount).isAtLeast(1);

        // Assert nothing has changed
        configs = mWifiConfigCreator.getConfiguredNetworks();
        int notChangedCount = 0;
        for (WifiConfiguration config : configs) {
            Log.d(TAG, "testRegularAppCannotRemoveDeviceOwnerConfig(): testing " + config.SSID);
            if (areMatchingSsids(ORIGINAL_DEVICE_OWNER_SSID, config.SSID)) {
                ++notChangedCount;
            }
        }
        // There might be auto-upgrade configs returned.
        assertWithMessage("number of unchanged configs").that(notChangedCount).isAtLeast(1);
    }

    private void startRegularActivity(String action, int netId, String ssid, int securityType,
            String password) throws InterruptedException {
        Intent createRegularConfig = new Intent(action);
        createRegularConfig.putExtra(EXTRA_NETID, netId);
        createRegularConfig.putExtra(EXTRA_SSID, ssid);
        createRegularConfig.putExtra(EXTRA_SECURITY_TYPE, securityType);
        createRegularConfig.putExtra(EXTRA_PASSWORD, password);
        createRegularConfig.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Log.d(TAG, "Starting " + action  + " on user " + mContext.getUserId());
        mContext.startActivity(createRegularConfig);

        // Give some time for the other app to finish the action
        Log.d(TAG, "Sleeping 5s");
        Thread.sleep(5000);
    }

    private boolean areMatchingSsids(String s1, String s2) {
        if (s1 == null || s2 == null) {
            return false;
        }
        return s1.replace("\"", "").equals(s2.replace("\"", ""));
    }

    private void logConfigs(String prefix, List<WifiConfiguration> configs) {
        if (configs == null) {
            Log.d(TAG, prefix + ": null configs");
            return;
        }
        Log.d(TAG, prefix + ": " + configs.size() + " configs: "
                + configs.stream().map((c) -> c.SSID).collect(Collectors.toList()));
    }
}
