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

package android.net.wifi.rtt.cts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.location.LocationManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.cts.WifiJUnit3TestBase;
import android.net.wifi.rtt.RangingResult;
import android.net.wifi.rtt.RangingResultCallback;
import android.net.wifi.rtt.WifiRttManager;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.HandlerThread;

import com.android.compatibility.common.util.ShellIdentityUtils;
import com.android.compatibility.common.util.SystemUtil;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

/**
 * Base class for Wi-Fi RTT CTS test cases. Provides a uniform configuration and event management
 * facility.
 */
public class TestBase extends WifiJUnit3TestBase {
    protected static final String TAG = "WifiRttCtsTests";

    // wait for Wi-Fi RTT to become available
    private static final int WAIT_FOR_RTT_CHANGE_SECS = 10;

    // wait for Wi-Fi scan results to become available
    private static final int WAIT_FOR_SCAN_RESULTS_SECS = 20;

    // wait for network selection and connection finish
    private static final int WAIT_FOR_CONNECTION_FINISH_MS = 30_000;

    // Interval between failure scans
    private static final int INTERVAL_BETWEEN_FAILURE_SCAN_MILLIS = 5_000;

    // 5GHz Frequency band
    private static final int FREQUENCY_OF_5GHZ_BAND_IN_MHZ = 5_000;

    protected WifiRttManager mWifiRttManager;
    protected WifiManager mWifiManager;
    private LocationManager mLocationManager;
    private WifiManager.WifiLock mWifiLock;

    private final HandlerThread mHandlerThread = new HandlerThread("SingleDeviceTest");
    protected final Executor mExecutor;
    private Boolean mWasVerboseLoggingEnabled;

    {
        mHandlerThread.start();
        mExecutor = new HandlerExecutor(new Handler(mHandlerThread.getLooper()));
    }

    /**
     * Returns a flag indicating whether or not Wi-Fi RTT should be tested. Wi-Fi RTT
     * should be tested if the feature is supported on the current device.
     */
    static boolean shouldTestWifiRtt(Context context) {
        final PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_WIFI_RTT);
    }

    /**
     * Returns a flag indicating whether or not Wi-Fi Aware should be tested. Wi-Fi Aware
     * should be tested if the feature is supported on the current device.
     */
    static boolean shouldTestWifiAware(Context context) {
        final PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_WIFI_AWARE);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        if (!shouldTestWifiRtt(getContext())) {
            return;
        }

        mLocationManager = (LocationManager) getContext().getSystemService(
                Context.LOCATION_SERVICE);
        assertTrue("RTT testing requires Location to be enabled",
                mLocationManager.isLocationEnabled());

        mWifiRttManager = (WifiRttManager) getContext().getSystemService(
                Context.WIFI_RTT_RANGING_SERVICE);
        assertNotNull("Wi-Fi RTT Manager", mWifiRttManager);

        mWifiManager = (WifiManager) getContext().getSystemService(Context.WIFI_SERVICE);
        assertNotNull("Wi-Fi Manager", mWifiManager);

        // turn on verbose logging for tests
        mWasVerboseLoggingEnabled = ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.isVerboseLoggingEnabled());
        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setVerboseLoggingEnabled(true));

        mWifiLock = mWifiManager.createWifiLock(TAG);
        mWifiLock.acquire();
        if (!mWifiManager.isWifiEnabled()) {
            SystemUtil.runShellCommand("svc wifi enable");
            // Turn on Wi-Fi may trigger connection. Wait connection state stable.
            scanAps();
            Thread.sleep(WAIT_FOR_CONNECTION_FINISH_MS);
        }
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiRttManager.ACTION_WIFI_RTT_STATE_CHANGED);
        WifiRttBroadcastReceiver receiver = new WifiRttBroadcastReceiver();
        mContext.registerReceiver(receiver, intentFilter);
        if (!mWifiRttManager.isAvailable()) {
            assertTrue("Timeout waiting for Wi-Fi RTT to change status",
                    receiver.waitForStateChange());
            assertTrue("Wi-Fi RTT is not available (should be)", mWifiRttManager.isAvailable());
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (!shouldTestWifiRtt(getContext())) {
            super.tearDown();
            return;
        }

        ShellIdentityUtils.invokeWithShellPermissions(
                () -> mWifiManager.setVerboseLoggingEnabled(mWasVerboseLoggingEnabled));

        super.tearDown();
    }

    class WifiRttBroadcastReceiver extends BroadcastReceiver {
        private CountDownLatch mBlocker = new CountDownLatch(1);

        @Override
        public void onReceive(Context context, Intent intent) {
            if (WifiRttManager.ACTION_WIFI_RTT_STATE_CHANGED.equals(intent.getAction())) {
                mBlocker.countDown();
            }
        }

        boolean waitForStateChange() throws InterruptedException {
            return mBlocker.await(WAIT_FOR_RTT_CHANGE_SECS, TimeUnit.SECONDS);
        }
    }

    class WifiScansBroadcastReceiver extends BroadcastReceiver {
        private CountDownLatch mBlocker = new CountDownLatch(1);

        @Override
        public void onReceive(Context context, Intent intent) {
            if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(intent.getAction())) {
                mBlocker.countDown();
            }
        }

        boolean waitForStateChange() throws InterruptedException {
            return mBlocker.await(WAIT_FOR_SCAN_RESULTS_SECS, TimeUnit.SECONDS);
        }
    }

    class ResultCallback extends RangingResultCallback {
        private CountDownLatch mBlocker = new CountDownLatch(1);
        private int mCode; // 0: success, otherwise RangingResultCallback STATUS_CODE_*.
        private List<RangingResult> mResults;

        @Override
        public void onRangingFailure(int code) {
            mCode = code;
            mResults = null; // not necessary since intialized to null - but for completeness
            mBlocker.countDown();
        }

        @Override
        public void onRangingResults(List<RangingResult> results) {
            mCode = 0; // not necessary since initialized to 0 - but for completeness
            mResults = results;
            mBlocker.countDown();
        }

        /**
         * Waits for the listener callback to be called - or an error (timeout, interruption).
         * Returns true on callback called, false on error (timeout, interruption).
         */
        boolean waitForCallback() throws InterruptedException {
            return mBlocker.await(WAIT_FOR_RTT_CHANGE_SECS, TimeUnit.SECONDS);
        }

        /**
         * Returns the code of the callback operation. Will be 0 for success (onRangingResults
         * called), else (if onRangingFailure called) will be one of the STATUS_CODE_* values.
         */
        int getCode() {
            return mCode;
        }

        /**
         * Returns the list of ranging results. In cases of error (getCode() != 0) will return null.
         */
        List<RangingResult> getResults() {
            return mResults;
        }
    }

    /**
     * Start a scan and return a list of observed ScanResults (APs).
     */
    protected List<ScanResult> scanAps() throws InterruptedException {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        WifiScansBroadcastReceiver receiver = new WifiScansBroadcastReceiver();
        mContext.registerReceiver(receiver, intentFilter);

        mWifiManager.startScan();
        receiver.waitForStateChange();
        mContext.unregisterReceiver(receiver);
        return mWifiManager.getScanResults();
    }

    /**
     * Start a scan and return a test AP which supports IEEE 802.11mc and which has the highest
     * RSSI. Will perform N (parameterized) scans and get the best AP across both scans.
     *
     * Returns null if test AP is not found in the specified number of scans.
     *
     * @param numScanRetries Maximum number of scans retries (in addition to first scan).
     */
    protected ScanResult scanForTest11mcCapableAp(int numScanRetries)
            throws InterruptedException {
        int scanCount = 0;
        ScanResult bestTestAp = null;
        while (scanCount <= numScanRetries) {
            for (ScanResult scanResult : scanAps()) {
                if (!scanResult.is80211mcResponder()) {
                    continue;
                }
                if (bestTestAp == null || scanResult.level > bestTestAp.level) {
                    bestTestAp = scanResult;
                }
            }
            if (bestTestAp == null) {
                // Ongoing connection may cause scan failure, wait for a while before next scan.
                Thread.sleep(INTERVAL_BETWEEN_FAILURE_SCAN_MILLIS);
            }
            scanCount++;
        }
        return bestTestAp;
    }

    /**
     * Start a scan and return a test AP which does NOT support IEEE 802.11mc, with a BSS in the
     * 5GHz band, and which has the highest RSSI. Will perform N (parameterized) scans and get
     * the best AP across all scan results.
     *
     * Returns null if test AP is not found in the specified number of scans.
     *
     * @param numScanRetries Maximum number of scans retries (in addition to first scan).
     */
    protected ScanResult scanForTestNon11mcCapableAp(int numScanRetries)
            throws InterruptedException {
        int scanCount = 0;
        ScanResult bestTestAp = null;
        while (scanCount <= numScanRetries) {
            for (ScanResult scanResult : scanAps()) {
                // Ensure using a 5GHz or greater channel
                if (scanResult.is80211mcResponder()
                        || scanResult.centerFreq0 < FREQUENCY_OF_5GHZ_BAND_IN_MHZ) {
                    continue;
                }
                if (bestTestAp == null || scanResult.level > bestTestAp.level) {
                    bestTestAp = scanResult;
                }
            }
            if (bestTestAp == null) {
                // Ongoing connection may cause scan failure, wait for a while before next scan.
                Thread.sleep(INTERVAL_BETWEEN_FAILURE_SCAN_MILLIS);
            }
            scanCount++;
        }
        return bestTestAp;
    }
}
