/* Copyright (C) 2016 The Android Open Source Project
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


import static android.net.wifi.WifiManager.EXTRA_PREVIOUS_WIFI_AP_STATE;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_FAILURE_REASON;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_INTERFACE_NAME;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_MODE;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_STATE;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_LOCAL_ONLY;
import static android.net.wifi.WifiManager.SAP_CLIENT_DISCONNECT_REASON_CODE_UNSPECIFIED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_FAILED;

import static com.android.server.wifi.ActiveModeManager.ROLE_SOFTAP_LOCAL_ONLY;
import static com.android.server.wifi.ActiveModeManager.ROLE_SOFTAP_TETHERED;
import static com.android.server.wifi.LocalOnlyHotspotRequestInfo.HOTSPOT_NO_ERROR;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertEquals;
import static org.junit.Assume.assumeTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyBoolean;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyLong;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.app.test.TestAlarmManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.net.MacAddress;
import android.net.wifi.CoexUnsafeChannel;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.SoftApConfiguration.Builder;
import android.net.wifi.SoftApInfo;
import android.net.wifi.WifiClient;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.net.wifi.nl80211.NativeWifiClient;
import android.os.UserHandle;
import android.os.WorkSource;
import android.os.test.TestLooper;
import android.provider.Settings;
import android.util.SparseIntArray;

import androidx.test.filters.SmallTest;

import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.coex.CoexManager;
import com.android.wifi.resources.R;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/** Unit tests for {@link SoftApManager}. */
@SmallTest
public class SoftApManagerTest extends WifiBaseTest {

    private static final String TAG = "SoftApManagerTest";

    private static final int TEST_MANAGER_ID = 1000;
    private static final String DEFAULT_SSID = "DefaultTestSSID";
    private static final String TEST_SSID = "TestSSID";
    private static final String TEST_PASSWORD = "TestPassword";
    private static final String TEST_COUNTRY_CODE = "TestCountry";
    private static final String TEST_INTERFACE_NAME = "testif0";
    private static final String TEST_SECOND_INTERFACE_NAME = "testif1";
    private static final String OTHER_INTERFACE_NAME = "otherif";
    private static final long TEST_DEFAULT_SHUTDOWN_TIMEOUT_MILLIS = 600_000;
    private static final long TEST_DEFAULT_SHUTDOWN_IDLE_INSTANCE_IN_BRIDGED_MODE_TIMEOUT_MILLIS =
            300_000;
    private static final MacAddress TEST_INTERFACE_MAC_ADDRESS =
            MacAddress.fromString("22:12:11:11:11:11");
    private static final MacAddress TEST_SECOND_INTERFACE_MAC_ADDRESS =
            MacAddress.fromString("22:22:22:22:22:22");
    private static final MacAddress TEST_CLIENT_MAC_ADDRESS =
            MacAddress.fromString("22:33:44:55:66:77");
    private static final MacAddress TEST_CLIENT_MAC_ADDRESS_2 =
            MacAddress.fromString("aa:bb:cc:dd:ee:ff");
    private static final MacAddress TEST_CLIENT_MAC_ADDRESS_ON_SECOND_IFACE =
            MacAddress.fromString("aa:bb:cc:11:22:33");
    private static final WifiClient TEST_CONNECTED_CLIENT = new WifiClient(TEST_CLIENT_MAC_ADDRESS,
            TEST_INTERFACE_NAME);
    private static final NativeWifiClient TEST_NATIVE_CLIENT = new NativeWifiClient(
            TEST_CLIENT_MAC_ADDRESS);
    private static final WifiClient TEST_CONNECTED_CLIENT_2 =
            new WifiClient(TEST_CLIENT_MAC_ADDRESS_2, TEST_INTERFACE_NAME);
    private static final NativeWifiClient TEST_NATIVE_CLIENT_2 = new NativeWifiClient(
            TEST_CLIENT_MAC_ADDRESS_2);
    private static final WifiClient TEST_CONNECTED_CLIENT_ON_SECOND_IFACE =
            new WifiClient(TEST_CLIENT_MAC_ADDRESS_ON_SECOND_IFACE, TEST_SECOND_INTERFACE_NAME);
    private static final int TEST_AP_FREQUENCY = 2412;
    private static final int TEST_AP_FREQUENCY_5G = 5220;
    private static final int TEST_AP_BANDWIDTH_FROM_IFACE_CALLBACK =
            SoftApInfo.CHANNEL_WIDTH_20MHZ_NOHT;
    private static final int TEST_AP_BANDWIDTH_IN_SOFTAPINFO = SoftApInfo.CHANNEL_WIDTH_20MHZ_NOHT;
    private static final int[] EMPTY_CHANNEL_ARRAY = {};
    private static final int[] ALLOWED_2G_FREQS = {2462}; //ch# 11
    private static final int[] ALLOWED_5G_FREQS = {5745, 5765}; //ch# 149, 153
    private static final int[] ALLOWED_6G_FREQS = {5945, 5965};
    private static final int[] ALLOWED_60G_FREQS = {58320, 60480}; // ch# 1, 2
    private static final WorkSource TEST_WORKSOURCE = new WorkSource();
    private SoftApConfiguration mDefaultApConfig = createDefaultApConfig();
    private final int mBand256G = SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ
            | SoftApConfiguration.BAND_6GHZ;
    private static final int[] TEST_SUPPORTED_24G_CHANNELS = new int[] {1, 2};
    private static final int[] TEST_SUPPORTED_5G_CHANNELS = new int[] {36, 149};

    private TestLooper mLooper;
    private TestAlarmManager mAlarmManager;
    private SoftApInfo mTestSoftApInfo;
    private SoftApInfo mTestSoftApInfoOnSecondInterface;
    private Map<String, SoftApInfo> mTestSoftApInfoMap = new HashMap<>();
    private Map<String, List<WifiClient>> mTestWifiClientsMap = new HashMap<>();
    private List<WifiClient> mCurrentConnectedTestedClientListOnTestInterface = new ArrayList();
    private List<WifiClient> mCurrentConnectedTestedClientListOnSecondInterface = new ArrayList();
    private SoftApCapability mTestSoftApCapability;
    private List<ClientModeManager> mTestClientModeManagers = new ArrayList<>();

    @Mock WifiContext mContext;
    @Mock Resources mResources;
    @Mock WifiNative mWifiNative;
    @Mock CoexManager mCoexManager;
    @Mock WifiServiceImpl.SoftApCallbackInternal mCallback;
    @Mock ActiveModeManager.Listener<SoftApManager> mListener;
    @Mock FrameworkFacade mFrameworkFacade;
    @Mock WifiApConfigStore mWifiApConfigStore;
    @Mock SarManager mSarManager;
    @Mock WifiMetrics mWifiMetrics;
    @Mock WifiDiagnostics mWifiDiagnostics;
    @Mock WifiNotificationManager mWifiNotificationManager;
    @Mock SoftApNotifier mFakeSoftApNotifier;
    @Mock ClientModeImplMonitor mCmiMonitor;
    @Mock ActiveModeWarden mActiveModeWarden;
    @Mock ClientModeManager mPrimaryConcreteClientModeManager;
    @Mock ClientModeManager mSecondConcreteClientModeManager;
    @Mock ConcreteClientModeManager mConcreteClientModeManager;
    @Mock WifiInfo mPrimaryWifiInfo;
    @Mock WifiInfo mSecondWifiInfo;

    final ArgumentCaptor<WifiNative.InterfaceCallback> mWifiNativeInterfaceCallbackCaptor =
            ArgumentCaptor.forClass(WifiNative.InterfaceCallback.class);

    final ArgumentCaptor<WifiNative.SoftApListener> mSoftApListenerCaptor =
            ArgumentCaptor.forClass(WifiNative.SoftApListener.class);

    // CoexListener will only be captured if SdkLevel is at least S
    private final ArgumentCaptor<CoexManager.CoexListener> mCoexListenerCaptor =
            ArgumentCaptor.forClass(CoexManager.CoexListener.class);

    private final ArgumentCaptor<ClientModeImplListener> mCmiListenerCaptor =
            ArgumentCaptor.forClass(ClientModeImplListener.class);

    SoftApManager mSoftApManager;

    /** Old callback event from wificond */
    private void mockChannelSwitchEvent(int frequency, int bandwidth) {
        mSoftApListenerCaptor.getValue().onInfoChanged(
                TEST_INTERFACE_NAME, frequency, bandwidth, 0, null);
    }

    /** New callback event from hostapd */
    private void mockApInfoChangedEvent(SoftApInfo apInfo) {
        mSoftApListenerCaptor.getValue().onInfoChanged(
                apInfo.getApInstanceIdentifier(), apInfo.getFrequency(), apInfo.getBandwidth(),
                apInfo.getWifiStandardInternal(), apInfo.getBssidInternal());
        mTestSoftApInfoMap.put(apInfo.getApInstanceIdentifier(), apInfo);
        mTestWifiClientsMap.put(apInfo.getApInstanceIdentifier(), new ArrayList<WifiClient>());
    }

    private void mockClientConnectedEvent(MacAddress mac, boolean isConnected,
            String apIfaceInstance, boolean updateTheTestMap) {
        mSoftApListenerCaptor.getValue().onConnectedClientsChanged(
                apIfaceInstance, mac, isConnected);
        if (mac == null || !updateTheTestMap) return;
        WifiClient client = new WifiClient(mac, apIfaceInstance);
        List<WifiClient> targetList;
        if (apIfaceInstance.equals(TEST_INTERFACE_NAME)) {
            targetList = mCurrentConnectedTestedClientListOnTestInterface;
        } else {
            targetList = mCurrentConnectedTestedClientListOnSecondInterface;
        }
        if (isConnected) {
            targetList.add(client);
        } else {
            targetList.remove(client);
        }
        mTestWifiClientsMap.put(apIfaceInstance, targetList);
    }

    /** Sets up test. */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mLooper = new TestLooper();

        when(mWifiNative.isApSetMacAddressSupported(any())).thenReturn(true);
        when(mWifiNative.setApMacAddress(any(), any())).thenReturn(true);
        when(mWifiNative.startSoftAp(eq(TEST_INTERFACE_NAME), any(), anyBoolean(),
                any(WifiNative.SoftApListener.class))).thenReturn(true);
        when(mWifiNative.setupInterfaceForSoftApMode(any(), any(), anyInt(), anyBoolean()))
                .thenReturn(TEST_INTERFACE_NAME);
        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(1);
        mAlarmManager = new TestAlarmManager();
        when(mContext.getSystemService(Context.ALARM_SERVICE))
                .thenReturn(mAlarmManager.getAlarmManager());
        when(mContext.getResources()).thenReturn(mResources);
        when(mContext.getWifiOverlayApkPkgName()).thenReturn("test.com.android.wifi.resources");

        when(mResources.getInteger(R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds))
                .thenReturn((int) TEST_DEFAULT_SHUTDOWN_TIMEOUT_MILLIS);
        when(mResources.getInteger(R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond))
                .thenReturn(
                (int) TEST_DEFAULT_SHUTDOWN_IDLE_INSTANCE_IN_BRIDGED_MODE_TIMEOUT_MILLIS);
        when(mWifiNative.setApCountryCode(
                TEST_INTERFACE_NAME, TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT)))
                .thenReturn(true);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_6_GHZ))
                .thenReturn(ALLOWED_6G_FREQS);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_60_GHZ))
                .thenReturn(ALLOWED_60G_FREQS);
        when(mWifiNative.getApFactoryMacAddress(any())).thenReturn(TEST_INTERFACE_MAC_ADDRESS);
        when(mWifiApConfigStore.randomizeBssidIfUnset(any(), any())).thenAnswer(
                (invocation) -> invocation.getArgument(1));
        when(mActiveModeWarden.getClientModeManagers())
                .thenReturn(mTestClientModeManagers);
        mTestClientModeManagers.add(mPrimaryConcreteClientModeManager);
        when(mPrimaryConcreteClientModeManager.syncRequestConnectionInfo())
                .thenReturn(mPrimaryWifiInfo);
        when(mConcreteClientModeManager.syncRequestConnectionInfo())
                .thenReturn(mPrimaryWifiInfo);
        when(mWifiNative.forceClientDisconnect(any(), any(), anyInt())).thenReturn(true);
        mTestSoftApInfo = new SoftApInfo();
        mTestSoftApInfo.setFrequency(TEST_AP_FREQUENCY);
        mTestSoftApInfo.setBandwidth(TEST_AP_BANDWIDTH_IN_SOFTAPINFO);
        mTestSoftApInfo.setBssid(TEST_INTERFACE_MAC_ADDRESS);
        mTestSoftApInfo.setApInstanceIdentifier(TEST_INTERFACE_NAME);
        mTestSoftApInfo.setAutoShutdownTimeoutMillis(TEST_DEFAULT_SHUTDOWN_TIMEOUT_MILLIS);
        mTestSoftApInfoOnSecondInterface = new SoftApInfo();
        mTestSoftApInfoOnSecondInterface.setFrequency(TEST_AP_FREQUENCY_5G);
        mTestSoftApInfoOnSecondInterface.setBandwidth(TEST_AP_BANDWIDTH_IN_SOFTAPINFO);
        mTestSoftApInfoOnSecondInterface.setBssid(TEST_SECOND_INTERFACE_MAC_ADDRESS);
        mTestSoftApInfoOnSecondInterface.setApInstanceIdentifier(TEST_SECOND_INTERFACE_NAME);
        mTestSoftApInfoOnSecondInterface.setAutoShutdownTimeoutMillis(
                TEST_DEFAULT_SHUTDOWN_TIMEOUT_MILLIS);
        // Default set up all features support.
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT
                | SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD
                | SoftApCapability.SOFTAP_FEATURE_WPA3_SAE
                | SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION;
        mTestSoftApCapability = new SoftApCapability(testSoftApFeature);
        mTestSoftApCapability.setMaxSupportedClients(10);
        mTestSoftApCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_2GHZ, TEST_SUPPORTED_24G_CHANNELS);
        mTestSoftApCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_5GHZ, TEST_SUPPORTED_5G_CHANNELS);
        when(mWifiApConfigStore.getApConfiguration()).thenReturn(mDefaultApConfig);
        when(mWifiNative.isHalStarted()).thenReturn(true);

        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();
        mCurrentConnectedTestedClientListOnTestInterface.clear();
        mCurrentConnectedTestedClientListOnSecondInterface.clear();
    }

    private SoftApConfiguration createDefaultApConfig() {
        Builder defaultConfigBuilder = new SoftApConfiguration.Builder();
        defaultConfigBuilder.setSsid(DEFAULT_SSID);
        return defaultConfigBuilder.build();
    }

    private SoftApManager createSoftApManager(SoftApModeConfiguration config, String countryCode,
            ActiveModeManager.SoftApRole role) {
        SoftApManager newSoftApManager = new SoftApManager(
                mContext,
                mLooper.getLooper(),
                mFrameworkFacade,
                mWifiNative,
                mCoexManager,
                countryCode,
                mListener,
                mCallback,
                mWifiApConfigStore,
                config,
                mWifiMetrics,
                mSarManager,
                mWifiDiagnostics,
                mFakeSoftApNotifier,
                mCmiMonitor,
                mActiveModeWarden,
                TEST_MANAGER_ID,
                TEST_WORKSOURCE,
                role,
                false);
        mLooper.dispatchAll();

        return newSoftApManager;
    }

    /** Verifies startSoftAp will use default config if AP configuration is not provided. */
    @Test
    public void startSoftApWithoutConfig() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /** Verifies startSoftAp will use provided config and start AP. */
    @Test
    public void startSoftApWithConfig() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /**
     * Verifies startSoftAp will start with the hiddenSSID param set when it is set to true in the
     * supplied config.
     */
    @Test
    public void startSoftApWithHiddenSsidTrueInConfig() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setHiddenSsid(true);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /**
     * Verifies startSoftAp will start with the password param set in the
     * supplied config.
     */
    @Test
    public void startSoftApWithPassphraseInConfig() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setPassphrase(TEST_PASSWORD,
                SoftApConfiguration.SECURITY_TYPE_WPA2_PSK);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /** Tests softap startup if default config fails to load. **/
    @Test
    public void startSoftApDefaultConfigFailedToLoad() throws Exception {
        when(mWifiApConfigStore.getApConfiguration()).thenReturn(null);
        SoftApModeConfiguration nullApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        mSoftApManager = createSoftApManager(nullApConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        verify(mListener).onStartFailure(mSoftApManager);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_DISABLED, WifiManager.SAP_START_FAILURE_GENERAL, null,
                nullApConfig.getTargetMode());
    }

    /**
     * Test that failure to retrieve the SoftApInterface name increments the corresponding metrics
     * and proper state updates are sent out.
     */
    @Test
    public void testSetupForSoftApModeNullApInterfaceNameFailureIncrementsMetrics()
            throws Exception {
        when(mWifiNative.setupInterfaceForSoftApMode(
                    any(), any(), anyInt(), anyBoolean())).thenReturn(null);
        when(mWifiApConfigStore.getApConfiguration()).thenReturn(null);
        SoftApModeConfiguration nullApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        mSoftApManager = createSoftApManager(nullApConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        verify(mListener).onStartFailure(mSoftApManager);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_DISABLED, WifiManager.SAP_START_FAILURE_GENERAL, null,
                nullApConfig.getTargetMode());

        verify(mWifiMetrics).incrementSoftApStartResult(false,
                WifiManager.SAP_START_FAILURE_GENERAL);
    }

    /**
     * Test that an empty SoftApInterface name is detected as a failure and increments the
     * corresponding metrics and proper state updates are sent out.
     */
    @Test
    public void testSetupForSoftApModeEmptyInterfaceNameFailureIncrementsMetrics()
            throws Exception {
        when(mWifiNative.setupInterfaceForSoftApMode(
                    any(), any(), anyInt(), anyBoolean())).thenReturn("");
        SoftApModeConfiguration nullApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        mSoftApManager = createSoftApManager(nullApConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        verify(mListener).onStartFailure(mSoftApManager);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_DISABLED, WifiManager.SAP_START_FAILURE_GENERAL, "",
                nullApConfig.getTargetMode());

        verify(mWifiMetrics).incrementSoftApStartResult(false,
                WifiManager.SAP_START_FAILURE_GENERAL);
    }

    /**
     * Tests that the generic error is propagated and properly reported when starting softap and no
     * country code is provided.
     */
    @Test
    public void startSoftApOn5GhzFailGeneralErrorForNoCountryCode() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);

        mSoftApManager = createSoftApManager(softApConfig, null, ROLE_SOFTAP_TETHERED);

        verify(mWifiNative, never()).setApCountryCode(eq(TEST_INTERFACE_NAME), any());

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLING, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
    }

    /**
     * Tests that the generic error is propagated and properly reported when starting softap and the
     * country code cannot be set.
     */
    @Test
    public void startSoftApOn5GhzFailGeneralErrorForCountryCodeSetFailure() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);

        when(mWifiNative.setApCountryCode(
                TEST_INTERFACE_NAME, TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT)))
                .thenReturn(false);


        mSoftApManager = createSoftApManager(softApConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);

        verify(mWifiNative).setApCountryCode(
                TEST_INTERFACE_NAME, TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT));

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLING, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
    }

    /**
     * Tests that there is no failure in starting softap in 2Ghz band when no country code is
     * provided.
     */
    @Test
    public void startSoftApOn24GhzNoFailForNoCountryCode() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);

        startSoftApAndVerifyEnabled(softApConfig, null);
        verify(mWifiNative, never()).setApCountryCode(eq(TEST_INTERFACE_NAME), any());
    }

    /**
     * Tests that there is no failure in starting softap in ANY band when no country code is
     * provided.
     */
    @Test
    public void startSoftApOnAnyGhzNoFailForNoCountryCode() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(mBand256G);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);

        startSoftApAndVerifyEnabled(softApConfig, null);
        verify(mWifiNative, never()).setApCountryCode(eq(TEST_INTERFACE_NAME), any());
    }

    /**
     * Tests that there is no failure in starting softap in 2Ghz band when country code cannot be
     * set.
     */
    @Test
    public void startSoftApOn2GhzNoFailForCountryCodeSetFailure() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);

        when(mWifiNative.setApCountryCode(eq(TEST_INTERFACE_NAME), any())).thenReturn(false);

        startSoftApAndVerifyEnabled(softApConfig, TEST_COUNTRY_CODE);
        verify(mWifiNative).setApCountryCode(
                TEST_INTERFACE_NAME, TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT));
    }

    /**
     * Tests that there is no failure in starting softap in ANY band when country code cannot be
     * set.
     */
    @Test
    public void startSoftApOnAnyNoFailForCountryCodeSetFailure() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(mBand256G);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);

        when(mWifiNative.setApCountryCode(eq(TEST_INTERFACE_NAME), any())).thenReturn(false);

        startSoftApAndVerifyEnabled(softApConfig, TEST_COUNTRY_CODE);
        verify(mWifiNative).setApCountryCode(
                TEST_INTERFACE_NAME, TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT));
    }

    /**
     * Tests that the NO_CHANNEL error is propagated and properly reported when starting softap and
     * a valid channel cannot be determined from WifiNative.
     */
    @Test
    public void startSoftApFailNoChannel() throws Exception {
        SoftApCapability noAcsCapability = new SoftApCapability(0);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration softApConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                noAcsCapability);

        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(EMPTY_CHANNEL_ARRAY);

        mSoftApManager = createSoftApManager(softApConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLING, WifiManager.SAP_START_FAILURE_NO_CHANNEL,
                TEST_INTERFACE_NAME, softApConfig.getTargetMode());
    }

    /**
     * Tests startup when Ap Interface fails to start successfully.
     */
    @Test
    public void startSoftApApInterfaceFailedToStart() throws Exception {
        when(mWifiNative.startSoftAp(eq(TEST_INTERFACE_NAME), any(), anyBoolean(),
                any(WifiNative.SoftApListener.class))).thenReturn(false);

        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, mDefaultApConfig,
                mTestSoftApCapability);

        mSoftApManager = createSoftApManager(
                softApModeConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);

        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        verify(mListener).onStartFailure(mSoftApManager);
        verify(mWifiNative).teardownInterface(TEST_INTERFACE_NAME);
    }

    /**
     * Tests the handling of stop command when soft AP is started.
     */
    @Test
    public void stopWhenStarted() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext);

        InOrder order = inOrder(mCallback, mListener, mContext);

        mSoftApManager.stop();
        mLooper.dispatchAll();

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
        verify(mSarManager).setSapWifiState(WifiManager.WIFI_AP_STATE_DISABLED);
        verify(mWifiDiagnostics).stopLogging(TEST_INTERFACE_NAME);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        order.verify(mListener).onStopped(mSoftApManager);
    }

    /**
     * Verify that onDestroyed properly reports softap stop.
     */
    @Test
    public void cleanStopOnInterfaceDestroyed() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext);

        InOrder order = inOrder(mCallback, mListener, mContext);

        mWifiNativeInterfaceCallbackCaptor.getValue().onDestroyed(TEST_INTERFACE_NAME);

        mLooper.dispatchAll();
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        order.verify(mListener).onStopped(mSoftApManager);
    }

    /**
     * Verify that onDestroyed after softap is stopped doesn't trigger a callback.
     */
    @Test
    public void noCallbackOnInterfaceDestroyedWhenAlreadyStopped() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(softApModeConfig);

        mSoftApManager.stop();
        mLooper.dispatchAll();
        verify(mListener).onStopped(mSoftApManager);

        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);

        reset(mCallback);

        // now trigger interface destroyed and make sure callback doesn't get called
        mWifiNativeInterfaceCallbackCaptor.getValue().onDestroyed(TEST_INTERFACE_NAME);
        mLooper.dispatchAll();

        verifyNoMoreInteractions(mCallback, mListener);
    }

    /**
     * Verify that onDown is handled by SoftApManager.
     */
    @Test
    public void testInterfaceOnDownHandled() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext, mCallback, mWifiNative);

        InOrder order = inOrder(mCallback, mListener, mContext);

        mWifiNativeInterfaceCallbackCaptor.getValue().onDown(TEST_INTERFACE_NAME);

        mLooper.dispatchAll();

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        order.verify(mListener).onStopped(mSoftApManager);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(3)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLED, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_FAILED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(2), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
    }

    /**
     * Verify that onDown for a different interface name does not stop SoftApManager.
     */
    @Test
    public void testInterfaceOnDownForDifferentInterfaceDoesNotTriggerStop() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext, mCallback, mWifiNative);

        mWifiNativeInterfaceCallbackCaptor.getValue().onDown(OTHER_INTERFACE_NAME);

        mLooper.dispatchAll();

        verifyNoMoreInteractions(mContext, mCallback, mListener, mWifiNative);
    }

    /**
     * Verify that onFailure from hostapd is handled by SoftApManager.
     */
    @Test
    public void testHostapdOnFailureHandled() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext, mCallback, mWifiNative);

        InOrder order = inOrder(mCallback, mListener, mContext);
        mSoftApListenerCaptor.getValue().onFailure();
        mLooper.dispatchAll();

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        order.verify(mListener).onStopped(mSoftApManager);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(3)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLED, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_FAILED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(2), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEvent() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();

        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDetectsBandUnsatisfiedOnBand2Ghz()
            throws Exception {
        SoftApConfiguration config = createDefaultApConfig();
        Builder configBuilder = new SoftApConfiguration.Builder(config);
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        SoftApInfo testSoftApInfo = new SoftApInfo(mTestSoftApInfo);
        testSoftApInfo.setFrequency(5220);
        testSoftApInfo.setBandwidth(SoftApInfo.CHANNEL_WIDTH_20MHZ_NOHT);
        mockApInfoChangedEvent(testSoftApInfo);
        mLooper.dispatchAll();

        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);
        verify(mWifiMetrics).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDetectsBandUnsatisfiedOnBand5Ghz()
            throws Exception {
        SoftApConfiguration config = createDefaultApConfig();
        Builder configBuilder = new SoftApConfiguration.Builder(config);
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();

        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);
        verify(mWifiMetrics).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDoesNotDetectBandUnsatisfiedOnBandAny()
            throws Exception {
        SoftApConfiguration config = createDefaultApConfig();
        Builder configBuilder = new SoftApConfiguration.Builder(config);
        configBuilder.setBand(mBand256G);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();

        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);
        verify(mWifiMetrics, never()).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    /**
     * If SoftApManager gets an update for the ap channal and the frequency, it will trigger
     * callbacks to update softap information.
     */
    @Test
    public void testOnSoftApChannelSwitchedEventTriggerSoftApInfoUpdate() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();

        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);
    }

    /**
     * If SoftApManager gets an update for the ap channal and the frequency those are the same,
     * do not trigger callbacks a second time.
     */
    @Test
    public void testDoesNotTriggerCallbackForSameChannelInfoUpdate() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);

        reset(mCallback);
        // now trigger callback again, but we should have each method only called once
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(any(), any(), anyBoolean());
    }

    /**
     * If SoftApManager gets an update for the invalid ap frequency, it will not
     * trigger callbacks
     */
    @Test
    public void testHandlesInvalidChannelFrequency() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockChannelSwitchEvent(-1, TEST_AP_BANDWIDTH_FROM_IFACE_CALLBACK);
        mLooper.dispatchAll();
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(any(), any(), anyBoolean());
        verify(mWifiMetrics, never()).addSoftApChannelSwitchedEvent(any(),
                anyInt(), anyBoolean());
    }

    /**
     * If softap leave started state, it should update softap inforation which frequency is 0 via
     * trigger callbacks.
     */
    @Test
    public void testCallbackForChannelUpdateToZeroWhenLeaveSoftapStarted() throws Exception {
        InOrder order = inOrder(mCallback, mWifiMetrics);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        order.verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);

        mSoftApManager.stop();
        mLooper.dispatchAll();
        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        order.verify(mWifiMetrics, never()).addSoftApChannelSwitchedEvent(any(),
                eq(apConfig.getTargetMode()), anyBoolean());
    }

    @Test
    public void updatesConnectedClients() throws Exception {
        InOrder order = inOrder(mCallback, mWifiMetrics);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        reset(mCallback);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
    }

    /**
     * If SoftApManager gets an update for the number of connected clients that is the same, do not
     * trigger callbacks a second time.
     */
    @Test
    public void testDoesNotTriggerCallbackForSameClients() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);

        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        // now trigger callback again, but we should have each method only called once
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();

        // Should just trigger 1 time callback, the first time will be happen when softap enable
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, false, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        // now trigger callback again, but we should have each method only called once
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, false, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();

        // Should just trigger 1 time callback to update to zero client.
        // Should just trigger 1 time callback, the first time will be happen when softap enable
        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics)
                .addSoftApNumAssociatedStationsChangedEvent(0, 0,
                apConfig.getTargetMode(), mTestSoftApInfo);

    }

    @Test
    public void stopDisconnectsConnectedClients() throws Exception {
        InOrder order = inOrder(mCallback, mWifiMetrics);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                        mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);

        mSoftApManager.stop();
        mLooper.dispatchAll();

        verify(mWifiNative).forceClientDisconnect(TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                SAP_CLIENT_DISCONNECT_REASON_CODE_UNSPECIFIED);
    }

    @Test
    public void handlesInvalidConnectedClients() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        /* Invalid values should be ignored */
        mockClientConnectedEvent(null, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(any(),
                  any(), anyBoolean());
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(anyInt(),
                anyInt(), anyInt(), any());
    }

    @Test
    public void testCallbackForClientUpdateToZeroWhenLeaveSoftapStarted() throws Exception {
        InOrder order = inOrder(mCallback, mWifiMetrics);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        order.verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        mSoftApManager.stop();
        mLooper.dispatchAll();
        order.verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(0, 0,
                apConfig.getTargetMode(), mTestSoftApInfo);
        mTestWifiClientsMap.clear();
        mTestSoftApInfoMap.clear();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        // Verify timer is canceled after stop softap
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
    }

    @Test
    public void testClientConnectFailureWhenClientInBlcokedListAndClientAuthorizationDisabled()
            throws Exception {
        ArrayList<MacAddress> blockedClientList = new ArrayList<>();
        mTestSoftApCapability.setMaxSupportedClients(10);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setClientControlByUserEnabled(false);
        // Client in blocked list
        blockedClientList.add(TEST_CLIENT_MAC_ADDRESS);
        configBuilder.setBlockedClientList(blockedClientList);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();

        // Client is not allow verify
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(
                anyInt(), anyInt(), eq(apConfig.getTargetMode()), any());
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

    }

    @Test
    public void testClientDisconnectWhenClientInBlcokedLisUpdatedtAndClientAuthorizationDisabled()
            throws Exception {
        ArrayList<MacAddress> blockedClientList = new ArrayList<>();
        mTestSoftApCapability.setMaxSupportedClients(10);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setClientControlByUserEnabled(false);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        // Client connected check
        verify(mWifiNative, never()).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        reset(mCallback);
        reset(mWifiNative);
        // Update configuration
        blockedClientList.add(TEST_CLIENT_MAC_ADDRESS);
        configBuilder.setBlockedClientList(blockedClientList);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Client difconnected
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        // The callback should not trigger in configuration update case.
        verify(mCallback, never()).onBlockedClientConnecting(TEST_CONNECTED_CLIENT,
                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);

    }

    @Test
    public void testForceClientDisconnectInvokeBecauseClientAuthorizationEnabled()
            throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(10);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setClientControlByUserEnabled(true);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        reset(mCallback);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();

        // Client is not allow verify
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(anyInt(), anyInt(),
                anyInt(), any());
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(any(),
                any(), anyBoolean());

    }

    @Test
    public void testClientConnectedAfterUpdateToAllowListwhenClientAuthorizationEnabled()
            throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(10);
        ArrayList<MacAddress> allowedClientList = new ArrayList<>();
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setClientControlByUserEnabled(true);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        reset(mWifiMetrics);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        reset(mCallback);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();

        // Client is not allow verify
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(anyInt(), anyInt(),
                anyInt(), any());
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mCallback).onBlockedClientConnecting(TEST_CONNECTED_CLIENT,
                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        reset(mCallback);
        reset(mWifiNative);
        // Update configuration
        allowedClientList.add(TEST_CLIENT_MAC_ADDRESS);
        configBuilder.setAllowedClientList(allowedClientList);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Client connected again
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mWifiNative, never()).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mCallback, never()).onBlockedClientConnecting(TEST_CONNECTED_CLIENT,
                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
    }

    @Test
    public void testClientConnectedAfterUpdateToBlockListwhenClientAuthorizationEnabled()
            throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(10);
        ArrayList<MacAddress> blockedClientList = new ArrayList<>();
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setClientControlByUserEnabled(true);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        reset(mWifiMetrics);
        reset(mCallback);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();

        // Client is not allow verify
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                mTestWifiClientsMap, false);
        verify(mCallback).onBlockedClientConnecting(TEST_CONNECTED_CLIENT,
                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        reset(mCallback);
        reset(mWifiNative);
        // Update configuration
        blockedClientList.add(TEST_CLIENT_MAC_ADDRESS);
        configBuilder.setBlockedClientList(blockedClientList);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Client connected again
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(
                anyInt(), anyInt(), anyInt(), any());
        verify(mCallback, never()).onConnectedClientsOrInfoChanged(any(),
                any(), anyBoolean());
        verify(mCallback, never()).onBlockedClientConnecting(TEST_CONNECTED_CLIENT,
                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
    }

    @Test
    public void testConfigChangeToSmallAndClientAddBlockListCauseClientDisconnect()
            throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(10);
        ArrayList<MacAddress> allowedClientList = new ArrayList<>();
        allowedClientList.add(TEST_CLIENT_MAC_ADDRESS);
        allowedClientList.add(TEST_CLIENT_MAC_ADDRESS_2);
        ArrayList<MacAddress> blockedClientList = new ArrayList<>();

        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setClientControlByUserEnabled(true);
        configBuilder.setMaxNumberOfClients(2);
        configBuilder.setAllowedClientList(allowedClientList);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(2, 2,
                apConfig.getTargetMode(), mTestSoftApInfo);
        reset(mCallback);
        reset(mWifiNative);
        // Update configuration
        allowedClientList.clear();
        allowedClientList.add(TEST_CLIENT_MAC_ADDRESS_2);

        blockedClientList.add(TEST_CLIENT_MAC_ADDRESS);
        configBuilder.setBlockedClientList(blockedClientList);
        configBuilder.setAllowedClientList(allowedClientList);
        configBuilder.setMaxNumberOfClients(1);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
        verify(mWifiNative, never()).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
    }


    @Test
    public void schedulesTimeoutTimerOnStart() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        verify(mResources)
                .getInteger(R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds);
        verify(mResources)
                .getInteger(R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond);


        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());

        // The single AP should not start the bridged mode timer
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());
    }

    @Test
    public void schedulesTimeoutTimerOnStartWithConfigedValue() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setShutdownTimeoutMillis(50000);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();

        SoftApInfo expectedInfo = new SoftApInfo(mTestSoftApInfo);
        expectedInfo.setAutoShutdownTimeoutMillis(50000);
        mTestSoftApInfoMap.put(TEST_INTERFACE_NAME, expectedInfo);
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);

        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void cancelsTimeoutTimerOnStop() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        mSoftApManager.stop();
        mLooper.dispatchAll();

        // Verify timer is canceled
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
    }

    @Test
    public void cancelsTimeoutTimerOnNewClientsConnect() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        // Verify timer is canceled
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
    }

    @Test
    public void schedulesTimeoutTimerWhenAllClientsDisconnect() throws Exception {
        InOrder order = inOrder(mCallback, mWifiMetrics);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, false, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // Verify timer is scheduled again
        verify(mAlarmManager.getAlarmManager(), times(2)).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void stopsSoftApOnTimeoutMessage() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        doNothing().when(mFakeSoftApNotifier)
                .showSoftApShutdownTimeoutExpiredNotification();
        mAlarmManager.dispatch(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG);
        mLooper.dispatchAll();

        verify(mWifiNative).teardownInterface(TEST_INTERFACE_NAME);
        verify(mFakeSoftApNotifier).showSoftApShutdownTimeoutExpiredNotification();
    }

    @Test
    public void cancelsTimeoutTimerOnTimeoutToggleChangeWhenNoClients() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        SoftApConfiguration newConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setAutoShutdownEnabled(false)
                .build();
        mSoftApManager.updateConfiguration(newConfig);
        mLooper.dispatchAll();

        // Verify timer is canceled
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
    }

    @Test
    public void schedulesTimeoutTimerOnTimeoutToggleChangeWhenNoClients() throws Exception {
        // start with timeout toggle disabled
        mDefaultApConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setAutoShutdownEnabled(false)
                .build();
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        SoftApConfiguration newConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setAutoShutdownEnabled(true)
                .build();
        mSoftApManager.updateConfiguration(newConfig);
        mLooper.dispatchAll();

        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void doesNotScheduleTimeoutTimerOnStartWhenTimeoutIsDisabled() throws Exception {
        // start with timeout toggle disabled
        mDefaultApConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setAutoShutdownEnabled(false)
                .build();
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        // Verify timer is not scheduled
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void doesNotScheduleTimeoutTimerWhenAllClientsDisconnectButTimeoutIsDisabled()
            throws Exception {
        // start with timeout toggle disabled
        mDefaultApConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setAutoShutdownEnabled(false)
                .build();
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        // add client
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // remove client
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, false, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // Verify timer is not scheduled
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void resetsFactoryMacWhenRandomizationOff() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBssid(null);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(), mTestSoftApCapability);
        ArgumentCaptor<MacAddress> mac = ArgumentCaptor.forClass(MacAddress.class);

        startSoftApAndVerifyEnabled(apConfig);
        verify(mWifiNative).resetApMacToFactoryMacAddress(eq(TEST_INTERFACE_NAME));
    }

    @Test
    public void resetsFactoryMacWhenRandomizationDoesntSupport() throws Exception {
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT
                | SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD
                | SoftApCapability.SOFTAP_FEATURE_WPA3_SAE;
        SoftApCapability testSoftApCapability = new SoftApCapability(testSoftApFeature);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBssid(null);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(), testSoftApCapability);
        ArgumentCaptor<MacAddress> mac = ArgumentCaptor.forClass(MacAddress.class);

        startSoftApAndVerifyEnabled(apConfig);
        verify(mWifiNative).resetApMacToFactoryMacAddress(eq(TEST_INTERFACE_NAME));
        verify(mWifiApConfigStore, never()).randomizeBssidIfUnset(any(), any());
    }

    @Test
    public void setsCustomMac() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBssid(TEST_CLIENT_MAC_ADDRESS);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                IFACE_IP_MODE_LOCAL_ONLY, configBuilder.build(), mTestSoftApCapability);
        ArgumentCaptor<MacAddress> mac = ArgumentCaptor.forClass(MacAddress.class);
        when(mWifiNative.setApMacAddress(eq(TEST_INTERFACE_NAME), mac.capture())).thenReturn(true);

        startSoftApAndVerifyEnabled(apConfig);

        assertThat(mac.getValue()).isEqualTo(TEST_CLIENT_MAC_ADDRESS);
    }

    @Test
    public void setsCustomMacWhenSetMacNotSupport() throws Exception {
        when(mWifiNative.isApSetMacAddressSupported(any())).thenReturn(false);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBssid(TEST_CLIENT_MAC_ADDRESS);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                IFACE_IP_MODE_LOCAL_ONLY, configBuilder.build(), mTestSoftApCapability);
        ArgumentCaptor<MacAddress> mac = ArgumentCaptor.forClass(MacAddress.class);

        mSoftApManager = createSoftApManager(apConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION);
        verify(mWifiNative, never()).setApMacAddress(any(), any());
    }

    @Test
    public void setMacFailureWhenCustomMac() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBssid(TEST_CLIENT_MAC_ADDRESS);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                IFACE_IP_MODE_LOCAL_ONLY, configBuilder.build(), mTestSoftApCapability);
        ArgumentCaptor<MacAddress> mac = ArgumentCaptor.forClass(MacAddress.class);
        when(mWifiNative.setApMacAddress(eq(TEST_INTERFACE_NAME), mac.capture())).thenReturn(false);

        mSoftApManager = createSoftApManager(apConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        assertThat(mac.getValue()).isEqualTo(TEST_CLIENT_MAC_ADDRESS);
    }

    @Test
    public void setMacFailureWhenRandomMac() throws Exception {
        SoftApConfiguration randomizedBssidConfig =
                new SoftApConfiguration.Builder(mDefaultApConfig)
                .setBssid(TEST_CLIENT_MAC_ADDRESS).build();
        when(mWifiApConfigStore.randomizeBssidIfUnset(any(), any())).thenReturn(
                randomizedBssidConfig);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                IFACE_IP_MODE_LOCAL_ONLY, null, mTestSoftApCapability);
        ArgumentCaptor<MacAddress> mac = ArgumentCaptor.forClass(MacAddress.class);
        when(mWifiNative.setApMacAddress(eq(TEST_INTERFACE_NAME), mac.capture())).thenReturn(false);
        mSoftApManager = createSoftApManager(apConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
    }

    @Test
    public void setRandomMacWhenSetMacNotsupport() throws Exception {
        when(mWifiNative.isApSetMacAddressSupported(any())).thenReturn(false);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                IFACE_IP_MODE_LOCAL_ONLY, null, mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        verify(mWifiNative, never()).setApMacAddress(any(), any());
    }

    @Test
    public void testForceClientDisconnectInvokeBecauseReachMaxClient() throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(1);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        reset(mWifiMetrics);
        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS_2,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(
                anyInt(), anyInt(), anyInt(), any());
        // Trigger connection again
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // Verify just update metrics one time
        verify(mWifiMetrics).noteSoftApClientBlocked(1);
    }

    @Test
    public void testCapabilityChangeToSmallCauseClientDisconnect() throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(2);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(2, 2,
                apConfig.getTargetMode(), mTestSoftApInfo);

        // Trigger Capability Change
        mTestSoftApCapability.setMaxSupportedClients(1);
        mSoftApManager.updateCapability(mTestSoftApCapability);
        mLooper.dispatchAll();
        // Verify Disconnect will trigger
        verify(mWifiNative).forceClientDisconnect(
                        any(), any(), anyInt());
    }

    @Test
    public void testCapabilityChangeNotApplyForLohsMode() throws Exception {
        mTestSoftApCapability.setMaxSupportedClients(2);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_LOCAL_ONLY, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(2, 2,
                apConfig.getTargetMode(), mTestSoftApInfo);

        // Trigger Capability Change
        mTestSoftApCapability.setMaxSupportedClients(1);
        mSoftApManager.updateCapability(mTestSoftApCapability);
        mLooper.dispatchAll();
        // Verify Disconnect will NOT trigger even capability changed
        verify(mWifiNative, never()).forceClientDisconnect(
                        any(), any(), anyInt());
    }

    /** Starts soft AP and verifies that it is enabled successfully. */
    protected void startSoftApAndVerifyEnabled(
            SoftApModeConfiguration softApConfig) throws Exception {
        startSoftApAndVerifyEnabled(softApConfig, TEST_COUNTRY_CODE, null);
    }

    /** Starts soft AP and verifies that it is enabled successfully. */
    protected void startSoftApAndVerifyEnabled(
            SoftApModeConfiguration softApConfig, String countryCode) throws Exception {
        startSoftApAndVerifyEnabled(softApConfig, countryCode, null);
    }

    /** Starts soft AP and verifies that it is enabled successfully. */
    protected void startSoftApAndVerifyEnabled(
            SoftApModeConfiguration softApConfig, String countryCode,
            SoftApConfiguration expectedConfig) throws Exception {
        // The config which base on mDefaultApConfig and generate ramdonized mac address
        SoftApConfiguration randomizedBssidConfig = null;
        InOrder order = inOrder(mCallback, mWifiNative);

        SoftApConfiguration config = softApConfig.getSoftApConfiguration();
        if (expectedConfig == null) {
            if (config == null) {
                // Only generate randomized mac for default config since test case doesn't care it.
                randomizedBssidConfig =
                        new SoftApConfiguration.Builder(mDefaultApConfig)
                        .setBssid(TEST_INTERFACE_MAC_ADDRESS).build();
                when(mWifiApConfigStore.randomizeBssidIfUnset(any(), any())).thenReturn(
                        randomizedBssidConfig);
                expectedConfig = new SoftApConfiguration.Builder(randomizedBssidConfig)
                        .build();
            } else {
                expectedConfig = new SoftApConfiguration.Builder(config)
                        .build();
            }
        }

        SoftApConfiguration expectedConfigWithFrameworkACS = null;
        if (!softApConfig.getCapability().areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD)) {
            if (expectedConfig.getChannel() == 0 && expectedConfig.getBands().length == 1) {
                // Reset channel to 2.4G channel 11 for expected configuration
                // Reason:The test 2G freq is "ALLOWED_2G_FREQS = {2462}; //ch# 11"
                expectedConfigWithFrameworkACS = new SoftApConfiguration.Builder(expectedConfig)
                        .setChannel(11, SoftApConfiguration.BAND_2GHZ)
                        .build();
            }
        }


        mSoftApManager = createSoftApManager(softApConfig, countryCode,
                softApConfig.getTargetMode() == IFACE_IP_MODE_LOCAL_ONLY
                        ? ROLE_SOFTAP_LOCAL_ONLY : ROLE_SOFTAP_TETHERED);
        verify(mCmiMonitor).registerListener(mCmiListenerCaptor.capture());
        mLooper.dispatchAll();

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mFakeSoftApNotifier).dismissSoftApShutdownTimeoutExpiredNotification();
        order.verify(mWifiNative).setupInterfaceForSoftApMode(
                mWifiNativeInterfaceCallbackCaptor.capture(), eq(TEST_WORKSOURCE),
                eq(expectedConfig.getBand()), eq(expectedConfig.getBands().length > 1));
        ArgumentCaptor<SoftApConfiguration> configCaptor =
                ArgumentCaptor.forClass(SoftApConfiguration.class);
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        order.verify(mWifiNative).startSoftAp(eq(TEST_INTERFACE_NAME),
                configCaptor.capture(),
                eq(softApConfig.getTargetMode() ==  WifiManager.IFACE_IP_MODE_TETHERED),
                mSoftApListenerCaptor.capture());
        assertThat(configCaptor.getValue()).isEqualTo(expectedConfigWithFrameworkACS != null
                ? expectedConfigWithFrameworkACS : expectedConfig);
        mWifiNativeInterfaceCallbackCaptor.getValue().onUp(TEST_INTERFACE_NAME);
        mLooper.dispatchAll();
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLED, 0);
        order.verify(mCallback).onConnectedClientsOrInfoChanged(eq(mTestSoftApInfoMap),
                  eq(mTestWifiClientsMap), eq(expectedConfig.getBands().length > 1));
        verify(mSarManager).setSapWifiState(WifiManager.WIFI_AP_STATE_ENABLED);
        verify(mWifiDiagnostics).startLogging(TEST_INTERFACE_NAME);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_ENABLED,
                WIFI_AP_STATE_ENABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        verify(mListener).onStarted(mSoftApManager);
        verify(mWifiMetrics).addSoftApUpChangedEvent(true, softApConfig.getTargetMode(),
                TEST_DEFAULT_SHUTDOWN_TIMEOUT_MILLIS, expectedConfig.getBands().length > 1);
        verify(mWifiMetrics).updateSoftApConfiguration(config == null
                ? randomizedBssidConfig : expectedConfig, softApConfig.getTargetMode(),
                expectedConfig.getBands().length > 1);
        verify(mWifiMetrics).updateSoftApCapability(softApConfig.getCapability(),
                softApConfig.getTargetMode(), expectedConfig.getBands().length > 1);
        if (SdkLevel.isAtLeastS()) {
            verify(mCoexManager).registerCoexListener(mCoexListenerCaptor.capture());
        }
    }

    private void checkApStateChangedBroadcast(Intent intent, int expectedCurrentState,
            int expectedPrevState, int expectedErrorCode,
            String expectedIfaceName, int expectedMode) {
        int currentState = intent.getIntExtra(EXTRA_WIFI_AP_STATE, WIFI_AP_STATE_DISABLED);
        int prevState = intent.getIntExtra(EXTRA_PREVIOUS_WIFI_AP_STATE, WIFI_AP_STATE_DISABLED);
        int errorCode = intent.getIntExtra(EXTRA_WIFI_AP_FAILURE_REASON, HOTSPOT_NO_ERROR);
        String ifaceName = intent.getStringExtra(EXTRA_WIFI_AP_INTERFACE_NAME);
        int mode = intent.getIntExtra(EXTRA_WIFI_AP_MODE, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        assertEquals(expectedCurrentState, currentState);
        assertEquals(expectedPrevState, prevState);
        assertEquals(expectedErrorCode, errorCode);
        assertEquals(expectedIfaceName, ifaceName);
        assertEquals(expectedMode, mode);
    }

    @Test
    public void testForceClientDisconnectNotInvokeWhenNotSupport() throws Exception {
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_WPA3_SAE
                | SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD
                | SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION;
        SoftApCapability noClientControlCapability = new SoftApCapability(testSoftApFeature);
        noClientControlCapability.setMaxSupportedClients(1);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                noClientControlCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // feature not support thus it should not trigger disconnect
        verify(mWifiNative, never()).forceClientDisconnect(
                        any(), any(), anyInt());
        // feature not support thus client still allow connected.
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(2, 2,
                apConfig.getTargetMode(), mTestSoftApInfo);
    }

    @Test
    public void testSoftApEnableFailureBecauseSetMaxClientWhenNotSupport() throws Exception {
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_WPA3_SAE
                | SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD;
        SoftApCapability noClientControlCapability = new SoftApCapability(testSoftApFeature);
        noClientControlCapability.setMaxSupportedClients(1);
        SoftApConfiguration softApConfig = new SoftApConfiguration.Builder(
                mDefaultApConfig).setMaxNumberOfClients(1).build();

        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, softApConfig,
                noClientControlCapability);
        mSoftApManager = createSoftApManager(apConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);

        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION);
        verify(mWifiMetrics).incrementSoftApStartResult(false,
                WifiManager.SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION);
        verify(mListener).onStartFailure(mSoftApManager);
    }

    @Test
    public void testSoftApEnableFailureBecauseSecurityTypeSaeSetupButSaeNotSupport()
            throws Exception {
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT
                | SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD;
        SoftApCapability noSaeCapability = new SoftApCapability(testSoftApFeature);
        SoftApConfiguration softApConfig = new SoftApConfiguration.Builder(
                mDefaultApConfig).setPassphrase(TEST_PASSWORD,
                SoftApConfiguration.SECURITY_TYPE_WPA3_SAE).build();

        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, softApConfig,
                noSaeCapability);
        mSoftApManager = createSoftApManager(apConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);

        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION);
        verify(mListener).onStartFailure(mSoftApManager);
    }

    @Test
    public void testSoftApEnableFailureBecauseDaulBandConfigSetWhenACSNotSupport()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT
                | SoftApCapability.SOFTAP_FEATURE_WPA3_SAE;
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ, SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(testSoftApFeature);
        testCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_2GHZ, TEST_SUPPORTED_24G_CHANNELS);
        testCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_5GHZ, TEST_SUPPORTED_5G_CHANNELS);
        SoftApConfiguration softApConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setBands(dual_bands)
                .build();
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, softApConfig,
                testCapability);

        mSoftApManager = createSoftApManager(apConfig, TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);

        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION);
        verify(mWifiMetrics).incrementSoftApStartResult(false,
                WifiManager.SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION);
        verify(mListener).onStartFailure(mSoftApManager);
    }

    @Test
    public void testSoftApEnableWhenDaulBandConfigwithChannelSetWhenACSNotSupport()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        long testSoftApFeature = SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT
                | SoftApCapability.SOFTAP_FEATURE_WPA3_SAE;
        SparseIntArray dual_channels = new SparseIntArray(2);
        dual_channels.put(SoftApConfiguration.BAND_5GHZ, 149);
        dual_channels.put(SoftApConfiguration.BAND_2GHZ, 2);
        SoftApCapability testCapability = new SoftApCapability(testSoftApFeature);
        testCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_2GHZ, TEST_SUPPORTED_24G_CHANNELS);
        testCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_5GHZ, TEST_SUPPORTED_5G_CHANNELS);
        SoftApConfiguration softApConfig = new SoftApConfiguration.Builder(mDefaultApConfig)
                .setChannels(dual_channels)
                .build();
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, softApConfig,
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, null);
    }

    @Test
    public void testConfigurationChangedApplySinceDoesNotNeedToRestart() throws Exception {
        long testShutdownTimeout = 50000;
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mWifiMetrics).updateSoftApConfiguration(configBuilder.build(),
                WifiManager.IFACE_IP_MODE_TETHERED, false);

        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        reset(mCallback);
        // Trigger Configuration Change
        configBuilder.setShutdownTimeoutMillis(testShutdownTimeout);
        mSoftApManager.updateConfiguration(configBuilder.build());
        SoftApInfo expectedInfo = new SoftApInfo(mTestSoftApInfo);
        expectedInfo.setAutoShutdownTimeoutMillis(testShutdownTimeout);
        mTestSoftApInfoMap.put(TEST_INTERFACE_NAME, expectedInfo);
        mLooper.dispatchAll();
        // Verify the info changed
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        // Verify timer is canceled at this point since timeout changed
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
        // Verify timer setup again
        verify(mAlarmManager.getAlarmManager(), times(2)).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
        verify(mWifiMetrics).updateSoftApConfiguration(configBuilder.build(),
                WifiManager.IFACE_IP_MODE_TETHERED, false);
    }

    @Test
    public void testConfigurationChangedDoesNotApplySinceNeedToRestart() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());

        mLooper.dispatchAll();

        // Trigger Configuration Change
        configBuilder.setShutdownTimeoutMillis(500000);
        configBuilder.setSsid(TEST_SSID + "new");
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Verify timer cancel will not apply since changed config need to apply via restart.
        verify(mAlarmManager.getAlarmManager(), never()).cancel(
                eq(mSoftApManager.mSoftApTimeoutMessage));
    }

    @Test
    public void testConfigChangeToSmallCauseClientDisconnect() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setMaxNumberOfClients(2);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        // Second client connect and max client set is 2.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(2, 2,
                  apConfig.getTargetMode(), mTestSoftApInfo);

        // Trigger Configuration Change
        configBuilder.setMaxNumberOfClients(1);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Verify Disconnect will trigger
        verify(mWifiNative).forceClientDisconnect(
                        any(), any(), anyInt());
    }

    @Test
    public void testConfigChangeWillTriggerUpdateMetricsAgain() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setMaxNumberOfClients(1);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                configBuilder.build(), mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();


        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS_2,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);

        // Verify update metrics
        verify(mWifiMetrics).noteSoftApClientBlocked(1);

        // Trigger Configuration Change
        configBuilder.setMaxNumberOfClients(2);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();

        // Second client connect and max client set is 2.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        // Trigger Configuration Change
        configBuilder.setMaxNumberOfClients(1);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Let client disconnect due to maximum number change to small.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, false, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        // Trigger connection again
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // Verify just update metrics one time
        verify(mWifiMetrics, times(2)).noteSoftApClientBlocked(1);
    }

    /**
     * If SoftApManager gets an update for the ap channal and the frequency, it will trigger
     * callbacks to update softap information with bssid field.
     */
    @Test
    public void testBssidUpdatedWhenSoftApInfoUpdate() throws Exception {
        MacAddress testBssid = MacAddress.fromString("aa:bb:cc:11:22:33");
        SoftApConfiguration customizedBssidConfig = new SoftApConfiguration
                .Builder(mDefaultApConfig).setBssid(testBssid).build();
        when(mWifiNative.setApMacAddress(eq(TEST_INTERFACE_NAME), eq(testBssid))).thenReturn(true);
        mTestSoftApInfo.setBssid(testBssid);
        InOrder order = inOrder(mCallback, mWifiMetrics);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED,
                customizedBssidConfig, mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        order.verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);

        // Verify stop will set bssid back to null
        mSoftApManager.stop();
        mLooper.dispatchAll();
        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();
        order.verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        order.verify(mWifiMetrics, never()).addSoftApChannelSwitchedEvent(any(),
                eq(apConfig.getTargetMode()), anyBoolean());
    }

    /**
     * If SoftApManager gets an update for the invalid ap frequency, it will not
     * trigger callbacks
     */
    @Test
    public void testHandleCallbackFromWificond() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockChannelSwitchEvent(mTestSoftApInfo.getFrequency(), mTestSoftApInfo.getBandwidth());
        mLooper.dispatchAll();

        mTestSoftApInfoMap.clear();
        SoftApInfo expectedInfo = new SoftApInfo(mTestSoftApInfo);
        // Old callback should doesn't include the wifiStandard and bssid.
        expectedInfo.setBssid(null);
        expectedInfo.setWifiStandard(ScanResult.WIFI_STANDARD_UNKNOWN);
        mTestSoftApInfoMap.put(TEST_INTERFACE_NAME, expectedInfo);
        mTestWifiClientsMap.put(TEST_INTERFACE_NAME, new ArrayList<WifiClient>());
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                mTestWifiClientsMap, false);
        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(
                new ArrayList<>(mTestSoftApInfoMap.values()),
                apConfig.getTargetMode(), false);
    }

    @Test
    public void testForceClientFailureWillTriggerForceDisconnectAgain() throws Exception {
        when(mWifiNative.forceClientDisconnect(any(), any(), anyInt())).thenReturn(false);

        mTestSoftApCapability.setMaxSupportedClients(1);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        reset(mWifiMetrics);
        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS_2,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
        assertEquals(1, mSoftApManager.mPendingDisconnectClients.size());
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(
                anyInt(), anyInt(), anyInt(), any());

        // Let force disconnect succeed on next time.
        when(mWifiNative.forceClientDisconnect(any(), any(), anyInt())).thenReturn(true);

        mLooper.moveTimeForward(mSoftApManager.SOFT_AP_PENDING_DISCONNECTION_CHECK_DELAY_MS);
        mLooper.dispatchAll();
        verify(mWifiNative, times(2)).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS_2,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);

        // The pending list doesn't clean, it needs to wait client connection update event.
        assertEquals(1, mSoftApManager.mPendingDisconnectClients.size());

    }

    @Test
    public void testForceClientFailureButClientDisconnectSelf() throws Exception {
        when(mWifiNative.forceClientDisconnect(any(), any(), anyInt())).thenReturn(false);

        mTestSoftApCapability.setMaxSupportedClients(1);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
        reset(mCallback);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();

        verify(mCallback).onConnectedClientsOrInfoChanged(mTestSoftApInfoMap,
                  mTestWifiClientsMap, false);

        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(1, 1,
                apConfig.getTargetMode(), mTestSoftApInfo);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));

        reset(mWifiMetrics);
        // Second client connect and max client set is 1.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        verify(mWifiNative).forceClientDisconnect(
                        TEST_INTERFACE_NAME, TEST_CLIENT_MAC_ADDRESS_2,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(
                anyInt(), anyInt(), anyInt(), any());
        // Receive second client disconnection.
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, false, TEST_INTERFACE_NAME, false);
        mLooper.dispatchAll();
        // Sleep to wait execute pending list check
        reset(mWifiNative);
        mLooper.moveTimeForward(mSoftApManager.SOFT_AP_PENDING_DISCONNECTION_CHECK_DELAY_MS);
        mLooper.dispatchAll();
        verify(mWifiNative, never()).forceClientDisconnect(any(), any(), anyInt());
    }

    /**
     * Test that dual interfaces will be setup when dual band config.
     */
    @Test
    public void testSetupDualBandForSoftApModeApInterfaceName() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = new int[] {
                SoftApConfiguration.BAND_2GHZ, SoftApConfiguration.BAND_5GHZ};
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBands(dual_bands);
        configBuilder.setSsid(TEST_SSID);
        SoftApModeConfiguration dualBandConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        mSoftApManager = createSoftApManager(dualBandConfig,
                TEST_COUNTRY_CODE, ROLE_SOFTAP_TETHERED);
        verify(mWifiNative).setupInterfaceForSoftApMode(
                any(), any(), eq(SoftApConfiguration.BAND_2GHZ), eq(true));
    }

    @Test
    public void testOnInfoChangedFromDifferentInstancesTriggerSoftApInfoUpdate() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mCallback);
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, false);
    }

    @Test
    public void schedulesTimeoutTimerOnStartInBridgedMode() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ ,
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ};
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        verify(mResources)
                .getInteger(R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds);
        verify(mResources)
                .getInteger(R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond);


        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());

        // Verify the bridged mode timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        // Trigger the alarm
        mSoftApManager.mSoftApBridgedModeIdleInstanceTimeoutMessage.onAlarm();
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative).removeIfaceInstanceFromBridgedApIface(eq(TEST_INTERFACE_NAME),
                eq(TEST_SECOND_INTERFACE_NAME));
        mLooper.dispatchAll();
        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();

        mTestSoftApInfoMap.put(mTestSoftApInfo.getApInstanceIdentifier(), mTestSoftApInfo);
        mTestWifiClientsMap.put(mTestSoftApInfo.getApInstanceIdentifier(),
                new ArrayList<WifiClient>());

        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
    }

    @Test
    public void schedulesTimeoutTimerWorkFlowInBridgedMode() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = new int[] {
                SoftApConfiguration.BAND_2GHZ, SoftApConfiguration.BAND_5GHZ};
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        verify(mResources)
                .getInteger(R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds);
        verify(mResources)
                .getInteger(R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond);


        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());

        // Verify idle timer in bridged mode is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);


        // One Client connected
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
        // Verify original timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(eq(mSoftApManager.mSoftApTimeoutMessage));
        // Verify idle timer is NOT canceled at this point
        verify(mAlarmManager.getAlarmManager(), never())
                .cancel(eq(mSoftApManager.mSoftApBridgedModeIdleInstanceTimeoutMessage));

        // Second client connected to same interface
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(4)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
        // Verify idle timer is NOT canceled at this point
        verify(mAlarmManager.getAlarmManager(), never())
                .cancel(eq(mSoftApManager.mSoftApBridgedModeIdleInstanceTimeoutMessage));

        // Second client disconnected from the current interface and connected to another one
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, false, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(5)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, true, TEST_SECOND_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(6)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
        // Verify idle timer is canceled at this point
        verify(mAlarmManager.getAlarmManager())
                .cancel(eq(mSoftApManager.mSoftApBridgedModeIdleInstanceTimeoutMessage));
        // Second client disconnect
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS_2, false,
                TEST_SECOND_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        verify(mCallback, times(7)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
        // Verify idle timer in bridged mode is scheduled again
        verify(mAlarmManager.getAlarmManager(), times(2)).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());
        // Trigger the alarm
        mSoftApManager.mSoftApBridgedModeIdleInstanceTimeoutMessage.onAlarm();
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative).removeIfaceInstanceFromBridgedApIface(eq(TEST_INTERFACE_NAME),
                eq(TEST_SECOND_INTERFACE_NAME));

        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();
        WifiClient client = new WifiClient(TEST_CLIENT_MAC_ADDRESS, TEST_INTERFACE_NAME);
        List<WifiClient> targetList = new ArrayList();
        targetList.add(client);

        mTestSoftApInfoMap.put(mTestSoftApInfo.getApInstanceIdentifier(), mTestSoftApInfo);
        mTestWifiClientsMap.put(mTestSoftApInfo.getApInstanceIdentifier(), targetList);
        mLooper.dispatchAll();
        verify(mCallback, times(8)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        // Force all client disconnected
        // reset the alarm mock
        reset(mAlarmManager.getAlarmManager());
        mockClientConnectedEvent(TEST_CLIENT_MAC_ADDRESS, false, TEST_INTERFACE_NAME, true);
        mLooper.dispatchAll();
        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
        // The single AP should not start the bridged mode timer.
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());
    }

    @Test
    public void schedulesTimeoutTimerOnStartInBridgedModeWhenOpportunisticShutdownDisabled()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ ,
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ};
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        configBuilder.setBridgedModeOpportunisticShutdownEnabled(false);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        verify(mResources)
                .getInteger(R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds);
        verify(mResources)
                .getInteger(R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond);


        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());

        // Verify the bridged mode timer is scheduled
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());
    }

    @Test
    public void testBridgedModeOpportunisticShutdownConfigureChanged()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ ,
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ};
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        configBuilder.setBridgedModeOpportunisticShutdownEnabled(false);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);

        verify(mResources)
                .getInteger(R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds);
        verify(mResources)
                .getInteger(R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond);


        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());

        // Verify the bridged mode timer is scheduled
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());
        configBuilder.setBridgedModeOpportunisticShutdownEnabled(true);
        mSoftApManager.updateConfiguration(configBuilder.build());
        mLooper.dispatchAll();
        // Verify the bridged mode timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG),
                any(), any());
    }

    @Test
    public void testBridgedModeFallbackToSingleModeDueToUnavailableBand()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_6GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        testCapability.setSupportedChannelList(SoftApConfiguration.BAND_6GHZ, new int[0]);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        // Reset band to 2.4G | 5G to generate expected configuration
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());
    }

    @Test
    public void testBridgedModeFallbackToSingleModeDueToPrimaryWifiConnectToUnavailableChannel()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe. Let Wifi connect to 5200 (CH40)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5200);
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        // Reset band to 2.4G | 5G to generate expected configuration
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());
    }

    @Test
    public void testBridgedModeFallbackToSingleModeDueToSecondWifiConnectToUnavailableChannel()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        // Prepare second ClientModeManager
        List<ClientModeManager> testClientModeManagers = new ArrayList<>(mTestClientModeManagers);
        testClientModeManagers.add(mSecondConcreteClientModeManager);
        when(mSecondConcreteClientModeManager.syncRequestConnectionInfo())
                .thenReturn(mSecondWifiInfo);
        when(mActiveModeWarden.getClientModeManagers()).thenReturn(testClientModeManagers);
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe. Let Wifi connect to 5200 (CH40)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5180);
        when(mSecondWifiInfo.getFrequency()).thenReturn(5200);
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        // Reset band to 2.4G | 5G to generate expected configuration
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());
    }

    @Test
    public void testKeepBridgedModeWhenWifiConnectToAvailableChannel()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe. Let Wifi connect to 5180 (CH36)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5180);
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());
    }

    @Test
    public void testBridgedModeKeepDueToCoexIsSoftUnsafeWhenStartingSAP()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);

        // TEST_SUPPORTED_5G_CHANNELS = 36, 149,
        // mark to unsafe but it doesn't change to hard unsafe.
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 36),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149)
        ));

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());
    }

    @Test
    public void testBridgedModeFallbackToSingleModeDueToCoexIsHardUnsafe()
            throws Exception {
        //leslee
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);

        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe.
        when(mCoexManager.getCoexRestrictions()).thenReturn(WifiManager.COEX_RESTRICTION_SOFTAP);
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 36),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149)
        ));

        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        // Reset band to 2.4G to generate expected configuration
        configBuilder.setBand(SoftApConfiguration.BAND_2GHZ);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());
    }

    @Test
    public void testBridgedModeKeepWhenCoexChangedToSoftUnsafe()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to safe. Let Wifi connect to 5180 (CH36)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5180);
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        // Test with soft unsafe channels
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe.
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 36),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149)
        ));

        // Trigger coex unsafe channel changed
        mCoexListenerCaptor.getValue().onCoexUnsafeChannelsChanged();
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative, never()).removeIfaceInstanceFromBridgedApIface(any(),
                any());
    }


    @Test
    public void testBridgedModeShutDownInstanceDueToCoexIsHardUnsafe()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to safe. Let Wifi connect to 5180 (CH36)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5180);
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        // Test with hard unsafe channels
        when(mCoexManager.getCoexRestrictions()).thenReturn(WifiManager.COEX_RESTRICTION_SOFTAP);
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe.
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 36),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149)
        ));

        // Trigger coex unsafe channel changed
        mCoexListenerCaptor.getValue().onCoexUnsafeChannelsChanged();
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative).removeIfaceInstanceFromBridgedApIface(eq(TEST_INTERFACE_NAME),
                eq(TEST_SECOND_INTERFACE_NAME));
        mLooper.dispatchAll();
        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();

        mTestSoftApInfoMap.put(mTestSoftApInfo.getApInstanceIdentifier(), mTestSoftApInfo);
        mTestWifiClientsMap.put(mTestSoftApInfo.getApInstanceIdentifier(),
                new ArrayList<WifiClient>());

        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
    }

    @Test
    public void testBridgedModeKeepWhenCoexChangedButAvailableChannelExist()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe. Let Wifi connect to 5180 (CH36)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5180);
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);


        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, only mark 36 is unsafe.
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 36)
        ));

        // Trigger coex unsafe channel changed
        mCoexListenerCaptor.getValue().onCoexUnsafeChannelsChanged();
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative, never()).removeIfaceInstanceFromBridgedApIface(any(), any());
    }

    @Test
    public void testBridgedModeKeepWhenWifiConnectedToAvailableChannel()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe. Let Wifi connect to 5180 (CH36)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5180);

        // Trigger wifi connected
        mCmiListenerCaptor.getValue().onL2Connected(mConcreteClientModeManager);
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative, never()).removeIfaceInstanceFromBridgedApIface(any(), any());
    }

    @Test
    public void testBridgedModeShutDownInstanceDueToWifiConnectedToUnavailableChannel()
            throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        int[] dual_bands = {SoftApConfiguration.BAND_2GHZ, SoftApConfiguration.BAND_5GHZ};
        SoftApCapability testCapability = new SoftApCapability(mTestSoftApCapability);
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setSsid(TEST_SSID);
        configBuilder.setBands(dual_bands);
        SoftApModeConfiguration apConfig = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, configBuilder.build(),
                testCapability);
        startSoftApAndVerifyEnabled(apConfig, TEST_COUNTRY_CODE, configBuilder.build());

        // SoftApInfo updated
        mockApInfoChangedEvent(mTestSoftApInfo);
        mLooper.dispatchAll();
        verify(mCallback).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        mockApInfoChangedEvent(mTestSoftApInfoOnSecondInterface);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);

        // TEST_SUPPORTED_5G_CHANNELS = 36, 149, mark to unsafe. Let Wifi connect to 5945 (6G)
        when(mPrimaryWifiInfo.getFrequency()).thenReturn(5945);

        // Trigger wifi connected
        mCmiListenerCaptor.getValue().onL2Connected(mConcreteClientModeManager);
        mLooper.dispatchAll();
        // Verify the remove correct iface and instance
        verify(mWifiNative).removeIfaceInstanceFromBridgedApIface(eq(TEST_INTERFACE_NAME),
                eq(TEST_SECOND_INTERFACE_NAME));
        mLooper.dispatchAll();
        mTestSoftApInfoMap.clear();
        mTestWifiClientsMap.clear();

        mTestSoftApInfoMap.put(mTestSoftApInfo.getApInstanceIdentifier(), mTestSoftApInfo);
        mTestWifiClientsMap.put(mTestSoftApInfo.getApInstanceIdentifier(),
                new ArrayList<WifiClient>());

        verify(mCallback, times(3)).onConnectedClientsOrInfoChanged(
                mTestSoftApInfoMap, mTestWifiClientsMap, true);
    }

    @Test
    public void testUpdateCountryCodeWhenConfigDisabled() throws Exception {
        when(mResources.getBoolean(R.bool.config_wifiSoftApDynamicCountryCodeUpdateSupported))
                .thenReturn(false);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mWifiNative);
        mSoftApManager.updateCountryCode(TEST_COUNTRY_CODE + "TW");
        mLooper.dispatchAll();
        verify(mWifiNative, never()).setApCountryCode(any(), any());
    }

    @Test
    public void testUpdateCountryCodeWhenConfigEnabled() throws Exception {
        when(mResources.getBoolean(R.bool.config_wifiSoftApDynamicCountryCodeUpdateSupported))
                .thenReturn(true);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mWifiNative);
        mSoftApManager.updateCountryCode(TEST_COUNTRY_CODE + "TW");
        mLooper.dispatchAll();
        verify(mWifiNative).setApCountryCode(any(), any());
    }

    @Test
    public void testUpdateSameCountryCodeWhenConfigEnabled() throws Exception {
        when(mResources.getBoolean(R.bool.config_wifiSoftApDynamicCountryCodeUpdateSupported))
                .thenReturn(true);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null,
                mTestSoftApCapability);
        startSoftApAndVerifyEnabled(apConfig);
        reset(mWifiNative);
        mSoftApManager.updateCountryCode(TEST_COUNTRY_CODE);
        mLooper.dispatchAll();
        verify(mWifiNative, never()).setApCountryCode(any(), any());
    }
}
