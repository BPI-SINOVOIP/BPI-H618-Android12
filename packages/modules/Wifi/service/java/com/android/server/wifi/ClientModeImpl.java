/*
 * Copyright (C) 2010 The Android Open Source Project
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

import static android.net.util.KeepalivePacketDataUtil.parseTcpKeepalivePacketData;
import static android.net.wifi.WifiConfiguration.NetworkSelectionStatus.DISABLED_NO_INTERNET_PERMANENT;
import static android.net.wifi.WifiConfiguration.NetworkSelectionStatus.DISABLED_NO_INTERNET_TEMPORARY;
import static android.net.wifi.WifiManager.WIFI_FEATURE_FILS_SHA256;
import static android.net.wifi.WifiManager.WIFI_FEATURE_FILS_SHA384;

import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_PRIMARY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_LONG_LIVED;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_TRANSIENT;
import static com.android.server.wifi.WifiSettingsConfigStore.WIFI_STA_FACTORY_MAC_ADDRESS;
import static com.android.server.wifi.proto.WifiStatsLog.WIFI_DISCONNECT_REPORTED__FAILURE_CODE__SUPPLICANT_DISCONNECTED;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.wifi.supplicant.V1_0.ISupplicantStaIfaceCallback.ReasonCode;
import android.hardware.wifi.supplicant.V1_0.ISupplicantStaIfaceCallback.StatusCode;
import android.net.CaptivePortalData;
import android.net.ConnectivityManager;
import android.net.DhcpResultsParcelable;
import android.net.InvalidPacketException;
import android.net.IpConfiguration;
import android.net.KeepalivePacketData;
import android.net.Layer2PacketParcelable;
import android.net.LinkProperties;
import android.net.MacAddress;
import android.net.NattKeepalivePacketData;
import android.net.Network;
import android.net.NetworkAgent;
import android.net.NetworkAgentConfig;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.RouteInfo;
import android.net.SocketKeepalive;
import android.net.StaticIpConfiguration;
import android.net.TcpKeepalivePacketData;
import android.net.TcpKeepalivePacketDataParcelable;
import android.net.Uri;
import android.net.ip.IIpClient;
import android.net.ip.IpClientCallbacks;
import android.net.ip.IpClientManager;
import android.net.shared.Layer2Information;
import android.net.shared.ProvisioningConfiguration;
import android.net.shared.ProvisioningConfiguration.ScanResultInfo;
import android.net.vcn.VcnManager;
import android.net.vcn.VcnNetworkPolicyResult;
import android.net.wifi.IWifiConnectedNetworkScorer;
import android.net.wifi.ScanResult;
import android.net.wifi.SecurityParams;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiAnnotations.WifiStandard;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkAgentSpecifier;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.hotspot2.IProvisioningCallback;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.nl80211.DeviceWiphyCapabilities;
import android.net.wifi.nl80211.WifiNl80211Manager;
import android.os.BatteryStatsManager;
import android.os.Build;
import android.os.ConditionVariable;
import android.os.HandlerExecutor;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager;
import android.os.Process;
import android.os.UserHandle;
import android.os.WorkSource;
import android.provider.Settings;
import android.system.OsConstants;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.util.Pair;

import androidx.annotation.RequiresApi;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.IState;
import com.android.internal.util.Protocol;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.modules.utils.build.SdkLevel;
import com.android.net.module.util.Inet4AddressUtils;
import com.android.net.module.util.MacAddressUtils;
import com.android.net.module.util.NetUtils;
import com.android.server.wifi.ActiveModeManager.ClientRole;
import com.android.server.wifi.MboOceController.BtmFrameData;
import com.android.server.wifi.WifiCarrierInfoManager.SimAuthRequestData;
import com.android.server.wifi.WifiCarrierInfoManager.SimAuthResponseData;
import com.android.server.wifi.WifiNative.RxFateReport;
import com.android.server.wifi.WifiNative.TxFateReport;
import com.android.server.wifi.hotspot2.AnqpEvent;
import com.android.server.wifi.hotspot2.IconEvent;
import com.android.server.wifi.hotspot2.NetworkDetail;
import com.android.server.wifi.hotspot2.PasspointManager;
import com.android.server.wifi.hotspot2.WnmData;
import com.android.server.wifi.p2p.WifiP2pServiceImpl;
import com.android.server.wifi.proto.WifiStatsLog;
import com.android.server.wifi.proto.nano.WifiMetricsProto;
import com.android.server.wifi.proto.nano.WifiMetricsProto.StaEvent;
import com.android.server.wifi.proto.nano.WifiMetricsProto.WifiIsUnusableEvent;
import com.android.server.wifi.proto.nano.WifiMetricsProto.WifiUsabilityStats;
import com.android.server.wifi.util.ActionListenerWrapper;
import com.android.server.wifi.util.NativeUtil;
import com.android.server.wifi.util.RssiUtil;
import com.android.server.wifi.util.ScanResultUtil;
import com.android.server.wifi.util.StateMachineObituary;
import com.android.server.wifi.util.WifiPermissionsUtil;
import com.android.wifi.resources.R;

import java.io.BufferedReader;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.URL;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Implementation of ClientMode.  Event handling for Client mode logic is done here,
 * and all changes in connectivity state are initiated here.
 *
 * Note: No external modules should be calling into {@link ClientModeImpl}. Please plumb it via
 * {@link ClientModeManager} until b/160014176 is fixed.
 */
public class ClientModeImpl extends StateMachine implements ClientMode {
    private static final String NETWORKTYPE = "WIFI";
    @VisibleForTesting public static final short NUM_LOG_RECS_VERBOSE_LOW_MEMORY = 200;
    @VisibleForTesting public static final short NUM_LOG_RECS_VERBOSE = 3000;

    private static final String TAG = "WifiClientModeImpl";

    private static final int IPCLIENT_STARTUP_TIMEOUT_MS = 2_000;
    private static final int IPCLIENT_SHUTDOWN_TIMEOUT_MS = 60_000; // 60 seconds
    @VisibleForTesting public static final long CONNECTING_WATCHDOG_TIMEOUT_MS = 30_000; // 30 secs.
    @VisibleForTesting
    public static final short NETWORK_NOT_FOUND_EVENT_THRESHOLD = 3;

    private boolean mVerboseLoggingEnabled = false;

    /**
     * Log with error attribute
     *
     * @param s is string log
     */
    @Override
    protected void loge(String s) {
        Log.e(getTag(), s);
    }
    @Override
    protected void logd(String s) {
        Log.d(getTag(), s);
    }
    @Override
    protected void log(String s) {
        Log.d(getTag(), s);
    }
    private final Context mContext;
    private final WifiMetrics mWifiMetrics;
    private final WifiMonitor mWifiMonitor;
    private final WifiNative mWifiNative;
    private final WifiPermissionsUtil mWifiPermissionsUtil;
    private final WifiConfigManager mWifiConfigManager;
    private final WifiConnectivityManager mWifiConnectivityManager;
    private final WifiBlocklistMonitor mWifiBlocklistMonitor;
    private final WifiDiagnostics mWifiDiagnostics;
    private final Clock mClock;
    private final WifiScoreCard mWifiScoreCard;
    private final WifiHealthMonitor mWifiHealthMonitor;
    private final WifiScoreReport mWifiScoreReport;
    private final WifiTrafficPoller mWifiTrafficPoller;
    private final PasspointManager mPasspointManager;
    private final WifiDataStall mWifiDataStall;
    private final LinkProbeManager mLinkProbeManager;
    private final MboOceController mMboOceController;
    private final McastLockManagerFilterController mMcastLockManagerFilterController;
    private final ActivityManager mActivityManager;
    private final FrameworkFacade mFacade;
    private final WifiStateTracker mWifiStateTracker;
    private final WrongPasswordNotifier mWrongPasswordNotifier;
    private final EapFailureNotifier mEapFailureNotifier;
    private final SimRequiredNotifier mSimRequiredNotifier;
    private final ConnectionFailureNotifier mConnectionFailureNotifier;
    private final WifiNetworkSuggestionsManager mWifiNetworkSuggestionsManager;
    private final ThroughputPredictor mThroughputPredictor;
    private final DeviceConfigFacade mDeviceConfigFacade;
    private final ScoringParams mScoringParams;
    private final WifiThreadRunner mWifiThreadRunner;
    private final ScanRequestProxy mScanRequestProxy;
    private final WifiLastResortWatchdog mWifiLastResortWatchdog;
    private final WakeupController mWakeupController;
    private final WifiLockManager mWifiLockManager;
    private final WifiP2pConnection mWifiP2pConnection;
    private final WifiGlobals mWifiGlobals;
    private final ClientModeManagerBroadcastQueue mBroadcastQueue;
    private final TelephonyManager mTelephonyManager;
    private final WifiSettingsConfigStore mSettingsConfigStore;
    private final long mId;

    private boolean mScreenOn = false;

    private final String mInterfaceName;
    private final ConcreteClientModeManager mClientModeManager;

    private int mLastSignalLevel = -1;
    private int mLastTxKbps = -1;
    private int mLastRxKbps = -1;
    private int mLastScanRssi = WifiInfo.INVALID_RSSI;
    private String mLastBssid;
    // TODO (b/162942761): Ensure this is reset when mTargetNetworkId is set.
    private int mLastNetworkId; // The network Id we successfully joined
    // The subId used by WifiConfiguration with SIM credential which was connected successfully
    private int mLastSubId;
    private String mLastSimBasedConnectionCarrierName;
    private URL mTermsAndConditionsUrl; // Indicates that the Passpoint network is captive
    @Nullable
    private byte[] mCachedPacketFilter;
    @Nullable
    private WifiNative.ConnectionCapabilities mLastConnectionCapabilities;

    private String getTag() {
        return TAG + "[" + (mInterfaceName == null ? "unknown" : mInterfaceName) + "]";
    }

    private void processRssiThreshold(byte curRssi, int reason,
            WifiNative.WifiRssiEventHandler rssiHandler) {
        if (curRssi == Byte.MAX_VALUE || curRssi == Byte.MIN_VALUE) {
            Log.wtf(getTag(), "processRssiThreshold: Invalid rssi " + curRssi);
            return;
        }
        for (int i = 0; i < mRssiRanges.length; i++) {
            if (curRssi < mRssiRanges[i]) {
                // Assume sorted values(ascending order) for rssi,
                // bounded by high(127) and low(-128) at extremeties
                byte maxRssi = mRssiRanges[i];
                byte minRssi = mRssiRanges[i - 1];
                // This value of hw has to be believed as this value is averaged and has breached
                // the rssi thresholds and raised event to host. This would be eggregious if this
                // value is invalid
                mWifiInfo.setRssi(curRssi);
                updateCapabilities();
                int ret = startRssiMonitoringOffload(maxRssi, minRssi, rssiHandler);
                Log.d(getTag(), "Re-program RSSI thresholds for " + getWhatToString(reason)
                        + ": [" + minRssi + ", " + maxRssi + "], curRssi=" + curRssi
                        + " ret=" + ret);
                break;
            }
        }
    }

    private boolean mEnableRssiPolling = false;
    private int mRssiPollToken = 0;

    private PowerManager.WakeLock mSuspendWakeLock;

    /**
     * Value to set in wpa_supplicant "bssid" field when we don't want to restrict connection to
     * a specific AP.
     */
    public static final String SUPPLICANT_BSSID_ANY = "any";

    /**
     * The link properties of the wifi interface.
     * Do not modify this directly; use updateLinkProperties instead.
     */
    private LinkProperties mLinkProperties;

    private final Object mDhcpResultsParcelableLock = new Object();
    @NonNull
    private DhcpResultsParcelable mDhcpResultsParcelable = new DhcpResultsParcelable();

    // NOTE: Do not return to clients - see syncRequestConnectionInfo()
    private final ExtendedWifiInfo mWifiInfo;
    // TODO : remove this member. It should be possible to only call sendNetworkChangeBroadcast when
    // the state actually changed, and to deduce the state of the agent from the state of the
    // machine when generating the NetworkInfo for the broadcast.
    private DetailedState mNetworkAgentState;
    private final SupplicantStateTracker mSupplicantStateTracker;

    // Indicates that framework is attempting to roam, set true on CMD_START_ROAM, set false when
    // wifi connects or fails to connect
    private boolean mIsAutoRoaming = false;

    // Indicates that driver is attempting to allowlist roaming, set true on allowlist roam BSSID
    // associated, set false when wifi connects or fails to connect
    private boolean mIsLinkedNetworkRoaming = false;

    // Roaming failure count
    private int mRoamFailCount = 0;

    // This is the BSSID we are trying to associate to, it can be set to SUPPLICANT_BSSID_ANY
    // if we havent selected a BSSID for joining.
    private String mTargetBssid = SUPPLICANT_BSSID_ANY;
    // This one is used to track the current target network ID. This is used for error
    // handling during connection setup since many error message from supplicant does not report
    // SSID. Once connected, it will be set to invalid
    // TODO (b/162942761): Ensure this is reset when mLastNetworkId is set.
    private int mTargetNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
    private WifiConfiguration mTargetWifiConfiguration = null;
    @Nullable private VcnManager mVcnManager = null;

    /**
     * Method to clear {@link #mTargetBssid} and reset the current connected network's
     * bssid in wpa_supplicant after a roam/connect attempt.
     */
    public boolean clearTargetBssid(String dbg) {
        WifiConfiguration config = mWifiConfigManager.getConfiguredNetwork(mTargetNetworkId);
        if (config == null) {
            return false;
        }
        String bssid = SUPPLICANT_BSSID_ANY;
        if (config.BSSID != null) {
            bssid = config.BSSID;
            if (mVerboseLoggingEnabled) {
                Log.d(getTag(), "force BSSID to " + bssid + "due to config");
            }
        }
        if (mVerboseLoggingEnabled) {
            logd(dbg + " clearTargetBssid " + bssid + " key=" + config.getProfileKey());
        }
        mTargetBssid = bssid;
        return mWifiNative.setNetworkBSSID(mInterfaceName, bssid);
    }

    /**
     * Set Config's default BSSID (for association purpose) and {@link #mTargetBssid}
     * @param config config need set BSSID
     * @param bssid  default BSSID to assocaite with when connect to this network
     * @return false -- does not change the current default BSSID of the configure
     *         true -- change the  current default BSSID of the configur
     */
    private boolean setTargetBssid(WifiConfiguration config, String bssid) {
        if (config == null || bssid == null) {
            return false;
        }
        if (config.BSSID != null) {
            bssid = config.BSSID;
            if (mVerboseLoggingEnabled) {
                Log.d(getTag(), "force BSSID to " + bssid + "due to config");
            }
        }
        if (mVerboseLoggingEnabled) {
            Log.d(getTag(), "setTargetBssid set to " + bssid + " key="
                    + config.getProfileKey());
        }
        mTargetBssid = bssid;
        config.getNetworkSelectionStatus().setNetworkSelectionBSSID(bssid);
        return true;
    }

    private volatile IpClientManager mIpClient;
    private IpClientCallbacksImpl mIpClientCallbacks;

    private final WifiNetworkFactory mNetworkFactory;
    private final UntrustedWifiNetworkFactory mUntrustedNetworkFactory;
    private final OemWifiNetworkFactory mOemWifiNetworkFactory;

    @VisibleForTesting
    @Nullable
    WifiNetworkAgent mNetworkAgent;

    private byte[] mRssiRanges;

    // Used to filter out requests we couldn't possibly satisfy.
    private final NetworkCapabilities mNetworkCapabilitiesFilter;

    /* The base for wifi message types */
    static final int BASE = Protocol.BASE_WIFI;

    /* BT connection state changed, e.g., connected/disconnected */
    static final int CMD_BLUETOOTH_CONNECTION_STATE_CHANGE              = BASE + 31;

    /* Supplicant commands after driver start*/
    /* Disconnect from a network */
    static final int CMD_DISCONNECT                                     = BASE + 73;
    /* Reconnect to a network */
    static final int CMD_RECONNECT                                      = BASE + 74;
    /* Reassociate to a network */
    static final int CMD_REASSOCIATE                                    = BASE + 75;

    /* Enables RSSI poll */
    static final int CMD_ENABLE_RSSI_POLL                               = BASE + 82;
    /* RSSI poll */
    static final int CMD_RSSI_POLL                                      = BASE + 83;
    /** Runs RSSI poll once */
    static final int CMD_ONESHOT_RSSI_POLL                              = BASE + 84;
    /* Enable suspend mode optimizations in the driver */
    static final int CMD_SET_SUSPEND_OPT_ENABLED                        = BASE + 86;

    /**
     * Watchdog for protecting against b/16823537
     * Leave time for 4-way handshake to succeed
     */
    static final int ROAM_GUARD_TIMER_MSEC = 15000;

    int mRoamWatchdogCount = 0;
    /* Roam state watchdog */
    static final int CMD_ROAM_WATCHDOG_TIMER                            = BASE + 94;
    /* Screen change intent handling */
    static final int CMD_SCREEN_STATE_CHANGED                           = BASE + 95;

    /* Disconnecting state watchdog */
    static final int CMD_CONNECTING_WATCHDOG_TIMER                      = BASE + 96;

    /* SIM is removed; reset any cached data for it */
    static final int CMD_RESET_SIM_NETWORKS                             = BASE + 101;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef(prefix = {"RESET_SIM_REASON_"},
            value = {
                    RESET_SIM_REASON_SIM_REMOVED,
                    RESET_SIM_REASON_SIM_INSERTED,
                    RESET_SIM_REASON_DEFAULT_DATA_SIM_CHANGED})
    @interface ResetSimReason {}
    static final int RESET_SIM_REASON_SIM_REMOVED              = 0;
    static final int RESET_SIM_REASON_SIM_INSERTED             = 1;
    static final int RESET_SIM_REASON_DEFAULT_DATA_SIM_CHANGED = 2;

    /** Connecting watchdog timeout counter */
    private int mConnectingWatchdogCount = 0;

    /* We now have a valid IP configuration. */
    static final int CMD_IP_CONFIGURATION_SUCCESSFUL                    = BASE + 138;
    /* We no longer have a valid IP configuration. */
    static final int CMD_IP_CONFIGURATION_LOST                          = BASE + 139;
    /* Link configuration (IP address, DNS, ...) changes notified via netlink */
    static final int CMD_UPDATE_LINKPROPERTIES                          = BASE + 140;

    static final int CMD_START_CONNECT                                  = BASE + 143;

    private static final int NETWORK_STATUS_UNWANTED_DISCONNECT         = 0;
    private static final int NETWORK_STATUS_UNWANTED_VALIDATION_FAILED  = 1;
    private static final int NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN   = 2;

    static final int CMD_UNWANTED_NETWORK                               = BASE + 144;

    static final int CMD_START_ROAM                                     = BASE + 145;

    static final int CMD_NETWORK_STATUS                                 = BASE + 148;

    /* A layer 3 neighbor on the Wi-Fi link became unreachable. */
    static final int CMD_IP_REACHABILITY_LOST                           = BASE + 149;

    static final int CMD_ACCEPT_UNVALIDATED                             = BASE + 153;

    /* used to offload sending IP packet */
    static final int CMD_START_IP_PACKET_OFFLOAD                        = BASE + 160;

    /* used to stop offload sending IP packet */
    static final int CMD_STOP_IP_PACKET_OFFLOAD                         = BASE + 161;

    /* used to start rssi monitoring in hw */
    static final int CMD_START_RSSI_MONITORING_OFFLOAD                  = BASE + 162;

    /* used to stop rssi moniroting in hw */
    static final int CMD_STOP_RSSI_MONITORING_OFFLOAD                   = BASE + 163;

    /* used to indicated RSSI threshold breach in hw */
    static final int CMD_RSSI_THRESHOLD_BREACHED                        = BASE + 164;

    /**
     * Used to handle messages bounced between ClientModeImpl and IpClient.
     */
    static final int CMD_IPV4_PROVISIONING_SUCCESS                      = BASE + 200;
    static final int CMD_IPV4_PROVISIONING_FAILURE                      = BASE + 201;

    /* Push a new APF program to the HAL */
    static final int CMD_INSTALL_PACKET_FILTER                          = BASE + 202;

    /* Enable/disable fallback packet filtering */
    static final int CMD_SET_FALLBACK_PACKET_FILTERING                  = BASE + 203;

    /* Enable/disable Neighbor Discovery offload functionality. */
    static final int CMD_CONFIG_ND_OFFLOAD                              = BASE + 204;

    /* Read the APF program & data buffer */
    static final int CMD_READ_PACKET_FILTER                             = BASE + 208;

    /** Used to add packet filter to apf. */
    static final int CMD_ADD_KEEPALIVE_PACKET_FILTER_TO_APF = BASE + 209;

    /** Used to remove packet filter from apf. */
    static final int CMD_REMOVE_KEEPALIVE_PACKET_FILTER_FROM_APF = BASE + 210;

    /* Indicates that diagnostics should time out a connection start event. */
    static final int CMD_DIAGS_CONNECT_TIMEOUT                          = BASE + 252;

    @VisibleForTesting
    static final int CMD_PRE_DHCP_ACTION                                = BASE + 255;
    private static final int CMD_PRE_DHCP_ACTION_COMPLETE               = BASE + 256;
    private static final int CMD_POST_DHCP_ACTION                       = BASE + 257;

    private static final int CMD_CONNECT_NETWORK                        = BASE + 258;
    private static final int CMD_SAVE_NETWORK                           = BASE + 259;

    /* Start connection to FILS AP*/
    static final int CMD_START_FILS_CONNECTION                          = BASE + 262;

    static final int CMD_CONNECTABLE_STATE_SETUP                        = BASE + 300;

    /* Tracks if suspend optimizations need to be disabled by DHCP,
     * screen or due to high perf mode.
     * When any of them needs to disable it, we keep the suspend optimizations
     * disabled
     */
    private int mSuspendOptNeedsDisabled = 0;

    private static final int SUSPEND_DUE_TO_DHCP = 1;
    private static final int SUSPEND_DUE_TO_HIGH_PERF = 1 << 1;
    private static final int SUSPEND_DUE_TO_SCREEN = 1 << 2;

    /** @see #isRecentlySelectedByTheUser */
    @VisibleForTesting
    public static final int LAST_SELECTED_NETWORK_EXPIRATION_AGE_MILLIS = 30 * 1000;

    /* Tracks if user has enabled Connected Mac Randomization through settings */

    int mRunningBeaconCount = 0;

    /* Parent state where connections are allowed */
    private State mConnectableState = new ConnectableState();
    /* Connecting/Connected to an access point */
    private State mConnectingOrConnectedState = new ConnectingOrConnectedState();
    /* Connecting to an access point */
    private State mL2ConnectingState = new L2ConnectingState();
    /* Connected at 802.11 (L2) level */
    private State mL2ConnectedState = new L2ConnectedState();
    /* fetching IP after connection to access point (assoc+auth complete) */
    private State mL3ProvisioningState = new L3ProvisioningState();
    /* Connected with IP addr */
    private State mL3ConnectedState = new L3ConnectedState();
    /* Roaming */
    private State mRoamingState = new RoamingState();
    /* Network is not connected, supplicant assoc+auth is not complete */
    private State mDisconnectedState = new DisconnectedState();

    /*
     * FILS connection related variables.
     */
    /* To indicate to IpClient whether HLP IEs were included or not in assoc request */
    private boolean mSentHLPs = false;
    /* Tracks IpClient start state until (FILS_)NETWORK_CONNECTION_EVENT event */
    private boolean mIpClientWithPreConnection = false;

    /**
     * Work source to use to blame usage on the WiFi service
     */
    public static final WorkSource WIFI_WORK_SOURCE = new WorkSource(Process.WIFI_UID);

    private final BatteryStatsManager mBatteryStatsManager;

    private final WifiCarrierInfoManager mWifiCarrierInfoManager;

    private final OnNetworkUpdateListener mOnNetworkUpdateListener;

    private final OnCarrierOffloadDisabledListener mOnCarrierOffloadDisabledListener;

    private final ClientModeImplMonitor mCmiMonitor;

    private final WifiNetworkSelector mWifiNetworkSelector;

    private final WifiInjector mWifiInjector;

    // Maximum duration to continue to log Wifi usability stats after a data stall is triggered.
    @VisibleForTesting
    public static final long DURATION_TO_WAIT_ADD_STATS_AFTER_DATA_STALL_MS = 30 * 1000;
    private long mDataStallTriggerTimeMs = -1;
    private int mLastStatusDataStall = WifiIsUnusableEvent.TYPE_UNKNOWN;

    @Nullable
    private StateMachineObituary mObituary = null;

    @Nullable
    private WifiVcnNetworkPolicyChangeListener mVcnPolicyChangeListener;

    /** NETWORK_NOT_FOUND_EVENT event counter */
    private int mNetworkNotFoundEventCount = 0;

    /** Note that this constructor will also start() the StateMachine. */
    public ClientModeImpl(
            @NonNull Context context,
            @NonNull WifiMetrics wifiMetrics,
            @NonNull Clock clock,
            @NonNull WifiScoreCard wifiScoreCard,
            @NonNull WifiStateTracker wifiStateTracker,
            @NonNull WifiPermissionsUtil wifiPermissionsUtil,
            @NonNull WifiConfigManager wifiConfigManager,
            @NonNull PasspointManager passpointManager,
            @NonNull WifiMonitor wifiMonitor,
            @NonNull WifiDiagnostics wifiDiagnostics,
            @NonNull WifiDataStall wifiDataStall,
            @NonNull ScoringParams scoringParams,
            @NonNull WifiThreadRunner wifiThreadRunner,
            @NonNull WifiNetworkSuggestionsManager wifiNetworkSuggestionsManager,
            @NonNull WifiHealthMonitor wifiHealthMonitor,
            @NonNull ThroughputPredictor throughputPredictor,
            @NonNull DeviceConfigFacade deviceConfigFacade,
            @NonNull ScanRequestProxy scanRequestProxy,
            @NonNull ExtendedWifiInfo wifiInfo,
            @NonNull WifiConnectivityManager wifiConnectivityManager,
            @NonNull WifiBlocklistMonitor wifiBlocklistMonitor,
            @NonNull ConnectionFailureNotifier connectionFailureNotifier,
            @NonNull NetworkCapabilities networkCapabilitiesFilter,
            @NonNull WifiNetworkFactory networkFactory,
            @NonNull UntrustedWifiNetworkFactory untrustedWifiNetworkFactory,
            @NonNull OemWifiNetworkFactory oemPaidWifiNetworkFactory,
            @NonNull WifiLastResortWatchdog wifiLastResortWatchdog,
            @NonNull WakeupController wakeupController,
            @NonNull WifiLockManager wifiLockManager,
            @NonNull FrameworkFacade facade,
            @NonNull Looper looper,
            @NonNull WifiNative wifiNative,
            @NonNull WrongPasswordNotifier wrongPasswordNotifier,
            @NonNull WifiTrafficPoller wifiTrafficPoller,
            @NonNull LinkProbeManager linkProbeManager,
            long id,
            @NonNull BatteryStatsManager batteryStatsManager,
            @NonNull SupplicantStateTracker supplicantStateTracker,
            @NonNull MboOceController mboOceController,
            @NonNull WifiCarrierInfoManager wifiCarrierInfoManager,
            @NonNull EapFailureNotifier eapFailureNotifier,
            @NonNull SimRequiredNotifier simRequiredNotifier,
            @NonNull WifiScoreReport wifiScoreReport,
            @NonNull WifiP2pConnection wifiP2pConnection,
            @NonNull WifiGlobals wifiGlobals,
            @NonNull String ifaceName,
            @NonNull ConcreteClientModeManager clientModeManager,
            @NonNull ClientModeImplMonitor cmiMonitor,
            @NonNull ClientModeManagerBroadcastQueue broadcastQueue,
            @NonNull WifiNetworkSelector wifiNetworkSelector,
            @NonNull TelephonyManager telephonyManager,
            @NonNull WifiInjector wifiInjector,
            @NonNull WifiSettingsConfigStore settingsConfigStore,
            boolean verboseLoggingEnabled) {
        super(TAG, looper);
        mWifiMetrics = wifiMetrics;
        mClock = clock;
        mWifiScoreCard = wifiScoreCard;
        mContext = context;
        mFacade = facade;
        mWifiNative = wifiNative;
        mWrongPasswordNotifier = wrongPasswordNotifier;
        mId = id;
        mEapFailureNotifier = eapFailureNotifier;
        mSimRequiredNotifier = simRequiredNotifier;
        mWifiTrafficPoller = wifiTrafficPoller;
        mLinkProbeManager = linkProbeManager;
        mMboOceController = mboOceController;
        mWifiCarrierInfoManager = wifiCarrierInfoManager;
        mBroadcastQueue = broadcastQueue;
        mNetworkAgentState = DetailedState.DISCONNECTED;

        mBatteryStatsManager = batteryStatsManager;
        mWifiStateTracker = wifiStateTracker;

        mWifiPermissionsUtil = wifiPermissionsUtil;
        mWifiConfigManager = wifiConfigManager;

        mPasspointManager = passpointManager;

        mWifiMonitor = wifiMonitor;
        mWifiDiagnostics = wifiDiagnostics;
        mWifiDataStall = wifiDataStall;
        mThroughputPredictor = throughputPredictor;
        mDeviceConfigFacade = deviceConfigFacade;

        mWifiInfo = wifiInfo;
        mSupplicantStateTracker = supplicantStateTracker;
        mWifiConnectivityManager = wifiConnectivityManager;
        mWifiBlocklistMonitor = wifiBlocklistMonitor;
        mConnectionFailureNotifier = connectionFailureNotifier;

        mLinkProperties = new LinkProperties();
        mMcastLockManagerFilterController = new McastLockManagerFilterController();
        mActivityManager = context.getSystemService(ActivityManager.class);

        mLastBssid = null;
        mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
        mLastSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        mLastSimBasedConnectionCarrierName = null;
        mLastSignalLevel = -1;

        mScoringParams = scoringParams;
        mWifiThreadRunner = wifiThreadRunner;
        mScanRequestProxy = scanRequestProxy;
        mWifiScoreReport = wifiScoreReport;

        mNetworkCapabilitiesFilter = networkCapabilitiesFilter;
        mNetworkFactory = networkFactory;

        mUntrustedNetworkFactory = untrustedWifiNetworkFactory;
        mOemWifiNetworkFactory = oemPaidWifiNetworkFactory;

        mWifiLastResortWatchdog = wifiLastResortWatchdog;
        mWakeupController = wakeupController;
        mWifiLockManager = wifiLockManager;

        mWifiNetworkSuggestionsManager = wifiNetworkSuggestionsManager;
        mWifiHealthMonitor = wifiHealthMonitor;
        mWifiP2pConnection = wifiP2pConnection;
        mWifiGlobals = wifiGlobals;

        mInterfaceName = ifaceName;
        mClientModeManager = clientModeManager;
        mCmiMonitor = cmiMonitor;
        mTelephonyManager = telephonyManager;
        mSettingsConfigStore = settingsConfigStore;
        updateInterfaceCapabilities();

        PowerManager powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);

        mSuspendWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "WifiSuspend");
        mSuspendWakeLock.setReferenceCounted(false);

        mOnNetworkUpdateListener = new OnNetworkUpdateListener();
        mWifiConfigManager.addOnNetworkUpdateListener(mOnNetworkUpdateListener);

        mOnCarrierOffloadDisabledListener = new OnCarrierOffloadDisabledListener();
        mWifiCarrierInfoManager.addOnCarrierOffloadDisabledListener(
                mOnCarrierOffloadDisabledListener);

        mWifiNetworkSelector = wifiNetworkSelector;
        mWifiInjector = wifiInjector;

        enableVerboseLogging(verboseLoggingEnabled);

        addState(mConnectableState); {
            addState(mConnectingOrConnectedState, mConnectableState); {
                addState(mL2ConnectingState, mConnectingOrConnectedState);
                addState(mL2ConnectedState, mConnectingOrConnectedState); {
                    addState(mL3ProvisioningState, mL2ConnectedState);
                    addState(mL3ConnectedState, mL2ConnectedState);
                    addState(mRoamingState, mL2ConnectedState);
                }
            }
            addState(mDisconnectedState, mConnectableState);
        }

        setInitialState(mDisconnectedState);

        setLogOnlyTransitions(false);

        // Start the StateMachine
        start();

        // update with initial role for ConcreteClientModeManager
        onRoleChanged();
    }

    private static final int[] WIFI_MONITOR_EVENTS = {
            WifiMonitor.TARGET_BSSID_EVENT,
            WifiMonitor.ASSOCIATED_BSSID_EVENT,
            WifiMonitor.ANQP_DONE_EVENT,
            WifiMonitor.ASSOCIATION_REJECTION_EVENT,
            WifiMonitor.AUTHENTICATION_FAILURE_EVENT,
            WifiMonitor.GAS_QUERY_DONE_EVENT,
            WifiMonitor.GAS_QUERY_START_EVENT,
            WifiMonitor.HS20_REMEDIATION_EVENT,
            WifiMonitor.HS20_DEAUTH_IMMINENT_EVENT,
            WifiMonitor.HS20_TERMS_AND_CONDITIONS_ACCEPTANCE_REQUIRED_EVENT,
            WifiMonitor.NETWORK_CONNECTION_EVENT,
            WifiMonitor.NETWORK_DISCONNECTION_EVENT,
            WifiMonitor.RX_HS20_ANQP_ICON_EVENT,
            WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT,
            WifiMonitor.SUP_REQUEST_IDENTITY,
            WifiMonitor.SUP_REQUEST_SIM_AUTH,
            WifiMonitor.MBO_OCE_BSS_TM_HANDLING_DONE,
            WifiMonitor.TRANSITION_DISABLE_INDICATION,
            WifiMonitor.NETWORK_NOT_FOUND_EVENT,
    };

    private void registerForWifiMonitorEvents()  {
        for (int event : WIFI_MONITOR_EVENTS) {
            mWifiMonitor.registerHandler(mInterfaceName, event, getHandler());
        }

        mWifiMetrics.registerForWifiMonitorEvents(mInterfaceName);
        mWifiLastResortWatchdog.registerForWifiMonitorEvents(mInterfaceName);
    }

    private void deregisterForWifiMonitorEvents()  {
        for (int event : WIFI_MONITOR_EVENTS) {
            mWifiMonitor.deregisterHandler(mInterfaceName, event, getHandler());
        }

        mWifiMetrics.deregisterForWifiMonitorEvents(mInterfaceName);
        mWifiLastResortWatchdog.deregisterForWifiMonitorEvents(mInterfaceName);
    }

    private static boolean isValidBssid(String bssidStr) {
        try {
            MacAddress bssid = MacAddress.fromString(bssidStr);
            return !bssid.equals(WifiManager.ALL_ZEROS_MAC_ADDRESS);
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    private void setMulticastFilter(boolean enabled) {
        if (mIpClient != null) {
            mIpClient.setMulticastFilter(enabled);
        }
    }

    /**
     * Class to implement the MulticastLockManager.FilterController callback.
     */
    class McastLockManagerFilterController implements WifiMulticastLockManager.FilterController {
        /**
         * Start filtering Multicast v4 packets
         */
        public void startFilteringMulticastPackets() {
            setMulticastFilter(true);
        }

        /**
         * Stop filtering Multicast v4 packets
         */
        public void stopFilteringMulticastPackets() {
            setMulticastFilter(false);
        }
    }

    class IpClientCallbacksImpl extends IpClientCallbacks {
        private final ConditionVariable mWaitForCreationCv = new ConditionVariable(false);
        private final ConditionVariable mWaitForStopCv = new ConditionVariable(false);

        @Override
        public void onIpClientCreated(IIpClient ipClient) {
            // IpClient may take a very long time (many minutes) to start at boot time. But after
            // that IpClient should start pretty quickly (a few seconds).
            // Blocking wait for 5 seconds first (for when the wait is short)
            // If IpClient is still not ready after blocking wait, async wait (for when wait is
            // long). Will drop all connection requests until IpClient is ready. Other requests
            // will still be processed.
            sendMessageAtFrontOfQueue(CMD_CONNECTABLE_STATE_SETUP,
                    new IpClientManager(ipClient, getName()));
            mWaitForCreationCv.open();
        }

        @Override
        public void onPreDhcpAction() {
            sendMessage(CMD_PRE_DHCP_ACTION);
        }

        @Override
        public void onPostDhcpAction() {
            sendMessage(CMD_POST_DHCP_ACTION);
        }

        @Override
        public void onNewDhcpResults(DhcpResultsParcelable dhcpResults) {
            if (dhcpResults != null) {
                sendMessage(CMD_IPV4_PROVISIONING_SUCCESS, dhcpResults);
            } else {
                sendMessage(CMD_IPV4_PROVISIONING_FAILURE);
            }
        }

        @Override
        public void onProvisioningSuccess(LinkProperties newLp) {
            addPasspointInfoToLinkProperties(newLp);
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_CMD_IP_CONFIGURATION_SUCCESSFUL);
            sendMessage(CMD_UPDATE_LINKPROPERTIES, newLp);
            sendMessage(CMD_IP_CONFIGURATION_SUCCESSFUL);
        }

        @Override
        public void onProvisioningFailure(LinkProperties newLp) {
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_CMD_IP_CONFIGURATION_LOST);
            sendMessage(CMD_IP_CONFIGURATION_LOST);
        }

        @Override
        public void onLinkPropertiesChange(LinkProperties newLp) {
            addPasspointInfoToLinkProperties(newLp);
            sendMessage(CMD_UPDATE_LINKPROPERTIES, newLp);
        }

        @Override
        public void onReachabilityLost(String logMsg) {
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_CMD_IP_REACHABILITY_LOST);
            sendMessage(CMD_IP_REACHABILITY_LOST, logMsg);
        }

        @Override
        public void installPacketFilter(byte[] filter) {
            sendMessage(CMD_INSTALL_PACKET_FILTER, filter);
        }

        @Override
        public void startReadPacketFilter() {
            sendMessage(CMD_READ_PACKET_FILTER);
        }

        @Override
        public void setFallbackMulticastFilter(boolean enabled) {
            sendMessage(CMD_SET_FALLBACK_PACKET_FILTERING, enabled);
        }

        @Override
        public void setNeighborDiscoveryOffload(boolean enabled) {
            sendMessage(CMD_CONFIG_ND_OFFLOAD, (enabled ? 1 : 0));
        }

        @Override
        public void onPreconnectionStart(List<Layer2PacketParcelable> packets) {
            sendMessage(CMD_START_FILS_CONNECTION, 0, 0, packets);
        }

        @Override
        public void onQuit() {
            mWaitForStopCv.open();
        }

        boolean awaitCreation() {
            return mWaitForCreationCv.block(IPCLIENT_STARTUP_TIMEOUT_MS);
        }

        boolean awaitShutdown() {
            return mWaitForStopCv.block(IPCLIENT_SHUTDOWN_TIMEOUT_MS);
        }
    }

    private void stopIpClient() {
        if (mVerboseLoggingEnabled) {
            Log.v(getTag(), "stopIpClient IpClientWithPreConnection: "
                    + mIpClientWithPreConnection);
        }
        if (mIpClient != null) {
            if (mIpClientWithPreConnection) {
                mIpClient.notifyPreconnectionComplete(false);
            }
            mIpClient.stop();
        }
        mIpClientWithPreConnection = false;
        mSentHLPs = false;
    }

    private void stopDhcpSetup() {
        /* Restore power save and suspend optimizations */
        handlePostDhcpSetup();
        stopIpClient();
    }

    /**
     * Listener for config manager network config related events.
     * TODO (b/117601161) : Move some of the existing handling in WifiConnectivityManager's listener
     * for the same events.
     */
    private class OnNetworkUpdateListener implements
            WifiConfigManager.OnNetworkUpdateListener {
        @Override
        public void onNetworkRemoved(WifiConfiguration config) {
            // The current connected or connecting network has been removed, trigger a disconnect.
            if (config.networkId == mTargetNetworkId || config.networkId == mLastNetworkId) {
                // Disconnect and let autojoin reselect a new network
                sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_NETWORK_REMOVED);
            } else {
                WifiConfiguration currentConfig = getConnectedWifiConfiguration();
                if (currentConfig != null && currentConfig.isLinked(config)) {
                    logi("current network linked config removed, update allowlist networks");
                    updateLinkedNetworks(currentConfig);
                }
            }
            mWifiNative.removeNetworkCachedData(config.networkId);
        }

        @Override
        public void onNetworkUpdated(WifiConfiguration newConfig, WifiConfiguration oldConfig) {
            // Clear invalid cached data.
            mWifiNative.removeNetworkCachedData(oldConfig.networkId);

            if (WifiConfigurationUtil.hasCredentialChanged(oldConfig, newConfig)) {
                mWifiBlocklistMonitor.handleNetworkRemoved(newConfig.SSID);
            }

            // Check if user/app change meteredOverride for connected network.
            if (newConfig.networkId != mLastNetworkId
                    || newConfig.meteredOverride == oldConfig.meteredOverride) {
                // nothing to do.
                return;
            }
            boolean isMetered = WifiConfiguration.isMetered(newConfig, mWifiInfo);
            boolean wasMetered = WifiConfiguration.isMetered(oldConfig, mWifiInfo);
            if (isMetered == wasMetered) {
                // no meteredness change, nothing to do.
                if (mVerboseLoggingEnabled) {
                    Log.v(getTag(), "User/app changed meteredOverride, "
                            + "but no change in meteredness");
                }
                return;
            }
            // If unmetered->metered trigger a disconnect.
            // If metered->unmetered update capabilities.
            if (isMetered) {
                Log.w(getTag(), "Network marked metered, triggering disconnect");
                sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_NETWORK_METERED);
            } else {
                Log.i(getTag(), "Network marked unmetered, triggering capabilities update");
                updateCapabilities(newConfig);
            }
        }

        @Override
        public void onNetworkTemporarilyDisabled(WifiConfiguration config, int disableReason) {
            if (disableReason == DISABLED_NO_INTERNET_TEMPORARY) return;
            if (config.networkId == mTargetNetworkId || config.networkId == mLastNetworkId) {
                // Disconnect and let autojoin reselect a new network
                sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_NETWORK_TEMPORARY_DISABLED);
            }

        }

        @Override
        public void onNetworkPermanentlyDisabled(WifiConfiguration config, int disableReason) {
            // For DISABLED_NO_INTERNET_PERMANENT we do not need to remove the network
            // because supplicant won't be trying to reconnect. If this is due to a
            // preventAutomaticReconnect request from ConnectivityService, that service
            // will disconnect as appropriate.
            if (disableReason == DISABLED_NO_INTERNET_PERMANENT) return;
            if (config.networkId == mTargetNetworkId || config.networkId == mLastNetworkId) {
                // Disconnect and let autojoin reselect a new network
                sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_NETWORK_PERMANENT_DISABLED);
            }
        }
    }

    private class OnCarrierOffloadDisabledListener implements
            WifiCarrierInfoManager.OnCarrierOffloadDisabledListener {

        @Override
        public void onCarrierOffloadDisabled(int subscriptionId, boolean merged) {
            int networkId = mTargetNetworkId == WifiConfiguration.INVALID_NETWORK_ID
                    ? mLastNetworkId : mTargetNetworkId;
            if (networkId == WifiConfiguration.INVALID_NETWORK_ID) {
                return;
            }
            WifiConfiguration configuration = mWifiConfigManager.getConfiguredNetwork(networkId);
            if (configuration.subscriptionId == subscriptionId
                    && configuration.carrierMerged == merged) {
                Log.i(getTag(), "Carrier network offload disabled, triggering disconnect");
                sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_CARRIER_OFFLOAD_DISABLED);
            }
        }
    }

    /**
     * Method to update logging level in wifi service related classes.
     *
     * @param verbose int logging level to use
     */
    public void enableVerboseLogging(boolean verbose) {
        if (verbose) {
            mVerboseLoggingEnabled = true;
            setLogRecSize(mActivityManager.isLowRamDevice()
                    ? NUM_LOG_RECS_VERBOSE_LOW_MEMORY : NUM_LOG_RECS_VERBOSE);
        } else {
            mVerboseLoggingEnabled = false;
            setLogRecSize(mWifiGlobals.getClientModeImplNumLogRecs());
        }

        mWifiScoreReport.enableVerboseLogging(mVerboseLoggingEnabled);
        mSupplicantStateTracker.enableVerboseLogging(mVerboseLoggingEnabled);
    }

    /**
     * Initiates connection to a network specified by the user/app. This method checks if the
     * requesting app holds the NETWORK_SETTINGS permission.
     *
     * @param netId Id network to initiate connection.
     * @param uid UID of the app requesting the connection.
     * @param forceReconnect Whether to force a connection even if we're connected to the same
     *                       network currently.
     */
    private void connectToUserSelectNetwork(int netId, int uid, boolean forceReconnect) {
        logd("connectToUserSelectNetwork netId " + netId + ", uid " + uid
                + ", forceReconnect = " + forceReconnect);
        if (!forceReconnect && (mLastNetworkId == netId || mTargetNetworkId == netId)) {
            // We're already connecting/connected to the user specified network, don't trigger a
            // reconnection unless it was forced.
            logi("connectToUserSelectNetwork already connecting/connected=" + netId);
        } else {
            mWifiConnectivityManager.prepareForForcedConnection(netId);
            if (uid == Process.SYSTEM_UID) {
                mWifiMetrics.setNominatorForNetwork(netId,
                        WifiMetricsProto.ConnectionEvent.NOMINATOR_MANUAL);
            }
            startConnectToNetwork(netId, uid, SUPPLICANT_BSSID_ANY);
        }
    }

    /**
     * ******************************************************
     * Methods exposed for public use
     * ******************************************************
     */

    /**
     * Retrieve a Messenger for the ClientModeImpl Handler
     *
     * @return Messenger
     */
    public Messenger getMessenger() {
        return new Messenger(getHandler());
    }

    // For debugging, keep track of last message status handling
    // TODO, find an equivalent mechanism as part of parent class
    private static final int MESSAGE_HANDLING_STATUS_PROCESSED = 2;
    private static final int MESSAGE_HANDLING_STATUS_OK = 1;
    private static final int MESSAGE_HANDLING_STATUS_UNKNOWN = 0;
    private static final int MESSAGE_HANDLING_STATUS_REFUSED = -1;
    private static final int MESSAGE_HANDLING_STATUS_FAIL = -2;
    private static final int MESSAGE_HANDLING_STATUS_OBSOLETE = -3;
    private static final int MESSAGE_HANDLING_STATUS_DEFERRED = -4;
    private static final int MESSAGE_HANDLING_STATUS_DISCARD = -5;
    private static final int MESSAGE_HANDLING_STATUS_LOOPED = -6;
    private static final int MESSAGE_HANDLING_STATUS_HANDLING_ERROR = -7;

    private int mMessageHandlingStatus = 0;

    private int mOnTime = 0;
    private int mTxTime = 0;
    private int mRxTime = 0;

    private int mOnTimeScreenStateChange = 0;
    private long mLastOntimeReportTimeStamp = 0;
    private long mLastScreenStateChangeTimeStamp = 0;
    private int mOnTimeLastReport = 0;
    private int mTxTimeLastReport = 0;
    private int mRxTimeLastReport = 0;

    private WifiLinkLayerStats mLastLinkLayerStats;
    private long mLastLinkLayerStatsUpdate = 0;

    String reportOnTime() {
        long now = mClock.getWallClockMillis();
        StringBuilder sb = new StringBuilder();
        // Report stats since last report
        int on = mOnTime - mOnTimeLastReport;
        mOnTimeLastReport = mOnTime;
        int tx = mTxTime - mTxTimeLastReport;
        mTxTimeLastReport = mTxTime;
        int rx = mRxTime - mRxTimeLastReport;
        mRxTimeLastReport = mRxTime;
        int period = (int) (now - mLastOntimeReportTimeStamp);
        mLastOntimeReportTimeStamp = now;
        sb.append(String.format("[on:%d tx:%d rx:%d period:%d]", on, tx, rx, period));
        // Report stats since Screen State Changed
        on = mOnTime - mOnTimeScreenStateChange;
        period = (int) (now - mLastScreenStateChangeTimeStamp);
        sb.append(String.format(" from screen [on:%d period:%d]", on, period));
        return sb.toString();
    }

    public WifiLinkLayerStats getWifiLinkLayerStats() {
        if (mInterfaceName == null) {
            loge("getWifiLinkLayerStats called without an interface");
            return null;
        }
        mLastLinkLayerStatsUpdate = mClock.getWallClockMillis();
        WifiLinkLayerStats stats = mWifiNative.getWifiLinkLayerStats(mInterfaceName);
        if (stats != null) {
            mOnTime = stats.on_time;
            mTxTime = stats.tx_time;
            mRxTime = stats.rx_time;
            mRunningBeaconCount = stats.beacon_rx;
            mWifiInfo.updatePacketRates(stats, mLastLinkLayerStatsUpdate);
        } else {
            long mTxPkts = mFacade.getTxPackets(mInterfaceName);
            long mRxPkts = mFacade.getRxPackets(mInterfaceName);
            mWifiInfo.updatePacketRates(mTxPkts, mRxPkts, mLastLinkLayerStatsUpdate);
        }
        mWifiMetrics.incrementWifiLinkLayerUsageStats(mInterfaceName, stats);
        return stats;
    }

    /**
     * Update interface capabilities
     * This method is used to update some of interface capabilities defined in overlay
     */
    private void updateInterfaceCapabilities() {
        DeviceWiphyCapabilities cap = getDeviceWiphyCapabilities();
        if (cap != null) {
            // Some devices don't have support of 11ax indicated by the chip,
            // so an override config value is used
            if (mContext.getResources().getBoolean(R.bool.config_wifi11axSupportOverride)) {
                cap.setWifiStandardSupport(ScanResult.WIFI_STANDARD_11AX, true);
            }

            mWifiNative.setDeviceWiphyCapabilities(mInterfaceName, cap);
        }
    }

    @Override
    public DeviceWiphyCapabilities getDeviceWiphyCapabilities() {
        return mWifiNative.getDeviceWiphyCapabilities(mInterfaceName);
    }

    /**
     * Check if a Wi-Fi standard is supported
     *
     * @param standard A value from {@link ScanResult}'s {@code WIFI_STANDARD_}
     * @return {@code true} if standard is supported, {@code false} otherwise.
     */
    public boolean isWifiStandardSupported(@WifiStandard int standard) {
        return mWifiNative.isWifiStandardSupported(mInterfaceName, standard);
    }

    private byte[] getDstMacForKeepalive(KeepalivePacketData packetData)
            throws InvalidPacketException {
        try {
            InetAddress gateway = NetUtils.selectBestRoute(
                    mLinkProperties.getRoutes(), packetData.getDstAddress()).getGateway();
            String dstMacStr = macAddressFromRoute(gateway.getHostAddress());
            return NativeUtil.macAddressToByteArray(dstMacStr);
        } catch (NullPointerException | IllegalArgumentException e) {
            throw new InvalidPacketException(InvalidPacketException.ERROR_INVALID_IP_ADDRESS);
        }
    }

    private static int getEtherProtoForKeepalive(KeepalivePacketData packetData)
            throws InvalidPacketException {
        if (packetData.getDstAddress() instanceof Inet4Address) {
            return OsConstants.ETH_P_IP;
        } else if (packetData.getDstAddress() instanceof Inet6Address) {
            return OsConstants.ETH_P_IPV6;
        } else {
            throw new InvalidPacketException(InvalidPacketException.ERROR_INVALID_IP_ADDRESS);
        }
    }

    private int startWifiIPPacketOffload(int slot, KeepalivePacketData packetData,
            int intervalSeconds) {
        byte[] packet = null;
        byte[] dstMac = null;
        int proto = 0;

        try {
            packet = packetData.getPacket();
            dstMac = getDstMacForKeepalive(packetData);
            proto = getEtherProtoForKeepalive(packetData);
        } catch (InvalidPacketException e) {
            return e.getError();
        }

        int ret = mWifiNative.startSendingOffloadedPacket(
                mInterfaceName, slot, dstMac, packet, proto, intervalSeconds * 1000);
        if (ret != 0) {
            loge("startWifiIPPacketOffload(" + slot + ", " + intervalSeconds
                    + "): hardware error " + ret);
            return SocketKeepalive.ERROR_HARDWARE_ERROR;
        } else {
            return SocketKeepalive.SUCCESS;
        }
    }

    private int stopWifiIPPacketOffload(int slot) {
        int ret = mWifiNative.stopSendingOffloadedPacket(mInterfaceName, slot);
        if (ret != 0) {
            loge("stopWifiIPPacketOffload(" + slot + "): hardware error " + ret);
            return SocketKeepalive.ERROR_HARDWARE_ERROR;
        } else {
            return SocketKeepalive.SUCCESS;
        }
    }

    private int startRssiMonitoringOffload(byte maxRssi, byte minRssi,
            WifiNative.WifiRssiEventHandler rssiHandler) {
        return mWifiNative.startRssiMonitoring(mInterfaceName, maxRssi, minRssi, rssiHandler);
    }

    private int stopRssiMonitoringOffload() {
        return mWifiNative.stopRssiMonitoring(mInterfaceName);
    }

    @Override
    public boolean isConnected() {
        return getCurrentState() == mL3ConnectedState;
    }

    @Override
    public boolean isConnecting() {
        IState state = getCurrentState();
        return state == mL2ConnectingState || state == mL2ConnectedState
                || state == mL3ProvisioningState;
    }

    @Override
    public boolean isRoaming() {
        return getCurrentState() == mRoamingState;
    }

    @Override
    public boolean isDisconnected() {
        return getCurrentState() == mDisconnectedState;
    }

    /**
     * Method checking if supplicant is in a transient state
     *
     * @return boolean true if in transient state
     */
    public boolean isSupplicantTransientState() {
        SupplicantState supplicantState = mWifiInfo.getSupplicantState();
        if (supplicantState == SupplicantState.ASSOCIATING
                || supplicantState == SupplicantState.AUTHENTICATING
                || supplicantState == SupplicantState.FOUR_WAY_HANDSHAKE
                || supplicantState == SupplicantState.GROUP_HANDSHAKE) {

            if (mVerboseLoggingEnabled) {
                Log.d(getTag(), "Supplicant is under transient state: " + supplicantState);
            }
            return true;
        } else {
            if (mVerboseLoggingEnabled) {
                Log.d(getTag(), "Supplicant is under steady state: " + supplicantState);
            }
        }

        return false;
    }

    /**
     * Get status information for the current connection, if any.
     * Note: This call is synchronized and hence safe to call from any thread (if called from wifi
     * thread, will execute synchronously).
     *
     * @return a {@link WifiInfo} object containing information about the current connection
     * TODO (b/173551144): Change to direct call. Let callers use WifiThreadRunner if necessary.
     */
    @Override
    public WifiInfo syncRequestConnectionInfo() {
        return mWifiThreadRunner.call(() -> new WifiInfo(mWifiInfo), new WifiInfo());
    }

    /**
     * Blocking call to get the current DHCP results
     *
     * @return DhcpResultsParcelable current results
     */
    @NonNull
    public DhcpResultsParcelable syncGetDhcpResultsParcelable() {
        synchronized (mDhcpResultsParcelableLock) {
            return mDhcpResultsParcelable;
        }
    }

    /**
     * When the underlying interface is destroyed, we must immediately tell connectivity service to
     * mark network agent as disconnected and stop the ip client.
     */
    public void handleIfaceDestroyed() {
        handleNetworkDisconnect(false,
                WifiStatsLog.WIFI_DISCONNECT_REPORTED__FAILURE_CODE__IFACE_DESTROYED);
    }

    /** Stop this ClientModeImpl. Do not interact with ClientModeImpl after it has been stopped. */
    public void stop() {
        mSupplicantStateTracker.stop();
        mWifiScoreCard.noteWifiDisabled(mWifiInfo);
        // capture StateMachine LogRecs since we will lose them after we call quitNow()
        // This is used for debugging.
        mObituary = new StateMachineObituary(this);

        // quit discarding all unprocessed messages - this is to preserve the legacy behavior of
        // using sendMessageAtFrontOfQueue(CMD_SET_OPERATIONAL_MODE) which would force a state
        // transition immediately
        quitNow();

        mWifiConfigManager.removeOnNetworkUpdateListener(mOnNetworkUpdateListener);
        mWifiCarrierInfoManager
                .removeOnCarrierOffloadDisabledListener(mOnCarrierOffloadDisabledListener);
        if (mVcnPolicyChangeListener != null) {
            mVcnManager.removeVcnNetworkPolicyChangeListener(mVcnPolicyChangeListener);
            mVcnPolicyChangeListener = null;
        }
    }

    private void checkAbnormalConnectionFailureAndTakeBugReport(String ssid) {
        if (mDeviceConfigFacade.isAbnormalConnectionFailureBugreportEnabled()) {
            int reasonCode = mWifiScoreCard.detectAbnormalConnectionFailure(ssid);
            if (reasonCode != WifiHealthMonitor.REASON_NO_FAILURE) {
                String bugTitle = "Wi-Fi BugReport";
                String bugDetail = "Detect abnormal "
                        + WifiHealthMonitor.FAILURE_REASON_NAME[reasonCode];
                mWifiDiagnostics.takeBugReport(bugTitle, bugDetail);
            }
        }
    }

    private void checkAbnormalDisconnectionAndTakeBugReport() {
        if (mDeviceConfigFacade.isAbnormalDisconnectionBugreportEnabled()) {
            int reasonCode = mWifiScoreCard.detectAbnormalDisconnection(mInterfaceName);
            if (reasonCode != WifiHealthMonitor.REASON_NO_FAILURE) {
                String bugTitle = "Wi-Fi BugReport";
                String bugDetail = "Detect abnormal "
                        + WifiHealthMonitor.FAILURE_REASON_NAME[reasonCode];
                mWifiDiagnostics.takeBugReport(bugTitle, bugDetail);
            }
        }
    }

    /**
     * Retrieve the WifiMulticastLockManager.FilterController callback for registration.
     */
    public WifiMulticastLockManager.FilterController getMcastLockManagerFilterController() {
        return mMcastLockManagerFilterController;
    }

    /**
     * Blocking method to retrieve the passpoint icon.
     *
     * @param bssid representation of the bssid as a long
     * @param fileName name of the file
     *
     * @return boolean returning the result of the call
     */
    public boolean syncQueryPasspointIcon(long bssid, String fileName) {
        return mWifiThreadRunner.call(
                () -> mPasspointManager.queryPasspointIcon(bssid, fileName), false);
    }

    @Override
    public boolean requestAnqp(String bssid, Set<Integer> anqpIds, Set<Integer> hs20Subtypes) {
        return mWifiNative.requestAnqp(mInterfaceName, bssid, anqpIds, hs20Subtypes);
    }

    @Override
    public boolean requestVenueUrlAnqp(String bssid) {
        return mWifiNative.requestVenueUrlAnqp(mInterfaceName, bssid);
    }

    @Override
    public boolean requestIcon(String bssid, String fileName) {
        return mWifiNative.requestIcon(mInterfaceName, bssid, fileName);
    }

    /**
     * Disconnect from Access Point
     */
    public void disconnect() {
        sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_GENERIC);
    }

    /**
     * Initiate a reconnection to AP
     */
    public void reconnect(WorkSource workSource) {
        sendMessage(CMD_RECONNECT, workSource);
    }

    /**
     * Initiate a re-association to AP
     */
    public void reassociate() {
        sendMessage(CMD_REASSOCIATE);
    }

    /**
     * Start subscription provisioning synchronously
     *
     * @param provider {@link OsuProvider} the provider to provision with
     * @param callback {@link IProvisioningCallback} callback for provisioning status
     * @return boolean true indicates provisioning was started, false otherwise
     */
    public boolean syncStartSubscriptionProvisioning(int callingUid, OsuProvider provider,
            IProvisioningCallback callback) {
        return mWifiThreadRunner.call(
                () -> mPasspointManager.startSubscriptionProvisioning(
                        callingUid, provider, callback), false);
    }

    /**
     * Get the supported feature set synchronously
     */
    public long getSupportedFeatures() {
        return mWifiNative.getSupportedFeatureSet(mInterfaceName);
    }

    /**
     * Method to enable/disable RSSI polling
     * @param enabled boolean idicating if polling should start
     */
    @VisibleForTesting
    public void enableRssiPolling(boolean enabled) {
        sendMessage(CMD_ENABLE_RSSI_POLL, enabled ? 1 : 0, 0);
    }

    /**
     * reset cached SIM credential data
     */
    public void resetSimAuthNetworks(@ResetSimReason int resetReason) {
        sendMessage(CMD_RESET_SIM_NETWORKS, resetReason);
    }

    /**
     * Should only be used internally.
     * External callers should use {@link #syncGetCurrentNetwork()}.
     */
    private Network getCurrentNetwork() {
        if (mNetworkAgent != null) {
            return mNetworkAgent.getNetwork();
        } else {
            return null;
        }
    }

    /**
     * Get Network object of currently connected wifi network, or null if not connected.
     * @return Network object of current wifi network
     */
    public Network syncGetCurrentNetwork() {
        return mWifiThreadRunner.call(
                () -> {
                    if (getCurrentState() == mL3ConnectedState
                            || getCurrentState() == mRoamingState) {
                        return getCurrentNetwork();
                    }
                    return null;
                }, null);
    }

    /**
     * Enable TDLS for a specific MAC address
     */
    public void enableTdls(String remoteMacAddress, boolean enable) {
        mWifiNative.startTdls(mInterfaceName, remoteMacAddress, enable);
    }

    /** Send a message indicating bluetooth connection state changed, e.g. connected/disconnected */
    public void onBluetoothConnectionStateChanged() {
        sendMessage(CMD_BLUETOOTH_CONNECTION_STATE_CHANGE);
    }

    /**
     * Trigger dump on the class IpClient object.
     */
    public void dumpIpClient(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (mIpClient != null) {
            // All dumpIpClient does is print this log message.
            // TODO: consider deleting this, since it's not useful.
            pw.println("IpClient logs have moved to dumpsys network_stack");
        }
    }

    private static String dhcpResultsParcelableToString(DhcpResultsParcelable dhcpResults) {
        return new StringBuilder()
                .append("baseConfiguration ").append(dhcpResults.baseConfiguration)
                .append("leaseDuration ").append(dhcpResults.leaseDuration)
                .append("mtu ").append(dhcpResults.mtu)
                .append("serverAddress ").append(dhcpResults.serverAddress)
                .append("serverHostName ").append(dhcpResults.serverHostName)
                .append("vendorInfo ").append(dhcpResults.vendorInfo)
                .toString();
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of ClientModeImpl id=" + mId);
        if (mObituary == null) {
            // StateMachine hasn't quit yet, dump `this` via StateMachineObituary's dump()
            // method for consistency with `else` branch.
            new StateMachineObituary(this).dump(fd, pw, args);
        } else {
            // StateMachine has quit and cleared all LogRecs.
            // Get them from the obituary instead.
            mObituary.dump(fd, pw, args);
        }
        mSupplicantStateTracker.dump(fd, pw, args);
        // Polls link layer stats and RSSI. This allows the stats to show up in
        // WifiScoreReport's dump() output when taking a bug report even if the screen is off.
        updateLinkLayerStatsRssiAndScoreReport();
        pw.println("mLinkProperties " + mLinkProperties);
        pw.println("mWifiInfo " + mWifiInfo);
        pw.println("mDhcpResultsParcelable "
                + dhcpResultsParcelableToString(mDhcpResultsParcelable));
        pw.println("mLastSignalLevel " + mLastSignalLevel);
        pw.println("mLastTxKbps " + mLastTxKbps);
        pw.println("mLastRxKbps " + mLastRxKbps);
        pw.println("mLastBssid " + mLastBssid);
        pw.println("mLastNetworkId " + mLastNetworkId);
        pw.println("mLastSubId " + mLastSubId);
        pw.println("mLastSimBasedConnectionCarrierName " + mLastSimBasedConnectionCarrierName);
        pw.println("mSuspendOptimizationsEnabled " + mContext.getResources().getBoolean(
                R.bool.config_wifiSuspendOptimizationsEnabled));
        pw.println("mSuspendOptNeedsDisabled " + mSuspendOptNeedsDisabled);
        dumpIpClient(fd, pw, args);
        pw.println("WifiScoreReport:");
        mWifiScoreReport.dump(fd, pw, args);
        pw.println();
    }

    /**
     * ******************************************************
     * Internal private functions
     * ******************************************************
     */

    private void logStateAndMessage(Message message, State state) {
        mMessageHandlingStatus = 0;
        if (mVerboseLoggingEnabled) {
            logd(" " + state.getClass().getSimpleName() + " " + getLogRecString(message));
        }
    }

    @Override
    protected boolean recordLogRec(Message msg) {
        switch (msg.what) {
            case CMD_RSSI_POLL:
                return mVerboseLoggingEnabled;
            default:
                return true;
        }
    }

    /**
     * Return the additional string to be logged by LogRec, default
     *
     * @param msg that was processed
     * @return information to be logged as a String
     */
    @Override
    protected String getLogRecString(Message msg) {
        WifiConfiguration config;
        Long now;
        String report;
        String key;
        StringBuilder sb = new StringBuilder();
        sb.append("screen=").append(mScreenOn ? "on" : "off");
        if (mMessageHandlingStatus != MESSAGE_HANDLING_STATUS_UNKNOWN) {
            sb.append("(").append(mMessageHandlingStatus).append(")");
        }
        if (msg.sendingUid > 0 && msg.sendingUid != Process.WIFI_UID) {
            sb.append(" uid=" + msg.sendingUid);
        }
        switch (msg.what) {
            case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                StateChangeResult stateChangeResult = (StateChangeResult) msg.obj;
                if (stateChangeResult != null) {
                    sb.append(stateChangeResult.toString());
                }
                break;
            case CMD_CONNECT_NETWORK:
            case CMD_SAVE_NETWORK: {
                ConnectNetworkMessage cnm = (ConnectNetworkMessage) msg.obj;
                sb.append(" ");
                sb.append(cnm.result.getNetworkId());
                config = mWifiConfigManager.getConfiguredNetwork(cnm.result.getNetworkId());
                if (config != null) {
                    sb.append(" ").append(config.getProfileKey());
                    sb.append(" nid=").append(config.networkId);
                    if (config.hiddenSSID) {
                        sb.append(" hidden");
                    }
                    if (config.preSharedKey != null
                            && !config.preSharedKey.equals("*")) {
                        sb.append(" hasPSK");
                    }
                    if (config.ephemeral) {
                        sb.append(" ephemeral");
                    }
                    sb.append(" cuid=").append(config.creatorUid);
                    sb.append(" suid=").append(config.lastUpdateUid);
                }
                break;
            }
            case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                if (msg.obj != null) {
                    sb.append(" ").append((AssocRejectEventInfo) msg.obj);
                }
                break;
            case WifiMonitor.NETWORK_CONNECTION_EVENT: {
                NetworkConnectionEventInfo connectionInfo = (NetworkConnectionEventInfo) msg.obj;
                sb.append(" ");
                sb.append(connectionInfo.networkId);
                sb.append(" ");
                sb.append(connectionInfo.isFilsConnection);
                sb.append(" ").append(mLastBssid);
                sb.append(" nid=").append(mLastNetworkId);
                config = getConnectedWifiConfigurationInternal();
                if (config != null) {
                    sb.append(" ").append(config.getProfileKey());
                }
                key = mWifiConfigManager.getLastSelectedNetworkConfigKey();
                if (key != null) {
                    sb.append(" last=").append(key);
                }
                break;
            }
            case WifiMonitor.TARGET_BSSID_EVENT:
            case WifiMonitor.ASSOCIATED_BSSID_EVENT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (msg.obj != null) {
                    sb.append(" BSSID=").append((String) msg.obj);
                }
                if (mTargetBssid != null) {
                    sb.append(" Target=").append(mTargetBssid);
                }
                sb.append(" roam=").append(Boolean.toString(mIsAutoRoaming));
                break;
            case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                if (msg.obj != null) {
                    sb.append(" ").append((DisconnectEventInfo) msg.obj);
                }
                if (mLastBssid != null) {
                    sb.append(" lastbssid=").append(mLastBssid);
                }
                if (mWifiInfo.getFrequency() != -1) {
                    sb.append(" freq=").append(mWifiInfo.getFrequency());
                    sb.append(" rssi=").append(mWifiInfo.getRssi());
                }
                break;
            case CMD_RSSI_POLL:
            case CMD_ONESHOT_RSSI_POLL:
            case CMD_UNWANTED_NETWORK:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (mWifiInfo.getSSID() != null) {
                    if (mWifiInfo.getSSID() != null) {
                        sb.append(" ").append(mWifiInfo.getSSID());
                    }
                }
                if (mWifiInfo.getBSSID() != null) {
                    sb.append(" ").append(mWifiInfo.getBSSID());
                }
                sb.append(" rssi=").append(mWifiInfo.getRssi());
                sb.append(" f=").append(mWifiInfo.getFrequency());
                sb.append(" sc=").append(mWifiInfo.getScore());
                sb.append(" link=").append(mWifiInfo.getLinkSpeed());
                sb.append(String.format(" tx=%.1f,", mWifiInfo.getSuccessfulTxPacketsPerSecond()));
                sb.append(String.format(" %.1f,", mWifiInfo.getRetriedTxPacketsPerSecond()));
                sb.append(String.format(" %.1f ", mWifiInfo.getLostTxPacketsPerSecond()));
                sb.append(String.format(" rx=%.1f", mWifiInfo.getSuccessfulRxPacketsPerSecond()));
                sb.append(String.format(" bcn=%d", mRunningBeaconCount));
                report = reportOnTime();
                if (report != null) {
                    sb.append(" ").append(report);
                }
                sb.append(String.format(" score=%d", mWifiInfo.getScore()));
                break;
            case CMD_START_CONNECT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                config = mWifiConfigManager.getConfiguredNetwork(msg.arg1);
                if (config != null) {
                    sb.append(" targetConfigKey=").append(config.getProfileKey());
                    sb.append(" BSSID=" + config.BSSID);
                }
                if (mTargetBssid != null) {
                    sb.append(" targetBssid=").append(mTargetBssid);
                }
                sb.append(" roam=").append(Boolean.toString(mIsAutoRoaming));
                config = getConnectedWifiConfigurationInternal();
                if (config != null) {
                    sb.append(" currentConfigKey=").append(config.getProfileKey());
                }
                break;
            case CMD_START_ROAM:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                String bssid = (String) msg.obj;
                sb.append(" bssid=").append(bssid);
                if (mTargetBssid != null) {
                    sb.append(" ").append(mTargetBssid);
                }
                sb.append(" roam=").append(Boolean.toString(mIsAutoRoaming));
                sb.append(" fail count=").append(Integer.toString(mRoamFailCount));
                break;
            case CMD_PRE_DHCP_ACTION:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" txpkts=").append(mWifiInfo.txSuccess);
                sb.append(",").append(mWifiInfo.txBad);
                sb.append(",").append(mWifiInfo.txRetries);
                break;
            case CMD_POST_DHCP_ACTION:
                if (mLinkProperties != null) {
                    sb.append(" ");
                    sb.append(getLinkPropertiesSummary(mLinkProperties));
                }
                break;
            case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (msg.obj != null) {
                    NetworkInfo info = (NetworkInfo) msg.obj;
                    NetworkInfo.State state = info.getState();
                    NetworkInfo.DetailedState detailedState = info.getDetailedState();
                    if (state != null) {
                        sb.append(" st=").append(state);
                    }
                    if (detailedState != null) {
                        sb.append("/").append(detailedState);
                    }
                }
                break;
            case CMD_IP_CONFIGURATION_LOST:
                int count = -1;
                WifiConfiguration c = getConnectedWifiConfigurationInternal();
                if (c != null) {
                    count = c.getNetworkSelectionStatus().getDisableReasonCounter(
                            WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);
                }
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" failures: ");
                sb.append(Integer.toString(count));
                sb.append("/");
                sb.append(Integer.toString(mFacade.getIntegerSetting(
                        mContext, Settings.Global.WIFI_MAX_DHCP_RETRY_COUNT, 0)));
                if (mWifiInfo.getBSSID() != null) {
                    sb.append(" ").append(mWifiInfo.getBSSID());
                }
                sb.append(String.format(" bcn=%d", mRunningBeaconCount));
                break;
            case CMD_UPDATE_LINKPROPERTIES:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (mLinkProperties != null) {
                    sb.append(" ");
                    sb.append(getLinkPropertiesSummary(mLinkProperties));
                }
                break;
            case CMD_IP_REACHABILITY_LOST:
                if (msg.obj != null) {
                    sb.append(" ").append((String) msg.obj);
                }
                break;
            case CMD_INSTALL_PACKET_FILTER:
                sb.append(" len=" + ((byte[]) msg.obj).length);
                break;
            case CMD_SET_FALLBACK_PACKET_FILTERING:
                sb.append(" enabled=" + (boolean) msg.obj);
                break;
            case CMD_ROAM_WATCHDOG_TIMER:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" cur=").append(mRoamWatchdogCount);
                break;
            case CMD_CONNECTING_WATCHDOG_TIMER:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" cur=").append(mConnectingWatchdogCount);
                break;
            case CMD_START_RSSI_MONITORING_OFFLOAD:
            case CMD_STOP_RSSI_MONITORING_OFFLOAD:
            case CMD_RSSI_THRESHOLD_BREACHED:
                sb.append(" rssi=");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" thresholds=");
                sb.append(Arrays.toString(mRssiRanges));
                break;
            case CMD_IPV4_PROVISIONING_SUCCESS:
                sb.append(" ");
                sb.append(/* DhcpResultsParcelable */ msg.obj);
                break;
            case WifiMonitor.MBO_OCE_BSS_TM_HANDLING_DONE:
                BtmFrameData frameData = (BtmFrameData) msg.obj;
                if (frameData != null) {
                    sb.append(" ").append(frameData.toString());
                }
                break;
            case WifiMonitor.NETWORK_NOT_FOUND_EVENT:
                sb.append(" ssid=" + msg.obj);
                break;
            default:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                break;
        }

        return sb.toString();
    }

    @Override
    protected String getWhatToString(int what) {
        switch (what) {
            case CMD_ACCEPT_UNVALIDATED:
                return "CMD_ACCEPT_UNVALIDATED";
            case CMD_ADD_KEEPALIVE_PACKET_FILTER_TO_APF:
                return "CMD_ADD_KEEPALIVE_PACKET_FILTER_TO_APF";
            case CMD_BLUETOOTH_CONNECTION_STATE_CHANGE:
                return "CMD_BLUETOOTH_CONNECTION_STATE_CHANGE";
            case CMD_CONFIG_ND_OFFLOAD:
                return "CMD_CONFIG_ND_OFFLOAD";
            case CMD_CONNECTING_WATCHDOG_TIMER:
                return "CMD_CONNECTING_WATCHDOG_TIMER";
            case CMD_CONNECT_NETWORK:
                return "CMD_CONNECT_NETWORK";
            case CMD_DIAGS_CONNECT_TIMEOUT:
                return "CMD_DIAGS_CONNECT_TIMEOUT";
            case CMD_DISCONNECT:
                return "CMD_DISCONNECT";
            case CMD_ENABLE_RSSI_POLL:
                return "CMD_ENABLE_RSSI_POLL";
            case CMD_INSTALL_PACKET_FILTER:
                return "CMD_INSTALL_PACKET_FILTER";
            case CMD_IP_CONFIGURATION_LOST:
                return "CMD_IP_CONFIGURATION_LOST";
            case CMD_IP_CONFIGURATION_SUCCESSFUL:
                return "CMD_IP_CONFIGURATION_SUCCESSFUL";
            case CMD_IP_REACHABILITY_LOST:
                return "CMD_IP_REACHABILITY_LOST";
            case CMD_IPV4_PROVISIONING_FAILURE:
                return "CMD_IPV4_PROVISIONING_FAILURE";
            case CMD_IPV4_PROVISIONING_SUCCESS:
                return "CMD_IPV4_PROVISIONING_SUCCESS";
            case CMD_NETWORK_STATUS:
                return "CMD_NETWORK_STATUS";
            case CMD_ONESHOT_RSSI_POLL:
                return "CMD_ONESHOT_RSSI_POLL";
            case CMD_POST_DHCP_ACTION:
                return "CMD_POST_DHCP_ACTION";
            case CMD_PRE_DHCP_ACTION:
                return "CMD_PRE_DHCP_ACTION";
            case CMD_PRE_DHCP_ACTION_COMPLETE:
                return "CMD_PRE_DHCP_ACTION_COMPLETE";
            case CMD_READ_PACKET_FILTER:
                return "CMD_READ_PACKET_FILTER";
            case CMD_REASSOCIATE:
                return "CMD_REASSOCIATE";
            case CMD_RECONNECT:
                return "CMD_RECONNECT";
            case CMD_REMOVE_KEEPALIVE_PACKET_FILTER_FROM_APF:
                return "CMD_REMOVE_KEEPALIVE_PACKET_FILTER_FROM_APF";
            case CMD_RESET_SIM_NETWORKS:
                return "CMD_RESET_SIM_NETWORKS";
            case CMD_ROAM_WATCHDOG_TIMER:
                return "CMD_ROAM_WATCHDOG_TIMER";
            case CMD_RSSI_POLL:
                return "CMD_RSSI_POLL";
            case CMD_RSSI_THRESHOLD_BREACHED:
                return "CMD_RSSI_THRESHOLD_BREACHED";
            case CMD_SAVE_NETWORK:
                return "CMD_SAVE_NETWORK";
            case CMD_SCREEN_STATE_CHANGED:
                return "CMD_SCREEN_STATE_CHANGED";
            case CMD_SET_FALLBACK_PACKET_FILTERING:
                return "CMD_SET_FALLBACK_PACKET_FILTERING";
            case CMD_SET_SUSPEND_OPT_ENABLED:
                return "CMD_SET_SUSPEND_OPT_ENABLED";
            case CMD_START_CONNECT:
                return "CMD_START_CONNECT";
            case CMD_START_FILS_CONNECTION:
                return "CMD_START_FILS_CONNECTION";
            case CMD_START_IP_PACKET_OFFLOAD:
                return "CMD_START_IP_PACKET_OFFLOAD";
            case CMD_START_ROAM:
                return "CMD_START_ROAM";
            case CMD_START_RSSI_MONITORING_OFFLOAD:
                return "CMD_START_RSSI_MONITORING_OFFLOAD";
            case CMD_STOP_IP_PACKET_OFFLOAD:
                return "CMD_STOP_IP_PACKET_OFFLOAD";
            case CMD_STOP_RSSI_MONITORING_OFFLOAD:
                return "CMD_STOP_RSSI_MONITORING_OFFLOAD";
            case CMD_UNWANTED_NETWORK:
                return "CMD_UNWANTED_NETWORK";
            case CMD_UPDATE_LINKPROPERTIES:
                return "CMD_UPDATE_LINKPROPERTIES";
            case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                return "SUPPLICANT_STATE_CHANGE_EVENT";
            case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                return "AUTHENTICATION_FAILURE_EVENT";
            case WifiMonitor.SUP_REQUEST_IDENTITY:
                return "SUP_REQUEST_IDENTITY";
            case WifiMonitor.NETWORK_CONNECTION_EVENT:
                return "NETWORK_CONNECTION_EVENT";
            case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                return "NETWORK_DISCONNECTION_EVENT";
            case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                return "ASSOCIATION_REJECTION_EVENT";
            case WifiMonitor.ANQP_DONE_EVENT:
                return "ANQP_DONE_EVENT";
            case WifiMonitor.RX_HS20_ANQP_ICON_EVENT:
                return "RX_HS20_ANQP_ICON_EVENT";
            case WifiMonitor.GAS_QUERY_DONE_EVENT:
                return "GAS_QUERY_DONE_EVENT";
            case WifiMonitor.HS20_REMEDIATION_EVENT:
                return "HS20_REMEDIATION_EVENT";
            case WifiMonitor.HS20_DEAUTH_IMMINENT_EVENT:
                return "HS20_DEAUTH_IMMINENT_EVENT";
            case WifiMonitor.HS20_TERMS_AND_CONDITIONS_ACCEPTANCE_REQUIRED_EVENT:
                return "HS20_TERMS_AND_CONDITIONS_ACCEPTANCE_REQUIRED_EVENT";
            case WifiMonitor.GAS_QUERY_START_EVENT:
                return "GAS_QUERY_START_EVENT";
            case WifiMonitor.MBO_OCE_BSS_TM_HANDLING_DONE:
                return "MBO_OCE_BSS_TM_HANDLING_DONE";
            case WifiMonitor.TRANSITION_DISABLE_INDICATION:
                return "TRANSITION_DISABLE_INDICATION";
            case WifiP2pServiceImpl.GROUP_CREATING_TIMED_OUT:
                return "GROUP_CREATING_TIMED_OUT";
            case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                return "P2P_CONNECTION_CHANGED";
            case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST:
                return "DISCONNECT_WIFI_REQUEST";
            case WifiP2pServiceImpl.DISCONNECT_WIFI_RESPONSE:
                return "DISCONNECT_WIFI_RESPONSE";
            case WifiP2pServiceImpl.SET_MIRACAST_MODE:
                return "SET_MIRACAST_MODE";
            case WifiP2pServiceImpl.BLOCK_DISCOVERY:
                return "BLOCK_DISCOVERY";
            case WifiMonitor.NETWORK_NOT_FOUND_EVENT:
                return "NETWORK_NOT_FOUND_EVENT";
            default:
                return "what:" + what;
        }
    }

    /** Check whether this connection is the primary connection on the device. */
    private boolean isPrimary() {
        return mClientModeManager.getRole() == ROLE_CLIENT_PRIMARY;
    }

    private void handleScreenStateChanged(boolean screenOn) {
        mScreenOn = screenOn;
        if (mVerboseLoggingEnabled) {
            logd(" handleScreenStateChanged Enter: screenOn=" + screenOn
                    + " mSuspendOptimizationsEnabled="
                    + mContext.getResources().getBoolean(
                            R.bool.config_wifiSuspendOptimizationsEnabled)
                    + " state " + getCurrentState().getName());
        }
        if (isPrimary()) {
            // Only enable RSSI polling on primary STA, none of the secondary STA use-cases
            // can become the default route when other networks types that provide internet
            // connectivity (e.g. cellular) are available. So, no point in scoring
            // these connections for the purpose of switching between wifi and other network
            // types.
            // TODO(b/179518316): Enable this for secondary transient STA also if external scorer
            // is in charge of MBB.
            enableRssiPolling(screenOn);
        }
        if (mContext.getResources().getBoolean(R.bool.config_wifiSuspendOptimizationsEnabled)) {
            int shouldReleaseWakeLock = 0;
            if (screenOn) {
                sendMessage(CMD_SET_SUSPEND_OPT_ENABLED, 0, shouldReleaseWakeLock);
            } else {
                if (isConnected()) {
                    // Allow 2s for suspend optimizations to be set
                    mSuspendWakeLock.acquire(2000);
                    shouldReleaseWakeLock = 1;
                }
                sendMessage(CMD_SET_SUSPEND_OPT_ENABLED, 1, shouldReleaseWakeLock);
            }
        }

        getWifiLinkLayerStats();
        mOnTimeScreenStateChange = mOnTime;
        mLastScreenStateChangeTimeStamp = mLastLinkLayerStatsUpdate;

        if (mVerboseLoggingEnabled) log("handleScreenStateChanged Exit: " + screenOn);
    }

    private void setSuspendOptimizationsNative(int reason, boolean enabled) {
        if (mVerboseLoggingEnabled) {
            log("setSuspendOptimizationsNative: " + reason + " " + enabled
                    + " -want " + mContext.getResources().getBoolean(
                            R.bool.config_wifiSuspendOptimizationsEnabled)
                    + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());
        }
        //mWifiNative.setSuspendOptimizations(enabled);

        if (enabled) {
            mSuspendOptNeedsDisabled &= ~reason;
            /* None of dhcp, screen or highperf need it disabled and user wants it enabled */
            if (mSuspendOptNeedsDisabled == 0
                    && mContext.getResources().getBoolean(
                            R.bool.config_wifiSuspendOptimizationsEnabled)) {
                if (mVerboseLoggingEnabled) {
                    log("setSuspendOptimizationsNative do it " + reason + " " + enabled
                            + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                            + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                            + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                            + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());
                }
                mWifiNative.setSuspendOptimizations(mInterfaceName, true);
            }
        } else {
            mSuspendOptNeedsDisabled |= reason;
            mWifiNative.setSuspendOptimizations(mInterfaceName, false);
        }
    }

    /**
     * Makes a record of the user intent about suspend optimizations.
     */
    private void setSuspendOptimizations(int reason, boolean enabled) {
        if (mVerboseLoggingEnabled) log("setSuspendOptimizations: " + reason + " " + enabled);
        if (enabled) {
            mSuspendOptNeedsDisabled &= ~reason;
        } else {
            mSuspendOptNeedsDisabled |= reason;
        }
        if (mVerboseLoggingEnabled) log("mSuspendOptNeedsDisabled " + mSuspendOptNeedsDisabled);
    }

    /*
     * Fetch link layer stats, RSSI, linkspeed, and frequency on current connection
     * and update Network capabilities
     */
    private WifiLinkLayerStats updateLinkLayerStatsRssiSpeedFrequencyCapabilities() {
        WifiLinkLayerStats stats = getWifiLinkLayerStats();
        WifiNl80211Manager.SignalPollResult pollResult = mWifiNative.signalPoll(mInterfaceName);
        if (pollResult == null) {
            return stats;
        }

        int newRssi = pollResult.currentRssiDbm;
        int newTxLinkSpeed = pollResult.txBitrateMbps;
        int newFrequency = pollResult.associationFrequencyMHz;
        int newRxLinkSpeed = pollResult.rxBitrateMbps;

        if (mVerboseLoggingEnabled) {
            logd("updateLinkLayerStatsRssiSpeedFrequencyCapabilities rssi=" + newRssi
                    + " TxLinkspeed=" + newTxLinkSpeed + " freq=" + newFrequency
                    + " RxLinkSpeed=" + newRxLinkSpeed);
        }

        /*
         * set Tx link speed only if it is valid
         */
        if (newTxLinkSpeed > 0) {
            mWifiInfo.setLinkSpeed(newTxLinkSpeed);
            mWifiInfo.setTxLinkSpeedMbps(newTxLinkSpeed);
        }
        /*
         * set Rx link speed only if it is valid
         */
        if (newRxLinkSpeed > 0) {
            mWifiInfo.setRxLinkSpeedMbps(newRxLinkSpeed);
        }
        if (newFrequency > 0) {
            mWifiInfo.setFrequency(newFrequency);
        }
        // updateLinkBandwidth() requires the latest frequency information
        if (newRssi > WifiInfo.INVALID_RSSI && newRssi < WifiInfo.MAX_RSSI) {
            /*
             * Positive RSSI is possible when devices are close(~0m apart) to each other.
             * And there are some driver/firmware implementation, where they avoid
             * reporting large negative rssi values by adding 256.
             * so adjust the valid rssi reports for such implementations.
             */
            if (newRssi > (WifiInfo.INVALID_RSSI + 256)) {
                Log.wtf(getTag(), "Error! +ve value RSSI: " + newRssi);
                newRssi -= 256;
            }
            mWifiInfo.setRssi(newRssi);
            /*
             * Rather then sending the raw RSSI out every time it
             * changes, we precalculate the signal level that would
             * be displayed in the status bar, and only send the
             * broadcast if that much more coarse-grained number
             * changes. This cuts down greatly on the number of
             * broadcasts, at the cost of not informing others
             * interested in RSSI of all the changes in signal
             * level.
             */
            int newSignalLevel = RssiUtil.calculateSignalLevel(mContext, newRssi);
            if (newSignalLevel != mLastSignalLevel) {
                // TODO (b/162602799): Do we need to change the update frequency?
                sendRssiChangeBroadcast(newRssi);
            }
            updateLinkBandwidthAndCapabilities(stats, newSignalLevel != mLastSignalLevel);
            mLastSignalLevel = newSignalLevel;
        } else {
            mWifiInfo.setRssi(WifiInfo.INVALID_RSSI);
            updateCapabilities();
        }
        mWifiConfigManager.updateScanDetailCacheFromWifiInfo(mWifiInfo);
        /*
         * Increment various performance metrics
         */
        mWifiMetrics.handlePollResult(mInterfaceName, mWifiInfo);
        return stats;
    }

    // Update the link bandwidth. If the link bandwidth changes by a large amount or signal level
    // changes, also update network capabilities.
    private void updateLinkBandwidthAndCapabilities(WifiLinkLayerStats stats,
            boolean hasSignalLevelChanged) {
        WifiScoreCard.PerNetwork network = mWifiScoreCard.lookupNetwork(mWifiInfo.getSSID());
        network.updateLinkBandwidth(mLastLinkLayerStats, stats, mWifiInfo);
        int newTxKbps = network.getTxLinkBandwidthKbps();
        int newRxKbps = network.getRxLinkBandwidthKbps();
        int txDeltaKbps = Math.abs(newTxKbps - mLastTxKbps);
        int rxDeltaKbps = Math.abs(newRxKbps - mLastRxKbps);
        int bwUpdateThresholdPercent = mContext.getResources().getInteger(
                R.integer.config_wifiLinkBandwidthUpdateThresholdPercent);
        if ((txDeltaKbps * 100  >  bwUpdateThresholdPercent * mLastTxKbps)
                || (rxDeltaKbps * 100  >  bwUpdateThresholdPercent * mLastRxKbps)
                || hasSignalLevelChanged) {
            mLastTxKbps = newTxKbps;
            mLastRxKbps = newRxKbps;
            updateCapabilities();
        }

        int l2TxKbps = mWifiDataStall.getTxThroughputKbps();
        int l2RxKbps = mWifiDataStall.getRxThroughputKbps();
        if (l2RxKbps < 0 && l2TxKbps > 0) {
            l2RxKbps = l2TxKbps;
        }
        int [] reportedKbps = {mLastTxKbps, mLastRxKbps};
        int [] l2Kbps = {l2TxKbps, l2RxKbps};
        network.updateBwMetrics(reportedKbps, l2Kbps);
    }

    // Polling has completed, hence we won't have a score anymore
    private void cleanWifiScore() {
        mWifiInfo.setLostTxPacketsPerSecond(0);
        mWifiInfo.setSuccessfulTxPacketsPerSecond(0);
        mWifiInfo.setRetriedTxPacketsRate(0);
        mWifiInfo.setSuccessfulRxPacketsPerSecond(0);
        mWifiScoreReport.reset();
        mLastLinkLayerStats = null;
    }

    private void updateLinkProperties(LinkProperties newLp) {
        if (mVerboseLoggingEnabled) {
            log("Link configuration changed for netId: " + mLastNetworkId
                    + " old: " + mLinkProperties + " new: " + newLp);
        }
        // We own this instance of LinkProperties because IpClient passes us a copy.
        mLinkProperties = newLp;

        if (mNetworkAgent != null) {
            mNetworkAgent.sendLinkProperties(mLinkProperties);
        }

        if (mNetworkAgentState == DetailedState.CONNECTED) {
            // If anything has changed and we're already connected, send out a notification.
            // TODO: Update all callers to use NetworkCallbacks and delete this.
            sendLinkConfigurationChangedBroadcast();
        }

        if (mVerboseLoggingEnabled) {
            StringBuilder sb = new StringBuilder();
            sb.append("updateLinkProperties nid: " + mLastNetworkId);
            sb.append(" state: " + mNetworkAgentState);

            if (mLinkProperties != null) {
                sb.append(" ");
                sb.append(getLinkPropertiesSummary(mLinkProperties));
            }
            logd(sb.toString());
        }
    }

    /**
     * Clears all our link properties.
     */
    private void clearLinkProperties() {
        // Clear the link properties obtained from DHCP. The only caller of this
        // function has already called IpClient#stop(), which clears its state.
        synchronized (mDhcpResultsParcelableLock) {
            mDhcpResultsParcelable = new DhcpResultsParcelable();
        }

        // Now clear the merged link properties.
        mLinkProperties.clear();
        if (mNetworkAgent != null) mNetworkAgent.sendLinkProperties(mLinkProperties);
    }

    private void sendRssiChangeBroadcast(final int newRssi) {
        mBatteryStatsManager.reportWifiRssiChanged(newRssi);
        WifiStatsLog.write(WifiStatsLog.WIFI_SIGNAL_STRENGTH_CHANGED,
                RssiUtil.calculateSignalLevel(mContext, newRssi));

        Intent intent = new Intent(WifiManager.RSSI_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_NEW_RSSI, newRssi);
        mBroadcastQueue.queueOrSendBroadcast(
                mClientModeManager,
                () -> mContext.sendBroadcastAsUser(intent, UserHandle.ALL,
                        android.Manifest.permission.ACCESS_WIFI_STATE));
    }

    private void sendLinkConfigurationChangedBroadcast() {
        Intent intent = new Intent(WifiManager.ACTION_LINK_CONFIGURATION_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        String summary = "broadcast=ACTION_LINK_CONFIGURATION_CHANGED";
        if (mVerboseLoggingEnabled) Log.d(getTag(), "Queuing " + summary);
        mBroadcastQueue.queueOrSendBroadcast(
                mClientModeManager,
                () -> {
                    if (mVerboseLoggingEnabled) Log.d(getTag(), "Sending " + summary);
                    mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
                });
    }

    /**
     * Helper method used to send state about supplicant - This is NOT information about the current
     * wifi connection state.
     *
     * TODO: b/79504296 This broadcast has been deprecated and should be removed
     */
    private void sendSupplicantConnectionChangedBroadcast(boolean connected) {
        Intent intent = new Intent(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED, connected);
        String summary = "broadcast=SUPPLICANT_CONNECTION_CHANGE_ACTION"
                + " EXTRA_SUPPLICANT_CONNECTED=" + connected;
        if (mVerboseLoggingEnabled) Log.d(getTag(), "Queuing " + summary);
        mBroadcastQueue.queueOrSendBroadcast(
                mClientModeManager,
                () -> {
                    if (mVerboseLoggingEnabled) Log.d(getTag(), "Sending " + summary);
                    mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
                });
    }

    /**
     * Record the detailed state of a network.
     *
     * @param state the new {@code DetailedState}
     */
    private void sendNetworkChangeBroadcast(NetworkInfo.DetailedState state) {
        boolean hidden = false;

        if (mIsAutoRoaming) {
            // There is generally a confusion in the system about colluding
            // WiFi Layer 2 state (as reported by supplicant) and the Network state
            // which leads to multiple confusion.
            //
            // If link is roaming, we already have an IP address
            // as well we were connected and are doing L2 cycles of
            // reconnecting or renewing IP address to check that we still have it
            // This L2 link flapping should not be reflected into the Network state
            // which is the state of the WiFi Network visible to Layer 3 and applications
            // Note that once roaming is completed, we will
            // set the Network state to where it should be, or leave it as unchanged
            //
            hidden = true;
        }
        if (mVerboseLoggingEnabled) {
            log("sendNetworkChangeBroadcast"
                    + " oldState=" + mNetworkAgentState
                    + " newState=" + state
                    + " hidden=" + hidden);
        }
        if (hidden || state == mNetworkAgentState) return;
        mNetworkAgentState = state;
        sendNetworkChangeBroadcastWithCurrentState();
    }

    private void sendNetworkChangeBroadcastWithCurrentState() {
        // copy into local variables to force lambda to capture by value and not reference, since
        // mNetworkAgentState is mutable and can change
        final DetailedState networkAgentState = mNetworkAgentState;
        if (mVerboseLoggingEnabled) {
            Log.d(getTag(), "Queueing broadcast=NETWORK_STATE_CHANGED_ACTION"
                    + " networkAgentState=" + networkAgentState);
        }
        mBroadcastQueue.queueOrSendBroadcast(
                mClientModeManager,
                () -> sendNetworkChangeBroadcast(
                        mContext, networkAgentState, mVerboseLoggingEnabled));
    }

    /** Send a NETWORK_STATE_CHANGED_ACTION broadcast. */
    public static void sendNetworkChangeBroadcast(
            Context context, DetailedState networkAgentState, boolean verboseLoggingEnabled) {
        Intent intent = new Intent(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        NetworkInfo networkInfo = makeNetworkInfo(networkAgentState);
        intent.putExtra(WifiManager.EXTRA_NETWORK_INFO, networkInfo);
        if (verboseLoggingEnabled) {
            Log.d(TAG, "Sending broadcast=NETWORK_STATE_CHANGED_ACTION"
                    + " networkAgentState=" + networkAgentState);
        }
        //TODO(b/69974497) This should be non-sticky, but settings needs fixing first.
        context.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private static NetworkInfo makeNetworkInfo(DetailedState networkAgentState) {
        final NetworkInfo ni = new NetworkInfo(ConnectivityManager.TYPE_WIFI, 0, NETWORKTYPE, "");
        ni.setDetailedState(networkAgentState, null, null);
        return ni;
    }

    private List<ScanResult.InformationElement> findMatchingInfoElements(@Nullable String bssid) {
        if (bssid == null) return null;
        ScanResult matchingScanResult = mScanRequestProxy.getScanResult(bssid);
        if (matchingScanResult == null || matchingScanResult.informationElements == null) {
            return null;
        }
        return Arrays.asList(matchingScanResult.informationElements);
    }

    private SupplicantState handleSupplicantStateChange(StateChangeResult stateChangeResult) {
        SupplicantState state = stateChangeResult.state;
        mWifiScoreCard.noteSupplicantStateChanging(mWifiInfo, state);
        // Supplicant state change
        // [31-13] Reserved for future use
        // [8 - 0] Supplicant state (as defined in SupplicantState.java)
        // 50023 supplicant_state_changed (custom|1|5)
        mWifiInfo.setSupplicantState(state);
        // Network id and SSID are only valid when we start connecting
        if (SupplicantState.isConnecting(state)) {
            mWifiInfo.setNetworkId(stateChangeResult.networkId);
            mWifiInfo.setBSSID(stateChangeResult.bssid);
            mWifiInfo.setSSID(stateChangeResult.wifiSsid);
            if (state == SupplicantState.ASSOCIATED) {
                updateWifiInfoLinkParamsAfterAssociation();
            }
            mWifiInfo.setInformationElements(findMatchingInfoElements(stateChangeResult.bssid));
        } else {
            // Reset parameters according to WifiInfo.reset()
            mWifiInfo.setNetworkId(WifiConfiguration.INVALID_NETWORK_ID);
            mWifiInfo.setBSSID(null);
            mWifiInfo.setSSID(null);
            mWifiInfo.setWifiStandard(ScanResult.WIFI_STANDARD_UNKNOWN);
            mWifiInfo.setInformationElements(null);
            mWifiInfo.clearCurrentSecurityType();
        }
        updateLayer2Information();
        // SSID might have been updated, so call updateCapabilities
        updateCapabilities();

        WifiConfiguration config = getConnectedWifiConfigurationInternal();
        if (config == null) {
            // If not connected, this should be non-null.
            config = getConnectingWifiConfigurationInternal();
        }
        if (config != null && config.networkId == mWifiInfo.getNetworkId()) {
            updateWifiInfoWhenConnected(config);

            // Set meteredHint if scan result says network is expensive
            ScanDetailCache scanDetailCache = mWifiConfigManager.getScanDetailCacheForNetwork(
                    config.networkId);
            if (scanDetailCache != null) {
                ScanDetail scanDetail = scanDetailCache.getScanDetail(stateChangeResult.bssid);
                if (scanDetail != null) {
                    mWifiInfo.setFrequency(scanDetail.getScanResult().frequency);
                    NetworkDetail networkDetail = scanDetail.getNetworkDetail();
                    if (networkDetail != null
                            && networkDetail.getAnt() == NetworkDetail.Ant.ChargeablePublic) {
                        mWifiInfo.setMeteredHint(true);
                    }
                }
            }
        }
        mWifiScoreCard.noteSupplicantStateChanged(mWifiInfo);
        return state;
    }

    private void updateWifiInfoWhenConnected(@NonNull WifiConfiguration config) {
        mWifiInfo.setEphemeral(config.ephemeral);
        mWifiInfo.setTrusted(config.trusted);
        mWifiInfo.setOemPaid(config.oemPaid);
        mWifiInfo.setOemPrivate(config.oemPrivate);
        mWifiInfo.setCarrierMerged(config.carrierMerged);
        mWifiInfo.setSubscriptionId(config.subscriptionId);
        mWifiInfo.setOsuAp(config.osu);
        if (config.fromWifiNetworkSpecifier || config.fromWifiNetworkSuggestion) {
            mWifiInfo.setRequestingPackageName(config.creatorName);
        }
        mWifiInfo.setIsPrimary(isPrimary());
        SecurityParams securityParams = config.getNetworkSelectionStatus()
                .getCandidateSecurityParams();
        if (securityParams != null) {
            mWifiInfo.setCurrentSecurityType(securityParams.getSecurityType());
        } else {
            mWifiInfo.clearCurrentSecurityType();
            Log.e(TAG, "Network connection candidate with no security parameters");
        }
    }

    private void updateWifiInfoLinkParamsAfterAssociation() {
        mLastConnectionCapabilities = mWifiNative.getConnectionCapabilities(mInterfaceName);
        int maxTxLinkSpeedMbps = mThroughputPredictor.predictMaxTxThroughput(
                mLastConnectionCapabilities);
        int maxRxLinkSpeedMbps = mThroughputPredictor.predictMaxRxThroughput(
                mLastConnectionCapabilities);
        mWifiInfo.setWifiStandard(mLastConnectionCapabilities.wifiStandard);
        mWifiInfo.setMaxSupportedTxLinkSpeedMbps(maxTxLinkSpeedMbps);
        mWifiInfo.setMaxSupportedRxLinkSpeedMbps(maxRxLinkSpeedMbps);
        mWifiMetrics.setConnectionMaxSupportedLinkSpeedMbps(mInterfaceName,
                maxTxLinkSpeedMbps, maxRxLinkSpeedMbps);
        if (mVerboseLoggingEnabled) {
            StringBuilder sb = new StringBuilder();
            logd(sb.append("WifiStandard: ").append(mLastConnectionCapabilities.wifiStandard)
                    .append(" maxTxSpeed: ").append(maxTxLinkSpeedMbps)
                    .append(" maxRxSpeed: ").append(maxRxLinkSpeedMbps)
                    .toString());
        }
    }

    /**
     * Tells IpClient what BSSID, L2Key and GroupHint to use for IpMemoryStore.
     */
    private void updateLayer2Information() {
        if (mIpClient != null) {
            Pair<String, String> p = mWifiScoreCard.getL2KeyAndGroupHint(mWifiInfo);
            if (!p.equals(mLastL2KeyAndGroupHint)) {
                final MacAddress currentBssid = getMacAddressFromBssidString(mWifiInfo.getBSSID());
                final Layer2Information l2Information = new Layer2Information(
                        p.first, p.second, currentBssid);
                // Update current BSSID on IpClient side whenever l2Key and groupHint
                // pair changes (i.e. the initial connection establishment or L2 roaming
                // happened). If we have COMPLETED the roaming to a different BSSID, start
                // doing DNAv4/DNAv6 -style probing for on-link neighbors of interest (e.g.
                // routers/DNS servers/default gateway).
                if (mIpClient.updateLayer2Information(l2Information)) {
                    mLastL2KeyAndGroupHint = p;
                } else {
                    mLastL2KeyAndGroupHint = null;
                }
            }
        }
    }
    private @Nullable Pair<String, String> mLastL2KeyAndGroupHint = null;

    /**
     * Resets the Wi-Fi Connections by clearing any state, resetting any sockets
     * using the interface, stopping DHCP & disabling interface
     *
     * @param disconnectReason must be one of WifiDisconnectReported.FailureReason values
     *                         defined in /frameworks/proto_logging/stats/atoms.proto
     */
    private void handleNetworkDisconnect(boolean newConnectionInProgress, int disconnectReason) {
        mWifiMetrics.reportNetworkDisconnect(mInterfaceName, disconnectReason,
                mWifiInfo.getRssi(),
                mWifiInfo.getLinkSpeed());

        if (mVerboseLoggingEnabled) {
            Log.v(getTag(), "handleNetworkDisconnect: newConnectionInProgress: "
                    + newConnectionInProgress, new Throwable());
        }

        WifiConfiguration wifiConfig = getConnectedWifiConfigurationInternal();
        if (wifiConfig != null) {
            ScanResultMatchInfo matchInfo = ScanResultMatchInfo.fromWifiConfiguration(wifiConfig);
            // WakeupController should only care about the primary, internet providing network
            if (isPrimary()) {
                mWakeupController.setLastDisconnectInfo(matchInfo);
            }
        }
        stopRssiMonitoringOffload();

        clearTargetBssid("handleNetworkDisconnect");

        // Don't stop DHCP if Fils connection is in progress.
        if (newConnectionInProgress && mIpClientWithPreConnection) {
            if (mVerboseLoggingEnabled) {
                log("handleNetworkDisconnect: Don't stop IpClient as fils connection in progress: "
                        + " mLastNetworkId: " + mLastNetworkId
                        + " mTargetNetworkId" + mTargetNetworkId);
            }
        } else {
            stopDhcpSetup();
        }

        // DISASSOC_AP_BUSY could be received in both after L3 connection is successful or right
        // after BSSID association if the AP can't accept more stations.
        if (disconnectReason == ReasonCode.DISASSOC_AP_BUSY) {
            mWifiConfigManager.setRecentFailureAssociationStatus(
                    mWifiInfo.getNetworkId(),
                    WifiConfiguration.RECENT_FAILURE_DISCONNECTION_AP_BUSY);
        }

        mWifiScoreReport.stopConnectedNetworkScorer();
        /* Reset data structures */
        mWifiScoreReport.reset();
        mWifiInfo.reset();
        /* Reset roaming parameters */
        mIsAutoRoaming = false;

        sendNetworkChangeBroadcast(DetailedState.DISCONNECTED);
        if (mNetworkAgent != null) {
            mNetworkAgent.unregister();
            mNetworkAgent = null;
        }

        /* Clear network properties */
        clearLinkProperties();

        mLastBssid = null;
        mLastLinkLayerStats = null;
        registerDisconnected();
        mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
        mLastSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        mLastSimBasedConnectionCarrierName = null;
        checkAbnormalDisconnectionAndTakeBugReport();
        mWifiScoreCard.resetConnectionState(mInterfaceName);
        updateLayer2Information();
    }

    void handlePreDhcpSetup() {
        if (!mWifiGlobals.isBluetoothConnected()) {
            /*
             * There are problems setting the Wi-Fi driver's power
             * mode to active when bluetooth coexistence mode is
             * enabled or sense.
             * <p>
             * We set Wi-Fi to active mode when
             * obtaining an IP address because we've found
             * compatibility issues with some routers with low power
             * mode.
             * <p>
             * In order for this active power mode to properly be set,
             * we disable coexistence mode until we're done with
             * obtaining an IP address.  One exception is if we
             * are currently connected to a headset, since disabling
             * coexistence would interrupt that connection.
             */
            // Disable the coexistence mode
            mWifiNative.setBluetoothCoexistenceMode(
                    mInterfaceName, WifiNative.BLUETOOTH_COEXISTENCE_MODE_DISABLED);
        }

        // Disable power save and suspend optimizations during DHCP
        // Note: The order here is important for now. Brcm driver changes
        // power settings when we control suspend mode optimizations.
        // TODO: Remove this comment when the driver is fixed.
        setSuspendOptimizationsNative(SUSPEND_DUE_TO_DHCP, false);
        setPowerSave(false);

        // Update link layer stats
        getWifiLinkLayerStats();

        if (mWifiP2pConnection.isConnected()) {
            // P2P discovery breaks DHCP, so shut it down in order to get through this.
            // Once P2P service receives this message and processes it accordingly, it is supposed
            // to send arg2 (i.e. CMD_PRE_DHCP_ACTION_COMPLETE) in a new Message.what back to
            // ClientModeImpl so that we can continue.
            // TODO(b/159060934): Need to ensure that CMD_PRE_DHCP_ACTION_COMPLETE is sent back to
            //  the ClientModeImpl instance that originally sent it. Right now it is sent back to
            //  all ClientModeImpl instances by WifiP2pConnection.
            mWifiP2pConnection.sendMessage(
                    WifiP2pServiceImpl.BLOCK_DISCOVERY,
                    WifiP2pServiceImpl.ENABLED,
                    CMD_PRE_DHCP_ACTION_COMPLETE);
        } else {
            // If the p2p service is not running, we can proceed directly.
            sendMessage(CMD_PRE_DHCP_ACTION_COMPLETE);
        }
    }

    void addLayer2PacketsToHlpReq(List<Layer2PacketParcelable> packets) {
        List<Layer2PacketParcelable> mLayer2Packet = packets;
        if ((mLayer2Packet != null) && (mLayer2Packet.size() > 0)) {
            mWifiNative.flushAllHlp(mInterfaceName);

            for (int j = 0; j < mLayer2Packet.size(); j++) {
                byte [] bytes = mLayer2Packet.get(j).payload;
                byte [] payloadBytes = Arrays.copyOfRange(bytes, 12, bytes.length);
                MacAddress dstAddress = mLayer2Packet.get(j).dstMacAddress;

                mWifiNative.addHlpReq(mInterfaceName, dstAddress, payloadBytes);
            }
        }
    }

    void handlePostDhcpSetup() {
        /* Restore power save and suspend optimizations */
        setSuspendOptimizationsNative(SUSPEND_DUE_TO_DHCP, true);
        setPowerSave(true);

        mWifiP2pConnection.sendMessage(
                WifiP2pServiceImpl.BLOCK_DISCOVERY, WifiP2pServiceImpl.DISABLED);

        // Set the coexistence mode back to its default value
        mWifiNative.setBluetoothCoexistenceMode(
                mInterfaceName, WifiNative.BLUETOOTH_COEXISTENCE_MODE_SENSE);
    }

    /**
     * Set power save mode
     *
     * @param ps true to enable power save (default behavior)
     *           false to disable power save.
     * @return true for success, false for failure
     */
    public boolean setPowerSave(boolean ps) {
        if (mInterfaceName != null) {
            if (mVerboseLoggingEnabled) {
                Log.d(getTag(), "Setting power save for: " + mInterfaceName + " to: " + ps);
            }
            mWifiNative.setPowerSave(mInterfaceName, ps);
        } else {
            Log.e(getTag(), "Failed to setPowerSave, interfaceName is null");
            return false;
        }
        return true;
    }

    /**
     * Set low latency mode
     *
     * @param enabled true to enable low latency
     *                false to disable low latency (default behavior).
     * @return true for success, false for failure
     */
    public boolean setLowLatencyMode(boolean enabled) {
        if (mVerboseLoggingEnabled) {
            Log.d(getTag(), "Setting low latency mode to " + enabled);
        }
        if (!mWifiNative.setLowLatencyMode(enabled)) {
            Log.e(getTag(), "Failed to setLowLatencyMode");
            return false;
        }
        return true;
    }

    @VisibleForTesting
    public static final long DIAGS_CONNECT_TIMEOUT_MILLIS = 60 * 1000;
    /**
     * Inform other components that a new connection attempt is starting.
     */
    private void reportConnectionAttemptStart(
            WifiConfiguration config, String targetBSSID, int roamType) {
        int overlapWithLastConnectionMs =
                mWifiMetrics.startConnectionEvent(mInterfaceName, config, targetBSSID, roamType);
        if (mDeviceConfigFacade.isOverlappingConnectionBugreportEnabled()
                && overlapWithLastConnectionMs
                > mDeviceConfigFacade.getOverlappingConnectionDurationThresholdMs()) {
            String bugTitle = "Wi-Fi BugReport";
            String bugDetail = "Detect abnormal overlapping connection";
            mWifiDiagnostics.takeBugReport(bugTitle, bugDetail);
        }
        mWifiDiagnostics.reportConnectionEvent(WifiDiagnostics.CONNECTION_EVENT_STARTED,
                mClientModeManager);
        if (isPrimary()) {
            mWrongPasswordNotifier.onNewConnectionAttempt();
        }
        removeMessages(CMD_DIAGS_CONNECT_TIMEOUT);
        sendMessageDelayed(CMD_DIAGS_CONNECT_TIMEOUT, DIAGS_CONNECT_TIMEOUT_MILLIS);
    }

    private void handleConnectionAttemptEndForDiagnostics(int level2FailureCode) {
        switch (level2FailureCode) {
            case WifiMetrics.ConnectionEvent.FAILURE_NONE:
                break;
            case WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED:
                // WifiDiagnostics doesn't care about pre-empted connections, or cases
                // where we failed to initiate a connection attempt with supplicant.
                break;
            default:
                removeMessages(CMD_DIAGS_CONNECT_TIMEOUT);
                mWifiDiagnostics.reportConnectionEvent(WifiDiagnostics.CONNECTION_EVENT_FAILED,
                        mClientModeManager);
        }
    }

    /**
     * Inform other components (WifiMetrics, WifiDiagnostics, WifiConnectivityManager, etc.) that
     * the current connection attempt has concluded.
     */
    private void reportConnectionAttemptEnd(int level2FailureCode, int connectivityFailureCode,
            int level2FailureReason) {
        // if connected, this should be non-null.
        WifiConfiguration configuration = getConnectedWifiConfigurationInternal();
        if (configuration == null) {
            // If not connected, this should be non-null.
            configuration = getConnectingWifiConfigurationInternal();
        }

        String bssid = mLastBssid == null ? mTargetBssid : mLastBssid;
        String ssid = mWifiInfo.getSSID();
        if (WifiManager.UNKNOWN_SSID.equals(ssid)) {
            ssid = getConnectingSsidInternal();
        }
        if (level2FailureCode != WifiMetrics.ConnectionEvent.FAILURE_NONE) {
            int blocklistReason = convertToWifiBlocklistMonitorFailureReason(
                    level2FailureCode, level2FailureReason);
            if (blocklistReason != -1) {
                mWifiScoreCard.noteConnectionFailure(mWifiInfo, mLastScanRssi, ssid,
                        blocklistReason);
                checkAbnormalConnectionFailureAndTakeBugReport(ssid);
                mWifiBlocklistMonitor.handleBssidConnectionFailure(bssid, ssid,
                        blocklistReason, mLastScanRssi);
                WifiScoreCard.NetworkConnectionStats recentStats = mWifiScoreCard.lookupNetwork(
                        ssid).getRecentStats();
                if (recentStats.getCount(WifiScoreCard.CNT_CONSECUTIVE_CONNECTION_FAILURE)
                        >= WifiBlocklistMonitor.NUM_CONSECUTIVE_FAILURES_PER_NETWORK_EXP_BACKOFF) {
                    mWifiConfigManager.updateNetworkSelectionStatus(mTargetNetworkId,
                            WifiConfiguration.NetworkSelectionStatus.DISABLED_CONSECUTIVE_FAILURES);
                }
            }
        }

        if (configuration != null
                && configuration.carrierId != TelephonyManager.UNKNOWN_CARRIER_ID) {
            if (level2FailureCode == WifiMetrics.ConnectionEvent.FAILURE_NONE) {
                mWifiMetrics.incrementNumOfCarrierWifiConnectionSuccess();
            } else if (level2FailureCode
                            == WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE
                    && level2FailureReason
                            != WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_NONE) {
                mWifiMetrics.incrementNumOfCarrierWifiConnectionAuthFailure();
            } else {
                mWifiMetrics.incrementNumOfCarrierWifiConnectionNonAuthFailure();
            }
        }

        boolean isAssociationRejection = level2FailureCode
                == WifiMetrics.ConnectionEvent.FAILURE_ASSOCIATION_REJECTION;
        boolean isAuthenticationFailure = level2FailureCode
                == WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE
                && level2FailureReason != WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_WRONG_PSWD;
        if ((isAssociationRejection || isAuthenticationFailure)
                && mWifiConfigManager.isInFlakyRandomizationSsidHotlist(mTargetNetworkId)
                && isPrimary()) {
            mConnectionFailureNotifier
                    .showFailedToConnectDueToNoRandomizedMacSupportNotification(mTargetNetworkId);
        }

        mWifiMetrics.endConnectionEvent(mInterfaceName, level2FailureCode,
                connectivityFailureCode, level2FailureReason, mWifiInfo.getFrequency());
        mWifiConnectivityManager.handleConnectionAttemptEnded(
                mClientModeManager, level2FailureCode, bssid, ssid);
        if (configuration != null) {
            mNetworkFactory.handleConnectionAttemptEnded(level2FailureCode, configuration, bssid);
            mWifiNetworkSuggestionsManager.handleConnectionAttemptEnded(
                    level2FailureCode, configuration, getConnectedBssidInternal());
            ScanResult candidate = configuration.getNetworkSelectionStatus().getCandidate();
            if (candidate != null
                    && !TextUtils.equals(candidate.BSSID, getConnectedBssidInternal())) {
                mWifiMetrics.incrementNumBssidDifferentSelectionBetweenFrameworkAndFirmware();
            }
        }
        handleConnectionAttemptEndForDiagnostics(level2FailureCode);
    }

    /* If this connection attempt fails after 802.1x stage, clear intermediate cached data. */
    void clearNetworkCachedDataIfNeeded(WifiConfiguration config, int reason) {
        if (config == null) return;

        switch(reason) {
            case ReasonCode.UNSPECIFIED:
            case ReasonCode.DEAUTH_LEAVING:
                logi("Keep PMK cache for network disconnection reason " + reason);
                break;
            default:
                mWifiNative.removeNetworkCachedData(config.networkId);
                break;
        }
    }

    /**
     * Returns the sufficient RSSI for the frequency that this network is last seen on.
     */
    private int getSufficientRssi(int networkId, String bssid) {
        ScanDetailCache scanDetailCache =
                mWifiConfigManager.getScanDetailCacheForNetwork(networkId);
        if (scanDetailCache == null) {
            return WifiInfo.INVALID_RSSI;
        }
        ScanResult scanResult = scanDetailCache.getScanResult(bssid);
        if (scanResult == null) {
            return WifiInfo.INVALID_RSSI;
        }
        return mScoringParams.getSufficientRssi(scanResult.frequency);
    }

    private int convertToWifiBlocklistMonitorFailureReason(
            int level2FailureCode, int failureReason) {
        switch (level2FailureCode) {
            case WifiMetrics.ConnectionEvent.FAILURE_ASSOCIATION_TIMED_OUT:
                return WifiBlocklistMonitor.REASON_ASSOCIATION_TIMEOUT;
            case WifiMetrics.ConnectionEvent.FAILURE_ASSOCIATION_REJECTION:
                if (failureReason == WifiMetricsProto.ConnectionEvent
                        .ASSOCIATION_REJECTION_AP_UNABLE_TO_HANDLE_NEW_STA) {
                    return WifiBlocklistMonitor.REASON_AP_UNABLE_TO_HANDLE_NEW_STA;
                }
                return WifiBlocklistMonitor.REASON_ASSOCIATION_REJECTION;
            case WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE:
                if (failureReason == WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_WRONG_PSWD) {
                    return WifiBlocklistMonitor.REASON_WRONG_PASSWORD;
                } else if (failureReason == WifiMetricsProto.ConnectionEvent
                        .AUTH_FAILURE_EAP_FAILURE) {
                    return WifiBlocklistMonitor.REASON_EAP_FAILURE;
                }
                return WifiBlocklistMonitor.REASON_AUTHENTICATION_FAILURE;
            case WifiMetrics.ConnectionEvent.FAILURE_DHCP:
                return WifiBlocklistMonitor.REASON_DHCP_FAILURE;
            case WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION:
                if (failureReason == WifiMetricsProto.ConnectionEvent.DISCONNECTION_NON_LOCAL) {
                    return WifiBlocklistMonitor.REASON_NONLOCAL_DISCONNECT_CONNECTING;
                }
                return -1;
            default:
                return -1;
        }
    }

    private void handleIPv4Success(DhcpResultsParcelable dhcpResults) {
        if (mVerboseLoggingEnabled) {
            logd("handleIPv4Success <" + dhcpResults.toString() + ">");
            logd("link address " + dhcpResults.baseConfiguration.getIpAddress());
        }

        Inet4Address addr;
        synchronized (mDhcpResultsParcelableLock) {
            mDhcpResultsParcelable = dhcpResults;
            addr = (Inet4Address) dhcpResults.baseConfiguration.getIpAddress().getAddress();
        }

        if (mIsAutoRoaming) {
            int previousAddress = mWifiInfo.getIpAddress();
            int newAddress = Inet4AddressUtils.inet4AddressToIntHTL(addr);
            if (previousAddress != newAddress) {
                logd("handleIPv4Success, roaming and address changed"
                        + mWifiInfo + " got: " + addr);
            }
        }

        mWifiInfo.setInetAddress(addr);

        final WifiConfiguration config = getConnectedWifiConfigurationInternal();
        if (config != null) {
            updateWifiInfoWhenConnected(config);
            mWifiConfigManager.updateRandomizedMacExpireTime(config, dhcpResults.leaseDuration);
            mWifiBlocklistMonitor.handleDhcpProvisioningSuccess(mLastBssid, mWifiInfo.getSSID());
        }

        // Set meteredHint if DHCP result says network is metered
        if (dhcpResults.vendorInfo != null && dhcpResults.vendorInfo.contains("ANDROID_METERED")) {
            mWifiInfo.setMeteredHint(true);
            mWifiMetrics.addMeteredStat(config, true);
        } else {
            mWifiMetrics.addMeteredStat(config, false);
        }

        updateCapabilities();
    }

    private void handleSuccessfulIpConfiguration() {
        mLastSignalLevel = -1; // Force update of signal strength
        WifiConfiguration c = getConnectedWifiConfigurationInternal();
        if (c != null) {
            // Reset IP failure tracking
            c.getNetworkSelectionStatus().clearDisableReasonCounter(
                    WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);
        }
    }

    private void handleIPv4Failure() {
        // TODO: Move this to provisioning failure, not DHCP failure.
        // DHCPv4 failure is expected on an IPv6-only network.
        mWifiDiagnostics.triggerBugReportDataCapture(WifiDiagnostics.REPORT_REASON_DHCP_FAILURE);
        if (mVerboseLoggingEnabled) {
            int count = -1;
            WifiConfiguration config = getConnectedWifiConfigurationInternal();
            if (config != null) {
                count = config.getNetworkSelectionStatus().getDisableReasonCounter(
                        WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);
            }
            log("DHCP failure count=" + count);
        }
        reportConnectionAttemptEnd(
                WifiMetrics.ConnectionEvent.FAILURE_DHCP,
                WifiMetricsProto.ConnectionEvent.HLF_DHCP,
                WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
        synchronized (mDhcpResultsParcelableLock) {
            mDhcpResultsParcelable = new DhcpResultsParcelable();
        }
        if (mVerboseLoggingEnabled) {
            logd("handleIPv4Failure");
        }
    }

    private void handleIpConfigurationLost() {
        mWifiInfo.setInetAddress(null);
        mWifiInfo.setMeteredHint(false);

        mWifiConfigManager.updateNetworkSelectionStatus(mLastNetworkId,
                WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);

        /* DHCP times out after about 30 seconds, we do a
         * disconnect thru supplicant, we will let autojoin retry connecting to the network
         */
        mWifiNative.disconnect(mInterfaceName);
    }

    private void handleIpReachabilityLost() {
        mWifiScoreCard.noteIpReachabilityLost(mWifiInfo);
        mWifiInfo.setInetAddress(null);
        mWifiInfo.setMeteredHint(false);

        // Disconnect via supplicant, and let autojoin retry connecting to the network.
        mWifiNative.disconnect(mInterfaceName);
    }

    /*
     * Read a MAC address in /proc/arp/table, used by ClientModeImpl
     * so as to record MAC address of default gateway.
     **/
    private String macAddressFromRoute(String ipAddress) {
        String macAddress = null;
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader("/proc/net/arp"));

            // Skip over the line bearing column titles
            String line = reader.readLine();

            while ((line = reader.readLine()) != null) {
                String[] tokens = line.split("[ ]+");
                if (tokens.length < 6) {
                    continue;
                }

                // ARP column format is
                // Address HWType HWAddress Flags Mask IFace
                String ip = tokens[0];
                String mac = tokens[3];

                if (ipAddress.equals(ip)) {
                    macAddress = mac;
                    break;
                }
            }

            if (macAddress == null) {
                loge("Did not find remoteAddress {" + ipAddress + "} in /proc/net/arp");
            }

        } catch (FileNotFoundException e) {
            loge("Could not open /proc/net/arp to lookup mac address");
        } catch (IOException e) {
            loge("Could not read /proc/net/arp to lookup mac address");
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) {
                // Do nothing
            }
        }
        return macAddress;

    }

    /**
     * Determine if the specified auth failure is considered to be a permanent wrong password
     * failure. The criteria for such failure is when wrong password error is detected
     * and the network had never been connected before.
     *
     * For networks that have previously connected successfully, we consider wrong password
     * failures to be temporary, to be on the conservative side.  Since this might be the
     * case where we are trying to connect to a wrong network (e.g. A network with same SSID
     * but different password).
     */
    private boolean isPermanentWrongPasswordFailure(int networkId, int reasonCode) {
        if (reasonCode != WifiManager.ERROR_AUTH_FAILURE_WRONG_PSWD) {
            return false;
        }
        WifiConfiguration network = mWifiConfigManager.getConfiguredNetwork(networkId);
        if (network != null && network.getNetworkSelectionStatus().hasEverConnected()) {
            return false;
        }
        return true;
    }

     /**
     * Dynamically change the MAC address to use the locally randomized
     * MAC address generated for each network.
     * @param config WifiConfiguration with mRandomizedMacAddress to change into. If the address
     * is masked out or not set, it will generate a new random MAC address.
     */
    private void configureRandomizedMacAddress(WifiConfiguration config) {
        if (config == null) {
            Log.e(getTag(), "No config to change MAC address to");
            return;
        }
        String currentMacString = mWifiNative.getMacAddress(mInterfaceName);
        MacAddress currentMac = getMacAddressFromBssidString(currentMacString);
        MacAddress newMac = mWifiConfigManager.getRandomizedMacAndUpdateIfNeeded(config);
        if (!WifiConfiguration.isValidMacAddressForRandomization(newMac)) {
            Log.wtf(getTag(), "Config generated an invalid MAC address");
        } else if (newMac.equals(currentMac)) {
            Log.d(getTag(), "No changes in MAC address");
        } else {
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_MAC_CHANGE, config);
            boolean setMacSuccess =
                    mWifiNative.setStaMacAddress(mInterfaceName, newMac);
            if (setMacSuccess) {
                mWifiNative.removeNetworkCachedDataIfNeeded(config.networkId, newMac);
            }
            Log.d(getTag(), "ConnectedMacRandomization SSID(" + config.getPrintableSsid()
                    + "). setMacAddress(" + newMac.toString() + ") from "
                    + currentMacString + " = " + setMacSuccess);
        }
    }

    /**
     * Sets the current MAC to the factory MAC address.
     */
    private void setCurrentMacToFactoryMac(WifiConfiguration config) {
        MacAddress factoryMac = retrieveFactoryMacAddressAndStoreIfNecessary();
        if (factoryMac == null) {
            Log.e(getTag(), "Fail to set factory MAC address. Factory MAC is null.");
            return;
        }
        String currentMacStr = mWifiNative.getMacAddress(mInterfaceName);
        if (!TextUtils.equals(currentMacStr, factoryMac.toString())) {
            if (mWifiNative.setStaMacAddress(mInterfaceName, factoryMac)) {
                mWifiNative.removeNetworkCachedDataIfNeeded(config.networkId, factoryMac);
                mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_MAC_CHANGE, config);
            } else {
                Log.e(getTag(), "Failed to set MAC address to " + "'"
                        + factoryMac.toString() + "'");
            }
        }
    }

    /**
     * Helper method to start other services and get state ready for client mode
     */
    private void setupClientMode() {
        Log.d(getTag(), "setupClientMode() ifacename = " + mInterfaceName);

        setMulticastFilter(true);
        registerForWifiMonitorEvents();
        if (isPrimary()) {
            mWifiLastResortWatchdog.clearAllFailureCounts();
        }
        mWifiNative.setSupplicantLogLevel(mVerboseLoggingEnabled);

        // Initialize data structures
        mLastBssid = null;
        mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
        mLastSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        mLastSimBasedConnectionCarrierName = null;
        mLastSignalLevel = -1;
        if (mWifiGlobals.isConnectedMacRandomizationEnabled()) {
            if (!mWifiNative.setStaMacAddress(
                    mInterfaceName, MacAddressUtils.createRandomUnicastAddress())) {
                Log.e(getTag(), "Failed to set random MAC address on bootup");
            }
        }
        mWifiInfo.setMacAddress(mWifiNative.getMacAddress(mInterfaceName));
        // TODO: b/79504296 This broadcast has been deprecated and should be removed
        sendSupplicantConnectionChangedBroadcast(true);

        mWifiNative.setExternalSim(mInterfaceName, true);

        mWifiDiagnostics.startPktFateMonitoring(mInterfaceName);
        mWifiDiagnostics.startLogging(mInterfaceName);

        mMboOceController.enable();

        // Enable bluetooth coexistence scan mode when bluetooth connection is active.
        // When this mode is on, some of the low-level scan parameters used by the
        // driver are changed to reduce interference with bluetooth
        mWifiNative.setBluetoothCoexistenceScanMode(
                mInterfaceName, mWifiGlobals.isBluetoothConnected());
        sendNetworkChangeBroadcast(DetailedState.DISCONNECTED);

        // Disable legacy multicast filtering, which on some chipsets defaults to enabled.
        // Legacy IPv6 multicast filtering blocks ICMPv6 router advertisements which breaks IPv6
        // provisioning. Legacy IPv4 multicast filtering may be re-enabled later via
        // IpClient.Callback.setFallbackMulticastFilter()
        mWifiNative.stopFilteringMulticastV4Packets(mInterfaceName);
        mWifiNative.stopFilteringMulticastV6Packets(mInterfaceName);

        // Set the right suspend mode settings
        mWifiNative.setSuspendOptimizations(mInterfaceName, mSuspendOptNeedsDisabled == 0
                && mContext.getResources().getBoolean(
                        R.bool.config_wifiSuspendOptimizationsEnabled));

        setPowerSave(true);

        // Disable wpa_supplicant from auto reconnecting.
        mWifiNative.enableStaAutoReconnect(mInterfaceName, false);
        // STA has higher priority over P2P
        mWifiNative.setConcurrencyPriority(true);

        // Retrieve and store the factory MAC address (on first bootup).
        retrieveFactoryMacAddressAndStoreIfNecessary();
    }

    /**
     * Helper method to stop external services and clean up state from client mode.
     */
    private void stopClientMode() {
        handleNetworkDisconnect(false,
                WifiStatsLog.WIFI_DISCONNECT_REPORTED__FAILURE_CODE__WIFI_DISABLED);
        // exiting supplicant started state is now only applicable to client mode
        mWifiDiagnostics.stopLogging(mInterfaceName);

        mMboOceController.disable();
        if (mIpClient != null && mIpClient.shutdown()) {
            // Block to make sure IpClient has really shut down, lest cleanup
            // race with, say, bringup code over in tethering.
            mIpClientCallbacks.awaitShutdown();
        }
        deregisterForWifiMonitorEvents(); // uses mInterfaceName, must call before nulling out
        // TODO: b/79504296 This broadcast has been deprecated and should be removed
        sendSupplicantConnectionChangedBroadcast(false);
    }

    /**
     * Helper method called when a L3 connection is successfully established to a network.
     */
    void registerConnected() {
        if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
            WifiConfiguration config = getConnectedWifiConfigurationInternal();
            boolean shouldSetUserConnectChoice = config != null
                    && isRecentlySelectedByTheUser(config)
                    && config.getNetworkSelectionStatus().hasEverConnected()
                    && mWifiPermissionsUtil.checkNetworkSettingsPermission(config.lastConnectUid);
            mWifiConfigManager.updateNetworkAfterConnect(mLastNetworkId,
                    shouldSetUserConnectChoice, mWifiInfo.getRssi());
            // Notify PasspointManager of Passpoint network connected event.
            WifiConfiguration currentNetwork = getConnectedWifiConfigurationInternal();
            if (currentNetwork != null && currentNetwork.isPasspoint()) {
                mPasspointManager.onPasspointNetworkConnected(
                        currentNetwork.getProfileKey());
            }
        }
    }

    void registerDisconnected() {
        if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
            mWifiConfigManager.updateNetworkAfterDisconnect(mLastNetworkId);
        }
    }

    /**
     * Returns WifiConfiguration object corresponding to the currently connected network, null if
     * not connected.
     */
    @Nullable private WifiConfiguration getConnectedWifiConfigurationInternal() {
        if (mLastNetworkId == WifiConfiguration.INVALID_NETWORK_ID) {
            return null;
        }
        return mWifiConfigManager.getConfiguredNetwork(mLastNetworkId);
    }

    /**
     * Returns WifiConfiguration object corresponding to the currently connecting network, null if
     * not connecting.
     */
    @Nullable private WifiConfiguration getConnectingWifiConfigurationInternal() {
        if (mTargetNetworkId == WifiConfiguration.INVALID_NETWORK_ID) {
            return null;
        }
        return mWifiConfigManager.getConfiguredNetwork(mTargetNetworkId);
    }

    @Nullable private String getConnectedBssidInternal() {
        return mLastBssid;
    }

    @Nullable private String getConnectingBssidInternal() {
        return mTargetBssid;
    }

    /**
     * Returns WifiConfiguration object corresponding to the currently connected network, null if
     * not connected.
     */
    @Override
    @Nullable public WifiConfiguration getConnectedWifiConfiguration() {
        if (!isConnected()) return null;
        return getConnectedWifiConfigurationInternal();
    }

    /**
     * Returns WifiConfiguration object corresponding to the currently connecting network, null if
     * not connecting.
     */
    @Override
    @Nullable public WifiConfiguration getConnectingWifiConfiguration() {
        if (!isConnecting() && !isRoaming()) return null;
        return getConnectingWifiConfigurationInternal();
    }

    @Override
    @Nullable public String getConnectedBssid() {
        if (!isConnected()) return null;
        return getConnectedBssidInternal();
    }

    @Override
    @Nullable public String getConnectingBssid() {
        if (!isConnecting() && !isRoaming()) return null;
        return getConnectingBssidInternal();
    }

    ScanResult getCurrentScanResult() {
        WifiConfiguration config = getConnectedWifiConfigurationInternal();
        if (config == null) {
            return null;
        }
        String bssid = mWifiInfo.getBSSID();
        if (bssid == null) {
            bssid = mTargetBssid;
        }
        ScanDetailCache scanDetailCache =
                mWifiConfigManager.getScanDetailCacheForNetwork(config.networkId);

        if (scanDetailCache == null) {
            return null;
        }

        return scanDetailCache.getScanResult(bssid);
    }

    private MacAddress getMacAddressFromBssidString(@Nullable String bssidStr) {
        try {
            return (bssidStr != null) ? MacAddress.fromString(bssidStr) : null;
        } catch (IllegalArgumentException e) {
            Log.e(getTag(), "Invalid BSSID format: " + bssidStr);
            return null;
        }
    }

    private MacAddress getCurrentBssidInternalMacAddress() {
        return getMacAddressFromBssidString(mLastBssid);
    }

    private void connectToNetwork(WifiConfiguration config) {
        if ((config != null) && mWifiNative.connectToNetwork(mInterfaceName, config)) {
            mWifiLastResortWatchdog.noteStartConnectTime(config.networkId);
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_CMD_START_CONNECT, config);
            mIsAutoRoaming = false;
            transitionTo(mL2ConnectingState);
        } else {
            loge("CMD_START_CONNECT Failed to start connection to network " + config);
            mTargetWifiConfiguration = null;
            stopIpClient();
            reportConnectionAttemptEnd(
                    WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                    WifiMetricsProto.ConnectionEvent.HLF_NONE,
                    WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
        }
    }

    /********************************************************
     * HSM states
     *******************************************************/

    class ConnectableState extends State {
        private boolean mIsScreenStateChangeReceiverRegistered = false;
        BroadcastReceiver mScreenStateChangeReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (action.equals(Intent.ACTION_SCREEN_ON)) {
                    sendMessage(CMD_SCREEN_STATE_CHANGED, 1);
                } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                    sendMessage(CMD_SCREEN_STATE_CHANGED, 0);
                }
            }
        };

        @Override
        public void enter() {
            Log.d(getTag(), "entering ConnectableState: ifaceName = " + mInterfaceName);

            setSuspendOptimizationsNative(SUSPEND_DUE_TO_HIGH_PERF, true);

            mWifiStateTracker.updateState(mInterfaceName, WifiStateTracker.INVALID);
            mIpClientCallbacks = new IpClientCallbacksImpl();
            Log.d(getTag(), "Start makeIpClient ifaceName = " + mInterfaceName);
            mFacade.makeIpClient(mContext, mInterfaceName, mIpClientCallbacks);
            mIpClientCallbacks.awaitCreation();
        }

        private void continueEnterSetup(IpClientManager ipClientManager) {
            mIpClient = ipClientManager;
            setupClientMode();

            IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_SCREEN_ON);
            filter.addAction(Intent.ACTION_SCREEN_OFF);
            if (!mIsScreenStateChangeReceiverRegistered) {
                mContext.registerReceiver(mScreenStateChangeReceiver, filter);
                mIsScreenStateChangeReceiverRegistered = true;
            }
            // Learn the initial state of whether the screen is on.
            // We update this field when we receive broadcasts from the system.
            handleScreenStateChanged(mContext.getSystemService(PowerManager.class).isInteractive());

            if (!mWifiNative.removeAllNetworks(mInterfaceName)) {
                loge("Failed to remove networks on entering connect mode");
            }
            mWifiInfo.reset();
            mWifiInfo.setSupplicantState(SupplicantState.DISCONNECTED);

            sendNetworkChangeBroadcast(DetailedState.DISCONNECTED);

            // Inform metrics that Wifi is Enabled (but not yet connected)
            mWifiMetrics.setWifiState(mInterfaceName, WifiMetricsProto.WifiLog.WIFI_DISCONNECTED);
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_WIFI_ENABLED);
            mWifiScoreCard.noteSupplicantStateChanged(mWifiInfo);
        }

        @Override
        public void exit() {
            // Inform metrics that Wifi is being disabled (Toggled, airplane enabled, etc)
            mWifiMetrics.setWifiState(mInterfaceName, WifiMetricsProto.WifiLog.WIFI_DISABLED);
            mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_WIFI_DISABLED);

            if (!mWifiNative.removeAllNetworks(mInterfaceName)) {
                loge("Failed to remove networks on exiting connect mode");
            }
            if (mIsScreenStateChangeReceiverRegistered) {
                mContext.unregisterReceiver(mScreenStateChangeReceiver);
                mIsScreenStateChangeReceiverRegistered = false;
            }

            stopClientMode();
            mWifiScoreCard.doWrites();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CMD_CONNECTABLE_STATE_SETUP:
                    if (mIpClient != null) {
                        loge("Setup connectable state again when IpClient is ready?");
                    } else {
                        IpClientManager ipClientManager = (IpClientManager) message.obj;
                        continueEnterSetup(ipClientManager);
                    }
                    break;
                case CMD_ENABLE_RSSI_POLL: {
                    mEnableRssiPolling = (message.arg1 == 1);
                    break;
                }
                case CMD_SCREEN_STATE_CHANGED: {
                    handleScreenStateChanged(message.arg1 != 0);
                    break;
                }
                case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST: {
                    if (mIpClient == null) {
                        logd("IpClient is not ready, "
                                + "WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST dropped");
                        break;
                    }
                    if (mWifiP2pConnection.shouldTemporarilyDisconnectWifi()) {
                        mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                StaEvent.DISCONNECT_P2P_DISCONNECT_WIFI_REQUEST);
                        mWifiNative.disconnect(mInterfaceName);
                    } else {
                        mWifiNative.reconnect(mInterfaceName);
                    }
                    break;
                }
                case CMD_RECONNECT: {
                    WorkSource workSource = (WorkSource) message.obj;
                    mWifiConnectivityManager.forceConnectivityScan(workSource);
                    break;
                }
                case CMD_REASSOCIATE: {
                    if (mIpClient != null) {
                        logd("IpClient is not ready, REASSOCIATE dropped");

                        mWifiNative.reassociate(mInterfaceName);
                    }
                    break;
                }
                case CMD_START_CONNECT: {
                    if (mIpClient == null) {
                        logd("IpClient is not ready, START_CONNECT dropped");

                        break;
                    }
                    /* connect command coming from auto-join */
                    int netId = message.arg1;
                    int uid = message.arg2;
                    String bssid = (String) message.obj;
                    mSentHLPs = false;
                    // Stop lingering (if it was lingering before) if we start a new connection.
                    // This means that the ClientModeManager was reused for another purpose, so it
                    // should no longer be in lingering mode.
                    mClientModeManager.setShouldReduceNetworkScore(false);

                    if (!hasConnectionRequests()) {
                        if (mNetworkAgent == null) {
                            loge("CMD_START_CONNECT but no requests and not connected,"
                                    + " bailing");
                            break;
                        } else if (!mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
                            loge("CMD_START_CONNECT but no requests and connected, but app "
                                    + "does not have sufficient permissions, bailing");
                            break;
                        }
                    }
                    WifiConfiguration config =
                            mWifiConfigManager.getConfiguredNetworkWithoutMasking(netId);
                    logd("CMD_START_CONNECT "
                            + " my state " + getCurrentState().getName()
                            + " nid=" + netId
                            + " roam=" + mIsAutoRoaming);
                    if (config == null) {
                        loge("CMD_START_CONNECT and no config, bail out...");
                        break;
                    }
                    mTargetNetworkId = netId;
                    // Update scorecard while there is still state from existing connection
                    mLastScanRssi = mWifiConfigManager.findScanRssi(netId,
                            mWifiHealthMonitor.getScanRssiValidTimeMs());
                    mWifiScoreCard.noteConnectionAttempt(mWifiInfo, mLastScanRssi, config.SSID);
                    mWifiBlocklistMonitor.setAllowlistSsids(config.SSID, Collections.emptyList());
                    mWifiBlocklistMonitor.updateFirmwareRoamingConfiguration(Set.of(config.SSID));

                    updateWifiConfigOnStartConnection(config, bssid);
                    reportConnectionAttemptStart(config, mTargetBssid,
                            WifiMetricsProto.ConnectionEvent.ROAM_UNRELATED);

                    String currentMacAddress = mWifiNative.getMacAddress(mInterfaceName);
                    mWifiInfo.setMacAddress(currentMacAddress);
                    Log.i(getTag(), "Connecting with " + currentMacAddress + " as the mac address");

                    mTargetWifiConfiguration = config;
                    mNetworkNotFoundEventCount = 0;
                    /* Check for FILS configuration again after updating the config */
                    if (config.isFilsSha256Enabled() || config.isFilsSha384Enabled()) {
                        boolean isIpClientStarted = startIpClient(config, true);
                        if (isIpClientStarted) {
                            mIpClientWithPreConnection = true;
                            break;
                        }
                    }
                    connectToNetwork(config);
                    break;
                }
                case CMD_START_FILS_CONNECTION: {
                    if (mIpClient == null) {
                        logd("IpClient is not ready, START_FILS_CONNECTION dropped");
                        break;
                    }
                    mWifiMetrics.incrementConnectRequestWithFilsAkmCount();
                    List<Layer2PacketParcelable> packets;
                    packets = (List<Layer2PacketParcelable>) message.obj;
                    if (mVerboseLoggingEnabled) {
                        Log.d(getTag(), "Send HLP IEs to supplicant");
                    }
                    addLayer2PacketsToHlpReq(packets);
                    WifiConfiguration config = mTargetWifiConfiguration;
                    connectToNetwork(config);
                    break;
                }
                case CMD_CONNECT_NETWORK: {
                    ConnectNetworkMessage cnm = (ConnectNetworkMessage) message.obj;
                    if (mIpClient == null) {
                        logd("IpClient is not ready, CONNECT_NETWORK dropped");
                        cnm.listener.sendFailure(WifiManager.ERROR);
                        break;
                    }
                    NetworkUpdateResult result = cnm.result;
                    int netId = result.getNetworkId();
                    connectToUserSelectNetwork(
                            netId, message.sendingUid, result.hasCredentialChanged());
                    mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_CONNECT_NETWORK,
                            mWifiConfigManager.getConfiguredNetwork(netId));
                    cnm.listener.sendSuccess();
                    break;
                }
                case CMD_SAVE_NETWORK: {
                    ConnectNetworkMessage cnm = (ConnectNetworkMessage) message.obj;
                    if (mIpClient == null) {
                        logd("IpClient is not ready, SAVE_NETWORK dropped");
                        cnm.listener.sendFailure(WifiManager.ERROR);
                        break;
                    }
                    NetworkUpdateResult result = cnm.result;
                    int netId = result.getNetworkId();
                    if (mWifiInfo.getNetworkId() == netId) {
                        if (result.hasCredentialChanged()) {
                            // The network credentials changed and we're connected to this network,
                            // start a new connection with the updated credentials.
                            logi("CMD_SAVE_NETWORK credential changed for nid="
                                    + netId + ". Reconnecting.");
                            startConnectToNetwork(netId, message.sendingUid, SUPPLICANT_BSSID_ANY);
                        } else {
                            if (result.hasProxyChanged()) {
                                if (mIpClient != null) {
                                    log("Reconfiguring proxy on connection");
                                    WifiConfiguration currentConfig =
                                            getConnectedWifiConfigurationInternal();
                                    if (currentConfig != null) {
                                        mIpClient.setHttpProxy(currentConfig.getHttpProxy());
                                    } else {
                                        Log.w(getTag(),
                                                "CMD_SAVE_NETWORK proxy change - but no current "
                                                        + "Wi-Fi config");
                                    }
                                }
                            }
                            if (result.hasIpChanged()) {
                                // The current connection configuration was changed
                                // We switched from DHCP to static or from static to DHCP, or the
                                // static IP address has changed.
                                log("Reconfiguring IP on connection");
                                WifiConfiguration currentConfig =
                                        getConnectedWifiConfigurationInternal();
                                if (currentConfig != null) {
                                    transitionTo(mL3ProvisioningState);
                                } else {
                                    Log.w(getTag(), "CMD_SAVE_NETWORK Ip change - but no current "
                                            + "Wi-Fi config");
                                }
                            }
                        }
                    } else if (mWifiInfo.getNetworkId() == WifiConfiguration.INVALID_NETWORK_ID
                            && result.hasCredentialChanged()) {
                        logi("CMD_SAVE_NETWORK credential changed for nid="
                                + netId + " while disconnected. Connecting.");
                        startConnectToNetwork(netId, message.sendingUid, SUPPLICANT_BSSID_ANY);
                    } else if (result.hasCredentialChanged()) {
                        WifiConfiguration currentConfig =
                                getConnectedWifiConfigurationInternal();
                        WifiConfiguration updatedConfig =
                                mWifiConfigManager.getConfiguredNetwork(netId);
                        if (currentConfig != null && currentConfig.isLinked(updatedConfig)) {
                            logi("current network linked config saved, update linked networks");
                            updateLinkedNetworks(currentConfig);
                        }
                    }
                    cnm.listener.sendSuccess();
                    break;
                }
                case CMD_BLUETOOTH_CONNECTION_STATE_CHANGE: {
                    mWifiNative.setBluetoothCoexistenceScanMode(
                            mInterfaceName, mWifiGlobals.isBluetoothConnected());
                    break;
                }
                case CMD_SET_SUSPEND_OPT_ENABLED: {
                    if (message.arg1 == 1) {
                        setSuspendOptimizationsNative(SUSPEND_DUE_TO_SCREEN, true);
                        if (message.arg2 == 1) {
                            mSuspendWakeLock.release();
                        }
                    } else {
                        setSuspendOptimizationsNative(SUSPEND_DUE_TO_SCREEN, false);
                    }
                    break;
                }
                case WifiMonitor.ANQP_DONE_EVENT: {
                    mPasspointManager.notifyANQPDone((AnqpEvent) message.obj);
                    break;
                }
                case CMD_STOP_IP_PACKET_OFFLOAD: {
                    int slot = message.arg1;
                    int ret = stopWifiIPPacketOffload(slot);
                    if (mNetworkAgent != null) {
                        mNetworkAgent.sendSocketKeepaliveEvent(slot, ret);
                    }
                    break;
                }
                case WifiMonitor.RX_HS20_ANQP_ICON_EVENT: {
                    mPasspointManager.notifyIconDone((IconEvent) message.obj);
                    break;
                }
                case WifiMonitor.HS20_DEAUTH_IMMINENT_EVENT:
                    mPasspointManager.handleDeauthImminentEvent((WnmData) message.obj,
                            getConnectedWifiConfigurationInternal());
                    break;
                case WifiMonitor.HS20_TERMS_AND_CONDITIONS_ACCEPTANCE_REQUIRED_EVENT:
                    mWifiMetrics
                            .incrementTotalNumberOfPasspointConnectionsWithTermsAndConditionsUrl();
                    mTermsAndConditionsUrl = mPasspointManager
                            .handleTermsAndConditionsEvent((WnmData) message.obj,
                            getConnectedWifiConfigurationInternal());
                    if (mTermsAndConditionsUrl == null) {
                        loge("Disconnecting from Passpoint network due to an issue with the "
                                + "Terms and Conditions URL");
                        sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_PASSPOINT_TAC);
                    }
                    break;
                case WifiMonitor.HS20_REMEDIATION_EVENT:
                    mPasspointManager.receivedWnmFrame((WnmData) message.obj);
                    break;
                case WifiMonitor.MBO_OCE_BSS_TM_HANDLING_DONE: {
                    handleBssTransitionRequest((BtmFrameData) message.obj);
                    break;
                }
                case CMD_CONFIG_ND_OFFLOAD: {
                    final boolean enabled = (message.arg1 > 0);
                    mWifiNative.configureNeighborDiscoveryOffload(mInterfaceName, enabled);
                    break;
                }
                // Link configuration (IP address, DNS, ...) changes notified via netlink
                case CMD_UPDATE_LINKPROPERTIES: {
                    updateLinkProperties((LinkProperties) message.obj);
                    break;
                }
                case CMD_START_IP_PACKET_OFFLOAD:
                case CMD_ADD_KEEPALIVE_PACKET_FILTER_TO_APF:
                case CMD_REMOVE_KEEPALIVE_PACKET_FILTER_FROM_APF: {
                    if (mNetworkAgent != null) {
                        mNetworkAgent.sendSocketKeepaliveEvent(message.arg1,
                                SocketKeepalive.ERROR_INVALID_NETWORK);
                    }
                    break;
                }
                case CMD_INSTALL_PACKET_FILTER: {
                    mCachedPacketFilter = (byte[]) message.obj;
                    if (mContext.getResources().getBoolean(
                            R.bool.config_wifiEnableApfOnNonPrimarySta)
                            || isPrimary()) {
                        mWifiNative.installPacketFilter(mInterfaceName, mCachedPacketFilter);
                    } else {
                        Log.v(TAG, "Not applying packet filter on non primary CMM");
                    }
                    break;
                }
                case CMD_READ_PACKET_FILTER: {
                    final byte[] packetFilter;
                    if (mContext.getResources().getBoolean(
                            R.bool.config_wifiEnableApfOnNonPrimarySta)
                            || isPrimary()) {
                        packetFilter = mWifiNative.readPacketFilter(mInterfaceName);
                    } else {
                        Log.v(TAG, "Retrieving cached packet filter on non primary CMM");
                        packetFilter = mCachedPacketFilter;
                    }
                    if (mIpClient != null) {
                        mIpClient.readPacketFilterComplete(packetFilter);
                    }
                    break;
                }
                case CMD_SET_FALLBACK_PACKET_FILTERING: {
                    if ((boolean) message.obj) {
                        mWifiNative.startFilteringMulticastV4Packets(mInterfaceName);
                    } else {
                        mWifiNative.stopFilteringMulticastV4Packets(mInterfaceName);
                    }
                    break;
                }
                case CMD_DIAGS_CONNECT_TIMEOUT: {
                    mWifiDiagnostics.reportConnectionEvent(
                            WifiDiagnostics.CONNECTION_EVENT_TIMEOUT, mClientModeManager);
                    break;
                }
                case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                case CMD_RESET_SIM_NETWORKS:
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                case CMD_RSSI_POLL:
                case CMD_ONESHOT_RSSI_POLL:
                case CMD_PRE_DHCP_ACTION:
                case CMD_PRE_DHCP_ACTION_COMPLETE:
                case CMD_POST_DHCP_ACTION:
                case WifiMonitor.SUP_REQUEST_IDENTITY:
                case WifiMonitor.SUP_REQUEST_SIM_AUTH:
                case WifiMonitor.TARGET_BSSID_EVENT:
                case WifiMonitor.ASSOCIATED_BSSID_EVENT:
                case WifiMonitor.TRANSITION_DISABLE_INDICATION:
                case CMD_UNWANTED_NETWORK:
                case CMD_CONNECTING_WATCHDOG_TIMER:
                case WifiMonitor.NETWORK_NOT_FOUND_EVENT:
                case CMD_ROAM_WATCHDOG_TIMER: {
                    // no-op: all messages must be handled in the base state in case it was missed
                    // in one of the child states.
                    break;
                }
                case CMD_START_ROAM:
                case CMD_START_RSSI_MONITORING_OFFLOAD:
                case CMD_STOP_RSSI_MONITORING_OFFLOAD:
                case CMD_IP_CONFIGURATION_SUCCESSFUL:
                case CMD_IP_CONFIGURATION_LOST:
                case CMD_IP_REACHABILITY_LOST: {
                    mMessageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                }
                case 0: {
                    // We want to notice any empty messages (with what == 0) that might crop up.
                    // For example, we may have recycled a message sent to multiple handlers.
                    Log.wtf(getTag(), "Error! empty message encountered");
                    break;
                }
                default: {
                    loge("Error! unhandled message" + message);
                    break;
                }
            }

            logStateAndMessage(message, this);
            return HANDLED;
        }
    }

    private boolean handleL3MessagesWhenNotConnected(Message message) {
        boolean handleStatus = HANDLED;

        if (!mIpClientWithPreConnection) {
            return NOT_HANDLED;
        }

        switch (message.what) {
            case CMD_PRE_DHCP_ACTION:
                handlePreDhcpSetup();
                break;
            case CMD_PRE_DHCP_ACTION_COMPLETE:
                if (mIpClient != null) {
                    mIpClient.completedPreDhcpAction();
                }
                break;
            case CMD_IPV4_PROVISIONING_FAILURE:
                stopDhcpSetup();
                deferMessage(message);
                break;
            case CMD_POST_DHCP_ACTION:
            case CMD_IPV4_PROVISIONING_SUCCESS:
            case CMD_IP_CONFIGURATION_SUCCESSFUL:
                deferMessage(message);
                break;
            default:
                return NOT_HANDLED;
        }

        return handleStatus;
    }

    private WifiNetworkAgentSpecifier createNetworkAgentSpecifier(
            @NonNull WifiConfiguration currentWifiConfiguration, @Nullable String currentBssid,
            boolean matchLocationSensitiveInformation) {
        // Defensive copy to avoid mutating the passed argument
        final WifiConfiguration conf = new WifiConfiguration(currentWifiConfiguration);
        conf.BSSID = currentBssid;
        WifiNetworkAgentSpecifier wns =
                new WifiNetworkAgentSpecifier(conf,
                        WifiNetworkSpecifier.getBand(mWifiInfo.getFrequency()),
                        matchLocationSensitiveInformation);
        return wns;
    }

    private NetworkCapabilities getCapabilities(
            WifiConfiguration currentWifiConfiguration, String currentBssid) {
        final NetworkCapabilities.Builder builder =
                new NetworkCapabilities.Builder(mNetworkCapabilitiesFilter);
        // MatchAllNetworkSpecifier set in the mNetworkCapabilitiesFilter should never be set in the
        // agent's specifier.
        builder.setNetworkSpecifier(null);
        if (currentWifiConfiguration == null) {
            return builder.build();
        }

        if (mWifiInfo.isTrusted()) {
            builder.addCapability(NetworkCapabilities.NET_CAPABILITY_TRUSTED);
        } else {
            builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_TRUSTED);
        }
        if (SdkLevel.isAtLeastS()) {
            if (mWifiInfo.isOemPaid()) {
                builder.addCapability(NetworkCapabilities.NET_CAPABILITY_OEM_PAID);
                builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED);
            } else {
                builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_OEM_PAID);
            }
            if (mWifiInfo.isOemPrivate()) {
                builder.addCapability(NetworkCapabilities.NET_CAPABILITY_OEM_PRIVATE);
                builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED);
            } else {
                builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_OEM_PRIVATE);
            }
        }

        builder.setOwnerUid(currentWifiConfiguration.creatorUid);
        builder.setAdministratorUids(new int[] {currentWifiConfiguration.creatorUid});

        if (!WifiConfiguration.isMetered(currentWifiConfiguration, mWifiInfo)) {
            builder.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
        } else {
            builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
        }

        if (mWifiInfo.getRssi() != WifiInfo.INVALID_RSSI) {
            builder.setSignalStrength(mWifiInfo.getRssi());
        } else {
            builder.setSignalStrength(NetworkCapabilities.SIGNAL_STRENGTH_UNSPECIFIED);
        }

        if (currentWifiConfiguration.osu) {
            builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        }

        if (!mWifiInfo.getSSID().equals(WifiManager.UNKNOWN_SSID)) {
            builder.setSsid(mWifiInfo.getSSID());
        }

        // Only send out WifiInfo in >= Android S devices.
        if (SdkLevel.isAtLeastS()) {
            builder.setTransportInfo(new WifiInfo(mWifiInfo));

            if (mWifiInfo.getSubscriptionId() != SubscriptionManager.INVALID_SUBSCRIPTION_ID
                    && mWifiInfo.isCarrierMerged()) {
                builder.setSubscriptionIds(Collections.singleton(mWifiInfo.getSubscriptionId()));
            }
        }

        Pair<Integer, String> specificRequestUidAndPackageName =
                mNetworkFactory.getSpecificNetworkRequestUidAndPackageName(
                        currentWifiConfiguration, currentBssid);
        // There is an active specific request.
        if (specificRequestUidAndPackageName.first != Process.INVALID_UID) {
            // Remove internet capability.
            builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
            // Fill up the uid/packageName for this connection.
            builder.setRequestorUid(specificRequestUidAndPackageName.first);
            builder.setRequestorPackageName(specificRequestUidAndPackageName.second);
            // Fill up the network agent specifier for this connection, allowing NetworkCallbacks
            // to match local-only specifiers in requests. TODO(b/187921303): a third-party app can
            // observe this location-sensitive information by registering a NetworkCallback.
            builder.setNetworkSpecifier(createNetworkAgentSpecifier(currentWifiConfiguration,
                    getConnectedBssidInternal(), true /* matchLocalOnlySpecifiers */));
        } else {
            // Fill up the network agent specifier for this connection, without allowing
            // NetworkCallbacks to match local-only specifiers in requests.
            builder.setNetworkSpecifier(createNetworkAgentSpecifier(currentWifiConfiguration,
                    getConnectedBssidInternal(), false /* matchLocalOnlySpecifiers */));
        }

        updateLinkBandwidth(builder);
        final NetworkCapabilities networkCapabilities = builder.build();
        if (mVcnManager == null || !currentWifiConfiguration.carrierMerged) {
            return networkCapabilities;
        }
        final VcnNetworkPolicyResult vcnNetworkPolicy =
                mVcnManager.applyVcnNetworkPolicy(networkCapabilities, mLinkProperties);
        if (vcnNetworkPolicy.isTeardownRequested()) {
            sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_VCN_REQUEST);
        }
        final NetworkCapabilities vcnCapability = vcnNetworkPolicy.getNetworkCapabilities();
        if (!vcnCapability.hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VCN_MANAGED)) {
            if (mVerboseLoggingEnabled) {
                logd("NET_CAPABILITY_NOT_VCN_MANAGED is removed");
            }
            builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VCN_MANAGED);
        }
        if (!vcnCapability.hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED)) {
            if (mVerboseLoggingEnabled) {
                logd("NET_CAPABILITY_NOT_RESTRICTED is removed");
            }
            builder.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED);
        }
        return builder.build();
    }

    private void updateLinkBandwidth(NetworkCapabilities.Builder networkCapabilitiesBuilder) {
        int txTputKbps = 0;
        int rxTputKbps = 0;
        int currRssi = mWifiInfo.getRssi();
        if (currRssi != WifiInfo.INVALID_RSSI) {
            WifiScoreCard.PerNetwork network = mWifiScoreCard.lookupNetwork(mWifiInfo.getSSID());
            txTputKbps = network.getTxLinkBandwidthKbps();
            rxTputKbps = network.getRxLinkBandwidthKbps();
        } else {
            // Fall back to max link speed. This should rarely happen if ever
            int maxTxLinkSpeedMbps = mWifiInfo.getMaxSupportedTxLinkSpeedMbps();
            int maxRxLinkSpeedMbps = mWifiInfo.getMaxSupportedRxLinkSpeedMbps();
            txTputKbps = maxTxLinkSpeedMbps * 1000;
            rxTputKbps = maxRxLinkSpeedMbps * 1000;
        }
        if (mVerboseLoggingEnabled) {
            logd("reported txKbps " + txTputKbps + " rxKbps " + rxTputKbps);
        }
        if (txTputKbps > 0) {
            networkCapabilitiesBuilder.setLinkUpstreamBandwidthKbps(txTputKbps);
        }
        if (rxTputKbps > 0) {
            networkCapabilitiesBuilder.setLinkDownstreamBandwidthKbps(rxTputKbps);
        }
    }

    /**
     * Method to update network capabilities from the current WifiConfiguration.
     */
    public void updateCapabilities() {
        updateCapabilities(getConnectedWifiConfigurationInternal());
    }

    private void updateCapabilities(WifiConfiguration currentWifiConfiguration) {
        updateCapabilities(getCapabilities(currentWifiConfiguration, getConnectedBssidInternal()));
    }

    private void updateCapabilities(NetworkCapabilities networkCapabilities) {
        if (mNetworkAgent == null) {
            return;
        }
        mNetworkAgent.sendNetworkCapabilitiesAndCache(networkCapabilities);
    }

    private void handleEapAuthFailure(int networkId, int errorCode) {
        WifiConfiguration targetedNetwork =
                mWifiConfigManager.getConfiguredNetwork(mTargetNetworkId);
        if (targetedNetwork != null) {
            switch (targetedNetwork.enterpriseConfig.getEapMethod()) {
                case WifiEnterpriseConfig.Eap.SIM:
                case WifiEnterpriseConfig.Eap.AKA:
                case WifiEnterpriseConfig.Eap.AKA_PRIME:
                    if (errorCode == WifiNative.EAP_SIM_VENDOR_SPECIFIC_CERT_EXPIRED) {
                        mWifiCarrierInfoManager.resetCarrierKeysForImsiEncryption(targetedNetwork);
                    }
                    break;

                default:
                    // Do Nothing
            }
        }
    }

    /**
     * All callbacks are triggered on the main Wifi thread.
     * See {@link WifiNetworkAgent#WifiNetworkAgent}'s looper argument in
     * {@link WifiInjector#makeWifiNetworkAgent}.
     */
    private class WifiNetworkAgentCallback implements WifiNetworkAgent.Callback {
        private int mLastNetworkStatus = -1; // To detect when the status really changes

        private boolean isThisCallbackActive() {
            return mNetworkAgent != null && mNetworkAgent.getCallback() == this;
        }

        @Override
        public void onNetworkUnwanted() {
            // Ignore if we're not the current networkAgent.
            if (!isThisCallbackActive()) return;
            if (mVerboseLoggingEnabled) {
                logd("WifiNetworkAgent -> Wifi unwanted score " + mWifiInfo.getScore());
            }
            unwantedNetwork(NETWORK_STATUS_UNWANTED_DISCONNECT);
        }

        @Override
        public void onValidationStatus(int status, @Nullable Uri redirectUri) {
            if (!isThisCallbackActive()) return;
            if (status == mLastNetworkStatus) return;
            mLastNetworkStatus = status;
            if (status == NetworkAgent.VALIDATION_STATUS_NOT_VALID) {
                if (mVerboseLoggingEnabled) {
                    logd("WifiNetworkAgent -> Wifi networkStatus invalid, score="
                            + mWifiInfo.getScore());
                }
                unwantedNetwork(NETWORK_STATUS_UNWANTED_VALIDATION_FAILED);
            } else if (status == NetworkAgent.VALIDATION_STATUS_VALID) {
                if (mVerboseLoggingEnabled) {
                    logd("WifiNetworkAgent -> Wifi networkStatus valid, score= "
                            + mWifiInfo.getScore());
                }
                mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_NETWORK_AGENT_VALID_NETWORK);
                doNetworkStatus(status);
            }
            boolean captivePortalDetected = redirectUri != null
                    && redirectUri.toString() != null
                    && redirectUri.toString().length() > 0;
            if (captivePortalDetected) {
                Log.i(getTag(), "Captive Portal detected, status=" + status
                        + ", redirectUri=" + redirectUri);
                mWifiConfigManager.noteCaptivePortalDetected(mWifiInfo.getNetworkId());
                mCmiMonitor.onCaptivePortalDetected(mClientModeManager);
            }
        }

        @Override
        public void onSaveAcceptUnvalidated(boolean accept) {
            if (!isThisCallbackActive()) return;
            ClientModeImpl.this.sendMessage(CMD_ACCEPT_UNVALIDATED, accept ? 1 : 0);
        }

        @Override
        public void onStartSocketKeepalive(int slot, @NonNull Duration interval,
                @NonNull KeepalivePacketData packet) {
            if (!isThisCallbackActive()) return;
            ClientModeImpl.this.sendMessage(
                    CMD_START_IP_PACKET_OFFLOAD, slot, (int) interval.getSeconds(), packet);
        }

        @Override
        public void onStopSocketKeepalive(int slot) {
            if (!isThisCallbackActive()) return;
            ClientModeImpl.this.sendMessage(CMD_STOP_IP_PACKET_OFFLOAD, slot);
        }

        @Override
        public void onAddKeepalivePacketFilter(int slot, @NonNull KeepalivePacketData packet) {
            if (!isThisCallbackActive()) return;
            ClientModeImpl.this.sendMessage(
                    CMD_ADD_KEEPALIVE_PACKET_FILTER_TO_APF, slot, 0, packet);
        }

        @Override
        public void onRemoveKeepalivePacketFilter(int slot) {
            if (!isThisCallbackActive()) return;
            ClientModeImpl.this.sendMessage(CMD_REMOVE_KEEPALIVE_PACKET_FILTER_FROM_APF, slot);
        }

        @Override
        public void onSignalStrengthThresholdsUpdated(@NonNull int[] thresholds) {
            if (!isThisCallbackActive()) return;
            // 0. If there are no thresholds, or if the thresholds are invalid,
            //    stop RSSI monitoring.
            // 1. Tell the hardware to start RSSI monitoring here, possibly adding MIN_VALUE and
            //    MAX_VALUE at the start/end of the thresholds array if necessary.
            // 2. Ensure that when the hardware event fires, we fetch the RSSI from the hardware
            //    event, call mWifiInfo.setRssi() with it, and call updateCapabilities(), and then
            //    re-arm the hardware event. This needs to be done on the state machine thread to
            //    avoid race conditions. The RSSI used to re-arm the event (and perhaps also the one
            //    sent in the NetworkCapabilities) must be the one received from the hardware event
            //    received, or we might skip callbacks.
            // 3. Ensure that when we disconnect, RSSI monitoring is stopped.
            logd("Received signal strength thresholds: " + Arrays.toString(thresholds));
            if (thresholds.length == 0) {
                ClientModeImpl.this.sendMessage(CMD_STOP_RSSI_MONITORING_OFFLOAD,
                        mWifiInfo.getRssi());
                return;
            }
            int [] rssiVals = Arrays.copyOf(thresholds, thresholds.length + 2);
            rssiVals[rssiVals.length - 2] = Byte.MIN_VALUE;
            rssiVals[rssiVals.length - 1] = Byte.MAX_VALUE;
            Arrays.sort(rssiVals);
            byte[] rssiRange = new byte[rssiVals.length];
            for (int i = 0; i < rssiVals.length; i++) {
                int val = rssiVals[i];
                if (val <= Byte.MAX_VALUE && val >= Byte.MIN_VALUE) {
                    rssiRange[i] = (byte) val;
                } else {
                    Log.e(getTag(), "Illegal value " + val + " for RSSI thresholds: "
                            + Arrays.toString(rssiVals));
                    ClientModeImpl.this.sendMessage(CMD_STOP_RSSI_MONITORING_OFFLOAD,
                            mWifiInfo.getRssi());
                    return;
                }
            }
            // TODO: Do we quash rssi values in this sorted array which are very close?
            mRssiRanges = rssiRange;
            ClientModeImpl.this.sendMessage(CMD_START_RSSI_MONITORING_OFFLOAD,
                    mWifiInfo.getRssi());
        }

        @Override
        public void onAutomaticReconnectDisabled() {
            if (!isThisCallbackActive()) return;
            unwantedNetwork(NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN);
        }
    }

    private void unwantedNetwork(int reason) {
        sendMessage(CMD_UNWANTED_NETWORK, reason);
    }

    private void doNetworkStatus(int status) {
        sendMessage(CMD_NETWORK_STATUS, status);
    }

    class ConnectingOrConnectedState extends State {
        @Override
        public void enter() {
            if (mVerboseLoggingEnabled) Log.v(getTag(), "Entering ConnectingOrConnectedState");
            mCmiMonitor.onConnectionStart(mClientModeManager);
        }

        @Override
        public void exit() {
            if (mVerboseLoggingEnabled) Log.v(getTag(), "Exiting ConnectingOrConnectedState");
            mCmiMonitor.onConnectionEnd(mClientModeManager);

            // Not connected/connecting to any network:
            // 1. Disable the network in supplicant to prevent it from auto-connecting. We don't
            // remove the network to avoid losing any cached info in supplicant (reauth, etc) in
            // case we reconnect back to the same network.
            // 2. Set a random MAC address to ensure that we're not leaking the MAC address.
            mWifiNative.disableNetwork(mInterfaceName);
            if (mWifiGlobals.isConnectedMacRandomizationEnabled()) {
                if (!mWifiNative.setStaMacAddress(
                        mInterfaceName, MacAddressUtils.createRandomUnicastAddress())) {
                    Log.e(getTag(), "Failed to set random MAC address on disconnect");
                }
            }
            mWifiInfo.reset();
            mWifiInfo.setSupplicantState(SupplicantState.DISCONNECTED);
            mWifiScoreCard.noteSupplicantStateChanged(mWifiInfo);

            // For secondary client roles, they should stop themselves upon disconnection.
            // - Primary role shouldn't because it is persistent, and should try connecting to other
            //   networks upon disconnection.
            // - ROLE_CLIENT_LOCAL_ONLY shouldn't because it has auto-retry logic if the connection
            //   fails. WifiNetworkFactory will explicitly remove the CMM when the request is
            //   complete.
            // TODO(b/160346062): Maybe clean this up by having ClientModeManager register a
            //  onExitConnectingOrConnectedState() callback with ClientModeImpl and implementing
            //  this logic in ClientModeManager. ClientModeImpl should be role-agnostic.
            ClientRole role = mClientModeManager.getRole();
            if (role == ROLE_CLIENT_SECONDARY_LONG_LIVED
                    || role == ROLE_CLIENT_SECONDARY_TRANSIENT) {
                if (mVerboseLoggingEnabled) {
                    Log.d(getTag(), "Disconnected in ROLE_CLIENT_SECONDARY_*, "
                            + "stop ClientModeManager=" + mClientModeManager);
                }
                // stop owner ClientModeManager, which will in turn stop this ClientModeImpl
                mClientModeManager.stop();
            }
        }

        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;
            switch (message.what) {
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT: {
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    SupplicantState state = handleSupplicantStateChange(stateChangeResult);
                    // Supplicant can fail to report a NETWORK_DISCONNECTION_EVENT
                    // when authentication times out after a successful connection,
                    // we can figure this from the supplicant state. If supplicant
                    // state is DISCONNECTED, but the agent is not disconnected, we
                    // need to handle a disconnection
                    if (mVerboseLoggingEnabled) {
                        log("ConnectingOrConnectedState: Supplicant State change "
                                + stateChangeResult);
                    }
                    if (state == SupplicantState.DISCONNECTED && mNetworkAgent != null) {
                        if (mVerboseLoggingEnabled) {
                            log("Missed CTRL-EVENT-DISCONNECTED, disconnect");
                        }
                        handleNetworkDisconnect(false,
                                WIFI_DISCONNECT_REPORTED__FAILURE_CODE__SUPPLICANT_DISCONNECTED);
                        transitionTo(mDisconnectedState);
                    }
                    if (state == SupplicantState.COMPLETED) {
                        mWifiScoreReport.noteIpCheck();
                    }
                    break;
                }
                case WifiMonitor.ASSOCIATED_BSSID_EVENT: {
                    // This is where we can confirm the connection BSSID. Use it to find the
                    // right ScanDetail to populate metrics.
                    String someBssid = (String) message.obj;
                    if (someBssid != null) {
                        // Get the ScanDetail associated with this BSSID.
                        ScanDetailCache scanDetailCache =
                                mWifiConfigManager.getScanDetailCacheForNetwork(mTargetNetworkId);
                        if (scanDetailCache != null) {
                            mWifiMetrics.setConnectionScanDetail(mInterfaceName,
                                    scanDetailCache.getScanDetail(someBssid));
                        }
                        // Update last associated BSSID
                        mLastBssid = someBssid;
                    }
                    handleStatus = NOT_HANDLED;
                    break;
                }
                case WifiMonitor.NETWORK_CONNECTION_EVENT: {
                    if (mVerboseLoggingEnabled) log("Network connection established");
                    NetworkConnectionEventInfo connectionInfo =
                            (NetworkConnectionEventInfo) message.obj;
                    mLastNetworkId = connectionInfo.networkId;
                    mSentHLPs = connectionInfo.isFilsConnection;
                    if (mSentHLPs) mWifiMetrics.incrementL2ConnectionThroughFilsAuthCount();
                    mWifiConfigManager.clearRecentFailureReason(mLastNetworkId);
                    mLastBssid = connectionInfo.bssid;
                    // TODO: This check should not be needed after ClientModeImpl refactor.
                    // Currently, the last connected network configuration is left in
                    // wpa_supplicant, this may result in wpa_supplicant initiating connection
                    // to it after a config store reload. Hence the old network Id lookups may not
                    // work, so disconnect the network and let network selector reselect a new
                    // network.
                    WifiConfiguration config = getConnectedWifiConfigurationInternal();
                    if (config == null) {
                        logw("Connected to unknown networkId " + mLastNetworkId
                                + ", disconnecting...");
                        sendMessage(CMD_DISCONNECT, StaEvent.DISCONNECT_UNKNOWN_NETWORK);
                        break;
                    }
                    mWifiInfo.setBSSID(mLastBssid);
                    mWifiInfo.setNetworkId(mLastNetworkId);
                    mWifiInfo.setMacAddress(mWifiNative.getMacAddress(mInterfaceName));

                    ScanDetailCache scanDetailCache =
                            mWifiConfigManager.getScanDetailCacheForNetwork(config.networkId);
                    ScanResult scanResult = null;
                    if (scanDetailCache != null && mLastBssid != null) {
                        scanResult = scanDetailCache.getScanResult(mLastBssid);
                        if (scanResult != null) {
                            mWifiInfo.setFrequency(scanResult.frequency);
                        }
                    }

                    // We need to get the updated pseudonym from supplicant for EAP-SIM/AKA/AKA'
                    if (config.enterpriseConfig != null
                            && config.enterpriseConfig.isAuthenticationSimBased()) {
                        mLastSubId = mWifiCarrierInfoManager.getBestMatchSubscriptionId(config);
                        mLastSimBasedConnectionCarrierName =
                                mWifiCarrierInfoManager.getCarrierNameForSubId(mLastSubId);
                        String anonymousIdentity =
                                mWifiNative.getEapAnonymousIdentity(mInterfaceName);
                        if (!TextUtils.isEmpty(anonymousIdentity)
                                && !WifiCarrierInfoManager
                                .isAnonymousAtRealmIdentity(anonymousIdentity)) {
                            String decoratedPseudonym = mWifiCarrierInfoManager
                                    .decoratePseudonymWith3GppRealm(config,
                                            anonymousIdentity);
                            if (decoratedPseudonym != null) {
                                anonymousIdentity = decoratedPseudonym;
                            }
                            if (mVerboseLoggingEnabled) {
                                log("EAP Pseudonym: " + anonymousIdentity);
                            }
                            // Save the pseudonym only if it is a real one
                            config.enterpriseConfig.setAnonymousIdentity(anonymousIdentity);
                        } else {
                            // Clear any stored pseudonyms
                            config.enterpriseConfig.setAnonymousIdentity(null);
                        }
                        mWifiConfigManager.addOrUpdateNetwork(config, Process.WIFI_UID);
                        if (config.isPasspoint()) {
                            mPasspointManager.setAnonymousIdentity(config);
                        } else if (config.fromWifiNetworkSuggestion) {
                            mWifiNetworkSuggestionsManager.setAnonymousIdentity(config);
                        }
                    }
                    // When connecting to Passpoint, ask for the Venue URL
                    if (config.isPasspoint()) {
                        mTermsAndConditionsUrl = null;
                        if (scanResult == null && mLastBssid != null) {
                            // The cached scan result of connected network would be null at the
                            // first connection, try to check full scan result list again to look up
                            // matched scan result associated to the current SSID and BSSID.
                            scanResult = mScanRequestProxy.getScanResult(mLastBssid);
                        }
                        if (scanResult != null) {
                            mPasspointManager.requestVenueUrlAnqpElement(scanResult);
                        }
                    }
                    transitionTo(mL3ProvisioningState);
                    break;
                }
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT: {
                    DisconnectEventInfo eventInfo = (DisconnectEventInfo) message.obj;
                    if (mVerboseLoggingEnabled) {
                        log("ConnectingOrConnectedState: Network disconnection " + eventInfo);
                    }
                    if (eventInfo.reasonCode == ReasonCode.FOURWAY_HANDSHAKE_TIMEOUT) {
                        String bssid = !isValidBssid(eventInfo.bssid)
                                ? mTargetBssid : eventInfo.bssid;
                        mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(
                                getConnectingSsidInternal(), bssid,
                                WifiLastResortWatchdog.FAILURE_CODE_AUTHENTICATION,
                                isConnected());
                    }
                    clearNetworkCachedDataIfNeeded(
                            getConnectingWifiConfigurationInternal(), eventInfo.reasonCode);
                    String targetSsid = getConnectingSsidInternal();
                    // If network is removed while connecting, targetSsid can be null.
                    boolean newConnectionInProgress =
                            targetSsid != null && !eventInfo.ssid.equals(targetSsid);
                    if (!newConnectionInProgress) {
                        int level2FailureReason = eventInfo.locallyGenerated
                                ? WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN :
                                WifiMetricsProto.ConnectionEvent.DISCONNECTION_NON_LOCAL;
                        if (!eventInfo.locallyGenerated) {
                            mWifiScoreCard.noteNonlocalDisconnect(
                                    mInterfaceName, eventInfo.reasonCode);
                        }
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE, level2FailureReason);
                    }
                    handleNetworkDisconnect(newConnectionInProgress, eventInfo.reasonCode);
                    if (!newConnectionInProgress) {
                        transitionTo(mDisconnectedState);
                    }
                    mTermsAndConditionsUrl = null;
                    break;
                }
                case WifiMonitor.TARGET_BSSID_EVENT: {
                    // Trying to associate to this BSSID
                    if (message.obj != null) {
                        mTargetBssid = (String) message.obj;
                    }
                    break;
                }
                case CMD_DISCONNECT: {
                    mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                            message.arg1);
                    mWifiNative.disconnect(mInterfaceName);
                    break;
                }
                case CMD_PRE_DHCP_ACTION:
                case CMD_PRE_DHCP_ACTION_COMPLETE:
                case CMD_POST_DHCP_ACTION:
                case CMD_IPV4_PROVISIONING_SUCCESS:
                case CMD_IP_CONFIGURATION_SUCCESSFUL:
                case CMD_IPV4_PROVISIONING_FAILURE: {
                    handleStatus = handleL3MessagesWhenNotConnected(message);
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }
            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }
            return handleStatus;
        }
    }

    class L2ConnectingState extends State {
        @Override
        public void enter() {
            if (mVerboseLoggingEnabled) Log.v(getTag(), "Entering L2ConnectingState");
            // Make sure we connect: we enter this state prior to connecting to a new
            // network. In some cases supplicant ignores the connect requests (it might not
            // find the target SSID in its cache), Therefore we end up stuck that state, hence the
            // need for the watchdog.
            mConnectingWatchdogCount++;
            logd("Start Connecting Watchdog " + mConnectingWatchdogCount);
            sendMessageDelayed(obtainMessage(CMD_CONNECTING_WATCHDOG_TIMER,
                    mConnectingWatchdogCount, 0), CONNECTING_WATCHDOG_TIMEOUT_MS);
        }

        @Override
        public void exit() {
            if (mVerboseLoggingEnabled) Log.v(getTag(), "Exiting L2ConnectingState");
        }

        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;
            switch (message.what) {
                case WifiMonitor.NETWORK_NOT_FOUND_EVENT:
                    mNetworkNotFoundEventCount++;
                    String networkName = (String) message.obj;
                    if (networkName != null && !networkName.equals(getConnectingSsidInternal())) {
                        loge("Network not found event received, network: " + networkName
                                + " which is not the target network: "
                                + getConnectingSsidInternal());
                        break;
                    }
                    Log.d(getTag(), "Network not found event received: network: " + networkName);
                    if (mNetworkNotFoundEventCount >= NETWORK_NOT_FOUND_EVENT_THRESHOLD
                            && mTargetWifiConfiguration != null
                            && mTargetWifiConfiguration.SSID != null
                            && mTargetWifiConfiguration.SSID.equals(networkName)) {
                        stopIpClient();
                        mWifiConfigManager.updateNetworkSelectionStatus(
                                mTargetWifiConfiguration.networkId,
                                WifiConfiguration.NetworkSelectionStatus
                                        .DISABLED_NETWORK_NOT_FOUND);
                        if (SdkLevel.isAtLeastS()) {
                            mWifiConfigManager.setRecentFailureAssociationStatus(
                                    mTargetWifiConfiguration.networkId,
                                    WifiConfiguration.RECENT_FAILURE_NETWORK_NOT_FOUND);
                        }
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_NETWORK_NOT_FOUND,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE,
                                WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
                        transitionTo(mDisconnectedState); // End of connection attempt.
                    }
                    break;
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT: {
                    AssocRejectEventInfo assocRejectEventInfo = (AssocRejectEventInfo) message.obj;
                    log("L2ConnectingState: Association rejection " + assocRejectEventInfo);
                    if (!assocRejectEventInfo.ssid.equals(getConnectingSsidInternal())) {
                        loge("Association rejection event received on not target network");
                        break;
                    }
                    stopIpClient();
                    mWifiDiagnostics.triggerBugReportDataCapture(
                            WifiDiagnostics.REPORT_REASON_ASSOC_FAILURE);
                    String bssid = assocRejectEventInfo.bssid;
                    boolean timedOut = assocRejectEventInfo.timedOut;
                    int statusCode = assocRejectEventInfo.statusCode;
                    if (!isValidBssid(bssid)) {
                        // If BSSID is null, use the target roam BSSID
                        bssid = mTargetBssid;
                    } else if (SUPPLICANT_BSSID_ANY.equals(mTargetBssid)) {
                        // This is needed by WifiBlocklistMonitor to block continuously
                        // failing BSSIDs. Need to set here because mTargetBssid is currently
                        // not being set until association success.
                        mTargetBssid = bssid;
                    }
                    mWifiConfigManager.updateNetworkSelectionStatus(mTargetNetworkId,
                            WifiConfiguration.NetworkSelectionStatus
                                    .DISABLED_ASSOCIATION_REJECTION);
                    setAssociationRejectionStatusInConfig(mTargetNetworkId, assocRejectEventInfo);
                    int level2FailureReason =
                            WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN;
                    if (statusCode == StatusCode.AP_UNABLE_TO_HANDLE_NEW_STA
                            || statusCode == StatusCode.ASSOC_REJECTED_TEMPORARILY
                            || statusCode == StatusCode.DENIED_INSUFFICIENT_BANDWIDTH) {
                        level2FailureReason = WifiMetricsProto.ConnectionEvent
                                .ASSOCIATION_REJECTION_AP_UNABLE_TO_HANDLE_NEW_STA;
                    }
                    // If rejection occurred while Metrics is tracking a ConnectionEvent, end it.
                    reportConnectionAttemptEnd(
                            timedOut
                                    ? WifiMetrics.ConnectionEvent.FAILURE_ASSOCIATION_TIMED_OUT
                                    : WifiMetrics.ConnectionEvent.FAILURE_ASSOCIATION_REJECTION,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE,
                            level2FailureReason);
                    if (level2FailureReason != WifiMetricsProto.ConnectionEvent
                            .ASSOCIATION_REJECTION_AP_UNABLE_TO_HANDLE_NEW_STA) {
                        mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(
                                getConnectingSsidInternal(), bssid,
                                WifiLastResortWatchdog.FAILURE_CODE_ASSOCIATION,
                                isConnected());
                    }
                    transitionTo(mDisconnectedState); // End of connection attempt.
                    break;
                }
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT: {
                    stopIpClient();
                    mWifiDiagnostics.triggerBugReportDataCapture(
                            WifiDiagnostics.REPORT_REASON_AUTH_FAILURE);
                    int disableReason = WifiConfiguration.NetworkSelectionStatus
                            .DISABLED_AUTHENTICATION_FAILURE;
                    int reasonCode = message.arg1;
                    int errorCode = message.arg2;
                    log("L2ConnectingState: Authentication failure "
                            + " reason=" + reasonCode + " error=" + errorCode);
                    WifiConfiguration targetedNetwork =
                            mWifiConfigManager.getConfiguredNetwork(mTargetNetworkId);
                    // Check if this is a permanent wrong password failure.
                    if (isPermanentWrongPasswordFailure(mTargetNetworkId, reasonCode)) {
                        disableReason = WifiConfiguration.NetworkSelectionStatus
                                .DISABLED_BY_WRONG_PASSWORD;
                        if (targetedNetwork != null && isPrimary()) {
                            mWrongPasswordNotifier.onWrongPasswordError(targetedNetwork.SSID);
                        }
                    } else if (reasonCode == WifiManager.ERROR_AUTH_FAILURE_EAP_FAILURE) {
                        if (targetedNetwork != null && targetedNetwork.enterpriseConfig != null
                                && targetedNetwork.enterpriseConfig.isAuthenticationSimBased()) {
                            // only show EAP failure notification if primary
                            if (mEapFailureNotifier
                                    .onEapFailure(errorCode, targetedNetwork, isPrimary())) {
                                disableReason = WifiConfiguration.NetworkSelectionStatus
                                    .DISABLED_AUTHENTICATION_PRIVATE_EAP_ERROR;
                                mWifiBlocklistMonitor.loadCarrierConfigsForDisableReasonInfos();
                            }
                        }
                        handleEapAuthFailure(mTargetNetworkId, errorCode);
                        if (errorCode == WifiNative.EAP_SIM_NOT_SUBSCRIBED) {
                            disableReason = WifiConfiguration.NetworkSelectionStatus
                                    .DISABLED_AUTHENTICATION_NO_SUBSCRIPTION;
                        }
                    }
                    mWifiConfigManager.updateNetworkSelectionStatus(
                            mTargetNetworkId, disableReason);
                    mWifiConfigManager.clearRecentFailureReason(mTargetNetworkId);

                    //If failure occurred while Metrics is tracking a ConnnectionEvent, end it.
                    int level2FailureReason;
                    switch (reasonCode) {
                        case WifiManager.ERROR_AUTH_FAILURE_NONE:
                            level2FailureReason =
                                    WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_NONE;
                            break;
                        case WifiManager.ERROR_AUTH_FAILURE_TIMEOUT:
                            level2FailureReason =
                                    WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_TIMEOUT;
                            break;
                        case WifiManager.ERROR_AUTH_FAILURE_WRONG_PSWD:
                            level2FailureReason =
                                    WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_WRONG_PSWD;
                            break;
                        case WifiManager.ERROR_AUTH_FAILURE_EAP_FAILURE:
                            level2FailureReason =
                                    WifiMetricsProto.ConnectionEvent.AUTH_FAILURE_EAP_FAILURE;
                            break;
                        default:
                            level2FailureReason =
                                    WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN;
                            break;
                    }
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE,
                            level2FailureReason);
                    if (reasonCode != WifiManager.ERROR_AUTH_FAILURE_WRONG_PSWD && reasonCode
                            != WifiManager.ERROR_AUTH_FAILURE_EAP_FAILURE) {
                        mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(
                                getConnectingSsidInternal(),
                                (mLastBssid == null) ? mTargetBssid : mLastBssid,
                                WifiLastResortWatchdog.FAILURE_CODE_AUTHENTICATION,
                                isConnected());
                    }
                    transitionTo(mDisconnectedState); // End of connection attempt.
                    break;
                }
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT: {
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    if (SupplicantState.isConnecting(stateChangeResult.state)) {
                        WifiConfiguration config = mWifiConfigManager.getConfiguredNetwork(
                                stateChangeResult.networkId);
                        // Update Passpoint information before setNetworkDetailedState as
                        // WifiTracker monitors NETWORK_STATE_CHANGED_ACTION to update UI.
                        mWifiInfo.setFQDN(null);
                        mWifiInfo.setPasspointUniqueId(null);
                        mWifiInfo.setOsuAp(false);
                        mWifiInfo.setProviderFriendlyName(null);
                        if (config != null && (config.isPasspoint() || config.osu)) {
                            if (config.isPasspoint()) {
                                mWifiInfo.setFQDN(config.FQDN);
                                mWifiInfo.setPasspointUniqueId(config.getPasspointUniqueId());
                            } else {
                                mWifiInfo.setOsuAp(true);
                            }
                            mWifiInfo.setProviderFriendlyName(config.providerFriendlyName);
                        }
                    }
                    sendNetworkChangeBroadcast(
                            WifiInfo.getDetailedStateOf(stateChangeResult.state));
                    // Let the parent state handle the rest of the state changed.
                    handleStatus = NOT_HANDLED;
                    break;
                }
                case WifiMonitor.SUP_REQUEST_IDENTITY: {
                    int netId = message.arg2;
                    boolean identitySent = false;
                    // For SIM & AKA/AKA' EAP method Only, get identity from ICC
                    if (mTargetWifiConfiguration != null
                            && mTargetWifiConfiguration.networkId == netId
                            && mTargetWifiConfiguration.enterpriseConfig != null
                            && mTargetWifiConfiguration.enterpriseConfig
                            .isAuthenticationSimBased()) {
                        // Pair<identity, encrypted identity>
                        Pair<String, String> identityPair = mWifiCarrierInfoManager
                                .getSimIdentity(mTargetWifiConfiguration);
                        if (identityPair != null && identityPair.first != null) {
                            Log.i(getTag(), "SUP_REQUEST_IDENTITY: identityPair=["
                                    + ((identityPair.first.length() >= 7)
                                    ? identityPair.first.substring(0, 7 /* Prefix+PLMN ID */)
                                    + "****"
                                    : identityPair.first) + ", "
                                    + (!TextUtils.isEmpty(identityPair.second) ? identityPair.second
                                    : "<NONE>") + "]");
                            mWifiNative.simIdentityResponse(mInterfaceName, identityPair.first,
                                    identityPair.second);
                            identitySent = true;
                        } else {
                            Log.e(getTag(), "Unable to retrieve identity from Telephony");
                        }
                    }

                    if (!identitySent) {
                        // Supplicant lacks credentials to connect to that network, hence black list
                        String ssid = (String) message.obj;
                        if (mTargetWifiConfiguration != null && ssid != null
                                && mTargetWifiConfiguration.SSID != null
                                && mTargetWifiConfiguration.SSID.equals("\"" + ssid + "\"")) {
                            mWifiConfigManager.updateNetworkSelectionStatus(
                                    mTargetWifiConfiguration.networkId,
                                    WifiConfiguration.NetworkSelectionStatus
                                            .DISABLED_AUTHENTICATION_NO_CREDENTIALS);
                        }
                        mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                StaEvent.DISCONNECT_GENERIC);
                        mWifiNative.disconnect(mInterfaceName);
                    }
                    break;
                }
                case WifiMonitor.SUP_REQUEST_SIM_AUTH: {
                    logd("Received SUP_REQUEST_SIM_AUTH");
                    SimAuthRequestData requestData = (SimAuthRequestData) message.obj;
                    if (requestData != null) {
                        if (requestData.protocol == WifiEnterpriseConfig.Eap.SIM) {
                            handleGsmAuthRequest(requestData);
                        } else if (requestData.protocol == WifiEnterpriseConfig.Eap.AKA
                                || requestData.protocol == WifiEnterpriseConfig.Eap.AKA_PRIME) {
                            handle3GAuthRequest(requestData);
                        }
                    } else {
                        loge("Invalid SIM auth request");
                    }
                    break;
                }
                case CMD_CONNECTING_WATCHDOG_TIMER: {
                    if (mConnectingWatchdogCount == message.arg1) {
                        if (mVerboseLoggingEnabled) log("Connecting watchdog! -> disconnect");
                        handleNetworkDisconnect(false,
                                WifiStatsLog.WIFI_DISCONNECT_REPORTED__FAILURE_CODE__CONNECTING_WATCHDOG_TIMER);
                        transitionTo(mDisconnectedState);
                    }
                    break;
                }
                case WifiMonitor.TRANSITION_DISABLE_INDICATION: {
                    log("Received TRANSITION_DISABLE_INDICATION: networkId=" + message.arg1
                            + ", indication=" + message.arg2);
                    mWifiConfigManager.updateNetworkTransitionDisable(message.arg1, message.arg2);
                    break;
                }
                case WifiMonitor.NETWORK_CONNECTION_EVENT: {
                    NetworkConnectionEventInfo connectionInfo =
                            (NetworkConnectionEventInfo) message.obj;
                    String quotedOrHexConnectingSsid = getConnectingSsidInternal();
                    String quotedOrHexConnectedSsid = NativeUtil.encodeSsid(
                            NativeUtil.byteArrayToArrayList(connectionInfo.wifiSsid.getOctets()));
                    if (quotedOrHexConnectingSsid != null
                            && !quotedOrHexConnectingSsid.equals(quotedOrHexConnectedSsid)) {
                        // possibly a NETWORK_CONNECTION_EVENT for a successful roam on the previous
                        // network while connecting to a new network, ignore it.
                        Log.d(TAG, "Connecting to ssid=" + quotedOrHexConnectingSsid + ", but got "
                                + "NETWORK_CONNECTION_EVENT for ssid=" + quotedOrHexConnectedSsid
                                + ", ignoring");
                        break;
                    }
                    handleStatus = NOT_HANDLED;
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }
            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }
            return handleStatus;
        }
    }

    class L2ConnectedState extends State {
        class RssiEventHandler implements WifiNative.WifiRssiEventHandler {
            @Override
            public void onRssiThresholdBreached(byte curRssi) {
                if (mVerboseLoggingEnabled) {
                    Log.e(getTag(), "onRssiThresholdBreach event. Cur Rssi = " + curRssi);
                }
                sendMessage(CMD_RSSI_THRESHOLD_BREACHED, curRssi);
            }
        }

        RssiEventHandler mRssiEventHandler = new RssiEventHandler();

        @Override
        public void enter() {
            mRssiPollToken++;
            if (mEnableRssiPolling) {
                if (isPrimary()) {
                    mLinkProbeManager.resetOnNewConnection();
                }
                sendMessage(CMD_RSSI_POLL, mRssiPollToken, 0);
            }
            sendNetworkChangeBroadcast(DetailedState.CONNECTING);

            // If this network was explicitly selected by the user, evaluate whether to inform
            // ConnectivityService of that fact so the system can treat it appropriately.
            final WifiConfiguration config = getConnectedWifiConfigurationInternal();

            final boolean explicitlySelected;
            // Non primary CMMs is never user selected. This prevents triggering the No Internet
            // dialog for those networks, which is difficult to handle.
            if (isPrimary() && isRecentlySelectedByTheUser(config)) {
                // If explicitlySelected is true, the network was selected by the user via Settings
                // or QuickSettings. If this network has Internet access, switch to it. Otherwise,
                // switch to it only if the user confirms that they really want to switch, or has
                // already confirmed and selected "Don't ask again".
                explicitlySelected =
                        mWifiPermissionsUtil.checkNetworkSettingsPermission(config.lastConnectUid);
                if (mVerboseLoggingEnabled) {
                    log("Network selected by UID " + config.lastConnectUid + " explicitlySelected="
                            + explicitlySelected);
                }
            } else {
                explicitlySelected = false;
            }

            if (mVerboseLoggingEnabled) {
                log("explicitlySelected=" + explicitlySelected + " acceptUnvalidated="
                        + config.noInternetAccessExpected);
            }

            NetworkAgentConfig.Builder naConfigBuilder = new NetworkAgentConfig.Builder()
                    .setLegacyType(ConnectivityManager.TYPE_WIFI)
                    .setLegacyTypeName(NETWORKTYPE)
                    .setExplicitlySelected(explicitlySelected)
                    .setUnvalidatedConnectivityAcceptable(
                            explicitlySelected && config.noInternetAccessExpected)
                    .setPartialConnectivityAcceptable(config.noInternetAccessExpected);
            if (config.carrierMerged) {
                String subscriberId = null;
                TelephonyManager subMgr = mTelephonyManager.createForSubscriptionId(
                        config.subscriptionId);
                if (subMgr != null) {
                    subscriberId = subMgr.getSubscriberId();
                }
                if (subscriberId != null) {
                    naConfigBuilder.setSubscriberId(subscriberId);
                }
            }
            if (mVcnManager == null && SdkLevel.isAtLeastS()) {
                mVcnManager = mContext.getSystemService(VcnManager.class);
            }
            if (mVcnManager != null) {
                mVcnPolicyChangeListener = new WifiVcnNetworkPolicyChangeListener();
                mVcnManager.addVcnNetworkPolicyChangeListener(new HandlerExecutor(getHandler()),
                        mVcnPolicyChangeListener);
            }
            final NetworkAgentConfig naConfig = naConfigBuilder.build();
            final NetworkCapabilities nc = getCapabilities(
                    getConnectedWifiConfigurationInternal(), getConnectedBssidInternal());
            // This should never happen.
            if (mNetworkAgent != null) {
                Log.wtf(getTag(), "mNetworkAgent is not null: " + mNetworkAgent);
                mNetworkAgent.unregister();
            }
            mNetworkAgent = mWifiInjector.makeWifiNetworkAgent(nc, mLinkProperties, naConfig,
                    mNetworkFactory.getProvider(), new WifiNetworkAgentCallback());
            mWifiScoreReport.setNetworkAgent(mNetworkAgent);

            // We must clear the config BSSID, as the wifi chipset may decide to roam
            // from this point on and having the BSSID specified in the network block would
            // cause the roam to faile and the device to disconnect
            clearTargetBssid("L2ConnectedState");
            mWifiMetrics.setWifiState(mInterfaceName, WifiMetricsProto.WifiLog.WIFI_ASSOCIATED);
            mWifiScoreCard.noteNetworkAgentCreated(mWifiInfo,
                    mNetworkAgent.getNetwork().getNetId());
            mWifiBlocklistMonitor.handleBssidConnectionSuccess(mLastBssid, mWifiInfo.getSSID());
            // too many places to record connection failure with too many failure reasons.
            // So only record success here.
            mWifiMetrics.noteFirstL2ConnectionAfterBoot(true);
            mCmiMonitor.onL2Connected(mClientModeManager);
            mIsLinkedNetworkRoaming = false;
        }

        @Override
        public void exit() {
            // This is handled by receiving a NETWORK_DISCONNECTION_EVENT in ConnectableState
            // Bug: 15347363
            // For paranoia's sake, call handleNetworkDisconnect
            // only if BSSID is null or last networkId
            // is not invalid.
            if (mVerboseLoggingEnabled) {
                StringBuilder sb = new StringBuilder();
                sb.append("leaving L2ConnectedState state nid=" + Integer.toString(mLastNetworkId));
                if (mLastBssid != null) {
                    sb.append(" ").append(mLastBssid);
                }
            }
            mWifiMetrics.setWifiState(mInterfaceName, WifiMetricsProto.WifiLog.WIFI_DISCONNECTED);
            mWifiStateTracker.updateState(mInterfaceName, WifiStateTracker.DISCONNECTED);
            // Inform WifiLockManager
            mWifiLockManager.updateWifiClientConnected(mClientModeManager, false);
            mLastConnectionCapabilities = null;
        }

        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;

            switch (message.what) {
                case CMD_PRE_DHCP_ACTION: {
                    handlePreDhcpSetup();
                    break;
                }
                case CMD_PRE_DHCP_ACTION_COMPLETE: {
                    if (mIpClient != null) {
                        mIpClient.completedPreDhcpAction();
                    }
                    break;
                }
                case CMD_POST_DHCP_ACTION: {
                    handlePostDhcpSetup();
                    // We advance to mL3ConnectedState because IpClient will also send a
                    // CMD_IPV4_PROVISIONING_SUCCESS message, which calls handleIPv4Success(),
                    // which calls updateLinkProperties, which then sends
                    // CMD_IP_CONFIGURATION_SUCCESSFUL.
                    break;
                }
                case CMD_IPV4_PROVISIONING_SUCCESS: {
                    handleIPv4Success((DhcpResultsParcelable) message.obj);
                    sendNetworkChangeBroadcastWithCurrentState();
                    break;
                }
                case CMD_IPV4_PROVISIONING_FAILURE: {
                    handleIPv4Failure();
                    mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(
                            getConnectingSsidInternal(),
                            (mLastBssid == null) ? mTargetBssid : mLastBssid,
                            WifiLastResortWatchdog.FAILURE_CODE_DHCP,
                            isConnected());
                    break;
                }
                case CMD_IP_CONFIGURATION_SUCCESSFUL: {
                    if (getConnectedWifiConfigurationInternal() == null || mNetworkAgent == null) {
                        // The current config may have been removed while we were connecting,
                        // trigger a disconnect to clear up state.
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE,
                                WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
                        mWifiNative.disconnect(mInterfaceName);
                    } else {
                        handleSuccessfulIpConfiguration();
                        sendConnectedState();
                        transitionTo(mL3ConnectedState);
                    }
                    break;
                }
                case CMD_IP_CONFIGURATION_LOST: {
                    // Get Link layer stats so that we get fresh tx packet counters.
                    getWifiLinkLayerStats();
                    handleIpConfigurationLost();
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_DHCP,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE,
                            WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
                    mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(
                            getConnectingSsidInternal(),
                            (mLastBssid == null) ? mTargetBssid : mLastBssid,
                            WifiLastResortWatchdog.FAILURE_CODE_DHCP,
                            isConnected());
                    break;
                }
                case CMD_IP_REACHABILITY_LOST: {
                    if (mVerboseLoggingEnabled && message.obj != null) log((String) message.obj);
                    mWifiDiagnostics.triggerBugReportDataCapture(
                            WifiDiagnostics.REPORT_REASON_REACHABILITY_LOST);
                    mWifiMetrics.logWifiIsUnusableEvent(mInterfaceName,
                            WifiIsUnusableEvent.TYPE_IP_REACHABILITY_LOST);
                    mWifiMetrics.addToWifiUsabilityStatsList(mInterfaceName,
                            WifiUsabilityStats.LABEL_BAD,
                            WifiUsabilityStats.TYPE_IP_REACHABILITY_LOST, -1);
                    if (mWifiGlobals.getIpReachabilityDisconnectEnabled()) {
                        handleIpReachabilityLost();
                    } else {
                        logd("CMD_IP_REACHABILITY_LOST but disconnect disabled -- ignore");
                    }
                    break;
                }
                case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST: {
                    if (mWifiP2pConnection.shouldTemporarilyDisconnectWifi()) {
                        mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                StaEvent.DISCONNECT_P2P_DISCONNECT_WIFI_REQUEST);
                        mWifiNative.disconnect(mInterfaceName);
                    }
                    break;
                }
                case WifiMonitor.NETWORK_CONNECTION_EVENT: {
                    NetworkConnectionEventInfo connectionInfo =
                            (NetworkConnectionEventInfo) message.obj;
                    mWifiInfo.setBSSID(connectionInfo.bssid);
                    mLastNetworkId = connectionInfo.networkId;
                    mWifiInfo.setNetworkId(mLastNetworkId);
                    mWifiInfo.setMacAddress(mWifiNative.getMacAddress(mInterfaceName));
                    if (!Objects.equals(mLastBssid, connectionInfo.bssid)) {
                        mLastBssid = connectionInfo.bssid;
                        sendNetworkChangeBroadcastWithCurrentState();
                    }
                    if (mIsLinkedNetworkRoaming) {
                        mIsLinkedNetworkRoaming = false;
                        mTargetNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
                        mTargetWifiConfiguration = null;
                        clearTargetBssid("AllowlistRoamingCompleted");
                        sendNetworkChangeBroadcast(DetailedState.CONNECTED);
                    }
                    break;
                }
                case CMD_ONESHOT_RSSI_POLL: {
                    if (!mEnableRssiPolling) {
                        updateLinkLayerStatsRssiDataStallScoreReport();
                    }
                    break;
                }
                case CMD_RSSI_POLL: {
                    // TODO(b/179792830): getBSSID() shouldn't be null in L2ConnectedState,
                    //  add debug logs in the meantime. Remove once root cause identified.
                    if (mWifiInfo.getBSSID() == null) {
                        Log.wtf(getTag(), "WifiInfo.getBSSID() is null in L2ConnectedState!");
                        break;
                    }
                    if (message.arg1 == mRssiPollToken) {
                        updateLinkLayerStatsRssiDataStallScoreReport();
                        mWifiScoreCard.noteSignalPoll(mWifiInfo);
                        if (isPrimary()) {
                            mLinkProbeManager.updateConnectionStats(mWifiInfo, mInterfaceName);
                        }
                        sendMessageDelayed(obtainMessage(CMD_RSSI_POLL, mRssiPollToken, 0),
                                mWifiGlobals.getPollRssiIntervalMillis());
                        if (mVerboseLoggingEnabled) sendRssiChangeBroadcast(mWifiInfo.getRssi());
                        if (isPrimary()) {
                            mWifiTrafficPoller.notifyOnDataActivity(
                                    mWifiInfo.txSuccess, mWifiInfo.rxSuccess);
                        }
                    } else {
                        // Polling has completed
                    }
                    break;
                }
                case CMD_ENABLE_RSSI_POLL: {
                    cleanWifiScore();
                    mEnableRssiPolling = (message.arg1 == 1);
                    mRssiPollToken++;
                    if (mEnableRssiPolling) {
                        // First poll
                        mLastSignalLevel = -1;
                        if (isPrimary()) {
                            mLinkProbeManager.resetOnScreenTurnedOn();
                        }
                        updateLinkLayerStatsRssiSpeedFrequencyCapabilities();
                        sendMessageDelayed(obtainMessage(CMD_RSSI_POLL, mRssiPollToken, 0),
                                mWifiGlobals.getPollRssiIntervalMillis());
                    }
                    break;
                }
                case WifiMonitor.ASSOCIATED_BSSID_EVENT: {
                    if ((String) message.obj == null) {
                        logw("Associated command w/o BSSID");
                        break;
                    }
                    mLastBssid = (String) message.obj;
                    if (checkAndHandleLinkedNetworkRoaming(mLastBssid)) {
                        Log.i(TAG, "Driver initiated allowlist SSID roaming");
                        break;
                    }
                    if (mLastBssid != null && (mWifiInfo.getBSSID() == null
                            || !mLastBssid.equals(mWifiInfo.getBSSID()))) {
                        mWifiInfo.setBSSID(mLastBssid);
                        WifiConfiguration config = getConnectedWifiConfigurationInternal();
                        if (config != null) {
                            ScanDetailCache scanDetailCache = mWifiConfigManager
                                    .getScanDetailCacheForNetwork(config.networkId);
                            if (scanDetailCache != null) {
                                ScanResult scanResult = scanDetailCache.getScanResult(mLastBssid);
                                if (scanResult != null) {
                                    mWifiInfo.setFrequency(scanResult.frequency);
                                }
                            }
                        }
                        sendNetworkChangeBroadcastWithCurrentState();
                    }
                    break;
                }
                case CMD_START_RSSI_MONITORING_OFFLOAD:
                case CMD_RSSI_THRESHOLD_BREACHED: {
                    byte currRssi = (byte) message.arg1;
                    processRssiThreshold(currRssi, message.what, mRssiEventHandler);
                    break;
                }
                case CMD_STOP_RSSI_MONITORING_OFFLOAD: {
                    stopRssiMonitoringOffload();
                    break;
                }
                case CMD_RECONNECT: {
                    log(" Ignore CMD_RECONNECT request because wifi is already connected");
                    break;
                }
                case CMD_RESET_SIM_NETWORKS: {
                    if (message.arg1 != RESET_SIM_REASON_SIM_INSERTED
                            && mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
                        WifiConfiguration config =
                                mWifiConfigManager.getConfiguredNetwork(mLastNetworkId);
                        if (config != null
                            && ((message.arg1 == RESET_SIM_REASON_DEFAULT_DATA_SIM_CHANGED
                                && config.carrierId != TelephonyManager.UNKNOWN_CARRIER_ID)
                                || (config.enterpriseConfig != null
                                && config.enterpriseConfig.isAuthenticationSimBased()
                                && !mWifiCarrierInfoManager.isSimReady(mLastSubId)))) {
                            mWifiMetrics.logStaEvent(mInterfaceName,
                                    StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                    StaEvent.DISCONNECT_RESET_SIM_NETWORKS);
                            // remove local PMKSA cache in framework
                            mWifiNative.removeNetworkCachedData(mLastNetworkId);
                            // remove network so that supplicant's PMKSA cache is cleared
                            mWifiNative.removeAllNetworks(mInterfaceName);
                            if (isPrimary()) {
                                mSimRequiredNotifier.showSimRequiredNotification(
                                        config, mLastSimBasedConnectionCarrierName);
                            }
                        }
                    }
                    break;
                }
                case CMD_START_IP_PACKET_OFFLOAD: {
                    int slot = message.arg1;
                    int intervalSeconds = message.arg2;
                    KeepalivePacketData pkt = (KeepalivePacketData) message.obj;
                    int result = startWifiIPPacketOffload(slot, pkt, intervalSeconds);
                    if (mNetworkAgent != null) {
                        mNetworkAgent.sendSocketKeepaliveEvent(slot, result);
                    }
                    break;
                }
                case CMD_ADD_KEEPALIVE_PACKET_FILTER_TO_APF: {
                    if (mIpClient != null) {
                        final int slot = message.arg1;
                        if (message.obj instanceof NattKeepalivePacketData) {
                            final NattKeepalivePacketData pkt =
                                    (NattKeepalivePacketData) message.obj;
                            mIpClient.addKeepalivePacketFilter(slot, pkt);
                        } else if (SdkLevel.isAtLeastS()) {
                            if (message.obj instanceof TcpKeepalivePacketData) {
                                final TcpKeepalivePacketData pkt =
                                        (TcpKeepalivePacketData) message.obj;
                                mIpClient.addKeepalivePacketFilter(slot, pkt);
                            }
                            // Otherwise unsupported keepalive data class: skip
                        } else {
                            // Before S, non-NattKeepalivePacketData KeepalivePacketData would be
                            // the not-yet-SystemApi android.net.TcpKeepalivePacketData.
                            // Attempt to parse TcpKeepalivePacketDataParcelable from the
                            // KeepalivePacketData superclass.
                            final TcpKeepalivePacketDataParcelable p =
                                    parseTcpKeepalivePacketData((KeepalivePacketData) message.obj);
                            if (p != null) {
                                mIpClient.addKeepalivePacketFilter(slot, p);
                            }
                        }
                    }
                    break;
                }
                case CMD_REMOVE_KEEPALIVE_PACKET_FILTER_FROM_APF: {
                    if (mIpClient != null) {
                        mIpClient.removeKeepalivePacketFilter(message.arg1);
                    }
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }

            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }

            return handleStatus;
        }

        /**
         * Fetches link stats, updates Wifi Data Stall, Score Card and Score Report.
         */
        private WifiLinkLayerStats updateLinkLayerStatsRssiDataStallScoreReport() {
            // Get Info and continue polling
            WifiLinkLayerStats stats = updateLinkLayerStatsRssiSpeedFrequencyCapabilities();
            mWifiMetrics.updateWifiUsabilityStatsEntries(mInterfaceName, mWifiInfo, stats);
            // checkDataStallAndThroughputSufficiency() should be called before
            // mWifiScoreReport.calculateAndReportScore() which needs the latest throughput
            int statusDataStall = mWifiDataStall.checkDataStallAndThroughputSufficiency(
                    mInterfaceName, mLastConnectionCapabilities, mLastLinkLayerStats, stats,
                    mWifiInfo);
            if (mDataStallTriggerTimeMs == -1
                    && statusDataStall != WifiIsUnusableEvent.TYPE_UNKNOWN) {
                mDataStallTriggerTimeMs = mClock.getElapsedSinceBootMillis();
                mLastStatusDataStall = statusDataStall;
            }
            if (mDataStallTriggerTimeMs != -1) {
                long elapsedTime =  mClock.getElapsedSinceBootMillis()
                        - mDataStallTriggerTimeMs;
                if (elapsedTime >= DURATION_TO_WAIT_ADD_STATS_AFTER_DATA_STALL_MS) {
                    mDataStallTriggerTimeMs = -1;
                    mWifiMetrics.addToWifiUsabilityStatsList(mInterfaceName,
                            WifiUsabilityStats.LABEL_BAD,
                            convertToUsabilityStatsTriggerType(mLastStatusDataStall),
                            -1);
                    mLastStatusDataStall = WifiIsUnusableEvent.TYPE_UNKNOWN;
                }
            }
            // Send the update score to network agent.
            mWifiScoreReport.calculateAndReportScore();

            if (mWifiScoreReport.shouldCheckIpLayer()) {
                if (mIpClient != null) {
                    mIpClient.confirmConfiguration();
                }
                mWifiScoreReport.noteIpCheck();
            }

            mLastLinkLayerStats = stats;
            return stats;
        }
    }

    /**
     * Fetches link stats and updates Wifi Score Report.
     */
    private void updateLinkLayerStatsRssiAndScoreReport() {
        sendMessage(CMD_ONESHOT_RSSI_POLL);
    }

    private int convertToUsabilityStatsTriggerType(int unusableEventTriggerType) {
        int triggerType;
        switch (unusableEventTriggerType) {
            case WifiIsUnusableEvent.TYPE_DATA_STALL_BAD_TX:
                triggerType = WifiUsabilityStats.TYPE_DATA_STALL_BAD_TX;
                break;
            case WifiIsUnusableEvent.TYPE_DATA_STALL_TX_WITHOUT_RX:
                triggerType = WifiUsabilityStats.TYPE_DATA_STALL_TX_WITHOUT_RX;
                break;
            case WifiIsUnusableEvent.TYPE_DATA_STALL_BOTH:
                triggerType = WifiUsabilityStats.TYPE_DATA_STALL_BOTH;
                break;
            case WifiIsUnusableEvent.TYPE_FIRMWARE_ALERT:
                triggerType = WifiUsabilityStats.TYPE_FIRMWARE_ALERT;
                break;
            case WifiIsUnusableEvent.TYPE_IP_REACHABILITY_LOST:
                triggerType = WifiUsabilityStats.TYPE_IP_REACHABILITY_LOST;
                break;
            default:
                triggerType = WifiUsabilityStats.TYPE_UNKNOWN;
                Log.e(getTag(), "Unknown WifiIsUnusableEvent: " + unusableEventTriggerType);
        }
        return triggerType;
    }

    class L3ProvisioningState extends State {
        @Override
        public void enter() {
            WifiConfiguration currentConfig = getConnectedWifiConfigurationInternal();
            if (mIpClientWithPreConnection && mIpClient != null) {
                mIpClient.notifyPreconnectionComplete(mSentHLPs);
                mIpClientWithPreConnection = false;
                mSentHLPs = false;
            } else {
                startIpClient(currentConfig, false);
            }
            // Get Link layer stats so as we get fresh tx packet counters
            getWifiLinkLayerStats();
        }

        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;

            switch(message.what) {
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT: {
                    DisconnectEventInfo eventInfo = (DisconnectEventInfo) message.obj;
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE,
                            WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
                    mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(
                            getConnectingSsidInternal(),
                            !isValidBssid(eventInfo.bssid)
                            ? mTargetBssid : eventInfo.bssid,
                            WifiLastResortWatchdog.FAILURE_CODE_DHCP,
                            isConnected());
                    handleStatus = NOT_HANDLED;
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }

            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }
            return handleStatus;
        }
    }

    /**
     * Helper function to check if a network has been recently selected by the user.
     * (i.e less than {@link #LAST_SELECTED_NETWORK_EXPIRATION_AGE_MILLIS) before).
     */
    @VisibleForTesting
    public boolean isRecentlySelectedByTheUser(@NonNull WifiConfiguration currentConfig) {
        long currentTimeMillis = mClock.getElapsedSinceBootMillis();
        return mWifiConfigManager.getLastSelectedNetwork() == currentConfig.networkId
                && currentTimeMillis - mWifiConfigManager.getLastSelectedTimeStamp()
                < LAST_SELECTED_NETWORK_EXPIRATION_AGE_MILLIS;
    }

    private void sendConnectedState() {
        mNetworkAgent.markConnected();
        sendNetworkChangeBroadcast(DetailedState.CONNECTED);
    }

    class RoamingState extends State {
        boolean mAssociated;
        @Override
        public void enter() {
            if (mVerboseLoggingEnabled) {
                log("RoamingState Enter mScreenOn=" + mScreenOn);
            }

            // Make sure we disconnect if roaming fails
            mRoamWatchdogCount++;
            logd("Start Roam Watchdog " + mRoamWatchdogCount);
            sendMessageDelayed(obtainMessage(CMD_ROAM_WATCHDOG_TIMER,
                    mRoamWatchdogCount, 0), ROAM_GUARD_TIMER_MSEC);
            mAssociated = false;
        }
        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;

            switch (message.what) {
                case CMD_IP_CONFIGURATION_LOST: {
                    WifiConfiguration config = getConnectedWifiConfigurationInternal();
                    if (config != null) {
                        mWifiDiagnostics.triggerBugReportDataCapture(
                                WifiDiagnostics.REPORT_REASON_AUTOROAM_FAILURE);
                    }
                    handleStatus = NOT_HANDLED;
                    break;
                }
                case CMD_UNWANTED_NETWORK: {
                    if (mVerboseLoggingEnabled) {
                        log("Roaming and CS doesn't want the network -> ignore");
                    }
                    break;
                }
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT: {
                    /**
                     * If we get a SUPPLICANT_STATE_CHANGE_EVENT indicating a DISCONNECT
                     * before NETWORK_DISCONNECTION_EVENT
                     * And there is an associated BSSID corresponding to our target BSSID, then
                     * we have missed the network disconnection, transition to mDisconnectedState
                     * and handle the rest of the events there.
                     */
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    SupplicantState state = handleSupplicantStateChange(stateChangeResult);
                    if (state == SupplicantState.DISCONNECTED
                            || state == SupplicantState.INACTIVE
                            || state == SupplicantState.INTERFACE_DISABLED) {
                        if (mVerboseLoggingEnabled) {
                            log("RoamingState: Supplicant State change " + stateChangeResult);
                        }
                        handleNetworkDisconnect(false,
                                WIFI_DISCONNECT_REPORTED__FAILURE_CODE__SUPPLICANT_DISCONNECTED);
                        transitionTo(mDisconnectedState);
                    }
                    if (stateChangeResult.state == SupplicantState.ASSOCIATED) {
                        // We completed the layer2 roaming part
                        mAssociated = true;
                        mTargetBssid = stateChangeResult.bssid;
                    }
                    break;
                }
                case CMD_ROAM_WATCHDOG_TIMER: {
                    if (mRoamWatchdogCount == message.arg1) {
                        if (mVerboseLoggingEnabled) log("roaming watchdog! -> disconnect");
                        mWifiMetrics.endConnectionEvent(
                                mInterfaceName,
                                WifiMetrics.ConnectionEvent.FAILURE_ROAM_TIMEOUT,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE,
                                WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN,
                                mWifiInfo.getFrequency());
                        mRoamFailCount++;
                        handleNetworkDisconnect(false,
                                WifiStatsLog.WIFI_DISCONNECT_REPORTED__FAILURE_CODE__ROAM_WATCHDOG_TIMER);
                        mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                StaEvent.DISCONNECT_ROAM_WATCHDOG_TIMER);
                        mWifiNative.disconnect(mInterfaceName);
                        transitionTo(mDisconnectedState);
                    }
                    break;
                }
                case WifiMonitor.NETWORK_CONNECTION_EVENT: {
                    if (mAssociated) {
                        if (mVerboseLoggingEnabled) {
                            log("roaming and Network connection established");
                        }
                        NetworkConnectionEventInfo connectionInfo =
                                (NetworkConnectionEventInfo) message.obj;
                        mLastNetworkId = connectionInfo.networkId;
                        mLastBssid = connectionInfo.bssid;
                        mWifiInfo.setBSSID(mLastBssid);
                        mWifiInfo.setNetworkId(mLastNetworkId);
                        sendNetworkChangeBroadcastWithCurrentState();

                        // Successful framework roam! (probably)
                        mWifiBlocklistMonitor.handleBssidConnectionSuccess(mLastBssid,
                                mWifiInfo.getSSID());
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE,
                                WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);

                        // We must clear the config BSSID, as the wifi chipset may decide to roam
                        // from this point on and having the BSSID specified by QNS would cause
                        // the roam to fail and the device to disconnect.
                        // When transition from RoamingState to DisconnectedState, the config BSSID
                        // is cleared by handleNetworkDisconnect().
                        clearTargetBssid("RoamingCompleted");

                        // We used to transition to L3ProvisioningState in an
                        // attempt to do DHCPv4 RENEWs on framework roams.
                        // DHCP can take too long to time out, and we now rely
                        // upon IpClient's use of IpReachabilityMonitor to
                        // confirm our current network configuration.
                        //
                        // mIpClient.confirmConfiguration() is called within
                        // the handling of SupplicantState.COMPLETED.
                        transitionTo(mL3ConnectedState);
                    } else {
                        mMessageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    }
                    break;
                }
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT: {
                    // Throw away but only if it corresponds to the network we're roaming to
                    DisconnectEventInfo eventInfo = (DisconnectEventInfo) message.obj;
                    if (true) {
                        String target = "";
                        if (mTargetBssid != null) target = mTargetBssid;
                        log("NETWORK_DISCONNECTION_EVENT in roaming state"
                                + " BSSID=" + eventInfo.bssid
                                + " target=" + target);
                    }
                    clearNetworkCachedDataIfNeeded(
                            getConnectingWifiConfigurationInternal(), eventInfo.reasonCode);
                    if (eventInfo.bssid.equals(mTargetBssid)) {
                        handleNetworkDisconnect(false, eventInfo.reasonCode);
                        transitionTo(mDisconnectedState);
                    }
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }

            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }
            return handleStatus;
        }

        @Override
        public void exit() {
            logd("ClientModeImpl: Leaving Roaming state");
        }
    }

    class L3ConnectedState extends State {
        @Override
        public void enter() {
            if (mVerboseLoggingEnabled) {
                log("Enter ConnectedState  mScreenOn=" + mScreenOn);
            }

            reportConnectionAttemptEnd(
                    WifiMetrics.ConnectionEvent.FAILURE_NONE,
                    WifiMetricsProto.ConnectionEvent.HLF_NONE,
                    WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
            mWifiConnectivityManager.handleConnectionStateChanged(
                    mClientModeManager,
                    WifiConnectivityManager.WIFI_STATE_CONNECTED);
            registerConnected();
            mTargetWifiConfiguration = null;
            mWifiScoreReport.reset();
            mLastSignalLevel = -1;

            // Not roaming anymore
            mIsAutoRoaming = false;

            mTargetNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
            mWifiLastResortWatchdog.connectedStateTransition(true);
            mWifiStateTracker.updateState(mInterfaceName, WifiStateTracker.CONNECTED);
            // Inform WifiLockManager
            mWifiLockManager.updateWifiClientConnected(mClientModeManager, true);
            WifiConfiguration config = getConnectedWifiConfigurationInternal();
            mWifiScoreReport.startConnectedNetworkScorer(
                    mNetworkAgent.getNetwork().getNetId(), isRecentlySelectedByTheUser(config));
            updateLinkLayerStatsRssiAndScoreReport();
            mWifiScoreCard.noteIpConfiguration(mWifiInfo);
            // too many places to record L3 failure with too many failure reasons.
            // So only record success here.
            mWifiMetrics.noteFirstL3ConnectionAfterBoot(true);
        }
        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;

            switch (message.what) {
                case CMD_UNWANTED_NETWORK: {
                    if (message.arg1 == NETWORK_STATUS_UNWANTED_DISCONNECT) {
                        mWifiMetrics.logStaEvent(mInterfaceName, StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                StaEvent.DISCONNECT_UNWANTED);
                        if (mClientModeManager.getRole() == ROLE_CLIENT_SECONDARY_TRANSIENT
                                && mClientModeManager.getPreviousRole() == ROLE_CLIENT_PRIMARY) {
                            mWifiMetrics.incrementMakeBeforeBreakLingerCompletedCount(
                                    mClock.getElapsedSinceBootMillis()
                                            - mClientModeManager.getLastRoleChangeSinceBootMs());
                        }
                        mWifiNative.disconnect(mInterfaceName);
                    } else if (message.arg1 == NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN
                            || message.arg1 == NETWORK_STATUS_UNWANTED_VALIDATION_FAILED) {
                        Log.d(getTag(), (message.arg1 == NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN
                                ? "NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN"
                                : "NETWORK_STATUS_UNWANTED_VALIDATION_FAILED"));
                        WifiConfiguration config = getConnectedWifiConfigurationInternal();
                        if (config != null) {
                            // Disable autojoin
                            if (message.arg1 == NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN) {
                                mWifiConfigManager.setNetworkValidatedInternetAccess(
                                        config.networkId, false);
                                mWifiConfigManager.updateNetworkSelectionStatus(config.networkId,
                                        DISABLED_NO_INTERNET_PERMANENT);
                            } else {
                                // stop collect last-mile stats since validation fail
                                removeMessages(CMD_DIAGS_CONNECT_TIMEOUT);
                                mWifiDiagnostics.reportConnectionEvent(
                                        WifiDiagnostics.CONNECTION_EVENT_FAILED,
                                        mClientModeManager);
                                mWifiConfigManager.incrementNetworkNoInternetAccessReports(
                                        config.networkId);
                                // If this was not recently selected by the user, update network
                                // selection status to temporarily disable the network.
                                if (!isRecentlySelectedByTheUser(config)
                                        && !config.noInternetAccessExpected) {
                                    Log.i(getTag(), "Temporarily disabling network because of"
                                            + " no-internet access");
                                    mWifiConfigManager.updateNetworkSelectionStatus(
                                            config.networkId,
                                            DISABLED_NO_INTERNET_TEMPORARY);
                                    mWifiBlocklistMonitor.handleBssidConnectionFailure(
                                            mLastBssid, config.SSID,
                                            WifiBlocklistMonitor.REASON_NETWORK_VALIDATION_FAILURE,
                                            mWifiInfo.getRssi());
                                }
                                mWifiScoreCard.noteValidationFailure(mWifiInfo);
                            }
                        }
                        if (mClientModeManager.getRole() == ROLE_CLIENT_SECONDARY_TRANSIENT) {
                            Log.d(getTag(), "Internet validation failed during MBB,"
                                    + " disconnecting ClientModeManager=" + mClientModeManager);
                            mWifiMetrics.logStaEvent(
                                    mInterfaceName,
                                    StaEvent.TYPE_FRAMEWORK_DISCONNECT,
                                    StaEvent.DISCONNECT_MBB_NO_INTERNET);
                            mWifiMetrics.incrementMakeBeforeBreakNoInternetCount();
                            mWifiNative.disconnect(mInterfaceName);
                        }
                    }
                    break;
                }
                case CMD_NETWORK_STATUS: {
                    if (message.arg1 == NetworkAgent.VALIDATION_STATUS_VALID) {
                        // stop collect last-mile stats since validation pass
                        removeMessages(CMD_DIAGS_CONNECT_TIMEOUT);
                        mWifiDiagnostics.reportConnectionEvent(
                                WifiDiagnostics.CONNECTION_EVENT_SUCCEEDED, mClientModeManager);
                        mWifiScoreCard.noteValidationSuccess(mWifiInfo);
                        mWifiBlocklistMonitor.handleNetworkValidationSuccess(mLastBssid,
                                mWifiInfo.getSSID());
                        WifiConfiguration config = getConnectedWifiConfigurationInternal();
                        if (config != null) {
                            // re-enable autojoin
                            mWifiConfigManager.updateNetworkSelectionStatus(
                                    config.networkId,
                                    WifiConfiguration.NetworkSelectionStatus
                                            .DISABLED_NONE);
                            mWifiConfigManager.setNetworkValidatedInternetAccess(
                                    config.networkId, true);
                            if (config.isPasspoint()
                                    && mTermsAndConditionsUrl != null) {
                                // Clear the T&C after the user accepted them and the we are
                                // notified that the network validation is successful
                                mTermsAndConditionsUrl = null;
                                LinkProperties newLp = new LinkProperties(mLinkProperties);
                                addPasspointInfoToLinkProperties(newLp);
                                sendMessage(CMD_UPDATE_LINKPROPERTIES, newLp);
                                mWifiMetrics
                                        .incrementTotalNumberOfPasspointAcceptanceOfTermsAndConditions();
                            }
                            if (retrieveConnectedNetworkDefaultGateway()) {
                                updateLinkedNetworks(config);
                            }
                        }
                        mCmiMonitor.onInternetValidated(mClientModeManager);
                    }
                    break;
                }
                case CMD_ACCEPT_UNVALIDATED: {
                    boolean accept = (message.arg1 != 0);
                    mWifiConfigManager.setNetworkNoInternetAccessExpected(mLastNetworkId, accept);
                    break;
                }
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT: {
                    DisconnectEventInfo eventInfo = (DisconnectEventInfo) message.obj;
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE,
                            WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
                    if (unexpectedDisconnectedReason(eventInfo.reasonCode)) {
                        mWifiDiagnostics.triggerBugReportDataCapture(
                                WifiDiagnostics.REPORT_REASON_UNEXPECTED_DISCONNECT);
                    }

                    if (!eventInfo.locallyGenerated) {
                        // ignore disconnects initiated by wpa_supplicant.
                        mWifiScoreCard.noteNonlocalDisconnect(mInterfaceName, eventInfo.reasonCode);
                        int rssi = mWifiInfo.getRssi();
                        mWifiBlocklistMonitor.handleBssidConnectionFailure(mWifiInfo.getBSSID(),
                                mWifiInfo.getSSID(),
                                WifiBlocklistMonitor.REASON_ABNORMAL_DISCONNECT, rssi);
                    }
                    WifiConfiguration config = getConnectedWifiConfigurationInternal();

                    if (mVerboseLoggingEnabled) {
                        log("NETWORK_DISCONNECTION_EVENT in connected state"
                                + " BSSID=" + mWifiInfo.getBSSID()
                                + " RSSI=" + mWifiInfo.getRssi()
                                + " freq=" + mWifiInfo.getFrequency()
                                + " reason=" + eventInfo.reasonCode
                                + " Network Selection Status=" + (config == null ? "Unavailable"
                                : config.getNetworkSelectionStatus().getNetworkStatusString()));
                    }
                    handleNetworkDisconnect(false, eventInfo.reasonCode);
                    transitionTo(mDisconnectedState);
                    break;
                }
                case CMD_START_ROAM: {
                    /* Connect command coming from auto-join */
                    int netId = message.arg1;
                    String bssid = (String) message.obj;
                    if (bssid == null) {
                        bssid = SUPPLICANT_BSSID_ANY;
                    }
                    WifiConfiguration config =
                            mWifiConfigManager.getConfiguredNetworkWithoutMasking(netId);
                    if (config == null) {
                        loge("CMD_START_ROAM and no config, bail out...");
                        break;
                    }
                    mLastScanRssi = mWifiConfigManager.findScanRssi(netId,
                            mWifiHealthMonitor.getScanRssiValidTimeMs());
                    mWifiScoreCard.noteConnectionAttempt(mWifiInfo, mLastScanRssi, config.SSID);
                    setTargetBssid(config, bssid);
                    mTargetNetworkId = netId;

                    logd("CMD_START_ROAM sup state "
                            + " my state " + getCurrentState().getName()
                            + " nid=" + Integer.toString(netId)
                            + " config " + config.getProfileKey()
                            + " targetRoamBSSID " + mTargetBssid);

                    reportConnectionAttemptStart(config, mTargetBssid,
                            WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
                    if (mWifiNative.roamToNetwork(mInterfaceName, config)) {
                        mTargetWifiConfiguration = config;
                        mIsAutoRoaming = true;
                        mWifiMetrics.logStaEvent(
                                mInterfaceName, StaEvent.TYPE_CMD_START_ROAM, config);
                        transitionTo(mRoamingState);
                    } else {
                        loge("CMD_START_ROAM Failed to start roaming to network " + config);
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE,
                                WifiMetricsProto.ConnectionEvent.FAILURE_REASON_UNKNOWN);
                        mMessageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                        break;
                    }
                    break;
                }
                case CMD_IP_CONFIGURATION_LOST: {
                    mWifiMetrics.incrementIpRenewalFailure();
                    handleStatus = NOT_HANDLED;
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }

            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }

            return handleStatus;
        }

        @Override
        public void exit() {
            logd("ClientModeImpl: Leaving Connected state");
            mWifiConnectivityManager.handleConnectionStateChanged(
                    mClientModeManager,
                     WifiConnectivityManager.WIFI_STATE_TRANSITIONING);

            mWifiLastResortWatchdog.connectedStateTransition(false);
        }
    }

    class DisconnectedState extends State {
        @Override
        public void enter() {
            Log.i(getTag(), "disconnectedstate enter");
            // We don't scan frequently if this is a temporary disconnect
            // due to p2p
            if (mWifiP2pConnection.shouldTemporarilyDisconnectWifi()) {
                // TODO(b/161569371): P2P should wait for all ClientModeImpls to enter
                //  DisconnectedState, not just one instance.
                // (Does P2P Service support STA+P2P concurrency?)
                mWifiP2pConnection.sendMessage(WifiP2pServiceImpl.DISCONNECT_WIFI_RESPONSE);
                return;
            }

            if (mVerboseLoggingEnabled) {
                logd(" Enter DisconnectedState screenOn=" + mScreenOn);
            }

            /** clear the roaming state, if we were roaming, we failed */
            mIsAutoRoaming = false;
            mTargetNetworkId = WifiConfiguration.INVALID_NETWORK_ID;

            mWifiConnectivityManager.handleConnectionStateChanged(
                    mClientModeManager,
                    WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
        }

        @Override
        public boolean processMessage(Message message) {
            boolean handleStatus = HANDLED;

            switch (message.what) {
                case CMD_RECONNECT:
                case CMD_REASSOCIATE: {
                    if (mWifiP2pConnection.shouldTemporarilyDisconnectWifi()) {
                        // Drop a third party reconnect/reassociate if STA is
                        // temporarily disconnected for p2p
                        break;
                    } else {
                        // ConnectableState handles it
                        handleStatus = NOT_HANDLED;
                    }
                    break;
                }
                default: {
                    handleStatus = NOT_HANDLED;
                    break;
                }
            }

            if (handleStatus == HANDLED) {
                logStateAndMessage(message, this);
            }
            return handleStatus;
        }

        @Override
        public void exit() {
            mWifiConnectivityManager.handleConnectionStateChanged(
                    mClientModeManager,
                     WifiConnectivityManager.WIFI_STATE_TRANSITIONING);
        }
    }

    void handleGsmAuthRequest(SimAuthRequestData requestData) {
        WifiConfiguration requestingWifiConfiguration = null;
        if (mTargetWifiConfiguration != null
                && mTargetWifiConfiguration.networkId
                == requestData.networkId) {
            requestingWifiConfiguration = mTargetWifiConfiguration;
            logd("id matches targetWifiConfiguration");
        } else if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID
                && mLastNetworkId == requestData.networkId) {
            requestingWifiConfiguration = getConnectedWifiConfigurationInternal();
            logd("id matches currentWifiConfiguration");
        }

        if (requestingWifiConfiguration == null) {
            logd("GsmAuthRequest received with null target/current WifiConfiguration.");
            return;
        }

        /*
         * Try authentication in the following order.
         *
         *    Standard       Cellular_auth     Type Command
         *
         * 1. 3GPP TS 31.102 3G_authentication [Length][RAND][Length][AUTN]
         *                            [Length][RES][Length][CK][Length][IK] and more
         * 2. 3GPP TS 31.102 2G_authentication [Length][RAND]
         *                            [Length][SRES][Length][Cipher Key Kc]
         * 3. 3GPP TS 11.11  2G_authentication [RAND]
         *                            [SRES][Cipher Key Kc]
         */
        String response = mWifiCarrierInfoManager
                .getGsmSimAuthResponse(requestData.data, requestingWifiConfiguration);
        if (response == null) {
            // In case of failure, issue may be due to sim type, retry as No.2 case
            response = mWifiCarrierInfoManager
                    .getGsmSimpleSimAuthResponse(requestData.data, requestingWifiConfiguration);
            if (response == null) {
                // In case of failure, issue may be due to sim type, retry as No.3 case
                response = mWifiCarrierInfoManager.getGsmSimpleSimNoLengthAuthResponse(
                                requestData.data, requestingWifiConfiguration);
            }
        }
        if (response == null || response.length() == 0) {
            mWifiNative.simAuthFailedResponse(mInterfaceName);
        } else {
            logv("Supplicant Response -" + response);
            mWifiNative.simAuthResponse(
                    mInterfaceName, WifiNative.SIM_AUTH_RESP_TYPE_GSM_AUTH, response);
        }
    }

    void handle3GAuthRequest(SimAuthRequestData requestData) {
        WifiConfiguration requestingWifiConfiguration = null;
        if (mTargetWifiConfiguration != null
                && mTargetWifiConfiguration.networkId
                == requestData.networkId) {
            requestingWifiConfiguration = mTargetWifiConfiguration;
            logd("id matches targetWifiConfiguration");
        } else if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID
                && mLastNetworkId == requestData.networkId) {
            requestingWifiConfiguration = getConnectedWifiConfigurationInternal();
            logd("id matches currentWifiConfiguration");
        }

        if (requestingWifiConfiguration == null) {
            logd("3GAuthRequest received with null target/current WifiConfiguration.");
            return;
        }

        SimAuthResponseData response = mWifiCarrierInfoManager
                .get3GAuthResponse(requestData, requestingWifiConfiguration);
        if (response != null) {
            mWifiNative.simAuthResponse(
                    mInterfaceName, response.type, response.response);
        } else {
            mWifiNative.umtsAuthFailedResponse(mInterfaceName);
        }
    }

    /**
     * Automatically connect to the network specified
     *
     * @param networkId ID of the network to connect to
     * @param uid UID of the app triggering the connection.
     * @param bssid BSSID of the network
     */
    public void startConnectToNetwork(int networkId, int uid, String bssid) {
        sendMessage(CMD_START_CONNECT, networkId, uid, bssid);
    }

    /**
     * Automatically roam to the network specified
     *
     * @param networkId ID of the network to roam to
     * @param bssid BSSID of the access point to roam to.
     */
    public void startRoamToNetwork(int networkId, String bssid) {
        sendMessage(CMD_START_ROAM, networkId, 0, bssid);
    }

    /**
     * @param reason reason code from supplicant on network disconnected event
     * @return true if this is a suspicious disconnect
     */
    static boolean unexpectedDisconnectedReason(int reason) {
        return reason == ReasonCode.PREV_AUTH_NOT_VALID
                || reason == ReasonCode.CLASS2_FRAME_FROM_NONAUTH_STA
                || reason == ReasonCode.CLASS3_FRAME_FROM_NONASSOC_STA
                || reason == ReasonCode.DISASSOC_STA_HAS_LEFT
                || reason == ReasonCode.STA_REQ_ASSOC_WITHOUT_AUTH
                || reason == ReasonCode.MICHAEL_MIC_FAILURE
                || reason == ReasonCode.FOURWAY_HANDSHAKE_TIMEOUT
                || reason == ReasonCode.GROUP_KEY_UPDATE_TIMEOUT
                || reason == ReasonCode.GROUP_CIPHER_NOT_VALID
                || reason == ReasonCode.PAIRWISE_CIPHER_NOT_VALID
                || reason == ReasonCode.IEEE_802_1X_AUTH_FAILED
                || reason == ReasonCode.DISASSOC_LOW_ACK;
    }

    private static String getLinkPropertiesSummary(LinkProperties lp) {
        List<String> attributes = new ArrayList<>(6);
        if (lp.hasIpv4Address()) {
            attributes.add("v4");
        }
        if (lp.hasIpv4DefaultRoute()) {
            attributes.add("v4r");
        }
        if (lp.hasIpv4DnsServer()) {
            attributes.add("v4dns");
        }
        if (lp.hasGlobalIpv6Address()) {
            attributes.add("v6");
        }
        if (lp.hasIpv6DefaultRoute()) {
            attributes.add("v6r");
        }
        if (lp.hasIpv6DnsServer()) {
            attributes.add("v6dns");
        }

        return TextUtils.join(" ", attributes);
    }

    /**
     * Gets the SSID from the WifiConfiguration pointed at by 'mTargetNetworkId'
     * This should match the network config framework is attempting to connect to.
     */
    private String getConnectingSsidInternal() {
        WifiConfiguration config = getConnectingWifiConfigurationInternal();
        return config != null ? config.SSID : null;
    }

    /**
     * Check if there is any connection request for WiFi network.
     */
    private boolean hasConnectionRequests() {
        return mNetworkFactory.hasConnectionRequests()
                || mUntrustedNetworkFactory.hasConnectionRequests()
                || mOemWifiNetworkFactory.hasConnectionRequests();
    }

    /**
     * Retrieve the factory MAC address from config store (stored on first bootup). If we don't have
     * a factory MAC address stored in config store, retrieve it now and store it.
     *
     * Note:
     * <li> This is needed to ensure that we use the same MAC address for connecting to
     * networks with MAC randomization disabled regardless of whether the connection is
     * occurring on "wlan0" or "wlan1" due to STA + STA. </li>
     * <li> Retries added to deal with any transient failures when invoking
     * {@link WifiNative#getStaFactoryMacAddress(String)}.
     */
    @Nullable
    private MacAddress retrieveFactoryMacAddressAndStoreIfNecessary() {
        // Get value from hal to update MAC address.
        return mWifiNative.getStaFactoryMacAddress(mInterfaceName);
    }

    /**
     * Gets the factory MAC address of wlan0 (station interface).
     * @return String representation of the factory MAC address.
     */
    @Nullable
    public String getFactoryMacAddress() {
        MacAddress factoryMacAddress = retrieveFactoryMacAddressAndStoreIfNecessary();
        if (factoryMacAddress != null) return factoryMacAddress.toString();

        // For devices with older HAL's (version < 1.3), no API exists to retrieve factory MAC
        // address (and also does not support MAC randomization - needs verson 1.2). So, just
        // return the regular MAC address from the interface.
        if (!mWifiGlobals.isConnectedMacRandomizationEnabled()) {
            Log.w(TAG, "Can't get factory MAC address, return the MAC address");
            return mWifiNative.getMacAddress(mInterfaceName);
        }
        return null;
    }

    /** Sends a link probe. */
    public void probeLink(LinkProbeCallback callback, int mcs) {
        String bssid = mWifiInfo.getBSSID();
        if (bssid == null) {
            Log.w(getTag(), "Attempted to send link probe when not connected!");
            callback.onFailure(LinkProbeCallback.LINK_PROBE_ERROR_NOT_CONNECTED);
            return;
        }
        mWifiNative.probeLink(mInterfaceName, MacAddress.fromString(bssid), callback, mcs);
    }

    private static class ConnectNetworkMessage {
        public final NetworkUpdateResult result;
        public final ActionListenerWrapper listener;

        ConnectNetworkMessage(NetworkUpdateResult result, ActionListenerWrapper listener) {
            this.result = result;
            this.listener = listener;
        }
    }

    /** Trigger network connection and provide status via the provided callback. */
    public void connectNetwork(NetworkUpdateResult result, ActionListenerWrapper wrapper,
            int callingUid) {
        Message message =
                obtainMessage(CMD_CONNECT_NETWORK, new ConnectNetworkMessage(result, wrapper));
        message.sendingUid = callingUid;
        sendMessage(message);
    }

    /** Trigger network save and provide status via the provided callback. */
    public void saveNetwork(NetworkUpdateResult result, ActionListenerWrapper wrapper,
            int callingUid) {
        Message message =
                obtainMessage(CMD_SAVE_NETWORK, new ConnectNetworkMessage(result, wrapper));
        message.sendingUid = callingUid;
        sendMessage(message);
    }

    /**
     * Handle BSS transition request from Connected BSS.
     *
     * @param frameData Data retrieved from received BTM request frame.
     */
    private void handleBssTransitionRequest(BtmFrameData frameData) {
        if (frameData == null) {
            return;
        }

        String bssid = mWifiInfo.getBSSID();
        String ssid = mWifiInfo.getSSID();
        if ((bssid == null) || (ssid == null) || WifiManager.UNKNOWN_SSID.equals(ssid)) {
            Log.e(getTag(), "Failed to handle BSS transition: bssid: " + bssid + " ssid: " + ssid);
            return;
        }

        mWifiMetrics.incrementSteeringRequestCount();

        if ((frameData.mBssTmDataFlagsMask
                & MboOceConstants.BTM_DATA_FLAG_MBO_CELL_DATA_CONNECTION_PREFERENCE_INCLUDED)
                != 0) {
            mWifiMetrics.incrementMboCellularSwitchRequestCount();
        }


        if ((frameData.mBssTmDataFlagsMask
                & MboOceConstants.BTM_DATA_FLAG_DISASSOCIATION_IMMINENT) != 0) {
            long duration = 0;
            if ((frameData.mBssTmDataFlagsMask
                    & MboOceConstants.BTM_DATA_FLAG_MBO_ASSOC_RETRY_DELAY_INCLUDED) != 0) {
                mWifiMetrics.incrementSteeringRequestCountIncludingMboAssocRetryDelay();
                duration = frameData.mBlockListDurationMs;
            }
            if (duration == 0) {
                /*
                 * When disassoc imminent bit alone is set or MBO assoc retry delay is
                 * set to zero(reserved as per spec), blocklist the BSS for sometime to
                 * avoid AP rejecting the re-connect request.
                 */
                duration = MboOceConstants.DEFAULT_BLOCKLIST_DURATION_MS;
            }
            // Blocklist the current BSS
            mWifiBlocklistMonitor.blockBssidForDurationMs(bssid, ssid, duration,
                    WifiBlocklistMonitor.REASON_FRAMEWORK_DISCONNECT_MBO_OCE, 0);
        }

        if (frameData.mStatus != MboOceConstants.BTM_RESPONSE_STATUS_ACCEPT) {
            // Trigger the network selection and re-connect to new network if available.
            mWifiMetrics.incrementForceScanCountDueToSteeringRequest();
            mWifiConnectivityManager.forceConnectivityScan(ClientModeImpl.WIFI_WORK_SOURCE);
        }
    }

    /**
     * @return true if this device supports FILS-SHA256
     */
    private boolean isFilsSha256Supported() {
        return (getSupportedFeatures() & WIFI_FEATURE_FILS_SHA256) != 0;
    }

    /**
     * @return true if this device supports FILS-SHA384
     */
    private boolean isFilsSha384Supported() {
        return (getSupportedFeatures() & WIFI_FEATURE_FILS_SHA384) != 0;
    }

    /**
     * Helper method to set the allowed key management schemes from
     * scan result.
     * When the AKM is updated, changes should be propagated to the
     * actual saved network, and the correct AKM could be retrieved
     * on selecting the security params.
     */
    private void updateAllowedKeyManagementSchemesFromScanResult(
            WifiConfiguration config, ScanResult scanResult) {
        config.enableFils(
                isFilsSha256Supported()
                && ScanResultUtil.isScanResultForFilsSha256Network(scanResult),
                isFilsSha384Supported()
                && ScanResultUtil.isScanResultForFilsSha384Network(scanResult));
        mWifiConfigManager.updateFilsAkms(config.networkId,
                config.isFilsSha256Enabled(), config.isFilsSha384Enabled());
    }
    /**
     * Update wifi configuration based on the matching scan result.
     *
     * @param config Wifi configuration object.
     * @param scanResult Scan result matching the network.
     */
    private void updateWifiConfigFromMatchingScanResult(WifiConfiguration config,
            ScanResult scanResult) {
        updateAllowedKeyManagementSchemesFromScanResult(config, scanResult);
        if (config.isFilsSha256Enabled() || config.isFilsSha384Enabled()) {
            config.enterpriseConfig.setFieldValue(WifiEnterpriseConfig.EAP_ERP, "1");
        }
    }

    private void selectCandidateSecurityParamsIfNecessary(
            WifiConfiguration config,
            List<ScanResult> scanResults) {
        if (null != config.getNetworkSelectionStatus().getCandidateSecurityParams()) return;

        // This comes from wifi picker directly so there is no candidate security params.
        // Run network selection against this SSID.
        List<ScanDetail> scanDetailsList = scanResults.stream()
                .filter(scanResult -> config.SSID.equals(
                        ScanResultUtil.createQuotedSSID(scanResult.SSID)))
                .map(ScanResultUtil::toScanDetail)
                .collect(Collectors.toList());
        List<WifiNetworkSelector.ClientModeManagerState> cmmState = new ArrayList<>();
        cmmState.add(new WifiNetworkSelector.ClientModeManagerState(mClientModeManager));
        List<WifiCandidates.Candidate> candidates = mWifiNetworkSelector.getCandidatesFromScan(
                scanDetailsList,
                new HashSet<String>(),
                cmmState,
                true, true, true);
        WifiConfiguration selectedConfig = mWifiNetworkSelector.selectNetwork(candidates);
        if (null != selectedConfig && selectedConfig.networkId == config.networkId) {
            config.getNetworkSelectionStatus().setCandidateSecurityParams(
                    selectedConfig.getNetworkSelectionStatus().getCandidateSecurityParams());
            return;
        }

        // When a connecting request comes from network request or adding a network via
        // API directly, there might be no scan result to know the proper security params.
        // In this case, we use the first available security params to have a try first.
        Log.i(getTag(), "Cannot select a candidate security params from scan results,"
                + "try to select the first available security params.");
        SecurityParams defaultParams = config.getSecurityParamsList().stream()
                .filter(WifiConfigurationUtil::isSecurityParamsValid)
                .findFirst().orElse(null);
        config.getNetworkSelectionStatus().setCandidateSecurityParams(defaultParams);
        // populate the target security params to the internal configuration manually,
        // and then wifi info could retrieve this information.
        mWifiConfigManager.setNetworkCandidateScanResult(
                config.networkId, null, 0, defaultParams);
    }

    /**
     * Update the wifi configuration before sending connect to
     * supplicant/driver.
     *
     * @param config wifi configuration object.
     * @param bssid BSSID to assocaite with.
     */
    void updateWifiConfigOnStartConnection(WifiConfiguration config, String bssid) {
        setTargetBssid(config, bssid);

        // Go through the matching scan results and update wifi config.
        ScanResultMatchInfo key1 = ScanResultMatchInfo.fromWifiConfiguration(config);
        List<ScanResult> scanResults = mScanRequestProxy.getScanResults();
        for (ScanResult scanResult : scanResults) {
            if (!config.SSID.equals(ScanResultUtil.createQuotedSSID(scanResult.SSID))) {
                continue;
            }
            ScanResultMatchInfo key2 = ScanResultMatchInfo.fromScanResult(scanResult);
            if (!key1.equals(key2)) {
                continue;
            }
            updateWifiConfigFromMatchingScanResult(config, scanResult);
        }

        selectCandidateSecurityParamsIfNecessary(config, scanResults);

        if (mWifiGlobals.isConnectedMacRandomizationEnabled()) {
            if (config.macRandomizationSetting != WifiConfiguration.RANDOMIZATION_NONE) {
                configureRandomizedMacAddress(config);
            } else {
                setCurrentMacToFactoryMac(config);
            }
        }

        if (config.enterpriseConfig != null
                && config.enterpriseConfig.isAuthenticationSimBased()
                && mWifiCarrierInfoManager.isImsiEncryptionInfoAvailable(
                mWifiCarrierInfoManager.getBestMatchSubscriptionId(config))
                && TextUtils.isEmpty(config.enterpriseConfig.getAnonymousIdentity())) {
            String anonAtRealm = mWifiCarrierInfoManager
                    .getAnonymousIdentityWith3GppRealm(config);
            // Use anonymous@<realm> when pseudonym is not available
            config.enterpriseConfig.setAnonymousIdentity(anonAtRealm);
        }
    }

    private void setConfigurationsPriorToIpClientProvisioning(WifiConfiguration config) {
        mIpClient.setHttpProxy(config.getHttpProxy());
        if (!TextUtils.isEmpty(mContext.getResources().getString(
                R.string.config_wifi_tcp_buffers))) {
            mIpClient.setTcpBufferSizes(mContext.getResources().getString(
                    R.string.config_wifi_tcp_buffers));
        }
    }

    private boolean startIpClient(WifiConfiguration config, boolean isFilsConnection) {
        if (mIpClient == null) {
            return false;
        }

        final boolean isUsingStaticIp =
                (config.getIpAssignment() == IpConfiguration.IpAssignment.STATIC);
        final boolean isUsingMacRandomization =
                config.macRandomizationSetting
                        != WifiConfiguration.RANDOMIZATION_NONE
                        && mWifiGlobals.isConnectedMacRandomizationEnabled();
        if (mVerboseLoggingEnabled) {
            final String key = config.getProfileKey();
            log("startIpClient netId=" + Integer.toString(mLastNetworkId)
                    + " " + key + " "
                    + " roam=" + mIsAutoRoaming
                    + " static=" + isUsingStaticIp
                    + " randomMac=" + isUsingMacRandomization
                    + " isFilsConnection=" + isFilsConnection);
        }

        final MacAddress currentBssid = getCurrentBssidInternalMacAddress();
        final String l2Key = mLastL2KeyAndGroupHint != null
                ? mLastL2KeyAndGroupHint.first : null;
        final String groupHint = mLastL2KeyAndGroupHint != null
                ? mLastL2KeyAndGroupHint.second : null;
        final Layer2Information layer2Info = new Layer2Information(l2Key, groupHint,
                currentBssid);

        if (isFilsConnection) {
            stopIpClient();
            if (isUsingStaticIp) {
                mWifiNative.flushAllHlp(mInterfaceName);
                return false;
            }
            setConfigurationsPriorToIpClientProvisioning(config);
            final ProvisioningConfiguration.Builder prov =
                    new ProvisioningConfiguration.Builder()
                    .withPreDhcpAction()
                    .withPreconnection()
                    .withApfCapabilities(
                    mWifiNative.getApfCapabilities(mInterfaceName))
                    .withLayer2Information(layer2Info);
            if (isUsingMacRandomization) {
                // Use EUI64 address generation for link-local IPv6 addresses.
                prov.withRandomMacAddress();
            }
            mIpClient.startProvisioning(prov.build());
        } else {
            sendNetworkChangeBroadcast(DetailedState.OBTAINING_IPADDR);
            // We must clear the config BSSID, as the wifi chipset may decide to roam
            // from this point on and having the BSSID specified in the network block would
            // cause the roam to fail and the device to disconnect.
            clearTargetBssid("ObtainingIpAddress");

            // Stop IpClient in case we're switching from DHCP to static
            // configuration or vice versa.
            //
            // When we transition from static configuration to DHCP in
            // particular, we must tell ConnectivityService that we're
            // disconnected, because DHCP might take a long time during which
            // connectivity APIs such as getActiveNetworkInfo should not return
            // CONNECTED.
            stopDhcpSetup();
            setConfigurationsPriorToIpClientProvisioning(config);
            ScanDetailCache scanDetailCache =
                    mWifiConfigManager.getScanDetailCacheForNetwork(config.networkId);
            ScanResult scanResult = null;
            if (mLastBssid != null) {
                if (scanDetailCache != null) {
                    scanResult = scanDetailCache.getScanResult(mLastBssid);
                }

                // The cached scan result of connected network would be null at the first
                // connection, try to check full scan result list again to look up matched
                // scan result associated to the current BSSID.
                if (scanResult == null) {
                    scanResult = mScanRequestProxy.getScanResult(mLastBssid);
                }
            }

            final ProvisioningConfiguration.Builder prov;
            ProvisioningConfiguration.ScanResultInfo scanResultInfo = null;
            if (scanResult != null) {
                final List<ScanResultInfo.InformationElement> ies =
                        new ArrayList<ScanResultInfo.InformationElement>();
                for (ScanResult.InformationElement ie : scanResult.getInformationElements()) {
                    ScanResultInfo.InformationElement scanResultInfoIe =
                            new ScanResultInfo.InformationElement(ie.getId(), ie.getBytes());
                    ies.add(scanResultInfoIe);
                }
                scanResultInfo = new ProvisioningConfiguration.ScanResultInfo(scanResult.SSID,
                        scanResult.BSSID, ies);
            }

            if (!isUsingStaticIp) {
                prov = new ProvisioningConfiguration.Builder()
                    .withPreDhcpAction()
                    .withApfCapabilities(mWifiNative.getApfCapabilities(mInterfaceName))
                    .withNetwork(getCurrentNetwork())
                    .withDisplayName(config.SSID)
                    .withScanResultInfo(scanResultInfo)
                    .withLayer2Information(layer2Info);
            } else {
                StaticIpConfiguration staticIpConfig = config.getStaticIpConfiguration();
                prov = new ProvisioningConfiguration.Builder()
                        .withStaticConfiguration(staticIpConfig)
                        .withApfCapabilities(mWifiNative.getApfCapabilities(mInterfaceName))
                        .withNetwork(getCurrentNetwork())
                        .withDisplayName(config.SSID)
                        .withLayer2Information(layer2Info);
            }
            if (isUsingMacRandomization) {
                // Use EUI64 address generation for link-local IPv6 addresses.
                prov.withRandomMacAddress();
            }
            mIpClient.startProvisioning(prov.build());
        }

        return true;
    }

    @Override
    public boolean setWifiConnectedNetworkScorer(IBinder binder,
            IWifiConnectedNetworkScorer scorer) {
        return mWifiScoreReport.setWifiConnectedNetworkScorer(binder, scorer);
    }

    @Override
    public void clearWifiConnectedNetworkScorer() {
        mWifiScoreReport.clearWifiConnectedNetworkScorer();
    }

    @Override
    public void sendMessageToClientModeImpl(Message msg) {
        sendMessage(msg);
    }

    @Override
    public long getId() {
        return mId;
    }

    @Override
    public void dumpWifiScoreReport(FileDescriptor fd, PrintWriter pw, String[] args) {
        mWifiScoreReport.dump(fd, pw, args);
    }

    /**
     * Notifies changes in data connectivity of the default data SIM.
     */
    @Override
    public void onCellularConnectivityChanged(@WifiDataStall.CellularDataStatusCode int status) {
        mWifiConfigManager.onCellularConnectivityChanged(status);
        // do a scan if no cell data and currently not connect to wifi
        if (status == WifiDataStall.CELLULAR_DATA_NOT_AVAILABLE
                && getConnectedWifiConfigurationInternal() == null) {
            if (mContext.getResources().getBoolean(
                    R.bool.config_wifiScanOnCellularDataLossEnabled)) {
                mWifiConnectivityManager.forceConnectivityScan(WIFI_WORK_SOURCE);
            }
        }
    }

    @Override
    public void setMboCellularDataStatus(boolean available) {
        mWifiNative.setMboCellularDataStatus(mInterfaceName, available);
    }

    @Override
    public WifiNative.RoamingCapabilities getRoamingCapabilities() {
        return mWifiNative.getRoamingCapabilities(mInterfaceName);
    }

    @Override
    public boolean configureRoaming(WifiNative.RoamingConfig config) {
        return mWifiNative.configureRoaming(mInterfaceName, config);
    }

    @Override
    public boolean enableRoaming(boolean enabled) {
        int status = mWifiNative.enableFirmwareRoaming(
                mInterfaceName, enabled
                        ? WifiNative.ENABLE_FIRMWARE_ROAMING
                        : WifiNative.DISABLE_FIRMWARE_ROAMING);
        return status == WifiNative.SET_FIRMWARE_ROAMING_SUCCESS;
    }

    @Override
    public boolean setCountryCode(String countryCode) {
        return mWifiNative.setStaCountryCode(mInterfaceName, countryCode);
    }

    @Override
    public List<TxFateReport> getTxPktFates() {
        return mWifiNative.getTxPktFates(mInterfaceName);
    }

    @Override
    public List<RxFateReport> getRxPktFates() {
        return mWifiNative.getRxPktFates(mInterfaceName);
    }

    @Override
    public void setShouldReduceNetworkScore(boolean shouldReduceNetworkScore) {
        mWifiScoreReport.setShouldReduceNetworkScore(shouldReduceNetworkScore);
    }

    private void applyCachedPacketFilter() {
        // If packet filter is supported on both connections, ignore since we would have already
        // applied the filter.
        if (mContext.getResources().getBoolean(R.bool.config_wifiEnableApfOnNonPrimarySta)) return;
        if (mCachedPacketFilter == null) {
            Log.w(TAG, "No cached packet filter to apply");
            return;
        }
        Log.i(TAG, "Applying cached packet filter");
        mWifiNative.installPacketFilter(mInterfaceName, mCachedPacketFilter);
    }

    /**
     * Invoked by parent ConcreteClientModeManager whenever a role change occurs.
     */
    public void onRoleChanged() {
        ClientRole role = mClientModeManager.getRole();
        if (role == ROLE_CLIENT_PRIMARY) {
            applyCachedPacketFilter();
            if (mScreenOn) {
                // Start RSSI polling for the new primary network to enable scoring.
                enableRssiPolling(true);
            }
        } else {
            if (mScreenOn) {
                // Stop RSSI polling (if enabled) for the secondary network.
                enableRssiPolling(false);
            }
        }
        WifiConfiguration connectedNetwork = getConnectedWifiConfiguration();
        if (connectedNetwork != null) {
            updateWifiInfoWhenConnected(connectedNetwork);
            // Update capabilities after a role change.
            updateCapabilities(connectedNetwork);
        }
        mWifiScoreReport.onRoleChanged(role);
    }

    private void addPasspointInfoToLinkProperties(LinkProperties linkProperties) {
        // CaptivePortalData.Builder.setVenueFriendlyName API not available on R
        if (!SdkLevel.isAtLeastS()) {
            return;
        }
        WifiConfiguration currentNetwork = getConnectedWifiConfigurationInternal();
        if (currentNetwork == null || !currentNetwork.isPasspoint()) {
            return;
        }
        ScanResult scanResult = mScanRequestProxy.getScanResult(mLastBssid);

        if (scanResult == null) {
            return;
        }
        URL venueUrl = mPasspointManager.getVenueUrl(scanResult);

        // Update the friendly name to populate the notification
        CaptivePortalData.Builder captivePortalDataBuilder = new CaptivePortalData.Builder()
                .setVenueFriendlyName(currentNetwork.providerFriendlyName);

        // Update the Venue URL if available
        if (venueUrl != null) {
            captivePortalDataBuilder.setVenueInfoUrl(Uri.parse(venueUrl.toString()),
                    CaptivePortalData.CAPTIVE_PORTAL_DATA_SOURCE_PASSPOINT);
        }

        // Update the T&C URL if available. The network is captive if T&C URL is available
        if (mTermsAndConditionsUrl != null) {
            captivePortalDataBuilder.setUserPortalUrl(
                    Uri.parse(mTermsAndConditionsUrl.toString()),
                    CaptivePortalData.CAPTIVE_PORTAL_DATA_SOURCE_PASSPOINT).setCaptive(true);
        }

        linkProperties.setCaptivePortalData(captivePortalDataBuilder.build());
    }

    private boolean mHasQuit = false;

    @Override
    protected void onQuitting() {
        mHasQuit = true;
        mClientModeManager.onClientModeImplQuit();
    }

    /** Returns true if the ClientModeImpl has fully stopped, false otherwise. */
    public boolean hasQuit() {
        return mHasQuit;
    }

    /**
     * WifiVcnNetworkPolicyChangeListener tracks VCN-defined Network policies for a
     * WifiNetworkAgent. These policies are used to restart Networks or update their
     * NetworkCapabilities.
     */
    private class WifiVcnNetworkPolicyChangeListener
            implements VcnManager.VcnNetworkPolicyChangeListener {
        @Override
        public void onPolicyChanged() {
            if (mNetworkAgent == null) {
                return;
            }
            // Update the NetworkAgent's NetworkCapabilities which will merge the current
            // capabilities with VcnManagementService's underlying Network policy.
            Log.i(getTag(), "VCN policy changed, updating NetworkCapabilities.");
            updateCapabilities();
        }
    }

    /**
     * Updates the default gateway mac address of the connected network config and updates the
     * linked networks resulting from the new default gateway.
     */
    private boolean retrieveConnectedNetworkDefaultGateway() {
        WifiConfiguration currentConfig = getConnectedWifiConfiguration();
        if (currentConfig == null) {
            logi("can't fetch config of current network id " + mLastNetworkId);
            return false;
        }

        // Find IPv4 default gateway.
        if (mLinkProperties == null) {
            logi("cannot retrieve default gateway from null link properties");
            return false;
        }
        String gatewayIPv4 = null;
        for (RouteInfo routeInfo : mLinkProperties.getRoutes()) {
            if (routeInfo.isDefaultRoute()
                    && routeInfo.getDestination().getAddress() instanceof Inet4Address
                    && routeInfo.hasGateway()) {
                gatewayIPv4 = routeInfo.getGateway().getHostAddress();
                break;
            }
        }

        if (TextUtils.isEmpty(gatewayIPv4)) {
            logi("default gateway ipv4 is null");
            return false;
        }

        String gatewayMac = macAddressFromRoute(gatewayIPv4);
        if (TextUtils.isEmpty(gatewayMac)) {
            logi("default gateway mac fetch failed for ipv4 addr = " + gatewayIPv4);
            return false;
        }

        logi("Default Gateway MAC address of " + mLastBssid + " from routes is : " + gatewayMac);
        if (!mWifiConfigManager.setNetworkDefaultGwMacAddress(mLastNetworkId, gatewayMac)) {
            logi("default gateway mac set failed for " + currentConfig.getKey() + " network");
            return false;
        }

        return mWifiConfigManager.saveToStore(true);
    }

    /**
     * Links the supplied config to all matching saved configs and updates the WifiBlocklistMonitor
     * SSID allowlist with the linked networks.
     */
    private void updateLinkedNetworks(@NonNull WifiConfiguration config) {
        if (!mContext.getResources().getBoolean(R.bool.config_wifiEnableLinkedNetworkRoaming)
                || !config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_PSK)) {
            return;
        }

        mWifiConfigManager.updateLinkedNetworks(config.networkId);
        Map<String, WifiConfiguration> linkedNetworks = mWifiConfigManager
                .getLinkedNetworksWithoutMasking(config.networkId);
        if (!mWifiNative.updateLinkedNetworks(mInterfaceName, config.networkId, linkedNetworks)) {
            return;
        }

        List<String> allowlistSsids = new ArrayList<>(linkedNetworks.values().stream()
                .map(linkedConfig -> linkedConfig.SSID)
                .collect(Collectors.toList()));
        if (linkedNetworks.size() > 0) {
            allowlistSsids.add(config.SSID);
        }
        mWifiBlocklistMonitor.setAllowlistSsids(config.SSID, allowlistSsids);
        mWifiBlocklistMonitor.updateFirmwareRoamingConfiguration(new ArraySet<>(allowlistSsids));
    }

    private boolean checkAndHandleLinkedNetworkRoaming(String associatedBssid) {
        if (!mContext.getResources().getBoolean(R.bool.config_wifiEnableLinkedNetworkRoaming)) {
            return false;
        }

        ScanResult scanResult = mScanRequestProxy.getScanResult(associatedBssid);
        if (scanResult == null) {
            return false;
        }

        WifiConfiguration config = mWifiConfigManager
                .getSavedNetworkForScanResult(scanResult);
        if (config == null || !config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_PSK)
                || mLastNetworkId == config.networkId) {
            return false;
        }

        mIsLinkedNetworkRoaming = true;
        setTargetBssid(config, associatedBssid);
        mTargetNetworkId = config.networkId;
        mTargetWifiConfiguration = config;
        mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
        sendNetworkChangeBroadcast(DetailedState.CONNECTING);
        mWifiInfo.setFrequency(scanResult.frequency);
        mWifiInfo.setBSSID(associatedBssid);
        return true;
    }

    @RequiresApi(Build.VERSION_CODES.S)
    private @WifiConfiguration.RecentFailureReason int
            mboAssocDisallowedReasonCodeToWifiConfigurationRecentFailureReason(
            @MboOceConstants.MboAssocDisallowedReasonCode int reasonCode) {
        switch (reasonCode) {
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_MAX_NUM_STA_ASSOCIATED:
                return WifiConfiguration.RECENT_FAILURE_MBO_ASSOC_DISALLOWED_MAX_NUM_STA_ASSOCIATED;
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_AIR_INTERFACE_OVERLOADED:
                return WifiConfiguration
                        .RECENT_FAILURE_MBO_ASSOC_DISALLOWED_AIR_INTERFACE_OVERLOADED;
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_AUTH_SERVER_OVERLOADED:
                return WifiConfiguration.RECENT_FAILURE_MBO_ASSOC_DISALLOWED_AUTH_SERVER_OVERLOADED;
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_INSUFFICIENT_RSSI:
                return WifiConfiguration.RECENT_FAILURE_MBO_ASSOC_DISALLOWED_INSUFFICIENT_RSSI;
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_UNSPECIFIED:
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_RESERVED_0:
            case MboOceConstants.MBO_ASSOC_DISALLOWED_REASON_RESERVED:
            default:
                return WifiConfiguration.RECENT_FAILURE_MBO_ASSOC_DISALLOWED_UNSPECIFIED;
        }
    }

    /**
     * To set association rejection status in wifi config.
     * @param netId The network ID.
     * @param assocRejectEventInfo Association rejection information.
     */
    private void setAssociationRejectionStatusInConfig(int netId,
            AssocRejectEventInfo assocRejectEventInfo) {
        int statusCode = assocRejectEventInfo.statusCode;
        @WifiConfiguration.RecentFailureReason int reason;

        switch (statusCode) {
            case StatusCode.AP_UNABLE_TO_HANDLE_NEW_STA:
                reason = WifiConfiguration.RECENT_FAILURE_AP_UNABLE_TO_HANDLE_NEW_STA;
                break;
            case StatusCode.ASSOC_REJECTED_TEMPORARILY:
                reason = WifiConfiguration.RECENT_FAILURE_REFUSED_TEMPORARILY;
                break;
            case StatusCode.DENIED_POOR_CHANNEL_CONDITIONS:
                reason = WifiConfiguration.RECENT_FAILURE_POOR_CHANNEL_CONDITIONS;
                break;
            default:
                // do nothing
                return;
        }

        if (SdkLevel.isAtLeastS()) {
            if (assocRejectEventInfo.mboAssocDisallowedInfo != null) {
                reason = mboAssocDisallowedReasonCodeToWifiConfigurationRecentFailureReason(
                        assocRejectEventInfo.mboAssocDisallowedInfo.mReasonCode);
            } else if (assocRejectEventInfo.oceRssiBasedAssocRejectInfo != null) {
                reason = WifiConfiguration.RECENT_FAILURE_OCE_RSSI_BASED_ASSOCIATION_REJECTION;
            }
        }

        mWifiConfigManager.setRecentFailureAssociationStatus(netId, reason);

    }
}
