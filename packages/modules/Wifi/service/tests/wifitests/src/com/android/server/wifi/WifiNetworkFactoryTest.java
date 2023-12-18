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

package com.android.server.wifi;

import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND_SERVICE;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_GONE;
import static android.content.Intent.ACTION_SCREEN_OFF;
import static android.content.Intent.ACTION_SCREEN_ON;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;
import static com.android.server.wifi.WifiNetworkFactory.PERIODIC_SCAN_INTERVAL_MS;
import static com.android.server.wifi.util.NativeUtil.addEnclosingQuotes;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import static java.lang.Math.toIntExact;

import android.annotation.Nullable;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.AlarmManager.OnAlarmListener;
import android.app.AppOpsManager;
import android.app.test.MockAnswerUtil;
import android.companion.CompanionDeviceManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.MacAddress;
import android.net.NetworkCapabilities;
import android.net.NetworkProvider;
import android.net.NetworkRequest;
import android.net.NetworkSpecifier;
import android.net.wifi.INetworkRequestMatchCallback;
import android.net.wifi.INetworkRequestUserSelectionCallback;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiScanner.ScanListener;
import android.net.wifi.WifiScanner.ScanSettings;
import android.net.wifi.WifiSsid;
import android.os.IBinder;
import android.os.PatternMatcher;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.WorkSource;
import android.os.test.TestLooper;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Pair;
import android.util.Xml;

import com.android.internal.util.FastXmlSerializer;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.WifiNetworkFactory.AccessPoint;
import com.android.server.wifi.proto.nano.WifiMetricsProto;
import com.android.server.wifi.util.ActionListenerWrapper;
import com.android.server.wifi.util.ScanResultUtil;
import com.android.server.wifi.util.WifiConfigStoreEncryptionUtil;
import com.android.server.wifi.util.WifiPermissionsUtil;
import com.android.wifi.resources.R;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlSerializer;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link com.android.server.wifi.WifiNetworkFactory}.
 */
@SmallTest
public class WifiNetworkFactoryTest extends WifiBaseTest {
    private static final int TEST_NETWORK_ID_1 = 104;
    private static final int TEST_UID_1 = 10423;
    private static final int TEST_UID_2 = 10424;
    private static final String TEST_PACKAGE_NAME_1 = "com.test.networkrequest.1";
    private static final String TEST_PACKAGE_NAME_2 = "com.test.networkrequest.2";
    private static final String TEST_APP_NAME = "app";
    private static final String TEST_SSID_1 = "test1234";
    private static final String TEST_SSID_2 = "test12345678";
    private static final String TEST_SSID_3 = "abc1234";
    private static final String TEST_SSID_4 = "abc12345678";
    private static final String TEST_BSSID_1 = "12:34:23:23:45:ac";
    private static final String TEST_BSSID_2 = "12:34:23:32:12:67";
    private static final String TEST_BSSID_3 = "45:34:34:12:bb:dd";
    private static final String TEST_BSSID_4 = "45:34:34:56:ee:ff";
    private static final String TEST_BSSID_1_2_OUI = "12:34:23:00:00:00";
    private static final String TEST_BSSID_OUI_MASK = "ff:ff:ff:00:00:00";
    private static final String TEST_WPA_PRESHARED_KEY = "\"password123\"";

    @Mock Context mContext;
    @Mock Resources mResources;
    @Mock ActivityManager mActivityManager;
    @Mock AlarmManager mAlarmManager;
    @Mock AppOpsManager mAppOpsManager;
    @Mock CompanionDeviceManager mCompanionDeviceManager;
    @Mock Clock mClock;
    @Mock WifiInjector mWifiInjector;
    @Mock WifiConnectivityManager mWifiConnectivityManager;
    @Mock WifiConfigManager mWifiConfigManager;
    @Mock WifiConfigStore mWifiConfigStore;
    @Mock WifiPermissionsUtil mWifiPermissionsUtil;
    @Mock WifiScanner mWifiScanner;
    @Mock PackageManager mPackageManager;
    @Mock IBinder mAppBinder;
    @Mock INetworkRequestMatchCallback mNetworkRequestMatchCallback;
    @Mock ConcreteClientModeManager mClientModeManager;
    @Mock ConnectivityManager mConnectivityManager;
    @Mock WifiMetrics mWifiMetrics;
    @Mock NetworkProvider mNetworkProvider;
    @Mock ActiveModeWarden mActiveModeWarden;
    @Mock ConnectHelper mConnectHelper;
    @Mock PowerManager mPowerManager;
    @Mock ClientModeImplMonitor mCmiMonitor;
    private @Mock ClientModeManager mPrimaryClientModeManager;
    private @Mock WifiGlobals mWifiGlobals;
    private MockitoSession mStaticMockSession = null;
    NetworkCapabilities mNetworkCapabilities;
    TestLooper mLooper;
    NetworkRequest mNetworkRequest;
    WifiScanner.ScanData[] mTestScanDatas;
    WifiConfiguration mSelectedNetwork;
    ArgumentCaptor<ScanSettings> mScanSettingsArgumentCaptor =
            ArgumentCaptor.forClass(ScanSettings.class);
    ArgumentCaptor<WorkSource> mWorkSourceArgumentCaptor =
            ArgumentCaptor.forClass(WorkSource.class);
    ArgumentCaptor<INetworkRequestUserSelectionCallback> mNetworkRequestUserSelectionCallback =
            ArgumentCaptor.forClass(INetworkRequestUserSelectionCallback.class);
    ArgumentCaptor<OnAlarmListener> mPeriodicScanListenerArgumentCaptor =
            ArgumentCaptor.forClass(OnAlarmListener.class);
    ArgumentCaptor<OnAlarmListener> mConnectionTimeoutAlarmListenerArgumentCaptor =
            ArgumentCaptor.forClass(OnAlarmListener.class);
    ArgumentCaptor<ScanListener> mScanListenerArgumentCaptor =
            ArgumentCaptor.forClass(ScanListener.class);
    ArgumentCaptor<ActionListenerWrapper> mConnectListenerArgumentCaptor =
            ArgumentCaptor.forClass(ActionListenerWrapper.class);
    ArgumentCaptor<ActiveModeWarden.ModeChangeCallback> mModeChangeCallbackCaptor =
            ArgumentCaptor.forClass(ActiveModeWarden.ModeChangeCallback.class);
    @Captor ArgumentCaptor<BroadcastReceiver> mBroadcastReceiverCaptor;
    @Captor ArgumentCaptor<ClientModeImplListener> mCmiListenerCaptor;
    InOrder mInOrder;

    private WifiNetworkFactory mWifiNetworkFactory;
    private NetworkRequestStoreData.DataSource mDataSource;
    private NetworkRequestStoreData mNetworkRequestStoreData;

    /**
     * Setup the mocks.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mStaticMockSession = mockitoSession()
                .mockStatic(WifiInjector.class)
                .startMocking();
        lenient().when(WifiInjector.getInstance()).thenReturn(mWifiInjector);

        mLooper = new TestLooper();
        mNetworkCapabilities = new NetworkCapabilities();
        mNetworkCapabilities.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        if (SdkLevel.isAtLeastS()) {
            // NOT_VCN_MANAGED is not part of default network capabilities and needs to be manually
            // added for non-VCN-underlying network factory/agent implementations.
            mNetworkCapabilities.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VCN_MANAGED);
        }
        mNetworkCapabilities.removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        mTestScanDatas = ScanTestUtil.createScanDatas(new int[][]{ { 2417, 2427, 5180, 5170 } });

        when(mContext.getResources()).thenReturn(mResources);
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(eq(Context.CONNECTIVITY_SERVICE)))
                .thenReturn(mConnectivityManager);
        when(mContext.getSystemService(eq(ConnectivityManager.class)))
                .thenReturn(mConnectivityManager);
        when(mContext.getSystemService(CompanionDeviceManager.class))
                .thenReturn(mCompanionDeviceManager);
        when(mContext.getSystemService(PowerManager.class)).thenReturn(mPowerManager);
        when(mResources.getBoolean(
                eq(R.bool.config_wifiUseHalApiToDisableFwRoaming)))
                .thenReturn(true);
        when(mPackageManager.getNameForUid(TEST_UID_1)).thenReturn(TEST_PACKAGE_NAME_1);
        when(mPackageManager.getNameForUid(TEST_UID_2)).thenReturn(TEST_PACKAGE_NAME_2);
        when(mPackageManager.getApplicationInfoAsUser(any(), anyInt(), any()))
                .thenReturn(new ApplicationInfo());
        when(mPackageManager.getApplicationLabel(any())).thenReturn(TEST_APP_NAME);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);
        when(mWifiInjector.getWifiScanner()).thenReturn(mWifiScanner);
        when(mWifiInjector.getActiveModeWarden()).thenReturn(mActiveModeWarden);
        when(mWifiInjector.getWifiGlobals()).thenReturn(mWifiGlobals);
        when(mWifiGlobals.isWpa3SaeUpgradeEnabled()).thenReturn(true);
        when(mWifiGlobals.isOweUpgradeEnabled()).thenReturn(true);
        when(mWifiConfigManager.addOrUpdateNetwork(any(), anyInt(), anyString()))
                .thenReturn(new NetworkUpdateResult(TEST_NETWORK_ID_1));
        when(mWifiScanner.getSingleScanResults()).thenReturn(Collections.emptyList());
        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);

        when(mActiveModeWarden.hasPrimaryClientModeManager()).thenReturn(true);
        when(mActiveModeWarden.getPrimaryClientModeManager()).thenReturn(mPrimaryClientModeManager);
        doAnswer(new MockAnswerUtil.AnswerWithArguments() {
            public void answer(
                    ActiveModeWarden.ExternalClientModeManagerRequestListener requestListener,
                    WorkSource ws, String ssid, String bssid) {
                requestListener.onAnswer(mClientModeManager);
            }
        }).when(mActiveModeWarden).requestLocalOnlyClientModeManager(any(), any(), any(), any());
        when(mClientModeManager.getRole()).thenReturn(ActiveModeManager.ROLE_CLIENT_PRIMARY);

        when(mPrimaryClientModeManager.getSupportedFeatures()).thenReturn(
                WifiManager.WIFI_FEATURE_WPA3_SAE | WifiManager.WIFI_FEATURE_OWE);

        mWifiNetworkFactory = new WifiNetworkFactory(mLooper.getLooper(), mContext,
                mNetworkCapabilities, mActivityManager, mAlarmManager, mAppOpsManager,
                mClock, mWifiInjector, mWifiConnectivityManager,
                mWifiConfigManager, mWifiConfigStore, mWifiPermissionsUtil, mWifiMetrics,
                mActiveModeWarden, mConnectHelper, mCmiMonitor);

        verify(mContext, atLeastOnce()).registerReceiver(
                mBroadcastReceiverCaptor.capture(), any(), any(), any());

        ArgumentCaptor<NetworkRequestStoreData.DataSource> dataSourceArgumentCaptor =
                ArgumentCaptor.forClass(NetworkRequestStoreData.DataSource.class);
        verify(mWifiInjector).makeNetworkRequestStoreData(dataSourceArgumentCaptor.capture());
        mDataSource = dataSourceArgumentCaptor.getValue();
        assertNotNull(mDataSource);
        mNetworkRequestStoreData = new NetworkRequestStoreData(mDataSource);

        verify(mActiveModeWarden).registerModeChangeCallback(
                mModeChangeCallbackCaptor.capture());
        assertNotNull(mModeChangeCallbackCaptor.getValue());

        // Register factory with connectivity manager.
        mWifiNetworkFactory.register();
        ArgumentCaptor<NetworkProvider> networkProviderArgumentCaptor =
                ArgumentCaptor.forClass(NetworkProvider.class);
        verify(mConnectivityManager).registerNetworkProvider(
                networkProviderArgumentCaptor.capture());
        mNetworkProvider = networkProviderArgumentCaptor.getValue();
        assertNotNull(mNetworkProvider);
        mLooper.dispatchAll();

        mNetworkRequest = new NetworkRequest.Builder()
                .setCapabilities(mNetworkCapabilities)
                .build();

        // Setup with wifi on.
        mWifiNetworkFactory.enableVerboseLogging(true);
    }

    /**
     * Called after each test
     */
    @After
    public void cleanup() {
        validateMockitoUsage();
        if (null != mStaticMockSession) {
            mStaticMockSession.finishMocking();
        }
    }

    /**
     * Validates handling of needNetworkFor.
     */
    @Test
    public void testHandleNetworkRequestWithNoSpecifier() {
        assertFalse(mWifiNetworkFactory.hasConnectionRequests());
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // First network request should turn on auto-join.
        verify(mWifiConnectivityManager).setTrustedConnectionAllowed(true);
        assertTrue(mWifiNetworkFactory.hasConnectionRequests());

        // Subsequent ones should do nothing.
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        verifyNoMoreInteractions(mWifiConnectivityManager);
    }

    /**
     * Validates handling of releaseNetwork.
     */
    @Test
    public void testHandleNetworkReleaseWithNoSpecifier() {
        // Release network with out a corresponding request should be ignored.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        assertFalse(mWifiNetworkFactory.hasConnectionRequests());

        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        assertTrue(mWifiNetworkFactory.hasConnectionRequests());
        verify(mWifiConnectivityManager).setTrustedConnectionAllowed(true);

        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        assertFalse(mWifiNetworkFactory.hasConnectionRequests());
        verify(mWifiConnectivityManager).setTrustedConnectionAllowed(false);
    }

    /**
     * Validates handling of acceptNetwork for requests with no network specifier.
     */
    @Test
    public void testHandleAcceptNetworkRequestWithNoSpecifier() {
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with an unsupported network specifier
     */
    @Test
    public void testHandleAcceptNetworkRequestFromWithUnsupportedSpecifier() throws Exception {
        // Attach an unsupported specifier.
        mNetworkCapabilities.setNetworkSpecifier(mock(NetworkSpecifier.class));
        mNetworkRequest = new NetworkRequest.Builder()
                .setCapabilities(mNetworkCapabilities)
                .build();

        // request should be rejected, but not released.
        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager, never()).declareNetworkRequestUnfulfillable(any());
    }

    /**
     * Validates that requests that specify a frequency band are rejected.
     */
    @Test
    public void testHandleAcceptNetworkRequestWithBand() throws Exception {
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier.Builder()
                .setBand(ScanResult.WIFI_BAND_5_GHZ)
                .build();
        mNetworkCapabilities.setNetworkSpecifier(specifier);
        mNetworkRequest = new NetworkRequest.Builder()
                .setCapabilities(mNetworkCapabilities)
                .build();

        // request should be rejected and released.
        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(any());
    }

    /**
     * Validates handling of acceptNetwork with a network specifier with invalid uid/package name.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromWithInvalidWifiNetworkSpecifier()
            throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_GONE);
        when(mWifiPermissionsUtil.checkNetworkSettingsPermission(TEST_UID_1))
                .thenReturn(true);
        doThrow(new SecurityException()).when(mAppOpsManager).checkPackage(anyInt(), anyString());

        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier with internet capability.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromWithInternetCapability() throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);

        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mNetworkRequest.networkCapabilities.addCapability(
                NetworkCapabilities.NET_CAPABILITY_INTERNET);

        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a non foreground
     * app/service.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromNonFgAppOrSvcWithSpecifier() throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE + 1);

        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * app.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromFgAppWithSpecifier() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);

        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from apps holding
     * NETWORK_SETTINGS.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromNetworkSettingAppWithSpecifier() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_GONE);
        when(mWifiPermissionsUtil.checkNetworkSettingsPermission(TEST_UID_1))
                .thenReturn(true);

        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * app.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromFgAppWithSpecifierWithPendingRequestFromFgSvc() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND);

        // Handle request 1.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Make request 2 which will be accepted because a fg app request can
        // override a fg service request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * app.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromFgSvcWithSpecifierWithPendingRequestFromFgSvc() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);

        // Handle request 1.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Make request 2 which will be accepted because a fg service request can
        // override an existing fg service request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * app.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromFgAppWithSpecifierWithPendingRequestFromFgApp() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND);

        // Handle request 1.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Make request 2 which will be accepted because a fg app request can
        // override an existing fg app request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * service when we're in the midst of processing a request from a foreground app.
     */
    @Test
    public void testHandleAcceptNetworkRequestFromFgSvcWithSpecifierWithPendingRequestFromFgApp()
            throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);

        // Handle request 1.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Make request 2 which will be rejected because a fg service request cannot
        // override a fg app request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * service when we're in the midst of processing the same request from a foreground app.
     * Caused by the app transitioning to a fg service & connectivity stack triggering a
     * re-evaluation.
     */
    @Test
    public void
            testHandleAcceptNetworkRequestFromFgSvcWithSpecifierWithSamePendingRequestFromFgApp()
            throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);

        // Handle request 1.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Resend the request from a fg service (should be accepted since it is already being
        // processed).
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * app when we're connected to a request from a foreground app.
     */
    @Test
    public void
            testHandleAcceptNetworkRequestFromFgAppWithSpecifierWithConnectedRequestFromFgApp()
            throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND);

        // Connect to request 1
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);
        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Make request 2 which will be accepted because a fg app request can
        // override an existing fg app request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * service when we're connected to a request from a foreground app.
     */
    @Test
    public void
            testHandleAcceptNetworkRequestFromFgSvcWithSpecifierWithConnectedRequestFromFgApp()
            throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);

        // Connect to request 1
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);
        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Make request 2 which will be rejected because a fg service request cannot
        // override a fg app request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        assertFalse(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Validates handling of acceptNetwork with a network specifier from a foreground
     * service when we're in the connected to the same request from a foreground app.
     * Caused by the app transitioning to a fg service & connectivity stack triggering a
     * re-evaluation.
     */
    @Test
    public void
            testHandleAcceptNetworkRequestFromFgSvcWithSpecifierWithSameConnectedRequestFromFgApp()
            throws Exception {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);

        // Connect to request 1
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);
        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Resend the request from a fg service (should be accepted since it is already connected).
        assertTrue(mWifiNetworkFactory.acceptRequest(mNetworkRequest));
    }

    /**
     * Verify handling of new network request with network specifier.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifier() {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Verify UI start.
        validateUiStartParams(true);

        // Verify scan settings.
        verify(mWifiScanner).startScan(mScanSettingsArgumentCaptor.capture(), any(), any(),
                mWorkSourceArgumentCaptor.capture());
        validateScanSettings(null);

        verify(mWifiMetrics).incrementNetworkRequestApiNumRequest();
    }

    /**
     * Validates handling of new network request with unsupported network specifier.
     */
    @Test
    public void testHandleNetworkRequestWithUnsupportedSpecifier() throws Exception {
        // Attach an unsupported specifier.
        mNetworkCapabilities.setNetworkSpecifier(mock(NetworkSpecifier.class));
        mNetworkRequest = new NetworkRequest.Builder()
                .setCapabilities(mNetworkCapabilities)
                .build();

        // Ignore the request, but don't release it.
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mLooper.dispatchAll();
        verify(mConnectivityManager, never()).declareNetworkRequestUnfulfillable(any());

        verifyNoMoreInteractions(mWifiScanner, mWifiConnectivityManager, mWifiMetrics);
    }

    /**
     * Validates handling of new network request with network specifier with internet capability.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierAndInternetCapability() throws Exception {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mNetworkRequest.networkCapabilities.addCapability(
                NetworkCapabilities.NET_CAPABILITY_INTERNET);

        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mLooper.dispatchAll();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify handling of new network request with network specifier for a hidden network.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierForHiddenNetwork() {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, true);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Verify UI start.
        validateUiStartParams(true);

        // Verify scan settings.
        verify(mWifiScanner).startScan(mScanSettingsArgumentCaptor.capture(), any(), any(),
                mWorkSourceArgumentCaptor.capture());
        validateScanSettings(
                ((WifiNetworkSpecifier) mNetworkCapabilities.getNetworkSpecifier())
                        .ssidPatternMatcher.getPath());

        verify(mWifiMetrics).incrementNetworkRequestApiNumRequest();
    }

    /**
     * Verify handling of new network request with network specifier for a non-hidden network
     * after processing a previous hidden network requst.
     * Validates that the scan settings was properly reset between the 2 request
     * {@link ScanSettings#hiddenNetworks}
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierAfterPreviousHiddenNetworkRequest() {
        // Hidden request 1.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, true);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        // Verify scan settings.
        verify(mWifiScanner, times(1)).startScan(mScanSettingsArgumentCaptor.capture(), any(),
                any(), mWorkSourceArgumentCaptor.capture());
        validateScanSettings(
                ((WifiNetworkSpecifier) mNetworkCapabilities.getNetworkSpecifier())
                        .ssidPatternMatcher.getPath());

        // Release request 1.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        // Regular request 2.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        // Verify scan settings.
        verify(mWifiScanner, times(2)).startScan(mScanSettingsArgumentCaptor.capture(), any(),
                any(), mWorkSourceArgumentCaptor.capture());
        validateScanSettings(null);

        verify(mWifiMetrics, times(2)).incrementNetworkRequestApiNumRequest();
    }

    /**
     * Verify handling of release of the active network request with network specifier.
     */
    @Test
    public void testHandleNetworkReleaseWithSpecifier() {
        // Make a generic request first to ensure that we re-enable auto-join after release.
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        // Make the network request with specifier.
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        verify(mWifiScanner).startScan(any(), any(), any(), any());

        // Release the network request.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // Verify that we did not trigger a disconnect because we've not yet connected.
        verify(mClientModeManager, never()).disconnect();

        verify(mWifiMetrics).incrementNetworkRequestApiNumRequest();
    }

    /**
     * Verify the periodic scan to find a network matching the network specifier.
     * Simulates the case where the network is not found in any of the scan results.
     */
    @Test
    public void testPeriodicScanNetworkRequestWithSpecifier() {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        verifyPeriodicScans(0,
                PERIODIC_SCAN_INTERVAL_MS,     // 10s
                PERIODIC_SCAN_INTERVAL_MS,     // 10s
                PERIODIC_SCAN_INTERVAL_MS,     // 10s
                PERIODIC_SCAN_INTERVAL_MS);    // 10s
    }

    /**
     * Verify the periodic scan back off to find a network matching the network specifier
     * is cancelled when the active network request is released.
     */
    @Test
    public void testPeriodicScanCancelOnReleaseNetworkRequestWithSpecifier() {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        verifyPeriodicScans(0,
                PERIODIC_SCAN_INTERVAL_MS,     // 10s
                PERIODIC_SCAN_INTERVAL_MS);    // 10s

        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // Cancel the alarm set for the next scan.
        verify(mAlarmManager).cancel(any(OnAlarmListener.class));
    }

    /**
     * Verify the periodic scan back off to find a network matching the network specifier
     * is cancelled when the user selects a network.
     */
    @Test
    public void testPeriodicScanCancelOnUserSelectNetwork() throws Exception {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());

        verifyPeriodicScans(0,
                PERIODIC_SCAN_INTERVAL_MS,     // 10s
                PERIODIC_SCAN_INTERVAL_MS);    // 10s

        // Now trigger user selection to one of the network.
        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + TEST_SSID_1 + "\"";
        sendUserSelectionSelect(mNetworkRequestUserSelectionCallback.getValue(), mSelectedNetwork);
        mLooper.dispatchAll();

        // Cancel the alarm set for the next scan.
        verify(mAlarmManager).cancel(mPeriodicScanListenerArgumentCaptor.getValue());
    }


    /**
     * Verify callback registration/unregistration.
     */
    @Test
    public void testHandleCallbackRegistrationAndUnregistration() throws Exception {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        //Ensure that we register the user selection callback using the newly registered callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                any(INetworkRequestUserSelectionCallback.class));

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback, atLeastOnce()).asBinder();

        verifyNoMoreInteractions(mNetworkRequestMatchCallback);
    }

    /**
     * Verify callback registration when the active request has already been released..
     */
    @Test
    public void testHandleCallbackRegistrationWithNoActiveRequest() throws Exception {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        //Ensure that we trigger the onAbort callback & nothing else.
        verify(mNetworkRequestMatchCallback).onAbort();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);

        verifyNoMoreInteractions(mNetworkRequestMatchCallback);
    }

    /**
     * Verify network specifier matching for a specifier containing a specific SSID match using
     * 4 WPA_PSK scan results, each with unique SSID.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidMatch() throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We only expect 1 network match in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(), mTestScanDatas[0].getResults()[0]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    /**
     * Verify network specifier matching for a specifier containing a specific SSID match using
     * 4 WPA_PSK + SAE transition scan results, each with unique SSID.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidMatchForSaeTransitionScanResult()
            throws Exception {
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK_SAE_TRANSITION,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We only expect 1 network match in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(), mTestScanDatas[0].getResults()[0]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    /**
     * Verify network specifier matching for a specifier containing a Prefix SSID match using
     * 4 open scan results, each with unique SSID.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingPrefixSsidMatch() throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_OPEN,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_PREFIX);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_OPEN);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(false);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We expect 2 scan result matches in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(),
                mTestScanDatas[0].getResults()[0], mTestScanDatas[0].getResults()[1]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    /**
     * Verify network specifier matching for a specifier containing a specific BSSID match using
     * 4 WPA_PSK scan results, each with unique SSID.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralBssidMatch() throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(".*", PatternMatcher.PATTERN_SIMPLE_GLOB);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(TEST_BSSID_1), MacAddress.BROADCAST_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We only expect 1 scan result match in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(), mTestScanDatas[0].getResults()[0]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    /**
     * Verify network specifier matching for a specifier containing a prefix BSSID match using
     * 4 WPA_EAP scan results, each with unique SSID.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingOuiPrefixBssidMatch() throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_EAP,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(".*", PatternMatcher.PATTERN_SIMPLE_GLOB);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(TEST_BSSID_1_2_OUI),
                        MacAddress.fromString(TEST_BSSID_OUI_MASK));
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_EAP);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(false);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We expect 2 scan result matches in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(),
                mTestScanDatas[0].getResults()[0], mTestScanDatas[0].getResults()[1]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    /**
     * Verify network specifier matching for a specifier containing a specific SSID match using
     * 4 WPA_PSK scan results, 3 of which have the same SSID.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidMatchWithMultipleBssidMatches()
            throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_1, TEST_SSID_1, TEST_SSID_2);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We expect 3 scan result matches in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(),
                mTestScanDatas[0].getResults()[0], mTestScanDatas[0].getResults()[1],
                mTestScanDatas[0].getResults()[2]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    /**
     * Verify network specifier match failure for a specifier containing a specific SSID match using
     * 4 WPA_PSK scan results, 2 of which SSID_1 and the other 2 SSID_2. But, none of the scan
     * results' SSID match the one requested in the specifier.
     */
    @Test
    public void testNetworkSpecifierMatchFailUsingLiteralSsidMatchWhenSsidNotFound()
            throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_1, TEST_SSID_2, TEST_SSID_2);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_3, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        // We expect no network match in this case.
        verify(mNetworkRequestMatchCallback, never()).onMatch(any());

        // Don't increment metrics until we have a match
        verify(mWifiMetrics, never()).incrementNetworkRequestApiMatchSizeHistogram(anyInt());
    }

    /**
     * Verify network specifier match failure for a specifier containing a specific SSID match using
     * 4 open scan results, each with unique SSID. But, none of the scan
     * results' key mgmt match the one requested in the specifier.
     */
    @Test
    public void testNetworkSpecifierMatchFailUsingLiteralSsidMatchWhenKeyMgmtDiffers()
            throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_OPEN,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        // We expect no network match in this case.
        verify(mNetworkRequestMatchCallback, never()).onMatch(any());
    }

    /**
     * Verify handling of stale user selection (previous request released).
     */
    @Test
    public void testNetworkSpecifierHandleUserSelectionConnectToNetworkWithoutActiveRequest()
            throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now release the active network request.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        // Now trigger user selection to some network.
        WifiConfiguration selectedNetwork = WifiConfigurationTestUtil.createOpenNetwork();
        sendUserSelectionSelect(networkRequestUserSelectionCallback, selectedNetwork);
        mLooper.dispatchAll();

        // Verify we did not attempt to trigger a connection or disable connectivity manager.
        verifyNoMoreInteractions(mClientModeManager, mWifiConnectivityManager);
    }

    /**
     * Verify handling of stale user selection (new request replacing the previous request).
     */
    @Test
    public void testNetworkSpecifierHandleUserSelectionConnectToNetworkWithDifferentActiveRequest()
            throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now send another network request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Now trigger user selection to some network.
        WifiConfiguration selectedNetwork = WifiConfigurationTestUtil.createOpenNetwork();
        sendUserSelectionSelect(networkRequestUserSelectionCallback, selectedNetwork);
        mLooper.dispatchAll();

        // Verify we did not attempt to trigger a connection or disable connectivity manager.
        verifyNoMoreInteractions(mClientModeManager, mWifiConnectivityManager, mWifiConfigManager);
    }

    /**
     * Verify handling of user selection to trigger connection to a network.
     */
    @Test
    public void testNetworkSpecifierHandleUserSelectionConnectToNetwork() throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now trigger user selection to one of the network.
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];
        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + mTestScanDatas[0].getResults()[0].SSID + "\"";
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Cancel periodic scans.
        verify(mAlarmManager).cancel(any(OnAlarmListener.class));
        // Disable connectivity manager
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(true);

        validateConnectParams(mSelectedNetwork.SSID, matchingScanResult.BSSID);
        verify(mWifiMetrics).setNominatorForNetwork(anyInt(),
                eq(WifiMetricsProto.ConnectionEvent.NOMINATOR_SPECIFIER));

        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify when number of user approved access points exceed the capacity, framework should trim
     * the Set by removing the least recently used elements.
     */
    @Test
    public void testNetworkSpecifierHandleUserSelectionConnectToNetworkExceedApprovedListCapacity()
            throws Exception {
        int userApproveAccessPointCapacity = WifiNetworkFactory.NUM_OF_ACCESS_POINT_LIMIT_PER_APP;
        int numOfApPerSsid = userApproveAccessPointCapacity / 2 + 1;
        String[] testIds = new String[]{TEST_SSID_1, TEST_SSID_2};

        // Setup up scan data
        setupScanDataSameSsidWithDiffBssid(SCAN_RESULT_TYPE_WPA_PSK, numOfApPerSsid, testIds);

        // Setup network specifier for WPA-PSK networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_PREFIX);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = WifiConfigurationTestUtil.createPskNetwork();
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        // request network, trigger scan and get matched set.
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now trigger user selection to one of the network.
        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + mTestScanDatas[0].getResults()[0].SSID + "\"";
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Verifier num of Approved access points.
        assertEquals(mWifiNetworkFactory.mUserApprovedAccessPointMap
                .get(TEST_PACKAGE_NAME_1).size(), numOfApPerSsid);

        // Now trigger user selection to another network with different SSID.
        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + mTestScanDatas[0].getResults()[numOfApPerSsid].SSID + "\"";
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Verify triggered trim when user Approved Access Points exceed capacity.
        Set<AccessPoint> userApprovedAccessPoints = mWifiNetworkFactory.mUserApprovedAccessPointMap
                .get(TEST_PACKAGE_NAME_1);
        assertEquals(userApprovedAccessPoints.size(), userApproveAccessPointCapacity);
        long numOfSsid1Aps = userApprovedAccessPoints
                .stream()
                .filter(a->a.ssid.equals(TEST_SSID_1))
                .count();
        assertEquals(numOfSsid1Aps, userApproveAccessPointCapacity - numOfApPerSsid);
    }

    /**
     * Verify handling of user selection to trigger connection to an existing saved network.
     */
    @Test
    public void testNetworkSpecifierHandleUserSelectionConnectToExistingSavedNetwork()
            throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + mTestScanDatas[0].getResults()[0].SSID + "\"";
        mSelectedNetwork.shared = false;

        // Have a saved network with the same configuration.
        WifiConfiguration matchingSavedNetwork = new WifiConfiguration(mSelectedNetwork);
        matchingSavedNetwork.networkId = TEST_NETWORK_ID_1;
        when(mWifiConfigManager.getConfiguredNetwork(mSelectedNetwork.getProfileKey()))
                .thenReturn(matchingSavedNetwork);

        // Now trigger user selection to one of the network.
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Cancel periodic scans.
        verify(mAlarmManager).cancel(any(OnAlarmListener.class));
        // Disable connectivity manager
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(true);

        // verify we don't try to add the network to WifiConfigManager.
        verify(mWifiConfigManager, never()).addOrUpdateNetwork(any(), anyInt(), anyString());

        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify handling of user selection to trigger connection to a network. Ensure we fill
     * up the BSSID field.
     */
    @Test
    public void
            testNetworkSpecifierHandleUserSelectionConnectToNetworkUsingLiteralSsidAndBssidMatch()
            throws Exception {
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Make a specific AP request.
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        // Now trigger user selection to the network.
        mSelectedNetwork = ScanResultUtil.createNetworkFromScanResult(matchingScanResult);
        mSelectedNetwork.SSID = "\"" + matchingScanResult.SSID + "\"";
        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Verify WifiConfiguration params.
        validateConnectParams(mSelectedNetwork.SSID, matchingScanResult.BSSID);
        verify(mWifiMetrics).setNominatorForNetwork(anyInt(),
                eq(WifiMetricsProto.ConnectionEvent.NOMINATOR_SPECIFIER));

        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify handling of user selection to trigger connection to a network. Ensure we fill
     * up the BSSID field with scan result for highest RSSI.
     */
    @Test
    public void
            testNetworkSpecifierHandleUserSelectionConnectToNetworkMultipleBssidMatches()
            throws Exception {
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_1, TEST_SSID_1, TEST_SSID_4);

        // Make a ssid pattern request which matches 3 scan results - 0, 1, 2.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        // Scan result 2 has the highest RSSI, so that should be picked.
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[2];

        // Now trigger user selection to the network.
        mSelectedNetwork = ScanResultUtil.createNetworkFromScanResult(matchingScanResult);
        mSelectedNetwork.SSID = "\"" + matchingScanResult.SSID + "\"";
        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Verify WifiConfiguration params.
        validateConnectParams(mSelectedNetwork.SSID, matchingScanResult.BSSID);
        verify(mWifiMetrics).setNominatorForNetwork(anyInt(),
                eq(WifiMetricsProto.ConnectionEvent.NOMINATOR_SPECIFIER));

        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify handling of user selection to trigger connection to a network when the selected bssid
     * is no longer seen in scan results within the cache expiry duration. Ensure we fill up the
     * BSSID field.
     */
    @Test
    public void
            testNetworkSpecifierHandleUserSelectionConnectToNetworkMissingBssidInLatest()
            throws Exception {
        WifiScanner.ScanData[] scanDatas1 =
                ScanTestUtil.createScanDatas(new int[][]{ { 2417, 2427, 5180, 5170 }});
        setupScanData(scanDatas1, SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);
        // Modify the next set of scan results to simulate missing |TEST_SSID_1| ScanResult.
        WifiScanner.ScanData[] scanDatas2 =
                ScanTestUtil.createScanDatas(new int[][]{ { 2417, 2427, 5180, 5170 }});
        setupScanData(scanDatas2, SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_2, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Make a specific AP request.
        ScanResult matchingScanResult = scanDatas1[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());
        verifyPeriodicScans(
                0L,
                new PeriodicScanParams(0, scanDatas1),
                new PeriodicScanParams(PERIODIC_SCAN_INTERVAL_MS, scanDatas2));

        // Now trigger user selection to the network.
        mSelectedNetwork = ScanResultUtil.createNetworkFromScanResult(matchingScanResult);
        mSelectedNetwork.SSID = "\"" + matchingScanResult.SSID + "\"";
        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Verify WifiConfiguration params.
        validateConnectParams(mSelectedNetwork.SSID, matchingScanResult.BSSID);
        verify(mWifiMetrics).setNominatorForNetwork(anyInt(),
                eq(WifiMetricsProto.ConnectionEvent.NOMINATOR_SPECIFIER));

        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify handling of user selection to trigger connection to a network when the selected bssid
     * is no longer seen in scan results beyond the cache expiry duration. Ensure we don't fill up
     * the BSSID field.
     */
    @Test
    public void
            testNetworkSpecifierHandleUserSelectionConnectToNetworkStaleMissingBssidInLatest()
            throws Exception {
        WifiScanner.ScanData[] scanDatas1 =
                ScanTestUtil.createScanDatas(new int[][]{ { 2417, 2427, 5180, 5170 }});
        setupScanData(scanDatas1, SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);
        // Modify the next set of scan results to simulate missing |TEST_SSID_1| ScanResult.
        WifiScanner.ScanData[] scanDatas2 =
                ScanTestUtil.createScanDatas(new int[][]{ { 2417, 2427, 5180, 5170 }});
        setupScanData(scanDatas2, SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_2, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Make a specific AP request.
        ScanResult matchingScanResult = scanDatas1[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());

        long nowMs = WifiNetworkFactory.CACHED_SCAN_RESULTS_MAX_AGE_IN_MILLIS + 1;
        scanDatas2[0].getResults()[0].timestamp = nowMs;
        scanDatas2[0].getResults()[1].timestamp = nowMs;
        scanDatas2[0].getResults()[2].timestamp = nowMs;
        scanDatas2[0].getResults()[3].timestamp = nowMs;
        verifyPeriodicScans(
                nowMs,
                new PeriodicScanParams(0, scanDatas1),
                new PeriodicScanParams(PERIODIC_SCAN_INTERVAL_MS, scanDatas2));

        // Now trigger user selection to the network.
        mSelectedNetwork = ScanResultUtil.createNetworkFromScanResult(matchingScanResult);
        mSelectedNetwork.SSID = "\"" + matchingScanResult.SSID + "\"";
        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Verify WifiConfiguration params.
        validateConnectParams(mSelectedNetwork.SSID, null);
        verify(mWifiMetrics).setNominatorForNetwork(anyInt(),
                eq(WifiMetricsProto.ConnectionEvent.NOMINATOR_SPECIFIER));

        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify handling of user selection to reject the request.
     */
    @Test
    public void testNetworkSpecifierHandleUserSelectionReject() throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now trigger user rejection.
        networkRequestUserSelectionCallback.reject();
        mLooper.dispatchAll();

        // Cancel periodic scans.
        verify(mAlarmManager).cancel(any(OnAlarmListener.class));
        // Verify we reset the network request handling.
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));

        verify(mWifiMetrics).incrementNetworkRequestApiNumUserReject();

        // Verify we did not attempt to trigger a connection.
        verifyNoMoreInteractions(mClientModeManager, mWifiConfigManager);
    }

    /**
     * Verify handling of connection timeout.
     * The timeouts should trigger connection retries until we hit the max.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionTimeout() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Simulate connection timeout beyond the retry limit to trigger the failure handling.
        for (int i = 0; i <= WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            mConnectionTimeoutAlarmListenerArgumentCaptor.getValue().onAlarm();
            mLooper.dispatchAll();
        }

        mInOrder = inOrder(mAlarmManager, mClientModeManager, mConnectHelper);
        validateConnectionRetryAttempts(true);

        // Fail the request after all the retries are exhausted.
        verify(mNetworkRequestMatchCallback).onAbort();
        // Verify that we sent the connection failure callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectFailure(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // Verify we reset the network request handling.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify handling of connection trigger failure.
     * The trigger failures should trigger connection retries until we hit the max.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionTriggerFailure() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send failure message beyond the retry limit to trigger the failure handling.
        for (int i = 0; i <= WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            assertNotNull(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
            mConnectListenerArgumentCaptor.getValue().sendFailure(WifiManager.ERROR);
        }
        mLooper.dispatchAll();

        mInOrder = inOrder(mAlarmManager, mClientModeManager, mConnectHelper);
        validateConnectionRetryAttempts(false);

        // Fail the request after all the retries are exhausted.
        verify(mNetworkRequestMatchCallback).onAbort();
        // Verify that we sent the connection failure callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectFailure(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        mInOrder.verify(mAlarmManager).cancel(
                mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we reset the network request handling.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify handling of connection failure.
     * The connection failures should trigger connection retries until we hit the max.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionFailure() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        assertNotNull(mSelectedNetwork);

        // Send network connection failure indication beyond the retry limit to trigger the failure
        // handling.
        for (int i = 0; i <= WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            mWifiNetworkFactory.handleConnectionAttemptEnded(
                    WifiMetrics.ConnectionEvent.FAILURE_DHCP, mSelectedNetwork, TEST_BSSID_1);
            mLooper.dispatchAll();
        }

        mInOrder = inOrder(mAlarmManager, mClientModeManager, mConnectHelper);
        validateConnectionRetryAttempts(false);

        verify(mNetworkRequestMatchCallback).onAbort();
        // Verify that we sent the connection failure callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectFailure(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        mInOrder.verify(mAlarmManager).cancel(
                mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we reset the network request handling.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify handling of connection failure to a different network.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionFailureToWrongNetwork() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection failure to a different network indication.
        assertNotNull(mSelectedNetwork);
        WifiConfiguration connectedNetwork = new WifiConfiguration(mSelectedNetwork);
        connectedNetwork.SSID = TEST_SSID_2;
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_DHCP, connectedNetwork, TEST_BSSID_1);

        // Verify that we did not send the connection failure callback.
        verify(mNetworkRequestMatchCallback, never()).onUserSelectionConnectFailure(any());
        // verify we canceled the timeout alarm.
        verify(mAlarmManager, never())
                .cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we don't reset the network request handling.
        verify(mWifiConnectivityManager, never())
                .setSpecificNetworkRequestInProgress(false);

        // Send network connection failure indication beyond the retry limit to trigger the failure
        // handling.
        for (int i = 0; i <= WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            mWifiNetworkFactory.handleConnectionAttemptEnded(
                    WifiMetrics.ConnectionEvent.FAILURE_DHCP, mSelectedNetwork, TEST_BSSID_1);
            mLooper.dispatchAll();
        }

        mInOrder = inOrder(mAlarmManager, mClientModeManager, mConnectHelper);
        validateConnectionRetryAttempts(false);

        // Verify that we sent the connection failure callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectFailure(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        mInOrder.verify(mAlarmManager).cancel(
                mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we reset the network request handling.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify handling of connection failure to a different bssid.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionFailureToWrongBssid() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection failure to a different bssid indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_DHCP, mSelectedNetwork, TEST_BSSID_2);

        // Verify that we did not send the connection failure callback.
        verify(mNetworkRequestMatchCallback, never()).onUserSelectionConnectFailure(any());
        // verify we canceled the timeout alarm.
        verify(mAlarmManager, never())
                .cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we don't reset the network request handling.
        verify(mWifiConnectivityManager, never())
                .setSpecificNetworkRequestInProgress(false);

        // Send network connection failure indication beyond the retry limit to trigger the failure
        // handling.
        for (int i = 0; i <= WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            mWifiNetworkFactory.handleConnectionAttemptEnded(
                    WifiMetrics.ConnectionEvent.FAILURE_DHCP, mSelectedNetwork, TEST_BSSID_1);
            mLooper.dispatchAll();
        }

        mInOrder = inOrder(mAlarmManager, mClientModeManager, mConnectHelper);
        validateConnectionRetryAttempts(false);

        // Verify that we sent the connection failure callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectFailure(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        mInOrder.verify(mAlarmManager).cancel(
                mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we reset the network request handling.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify handling of connection success.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionSuccess() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we disabled fw roaming.
        verify(mClientModeManager).enableRoaming(false);

        // Now release the active network request.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnPrimaryIface();
        // Ensure that we toggle auto-join state.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(true);
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mClientModeManager).enableRoaming(true);
    }

    /**
     * Verify handling of connection success.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionSuccessWhenUseHalApiIsDisabled()
            throws Exception {
        when(mResources.getBoolean(
                eq(R.bool.config_wifiUseHalApiToDisableFwRoaming)))
                .thenReturn(false);
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we disabled fw roaming.
        verify(mClientModeManager, never()).enableRoaming(false);

        // Now release the active network request.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnPrimaryIface();
        // Ensure that we toggle auto-join state.
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(true);
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mClientModeManager, never()).enableRoaming(true);
    }

    /**
     * Verify handling of connection success.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionSuccessOnSecondaryCmm()
            throws Exception {
        when(mClientModeManager.getRole()).thenReturn(ActiveModeManager.ROLE_CLIENT_LOCAL_ONLY);
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify we disabled fw roaming.
        verify(mClientModeManager).enableRoaming(false);

        // Now release the active network request.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnSecondaryIface();
        // Don't toggle auto-join state.
        verify(mWifiConnectivityManager, never()).setSpecificNetworkRequestInProgress(anyBoolean());
        verify(mClientModeManager).enableRoaming(true);
    }

    /**
     * Verify that we ignore connection success events after the first one (may be triggered by a
     * roam event)
     */
    @Test
    public void testNetworkSpecifierDuplicateHandleConnectionSuccess() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnPrimaryIface();
        verify(mNetworkRequestMatchCallback, atLeastOnce()).asBinder();

        // Send second network connection success indication which should be ignored.
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        verifyNoMoreInteractions(mNetworkRequestMatchCallback);
    }

    /**
     * Verify that we ignore any connection failure events after the first connection success (may
     * be triggered by a disconnect).
     * Note: The disconnect handling will be done via the NetworkAgent.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionFailureAfterSuccess() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnPrimaryIface();
        verify(mNetworkRequestMatchCallback, atLeastOnce()).asBinder();

        // Send a network connection failure indication which should be ignored (beyond the retry
        // limit to trigger the failure handling).
        for (int i = 0; i <= WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            mWifiNetworkFactory.handleConnectionAttemptEnded(
                    WifiMetrics.ConnectionEvent.FAILURE_DHCP, mSelectedNetwork, TEST_BSSID_1);
            mLooper.dispatchAll();
        }
        // Verify that we ignore the second connection failure callback.
        verifyNoMoreInteractions(mNetworkRequestMatchCallback);
    }

    /**
     * Verify handling of connection success to a different network.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionSuccessToWrongNetwork() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success to a different network indication.
        assertNotNull(mSelectedNetwork);
        WifiConfiguration connectedNetwork = new WifiConfiguration(mSelectedNetwork);
        connectedNetwork.SSID = TEST_SSID_2;
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, connectedNetwork, TEST_BSSID_1);

        // verify that we did not send out the success callback and did not stop the alarm timeout.
        verify(mNetworkRequestMatchCallback, never()).onUserSelectionConnectSuccess(any());
        verify(mAlarmManager, never())
                .cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        // Send network connection success to the correct network indication.
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
    }

    /**
     * Verify handling of connection success to a different BSSID.
     */
    @Test
    public void testNetworkSpecifierHandleConnectionSuccessToWrongBssid() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success to a different network indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_2);

        // verify that we did not send out the success callback and did not stop the alarm timeout.
        verify(mNetworkRequestMatchCallback, never()).onUserSelectionConnectSuccess(any());
        verify(mAlarmManager, never())
                .cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        // Send network connection success to the correct network indication.
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
    }

    /**
     * Verify handling of request release after starting connection to the network.
     */
    @Test
    public void testHandleNetworkReleaseWithSpecifierAfterConnectionStart() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        assertNotNull(mSelectedNetwork);

        // Now release the network request.
        WifiConfiguration wcmNetwork = new WifiConfiguration(mSelectedNetwork);
        wcmNetwork.networkId = TEST_NETWORK_ID_1;
        wcmNetwork.creatorUid = TEST_UID_1;
        wcmNetwork.creatorName = TEST_PACKAGE_NAME_1;
        wcmNetwork.shared = false;
        wcmNetwork.fromWifiNetworkSpecifier = true;
        wcmNetwork.ephemeral = true;
        when(mWifiConfigManager.getConfiguredNetwork(wcmNetwork.getProfileKey()))
                .thenReturn(wcmNetwork);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        // Verify that we triggered a disconnect.
        verify(mClientModeManager, times(2)).disconnect();
        verify(mWifiConfigManager).removeNetwork(
                TEST_NETWORK_ID_1, TEST_UID_1, TEST_PACKAGE_NAME_1);
        // Re-enable connectivity manager .
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
    }

    /**
     * Verify handling of request release after connecting to the network.
     */
    @Test
    public void testHandleNetworkReleaseWithSpecifierAfterConnectionSuccess() throws Exception {
        when(mClock.getElapsedSinceBootMillis()).thenReturn(0L);
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnPrimaryIface();

        long connectionDurationMillis = 5665L;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(connectionDurationMillis);

        // Now release the network request.
        WifiConfiguration wcmNetwork = new WifiConfiguration(mSelectedNetwork);
        wcmNetwork.networkId = TEST_NETWORK_ID_1;
        wcmNetwork.creatorUid = TEST_UID_1;
        wcmNetwork.creatorName = TEST_PACKAGE_NAME_1;
        wcmNetwork.shared = false;
        wcmNetwork.fromWifiNetworkSpecifier = true;
        wcmNetwork.ephemeral = true;
        when(mWifiConfigManager.getConfiguredNetwork(wcmNetwork.getProfileKey()))
                .thenReturn(wcmNetwork);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // Verify that we triggered a disconnect.
        verify(mClientModeManager, times(2)).disconnect();
        verify(mWifiConfigManager).removeNetwork(
                TEST_NETWORK_ID_1, TEST_UID_1, TEST_PACKAGE_NAME_1);
        // Re-enable connectivity manager .
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mWifiMetrics).incrementNetworkRequestApiConnectionDurationSecOnPrimaryIfaceHistogram(
                toIntExact(TimeUnit.MILLISECONDS.toSeconds(connectionDurationMillis)));
    }

    /**
     * Verify handling of request release after connecting to the network.
     */
    @Test
    public void testHandleNetworkReleaseWithSpecifierAfterConnectionSuccessOnSecondaryCmm()
            throws Exception {
        when(mClientModeManager.getRole()).thenReturn(ActiveModeManager.ROLE_CLIENT_LOCAL_ONLY);
        when(mClock.getElapsedSinceBootMillis()).thenReturn(0L);
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnSecondaryIface();

        // Now release the network request.
        long connectionDurationMillis = 5665L;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(connectionDurationMillis);
        WifiConfiguration wcmNetwork = new WifiConfiguration(mSelectedNetwork);
        wcmNetwork.networkId = TEST_NETWORK_ID_1;
        wcmNetwork.creatorUid = TEST_UID_1;
        wcmNetwork.creatorName = TEST_PACKAGE_NAME_1;
        wcmNetwork.shared = false;
        wcmNetwork.fromWifiNetworkSpecifier = true;
        wcmNetwork.ephemeral = true;
        when(mWifiConfigManager.getConfiguredNetwork(wcmNetwork.getProfileKey()))
                .thenReturn(wcmNetwork);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // Verify that we triggered a disconnect.
        verify(mClientModeManager, times(2)).disconnect();
        verify(mWifiConfigManager).removeNetwork(
                TEST_NETWORK_ID_1, TEST_UID_1, TEST_PACKAGE_NAME_1);
        // Re-enable connectivity manager .
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mWifiMetrics)
                .incrementNetworkRequestApiConnectionDurationSecOnSecondaryIfaceHistogram(
                        toIntExact(TimeUnit.MILLISECONDS.toSeconds(connectionDurationMillis)));
    }

    @Test
    public void testMetricsUpdateForConcurrentConnections() throws Exception {
        when(mClientModeManager.getRole()).thenReturn(ActiveModeManager.ROLE_CLIENT_LOCAL_ONLY);
        when(mActiveModeWarden.getClientModeManagers()).thenReturn(
                Arrays.asList(mClientModeManager));

        when(mClock.getElapsedSinceBootMillis()).thenReturn(0L);
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);

        verify(mCmiMonitor).registerListener(mCmiListenerCaptor.capture());

        // Verify that we sent the connection success callback.
        verify(mNetworkRequestMatchCallback).onUserSelectionConnectSuccess(
                argThat(new WifiConfigMatcher(mSelectedNetwork)));
        // verify we canceled the timeout alarm.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mWifiMetrics).incrementNetworkRequestApiNumConnectSuccessOnSecondaryIface();

        // Indicate secondary connection via CMI listener.
        when(mClientModeManager.isConnected()).thenReturn(true);
        mCmiListenerCaptor.getValue().onL3Connected(mClientModeManager);

        // Now indicate connection on the primary STA.
        ConcreteClientModeManager primaryCmm = mock(ConcreteClientModeManager.class);
        when(primaryCmm.isConnected()).thenReturn(true);
        when(primaryCmm.getRole()).thenReturn(ActiveModeManager.ROLE_CLIENT_PRIMARY);
        when(mActiveModeWarden.getClientModeManagers()).thenReturn(
                Arrays.asList(mClientModeManager, primaryCmm));
        mCmiListenerCaptor.getValue().onL3Connected(primaryCmm);

        // verify metrics update for concurrent connection count.
        verify(mWifiMetrics).incrementNetworkRequestApiNumConcurrentConnection();

        // Now indicate end of connection on primary STA
        long primaryConnectionDurationMillis1 = 5665L;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(primaryConnectionDurationMillis1);
        when(primaryCmm.isConnected()).thenReturn(false);
        mCmiListenerCaptor.getValue().onConnectionEnd(primaryCmm);
        // verify metrics update for concurrent connection duration.
        verify(mWifiMetrics)
                .incrementNetworkRequestApiConcurrentConnectionDurationSecHistogram(
                        toIntExact(TimeUnit.MILLISECONDS.toSeconds(
                                primaryConnectionDurationMillis1)));

        // Indicate a new connection on primary STA.
        when(primaryCmm.isConnected()).thenReturn(true);
        mCmiListenerCaptor.getValue().onL3Connected(primaryCmm);

        // verify that we did not update metrics gain for concurrent connection count.
        verify(mWifiMetrics, times(1)).incrementNetworkRequestApiNumConcurrentConnection();

        // Now indicate end of new connection on primary STA
        long primaryConnectionDurationMillis2 = 5665L;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(primaryConnectionDurationMillis2);
        when(primaryCmm.isConnected()).thenReturn(false);
        mCmiListenerCaptor.getValue().onConnectionEnd(primaryCmm);
        // verify metrics update for concurrent connection duration.
        verify(mWifiMetrics)
                .incrementNetworkRequestApiConcurrentConnectionDurationSecHistogram(
                        toIntExact(TimeUnit.MILLISECONDS.toSeconds(
                                primaryConnectionDurationMillis2)));

        // Now release the network request.
        long secondaryConnectionDurationMillis = 5665L;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(secondaryConnectionDurationMillis);
        WifiConfiguration wcmNetwork = new WifiConfiguration(mSelectedNetwork);
        wcmNetwork.networkId = TEST_NETWORK_ID_1;
        wcmNetwork.creatorUid = TEST_UID_1;
        wcmNetwork.creatorName = TEST_PACKAGE_NAME_1;
        wcmNetwork.shared = false;
        wcmNetwork.fromWifiNetworkSpecifier = true;
        wcmNetwork.ephemeral = true;
        when(mWifiConfigManager.getConfiguredNetwork(wcmNetwork.getProfileKey()))
                .thenReturn(wcmNetwork);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // Verify that we triggered a disconnect.
        verify(mClientModeManager, times(2)).disconnect();
        verify(mWifiConfigManager).removeNetwork(
                TEST_NETWORK_ID_1, TEST_UID_1, TEST_PACKAGE_NAME_1);
        // Re-enable connectivity manager .
        verify(mActiveModeWarden).removeClientModeManager(any());
        verify(mWifiMetrics)
                .incrementNetworkRequestApiConnectionDurationSecOnSecondaryIfaceHistogram(
                        toIntExact(TimeUnit.MILLISECONDS.toSeconds(
                                secondaryConnectionDurationMillis)));
    }


    /**
     * Verify we return the correct UID when processing network request with network specifier.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierGetUid() throws Exception {
        assertEquals(Integer.valueOf(Process.INVALID_UID),
                mWifiNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                        new WifiConfiguration(), new String()).first);
        assertTrue(mWifiNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                new WifiConfiguration(), new String()).second.isEmpty());

        sendNetworkRequestAndSetupForConnectionStatus();
        assertNotNull(mSelectedNetwork);

        // connected to a different network.
        WifiConfiguration connectedNetwork = new WifiConfiguration(mSelectedNetwork);
        connectedNetwork.SSID += "test";
        assertEquals(Integer.valueOf(Process.INVALID_UID),
                mWifiNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                        new WifiConfiguration(), new String()).first);
        assertTrue(mWifiNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                new WifiConfiguration(), new String()).second.isEmpty());

        // connected to the correct network.
        connectedNetwork = new WifiConfiguration(mSelectedNetwork);
        String connectedBssid = TEST_BSSID_1;
        assertEquals(Integer.valueOf(TEST_UID_1),
                mWifiNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                        connectedNetwork, connectedBssid).first);
        assertEquals(TEST_PACKAGE_NAME_1,
                mWifiNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                        connectedNetwork, connectedBssid).second);
    }

    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierWhenAwaitingCmRetrieval() throws Exception {
        doNothing().when(mActiveModeWarden).requestLocalOnlyClientModeManager(
                any(), any(), any(), any());

        when(mClock.getElapsedSinceBootMillis()).thenReturn(0L);

        sendNetworkRequestAndSetupForUserSelection();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now trigger user selection to one of the network.
        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + TEST_SSID_1 + "\"";
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        ArgumentCaptor<ActiveModeWarden.ExternalClientModeManagerRequestListener> cmListenerCaptor =
                ArgumentCaptor.forClass(
                        ActiveModeWarden.ExternalClientModeManagerRequestListener.class);
        verify(mActiveModeWarden).requestLocalOnlyClientModeManager(
                cmListenerCaptor.capture(), eq(new WorkSource(TEST_UID_1, TEST_PACKAGE_NAME_1)),
                eq("\"" + TEST_SSID_1 + "\""), eq(TEST_BSSID_1));
        assertNotNull(cmListenerCaptor.getValue());

        NetworkRequest oldRequest = new NetworkRequest(mNetworkRequest);
        // Send second request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mLooper.dispatchAll();

        // Verify that we aborted the old request.
        verify(mNetworkRequestMatchCallback).onAbort();
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(oldRequest));

        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback, times(2)).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());

        // Now trigger user selection to one of the network.
        networkRequestUserSelectionCallback = mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        // Ensure we request a new ClientModeManager.
        verify(mActiveModeWarden, times(2)).requestLocalOnlyClientModeManager(
                any(), any(), any(), any());

        // Now return the CM instance for the previous request.
        cmListenerCaptor.getValue().onAnswer(mClientModeManager);

        // Ensure we continued processing the new request.
        verify(mClientModeManager).disconnect();
        verify(mConnectHelper).connectToNetwork(
                eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierWhenScanning() throws Exception {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Register callback.
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(any());

        NetworkRequest oldRequest = new NetworkRequest(mNetworkRequest);
        // Send second request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mLooper.dispatchAll();

        verify(mNetworkRequestMatchCallback).onAbort();
        verify(mNetworkRequestMatchCallback, atLeastOnce()).asBinder();
        verify(mWifiScanner, times(2)).getSingleScanResults();
        verify(mWifiScanner, times(2)).startScan(any(), any(), any(), any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(oldRequest));

        // Remove the stale request1 & ensure nothing happens.
        mWifiNetworkFactory.releaseNetworkFor(oldRequest);

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager, mNetworkRequestMatchCallback);

        // Remove the active request2.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
    }

    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierAfterMatch() throws Exception {
        sendNetworkRequestAndSetupForUserSelection();
        WifiNetworkSpecifier specifier1 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        NetworkRequest oldRequest = new NetworkRequest(mNetworkRequest);
        // Send second request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Ensure we don't request a new ClientModeManager.
        verify(mActiveModeWarden, never()).requestLocalOnlyClientModeManager(
                any(), any(), any(), any());

        // Ignore stale callbacks.
        WifiConfiguration selectedNetwork = WifiConfigurationTestUtil.createOpenNetwork();
        sendUserSelectionSelect(networkRequestUserSelectionCallback, selectedNetwork);
        mLooper.dispatchAll();

        verify(mNetworkRequestMatchCallback).onAbort();
        verify(mNetworkRequestMatchCallback, atLeastOnce()).asBinder();
        verify(mWifiScanner, times(2)).getSingleScanResults();
        verify(mWifiScanner, times(2)).startScan(any(), any(), any(), any());
        verify(mAlarmManager).cancel(mPeriodicScanListenerArgumentCaptor.getValue());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(oldRequest));

        // Remove the stale request1 & ensure nothing happens.
        mWifiNetworkFactory.releaseNetworkFor(oldRequest);

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager, mNetworkRequestMatchCallback);

        // Remove the active request2.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
    }

    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierAfterConnect() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();
        WifiNetworkSpecifier specifier1 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();

        NetworkRequest oldRequest = new NetworkRequest(mNetworkRequest);
        // Send second request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Ensure we don't request a new ClientModeManager.
        verify(mActiveModeWarden, times(1)).requestLocalOnlyClientModeManager(
                any(), any(), any(), any());

        verify(mNetworkRequestMatchCallback).onAbort();
        verify(mNetworkRequestMatchCallback, atLeastOnce()).asBinder();
        verify(mWifiConnectivityManager, times(1)).setSpecificNetworkRequestInProgress(true);
        verify(mWifiScanner, times(2)).getSingleScanResults();
        verify(mWifiScanner, times(2)).startScan(any(), any(), any(), any());
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mClientModeManager, times(3)).getRole();

        // Remove the stale request1 & ensure nothing happens.
        mWifiNetworkFactory.releaseNetworkFor(oldRequest);

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager, mNetworkRequestMatchCallback);

        // Remove the active request2 & ensure auto-join is re-enabled.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mActiveModeWarden).removeClientModeManager(any());
    }

    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierAfterConnectionSuccess() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();
        WifiNetworkSpecifier specifier1 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);
        // Cancel the connection timeout.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mClientModeManager).enableRoaming(false);

        NetworkRequest oldRequest = new NetworkRequest(mNetworkRequest);
        // Send second request.
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_2, false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Ensure we don't request a new ClientModeManager.
        verify(mActiveModeWarden, times(1)).requestLocalOnlyClientModeManager(
                any(), any(), any(), any());

        verify(mWifiConnectivityManager, times(1)).setSpecificNetworkRequestInProgress(true);
        verify(mWifiScanner, times(2)).getSingleScanResults();
        verify(mWifiScanner, times(2)).startScan(any(), any(), any(), any());
        // we shouldn't disconnect until the user accepts the next request.
        verify(mClientModeManager, times(1)).disconnect();

        // Remove the connected request1 & ensure we disconnect.
        mWifiNetworkFactory.releaseNetworkFor(oldRequest);
        verify(mClientModeManager, times(2)).disconnect();
        verify(mClientModeManager, times(3)).getRole();

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);

        // Now remove the active request2 & ensure auto-join is re-enabled.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verify(mClientModeManager, times(3)).getRole();
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mClientModeManager).enableRoaming(true);
        verify(mActiveModeWarden).removeClientModeManager(any());

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);
    }

    /**
     *  Verify handling for same network request while connected to it.
     */
    @Test
    public void testHandleSameNetworkRequestWithSpecifierAfterConnectionSuccess() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);
        // Cancel the connection timeout.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        clearInvocations(mWifiConnectivityManager, mWifiScanner, mClientModeManager, mAlarmManager);

        // Send same request again (nothing should happen).
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mLooper.dispatchAll();

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);
    }
    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierWhichUserSelectedAfterConnectionSuccess()
            throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);
        WifiNetworkSpecifier specifier1 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);
        // Cancel the connection timeout.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mClientModeManager).enableRoaming(false);

        // Send second request & we simulate the user selecting the request & connecting to it.
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager);
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_2);
        WifiNetworkSpecifier specifier2 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_2);
        verify(mClientModeManager, times(2)).enableRoaming(false);
        // Cancel the connection timeout.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        // We shouldn't explicitly disconnect, the new connection attempt will implicitly disconnect
        // from the connected network.
        verify(mClientModeManager, times(2)).disconnect();
        verify(mClientModeManager, times(6)).getRole();

        // Remove the stale request1 & ensure nothing happens (because it was replaced by the
        // second request)
        mNetworkRequest.networkCapabilities.setNetworkSpecifier(specifier1);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);

        // Now remove the rejected request2, ensure we disconnect & re-enable auto-join.
        mNetworkRequest.networkCapabilities.setNetworkSpecifier(specifier2);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        verify(mClientModeManager, times(3)).disconnect();
        verify(mClientModeManager, times(6)).getRole();
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mClientModeManager).enableRoaming(true);
        verify(mActiveModeWarden).removeClientModeManager(any());

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);
    }

    /**
     *  Verify handling for new network request while processing another one.
     */
    @Test
    public void testHandleNewNetworkRequestWithSpecifierWhichUserRejectedAfterConnectionSuccess()
            throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);
        WifiNetworkSpecifier specifier1 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();

        // Send network connection success indication.
        assertNotNull(mSelectedNetwork);
        mWifiNetworkFactory.handleConnectionAttemptEnded(
                WifiMetrics.ConnectionEvent.FAILURE_NONE, mSelectedNetwork, TEST_BSSID_1);
        // Cancel the connection timeout.
        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());

        // Send second request & we simulate the user rejecting the request.
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager);
        sendNetworkRequestAndSetupForUserSelection(TEST_SSID_2);
        WifiNetworkSpecifier specifier2 =
                (WifiNetworkSpecifier) mNetworkRequest.networkCapabilities.getNetworkSpecifier();
        mNetworkRequestUserSelectionCallback.getValue().reject();
        mLooper.dispatchAll();
        // cancel periodic scans.
        verify(mAlarmManager).cancel(mPeriodicScanListenerArgumentCaptor.getValue());
        // Verify we disabled fw roaming.
        verify(mClientModeManager).enableRoaming(false);

        // we shouldn't disconnect/re-enable auto-join until the connected request is released.
        verify(mWifiConnectivityManager, never()).setSpecificNetworkRequestInProgress(false);
        verify(mClientModeManager, times(1)).disconnect();

        // Remove the connected request1 & ensure we disconnect & ensure auto-join is re-enabled.
        mNetworkRequest.networkCapabilities.setNetworkSpecifier(specifier1);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        verify(mClientModeManager, times(2)).disconnect();
        verify(mClientModeManager, times(3)).getRole();
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mClientModeManager).enableRoaming(true);
        verify(mActiveModeWarden).removeClientModeManager(any());

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);

        // Now remove the rejected request2 & ensure nothing happens
        mNetworkRequest.networkCapabilities.setNetworkSpecifier(specifier2);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);

        verifyNoMoreInteractions(mWifiConnectivityManager, mWifiScanner, mClientModeManager,
                mAlarmManager);
    }

    /**
     * Verify handling of screen state changes while triggering periodic scans to find matching
     * networks.
     */
    @Test
    public void testNetworkSpecifierHandleScreenStateChangedWhileScanning() throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        // Turn off screen.
        setScreenState(false);

        // 1. Cancel the scan timer.
        mInOrder.verify(mAlarmManager).cancel(
                mPeriodicScanListenerArgumentCaptor.getValue());
        // 2. Simulate the scan results from an ongoing scan, ensure no more scans are scheduled.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas);

        // Ensure no more interactions.
        mInOrder.verifyNoMoreInteractions();

        // Now, turn the screen on.
        setScreenState(true);

        // Verify that we resumed periodic scanning.
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any(), any());
    }

    /**
     * Verify handling of screen state changes after the active network request was released.
     */
    @Test
    public void testNetworkSpecifierHandleScreenStateChangedWithoutActiveRequest()
            throws Exception {
        sendNetworkRequestAndSetupForUserSelection();
        // Now release the active network request.
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        // Cancel the scan timer on release.
        mInOrder.verify(mAlarmManager).cancel(
                mPeriodicScanListenerArgumentCaptor.getValue());

        // Turn off screen.
        setScreenState(false);

        // Now, turn the screen on.
        setScreenState(true);

        // Ensure that we did not pause or resume scanning.
        mInOrder.verifyNoMoreInteractions();
    }

    /**
     * Verify handling of screen state changes after user selected a network to connect to.
     */
    @Test
    public void testNetworkSpecifierHandleScreenStateChangedAfterUserSelection() throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus();

        // Turn off screen.
        setScreenState(false);

        // Now, turn the screen on.
        setScreenState(true);

        // Ensure that we did not pause or resume scanning.
        mInOrder.verifyNoMoreInteractions();
    }

    /**
     * Verify handling of new network request with network specifier when wifi is off.
     * The request should be rejected immediately.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierWhenWifiOff() {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        // wifi off
        when(mActiveModeWarden.hasPrimaryClientModeManager()).thenReturn(false);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        verify(mWifiScanner, never()).startScan(any(), any(), any(), any());

        // wifi on
        when(mActiveModeWarden.hasPrimaryClientModeManager()).thenReturn(true);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        verify(mWifiScanner).startScan(any(), any(), any(), any());
    }

    /**
     *  Verify wifi toggle off when scanning.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierWifiOffWhenScanning() throws Exception {
        sendNetworkRequestAndSetupForUserSelection();

        // toggle wifi off & verify we aborted ongoing request (CMM not retrieved yet).
        when(mActiveModeWarden.hasPrimaryClientModeManager()).thenReturn(false);
        when(mClientModeManager.getRole()).thenReturn(null); // Role returned is null on removal.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerRemoved(
                mock(ConcreteClientModeManager.class));
        mLooper.dispatchAll();
        verify(mNetworkRequestMatchCallback).onAbort();
    }

    /**
     *  Verify wifi toggle off after connection attempt is started.
     */
    @Test
    public void testHandleNetworkRequestWithSpecifierWifiOffAfterConnect() throws Exception {
        mWifiNetworkFactory.enableVerboseLogging(true);
        sendNetworkRequestAndSetupForConnectionStatus();

        // toggle wifi off & verify we aborted ongoing request.
        when(mClientModeManager.getRole()).thenReturn(null); // Role returned is null on removal.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerRemoved(mClientModeManager);
        mLooper.dispatchAll();

        verify(mAlarmManager).cancel(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        verify(mNetworkRequestMatchCallback).onAbort();
        verify(mWifiConnectivityManager).setSpecificNetworkRequestInProgress(false);
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
        verify(mActiveModeWarden).removeClientModeManager(any());
    }

    /**
     * Verify handling of new network request with network specifier when wifi is off.
     * The request should be rejected immediately.
     */
    @Test
    public void testFullHandleNetworkRequestWithSpecifierWhenWifiOff() {
        attachDefaultWifiNetworkSpecifierAndAppInfo(TEST_UID_1, false);

        // wifi off
        when(mActiveModeWarden.hasPrimaryClientModeManager()).thenReturn(false);
        // Add the request, should do nothing.
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mLooper.dispatchAll();
        verify(mWifiScanner, never()).startScan(any(), any(), any(), any());
        verify(mConnectivityManager).declareNetworkRequestUnfulfillable(eq(mNetworkRequest));
    }

    /**
     * Verify that we don't bypass user approval for a specific request for an access point that was
     * approved previously, but then the user forgot it sometime after.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchApprovedNForgot()
            throws Exception {
        // 1. First request (no user approval bypass)
        sendNetworkRequestAndSetupForConnectionStatus();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager, mClientModeManager,
                mConnectHelper);

        // 2. Simulate user forgeting the network.
        when(mWifiConfigManager.isNetworkTemporarilyDisabledByUser(
                ScanResultUtil.createQuotedSSID(mTestScanDatas[0].getResults()[0].SSID)))
                .thenReturn(true);

        // 3. Second request for the same access point (user approval bypass).
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch,
                WifiConfigurationTestUtil.createPskNetwork(), TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);
        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        // Verify we triggered the match callback.
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());
        assertNotNull(matchedScanResultsCaptor.getValue());
        validateScanResults(matchedScanResultsCaptor.getValue(), matchingScanResult);
        // Verify that we did not send a connection attempt to ModeImplProxy.
        verify(mConnectHelper, never()).connectToNetwork(any(), any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify that we don't bypass user approval for a specific request for an access point that was
     * not approved previously.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchNotApproved()
            throws Exception {
        // 1. First request (no user approval bypass)
        sendNetworkRequestAndSetupForConnectionStatus();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager, mClientModeManager,
                mConnectHelper);

        // 2. Second request for a different access point (but same network).
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_1, TEST_SSID_3, TEST_SSID_4);
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[1];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, WifiConfigurationTestUtil.createPskNetwork(),
                TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);
        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        // Verify we triggered the match callback.
        matchedScanResultsCaptor = ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());
        assertNotNull(matchedScanResultsCaptor.getValue());
        validateScanResults(matchedScanResultsCaptor.getValue(), matchingScanResult);
        // Verify that we did not send a connection attempt to ClientModeManager.
        verify(mConnectHelper, never()).connectToNetwork(any(), any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify that we don't bypass user approval for a specific request for a network
     * (not access point) that was approved previously.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidMatchApproved()
            throws Exception {
        // 1. First request (no user approval bypass)
        sendNetworkRequestAndSetupForConnectionStatus();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager, mClientModeManager,
                mConnectHelper);

        // 2. Second request for the same network (but not specific access point)
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        // match-all.
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, WifiConfigurationTestUtil.createPskNetwork(),
                TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);
        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        // Verify we triggered the match callback.
        matchedScanResultsCaptor = ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());
        assertNotNull(matchedScanResultsCaptor.getValue());
        validateScanResults(matchedScanResultsCaptor.getValue(), matchingScanResult);
        // Verify that we did not send a connection attempt to ClientModeManager.
        verify(mConnectHelper, never()).connectToNetwork(any(), any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify the we don't bypass user approval for a specific request for an access point that was
     * already approved previously, but was then removed (app uninstalled, user deleted it from
     * notification, from tests, etc).
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchAfterApprovalsRemove()
            throws Exception {
        // 1. First request (no user approval bypass)
        sendNetworkRequestAndSetupForConnectionStatus();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager, mClientModeManager,
                mConnectHelper);

        // 2. Remove all approvals for the app.
        mWifiNetworkFactory.removeUserApprovedAccessPointsForApp(TEST_PACKAGE_NAME_1);

        // 3. Second request for the same access point
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, WifiConfigurationTestUtil.createPskNetwork(),
                TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);
        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        // Verify we triggered the match callback.
        matchedScanResultsCaptor = ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());
        assertNotNull(matchedScanResultsCaptor.getValue());
        validateScanResults(matchedScanResultsCaptor.getValue(), matchingScanResult);
        // Verify that we did not send a connection attempt to ClientModeManager.
        verify(mConnectHelper, never()).connectToNetwork(any(), any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify the we don't bypass user approval for a specific request for an access point that was
     * already approved previously, but then the user perform network settings reset.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchAfterClear()
            throws Exception {
        // 1. First request (no user approval bypass)
        sendNetworkRequestAndSetupForConnectionStatus();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        mWifiNetworkFactory.releaseNetworkFor(mNetworkRequest);
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager, mClientModeManager,
                mConnectHelper);

        // 2. Remove all approvals.
        mWifiNetworkFactory.clear();

        // 3. Second request for the same access point
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, WifiConfigurationTestUtil.createPskNetwork(),
                TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);
        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        // Verify we triggered the match callback.
        matchedScanResultsCaptor = ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());
        assertNotNull(matchedScanResultsCaptor.getValue());
        validateScanResults(matchedScanResultsCaptor.getValue(), matchingScanResult);
        // Verify that we did not send a connection attempt to ClientModeManager.
        verify(mConnectHelper, never()).connectToNetwork(any(), any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify the config store save for store user approval.
     */
    @Test
    public void testNetworkSpecifierUserApprovalConfigStoreSave()
            throws Exception {
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);

        // Verify config store interactions.
        verify(mWifiConfigManager).saveToStore(true);
        assertTrue(mDataSource.hasNewDataToSerialize());

        Map<String, Set<AccessPoint>> approvedAccessPointsMapToWrite = mDataSource.toSerialize();
        assertEquals(1, approvedAccessPointsMapToWrite.size());
        assertTrue(approvedAccessPointsMapToWrite.keySet().contains(TEST_PACKAGE_NAME_1));
        Set<AccessPoint> approvedAccessPointsToWrite =
                approvedAccessPointsMapToWrite.get(TEST_PACKAGE_NAME_1);
        Set<AccessPoint> expectedApprovedAccessPoints =
                new HashSet<AccessPoint>() {{
                    add(new AccessPoint(TEST_SSID_1, MacAddress.fromString(TEST_BSSID_1),
                            WifiConfiguration.SECURITY_TYPE_PSK));
                }};
        assertEquals(expectedApprovedAccessPoints, approvedAccessPointsToWrite);
        // Ensure that the new data flag has been reset after read.
        assertFalse(mDataSource.hasNewDataToSerialize());
    }

    /**
     * Verify the config store load for store user approval.
     */
    @Test
    public void testNetworkSpecifierUserApprovalConfigStoreLoad()
            throws Exception {
        Map<String, Set<AccessPoint>> approvedAccessPointsMapToRead = new HashMap<>();
        Set<AccessPoint> approvedAccessPoints =
                new HashSet<AccessPoint>() {{
                    add(new AccessPoint(TEST_SSID_1, MacAddress.fromString(TEST_BSSID_1),
                            WifiConfiguration.SECURITY_TYPE_PSK));
                }};
        approvedAccessPointsMapToRead.put(TEST_PACKAGE_NAME_1, approvedAccessPoints);
        mDataSource.fromDeserialized(approvedAccessPointsMapToRead);

        // The new network request should bypass user approval for the same access point.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(TEST_BSSID_1),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, WifiConfigurationTestUtil.createPskNetwork(),
                TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        // Ensure we triggered a connect without issuing any scans.
        verify(mWifiScanner, never()).startScan(any(), any(), any(), any());

        // Verify we did not trigger the match callback.
        verify(mNetworkRequestMatchCallback, never()).onMatch(anyList());
        // Verify that we sent a connection attempt to ClientModeManager
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),  any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());
    }

    /**
     * Verify the config store save and load could preserve the elements order.
     */
    @Test
    public void testStoreConfigSaveAndLoadPreserveOrder() throws Exception {
        LinkedHashSet<AccessPoint> approvedApSet = new LinkedHashSet<>();
        approvedApSet.add(new AccessPoint(TEST_SSID_1,
                MacAddress.fromString(TEST_BSSID_1), WifiConfiguration.SECURITY_TYPE_PSK));
        approvedApSet.add(new AccessPoint(TEST_SSID_2,
                MacAddress.fromString(TEST_BSSID_2), WifiConfiguration.SECURITY_TYPE_PSK));
        approvedApSet.add(new AccessPoint(TEST_SSID_3,
                MacAddress.fromString(TEST_BSSID_3), WifiConfiguration.SECURITY_TYPE_PSK));
        approvedApSet.add(new AccessPoint(TEST_SSID_4,
                MacAddress.fromString(TEST_BSSID_4), WifiConfiguration.SECURITY_TYPE_PSK));
        mWifiNetworkFactory.mUserApprovedAccessPointMap.put(TEST_PACKAGE_NAME_1,
                new LinkedHashSet<>(approvedApSet));
        // Save config.
        byte[] xmlData = serializeData();
        mWifiNetworkFactory.mUserApprovedAccessPointMap.clear();
        // Load config.
        deserializeData(xmlData);

        LinkedHashSet<AccessPoint> storedApSet = mWifiNetworkFactory
                .mUserApprovedAccessPointMap.get(TEST_PACKAGE_NAME_1);
        // Check load config success and order preserved.
        assertNotNull(storedApSet);
        assertArrayEquals(approvedApSet.toArray(), storedApSet.toArray());
    }

    /**
     * Verify the user approval bypass for a specific request for an access point that was already
     * approved previously and the scan result is present in the cached scan results.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchApproved()
            throws Exception {
        // 1. First request (no user approval bypass)
        sendNetworkRequestAndSetupForConnectionStatus();

        mWifiNetworkFactory.removeCallback(mNetworkRequestMatchCallback);
        reset(mNetworkRequestMatchCallback, mWifiScanner, mAlarmManager, mClientModeManager,
                mConnectHelper);

        // 2. Second request for the same access point (user approval bypass).
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];

        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch,
                WifiConfigurationTestUtil.createPskNetwork(), TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Verify we did not trigger the UI for the second request.
        verify(mContext, times(1)).startActivityAsUser(any(), any());
        // Verify we did not trigger a scan.
        verify(mWifiScanner, never()).startScan(any(), any(), any(), any());
        // Verify we did not trigger the match callback.
        verify(mNetworkRequestMatchCallback, never()).onMatch(anyList());
        // Verify that we sent a connection attempt to ClientModeManager
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),  any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());

        verify(mWifiMetrics).incrementNetworkRequestApiNumUserApprovalBypass();
    }

    /**
     * Verify the user approval bypass for a specific request for an access point that was already
     * approved previously via CDM and the scan result is present in the cached scan results.
     */
    @Test
    public void
            testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchApprovedViaCDM()
            throws Exception {
        // Setup scan data for WPA-PSK networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Choose the matching scan result.
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];

        // Setup CDM approval for the scan result.
        when(mCompanionDeviceManager.isDeviceAssociatedForWifiConnection(
                TEST_PACKAGE_NAME_1,
                MacAddress.fromString(matchingScanResult.BSSID),
                UserHandle.getUserHandleForUid(TEST_UID_1))).thenReturn(true);

        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch,
                WifiConfigurationTestUtil.createPskNetwork(), TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Verify we did not trigger the UI for the second request.
        verify(mContext, never()).startActivityAsUser(any(), any());
        // Verify we did not trigger a scan.
        verify(mWifiScanner, never()).startScan(any(), any(), any(), any());
        // Verify we did not trigger the match callback.
        verify(mNetworkRequestMatchCallback, never()).onMatch(anyList());
        // Verify that we sent a connection attempt to ClientModeManager
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),  any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());

        verify(mWifiMetrics).incrementNetworkRequestApiNumUserApprovalBypass();
    }

    /**
     * Verify the user approval bypass for a specific request for an access point that was already
     * approved previously via shell command and the scan result is present in the cached scan
     * results.
     */
    @Test
    public void
            testNetworkSpecifierMatchSuccessUsingLiteralSsidAndBssidMatchApprovedViaShell()
            throws Exception {
        // Setup scan data for WPA-PSK networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Choose the matching scan result.
        ScanResult matchingScanResult = mTestScanDatas[0].getResults()[0];

        // Setup shell approval for the scan result.
        mWifiNetworkFactory.setUserApprovedApp(TEST_PACKAGE_NAME_1, true);

        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(MacAddress.fromString(matchingScanResult.BSSID),
                        MacAddress.BROADCAST_ADDRESS);
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch,
                WifiConfigurationTestUtil.createPskNetwork(), TEST_UID_1, TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        // Verify we did not trigger the UI for the second request.
        verify(mContext, never()).startActivityAsUser(any(), any());
        // Verify we did not trigger a scan.
        verify(mWifiScanner, never()).startScan(any(), any(), any(), any());
        // Verify we did not trigger the match callback.
        verify(mNetworkRequestMatchCallback, never()).onMatch(anyList());
        // Verify that we sent a connection attempt to ClientModeManager
        verify(mConnectHelper).connectToNetwork(eq(mClientModeManager),  any(),
                mConnectListenerArgumentCaptor.capture(), anyInt());

        verify(mWifiMetrics).incrementNetworkRequestApiNumUserApprovalBypass();
    }

    /**
     * Verify network specifier matching for a specifier containing a specific SSID match using
     * 4 WPA_PSK scan results, each with unique SSID when the UI callback registration is delayed.
     */
    @Test
    public void testNetworkSpecifierMatchSuccessUsingLiteralSsidMatchCallbackRegistrationDelayed()
            throws Exception {
        // Setup scan data for open networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for open networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_PSK);
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(mNetworkRequest);

        validateUiStartParams(true);

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        // Ensure we did not send any match callbacks, until the callback is registered
        verify(mNetworkRequestMatchCallback, never()).onMatch(any());

        // Register the callback & ensure we triggered the on match callback.
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        ArgumentCaptor<List<ScanResult>> matchedScanResultsCaptor =
                ArgumentCaptor.forClass(List.class);
        verify(mNetworkRequestMatchCallback).onMatch(matchedScanResultsCaptor.capture());

        assertNotNull(matchedScanResultsCaptor.getValue());
        // We only expect 1 network match in this case.
        validateScanResults(matchedScanResultsCaptor.getValue(), mTestScanDatas[0].getResults()[0]);

        verify(mWifiMetrics).incrementNetworkRequestApiMatchSizeHistogram(
                matchedScanResultsCaptor.getValue().size());
    }

    private void sendNetworkRequestAndSetupForConnectionStatus() throws RemoteException {
        sendNetworkRequestAndSetupForConnectionStatus(TEST_SSID_1);
    }

    // Helper method to setup the necessary pre-requisite steps for tracking connection status.
    private void sendNetworkRequestAndSetupForConnectionStatus(String targetSsid)
            throws RemoteException {
        when(mClock.getElapsedSinceBootMillis()).thenReturn(0L);

        sendNetworkRequestAndSetupForUserSelection(targetSsid);

        INetworkRequestUserSelectionCallback networkRequestUserSelectionCallback =
                mNetworkRequestUserSelectionCallback.getValue();
        assertNotNull(networkRequestUserSelectionCallback);

        // Now trigger user selection to one of the network.
        mSelectedNetwork = WifiConfigurationTestUtil.createPskNetwork();
        mSelectedNetwork.SSID = "\"" + targetSsid + "\"";
        sendUserSelectionSelect(networkRequestUserSelectionCallback, mSelectedNetwork);
        mLooper.dispatchAll();

        verify(mActiveModeWarden, atLeastOnce()).requestLocalOnlyClientModeManager(
                any(), any(), any(), any());

        // Cancel the periodic scan timer.
        mInOrder.verify(mAlarmManager).cancel(mPeriodicScanListenerArgumentCaptor.getValue());
        // Disable connectivity manager
        if (mClientModeManager.getRole() == ActiveModeManager.ROLE_CLIENT_PRIMARY) {
            verify(mWifiConnectivityManager, atLeastOnce())
                    .setSpecificNetworkRequestInProgress(true);
        }
        // Increment the number of unique apps.
        verify(mWifiMetrics).incrementNetworkRequestApiNumApps();

        verify(mClientModeManager, atLeastOnce()).disconnect();
        verify(mConnectHelper, atLeastOnce()).connectToNetwork(
                eq(mClientModeManager),
                eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                mConnectListenerArgumentCaptor.capture(), anyInt());
        if (mClientModeManager.getRole() == ActiveModeManager.ROLE_CLIENT_PRIMARY) {
            verify(mWifiMetrics, atLeastOnce())
                    .incrementNetworkRequestApiNumConnectOnPrimaryIface();
        } else {
            verify(mWifiMetrics, atLeastOnce())
                    .incrementNetworkRequestApiNumConnectOnSecondaryIface();
        }

        // Start the connection timeout alarm.
        mInOrder.verify(mAlarmManager).set(eq(AlarmManager.ELAPSED_REALTIME_WAKEUP),
                eq((long) WifiNetworkFactory.NETWORK_CONNECTION_TIMEOUT_MS), any(),
                mConnectionTimeoutAlarmListenerArgumentCaptor.capture(), any());
        assertNotNull(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
    }

    private void sendNetworkRequestAndSetupForUserSelection() throws RemoteException {
        sendNetworkRequestAndSetupForUserSelection(TEST_SSID_1);
    }

    // Helper method to setup the necessary pre-requisite steps for user selection.
    private void sendNetworkRequestAndSetupForUserSelection(String targetSsid)
            throws RemoteException {
        // Setup scan data for WPA-PSK networks.
        setupScanData(SCAN_RESULT_TYPE_WPA_PSK,
                TEST_SSID_1, TEST_SSID_2, TEST_SSID_3, TEST_SSID_4);

        // Setup network specifier for WPA-PSK networks.
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(targetSsid, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration = WifiConfigurationTestUtil.createPskNetwork();
        wifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        attachWifiNetworkSpecifierAndAppInfo(
                ssidPatternMatch, bssidPatternMatch, wifiConfiguration, TEST_UID_1,
                TEST_PACKAGE_NAME_1);
        mWifiNetworkFactory.needNetworkFor(new NetworkRequest(mNetworkRequest));

        validateUiStartParams(true);

        when(mNetworkRequestMatchCallback.asBinder()).thenReturn(mAppBinder);
        mWifiNetworkFactory.addCallback(mNetworkRequestMatchCallback);
        verify(mNetworkRequestMatchCallback).onUserSelectionCallbackRegistration(
                mNetworkRequestUserSelectionCallback.capture());

        verifyPeriodicScans(0, PERIODIC_SCAN_INTERVAL_MS);

        verify(mNetworkRequestMatchCallback, atLeastOnce()).onMatch(anyList());
    }

    private void verifyPeriodicScans(long...expectedIntervalsInSeconds) {
        PeriodicScanParams[] periodicScanParams =
                new PeriodicScanParams[expectedIntervalsInSeconds.length];
        for (int i = 0; i < expectedIntervalsInSeconds.length; i++) {
            periodicScanParams[i] =
                    new PeriodicScanParams(expectedIntervalsInSeconds[i], mTestScanDatas);
        }
        verifyPeriodicScans(0L, periodicScanParams);
    }

    private static class PeriodicScanParams {
        public final long expectedIntervalInSeconds;
        public final WifiScanner.ScanData[] scanDatas;

        PeriodicScanParams(long expectedIntervalInSeconds, WifiScanner.ScanData[] scanDatas) {
            this.expectedIntervalInSeconds = expectedIntervalInSeconds;
            this.scanDatas = scanDatas;
        }
    }

    // Simulates the periodic scans performed to find a matching network.
    // a) Start scan
    // b) Scan results received.
    // c) Set alarm for next scan at the expected interval.
    // d) Alarm fires, go to step a) again and repeat.
    private void verifyPeriodicScans(long nowMs, PeriodicScanParams... scanParams) {
        when(mClock.getElapsedSinceBootMillis()).thenReturn(nowMs);

        OnAlarmListener alarmListener = null;
        ScanListener scanListener = null;

        mInOrder = inOrder(mWifiScanner, mAlarmManager);

        // Before we start scans, ensure that we look at the latest cached scan results.
        mInOrder.verify(mWifiScanner).getSingleScanResults();

        for (int i = 0; i < scanParams.length - 1; i++) {
            long expectedCurrentIntervalInMs = scanParams[i].expectedIntervalInSeconds;
            long expectedNextIntervalInMs = scanParams[i + 1].expectedIntervalInSeconds;

            // First scan is immediately fired, so need for the alarm to fire.
            if (expectedCurrentIntervalInMs != 0) {
                // Fire the alarm and ensure that we started the next scan.
                alarmListener.onAlarm();
            }
            mInOrder.verify(mWifiScanner).startScan(
                    any(), any(), mScanListenerArgumentCaptor.capture(), any());
            scanListener = mScanListenerArgumentCaptor.getValue();
            assertNotNull(scanListener);

            // Now trigger the scan results callback and verify the alarm set for the next scan.
            scanListener.onResults(scanParams[i].scanDatas);

            mInOrder.verify(mAlarmManager).set(eq(AlarmManager.ELAPSED_REALTIME_WAKEUP),
                    eq(expectedNextIntervalInMs + nowMs), any(),
                    mPeriodicScanListenerArgumentCaptor.capture(), any());
            alarmListener = mPeriodicScanListenerArgumentCaptor.getValue();
            assertNotNull(alarmListener);
        }

        mInOrder.verifyNoMoreInteractions();
    }

    private void attachDefaultWifiNetworkSpecifierAndAppInfo(int uid, boolean isHidden) {
        PatternMatcher ssidPatternMatch =
                new PatternMatcher(TEST_SSID_1, PatternMatcher.PATTERN_LITERAL);
        Pair<MacAddress, MacAddress> bssidPatternMatch =
                Pair.create(WifiManager.ALL_ZEROS_MAC_ADDRESS, WifiManager.ALL_ZEROS_MAC_ADDRESS);
        WifiConfiguration wifiConfiguration;
        if (isHidden) {
            wifiConfiguration = WifiConfigurationTestUtil.createPskHiddenNetwork();
        } else {
            wifiConfiguration = WifiConfigurationTestUtil.createPskNetwork();
        }
        String packageName = null;
        if (uid == TEST_UID_1) {
            packageName = TEST_PACKAGE_NAME_1;
        } else if (uid == TEST_UID_2) {
            packageName = TEST_PACKAGE_NAME_2;
        } else {
            fail();
        }
        attachWifiNetworkSpecifierAndAppInfo(ssidPatternMatch, bssidPatternMatch, wifiConfiguration,
                uid, packageName);
    }

    private void attachWifiNetworkSpecifierAndAppInfo(
            PatternMatcher ssidPatternMatch, Pair<MacAddress, MacAddress> bssidPatternMatch,
            WifiConfiguration wifiConfiguration, int uid, String packageName) {
        mNetworkCapabilities.setRequestorUid(uid);
        mNetworkCapabilities.setRequestorPackageName(packageName);
        mNetworkCapabilities.setNetworkSpecifier(
                new WifiNetworkSpecifier(ssidPatternMatch, bssidPatternMatch,
                        ScanResult.UNSPECIFIED, wifiConfiguration));
        mNetworkRequest = new NetworkRequest.Builder()
                .setCapabilities(mNetworkCapabilities)
                .build();
    }

    private static final int SCAN_RESULT_TYPE_OPEN = 0;
    private static final int SCAN_RESULT_TYPE_WPA_PSK = 1;
    private static final int SCAN_RESULT_TYPE_WPA_EAP = 2;
    private static final int SCAN_RESULT_TYPE_WPA_PSK_SAE_TRANSITION = 3;

    private String getScanResultCapsForType(int scanResultType) {
        switch (scanResultType) {
            case SCAN_RESULT_TYPE_OPEN:
                return WifiConfigurationTestUtil.getScanResultCapsForNetwork(
                        WifiConfigurationTestUtil.createOpenNetwork());
            case SCAN_RESULT_TYPE_WPA_PSK:
                return WifiConfigurationTestUtil.getScanResultCapsForNetwork(
                        WifiConfigurationTestUtil.createPskNetwork());
            case SCAN_RESULT_TYPE_WPA_EAP:
                return WifiConfigurationTestUtil.getScanResultCapsForNetwork(
                        WifiConfigurationTestUtil.createEapNetwork());
            case SCAN_RESULT_TYPE_WPA_PSK_SAE_TRANSITION:
                return WifiConfigurationTestUtil.getScanResultCapsForWpa2Wpa3TransitionNetwork();
        }
        fail("Invalid scan result type " + scanResultType);
        return "";
    }

    private void setupScanData(int scanResultType, String ssid1, String ssid2, String ssid3,
            String ssid4) {
        setupScanData(mTestScanDatas, scanResultType, ssid1, ssid2, ssid3, ssid4);
    }

    // Helper method to setup the scan data for verifying the matching algo.
    private void setupScanData(WifiScanner.ScanData[] testScanDatas, int scanResultType,
            String ssid1, String ssid2, String ssid3, String ssid4) {
        // 4 scan results,
        assertEquals(1, testScanDatas.length);
        ScanResult[] scanResults = testScanDatas[0].getResults();
        assertEquals(4, scanResults.length);

        String caps = getScanResultCapsForType(scanResultType);

        // Scan results have increasing RSSI.
        scanResults[0].SSID = ssid1;
        scanResults[0].wifiSsid = WifiSsid.createFromAsciiEncoded(ssid1);
        scanResults[0].BSSID = TEST_BSSID_1;
        scanResults[0].capabilities = caps;
        scanResults[0].level = -45;
        scanResults[1].SSID = ssid2;
        scanResults[1].wifiSsid = WifiSsid.createFromAsciiEncoded(ssid2);
        scanResults[1].BSSID = TEST_BSSID_2;
        scanResults[1].capabilities = caps;
        scanResults[1].level = -35;
        scanResults[2].SSID = ssid3;
        scanResults[2].wifiSsid = WifiSsid.createFromAsciiEncoded(ssid3);
        scanResults[2].BSSID = TEST_BSSID_3;
        scanResults[2].capabilities = caps;
        scanResults[2].level = -25;
        scanResults[3].SSID = ssid4;
        scanResults[3].wifiSsid = WifiSsid.createFromAsciiEncoded(ssid4);
        scanResults[3].BSSID = TEST_BSSID_4;
        scanResults[3].capabilities = caps;
        scanResults[3].level = -15;
    }

    private void validateScanResults(
            List<ScanResult> actualScanResults, ScanResult...expectedScanResults) {
        assertEquals(expectedScanResults.length, actualScanResults.size());
        for (int i = 0; i < expectedScanResults.length; i++) {
            ScanResult expectedScanResult = expectedScanResults[i];
            ScanResult actualScanResult = actualScanResults.stream()
                    .filter(x -> x.BSSID.equals(expectedScanResult.BSSID))
                    .findFirst()
                    .orElse(null);
            ScanTestUtil.assertScanResultEquals(expectedScanResult, actualScanResult);
        }
    }

    private void validateConnectionRetryAttempts(boolean onTimeout) {
        for (int i = 0; i < WifiNetworkFactory.USER_SELECTED_NETWORK_CONNECT_RETRY_MAX; i++) {
            if (!onTimeout) {
                // Cancel the existing connection timeout.
                mInOrder.verify(mAlarmManager).cancel(
                        mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
            }

            // Trigger new connection.
            mInOrder.verify(mConnectHelper).connectToNetwork(
                    eq(mClientModeManager),
                    eq(new NetworkUpdateResult(TEST_NETWORK_ID_1)),
                    mConnectListenerArgumentCaptor.capture(),
                    anyInt());

            // Start the new connection timeout alarm.
            mInOrder.verify(mAlarmManager).set(eq(AlarmManager.ELAPSED_REALTIME_WAKEUP),
                    eq((long) WifiNetworkFactory.NETWORK_CONNECTION_TIMEOUT_MS), any(),
                    mConnectionTimeoutAlarmListenerArgumentCaptor.capture(), any());
            assertNotNull(mConnectionTimeoutAlarmListenerArgumentCaptor.getValue());
        }
    }

    private void validateScanSettings(@Nullable String hiddenSsid) {
        ScanSettings scanSettings = mScanSettingsArgumentCaptor.getValue();
        assertNotNull(scanSettings);
        assertEquals(WifiScanner.WIFI_BAND_ALL, scanSettings.band);
        assertEquals(WifiScanner.SCAN_TYPE_HIGH_ACCURACY, scanSettings.type);
        assertEquals(WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN, scanSettings.reportEvents);
        if (hiddenSsid == null) {
            assertEquals(Collections.emptyList(), scanSettings.hiddenNetworks);
        } else {
            assertNotNull(scanSettings.hiddenNetworks.get(0));
            assertEquals(scanSettings.hiddenNetworks.get(0).ssid, addEnclosingQuotes(hiddenSsid));
        }
        WorkSource workSource = mWorkSourceArgumentCaptor.getValue();
        assertNotNull(workSource);
        assertEquals(TEST_UID_1, workSource.getUid(0));
    }

    class WifiConfigMatcher implements ArgumentMatcher<WifiConfiguration> {
        private final WifiConfiguration mConfig;

        WifiConfigMatcher(WifiConfiguration config) {
            assertNotNull(config);
            mConfig = config;
            mConfig.shared = false;
        }

        @Override
        public boolean matches(WifiConfiguration otherConfig) {
            if (otherConfig == null) return false;
            return mConfig.getProfileKey().equals(otherConfig.getProfileKey());
        }
    }

    private void validateUiStartParams(boolean expectedIsReqForSingeNetwork) {
        ArgumentCaptor<Intent> intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, atLeastOnce()).startActivityAsUser(
                intentArgumentCaptor.capture(), eq(UserHandle.getUserHandleForUid(TEST_UID_1)));
        Intent intent = intentArgumentCaptor.getValue();
        assertNotNull(intent);
        assertEquals(intent.getAction(), WifiNetworkFactory.UI_START_INTENT_ACTION);
        assertTrue(intent.getCategories().contains(WifiNetworkFactory.UI_START_INTENT_CATEGORY));
        assertEquals(intent.getStringExtra(WifiNetworkFactory.UI_START_INTENT_EXTRA_APP_NAME),
                TEST_APP_NAME);
        assertEquals(expectedIsReqForSingeNetwork, intent.getBooleanExtra(
                WifiNetworkFactory.UI_START_INTENT_EXTRA_REQUEST_IS_FOR_SINGLE_NETWORK, false));
        assertTrue((intent.getFlags() & Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT) != 0);
        assertTrue((intent.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK) != 0);
    }

    private void validateConnectParams(String ssid, String bssid) {
        ArgumentCaptor<WifiConfiguration> wifiConfigurationCaptor =
                ArgumentCaptor.forClass(WifiConfiguration.class);
        verify(mWifiConfigManager).addOrUpdateNetwork(
                wifiConfigurationCaptor.capture(), eq(TEST_UID_1), eq(TEST_PACKAGE_NAME_1));
        WifiConfiguration network =  wifiConfigurationCaptor.getValue();
        assertNotNull(network);
        WifiConfiguration expectedWifiConfiguration =
                new WifiConfiguration(((WifiNetworkSpecifier) mNetworkRequest.networkCapabilities
                        .getNetworkSpecifier()).wifiConfiguration);
        expectedWifiConfiguration.SSID = ssid;
        expectedWifiConfiguration.preSharedKey = TEST_WPA_PRESHARED_KEY;
        expectedWifiConfiguration.BSSID = bssid;
        expectedWifiConfiguration.ephemeral = true;
        expectedWifiConfiguration.shared = false;
        expectedWifiConfiguration.fromWifiNetworkSpecifier = true;
        WifiConfigurationTestUtil.assertConfigurationEqual(expectedWifiConfiguration, network);
    }

    /**
     * Create a test scan data for target SSID list with specified number and encryption type
     * @param scanResultType   network encryption type
     * @param nums             Number of results with different BSSIDs for one SSID
     * @param ssids            target SSID list
     */
    private void setupScanDataSameSsidWithDiffBssid(int scanResultType, int nums, String[] ssids) {
        String baseBssid = "11:34:56:78:90:";
        int[] freq = new int[nums * ssids.length];
        for (int i = 0; i < nums; i++) {
            freq[i] = 2417 + i;
        }
        mTestScanDatas = ScanTestUtil.createScanDatas(new int[][]{ freq });
        assertEquals(1, mTestScanDatas.length);
        ScanResult[] scanResults = mTestScanDatas[0].getResults();
        assertEquals(nums * ssids.length, scanResults.length);
        String caps = getScanResultCapsForType(scanResultType);
        for (int i = 0; i < ssids.length; i++) {
            for (int j = i * nums; j < (i + 1) * nums; j++) {
                scanResults[j].SSID = ssids[i];
                scanResults[j].BSSID = baseBssid + Integer.toHexString(16 + j);
                scanResults[j].capabilities = caps;
                scanResults[j].level = -45;
            }
        }
    }

    /**
     * Helper function for serializing configuration data to a XML block.
     *
     * @return byte[] of the XML data
     * @throws Exception
     */
    private byte[] serializeData() throws Exception {
        final XmlSerializer out = new FastXmlSerializer();
        final ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        out.setOutput(outputStream, StandardCharsets.UTF_8.name());
        mNetworkRequestStoreData.serializeData(out, mock(WifiConfigStoreEncryptionUtil.class));
        out.flush();
        return outputStream.toByteArray();
    }

    /**
     * Helper function for parsing configuration data from a XML block.
     *
     * @param data XML data to parse from
     * @throws Exception
     */
    private void deserializeData(byte[] data) throws Exception {
        final XmlPullParser in = Xml.newPullParser();
        final ByteArrayInputStream inputStream = new ByteArrayInputStream(data);
        in.setInput(inputStream, StandardCharsets.UTF_8.name());
        mNetworkRequestStoreData.deserializeData(in, in.getDepth(),
                WifiConfigStore.ENCRYPT_CREDENTIALS_CONFIG_STORE_DATA_VERSION,
                mock(WifiConfigStoreEncryptionUtil.class));
    }

    private void sendUserSelectionSelect(INetworkRequestUserSelectionCallback callback,
            WifiConfiguration selectedNetwork) throws RemoteException {
        WifiConfiguration selectedNetworkinCb = new WifiConfiguration();
        // only copy over the ssid
        selectedNetworkinCb.SSID = selectedNetwork.SSID;
        callback.select(selectedNetworkinCb);
    }

    private void setScreenState(boolean screenOn) {
        BroadcastReceiver broadcastReceiver = mBroadcastReceiverCaptor.getValue();
        assertNotNull(broadcastReceiver);
        Intent intent = new Intent(screenOn  ? ACTION_SCREEN_ON : ACTION_SCREEN_OFF);
        broadcastReceiver.onReceive(mContext, intent);
    }
}
