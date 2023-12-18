/*
 * Copyright (C) 2020 The Android Open Source Project
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

import static com.google.common.truth.Truth.assertThat;

import static junit.framework.Assert.assertEquals;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.content.Context;

import androidx.test.filters.SmallTest;

import com.android.wifi.resources.R;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;


/** Unit tests for {@link WifiGlobals} */
@SmallTest
public class WifiGlobalsTest extends WifiBaseTest {

    private WifiGlobals mWifiGlobals;
    private MockResources mResources;

    @Mock private Context mContext;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mResources = new MockResources();
        mResources.setInteger(R.integer.config_wifiPollRssiIntervalMilliseconds, 3000);
        mResources.setInteger(R.integer.config_wifiClientModeImplNumLogRecs, 200);
        when(mContext.getResources()).thenReturn(mResources);

        mWifiGlobals = new WifiGlobals(mContext);
    }

    /** Test that the interval for poll RSSI is read from config overlay correctly. */
    @Test
    public void testPollRssiIntervalIsSetCorrectly() throws Exception {
        assertEquals(3000, mWifiGlobals.getPollRssiIntervalMillis());
        mResources.setInteger(R.integer.config_wifiPollRssiIntervalMilliseconds, 6000);
        assertEquals(6000, mWifiGlobals.getPollRssiIntervalMillis());
        mResources.setInteger(R.integer.config_wifiPollRssiIntervalMilliseconds, 7000);
        assertEquals(6000, mWifiGlobals.getPollRssiIntervalMillis());
    }

    /** Verify that Bluetooth active is set correctly with BT state/connection state changes */
    @Test
    public void verifyBluetoothStateAndConnectionStateChanges() {
        mWifiGlobals.setBluetoothEnabled(true);
        assertThat(mWifiGlobals.isBluetoothConnected()).isFalse();

        mWifiGlobals.setBluetoothConnected(true);
        assertThat(mWifiGlobals.isBluetoothConnected()).isTrue();

        mWifiGlobals.setBluetoothEnabled(false);
        assertThat(mWifiGlobals.isBluetoothConnected()).isFalse();

        mWifiGlobals.setBluetoothEnabled(true);
        assertThat(mWifiGlobals.isBluetoothConnected()).isFalse();

        mWifiGlobals.setBluetoothConnected(true);
        assertThat(mWifiGlobals.isBluetoothConnected()).isTrue();

        mWifiGlobals.setBluetoothConnected(false);
        assertThat(mWifiGlobals.isBluetoothConnected()).isFalse();

        mWifiGlobals.setBluetoothConnected(true);
        assertThat(mWifiGlobals.isBluetoothConnected()).isTrue();
    }

    /** Verify SAE Hash-to-Element overlay. */
    @Test
    public void testSaeH2eSupportOverlay() {
        mResources.setBoolean(R.bool.config_wifiSaeH2eSupported, false);
        mWifiGlobals = new WifiGlobals(mContext);
        assertFalse(mWifiGlobals.isWpa3SaeH2eSupported());

        mResources.setBoolean(R.bool.config_wifiSaeH2eSupported, true);
        mWifiGlobals = new WifiGlobals(mContext);
        assertTrue(mWifiGlobals.isWpa3SaeH2eSupported());
    }

    /** Verify P2P device name customization. */
    @Test
    public void testP2pDeviceNameCustomization() {
        final String customPrefix = "Custom-";
        final int customPostfixDigit = 5;
        mResources.setString(R.string.config_wifiP2pDeviceNamePrefix, customPrefix);
        mResources.setInteger(R.integer.config_wifiP2pDeviceNamePostfixNumDigits,
                customPostfixDigit);
        mWifiGlobals = new WifiGlobals(mContext);
        assertEquals(customPrefix, mWifiGlobals.getWifiP2pDeviceNamePrefix());
        assertEquals(customPostfixDigit, mWifiGlobals.getWifiP2pDeviceNamePostfixNumDigits());
    }

    /** Test that the number of log records is read from config overlay correctly. */
    @Test
    public void testNumLogRecsNormalIsSetCorrectly() throws Exception {
        assertEquals(200, mWifiGlobals.getClientModeImplNumLogRecs());
    }
}
