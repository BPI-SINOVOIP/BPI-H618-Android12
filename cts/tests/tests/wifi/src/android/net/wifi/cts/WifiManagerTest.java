/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.net.wifi.cts;

import static android.net.NetworkCapabilities.NET_CAPABILITY_NOT_METERED;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;
import static android.net.wifi.WifiAvailableChannel.OP_MODE_SAP;
import static android.net.wifi.WifiAvailableChannel.OP_MODE_STA;
import static android.net.wifi.WifiConfiguration.INVALID_NETWORK_ID;
import static android.net.wifi.WifiManager.COEX_RESTRICTION_SOFTAP;
import static android.net.wifi.WifiManager.COEX_RESTRICTION_WIFI_AWARE;
import static android.net.wifi.WifiManager.COEX_RESTRICTION_WIFI_DIRECT;
import static android.net.wifi.WifiScanner.WIFI_BAND_24_GHZ;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertNotEquals;

import android.annotation.NonNull;
import android.app.UiAutomation;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.ConnectivityManager;
import android.net.MacAddress;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.TetheringManager;
import android.net.Uri;
import android.net.wifi.CoexUnsafeChannel;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.SoftApInfo;
import android.net.wifi.WifiAvailableChannel;
import android.net.wifi.WifiClient;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.SubsystemRestartTrackingCallback;
import android.net.wifi.WifiManager.WifiLock;
import android.net.wifi.WifiNetworkConnectionStatistics;
import android.net.wifi.WifiNetworkSuggestion;
import android.net.wifi.hotspot2.ConfigParser;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.hotspot2.PasspointConfiguration;
import android.net.wifi.hotspot2.ProvisioningCallback;
import android.net.wifi.hotspot2.pps.Credential;
import android.net.wifi.hotspot2.pps.HomeSp;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.HandlerThread;
import android.os.Process;
import android.os.SystemClock;
import android.os.UserHandle;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.AsbSecurityTest;
import android.provider.Settings;
import android.support.test.uiautomator.UiDevice;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseIntArray;

import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.ApiLevelUtil;
import com.android.compatibility.common.util.FeatureUtil;
import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.PropertyUtil;
import com.android.compatibility.common.util.ShellIdentityUtils;
import com.android.compatibility.common.util.SystemUtil;
import com.android.compatibility.common.util.ThrowingRunnable;
import com.android.modules.utils.build.SdkLevel;
import com.android.net.module.util.MacAddressUtils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Constructor;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

@AppModeFull(reason = "Cannot get WifiManager in instant app mode")
public class WifiManagerTest extends WifiJUnit3TestBase {
    private static class MySync {
        int expectedState = STATE_NULL;
    }

    private WifiManager mWifiManager;
    private ConnectivityManager mConnectivityManager;
    private TetheringManager mTetheringManager;
    private WifiLock mWifiLock;
    private static MySync mMySync;
    private List<ScanResult> mScanResults = null;
    private NetworkInfo mNetworkInfo =
            new NetworkInfo(ConnectivityManager.TYPE_WIFI, TelephonyManager.NETWORK_TYPE_UNKNOWN,
                    "wifi", "unknown");
    private final Object mLock = new Object();
    private UiDevice mUiDevice;
    private boolean mWasVerboseLoggingEnabled;
    private boolean mWasScanThrottleEnabled;
    private SoftApConfiguration mOriginalSoftApConfig = null;

    // Please refer to WifiManager
    private static final int MIN_RSSI = -100;
    private static final int MAX_RSSI = -55;

    private static final int STATE_NULL = 0;
    private static final int STATE_WIFI_CHANGING = 1;
    private static final int STATE_WIFI_ENABLED = 2;
    private static final int STATE_WIFI_DISABLED = 3;
    private static final int STATE_SCANNING = 4;
    private static final int STATE_SCAN_DONE = 5;

    private static final String TAG = "WifiManagerTest";
    private static final String SSID1 = "\"WifiManagerTest\"";
    // A full single scan duration is about 6-7 seconds if country code is set
    // to US. If country code is set to world mode (00), we would expect a scan
    // duration of roughly 8 seconds. So we set scan timeout as 9 seconds here.
    private static final int SCAN_TEST_WAIT_DURATION_MS = 9000;
    private static final int TEST_WAIT_DURATION_MS = 10_000;
    private static final int WIFI_CONNECT_TIMEOUT_MILLIS = 30_000;
    private static final int WAIT_MSEC = 60;
    private static final int DURATION_SCREEN_TOGGLE = 2000;
    private static final int DURATION_SETTINGS_TOGGLE = 1_000;
    private static final int WIFI_SCAN_TEST_CACHE_DELAY_MILLIS = 3 * 60 * 1000;

    private static final int ENFORCED_NUM_NETWORK_SUGGESTIONS_PER_APP = 50;

    private static final String TEST_PAC_URL = "http://www.example.com/proxy.pac";
    private static final String MANAGED_PROVISIONING_PACKAGE_NAME
            = "com.android.managedprovisioning";

    private static final String TEST_SSID_UNQUOTED = "testSsid1";
    private static final String TEST_IP_ADDRESS = "192.168.5.5";
    private static final String TEST_MAC_ADDRESS = "aa:bb:cc:dd:ee:ff";
    private static final MacAddress TEST_MAC = MacAddress.fromString(TEST_MAC_ADDRESS);
    private static final String TEST_PASSPHRASE = "passphrase";
    private static final String PASSPOINT_INSTALLATION_FILE_WITH_CA_CERT =
            "assets/ValidPasspointProfile.base64";
    private static final String TYPE_WIFI_CONFIG = "application/x-wifi-config";
    private static final String TEST_PSK_CAP = "[RSN-PSK-CCMP]";
    private static final String TEST_BSSID = "00:01:02:03:04:05";
    private static final String TEST_COUNTRY_CODE = "JP";
    private static final String TEST_DOM_SUBJECT_MATCH = "domSubjectMatch";
    private static final int TEST_SUB_ID = 2;

    private IntentFilter mIntentFilter;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (action.equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)) {

                synchronized (mMySync) {
                    if (intent.getBooleanExtra(WifiManager.EXTRA_RESULTS_UPDATED, false)) {
                        mScanResults = mWifiManager.getScanResults();
                    } else {
                        mScanResults = null;
                    }
                    mMySync.expectedState = STATE_SCAN_DONE;
                    mMySync.notifyAll();
                }
            } else if (action.equals(WifiManager.WIFI_STATE_CHANGED_ACTION)) {
                int newState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE,
                        WifiManager.WIFI_STATE_UNKNOWN);
                synchronized (mMySync) {
                    if (newState == WifiManager.WIFI_STATE_ENABLED) {
                        Log.d(TAG, "*** New WiFi state is ENABLED ***");
                        mMySync.expectedState = STATE_WIFI_ENABLED;
                        mMySync.notifyAll();
                    } else if (newState == WifiManager.WIFI_STATE_DISABLED) {
                        Log.d(TAG, "*** New WiFi state is DISABLED ***");
                        mMySync.expectedState = STATE_WIFI_DISABLED;
                        mMySync.notifyAll();
                    }
                }
            } else if (action.equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
                synchronized (mMySync) {
                    mNetworkInfo =
                            (NetworkInfo) intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                    if (mNetworkInfo.getState() == NetworkInfo.State.CONNECTED)
                        mMySync.notifyAll();
                }
            }
        }
    };
    // Initialize with an invalid status value (0)
    private int mProvisioningStatus = 0;
    // Initialize with an invalid status value (0)
    private int mProvisioningFailureStatus = 0;
    private boolean mProvisioningComplete = false;
    private ProvisioningCallback mProvisioningCallback = new ProvisioningCallback() {
        @Override
        public void onProvisioningFailure(int status) {
            synchronized (mLock) {
                mProvisioningFailureStatus = status;
                mLock.notify();
            }
        }

        @Override
        public void onProvisioningStatus(int status) {
            synchronized (mLock) {
                mProvisioningStatus = status;
                mLock.notify();
            }
        }

        @Override
        public void onProvisioningComplete() {
            mProvisioningComplete = true;
        }
    };
    private int mSubsystemRestartStatus = 0; // 0: nada, 1: restarting, 2: restarted
    private SubsystemRestartTrackingCallback mSubsystemRestartTrackingCallback =
            new SubsystemRestartTrackingCallback() {
                @Override
                public void onSubsystemRestarting() {
                    synchronized (mLock) {
                        mSubsystemRestartStatus = 1;
                        mLock.notify();
                    }
                }

                @Override
                public void onSubsystemRestarted() {
                    synchronized (mLock) {
                        mSubsystemRestartStatus = 2;
                        mLock.notify();
                    }
                }
            };
    private static final String TEST_SSID = "TEST SSID";
    private static final String TEST_FRIENDLY_NAME = "Friendly Name";
    private static final Map<String, String> TEST_FRIENDLY_NAMES =
            new HashMap<String, String>() {
                {
                    put("en", TEST_FRIENDLY_NAME);
                    put("kr", TEST_FRIENDLY_NAME + 2);
                    put("jp", TEST_FRIENDLY_NAME + 3);
                }
            };
    private static final String TEST_SERVICE_DESCRIPTION = "Dummy Service";
    private static final Uri TEST_SERVER_URI = Uri.parse("https://test.com");
    private static final String TEST_NAI = "test.access.com";
    private static final List<Integer> TEST_METHOD_LIST =
            Arrays.asList(1 /* METHOD_SOAP_XML_SPP */);
    private final HandlerThread mHandlerThread = new HandlerThread("WifiManagerTest");
    protected final Executor mExecutor;
    {
        mHandlerThread.start();
        mExecutor = new HandlerExecutor(new Handler(mHandlerThread.getLooper()));
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mMySync = new MySync();
        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        mIntentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        mIntentFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
        mIntentFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
        mIntentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        mIntentFilter.addAction(WifiManager.RSSI_CHANGED_ACTION);
        mIntentFilter.addAction(WifiManager.NETWORK_IDS_CHANGED_ACTION);
        mIntentFilter.addAction(WifiManager.ACTION_PICK_WIFI_NETWORK);

        mContext.registerReceiver(mReceiver, mIntentFilter);
        mWifiManager = (WifiManager) getContext().getSystemService(Context.WIFI_SERVICE);
        mConnectivityManager = getContext().getSystemService(ConnectivityManager.class);
        mTetheringManager = getContext().getSystemService(TetheringManager.class);
        assertNotNull(mWifiManager);
        assertNotNull(mTetheringManager);

        // turn on verbose logging for tests
        mWasVerboseLoggingEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.isVerboseLoggingEnabled());
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setVerboseLoggingEnabled(true));
        // Disable scan throttling for tests.
        mWasScanThrottleEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.isScanThrottleEnabled());
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setScanThrottleEnabled(false));

        mWifiLock = mWifiManager.createWifiLock(TAG);
        mWifiLock.acquire();
        // enable Wifi
        if (!mWifiManager.isWifiEnabled()) setWifiEnabled(true);
        PollingCheck.check("Wifi not enabled", TEST_WAIT_DURATION_MS,
                () -> mWifiManager.isWifiEnabled());

        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        turnScreenOnNoDelay();

        synchronized (mMySync) {
            mMySync.expectedState = STATE_NULL;
        }

        List<WifiConfiguration> savedNetworks = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getConfiguredNetworks);
        assertFalse("Need at least one saved network", savedNetworks.isEmpty());

        // Get original config for restore
        mOriginalSoftApConfig = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getSoftApConfiguration);
    }

    @Override
    protected void tearDown() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            super.tearDown();
            return;
        }
        if (!mWifiManager.isWifiEnabled())
            setWifiEnabled(true);
        mWifiLock.release();
        mContext.unregisterReceiver(mReceiver);
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setScanThrottleEnabled(mWasScanThrottleEnabled));
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setVerboseLoggingEnabled(mWasVerboseLoggingEnabled));
        // restore original softap config
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setSoftApConfiguration(mOriginalSoftApConfig));
        Thread.sleep(TEST_WAIT_DURATION_MS);
        super.tearDown();
    }

    private void setWifiEnabled(boolean enable) throws Exception {
        synchronized (mMySync) {
            if (mWifiManager.isWifiEnabled() != enable) {
                // the new state is different, we expect it to change
                mMySync.expectedState = STATE_WIFI_CHANGING;
            } else {
                mMySync.expectedState = (enable ? STATE_WIFI_ENABLED : STATE_WIFI_DISABLED);
            }
            // now trigger the change using shell commands.
            SystemUtil.runShellCommand("svc wifi " + (enable ? "enable" : "disable"));
            waitForExpectedWifiState(enable);
        }
    }

    private void waitForExpectedWifiState(boolean enabled) throws InterruptedException {
        synchronized (mMySync) {
            long timeout = System.currentTimeMillis() + TEST_WAIT_DURATION_MS;
            int expected = (enabled ? STATE_WIFI_ENABLED : STATE_WIFI_DISABLED);
            while (System.currentTimeMillis() < timeout
                    && mMySync.expectedState != expected) {
                mMySync.wait(WAIT_MSEC);
            }
        }
    }

    // Get the current scan status from sticky broadcast.
    private boolean isScanCurrentlyAvailable() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.ACTION_WIFI_SCAN_AVAILABILITY_CHANGED);
        Intent intent = mContext.registerReceiver(null, intentFilter);
        assertNotNull(intent);
        if (intent.getAction().equals(WifiManager.ACTION_WIFI_SCAN_AVAILABILITY_CHANGED)) {
            return intent.getBooleanExtra(WifiManager.EXTRA_SCAN_AVAILABLE, false);
        }
        return false;
    }

    private void startScan() throws Exception {
        synchronized (mMySync) {
            mMySync.expectedState = STATE_SCANNING;
            mScanResults = null;
            assertTrue(mWifiManager.startScan());
            long timeout = System.currentTimeMillis() + SCAN_TEST_WAIT_DURATION_MS;
            while (System.currentTimeMillis() < timeout && mMySync.expectedState == STATE_SCANNING)
                mMySync.wait(WAIT_MSEC);
        }
    }

    private void waitForNetworkInfoState(NetworkInfo.State state, int timeoutMillis)
            throws Exception {
        synchronized (mMySync) {
            if (mNetworkInfo.getState() == state) return;
            long timeout = System.currentTimeMillis() + timeoutMillis;
            while (System.currentTimeMillis() < timeout
                    && mNetworkInfo.getState() != state)
                mMySync.wait(WAIT_MSEC);
            assertEquals(state, mNetworkInfo.getState());
        }
    }

    private void waitForConnection() throws Exception {
        waitForNetworkInfoState(NetworkInfo.State.CONNECTED, WIFI_CONNECT_TIMEOUT_MILLIS);
    }

    private void waitForDisconnection() throws Exception {
        waitForNetworkInfoState(NetworkInfo.State.DISCONNECTED, TEST_WAIT_DURATION_MS);
    }

    private void ensureNotNetworkInfoState(NetworkInfo.State state) throws Exception {
        synchronized (mMySync) {
            long timeout = System.currentTimeMillis() + TEST_WAIT_DURATION_MS + WAIT_MSEC;
            while (System.currentTimeMillis() < timeout) {
                assertNotEquals(state, mNetworkInfo.getState());
                mMySync.wait(WAIT_MSEC);
            }
        }
    }

    private void ensureNotConnected() throws Exception {
        ensureNotNetworkInfoState(NetworkInfo.State.CONNECTED);
    }

    private void ensureNotDisconnected() throws Exception {
        ensureNotNetworkInfoState(NetworkInfo.State.DISCONNECTED);
    }

    private boolean existSSID(String ssid) {
        for (final WifiConfiguration w : mWifiManager.getConfiguredNetworks()) {
            if (w.SSID.equals(ssid))
                return true;
        }
        return false;
    }

    private int findConfiguredNetworks(String SSID, List<WifiConfiguration> networks) {
        for (final WifiConfiguration w : networks) {
            if (w.SSID.equals(SSID))
                return networks.indexOf(w);
        }
        return -1;
    }

    /**
     * Test creation of WifiManager Lock.
     */
    public void testWifiManagerLock() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final String TAG = "Test";
        assertNotNull(mWifiManager.createWifiLock(TAG));
        assertNotNull(mWifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL, TAG));
    }

    /**
     * Test wifi scanning when Wifi is off and location scanning is turned on.
     */
    public void testWifiManagerScanWhenWifiOffLocationTurnedOn() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!hasLocationFeature()) {
            Log.d(TAG, "Skipping test as location is not supported");
            return;
        }
        if (!isLocationEnabled()) {
            fail("Please enable location for this test - since Marshmallow WiFi scan results are"
                    + " empty when location is disabled!");
        }
        runWithScanningEnabled(() -> {
            setWifiEnabled(false);
            Thread.sleep(TEST_WAIT_DURATION_MS);
            startScan();
            if (mWifiManager.isScanAlwaysAvailable() && isScanCurrentlyAvailable()) {
                // Make sure at least one AP is found.
                assertNotNull("mScanResult should not be null!", mScanResults);
                assertFalse("empty scan results!", mScanResults.isEmpty());
            } else {
                // Make sure no scan results are available.
                assertNull("mScanResult should be null!", mScanResults);
            }
            final String TAG = "Test";
            assertNotNull(mWifiManager.createWifiLock(TAG));
            assertNotNull(mWifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL, TAG));
        });
    }

    /**
     * Restart WiFi subsystem - verify that privileged call fails.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testRestartWifiSubsystemShouldFailNoPermission() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        try {
            mWifiManager.restartWifiSubsystem();
            fail("The restartWifiSubsystem should not succeed - privileged call");
        } catch (SecurityException e) {
            // expected
        }
    }

    /**
     * Restart WiFi subsystem and verify transition through states.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testRestartWifiSubsystem() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mSubsystemRestartStatus = 0; // 0: uninitialized
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.registerSubsystemRestartTrackingCallback(mExecutor,
                    mSubsystemRestartTrackingCallback);
            synchronized (mLock) {
                mWifiManager.restartWifiSubsystem();
                mLock.wait(TEST_WAIT_DURATION_MS);
            }
            assertEquals(mSubsystemRestartStatus, 1); // 1: restarting
            waitForExpectedWifiState(false);
            assertFalse(mWifiManager.isWifiEnabled());
            synchronized (mLock) {
                mLock.wait(TEST_WAIT_DURATION_MS);
                assertEquals(mSubsystemRestartStatus, 2); // 2: restarted
            }
            waitForExpectedWifiState(true);
            assertTrue(mWifiManager.isWifiEnabled());
        } finally {
            // cleanup
            mWifiManager.unregisterSubsystemRestartTrackingCallback(
                    mSubsystemRestartTrackingCallback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * test point of wifiManager properties:
     * 1.enable properties
     * 2.DhcpInfo properties
     * 3.wifi state
     * 4.ConnectionInfo
     */
    public void testWifiManagerProperties() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        setWifiEnabled(true);
        assertTrue(mWifiManager.isWifiEnabled());
        assertNotNull(mWifiManager.getDhcpInfo());
        assertEquals(WifiManager.WIFI_STATE_ENABLED, mWifiManager.getWifiState());
        mWifiManager.getConnectionInfo();
        setWifiEnabled(false);
        assertFalse(mWifiManager.isWifiEnabled());
    }

    /**
     * Test WiFi scan timestamp - fails when WiFi scan timestamps are inconsistent with
     * {@link SystemClock#elapsedRealtime()} on device.<p>
     * To run this test in cts-tradefed:
     * run cts --class android.net.wifi.cts.WifiManagerTest --method testWifiScanTimestamp
     */
    @VirtualDeviceNotSupported
    public void testWifiScanTimestamp() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            Log.d(TAG, "Skipping test as WiFi is not supported");
            return;
        }
        if (!hasLocationFeature()) {
            Log.d(TAG, "Skipping test as location is not supported");
            return;
        }
        if (!isLocationEnabled()) {
            fail("Please enable location for this test - since Marshmallow WiFi scan results are"
                    + " empty when location is disabled!");
        }
        if (!mWifiManager.isWifiEnabled()) {
            setWifiEnabled(true);
        }
        // Make sure the scan timestamps are consistent with the device timestamp within the range
        // of WIFI_SCAN_TEST_CACHE_DELAY_MILLIS.
        startScan();
        // Make sure at least one AP is found.
        assertTrue("mScanResult should not be null. This may be due to a scan timeout",
                   mScanResults != null);
        assertFalse("empty scan results!", mScanResults.isEmpty());
        long nowMillis = SystemClock.elapsedRealtime();
        // Keep track of how many APs are fresh in one scan.
        int numFreshAps = 0;
        for (ScanResult result : mScanResults) {
            long scanTimeMillis = TimeUnit.MICROSECONDS.toMillis(result.timestamp);
            if (Math.abs(nowMillis - scanTimeMillis)  < WIFI_SCAN_TEST_CACHE_DELAY_MILLIS) {
                numFreshAps++;
            }
        }
        // At least half of the APs in the scan should be fresh.
        int numTotalAps = mScanResults.size();
        String msg = "Stale AP count: " + (numTotalAps - numFreshAps) + ", fresh AP count: "
                + numFreshAps;
        assertTrue(msg, numFreshAps * 2 >= mScanResults.size());
    }

    public void testConvertBetweenChannelFrequencyMhz() throws Exception {
        int[] testFrequency_2G = {2412, 2437, 2462, 2484};
        int[] testFrequency_5G = {5180, 5220, 5540, 5745};
        int[] testFrequency_6G = {5955, 6435, 6535, 7115};
        int[] testFrequency_60G = {58320, 64800};
        SparseArray<int[]> testData = new SparseArray<>() {{
            put(ScanResult.WIFI_BAND_24_GHZ, testFrequency_2G);
            put(ScanResult.WIFI_BAND_5_GHZ, testFrequency_5G);
            put(ScanResult.WIFI_BAND_6_GHZ, testFrequency_6G);
            put(ScanResult.WIFI_BAND_60_GHZ, testFrequency_60G);
        }};

        for (int i = 0; i < testData.size(); i++) {
            for (int frequency : testData.valueAt(i)) {
                assertEquals(frequency, ScanResult.convertChannelToFrequencyMhzIfSupported(
                      ScanResult.convertFrequencyMhzToChannelIfSupported(frequency), testData.keyAt(i)));
            }
        }
    }

    // Return true if location is enabled.
    private boolean isLocationEnabled() {
        return Settings.Secure.getInt(getContext().getContentResolver(),
                Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_OFF) !=
                Settings.Secure.LOCATION_MODE_OFF;
    }

    // Returns true if the device has location feature.
    private boolean hasLocationFeature() {
        return getContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_LOCATION);
    }

    private boolean hasAutomotiveFeature() {
        return getContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
    }

    public void testSignal() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final int numLevels = 9;
        int expectLevel = 0;
        assertEquals(expectLevel, WifiManager.calculateSignalLevel(MIN_RSSI, numLevels));
        assertEquals(numLevels - 1, WifiManager.calculateSignalLevel(MAX_RSSI, numLevels));
        expectLevel = 4;
        assertEquals(expectLevel, WifiManager.calculateSignalLevel((MIN_RSSI + MAX_RSSI) / 2,
                numLevels));
        int rssiA = 4;
        int rssiB = 5;
        assertTrue(WifiManager.compareSignalLevel(rssiA, rssiB) < 0);
        rssiB = 4;
        assertTrue(WifiManager.compareSignalLevel(rssiA, rssiB) == 0);
        rssiA = 5;
        rssiB = 4;
        assertTrue(WifiManager.compareSignalLevel(rssiA, rssiB) > 0);
    }

    /**
     * Test that {@link WifiManager#calculateSignalLevel(int)} returns a value in the range
     * [0, {@link WifiManager#getMaxSignalLevel()}], and its value is monotonically increasing as
     * the RSSI increases.
     */
    public void testCalculateSignalLevel() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        int maxSignalLevel = mWifiManager.getMaxSignalLevel();

        int prevSignalLevel = 0;
        for (int rssi = -150; rssi <= 50; rssi++) {
            int signalLevel = mWifiManager.calculateSignalLevel(rssi);

            // between [0, maxSignalLevel]
            assertWithMessage("For RSSI=%s", rssi).that(signalLevel).isAtLeast(0);
            assertWithMessage("For RSSI=%s", rssi).that(signalLevel).isAtMost(maxSignalLevel);

            // calculateSignalLevel(rssi) <= calculateSignalLevel(rssi + 1)
            assertWithMessage("For RSSI=%s", rssi).that(signalLevel).isAtLeast(prevSignalLevel);
            prevSignalLevel = signalLevel;
        }
    }

    public class TestWifiVerboseLoggingStatusChangedListener implements
            WifiManager.WifiVerboseLoggingStatusChangedListener {
        public int numCalls;
        public boolean status;

        @Override
        public void onWifiVerboseLoggingStatusChanged(boolean enabled) {
            numCalls++;
            status = enabled;
        }
    }

    public class TestSoftApCallback implements WifiManager.SoftApCallback {
        Object softApLock;
        int currentState;
        int currentFailureReason;
        List<SoftApInfo> apInfoList = new ArrayList<>();
        SoftApInfo apInfoOnSingleApMode;
        Map<SoftApInfo, List<WifiClient>> apInfoClients = new HashMap<>();
        List<WifiClient> currentClientList;
        SoftApCapability currentSoftApCapability;
        MacAddress lastBlockedClientMacAddress;
        int lastBlockedClientReason;
        boolean onStateChangedCalled = false;
        boolean onSoftApCapabilityChangedCalled = false;
        boolean onConnectedClientCalled = false;
        boolean onConnectedClientChangedWithInfoCalled = false;
        boolean onBlockedClientConnectingCalled = false;
        int onSoftapInfoChangedCalledCount = 0;
        int onSoftapInfoChangedWithListCalledCount = 0;

        TestSoftApCallback(Object lock) {
            softApLock = lock;
        }

        public boolean getOnStateChangedCalled() {
            synchronized(softApLock) {
                return onStateChangedCalled;
            }
        }

        public int getOnSoftapInfoChangedCalledCount() {
            synchronized(softApLock) {
                return onSoftapInfoChangedCalledCount;
            }
        }

        public int getOnSoftApInfoChangedWithListCalledCount() {
            synchronized(softApLock) {
                return onSoftapInfoChangedWithListCalledCount;
            }
        }

        public boolean getOnSoftApCapabilityChangedCalled() {
            synchronized(softApLock) {
                return onSoftApCapabilityChangedCalled;
            }
        }

        public boolean getOnConnectedClientChangedWithInfoCalled() {
            synchronized(softApLock) {
                return onConnectedClientChangedWithInfoCalled;
            }
        }

        public boolean getOnConnectedClientCalled() {
            synchronized(softApLock) {
                return onConnectedClientCalled;
            }
        }

        public boolean getOnBlockedClientConnectingCalled() {
            synchronized(softApLock) {
                return onBlockedClientConnectingCalled;
            }
        }

        public int getCurrentState() {
            synchronized(softApLock) {
                return currentState;
            }
        }

        public int getCurrentStateFailureReason() {
            synchronized(softApLock) {
                return currentFailureReason;
            }
        }

        public List<WifiClient> getCurrentClientList() {
            synchronized(softApLock) {
                return new ArrayList<>(currentClientList);
            }
        }

        public SoftApInfo getCurrentSoftApInfo() {
            synchronized(softApLock) {
                return apInfoOnSingleApMode;
            }
        }

        public List<SoftApInfo> getCurrentSoftApInfoList() {
            synchronized(softApLock) {
                return new ArrayList<>(apInfoList);
            }
        }

        public SoftApCapability getCurrentSoftApCapability() {
            synchronized(softApLock) {
                return currentSoftApCapability;
            }
        }

        public MacAddress getLastBlockedClientMacAddress() {
            synchronized(softApLock) {
                return lastBlockedClientMacAddress;
            }
        }

        public int getLastBlockedClientReason() {
            synchronized(softApLock) {
                return lastBlockedClientReason;
            }
        }

        @Override
        public void onStateChanged(int state, int failureReason) {
            synchronized(softApLock) {
                currentState = state;
                currentFailureReason = failureReason;
                onStateChangedCalled = true;
            }
        }

        @Override
        public void onConnectedClientsChanged(List<WifiClient> clients) {
            synchronized(softApLock) {
                currentClientList = new ArrayList<>(clients);
                onConnectedClientCalled = true;
            }
        }

        @Override
        public void onConnectedClientsChanged(SoftApInfo info, List<WifiClient> clients) {
            synchronized(softApLock) {
                apInfoClients.put(info, clients);
                onConnectedClientChangedWithInfoCalled = true;
            }
        }

        @Override
        public void onInfoChanged(List<SoftApInfo> infoList) {
            synchronized(softApLock) {
                apInfoList = new ArrayList<>(infoList);
                onSoftapInfoChangedWithListCalledCount++;
            }
        }

        @Override
        public void onInfoChanged(SoftApInfo softApInfo) {
            synchronized(softApLock) {
                apInfoOnSingleApMode = softApInfo;
                onSoftapInfoChangedCalledCount++;
            }
        }

        @Override
        public void onCapabilityChanged(SoftApCapability softApCapability) {
            synchronized(softApLock) {
                currentSoftApCapability = softApCapability;
                onSoftApCapabilityChangedCalled = true;
            }
        }

        @Override
        public void onBlockedClientConnecting(WifiClient client, int blockedReason) {
            synchronized(softApLock) {
                lastBlockedClientMacAddress = client.getMacAddress();
                lastBlockedClientReason = blockedReason;
                onBlockedClientConnectingCalled = true;
            }
        }
    }

    private static class TestLocalOnlyHotspotCallback extends WifiManager.LocalOnlyHotspotCallback {
        Object hotspotLock;
        WifiManager.LocalOnlyHotspotReservation reservation = null;
        boolean onStartedCalled = false;
        boolean onStoppedCalled = false;
        boolean onFailedCalled = false;
        int failureReason = -1;

        TestLocalOnlyHotspotCallback(Object lock) {
            hotspotLock = lock;
        }

        @Override
        public void onStarted(WifiManager.LocalOnlyHotspotReservation r) {
            synchronized (hotspotLock) {
                reservation = r;
                onStartedCalled = true;
                hotspotLock.notify();
            }
        }

        @Override
        public void onStopped() {
            synchronized (hotspotLock) {
                onStoppedCalled = true;
                hotspotLock.notify();
            }
        }

        @Override
        public void onFailed(int reason) {
            synchronized (hotspotLock) {
                onFailedCalled = true;
                failureReason = reason;
                hotspotLock.notify();
            }
        }
    }

    private List<Integer> getSupportedSoftApBand(SoftApCapability capability) {
        List<Integer> supportedApBands = new ArrayList<>();
        if (mWifiManager.is24GHzBandSupported() &&
                capability.areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_BAND_24G_SUPPORTED)) {
            supportedApBands.add(SoftApConfiguration.BAND_2GHZ);
        }
        if (mWifiManager.is5GHzBandSupported() &&
                capability.areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_BAND_5G_SUPPORTED)) {
            supportedApBands.add(SoftApConfiguration.BAND_5GHZ);
        }
        if (mWifiManager.is6GHzBandSupported() &&
                capability.areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_BAND_6G_SUPPORTED)) {
            supportedApBands.add(SoftApConfiguration.BAND_6GHZ);
        }
        if (mWifiManager.is60GHzBandSupported() &&
                capability.areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_BAND_60G_SUPPORTED)) {
            supportedApBands.add(SoftApConfiguration.BAND_60GHZ);
        }
        return supportedApBands;
    }

    private TestLocalOnlyHotspotCallback startLocalOnlyHotspot() {
        // Location mode must be enabled for this test
        if (!isLocationEnabled()) {
            fail("Please enable location for this test");
        }

        TestExecutor executor = new TestExecutor();
        TestSoftApCallback capabilityCallback = new TestSoftApCallback(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        List<Integer> supportedSoftApBands = new ArrayList<>();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            verifyRegisterSoftApCallback(executor, capabilityCallback);
            supportedSoftApBands = getSupportedSoftApBand(
                    capabilityCallback.getCurrentSoftApCapability());
        } catch (Exception ex) {
        } finally {
            // clean up
            mWifiManager.unregisterSoftApCallback(capabilityCallback);
            uiAutomation.dropShellPermissionIdentity();
        }
        TestLocalOnlyHotspotCallback callback = new TestLocalOnlyHotspotCallback(mLock);
        synchronized (mLock) {
            try {
                mWifiManager.startLocalOnlyHotspot(callback, null);
                // now wait for callback
                mLock.wait(TEST_WAIT_DURATION_MS);
            } catch (InterruptedException e) {
            }
            // check if we got the callback
            assertTrue(callback.onStartedCalled);

            SoftApConfiguration softApConfig = callback.reservation.getSoftApConfiguration();
            assertNotNull(softApConfig);
            int securityType = softApConfig.getSecurityType();
            if (securityType == SoftApConfiguration.SECURITY_TYPE_OPEN
                    || securityType == SoftApConfiguration.SECURITY_TYPE_WPA2_PSK
                    || securityType == SoftApConfiguration.SECURITY_TYPE_WPA3_SAE_TRANSITION) {
                assertNotNull(softApConfig.toWifiConfiguration());
            } else {
                assertNull(softApConfig.toWifiConfiguration());
            }
            if (!hasAutomotiveFeature()) {
                assertEquals(supportedSoftApBands.size() > 0 ? supportedSoftApBands.get(0)
                        : SoftApConfiguration.BAND_2GHZ,
                        callback.reservation.getSoftApConfiguration().getBand());
            }
            assertFalse(callback.onFailedCalled);
            assertFalse(callback.onStoppedCalled);
        }
        return callback;
    }

    private void stopLocalOnlyHotspot(TestLocalOnlyHotspotCallback callback, boolean wifiEnabled) {
       synchronized (mMySync) {
           // we are expecting a new state
           mMySync.expectedState = STATE_WIFI_CHANGING;

           // now shut down LocalOnlyHotspot
           callback.reservation.close();

           try {
               waitForExpectedWifiState(wifiEnabled);
           } catch (InterruptedException e) {}
        }
    }

    /**
     * Verify that calls to startLocalOnlyHotspot succeed with proper permissions.
     *
     * Note: Location mode must be enabled for this test.
     */
    public void testStartLocalOnlyHotspotSuccess() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }

        boolean wifiEnabled = mWifiManager.isWifiEnabled();

        TestLocalOnlyHotspotCallback callback = startLocalOnlyHotspot();

        // add sleep to avoid calling stopLocalOnlyHotspot before TetherController initialization.
        // TODO: remove this sleep as soon as b/124330089 is fixed.
        Log.d(TAG, "Sleeping for 2 seconds");
        Thread.sleep(2000);

        stopLocalOnlyHotspot(callback, wifiEnabled);

        // wifi should either stay on, or come back on
        assertEquals(wifiEnabled, mWifiManager.isWifiEnabled());
    }

    /**
     * Verify calls to deprecated API's all fail for non-settings apps targeting >= Q SDK.
     */
    public void testDeprecatedApis() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        setWifiEnabled(true);
        waitForConnection(); // ensures that there is at-least 1 saved network on the device.

        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.SSID = SSID1;
        wifiConfiguration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);

        assertEquals(INVALID_NETWORK_ID,
                mWifiManager.addNetwork(wifiConfiguration));
        assertEquals(INVALID_NETWORK_ID,
                mWifiManager.updateNetwork(wifiConfiguration));
        assertFalse(mWifiManager.enableNetwork(0, true));
        assertFalse(mWifiManager.disableNetwork(0));
        assertFalse(mWifiManager.removeNetwork(0));
        assertFalse(mWifiManager.disconnect());
        assertFalse(mWifiManager.reconnect());
        assertFalse(mWifiManager.reassociate());
        assertTrue(mWifiManager.getConfiguredNetworks().isEmpty());

        boolean wifiEnabled = mWifiManager.isWifiEnabled();
        // now we should fail to toggle wifi state.
        assertFalse(mWifiManager.setWifiEnabled(!wifiEnabled));
        Thread.sleep(TEST_WAIT_DURATION_MS);
        assertEquals(wifiEnabled, mWifiManager.isWifiEnabled());
    }

    /**
     * Test the WifiManager APIs that return whether a feature is supported.
     */
    public void testGetSupportedFeatures() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        mWifiManager.isMakeBeforeBreakWifiSwitchingSupported();
        mWifiManager.isStaBridgedApConcurrencySupported();
    }

    /**
     * Verify non DO apps cannot call removeNonCallerConfiguredNetworks.
     */
    public void testRemoveNonCallerConfiguredNetworksNotAllowed() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        try {
            mWifiManager.removeNonCallerConfiguredNetworks();
            fail("Expected security exception for non DO app");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test coverage for the constructor of AddNetworkResult.
     */
    public void testAddNetworkResultCreation() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        int statusCode = WifiManager.AddNetworkResult.STATUS_NO_PERMISSION;
        int networkId = 5;
        WifiManager.AddNetworkResult result = new WifiManager.AddNetworkResult(
            statusCode, networkId);
        assertEquals("statusCode should match", statusCode, result.statusCode);
        assertEquals("networkId should match", networkId, result.networkId);
    }

    /**
     * Verify that {@link WifiManager#addNetworkPrivileged(WifiConfiguration)} throws a
     * SecurityException when called by a normal app.
     */
    public void testAddNetworkPrivilegedNotAllowedForNormalApps() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        try {
            WifiConfiguration newOpenNetwork = new WifiConfiguration();
            newOpenNetwork.SSID = "\"" + TEST_SSID_UNQUOTED + "\"";
            mWifiManager.addNetworkPrivileged(newOpenNetwork);
            fail("A normal app should not be able to call this API.");
        } catch (SecurityException e) {
        }
    }

    /**
     * Verify {@link WifiManager#addNetworkPrivileged(WifiConfiguration)} throws an exception when
     * null is the input.
     */
    public void testAddNetworkPrivilegedBadInput() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.addNetworkPrivileged(null);
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Verify {@link WifiManager#addNetworkPrivileged(WifiConfiguration)} returns the proper
     * failure status code when adding an enterprise config with mandatory fields not filled in.
     */
    public void testAddNetworkPrivilegedFailureBadEnterpriseConfig() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            WifiConfiguration wifiConfiguration = new WifiConfiguration();
            wifiConfiguration.SSID = SSID1;
            wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE);
            wifiConfiguration.enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.TTLS);
            WifiManager.AddNetworkResult result =
                    mWifiManager.addNetworkPrivileged(wifiConfiguration);
            assertEquals(WifiManager.AddNetworkResult.STATUS_INVALID_CONFIGURATION_ENTERPRISE,
                    result.statusCode);
            assertEquals(-1, result.networkId);
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Verify {@link WifiManager#addNetworkPrivileged(WifiConfiguration)} works properly when the
     * calling app has permissions.
     */
    public void testAddNetworkPrivilegedSuccess() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        WifiManager.AddNetworkResult result = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            WifiConfiguration newOpenNetwork = new WifiConfiguration();
            newOpenNetwork.SSID = "\"" + TEST_SSID_UNQUOTED + "\"";
            result = mWifiManager.addNetworkPrivileged(newOpenNetwork);
            assertEquals(WifiManager.AddNetworkResult.STATUS_SUCCESS, result.statusCode);
            assertTrue(result.networkId >= 0);
            List<WifiConfiguration> configuredNetworks = mWifiManager.getConfiguredNetworks();
            boolean found = false;
            for (WifiConfiguration config : configuredNetworks) {
                if (config.networkId == result.networkId
                        && config.SSID.equals(newOpenNetwork.SSID)) {
                    found = true;
                    break;
                }
            }
            assertTrue("addNetworkPrivileged returns success"
                    + "but the network is not found in getConfiguredNetworks", found);

            List<WifiConfiguration> privilegedConfiguredNetworks =
                    mWifiManager.getPrivilegedConfiguredNetworks();
            found = false;
            for (WifiConfiguration config : privilegedConfiguredNetworks) {
                if (config.networkId == result.networkId
                        && config.SSID.equals(newOpenNetwork.SSID)) {
                    found = true;
                    break;
                }
            }
            assertTrue("addNetworkPrivileged returns success"
                    + "but the network is not found in getPrivilegedConfiguredNetworks", found);

            List<WifiConfiguration> callerConfiguredNetworks =
                    mWifiManager.getCallerConfiguredNetworks();
            found = false;
            for (WifiConfiguration config : callerConfiguredNetworks) {
                if (config.networkId == result.networkId
                        && config.SSID.equals(newOpenNetwork.SSID)) {
                    found = true;
                    break;
                }
            }
            assertTrue("addNetworkPrivileged returns success"
                    + "but the network is not found in getCallerConfiguredNetworks", found);
        } finally {
            if (null != result) {
                mWifiManager.removeNetwork(result.networkId);
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    private WifiConfiguration createConfig(
            String ssid, int type) {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = "\"" + ssid + "\"";
        config.setSecurityParams(type);
        // set necessary fields for different types.
        switch (type) {
            case WifiConfiguration.SECURITY_TYPE_OPEN:
            case WifiConfiguration.SECURITY_TYPE_OWE:
                break;
            case WifiConfiguration.SECURITY_TYPE_PSK:
            case WifiConfiguration.SECURITY_TYPE_SAE:
                config.preSharedKey = "\"1qaz@WSX\"";
                break;
            case WifiConfiguration.SECURITY_TYPE_EAP:
            case WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE:
            case WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE_192_BIT:
                config.enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.SIM);
                break;
        }
        return config;
    }

    private void assertConfigsAreFound(
            List<WifiConfiguration> expectedConfigs,
            List<WifiConfiguration> configs) {
        for (WifiConfiguration expectedConfig: expectedConfigs) {
            boolean found = false;
            for (WifiConfiguration config : configs) {
                if (config.networkId == expectedConfig.networkId
                        && config.getKey().equals(expectedConfig.getKey())) {
                    found = true;
                    break;
                }
            }
            assertTrue("the network " + expectedConfig.getKey() + " is not found", found);
        }
    }

    /**
     * Verify {@link WifiManager#addNetworkPrivileged(WifiConfiguration)} works
     * with merging types properly when the calling app has permissions.
     */
    public void testAddNetworkPrivilegedMergingTypeSuccess() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        List<WifiConfiguration> testConfigs = new ArrayList<>();
        testConfigs.add(createConfig("test-open-owe-jdur", WifiConfiguration.SECURITY_TYPE_OPEN));
        testConfigs.add(createConfig("test-open-owe-jdur", WifiConfiguration.SECURITY_TYPE_OWE));
        testConfigs.add(createConfig("test-psk-sae-ijfe", WifiConfiguration.SECURITY_TYPE_PSK));
        testConfigs.add(createConfig("test-psk-sae-ijfe", WifiConfiguration.SECURITY_TYPE_SAE));
        testConfigs.add(createConfig("test-wpa2e-wpa3e-plki",
                WifiConfiguration.SECURITY_TYPE_EAP));
        testConfigs.add(createConfig("test-wpa2e-wpa3e-plki",
                WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE));
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            final int originalConfiguredNetworksNumber = mWifiManager.getConfiguredNetworks().size();
            final int originalPrivilegedConfiguredNetworksNumber =
                    mWifiManager.getPrivilegedConfiguredNetworks().size();
            final int originalCallerConfiguredNetworksNumber =
                mWifiManager.getCallerConfiguredNetworks().size();
            for (WifiConfiguration c: testConfigs) {
                WifiManager.AddNetworkResult result = mWifiManager.addNetworkPrivileged(c);
                assertEquals(WifiManager.AddNetworkResult.STATUS_SUCCESS, result.statusCode);
                assertTrue(result.networkId >= 0);
                c.networkId = result.networkId;
            }
            List<WifiConfiguration> expectedConfigs = testConfigs;
            if (SdkLevel.isAtLeastS()) {
                // open/owe, psk/sae, and wpa2e/wpa3e should be merged
                // so they should have the same network ID.
                assertEquals(testConfigs.get(0).networkId, testConfigs.get(1).networkId);
                assertEquals(testConfigs.get(2).networkId, testConfigs.get(3).networkId);
                assertEquals(testConfigs.get(4).networkId, testConfigs.get(5).networkId);
            } else {
                // Network IDs for different security types should be unique for R
                assertNotEquals(testConfigs.get(0).networkId, testConfigs.get(1).networkId);
                assertNotEquals(testConfigs.get(2).networkId, testConfigs.get(3).networkId);
                assertNotEquals(testConfigs.get(4).networkId, testConfigs.get(5).networkId);
                // WPA3-Enterprise is omitted when WPA2-Enterprise is present for R
                expectedConfigs = testConfigs.subList(0, 5);
            }
            List<WifiConfiguration> configuredNetworks = mWifiManager.getConfiguredNetworks();
            assertEquals(originalConfiguredNetworksNumber + expectedConfigs.size(),
                    configuredNetworks.size());
            assertConfigsAreFound(expectedConfigs, configuredNetworks);

            List<WifiConfiguration> privilegedConfiguredNetworks =
                    mWifiManager.getPrivilegedConfiguredNetworks();
            assertEquals(originalPrivilegedConfiguredNetworksNumber + expectedConfigs.size(),
                    privilegedConfiguredNetworks.size());
            assertConfigsAreFound(expectedConfigs, privilegedConfiguredNetworks);

            List<WifiConfiguration> callerConfiguredNetworks =
                    mWifiManager.getCallerConfiguredNetworks();
            assertEquals(originalCallerConfiguredNetworksNumber + expectedConfigs.size(),
                    callerConfiguredNetworks.size());
            assertConfigsAreFound(expectedConfigs, callerConfiguredNetworks);

        } finally {
            for (WifiConfiguration c: testConfigs) {
                if (c.networkId >= 0) {
                    mWifiManager.removeNetwork(c.networkId);
                }
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Verify that applications can only have one registered LocalOnlyHotspot request at a time.
     *
     * Note: Location mode must be enabled for this test.
     */
    public void testStartLocalOnlyHotspotSingleRequestByApps() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }

        boolean caughtException = false;

        boolean wifiEnabled = mWifiManager.isWifiEnabled();

        TestLocalOnlyHotspotCallback callback = startLocalOnlyHotspot();

        // now make a second request - this should fail.
        TestLocalOnlyHotspotCallback callback2 = new TestLocalOnlyHotspotCallback(mLock);
        try {
            mWifiManager.startLocalOnlyHotspot(callback2, null);
        } catch (IllegalStateException e) {
            Log.d(TAG, "Caught the IllegalStateException we expected: called startLOHS twice");
            caughtException = true;
        }
        if (!caughtException) {
            // second start did not fail, should clean up the hotspot.

            // add sleep to avoid calling stopLocalOnlyHotspot before TetherController initialization.
            // TODO: remove this sleep as soon as b/124330089 is fixed.
            Log.d(TAG, "Sleeping for 2 seconds");
            Thread.sleep(2000);

            stopLocalOnlyHotspot(callback2, wifiEnabled);
        }
        assertTrue(caughtException);

        // add sleep to avoid calling stopLocalOnlyHotspot before TetherController initialization.
        // TODO: remove this sleep as soon as b/124330089 is fixed.
        Log.d(TAG, "Sleeping for 2 seconds");
        Thread.sleep(2000);

        stopLocalOnlyHotspot(callback, wifiEnabled);
    }

    private static class TestExecutor implements Executor {
        private ConcurrentLinkedQueue<Runnable> tasks = new ConcurrentLinkedQueue<>();

        @Override
        public void execute(Runnable task) {
            tasks.add(task);
        }

        private void runAll() {
            Runnable task = tasks.poll();
            while (task != null) {
                task.run();
                task = tasks.poll();
            }
        }
    }

    public void testStartLocalOnlyHotspotWithConfigBssid() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }

        TestExecutor executor = new TestExecutor();
        TestLocalOnlyHotspotCallback callback = new TestLocalOnlyHotspotCallback(mLock);
        TestSoftApCallback capabilityCallback = new TestSoftApCallback(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        boolean wifiEnabled = mWifiManager.isWifiEnabled();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            verifyRegisterSoftApCallback(executor, capabilityCallback);
            SoftApConfiguration.Builder customConfigBuilder = new SoftApConfiguration.Builder()
                    .setSsid(TEST_SSID_UNQUOTED)
                    .setPassphrase(TEST_PASSPHRASE, SoftApConfiguration.SECURITY_TYPE_WPA2_PSK);

            boolean isSupportCustomizedMac = capabilityCallback.getCurrentSoftApCapability()
                        .areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION)
                    && PropertyUtil.isVndkApiLevelNewerThan(Build.VERSION_CODES.S);
            if (isSupportCustomizedMac) {
                customConfigBuilder.setBssid(TEST_MAC);
            }
            SoftApConfiguration customConfig = customConfigBuilder.build();

            mWifiManager.startLocalOnlyHotspot(customConfig, executor, callback);
            // now wait for callback
            Thread.sleep(TEST_WAIT_DURATION_MS);

            // Verify callback is run on the supplied executor
            assertFalse(callback.onStartedCalled);
            executor.runAll();
            assertTrue(callback.onStartedCalled);

            assertNotNull(callback.reservation);
            SoftApConfiguration softApConfig = callback.reservation.getSoftApConfiguration();
            assertNotNull(softApConfig);
            if (isSupportCustomizedMac) {
                assertEquals(TEST_MAC, softApConfig.getBssid());
            }
            assertEquals(TEST_SSID_UNQUOTED, softApConfig.getSsid());
            assertEquals(TEST_PASSPHRASE, softApConfig.getPassphrase());
        } finally {
            // clean up
            stopLocalOnlyHotspot(callback, wifiEnabled);
            mWifiManager.unregisterSoftApCallback(capabilityCallback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    public void testStartLocalOnlyHotspotWithNullBssidConfig() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }
        SoftApConfiguration customConfig = new SoftApConfiguration.Builder()
                .setSsid(TEST_SSID_UNQUOTED)
                .setPassphrase(TEST_PASSPHRASE, SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                .build();
        TestExecutor executor = new TestExecutor();
        TestLocalOnlyHotspotCallback callback = new TestLocalOnlyHotspotCallback(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        boolean wifiEnabled = mWifiManager.isWifiEnabled();
        try {
            uiAutomation.adoptShellPermissionIdentity();

            mWifiManager.startLocalOnlyHotspot(customConfig, executor, callback);
            // now wait for callback
            Thread.sleep(TEST_WAIT_DURATION_MS);

            // Verify callback is run on the supplied executor
            assertFalse(callback.onStartedCalled);
            executor.runAll();
            assertTrue(callback.onStartedCalled);

            assertNotNull(callback.reservation);
            SoftApConfiguration softApConfig = callback.reservation.getSoftApConfiguration();
            assertNotNull(softApConfig);
            assertEquals(TEST_SSID_UNQUOTED, softApConfig.getSsid());
            assertEquals(TEST_PASSPHRASE, softApConfig.getPassphrase());
        } finally {
            // clean up
            stopLocalOnlyHotspot(callback, wifiEnabled);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Read the content of the given resource file into a String.
     *
     * @param filename String name of the file
     * @return String
     * @throws IOException
     */
    private String loadResourceFile(String filename) throws IOException {
        InputStream in = getClass().getClassLoader().getResourceAsStream(filename);
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        StringBuilder builder = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) {
            builder.append(line).append("\n");
        }
        return builder.toString();
    }

    /**
     * Verify that changing the mac randomization setting of a Passpoint configuration.
     */
    public void testMacRandomizationSettingPasspoint() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        String configStr = loadResourceFile(PASSPOINT_INSTALLATION_FILE_WITH_CA_CERT);
        PasspointConfiguration config =
                ConfigParser.parsePasspointConfig(TYPE_WIFI_CONFIG, configStr.getBytes());
        String fqdn = config.getHomeSp().getFqdn();
        String uniqueId = config.getUniqueId();
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();

            mWifiManager.addOrUpdatePasspointConfiguration(config);
            PasspointConfiguration passpointConfig = getTargetPasspointConfiguration(
                    mWifiManager.getPasspointConfigurations(), uniqueId);
            assertNotNull("The installed passpoint profile is missing", passpointConfig);
            assertTrue("Mac randomization should be enabled for passpoint networks by default.",
                    passpointConfig.isMacRandomizationEnabled());

            mWifiManager.setMacRandomizationSettingPasspointEnabled(fqdn, false);
            passpointConfig = getTargetPasspointConfiguration(
                    mWifiManager.getPasspointConfigurations(), uniqueId);
            assertNotNull("The installed passpoint profile is missing", passpointConfig);
            assertFalse("Mac randomization should be disabled by the API call.",
                    passpointConfig.isMacRandomizationEnabled());
        } finally {
            // Clean up
            mWifiManager.removePasspointConfiguration(fqdn);
            uiAutomation.dropShellPermissionIdentity();
        }
    }
    /**
     * Verify that the {@link android.Manifest.permission#NETWORK_STACK} permission is never held by
     * any package.
     * <p>
     * No apps should <em>ever</em> attempt to acquire this permission, since it would give those
     * apps extremely broad access to connectivity functionality.
     */
    public void testNetworkStackPermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final PackageManager pm = getContext().getPackageManager();

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.NETWORK_STACK
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);
        for (PackageInfo pi : holding) {
            fail("The NETWORK_STACK permission must not be held by " + pi.packageName
                    + " and must be revoked for security reasons");
        }
    }

    /**
     * Verify that the {@link android.Manifest.permission#NETWORK_SETTINGS} permission is
     * never held by any package.
     * <p>
     * Only Settings, SysUi, NetworkStack and shell apps should <em>ever</em> attempt to acquire
     * this permission, since it would give those apps extremely broad access to connectivity
     * functionality.  The permission is intended to be granted to only those apps with direct user
     * access and no others.
     */
    public void testNetworkSettingsPermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final PackageManager pm = getContext().getPackageManager();

        final ArraySet<String> allowedPackages = new ArraySet();
        final ArraySet<Integer> allowedUIDs = new ArraySet();
        // explicitly add allowed UIDs
        allowedUIDs.add(Process.SYSTEM_UID);
        allowedUIDs.add(Process.SHELL_UID);
        allowedUIDs.add(Process.PHONE_UID);
        allowedUIDs.add(Process.NETWORK_STACK_UID);
        allowedUIDs.add(Process.NFC_UID);

        // only quick settings is allowed to bind to the BIND_QUICK_SETTINGS_TILE permission, using
        // this fact to determined allowed package name for sysui. This is a signature permission,
        // so allow any package with this permission.
        final List<PackageInfo> sysuiPackages = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.BIND_QUICK_SETTINGS_TILE
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);
        for (PackageInfo info : sysuiPackages) {
            allowedPackages.add(info.packageName);
        }

        // the captive portal flow also currently holds the NETWORK_SETTINGS permission
        final Intent intent = new Intent(ConnectivityManager.ACTION_CAPTIVE_PORTAL_SIGN_IN);
        final ResolveInfo ri = pm.resolveActivity(intent, PackageManager.MATCH_DISABLED_COMPONENTS);
        if (ri != null) {
            allowedPackages.add(ri.activityInfo.packageName);
        }

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.NETWORK_SETTINGS
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);
        StringBuilder stringBuilder = new StringBuilder();
        for (PackageInfo pi : holding) {
            String packageName = pi.packageName;

            // this is an explicitly allowed package
            if (allowedPackages.contains(packageName)) continue;

            // now check if the packages are from allowed UIDs
            int uid = -1;
            try {
                uid = pm.getPackageUidAsUser(packageName, UserHandle.USER_SYSTEM);
            } catch (PackageManager.NameNotFoundException e) {
                continue;
            }
            if (!allowedUIDs.contains(uid)) {
                stringBuilder.append("The NETWORK_SETTINGS permission must not be held by "
                    + packageName + ":" + uid + " and must be revoked for security reasons\n");
            }
        }
        if (stringBuilder.length() > 0) {
            fail(stringBuilder.toString());
        }
    }

    /**
     * Verify that the {@link android.Manifest.permission#NETWORK_SETUP_WIZARD} permission is
     * only held by the device setup wizard application.
     * <p>
     * Only the SetupWizard app should <em>ever</em> attempt to acquire this
     * permission, since it would give those apps extremely broad access to connectivity
     * functionality.  The permission is intended to be granted to only the device setup wizard.
     */
    public void testNetworkSetupWizardPermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final ArraySet<String> allowedPackages = new ArraySet();

        final PackageManager pm = getContext().getPackageManager();

        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_SETUP_WIZARD);
        final ResolveInfo ri = pm.resolveActivity(intent, PackageManager.MATCH_DISABLED_COMPONENTS);
        String validPkg = "";
        if (ri != null) {
            allowedPackages.add(ri.activityInfo.packageName);
            validPkg = ri.activityInfo.packageName;
        }

        final Intent preIntent = new Intent("com.android.setupwizard.OEM_PRE_SETUP");
        preIntent.addCategory(Intent.CATEGORY_DEFAULT);
        final ResolveInfo preRi = pm
            .resolveActivity(preIntent, PackageManager.MATCH_DISABLED_COMPONENTS);
        String prePackageName = "";
        if (null != preRi) {
            prePackageName = preRi.activityInfo.packageName;
        }

        final Intent postIntent = new Intent("com.android.setupwizard.OEM_POST_SETUP");
        postIntent.addCategory(Intent.CATEGORY_DEFAULT);
        final ResolveInfo postRi = pm
            .resolveActivity(postIntent, PackageManager.MATCH_DISABLED_COMPONENTS);
        String postPackageName = "";
        if (null != postRi) {
            postPackageName = postRi.activityInfo.packageName;
        }
        if (!TextUtils.isEmpty(prePackageName) && !TextUtils.isEmpty(postPackageName)
            && prePackageName.equals(postPackageName)) {
            allowedPackages.add(prePackageName);
        }

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[]{
            android.Manifest.permission.NETWORK_SETUP_WIZARD
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);
        for (PackageInfo pi : holding) {
            if (!allowedPackages.contains(pi.packageName)) {
                fail("The NETWORK_SETUP_WIZARD permission must not be held by " + pi.packageName
                    + " and must be revoked for security reasons [" + validPkg + "]");
            }
        }
    }

    /**
     * Verify that the {@link android.Manifest.permission#NETWORK_MANAGED_PROVISIONING} permission
     * is only held by the device managed provisioning application.
     * <p>
     * Only the ManagedProvisioning app should <em>ever</em> attempt to acquire this
     * permission, since it would give those apps extremely broad access to connectivity
     * functionality.  The permission is intended to be granted to only the device managed
     * provisioning.
     */
    public void testNetworkManagedProvisioningPermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final PackageManager pm = getContext().getPackageManager();

        // TODO(b/115980767): Using hardcoded package name. Need a better mechanism to find the
        // managed provisioning app.
        // Ensure that the package exists.
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setPackage(MANAGED_PROVISIONING_PACKAGE_NAME);
        final ResolveInfo ri = pm.resolveActivity(intent, PackageManager.MATCH_DISABLED_COMPONENTS);
        String validPkg = "";
        if (ri != null) {
            validPkg = ri.activityInfo.packageName;
        }

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.NETWORK_MANAGED_PROVISIONING
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);
        for (PackageInfo pi : holding) {
            if (!Objects.equals(pi.packageName, validPkg)) {
                fail("The NETWORK_MANAGED_PROVISIONING permission must not be held by "
                        + pi.packageName + " and must be revoked for security reasons ["
                        + validPkg +"]");
            }
        }
    }

    /**
     * Verify that the {@link android.Manifest.permission#WIFI_SET_DEVICE_MOBILITY_STATE} permission
     * is held by at most one application.
     */
    public void testWifiSetDeviceMobilityStatePermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final PackageManager pm = getContext().getPackageManager();

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.WIFI_SET_DEVICE_MOBILITY_STATE
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);

        List<String> uniquePackageNames = holding
                .stream()
                .map(pi -> pi.packageName)
                .distinct()
                .collect(Collectors.toList());

        if (uniquePackageNames.size() > 1) {
            fail("The WIFI_SET_DEVICE_MOBILITY_STATE permission must not be held by more than one "
                    + "application, but is held by " + uniquePackageNames.size() + " applications: "
                    + String.join(", ", uniquePackageNames));
        }
    }

    /**
     * Verify that the {@link android.Manifest.permission#NETWORK_CARRIER_PROVISIONING} permission
     * is held by at most one application.
     */
    public void testNetworkCarrierProvisioningPermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final PackageManager pm = getContext().getPackageManager();

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.NETWORK_CARRIER_PROVISIONING
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);

        List<String> uniquePackageNames = holding
                .stream()
                .map(pi -> pi.packageName)
                .distinct()
                .collect(Collectors.toList());

        if (uniquePackageNames.size() > 2) {
            fail("The NETWORK_CARRIER_PROVISIONING permission must not be held by more than two "
                    + "applications, but is held by " + uniquePackageNames.size() + " applications: "
                    + String.join(", ", uniquePackageNames));
        }
    }

    /**
     * Verify that the {@link android.Manifest.permission#WIFI_UPDATE_USABILITY_STATS_SCORE}
     * permission is held by at most one application.
     */
    public void testUpdateWifiUsabilityStatsScorePermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        final PackageManager pm = getContext().getPackageManager();

        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[] {
                android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE
        }, PackageManager.MATCH_UNINSTALLED_PACKAGES);

        Set<String> uniqueNonSystemPackageNames = new HashSet<>();
        for (PackageInfo pi : holding) {
            String packageName = pi.packageName;
            // Shell is allowed to hold this permission for testing.
            int uid = -1;
            try {
                uid = pm.getPackageUidAsUser(packageName, UserHandle.USER_SYSTEM);
            } catch (PackageManager.NameNotFoundException e) {
                continue;
            }
            if (uid == Process.SHELL_UID) continue;

            uniqueNonSystemPackageNames.add(packageName);
        }

        if (uniqueNonSystemPackageNames.size() > 1) {
            fail("The WIFI_UPDATE_USABILITY_STATS_SCORE permission must not be held by more than "
                + "one application, but is held by " + uniqueNonSystemPackageNames.size()
                + " applications: " + String.join(", ", uniqueNonSystemPackageNames));
        }
    }

    private void turnScreenOnNoDelay() throws Exception {
        mUiDevice.executeShellCommand("input keyevent KEYCODE_WAKEUP");
        mUiDevice.executeShellCommand("wm dismiss-keyguard");
    }

    private void turnScreenOn() throws Exception {
        turnScreenOnNoDelay();
        // Since the screen on/off intent is ordered, they will not be sent right now.
        Thread.sleep(DURATION_SCREEN_TOGGLE);
    }

    private void turnScreenOffNoDelay() throws Exception {
        mUiDevice.executeShellCommand("input keyevent KEYCODE_SLEEP");
    }

    private void turnScreenOff() throws Exception {
        turnScreenOffNoDelay();
        // Since the screen on/off intent is ordered, they will not be sent right now.
        Thread.sleep(DURATION_SCREEN_TOGGLE);
    }

    private void assertWifiScanningIsOn() {
        if (!mWifiManager.isScanAlwaysAvailable()) {
            fail("Wi-Fi scanning should be on.");
        }
    }

    private void runWithScanningEnabled(ThrowingRunnable r) throws Exception {
        boolean wasScanEnabledForTest = false;
        if (!mWifiManager.isScanAlwaysAvailable()) {
            ShellIdentityUtils.invokeWithShellPermissions(
                    () -> mWifiManager.setScanAlwaysAvailable(true));
            wasScanEnabledForTest = true;
        }
        try {
            r.run();
        } finally {
            if (wasScanEnabledForTest) {
                ShellIdentityUtils.invokeWithShellPermissions(
                        () -> mWifiManager.setScanAlwaysAvailable(false));
            }
        }
    }

    /**
     * Verify that Wi-Fi scanning is not turned off when the screen turns off while wifi is disabled
     * but location is on.
     * @throws Exception
     */
    public void testScreenOffDoesNotTurnOffWifiScanningWhenWifiDisabled() throws Exception {
        if (FeatureUtil.isTV() || FeatureUtil.isAutomotive()) {
            // TV and auto do not support the setting options of WIFI scanning and Bluetooth
            // scanning
            return;
        }
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!hasLocationFeature()) {
            // skip the test if location is not supported
            return;
        }
        if (!isLocationEnabled()) {
            fail("Please enable location for this test - since Marshmallow WiFi scan results are"
                    + " empty when location is disabled!");
        }
        runWithScanningEnabled(() -> {
            setWifiEnabled(false);
            turnScreenOn();
            assertWifiScanningIsOn();
            // Toggle screen and verify Wi-Fi scanning is still on.
            turnScreenOff();
            assertWifiScanningIsOn();
            turnScreenOn();
            assertWifiScanningIsOn();
        });
    }

    /**
     * Verify that Wi-Fi scanning is not turned off when the screen turns off while wifi is enabled.
     * @throws Exception
     */
    public void testScreenOffDoesNotTurnOffWifiScanningWhenWifiEnabled() throws Exception {
        if (FeatureUtil.isTV() || FeatureUtil.isAutomotive()) {
            // TV and auto do not support the setting options of WIFI scanning and Bluetooth
            // scanning
            return;
        }
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!hasLocationFeature()) {
            // skip the test if location is not supported
            return;
        }
        if (!isLocationEnabled()) {
            fail("Please enable location for this test - since Marshmallow WiFi scan results are"
                    + " empty when location is disabled!");
        }
        runWithScanningEnabled(() -> {
            setWifiEnabled(true);
            turnScreenOn();
            assertWifiScanningIsOn();
            // Toggle screen and verify Wi-Fi scanning is still on.
            turnScreenOff();
            assertWifiScanningIsOn();
            turnScreenOn();
            assertWifiScanningIsOn();
        });
    }

    /**
     * Verify that the platform supports a reasonable number of suggestions per app.
     * @throws Exception
     */
    public void testMaxNumberOfNetworkSuggestionsPerApp() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        assertTrue(mWifiManager.getMaxNumberOfNetworkSuggestionsPerApp()
                > ENFORCED_NUM_NETWORK_SUGGESTIONS_PER_APP);
    }

    private void verifyRegisterSoftApCallback(TestExecutor executor, TestSoftApCallback callback)
            throws Exception {
        // Register callback to get SoftApCapability
        mWifiManager.registerSoftApCallback(executor, callback);
        PollingCheck.check(
                "SoftAp register failed!", 5_000,
                () -> {
                    executor.runAll();
                    // Verify callback is run on the supplied executor and called
                    return callback.getOnStateChangedCalled() &&
                            callback.getOnSoftapInfoChangedCalledCount() > 0 &&
                            callback.getOnSoftApCapabilityChangedCalled() &&
                            callback.getOnConnectedClientCalled();
                });
    }

    private void verifySetGetSoftApConfig(SoftApConfiguration targetConfig) {
        mWifiManager.setSoftApConfiguration(targetConfig);
        // Bssid set dodesn't support for tethered hotspot
        SoftApConfiguration currentConfig = mWifiManager.getSoftApConfiguration();
        compareSoftApConfiguration(targetConfig, currentConfig);
        if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
            assertTrue(currentConfig.isUserConfiguration());
        }
    }

    private void compareSoftApConfiguration(SoftApConfiguration currentConfig,
        SoftApConfiguration testSoftApConfig) {
        assertEquals(currentConfig.getSsid(), testSoftApConfig.getSsid());
        assertEquals(currentConfig.getBssid(), testSoftApConfig.getBssid());
        assertEquals(currentConfig.getSecurityType(), testSoftApConfig.getSecurityType());
        assertEquals(currentConfig.getPassphrase(), testSoftApConfig.getPassphrase());
        assertEquals(currentConfig.isHiddenSsid(), testSoftApConfig.isHiddenSsid());
        assertEquals(currentConfig.getBand(), testSoftApConfig.getBand());
        assertEquals(currentConfig.getChannel(), testSoftApConfig.getChannel());
        assertEquals(currentConfig.getMaxNumberOfClients(),
                testSoftApConfig.getMaxNumberOfClients());
        assertEquals(currentConfig.isAutoShutdownEnabled(),
                testSoftApConfig.isAutoShutdownEnabled());
        assertEquals(currentConfig.getShutdownTimeoutMillis(),
                testSoftApConfig.getShutdownTimeoutMillis());
        assertEquals(currentConfig.isClientControlByUserEnabled(),
                testSoftApConfig.isClientControlByUserEnabled());
        assertEquals(currentConfig.getAllowedClientList(),
                testSoftApConfig.getAllowedClientList());
        assertEquals(currentConfig.getBlockedClientList(),
                testSoftApConfig.getBlockedClientList());
        if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
            assertEquals(currentConfig.getMacRandomizationSetting(),
                    testSoftApConfig.getMacRandomizationSetting());
            assertEquals(currentConfig.getChannels().toString(),
                    testSoftApConfig.getChannels().toString());
            assertEquals(currentConfig.isBridgedModeOpportunisticShutdownEnabled(),
                    testSoftApConfig.isBridgedModeOpportunisticShutdownEnabled());
            assertEquals(currentConfig.isIeee80211axEnabled(),
                    testSoftApConfig.isIeee80211axEnabled());
        }
    }

    private void turnOffWifiAndTetheredHotspotIfEnabled() throws Exception {
        if (mWifiManager.isWifiEnabled()) {
            Log.d(TAG, "Turn off WiFi");
            mWifiManager.setWifiEnabled(false);
            PollingCheck.check(
                "Wifi turn off failed!", 2_000,
                () -> mWifiManager.isWifiEnabled() == false);
        }
        if (mWifiManager.isWifiApEnabled()) {
            mTetheringManager.stopTethering(ConnectivityManager.TETHERING_WIFI);
            Log.d(TAG, "Turn off tethered Hotspot");
            PollingCheck.check(
                "SoftAp turn off failed!", 2_000,
                () -> mWifiManager.isWifiApEnabled() == false);
            mTetheringManager.stopTethering(ConnectivityManager.TETHERING_WIFI);
        }
    }

    private void verifyBridgedModeSoftApCallback(TestExecutor executor,
            TestSoftApCallback callback, boolean shouldFallbackSingleApMode, boolean isEnabled)
            throws Exception {
            // Verify state and info callback value as expected
            PollingCheck.check(
                    "SoftAp state and info on bridged AP mode are mismatch!!!"
                    + " shouldFallbackSingleApMode = " + shouldFallbackSingleApMode
                    + ", isEnabled = "  + isEnabled, 10_000,
                    () -> {
                        executor.runAll();
                        int expectedState = isEnabled ? WifiManager.WIFI_AP_STATE_ENABLED
                                : WifiManager.WIFI_AP_STATE_DISABLED;
                        int expectedInfoSize = isEnabled
                                ? (shouldFallbackSingleApMode ? 1 : 2) : 0;
                        return expectedState == callback.getCurrentState()
                                && callback.getCurrentSoftApInfoList().size() == expectedInfoSize;
                    });
    }

    private boolean shouldFallbackToSingleAp(int[] bands, SoftApCapability capability) {
        for (int band : bands) {
            if (capability.getSupportedChannelList(band).length == 0) {
                return true;
            }
        }
        return false;
    }

    private SparseIntArray getAvailableBandAndChannelForTesting(SoftApCapability capability) {
        final int[] bands = {SoftApConfiguration.BAND_2GHZ, SoftApConfiguration.BAND_5GHZ,
              SoftApConfiguration.BAND_6GHZ, SoftApConfiguration.BAND_60GHZ};
        SparseIntArray testBandsAndChannels = new SparseIntArray();
        if (!ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
            testBandsAndChannels.put(SoftApConfiguration.BAND_2GHZ, 1);
            return testBandsAndChannels;
        }
        for (int band : bands) {
            int[] supportedList = capability.getSupportedChannelList(band);
            if (supportedList.length != 0) {
                testBandsAndChannels.put(band, supportedList[0]);
            }
        }
        return testBandsAndChannels;
    }


    /**
     * Test bridged AP enable succeeful when device supports it.
     * Also verify the callback info update correctly.
     * @throws Exception
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testTetheredBridgedAp() throws Exception {
        // check that softap bridged mode is supported by the device
        if (!mWifiManager.isBridgedApConcurrencySupported()) {
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestExecutor executor = new TestExecutor();
        TestSoftApCallback callback = new TestSoftApCallback(mLock);
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Off/On Wifi to make sure that we get the supported channel
            turnOffWifiAndTetheredHotspotIfEnabled();
            mWifiManager.setWifiEnabled(true);
            PollingCheck.check(
                "Wifi turn on failed!", 2_000,
                () -> mWifiManager.isWifiEnabled() == true);
            turnOffWifiAndTetheredHotspotIfEnabled();
            verifyRegisterSoftApCallback(executor, callback);
            int[] testBands = {SoftApConfiguration.BAND_2GHZ,
                    SoftApConfiguration.BAND_5GHZ};
            int[] expectedBands = {SoftApConfiguration.BAND_2GHZ,
                    SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ};
            // Test bridged SoftApConfiguration set and get (setBands)
            SoftApConfiguration testSoftApConfig = new SoftApConfiguration.Builder()
                    .setSsid(TEST_SSID_UNQUOTED)
                    .setPassphrase(TEST_PASSPHRASE, SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                    .setBands(expectedBands)
                    .build();
            boolean shouldFallbackToSingleAp = shouldFallbackToSingleAp(testBands,
                    callback.getCurrentSoftApCapability());
            verifySetGetSoftApConfig(testSoftApConfig);

            // start tethering which used to verify startTetheredHotspot
            mTetheringManager.startTethering(ConnectivityManager.TETHERING_WIFI, executor,
                new TetheringManager.StartTetheringCallback() {
                    @Override
                    public void onTetheringFailed(final int result) {
                    }
                });
            verifyBridgedModeSoftApCallback(executor, callback,
                    shouldFallbackToSingleAp, true /* enabled */);
            // stop tethering which used to verify stopSoftAp
            mTetheringManager.stopTethering(ConnectivityManager.TETHERING_WIFI);
            verifyBridgedModeSoftApCallback(executor, callback,
                    shouldFallbackToSingleAp, false /* disabled */);
        } finally {
            mWifiManager.unregisterSoftApCallback(callback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Test bridged AP with forced channel config enable succeeful when device supports it.
     * Also verify the callback info update correctly.
     * @throws Exception
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testTetheredBridgedApWifiForcedChannel() throws Exception {
        // check that softap bridged mode is supported by the device
        if (!mWifiManager.isBridgedApConcurrencySupported()) {
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestExecutor executor = new TestExecutor();
        TestSoftApCallback callback = new TestSoftApCallback(mLock);
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Off/On Wifi to make sure that we get the supported channel
            turnOffWifiAndTetheredHotspotIfEnabled();
            mWifiManager.setWifiEnabled(true);
            PollingCheck.check(
                "Wifi turn on failed!", 2_000,
                () -> mWifiManager.isWifiEnabled() == true);
            turnOffWifiAndTetheredHotspotIfEnabled();
            verifyRegisterSoftApCallback(executor, callback);

            boolean shouldFallbackToSingleAp = shouldFallbackToSingleAp(
                    new int[] {SoftApConfiguration.BAND_2GHZ, SoftApConfiguration.BAND_5GHZ},
                    callback.getCurrentSoftApCapability());

            // Test when there are supported channels in both of the bands.
            if (!shouldFallbackToSingleAp) {
                // Test bridged SoftApConfiguration set and get (setChannels)
                SparseIntArray dual_channels = new SparseIntArray(2);
                dual_channels.put(SoftApConfiguration.BAND_2GHZ,
                        callback.getCurrentSoftApCapability()
                        .getSupportedChannelList(SoftApConfiguration.BAND_2GHZ)[0]);
                dual_channels.put(SoftApConfiguration.BAND_5GHZ,
                        callback.getCurrentSoftApCapability()
                        .getSupportedChannelList(SoftApConfiguration.BAND_5GHZ)[0]);
                SoftApConfiguration testSoftApConfig = new SoftApConfiguration.Builder()
                        .setSsid(TEST_SSID_UNQUOTED)
                        .setPassphrase(TEST_PASSPHRASE, SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                        .setChannels(dual_channels)
                        .build();

                verifySetGetSoftApConfig(testSoftApConfig);

                // start tethering which used to verify startTetheredHotspot
                mTetheringManager.startTethering(ConnectivityManager.TETHERING_WIFI, executor,
                    new TetheringManager.StartTetheringCallback() {
                        @Override
                        public void onTetheringFailed(final int result) {
                        }
                    });
                verifyBridgedModeSoftApCallback(executor, callback,
                        shouldFallbackToSingleAp, true /* enabled */);
                // stop tethering which used to verify stopSoftAp
                mTetheringManager.stopTethering(ConnectivityManager.TETHERING_WIFI);
                verifyBridgedModeSoftApCallback(executor, callback,
                        shouldFallbackToSingleAp, false /* disabled */);
            }
        } finally {
            mWifiManager.unregisterSoftApCallback(callback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Verify that the configuration from getSoftApConfiguration is same as the configuration which
     * set by setSoftApConfiguration. And depends softap capability callback to test different
     * configuration.
     * @throws Exception
     */
    public void testSetGetSoftApConfigurationAndSoftApCapabilityCallback() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestExecutor executor = new TestExecutor();
        TestSoftApCallback callback = new TestSoftApCallback(mLock);
        try {
            uiAutomation.adoptShellPermissionIdentity();
            turnOffWifiAndTetheredHotspotIfEnabled();
            verifyRegisterSoftApCallback(executor, callback);

            SoftApConfiguration.Builder softApConfigBuilder = new SoftApConfiguration.Builder()
                    .setSsid(TEST_SSID_UNQUOTED)
                    .setPassphrase(TEST_PASSPHRASE, SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                    .setAutoShutdownEnabled(true)
                    .setShutdownTimeoutMillis(100000)
                    .setBand(getAvailableBandAndChannelForTesting(
                            callback.getCurrentSoftApCapability()).keyAt(0))
                    .setHiddenSsid(false);

            // Test SoftApConfiguration set and get
            verifySetGetSoftApConfig(softApConfigBuilder.build());

            boolean isSupportCustomizedMac = callback.getCurrentSoftApCapability()
                        .areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION)
                    && PropertyUtil.isVndkApiLevelNewerThan(Build.VERSION_CODES.S);

            //Test MAC_ADDRESS_CUSTOMIZATION supported config
            if (isSupportCustomizedMac) {
                softApConfigBuilder.setBssid(TEST_MAC)
                        .setMacRandomizationSetting(SoftApConfiguration.RANDOMIZATION_NONE);

                // Test SoftApConfiguration set and get
                verifySetGetSoftApConfig(softApConfigBuilder.build());
            }

            // Test CLIENT_FORCE_DISCONNECT supported config.
            if (callback.getCurrentSoftApCapability()
                    .areFeaturesSupported(
                    SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT)) {
                softApConfigBuilder.setMaxNumberOfClients(10);
                softApConfigBuilder.setClientControlByUserEnabled(true);
                softApConfigBuilder.setBlockedClientList(new ArrayList<>());
                softApConfigBuilder.setAllowedClientList(new ArrayList<>());
                verifySetGetSoftApConfig(softApConfigBuilder.build());
            }

            // Test SAE config
            if (callback.getCurrentSoftApCapability()
                    .areFeaturesSupported(SoftApCapability.SOFTAP_FEATURE_WPA3_SAE)) {
                softApConfigBuilder
                        .setPassphrase(TEST_PASSPHRASE,
                          SoftApConfiguration.SECURITY_TYPE_WPA3_SAE_TRANSITION);
                verifySetGetSoftApConfig(softApConfigBuilder.build());
                softApConfigBuilder
                        .setPassphrase(TEST_PASSPHRASE,
                        SoftApConfiguration.SECURITY_TYPE_WPA3_SAE);
                verifySetGetSoftApConfig(softApConfigBuilder.build());
            }

            // Test 11 AX control config.
            if (callback.getCurrentSoftApCapability()
                    .areFeaturesSupported(SoftApCapability.SOFTAP_FEATURE_IEEE80211_AX)) {
                softApConfigBuilder.setIeee80211axEnabled(true);
                verifySetGetSoftApConfig(softApConfigBuilder.build());
            }

            if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
                softApConfigBuilder.setBridgedModeOpportunisticShutdownEnabled(false);
                verifySetGetSoftApConfig(softApConfigBuilder.build());
            }

        } finally {
            mWifiManager.unregisterSoftApCallback(callback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Verify that startTetheredHotspot with specific channel config.
     * @throws Exception
     */
    public void testStartTetheredHotspotWithChannelConfigAndSoftApStateAndInfoCallback()
            throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestExecutor executor = new TestExecutor();
        TestSoftApCallback callback = new TestSoftApCallback(mLock);
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // check that tethering is supported by the device
            if (!mTetheringManager.isTetheringSupported()) {
                return;
            }
            turnOffWifiAndTetheredHotspotIfEnabled();
            verifyRegisterSoftApCallback(executor, callback);

            SparseIntArray testBandsAndChannels = getAvailableBandAndChannelForTesting(
                    callback.getCurrentSoftApCapability());

            if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
                assertNotEquals(0, testBandsAndChannels.size());
            }
            boolean isSupportCustomizedMac = callback.getCurrentSoftApCapability()
                    .areFeaturesSupported(
                    SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION)
                    && PropertyUtil.isVndkApiLevelNewerThan(Build.VERSION_CODES.S);

            SoftApConfiguration.Builder testSoftApConfigBuilder = new SoftApConfiguration.Builder()
                    .setSsid(TEST_SSID_UNQUOTED)
                    .setPassphrase(TEST_PASSPHRASE, SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                    .setChannel(testBandsAndChannels.valueAt(0), testBandsAndChannels.keyAt(0));

            if (isSupportCustomizedMac) testSoftApConfigBuilder.setBssid(TEST_MAC);

            SoftApConfiguration testSoftApConfig = testSoftApConfigBuilder.build();

            mWifiManager.setSoftApConfiguration(testSoftApConfig);

            // start tethering which used to verify startTetheredHotspot
            mTetheringManager.startTethering(ConnectivityManager.TETHERING_WIFI, executor,
                new TetheringManager.StartTetheringCallback() {
                    @Override
                    public void onTetheringFailed(final int result) {
                    }
                });

            // Verify state and info callback value as expected
            PollingCheck.check(
                    "SoftAp channel and state mismatch!!!", 10_000,
                    () -> {
                        executor.runAll();
                        int sapChannel = ScanResult.convertFrequencyMhzToChannelIfSupported(
                                callback.getCurrentSoftApInfo().getFrequency());
                        boolean isInfoCallbackSupported =
                                callback.getOnSoftapInfoChangedCalledCount() > 1;
                        if (isInfoCallbackSupported) {
                            return WifiManager.WIFI_AP_STATE_ENABLED == callback.getCurrentState()
                                && testBandsAndChannels.valueAt(0) == sapChannel;
                        }
                        return WifiManager.WIFI_AP_STATE_ENABLED == callback.getCurrentState();
                    });
            // After Soft Ap enabled, check SoftAp info if it supported
            if (isSupportCustomizedMac && callback.getOnSoftapInfoChangedCalledCount() > 1) {
                assertEquals(callback.getCurrentSoftApInfo().getBssid(), TEST_MAC);
            }
            if (PropertyUtil.isVndkApiLevelNewerThan(Build.VERSION_CODES.S)
                    && callback.getOnSoftapInfoChangedCalledCount() > 1) {
                assertNotEquals(callback.getCurrentSoftApInfo().getWifiStandard(),
                        ScanResult.WIFI_STANDARD_UNKNOWN);
            }

            if (callback.getOnSoftapInfoChangedCalledCount() > 1) {
                assertTrue(callback.getCurrentSoftApInfo().getAutoShutdownTimeoutMillis() > 0);
            }
        } finally {
            // stop tethering which used to verify stopSoftAp
            mTetheringManager.stopTethering(ConnectivityManager.TETHERING_WIFI);

            // Verify clean up
            PollingCheck.check(
                    "Stop Softap failed", 3_000,
                    () -> {
                        executor.runAll();
                        return WifiManager.WIFI_AP_STATE_DISABLED == callback.getCurrentState() &&
                                0 == callback.getCurrentSoftApInfo().getBandwidth() &&
                                0 == callback.getCurrentSoftApInfo().getFrequency();
                    });
            if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
                assertEquals(callback.getCurrentSoftApInfo().getBssid(), null);
                assertEquals(ScanResult.WIFI_STANDARD_UNKNOWN,
                        callback.getCurrentSoftApInfo().getWifiStandard());
            }
            mWifiManager.unregisterSoftApCallback(callback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    private static class TestActionListener implements WifiManager.ActionListener {
        private final Object mLock;
        public boolean onSuccessCalled = false;
        public boolean onFailedCalled = false;
        public int failureReason = -1;

        TestActionListener(Object lock) {
            mLock = lock;
        }

        @Override
        public void onSuccess() {
            synchronized (mLock) {
                onSuccessCalled = true;
                mLock.notify();
            }
        }

        @Override
        public void onFailure(int reason) {
            synchronized (mLock) {
                onFailedCalled = true;
                failureReason = reason;
                mLock.notify();
            }
        }
    }

    /**
     * Triggers connection to one of the saved networks using {@link WifiManager#connect(
     * int, WifiManager.ActionListener)} or {@link WifiManager#connect(WifiConfiguration,
     * WifiManager.ActionListener)}
     *
     * @param withNetworkId Use networkId for triggering connection, false for using
     *                      WifiConfiguration.
     * @throws Exception
     */
    private void testConnect(boolean withNetworkId) throws Exception {
        TestActionListener actionListener = new TestActionListener(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        List<WifiConfiguration> savedNetworks = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // These below API's only work with privileged permissions (obtained via shell identity
            // for test)
            savedNetworks = mWifiManager.getConfiguredNetworks();

            // Disable all the saved networks to trigger disconnect & disable autojoin.
            for (WifiConfiguration network : savedNetworks) {
                assertTrue(mWifiManager.disableNetwork(network.networkId));
            }
            waitForDisconnection();

            // Now trigger connection to the last saved network.
            WifiConfiguration savedNetworkToConnect =
                    savedNetworks.get(savedNetworks.size() - 1);
            synchronized (mLock) {
                try {
                    if (withNetworkId) {
                        mWifiManager.connect(savedNetworkToConnect.networkId, actionListener);
                    } else {
                        mWifiManager.connect(savedNetworkToConnect, actionListener);
                    }
                    // now wait for callback
                    mLock.wait(TEST_WAIT_DURATION_MS);
                } catch (InterruptedException e) {
                }
            }
            // check if we got the success callback
            assertTrue(actionListener.onSuccessCalled);
            // Wait for connection to complete & ensure we are connected to the saved network.
            waitForConnection();
            if (SdkLevel.isAtLeastS()) {
                assertEquals(savedNetworkToConnect.networkId,
                        mWifiManager.getConnectionInfo().getNetworkId());
            } else {
                // In R, auto-upgraded network IDs may be different from the original saved network.
                // Since we may end up selecting the auto-upgraded network ID for connection and end
                // up connected to the original saved network with a different network ID, we should
                // instead match by SSID.
                assertEquals(savedNetworkToConnect.SSID,
                        mWifiManager.getConnectionInfo().getSSID());
            }
        } finally {
            // Re-enable all saved networks before exiting.
            if (savedNetworks != null) {
                for (WifiConfiguration network : savedNetworks) {
                    mWifiManager.enableNetwork(network.networkId, true);
                }
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#connect(int, WifiManager.ActionListener)} to an existing saved
     * network.
     */
    public void testConnectWithNetworkId() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        testConnect(true);
    }

    /**
     * Tests {@link WifiManager#connect(WifiConfiguration, WifiManager.ActionListener)} to an
     * existing saved network.
     */
    public void testConnectWithWifiConfiguration() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        testConnect(false);

    }

    private static class TestNetworkCallback extends ConnectivityManager.NetworkCallback {
        private final Object mLock;
        public boolean onAvailableCalled = false;
        public Network network;
        public NetworkCapabilities networkCapabilities;

        TestNetworkCallback(Object lock) {
            mLock = lock;
        }

        @Override
        public void onAvailable(Network network) {
            synchronized (mLock) {
                onAvailableCalled = true;
                this.network = network;
            }
        }

        @Override
        public void onCapabilitiesChanged(Network network,
                NetworkCapabilities networkCapabilities) {
            synchronized (mLock) {
                this.networkCapabilities = networkCapabilities;
                mLock.notify();
            }
        }
    }

    private void waitForNetworkCallbackAndCheckForMeteredness(boolean expectMetered) {
        TestNetworkCallback networkCallbackListener = new TestNetworkCallback(mLock);
        synchronized (mLock) {
            try {
                NetworkRequest.Builder networkRequestBuilder = new NetworkRequest.Builder()
                        .addTransportType(TRANSPORT_WIFI);
                if (expectMetered) {
                    networkRequestBuilder.removeCapability(NET_CAPABILITY_NOT_METERED);
                } else {
                    networkRequestBuilder.addCapability(NET_CAPABILITY_NOT_METERED);
                }
                // File a request for wifi network.
                mConnectivityManager.registerNetworkCallback(
                        networkRequestBuilder.build(), networkCallbackListener);
                // now wait for callback
                mLock.wait(TEST_WAIT_DURATION_MS);
            } catch (InterruptedException e) {
            }
        }
        assertTrue(networkCallbackListener.onAvailableCalled);
    }

    /**
     * Tests {@link WifiManager#save(WifiConfiguration, WifiManager.ActionListener)} by marking
     * an existing saved network metered.
     */
    public void testSave() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        TestActionListener actionListener = new TestActionListener(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        List<WifiConfiguration> savedNetworks = null;
        WifiConfiguration currentConfig = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // These below API's only work with privileged permissions (obtained via shell identity
            // for test)

            // Trigger a scan & wait for connection to one of the saved networks.
            mWifiManager.startScan();
            waitForConnection();

            WifiInfo wifiInfo = mWifiManager.getConnectionInfo();

            // find the current network's WifiConfiguration
            currentConfig = mWifiManager.getConfiguredNetworks()
                    .stream()
                    .filter(config -> config.networkId == wifiInfo.getNetworkId())
                    .findAny()
                    .get();

            // Ensure that the current network is not metered.
            assertNotEquals("Ensure that the saved network is configured as unmetered",
                    currentConfig.meteredOverride,
                    WifiConfiguration.METERED_OVERRIDE_METERED);

            // Check the network capabilities to ensure that the network is marked not metered.
            waitForNetworkCallbackAndCheckForMeteredness(false);

            // Now mark the network metered and save.
            synchronized (mLock) {
                try {
                    WifiConfiguration modSavedNetwork = new WifiConfiguration(currentConfig);
                    modSavedNetwork.meteredOverride = WifiConfiguration.METERED_OVERRIDE_METERED;
                    mWifiManager.save(modSavedNetwork, actionListener);
                    // now wait for callback
                    mLock.wait(TEST_WAIT_DURATION_MS);
                } catch (InterruptedException e) {
                }
            }
            // check if we got the success callback
            assertTrue(actionListener.onSuccessCalled);
            // Ensure we disconnected on marking the network metered & connect back.
            waitForDisconnection();
            waitForConnection();
            // Check the network capabilities to ensure that the network is marked metered now.
            waitForNetworkCallbackAndCheckForMeteredness(true);

        } finally {
            // Restore original network config (restore the meteredness back);
            if (currentConfig != null) {
                mWifiManager.updateNetwork(currentConfig);
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#forget(int, WifiManager.ActionListener)} by adding/removing a new
     * network.
     */
    @AsbSecurityTest(cveBugId = 159373687)
    public void testForget() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        TestActionListener actionListener = new TestActionListener(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        int newNetworkId = INVALID_NETWORK_ID;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // These below API's only work with privileged permissions (obtained via shell identity
            // for test)
            List<WifiConfiguration> savedNetworks = mWifiManager.getConfiguredNetworks();

            WifiConfiguration newOpenNetwork = new WifiConfiguration();
            newOpenNetwork.SSID = "\"" + TEST_SSID_UNQUOTED + "\"";
            newNetworkId = mWifiManager.addNetwork(newOpenNetwork);
            assertNotEquals(INVALID_NETWORK_ID, newNetworkId);

            // Multi-type configurations might be converted to more than 1 configuration.
            assertThat(savedNetworks.size() < mWifiManager.getConfiguredNetworks().size()).isTrue();

            // Need an effectively-final holder because we need to modify inner Intent in callback.
            class IntentHolder {
                Intent intent;
            }
            IntentHolder intentHolder = new IntentHolder();
            mContext.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Log.i(TAG, "Received CONFIGURED_NETWORKS_CHANGED_ACTION broadcast: " + intent);
                    intentHolder.intent = intent;
                }
            }, new IntentFilter(WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION));

            // Now remove the network
            synchronized (mLock) {
                try {
                    mWifiManager.forget(newNetworkId, actionListener);
                    // now wait for callback
                    mLock.wait(TEST_WAIT_DURATION_MS);
                } catch (InterruptedException e) {
                }
            }
            // check if we got the success callback
            assertTrue(actionListener.onSuccessCalled);

            PollingCheck.check(
                    "Didn't receive CONFIGURED_NETWORKS_CHANGED_ACTION broadcast!",
                    TEST_WAIT_DURATION_MS,
                    () -> intentHolder.intent != null);
            Intent intent = intentHolder.intent;
            assertEquals(WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION, intent.getAction());
            assertTrue(intent.getBooleanExtra(WifiManager.EXTRA_MULTIPLE_NETWORKS_CHANGED, false));
            assertEquals(WifiManager.CHANGE_REASON_REMOVED,
                    intent.getIntExtra(WifiManager.EXTRA_CHANGE_REASON, -1));
            assertNull(intent.getParcelableExtra(WifiManager.EXTRA_WIFI_CONFIGURATION));

            // Ensure that the new network has been successfully removed.
            assertEquals(savedNetworks.size(), mWifiManager.getConfiguredNetworks().size());
        } finally {
            // For whatever reason, if the forget fails, try removing using the public remove API.
            if (newNetworkId != INVALID_NETWORK_ID) mWifiManager.removeNetwork(newNetworkId);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#getFactoryMacAddresses()} returns at least one valid MAC address.
     */
    @VirtualDeviceNotSupported
    public void testGetFactoryMacAddresses() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        TestActionListener actionListener = new TestActionListener(mLock);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        int newNetworkId = INVALID_NETWORK_ID;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Obtain the factory MAC address
            String[] macAddresses = mWifiManager.getFactoryMacAddresses();
            assertTrue("At list one MAC address should be returned.", macAddresses.length > 0);
            try {
                MacAddress mac = MacAddress.fromString(macAddresses[0]);
                assertNotEquals(WifiInfo.DEFAULT_MAC_ADDRESS, mac);
                assertFalse(MacAddressUtils.isMulticastAddress(mac));
            } catch (IllegalArgumentException e) {
                fail("Factory MAC address is invalid");
            }
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#isApMacRandomizationSupported()} does not crash.
     */
    public void testIsApMacRandomizationSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isApMacRandomizationSupported();
    }

    /**
     * Tests {@link WifiManager#isConnectedMacRandomizationSupported()} does not crash.
     */
    public void testIsConnectedMacRandomizationSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isConnectedMacRandomizationSupported();
    }

    /**
     * Tests {@link WifiManager#isPreferredNetworkOffloadSupported()} does not crash.
     */
    public void testIsPreferredNetworkOffloadSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isPreferredNetworkOffloadSupported();
    }

    /** Test that PNO scans reconnects us when the device is disconnected and the screen is off. */
    public void testPnoScan() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!mWifiManager.isPreferredNetworkOffloadSupported()) {
            // skip the test if PNO scanning is not supported
            return;
        }

        // make sure we're connected
        waitForConnection();

        WifiInfo currentNetwork = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getConnectionInfo);

        // disable all networks that aren't already disabled
        List<WifiConfiguration> savedNetworks = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getConfiguredNetworks);
        Set<Integer> disabledNetworkIds = new HashSet<>();
        for (WifiConfiguration config : savedNetworks) {
            if (config.getNetworkSelectionStatus().getNetworkSelectionDisableReason()
                    == WifiConfiguration.NetworkSelectionStatus.DISABLED_NONE) {
                ShellIdentityUtils.invokeWithShellPermissions(
                        () -> mWifiManager.disableNetwork(config.networkId));
                disabledNetworkIds.add(config.networkId);
            }
        }

        try {
            // wait for disconnection from current network
            waitForDisconnection();

            // turn screen off
            turnScreenOffNoDelay();

            // re-enable the current network - this will trigger PNO
            ShellIdentityUtils.invokeWithShellPermissions(
                    () -> mWifiManager.enableNetwork(currentNetwork.getNetworkId(), false));
            disabledNetworkIds.remove(currentNetwork.getNetworkId());

            // PNO should reconnect us back to the network we disconnected from
            waitForConnection();
        } finally {
            // re-enable disabled networks
            for (int disabledNetworkId : disabledNetworkIds) {
                ShellIdentityUtils.invokeWithShellPermissions(
                        () -> mWifiManager.enableNetwork(disabledNetworkId, true));
            }
        }
    }

    /**
     * Tests {@link WifiManager#isTdlsSupported()} does not crash.
     */
    public void testIsTdlsSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isTdlsSupported();
    }

    /**
     * Tests {@link WifiManager#isStaApConcurrencySupported().
     */
    public void testIsStaApConcurrencySupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // check that softap mode is supported by the device
        if (!mWifiManager.isPortableHotspotSupported()) {
            return;
        }
        assertTrue(mWifiManager.isWifiEnabled());

        boolean isStaApConcurrencySupported = mWifiManager.isStaApConcurrencySupported();
        // start local only hotspot.
        TestLocalOnlyHotspotCallback callback = startLocalOnlyHotspot();
        try {
            if (isStaApConcurrencySupported) {
                assertTrue(mWifiManager.isWifiEnabled());
            } else {
                // no concurrency, wifi should be disabled.
                assertFalse(mWifiManager.isWifiEnabled());
            }
        } finally {
            // clean up local only hotspot no matter if assertion passed or failed
            stopLocalOnlyHotspot(callback, true);
        }

        assertTrue(mWifiManager.isWifiEnabled());
    }

    /**
     * state is a bitset, where bit 0 indicates whether there was data in, and bit 1 indicates
     * whether there was data out. Only count down on the latch once there was both data in and out.
     */
    private static class TestTrafficStateCallback implements WifiManager.TrafficStateCallback {
        public final CountDownLatch latch = new CountDownLatch(1);
        private int mAccumulator = 0;

        @Override
        public void onStateChanged(int state) {
            mAccumulator |= state;
            if (mAccumulator == DATA_ACTIVITY_INOUT) {
                latch.countDown();
            }
        }
    }

    private void sendTraffic() {
        boolean didAnyConnectionSucceed = false;
        for (int i = 0; i < 10; i++) {
            // Do some network operations
            HttpURLConnection connection = null;
            try {
                URL url = new URL("http://www.google.com/");
                connection = (HttpURLConnection) url.openConnection();
                connection.setInstanceFollowRedirects(false);
                connection.setConnectTimeout(TEST_WAIT_DURATION_MS);
                connection.setReadTimeout(TEST_WAIT_DURATION_MS);
                connection.setUseCaches(false);
                InputStream stream = connection.getInputStream();
                byte[] bytes = new byte[100];
                int receivedBytes = stream.read(bytes);
                if (receivedBytes > 0) {
                    didAnyConnectionSucceed = true;
                }
            } catch (Exception e) {
                // ignore
            } finally {
                if (connection != null) connection.disconnect();
            }
        }
        assertTrue("All connections failed!", didAnyConnectionSucceed);
    }

    /**
     * Tests {@link WifiManager#registerTrafficStateCallback(Executor,
     * WifiManager.TrafficStateCallback)} by sending some traffic.
     */
    public void testTrafficStateCallback() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        TestTrafficStateCallback callback = new TestTrafficStateCallback();
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Trigger a scan & wait for connection to one of the saved networks.
            mWifiManager.startScan();
            waitForConnection();

            // Turn screen on for wifi traffic polling.
            turnScreenOn();
            mWifiManager.registerTrafficStateCallback(
                    Executors.newSingleThreadExecutor(), callback);
            // Send some traffic to trigger the traffic state change callbacks.
            sendTraffic();
            // now wait for callback
            boolean success = callback.latch.await(TEST_WAIT_DURATION_MS, TimeUnit.MILLISECONDS);
            // check if we got the state changed callback with both data in and out
            assertTrue(success);
        } finally {
            turnScreenOff();
            mWifiManager.unregisterTrafficStateCallback(callback);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#setScanAlwaysAvailable(boolean)} &
     * {@link WifiManager#isScanAlwaysAvailable()}.
     */
    public void testScanAlwaysAvailable() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            currState = mWifiManager.isScanAlwaysAvailable();
            boolean newState = !currState;
            mWifiManager.setScanAlwaysAvailable(newState);
            PollingCheck.check(
                    "Wifi settings toggle failed!",
                    DURATION_SETTINGS_TOGGLE,
                    () -> mWifiManager.isScanAlwaysAvailable() == newState);
            assertEquals(newState, mWifiManager.isScanAlwaysAvailable());
        } finally {
            if (currState != null) mWifiManager.setScanAlwaysAvailable(currState);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#setScanThrottleEnabled(boolean)} &
     * {@link WifiManager#isScanThrottleEnabled()}.
     */
    public void testScanThrottleEnabled() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            currState = mWifiManager.isScanThrottleEnabled();
            boolean newState = !currState;
            mWifiManager.setScanThrottleEnabled(newState);
            PollingCheck.check(
                    "Wifi settings toggle failed!",
                    DURATION_SETTINGS_TOGGLE,
                    () -> mWifiManager.isScanThrottleEnabled() == newState);
            assertEquals(newState, mWifiManager.isScanThrottleEnabled());
        } finally {
            if (currState != null) mWifiManager.setScanThrottleEnabled(currState);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#setAutoWakeupEnabled(boolean)} &
     * {@link WifiManager#isAutoWakeupEnabled()}.
     */
    public void testAutoWakeUpEnabled() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            currState = mWifiManager.isAutoWakeupEnabled();
            boolean newState = !currState;
            mWifiManager.setAutoWakeupEnabled(newState);
            PollingCheck.check(
                    "Wifi settings toggle failed!",
                    DURATION_SETTINGS_TOGGLE,
                    () -> mWifiManager.isAutoWakeupEnabled() == newState);
            assertEquals(newState, mWifiManager.isAutoWakeupEnabled());
        } finally {
            if (currState != null) mWifiManager.setAutoWakeupEnabled(currState);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#setVerboseLoggingEnabled(boolean)} &
     * {@link WifiManager#isVerboseLoggingEnabled()}.
     */
    public void testVerboseLoggingEnabled() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        TestWifiVerboseLoggingStatusChangedListener listener =
                WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext) ?
                new TestWifiVerboseLoggingStatusChangedListener() : null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            if (listener != null) {
                mWifiManager.addWifiVerboseLoggingStatusChangedListener(mExecutor, listener);
            }
            currState = mWifiManager.isVerboseLoggingEnabled();
            boolean newState = !currState;
            if (listener != null) {
                assertEquals(0, listener.numCalls);
            }
            mWifiManager.setVerboseLoggingEnabled(newState);
            PollingCheck.check(
                    "Wifi verbose logging toggle failed!",
                    DURATION_SETTINGS_TOGGLE,
                    () -> mWifiManager.isVerboseLoggingEnabled() == newState);
            if (listener != null) {
                PollingCheck.check(
                        "Verbose logging listener timeout",
                        DURATION_SETTINGS_TOGGLE,
                        () -> listener.status == newState && listener.numCalls == 1);
            }
        } finally {
            if (currState != null) mWifiManager.setVerboseLoggingEnabled(currState);
            if (listener != null) {
                mWifiManager.removeWifiVerboseLoggingStatusChangedListener(listener);
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#setVerboseLoggingLevel(int)}.
     */
    public void testSetVerboseLogging() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            currState = mWifiManager.isVerboseLoggingEnabled();

            mWifiManager.setVerboseLoggingLevel(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED);
            assertTrue(mWifiManager.isVerboseLoggingEnabled());
            assertEquals(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED,
                    mWifiManager.getVerboseLoggingLevel());

            mWifiManager.setVerboseLoggingLevel(WifiManager.VERBOSE_LOGGING_LEVEL_DISABLED);
            assertFalse(mWifiManager.isVerboseLoggingEnabled());
            assertEquals(WifiManager.VERBOSE_LOGGING_LEVEL_DISABLED,
                    mWifiManager.getVerboseLoggingLevel());
        } finally {
            if (currState != null) mWifiManager.setVerboseLoggingEnabled(currState);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Test {@link WifiManager#setVerboseLoggingLevel(int)} for show key mode.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testSetVerboseLoggingShowKeyModeNonUserBuild() throws Exception {
        if (Build.TYPE.equals("user")) return;
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            currState = mWifiManager.isVerboseLoggingEnabled();

            mWifiManager.setVerboseLoggingLevel(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED_SHOW_KEY);
            assertTrue(mWifiManager.isVerboseLoggingEnabled());
            assertEquals(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED_SHOW_KEY,
                    mWifiManager.getVerboseLoggingLevel());
        } finally {
            if (currState != null) mWifiManager.setVerboseLoggingEnabled(currState);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Test {@link WifiManager#setVerboseLoggingLevel(int)} for show key mode.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testSetVerboseLoggingShowKeyModeUserBuild() throws Exception {
        if (!Build.TYPE.equals("user")) return;
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        Boolean currState = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            currState = mWifiManager.isVerboseLoggingEnabled();

            mWifiManager.setVerboseLoggingLevel(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED_SHOW_KEY);
            assertTrue(mWifiManager.isVerboseLoggingEnabled());
            assertEquals(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED_SHOW_KEY,
                    mWifiManager.getVerboseLoggingLevel());
            fail("Verbosing logging show key mode should not be allowed for user build.");
        } catch (SecurityException e) {
            // expected
        } finally {
            if (currState != null) mWifiManager.setVerboseLoggingEnabled(currState);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#factoryReset()} cannot be invoked from a non-privileged app.
     *
     * Note: This intentionally does not test the full reset functionality because it causes
     * the existing saved networks on the device to be lost after the test. If you add the
     * networks back after reset, the ownership of saved networks will change.
     */
    public void testFactoryReset() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        List<WifiConfiguration> beforeSavedNetworks = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getConfiguredNetworks);
        try {
            mWifiManager.factoryReset();
            fail("Factory reset should not be allowed for non-privileged apps");
        } catch (SecurityException e) {
            // expected
        }
        List<WifiConfiguration> afterSavedNetworks = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getConfiguredNetworks);
        assertEquals(beforeSavedNetworks.size(), afterSavedNetworks.size());
    }

    /**
     * Test {@link WifiNetworkConnectionStatistics} does not crash.
     * TODO(b/150891569): deprecate it in Android S, this API is not used anywhere.
     */
    public void testWifiNetworkConnectionStatistics() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        new WifiNetworkConnectionStatistics();
        WifiNetworkConnectionStatistics stats = new WifiNetworkConnectionStatistics(0, 0);
        new WifiNetworkConnectionStatistics(stats);
    }

    /**
     * Verify that startRestrictingAutoJoinToSubscriptionId disconnects wifi and disables
     * auto-connect to non-carrier-merged networks. Then verify that
     * stopRestrictingAutoJoinToSubscriptionId makes the disabled networks clear to connect
     * again.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testStartAndStopRestrictingAutoJoinToSubscriptionId() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        startScan();
        waitForConnection();
        int fakeSubscriptionId = 5;
        ShellIdentityUtils.invokeWithShellPermissions(() ->
                mWifiManager.startRestrictingAutoJoinToSubscriptionId(fakeSubscriptionId));
        startScan();
        ensureNotConnected();
        ShellIdentityUtils.invokeWithShellPermissions(() ->
                mWifiManager.stopRestrictingAutoJoinToSubscriptionId());
        startScan();
        waitForConnection();
    }

    /**
     * Test that the wifi country code is either null, or a length-2 string.
     */
    public void testGetCountryCode() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        String wifiCountryCode = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getCountryCode);

        if (wifiCountryCode == null) {
            return;
        }
        assertEquals(2, wifiCountryCode.length());

        // assert that the country code is all uppercase
        assertEquals(wifiCountryCode.toUpperCase(Locale.US), wifiCountryCode);

        // skip if Telephony is unsupported
        if (!WifiFeature.isTelephonySupported(getContext())) {
            return;
        }

        String telephonyCountryCode = getContext().getSystemService(TelephonyManager.class)
                .getNetworkCountryIso();

        // skip if Telephony country code is unavailable
        if (telephonyCountryCode == null || telephonyCountryCode.isEmpty()) {
            return;
        }

        assertEquals(telephonyCountryCode, wifiCountryCode.toLowerCase(Locale.US));
    }

    /**
     * Test that {@link WifiManager#getCurrentNetwork()} returns a Network obeject consistent
     * with {@link ConnectivityManager#registerNetworkCallback} when connected to a Wifi network,
     * and returns null when not connected.
     */
    public void testGetCurrentNetwork() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // ensure Wifi is connected
        ShellIdentityUtils.invokeWithShellPermissions(() -> mWifiManager.reconnect());
        PollingCheck.check(
                "Wifi not connected - Please ensure there is a saved network in range of this "
                        + "device",
                WIFI_CONNECT_TIMEOUT_MILLIS,
                () -> mWifiManager.getConnectionInfo().getNetworkId() != -1);

        Network wifiCurrentNetwork = ShellIdentityUtils.invokeWithShellPermissions(
                mWifiManager::getCurrentNetwork);
        assertNotNull(wifiCurrentNetwork);

        TestNetworkCallback networkCallbackListener = new TestNetworkCallback(mLock);
        synchronized (mLock) {
            try {
                // File a request for wifi network.
                mConnectivityManager.registerNetworkCallback(
                        new NetworkRequest.Builder()
                                .addTransportType(TRANSPORT_WIFI)
                                .build(),
                        networkCallbackListener);
                // now wait for callback
                mLock.wait(TEST_WAIT_DURATION_MS);
            } catch (InterruptedException e) {
            }
        }
        assertTrue(networkCallbackListener.onAvailableCalled);
        Network connectivityCurrentNetwork = networkCallbackListener.network;
        assertEquals(connectivityCurrentNetwork, wifiCurrentNetwork);

        setWifiEnabled(false);
        PollingCheck.check(
                "Wifi not disconnected!",
                20000,
                () -> mWifiManager.getConnectionInfo().getNetworkId() == -1);

        assertNull(ShellIdentityUtils.invokeWithShellPermissions(mWifiManager::getCurrentNetwork));
    }

    /**
     * Tests {@link WifiManager#isWpa3SaeSupported()} does not crash.
     */
    public void testIsWpa3SaeSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isWpa3SaeSupported();
    }

    /**
     * Tests {@link WifiManager#isWpa3SuiteBSupported()} does not crash.
     */
    public void testIsWpa3SuiteBSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isWpa3SuiteBSupported();
    }

    /**
     * Tests {@link WifiManager#isEnhancedOpenSupported()} does not crash.
     */
    public void testIsEnhancedOpenSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isEnhancedOpenSupported();
    }

    /**
     * Test that {@link WifiManager#is5GHzBandSupported()} returns successfully in
     * both WiFi enabled/disabled states.
     * Note that the response depends on device support and hence both true/false
     * are valid responses.
     */
    public void testIs5GhzBandSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // Check for 5GHz support with wifi enabled
        setWifiEnabled(true);
        PollingCheck.check(
                "Wifi not enabled!",
                20000,
                () -> mWifiManager.isWifiEnabled());
        boolean isSupportedEnabled = mWifiManager.is5GHzBandSupported();

        // Check for 5GHz support with wifi disabled
        setWifiEnabled(false);
        PollingCheck.check(
                "Wifi not disabled!",
                20000,
                () -> !mWifiManager.isWifiEnabled());
        boolean isSupportedDisabled = mWifiManager.is5GHzBandSupported();

        // If Support is true when WiFi is disable, then it has to be true when it is enabled.
        // Note, the reverse is a valid case.
        if (isSupportedDisabled) {
            assertTrue(isSupportedEnabled);
        }
    }

    /**
     * Test that {@link WifiManager#is6GHzBandSupported()} returns successfully in
     * both Wifi enabled/disabled states.
     * Note that the response depends on device support and hence both true/false
     * are valid responses.
     */
    public void testIs6GhzBandSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // Check for 6GHz support with wifi enabled
        setWifiEnabled(true);
        PollingCheck.check(
                "Wifi not enabled!",
                20000,
                () -> mWifiManager.isWifiEnabled());
        boolean isSupportedEnabled = mWifiManager.is6GHzBandSupported();

        // Check for 6GHz support with wifi disabled
        setWifiEnabled(false);
        PollingCheck.check(
                "Wifi not disabled!",
                20000,
                () -> !mWifiManager.isWifiEnabled());
        boolean isSupportedDisabled = mWifiManager.is6GHzBandSupported();

        // If Support is true when WiFi is disable, then it has to be true when it is enabled.
        // Note, the reverse is a valid case.
        if (isSupportedDisabled) {
            assertTrue(isSupportedEnabled);
        }
    }

    /**
     * Test that {@link WifiManager#is60GHzBandSupported()} returns successfully in
     * both Wifi enabled/disabled states.
     * Note that the response depends on device support and hence both true/false
     * are valid responses.
     */
    public void testIs60GhzBandSupported() throws Exception {
        if (!(WifiFeature.isWifiSupported(getContext())
                && ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S))) {
            // skip the test if WiFi is not supported
            return;
        }

        // Check for 60GHz support with wifi enabled
        setWifiEnabled(true);
        PollingCheck.check(
                "Wifi not enabled!",
                20000,
                () -> mWifiManager.isWifiEnabled());
        boolean isSupportedEnabled = mWifiManager.is60GHzBandSupported();

        // Check for 60GHz support with wifi disabled
        setWifiEnabled(false);
        PollingCheck.check(
                "Wifi not disabled!",
                20000,
                () -> !mWifiManager.isWifiEnabled());
        boolean isSupportedDisabled = mWifiManager.is60GHzBandSupported();

        // If Support is true when WiFi is disable, then it has to be true when it is enabled.
        // Note, the reverse is a valid case.
        if (isSupportedDisabled) {
            assertTrue(isSupportedEnabled);
        }
    }

    /**
     * Test that {@link WifiManager#isWifiStandardSupported()} returns successfully in
     * both Wifi enabled/disabled states. The test is to be performed on
     * {@link WifiAnnotations}'s {@code WIFI_STANDARD_}
     * Note that the response depends on device support and hence both true/false
     * are valid responses.
     */
    public void testIsWifiStandardsSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // Check for WiFi standards support with wifi enabled
        setWifiEnabled(true);
        PollingCheck.check(
                "Wifi not enabled!",
                20000,
                () -> mWifiManager.isWifiEnabled());
        boolean isLegacySupportedEnabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_LEGACY);
        boolean is11nSupporedEnabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_11N);
        boolean is11acSupportedEnabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_11AC);
        boolean is11axSupportedEnabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_11AX);

        // Check for WiFi standards support with wifi disabled
        setWifiEnabled(false);
        PollingCheck.check(
                "Wifi not disabled!",
                20000,
                () -> !mWifiManager.isWifiEnabled());

        boolean isLegacySupportedDisabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_LEGACY);
        boolean is11nSupportedDisabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_11N);
        boolean is11acSupportedDisabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_11AC);
        boolean is11axSupportedDisabled =
                mWifiManager.isWifiStandardSupported(ScanResult.WIFI_STANDARD_11AX);

        if (isLegacySupportedDisabled) {
            assertTrue(isLegacySupportedEnabled);
        }

        if (is11nSupportedDisabled) {
            assertTrue(is11nSupporedEnabled);
        }

        if (is11acSupportedDisabled) {
            assertTrue(is11acSupportedEnabled);
        }

        if (is11axSupportedDisabled) {
            assertTrue(is11axSupportedEnabled);
        }
    }

    private static PasspointConfiguration createPasspointConfiguration() {
        PasspointConfiguration config = new PasspointConfiguration();
        HomeSp homeSp = new HomeSp();
        homeSp.setFqdn("test.com");
        homeSp.setFriendlyName("friendly name");
        homeSp.setRoamingConsortiumOis(new long[]{0x55, 0x66});
        config.setHomeSp(homeSp);
        Credential.SimCredential simCred = new Credential.SimCredential();
        simCred.setImsi("123456*");
        simCred.setEapType(23 /* EAP_AKA */);
        Credential cred = new Credential();
        cred.setRealm("realm");
        cred.setSimCredential(simCred);
        config.setCredential(cred);

        return config;
    }

    /**
     * Tests {@link WifiManager#addOrUpdatePasspointConfiguration(PasspointConfiguration)}
     * adds a Passpoint configuration correctly by getting it once it is added, and comparing it
     * to the local copy of the configuration.
     */
    public void testAddOrUpdatePasspointConfiguration() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // Create and install a Passpoint configuration
        PasspointConfiguration passpointConfiguration = createPasspointConfiguration();
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.addOrUpdatePasspointConfiguration(passpointConfiguration);

            // Compare configurations
            List<PasspointConfiguration> configurations = mWifiManager.getPasspointConfigurations();
            assertNotNull("The installed passpoint profile is missing", configurations);
            assertEquals(passpointConfiguration, getTargetPasspointConfiguration(configurations,
                    passpointConfiguration.getUniqueId()));
        } finally {
            // Clean up
            mWifiManager.removePasspointConfiguration(passpointConfiguration.getHomeSp().getFqdn());
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#setPasspointMeteredOverride(String, int)}
     * adds a Passpoint configuration correctly, check the default metered setting. Use API change
     * metered override, verify Passpoint configuration changes with it.
     */
    public void testSetPasspointMeteredOverride() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Create and install a Passpoint configuration
        PasspointConfiguration passpointConfiguration = createPasspointConfiguration();
        String fqdn = passpointConfiguration.getHomeSp().getFqdn();
        String uniqueId = passpointConfiguration.getUniqueId();
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();

        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.addOrUpdatePasspointConfiguration(passpointConfiguration);
            PasspointConfiguration saved = getTargetPasspointConfiguration(
                    mWifiManager.getPasspointConfigurations(), uniqueId);
            assertNotNull("The installed passpoint profile is missing", saved);
            // Verify meter override setting.
            assertEquals("Metered overrider default should be none",
                    WifiConfiguration.METERED_OVERRIDE_NONE, saved.getMeteredOverride());
            // Change the meter override setting.
            mWifiManager.setPasspointMeteredOverride(fqdn,
                    WifiConfiguration.METERED_OVERRIDE_METERED);
            // Verify passpoint config change with the new setting.
            saved = getTargetPasspointConfiguration(
                    mWifiManager.getPasspointConfigurations(), uniqueId);
            assertNotNull("The installed passpoint profile is missing", saved);
            assertEquals("Metered override should be metered",
                    WifiConfiguration.METERED_OVERRIDE_METERED, saved.getMeteredOverride());
        } finally {
            // Clean up
            mWifiManager.removePasspointConfiguration(fqdn);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests that
     * {@link WifiManager#startSubscriptionProvisioning(OsuProvider, Executor, ProvisioningCallback)}
     * starts a subscription provisioning, and confirm a status callback invoked once.
     */
    public void testStartSubscriptionProvisioning() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // Using Java reflection to construct an OsuProvider instance because its constructor is
        // hidden and not available to apps.
        Class<?> osuProviderClass = Class.forName("android.net.wifi.hotspot2.OsuProvider");
        Constructor<?> osuProviderClassConstructor = osuProviderClass.getConstructor(String.class,
                Map.class, String.class, Uri.class, String.class, List.class);

        OsuProvider osuProvider = (OsuProvider) osuProviderClassConstructor.newInstance(TEST_SSID,
                TEST_FRIENDLY_NAMES, TEST_SERVICE_DESCRIPTION, TEST_SERVER_URI, TEST_NAI,
                TEST_METHOD_LIST);

        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            synchronized (mLock) {
                // Start a subscription provisioning for a non-existent Passpoint R2 AP
                mWifiManager.startSubscriptionProvisioning(osuProvider, mExecutor,
                        mProvisioningCallback);
                mLock.wait(TEST_WAIT_DURATION_MS);
            }
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }

        // Expect only a single callback event, connecting. Since AP doesn't exist, it ends here
        assertEquals(ProvisioningCallback.OSU_STATUS_AP_CONNECTING, mProvisioningStatus);
        // No failure callbacks expected
        assertEquals(0, mProvisioningFailureStatus);
        // No completion callback expected
        assertFalse(mProvisioningComplete);
    }

    /**
     * Tests {@link WifiManager#setTdlsEnabled(InetAddress, boolean)} does not crash.
     */
    public void testSetTdlsEnabled() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Trigger a scan & wait for connection to one of the saved networks.
        mWifiManager.startScan();
        waitForConnection();

        InetAddress inetAddress = InetAddress.getByName(TEST_IP_ADDRESS);

        mWifiManager.setTdlsEnabled(inetAddress, true);
        Thread.sleep(50);
        mWifiManager.setTdlsEnabled(inetAddress, false);
    }

    /**
     * Tests {@link WifiManager#setTdlsEnabledWithMacAddress(String, boolean)} does not crash.
     */
    public void testSetTdlsEnabledWithMacAddress() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Trigger a scan & wait for connection to one of the saved networks.
        mWifiManager.startScan();
        waitForConnection();

        mWifiManager.setTdlsEnabledWithMacAddress(TEST_MAC_ADDRESS, true);
        Thread.sleep(50);
        mWifiManager.setTdlsEnabledWithMacAddress(TEST_MAC_ADDRESS, false);
    }

    /**
     * Verify WifiNetworkSuggestion.Builder.setMacRandomizationSetting(WifiNetworkSuggestion
     * .RANDOMIZATION_NON_PERSISTENT) creates a
     * WifiConfiguration with macRandomizationSetting == RANDOMIZATION_NON_PERSISTENT.
     * Then verify by default, a WifiConfiguration created by suggestions should have
     * macRandomizationSetting == RANDOMIZATION_PERSISTENT.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testSuggestionBuilderNonPersistentRandomization() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        WifiNetworkSuggestion suggestion = new WifiNetworkSuggestion.Builder()
                .setSsid(TEST_SSID).setWpa2Passphrase(TEST_PASSPHRASE)
                .setMacRandomizationSetting(WifiNetworkSuggestion.RANDOMIZATION_NON_PERSISTENT)
                .build();
        assertEquals(WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS,
                mWifiManager.addNetworkSuggestions(Arrays.asList(suggestion)));
        verifySuggestionFoundWithMacRandomizationSetting(TEST_SSID,
                WifiConfiguration.RANDOMIZATION_NON_PERSISTENT);

        suggestion = new WifiNetworkSuggestion.Builder()
                .setSsid(TEST_SSID).setWpa2Passphrase(TEST_PASSPHRASE)
                .build();
        assertEquals(WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS,
                mWifiManager.addNetworkSuggestions(Arrays.asList(suggestion)));
        verifySuggestionFoundWithMacRandomizationSetting(TEST_SSID,
                WifiConfiguration.RANDOMIZATION_PERSISTENT);
    }

    private void verifySuggestionFoundWithMacRandomizationSetting(String ssid,
            int macRandomizationSetting) {
        List<WifiNetworkSuggestion> retrievedSuggestions = mWifiManager.getNetworkSuggestions();
        for (WifiNetworkSuggestion entry : retrievedSuggestions) {
            if (entry.getSsid().equals(ssid)) {
                assertEquals(macRandomizationSetting,
                        entry.getWifiConfiguration().macRandomizationSetting);
                return; // pass test after the MAC randomization setting is verified.
            }
        }
        fail("WifiNetworkSuggestion not found for SSID=" + ssid + ", macRandomizationSetting="
                + macRandomizationSetting);
    }

    /**
     * Tests {@link WifiManager#getWifiConfigForMatchedNetworkSuggestionsSharedWithUser(List)}
     */
    public void testGetAllWifiConfigForMatchedNetworkSuggestion() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = TEST_SSID;
        scanResult.capabilities = TEST_PSK_CAP;
        scanResult.BSSID = TEST_BSSID;
        List<ScanResult> testList = Arrays.asList(scanResult);
        WifiNetworkSuggestion suggestion = new WifiNetworkSuggestion.Builder()
                .setSsid(TEST_SSID).setWpa2Passphrase(TEST_PASSPHRASE).build();

        assertEquals(WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS,
                mWifiManager.addNetworkSuggestions(Arrays.asList(suggestion)));
        List<WifiConfiguration> matchedResult;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            matchedResult = mWifiManager
                    .getWifiConfigForMatchedNetworkSuggestionsSharedWithUser(testList);
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
        // As suggestion is not approved, will return empty list.
        assertTrue(matchedResult.isEmpty());
    }

    /**
     * Tests {@link WifiManager#getMatchingScanResults(List, List)}
     */
    public void testGetMatchingScanResults() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Create pair of ScanResult and WifiNetworkSuggestion
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = TEST_SSID;
        scanResult.capabilities = TEST_PSK_CAP;
        scanResult.BSSID = TEST_BSSID;

        WifiNetworkSuggestion suggestion = new WifiNetworkSuggestion.Builder()
                .setSsid(TEST_SSID).setWpa2Passphrase(TEST_PASSPHRASE).build();

        Map<WifiNetworkSuggestion, List<ScanResult>> matchedResults = mWifiManager
                .getMatchingScanResults(Arrays.asList(suggestion), Arrays.asList(scanResult));
        // Verify result is matched pair of ScanResult and WifiNetworkSuggestion
        assertEquals(scanResult.SSID, matchedResults.get(suggestion).get(0).SSID);

        // Change ScanResult to unmatched should return empty result.
        scanResult.SSID = TEST_SSID_UNQUOTED;
        matchedResults = mWifiManager
                .getMatchingScanResults(Arrays.asList(suggestion), Arrays.asList(scanResult));
        assertTrue(matchedResults.get(suggestion).isEmpty());
    }

    /**
     * Tests {@link WifiManager#disableEphemeralNetwork(String)}.
     */
    public void testDisableEphemeralNetwork() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Trigger a scan & wait for connection to one of the saved networks.
        mWifiManager.startScan();
        waitForConnection();

        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        List<WifiConfiguration> savedNetworks = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Temporarily disable on all networks.
            savedNetworks = mWifiManager.getConfiguredNetworks();
            for (WifiConfiguration network : savedNetworks) {
                mWifiManager.disableEphemeralNetwork(network.SSID);
            }
            // trigger a disconnect and wait for disconnect.
            mWifiManager.disconnect();
            waitForDisconnection();

            // Now trigger scan and ensure that the device does not connect to any networks.
            mWifiManager.startScan();
            ensureNotConnected();
        } finally {
            uiAutomation.dropShellPermissionIdentity();
            setWifiEnabled(false);
        }
    }

    /**
     * Tests {@link WifiManager#allowAutojoin(int, boolean)}.
     */
    public void testAllowAutojoin() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Trigger a scan & wait for connection to one of the saved networks.
        mWifiManager.startScan();
        waitForConnection();

        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        List<WifiConfiguration> savedNetworks = null;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // disable autojoin on all networks.
            savedNetworks = mWifiManager.getConfiguredNetworks();
            for (WifiConfiguration network : savedNetworks) {
                mWifiManager.allowAutojoin(network.networkId, false);
            }
            // trigger a disconnect and wait for disconnect.
            mWifiManager.disconnect();
            waitForDisconnection();

            // Now trigger scan and ensure that the device does not connect to any networks.
            mWifiManager.startScan();
            ensureNotConnected();

            // Now enable autojoin on all networks.
            for (WifiConfiguration network : savedNetworks) {
                mWifiManager.allowAutojoin(network.networkId, true);
            }

            // Trigger a scan & wait for connection to one of the saved networks.
            mWifiManager.startScan();
            waitForConnection();
        } finally {
            // Restore auto join state.
            if (savedNetworks != null) {
                for (WifiConfiguration network : savedNetworks) {
                    mWifiManager.allowAutojoin(network.networkId, network.allowAutojoin);
                }
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#allowAutojoinPasspoint(String, boolean)}.
     */
    public void testAllowAutojoinPasspoint() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        PasspointConfiguration passpointConfiguration = createPasspointConfiguration();
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.addOrUpdatePasspointConfiguration(passpointConfiguration);
            // Turn off auto-join
            mWifiManager.allowAutojoinPasspoint(
                    passpointConfiguration.getHomeSp().getFqdn(), false);
            // Turn on auto-join
            mWifiManager.allowAutojoinPasspoint(
                    passpointConfiguration.getHomeSp().getFqdn(), true);
        } finally {
            mWifiManager.removePasspointConfiguration(passpointConfiguration.getHomeSp().getFqdn());
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#allowAutojoinGlobal(boolean)}.
     */
    public void testAllowAutojoinGlobal() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // Trigger a scan & wait for connection to one of the saved networks.
        mWifiManager.startScan();
        waitForConnection();

        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // disable autojoin on all networks.
            mWifiManager.allowAutojoinGlobal(false);

            // trigger a disconnect and wait for disconnect.
            mWifiManager.disconnect();
            waitForDisconnection();

            // Now trigger scan and ensure that the device does not connect to any networks.
            mWifiManager.startScan();
            ensureNotConnected();

            // Now enable autojoin on all networks.
            mWifiManager.allowAutojoinGlobal(true);

            // Trigger a scan & wait for connection to one of the saved networks.
            mWifiManager.startScan();
            waitForConnection();
        } finally {
            // Re-enable auto join if the test fails for some reason.
            mWifiManager.allowAutojoinGlobal(true);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#isWapiSupported()} does not crash.
     */
    public void testIsWapiSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isWapiSupported();
    }

    /**
     * Tests {@link WifiManager#isWpa3SaePublicKeySupported()} does not crash.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testIsWpa3SaePublicKeySupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isWpa3SaePublicKeySupported();
    }

    /**
     * Tests {@link WifiManager#isWpa3SaeH2eSupported()} does not crash.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testIsWpa3SaeH2eSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isWpa3SaeH2eSupported();
    }

    /**
     * Tests {@link WifiManager#isWifiDisplayR2Supported()} does not crash.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testIsWifiDisplayR2Supported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        mWifiManager.isWifiDisplayR2Supported();
    }

    /**
     * Tests {@link WifiManager#isP2pSupported()} returns true
     * if this device supports it, otherwise, ensure no crash.
     */
    public void testIsP2pSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        if (WifiFeature.isP2pSupported(getContext())) {
            // if this device supports P2P, ensure hw capability is correct.
            assertTrue(mWifiManager.isP2pSupported());
        } else {
            // ensure no crash.
            mWifiManager.isP2pSupported();
        }

    }

    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testIsMultiStaConcurrencySupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // ensure no crash.
        mWifiManager.isStaApConcurrencySupported();
    }

    private PasspointConfiguration getTargetPasspointConfiguration(
            List<PasspointConfiguration> configurationList, String uniqueId) {
        if (configurationList == null || configurationList.isEmpty()) {
            return null;
        }
        for (PasspointConfiguration config : configurationList) {
            if (TextUtils.equals(config.getUniqueId(), uniqueId)) {
                return config;
            }
        }
        return null;
    }

    /**
     * Test that {@link WifiManager#is60GHzBandSupported()} throws UnsupportedOperationException
     * if the release is older than S.
     */
    @SdkSuppress(maxSdkVersion = Build.VERSION_CODES.R)
    public void testIs60GhzBandSupportedOnROrOlder() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // check for 60ghz support with wifi enabled
        try {
            boolean isSupported = mWifiManager.is60GHzBandSupported();
            fail("Expected UnsupportedOperationException");
        } catch (UnsupportedOperationException ex) {
        }

    }

    /**
     * Test that {@link WifiManager#is60GHzBandSupported()} returns successfully in
     * both Wifi enabled/disabled states for release newer than R.
     * Note that the response depends on device support and hence both true/false
     * are valid responses.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testIs60GhzBandSupportedOnSOrNewer() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // check for 60ghz support with wifi enabled
        boolean isSupportedWhenWifiEnabled = mWifiManager.is60GHzBandSupported();

        // Check for 60GHz support with wifi disabled
        setWifiEnabled(false);
        PollingCheck.check(
                "Wifi not disabled!",
                20000,
                () -> !mWifiManager.isWifiEnabled());
        boolean isSupportedWhenWifiDisabled = mWifiManager.is60GHzBandSupported();

        // If Support is true when WiFi is disable, then it has to be true when it is enabled.
        // Note, the reverse is a valid case.
        if (isSupportedWhenWifiDisabled) {
            assertTrue(isSupportedWhenWifiEnabled);
        }
    }

    public class TestCoexCallback extends WifiManager.CoexCallback {
        private Object mCoexLock;
        private int mOnCoexUnsafeChannelChangedCount;
        private List<CoexUnsafeChannel> mCoexUnsafeChannels;
        private int mCoexRestrictions;

        TestCoexCallback(Object lock) {
            mCoexLock = lock;
        }

        @Override
        public void onCoexUnsafeChannelsChanged(
                    @NonNull List<CoexUnsafeChannel> unsafeChannels, int restrictions) {
            synchronized (mCoexLock) {
                mCoexUnsafeChannels = unsafeChannels;
                mCoexRestrictions = restrictions;
                mOnCoexUnsafeChannelChangedCount++;
                mCoexLock.notify();
            }
        }

        public int getOnCoexUnsafeChannelChangedCount() {
            synchronized (mCoexLock) {
                return mOnCoexUnsafeChannelChangedCount;
            }
        }

        public List<CoexUnsafeChannel> getCoexUnsafeChannels() {
            return mCoexUnsafeChannels;
        }

        public int getCoexRestrictions() {
            return mCoexRestrictions;
        }
    }

    /**
     * Test that coex-related methods fail without the needed privileged permissions
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testCoexMethodsShouldFailNoPermission() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        try {
            mWifiManager.setCoexUnsafeChannels(Collections.emptyList(), 0);
            fail("setCoexUnsafeChannels should not succeed - privileged call");
        } catch (SecurityException e) {
            // expected
        }
        final TestCoexCallback callback = new TestCoexCallback(mLock);
        try {
            mWifiManager.registerCoexCallback(mExecutor, callback);
            fail("registerCoexCallback should not succeed - privileged call");
        } catch (SecurityException e) {
            // expected
        }
        try {
            mWifiManager.unregisterCoexCallback(callback);
            fail("unregisterCoexCallback should not succeed - privileged call");
        } catch (SecurityException e) {
            // expected
        }
    }

    /**
     * Test that coex-related methods succeed in setting the current unsafe channels and notifying
     * the listener. Since the default coex algorithm may be enabled, no-op is also valid behavior.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testListenOnCoexUnsafeChannels() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }

        // These below API's only work with privileged permissions (obtained via shell identity
        // for test)
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        List<CoexUnsafeChannel> prevUnsafeChannels = null;
        int prevRestrictions = -1;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            final TestCoexCallback callback = new TestCoexCallback(mLock);
            final List<CoexUnsafeChannel> testUnsafeChannels = new ArrayList<>();
            testUnsafeChannels.add(new CoexUnsafeChannel(WIFI_BAND_24_GHZ, 6));
            final int testRestrictions = COEX_RESTRICTION_WIFI_DIRECT
                    | COEX_RESTRICTION_SOFTAP | COEX_RESTRICTION_WIFI_AWARE;
            synchronized (mLock) {
                try {
                    mWifiManager.registerCoexCallback(mExecutor, callback);
                    // Callback should be called after registering
                    mLock.wait(TEST_WAIT_DURATION_MS);
                    assertEquals(1, callback.getOnCoexUnsafeChannelChangedCount());
                    // Store the previous coex channels and set new coex channels
                    prevUnsafeChannels = callback.getCoexUnsafeChannels();
                    prevRestrictions = callback.getCoexRestrictions();
                    mWifiManager.setCoexUnsafeChannels(testUnsafeChannels, testRestrictions);
                    mLock.wait(TEST_WAIT_DURATION_MS);
                    // Unregister callback and try setting again
                    mWifiManager.unregisterCoexCallback(callback);
                    mWifiManager.setCoexUnsafeChannels(testUnsafeChannels, testRestrictions);
                    // Callback should not be called here since it was unregistered.
                    mLock.wait(TEST_WAIT_DURATION_MS);
                } catch (InterruptedException e) {
                    fail("Thread interrupted unexpectedly while waiting on mLock");
                }
            }
            if (callback.getOnCoexUnsafeChannelChangedCount() == 2) {
                // Default algorithm disabled, setter should set the getter values.
                assertEquals(testUnsafeChannels, callback.getCoexUnsafeChannels());
                assertEquals(testRestrictions, callback.getCoexRestrictions());
            } else if (callback.getOnCoexUnsafeChannelChangedCount() != 1) {
                fail("Coex callback called " + callback.mOnCoexUnsafeChannelChangedCount
                        + " times. Expected 0 or 1 calls." );
            }
        } finally {
            // Reset the previous unsafe channels if we overrode them.
            if (prevRestrictions != -1) {
                mWifiManager.setCoexUnsafeChannels(prevUnsafeChannels, prevRestrictions);
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }


    /**
     * Verify that insecure WPA-Enterprise network configurations are rejected.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testInsecureEnterpriseConfigurationsRejected() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.SSID = SSID1;
        wifiConfiguration.setSecurityParams(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE);
        wifiConfiguration.enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.TTLS);
        int networkId = INVALID_NETWORK_ID;

        // These below API's only work with privileged permissions (obtained via shell identity
        // for test)
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();

            // Verify that an insecure network is rejected
            assertEquals(INVALID_NETWORK_ID, mWifiManager.addNetwork(wifiConfiguration));

            // Now configure it correctly with a Root CA cert and domain name
            wifiConfiguration.enterpriseConfig.setCaCertificate(FakeKeys.CA_CERT0);
            wifiConfiguration.enterpriseConfig.setAltSubjectMatch(TEST_DOM_SUBJECT_MATCH);

            // Verify that the network is added
            networkId = mWifiManager.addNetwork(wifiConfiguration);
            assertNotEquals(INVALID_NETWORK_ID, networkId);

            // Verify that the update API accepts configurations configured securely
            wifiConfiguration.networkId = networkId;
            assertEquals(networkId, mWifiManager.updateNetwork(wifiConfiguration));

            // Now clear the security configuration
            wifiConfiguration.enterpriseConfig.setCaCertificate(null);
            wifiConfiguration.enterpriseConfig.setAltSubjectMatch(null);

            // Verify that the update API rejects insecure configurations
            assertEquals(INVALID_NETWORK_ID, mWifiManager.updateNetwork(wifiConfiguration));
        } finally {
            if (networkId != INVALID_NETWORK_ID) {
                // Clean up the previously added network
                mWifiManager.removeNetwork(networkId);
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#isPasspointTermsAndConditionsSupported)} does not crash.
     */
    public void testIsPasspointTermsAndConditionsSupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        mWifiManager.isPasspointTermsAndConditionsSupported();
    }

    /**
     * Test that call to {@link WifiManager#setOverrideCountryCode()},
     * {@link WifiManager#clearOverrideCountryCode()} and
     * {@link WifiManager#setDefaultCountryCode()} need privileged permission
     * and the permission is not even given to shell user.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testManageCountryCodeMethodsFailWithoutPermissions() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        ShellIdentityUtils.invokeWithShellPermissions(() -> {
            try {
                mWifiManager.setOverrideCountryCode(TEST_COUNTRY_CODE);
                fail("setOverrideCountryCode() expected to fail - privileged call");
            } catch (SecurityException e) {
                // expected
            }

            try {
                mWifiManager.clearOverrideCountryCode();
                fail("clearOverrideCountryCode() expected to fail - privileged call");
            } catch (SecurityException e) {
                // expected
            }

            try {
                mWifiManager.setDefaultCountryCode(TEST_COUNTRY_CODE);
                fail("setDefaultCountryCode() expected to fail - privileged call");
            } catch (SecurityException e) {
                // expected
            }
        });
    }

    /**
     * Tests {@link WifiManager#flushPasspointAnqpCache)} does not crash.
     */
    public void testFlushPasspointAnqpCache() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        // The below API only works with privileged permissions (obtained via shell identity
        // for test)
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.flushPasspointAnqpCache();
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#isDecoratedIdentitySupported)} does not crash.
     */
    public void testIsDecoratedIdentitySupported() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        if (!WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            // Skip the test if wifi module version is older than S.
            return;
        }
        mWifiManager.isDecoratedIdentitySupported();
    }

    /**
     * Tests {@link WifiManager#setCarrierNetworkOffloadEnabled)} and
     * {@link WifiManager#isCarrierNetworkOffloadEnabled} work as expected.
     */
    public void testSetCarrierNetworkOffloadEnabled() {
        if (!WifiFeature.isWifiSupported(getContext())
                || !WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        assertTrue(mWifiManager.isCarrierNetworkOffloadEnabled(TEST_SUB_ID, false));
        // The below API only works with privileged permissions (obtained via shell identity
        // for test)
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.setCarrierNetworkOffloadEnabled(TEST_SUB_ID, false, false);
            assertFalse(mWifiManager.isCarrierNetworkOffloadEnabled(TEST_SUB_ID, false));
        } finally {
            mWifiManager.setCarrierNetworkOffloadEnabled(TEST_SUB_ID, false, true);
            uiAutomation.dropShellPermissionIdentity();
        }
        assertTrue(mWifiManager.isCarrierNetworkOffloadEnabled(TEST_SUB_ID, false));
    }

   /**
     * Test that {@link WifiManager#getUsableChannels(int, int)},
     * {@link WifiManager#getAllowedChannels(int, int)}
     * throws UnsupportedOperationException if the release is older than S.
     */
    @SdkSuppress(maxSdkVersion = Build.VERSION_CODES.R)
    public void testGetAllowedUsableChannelsOnROrOlder() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        try {
            mWifiManager.getAllowedChannels(WIFI_BAND_24_GHZ, OP_MODE_STA);
            fail("getAllowedChannels Expected to fail - UnsupportedOperationException");
        } catch (UnsupportedOperationException ex) {}

        try {
            mWifiManager.getUsableChannels(WIFI_BAND_24_GHZ, OP_MODE_STA);
            fail("getUsableChannels Expected to fail - UnsupportedOperationException");
        } catch (UnsupportedOperationException ex) {}
    }

    /**
     * Tests {@link WifiManager#getAllowedChannels(int, int))} does not crash
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testGetAllowedChannels() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // The below API only works with privileged permissions (obtained via shell identity
        // for test)
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {

            WifiAvailableChannel channel = new WifiAvailableChannel(2412, OP_MODE_SAP);
            assertEquals(channel.getFrequencyMhz(), 2412);
            assertEquals(channel.getOperationalModes(), OP_MODE_SAP);
            final List<Integer> valid24GhzFreqs = Arrays.asList(
                2412, 2417, 2422, 2427, 2432, 2437, 2442,
                2447, 2452, 2457, 2462, 2467, 2472, 2484);
            Set<Integer> supported24GhzFreqs = new HashSet<Integer>();
            uiAutomation.adoptShellPermissionIdentity();
            List<WifiAvailableChannel> allowedChannels =
                mWifiManager.getAllowedChannels(WIFI_BAND_24_GHZ, OP_MODE_STA);
            assertNotNull(allowedChannels);
            for (WifiAvailableChannel ch : allowedChannels) {
                //Must contain a valid 2.4GHz frequency
                assertTrue(valid24GhzFreqs.contains(ch.getFrequencyMhz()));
                if(ch.getFrequencyMhz() <= 2462) {
                    //Channels 1-11 are supported for STA in all countries
                    assertEquals(ch.getOperationalModes() & OP_MODE_STA, OP_MODE_STA);
                    supported24GhzFreqs.add(ch.getFrequencyMhz());
                }
            }
            //Channels 1-11 are supported for STA in all countries
            assertEquals(supported24GhzFreqs.size(), 11);
        } catch (UnsupportedOperationException ex) {
            //expected if the device does not support this API
        } catch (Exception ex) {
            fail("getAllowedChannels unexpected Exception " + ex);
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests {@link WifiManager#getUsableChannels(int, int))} does not crash.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testGetUsableChannels() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        // The below API only works with privileged permissions (obtained via shell identity
        // for test)
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            List<WifiAvailableChannel> usableChannels =
                mWifiManager.getUsableChannels(WIFI_BAND_24_GHZ, OP_MODE_STA);
            //There must be at least one usable channel in 2.4GHz band
            assertFalse(usableChannels.isEmpty());
        } catch (UnsupportedOperationException ex) {
            //expected if the device does not support this API
        } catch (Exception ex) {
            fail("getUsableChannels unexpected Exception " + ex);
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Validate that the Passpoint feature is enabled on the device.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    public void testPasspointCapability() {
        if (!WifiFeature.isWifiSupported(getContext())) {
            // skip the test if WiFi is not supported
            return;
        }
        PackageManager packageManager = mContext.getPackageManager();
        assertTrue("Passpoint must be supported",
                packageManager.hasSystemFeature(PackageManager.FEATURE_WIFI_PASSPOINT));
    }

    /**
     * Validate add and remove SuggestionUserApprovalStatusListener. And verify the listener's
     * stickiness.
     */
    public void testAddRemoveSuggestionUserApprovalStatusListener() throws Exception {
        if (!WifiFeature.isWifiSupported(getContext())
                || !WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            return;
        }
        CountDownLatch countDownLatch = new CountDownLatch(1);
        TestUserApprovalStatusListener listener = new TestUserApprovalStatusListener(
                countDownLatch);
        try {
            mWifiManager.addSuggestionUserApprovalStatusListener(mExecutor, listener);
            assertTrue(countDownLatch.await(TEST_WAIT_DURATION_MS, TimeUnit.MILLISECONDS));
        } finally {
            mWifiManager.removeSuggestionUserApprovalStatusListener(listener);
        }
    }

    private static class TestUserApprovalStatusListener implements
            WifiManager.SuggestionUserApprovalStatusListener {
        private final CountDownLatch mBlocker;

        public TestUserApprovalStatusListener(CountDownLatch countDownLatch) {
            mBlocker = countDownLatch;
        }
        @Override
        public void onUserApprovalStatusChange(int status) {
            mBlocker.countDown();
        }
    }
}
