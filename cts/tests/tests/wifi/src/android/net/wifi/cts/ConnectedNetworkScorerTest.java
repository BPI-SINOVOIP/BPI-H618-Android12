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

package android.net.wifi.cts;

import static android.Manifest.permission.CONNECTIVITY_INTERNAL;
import static android.Manifest.permission.NETWORK_SETTINGS;
import static android.Manifest.permission.READ_WIFI_CREDENTIAL;
import static android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE;
import static android.net.NetworkCapabilities.NET_CAPABILITY_OEM_PAID;
import static android.net.NetworkCapabilities.NET_CAPABILITY_OEM_PRIVATE;
import static android.net.wifi.WifiUsabilityStatsEntry.ContentionTimeStats;
import static android.net.wifi.WifiUsabilityStatsEntry.RadioStats;
import static android.net.wifi.WifiUsabilityStatsEntry.RateStats;
import static android.net.wifi.WifiUsabilityStatsEntry.PROBE_STATUS_FAILURE;
import static android.net.wifi.WifiUsabilityStatsEntry.PROBE_STATUS_NO_PROBE;
import static android.net.wifi.WifiUsabilityStatsEntry.PROBE_STATUS_SUCCESS;
import static android.net.wifi.WifiUsabilityStatsEntry.PROBE_STATUS_UNKNOWN;
import static android.net.wifi.WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE;
import static android.net.wifi.WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK;
import static android.net.wifi.WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI;
import static android.net.wifi.WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO;
import static android.os.Process.myUid;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.annotation.NonNull;
import android.app.UiAutomation;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.WifiNetworkSuggestion;
import android.net.wifi.WifiUsabilityStatsEntry;
import android.net.wifi.WifiConnectedSessionInfo;
import android.os.Build;
import android.platform.test.annotations.AppModeFull;
import android.support.test.uiautomator.UiDevice;
import android.telephony.TelephonyManager;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.ApiLevelUtil;
import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.PropertyUtil;
import com.android.compatibility.common.util.ShellIdentityUtils;

import com.google.common.collect.Range;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * Tests for wifi connected network scorer interface and usability stats.
 */
@AppModeFull(reason = "Cannot get WifiManager in instant app mode")
@LargeTest
@RunWith(AndroidJUnit4.class)
public class ConnectedNetworkScorerTest extends WifiJUnit4TestBase {
    private Context mContext;
    private WifiManager mWifiManager;
    private ConnectivityManager mConnectivityManager;
    private UiDevice mUiDevice;
    private TestHelper mTestHelper;
    private TelephonyManager mTelephonyManager;

    private boolean mWasVerboseLoggingEnabled;

    private static final int WIFI_CONNECT_TIMEOUT_MILLIS = 30_000;
    private static final int TIMEOUT = 12_000;
    private static final int WAIT_DURATION = 5_000;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getInstrumentation().getContext();

        // skip the test if WiFi is not supported
        assumeTrue(WifiFeature.isWifiSupported(mContext));

        mWifiManager = mContext.getSystemService(WifiManager.class);
        assertThat(mWifiManager).isNotNull();

        mConnectivityManager = mContext.getSystemService(ConnectivityManager.class);

        // turn on verbose logging for tests
        mWasVerboseLoggingEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.isVerboseLoggingEnabled());
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setVerboseLoggingEnabled(true));

        // enable Wifi
        if (!mWifiManager.isWifiEnabled()) {
            ShellIdentityUtils.invokeWithShellPermissions(() -> mWifiManager.setWifiEnabled(true));
        }
        PollingCheck.check("Wifi not enabled", TIMEOUT, () -> mWifiManager.isWifiEnabled());

        // turn screen on
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());

        mTestHelper = new TestHelper(mContext, mUiDevice);
        mTestHelper.turnScreenOn();

        // Clear any existing app state before each test.
        if (WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(mContext)) {
            ShellIdentityUtils.invokeWithShellPermissions(
                    () -> mWifiManager.removeAppState(myUid(), mContext.getPackageName()));
        }

        // check we have >= 1 saved network
        List<WifiConfiguration> savedNetworks = ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.getConfiguredNetworks());
        assertWithMessage("Need at least one saved network").that(savedNetworks).isNotEmpty();

        // ensure Wifi is connected
        ShellIdentityUtils.invokeWithShellPermissions(() -> mWifiManager.reconnect());
        PollingCheck.check(
                "Wifi not connected",
                WIFI_CONNECT_TIMEOUT_MILLIS,
                () -> mWifiManager.getConnectionInfo().getNetworkId() != -1);
        mTelephonyManager = mContext.getSystemService(TelephonyManager.class);
    }

    @After
    public void tearDown() throws Exception {
        if (!WifiFeature.isWifiSupported(mContext)) return;
        if (!mWifiManager.isWifiEnabled()) {
            ShellIdentityUtils.invokeWithShellPermissions(() -> mWifiManager.setWifiEnabled(true));
        }
        mTestHelper.turnScreenOff();
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setVerboseLoggingEnabled(mWasVerboseLoggingEnabled));
    }

    private static class TestUsabilityStatsListener implements
            WifiManager.OnWifiUsabilityStatsListener {
        private final CountDownLatch mCountDownLatch;
        public int seqNum;
        public boolean isSameBssidAndFre;
        public WifiUsabilityStatsEntry statsEntry;

        TestUsabilityStatsListener(CountDownLatch countDownLatch) {
            mCountDownLatch = countDownLatch;
        }

        @Override
        public void onWifiUsabilityStats(int seqNum, boolean isSameBssidAndFreq,
                WifiUsabilityStatsEntry statsEntry) {
            this.seqNum = seqNum;
            this.isSameBssidAndFre = isSameBssidAndFreq;
            this.statsEntry = statsEntry;
            mCountDownLatch.countDown();
        }
    }

    /**
     * Tests the {@link android.net.wifi.WifiUsabilityStatsEntry} retrieved from
     * {@link WifiManager.OnWifiUsabilityStatsListener}.
     */
    @Test
    public void testWifiUsabilityStatsEntry() throws Exception {
        // Usability stats collection only supported by vendor version Q and above.
        if (!PropertyUtil.isVendorApiLevelAtLeast(Build.VERSION_CODES.Q)) {
            return;
        }
        CountDownLatch countDownLatch = new CountDownLatch(1);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestUsabilityStatsListener usabilityStatsListener =
                new TestUsabilityStatsListener(countDownLatch);
        try {
            uiAutomation.adoptShellPermissionIdentity();
            mWifiManager.addOnWifiUsabilityStatsListener(
                    Executors.newSingleThreadExecutor(), usabilityStatsListener);
            // Wait for new usability stats (while connected & screen on this is triggered
            // by platform periodically).
            assertThat(countDownLatch.await(TIMEOUT, TimeUnit.MILLISECONDS)).isTrue();

            assertThat(usabilityStatsListener.statsEntry).isNotNull();
            WifiUsabilityStatsEntry statsEntry = usabilityStatsListener.statsEntry;

            assertThat(statsEntry.getTimeStampMillis()).isGreaterThan(0L);
            assertThat(statsEntry.getRssi()).isLessThan(0);
            assertThat(statsEntry.getLinkSpeedMbps()).isAtLeast(0);
            assertThat(statsEntry.getTotalTxSuccess()).isAtLeast(0L);
            assertThat(statsEntry.getTotalTxRetries()).isAtLeast(0L);
            assertThat(statsEntry.getTotalTxBad()).isAtLeast(0L);
            assertThat(statsEntry.getTotalRxSuccess()).isAtLeast(0L);
            if (mWifiManager.isEnhancedPowerReportingSupported()) {
                assertThat(statsEntry.getTotalRadioOnTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalRadioTxTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalRadioRxTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalScanTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalNanScanTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalBackgroundScanTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalRoamScanTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalPnoScanTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalHotspot2ScanTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalCcaBusyFreqTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalRadioOnTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalRadioOnFreqTimeMillis()).isAtLeast(0L);
                assertThat(statsEntry.getTotalBeaconRx()).isAtLeast(0L);
                assertThat(statsEntry.getProbeStatusSinceLastUpdate())
                        .isAnyOf(PROBE_STATUS_SUCCESS,
                                PROBE_STATUS_FAILURE,
                                PROBE_STATUS_NO_PROBE,
                                PROBE_STATUS_UNKNOWN);
                // -1 is default value for some of these fields if they're not available.
                assertThat(statsEntry.getProbeElapsedTimeSinceLastUpdateMillis()).isAtLeast(-1);
                assertThat(statsEntry.getProbeMcsRateSinceLastUpdate()).isAtLeast(-1);
                assertThat(statsEntry.getRxLinkSpeedMbps()).isAtLeast(-1);
                if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
                    try {
                        assertThat(statsEntry.getTimeSliceDutyCycleInPercent())
                                .isIn(Range.closed(0, 100));
                    } catch (NoSuchElementException e) {
                        // pass - Device does not support the field.
                    }
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BE).getContentionTimeMinMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BE).getContentionTimeMaxMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BE).getContentionTimeAvgMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BE).getContentionNumSamples()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BK).getContentionTimeMinMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BK).getContentionTimeMaxMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BK).getContentionTimeAvgMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_BK).getContentionNumSamples()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VI).getContentionTimeMinMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VI).getContentionTimeMaxMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VI).getContentionTimeAvgMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VI).getContentionNumSamples()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VO).getContentionTimeMinMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VO).getContentionTimeMaxMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VO).getContentionTimeAvgMicros()).isAtLeast(0);
                    assertThat(statsEntry.getContentionTimeStats(
                            WME_ACCESS_CATEGORY_VO).getContentionNumSamples()).isAtLeast(0);
                    // This is to add CTS test for the constructor function.
                    ContentionTimeStats contentionStats = new ContentionTimeStats(2, 1, 4, 10);
                    assertEquals(2, contentionStats.getContentionTimeMinMicros());
                    assertEquals(1, contentionStats.getContentionTimeMaxMicros());
                    assertEquals(4, contentionStats.getContentionTimeAvgMicros());
                    assertEquals(10, contentionStats.getContentionNumSamples());
                    assertThat(statsEntry.getChannelUtilizationRatio()).isIn(Range.closed(0, 255));
                    if (mTelephonyManager != null) {
                        boolean isCellularDataAvailable =
                                mTelephonyManager.getDataState() == TelephonyManager.DATA_CONNECTED;
                        assertEquals(isCellularDataAvailable, statsEntry.isCellularDataAvailable());
                    } else {
                        assertFalse(statsEntry.isCellularDataAvailable());
                    }
                    statsEntry.isWifiScoringEnabled();
                    statsEntry.isThroughputSufficient();
                    RateStats rateStats = new RateStats(WifiUsabilityStatsEntry.WIFI_PREAMBLE_VHT,
                            WifiUsabilityStatsEntry.WIFI_SPATIAL_STREAMS_TWO,
                            WifiUsabilityStatsEntry.WIFI_BANDWIDTH_40_MHZ,
                            2, 20, 100, 200, 5, 10);
                    assertThat(statsEntry.getRateStats()).isNotNull();
                    if(statsEntry.getRateStats().size() > 0) {
                        assertThat(statsEntry.getRateStats().get(0).getPreamble()).isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getNumberOfSpatialStreams())
                                .isAtLeast(1);
                        assertThat(statsEntry.getRateStats().get(0).getBandwidthInMhz())
                                .isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getRateMcsIdx()).isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getBitRateInKbps())
                                .isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getTxMpdu()).isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getRxMpdu()).isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getMpduLost()).isAtLeast(0);
                        assertThat(statsEntry.getRateStats().get(0).getRetries()).isAtLeast(0);
                    }
                    RadioStats radioStat = new RadioStats(0, 10, 11, 12, 13, 14, 15, 16, 17, 18);
                    assertThat(statsEntry.getWifiLinkLayerRadioStats()).isNotNull();
                    int numRadios = statsEntry.getWifiLinkLayerRadioStats().size();
                    for (int i = 0; i < numRadios; i++) {
                        RadioStats radioStats = statsEntry.getWifiLinkLayerRadioStats().get(i);
                        assertThat(radioStats.getRadioId()).isAtLeast(0);
                        assertThat(radioStats.getTotalRadioOnTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalRadioTxTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalRadioRxTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalScanTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalNanScanTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalBackgroundScanTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalRoamScanTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalPnoScanTimeMillis()).isAtLeast(0);
                        assertThat(radioStats.getTotalHotspot2ScanTimeMillis()).isAtLeast(0);
                    }
                }
                // no longer populated, return default value.
                assertThat(statsEntry.getCellularDataNetworkType())
                        .isAnyOf(TelephonyManager.NETWORK_TYPE_UNKNOWN,
                                TelephonyManager.NETWORK_TYPE_GPRS,
                                TelephonyManager.NETWORK_TYPE_EDGE,
                                TelephonyManager.NETWORK_TYPE_UMTS,
                                TelephonyManager.NETWORK_TYPE_CDMA,
                                TelephonyManager.NETWORK_TYPE_EVDO_0,
                                TelephonyManager.NETWORK_TYPE_EVDO_A,
                                TelephonyManager.NETWORK_TYPE_1xRTT,
                                TelephonyManager.NETWORK_TYPE_HSDPA,
                                TelephonyManager.NETWORK_TYPE_HSUPA,
                                TelephonyManager.NETWORK_TYPE_HSPA,
                                TelephonyManager.NETWORK_TYPE_IDEN,
                                TelephonyManager.NETWORK_TYPE_EVDO_B,
                                TelephonyManager.NETWORK_TYPE_LTE,
                                TelephonyManager.NETWORK_TYPE_EHRPD,
                                TelephonyManager.NETWORK_TYPE_HSPAP,
                                TelephonyManager.NETWORK_TYPE_GSM,
                                TelephonyManager.NETWORK_TYPE_TD_SCDMA,
                                TelephonyManager.NETWORK_TYPE_IWLAN,
                                TelephonyManager.NETWORK_TYPE_NR);
                assertThat(statsEntry.getCellularSignalStrengthDbm()).isAtMost(0);
                assertThat(statsEntry.getCellularSignalStrengthDb()).isAtMost(0);
                assertThat(statsEntry.isSameRegisteredCell()).isFalse();
            }
        } finally {
            mWifiManager.removeOnWifiUsabilityStatsListener(usabilityStatsListener);
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests the {@link android.net.wifi.WifiManager#updateWifiUsabilityScore(int, int, int)}
     */
    @Test
    public void testUpdateWifiUsabilityScore() throws Exception {
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // update scoring with dummy values.
            mWifiManager.updateWifiUsabilityScore(0, 50, 50);
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests the {@link android.net.wifi.WifiManager#setWifiScoringEnabled(boolean)}
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    @Test
    public void testSetWifiScoringEnabled() throws Exception {
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try {
            uiAutomation.adoptShellPermissionIdentity();
            assertTrue(mWifiManager.setWifiScoringEnabled(true));
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    private static abstract class TestConnectedNetworkScorer implements
            WifiManager.WifiConnectedNetworkScorer {
        protected CountDownLatch mCountDownLatch;
        public Integer startSessionId;
        public Integer stopSessionId;
        public WifiManager.ScoreUpdateObserver scoreUpdateObserver;
        public boolean isUserSelected;

        TestConnectedNetworkScorer(CountDownLatch countDownLatch) {
            mCountDownLatch = countDownLatch;
        }

        @Override
        public void onStop(int sessionId) {
            synchronized (mCountDownLatch) {
                this.stopSessionId = sessionId;
                mCountDownLatch.countDown();
            }
        }

        @Override
        public void onSetScoreUpdateObserver(WifiManager.ScoreUpdateObserver observerImpl) {
            synchronized (mCountDownLatch) {
                this.scoreUpdateObserver = observerImpl;
            }
        }

        public void resetCountDownLatch(CountDownLatch countDownLatch) {
            synchronized (mCountDownLatch) {
                mCountDownLatch = countDownLatch;
            }
        }
    }

    private static class TestConnectedNetworkScorerWithSessionId extends
            TestConnectedNetworkScorer {
        TestConnectedNetworkScorerWithSessionId(CountDownLatch countDownLatch) {
            super(countDownLatch);
            isUserSelected = false;
        }

        @Override
        public void onStart(int sessionId) {
            synchronized (mCountDownLatch) {
                this.startSessionId = sessionId;
                mCountDownLatch.countDown();
            }
        }
    }

    private static class TestConnectedNetworkScorerWithSessionInfo extends
            TestConnectedNetworkScorer {
        TestConnectedNetworkScorerWithSessionInfo(CountDownLatch countDownLatch) {
            super(countDownLatch);
        }

        @Override
        public void onStart(WifiConnectedSessionInfo sessionInfo) {
            synchronized (mCountDownLatch) {
                this.startSessionId = sessionInfo.getSessionId();
                this.isUserSelected = sessionInfo.isUserSelected();
                // Build a WifiConnectedSessionInfo object
                WifiConnectedSessionInfo.Builder sessionBuilder =
                        new WifiConnectedSessionInfo.Builder(startSessionId.intValue())
                                .setUserSelected(isUserSelected);
                sessionBuilder.build();
                mCountDownLatch.countDown();
            }
        }
    }

    /**
     * Tests the
     * {@link android.net.wifi.WifiConnectedNetworkScorer#onStart(WifiConnectedSessionInfo)}.
     */
    @Test
    public void testConnectedNetworkScorerWithSessionInfo() throws Exception {
        CountDownLatch countDownLatchScorer = new CountDownLatch(1);
        TestConnectedNetworkScorerWithSessionInfo connectedNetworkScorer =
                new TestConnectedNetworkScorerWithSessionInfo(countDownLatchScorer);
        testSetWifiConnectedNetworkScorer(connectedNetworkScorer, countDownLatchScorer);
    }

    /**
     * Tests the {@link android.net.wifi.WifiConnectedNetworkScorer#onStart(int)}.
     */
    @Test
    public void testConnectedNetworkScorerWithSessionId() throws Exception {
        CountDownLatch countDownLatchScorer = new CountDownLatch(1);
        TestConnectedNetworkScorerWithSessionId connectedNetworkScorer =
                new TestConnectedNetworkScorerWithSessionId(countDownLatchScorer);
        testSetWifiConnectedNetworkScorer(connectedNetworkScorer, countDownLatchScorer);
    }

    /**
     * Note: We could write more interesting test cases (if the device has a mobile connection), but
     * that would make the test flaky. The default network/route selection on the device is not just
     * controlled by the wifi scorer input, but also based on params which are controlled by
     * other parts of the platform (likely in connectivity service) and hence will behave
     * differently on OEM devices.
     */
    private void testSetWifiConnectedNetworkScorer(
            TestConnectedNetworkScorer connectedNetworkScorer,
                    CountDownLatch countDownLatchScorer) throws Exception {
        CountDownLatch countDownLatchUsabilityStats = new CountDownLatch(1);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestUsabilityStatsListener usabilityStatsListener =
                new TestUsabilityStatsListener(countDownLatchUsabilityStats);
        boolean disconnected = false;
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Clear any external scorer already active on the device.
            mWifiManager.clearWifiConnectedNetworkScorer();
            Thread.sleep(500);

            mWifiManager.setWifiConnectedNetworkScorer(
                    Executors.newSingleThreadExecutor(), connectedNetworkScorer);
            // Since we're already connected, wait for onStart to be invoked.
            assertThat(countDownLatchScorer.await(TIMEOUT, TimeUnit.MILLISECONDS)).isTrue();

            assertThat(connectedNetworkScorer.startSessionId).isAtLeast(0);
            assertThat(connectedNetworkScorer.isUserSelected).isEqualTo(false);
            assertThat(connectedNetworkScorer.scoreUpdateObserver).isNotNull();
            WifiManager.ScoreUpdateObserver scoreUpdateObserver =
                    connectedNetworkScorer.scoreUpdateObserver;

            // Now trigger a dummy score update.
            scoreUpdateObserver.notifyScoreUpdate(connectedNetworkScorer.startSessionId, 50);

            // Register the usability listener
            mWifiManager.addOnWifiUsabilityStatsListener(
                    Executors.newSingleThreadExecutor(), usabilityStatsListener);
            // Trigger a usability stats update.
            scoreUpdateObserver.triggerUpdateOfWifiUsabilityStats(
                    connectedNetworkScorer.startSessionId);
            // Ensure that we got the stats update callback.
            assertThat(countDownLatchUsabilityStats.await(TIMEOUT, TimeUnit.MILLISECONDS))
                    .isTrue();
            assertThat(usabilityStatsListener.seqNum).isAtLeast(0);

            // Reset the scorer countdown latch for onStop
            countDownLatchScorer = new CountDownLatch(1);
            connectedNetworkScorer.resetCountDownLatch(countDownLatchScorer);
            if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
                // Notify status change and request a NUD check
                scoreUpdateObserver.notifyStatusUpdate(
                        connectedNetworkScorer.startSessionId, false);
                scoreUpdateObserver.requestNudOperation(connectedNetworkScorer.startSessionId);
                // Blocklist current AP with invalid session Id
                scoreUpdateObserver.blocklistCurrentBssid(-1);
            }
            // Now disconnect from the network.
            mWifiManager.disconnect();
            // Wait for it to be disconnected.
            PollingCheck.check(
                    "Wifi not disconnected",
                    TIMEOUT,
                    () -> mWifiManager.getConnectionInfo().getNetworkId() == -1);
            disconnected = true;

            // Wait for stop to be invoked and ensure that the session id matches.
            assertThat(countDownLatchScorer.await(TIMEOUT, TimeUnit.MILLISECONDS)).isTrue();
            assertThat(connectedNetworkScorer.stopSessionId)
                    .isEqualTo(connectedNetworkScorer.startSessionId);
            // Verify that onStart() and onStop() set internal variables correctly.
            connectedNetworkScorer.onStart(
                    new WifiConnectedSessionInfo.Builder(100)
                            .setUserSelected(false)
                            .build());
            assertEquals(100, connectedNetworkScorer.startSessionId.intValue());
            assertEquals(false, connectedNetworkScorer.isUserSelected);
            connectedNetworkScorer.onStop(200);
            assertEquals(200, connectedNetworkScorer.stopSessionId.intValue());
        } finally {
            mWifiManager.removeOnWifiUsabilityStatsListener(usabilityStatsListener);
            mWifiManager.clearWifiConnectedNetworkScorer();

            if (disconnected) {
                mWifiManager.reconnect();
                // Wait for it to be reconnected.
                PollingCheck.check(
                        "Wifi not reconnected",
                        WIFI_CONNECT_TIMEOUT_MILLIS,
                        () -> mWifiManager.getConnectionInfo().getNetworkId() != -1);
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Tests the {@link android.net.wifi.WifiConnectedNetworkScorer} interface.
     *
     * Verifies that the external scorer works even after wifi restart.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    @Test
    public void testSetWifiConnectedNetworkScorerOnSubsystemRestart() throws Exception {
        CountDownLatch countDownLatchScorer = new CountDownLatch(1);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestConnectedNetworkScorerWithSessionInfo connectedNetworkScorer =
                new TestConnectedNetworkScorerWithSessionInfo(countDownLatchScorer);
        try {
            uiAutomation.adoptShellPermissionIdentity();
            // Clear any external scorer already active on the device.
            mWifiManager.clearWifiConnectedNetworkScorer();
            Thread.sleep(500);

            mWifiManager.setWifiConnectedNetworkScorer(
                    Executors.newSingleThreadExecutor(), connectedNetworkScorer);
            // Since we're already connected, wait for onStart to be invoked.
            assertThat(countDownLatchScorer.await(TIMEOUT, TimeUnit.MILLISECONDS)).isTrue();

            int prevSessionId = connectedNetworkScorer.startSessionId;
            WifiManager.ScoreUpdateObserver prevScoreUpdateObserver =
                    connectedNetworkScorer.scoreUpdateObserver;

            // Expect one stop followed by one start after the restart

            // Ensure that we got an onStop() for the previous connection when restart is invoked.
            countDownLatchScorer = new CountDownLatch(1);
            connectedNetworkScorer.resetCountDownLatch(countDownLatchScorer);

            // Restart wifi subsystem.
            mWifiManager.restartWifiSubsystem();
            // Wait for the device to connect back.
            PollingCheck.check(
                    "Wifi not connected",
                    WIFI_CONNECT_TIMEOUT_MILLIS * 2,
                    () -> mWifiManager.getConnectionInfo().getNetworkId() != -1);

            assertThat(countDownLatchScorer.await(TIMEOUT, TimeUnit.MILLISECONDS)).isTrue();
            assertThat(connectedNetworkScorer.stopSessionId).isEqualTo(prevSessionId);

            // Followed by a new onStart() after the connection.
            // Note: There is a 5 second delay between stop/start when restartWifiSubsystem() is
            // invoked, so this should not be racy.
            countDownLatchScorer = new CountDownLatch(1);
            connectedNetworkScorer.resetCountDownLatch(countDownLatchScorer);
            assertThat(countDownLatchScorer.await(TIMEOUT, TimeUnit.MILLISECONDS)).isTrue();
            assertThat(connectedNetworkScorer.startSessionId).isNotEqualTo(prevSessionId);

            // Ensure that we did not get a new score update observer.
            assertThat(connectedNetworkScorer.scoreUpdateObserver).isSameInstanceAs(
                    prevScoreUpdateObserver);
        } finally {
            mWifiManager.clearWifiConnectedNetworkScorer();
            uiAutomation.dropShellPermissionIdentity();
        }
    }

    private interface ConnectionInitiator {
        /**
         * Trigger connection (using suggestion or specifier) to the provided network.
         */
        ConnectivityManager.NetworkCallback initiateConnection(
                @NonNull WifiConfiguration testNetwork,
                @NonNull ScheduledExecutorService executorService) throws Exception;
    }

    private void setWifiConnectedNetworkScorerAndInitiateConnectToSpecifierOrRestrictedSuggestion(
            @NonNull ConnectionInitiator connectionInitiator) throws Exception {
        CountDownLatch countDownLatchScorer = new CountDownLatch(1);
        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        TestConnectedNetworkScorerWithSessionInfo connectedNetworkScorer =
                new TestConnectedNetworkScorerWithSessionInfo(countDownLatchScorer);
        ConnectivityManager.NetworkCallback networkCallback = null;
        ScheduledExecutorService executorService = Executors.newSingleThreadScheduledExecutor();
        List<WifiConfiguration> savedNetworks = null;
        try {
            uiAutomation.adoptShellPermissionIdentity(
                    NETWORK_SETTINGS, WIFI_UPDATE_USABILITY_STATS_SCORE, CONNECTIVITY_INTERNAL,
                    READ_WIFI_CREDENTIAL);

            // Clear any external scorer already active on the device.
            mWifiManager.clearWifiConnectedNetworkScorer();
            Thread.sleep(500);

            savedNetworks = mWifiManager.getPrivilegedConfiguredNetworks();
            WifiConfiguration testNetwork =
                    TestHelper.findMatchingSavedNetworksWithBssid(mWifiManager, savedNetworks)
                            .get(0);
            // Disconnect & disable auto-join on the saved network to prevent auto-connect from
            // interfering with the test.
            for (WifiConfiguration savedNetwork : savedNetworks) {
                mWifiManager.disableNetwork(savedNetwork.networkId);
            }
            // Wait for Wifi to be disconnected.
            PollingCheck.check(
                    "Wifi not disconnected",
                    20000,
                    () -> mWifiManager.getConnectionInfo().getNetworkId() == -1);
            assertThat(testNetwork).isNotNull();

            // Register the external scorer.
            mWifiManager.setWifiConnectedNetworkScorer(
                    Executors.newSingleThreadExecutor(), connectedNetworkScorer);

            // Now connect using the provided connection initiator
            networkCallback = connectionInitiator.initiateConnection(testNetwork, executorService);

            // We should not receive the start
            assertThat(countDownLatchScorer.await(WAIT_DURATION, TimeUnit.MILLISECONDS)).isFalse();
            assertThat(connectedNetworkScorer.startSessionId).isNull();

            // Now disconnect from the network.
            mConnectivityManager.unregisterNetworkCallback(networkCallback);
            networkCallback = null;

            // We should not receive the stop either
            countDownLatchScorer = new CountDownLatch(1);
            connectedNetworkScorer.resetCountDownLatch(countDownLatchScorer);
            assertThat(countDownLatchScorer.await(WAIT_DURATION, TimeUnit.MILLISECONDS)).isFalse();
            assertThat(connectedNetworkScorer.stopSessionId).isNull();
        } finally {
            executorService.shutdownNow();
            mWifiManager.clearWifiConnectedNetworkScorer();
            if (networkCallback != null) {
                mConnectivityManager.unregisterNetworkCallback(networkCallback);
            }
            // Re-enable the networks after the test.
            if (savedNetworks != null) {
                for (WifiConfiguration savedNetwork : savedNetworks) {
                    mWifiManager.enableNetwork(savedNetwork.networkId, false);
                }
            }
            uiAutomation.dropShellPermissionIdentity();
        }
    }
    /**
     * Tests the {@link android.net.wifi.WifiConnectedNetworkScorer} interface.
     *
     * Verifies that the external scorer is not notified for local only connections.
     */
    @Test
    public void testSetWifiConnectedNetworkScorerForSpecifierConnection() throws Exception {
        setWifiConnectedNetworkScorerAndInitiateConnectToSpecifierOrRestrictedSuggestion(
                (testNetwork, executorService) -> {
                    // Connect using wifi network specifier.
                    WifiNetworkSpecifier specifier =
                            TestHelper.createSpecifierBuilderWithCredentialFromSavedNetwork(
                                    testNetwork)
                                    .build();
                    return mTestHelper.testConnectionFlowWithSpecifierWithShellIdentity(
                            testNetwork, specifier, false);
                }
        );
    }

    private void testSetWifiConnectedNetworkScorerForRestrictedSuggestionConnection(
            Set<Integer> restrictedNetworkCapabilities) throws Exception {
        setWifiConnectedNetworkScorerAndInitiateConnectToSpecifierOrRestrictedSuggestion(
                (testNetwork, executorService) -> {
                    // Connect using wifi network suggestion.
                    WifiNetworkSuggestion.Builder suggestionBuilder =
                            TestHelper
                                    .createSuggestionBuilderWithCredentialFromSavedNetworkWithBssid(
                                    testNetwork);
                    if (restrictedNetworkCapabilities.contains(NET_CAPABILITY_OEM_PAID)) {
                        suggestionBuilder.setOemPaid(true);
                    }
                    if (restrictedNetworkCapabilities.contains(NET_CAPABILITY_OEM_PRIVATE)) {
                        suggestionBuilder.setOemPrivate(true);
                    }
                    return mTestHelper.testConnectionFlowWithSuggestionWithShellIdentity(
                            testNetwork, suggestionBuilder.build(), executorService,
                            restrictedNetworkCapabilities);
                }
        );
    }

    /**
     * Tests the {@link android.net.wifi.WifiConnectedNetworkScorer} interface.
     *
     * Verifies that the external scorer is not notified for oem paid suggestion connections.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    @Test
    public void testSetWifiConnectedNetworkScorerForOemPaidSuggestionConnection() throws Exception {
        testSetWifiConnectedNetworkScorerForRestrictedSuggestionConnection(
                Set.of(NET_CAPABILITY_OEM_PAID));
    }

    /**
     * Tests the {@link android.net.wifi.WifiConnectedNetworkScorer} interface.
     *
     * Verifies that the external scorer is not notified for oem private suggestion connections.
     */
    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    @Test
    public void testSetWifiConnectedNetworkScorerForOemPrivateSuggestionConnection()
            throws Exception {
        testSetWifiConnectedNetworkScorerForRestrictedSuggestionConnection(
                Set.of(NET_CAPABILITY_OEM_PRIVATE));
    }
}
