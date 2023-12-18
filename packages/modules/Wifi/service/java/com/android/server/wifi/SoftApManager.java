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

import static android.net.wifi.WifiManager.SAP_CLIENT_DISCONNECT_REASON_CODE_UNSPECIFIED;

import static com.android.server.wifi.util.ApConfigUtil.ERROR_GENERIC;
import static com.android.server.wifi.util.ApConfigUtil.ERROR_NO_CHANNEL;
import static com.android.server.wifi.util.ApConfigUtil.ERROR_UNSUPPORTED_CONFIGURATION;
import static com.android.server.wifi.util.ApConfigUtil.SUCCESS;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Intent;
import android.net.MacAddress;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.SoftApInfo;
import android.net.wifi.WifiAnnotations;
import android.net.wifi.WifiClient;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.WorkSource;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.IState;
import com.android.internal.util.Preconditions;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.internal.util.WakeupMessage;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.WifiNative.InterfaceCallback;
import com.android.server.wifi.WifiNative.SoftApListener;
import com.android.server.wifi.coex.CoexManager;
import com.android.server.wifi.coex.CoexManager.CoexListener;
import com.android.server.wifi.util.ApConfigUtil;
import com.android.wifi.resources.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Manage WiFi in AP mode.
 * The internal state machine runs under the ClientModeImpl handler thread context.
 */
public class SoftApManager implements ActiveModeManager {
    private static final String TAG = "SoftApManager";

    @VisibleForTesting
    public static final String SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG = TAG
            + " Soft AP Send Message Timeout";
    @VisibleForTesting
    public static final String SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG = TAG
            + " Soft AP Send Message Bridged Mode Idle Timeout";


    private final WifiContext mContext;
    private final FrameworkFacade mFrameworkFacade;
    private final WifiNative mWifiNative;
    // This will only be null if SdkLevel is not at least S
    @Nullable private final CoexManager mCoexManager;
    private final ClientModeImplMonitor mCmiMonitor;
    private final ActiveModeWarden mActiveModeWarden;
    private final SoftApNotifier mSoftApNotifier;

    @VisibleForTesting
    static final long SOFT_AP_PENDING_DISCONNECTION_CHECK_DELAY_MS = 1000;

    private String mCountryCode;

    private final SoftApStateMachine mStateMachine;

    private final Listener<SoftApManager> mModeListener;
    private final WifiServiceImpl.SoftApCallbackInternal mSoftApCallback;

    private String mApInterfaceName;
    private boolean mIfaceIsUp;
    private boolean mIfaceIsDestroyed;

    private final WifiApConfigStore mWifiApConfigStore;

    private final WifiMetrics mWifiMetrics;
    private final long mId;

    private boolean mIsUnsetBssid;

    private boolean mVerboseLoggingEnabled = false;

    /**
     * Original configuration, which is the passed configuration when init or
     * the user-configured {@code WifiApConfigStore#getApConfiguration}tethering}
     * settings when input is null.
     *
     * Use it when doing configuration update to know if the input configuration was changed.
     * For others use case, it should use {@code mCurrentSoftApConfiguration}.
     */
    @NonNull
    private final SoftApModeConfiguration mOriginalModeConfiguration;


    /**
     * Current Soft AP configuration which is used to start Soft AP.
     * The configuration may be changed because
     * 1. bssid is changed because MAC randomization
     * 2. bands are changed because fallback to single AP mode mechanism.
     */
    @Nullable
    private SoftApConfiguration mCurrentSoftApConfiguration;

    @NonNull
    private Map<String, SoftApInfo> mCurrentSoftApInfoMap = new HashMap<>();

    @NonNull
    private SoftApCapability mCurrentSoftApCapability;

    private Map<String, List<WifiClient>> mConnectedClientWithApInfoMap = new HashMap<>();
    @VisibleForTesting
    Map<WifiClient, Integer> mPendingDisconnectClients = new HashMap<>();

    private boolean mTimeoutEnabled = false;
    private boolean mBridgedModeOpportunisticsShutdownTimeoutEnabled = false;

    private final SarManager mSarManager;

    private String mStartTimestamp;

    private long mDefaultShutdownTimeoutMillis;

    private long mDefaultShutdownIdleInstanceInBridgedModeTimeoutMillis;

    private static final SimpleDateFormat FORMATTER = new SimpleDateFormat("MM-dd HH:mm:ss.SSS");

    private WifiDiagnostics mWifiDiagnostics;

    @Nullable
    private SoftApRole mRole = null;
    @Nullable
    private WorkSource mRequestorWs = null;

    private boolean mEverReportMetricsForMaxClient = false;

    @NonNull
    private Set<MacAddress> mBlockedClientList = new HashSet<>();

    @NonNull
    private Set<MacAddress> mAllowedClientList = new HashSet<>();

    @NonNull
    private Set<Integer> mSafeChannelFrequencyList = new HashSet<>();

    @VisibleForTesting
    public WakeupMessage mSoftApTimeoutMessage;
    @VisibleForTesting
    public WakeupMessage mSoftApBridgedModeIdleInstanceTimeoutMessage;

    // Internal flag which is used to avoid the timer re-schedule.
    private boolean mIsBridgedModeIdleInstanceTimerActive = false;

    /**
     * Listener for soft AP events.
     */
    private final SoftApListener mSoftApListener = new SoftApListener() {
        @Override
        public void onFailure() {
            mStateMachine.sendMessage(SoftApStateMachine.CMD_FAILURE);
        }

        @Override
        public void onInfoChanged(String apIfaceInstance, int frequency,
                @WifiAnnotations.Bandwidth int bandwidth,
                @WifiAnnotations.WifiStandard int generation,
                MacAddress apIfaceInstanceMacAddress) {
            SoftApInfo apInfo = new SoftApInfo();
            apInfo.setFrequency(frequency);
            apInfo.setBandwidth(bandwidth);
            apInfo.setWifiStandard(generation);
            if (apIfaceInstanceMacAddress != null) {
                apInfo.setBssid(apIfaceInstanceMacAddress);
            }
            apInfo.setApInstanceIdentifier(apIfaceInstance != null
                    ? apIfaceInstance : mApInterfaceName);
            mStateMachine.sendMessage(
                    SoftApStateMachine.CMD_AP_INFO_CHANGED, 0, 0, apInfo);
        }

        @Override
        public void onConnectedClientsChanged(String apIfaceInstance, MacAddress clientAddress,
                boolean isConnected) {
            if (clientAddress != null) {
                WifiClient client = new WifiClient(clientAddress, apIfaceInstance != null
                        ? apIfaceInstance : mApInterfaceName);
                mStateMachine.sendMessage(SoftApStateMachine.CMD_ASSOCIATED_STATIONS_CHANGED,
                        isConnected ? 1 : 0, 0, client);
            } else {
                Log.e(getTag(), "onConnectedClientsChanged: Invalid type returned");
            }
        }
    };

    private final CoexListener mCoexListener = new CoexListener() {
        @Override
        public void onCoexUnsafeChannelsChanged() {
            if (mCurrentSoftApConfiguration == null) {
                return;
            }
            mStateMachine.sendMessage(SoftApStateMachine.CMD_SAFE_CHANNEL_FREQUENCY_CHANGED);
        }
    };

    private void updateSafeChannelFrequencyList() {
        if (!SdkLevel.isAtLeastS() || mCurrentSoftApConfiguration == null) {
            return;
        }
        mSafeChannelFrequencyList.clear();
        for (int configuredBand : mCurrentSoftApConfiguration.getBands()) {
            for (int band : SoftApConfiguration.BAND_TYPES) {
                if ((band & configuredBand) == 0) {
                    continue;
                }
                for (int channel : mCurrentSoftApCapability.getSupportedChannelList(band)) {
                    mSafeChannelFrequencyList.add(
                            ApConfigUtil.convertChannelToFrequency(channel, band));
                }
            }
        }
        if ((mCoexManager.getCoexRestrictions() & WifiManager.COEX_RESTRICTION_SOFTAP) != 0) {
            mSafeChannelFrequencyList.removeAll(
                    ApConfigUtil.getUnsafeChannelFreqsFromCoex(mCoexManager));
        }
        Log.d(getTag(), "SafeChannelFrequencyList = " + mSafeChannelFrequencyList);
    }

    private void configureInternalConfiguration() {
        if (mCurrentSoftApConfiguration == null) {
            return;
        }
        mBlockedClientList = new HashSet<>(mCurrentSoftApConfiguration.getBlockedClientList());
        mAllowedClientList = new HashSet<>(mCurrentSoftApConfiguration.getAllowedClientList());
        mTimeoutEnabled = mCurrentSoftApConfiguration.isAutoShutdownEnabled();
        mBridgedModeOpportunisticsShutdownTimeoutEnabled =
                mCurrentSoftApConfiguration.isBridgedModeOpportunisticShutdownEnabledInternal();
    }

    private void updateChangeableConfiguration(SoftApConfiguration newConfig) {
        if (mCurrentSoftApConfiguration == null || newConfig == null) {
            return;
        }
        /**
         * update configurations only which mentioned in WifiManager#setSoftApConfiguration
         */
        SoftApConfiguration.Builder newConfigurBuilder =
                new SoftApConfiguration.Builder(mCurrentSoftApConfiguration)
                .setAllowedClientList(newConfig.getAllowedClientList())
                .setBlockedClientList(newConfig.getBlockedClientList())
                .setClientControlByUserEnabled(newConfig.isClientControlByUserEnabled())
                .setMaxNumberOfClients(newConfig.getMaxNumberOfClients())
                .setShutdownTimeoutMillis(newConfig.getShutdownTimeoutMillis())
                .setAutoShutdownEnabled(newConfig.isAutoShutdownEnabled());
        if (SdkLevel.isAtLeastS()) {
            newConfigurBuilder.setBridgedModeOpportunisticShutdownEnabled(
                    newConfig.isBridgedModeOpportunisticShutdownEnabledInternal());
        }
        mCurrentSoftApConfiguration = newConfigurBuilder.build();
        configureInternalConfiguration();
    }

    public SoftApManager(
            @NonNull WifiContext context,
            @NonNull Looper looper,
            @NonNull FrameworkFacade framework,
            @NonNull WifiNative wifiNative,
            @NonNull CoexManager coexManager,
            String countryCode,
            @NonNull Listener<SoftApManager> listener,
            @NonNull WifiServiceImpl.SoftApCallbackInternal callback,
            @NonNull WifiApConfigStore wifiApConfigStore,
            @NonNull SoftApModeConfiguration apConfig,
            @NonNull WifiMetrics wifiMetrics,
            @NonNull SarManager sarManager,
            @NonNull WifiDiagnostics wifiDiagnostics,
            @NonNull SoftApNotifier softApNotifier,
            @NonNull ClientModeImplMonitor cmiMonitor,
            @NonNull ActiveModeWarden activeModeWarden,
            long id,
            @NonNull WorkSource requestorWs,
            @NonNull SoftApRole role,
            boolean verboseLoggingEnabled) {
        mContext = context;
        mFrameworkFacade = framework;
        mSoftApNotifier = softApNotifier;
        mWifiNative = wifiNative;
        mCoexManager = coexManager;
        mCountryCode = countryCode;
        mModeListener = listener;
        mSoftApCallback = callback;
        mWifiApConfigStore = wifiApConfigStore;
        mCurrentSoftApConfiguration = apConfig.getSoftApConfiguration();
        mCurrentSoftApCapability = apConfig.getCapability();
        // null is a valid input and means we use the user-configured tethering settings.
        if (mCurrentSoftApConfiguration == null) {
            mCurrentSoftApConfiguration = mWifiApConfigStore.getApConfiguration();
            // may still be null if we fail to load the default config
        }
        // Store mode configuration before update the configuration.
        mOriginalModeConfiguration = new SoftApModeConfiguration(apConfig.getTargetMode(),
                mCurrentSoftApConfiguration, mCurrentSoftApCapability);
        if (mCurrentSoftApConfiguration != null) {
            mIsUnsetBssid = mCurrentSoftApConfiguration.getBssid() == null;
            if (mCurrentSoftApCapability.areFeaturesSupported(
                    SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION)) {
                mCurrentSoftApConfiguration = mWifiApConfigStore.randomizeBssidIfUnset(
                        mContext, mCurrentSoftApConfiguration);
            }
        }
        mWifiMetrics = wifiMetrics;
        mSarManager = sarManager;
        mWifiDiagnostics = wifiDiagnostics;
        mStateMachine = new SoftApStateMachine(looper);
        configureInternalConfiguration();
        mDefaultShutdownTimeoutMillis = mContext.getResources().getInteger(
                R.integer.config_wifiFrameworkSoftApShutDownTimeoutMilliseconds);
        mDefaultShutdownIdleInstanceInBridgedModeTimeoutMillis = mContext.getResources().getInteger(
                R.integer
                .config_wifiFrameworkSoftApShutDownIdleInstanceInBridgedModeTimeoutMillisecond);
        mCmiMonitor = cmiMonitor;
        mActiveModeWarden = activeModeWarden;
        mCmiMonitor.registerListener(new ClientModeImplListener() {
            @Override
            public void onL2Connected(@NonNull ConcreteClientModeManager clientModeManager) {
                SoftApManager.this.onL2Connected(clientModeManager);
            }
        });
        updateSafeChannelFrequencyList();
        mId = id;
        mRole = role;
        enableVerboseLogging(verboseLoggingEnabled);
        mStateMachine.sendMessage(SoftApStateMachine.CMD_START, requestorWs);
    }

    @Override
    public long getId() {
        return mId;
    }

    private String getTag() {
        return TAG + "[" + (mApInterfaceName == null ? "unknown" : mApInterfaceName) + "]";
    }

    /**
     * Stop soft AP.
     */
    @Override
    public void stop() {
        Log.d(getTag(), " currentstate: " + getCurrentStateName());
        mStateMachine.sendMessage(SoftApStateMachine.CMD_STOP);
    }

    private boolean isBridgedMode() {
        return (SdkLevel.isAtLeastS() && mCurrentSoftApConfiguration != null
                && mCurrentSoftApConfiguration.getBands().length > 1);
    }

    private long getShutdownTimeoutMillis() {
        long timeout = mCurrentSoftApConfiguration.getShutdownTimeoutMillis();
        return timeout > 0 ? timeout : mDefaultShutdownTimeoutMillis;
    }

    @Override
    @Nullable public SoftApRole getRole() {
        return mRole;
    }

    @Override
    @Nullable public ClientRole getPreviousRole() {
        return null;
    }

    @Override
    public long getLastRoleChangeSinceBootMs() {
        return 0;
    }

    /** Set the role of this SoftApManager */
    public void setRole(SoftApRole role) {
        // softap does not allow in-place switching of roles.
        Preconditions.checkState(mRole == null);
        mRole = role;
    }

    @Override
    public String getInterfaceName() {
        return mApInterfaceName;
    }

    @Override
    public WorkSource getRequestorWs() {
        return mRequestorWs;
    }

    /**
     * Update AP capability. Called when carrier config or device resouce config changed.
     *
     * @param capability new AP capability.
     */
    public void updateCapability(@NonNull SoftApCapability capability) {
        mStateMachine.sendMessage(SoftApStateMachine.CMD_UPDATE_CAPABILITY, capability);
    }

    /**
     * Update AP configuration. Called when setting update config via
     * {@link WifiManager#setSoftApConfiguration(SoftApConfiguration)}
     *
     * @param config new AP config.
     */
    public void updateConfiguration(@NonNull SoftApConfiguration config) {
        mStateMachine.sendMessage(SoftApStateMachine.CMD_UPDATE_CONFIG, config);
    }

    /**
     * Retrieve the {@link SoftApModeConfiguration} instance associated with this mode manager.
     */
    public SoftApModeConfiguration getSoftApModeConfiguration() {
        return new SoftApModeConfiguration(mOriginalModeConfiguration.getTargetMode(),
                mCurrentSoftApConfiguration, mCurrentSoftApCapability);
    }

    /**
     * Dump info about this softap manager.
     */
    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of SoftApManager id=" + mId);

        pw.println("current StateMachine mode: " + getCurrentStateName());
        pw.println("mRole: " + mRole);
        pw.println("mApInterfaceName: " + mApInterfaceName);
        pw.println("mIfaceIsUp: " + mIfaceIsUp);
        pw.println("mSoftApCountryCode: " + mCountryCode);
        pw.println("mOriginalModeConfiguration.targetMode: "
                + mOriginalModeConfiguration.getTargetMode());
        pw.println("mCurrentSoftApConfiguration.SSID: " + mCurrentSoftApConfiguration.getSsid());
        pw.println("mCurrentSoftApConfiguration.mBand: " + mCurrentSoftApConfiguration.getBand());
        pw.println("mCurrentSoftApConfiguration.hiddenSSID: "
                + mCurrentSoftApConfiguration.isHiddenSsid());
        pw.println("getConnectedClientList().size(): " + getConnectedClientList().size());
        pw.println("mTimeoutEnabled: " + mTimeoutEnabled);
        pw.println("mBridgedModeOpportunisticsShutdownTimeoutEnabled: "
                + mBridgedModeOpportunisticsShutdownTimeoutEnabled);
        pw.println("mCurrentSoftApInfoMap " + mCurrentSoftApInfoMap);
        pw.println("mStartTimestamp: " + mStartTimestamp);
        mStateMachine.dump(fd, pw, args);
    }

    @Override
    public void enableVerboseLogging(boolean verbose) {
        mVerboseLoggingEnabled = verbose;
    }

    @Override
    public String toString() {
        return "SoftApManager{id=" + getId()
                + " iface=" + getInterfaceName()
                + " role=" + getRole()
                + "}";
    }

    /**
     * A ClientModeImpl instance has been L2 connected.
     *
     * @param newPrimary the corresponding ConcreteClientModeManager instance for the ClientModeImpl
     *                   that has been L2 connected.
     */
    private void onL2Connected(@NonNull ConcreteClientModeManager clientModeManager) {
        Log.d(getTag(), "onL2Connected called");
        mStateMachine.sendMessage(SoftApStateMachine.CMD_HANDLE_WIFI_CONNECTED,
                clientModeManager.syncRequestConnectionInfo());
    }


    private String getCurrentStateName() {
        IState currentState = mStateMachine.getCurrentState();

        if (currentState != null) {
            return currentState.getName();
        }

        return "StateMachine not active";
    }

    /**
     * Update AP state.
     *
     * @param newState     new AP state
     * @param currentState current AP state
     * @param reason       Failure reason if the new AP state is in failure state
     */
    private void updateApState(int newState, int currentState, int reason) {
        mSoftApCallback.onStateChanged(newState, reason);

        //send the AP state change broadcast
        final Intent intent = new Intent(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_WIFI_AP_STATE, newState);
        intent.putExtra(WifiManager.EXTRA_PREVIOUS_WIFI_AP_STATE, currentState);
        if (newState == WifiManager.WIFI_AP_STATE_FAILED) {
            //only set reason number when softAP start failed
            intent.putExtra(WifiManager.EXTRA_WIFI_AP_FAILURE_REASON, reason);
        }

        intent.putExtra(WifiManager.EXTRA_WIFI_AP_INTERFACE_NAME, mApInterfaceName);
        intent.putExtra(WifiManager.EXTRA_WIFI_AP_MODE, mOriginalModeConfiguration.getTargetMode());
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private int setMacAddress() {
        MacAddress mac = mCurrentSoftApConfiguration.getBssid();

        if (mac == null) {
            // If no BSSID is explicitly requested, (re-)configure the factory MAC address. Some
            // drivers may not support setting the MAC at all, so fail soft in this case.
            if (!mWifiNative.resetApMacToFactoryMacAddress(mApInterfaceName)) {
                Log.w(getTag(), "failed to reset to factory MAC address; "
                        + "continuing with current MAC");
            }
        } else {
            if (mWifiNative.isApSetMacAddressSupported(mApInterfaceName)) {
                if (!mWifiNative.setApMacAddress(mApInterfaceName, mac)) {
                    Log.e(getTag(), "failed to set explicitly requested MAC address");
                    return ERROR_GENERIC;
                }
            } else if (!mIsUnsetBssid) {
                // If hardware does not support MAC address setter,
                // only report the error for non randomization.
                return ERROR_UNSUPPORTED_CONFIGURATION;
            }
        }

        return SUCCESS;
    }

    /**
     * Dynamic update the country code when Soft AP enabled.
     *
     * @param countryCode 2 byte ASCII string. For ex: US, CA.
     * @return true if request is sent successfully, false otherwise.
     */
    public boolean updateCountryCode(@NonNull String countryCode) {
        if (ApConfigUtil.isSoftApDynamicCountryCodeSupported(mContext)
                && mCurrentSoftApCapability.areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD)) {
            mStateMachine.sendMessage(SoftApStateMachine.CMD_UPDATE_COUNTRY_CODE, countryCode);
            return true;
        }
        return false;
    }

    private int setCountryCode() {
        int band = mCurrentSoftApConfiguration.getBand();
        if (TextUtils.isEmpty(mCountryCode)) {
            if (band == SoftApConfiguration.BAND_5GHZ) {
                // Country code is mandatory for 5GHz band.
                Log.e(getTag(), "Invalid country code, required for setting up soft ap in 5GHz");
                return ERROR_GENERIC;
            }
            // Absence of country code is not fatal for 2Ghz & Any band options.
            return SUCCESS;
        }
        if (!mWifiNative.setApCountryCode(
                mApInterfaceName, mCountryCode.toUpperCase(Locale.ROOT))) {
            if (band == SoftApConfiguration.BAND_5GHZ) {
                // Return an error if failed to set country code when AP is configured for
                // 5GHz band.
                Log.e(getTag(), "Failed to set country code, "
                        + "required for setting up soft ap in 5GHz");
                return ERROR_GENERIC;
            }
            // Failure to set country code is not fatal for other band options.
        }
        return SUCCESS;
    }

    /**
     * Start a soft AP instance as configured.
     *
     * @return integer result code
     */
    private int startSoftAp() {
        Log.d(getTag(), "band " + mCurrentSoftApConfiguration.getBand() + " iface "
                + mApInterfaceName + " country " + mCountryCode);

        int result = setMacAddress();
        if (result != SUCCESS) {
            return result;
        }

        result = setCountryCode();
        if (result != SUCCESS) {
            return result;
        }

        // Make a copy of configuration for updating AP band and channel.
        SoftApConfiguration.Builder localConfigBuilder =
                new SoftApConfiguration.Builder(mCurrentSoftApConfiguration);

        boolean acsEnabled = mCurrentSoftApCapability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD);

        result = ApConfigUtil.updateApChannelConfig(
                mWifiNative, mCoexManager, mContext.getResources(), mCountryCode,
                localConfigBuilder, mCurrentSoftApConfiguration, acsEnabled);
        if (result != SUCCESS) {
            Log.e(getTag(), "Failed to update AP band and channel");
            return result;
        }

        if (mCurrentSoftApConfiguration.isHiddenSsid()) {
            Log.d(getTag(), "SoftAP is a hidden network");
        }

        if (!ApConfigUtil.checkSupportAllConfiguration(
                mCurrentSoftApConfiguration, mCurrentSoftApCapability)) {
            Log.d(getTag(), "Unsupported Configuration detect! config = "
                    + mCurrentSoftApConfiguration);
            return ERROR_UNSUPPORTED_CONFIGURATION;
        }

        if (!mWifiNative.startSoftAp(mApInterfaceName,
                  localConfigBuilder.build(),
                  mOriginalModeConfiguration.getTargetMode() ==  WifiManager.IFACE_IP_MODE_TETHERED,
                  mSoftApListener)) {
            Log.e(getTag(), "Soft AP start failed");
            return ERROR_GENERIC;
        }

        mWifiDiagnostics.startLogging(mApInterfaceName);
        mStartTimestamp = FORMATTER.format(new Date(System.currentTimeMillis()));
        Log.d(getTag(), "Soft AP is started ");

        return SUCCESS;
    }

    /**
     * Disconnect all connected clients on active softap interface(s).
     * This is usually done just before stopSoftAp().
     */
    private void disconnectAllClients() {
        for (WifiClient client : getConnectedClientList()) {
            mWifiNative.forceClientDisconnect(mApInterfaceName, client.getMacAddress(),
                    SAP_CLIENT_DISCONNECT_REASON_CODE_UNSPECIFIED);
        }
    }

    /**
     * Teardown soft AP and teardown the interface.
     */
    private void stopSoftAp() {
        disconnectAllClients();
        mWifiDiagnostics.stopLogging(mApInterfaceName);
        mWifiNative.teardownInterface(mApInterfaceName);
        Log.d(getTag(), "Soft AP is stopped");
    }

    private void addClientToPendingDisconnectionList(WifiClient client, int reason) {
        Log.d(getTag(), "Fail to disconnect client: " + client.getMacAddress()
                + ", add it into pending list");
        mPendingDisconnectClients.put(client, reason);
        mStateMachine.getHandler().removeMessages(
                SoftApStateMachine.CMD_FORCE_DISCONNECT_PENDING_CLIENTS);
        mStateMachine.sendMessageDelayed(
                SoftApStateMachine.CMD_FORCE_DISCONNECT_PENDING_CLIENTS,
                SOFT_AP_PENDING_DISCONNECTION_CHECK_DELAY_MS);
    }

    private List<WifiClient> getConnectedClientList() {
        List<WifiClient> connectedClientList = new ArrayList<>();
        for (List<WifiClient> it : mConnectedClientWithApInfoMap.values()) {
            connectedClientList.addAll(it);
        }
        return connectedClientList;
    }

    private boolean checkSoftApClient(SoftApConfiguration config, WifiClient newClient) {
        if (!mCurrentSoftApCapability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT)) {
            return true;
        }

        if (mBlockedClientList.contains(newClient.getMacAddress())) {
            Log.d(getTag(), "Force disconnect for client: " + newClient + "in blocked list");
            if (!mWifiNative.forceClientDisconnect(
                    mApInterfaceName, newClient.getMacAddress(),
                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER)) {
                addClientToPendingDisconnectionList(newClient,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
            }
            return false;
        }
        if (config.isClientControlByUserEnabled()
                && !mAllowedClientList.contains(newClient.getMacAddress())) {
            mSoftApCallback.onBlockedClientConnecting(newClient,
                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
            Log.d(getTag(), "Force disconnect for unauthorized client: " + newClient);
            if (!mWifiNative.forceClientDisconnect(
                    mApInterfaceName, newClient.getMacAddress(),
                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER)) {
                addClientToPendingDisconnectionList(newClient,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
            }
            return false;
        }
        int maxConfig = mCurrentSoftApCapability.getMaxSupportedClients();
        if (config.getMaxNumberOfClients() > 0) {
            maxConfig = Math.min(maxConfig, config.getMaxNumberOfClients());
        }

        if (getConnectedClientList().size() >= maxConfig) {
            Log.i(getTag(), "No more room for new client:" + newClient);
            if (!mWifiNative.forceClientDisconnect(
                    mApInterfaceName, newClient.getMacAddress(),
                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS)) {
                addClientToPendingDisconnectionList(newClient,
                        WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
            }
            mSoftApCallback.onBlockedClientConnecting(newClient,
                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
            // Avoid report the max client blocked in the same settings.
            if (!mEverReportMetricsForMaxClient) {
                mWifiMetrics.noteSoftApClientBlocked(maxConfig);
                mEverReportMetricsForMaxClient = true;
            }
            return false;
        }
        return true;
    }

    private class SoftApStateMachine extends StateMachine {
        // Commands for the state machine.
        public static final int CMD_START = 0;
        public static final int CMD_STOP = 1;
        public static final int CMD_FAILURE = 2;
        public static final int CMD_INTERFACE_STATUS_CHANGED = 3;
        public static final int CMD_ASSOCIATED_STATIONS_CHANGED = 4;
        public static final int CMD_NO_ASSOCIATED_STATIONS_TIMEOUT = 5;
        public static final int CMD_INTERFACE_DESTROYED = 7;
        public static final int CMD_INTERFACE_DOWN = 8;
        public static final int CMD_AP_INFO_CHANGED = 9;
        public static final int CMD_UPDATE_CAPABILITY = 10;
        public static final int CMD_UPDATE_CONFIG = 11;
        public static final int CMD_FORCE_DISCONNECT_PENDING_CLIENTS = 12;
        public static final int CMD_NO_ASSOCIATED_STATIONS_TIMEOUT_ON_ONE_INSTANCE = 13;
        public static final int CMD_SAFE_CHANNEL_FREQUENCY_CHANGED = 14;
        public static final int CMD_HANDLE_WIFI_CONNECTED = 15;
        public static final int CMD_UPDATE_COUNTRY_CODE = 16;

        private final State mIdleState = new IdleState();
        private final State mStartedState = new StartedState();

        private final InterfaceCallback mWifiNativeInterfaceCallback = new InterfaceCallback() {
            @Override
            public void onDestroyed(String ifaceName) {
                if (mApInterfaceName != null && mApInterfaceName.equals(ifaceName)) {
                    sendMessage(CMD_INTERFACE_DESTROYED);
                }
            }

            @Override
            public void onUp(String ifaceName) {
                if (mApInterfaceName != null && mApInterfaceName.equals(ifaceName)) {
                    sendMessage(CMD_INTERFACE_STATUS_CHANGED, 1);
                }
            }

            @Override
            public void onDown(String ifaceName) {
                if (mApInterfaceName != null && mApInterfaceName.equals(ifaceName)) {
                    sendMessage(CMD_INTERFACE_STATUS_CHANGED, 0);
                }
            }
        };

        SoftApStateMachine(Looper looper) {
            super(TAG, looper);

            // CHECKSTYLE:OFF IndentationCheck
            addState(mIdleState);
                addState(mStartedState, mIdleState);
            // CHECKSTYLE:ON IndentationCheck

            setInitialState(mIdleState);
            start();
        }

        private class IdleState extends State {
            @Override
            public void enter() {
                mApInterfaceName = null;
                mIfaceIsUp = false;
                mIfaceIsDestroyed = false;
            }

            @Override
            public void exit() {
                mModeListener.onStopped(SoftApManager.this);
            }

            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_STOP:
                        mStateMachine.quitNow();
                        break;
                    case CMD_START:
                        mRequestorWs = (WorkSource) message.obj;
                        if (mCurrentSoftApConfiguration == null
                                || mCurrentSoftApConfiguration.getSsid() == null) {
                            Log.e(getTag(), "Unable to start soft AP without valid configuration");
                            updateApState(WifiManager.WIFI_AP_STATE_FAILED,
                                    WifiManager.WIFI_AP_STATE_DISABLED,
                                    WifiManager.SAP_START_FAILURE_GENERAL);
                            mWifiMetrics.incrementSoftApStartResult(
                                    false, WifiManager.SAP_START_FAILURE_GENERAL);
                            mModeListener.onStartFailure(SoftApManager.this);
                            break;
                        }
                        if (isBridgedMode()) {
                            boolean isFallbackToSingleAp = false;
                            int newSingleApBand = 0;
                            for (ClientModeManager cmm
                                    : mActiveModeWarden.getClientModeManagers()) {
                                WifiInfo wifiConnectedInfo = cmm.syncRequestConnectionInfo();
                                int wifiFrequency = wifiConnectedInfo.getFrequency();
                                if (wifiFrequency > 0
                                        && !mSafeChannelFrequencyList.contains(
                                        wifiFrequency)) {
                                    Log.d(getTag(), "Wifi connected to unavailable freq: "
                                            + wifiFrequency);
                                    isFallbackToSingleAp = true;
                                    break;
                                }
                            }
                            for (int configuredBand : mCurrentSoftApConfiguration.getBands()) {
                                int availableBand = ApConfigUtil.removeUnavailableBands(
                                        mCurrentSoftApCapability,
                                        configuredBand, mCoexManager);
                                if (configuredBand != availableBand) {
                                    isFallbackToSingleAp = true;
                                }
                                newSingleApBand |= availableBand;
                            }
                            if (isFallbackToSingleAp) {
                                newSingleApBand = ApConfigUtil.append24GToBandIf24GSupported(
                                        newSingleApBand, mContext);
                                Log.i(getTag(), "Fallback to single AP mode with band "
                                        + newSingleApBand);
                                mCurrentSoftApConfiguration =
                                        new SoftApConfiguration.Builder(mCurrentSoftApConfiguration)
                                        .setBand(newSingleApBand)
                                        .build();
                            }
                        }
                        mApInterfaceName = mWifiNative.setupInterfaceForSoftApMode(
                                mWifiNativeInterfaceCallback, mRequestorWs,
                                mCurrentSoftApConfiguration.getBand(), isBridgedMode());
                        if (TextUtils.isEmpty(mApInterfaceName)) {
                            Log.e(getTag(), "setup failure when creating ap interface.");
                            updateApState(WifiManager.WIFI_AP_STATE_FAILED,
                                    WifiManager.WIFI_AP_STATE_DISABLED,
                                    WifiManager.SAP_START_FAILURE_GENERAL);
                            mWifiMetrics.incrementSoftApStartResult(
                                    false, WifiManager.SAP_START_FAILURE_GENERAL);
                            mModeListener.onStartFailure(SoftApManager.this);
                            break;
                        }
                        mSoftApNotifier.dismissSoftApShutdownTimeoutExpiredNotification();
                        updateApState(WifiManager.WIFI_AP_STATE_ENABLING,
                                WifiManager.WIFI_AP_STATE_DISABLED, 0);
                        int result = startSoftAp();
                        if (result != SUCCESS) {
                            int failureReason = WifiManager.SAP_START_FAILURE_GENERAL;
                            if (result == ERROR_NO_CHANNEL) {
                                failureReason = WifiManager.SAP_START_FAILURE_NO_CHANNEL;
                            } else if (result == ERROR_UNSUPPORTED_CONFIGURATION) {
                                failureReason = WifiManager
                                        .SAP_START_FAILURE_UNSUPPORTED_CONFIGURATION;
                            }
                            updateApState(WifiManager.WIFI_AP_STATE_FAILED,
                                    WifiManager.WIFI_AP_STATE_ENABLING,
                                    failureReason);
                            stopSoftAp();
                            mWifiMetrics.incrementSoftApStartResult(false, failureReason);
                            mModeListener.onStartFailure(SoftApManager.this);
                            break;
                        }
                        transitionTo(mStartedState);
                        break;
                    case CMD_UPDATE_CAPABILITY:
                        // Capability should only changed by carrier requirement. Only apply to
                        // Tether Mode
                        if (mOriginalModeConfiguration.getTargetMode()
                                ==  WifiManager.IFACE_IP_MODE_TETHERED) {
                            SoftApCapability capability = (SoftApCapability) message.obj;
                            mCurrentSoftApCapability = new SoftApCapability(capability);
                        }
                        break;
                    case CMD_UPDATE_CONFIG:
                        SoftApConfiguration newConfig = (SoftApConfiguration) message.obj;
                        Log.d(getTag(), "Configuration changed to " + newConfig);
                        // Idle mode, update all configurations.
                        mCurrentSoftApConfiguration = newConfig;
                        configureInternalConfiguration();
                        break;
                    case CMD_UPDATE_COUNTRY_CODE:
                        String countryCode = (String) message.obj;
                        if (!TextUtils.isEmpty(countryCode)) {
                            mCountryCode = countryCode;
                        }
                        break;
                    default:
                        // Ignore all other commands.
                        break;
                }

                return HANDLED;
            }
        }

        private class StartedState extends State {
            private void scheduleTimeoutMessages() {
                // When SAP started, the mCurrentSoftApInfoMap is 0 because info does not update.
                // Don't trigger bridged mode shutdown timeout when only one active instance
                // In Dual AP, one instance may already be closed due to LTE coexistence or DFS
                // restrictions or due to inactivity. i.e. mCurrentSoftApInfoMap.size() is 1)
                final int connectedClients = getConnectedClientList().size();
                if (isBridgedMode() && mCurrentSoftApInfoMap.size() != 1) {
                    if (mBridgedModeOpportunisticsShutdownTimeoutEnabled
                            && (connectedClients == 0 || getIdleInstances().size() != 0)) {
                        if (!mIsBridgedModeIdleInstanceTimerActive) {
                            mSoftApBridgedModeIdleInstanceTimeoutMessage.schedule(SystemClock
                                    .elapsedRealtime()
                                    + mDefaultShutdownIdleInstanceInBridgedModeTimeoutMillis);
                            mIsBridgedModeIdleInstanceTimerActive = true;
                            Log.d(getTag(), "Bridged mode instance opportunistic timeout message"
                                    + " scheduled, delay = "
                                    + mDefaultShutdownIdleInstanceInBridgedModeTimeoutMillis);
                        }
                    } else {
                        cancelBridgedModeIdleInstanceTimeoutMessage();
                    }
                }
                if (!mTimeoutEnabled || connectedClients != 0) {
                    cancelTimeoutMessage();
                    return;
                }
                long timeout = getShutdownTimeoutMillis();
                mSoftApTimeoutMessage.schedule(SystemClock.elapsedRealtime()
                        + timeout);
                Log.d(getTag(), "Timeout message scheduled, delay = "
                        + timeout);
            }

            private String getHighestFrequencyInstance(Set<String> candidateInstances) {
                int currentHighestFrequencyOnAP = 0;
                String highestFrequencyInstance = null;
                for (String instance : candidateInstances) {
                    SoftApInfo info = mCurrentSoftApInfoMap.get(instance);
                    if (info == null) {
                        Log.wtf(getTag(), "Invalid instance name, no way to get the frequency");
                        return "";
                    }
                    int frequencyOnInstance = info.getFrequency();
                    if (frequencyOnInstance > currentHighestFrequencyOnAP) {
                        currentHighestFrequencyOnAP = frequencyOnInstance;
                        highestFrequencyInstance = instance;
                    }
                }
                return highestFrequencyInstance;
            }

            private void removeIfaceInstanceFromBridgedApIface(String instanceName) {
                if (TextUtils.isEmpty(instanceName)) {
                    return;
                }
                if (mCurrentSoftApInfoMap.containsKey(instanceName)) {
                    Log.i(getTag(), "remove instance " + instanceName + "("
                            + mCurrentSoftApInfoMap.get(instanceName).getFrequency()
                            + ") from bridged iface " + mApInterfaceName);
                    mWifiNative.removeIfaceInstanceFromBridgedApIface(mApInterfaceName,
                            instanceName);
                    // Remove the info and update it.
                    updateSoftApInfo(mCurrentSoftApInfoMap.get(instanceName), true);
                }
            }

            private Set<String> getIdleInstances() {
                Set<String> idleInstances = new HashSet<String>();
                for (String instance : mConnectedClientWithApInfoMap.keySet()) {
                    if (mConnectedClientWithApInfoMap.getOrDefault(
                            instance, Collections.emptyList()).size() == 0) {
                        idleInstances.add(instance);
                    }
                }
                return idleInstances;
            }

            private void cancelTimeoutMessage() {
                mSoftApTimeoutMessage.cancel();
                Log.d(getTag(), "Timeout message canceled");
            }

            private void cancelBridgedModeIdleInstanceTimeoutMessage() {
                mSoftApBridgedModeIdleInstanceTimeoutMessage.cancel();
                mIsBridgedModeIdleInstanceTimerActive = false;
                Log.d(getTag(), "Bridged mode idle instance timeout message canceled");
            }

            /**
             * When configuration changed, it need to force some clients disconnect to match the
             * configuration.
             */
            private void updateClientConnection() {
                if (!mCurrentSoftApCapability.areFeaturesSupported(
                        SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT)) {
                    return;
                }
                final int maxAllowedClientsByHardwareAndCarrier =
                        mCurrentSoftApCapability.getMaxSupportedClients();
                final int userApConfigMaxClientCount =
                        mCurrentSoftApConfiguration.getMaxNumberOfClients();
                int finalMaxClientCount = maxAllowedClientsByHardwareAndCarrier;
                if (userApConfigMaxClientCount > 0) {
                    finalMaxClientCount = Math.min(userApConfigMaxClientCount,
                            maxAllowedClientsByHardwareAndCarrier);
                }
                List<WifiClient> currentClients = getConnectedClientList();
                int targetDisconnectClientNumber = currentClients.size() - finalMaxClientCount;
                List<WifiClient> allowedConnectedList = new ArrayList<>();
                Iterator<WifiClient> iterator = currentClients.iterator();
                while (iterator.hasNext()) {
                    WifiClient client = iterator.next();
                    if (mBlockedClientList.contains(client.getMacAddress())
                              || (mCurrentSoftApConfiguration.isClientControlByUserEnabled()
                              && !mAllowedClientList.contains(client.getMacAddress()))) {
                        Log.d(getTag(), "Force disconnect for not allowed client: " + client);
                        if (!mWifiNative.forceClientDisconnect(
                                mApInterfaceName, client.getMacAddress(),
                                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER)) {
                            addClientToPendingDisconnectionList(client,
                                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_BLOCKED_BY_USER);
                        }
                        targetDisconnectClientNumber--;
                    } else {
                        allowedConnectedList.add(client);
                    }
                }

                if (targetDisconnectClientNumber > 0) {
                    Iterator<WifiClient> allowedClientIterator = allowedConnectedList.iterator();
                    while (allowedClientIterator.hasNext()) {
                        if (targetDisconnectClientNumber == 0) break;
                        WifiClient allowedClient = allowedClientIterator.next();
                        Log.d(getTag(), "Force disconnect for client due to no more room: "
                                + allowedClient);
                        if (!mWifiNative.forceClientDisconnect(
                                mApInterfaceName, allowedClient.getMacAddress(),
                                WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS)) {
                            addClientToPendingDisconnectionList(allowedClient,
                                    WifiManager.SAP_CLIENT_BLOCK_REASON_CODE_NO_MORE_STAS);
                        }
                        targetDisconnectClientNumber--;
                    }
                }
            }

            /**
             * Set stations associated with this soft AP
             * @param client The station for which connection state changed.
             * @param isConnected True for the connection changed to connect, otherwise false.
             */
            private void updateConnectedClients(WifiClient client, boolean isConnected) {
                if (client == null) {
                    return;
                }

                if (null != mPendingDisconnectClients.remove(client)) {
                    Log.d(getTag(), "Remove client: " + client.getMacAddress()
                            + "from pending disconnectionlist");
                }

                String apInstanceIdentifier = client.getApInstanceIdentifier();
                List clientList = mConnectedClientWithApInfoMap.computeIfAbsent(
                        apInstanceIdentifier, k -> new ArrayList<>());
                int index = clientList.indexOf(client);

                if ((index != -1) == isConnected) {
                    Log.e(getTag(), "Drop client connection event, client "
                            + client + "isConnected: " + isConnected
                            + " , duplicate event or client is blocked");
                    return;
                }
                if (isConnected) {
                    boolean isAllow = checkSoftApClient(mCurrentSoftApConfiguration, client);
                    if (isAllow) {
                        clientList.add(client);
                    } else {
                        return;
                    }
                } else {
                    if (null == clientList.remove(index)) {
                        Log.e(getTag(), "client doesn't exist in list, it should NOT happen");
                    }
                }

                // Update clients list.
                mConnectedClientWithApInfoMap.put(apInstanceIdentifier, clientList);
                SoftApInfo currentInfoWithClientsChanged = mCurrentSoftApInfoMap
                        .get(apInstanceIdentifier);
                Log.d(getTag(), "The connected wifi stations have changed with count: "
                        + clientList.size() + ": " + clientList + " on the AP which info is "
                        + currentInfoWithClientsChanged);

                if (mSoftApCallback != null) {
                    mSoftApCallback.onConnectedClientsOrInfoChanged(mCurrentSoftApInfoMap,
                            mConnectedClientWithApInfoMap, isBridgedMode());
                } else {
                    Log.e(getTag(),
                            "SoftApCallback is null. Dropping ConnectedClientsChanged event.");
                }

                mWifiMetrics.addSoftApNumAssociatedStationsChangedEvent(
                        getConnectedClientList().size(),
                        mConnectedClientWithApInfoMap.get(apInstanceIdentifier).size(),
                        mOriginalModeConfiguration.getTargetMode(),
                        mCurrentSoftApInfoMap.get(apInstanceIdentifier));

                scheduleTimeoutMessages();
            }

            /**
             * @param apInfo, the new SoftApInfo changed. Null used to clean up.
             */
            private void updateSoftApInfo(@Nullable SoftApInfo apInfo, boolean isRemoved) {
                Log.d(getTag(), "SoftApInfo update " + apInfo + ", isRemoved: " + isRemoved);
                if (apInfo == null) {
                    // Clean up
                    mCurrentSoftApInfoMap.clear();
                    mConnectedClientWithApInfoMap.clear();
                    mSoftApCallback.onConnectedClientsOrInfoChanged(mCurrentSoftApInfoMap,
                            mConnectedClientWithApInfoMap, isBridgedMode());
                    return;
                }
                String changedInstance = apInfo.getApInstanceIdentifier();
                if (apInfo.equals(mCurrentSoftApInfoMap.get(changedInstance))) {
                    if (isRemoved) {
                        boolean isClientConnected =
                                mConnectedClientWithApInfoMap.get(changedInstance).size() > 0;
                        mCurrentSoftApInfoMap.remove(changedInstance);
                        mConnectedClientWithApInfoMap.remove(changedInstance);
                        mSoftApCallback.onConnectedClientsOrInfoChanged(mCurrentSoftApInfoMap,
                                mConnectedClientWithApInfoMap, isBridgedMode());
                        if (isClientConnected) {
                            mWifiMetrics.addSoftApNumAssociatedStationsChangedEvent(
                                    getConnectedClientList().size(), 0,
                                    mOriginalModeConfiguration.getTargetMode(), apInfo);
                        }
                        if (isBridgedMode()) {
                            mWifiMetrics.addSoftApInstanceDownEventInDualMode(
                                    mOriginalModeConfiguration.getTargetMode(), apInfo);
                        }
                    }
                    return;
                }

                // Make sure an empty client list is created when info updated
                List clientList = mConnectedClientWithApInfoMap.computeIfAbsent(
                        changedInstance, k -> new ArrayList<>());

                if (clientList.size() != 0) {
                    Log.e(getTag(), "The info: " + apInfo
                            + " changed when client connected, it should NOT happen!!");
                }

                // Update the info when getting two infos in bridged mode.
                // TODO: b/173999527. It may only one instance come up when starting bridged AP.
                // Consider the handling with co-ex mechanism in bridged mode.
                boolean waitForAnotherSoftApInfoInBridgedMode =
                        isBridgedMode() && mCurrentSoftApInfoMap.size() == 0;

                mCurrentSoftApInfoMap.put(changedInstance, new SoftApInfo(apInfo));
                if (!waitForAnotherSoftApInfoInBridgedMode) {
                    mSoftApCallback.onConnectedClientsOrInfoChanged(mCurrentSoftApInfoMap,
                            mConnectedClientWithApInfoMap, isBridgedMode());
                }

                // ignore invalid freq and softap disable case for metrics
                if (apInfo.getFrequency() > 0
                        && apInfo.getBandwidth() != SoftApInfo.CHANNEL_WIDTH_INVALID) {
                    mWifiMetrics.addSoftApChannelSwitchedEvent(
                            new ArrayList<>(mCurrentSoftApInfoMap.values()),
                            mOriginalModeConfiguration.getTargetMode(), isBridgedMode());
                    updateUserBandPreferenceViolationMetricsIfNeeded(apInfo);
                }
            }

            private void onUpChanged(boolean isUp) {
                if (isUp == mIfaceIsUp) {
                    return;  // no change
                }

                mIfaceIsUp = isUp;
                if (isUp) {
                    Log.d(getTag(), "SoftAp is ready for use");
                    updateApState(WifiManager.WIFI_AP_STATE_ENABLED,
                            WifiManager.WIFI_AP_STATE_ENABLING, 0);
                    mModeListener.onStarted(SoftApManager.this);
                    mWifiMetrics.incrementSoftApStartResult(true, 0);
                    mCurrentSoftApInfoMap.clear();
                    mConnectedClientWithApInfoMap.clear();
                    if (mSoftApCallback != null) {
                        mSoftApCallback.onConnectedClientsOrInfoChanged(mCurrentSoftApInfoMap,
                                mConnectedClientWithApInfoMap, isBridgedMode());
                    }
                } else {
                    // the interface was up, but goes down
                    sendMessage(CMD_INTERFACE_DOWN);
                }
                mWifiMetrics.addSoftApUpChangedEvent(isUp,
                        mOriginalModeConfiguration.getTargetMode(),
                        mDefaultShutdownTimeoutMillis, isBridgedMode());
                if (isUp) {
                    mWifiMetrics.updateSoftApConfiguration(mCurrentSoftApConfiguration,
                            mOriginalModeConfiguration.getTargetMode(), isBridgedMode());
                    mWifiMetrics.updateSoftApCapability(mCurrentSoftApCapability,
                            mOriginalModeConfiguration.getTargetMode(), isBridgedMode());
                }
            }

            @Override
            public void enter() {
                mIfaceIsUp = false;
                mIfaceIsDestroyed = false;
                onUpChanged(mWifiNative.isInterfaceUp(mApInterfaceName));

                Handler handler = mStateMachine.getHandler();
                mSoftApTimeoutMessage = new WakeupMessage(mContext, handler,
                        SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG,
                        SoftApStateMachine.CMD_NO_ASSOCIATED_STATIONS_TIMEOUT);

                mSoftApBridgedModeIdleInstanceTimeoutMessage = new WakeupMessage(mContext, handler,
                        SOFT_AP_SEND_MESSAGE_IDLE_IN_BRIDGED_MODE_TIMEOUT_TAG,
                        SoftApStateMachine.CMD_NO_ASSOCIATED_STATIONS_TIMEOUT_ON_ONE_INSTANCE);
                if (SdkLevel.isAtLeastS()) {
                    mCoexManager.registerCoexListener(mCoexListener);
                }
                mSarManager.setSapWifiState(WifiManager.WIFI_AP_STATE_ENABLED);
                Log.d(getTag(), "Resetting connected clients on start");
                mConnectedClientWithApInfoMap.clear();
                mPendingDisconnectClients.clear();
                mEverReportMetricsForMaxClient = false;
                scheduleTimeoutMessages();
            }

            @Override
            public void exit() {
                if (!mIfaceIsDestroyed) {
                    stopSoftAp();
                }
                if (SdkLevel.isAtLeastS()) {
                    mCoexManager.unregisterCoexListener(mCoexListener);
                }
                if (getConnectedClientList().size() != 0) {
                    Log.d(getTag(), "Resetting num stations on stop");
                    for (List<WifiClient> it : mConnectedClientWithApInfoMap.values()) {
                        if (it.size() != 0) {
                            mWifiMetrics.addSoftApNumAssociatedStationsChangedEvent(
                                    0, 0, mOriginalModeConfiguration.getTargetMode(),
                                    mCurrentSoftApInfoMap
                                            .get(it.get(0).getApInstanceIdentifier()));
                        }
                    }
                    mConnectedClientWithApInfoMap.clear();
                    if (mSoftApCallback != null) {
                        mSoftApCallback.onConnectedClientsOrInfoChanged(mCurrentSoftApInfoMap,
                                mConnectedClientWithApInfoMap, isBridgedMode());
                    }
                }
                mPendingDisconnectClients.clear();
                cancelTimeoutMessage();
                cancelBridgedModeIdleInstanceTimeoutMessage();

                // Need this here since we are exiting |Started| state and won't handle any
                // future CMD_INTERFACE_STATUS_CHANGED events after this point
                mWifiMetrics.addSoftApUpChangedEvent(false,
                        mOriginalModeConfiguration.getTargetMode(),
                        mDefaultShutdownTimeoutMillis, isBridgedMode());
                updateApState(WifiManager.WIFI_AP_STATE_DISABLED,
                        WifiManager.WIFI_AP_STATE_DISABLING, 0);

                mSarManager.setSapWifiState(WifiManager.WIFI_AP_STATE_DISABLED);

                mApInterfaceName = null;
                mIfaceIsUp = false;
                mIfaceIsDestroyed = false;
                mRole = null;
                updateSoftApInfo(null, false);
            }

            private void updateUserBandPreferenceViolationMetricsIfNeeded(SoftApInfo apInfo) {
                // The band preference violation only need to detect in single AP mode.
                if (isBridgedMode()) return;
                int band = mCurrentSoftApConfiguration.getBand();
                boolean bandPreferenceViolated =
                        (ScanResult.is24GHz(apInfo.getFrequency())
                            && !ApConfigUtil.containsBand(band,
                                    SoftApConfiguration.BAND_2GHZ))
                        || (ScanResult.is5GHz(apInfo.getFrequency())
                            && !ApConfigUtil.containsBand(band,
                                    SoftApConfiguration.BAND_5GHZ))
                        || (ScanResult.is6GHz(apInfo.getFrequency())
                            && !ApConfigUtil.containsBand(band,
                                    SoftApConfiguration.BAND_6GHZ));

                if (bandPreferenceViolated) {
                    Log.e(getTag(), "Channel does not satisfy user band preference: "
                            + apInfo.getFrequency());
                    mWifiMetrics.incrementNumSoftApUserBandPreferenceUnsatisfied();
                }
            }

            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_ASSOCIATED_STATIONS_CHANGED:
                        if (!(message.obj instanceof WifiClient)) {
                            Log.e(getTag(), "Invalid type returned for"
                                    + " CMD_ASSOCIATED_STATIONS_CHANGED");
                            break;
                        }
                        boolean isConnected = (message.arg1 == 1);
                        WifiClient client = (WifiClient) message.obj;
                        Log.d(getTag(), "CMD_ASSOCIATED_STATIONS_CHANGED, Client: "
                                + client.getMacAddress().toString() + " isConnected: "
                                + isConnected);
                        updateConnectedClients(client, isConnected);
                        break;
                    case CMD_AP_INFO_CHANGED:
                        if (!(message.obj instanceof SoftApInfo)) {
                            Log.e(getTag(), "Invalid type returned for"
                                    + " CMD_AP_INFO_CHANGED");
                            break;
                        }
                        SoftApInfo apInfo = (SoftApInfo) message.obj;
                        if (apInfo.getFrequency() < 0) {
                            Log.e(getTag(), "Invalid ap channel frequency: "
                                    + apInfo.getFrequency());
                            break;
                        }
                        // Update shutdown timeout
                        apInfo.setAutoShutdownTimeoutMillis(mTimeoutEnabled
                                ? getShutdownTimeoutMillis() : 0);
                        updateSoftApInfo(apInfo, false);
                        break;
                    case CMD_INTERFACE_STATUS_CHANGED:
                        boolean isUp = message.arg1 == 1;
                        onUpChanged(isUp);
                        break;
                    case CMD_STOP:
                        if (mIfaceIsUp) {
                            updateApState(WifiManager.WIFI_AP_STATE_DISABLING,
                                    WifiManager.WIFI_AP_STATE_ENABLED, 0);
                        } else {
                            updateApState(WifiManager.WIFI_AP_STATE_DISABLING,
                                    WifiManager.WIFI_AP_STATE_ENABLING, 0);
                        }
                        quitNow();
                        break;
                    case CMD_START:
                        // Already started, ignore this command.
                        break;
                    case CMD_NO_ASSOCIATED_STATIONS_TIMEOUT:
                        if (!mTimeoutEnabled) {
                            Log.wtf(getTag(), "Timeout message received while timeout is disabled."
                                    + " Dropping.");
                            break;
                        }
                        if (getConnectedClientList().size() != 0) {
                            Log.wtf(getTag(), "Timeout message received but has clients. "
                                    + "Dropping.");
                            break;
                        }
                        mSoftApNotifier.showSoftApShutdownTimeoutExpiredNotification();
                        Log.i(getTag(), "Timeout message received. Stopping soft AP.");
                        updateApState(WifiManager.WIFI_AP_STATE_DISABLING,
                                WifiManager.WIFI_AP_STATE_ENABLED, 0);
                        quitNow();
                        break;
                    case CMD_NO_ASSOCIATED_STATIONS_TIMEOUT_ON_ONE_INSTANCE:
                        if (!isBridgedMode() || mCurrentSoftApInfoMap.size() != 2) {
                            Log.wtf(getTag(), "Ignore Bridged Mode Timeout message received"
                                    + " in single AP state. Dropping");
                            break;
                        }
                        if (!mBridgedModeOpportunisticsShutdownTimeoutEnabled) {
                            Log.wtf(getTag(), "Bridged Mode Timeout message received"
                                    + " while timeout is disabled. Dropping.");
                            break;
                        }
                        Set<String> idleInstances = getIdleInstances();
                        if (idleInstances.size() == 0) {
                            break;
                        }
                        Log.d(getTag(), "Instance idle timout, the number of the idle instances is "
                                + idleInstances.size());
                        removeIfaceInstanceFromBridgedApIface(
                                getHighestFrequencyInstance(idleInstances));
                        break;
                    case CMD_INTERFACE_DESTROYED:
                        Log.d(getTag(), "Interface was cleanly destroyed.");
                        updateApState(WifiManager.WIFI_AP_STATE_DISABLING,
                                WifiManager.WIFI_AP_STATE_ENABLED, 0);
                        mIfaceIsDestroyed = true;
                        quitNow();
                        break;
                    case CMD_FAILURE:
                        Log.w(getTag(), "hostapd failure, stop and report failure");
                        /* fall through */
                    case CMD_INTERFACE_DOWN:
                        Log.w(getTag(), "interface error, stop and report failure");
                        updateApState(WifiManager.WIFI_AP_STATE_FAILED,
                                WifiManager.WIFI_AP_STATE_ENABLED,
                                WifiManager.SAP_START_FAILURE_GENERAL);
                        updateApState(WifiManager.WIFI_AP_STATE_DISABLING,
                                WifiManager.WIFI_AP_STATE_FAILED, 0);
                        quitNow();
                        break;
                    case CMD_UPDATE_CAPABILITY:
                        // Capability should only changed by carrier requirement. Only apply to
                        // Tether Mode
                        if (mOriginalModeConfiguration.getTargetMode()
                                ==  WifiManager.IFACE_IP_MODE_TETHERED) {
                            SoftApCapability capability = (SoftApCapability) message.obj;
                            mCurrentSoftApCapability = new SoftApCapability(capability);
                            mWifiMetrics.updateSoftApCapability(mCurrentSoftApCapability,
                                    mOriginalModeConfiguration.getTargetMode(), isBridgedMode());
                            updateClientConnection();
                        }
                        break;
                    case CMD_UPDATE_CONFIG:
                        SoftApConfiguration newConfig = (SoftApConfiguration) message.obj;
                        SoftApConfiguration originalConfig =
                                mOriginalModeConfiguration.getSoftApConfiguration();
                        if (!ApConfigUtil.checkConfigurationChangeNeedToRestart(
                                originalConfig, newConfig)) {
                            Log.d(getTag(), "Configuration changed to " + newConfig);
                            if (mCurrentSoftApConfiguration.getMaxNumberOfClients()
                                    != newConfig.getMaxNumberOfClients()) {
                                Log.d(getTag(), "Max Client changed, reset to record the metrics");
                                mEverReportMetricsForMaxClient = false;
                            }
                            boolean needRescheduleTimer =
                                    mCurrentSoftApConfiguration.getShutdownTimeoutMillis()
                                    != newConfig.getShutdownTimeoutMillis()
                                    || mTimeoutEnabled != newConfig.isAutoShutdownEnabled()
                                    || mBridgedModeOpportunisticsShutdownTimeoutEnabled
                                    != newConfig
                                    .isBridgedModeOpportunisticShutdownEnabledInternal();
                            updateChangeableConfiguration(newConfig);
                            updateClientConnection();
                            if (needRescheduleTimer) {
                                cancelTimeoutMessage();
                                cancelBridgedModeIdleInstanceTimeoutMessage();
                                scheduleTimeoutMessages();
                                // Update SoftApInfo
                                for (SoftApInfo info : mCurrentSoftApInfoMap.values()) {
                                    SoftApInfo newInfo = new SoftApInfo(info);
                                    newInfo.setAutoShutdownTimeoutMillis(mTimeoutEnabled
                                            ? getShutdownTimeoutMillis() : 0);
                                    updateSoftApInfo(newInfo, false);
                                }
                            }
                            mWifiMetrics.updateSoftApConfiguration(
                                    mCurrentSoftApConfiguration,
                                    mOriginalModeConfiguration.getTargetMode(), isBridgedMode());
                        } else {
                            Log.d(getTag(), "Ignore the config: " + newConfig
                                    + " update since it requires restart");
                        }
                        break;
                    case CMD_UPDATE_COUNTRY_CODE:
                        String countryCode = (String) message.obj;
                        if (!TextUtils.isEmpty(countryCode)
                                && !TextUtils.equals(mCountryCode, countryCode)
                                && mWifiNative.setApCountryCode(
                                mApInterfaceName, countryCode.toUpperCase(Locale.ROOT))) {
                            Log.i(getTag(), "Update country code when Soft AP enabled from "
                                    + mCountryCode + " to " + countryCode);
                            mCountryCode = countryCode;
                        }
                        break;
                    case CMD_FORCE_DISCONNECT_PENDING_CLIENTS:
                        if (mPendingDisconnectClients.size() != 0) {
                            Log.d(getTag(), "Disconnect pending list is NOT empty");
                            mPendingDisconnectClients.forEach((pendingClient, reason)->
                                    mWifiNative.forceClientDisconnect(mApInterfaceName,
                                    pendingClient.getMacAddress(), reason));
                            sendMessageDelayed(
                                    SoftApStateMachine.CMD_FORCE_DISCONNECT_PENDING_CLIENTS,
                                    SOFT_AP_PENDING_DISCONNECTION_CHECK_DELAY_MS);
                        }
                        break;
                    case CMD_SAFE_CHANNEL_FREQUENCY_CHANGED:
                        updateSafeChannelFrequencyList();
                        if (!isBridgedMode() || mCurrentSoftApInfoMap.size() != 2) {
                            Log.d(getTag(), "Ignore safe channel changed in single AP state");
                            break;
                        }
                        Set<String> unavailableInstances = new HashSet<>();
                        for (SoftApInfo currentInfo : mCurrentSoftApInfoMap.values()) {
                            int sapFreq = currentInfo.getFrequency();
                            if (!mSafeChannelFrequencyList.contains(sapFreq)) {
                                int sapBand = ApConfigUtil.convertFrequencyToBand(sapFreq);
                                if (sapBand != ApConfigUtil.removeUnavailableBands(
                                            mCurrentSoftApCapability,
                                            sapBand, mCoexManager)) {
                                    unavailableInstances.add(currentInfo.getApInstanceIdentifier());
                                }
                            }
                        }
                        removeIfaceInstanceFromBridgedApIface(
                                getHighestFrequencyInstance(unavailableInstances));
                        break;
                    case CMD_HANDLE_WIFI_CONNECTED:
                        if (!isBridgedMode() || mCurrentSoftApInfoMap.size() != 2) {
                            Log.d(getTag(), "Ignore wifi connected in single AP state");
                            break;
                        }
                        WifiInfo wifiInfo = (WifiInfo) message.obj;
                        int wifiFreq = wifiInfo.getFrequency();
                        String targetShutDownInstance = "";
                        if (!mSafeChannelFrequencyList.contains(wifiFreq)) {
                            Log.i(getTag(), "Wifi connected to freq:" + wifiFreq
                                    + " which is unavailable for SAP");
                            for (SoftApInfo sapInfo : mCurrentSoftApInfoMap.values()) {
                                if (ApConfigUtil.convertFrequencyToBand(sapInfo.getFrequency())
                                          == ApConfigUtil.convertFrequencyToBand(wifiFreq)) {
                                    targetShutDownInstance = sapInfo.getApInstanceIdentifier();
                                    Log.d(getTag(), "Remove the " + targetShutDownInstance
                                            + " instance which is running on the same band as "
                                            + "the wifi connection on an unsafe channel");
                                    break;
                                }
                            }
                            // Wifi may connect to different band as the SAP. For instances:
                            // Wifi connect to 6Ghz but bridged AP is running on 2.4Ghz + 5Ghz.
                            // In this case, targetShutDownInstance will be empty, shutdown the
                            // highest frequency instance.
                            removeIfaceInstanceFromBridgedApIface(
                                    TextUtils.isEmpty(targetShutDownInstance)
                                    ? getHighestFrequencyInstance(mCurrentSoftApInfoMap.keySet())
                                    : targetShutDownInstance);
                        }
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }
    }
}
