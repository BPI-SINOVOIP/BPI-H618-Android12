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

package com.android.server.wifi;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.res.Resources;
import android.net.wifi.WifiConfiguration;
import android.provider.Settings;
import android.service.notification.StatusBarNotification;
import android.telephony.SubscriptionManager;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Answers;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;

/**
 * Unit tests for {@link com.android.server.wifi.EapFailureNotifier}.
 */
@SmallTest
public class EapFailureNotifierTest extends WifiBaseTest {
    private static final String TEST_SETTINGS_PACKAGE = "test.com.android.settings";
    private static final String NOTIFICATION_TAG = "com.android.wifi";

    @Mock WifiContext mContext;
    @Mock Resources mResources;
    @Mock WifiNotificationManager mWifiNotificationManager;
    @Mock FrameworkFacade mFrameworkFacade;
    @Mock Notification mNotification;
    @Mock
    WifiCarrierInfoManager mWifiCarrierInfoManager;
    @Mock WifiConfiguration mWifiConfiguration;

    @Mock(answer = Answers.RETURNS_DEEP_STUBS) private Notification.Builder mNotificationBuilder;
    private static final int DEFINED_ERROR_CODE = 32764;
    private static final int UNDEFINED_ERROR_CODE = 12345;
    private static final String SSID_1 = "Carrier_AP_1";
    private static final String SSID_2 = "Carrier_AP_2";
    private static final String DEFINED_ERROR_RESOURCE_NAME = "wifi_eap_error_message_code_32764";
    private static final String UNDEFINED_ERROR_RESOURCE_NAME = "wifi_eap_error_message_code_12345";
    private MockitoSession mStaticMockSession = null;

    EapFailureNotifier mEapFailureNotifier;

    /**
     * Sets up for unit test
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        // static mocking
        mStaticMockSession = mockitoSession()
            .mockStatic(SubscriptionManager.class)
            .startMocking();
        when(mWifiCarrierInfoManager.getBestMatchSubscriptionId(mWifiConfiguration)).thenReturn(0);
        lenient().when(SubscriptionManager.getResourcesForSubId(eq(mContext), anyInt()))
                .thenReturn(mResources);
        when(mContext.getResources()).thenReturn(mResources);
        when(mResources.getIdentifier(eq(DEFINED_ERROR_RESOURCE_NAME), anyString(),
                anyString())).thenReturn(1);
        when(mResources.getIdentifier(eq(UNDEFINED_ERROR_RESOURCE_NAME), anyString(),
                anyString())).thenReturn(0);
        when(mResources.getString(eq(0), anyString())).thenReturn(null);
        when(mResources.getString(eq(1), anyString())).thenReturn("Error Message");
        when(mContext.createPackageContext(anyString(), eq(0))).thenReturn(mContext);
        when(mContext.getWifiOverlayApkPkgName()).thenReturn("test.com.android.wifi.resources");
        when(mFrameworkFacade.getSettingsPackageName(any())).thenReturn(TEST_SETTINGS_PACKAGE);
        mEapFailureNotifier =
                new EapFailureNotifier(mContext, mFrameworkFacade, mWifiCarrierInfoManager,
                        mWifiNotificationManager);
    }

    @After
    public void cleanUp() throws Exception {
        validateMockitoUsage();
        if (mStaticMockSession != null) {
            mStaticMockSession.finishMocking();
        }
    }

    /**
     * Verify that a eap failure notification will be generated with the following conditions :
     * No eap failure notification of eap failure is displayed now.
     * Error code is defined by carrier
     * @throws Exception
     */
    @Test
    public void onEapFailureWithDefinedErrorCodeWithoutNotificationShown() throws Exception {
        when(mFrameworkFacade.makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS))).thenReturn(mNotificationBuilder);
        StatusBarNotification[] activeNotifications = new StatusBarNotification[1];
        activeNotifications[0] = new StatusBarNotification("android", "", 56, "", 0, 0, 0,
                mNotification, android.os.Process.myUserHandle(), 0);
        when(mWifiNotificationManager.getActiveNotifications()).thenReturn(activeNotifications);
        mWifiConfiguration.SSID = SSID_2;
        mEapFailureNotifier.onEapFailure(DEFINED_ERROR_CODE, mWifiConfiguration, true);
        verify(mWifiNotificationManager).notify(eq(EapFailureNotifier.NOTIFICATION_ID), any());
        ArgumentCaptor<Intent> intent = ArgumentCaptor.forClass(Intent.class);
        verify(mFrameworkFacade).getActivity(
                eq(mContext), eq(0), intent.capture(),
                eq(PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE));
        assertEquals(TEST_SETTINGS_PACKAGE, intent.getValue().getPackage());
        assertEquals(Settings.ACTION_WIFI_SETTINGS, intent.getValue().getAction());
    }

    /**
     * Verify that a eap failure notification will not be generated with the following conditions :
     * No eap failure notification of eap failure is displayed now.
     * Error code is defined by carrier
     * showNotification = false
     */
    @Test
    public void onEapFailure_showNotificationFalse_notShown() throws Exception {
        when(mFrameworkFacade.makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS))).thenReturn(mNotificationBuilder);
        StatusBarNotification[] activeNotifications = new StatusBarNotification[1];
        activeNotifications[0] = new StatusBarNotification("android", "", 56, "", 0, 0, 0,
                mNotification, android.os.Process.myUserHandle(), 0);
        when(mWifiNotificationManager.getActiveNotifications()).thenReturn(activeNotifications);
        mWifiConfiguration.SSID = SSID_2;
        mEapFailureNotifier.onEapFailure(DEFINED_ERROR_CODE, mWifiConfiguration, false);
        verify(mWifiNotificationManager, never()).notify(anyInt(), any());
    }

    /**
     * Verify that a eap failure notification will be generated with the following conditions :
     * Previous notification of eap failure is still displayed on the notification bar.
     * Ssid of previous notification is not same as current ssid
     * Error code is defined by carrier
     * @throws Exception
     */
    @Test
    public void onEapFailureWithDefinedErroCodeWithNotificationShownWithoutSameSsid()
            throws Exception {
        when(mFrameworkFacade.makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS))).thenReturn(mNotificationBuilder);
        StatusBarNotification[] activeNotifications = new StatusBarNotification[1];
        activeNotifications[0] = new StatusBarNotification("android", "", 57, "", 0, 0, 0,
                mNotification, android.os.Process.myUserHandle(), 0);
        when(mWifiNotificationManager.getActiveNotifications()).thenReturn(activeNotifications);
        mEapFailureNotifier.setCurrentShownSsid(SSID_1);
        mWifiConfiguration.SSID = SSID_2;
        mEapFailureNotifier.onEapFailure(DEFINED_ERROR_CODE, mWifiConfiguration, true);
        verify(mWifiNotificationManager).notify(eq(EapFailureNotifier.NOTIFICATION_ID), any());
        ArgumentCaptor<Intent> intent = ArgumentCaptor.forClass(Intent.class);
        verify(mFrameworkFacade).getActivity(
                eq(mContext), eq(0), intent.capture(),
                eq(PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE));
        assertEquals(TEST_SETTINGS_PACKAGE, intent.getValue().getPackage());
        assertEquals(Settings.ACTION_WIFI_SETTINGS, intent.getValue().getAction());
    }

    /**
     * Verify that a eap failure notification will Not be generated with the following conditions :
     * Previous notification of eap failure is still displayed on the notification bar.
     * Ssid of previous notification is same as current ssid
     * Error code is defined by carrier
     * @throws Exception
     */
    @Test
    public void onEapFailureWithDefinedErroCodeWithNotificationShownWithSameSsid()
            throws Exception {
        when(mFrameworkFacade.makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS))).thenReturn(mNotificationBuilder);
        StatusBarNotification[] activeNotifications = new StatusBarNotification[1];
        activeNotifications[0] = new StatusBarNotification("android", "", 57, "", 0, 0, 0,
                mNotification, android.os.Process.myUserHandle(), 0);
        when(mWifiNotificationManager.getActiveNotifications()).thenReturn(activeNotifications);
        mEapFailureNotifier.setCurrentShownSsid(SSID_1);
        mWifiConfiguration.SSID = SSID_1;
        mEapFailureNotifier.onEapFailure(DEFINED_ERROR_CODE, mWifiConfiguration, true);
        verify(mFrameworkFacade, never()).makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS));
    }

    /**
     * Verify that a eap failure notification will Not be generated with the following conditions :
     * Error code is NOT defined by carrier
     * @throws Exception
     */
    @Test
    public void onEapFailureWithUnDefinedErrorCode() throws Exception {
        when(mFrameworkFacade.makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS))).thenReturn(mNotificationBuilder);
        StatusBarNotification[] activeNotifications = new StatusBarNotification[1];
        activeNotifications[0] = new StatusBarNotification("android", "", 56, "", 0, 0, 0,
                mNotification, android.os.Process.myUserHandle(), 0);
        when(mWifiNotificationManager.getActiveNotifications()).thenReturn(activeNotifications);
        mWifiConfiguration.SSID = SSID_1;
        mEapFailureNotifier.onEapFailure(UNDEFINED_ERROR_CODE, mWifiConfiguration, true);
        verify(mFrameworkFacade, never()).makeNotificationBuilder(any(),
                eq(WifiService.NOTIFICATION_NETWORK_ALERTS));
    }
}
