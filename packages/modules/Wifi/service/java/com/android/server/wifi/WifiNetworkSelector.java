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

import static android.net.wifi.WifiManager.WIFI_FEATURE_OWE;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.net.MacAddress;
import android.net.wifi.ScanResult;
import android.net.wifi.SecurityParams;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.LocalLog;
import android.util.Log;
import android.util.Pair;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.Preconditions;
import com.android.server.wifi.hotspot2.NetworkDetail;
import com.android.server.wifi.proto.nano.WifiMetricsProto;
import com.android.server.wifi.util.InformationElementUtil.BssLoad;
import com.android.server.wifi.util.ScanResultUtil;
import com.android.wifi.resources.R;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 * WifiNetworkSelector looks at all the connectivity scan results and
 * runs all the nominators to find or create matching configurations.
 * Then it makes a final selection from among the resulting candidates.
 */
public class WifiNetworkSelector {
    private static final String TAG = "WifiNetworkSelector";

    private static final long INVALID_TIME_STAMP = Long.MIN_VALUE;

    /**
     * Minimum time gap between last successful network selection and a
     * new selection attempt.
     */
    @VisibleForTesting
    public static final int MINIMUM_NETWORK_SELECTION_INTERVAL_MS = 10 * 1000;

    /**
     * Connected score value used to decide whether a still-connected wifi should be treated
     * as unconnected when filtering scan results.
     */
    @VisibleForTesting
    public static final int WIFI_POOR_SCORE = ConnectedScore.WIFI_TRANSITION_SCORE - 10;

    /**
     * The identifier string of the CandidateScorer to use (in the absence of overrides).
     */
    public static final String PRESET_CANDIDATE_SCORER_NAME = "ThroughputScorer";

    /**
     * Experiment ID for the legacy scorer.
     */
    public static final int LEGACY_CANDIDATE_SCORER_EXP_ID = 0;

    private final Context mContext;
    private final WifiConfigManager mWifiConfigManager;
    private final Clock mClock;
    private final LocalLog mLocalLog;
    private boolean mVerboseLoggingEnabled = false;
    private final WifiMetrics mWifiMetrics;
    private long mLastNetworkSelectionTimeStamp = INVALID_TIME_STAMP;
    // Buffer of filtered scan results (Scan results considered by network selection) & associated
    // WifiConfiguration (if any).
    private final List<Pair<ScanDetail, WifiConfiguration>> mConnectableNetworks =
            new ArrayList<>();
    private List<ScanDetail> mFilteredNetworks = new ArrayList<>();
    private final WifiScoreCard mWifiScoreCard;
    private final ScoringParams mScoringParams;
    private final WifiInjector mWifiInjector;
    private final ThroughputPredictor mThroughputPredictor;
    private final WifiChannelUtilization mWifiChannelUtilization;
    private final WifiGlobals mWifiGlobals;
    private final ScanRequestProxy mScanRequestProxy;

    private final Map<String, WifiCandidates.CandidateScorer> mCandidateScorers = new ArrayMap<>();
    private boolean mIsEnhancedOpenSupportedInitialized = false;
    private boolean mIsEnhancedOpenSupported;

    /**
     * Interface for WiFi Network Nominator
     *
     * A network nominator examines the scan results reports the
     * connectable candidates in its category for further consideration.
     */
    public interface NetworkNominator {
        /** Type of nominators */
        int NOMINATOR_ID_SAVED = 0;
        int NOMINATOR_ID_SUGGESTION = 1;
        int NOMINATOR_ID_SCORED = 4;
        int NOMINATOR_ID_CURRENT = 5; // Should always be last

        @IntDef(prefix = {"NOMINATOR_ID_"}, value = {
                NOMINATOR_ID_SAVED,
                NOMINATOR_ID_SUGGESTION,
                NOMINATOR_ID_SCORED,
                NOMINATOR_ID_CURRENT})
        @Retention(RetentionPolicy.SOURCE)
        public @interface NominatorId {
        }

        /**
         * Get the nominator type.
         */
        @NominatorId
        int getId();

        /**
         * Get the nominator name.
         */
        String getName();

        /**
         * Update the nominator.
         *
         * Certain nominators have to be updated with the new scan results. For example
         * the ScoredNetworkNominator needs to refresh its Score Cache.
         *
         * @param scanDetails a list of scan details constructed from the scan results
         */
        void update(List<ScanDetail> scanDetails);

        /**
         * Evaluate all the networks from the scan results.
         *
         * @param scanDetails              a list of scan details constructed from the scan results
         * @param untrustedNetworkAllowed  a flag to indicate if untrusted networks are allowed
         * @param oemPaidNetworkAllowed    a flag to indicate if oem paid networks are allowed
         * @param oemPrivateNetworkAllowed a flag to indicate if oem private networks are allowed
         * @param onConnectableListener    callback to record all of the connectable networks
         */
        void nominateNetworks(List<ScanDetail> scanDetails,
                boolean untrustedNetworkAllowed, boolean oemPaidNetworkAllowed,
                boolean oemPrivateNetworkAllowed, OnConnectableListener onConnectableListener);

        /**
         * Callback for recording connectable candidates
         */
        public interface OnConnectableListener {
            /**
             * Notes that an access point is an eligible connection candidate
             *
             * @param scanDetail describes the specific access point
             * @param config     is the WifiConfiguration for the network
             */
            void onConnectable(ScanDetail scanDetail, WifiConfiguration config);
        }
    }

    private final List<NetworkNominator> mNominators = new ArrayList<>(3);

    // A helper to log debugging information in the local log buffer, which can
    // be retrieved in bugreport. It is also used to print the log in the console.
    private void localLog(String log) {
        mLocalLog.log(log);
        if (mVerboseLoggingEnabled) Log.d(TAG, log);
    }

    /**
     * Enable verbose logging in the console
     */
    public void enableVerboseLogging(boolean verbose) {
        mVerboseLoggingEnabled = verbose;
    }

    /**
     * Check if current network has sufficient RSSI
     *
     * @param wifiInfo info of currently connected network
     * @return true if current link quality is sufficient, false otherwise.
     */
    public boolean hasSufficientLinkQuality(WifiInfo wifiInfo) {
        int currentRssi = wifiInfo.getRssi();
        return currentRssi >= mScoringParams.getSufficientRssi(wifiInfo.getFrequency());
    }

    /**
     * Check if current network has active Tx or Rx traffic
     *
     * @param wifiInfo info of currently connected network
     * @return true if it has active Tx or Rx traffic, false otherwise.
     */
    public boolean hasActiveStream(WifiInfo wifiInfo) {
        return wifiInfo.getSuccessfulTxPacketsPerSecond()
                > mScoringParams.getActiveTrafficPacketsPerSecond()
                || wifiInfo.getSuccessfulRxPacketsPerSecond()
                > mScoringParams.getActiveTrafficPacketsPerSecond();
    }

    /**
     * Check if current network has internet or is expected to not have internet
     */
    public boolean hasInternetOrExpectNoInternet(WifiInfo wifiInfo) {
        WifiConfiguration network =
                mWifiConfigManager.getConfiguredNetwork(wifiInfo.getNetworkId());
        if (network == null) {
            return false;
        }
        return !network.hasNoInternetAccess() || network.isNoInternetAccessExpected();
    }
    /**
     * Determines whether the currently connected network is sufficient.
     *
     * If the network is good enough, or if switching to a new network is likely to
     * be disruptive, we should avoid doing a network selection.
     *
     * @param wifiInfo info of currently connected network
     * @return true if the network is sufficient
     */
    public boolean isNetworkSufficient(WifiInfo wifiInfo) {
        // Currently connected?
        if (wifiInfo.getSupplicantState() != SupplicantState.COMPLETED) {
            return false;
        }

        localLog("Current connected network: " + wifiInfo.getNetworkId());

        WifiConfiguration network =
                mWifiConfigManager.getConfiguredNetwork(wifiInfo.getNetworkId());

        if (network == null) {
            localLog("Current network was removed");
            return false;
        }

        // Skip autojoin for the first few seconds of a user-initiated connection.
        // This delays network selection during the time that connectivity service may be posting
        // a dialog about a no-internet network.
        if (mWifiConfigManager.getLastSelectedNetwork() == network.networkId
                && (mClock.getElapsedSinceBootMillis()
                    - mWifiConfigManager.getLastSelectedTimeStamp())
                <= mContext.getResources().getInteger(
                    R.integer.config_wifiSufficientDurationAfterUserSelectionMilliseconds)) {
            localLog("Current network is recently user-selected");
            return true;
        }

        // Set OSU (Online Sign Up) network for Passpoint Release 2 to sufficient
        // so that network select selection is skipped and OSU process can complete.
        if (network.osu) {
            localLog("Current connection is OSU");
            return true;
        }

        // OEM paid/private networks are only available to system apps, so this is never sufficient.
        if (network.oemPaid || network.oemPrivate) {
            localLog("Current network is oem paid/private");
            return false;
        }

        // Network without internet access is not sufficient, unless expected
        if (!hasInternetOrExpectNoInternet(wifiInfo)) {
            localLog("Current network has [" + network.numNoInternetAccessReports
                    + "] no-internet access reports");
            return false;
        }

        if (!hasSufficientLinkQuality(wifiInfo) && !hasActiveStream(wifiInfo)) {
            localLog("Current network link quality is not sufficient and has low ongoing traffic");
            return false;
        }

        return true;
    }

    private boolean isNetworkSelectionNeededForCmm(@NonNull ClientModeManagerState cmmState) {
        if (cmmState.connected) {
            // Is roaming allowed?
            if (!mContext.getResources().getBoolean(
                    R.bool.config_wifi_framework_enable_associated_network_selection)) {
                localLog(cmmState.ifaceName + ": Switching networks in connected state is not "
                        + "allowed. Skip network selection.");
                return false;
            }

            // Has it been at least the minimum interval since last network selection?
            if (mLastNetworkSelectionTimeStamp != INVALID_TIME_STAMP) {
                long gap = mClock.getElapsedSinceBootMillis()
                        - mLastNetworkSelectionTimeStamp;
                if (gap < MINIMUM_NETWORK_SELECTION_INTERVAL_MS) {
                    localLog(cmmState.ifaceName + ": Too short since last network selection: "
                            + gap + " ms. Skip network selection.");
                    return false;
                }
            }
            // Please note other scans (e.g., location scan or app scan) may also trigger network
            // selection and these scans may or may not run sufficiency check.
            // So it is better to run sufficiency check here before network selection.
            if (isNetworkSufficient(cmmState.wifiInfo)) {
                localLog(cmmState.ifaceName
                        + ": Current connected network already sufficient."
                        + " Skip network selection.");
                return false;
            } else {
                localLog(cmmState.ifaceName + ": Current connected network is not sufficient.");
                return true;
            }
        } else if (cmmState.disconnected) {
            return true;
        } else {
            // No network selection if ClientModeImpl is in a state other than
            // connected or disconnected (i.e connecting).
            localLog(cmmState.ifaceName + ": ClientModeImpl is in neither CONNECTED nor "
                    + "DISCONNECTED state. Skip network selection.");
            return false;
        }

    }

    private boolean isNetworkSelectionNeeded(@NonNull List<ScanDetail> scanDetails,
            @NonNull List<ClientModeManagerState> cmmStates) {
        if (scanDetails.size() == 0) {
            localLog("Empty connectivity scan results. Skip network selection.");
            return false;
        }
        for (ClientModeManagerState cmmState : cmmStates) {
            // network selection needed by this CMM instance, perform network selection
            if (isNetworkSelectionNeededForCmm(cmmState)) {
                return true;
            }
        }
        // none of the CMM instances need network selection, skip network selection.
        return false;
    }

    /**
     * Format the given ScanResult as a scan ID for logging.
     */
    public static String toScanId(@Nullable ScanResult scanResult) {
        return scanResult == null ? "NULL"
                : String.format("%s:%s", scanResult.SSID, scanResult.BSSID);
    }

    /**
     * Format the given WifiConfiguration as a SSID:netId string
     */
    public static String toNetworkString(WifiConfiguration network) {
        if (network == null) {
            return null;
        }

        return (network.SSID + ":" + network.networkId);
    }

    /**
     * Compares ScanResult level against the minimum threshold for its band, returns true if lower
     */
    public boolean isSignalTooWeak(ScanResult scanResult) {
        return (scanResult.level < mScoringParams.getEntryRssi(scanResult.frequency));
    }

    private List<ScanDetail> filterScanResults(List<ScanDetail> scanDetails,
            Set<String> bssidBlocklist, List<ClientModeManagerState> cmmStates) {
        List<ScanDetail> validScanDetails = new ArrayList<>();
        StringBuffer noValidSsid = new StringBuffer();
        StringBuffer blockedBssid = new StringBuffer();
        StringBuffer lowRssi = new StringBuffer();
        StringBuffer mboAssociationDisallowedBssid = new StringBuffer();
        List<String> currentBssids = cmmStates.stream()
                .map(cmmState -> cmmState.wifiInfo.getBSSID())
                .collect(Collectors.toList());
        Set<String> scanResultPresentForCurrentBssids = new ArraySet<>();
        int numBssidFiltered = 0;

        for (ScanDetail scanDetail : scanDetails) {
            ScanResult scanResult = scanDetail.getScanResult();

            if (TextUtils.isEmpty(scanResult.SSID)) {
                noValidSsid.append(scanResult.BSSID).append(" / ");
                continue;
            }

            // Check if the scan results contain the currently connected BSSID's
            if (currentBssids.contains(scanResult.BSSID)) {
                scanResultPresentForCurrentBssids.add(scanResult.BSSID);
                validScanDetails.add(scanDetail);
                continue;
            }

            final String scanId = toScanId(scanResult);

            if (bssidBlocklist.contains(scanResult.BSSID)) {
                blockedBssid.append(scanId).append(" / ");
                numBssidFiltered++;
                continue;
            }

            // Skip network with too weak signals.
            if (isSignalTooWeak(scanResult)) {
                lowRssi.append(scanId);
                if (scanResult.is24GHz()) {
                    lowRssi.append("(2.4GHz)");
                } else if (scanResult.is5GHz()) {
                    lowRssi.append("(5GHz)");
                } else if (scanResult.is6GHz()) {
                    lowRssi.append("(6GHz)");
                }
                lowRssi.append(scanResult.level).append(" / ");
                continue;
            }

            // Skip BSS which is not accepting new connections.
            NetworkDetail networkDetail = scanDetail.getNetworkDetail();
            if (networkDetail != null) {
                if (networkDetail.getMboAssociationDisallowedReasonCode()
                        != MboOceConstants.MBO_OCE_ATTRIBUTE_NOT_PRESENT) {
                    mWifiMetrics
                            .incrementNetworkSelectionFilteredBssidCountDueToMboAssocDisallowInd();
                    mboAssociationDisallowedBssid.append(scanId).append("(")
                            .append(networkDetail.getMboAssociationDisallowedReasonCode())
                            .append(")").append(" / ");
                    continue;
                }
            }

            validScanDetails.add(scanDetail);
        }
        mWifiMetrics.incrementNetworkSelectionFilteredBssidCount(numBssidFiltered);

        // WNS listens to all single scan results. Some scan requests may not include
        // the channel of the currently connected network, so the currently connected
        // network won't show up in the scan results. We don't act on these scan results
        // to avoid aggressive network switching which might trigger disconnection.
        // TODO(b/147751334) this may no longer be needed
        for (ClientModeManagerState cmmState : cmmStates) {
            // TODO (b/169413079): Disable network selection on corresponding CMM instead.
            if (cmmState.connected && cmmState.wifiInfo.getScore() >= WIFI_POOR_SCORE
                    && !scanResultPresentForCurrentBssids.contains(cmmState.wifiInfo.getBSSID())) {
                localLog("Current connected BSSID " + cmmState.wifiInfo.getBSSID()
                        + " is not in the scan results. Skip network selection.");
                validScanDetails.clear();
                return validScanDetails;
            }
        }

        if (noValidSsid.length() != 0) {
            localLog("Networks filtered out due to invalid SSID: " + noValidSsid);
        }

        if (blockedBssid.length() != 0) {
            localLog("Networks filtered out due to blocklist: " + blockedBssid);
        }

        if (lowRssi.length() != 0) {
            localLog("Networks filtered out due to low signal strength: " + lowRssi);
        }

        if (mboAssociationDisallowedBssid.length() != 0) {
            localLog("Networks filtered out due to mbo association disallowed indication: "
                    + mboAssociationDisallowedBssid);
        }

        return validScanDetails;
    }

    private ScanDetail findScanDetailForBssid(List<ScanDetail> scanDetails,
            String currentBssid) {
        for (ScanDetail scanDetail : scanDetails) {
            ScanResult scanResult = scanDetail.getScanResult();
            if (scanResult.BSSID.equals(currentBssid)) {
                return scanDetail;
            }
        }
        return null;
    }

    private boolean isEnhancedOpenSupported() {
        if (mIsEnhancedOpenSupportedInitialized) {
            return mIsEnhancedOpenSupported;
        }

        mIsEnhancedOpenSupportedInitialized = true;
        ClientModeManager primaryManager =
                mWifiInjector.getActiveModeWarden().getPrimaryClientModeManager();
        mIsEnhancedOpenSupported = (primaryManager.getSupportedFeatures() & WIFI_FEATURE_OWE) != 0;
        return mIsEnhancedOpenSupported;
    }

    /**
     * This returns a list of ScanDetails that were filtered in the process of network selection.
     * The list is further filtered for only open unsaved networks.
     *
     * @return the list of ScanDetails for open unsaved networks that do not have invalid SSIDS,
     * blocked BSSIDS, or low signal strength. This will return an empty list when there are
     * no open unsaved networks, or when network selection has not been run.
     */
    public List<ScanDetail> getFilteredScanDetailsForOpenUnsavedNetworks() {
        List<ScanDetail> openUnsavedNetworks = new ArrayList<>();
        boolean enhancedOpenSupported = isEnhancedOpenSupported();
        for (ScanDetail scanDetail : mFilteredNetworks) {
            ScanResult scanResult = scanDetail.getScanResult();

            if (!ScanResultUtil.isScanResultForOpenNetwork(scanResult)) {
                continue;
            }

            // Filter out Enhanced Open networks on devices that do not support it
            if (ScanResultUtil.isScanResultForOweNetwork(scanResult)
                    && !enhancedOpenSupported) {
                continue;
            }

            // Skip saved networks
            if (mWifiConfigManager.getSavedNetworkForScanDetailAndCache(scanDetail) != null) {
                continue;
            }

            openUnsavedNetworks.add(scanDetail);
        }
        return openUnsavedNetworks;
    }

    /**
     * @return the list of ScanDetails scored as potential candidates by the last run of
     * selectNetwork, this will be empty if Network selector determined no selection was
     * needed on last run. This includes scan details of sufficient signal strength, and
     * had an associated WifiConfiguration.
     */
    public List<Pair<ScanDetail, WifiConfiguration>> getConnectableScanDetails() {
        return mConnectableNetworks;
    }

    /**
     * Iterate thru the list of configured networks (includes all saved network configurations +
     * any ephemeral network configurations created for passpoint networks, suggestions, carrier
     * networks, etc) and do the following:
     * a) Try to re-enable any temporarily enabled networks (if the blocklist duration has expired).
     * b) Clear the {@link WifiConfiguration.NetworkSelectionStatus#getCandidate()} field for all
     * of them to identify networks that are present in the current scan result.
     * c) Log any disabled networks.
     */
    private void updateConfiguredNetworks() {
        List<WifiConfiguration> configuredNetworks = mWifiConfigManager.getConfiguredNetworks();
        if (configuredNetworks.size() == 0) {
            localLog("No configured networks.");
            return;
        }

        StringBuffer sbuf = new StringBuffer();
        for (WifiConfiguration network : configuredNetworks) {
            // If a configuration is temporarily disabled, re-enable it before trying
            // to connect to it.
            mWifiConfigManager.tryEnableNetwork(network.networkId);
            // Clear the cached candidate, score and seen.
            mWifiConfigManager.clearNetworkCandidateScanResult(network.networkId);

            // Log disabled network.
            WifiConfiguration.NetworkSelectionStatus status = network.getNetworkSelectionStatus();
            if (!status.isNetworkEnabled()) {
                sbuf.append("  ").append(toNetworkString(network)).append(" ");
                for (int index = WifiConfiguration.NetworkSelectionStatus
                        .NETWORK_SELECTION_DISABLED_STARTING_INDEX;
                        index < WifiConfiguration.NetworkSelectionStatus
                                .NETWORK_SELECTION_DISABLED_MAX;
                        index++) {
                    int count = status.getDisableReasonCounter(index);
                    // Here we log the reason as long as its count is greater than zero. The
                    // network may not be disabled because of this particular reason. Logging
                    // this information anyway to help understand what happened to the network.
                    if (count > 0) {
                        sbuf.append("reason=")
                                .append(WifiConfiguration.NetworkSelectionStatus
                                        .getNetworkSelectionDisableReasonString(index))
                                .append(", count=").append(count).append("; ");
                    }
                }
                sbuf.append("\n");
            }
        }

        if (sbuf.length() > 0) {
            localLog("Disabled configured networks:");
            localLog(sbuf.toString());
        }
    }

    /**
     * Overrides the {@code candidate} chosen by the {@link #mNominators} with the user chosen
     * {@link WifiConfiguration} if one exists.
     *
     * @return the user chosen {@link WifiConfiguration} if one exists, {@code candidate} otherwise
     */
    private WifiConfiguration overrideCandidateWithUserConnectChoice(
            @NonNull WifiConfiguration candidate) {
        WifiConfiguration tempConfig = Preconditions.checkNotNull(candidate);
        WifiConfiguration originalCandidate = candidate;
        ScanResult scanResultCandidate = candidate.getNetworkSelectionStatus().getCandidate();

        while (tempConfig.getNetworkSelectionStatus().getConnectChoice() != null) {
            String key = tempConfig.getNetworkSelectionStatus().getConnectChoice();
            int userSelectedRssi = tempConfig.getNetworkSelectionStatus().getConnectChoiceRssi();
            tempConfig = mWifiConfigManager.getConfiguredNetwork(key);

            if (tempConfig != null) {
                WifiConfiguration.NetworkSelectionStatus tempStatus =
                        tempConfig.getNetworkSelectionStatus();
                boolean noInternetButInternetIsExpected = !tempConfig.isNoInternetAccessExpected()
                        && tempConfig.hasNoInternetAccess();
                if (tempStatus.getCandidate() != null && tempStatus.isNetworkEnabled()
                        && !noInternetButInternetIsExpected
                        && isUserChoiceRssiCloseToOrGreaterThanExpectedValue(
                                tempStatus.getCandidate().level, userSelectedRssi)) {
                    scanResultCandidate = tempStatus.getCandidate();
                    candidate = tempConfig;
                }
            } else {
                localLog("Connect choice: " + key + " has no corresponding saved config.");
                break;
            }
        }

        if (candidate != originalCandidate) {
            localLog("After user selection adjustment, the final candidate is:"
                    + WifiNetworkSelector.toNetworkString(candidate) + " : "
                    + scanResultCandidate.BSSID);
            mWifiMetrics.setNominatorForNetwork(candidate.networkId,
                    WifiMetricsProto.ConnectionEvent.NOMINATOR_SAVED_USER_CONNECT_CHOICE);
        }
        return candidate;
    }

    private boolean isUserChoiceRssiCloseToOrGreaterThanExpectedValue(int observedRssi,
            int expectedRssi) {
        // The expectedRssi may be 0 for newly upgraded devices which do not have this information,
        // pass the test for those devices to avoid regression.
        if (expectedRssi == 0) {
            return true;
        }
        return observedRssi >= expectedRssi - mScoringParams.getEstimateRssiErrorMargin();
    }


    /**
     * Indicates whether we have ever seen the network to be metered since wifi was enabled.
     *
     * This is sticky to prevent continuous flip-flopping between networks, when the metered
     * status is learned after association.
     */
    private boolean isEverMetered(@NonNull WifiConfiguration config, @Nullable WifiInfo info,
            @NonNull ScanDetail scanDetail) {
        // If info does not match config, don't use it.
        if (info != null && info.getNetworkId() != config.networkId) info = null;
        boolean metered = WifiConfiguration.isMetered(config, info);
        NetworkDetail networkDetail = scanDetail.getNetworkDetail();
        if (networkDetail != null
                && networkDetail.getAnt()
                == NetworkDetail.Ant.ChargeablePublic) {
            metered = true;
        }
        mWifiMetrics.addMeteredStat(config, metered);
        if (config.meteredOverride != WifiConfiguration.METERED_OVERRIDE_NONE) {
            // User override is in effect; we should trust it
            if (mKnownMeteredNetworkIds.remove(config.networkId)) {
                localLog("KnownMeteredNetworkIds = " + mKnownMeteredNetworkIds);
            }
            metered = config.meteredOverride == WifiConfiguration.METERED_OVERRIDE_METERED;
        } else if (mKnownMeteredNetworkIds.contains(config.networkId)) {
            // Use the saved information
            metered = true;
        } else if (metered) {
            // Update the saved information
            mKnownMeteredNetworkIds.add(config.networkId);
            localLog("KnownMeteredNetworkIds = " + mKnownMeteredNetworkIds);
        }
        return metered;
    }

    /**
     * Returns the set of known metered network ids (for tests. dumpsys, and metrics).
     */
    public Set<Integer> getKnownMeteredNetworkIds() {
        return new ArraySet<>(mKnownMeteredNetworkIds);
    }

    private final ArraySet<Integer> mKnownMeteredNetworkIds = new ArraySet<>();


    /**
     * Cleans up state that should go away when wifi is disabled.
     */
    public void resetOnDisable() {
        mWifiConfigManager.clearLastSelectedNetwork();
        mKnownMeteredNetworkIds.clear();
    }

    /**
     * Container class for passing the ClientModeManager state for each instance that is managed by
     * WifiConnectivityManager, i.e all {@link ClientModeManager#getRole()} equals
     * {@link ActiveModeManager#ROLE_CLIENT_PRIMARY} or
     * {@link ActiveModeManager#ROLE_CLIENT_SECONDARY_LONG_LIVED}.
     */
    public static class ClientModeManagerState {
        /** Iface Name corresponding to iface (if known) */
        public final String ifaceName;
        /** True if the device is connected */
        public final boolean connected;
        /** True if the device is disconnected */
        public final boolean disconnected;
         /** Currently connected network */
        public final WifiInfo wifiInfo;

        ClientModeManagerState(@NonNull ClientModeManager clientModeManager) {
            ifaceName = clientModeManager.getInterfaceName();
            connected = clientModeManager.isConnected();
            disconnected = clientModeManager.isDisconnected();
            wifiInfo = clientModeManager.syncRequestConnectionInfo();
        }

        ClientModeManagerState() {
            ifaceName = "unknown";
            connected = false;
            disconnected = true;
            wifiInfo = new WifiInfo();
        }

        @VisibleForTesting
        ClientModeManagerState(@NonNull String ifaceName, boolean connected, boolean disconnected,
                @NonNull WifiInfo wifiInfo) {
            this.ifaceName = ifaceName;
            this.connected = connected;
            this.disconnected = disconnected;
            this.wifiInfo = wifiInfo;
        }

        @Override
        public boolean equals(Object that) {
            if (this == that) return true;
            if (!(that instanceof ClientModeManagerState)) return false;
            ClientModeManagerState thatCmmState = (ClientModeManagerState) that;
            return Objects.equals(ifaceName, thatCmmState.ifaceName)
                    && connected == thatCmmState.connected
                    && disconnected == thatCmmState.disconnected
                    // Since wifiinfo does not have equals currently.
                    && Objects.equals(wifiInfo.getSSID(), thatCmmState.wifiInfo.getSSID())
                    && Objects.equals(wifiInfo.getBSSID(), thatCmmState.wifiInfo.getBSSID());
        }

        @Override
        public int hashCode() {
            return Objects.hash(ifaceName, connected, disconnected,
                    wifiInfo.getSSID(), wifiInfo.getBSSID());
        }

        @Override
        public String toString() {
            return "ClientModeManagerState: " + ifaceName
                    + ", connection state: "
                    + (connected ? " connected" : (disconnected ? " disconnected" : "unknown"))
                    + ", WifiInfo: " + wifiInfo;
        }
    }

    /**
     * Returns the list of Candidates from networks in range.
     *
     * @param scanDetails              List of ScanDetail for all the APs in range
     * @param bssidBlocklist           Blocked BSSIDs
     * @param cmmStates                State of all long lived client mode manager instances -
     *                                 {@link ActiveModeManager#ROLE_CLIENT_PRIMARY} &
     *                                 {@link ActiveModeManager#ROLE_CLIENT_SECONDARY_LONG_LIVED}.
     * @param untrustedNetworkAllowed  True if untrusted networks are allowed for connection
     * @param oemPaidNetworkAllowed    True if oem paid networks are allowed for connection
     * @param oemPrivateNetworkAllowed True if oem private networks are allowed for connection
     * @return list of valid Candidate(s)
     */
    public List<WifiCandidates.Candidate> getCandidatesFromScan(
            @NonNull List<ScanDetail> scanDetails, @NonNull Set<String> bssidBlocklist,
            @NonNull List<ClientModeManagerState> cmmStates, boolean untrustedNetworkAllowed,
            boolean oemPaidNetworkAllowed, boolean oemPrivateNetworkAllowed) {
        mFilteredNetworks.clear();
        mConnectableNetworks.clear();
        if (scanDetails.size() == 0) {
            localLog("Empty connectivity scan result");
            return null;
        }

        // Update the scan detail cache at the start, even if we skip network selection
        updateScanDetailCache(scanDetails);

        // Update the registered network nominators.
        for (NetworkNominator registeredNominator : mNominators) {
            registeredNominator.update(scanDetails);
        }

        updateCandidatesSecurityParams(scanDetails);

        // Shall we start network selection at all?
        if (!isNetworkSelectionNeeded(scanDetails, cmmStates)) {
            return null;
        }

        // Filter out unwanted networks.
        mFilteredNetworks = filterScanResults(scanDetails, bssidBlocklist, cmmStates);
        if (mFilteredNetworks.size() == 0) {
            return null;
        }

        WifiCandidates wifiCandidates = new WifiCandidates(mWifiScoreCard, mContext);
        for (ClientModeManagerState cmmState : cmmStates) {
            // Always get the current BSSID from WifiInfo in case that firmware initiated
            // roaming happened.
            String currentBssid = cmmState.wifiInfo.getBSSID();
            WifiConfiguration currentNetwork =
                    mWifiConfigManager.getConfiguredNetwork(cmmState.wifiInfo.getNetworkId());
            if (currentNetwork != null) {
                wifiCandidates.setCurrent(currentNetwork.networkId, currentBssid);
                // We always want the current network to be a candidate so that it can participate.
                // It may also get re-added by a nominator, in which case this fallback
                // will be replaced.
                MacAddress bssid = MacAddress.fromString(currentBssid);
                SecurityParams params = currentNetwork.getNetworkSelectionStatus()
                        .getCandidateSecurityParams();
                if (null == params) {
                    localLog("No known candidate security params for current network.");
                    continue;
                }
                WifiCandidates.Key key = new WifiCandidates.Key(
                        ScanResultMatchInfo.fromWifiConfiguration(currentNetwork),
                        bssid, currentNetwork.networkId,
                        params.getSecurityType());
                ScanDetail scanDetail = findScanDetailForBssid(mFilteredNetworks, currentBssid);
                int predictedTputMbps = (scanDetail == null) ? 0 : predictThroughput(scanDetail);
                wifiCandidates.add(key, currentNetwork,
                        NetworkNominator.NOMINATOR_ID_CURRENT,
                        cmmState.wifiInfo.getRssi(),
                        cmmState.wifiInfo.getFrequency(),
                        calculateLastSelectionWeight(currentNetwork.networkId),
                        WifiConfiguration.isMetered(currentNetwork, cmmState.wifiInfo),
                        isFromCarrierOrPrivilegedApp(currentNetwork),
                        predictedTputMbps);
            }
        }

        // Update all configured networks before initiating network selection.
        updateConfiguredNetworks();

        for (NetworkNominator registeredNominator : mNominators) {
            localLog("About to run " + registeredNominator.getName() + " :");
            registeredNominator.nominateNetworks(
                    new ArrayList<>(mFilteredNetworks),
                    untrustedNetworkAllowed, oemPaidNetworkAllowed, oemPrivateNetworkAllowed,
                    (scanDetail, config) -> {
                        WifiCandidates.Key key = wifiCandidates.keyFromScanDetailAndConfig(
                                scanDetail, config);
                        if (key != null) {
                            boolean metered = false;
                            for (ClientModeManagerState cmmState : cmmStates) {
                                if (isEverMetered(config, cmmState.wifiInfo, scanDetail)) {
                                    metered = true;
                                    break;
                                }
                            }
                            // TODO(b/151981920) Saved passpoint candidates are marked ephemeral
                            boolean added = wifiCandidates.add(key, config,
                                    registeredNominator.getId(),
                                    scanDetail.getScanResult().level,
                                    scanDetail.getScanResult().frequency,
                                    calculateLastSelectionWeight(config.networkId),
                                    metered,
                                    isFromCarrierOrPrivilegedApp(config),
                                    predictThroughput(scanDetail));
                            if (added) {
                                mConnectableNetworks.add(Pair.create(scanDetail, config));
                                mWifiConfigManager.updateScanDetailForNetwork(
                                        config.networkId, scanDetail);
                                mWifiMetrics.setNominatorForNetwork(config.networkId,
                                        toProtoNominatorId(registeredNominator.getId()));
                            }
                        }
                    });
        }
        if (mConnectableNetworks.size() != wifiCandidates.size()) {
            localLog("Connectable: " + mConnectableNetworks.size()
                    + " Candidates: " + wifiCandidates.size());
        }
        return wifiCandidates.getCandidates();
    }

    /**
     * For transition networks with only legacy networks,
     * remove auto-upgrade type to use the legacy type to
     * avoid roaming issues between two types.
     */
    private void removeAutoUpgradeSecurityParamsIfNecessary(
            WifiConfiguration config,
            List<SecurityParams> scanResultParamsList,
            @WifiConfiguration.SecurityType int upgradableSecurityType,
            boolean isLegacyNetworkInRange,
            boolean isUpgradableTypeOnlyInRange,
            boolean isAutoUpgradeEnabled) {
        localLog("removeAutoUpgradeSecurityParamsIfNecessary:"
                + " upgradableSecurityType: " + upgradableSecurityType
                + " isLegacyNetworkInRange: " + isLegacyNetworkInRange
                + " isUpgradableTypeOnlyInRange: " + isUpgradableTypeOnlyInRange
                + " isAutoUpgradeEnabled: " + isAutoUpgradeEnabled);
        if (isAutoUpgradeEnabled) {
            // Consider removing the auto-upgraded type if legacy networks are in range.
            if (!isLegacyNetworkInRange) return;
            // If there are APs with standalone-upgradeable security type is in range,
            // do not consider removing the auto-upgraded type.
            if (isUpgradableTypeOnlyInRange) return;
        }

        SecurityParams upgradableParams = config.getSecurityParams(upgradableSecurityType);
        if (null == upgradableParams) return;
        if (!upgradableParams.isAddedByAutoUpgrade()) return;
        localLog("Remove upgradable security type " + upgradableSecurityType + " for the network.");
        scanResultParamsList.removeIf(p -> p.isSecurityType(upgradableSecurityType));
    }

    /** Helper function to place all conditions which need to remove auto-upgrade types. */
    private void removeSecurityParamsIfNecessary(
            WifiConfiguration config,
            List<SecurityParams> scanResultParamsList) {
        // When offload is supported, both types are passed down.
        if (!mWifiGlobals.isWpa3SaeUpgradeOffloadEnabled()) {
            removeAutoUpgradeSecurityParamsIfNecessary(
                    config, scanResultParamsList,
                    WifiConfiguration.SECURITY_TYPE_SAE,
                    mScanRequestProxy.isWpa2PersonalOnlyNetworkInRange(config.SSID),
                    mScanRequestProxy.isWpa3PersonalOnlyNetworkInRange(config.SSID),
                    mWifiGlobals.isWpa3SaeUpgradeEnabled());
        }
        removeAutoUpgradeSecurityParamsIfNecessary(
                config, scanResultParamsList,
                WifiConfiguration.SECURITY_TYPE_OWE,
                mScanRequestProxy.isOpenOnlyNetworkInRange(config.SSID),
                mScanRequestProxy.isOweOnlyNetworkInRange(config.SSID),
                mWifiGlobals.isOweUpgradeEnabled());
        removeAutoUpgradeSecurityParamsIfNecessary(
                config, scanResultParamsList,
                WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE,
                mScanRequestProxy.isWpa2EnterpriseOnlyNetworkInRange(config.SSID),
                mScanRequestProxy.isWpa3EnterpriseOnlyNetworkInRange(config.SSID),
                true);
    }

    /**
     * For the transition mode, MFPC should be true, and MFPR should be false,
     * see WPA3 SAE specification section 2.3 and 3.3.
     */
    private void updateSecurityParamsForTransitionModeIfNecessary(
            ScanResult scanResult, SecurityParams params) {
        if (params.isSecurityType(WifiConfiguration.SECURITY_TYPE_SAE)
                && ScanResultUtil.isScanResultForPskSaeTransitionNetwork(scanResult)) {
            params.setRequirePmf(false);
        } else if (params.isSecurityType(WifiConfiguration.SECURITY_TYPE_EAP_WPA3_ENTERPRISE)
                && ScanResultUtil.isScanResultForWpa3EnterpriseTransitionNetwork(scanResult)) {
            params.setRequirePmf(false);
        }
    }

    /**
     * Using the registered Scorers, choose the best network from the list of Candidate(s).
     * The ScanDetailCache is also updated here.
     * @param candidates - Candidates to perferm network selection on.
     * @return WifiConfiguration - the selected network, or null.
     */
    @Nullable
    public WifiConfiguration selectNetwork(@NonNull List<WifiCandidates.Candidate> candidates) {
        if (candidates == null || candidates.size() == 0) {
            return null;
        }
        WifiCandidates wifiCandidates = new WifiCandidates(mWifiScoreCard, mContext, candidates);
        final WifiCandidates.CandidateScorer activeScorer = getActiveCandidateScorer();
        // Update the NetworkSelectionStatus in the configs for the current candidates
        // This is needed for the legacy user connect choice, at least
        Collection<Collection<WifiCandidates.Candidate>> groupedCandidates =
                wifiCandidates.getGroupedCandidates();
        for (Collection<WifiCandidates.Candidate> group : groupedCandidates) {
            WifiCandidates.ScoredCandidate choice = activeScorer.scoreCandidates(group);
            if (choice == null) continue;
            ScanDetail scanDetail = getScanDetailForCandidateKey(choice.candidateKey);
            if (scanDetail == null) continue;
            WifiConfiguration config = mWifiConfigManager
                    .getConfiguredNetwork(choice.candidateKey.networkId);
            if (config == null) continue;
            List<SecurityParams> scanResultParamsList = ScanResultUtil
                    .generateSecurityParamsListFromScanResult(scanDetail.getScanResult());
            if (scanResultParamsList == null) continue;
            // Under some conditions, the legacy type is preferred to have better
            // connectivity behaviors, and the auto-upgrade type should be removed.
            removeSecurityParamsIfNecessary(config, scanResultParamsList);
            SecurityParams params = ScanResultMatchInfo.getBestMatchingSecurityParams(
                    config,
                    scanResultParamsList);
            if (params == null) continue;
            updateSecurityParamsForTransitionModeIfNecessary(scanDetail.getScanResult(), params);
            mWifiConfigManager.setNetworkCandidateScanResult(choice.candidateKey.networkId,
                    scanDetail.getScanResult(), 0, params);
        }

        for (Collection<WifiCandidates.Candidate> group : groupedCandidates) {
            for (WifiCandidates.Candidate candidate : group.stream()
                    .sorted((a, b) -> (b.getScanRssi() - a.getScanRssi())) // decreasing rssi
                    .collect(Collectors.toList())) {
                localLog(candidate.toString());
            }
        }

        ArrayMap<Integer, Integer> experimentNetworkSelections = new ArrayMap<>(); // for metrics

        int selectedNetworkId = WifiConfiguration.INVALID_NETWORK_ID;

        // Run all the CandidateScorers
        boolean legacyOverrideWanted = true;
        for (WifiCandidates.CandidateScorer candidateScorer : mCandidateScorers.values()) {
            WifiCandidates.ScoredCandidate choice;
            try {
                choice = wifiCandidates.choose(candidateScorer);
            } catch (RuntimeException e) {
                Log.wtf(TAG, "Exception running a CandidateScorer", e);
                continue;
            }
            int networkId = choice.candidateKey == null
                    ? WifiConfiguration.INVALID_NETWORK_ID
                    : choice.candidateKey.networkId;
            String chooses = " would choose ";
            if (candidateScorer == activeScorer) {
                chooses = " chooses ";
                legacyOverrideWanted = choice.userConnectChoiceOverride;
                selectedNetworkId = networkId;
                updateChosenPasspointNetwork(choice);
            }
            String id = candidateScorer.getIdentifier();
            int expid = experimentIdFromIdentifier(id);
            localLog(id + chooses + networkId
                    + " score " + choice.value + "+/-" + choice.err
                    + " expid " + expid);
            experimentNetworkSelections.put(expid, networkId);
        }

        // Update metrics about differences in the selections made by various methods
        final int activeExperimentId = experimentIdFromIdentifier(activeScorer.getIdentifier());
        for (Map.Entry<Integer, Integer> entry :
                experimentNetworkSelections.entrySet()) {
            int experimentId = entry.getKey();
            if (experimentId == activeExperimentId) continue;
            int thisSelectedNetworkId = entry.getValue();
            mWifiMetrics.logNetworkSelectionDecision(experimentId, activeExperimentId,
                    selectedNetworkId == thisSelectedNetworkId,
                    groupedCandidates.size());
        }

        // Get a fresh copy of WifiConfiguration reflecting any scan result updates
        WifiConfiguration selectedNetwork =
                mWifiConfigManager.getConfiguredNetwork(selectedNetworkId);
        if (selectedNetwork != null && legacyOverrideWanted) {
            selectedNetwork = overrideCandidateWithUserConnectChoice(selectedNetwork);
        }
        if (selectedNetwork != null) {
            mLastNetworkSelectionTimeStamp = mClock.getElapsedSinceBootMillis();
        }
        return selectedNetwork;
    }

    /**
     * Returns the ScanDetail given the candidate key, using the saved list of connectible networks.
     */
    private ScanDetail getScanDetailForCandidateKey(WifiCandidates.Key candidateKey) {
        if (candidateKey == null) return null;
        String bssid = candidateKey.bssid.toString();
        for (Pair<ScanDetail, WifiConfiguration> pair : mConnectableNetworks) {
            if (candidateKey.networkId == pair.second.networkId
                    && bssid.equals(pair.first.getBSSIDString())) {
                return pair.first;
            }
        }
        return null;
    }

    private void updateChosenPasspointNetwork(WifiCandidates.ScoredCandidate choice) {
        if (choice.candidateKey == null) {
            return;
        }
        WifiConfiguration config =
                mWifiConfigManager.getConfiguredNetwork(choice.candidateKey.networkId);
        if (config == null) {
            return;
        }
        if (config.isPasspoint()) {
            config.SSID = choice.candidateKey.matchInfo.networkSsid;
            mWifiConfigManager.addOrUpdateNetwork(config, config.creatorUid, config.creatorName);
        }
    }

    private void updateScanDetailCache(List<ScanDetail> scanDetails) {
        for (ScanDetail scanDetail : scanDetails) {
            mWifiConfigManager.updateScanDetailCacheFromScanDetailForSavedNetwork(scanDetail);
        }
    }

    private static int toProtoNominatorId(@NetworkNominator.NominatorId int nominatorId) {
        switch (nominatorId) {
            case NetworkNominator.NOMINATOR_ID_SAVED:
                return WifiMetricsProto.ConnectionEvent.NOMINATOR_SAVED;
            case NetworkNominator.NOMINATOR_ID_SUGGESTION:
                return WifiMetricsProto.ConnectionEvent.NOMINATOR_SUGGESTION;
            case NetworkNominator.NOMINATOR_ID_SCORED:
                return WifiMetricsProto.ConnectionEvent.NOMINATOR_EXTERNAL_SCORED;
            case NetworkNominator.NOMINATOR_ID_CURRENT:
                Log.e(TAG, "Unexpected NOMINATOR_ID_CURRENT", new RuntimeException());
                return WifiMetricsProto.ConnectionEvent.NOMINATOR_UNKNOWN;
            default:
                Log.e(TAG, "UnrecognizedNominatorId" + nominatorId);
                return WifiMetricsProto.ConnectionEvent.NOMINATOR_UNKNOWN;
        }
    }

    private double calculateLastSelectionWeight(int networkId) {
        if (networkId != mWifiConfigManager.getLastSelectedNetwork()) return 0.0;
        double timeDifference = mClock.getElapsedSinceBootMillis()
                - mWifiConfigManager.getLastSelectedTimeStamp();
        long millis = TimeUnit.MINUTES.toMillis(mScoringParams.getLastSelectionMinutes());
        if (timeDifference >= millis) return 0.0;
        double unclipped = 1.0 - (timeDifference / millis);
        return Math.min(Math.max(unclipped, 0.0), 1.0);
    }

    private WifiCandidates.CandidateScorer getActiveCandidateScorer() {
        WifiCandidates.CandidateScorer ans = mCandidateScorers.get(PRESET_CANDIDATE_SCORER_NAME);
        int overrideExperimentId = mScoringParams.getExperimentIdentifier();
        if (overrideExperimentId >= MIN_SCORER_EXP_ID) {
            for (WifiCandidates.CandidateScorer candidateScorer : mCandidateScorers.values()) {
                int expId = experimentIdFromIdentifier(candidateScorer.getIdentifier());
                if (expId == overrideExperimentId) {
                    ans = candidateScorer;
                    break;
                }
            }
        }
        if (ans == null && PRESET_CANDIDATE_SCORER_NAME != null) {
            Log.wtf(TAG, PRESET_CANDIDATE_SCORER_NAME + " is not registered!");
        }
        mWifiMetrics.setNetworkSelectorExperimentId(ans == null
                ? LEGACY_CANDIDATE_SCORER_EXP_ID
                : experimentIdFromIdentifier(ans.getIdentifier()));
        return ans;
    }

    private int predictThroughput(@NonNull ScanDetail scanDetail) {
        if (scanDetail.getScanResult() == null || scanDetail.getNetworkDetail() == null) {
            return 0;
        }
        int channelUtilizationLinkLayerStats = BssLoad.INVALID;
        if (mWifiChannelUtilization != null) {
            channelUtilizationLinkLayerStats =
                    mWifiChannelUtilization.getUtilizationRatio(
                            scanDetail.getScanResult().frequency);
        }
        ClientModeManager primaryManager =
                mWifiInjector.getActiveModeWarden().getPrimaryClientModeManager();
        return mThroughputPredictor.predictThroughput(
                primaryManager.getDeviceWiphyCapabilities(),
                scanDetail.getScanResult().getWifiStandard(),
                scanDetail.getScanResult().channelWidth,
                scanDetail.getScanResult().level,
                scanDetail.getScanResult().frequency,
                scanDetail.getNetworkDetail().getMaxNumberSpatialStreams(),
                scanDetail.getNetworkDetail().getChannelUtilization(),
                channelUtilizationLinkLayerStats,
                mWifiGlobals.isBluetoothConnected());
    }

    /**
     * Register a network nominator
     *
     * @param nominator the network nominator to be registered
     */
    public void registerNetworkNominator(@NonNull NetworkNominator nominator) {
        mNominators.add(Preconditions.checkNotNull(nominator));
    }

    /**
     * Register a candidate scorer.
     *
     * Replaces any existing scorer having the same identifier.
     */
    public void registerCandidateScorer(@NonNull WifiCandidates.CandidateScorer candidateScorer) {
        String name = Preconditions.checkNotNull(candidateScorer).getIdentifier();
        if (name != null) {
            mCandidateScorers.put(name, candidateScorer);
        }
    }

    /**
     * Unregister a candidate scorer.
     */
    public void unregisterCandidateScorer(@NonNull WifiCandidates.CandidateScorer candidateScorer) {
        String name = Preconditions.checkNotNull(candidateScorer).getIdentifier();
        if (name != null) {
            mCandidateScorers.remove(name);
        }
    }

    private static boolean isFromCarrierOrPrivilegedApp(WifiConfiguration config) {
        if (config.fromWifiNetworkSuggestion
                && config.carrierId != TelephonyManager.UNKNOWN_CARRIER_ID) {
            // Privileged carrier suggestion
            return true;
        }
        if (config.isEphemeral()
                && !config.fromWifiNetworkSpecifier
                && !config.fromWifiNetworkSuggestion) {
            // From ScoredNetworkNominator
            return true;
        }
        return false;
    }

    /**
     * Derives a numeric experiment identifier from a CandidateScorer's identifier.
     *
     * @returns a positive number that starts with the decimal digits ID_PREFIX
     */
    public static int experimentIdFromIdentifier(String id) {
        final int digits = (int) (((long) id.hashCode()) & Integer.MAX_VALUE) % ID_SUFFIX_MOD;
        return ID_PREFIX * ID_SUFFIX_MOD + digits;
    }

    private static final int ID_SUFFIX_MOD = 1_000_000;
    private static final int ID_PREFIX = 42;
    private static final int MIN_SCORER_EXP_ID = ID_PREFIX * ID_SUFFIX_MOD;

    WifiNetworkSelector(
            Context context,
            WifiScoreCard wifiScoreCard,
            ScoringParams scoringParams,
            WifiConfigManager configManager,
            Clock clock,
            LocalLog localLog,
            WifiMetrics wifiMetrics,
            WifiInjector wifiInjector,
            ThroughputPredictor throughputPredictor,
            WifiChannelUtilization wifiChannelUtilization,
            WifiGlobals wifiGlobals,
            ScanRequestProxy scanRequestProxy) {
        mContext = context;
        mWifiScoreCard = wifiScoreCard;
        mScoringParams = scoringParams;
        mWifiConfigManager = configManager;
        mClock = clock;
        mLocalLog = localLog;
        mWifiMetrics = wifiMetrics;
        mWifiInjector = wifiInjector;
        mThroughputPredictor = throughputPredictor;
        mWifiChannelUtilization = wifiChannelUtilization;
        mWifiGlobals = wifiGlobals;
        mScanRequestProxy = scanRequestProxy;
    }

    private void updateCandidatesSecurityParams(List<ScanDetail> scanDetails) {
        for (ScanDetail scanDetail : scanDetails) {
            WifiConfiguration network =
                    mWifiConfigManager.getSavedNetworkForScanDetail(scanDetail);
            if (network == null || network.getSecurityParamsList().size() < 2) continue;

            List<SecurityParams> scanResultParamsList = ScanResultUtil
                    .generateSecurityParamsListFromScanResult(scanDetail.getScanResult());
            if (scanResultParamsList == null) continue;

            SecurityParams params = ScanResultMatchInfo.getBestMatchingSecurityParams(network,
                    scanResultParamsList);
            if (params == null) continue;

            network.getNetworkSelectionStatus().setCandidateSecurityParams(params);
        }
    }
}
