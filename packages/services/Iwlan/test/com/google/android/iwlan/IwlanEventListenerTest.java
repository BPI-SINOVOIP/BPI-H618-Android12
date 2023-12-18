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

package com.google.android.iwlan;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Message;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.ims.ImsManager;
import android.telephony.ims.ImsMmTelManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;

import java.util.*;

public class IwlanEventListenerTest {
    private static final String TAG = "IwlanEventListenerTest";

    @Mock private Context mMockContext;
    @Mock private Handler mMockHandler;
    @Mock private Message mMockMessage;
    @Mock private Message mMockMessage_2;
    @Mock private WifiManager mMockWifiManager;
    @Mock private WifiInfo mMockWifiInfo;
    @Mock private SubscriptionManager mMockSubscriptionManager;
    @Mock private SubscriptionInfo mMockSubscriptionInfo;
    @Mock private ContentResolver mMockContentResolver;
    @Mock private ImsManager mMockImsManager;
    @Mock private ImsMmTelManager mMockImsMmTelManager;

    private static final int DEFAULT_SLOT_INDEX = 0;
    private static final int DEFAULT_CARRIER_INDEX = 0;
    private static final String WIFI_SSID_1 = "TEST_AP_NAME_1";
    private static final String WIFI_SSID_2 = "TEST_AP_NAME_2";
    private static final Uri CROSS_SIM_URI =
            Uri.parse("content://telephony/siminfo/cross_sim_calling_enabled/2");
    private static final Uri WFC_ENABLED_URI = Uri.parse("content://telephony/siminfo/wfc/2");
    private IwlanEventListener mIwlanEventListener;
    private List<Integer> events;

    MockitoSession mStaticMockSession;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mStaticMockSession =
                mockitoSession()
                        .mockStatic(IwlanHelper.class)
                        .mockStatic(SubscriptionManager.class)
                        .startMocking();

        when(mMockContext.getSystemService(eq(WifiManager.class))).thenReturn(mMockWifiManager);

        when(mMockWifiManager.getConnectionInfo()).thenReturn(mMockWifiInfo);

        when(mMockContext.getSystemService(eq(SubscriptionManager.class)))
                .thenReturn(mMockSubscriptionManager);

        when(mMockSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(
                        eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(mMockSubscriptionInfo);

        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);

        when(mMockImsMmTelManager.isVoWiFiSettingEnabled()).thenReturn(true).thenReturn(false);

        when(mMockImsManager.getImsMmTelManager(eq(2))).thenReturn(mMockImsMmTelManager);

        when(mMockContext.getSystemService(eq(ImsManager.class))).thenReturn(mMockImsManager);

        mIwlanEventListener = IwlanEventListener.getInstance(mMockContext, DEFAULT_SLOT_INDEX);
    }

    @After
    public void cleanUp() throws Exception {
        mStaticMockSession.finishMocking();
    }

    @Test
    public void testWifiApChanged() throws Exception {
        when(mMockHandler.obtainMessage(eq(IwlanEventListener.WIFI_AP_CHANGED_EVENT)))
                .thenReturn(mMockMessage);

        events = new ArrayList<Integer>();
        events.add(IwlanEventListener.WIFI_AP_CHANGED_EVENT);
        mIwlanEventListener.addEventListener(events, mMockHandler);

        // First Wifi connected should not trigger WIFI_AP_CHANGED_EVENT
        when(mMockWifiInfo.getSSID()).thenReturn(WIFI_SSID_1);
        mIwlanEventListener.onWifiConnected(mMockContext);
        verify(mMockMessage, times(0)).sendToTarget();

        when(mMockWifiInfo.getSSID()).thenReturn(WIFI_SSID_2);
        mIwlanEventListener.onWifiConnected(mMockContext);
        verify(mMockMessage, times(1)).sendToTarget();
    }

    @Test
    public void testCrossSimCallingSettingEnableChanged() throws Exception {
        when(mMockHandler.obtainMessage(eq(IwlanEventListener.CROSS_SIM_CALLING_ENABLE_EVENT)))
                .thenReturn(mMockMessage);

        events = new ArrayList<Integer>();
        events.add(IwlanEventListener.CROSS_SIM_CALLING_ENABLE_EVENT);
        mIwlanEventListener.addEventListener(events, mMockHandler);
        mIwlanEventListener.setCrossSimCallingUri(CROSS_SIM_URI);

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);

        // Trigger CROSS_SIM_CALLING_ENABLE_EVENT when cross sim calling setting is enabled
        mIwlanEventListener.getCurrentUriSetting(CROSS_SIM_URI);
        verify(mMockMessage, times(1)).sendToTarget();
    }

    @Test
    public void testCrossSimCallingSettingDisableChanged() throws Exception {
        when(mMockHandler.obtainMessage(eq(IwlanEventListener.CROSS_SIM_CALLING_DISABLE_EVENT)))
                .thenReturn(mMockMessage);

        events = new ArrayList<Integer>();
        events.add(IwlanEventListener.CROSS_SIM_CALLING_DISABLE_EVENT);
        mIwlanEventListener.addEventListener(events, mMockHandler);
        mIwlanEventListener.setCrossSimCallingUri(CROSS_SIM_URI);

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(false);

        // Trigger CROSS_SIM_CALLING_DISABLE_EVENT when cross sim calling setting is disabled
        mIwlanEventListener.getCurrentUriSetting(CROSS_SIM_URI);
        verify(mMockMessage, times(1)).sendToTarget();
    }

    @Test
    public void testOnReceivedCarrierConfigChangedIntent() throws Exception {
        when(mMockHandler.obtainMessage(eq(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)))
                .thenReturn(mMockMessage);
        when(mMockHandler.obtainMessage(
                        eq(IwlanEventListener.CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT)))
                .thenReturn(mMockMessage_2);

        events = new ArrayList<Integer>();
        events.add(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT);
        events.add(IwlanEventListener.CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT);
        mIwlanEventListener.addEventListener(events, mMockHandler);

        // Send ACTION_CARRIER_CONFIG_CHANGED intent with valid Carrier id
        final Intent validCarrierIdintent =
                new Intent(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        validCarrierIdintent.putExtra(CarrierConfigManager.EXTRA_SLOT_INDEX, DEFAULT_SLOT_INDEX);
        validCarrierIdintent.putExtra(TelephonyManager.EXTRA_CARRIER_ID, DEFAULT_CARRIER_INDEX);

        mIwlanEventListener.onBroadcastReceived(validCarrierIdintent);

        verify(mMockMessage, times(1)).sendToTarget();

        // Send ACTION_CARRIER_CONFIG_CHANGED intent with invalid Carrier id
        final Intent invalidCarrierIdintent =
                new Intent(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        invalidCarrierIdintent.putExtra(CarrierConfigManager.EXTRA_SLOT_INDEX, DEFAULT_SLOT_INDEX);
        invalidCarrierIdintent.putExtra(
                TelephonyManager.EXTRA_CARRIER_ID, TelephonyManager.UNKNOWN_CARRIER_ID);

        mIwlanEventListener.onBroadcastReceived(invalidCarrierIdintent);

        verify(mMockMessage_2, times(1)).sendToTarget();
    }

    @Test
    public void testWfcSettingChanged() throws Exception {
        when(mMockHandler.obtainMessage(eq(IwlanEventListener.WIFI_CALLING_ENABLE_EVENT)))
                .thenReturn(mMockMessage);
        when(mMockHandler.obtainMessage(eq(IwlanEventListener.WIFI_CALLING_DISABLE_EVENT)))
                .thenReturn(mMockMessage_2);

        events = new ArrayList<Integer>();
        events.add(IwlanEventListener.WIFI_CALLING_ENABLE_EVENT);
        events.add(IwlanEventListener.WIFI_CALLING_DISABLE_EVENT);
        mIwlanEventListener.addEventListener(events, mMockHandler);
        mIwlanEventListener.setWfcEnabledUri(WFC_ENABLED_URI);

        mIwlanEventListener.getCurrentUriSetting(WFC_ENABLED_URI);
        verify(mMockMessage, times(1)).sendToTarget();

        mIwlanEventListener.getCurrentUriSetting(WFC_ENABLED_URI);
        verify(mMockMessage_2, times(1)).sendToTarget();
    }
}
