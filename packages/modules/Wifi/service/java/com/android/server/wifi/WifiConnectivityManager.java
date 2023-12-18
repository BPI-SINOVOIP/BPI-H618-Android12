/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static android.net.wifi.WifiConfiguration.RANDOMIZATION_NONE;

import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_PRIMARY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_LONG_LIVED;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_TRANSIENT;
import static com.android.server.wifi.ClientModeImpl.WIFI_WORK_SOURCE;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.MacAddress;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.DeviceMobilityState;
import android.net.wifi.WifiNetworkSuggestion;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiScanner.PnoSettings;
import android.net.wifi.WifiScanner.ScanSettings;
import android.net.wifi.hotspot2.PasspointConfiguration;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.PowerManager;
import android.os.Process;
import android.os.WorkSource;
import android.util.ArrayMap;
import android.util.LocalLog;
import android.util.Log;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.hotspot2.PasspointManager;
import com.android.server.wifi.util.ScanResultUtil;
import com.android.wifi.resources.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * This class manages all the connectivity related scanning activities.
 *
 * When the screen is turned on or off, WiFi is connected or disconnected,
 * or on-demand, a scan is initiatiated and the scan results are passed
 * to WifiNetworkSelector for it to make a recommendation on which network
 * to connect to.
 */
public class WifiConnectivityManager {
    public static final String WATCHDOG_TIMER_TAG =
            "WifiConnectivityManager Schedule Watchdog Timer";
    public static final String PERIODIC_SCAN_TIMER_TAG =
            "WifiConnectivityManager Schedule Periodic Scan Timer";
    public static final String RESTART_SINGLE_SCAN_TIMER_TAG =
            "WifiConnectivityManager Restart Single Scan";
    public static final String RESTART_CONNECTIVITY_SCAN_TIMER_TAG =
            "WifiConnectivityManager Restart Scan";
    public static final String DELAYED_PARTIAL_SCAN_TIMER_TAG =
            "WifiConnectivityManager Schedule Delayed Partial Scan Timer";

    private static final long RESET_TIME_STAMP = Long.MIN_VALUE;
    // Constants to indicate whether a scan should start immediately or
    // it should comply to the minimum scan interval rule.
    private static final boolean SCAN_IMMEDIATELY = true;
    private static final boolean SCAN_ON_SCHEDULE = false;

    // PNO scan interval in milli-seconds. This is the scan
    // performed when screen is off and connected.
    private static final int CONNECTED_PNO_SCAN_INTERVAL_MS = 160 * 1000; // 160 seconds
    // Maximum number of retries when starting a scan failed
    @VisibleForTesting
    public static final int MAX_SCAN_RESTART_ALLOWED = 5;
    // Number of milli-seconds to delay before retry starting
    // a previously failed scan
    private static final int RESTART_SCAN_DELAY_MS = 2 * 1000; // 2 seconds
    // Restricted channel list age out value.
    private static final long CHANNEL_LIST_AGE_MS = 60 * 60 * 1000; // 1 hour
    // This is the time interval for the connection attempt rate calculation. Connection attempt
    // timestamps beyond this interval is evicted from the list.
    public static final int MAX_CONNECTION_ATTEMPTS_TIME_INTERVAL_MS = 4 * 60 * 1000; // 4 mins
    // Max number of connection attempts in the above time interval.
    public static final int MAX_CONNECTION_ATTEMPTS_RATE = 6;
    private static final int TEMP_BSSID_BLOCK_DURATION = 10 * 1000; // 10 seconds
    // Maximum age of frequencies last seen to be included in pno scans. (30 days)
    private static final long MAX_PNO_SCAN_FREQUENCY_AGE_MS = (long) 1000 * 3600 * 24 * 30;
    private static final int POWER_SAVE_SCAN_INTERVAL_MULTIPLIER = 2;
    // ClientModeManager has a bunch of states. From the
    // WifiConnectivityManager's perspective it only cares
    // if it is in Connected state, Disconnected state or in
    // transition between these two states.
    public static final int WIFI_STATE_UNKNOWN = 0;
    public static final int WIFI_STATE_CONNECTED = 1;
    public static final int WIFI_STATE_DISCONNECTED = 2;
    public static final int WIFI_STATE_TRANSITIONING = 3;

    // Initial scan state, used to manage performing partial scans in initial scans
    // Initial scans are the first scan after enabling Wifi or turning on screen when disconnected
    @VisibleForTesting
    public static final int INITIAL_SCAN_STATE_START = 0;
    public static final int INITIAL_SCAN_STATE_AWAITING_RESPONSE = 1;
    public static final int INITIAL_SCAN_STATE_COMPLETE = 2;

    // Log tag for this class
    private static final String TAG = "WifiConnectivityManager";
    private static final String ALL_SINGLE_SCAN_LISTENER = "AllSingleScanListener";
    private static final String PNO_SCAN_LISTENER = "PnoScanListener";

    private final WifiContext mContext;
    private final WifiConfigManager mConfigManager;
    private final WifiNetworkSuggestionsManager mWifiNetworkSuggestionsManager;
    private final WifiConnectivityHelper mConnectivityHelper;
    private final WifiNetworkSelector mNetworkSelector;
    private final WifiLastResortWatchdog mWifiLastResortWatchdog;
    private final OpenNetworkNotifier mOpenNetworkNotifier;
    private final WifiMetrics mWifiMetrics;
    private final AlarmManager mAlarmManager;
    private final Handler mEventHandler;
    private final Clock mClock;
    private final ScoringParams mScoringParams;
    private final LocalLog mLocalLog;
    private final WifiGlobals mWifiGlobals;
    /**
     * Keeps connection attempts within the last {@link #MAX_CONNECTION_ATTEMPTS_TIME_INTERVAL_MS}
     * milliseconds.
     */
    private final LinkedList<Long> mConnectionAttemptTimeStamps = new LinkedList<>();
    private final WifiBlocklistMonitor mWifiBlocklistMonitor;
    private final PasspointManager mPasspointManager;
    private final WifiScoreCard mWifiScoreCard;
    private final WifiChannelUtilization mWifiChannelUtilization;
    private final PowerManager mPowerManager;
    private final DeviceConfigFacade mDeviceConfigFacade;
    private final ActiveModeWarden mActiveModeWarden;

    private WifiScanner mScanner;
    private boolean mDbg = false;
    private boolean mVerboseLoggingEnabled = false;
    private boolean mWifiEnabled = false;
    private boolean mAutoJoinEnabled = false; // disabled by default, enabled by external triggers
    private boolean mRunning = false;
    private boolean mScreenOn = false;
    private int mWifiState = WIFI_STATE_UNKNOWN;
    private int mInitialScanState = INITIAL_SCAN_STATE_COMPLETE;
    private boolean mAutoJoinEnabledExternal = true; // enabled by default
    private boolean mUntrustedConnectionAllowed = false;
    private boolean mOemPaidConnectionAllowed = false;
    private boolean mOemPrivateConnectionAllowed = false;
    private WorkSource mOemPaidConnectionRequestorWs = null;
    private WorkSource mOemPrivateConnectionRequestorWs = null;
    private boolean mTrustedConnectionAllowed = false;
    private boolean mSpecificNetworkRequestInProgress = false;
    private int mScanRestartCount = 0;
    private int mSingleScanRestartCount = 0;
    private int mTotalConnectivityAttemptsRateLimited = 0;
    private long mLastPeriodicSingleScanTimeStamp = RESET_TIME_STAMP;
    private long mLastNetworkSelectionTimeStamp = RESET_TIME_STAMP;
    private boolean mPnoScanStarted = false;
    private boolean mPeriodicScanTimerSet = false;
    private boolean mDelayedPartialScanTimerSet = false;
    private boolean mWatchdogScanTimerSet = false;

    // Used for Initial Scan metrics
    private boolean mFailedInitialPartialScan = false;
    private int mInitialPartialScanChannelCount;

    // Device configs
    private boolean mWaitForFullBandScanResults = false;

    // Scanning Schedules
    // Default schedule used in case of invalid configuration
    private static final int[] DEFAULT_SCANNING_SCHEDULE_SEC = {20, 40, 80, 160};
    private int[] mConnectedSingleScanScheduleSec;
    private int[] mDisconnectedSingleScanScheduleSec;
    private int[] mConnectedSingleSavedNetworkSingleScanScheduleSec;
    private List<WifiCandidates.Candidate> mLatestCandidates = null;
    private long mLatestCandidatesTimestampMs = 0;

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private int[] mCurrentSingleScanScheduleSec;

    private int mCurrentSingleScanScheduleIndex;
    // Cached WifiCandidates used in high mobility state to avoid connecting to APs that are
    // moving relative to the user.
    private CachedWifiCandidates mCachedWifiCandidates = null;
    private @DeviceMobilityState int mDeviceMobilityState =
            WifiManager.DEVICE_MOBILITY_STATE_UNKNOWN;

    // A helper to log debugging information in the local log buffer, which can
    // be retrieved in bugreport.
    private void localLog(String log) {
        mLocalLog.log(log);
        if (mVerboseLoggingEnabled) Log.v(TAG, log);
    }

    /**
     * Enable verbose logging for WifiConnectivityManager.
     */
    public void enableVerboseLogging(boolean verbose) {
        mVerboseLoggingEnabled = verbose;
    }

    // A periodic/PNO scan will be rescheduled up to MAX_SCAN_RESTART_ALLOWED times
    // if the start scan command failed. A timer is used here to make it a deferred retry.
    private final AlarmManager.OnAlarmListener mRestartScanListener =
            new AlarmManager.OnAlarmListener() {
                public void onAlarm() {
                    startConnectivityScan(SCAN_IMMEDIATELY);
                }
            };

    // A single scan will be rescheduled up to MAX_SCAN_RESTART_ALLOWED times
    // if the start scan command failed. An timer is used here to make it a deferred retry.
    private class RestartSingleScanListener implements AlarmManager.OnAlarmListener {
        private final boolean mIsFullBandScan;

        RestartSingleScanListener(boolean isFullBandScan) {
            mIsFullBandScan = isFullBandScan;
        }

        @Override
        public void onAlarm() {
            startSingleScan(mIsFullBandScan, WIFI_WORK_SOURCE);
        }
    }

    // As a watchdog mechanism, a single scan will be scheduled every
    // config_wifiPnoWatchdogIntervalMinutes if it is in the WIFI_STATE_DISCONNECTED state.
    private final AlarmManager.OnAlarmListener mWatchdogListener =
            new AlarmManager.OnAlarmListener() {
                public void onAlarm() {
                    watchdogHandler();
                }
            };

    // Due to b/28020168, timer based single scan will be scheduled
    // to provide periodic scan in an exponential backoff fashion.
    private final AlarmManager.OnAlarmListener mPeriodicScanTimerListener =
            new AlarmManager.OnAlarmListener() {
                public void onAlarm() {
                    periodicScanTimerHandler();
                }
            };

    private final AlarmManager.OnAlarmListener mDelayedPartialScanTimerListener =
            new AlarmManager.OnAlarmListener() {
                public void onAlarm() {
                    if (mCachedWifiCandidates == null
                            || mCachedWifiCandidates.frequencies == null
                            || mCachedWifiCandidates.frequencies.size() == 0) {
                        return;
                    }
                    ScanSettings settings = new ScanSettings();
                    settings.type = WifiScanner.SCAN_TYPE_HIGH_ACCURACY;
                    settings.band = getScanBand(false);
                    settings.reportEvents = WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT
                            | WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN;
                    settings.numBssidsPerScan = 0;
                    int index = 0;
                    settings.channels =
                            new WifiScanner.ChannelSpec[mCachedWifiCandidates.frequencies.size()];
                    for (Integer freq : mCachedWifiCandidates.frequencies) {
                        settings.channels[index++] = new WifiScanner.ChannelSpec(freq);
                    }
                    SingleScanListener singleScanListener = new SingleScanListener(false);
                    mScanner.startScan(settings, new HandlerExecutor(mEventHandler),
                            singleScanListener, WIFI_WORK_SOURCE);
                    mWifiMetrics.incrementConnectivityOneshotScanCount();
                }
            };

    /**
     * Interface for callback from handling scan results.
     */
    private interface HandleScanResultsListener {
        /**
         * @param wasCandidateSelected true - if a candidate is selected by WifiNetworkSelector
         *                             false - if no candidate is selected by WifiNetworkSelector
         */
        void onHandled(boolean wasCandidateSelected);
    }

    /**
     * Helper method to consolidate handling of scan results when no candidate is selected.
     */
    private void handleScanResultsWithNoCandidate(
            @NonNull HandleScanResultsListener handleScanResultsListener) {
        if (mWifiState == WIFI_STATE_DISCONNECTED) {
            mOpenNetworkNotifier.handleScanResults(
                    mNetworkSelector.getFilteredScanDetailsForOpenUnsavedNetworks());
        }
        mWifiMetrics.noteFirstNetworkSelectionAfterBoot(false);
        handleScanResultsListener.onHandled(false);
    }

    /**
     * Helper method to consolidate handling of scan results when a candidate is selected.
     */
    private void handleScanResultsWithCandidate(
            @NonNull HandleScanResultsListener handleScanResultsListener) {
        mWifiMetrics.noteFirstNetworkSelectionAfterBoot(true);
        handleScanResultsListener.onHandled(true);
    }

    /**
     * Handles 'onResult' callbacks for the Periodic, Single & Pno ScanListener.
     * Executes selection of potential network candidates, initiation of connection attempt to that
     * network.
     */
    private void handleScanResults(@NonNull List<ScanDetail> scanDetails,
            @NonNull String listenerName,
            boolean isFullScan,
            @NonNull HandleScanResultsListener handleScanResultsListener) {
        List<WifiNetworkSelector.ClientModeManagerState> cmmStates = new ArrayList<>();
        Set<String> connectedSsids = new HashSet<>();
        boolean hasExistingSecondaryCmm = false;
        for (ClientModeManager clientModeManager :
                mActiveModeWarden.getInternetConnectivityClientModeManagers()) {
            if (clientModeManager.getRole() == ROLE_CLIENT_SECONDARY_LONG_LIVED) {
                hasExistingSecondaryCmm = true;
            }
            mWifiChannelUtilization.refreshChannelStatsAndChannelUtilization(
                    clientModeManager.getWifiLinkLayerStats(),
                    WifiChannelUtilization.UNKNOWN_FREQ);
            WifiInfo wifiInfo = clientModeManager.syncRequestConnectionInfo();
            if (clientModeManager.isConnected()) {
                connectedSsids.add(wifiInfo.getSSID());
            }
            cmmStates.add(new WifiNetworkSelector.ClientModeManagerState(clientModeManager));
        }
        // We don't have any existing secondary CMM, but are we allowed to create a secondary CMM
        // and do we have a request for OEM_PAID/OEM_PRIVATE request? If yes, we need to perform
        // network selection to check if we have any potential candidate for the secondary CMM
        // creation.
        if (!hasExistingSecondaryCmm
                && (mOemPaidConnectionAllowed || mOemPrivateConnectionAllowed)) {
            // prefer OEM PAID requestor if it exists.
            WorkSource oemPaidOrOemPrivateRequestorWs =
                    mOemPaidConnectionRequestorWs != null
                            ? mOemPaidConnectionRequestorWs
                            : mOemPrivateConnectionRequestorWs;
            if (oemPaidOrOemPrivateRequestorWs == null) {
                Log.e(TAG, "Both mOemPaidConnectionRequestorWs & mOemPrivateConnectionRequestorWs "
                        + "are null!");
            }
            if (oemPaidOrOemPrivateRequestorWs != null
                    && mActiveModeWarden.canRequestMoreClientModeManagersInRole(
                            oemPaidOrOemPrivateRequestorWs,
                            ROLE_CLIENT_SECONDARY_LONG_LIVED)) {
                // Add a placeholder CMM state to ensure network selection is performed for a
                // potential second STA creation.
                cmmStates.add(new WifiNetworkSelector.ClientModeManagerState());
            }
        }
        // Check if any blocklisted BSSIDs can be freed.
        mWifiBlocklistMonitor.tryEnablingBlockedBssids(scanDetails);
        Set<String> bssidBlocklist = mWifiBlocklistMonitor.updateAndGetBssidBlocklistForSsids(
                connectedSsids);
        updateUserDisabledList(scanDetails);
        // Clear expired recent failure statuses
        mConfigManager.cleanupExpiredRecentFailureReasons();

        localLog(listenerName + " onResults: start network selection");

        List<WifiCandidates.Candidate> candidates = mNetworkSelector.getCandidatesFromScan(
                scanDetails, bssidBlocklist, cmmStates, mUntrustedConnectionAllowed,
                mOemPaidConnectionAllowed, mOemPrivateConnectionAllowed);
        mLatestCandidates = candidates;
        mLatestCandidatesTimestampMs = mClock.getElapsedSinceBootMillis();

        if (mDeviceMobilityState == WifiManager.DEVICE_MOBILITY_STATE_HIGH_MVMT
                && mContext.getResources().getBoolean(
                        R.bool.config_wifiHighMovementNetworkSelectionOptimizationEnabled)) {
            candidates = filterCandidatesHighMovement(candidates, listenerName, isFullScan);
        }

        mLastNetworkSelectionTimeStamp = mClock.getElapsedSinceBootMillis();
        mWifiLastResortWatchdog.updateAvailableNetworks(
                mNetworkSelector.getConnectableScanDetails());
        mWifiMetrics.countScanResults(scanDetails);
        // No candidates, return early.
        if (candidates == null || candidates.size() == 0) {
            localLog(listenerName + ":  No candidates");
            handleScanResultsWithNoCandidate(handleScanResultsListener);
            return;
        }
        // We have an oem paid/private network request and device supports STA + STA, check if there
        // are oem paid/private suggestions.
        if ((mOemPaidConnectionAllowed || mOemPrivateConnectionAllowed)
                && mActiveModeWarden.isStaStaConcurrencySupportedForRestrictedConnections()) {
            // Split the candidates based on whether they are oem paid/oem private or not.
            Map<Boolean, List<WifiCandidates.Candidate>> candidatesPartitioned =
                    candidates.stream()
                            .collect(Collectors.groupingBy(c -> c.isOemPaid() || c.isOemPrivate()));
            List<WifiCandidates.Candidate> primaryCmmCandidates =
                    candidatesPartitioned.getOrDefault(false, Collections.emptyList());
            List<WifiCandidates.Candidate> secondaryCmmCandidates =
                    candidatesPartitioned.getOrDefault(true, Collections.emptyList());
            // Some oem paid/private suggestions found, use secondary cmm flow.
            if (!secondaryCmmCandidates.isEmpty()) {
                handleCandidatesFromScanResultsUsingSecondaryCmmIfAvailable(
                        listenerName, primaryCmmCandidates, secondaryCmmCandidates,
                        handleScanResultsListener);
                return;
            }
            // intentional fallthrough: No oem paid/private suggestions, fallback to legacy flow.
        }
        handleCandidatesFromScanResultsForPrimaryCmmUsingMbbIfAvailable(
                listenerName, candidates, handleScanResultsListener);
    }

    /**
     * Executes selection of best network for 2 concurrent STA's from the candidates provided,
     * initiation of connection attempt to a network on both the STA's (if found).
     */
    private void handleCandidatesFromScanResultsUsingSecondaryCmmIfAvailable(
            @NonNull String listenerName,
            @NonNull List<WifiCandidates.Candidate> primaryCmmCandidates,
            @NonNull List<WifiCandidates.Candidate> secondaryCmmCandidates,
            @NonNull HandleScanResultsListener handleScanResultsListener) {
        // Perform network selection among secondary candidates.
        WifiConfiguration secondaryCmmCandidate =
                mNetworkSelector.selectNetwork(secondaryCmmCandidates);
        // No oem paid/private selected, fallback to legacy flow (should never happen!).
        if (secondaryCmmCandidate == null
                || secondaryCmmCandidate.getNetworkSelectionStatus().getCandidate() == null) {
            localLog(listenerName + ": No secondary candidate");
            handleCandidatesFromScanResultsForPrimaryCmmUsingMbbIfAvailable(
                    listenerName,
                    Stream.concat(primaryCmmCandidates.stream(), secondaryCmmCandidates.stream())
                            .collect(Collectors.toList()),
                    handleScanResultsListener);
            return;
        }
        String secondaryCmmCandidateBssid =
                secondaryCmmCandidate.getNetworkSelectionStatus().getCandidate().BSSID;
        WorkSource secondaryRequestorWs = null;
        // OEM_PAID takes precedence over OEM_PRIVATE, so attribute to OEM_PAID requesting app.
        if (secondaryCmmCandidate.oemPaid
                && mActiveModeWarden.canRequestMoreClientModeManagersInRole(
                mOemPaidConnectionRequestorWs, ROLE_CLIENT_SECONDARY_LONG_LIVED)) {
            secondaryRequestorWs = mOemPaidConnectionRequestorWs;
        } else if (secondaryCmmCandidate.oemPrivate
                && mActiveModeWarden.canRequestMoreClientModeManagersInRole(
                mOemPrivateConnectionRequestorWs, ROLE_CLIENT_SECONDARY_LONG_LIVED)) {
            secondaryRequestorWs = mOemPrivateConnectionRequestorWs;
        }
        // Secondary STA not available, fallback to legacy flow.
        if (secondaryRequestorWs == null) {
            localLog(listenerName + ": No secondary STA available");
            handleCandidatesFromScanResultsForPrimaryCmmUsingMbbIfAvailable(
                    listenerName,
                    Stream.concat(primaryCmmCandidates.stream(), secondaryCmmCandidates.stream())
                            .collect(Collectors.toList()),
                    handleScanResultsListener);
            return;
        }
        WifiConfiguration primaryCmmCandidate =
                mNetworkSelector.selectNetwork(primaryCmmCandidates);
        // Request for a new client mode manager to spin up concurrent connection
        mActiveModeWarden.requestSecondaryLongLivedClientModeManager(
                (cm) -> {
                    if (cm == null) {
                        localLog(listenerName + ": Secondary client mode manager request returned "
                                + "null, aborting (wifi off?)");
                        handleScanResultsWithNoCandidate(handleScanResultsListener);
                        return;
                    }
                    // We did not end up getting the secondary client mode manager for some reason
                    // after we checked above! Fallback to legacy flow.
                    if (cm.getRole() == ROLE_CLIENT_PRIMARY) {
                        localLog(listenerName + ": Secondary client mode manager request returned"
                                + " primary, falling back to single client mode manager flow.");
                        handleCandidatesFromScanResultsForPrimaryCmmUsingMbbIfAvailable(
                                listenerName,
                                Stream.concat(primaryCmmCandidates.stream(),
                                        secondaryCmmCandidates.stream())
                                        .collect(Collectors.toList()),
                                handleScanResultsListener);
                        return;
                    }
                    // Don't use make before break for these connection requests.

                    // If we also selected a primary candidate trigger connection.
                    if (primaryCmmCandidate != null) {
                        localLog(listenerName + ":  WNS candidate(primary)-"
                                + primaryCmmCandidate.SSID);
                        connectToNetworkUsingCmmWithoutMbb(
                                getPrimaryClientModeManager(), primaryCmmCandidate);
                    }

                    localLog(listenerName + ":  WNS candidate(secondary)-"
                            + secondaryCmmCandidate.SSID);
                    // Secndary candidate cannot be null (otherwise we would have switched to legacy
                    // flow above)
                    connectToNetworkUsingCmmWithoutMbb(cm, secondaryCmmCandidate);

                    handleScanResultsWithCandidate(handleScanResultsListener);
                }, secondaryRequestorWs,
                secondaryCmmCandidate.SSID,
                mConnectivityHelper.isFirmwareRoamingSupported()
                        ? null : secondaryCmmCandidateBssid);
    }

    /**
     * Executes selection of best network from the candidates provided, initiation of connection
     * attempt to that network.
     */
    private void handleCandidatesFromScanResultsForPrimaryCmmUsingMbbIfAvailable(
            @NonNull String listenerName, @NonNull List<WifiCandidates.Candidate> candidates,
            @NonNull HandleScanResultsListener handleScanResultsListener) {
        WifiConfiguration candidate = mNetworkSelector.selectNetwork(candidates);
        if (candidate != null) {
            localLog(listenerName + ":  WNS candidate-" + candidate.SSID);
            connectToNetworkForPrimaryCmmUsingMbbIfAvailable(candidate);
            handleScanResultsWithCandidate(handleScanResultsListener);
        } else {
            localLog(listenerName + ":  No candidate");
            handleScanResultsWithNoCandidate(handleScanResultsListener);
        }
    }

    private List<WifiCandidates.Candidate> filterCandidatesHighMovement(
            List<WifiCandidates.Candidate> candidates, String listenerName, boolean isFullScan) {
        boolean isNotPartialScan = isFullScan || listenerName.equals(PNO_SCAN_LISTENER);
        if (candidates == null || candidates.isEmpty()) {
            // No connectable networks nearby or network selection is unnecessary
            if (isNotPartialScan) {
                mCachedWifiCandidates = new CachedWifiCandidates(mClock.getElapsedSinceBootMillis(),
                        null);
            }
            return null;
        }

        long minimumTimeBetweenScansMs = mContext.getResources().getInteger(
                R.integer.config_wifiHighMovementNetworkSelectionOptimizationScanDelayMs);
        if (mCachedWifiCandidates != null && mCachedWifiCandidates.candidateRssiMap != null) {
            // cached candidates are too recent, wait for next scan
            if (mClock.getElapsedSinceBootMillis() - mCachedWifiCandidates.timeSinceBootMs
                    < minimumTimeBetweenScansMs) {
                mWifiMetrics.incrementNumHighMovementConnectionSkipped();
                return null;
            }

            int rssiDelta = mContext.getResources().getInteger(R.integer
                    .config_wifiHighMovementNetworkSelectionOptimizationRssiDelta);
            List<WifiCandidates.Candidate> filteredCandidates = candidates.stream().filter(
                    item -> mCachedWifiCandidates.candidateRssiMap.containsKey(item.getKey())
                            && Math.abs(mCachedWifiCandidates.candidateRssiMap.get(item.getKey())
                            - item.getScanRssi()) < rssiDelta)
                    .collect(Collectors.toList());

            if (!filteredCandidates.isEmpty()) {
                if (isNotPartialScan) {
                    mCachedWifiCandidates =
                            new CachedWifiCandidates(mClock.getElapsedSinceBootMillis(),
                            candidates);
                }
                mWifiMetrics.incrementNumHighMovementConnectionStarted();
                return filteredCandidates;
            }
        }

        // Either no cached candidates, or all candidates got filtered out.
        // Update the cached candidates here and schedule a delayed partial scan.
        if (isNotPartialScan) {
            mCachedWifiCandidates = new CachedWifiCandidates(mClock.getElapsedSinceBootMillis(),
                    candidates);
            localLog("Found " + candidates.size() + " candidates at high mobility state. "
                    + "Re-doing scan to confirm network quality.");
            scheduleDelayedPartialScan(minimumTimeBetweenScansMs);
        }
        mWifiMetrics.incrementNumHighMovementConnectionSkipped();
        return null;
    }

    private void updateUserDisabledList(List<ScanDetail> scanDetails) {
        List<String> results = new ArrayList<>();
        List<ScanResult> passpointAp = new ArrayList<>();
        for (ScanDetail scanDetail : scanDetails) {
            results.add(ScanResultUtil.createQuotedSSID(scanDetail.getScanResult().SSID));
            if (!scanDetail.getScanResult().isPasspointNetwork()) {
                continue;
            }
            passpointAp.add(scanDetail.getScanResult());
        }
        if (!passpointAp.isEmpty()) {
            results.addAll(mPasspointManager
                    .getAllMatchingPasspointProfilesForScanResults(passpointAp).keySet());
        }
        mConfigManager.updateUserDisabledList(results);
    }

    private class CachedWifiCandidates {
        public final long timeSinceBootMs;
        public final Map<WifiCandidates.Key, Integer> candidateRssiMap;
        public final Set<Integer> frequencies;

        CachedWifiCandidates(long timeSinceBootMs, List<WifiCandidates.Candidate> candidates) {
            this.timeSinceBootMs = timeSinceBootMs;
            if (candidates == null) {
                this.candidateRssiMap = null;
                this.frequencies = null;
            } else {
                this.candidateRssiMap = new ArrayMap<WifiCandidates.Key, Integer>();
                this.frequencies = new HashSet<Integer>();
                for (WifiCandidates.Candidate c : candidates) {
                    candidateRssiMap.put(c.getKey(), c.getScanRssi());
                    frequencies.add(c.getFrequency());
                }
            }
        }
    }

    // All single scan results listener.
    //
    // Note: This is the listener for all the available single scan results,
    //       including the ones initiated by WifiConnectivityManager and
    //       other modules.
    private class AllSingleScanListener implements WifiScanner.ScanListener {
        private List<ScanDetail> mScanDetails = new ArrayList<ScanDetail>();
        private int mNumScanResultsIgnoredDueToSingleRadioChain = 0;

        public void clearScanDetails() {
            mScanDetails.clear();
            mNumScanResultsIgnoredDueToSingleRadioChain = 0;
        }

        @Override
        public void onSuccess() {
        }

        @Override
        public void onFailure(int reason, String description) {
            localLog("registerScanListener onFailure:"
                      + " reason: " + reason + " description: " + description);
        }

        @Override
        public void onPeriodChanged(int periodInMs) {
        }

        @Override
        public void onResults(WifiScanner.ScanData[] results) {
            if (!mWifiEnabled || !mAutoJoinEnabled) {
                clearScanDetails();
                mWaitForFullBandScanResults = false;
                return;
            }

            // We treat any full band scans (with DFS or not) as "full".
            boolean isFullBandScanResults = false;
            if (results != null && results.length > 0) {
                isFullBandScanResults =
                        WifiScanner.isFullBandScan(results[0].getScannedBandsInternal(), true);
            }
            // Full band scan results only.
            if (mWaitForFullBandScanResults) {
                if (!isFullBandScanResults) {
                    localLog("AllSingleScanListener waiting for full band scan results.");
                    clearScanDetails();
                    return;
                } else {
                    mWaitForFullBandScanResults = false;
                }
            }

            // Create a new list to avoid looping call trigger concurrent exception.
            List<ScanDetail> scanDetailList = new ArrayList<>(mScanDetails);
            clearScanDetails();

            if (results != null && results.length > 0) {
                mWifiMetrics.incrementAvailableNetworksHistograms(scanDetailList,
                        isFullBandScanResults);
            }
            if (mNumScanResultsIgnoredDueToSingleRadioChain > 0) {
                Log.i(TAG, "Number of scan results ignored due to single radio chain scan: "
                        + mNumScanResultsIgnoredDueToSingleRadioChain);
            }
            handleScanResults(scanDetailList,
                    ALL_SINGLE_SCAN_LISTENER, isFullBandScanResults,
                    wasCandidateSelected -> {
                        // Update metrics to see if a single scan detected a valid network
                        // while PNO scan didn't.
                        // Note: We don't update the background scan metrics any more as it is
                        //       not in use.
                        if (mPnoScanStarted) {
                            if (wasCandidateSelected) {
                                mWifiMetrics.incrementNumConnectivityWatchdogPnoBad();
                            } else {
                                mWifiMetrics.incrementNumConnectivityWatchdogPnoGood();
                            }
                        }

                        // Check if we are in the middle of initial partial scan
                        if (mInitialScanState == INITIAL_SCAN_STATE_AWAITING_RESPONSE) {
                            // Done with initial scan
                            setInitialScanState(INITIAL_SCAN_STATE_COMPLETE);

                            if (wasCandidateSelected) {
                                Log.i(TAG, "Connection attempted with the reduced initial scans");
                                mWifiMetrics.reportInitialPartialScan(
                                        mInitialPartialScanChannelCount, true);
                                mInitialPartialScanChannelCount = 0;
                            } else {
                                Log.i(TAG, "Connection was not attempted, issuing a full scan");
                                startConnectivityScan(SCAN_IMMEDIATELY);
                                mFailedInitialPartialScan = true;
                            }
                        } else if (mInitialScanState == INITIAL_SCAN_STATE_COMPLETE) {
                            if (mFailedInitialPartialScan && wasCandidateSelected) {
                                // Initial scan failed, but following full scan succeeded
                                mWifiMetrics.reportInitialPartialScan(
                                        mInitialPartialScanChannelCount, false);
                            }
                            mFailedInitialPartialScan = false;
                            mInitialPartialScanChannelCount = 0;
                        }
                    });
        }

        @Override
        public void onFullResult(ScanResult fullScanResult) {
            if (!mWifiEnabled || !mAutoJoinEnabled) {
                return;
            }

            if (mDbg) {
                localLog("AllSingleScanListener onFullResult: " + fullScanResult.SSID
                        + " capabilities " + fullScanResult.capabilities);
            }

            // When the scan result has radio chain info, ensure we throw away scan results
            // not received with both radio chains (if |mUseSingleRadioChainScanResults| is false).
            if (!mContext.getResources().getBoolean(
                    R.bool.config_wifi_framework_use_single_radio_chain_scan_results_network_selection)
                    && fullScanResult.radioChainInfos != null
                    && fullScanResult.radioChainInfos.length == 1) {
                // Keep track of the number of dropped scan results for logging.
                mNumScanResultsIgnoredDueToSingleRadioChain++;
                return;
            }

            mScanDetails.add(ScanResultUtil.toScanDetail(fullScanResult));
        }
    }

    private final AllSingleScanListener mAllSingleScanListener = new AllSingleScanListener();

    // Single scan results listener. A single scan is initiated when
    // DisconnectedPNO scan found a valid network and woke up
    // the system, or by the watchdog timer, or to form the timer based
    // periodic scan.
    //
    // Note: This is the listener for the single scans initiated by the
    //        WifiConnectivityManager.
    private class SingleScanListener implements WifiScanner.ScanListener {
        private final boolean mIsFullBandScan;

        SingleScanListener(boolean isFullBandScan) {
            mIsFullBandScan = isFullBandScan;
        }

        @Override
        public void onSuccess() {
        }

        @Override
        public void onFailure(int reason, String description) {
            localLog("SingleScanListener onFailure:"
                    + " reason: " + reason + " description: " + description);

            // reschedule the scan
            if (mSingleScanRestartCount++ < MAX_SCAN_RESTART_ALLOWED) {
                scheduleDelayedSingleScan(mIsFullBandScan);
            } else {
                mSingleScanRestartCount = 0;
                localLog("Failed to successfully start single scan for "
                        + MAX_SCAN_RESTART_ALLOWED + " times");
            }
        }

        @Override
        public void onPeriodChanged(int periodInMs) {
            localLog("SingleScanListener onPeriodChanged: "
                    + "actual scan period " + periodInMs + "ms");
        }

        @Override
        public void onResults(WifiScanner.ScanData[] results) {
            mSingleScanRestartCount = 0;
        }

        @Override
        public void onFullResult(ScanResult fullScanResult) {
        }
    }

    // PNO scan results listener for both disconnected and connected PNO scanning.
    // A PNO scan is initiated when screen is off.
    private class PnoScanListener implements WifiScanner.PnoScanListener {
        private List<ScanDetail> mScanDetails = new ArrayList<ScanDetail>();
        private int mLowRssiNetworkRetryDelayMs;

        private void limitLowRssiNetworkRetryDelay() {
            mLowRssiNetworkRetryDelayMs = Math.min(mLowRssiNetworkRetryDelayMs,
                    mContext.getResources().getInteger(R.integer
                            .config_wifiPnoScanLowRssiNetworkRetryMaxDelaySec) * 1000);
        }

        public void clearScanDetails() {
            mScanDetails.clear();
        }

        // Reset to the start value when either a non-PNO scan is started or
        // WifiNetworkSelector selects a candidate from the PNO scan results.
        public void resetLowRssiNetworkRetryDelay() {
            mLowRssiNetworkRetryDelayMs = mContext.getResources().getInteger(R.integer
                    .config_wifiPnoScanLowRssiNetworkRetryStartDelaySec) * 1000;
        }

        @VisibleForTesting
        public int getLowRssiNetworkRetryDelay() {
            return mLowRssiNetworkRetryDelayMs;
        }

        @Override
        public void onSuccess() {
        }

        @Override
        public void onFailure(int reason, String description) {
            localLog("PnoScanListener onFailure:"
                    + " reason: " + reason + " description: " + description);

            // reschedule the scan
            if (mScanRestartCount++ < MAX_SCAN_RESTART_ALLOWED) {
                scheduleDelayedConnectivityScan(RESTART_SCAN_DELAY_MS);
            } else {
                mScanRestartCount = 0;
                localLog("Failed to successfully start PNO scan for "
                        + MAX_SCAN_RESTART_ALLOWED + " times");
            }
        }

        @Override
        public void onPeriodChanged(int periodInMs) {
            localLog("PnoScanListener onPeriodChanged: "
                    + "actual scan period " + periodInMs + "ms");
        }

        // Currently the PNO scan results doesn't include IE,
        // which contains information required by WifiNetworkSelector. Ignore them
        // for now.
        @Override
        public void onResults(WifiScanner.ScanData[] results) {
        }

        @Override
        public void onFullResult(ScanResult fullScanResult) {
        }

        @Override
        public void onPnoNetworkFound(ScanResult[] results) {
            for (ScanResult result: results) {
                if (result.informationElements == null) {
                    localLog("Skipping scan result with null information elements");
                    continue;
                }
                mScanDetails.add(ScanResultUtil.toScanDetail(result));
            }

            // Create a new list to avoid looping call trigger concurrent exception.
            List<ScanDetail> scanDetailList = new ArrayList<>(mScanDetails);
            clearScanDetails();
            mScanRestartCount = 0;

            handleScanResults(scanDetailList, PNO_SCAN_LISTENER, false,
                    wasCandidateSelected -> {
                        if (!wasCandidateSelected) {
                            // The scan results were rejected by WifiNetworkSelector due to low
                            // RSSI values
                            // Lazy initialization
                            if (mLowRssiNetworkRetryDelayMs == 0) {
                                resetLowRssiNetworkRetryDelay();
                            }
                            scheduleDelayedConnectivityScan(mLowRssiNetworkRetryDelayMs);

                            // Set up the delay value for next retry.
                            mLowRssiNetworkRetryDelayMs *= 2;
                            limitLowRssiNetworkRetryDelay();
                        } else {
                            resetLowRssiNetworkRetryDelay();
                        }
                    });
        }
    }

    private final PnoScanListener mPnoScanListener = new PnoScanListener();

    private class OnNetworkUpdateListener implements
            WifiConfigManager.OnNetworkUpdateListener {
        @Override
        public void onNetworkAdded(WifiConfiguration config) {
            triggerScanOnNetworkChanges();
        }
        @Override
        public void onNetworkEnabled(WifiConfiguration config) {
            triggerScanOnNetworkChanges();
        }
        @Override
        public void onNetworkRemoved(WifiConfiguration config) {
            triggerScanOnNetworkChanges();
        }
        @Override
        public void onNetworkUpdated(WifiConfiguration newConfig, WifiConfiguration oldConfig) {
            triggerScanOnNetworkChanges();
        }

        @Override
        public void onNetworkPermanentlyDisabled(WifiConfiguration config, int disableReason) {
            triggerScanOnNetworkChanges();
        }
    }

    private class OnSuggestionUpdateListener implements
            WifiNetworkSuggestionsManager.OnSuggestionUpdateListener {
        @Override
        public void onSuggestionsAddedOrUpdated(List<WifiNetworkSuggestion> suggestions) {
            triggerScanOnNetworkChanges();
        }

        @Override
        public void onSuggestionsRemoved(List<WifiNetworkSuggestion> suggestions) {
            triggerScanOnNetworkChanges();
        }
    }

    private class ModeChangeCallback implements ActiveModeWarden.ModeChangeCallback {
        @Override
        public void onActiveModeManagerAdded(@NonNull ActiveModeManager activeModeManager) {
            update();
        }

        @Override
        public void onActiveModeManagerRemoved(@NonNull ActiveModeManager activeModeManager) {
            update();
        }

        @Override
        public void onActiveModeManagerRoleChanged(@NonNull ActiveModeManager activeModeManager) {
            update();
        }

        private void update() {
            List<ClientModeManager> primaryManagers =
                    mActiveModeWarden.getInternetConnectivityClientModeManagers();
            setWifiEnabled(!primaryManagers.isEmpty());
        }
    }

    /**
     * WifiConnectivityManager constructor
     */
    WifiConnectivityManager(
            WifiContext context,
            ScoringParams scoringParams,
            WifiConfigManager configManager,
            WifiNetworkSuggestionsManager wifiNetworkSuggestionsManager,
            WifiNetworkSelector networkSelector,
            WifiConnectivityHelper connectivityHelper,
            WifiLastResortWatchdog wifiLastResortWatchdog,
            OpenNetworkNotifier openNetworkNotifier,
            WifiMetrics wifiMetrics,
            Handler handler,
            Clock clock,
            LocalLog localLog,
            WifiScoreCard scoreCard,
            WifiBlocklistMonitor wifiBlocklistMonitor,
            WifiChannelUtilization wifiChannelUtilization,
            PasspointManager passpointManager,
            DeviceConfigFacade deviceConfigFacade,
            ActiveModeWarden activeModeWarden,
            WifiGlobals wifiGlobals) {
        mContext = context;
        mScoringParams = scoringParams;
        mConfigManager = configManager;
        mWifiNetworkSuggestionsManager = wifiNetworkSuggestionsManager;
        mNetworkSelector = networkSelector;
        mConnectivityHelper = connectivityHelper;
        mWifiLastResortWatchdog = wifiLastResortWatchdog;
        mOpenNetworkNotifier = openNetworkNotifier;
        mWifiMetrics = wifiMetrics;
        mEventHandler = handler;
        mClock = clock;
        mLocalLog = localLog;
        mWifiScoreCard = scoreCard;
        mWifiBlocklistMonitor = wifiBlocklistMonitor;
        mWifiChannelUtilization = wifiChannelUtilization;
        mPasspointManager = passpointManager;
        mDeviceConfigFacade = deviceConfigFacade;
        mActiveModeWarden = activeModeWarden;
        mWifiGlobals = wifiGlobals;

        mAlarmManager = context.getSystemService(AlarmManager.class);
        mPowerManager = mContext.getSystemService(PowerManager.class);

        // Listen for screen state change events.
        // TODO: We should probably add a shared broadcast receiver in the wifi stack which
        // can used by various modules to listen to common system events. Creating multiple
        // broadcast receivers in each class within the wifi stack is *somewhat* expensive.
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        mContext.registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        String action = intent.getAction();
                        if (action.equals(Intent.ACTION_SCREEN_ON)) {
                            handleScreenStateChanged(true);
                        } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                            handleScreenStateChanged(false);
                        }
                    }
                }, filter, null, mEventHandler);
        handleScreenStateChanged(mPowerManager.isInteractive());

        // Listen to WifiConfigManager network update events
        mConfigManager.addOnNetworkUpdateListener(new OnNetworkUpdateListener());
        // Listen to WifiNetworkSuggestionsManager suggestion update events
        mWifiNetworkSuggestionsManager.addOnSuggestionUpdateListener(
                new OnSuggestionUpdateListener());
        mActiveModeWarden.registerModeChangeCallback(new ModeChangeCallback());
    }

    @NonNull
    private WifiInfo getPrimaryWifiInfo() {
        return getPrimaryClientModeManager().syncRequestConnectionInfo();
    }

    private ClientModeManager getPrimaryClientModeManager() {
        // There should only be 1 primary client mode manager at any point of time.
        return mActiveModeWarden.getPrimaryClientModeManager();
    }

    /** Initialize single scanning schedules, and validate them */
    private int[] initializeScanningSchedule(int state) {
        int[] scheduleSec;

        if (state == WIFI_STATE_CONNECTED) {
            scheduleSec = mContext.getResources().getIntArray(
                    R.array.config_wifiConnectedScanIntervalScheduleSec);
        } else if (state == WIFI_STATE_DISCONNECTED) {
            scheduleSec = mContext.getResources().getIntArray(
                    R.array.config_wifiDisconnectedScanIntervalScheduleSec);
        } else {
            scheduleSec = null;
        }

        boolean invalidConfig = false;
        if (scheduleSec == null || scheduleSec.length == 0) {
            invalidConfig = true;
        } else {
            for (int val : scheduleSec) {
                if (val <= 0) {
                    invalidConfig = true;
                    break;
                }
            }
        }
        if (!invalidConfig) {
            return scheduleSec;
        }

        Log.e(TAG, "Configuration for wifi scanning schedule is mis-configured,"
                + "using default schedule");
        return DEFAULT_SCANNING_SCHEDULE_SEC;
    }

    /**
     * This checks the connection attempt rate and recommends whether the connection attempt
     * should be skipped or not. This attempts to rate limit the rate of connections to
     * prevent us from flapping between networks and draining battery rapidly.
     */
    private boolean shouldSkipConnectionAttempt(long timeMillis) {
        Iterator<Long> attemptIter = mConnectionAttemptTimeStamps.iterator();
        // First evict old entries from the queue.
        while (attemptIter.hasNext()) {
            Long connectionAttemptTimeMillis = attemptIter.next();
            if ((timeMillis - connectionAttemptTimeMillis)
                    > MAX_CONNECTION_ATTEMPTS_TIME_INTERVAL_MS) {
                attemptIter.remove();
            } else {
                // This list is sorted by timestamps, so we can skip any more checks
                break;
            }
        }
        // If we've reached the max connection attempt rate, skip this connection attempt
        return (mConnectionAttemptTimeStamps.size() >= MAX_CONNECTION_ATTEMPTS_RATE);
    }

    /**
     * Add the current connection attempt timestamp to our queue of connection attempts.
     */
    private void noteConnectionAttempt(long timeMillis) {
        localLog("noteConnectionAttempt: timeMillis=" + timeMillis);
        mConnectionAttemptTimeStamps.addLast(timeMillis);
    }

    /**
     * This is used to clear the connection attempt rate limiter. This is done when the user
     * explicitly tries to connect to a specified network.
     */
    private void clearConnectionAttemptTimeStamps() {
        mConnectionAttemptTimeStamps.clear();
    }

    private static <T> T coalesce(T a, T  b) {
        return a != null ? a : b;
    }

    private boolean isClientModeManagerConnectedOrConnectingToCandidate(
            ClientModeManager clientModeManager, WifiConfiguration candidate) {
        int targetNetworkId = candidate.networkId;
        WifiConfiguration connectedOrConnectingWifiConfiguration = coalesce(
                clientModeManager.getConnectingWifiConfiguration(),
                clientModeManager.getConnectedWifiConfiguration());
        boolean connectingOrConnectedToTarget =
                connectedOrConnectingWifiConfiguration != null
                        && (targetNetworkId == connectedOrConnectingWifiConfiguration.networkId
                        || (mContext.getResources().getBoolean(
                                R.bool.config_wifiEnableLinkedNetworkRoaming)
                        && connectedOrConnectingWifiConfiguration.isLinked(candidate)));

        // Is Firmware roaming control is supported?
        //   - Yes, framework does nothing, firmware will roam if necessary.
        //   - No, framework initiates roaming.
        if (mConnectivityHelper.isFirmwareRoamingSupported()) {
            // just check for networkID.
            return connectingOrConnectedToTarget;
        }

        // check for networkID and BSSID.
        String connectedOrConnectingBssid = coalesce(
                clientModeManager.getConnectingBssid(),
                clientModeManager.getConnectedBssid());
        ScanResult scanResultCandidate =
                candidate.getNetworkSelectionStatus().getCandidate();
        String targetBssid = scanResultCandidate.BSSID;
        return connectingOrConnectedToTarget
                && Objects.equals(targetBssid, connectedOrConnectingBssid);
    }

    /**
     * Trigger network connection for primary client mode manager using make before break.
     *
     * Note: This may trigger make before break on a secondary STA if available which will
     * eventually become primary after validation or torn down if it does not become primary.
     */
    private void connectToNetworkForPrimaryCmmUsingMbbIfAvailable(
            @NonNull WifiConfiguration candidate) {
        ClientModeManager primaryManager = mActiveModeWarden.getPrimaryClientModeManager();
        connectToNetworkUsingCmm(
                primaryManager, candidate,
                new ConnectHandler() {
                    @Override
                    public void triggerConnectWhenDisconnected(
                            WifiConfiguration targetNetwork,
                            String targetBssid) {
                        triggerConnectToNetworkUsingCmm(primaryManager, targetNetwork, targetBssid);
                        // since using primary manager to connect, stop any existing managers in the
                        // secondary transient role since they are no longer needed.
                        mActiveModeWarden.stopAllClientModeManagersInRole(
                                ROLE_CLIENT_SECONDARY_TRANSIENT);
                    }

                    @Override
                    public void triggerConnectWhenConnected(
                            WifiConfiguration currentNetwork,
                            WifiConfiguration targetNetwork,
                            String targetBssid) {
                        mWifiMetrics.incrementWifiToWifiSwitchTriggerCount();
                        // If both the current & target networks have MAC randomization disabled,
                        // we cannot use MBB because then both ifaces would need to use the exact
                        // same MAC address (the "designated" factory MAC for the device), which is
                        // illegal. Fallback to single STA behavior.

                        // TODO(b/172086124): Possibly move this logic to
                        // ActiveModeWarden.handleAdditionalClientModeManagerRequest() to
                        // ensure that all fallback logic in 1 central place (all the necessary
                        // info is already included in the secondary STA creation request).
                        if (currentNetwork.macRandomizationSetting == RANDOMIZATION_NONE
                                && targetNetwork.macRandomizationSetting == RANDOMIZATION_NONE) {
                            triggerConnectToNetworkUsingCmm(
                                    primaryManager, targetNetwork, targetBssid);
                            // since using primary manager to connect, stop any existing managers in
                            // the secondary transient role since they are no longer needed.
                            mActiveModeWarden.stopAllClientModeManagersInRole(
                                    ROLE_CLIENT_SECONDARY_TRANSIENT);
                            return;
                        }
                        // Else, use MBB if available.
                        triggerConnectToNetworkUsingMbbIfAvailable(targetNetwork, targetBssid);
                    }

                    @Override
                    public void triggerRoamWhenConnected(
                            WifiConfiguration currentNetwork,
                            WifiConfiguration targetNetwork,
                            String targetBssid) {
                        triggerRoamToNetworkUsingCmm(
                                primaryManager, targetNetwork, targetBssid);
                        // since using primary manager to connect, stop any existing managers in the
                        // secondary transient role since they are no longer needed.
                        mActiveModeWarden.stopAllClientModeManagersInRole(
                                ROLE_CLIENT_SECONDARY_TRANSIENT);
                    }
                });

    }

    /**
     * Trigger network connection for provided client mode manager without using make before break.
     */
    private void connectToNetworkUsingCmmWithoutMbb(
            @NonNull ClientModeManager clientModeManager, @NonNull WifiConfiguration candidate) {
        connectToNetworkUsingCmm(clientModeManager, candidate,
                new ConnectHandler() {
                    @Override
                    public void triggerConnectWhenDisconnected(
                            WifiConfiguration targetNetwork,
                            String targetBssid) {
                        triggerConnectToNetworkUsingCmm(
                                clientModeManager, targetNetwork, targetBssid);
                    }

                    @Override
                    public void triggerConnectWhenConnected(
                            WifiConfiguration currentNetwork,
                            WifiConfiguration targetNetwork,
                            String targetBssid) {
                        triggerConnectToNetworkUsingCmm(
                                clientModeManager, targetNetwork, targetBssid);
                    }

                    @Override
                    public void triggerRoamWhenConnected(
                            WifiConfiguration currentNetwork,
                            WifiConfiguration targetNetwork,
                            String targetBssid) {
                        triggerRoamToNetworkUsingCmm(
                                clientModeManager, targetNetwork, targetBssid);
                    }
                });
    }

    /**
     * Interface to use for trigger connection in various scenarios.
     */
    private interface ConnectHandler {
        /**
         * Invoked to trigger connection to a network when disconnected.
         */
        void triggerConnectWhenDisconnected(
                @NonNull WifiConfiguration targetNetwork, @NonNull String targetBssid);
        /**
         * Invoked to trigger connection to a network when connected to a different network.
         */
        void triggerConnectWhenConnected(
                @NonNull WifiConfiguration currentNetwork, @NonNull WifiConfiguration targetNetwork,
                @NonNull String targetBssid);
        /**
         * Invoked to trigger roam to a specific bssid network when connected to a network.
         */
        void triggerRoamWhenConnected(
                @NonNull WifiConfiguration currentNetwork, @NonNull WifiConfiguration targetNetwork,
                @NonNull String targetBssid);
    }

    private String getAssociationId(@Nullable WifiConfiguration config, @Nullable String bssid) {
        return config == null ? "Disconnected" : config.SSID + " : " + bssid;
    }

    /**
     * Attempt to connect to a network candidate.
     *
     * Based on the currently connected network, this method determines whether we should
     * connect or roam to the network candidate recommended by WifiNetworkSelector.
     */
    private void connectToNetworkUsingCmm(@NonNull ClientModeManager clientModeManager,
            @NonNull WifiConfiguration targetNetwork,
            @NonNull ConnectHandler connectHandler) {
        if (targetNetwork.getNetworkSelectionStatus().getCandidate() == null) {
            localLog("connectToNetwork(" + clientModeManager + "): bad candidate - "
                    + targetNetwork + " scanResult is null!");
            return;
        }
        String targetBssid = targetNetwork.getNetworkSelectionStatus().getCandidate().BSSID;
        String targetAssociationId = getAssociationId(targetNetwork, targetBssid);

        if (isClientModeManagerConnectedOrConnectingToCandidate(clientModeManager, targetNetwork)) {
            localLog("connectToNetwork(" + clientModeManager + "): either already connected or is "
                    + "connecting to " + targetAssociationId);
            return;
        }

        if (targetNetwork.BSSID != null
                && !targetNetwork.BSSID.equals(ClientModeImpl.SUPPLICANT_BSSID_ANY)
                && !targetNetwork.BSSID.equals(targetBssid)) {
            localLog("connectToNetwork(" + clientModeManager + "): target BSSID " + targetBssid
                    + " does not match the config specified BSSID " + targetNetwork.BSSID
                    + ". Drop it!");
            return;
        }

        WifiConfiguration currentNetwork = coalesce(
                clientModeManager.getConnectedWifiConfiguration(),
                clientModeManager.getConnectingWifiConfiguration());
        String currentBssid = coalesce(
                clientModeManager.getConnectedBssid(), clientModeManager.getConnectingBssid());
        String currentAssociationId = getAssociationId(currentNetwork, currentBssid);

        // Already on desired network id, we need to trigger roam since the device does not
        // support firmware roaming (already checked in
        // isClientModeManagerConnectedOrConnectingToCandidate()).
        if (currentNetwork != null
                && (currentNetwork.networkId == targetNetwork.networkId
                || (mContext.getResources().getBoolean(R.bool.config_wifiEnableLinkedNetworkRoaming)
                && currentNetwork.isLinked(targetNetwork)))) {
            localLog("connectToNetwork(" + clientModeManager + "): Roam to " + targetAssociationId
                    + " from " + currentAssociationId);
            connectHandler.triggerRoamWhenConnected(currentNetwork, targetNetwork, targetBssid);
            return;
        }

        // Need to connect to a different network id
        // Framework specifies the connection target BSSID if firmware doesn't support
        // {@link android.net.wifi.WifiManager#WIFI_FEATURE_CONTROL_ROAMING} or the
        // candidate configuration contains a specified BSSID.
        if (mConnectivityHelper.isFirmwareRoamingSupported()
                && (targetNetwork.BSSID == null
                || targetNetwork.BSSID.equals(ClientModeImpl.SUPPLICANT_BSSID_ANY))) {
            targetBssid = ClientModeImpl.SUPPLICANT_BSSID_ANY;
        }
        localLog("connectToNetwork(" + clientModeManager + "): Connect to "
                + getAssociationId(targetNetwork, targetBssid) + " from "
                + currentAssociationId);
        if (currentNetwork == null) {
            connectHandler.triggerConnectWhenDisconnected(targetNetwork, targetBssid);
            return;
        }
        connectHandler.triggerConnectWhenConnected(currentNetwork, targetNetwork, targetBssid);
    }

    private boolean shouldConnect() {
        long elapsedTimeMillis = mClock.getElapsedSinceBootMillis();
        if (!mScreenOn && shouldSkipConnectionAttempt(elapsedTimeMillis)) {
            localLog("connectToNetwork: Too many connection attempts. Skipping this attempt!");
            mTotalConnectivityAttemptsRateLimited++;
            return false;
        }
        noteConnectionAttempt(elapsedTimeMillis);
        return true;
    }

    /**
     * Trigger roaming to a new bssid while being connected to a different bssid in same network.
     */
    private void triggerRoamToNetworkUsingCmm(
            @NonNull ClientModeManager clientModeManager,
            @NonNull WifiConfiguration targetNetwork,
            @NonNull String targetBssid) {
        if (!shouldConnect()) {
            return;
        }
        clientModeManager.startRoamToNetwork(targetNetwork.networkId, targetBssid);
    }

    /**
     * Trigger connection to a new wifi network while being disconnected.
     */
    private void triggerConnectToNetworkUsingCmm(
            @NonNull ClientModeManager clientModeManager,
            @NonNull WifiConfiguration targetNetwork, @NonNull String targetBssid) {
        if (!shouldConnect()) {
            return;
        }
        clientModeManager.startConnectToNetwork(
                targetNetwork.networkId, Process.WIFI_UID, targetBssid);
    }

    /**
     * Trigger connection to a new wifi network while being connected to another network.
     * Depending on device configuration, this uses
     *  - MBB make before break (Dual STA), or
     *  - BBM break before make (Single STA)
     */
    private void triggerConnectToNetworkUsingMbbIfAvailable(
            @NonNull WifiConfiguration targetNetwork, @NonNull String targetBssid) {
        // Request a ClientModeManager from ActiveModeWarden to connect with - may be an existing
        // CMM or a newly created one (potentially switching networks using Make-Before-Break)
        mActiveModeWarden.requestSecondaryTransientClientModeManager(
                (@Nullable ClientModeManager clientModeManager) -> {
                    localLog("connectToNetwork: received requested ClientModeManager "
                            + clientModeManager);
                    if (clientModeManager == null) {
                        localLog("connectToNetwork: Wifi has been toggled off, aborting");
                        return;
                    }
                    // we don't know which ClientModeManager will be allocated to us. Thus, double
                    // check if we're already connected before connecting.
                    if (isClientModeManagerConnectedOrConnectingToCandidate(
                            clientModeManager, targetNetwork)) {
                        localLog("connectToNetwork: already connected or connecting to candidate="
                                + targetNetwork + " on " + clientModeManager);
                        return;
                    }
                    if (clientModeManager.getRole() == ROLE_CLIENT_SECONDARY_TRANSIENT) {
                        mWifiMetrics.incrementMakeBeforeBreakTriggerCount();
                    }
                    triggerConnectToNetworkUsingCmm(clientModeManager, targetNetwork, targetBssid);
                },
                ActiveModeWarden.INTERNAL_REQUESTOR_WS,
                targetNetwork.SSID,
                mConnectivityHelper.isFirmwareRoamingSupported() ? null : targetBssid);
    }

    // Helper for selecting the band for connectivity scan
    private int getScanBand() {
        return getScanBand(true);
    }

    private int getScanBand(boolean isFullBandScan) {
        if (isFullBandScan) {
            if (SdkLevel.isAtLeastS()) {
                if (mContext.getResources().getBoolean(R.bool.config_wifiEnable6ghzPscScanning)) {
                    return WifiScanner.WIFI_BAND_24_5_WITH_DFS_6_GHZ;
                }
                return WifiScanner.WIFI_BAND_BOTH_WITH_DFS;
            }
            return WifiScanner.WIFI_BAND_ALL;
        } else {
            // Use channel list instead.
            return WifiScanner.WIFI_BAND_UNSPECIFIED;
        }
    }

    // Helper for setting the channels for connectivity scan when band is unspecified. Returns
    // false if we can't retrieve the info.
    // If connected, return channels used for the connected network
    // If disconnected, return channels used for any network.
    private boolean setScanChannels(ScanSettings settings) {
        Set<Integer> freqs;

        WifiConfiguration config = getPrimaryClientModeManager().getConnectedWifiConfiguration();
        if (config == null) {
            long ageInMillis = 1000 * 60 * mContext.getResources().getInteger(
                    R.integer.config_wifiInitialPartialScanChannelCacheAgeMins);
            int maxCount = mContext.getResources().getInteger(
                    R.integer.config_wifiInitialPartialScanChannelMaxCount);
            freqs = fetchChannelSetForPartialScan(maxCount, ageInMillis);
        } else {
            freqs = fetchChannelSetForNetworkForPartialScan(config.networkId);
        }

        if (freqs != null && freqs.size() != 0) {
            int index = 0;
            settings.channels = new WifiScanner.ChannelSpec[freqs.size()];
            for (Integer freq : freqs) {
                settings.channels[index++] = new WifiScanner.ChannelSpec(freq);
            }
            return true;
        } else {
            localLog("No history scan channels found, Perform full band scan");
            return false;
        }
    }

    /**
     * Add the channels into the channel set with a size limit.
     * If maxCount equals to 0, will add all available channels into the set.
     * @param channelSet Target set for adding channel to.
     * @param config Network for query channel from WifiScoreCard
     * @param maxCount Size limit of the set. If equals to 0, means no limit.
     * @param ageInMillis Only consider channel info whose timestamps are younger than this value.
     * @return True if all available channels for this network are added, otherwise false.
     */
    private boolean addChannelFromWifiScoreCard(@NonNull Set<Integer> channelSet,
            @NonNull WifiConfiguration config, int maxCount, long ageInMillis) {
        WifiScoreCard.PerNetwork network = mWifiScoreCard.lookupNetwork(config.SSID);
        for (Integer channel : network.getFrequencies(ageInMillis)) {
            if (maxCount > 0 && channelSet.size() >= maxCount) {
                localLog("addChannelFromWifiScoreCard: size limit reached for network:"
                        + config.SSID);
                return false;
            }
            channelSet.add(channel);
        }
        return true;
    }

    /**
     * Fetch channel set for target network.
     */
    @VisibleForTesting
    public Set<Integer> fetchChannelSetForNetworkForPartialScan(int networkId) {
        WifiConfiguration config = mConfigManager.getConfiguredNetwork(networkId);
        if (config == null) {
            return null;
        }
        final int maxNumActiveChannelsForPartialScans = mContext.getResources().getInteger(
                R.integer.config_wifi_framework_associated_partial_scan_max_num_active_channels);
        Set<Integer> channelSet = new HashSet<>();
        WifiInfo wifiInfo = getPrimaryWifiInfo();
        // First add the currently connected network channel.
        if (wifiInfo.getFrequency() > 0) {
            channelSet.add(wifiInfo.getFrequency());
        }
        // Then get channels for the network.
        addChannelFromWifiScoreCard(channelSet, config, maxNumActiveChannelsForPartialScans,
                CHANNEL_LIST_AGE_MS);
        return channelSet;
    }

    /**
     * Fetch channel set for all saved and suggestion non-passpoint network for partial scan.
     */
    @VisibleForTesting
    public Set<Integer> fetchChannelSetForPartialScan(int maxCount, long ageInMillis) {
        List<WifiConfiguration> networks = getAllScanOptimizationNetworks();
        if (networks.isEmpty()) {
            return null;
        }

        // Sort the networks with the most frequent ones at the front of the network list.
        Collections.sort(networks, mConfigManager.getScanListComparator());

        Set<Integer> channelSet = new HashSet<>();

        for (WifiConfiguration config : networks) {
            if (!addChannelFromWifiScoreCard(channelSet, config, maxCount, ageInMillis)) {
                return channelSet;
            }
        }

        return channelSet;
    }

    // Watchdog timer handler
    private void watchdogHandler() {
        // Schedule the next timer and start a single scan if we are in disconnected state.
        // Otherwise, the watchdog timer will be scheduled when entering disconnected
        // state.
        if (mWifiState == WIFI_STATE_DISCONNECTED) {
            localLog("start a single scan from watchdogHandler");

            scheduleWatchdogTimer();
            startSingleScan(true, WIFI_WORK_SOURCE);
        }
    }

    private void triggerScanOnNetworkChanges() {
        if (mScreenOn) {
            // Update scanning schedule if needed
            if (updateSingleScanningSchedule()) {
                localLog("Saved networks / suggestions updated impacting single scan schedule");
                startConnectivityScan(false);
            }
        } else {
            // Update the PNO scan network list when screen is off. Here we
            // rely on startConnectivityScan() to perform all the checks and clean up.
            localLog("Saved networks / suggestions updated impacting pno scan");
            startConnectivityScan(false);
        }
    }

    // Start a single scan and set up the interval for next single scan.
    private void startPeriodicSingleScan() {
        // Reaching here with scanning schedule is null means this is a false timer alarm
        if (getSingleScanningSchedule() == null) {
            return;
        }

        long currentTimeStamp = mClock.getElapsedSinceBootMillis();

        if (mLastPeriodicSingleScanTimeStamp != RESET_TIME_STAMP) {
            long msSinceLastScan = currentTimeStamp - mLastPeriodicSingleScanTimeStamp;
            if (msSinceLastScan < getScheduledSingleScanIntervalMs(0)) {
                localLog("Last periodic single scan started " + msSinceLastScan
                        + "ms ago, defer this new scan request.");
                schedulePeriodicScanTimer(
                        getScheduledSingleScanIntervalMs(0) - (int) msSinceLastScan);
                return;
            }
        }

        boolean isScanNeeded = true;
        boolean isFullBandScan = true;

        boolean isShortTimeSinceLastNetworkSelection =
                ((currentTimeStamp - mLastNetworkSelectionTimeStamp)
                <= 1000 * mContext.getResources().getInteger(
                R.integer.config_wifiConnectedHighRssiScanMinimumWindowSizeSec));

        WifiInfo wifiInfo = getPrimaryWifiInfo();
        boolean isGoodLinkAndAcceptableInternetAndShortTimeSinceLastNetworkSelection =
                mNetworkSelector.hasSufficientLinkQuality(wifiInfo)
                && mNetworkSelector.hasInternetOrExpectNoInternet(wifiInfo)
                && isShortTimeSinceLastNetworkSelection;
        // Check it is one of following conditions to skip scan (with firmware roaming)
        // or do partial scan only (without firmware roaming).
        // 1) Network is sufficient
        // 2) link is good, internet status is acceptable
        //    and it is a short time since last network selection
        // 3) There is active stream such that scan will be likely disruptive
        if (mWifiState == WIFI_STATE_CONNECTED
                && (mNetworkSelector.isNetworkSufficient(wifiInfo)
                || isGoodLinkAndAcceptableInternetAndShortTimeSinceLastNetworkSelection
                || mNetworkSelector.hasActiveStream(wifiInfo))) {
            // If only partial scan is proposed and firmware roaming control is supported,
            // we will not issue any scan because firmware roaming will take care of
            // intra-SSID roam.
            if (mConnectivityHelper.isFirmwareRoamingSupported()) {
                localLog("No partial scan because firmware roaming is supported.");
                isScanNeeded = false;
            } else {
                localLog("No full band scan because current network is sufficient");
                isFullBandScan = false;
            }
        }

        if (isScanNeeded) {
            mLastPeriodicSingleScanTimeStamp = currentTimeStamp;

            if (mWifiState == WIFI_STATE_DISCONNECTED
                    && mInitialScanState == INITIAL_SCAN_STATE_START) {
                startSingleScan(false, WIFI_WORK_SOURCE);

                // Note, initial partial scan may fail due to lack of channel history
                // Hence, we verify state before changing to AWIATING_RESPONSE
                if (mInitialScanState == INITIAL_SCAN_STATE_START) {
                    setInitialScanState(INITIAL_SCAN_STATE_AWAITING_RESPONSE);
                    mWifiMetrics.incrementInitialPartialScanCount();
                }
            } else {
                startSingleScan(isFullBandScan, WIFI_WORK_SOURCE);
            }
            schedulePeriodicScanTimer(
                    getScheduledSingleScanIntervalMs(mCurrentSingleScanScheduleIndex));

            // Set up the next scan interval in an exponential backoff fashion.
            mCurrentSingleScanScheduleIndex++;
        } else {
            // Since we already skipped this scan, keep the same scan interval for next scan.
            schedulePeriodicScanTimer(
                    getScheduledSingleScanIntervalMs(mCurrentSingleScanScheduleIndex));
        }
    }

    // Retrieve a value from single scanning schedule in ms
    private int getScheduledSingleScanIntervalMs(int index) {
        synchronized (mLock) {
            if (mCurrentSingleScanScheduleSec == null) {
                Log.e(TAG, "Invalid attempt to get schedule interval, Schedule array is null ");

                // Use a default value
                return DEFAULT_SCANNING_SCHEDULE_SEC[0] * 1000;
            }

            if (index >= mCurrentSingleScanScheduleSec.length) {
                index = mCurrentSingleScanScheduleSec.length - 1;
            }
            return getScanIntervalWithPowerSaveMultiplier(
                    mCurrentSingleScanScheduleSec[index] * 1000);
        }
    }

    private int getScanIntervalWithPowerSaveMultiplier(int interval) {
        if (!mDeviceConfigFacade.isWifiBatterySaverEnabled()) {
            return interval;
        }
        return mPowerManager.isPowerSaveMode()
                ? POWER_SAVE_SCAN_INTERVAL_MULTIPLIER * interval : interval;
    }

    // Set the single scanning schedule
    private void setSingleScanningSchedule(int[] scheduleSec) {
        synchronized (mLock) {
            mCurrentSingleScanScheduleSec = scheduleSec;
        }
    }

    // Get the single scanning schedule
    private int[] getSingleScanningSchedule() {
        synchronized (mLock) {
            return mCurrentSingleScanScheduleSec;
        }
    }

    // Update the single scanning schedule if needed, and return true if update occurs
    private boolean updateSingleScanningSchedule() {
        if (!mWifiEnabled || !mAutoJoinEnabled) {
            return false;
        }
        if (mWifiState != WIFI_STATE_CONNECTED) {
            // No need to update the scanning schedule
            return false;
        }

        boolean shouldUseSingleSavedNetworkSchedule = useSingleSavedNetworkSchedule();

        if (mCurrentSingleScanScheduleSec == mConnectedSingleScanScheduleSec
                && shouldUseSingleSavedNetworkSchedule) {
            mCurrentSingleScanScheduleSec = mConnectedSingleSavedNetworkSingleScanScheduleSec;
            return true;
        }
        if (mCurrentSingleScanScheduleSec == mConnectedSingleSavedNetworkSingleScanScheduleSec
                && !shouldUseSingleSavedNetworkSchedule) {
            mCurrentSingleScanScheduleSec = mConnectedSingleScanScheduleSec;
            return true;
        }
        return false;
    }

    // Set initial scan state
    private void setInitialScanState(int state) {
        Log.i(TAG, "SetInitialScanState to : " + state);
        mInitialScanState = state;
    }

    @VisibleForTesting
    public int getInitialScanState() {
        return mInitialScanState;
    }

    // Reset the last periodic single scan time stamp so that the next periodic single
    // scan can start immediately.
    private void resetLastPeriodicSingleScanTimeStamp() {
        mLastPeriodicSingleScanTimeStamp = RESET_TIME_STAMP;
    }

    // Periodic scan timer handler
    private void periodicScanTimerHandler() {
        localLog("periodicScanTimerHandler");

        // Schedule the next timer and start a single scan if screen is on.
        if (mScreenOn) {
            startPeriodicSingleScan();
        }
    }

    // Start a single scan
    private void startForcedSingleScan(boolean isFullBandScan, WorkSource workSource) {
        mPnoScanListener.resetLowRssiNetworkRetryDelay();

        ScanSettings settings = new ScanSettings();
        if (!isFullBandScan) {
            if (!setScanChannels(settings)) {
                isFullBandScan = true;
                // Skip the initial scan since no channel history available
                setInitialScanState(INITIAL_SCAN_STATE_COMPLETE);
            } else {
                mInitialPartialScanChannelCount = settings.channels.length;
            }
        }
        settings.type = WifiScanner.SCAN_TYPE_HIGH_ACCURACY; // always do high accuracy scans.
        settings.band = getScanBand(isFullBandScan);
        // Only enable RNR for full scans since we already have a known channel list for
        // partial scan. We do not want to enable RNR for partial scan since it could end up
        // wasting time scanning for 6Ghz APs that the device doesn't have credential to.
        if (SdkLevel.isAtLeastS()) {
            settings.setRnrSetting(isFullBandScan ? WifiScanner.WIFI_RNR_ENABLED
                    : WifiScanner.WIFI_RNR_NOT_NEEDED);
            settings.set6GhzPscOnlyEnabled(isFullBandScan
                    ? mContext.getResources().getBoolean(R.bool.config_wifiEnable6ghzPscScanning)
                    : false);
        }
        settings.reportEvents = WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT
                            | WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN;
        settings.numBssidsPerScan = 0;
        settings.hiddenNetworks.clear();
        // retrieve the list of hidden network SSIDs from saved network to scan for
        settings.hiddenNetworks.addAll(mConfigManager.retrieveHiddenNetworkList());
        // retrieve the list of hidden network SSIDs from Network suggestion to scan for
        settings.hiddenNetworks.addAll(mWifiNetworkSuggestionsManager.retrieveHiddenNetworkList());

        SingleScanListener singleScanListener =
                new SingleScanListener(isFullBandScan);
        mScanner.startScan(
                settings, new HandlerExecutor(mEventHandler), singleScanListener, workSource);
        mWifiMetrics.incrementConnectivityOneshotScanCount();
    }

    private void startSingleScan(boolean isFullBandScan, WorkSource workSource) {
        if (!mWifiEnabled || !mAutoJoinEnabled) {
            return;
        }
        startForcedSingleScan(isFullBandScan, workSource);
    }

    // Start a periodic scan when screen is on
    private void startPeriodicScan(boolean scanImmediately) {
        mPnoScanListener.resetLowRssiNetworkRetryDelay();

        // No connectivity scan if auto roaming is disabled.
        if (mWifiState == WIFI_STATE_CONNECTED && !mContext.getResources().getBoolean(
                R.bool.config_wifi_framework_enable_associated_network_selection)) {
            return;
        }

        // Due to b/28020168, timer based single scan will be scheduled
        // to provide periodic scan in an exponential backoff fashion.
        if (scanImmediately) {
            resetLastPeriodicSingleScanTimeStamp();
        }
        mCurrentSingleScanScheduleIndex = 0;
        startPeriodicSingleScan();
    }

    private int deviceMobilityStateToPnoScanIntervalMs(@DeviceMobilityState int state) {
        switch (state) {
            case WifiManager.DEVICE_MOBILITY_STATE_UNKNOWN:
            case WifiManager.DEVICE_MOBILITY_STATE_LOW_MVMT:
            case WifiManager.DEVICE_MOBILITY_STATE_HIGH_MVMT:
                return getScanIntervalWithPowerSaveMultiplier(mContext.getResources()
                        .getInteger(R.integer.config_wifiMovingPnoScanIntervalMillis));
            case WifiManager.DEVICE_MOBILITY_STATE_STATIONARY:
                return getScanIntervalWithPowerSaveMultiplier(mContext.getResources()
                        .getInteger(R.integer.config_wifiStationaryPnoScanIntervalMillis));
            default:
                return -1;
        }
    }

    /**
     * Pass device mobility state to WifiChannelUtilization and
     * alter the PNO scan interval based on the current device mobility state.
     * If the device is stationary, it will likely not find many new Wifi networks. Thus, increase
     * the interval between scans. Decrease the interval between scans if the device begins to move
     * again.
     * @param newState the new device mobility state
     */
    public void setDeviceMobilityState(@DeviceMobilityState int newState) {
        int oldDeviceMobilityState = mDeviceMobilityState;
        localLog("Device mobility state changed. state=" + newState);
        int newPnoScanIntervalMs = deviceMobilityStateToPnoScanIntervalMs(newState);
        if (newPnoScanIntervalMs < 0) {
            Log.e(TAG, "Invalid device mobility state: " + newState);
            return;
        }
        mDeviceMobilityState = newState;
        mWifiChannelUtilization.setDeviceMobilityState(newState);

        int oldPnoScanIntervalMs = deviceMobilityStateToPnoScanIntervalMs(oldDeviceMobilityState);
        if (newPnoScanIntervalMs == oldPnoScanIntervalMs) {
            if (mPnoScanStarted) {
                mWifiMetrics.logPnoScanStop();
                mWifiMetrics.enterDeviceMobilityState(newState);
                mWifiMetrics.logPnoScanStart();
            } else {
                mWifiMetrics.enterDeviceMobilityState(newState);
            }
        } else {
            Log.d(TAG, "PNO Scan Interval changed to " + newPnoScanIntervalMs + " ms.");

            if (mPnoScanStarted) {
                Log.d(TAG, "Restarting PNO Scan with new scan interval");
                stopPnoScan();
                mWifiMetrics.enterDeviceMobilityState(newState);
                startDisconnectedPnoScan();
            } else {
                mWifiMetrics.enterDeviceMobilityState(newState);
            }
        }
    }

    // Start a DisconnectedPNO scan when screen is off and Wifi is disconnected
    private void startDisconnectedPnoScan() {
        // Initialize PNO settings
        PnoSettings pnoSettings = new PnoSettings();
        List<PnoSettings.PnoNetwork> pnoNetworkList = retrievePnoNetworkList();
        int listSize = pnoNetworkList.size();

        if (listSize == 0) {
            // No saved network
            localLog("No saved network for starting disconnected PNO.");
            return;
        }

        pnoSettings.networkList = new PnoSettings.PnoNetwork[listSize];
        pnoSettings.networkList = pnoNetworkList.toArray(pnoSettings.networkList);
        pnoSettings.min6GHzRssi = mScoringParams.getEntryRssi(ScanResult.BAND_6_GHZ_START_FREQ_MHZ);
        pnoSettings.min5GHzRssi = mScoringParams.getEntryRssi(ScanResult.BAND_5_GHZ_START_FREQ_MHZ);
        pnoSettings.min24GHzRssi = mScoringParams.getEntryRssi(
                ScanResult.BAND_24_GHZ_START_FREQ_MHZ);

        // Initialize scan settings
        ScanSettings scanSettings = new ScanSettings();
        scanSettings.band = getScanBand();
        scanSettings.reportEvents = WifiScanner.REPORT_EVENT_NO_BATCH;
        scanSettings.numBssidsPerScan = 0;
        scanSettings.periodInMs = deviceMobilityStateToPnoScanIntervalMs(mDeviceMobilityState);

        mScanner.startDisconnectedPnoScan(
                scanSettings, pnoSettings, new HandlerExecutor(mEventHandler), mPnoScanListener);
        mPnoScanStarted = true;
        mWifiMetrics.logPnoScanStart();
    }

    private @NonNull List<WifiConfiguration> getAllScanOptimizationNetworks() {
        List<WifiConfiguration> networks = mConfigManager.getSavedNetworks(-1);
        networks.addAll(mWifiNetworkSuggestionsManager.getAllScanOptimizationSuggestionNetworks());
        // remove all auto-join disabled or network selection disabled network.
        networks.removeIf(config -> !config.allowAutojoin
                || !config.getNetworkSelectionStatus().isNetworkEnabled());
        return networks;
    }

    /**
     * Retrieve the PnoNetworks from Saved and suggestion non-passpoint network.
     */
    @VisibleForTesting
    public List<PnoSettings.PnoNetwork> retrievePnoNetworkList() {
        List<WifiConfiguration> networks = getAllScanOptimizationNetworks();

        if (networks.isEmpty()) {
            return Collections.EMPTY_LIST;
        }
        Collections.sort(networks, mConfigManager.getScanListComparator());
        boolean pnoFrequencyCullingEnabled = mContext.getResources()
                .getBoolean(R.bool.config_wifiPnoFrequencyCullingEnabled);

        List<PnoSettings.PnoNetwork> pnoList = new ArrayList<>();
        Set<WifiScanner.PnoSettings.PnoNetwork> pnoSet = new HashSet<>();
        for (WifiConfiguration config : networks) {
            WifiScanner.PnoSettings.PnoNetwork pnoNetwork =
                    WifiConfigurationUtil.createPnoNetwork(config);
            if (pnoSet.contains(pnoNetwork)) {
                continue;
            }
            pnoList.add(pnoNetwork);
            pnoSet.add(pnoNetwork);
            if (!pnoFrequencyCullingEnabled) {
                continue;
            }
            Set<Integer> channelList = new HashSet<>();
            addChannelFromWifiScoreCard(channelList, config, 0,
                    MAX_PNO_SCAN_FREQUENCY_AGE_MS);
            pnoNetwork.frequencies = channelList.stream().mapToInt(Integer::intValue).toArray();
        }
        return pnoList;
    }

    // Stop PNO scan.
    private void stopPnoScan() {
        if (!mPnoScanStarted) return;

        mScanner.stopPnoScan(mPnoScanListener);
        mPnoScanStarted = false;
        mWifiMetrics.logPnoScanStop();
    }

    // Set up watchdog timer
    private void scheduleWatchdogTimer() {
        localLog("scheduleWatchdogTimer");

        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                            mClock.getElapsedSinceBootMillis() + mContext.getResources().getInteger(
                                    R.integer.config_wifiPnoWatchdogIntervalMs),
                            WATCHDOG_TIMER_TAG,
                            mWatchdogListener, mEventHandler);
        mWatchdogScanTimerSet = true;
    }

    // Cancel the watchdog scan timer.
    private void cancelWatchdogScan() {
        if (mWatchdogScanTimerSet) {
            mAlarmManager.cancel(mWatchdogListener);
            mWatchdogScanTimerSet = false;
        }
    }

    // Schedules a delayed partial scan, which will scan the frequencies in mCachedWifiCandidates.
    private void scheduleDelayedPartialScan(long delayMillis) {
        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                mClock.getElapsedSinceBootMillis() + delayMillis, DELAYED_PARTIAL_SCAN_TIMER_TAG,
                mDelayedPartialScanTimerListener, mEventHandler);
        mDelayedPartialScanTimerSet = true;
    }

    // Cancel the delayed partial scan timer.
    private void cancelDelayedPartialScan() {
        if (mDelayedPartialScanTimerSet) {
            mAlarmManager.cancel(mDelayedPartialScanTimerListener);
            mDelayedPartialScanTimerSet = false;
        }
    }

    // Set up periodic scan timer
    private void schedulePeriodicScanTimer(int intervalMs) {
        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                            mClock.getElapsedSinceBootMillis() + intervalMs,
                            PERIODIC_SCAN_TIMER_TAG,
                            mPeriodicScanTimerListener, mEventHandler);
        mPeriodicScanTimerSet = true;
    }

    // Cancel periodic scan timer
    private void cancelPeriodicScanTimer() {
        if (mPeriodicScanTimerSet) {
            mAlarmManager.cancel(mPeriodicScanTimerListener);
            mPeriodicScanTimerSet = false;
        }
    }

    // Set up timer to start a delayed single scan after RESTART_SCAN_DELAY_MS
    private void scheduleDelayedSingleScan(boolean isFullBandScan) {
        localLog("scheduleDelayedSingleScan");

        RestartSingleScanListener restartSingleScanListener =
                new RestartSingleScanListener(isFullBandScan);
        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                            mClock.getElapsedSinceBootMillis() + RESTART_SCAN_DELAY_MS,
                            RESTART_SINGLE_SCAN_TIMER_TAG,
                            restartSingleScanListener, mEventHandler);
    }

    // Set up timer to start a delayed scan after msFromNow milli-seconds
    private void scheduleDelayedConnectivityScan(int msFromNow) {
        localLog("scheduleDelayedConnectivityScan");

        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                            mClock.getElapsedSinceBootMillis() + msFromNow,
                            RESTART_CONNECTIVITY_SCAN_TIMER_TAG,
                            mRestartScanListener, mEventHandler);

    }

    // Start a connectivity scan. The scan method is chosen according to
    // the current screen state and WiFi state.
    private void startConnectivityScan(boolean scanImmediately) {
        localLog("startConnectivityScan: screenOn=" + mScreenOn
                + " wifiState=" + stateToString(mWifiState)
                + " scanImmediately=" + scanImmediately
                + " wifiEnabled=" + mWifiEnabled
                + " mAutoJoinEnabled=" + mAutoJoinEnabled);

        if (!mWifiEnabled || !mAutoJoinEnabled) {
            return;
        }

        // Always stop outstanding connectivity scan if there is any
        stopConnectivityScan();

        // Don't start a connectivity scan while Wifi is in the transition
        // between connected and disconnected states.
        if ((mWifiState != WIFI_STATE_CONNECTED && mWifiState != WIFI_STATE_DISCONNECTED)
                || (getSingleScanningSchedule() == null)) {
            return;
        }

        if (mScreenOn) {
            startPeriodicScan(scanImmediately);
        } else {
            if (mWifiState == WIFI_STATE_DISCONNECTED && !mPnoScanStarted) {
                startDisconnectedPnoScan();
            }
        }
    }

    // Stop connectivity scan if there is any.
    private void stopConnectivityScan() {
        // Due to b/28020168, timer based single scan will be scheduled
        // to provide periodic scan in an exponential backoff fashion.
        cancelPeriodicScanTimer();
        cancelDelayedPartialScan();
        stopPnoScan();
    }

    /**
     * Handler for screen state (on/off) changes
     */
    private void handleScreenStateChanged(boolean screenOn) {
        localLog("handleScreenStateChanged: screenOn=" + screenOn);

        mScreenOn = screenOn;

        if (mWifiState == WIFI_STATE_DISCONNECTED
                && mContext.getResources().getBoolean(R.bool.config_wifiEnablePartialInitialScan)) {
            setInitialScanState(INITIAL_SCAN_STATE_START);
        }

        mOpenNetworkNotifier.handleScreenStateChanged(screenOn);

        startConnectivityScan(SCAN_ON_SCHEDULE);
    }

    /**
     * Helper function that converts the WIFI_STATE_XXX constants to string
     */
    private static String stateToString(int state) {
        switch (state) {
            case WIFI_STATE_CONNECTED:
                return "connected";
            case WIFI_STATE_DISCONNECTED:
                return "disconnected";
            case WIFI_STATE_TRANSITIONING:
                return "transitioning";
            default:
                return "unknown";
        }
    }

    /**
     * Check if Single saved network schedule should be used
     * This is true if the one of the following is satisfied:
     * 1. Device has a total of 1 network whether saved, passpoint, or suggestion.
     * 2. The device is connected to that network.
     */
    private boolean useSingleSavedNetworkSchedule() {
        WifiConfiguration currentNetwork =
                getPrimaryClientModeManager().getConnectedWifiConfiguration();
        if (currentNetwork == null) {
            localLog("Current network is missing, may caused by remove network and disconnecting");
            return false;
        }
        List<WifiConfiguration> savedNetworks =
                mConfigManager.getSavedNetworks(Process.WIFI_UID);
        // If we have multiple saved networks, then no need to proceed
        if (savedNetworks.size() > 1) {
            return false;
        }

        List<PasspointConfiguration> passpointNetworks =
                mPasspointManager.getProviderConfigs(Process.WIFI_UID, true);
        // If we have multiple networks (saved + passpoint), then no need to proceed
        if (passpointNetworks.size() + savedNetworks.size() > 1) {
            return false;
        }

        Set<WifiNetworkSuggestion> suggestionsNetworks =
                mWifiNetworkSuggestionsManager.getAllApprovedNetworkSuggestions();
        // If total size not equal to 1, then no need to proceed
        if (passpointNetworks.size() + savedNetworks.size() + suggestionsNetworks.size() != 1) {
            return false;
        }

        // Next verify that this network is the one device is connected to
        int currentNetworkId = currentNetwork.networkId;

        // If we have a single saved network, and we are connected to it, return true.
        if (savedNetworks.size() == 1) {
            return (savedNetworks.get(0).networkId == currentNetworkId);
        }

        // If we have a single passpoint network, and we are connected to it, return true.
        if (passpointNetworks.size() == 1) {
            String passpointKey = passpointNetworks.get(0).getUniqueId();
            WifiConfiguration config = mConfigManager.getConfiguredNetwork(passpointKey);
            return (config != null && config.networkId == currentNetworkId);
        }

        // If we have a single suggestion network, and we are connected to it, return true.
        WifiNetworkSuggestion network = suggestionsNetworks.iterator().next();
        String suggestionKey = network.getWifiConfiguration().getProfileKey();
        WifiConfiguration config = mConfigManager.getConfiguredNetwork(suggestionKey);
        return (config != null && config.networkId == currentNetworkId);
    }

    private int[] initSingleSavedNetworkSchedule() {
        int[] schedule = mContext.getResources().getIntArray(
                    R.array.config_wifiSingleSavedNetworkConnectedScanIntervalScheduleSec);
        if (schedule == null || schedule.length == 0) {
            return null;
        }

        for (int val : schedule) {
            if (val <= 0) {
                return null;
            }
        }
        return schedule;
    }

    /**
     * Handler for WiFi state (connected/disconnected) changes
     */
    public void handleConnectionStateChanged(
            ConcreteClientModeManager clientModeManager, int state) {
        List<ClientModeManager> internetConnectivityCmms =
                mActiveModeWarden.getInternetConnectivityClientModeManagers();
        if (!(internetConnectivityCmms.contains(clientModeManager))) {
            Log.w(TAG, "Ignoring call from non primary Mode Manager " + clientModeManager,
                    new Throwable());
            return;
        }
        localLog("handleConnectionStateChanged: state=" + stateToString(state));

        if (mConnectedSingleScanScheduleSec == null) {
            mConnectedSingleScanScheduleSec = initializeScanningSchedule(WIFI_STATE_CONNECTED);
        }
        if (mDisconnectedSingleScanScheduleSec == null) {
            mDisconnectedSingleScanScheduleSec =
                    initializeScanningSchedule(WIFI_STATE_DISCONNECTED);
        }
        if (mConnectedSingleSavedNetworkSingleScanScheduleSec == null) {
            mConnectedSingleSavedNetworkSingleScanScheduleSec =
                    initSingleSavedNetworkSchedule();
            if (mConnectedSingleSavedNetworkSingleScanScheduleSec == null) {
                mConnectedSingleSavedNetworkSingleScanScheduleSec = mConnectedSingleScanScheduleSec;
            }
        }

        mWifiState = state;

        // Reset BSSID of last connection attempt and kick off
        // the watchdog timer if entering disconnected state.
        if (mWifiState == WIFI_STATE_DISCONNECTED) {
            scheduleWatchdogTimer();
            // Switch to the disconnected scanning schedule
            setSingleScanningSchedule(mDisconnectedSingleScanScheduleSec);
            startConnectivityScan(SCAN_IMMEDIATELY);
        } else if (mWifiState == WIFI_STATE_CONNECTED) {
            if (useSingleSavedNetworkSchedule()) {
                // Switch to Single-Saved-Network connected schedule
                setSingleScanningSchedule(mConnectedSingleSavedNetworkSingleScanScheduleSec);
            } else {
                // Switch to connected single scanning schedule
                setSingleScanningSchedule(mConnectedSingleScanScheduleSec);
            }
            startConnectivityScan(SCAN_ON_SCHEDULE);
        } else {
            // Intermediate state, no applicable single scanning schedule
            setSingleScanningSchedule(null);
            startConnectivityScan(SCAN_ON_SCHEDULE);
        }
    }

    /**
     * Handler when a WiFi connection attempt ended.
     *
     * @param failureCode {@link WifiMetrics.ConnectionEvent} failure code.
     * @param bssid the failed network.
     * @param ssid identifies the failed network.
     */
    public void handleConnectionAttemptEnded(@NonNull ActiveModeManager activeModeManager,
            int failureCode, @NonNull String bssid, @NonNull String ssid) {
        List<ClientModeManager> internetConnectivityCmms =
                mActiveModeWarden.getInternetConnectivityClientModeManagers();
        if (!(internetConnectivityCmms.contains(activeModeManager))) {
            Log.w(TAG, "Ignoring call from non primary Mode Manager " + activeModeManager,
                    new Throwable());
            return;
        }
        WifiInfo wifiInfo = getPrimaryWifiInfo();
        if (failureCode == WifiMetrics.ConnectionEvent.FAILURE_NONE) {
            String ssidUnquoted = (wifiInfo.getWifiSsid() == null)
                    ? null
                    : wifiInfo.getWifiSsid().toString();
            mOpenNetworkNotifier.handleWifiConnected(ssidUnquoted);
        } else {
            mOpenNetworkNotifier.handleConnectionFailure();
            retryConnectionOnLatestCandidates(bssid, ssid);
        }
    }

    private void retryConnectionOnLatestCandidates(String bssid, String ssid) {
        try {
            if (mLatestCandidates == null || mLatestCandidates.size() == 0
                    || mClock.getElapsedSinceBootMillis() - mLatestCandidatesTimestampMs
                    > TEMP_BSSID_BLOCK_DURATION) {
                mLatestCandidates = null;
                return;
            }
            MacAddress macAddress = MacAddress.fromString(bssid);
            int prevNumCandidates = mLatestCandidates.size();
            mLatestCandidates = mLatestCandidates.stream()
                    .filter(candidate -> {
                        // filter out the candidate with the BSSID that just failed
                        if (macAddress.equals(candidate.getKey().bssid)) {
                            return false;
                        }
                        // filter out candidates that are disabled.
                        WifiConfiguration config =
                                mConfigManager.getConfiguredNetwork(candidate.getNetworkConfigId());
                        return config != null
                                && config.getNetworkSelectionStatus().isNetworkEnabled();
                    })
                    .collect(Collectors.toList());
            if (prevNumCandidates == mLatestCandidates.size()) {
                return;
            }
            WifiConfiguration candidate = mNetworkSelector.selectNetwork(mLatestCandidates);
            if (candidate != null) {
                localLog("Automatic retry on the next best WNS candidate-" + candidate.SSID);
                // Make sure that the failed BSSID is blocked for at least TEMP_BSSID_BLOCK_DURATION
                // to prevent the supplicant from trying it again.
                mWifiBlocklistMonitor.blockBssidForDurationMs(bssid, ssid,
                        TEMP_BSSID_BLOCK_DURATION,
                        WifiBlocklistMonitor.REASON_FRAMEWORK_DISCONNECT_FAST_RECONNECT, 0);
                connectToNetworkForPrimaryCmmUsingMbbIfAvailable(candidate);
            }
        } catch (IllegalArgumentException e) {
            localLog("retryConnectionOnLatestCandidates: failed to create MacAddress from bssid="
                    + bssid);
            mLatestCandidates = null;
        }
    }

    // Enable auto-join if WifiConnectivityManager is enabled & we have any pending generic network
    // request (trusted or untrusted) and no specific network request in progress.
    private void checkAllStatesAndEnableAutoJoin() {
        // if auto-join was disabled externally, don't re-enable for any triggers.
        // External triggers to disable always trumps any internal state.
        setAutoJoinEnabled(mAutoJoinEnabledExternal
                && (mUntrustedConnectionAllowed || mOemPaidConnectionAllowed
                || mOemPrivateConnectionAllowed || mTrustedConnectionAllowed)
                && !mSpecificNetworkRequestInProgress);
        startConnectivityScan(SCAN_IMMEDIATELY);
    }

    /**
     * Triggered when {@link WifiNetworkFactory} has a pending general network request.
     */
    public void setTrustedConnectionAllowed(boolean allowed) {
        localLog("setTrustedConnectionAllowed: allowed=" + allowed);

        if (mTrustedConnectionAllowed != allowed) {
            mTrustedConnectionAllowed = allowed;
            checkAllStatesAndEnableAutoJoin();
        }
    }

    /**
     * Triggered when {@link UntrustedWifiNetworkFactory} has a pending ephemeral network request.
     */
    public void setUntrustedConnectionAllowed(boolean allowed) {
        localLog("setUntrustedConnectionAllowed: allowed=" + allowed);

        if (mUntrustedConnectionAllowed != allowed) {
            mUntrustedConnectionAllowed = allowed;
            checkAllStatesAndEnableAutoJoin();
        }
    }

    /**
     * Triggered when {@link OemPaidWifiNetworkFactory} has a pending network request.
     */
    public void setOemPaidConnectionAllowed(boolean allowed, WorkSource requestorWs) {
        localLog("setOemPaidConnectionAllowed: allowed=" + allowed + ", requestorWs="
                + requestorWs);

        if (mOemPaidConnectionAllowed != allowed) {
            mOemPaidConnectionAllowed = allowed;
            mOemPaidConnectionRequestorWs = requestorWs;
            checkAllStatesAndEnableAutoJoin();
        }
    }

    /**
     * Triggered when {@link OemPrivateWifiNetworkFactory} has a pending network request.
     */
    public void setOemPrivateConnectionAllowed(boolean allowed, WorkSource requestorWs) {
        localLog("setOemPrivateConnectionAllowed: allowed=" + allowed + ", requestorWs="
                + requestorWs);

        if (mOemPrivateConnectionAllowed != allowed) {
            mOemPrivateConnectionAllowed = allowed;
            mOemPrivateConnectionRequestorWs = requestorWs;
            checkAllStatesAndEnableAutoJoin();
        }
    }

    /**
     * Triggered when {@link WifiNetworkFactory} is processing a specific network request.
     */
    public void setSpecificNetworkRequestInProgress(boolean inProgress) {
        localLog("setSpecificNetworkRequestInProgress : inProgress=" + inProgress);

        if (mSpecificNetworkRequestInProgress != inProgress) {
            mSpecificNetworkRequestInProgress = inProgress;
            checkAllStatesAndEnableAutoJoin();
        }
    }

    /**
     * Handler to prepare for connection to a user or app specified network
     */
    public void prepareForForcedConnection(int netId) {
        WifiConfiguration config = mConfigManager.getConfiguredNetwork(netId);
        if (config == null) {
            return;
        }
        localLog("prepareForForcedConnection: SSID=" + config.SSID);

        clearConnectionAttemptTimeStamps();
        mWifiBlocklistMonitor.clearBssidBlocklistForSsid(config.SSID);
    }

    /**
     * Handler for on-demand connectivity scan
     */
    public void forceConnectivityScan(WorkSource workSource) {
        if (!mWifiEnabled || !mRunning) return;
        localLog("forceConnectivityScan in request of " + workSource);

        clearConnectionAttemptTimeStamps();
        mWaitForFullBandScanResults = true;
        startForcedSingleScan(true, workSource);
    }

    /**
     * Helper method to populate WifiScanner handle. This is done lazily because
     * WifiScanningService is started after WifiService.
     */
    private void retrieveWifiScanner() {
        if (mScanner != null) return;
        mScanner = Objects.requireNonNull(mContext.getSystemService(WifiScanner.class),
                "Got a null instance of WifiScanner!");
        // Register for all single scan results
        mScanner.registerScanListener(new HandlerExecutor(mEventHandler), mAllSingleScanListener);
    }

    /**
     * Start WifiConnectivityManager
     */
    private void start() {
        if (mRunning) return;
        retrieveWifiScanner();
        mConnectivityHelper.getFirmwareRoamingInfo();
        mWifiChannelUtilization.init(getPrimaryClientModeManager().getWifiLinkLayerStats());
        clearConnectionAttemptTimeStamps(); // clear connection attempts.

        if (mContext.getResources().getBoolean(R.bool.config_wifiEnablePartialInitialScan)) {
            setInitialScanState(INITIAL_SCAN_STATE_START);
        }

        mRunning = true;
        mLatestCandidates = null;
        mLatestCandidatesTimestampMs = 0;
    }

    /**
     * Stop and reset WifiConnectivityManager
     */
    private void stop() {
        if (!mRunning) return;
        mRunning = false;
        stopConnectivityScan();
        cancelWatchdogScan();
        resetLastPeriodicSingleScanTimeStamp();
        mOpenNetworkNotifier.clearPendingNotification(true /* resetRepeatDelay */);
        mWaitForFullBandScanResults = false;
        mLatestCandidates = null;
        mLatestCandidatesTimestampMs = 0;
        mScanRestartCount = 0;
    }

    /**
     * Update WifiConnectivityManager running state
     *
     * Start WifiConnectivityManager only if both Wifi and WifiConnectivityManager
     * are enabled, otherwise stop it.
     */
    private void updateRunningState() {
        if (mWifiEnabled && mAutoJoinEnabled) {
            localLog("Starting up WifiConnectivityManager");
            start();
        } else {
            localLog("Stopping WifiConnectivityManager");
            stop();
        }
    }

    /**
     * Inform WiFi is enabled for connection or not
     */
    private void setWifiEnabled(boolean enable) {
        if (mWifiEnabled == enable) return;

        localLog("Set WiFi " + (enable ? "enabled" : "disabled"));

        if (!enable) {
            mNetworkSelector.resetOnDisable();
            mConfigManager.enableTemporaryDisabledNetworks();
            mConfigManager.stopRestrictingAutoJoinToSubscriptionId();
            mConfigManager.clearUserTemporarilyDisabledList();
            mConfigManager.removeAllEphemeralOrPasspointConfiguredNetworks();
            // Flush ANQP cache if configured to do so
            if (mWifiGlobals.flushAnqpCacheOnWifiToggleOffEvent()) {
                mPasspointManager.clearAnqpRequestsAndFlushCache();
            }
        }
        mWifiEnabled = enable;
        updateRunningState();
    }

    /**
     * Turn on/off the WifiConnectivityManager at runtime
     */
    private void setAutoJoinEnabled(boolean enable) {
        mAutoJoinEnabled = enable;
        updateRunningState();
    }

    /**
     * Turn on/off the auto join at runtime
     */
    public void setAutoJoinEnabledExternal(boolean enable) {
        localLog("Set auto join " + (enable ? "enabled" : "disabled"));

        if (mAutoJoinEnabledExternal != enable) {
            mAutoJoinEnabledExternal = enable;
            checkAllStatesAndEnableAutoJoin();
        }
    }

    @VisibleForTesting
    int getLowRssiNetworkRetryDelay() {
        return mPnoScanListener.getLowRssiNetworkRetryDelay();
    }

    @VisibleForTesting
    long getLastPeriodicSingleScanTimeStamp() {
        return mLastPeriodicSingleScanTimeStamp;
    }

    /**
     * Dump the local logs.
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of WifiConnectivityManager");
        pw.println("WifiConnectivityManager - Log Begin ----");
        mLocalLog.dump(fd, pw, args);
        pw.println("WifiConnectivityManager - Log End ----");
        mOpenNetworkNotifier.dump(fd, pw, args);
        mWifiBlocklistMonitor.dump(fd, pw, args);
    }
}
