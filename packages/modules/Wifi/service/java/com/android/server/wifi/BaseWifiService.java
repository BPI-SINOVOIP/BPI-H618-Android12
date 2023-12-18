/*
 * Copyright (c) 2018 The Android Open Source Project
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

import android.annotation.NonNull;
import android.net.DhcpInfo;
import android.net.Network;
import android.net.wifi.CoexUnsafeChannel;
import android.net.wifi.IActionListener;
import android.net.wifi.ICoexCallback;
import android.net.wifi.IDppCallback;
import android.net.wifi.ILocalOnlyHotspotCallback;
import android.net.wifi.INetworkRequestMatchCallback;
import android.net.wifi.IOnWifiActivityEnergyInfoListener;
import android.net.wifi.IOnWifiUsabilityStatsListener;
import android.net.wifi.IScanResultsCallback;
import android.net.wifi.ISoftApCallback;
import android.net.wifi.ISubsystemRestartCallback;
import android.net.wifi.ISuggestionConnectionStatusListener;
import android.net.wifi.ISuggestionUserApprovalStatusListener;
import android.net.wifi.ITrafficStateCallback;
import android.net.wifi.IWifiConnectedNetworkScorer;
import android.net.wifi.IWifiManager;
import android.net.wifi.IWifiVerboseLoggingStatusChangedListener;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.WifiAvailableChannel;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSuggestion;
import android.net.wifi.hotspot2.IProvisioningCallback;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.hotspot2.PasspointConfiguration;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.WorkSource;

import com.android.modules.utils.ParceledListSlice;

import java.util.List;
import java.util.Map;

/**
 * Empty concrete class implementing IWifiManager with stub methods throwing runtime exceptions.
 *
 * This class is meant to be extended by real implementations of IWifiManager in order to facilitate
 * cross-repo changes to WiFi internal APIs, including the introduction of new APIs, the removal of
 * deprecated APIs, or the migration of existing API signatures.
 *
 * When an existing API is scheduled for removal, it can be removed from IWifiManager.aidl
 * immediately and marked as @Deprecated first in this class. Children inheriting this class are
 * then given a short grace period to update themselves before the @Deprecated stub is removed for
 * good. If the API scheduled for removal has a replacement or an overload (signature change),
 * these should be introduced before the stub is removed to allow children to migrate.
 *
 * When a new API is added to IWifiManager.aidl, a stub should be added in BaseWifiService as
 * well otherwise compilation will fail.
 */
public class BaseWifiService extends IWifiManager.Stub {

    private static final String TAG = BaseWifiService.class.getSimpleName();

    @Override
    public long getSupportedFeatures() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void getWifiActivityEnergyInfoAsync(IOnWifiActivityEnergyInfoListener listener) {
        throw new UnsupportedOperationException();
    }

    @Deprecated
    public ParceledListSlice getConfiguredNetworks(String packageName, String featureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public ParceledListSlice getConfiguredNetworks(String packageName, String featureId,
            boolean callerNetworksOnly) {
        throw new UnsupportedOperationException();
    }

    @Override
    public ParceledListSlice getPrivilegedConfiguredNetworks(String packageName, String featureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<String, Map<Integer, List<ScanResult>>> getAllMatchingFqdnsForScanResults(
            List<ScanResult> scanResults) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<OsuProvider, List<ScanResult>> getMatchingOsuProviders(
            List<ScanResult> scanResults) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<OsuProvider, PasspointConfiguration> getMatchingPasspointConfigsForOsuProviders(
            List<OsuProvider> osuProviders) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int addOrUpdateNetwork(WifiConfiguration config, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public WifiManager.AddNetworkResult addOrUpdateNetworkPrivileged(WifiConfiguration config,
            String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean addOrUpdatePasspointConfiguration(
            PasspointConfiguration config, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean removePasspointConfiguration(String fqdn, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<PasspointConfiguration> getPasspointConfigurations(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<WifiConfiguration> getWifiConfigsForPasspointProfiles(List<String> fqdnList) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void queryPasspointIcon(long bssid, String fileName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int matchProviderWithCurrentNetwork(String fqdn) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean removeNetwork(int netId, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean removeNonCallerConfiguredNetworks(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean enableNetwork(int netId, boolean disableOthers, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean disableNetwork(int netId, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void allowAutojoinGlobal(boolean choice) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void allowAutojoin(int netId, boolean choice) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void allowAutojoinPasspoint(String fqdn, boolean enableAutoJoin) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setMacRandomizationSettingPasspointEnabled(String fqdn, boolean enable) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setPasspointMeteredOverride(String fqdn, int meteredOverride) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean startScan(String packageName, String featureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<ScanResult> getScanResults(String callingPackage, String callingFeatureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean disconnect(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean reconnect(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean reassociate(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public WifiInfo getConnectionInfo(String callingPackage, String callingFeatureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean setWifiEnabled(String packageName, boolean enable) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerSubsystemRestartCallback(ISubsystemRestartCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterSubsystemRestartCallback(ISubsystemRestartCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void restartWifiSubsystem() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getWifiEnabledState() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getCountryCode() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setOverrideCountryCode(@NonNull String countryCode) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void clearOverrideCountryCode() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setDefaultCountryCode(@NonNull String countryCode) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean is24GHzBandSupported() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean is5GHzBandSupported() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean is6GHzBandSupported() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean is60GHzBandSupported() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isWifiStandardSupported(int standard) {
        throw new UnsupportedOperationException();
    }

    @Override
    public DhcpInfo getDhcpInfo(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setScanAlwaysAvailable(boolean isAvailable, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isScanAlwaysAvailable() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean acquireWifiLock(IBinder lock, int lockType, String tag, WorkSource ws) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void updateWifiLockWorkSource(IBinder lock, WorkSource ws) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean releaseWifiLock(IBinder lock) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void initializeMulticastFiltering() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isMulticastEnabled() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void acquireMulticastLock(IBinder binder, String tag) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void releaseMulticastLock(String tag) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void updateInterfaceIpState(String ifaceName, int mode) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isDefaultCoexAlgorithmEnabled() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setCoexUnsafeChannels(List<CoexUnsafeChannel> unsafeChannels, int restrictions) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerCoexCallback(ICoexCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterCoexCallback(ICoexCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean startSoftAp(WifiConfiguration wifiConfig, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean startTetheredHotspot(SoftApConfiguration softApConfig, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean stopSoftAp() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int startLocalOnlyHotspot(ILocalOnlyHotspotCallback callback, String packageName,
            String featureId, SoftApConfiguration customConfig) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void stopLocalOnlyHotspot() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startWatchLocalOnlyHotspot(ILocalOnlyHotspotCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void stopWatchLocalOnlyHotspot() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getWifiApEnabledState() {
        throw new UnsupportedOperationException();
    }

    @Override
    public WifiConfiguration getWifiApConfiguration() {
        throw new UnsupportedOperationException();
    }

    @Override
    public SoftApConfiguration getSoftApConfiguration() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean setWifiApConfiguration(WifiConfiguration wifiConfig, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean setSoftApConfiguration(SoftApConfiguration softApConfig, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void notifyUserOfApBandConversion(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void enableTdls(String remoteIPAddress, boolean enable) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void enableTdlsWithMacAddress(String remoteMacAddress, boolean enable) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getCurrentNetworkWpsNfcConfigurationToken() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void enableVerboseLogging(int verbose) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getVerboseLoggingLevel() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void disableEphemeralNetwork(String SSID, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void factoryReset(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Network getCurrentNetwork() {
        throw new UnsupportedOperationException();
    }

    @Override
    public byte[] retrieveBackupData() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void restoreBackupData(byte[] data) {
        throw new UnsupportedOperationException();
    }

    @Override
    public byte[] retrieveSoftApBackupData() {
        throw new UnsupportedOperationException();
    }

    @Override
    public SoftApConfiguration restoreSoftApBackupData(byte[] data) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void restoreSupplicantBackupData(byte[] supplicantData, byte[] ipConfigData) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startSubscriptionProvisioning(
            OsuProvider provider, IProvisioningCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void addWifiVerboseLoggingStatusChangedListener(
            IWifiVerboseLoggingStatusChangedListener callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void removeWifiVerboseLoggingStatusChangedListener(
            IWifiVerboseLoggingStatusChangedListener callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerSoftApCallback(ISoftApCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterSoftApCallback(ISoftApCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerTrafficStateCallback(ITrafficStateCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterTrafficStateCallback(ITrafficStateCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerNetworkRequestMatchCallback(INetworkRequestMatchCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterNetworkRequestMatchCallback(INetworkRequestMatchCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int addNetworkSuggestions(
            List<WifiNetworkSuggestion> networkSuggestions, String callingPackageName,
            String callingFeatureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int removeNetworkSuggestions(
            List<WifiNetworkSuggestion> networkSuggestions, String callingPackageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<WifiNetworkSuggestion> getNetworkSuggestions(String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setCarrierNetworkOffloadEnabled(int subId, boolean merged, boolean enabled)
            throws RemoteException {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isCarrierNetworkOffloadEnabled(int subId, boolean merged)
            throws RemoteException {
        throw new UnsupportedOperationException();
    }

    @Override
    public String[] getFactoryMacAddresses() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setDeviceMobilityState(int state) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startDppAsConfiguratorInitiator(IBinder binder, String packageName,
            String enrolleeUri, int selectedNetworkId, int netRole, IDppCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startDppAsEnrolleeInitiator(IBinder binder, String configuratorUri,
            IDppCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startDppAsEnrolleeResponder(IBinder binder, String deviceInfo,
            int curve, IDppCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void stopDppSession() throws RemoteException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void addOnWifiUsabilityStatsListener(IOnWifiUsabilityStatsListener listener) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void removeOnWifiUsabilityStatsListener(IOnWifiUsabilityStatsListener listener) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void updateWifiUsabilityScore(int seqNum, int score, int predictionHorizonSec) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void connect(WifiConfiguration config, int netId, IActionListener callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startRestrictingAutoJoinToSubscriptionId(int subId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void stopRestrictingAutoJoinToSubscriptionId() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void save(WifiConfiguration config, IActionListener callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void forget(int netId, IActionListener callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerScanResultsCallback(IScanResultsCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterScanResultsCallback(IScanResultsCallback callback) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerSuggestionConnectionStatusListener(
            ISuggestionConnectionStatusListener listener, String packageName, String featureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unregisterSuggestionConnectionStatusListener(
            ISuggestionConnectionStatusListener listener, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int calculateSignalLevel(int rssi) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<WifiConfiguration> getWifiConfigForMatchedNetworkSuggestionsSharedWithUser(
            List<ScanResult> scanResults) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean setWifiConnectedNetworkScorer(IBinder binder,
            IWifiConnectedNetworkScorer scorer) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void clearWifiConnectedNetworkScorer() {
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<WifiNetworkSuggestion, List<ScanResult>> getMatchingScanResults(
            List<WifiNetworkSuggestion> networkSuggestions,
            List<ScanResult> scanResults,
            String callingPackage, String callingFeatureId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setScanThrottleEnabled(boolean enable) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isScanThrottleEnabled() {
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<String, Map<Integer, List<ScanResult>>>
            getAllMatchingPasspointProfilesForScanResults(List<ScanResult> scanResults) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setAutoWakeupEnabled(boolean enable) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isAutoWakeupEnabled() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void addSuggestionUserApprovalStatusListener(
            ISuggestionUserApprovalStatusListener listener, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void removeSuggestionUserApprovalStatusListener(
            ISuggestionUserApprovalStatusListener listener, String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setEmergencyScanRequestInProgress(boolean inProgress) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void removeAppState(int targetAppUid, @NonNull String targetAppPackageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean setWifiScoringEnabled(boolean enabled) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void flushPasspointAnqpCache(@NonNull String packageName) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<WifiAvailableChannel> getUsableChannels(
            int band, int mode, int filter) {
        throw new UnsupportedOperationException();
    }
}

