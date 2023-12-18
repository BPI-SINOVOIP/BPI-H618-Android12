/*
 * Copyright (C) 2021 The Android Open Source Project
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

import static android.net.NetworkCapabilities.NET_CAPABILITY_OEM_PAID;
import static android.net.NetworkCapabilities.NET_CAPABILITY_OEM_PRIVATE;
import static android.os.Process.myUid;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assume.assumeTrue;

import android.content.Context;
import android.content.pm.PackageManager;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSuggestion;
import android.os.Build;
import android.platform.test.annotations.AppModeFull;
import android.support.test.uiautomator.UiDevice;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.ShellIdentityUtils;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

/**
 * Tests multiple concurrent connection flow on devices that support multi STA concurrency
 * (indicated via {@link WifiManager#isStaConcurrencyForRestrictedConnectionsSupported()}.
 *
 * Tests the entire connection flow using {@link WifiNetworkSuggestion} which has
 * {@link WifiNetworkSuggestion.Builder#setOemPaid(boolean)} or
 * {@link WifiNetworkSuggestion.Builder#setOemPrivate(boolean)} set along with a concurrent internet
 * connection using {@link WifiManager#connect(int, WifiManager.ActionListener)}.
 *
 * Assumes that all the saved networks is either open/WPA1/WPA2/WPA3 authenticated network.
 */
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
@AppModeFull(reason = "Cannot get WifiManager in instant app mode")
@LargeTest
@RunWith(AndroidJUnit4.class)
public class MultiStaConcurrencyRestrictedWifiNetworkSuggestionTest extends WifiJUnit4TestBase {
    private static final String TAG = "MultiStaConcurrencyRestrictedWifiNetworkSuggestionTest";
    private static boolean sWasVerboseLoggingEnabled;
    private static boolean sWasScanThrottleEnabled;
    private static boolean sWasWifiEnabled;

    private Context mContext;
    private WifiManager mWifiManager;
    private ConnectivityManager mConnectivityManager;
    private UiDevice mUiDevice;
    private WifiConfiguration mTestNetworkForRestrictedConnection;
    private WifiConfiguration mTestNetworkForInternetConnection;
    private ConnectivityManager.NetworkCallback mNetworkCallback;
    private ConnectivityManager.NetworkCallback mNsNetworkCallback;
    private ScheduledExecutorService mExecutorService;
    private TestHelper mTestHelper;

    private static final int DURATION_MILLIS = 10_000;

    @BeforeClass
    public static void setUpClass() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        // skip the test if WiFi is not supported or not automotive platform.
        // Don't use assumeTrue in @BeforeClass
        if (!WifiFeature.isWifiSupported(context)) return;

        WifiManager wifiManager = context.getSystemService(WifiManager.class);
        assertThat(wifiManager).isNotNull();

        // turn on verbose logging for tests
        sWasVerboseLoggingEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.isVerboseLoggingEnabled());
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.setVerboseLoggingEnabled(true));
        // Disable scan throttling for tests.
        sWasScanThrottleEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.isScanThrottleEnabled());
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.setScanThrottleEnabled(false));

        // enable Wifi
        sWasWifiEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.isWifiEnabled());
        if (!wifiManager.isWifiEnabled()) {
            ShellIdentityUtils.invokeWithShellPermissions(() -> wifiManager.setWifiEnabled(true));
        }
        PollingCheck.check("Wifi not enabled", DURATION_MILLIS, () -> wifiManager.isWifiEnabled());
    }

    @AfterClass
    public static void tearDownClass() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        if (!WifiFeature.isWifiSupported(context)) return;

        WifiManager wifiManager = context.getSystemService(WifiManager.class);
        assertThat(wifiManager).isNotNull();

        ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.setScanThrottleEnabled(sWasScanThrottleEnabled));
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.setVerboseLoggingEnabled(sWasVerboseLoggingEnabled));
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> wifiManager.setWifiEnabled(sWasWifiEnabled));
    }

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getInstrumentation().getContext();
        mWifiManager = mContext.getSystemService(WifiManager.class);
        mConnectivityManager = mContext.getSystemService(ConnectivityManager.class);
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mExecutorService = Executors.newSingleThreadScheduledExecutor();
        mTestHelper = new TestHelper(mContext, mUiDevice);

        // skip the test if WiFi is not supported or not automitve platform.
        assumeTrue(WifiFeature.isWifiSupported(mContext));
        // skip the test if location is not supported
        assumeTrue(mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LOCATION));
        // skip if multi STA not supported.
        assumeTrue(mWifiManager.isStaConcurrencyForRestrictedConnectionsSupported());

        assertWithMessage("Please enable location for this test!").that(
                mContext.getSystemService(LocationManager.class).isLocationEnabled()).isTrue();

        // turn screen on
        mTestHelper.turnScreenOn();

        // Clear any existing app state before each test.
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.removeAppState(myUid(), mContext.getPackageName()));

        // We need 2 AP's for the test. If there are 2 networks saved on the device and in range,
        // use those. Otherwise, check if there are 2 BSSID's in range for the only saved network.
        // This assumes a CTS test environment with at least 2 connectable bssid's (Is that ok?).
        List<WifiConfiguration> savedNetworks = ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.getPrivilegedConfiguredNetworks());
        List<WifiConfiguration> matchingNetworksWithBssid =
                TestHelper.findMatchingSavedNetworksWithBssid(mWifiManager, savedNetworks);
        assertWithMessage("Need at least 2 saved network bssids in range").that(
                matchingNetworksWithBssid.size()).isAtLeast(2);
        // Pick any 2 bssid for test.
        mTestNetworkForRestrictedConnection = matchingNetworksWithBssid.get(0);
        // Try to find a bssid for another saved network in range. If none exists, fallback
        // to using 2 bssid's for the same network.
        mTestNetworkForInternetConnection = matchingNetworksWithBssid.stream()
                .filter(w -> !w.SSID.equals(mTestNetworkForRestrictedConnection.SSID))
                .findAny()
                .orElse(matchingNetworksWithBssid.get(1));

        // Disconnect & disable auto-join on the saved network to prevent auto-connect from
        // interfering with the test.
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> {
                    for (WifiConfiguration savedNetwork : savedNetworks) {
                        mWifiManager.disableNetwork(savedNetwork.networkId);
                    }
                    mWifiManager.disconnect();
                });

        // Wait for Wifi to be disconnected.
        PollingCheck.check(
                "Wifi not disconnected",
                20_000,
                () -> mWifiManager.getConnectionInfo().getNetworkId() == -1);
    }

    @After
    public void tearDown() throws Exception {
        // Re-enable networks.
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> {
                    for (WifiConfiguration savedNetwork : mWifiManager.getConfiguredNetworks()) {
                        mWifiManager.enableNetwork(savedNetwork.networkId, false);
                    }
                });
        // Release the requests after the test.
        if (mNetworkCallback != null) {
            mConnectivityManager.unregisterNetworkCallback(mNetworkCallback);
        }
        if (mNsNetworkCallback != null) {
            mConnectivityManager.unregisterNetworkCallback(mNsNetworkCallback);
        }
        mExecutorService.shutdownNow();
        // Clear any existing app state after each test.
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.removeAppState(myUid(), mContext.getPackageName()));
        mTestHelper.turnScreenOff();
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using internet connectivity API.
     * 2. Connect to a network using restricted suggestion API.
     * 3. Verify that both connections are active.
     */
    @Test
    public void testConnectToOemPaidSuggestionWhenConnectedToInternetNetwork() throws Exception {
        // First trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Now trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .setOemPaid(true)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PAID));

        // Ensure that there are 2 wifi connections available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(2);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using restricted suggestion API.
     * 2. Connect to a network using internet connectivity API.
     * 3. Verify that both connections are active.
     */
    @Test
    public void testConnectToInternetNetworkWhenConnectedToOemPaidSuggestion() throws Exception {
        // First trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .setOemPaid(true)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PAID));

        // Now trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Ensure that there are 2 wifi connections available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(2);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using internet connectivity API.
     * 2. Connect to a network using restricted suggestion API.
     * 3. Verify that both connections are active.
     */
    @Test
    public void testConnectToOemPrivateSuggestionWhenConnectedToInternetNetwork() throws Exception {
        // First trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Now trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .setOemPrivate(true)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PRIVATE));

        // Ensure that there are 2 wifi connections available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(2);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using restricted suggestion API.
     * 2. Connect to a network using internet connectivity API.
     * 3. Verify that both connections are active.
     */
    @Test
    public void testConnectToInternetNetworkWhenConnectedToOemPrivateSuggestion() throws Exception {
        // First trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .setOemPrivate(true)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PRIVATE));

        // Now trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Ensure that there are 2 wifi connections available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(2);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using internet connectivity API.
     * 2. Simulate connection failure to a network using restricted suggestion API & different net
     *    capability (need corresponding net capability requested for platform to connect).
     * 3. Verify that only 1 connection is active.
     */
    @Test
    public void testConnectToOemPaidSuggestionFailureWhenConnectedToInternetNetwork()
            throws Exception {
        // First trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Now trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .setOemPaid(true)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFailureFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PRIVATE));

        // Ensure that there is only 1 connection available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(1);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using internet connectivity API.
     * 2. Simulate connection failure to a network using restricted suggestion API & different net
     *    capability (need corresponding net capability requested for platform to connect).
     * 3. Verify that only 1 connection is active.
     */
    @Test
    public void testConnectToOemPrivateSuggestionFailureWhenConnectedToInternetNetwork()
            throws Exception {
        // First trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Now trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .setOemPrivate(true)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFailureFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PAID));

        // Ensure that there is only 1 connection available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(1);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using internet connectivity API.
     * 2. Simulate connection failure to a restricted network using suggestion API & restricted net
     *    capability (need corresponding restricted bit set in suggestion for platform to connect).
     * 3. Verify that only 1 connection is active.
     */
    @Test
    public void
            testConnectToSuggestionFailureWithOemPaidNetCapabilityWhenConnectedToInternetNetwork()
            throws Exception {
        // First trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Now trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFailureFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PAID));

        // Ensure that there is only 1 connection available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(1);
    }

    /**
     * Tests the concurrent connection flow.
     * 1. Connect to a network using internet connectivity API.
     * 2. Simulate connection failure to a restricted network using suggestion API & restricted net
     *    capability (need corresponding restricted bit set in suggestion for platform to connect).
     * 3. Verify that only 1 connection is active.
     */
    @Test
    public void
        testConnectToSuggestionFailureWithOemPrivateNetCapabilityWhenConnectedToInternetNetwork()
            throws Exception {
        // First trigger internet connectivity.
        mNetworkCallback = mTestHelper.testConnectionFlowWithConnect(
                mTestNetworkForInternetConnection);

        // Now trigger restricted connection.
        WifiNetworkSuggestion suggestion =
                TestHelper.createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                        mTestNetworkForRestrictedConnection)
                        .build();
        mNsNetworkCallback = mTestHelper.testConnectionFailureFlowWithSuggestion(
                mTestNetworkForRestrictedConnection, suggestion, mExecutorService,
                Set.of(NET_CAPABILITY_OEM_PRIVATE));

        // Ensure that there is only 1 connection available for apps.
        assertThat(mTestHelper.getNumWifiConnections()).isEqualTo(1);
    }
}
