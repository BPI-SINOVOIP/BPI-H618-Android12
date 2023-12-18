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

import static android.app.AppOpsManager.MODE_ALLOWED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.net.wifi.WifiManager.LocalOnlyHotspotCallback.ERROR_GENERIC;
import static android.net.wifi.WifiManager.LocalOnlyHotspotCallback.ERROR_NO_CHANNEL;
import static android.net.wifi.WifiManager.SAP_START_FAILURE_NO_CHANNEL;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_FAILED;

import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_LOCAL_ONLY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_LONG_LIVED;
import static com.android.server.wifi.ClientModeImpl.RESET_SIM_REASON_DEFAULT_DATA_SIM_CHANGED;
import static com.android.server.wifi.ClientModeImpl.RESET_SIM_REASON_SIM_INSERTED;
import static com.android.server.wifi.ClientModeImpl.RESET_SIM_REASON_SIM_REMOVED;
import static com.android.server.wifi.SelfRecovery.REASON_API_CALL;
import static com.android.server.wifi.WifiConfigurationUtil.addSecurityTypeToNetworkId;
import static com.android.server.wifi.WifiConfigurationUtil.convertWifiInfoSecurityTypeToWifiConfiguration;
import static com.android.server.wifi.WifiConfigurationUtil.removeSecurityTypeFromNetworkId;
import static com.android.server.wifi.WifiSettingsConfigStore.WIFI_VERBOSE_LOGGING_ENABLED;

import android.Manifest;
import android.annotation.CheckResult;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.AppOpsManager;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.DhcpInfo;
import android.net.DhcpResultsParcelable;
import android.net.InetAddresses;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkStack;
import android.net.Uri;
import android.net.ip.IpClientUtil;
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
import android.net.wifi.IWifiVerboseLoggingStatusChangedListener;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.SoftApInfo;
import android.net.wifi.WifiAnnotations.WifiStandard;
import android.net.wifi.WifiAvailableChannel;
import android.net.wifi.WifiClient;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.AddNetworkResult;
import android.net.wifi.WifiManager.CoexRestriction;
import android.net.wifi.WifiManager.DeviceMobilityState;
import android.net.wifi.WifiManager.LocalOnlyHotspotCallback;
import android.net.wifi.WifiManager.SapClientBlockedReason;
import android.net.wifi.WifiManager.SapStartFailure;
import android.net.wifi.WifiManager.SuggestionConnectionStatusListener;
import android.net.wifi.WifiManager.WifiApState;
import android.net.wifi.WifiNetworkSuggestion;
import android.net.wifi.WifiScanner;
import android.net.wifi.hotspot2.IProvisioningCallback;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.hotspot2.PasspointConfiguration;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.PersistableBundle;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.WorkSource;
import android.os.connectivity.WifiActivityEnergyInfo;
import android.provider.Settings;
import android.telephony.CarrierConfigManager;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.modules.utils.ParceledListSlice;
import com.android.modules.utils.build.SdkLevel;
import com.android.net.module.util.Inet4AddressUtils;
import com.android.server.wifi.coex.CoexManager;
import com.android.server.wifi.hotspot2.PasspointManager;
import com.android.server.wifi.hotspot2.PasspointProvider;
import com.android.server.wifi.proto.nano.WifiMetricsProto.UserActionEvent;
import com.android.server.wifi.util.ActionListenerWrapper;
import com.android.server.wifi.util.ApConfigUtil;
import com.android.server.wifi.util.GeneralUtil.Mutable;
import com.android.server.wifi.util.LastCallerInfoManager;
import com.android.server.wifi.util.RssiUtil;
import com.android.server.wifi.util.ScanResultUtil;
import com.android.server.wifi.util.WifiPermissionsUtil;
import com.android.wifi.resources.R;

import java.io.BufferedReader;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.security.GeneralSecurityException;
import java.security.KeyStore;
import java.security.cert.CertPath;
import java.security.cert.CertPathValidator;
import java.security.cert.CertificateFactory;
import java.security.cert.PKIXParameters;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

/**
 * WifiService handles remote WiFi operation requests by implementing
 * the IWifiManager interface.
 */
public class WifiServiceImpl extends BaseWifiService {
    private static final String TAG = "WifiService";
    private static final boolean VDBG = false;

    /** Max wait time for posting blocking runnables */
    private static final int RUN_WITH_SCISSORS_TIMEOUT_MILLIS = 4000;
    @VisibleForTesting
    static final int AUTO_DISABLE_SHOW_KEY_COUNTDOWN_MILLIS = 24 * 60 * 60 * 1000;

    private final ActiveModeWarden mActiveModeWarden;
    private final ScanRequestProxy mScanRequestProxy;

    private final Context mContext;
    private final FrameworkFacade mFacade;
    private final Clock mClock;

    private final PowerManager mPowerManager;
    private final AppOpsManager mAppOps;
    private final UserManager mUserManager;
    private final WifiCountryCode mCountryCode;

    /** Polls traffic stats and notifies clients */
    private final WifiTrafficPoller mWifiTrafficPoller;
    /** Tracks the persisted states for wi-fi & airplane mode */
    private final WifiSettingsStore mSettingsStore;
    /** Logs connection events and some general router and scan stats */
    private final WifiMetrics mWifiMetrics;

    private final WifiInjector mWifiInjector;
    /** Backup/Restore Module */
    private final WifiBackupRestore mWifiBackupRestore;
    private final SoftApBackupRestore mSoftApBackupRestore;
    private final CoexManager mCoexManager;
    private final WifiNetworkSuggestionsManager mWifiNetworkSuggestionsManager;
    private final WifiConfigManager mWifiConfigManager;
    private final PasspointManager mPasspointManager;
    private final WifiLog mLog;
    private final WifiConnectivityManager mWifiConnectivityManager;
    private final ConnectHelper mConnectHelper;
    private final WifiGlobals mWifiGlobals;
    private final WifiCarrierInfoManager mWifiCarrierInfoManager;
    private @WifiManager.VerboseLoggingLevel int mVerboseLoggingLevel =
            WifiManager.VERBOSE_LOGGING_LEVEL_DISABLED;
    private final RemoteCallbackList<IWifiVerboseLoggingStatusChangedListener>
            mRegisteredWifiLoggingStatusListeners = new RemoteCallbackList<>();

    private final FrameworkFacade mFrameworkFacade;

    private final WifiPermissionsUtil mWifiPermissionsUtil;

    private final TetheredSoftApTracker mTetheredSoftApTracker;

    private final LohsSoftApTracker mLohsSoftApTracker;

    private final BuildProperties mBuildProperties;

    private final DefaultClientModeManager mDefaultClientModeManager;

    /**
     * Callback for use with LocalOnlyHotspot to unregister requesting applications upon death.
     */
    public final class LocalOnlyRequestorCallback
            implements LocalOnlyHotspotRequestInfo.RequestingApplicationDeathCallback {
        /**
         * Called with requesting app has died.
         */
        @Override
        public void onLocalOnlyHotspotRequestorDeath(LocalOnlyHotspotRequestInfo requestor) {
            mLog.trace("onLocalOnlyHotspotRequestorDeath pid=%")
                    .c(requestor.getPid()).flush();
            mLohsSoftApTracker.stopByRequest(requestor);
        }
    }

    /**
     * Listen for phone call state events to get active data subcription id.
     */
    private class WifiPhoneStateListener extends PhoneStateListener {
        WifiPhoneStateListener(Looper looper) {
            super(new HandlerExecutor(new Handler(looper)));
        }

        @Override
        public void onActiveDataSubscriptionIdChanged(int subId) {
            // post operation to handler thread
            mWifiThreadRunner.post(() -> {
                Log.d(TAG, "OBSERVED active data subscription change, subId: " + subId);
                mTetheredSoftApTracker.updateSoftApCapabilityWhenCarrierConfigChanged(subId);
                mActiveModeWarden.updateSoftApCapability(
                        mTetheredSoftApTracker.getSoftApCapability());
            });
        }
    }

    private final WifiLockManager mWifiLockManager;
    private final WifiMulticastLockManager mWifiMulticastLockManager;
    private final DppManager mDppManager;
    private final WifiApConfigStore mWifiApConfigStore;
    private final WifiThreadRunner mWifiThreadRunner;
    private final MemoryStoreImpl mMemoryStoreImpl;
    private final WifiScoreCard mWifiScoreCard;
    private final WifiHealthMonitor mWifiHealthMonitor;
    private final WifiDataStall mWifiDataStall;
    private final WifiNative mWifiNative;
    private final SimRequiredNotifier mSimRequiredNotifier;
    private final MakeBeforeBreakManager mMakeBeforeBreakManager;
    private final LastCallerInfoManager mLastCallerInfoManager;

    /**
     * The wrapper of SoftApCallback is used in WifiService internally.
     * see: {@code WifiManager.SoftApCallback}
     */
    public interface SoftApCallbackInternal {
        /**
         * see: {@code WifiManager.SoftApCallback#onStateChanged(int, int)}
         */
        default void onStateChanged(@WifiApState int state, @SapStartFailure int failureReason) {}

        /**
         * The callback which only is used in service internally and pass to WifiManager.
         * It will base on the change to send corresponding callback as below:
         * 1. onInfoChanged(SoftApInfo)
         * 2. onInfoChanged(List<SoftApInfo>)
         * 3. onConnectedClientsChanged(SoftApInfo, List<WifiClient>)
         * 4. onConnectedClientsChanged(List<WifiClient>)
         */
        default void onConnectedClientsOrInfoChanged(Map<String, SoftApInfo> infos,
                Map<String, List<WifiClient>> clients, boolean isBridged) {}

        /**
         * see: {@code WifiManager.SoftApCallback#onCapabilityChanged(SoftApCapability)}
         */
        default void onCapabilityChanged(@NonNull SoftApCapability softApCapability) {}

        /**
         * see: {@code WifiManager.SoftApCallback#onBlockedClientConnecting(WifiClient, int)}
         */
        default void onBlockedClientConnecting(@NonNull WifiClient client,
                @SapClientBlockedReason int blockedReason) {}
    }


    public WifiServiceImpl(Context context, WifiInjector wifiInjector) {
        mContext = context;
        mWifiInjector = wifiInjector;
        mClock = wifiInjector.getClock();

        mFacade = mWifiInjector.getFrameworkFacade();
        mWifiMetrics = mWifiInjector.getWifiMetrics();
        mWifiTrafficPoller = mWifiInjector.getWifiTrafficPoller();
        mUserManager = mWifiInjector.getUserManager();
        mCountryCode = mWifiInjector.getWifiCountryCode();
        mActiveModeWarden = mWifiInjector.getActiveModeWarden();
        mScanRequestProxy = mWifiInjector.getScanRequestProxy();
        mSettingsStore = mWifiInjector.getWifiSettingsStore();
        mPowerManager = mContext.getSystemService(PowerManager.class);
        mAppOps = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
        mWifiLockManager = mWifiInjector.getWifiLockManager();
        mWifiMulticastLockManager = mWifiInjector.getWifiMulticastLockManager();
        mWifiBackupRestore = mWifiInjector.getWifiBackupRestore();
        mSoftApBackupRestore = mWifiInjector.getSoftApBackupRestore();
        mWifiApConfigStore = mWifiInjector.getWifiApConfigStore();
        mWifiPermissionsUtil = mWifiInjector.getWifiPermissionsUtil();
        mLog = mWifiInjector.makeLog(TAG);
        mFrameworkFacade = wifiInjector.getFrameworkFacade();
        mTetheredSoftApTracker = new TetheredSoftApTracker();
        mActiveModeWarden.registerSoftApCallback(mTetheredSoftApTracker);
        mLohsSoftApTracker = new LohsSoftApTracker();
        mActiveModeWarden.registerLohsCallback(mLohsSoftApTracker);
        mWifiNetworkSuggestionsManager = mWifiInjector.getWifiNetworkSuggestionsManager();
        mDppManager = mWifiInjector.getDppManager();
        mWifiThreadRunner = mWifiInjector.getWifiThreadRunner();
        mWifiConfigManager = mWifiInjector.getWifiConfigManager();
        mPasspointManager = mWifiInjector.getPasspointManager();
        mWifiScoreCard = mWifiInjector.getWifiScoreCard();
        mWifiHealthMonitor = wifiInjector.getWifiHealthMonitor();
        mMemoryStoreImpl = new MemoryStoreImpl(mContext, mWifiInjector,
                mWifiScoreCard,  mWifiHealthMonitor);
        mWifiConnectivityManager = wifiInjector.getWifiConnectivityManager();
        mWifiDataStall = wifiInjector.getWifiDataStall();
        mWifiNative = wifiInjector.getWifiNative();
        mCoexManager = wifiInjector.getCoexManager();
        mConnectHelper = wifiInjector.getConnectHelper();
        mWifiGlobals = wifiInjector.getWifiGlobals();
        mSimRequiredNotifier = wifiInjector.getSimRequiredNotifier();
        mWifiCarrierInfoManager = wifiInjector.getWifiCarrierInfoManager();
        mMakeBeforeBreakManager = mWifiInjector.getMakeBeforeBreakManager();
        mLastCallerInfoManager = mWifiInjector.getLastCallerInfoManager();
        mBuildProperties = mWifiInjector.getBuildProperties();
        mDefaultClientModeManager = mWifiInjector.getDefaultClientModeManager();
    }

    /**
     * Check if we are ready to start wifi.
     *
     * First check if we will be restarting system services to decrypt the device. If the device is
     * not encrypted, check if Wi-Fi needs to be enabled and start if needed
     *
     * This function is used only at boot time.
     */
    public void checkAndStartWifi() {
        mWifiThreadRunner.post(() -> {
            if (!mWifiConfigManager.loadFromStore()) {
                Log.e(TAG, "Failed to load from config store");
            }
            mWifiConfigManager.incrementNumRebootsSinceLastUse();
            // config store is read, check if verbose logging is enabled.
            enableVerboseLoggingInternal(
                    mWifiInjector.getSettingsConfigStore().get(WIFI_VERBOSE_LOGGING_ENABLED)
                    ? 1 : 0);
            // Check if wi-fi needs to be enabled
            boolean wifiEnabled = mSettingsStore.isWifiToggleEnabled();
            Log.i(TAG,
                    "WifiService starting up with Wi-Fi " + (wifiEnabled ? "enabled" : "disabled"));

            mWifiInjector.getWifiScanAlwaysAvailableSettingsCompatibility().initialize();
            mWifiInjector.getWifiNotificationManager().createNotificationChannels();
            mContext.registerReceiver(
                    new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            int state = intent.getIntExtra(TelephonyManager.EXTRA_SIM_STATE,
                                    TelephonyManager.SIM_STATE_UNKNOWN);
                            if (TelephonyManager.SIM_STATE_ABSENT == state) {
                                Log.d(TAG, "resetting networks because SIM was removed");
                                resetCarrierNetworks(RESET_SIM_REASON_SIM_REMOVED);
                            }
                        }
                    },
                    new IntentFilter(TelephonyManager.ACTION_SIM_CARD_STATE_CHANGED));

            mContext.registerReceiver(
                    new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            int state = intent.getIntExtra(TelephonyManager.EXTRA_SIM_STATE,
                                    TelephonyManager.SIM_STATE_UNKNOWN);
                            if (TelephonyManager.SIM_STATE_LOADED == state) {
                                Log.d(TAG, "resetting networks because SIM was loaded");
                                resetCarrierNetworks(RESET_SIM_REASON_SIM_INSERTED);
                            }
                        }
                    },
                    new IntentFilter(TelephonyManager.ACTION_SIM_APPLICATION_STATE_CHANGED));

            mContext.registerReceiver(
                    new BroadcastReceiver() {
                        private int mLastSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            final int subId = intent.getIntExtra("subscription",
                                    SubscriptionManager.INVALID_SUBSCRIPTION_ID);
                            if (subId != mLastSubId) {
                                Log.d(TAG, "resetting networks as default data SIM is changed");
                                resetCarrierNetworks(RESET_SIM_REASON_DEFAULT_DATA_SIM_CHANGED);
                                mLastSubId = subId;
                                mWifiThreadRunner.post(() -> {
                                    mWifiDataStall.resetPhoneStateListener();
                                });
                            }
                        }
                    },
                    new IntentFilter(TelephonyManager.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED));

            mContext.registerReceiver(
                    new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        String countryCode = intent.getStringExtra(
                                TelephonyManager.EXTRA_NETWORK_COUNTRY);
                        Log.d(TAG, "Country code changed to :" + countryCode);
                        mCountryCode.setTelephonyCountryCodeAndUpdate(countryCode);
                    }}, new IntentFilter(TelephonyManager.ACTION_NETWORK_COUNTRY_CHANGED));
            mContext.registerReceiver(
                    new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            Log.d(TAG, "locale changed");
                            resetNotificationManager();
                        }}, new IntentFilter(Intent.ACTION_LOCALE_CHANGED));

            // Adding optimizations of only receiving broadcasts when wifi is enabled
            // can result in race conditions when apps toggle wifi in the background
            // without active user involvement. Always receive broadcasts.
            registerForBroadcasts();
            mInIdleMode = mPowerManager.isDeviceIdleMode();

            mActiveModeWarden.start();
            registerForCarrierConfigChange();
            mWifiInjector.getAdaptiveConnectivityEnabledSettingObserver().initialize();
        });
    }

    private void resetCarrierNetworks(@ClientModeImpl.ResetSimReason int resetReason) {
        mWifiThreadRunner.post(() -> {
            Log.d(TAG, "resetting carrier networks since SIM was changed");
            if (resetReason == RESET_SIM_REASON_SIM_INSERTED
                    || resetReason == RESET_SIM_REASON_DEFAULT_DATA_SIM_CHANGED) {
                // clear all SIM related notifications since some action was taken to address
                // "missing" SIM issue
                mSimRequiredNotifier.dismissSimRequiredNotification();
            }
            if (resetReason != RESET_SIM_REASON_SIM_INSERTED) {
                mWifiConfigManager.resetSimNetworks();
                mWifiNetworkSuggestionsManager.resetSimNetworkSuggestions();
                mPasspointManager.resetSimPasspointNetwork();
                mWifiConfigManager.stopRestrictingAutoJoinToSubscriptionId();
            }

            // do additional handling if we are current connected to a sim auth network
            for (ClientModeManager cmm : mActiveModeWarden.getClientModeManagers()) {
                cmm.resetSimAuthNetworks(resetReason);
            }
            mWifiNetworkSuggestionsManager.resetCarrierPrivilegedApps();
            if (resetReason == RESET_SIM_REASON_SIM_INSERTED) {
                // clear the blocklists in case any SIM based network were disabled due to the SIM
                // not being available.
                mWifiConfigManager.enableTemporaryDisabledNetworks();
                mWifiConnectivityManager.forceConnectivityScan(ClientModeImpl.WIFI_WORK_SOURCE);
            } else {
                // Remove all ephemeral carrier networks keep subscriptionId update with SIM changes
                mWifiConfigManager.removeEphemeralCarrierNetworks();
            }
        });
    }

    public void handleBootCompleted() {
        mWifiThreadRunner.post(() -> {
            Log.d(TAG, "Handle boot completed");

            // Register for system broadcasts.
            IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(Intent.ACTION_USER_REMOVED);
            intentFilter.addAction(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED);
            intentFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
            intentFilter.addAction(PowerManager.ACTION_DEVICE_IDLE_MODE_CHANGED);
            intentFilter.addAction(Intent.ACTION_SHUTDOWN);
            mContext.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    if (action.equals(Intent.ACTION_USER_REMOVED)) {
                        UserHandle userHandle = intent.getParcelableExtra(Intent.EXTRA_USER);
                        if (userHandle == null) {
                            Log.e(TAG, "User removed broadcast received with no user handle");
                            return;
                        }
                        mWifiThreadRunner.post(() -> mWifiConfigManager
                                .removeNetworksForUser(userHandle.getIdentifier()));
                    } else if (action.equals(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED)) {
                        int state = intent.getIntExtra(BluetoothAdapter.EXTRA_CONNECTION_STATE,
                                BluetoothAdapter.STATE_DISCONNECTED);
                        boolean isConnected = state != BluetoothAdapter.STATE_DISCONNECTED;
                        mWifiGlobals.setBluetoothConnected(isConnected);
                        for (ClientModeManager cmm : mActiveModeWarden.getClientModeManagers()) {
                            cmm.onBluetoothConnectionStateChanged();
                        }
                    } else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                        int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                                BluetoothAdapter.STATE_OFF);
                        boolean isEnabled = state != BluetoothAdapter.STATE_OFF;
                        mWifiGlobals.setBluetoothEnabled(isEnabled);
                        for (ClientModeManager cmm : mActiveModeWarden.getClientModeManagers()) {
                            cmm.onBluetoothConnectionStateChanged();
                        }
                    } else if (action.equals(PowerManager.ACTION_DEVICE_IDLE_MODE_CHANGED)) {
                        handleIdleModeChanged();
                    } else if (action.equals(Intent.ACTION_SHUTDOWN)) {
                        handleShutDown();
                    }
                }
            }, intentFilter);
            mMemoryStoreImpl.start();
            mPasspointManager.initializeProvisioner(
                    mWifiInjector.getPasspointProvisionerHandlerThread().getLooper());
            mWifiInjector.getWifiNetworkFactory().register();
            mWifiInjector.getUntrustedWifiNetworkFactory().register();
            mWifiInjector.getOemWifiNetworkFactory().register();
            mWifiInjector.getWifiP2pConnection().handleBootCompleted();
            // Start to listen country code change.
            mCountryCode.registerListener(new CountryCodeListenerProxy());
            mTetheredSoftApTracker.handleBootCompleted();
            mWifiInjector.getSarManager().handleBootCompleted();
        });
    }

    public void handleUserSwitch(int userId) {
        Log.d(TAG, "Handle user switch " + userId);

        mWifiThreadRunner.post(() -> {
            mWifiConfigManager.handleUserSwitch(userId);
            resetNotificationManager();
        });
    }

    public void handleUserUnlock(int userId) {
        Log.d(TAG, "Handle user unlock " + userId);
        mWifiThreadRunner.post(() -> mWifiConfigManager.handleUserUnlock(userId));
    }

    public void handleUserStop(int userId) {
        Log.d(TAG, "Handle user stop " + userId);
        mWifiThreadRunner.post(() -> mWifiConfigManager.handleUserStop(userId));
    }

    /**
     * See {@link android.net.wifi.WifiManager#startScan}
     *
     * @param packageName Package name of the app that requests wifi scan.
     * @param featureId The feature in the package
     */
    @Override
    public boolean startScan(String packageName, String featureId) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }

        int callingUid = Binder.getCallingUid();
        long ident = Binder.clearCallingIdentity();
        mLog.info("startScan uid=%").c(callingUid).flush();
        synchronized (this) {
            if (mInIdleMode) {
                // Need to send an immediate scan result broadcast in case the
                // caller is waiting for a result ..

                // TODO: investigate if the logic to cancel scans when idle can move to
                // WifiScanningServiceImpl.  This will 1 - clean up WifiServiceImpl and 2 -
                // avoid plumbing an awkward path to report a cancelled/failed scan.  This will
                // be sent directly until b/31398592 is fixed.
                sendFailedScanBroadcast();
                mScanPending = true;
                return false;
            }
        }
        try {
            mWifiPermissionsUtil.enforceCanAccessScanResults(packageName, featureId, callingUid,
                    null);
            Boolean scanSuccess = mWifiThreadRunner.call(() ->
                    mScanRequestProxy.startScan(callingUid, packageName), null);
            if (scanSuccess == null) {
                sendFailedScanBroadcast();
                return false;
            }
            if (!scanSuccess) {
                Log.e(TAG, "Failed to start scan");
                return false;
            }
        } catch (SecurityException e) {
            Log.w(TAG, "Permission violation - startScan not allowed for"
                    + " uid=" + callingUid + ", packageName=" + packageName + ", reason=" + e);
            return false;
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
        return true;
    }

    // Send a failed scan broadcast to indicate the current scan request failed.
    private void sendFailedScanBroadcast() {
        // clear calling identity to send broadcast
        long callingIdentity = Binder.clearCallingIdentity();
        try {
            Intent intent = new Intent(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
            intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(WifiManager.EXTRA_RESULTS_UPDATED, false);
            mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
        } finally {
            // restore calling identity
            Binder.restoreCallingIdentity(callingIdentity);
        }

    }

    /**
     * WPS support in Client mode is deprecated.  Return null.
     */
    @Override
    public String getCurrentNetworkWpsNfcConfigurationToken() {
        // while CLs are in flight, return null here, will be removed (b/72423090)
        enforceNetworkStackPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getCurrentNetworkWpsNfcConfigurationToken uid=%")
                    .c(Binder.getCallingUid()).flush();
        }
        return null;
    }

    private boolean mInIdleMode;
    private boolean mScanPending;

    private void handleIdleModeChanged() {
        boolean doScan = false;
        synchronized (this) {
            boolean idle = mPowerManager.isDeviceIdleMode();
            if (mInIdleMode != idle) {
                mInIdleMode = idle;
                if (!idle) {
                    if (mScanPending) {
                        mScanPending = false;
                        doScan = true;
                    }
                }
            }
        }
        if (doScan) {
            // Someone requested a scan while we were idle; do a full scan now.
            // A security check of the caller's identity was made when the request arrived via
            // Binder. Now we'll pass the current process's identity to startScan().
            startScan(mContext.getOpPackageName(), mContext.getAttributionTag());
        }
    }

    private void handleShutDown() {
        // Direct call to notify ActiveModeWarden as soon as possible with the assumption that
        // notifyShuttingDown() doesn't have codes that may cause concurrentModificationException,
        // e.g., access to a collection.
        mActiveModeWarden.notifyShuttingDown();
        mWifiThreadRunner.post(()-> {
            // There is no explicit disconnection event in clientModeImpl during shutdown.
            // Call resetConnectionState() so that connection duration is calculated
            // before memory store write triggered by mMemoryStoreImpl.stop().
            mWifiScoreCard.resetAllConnectionStates();
            mMemoryStoreImpl.stop();
        });
    }

    private boolean checkNetworkSettingsPermission(int pid, int uid) {
        return mContext.checkPermission(android.Manifest.permission.NETWORK_SETTINGS, pid, uid)
                == PERMISSION_GRANTED;
    }

    private boolean checkNetworkSetupWizardPermission(int pid, int uid) {
        return mContext.checkPermission(android.Manifest.permission.NETWORK_SETUP_WIZARD, pid, uid)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean checkMainlineNetworkStackPermission(int pid, int uid) {
        return mContext.checkPermission(NetworkStack.PERMISSION_MAINLINE_NETWORK_STACK, pid, uid)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean checkNetworkStackPermission(int pid, int uid) {
        return mContext.checkPermission(android.Manifest.permission.NETWORK_STACK, pid, uid)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean checkNetworkManagedProvisioningPermission(int pid, int uid) {
        return mContext.checkPermission(android.Manifest.permission.NETWORK_MANAGED_PROVISIONING,
                pid, uid) == PackageManager.PERMISSION_GRANTED;
    }

    /**
     * Helper method to check if the entity initiating the binder call has any of the signature only
     * permissions.
     */
    private boolean isPrivileged(int pid, int uid) {
        return checkNetworkSettingsPermission(pid, uid)
                || checkNetworkSetupWizardPermission(pid, uid)
                || checkNetworkStackPermission(pid, uid)
                || checkNetworkManagedProvisioningPermission(pid, uid)
                || isSignedWithPlatformKey(uid);
    }

    /** Whether the uid is signed with the same key as the platform. */
    private boolean isSignedWithPlatformKey(int uid) {
        return mContext.getPackageManager().checkSignatures(uid, Process.SYSTEM_UID)
                == PackageManager.SIGNATURE_MATCH;
    }

    /**
     * Helper method to check if the entity initiating the binder call has setup wizard or settings
     * permissions.
     */
    private boolean isSettingsOrSuw(int pid, int uid) {
        return checkNetworkSettingsPermission(pid, uid)
                || checkNetworkSetupWizardPermission(pid, uid);
    }

    /** Helper method to check if the entity initiating the binder call is a DO/PO app. */
    private boolean isDeviceOrProfileOwner(int uid, String packageName) {
        return mWifiPermissionsUtil.isDeviceOwner(uid, packageName)
                || mWifiPermissionsUtil.isProfileOwner(uid, packageName);
    }

    private void enforceNetworkSettingsPermission() {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.NETWORK_SETTINGS,
                "WifiService");
    }

    private boolean checkAnyPermissionOf(String... permissions) {
        for (String permission : permissions) {
            if (mContext.checkCallingOrSelfPermission(permission) == PERMISSION_GRANTED) {
                return true;
            }
        }
        return false;
    }

    private void enforceAnyPermissionOf(String... permissions) {
        if (!checkAnyPermissionOf(permissions)) {
            throw new SecurityException("Requires one of the following permissions: "
                    + String.join(", ", permissions) + ".");
        }
    }

    private void enforceNetworkStackPermission() {
        // TODO(b/142554155): Only check for MAINLINE_NETWORK_STACK permission
        boolean granted = mContext.checkCallingOrSelfPermission(
                android.Manifest.permission.NETWORK_STACK)
                == PackageManager.PERMISSION_GRANTED;
        if (granted) {
            return;
        }
        mContext.enforceCallingOrSelfPermission(
                NetworkStack.PERMISSION_MAINLINE_NETWORK_STACK, "WifiService");
    }

    private void enforceAccessPermission() {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.ACCESS_WIFI_STATE,
                "WifiService");
    }

    private void enforceRestartWifiSubsystemPermission() {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.RESTART_WIFI_SUBSYSTEM,
                "WifiService");
    }

    /**
     * Checks whether the caller can change the wifi state.
     * Possible results:
     * 1. Operation is allowed. No exception thrown, and AppOpsManager.MODE_ALLOWED returned.
     * 2. Operation is not allowed, and caller must be told about this. SecurityException is thrown.
     * 3. Operation is not allowed, and caller must not be told about this (i.e. must silently
     * ignore the operation). No exception is thrown, and AppOpsManager.MODE_IGNORED returned.
     */
    @CheckResult
    private int enforceChangePermission(String callingPackage) {
        mAppOps.checkPackage(Binder.getCallingUid(), callingPackage);
        if (checkNetworkSettingsPermission(Binder.getCallingPid(), Binder.getCallingUid())) {
            return MODE_ALLOWED;
        }
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.CHANGE_WIFI_STATE,
                "WifiService");

        return mAppOps.noteOp(
                AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Binder.getCallingUid(), callingPackage);
    }

    private void enforceReadCredentialPermission() {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.READ_WIFI_CREDENTIAL,
                                                "WifiService");
    }

    private void enforceMulticastChangePermission() {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.CHANGE_WIFI_MULTICAST_STATE,
                "WifiService");
    }

    private void enforceConnectivityInternalPermission() {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.CONNECTIVITY_INTERNAL,
                "ConnectivityService");
    }

    private void enforceLocationPermission(String pkgName, @Nullable String featureId, int uid) {
        mWifiPermissionsUtil.enforceLocationPermission(pkgName, featureId, uid);
    }

    /**
     * Helper method to check if the app is allowed to access public API's deprecated in
     * {@link Build.VERSION_CODES#Q}.
     * Note: Invoke mAppOps.checkPackage(uid, packageName) before to ensure correct package name.
     */
    private boolean isTargetSdkLessThanQOrPrivileged(String packageName, int pid, int uid) {
        return mWifiPermissionsUtil.isTargetSdkLessThan(packageName, Build.VERSION_CODES.Q, uid)
                || isPrivileged(pid, uid)
                || isDeviceOrProfileOwner(uid, packageName)
                || mWifiPermissionsUtil.isSystem(packageName, uid)
                // TODO(b/140540984): Remove this bypass.
                || mWifiPermissionsUtil.checkSystemAlertWindowPermission(uid, packageName);
    }

    /**
     * Helper method to check if the app is allowed to access public API's deprecated in
     * {@link Build.VERSION_CODES#R}.
     * Note: Invoke mAppOps.checkPackage(uid, packageName) before to ensure correct package name.
     */
    private boolean isTargetSdkLessThanROrPrivileged(String packageName, int pid, int uid) {
        return mWifiPermissionsUtil.isTargetSdkLessThan(packageName, Build.VERSION_CODES.R, uid)
                || isPrivileged(pid, uid)
                || isDeviceOrProfileOwner(uid, packageName)
                || mWifiPermissionsUtil.isSystem(packageName, uid);
    }

    /**
     * Get the current primary ClientModeManager in a thread safe manner, but blocks on the main
     * Wifi thread.
     */
    private ClientModeManager getPrimaryClientModeManagerBlockingThreadSafe() {
        return mWifiThreadRunner.call(
                () -> mActiveModeWarden.getPrimaryClientModeManager(),
                mDefaultClientModeManager);
    }

    /**
     * see {@link android.net.wifi.WifiManager#setWifiEnabled(boolean)}
     * @param enable {@code true} to enable, {@code false} to disable.
     * @return {@code true} if the enable/disable operation was
     *         started or is already in the queue.
     */
    @Override
    public synchronized boolean setWifiEnabled(String packageName, boolean enable) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        boolean isPrivileged = isPrivileged(Binder.getCallingPid(), Binder.getCallingUid());
        if (!isPrivileged && !isDeviceOrProfileOwner(Binder.getCallingUid(), packageName)
                && !mWifiPermissionsUtil.isTargetSdkLessThan(packageName, Build.VERSION_CODES.Q,
                  Binder.getCallingUid())
                && !mWifiPermissionsUtil.isSystem(packageName, Binder.getCallingUid())) {
            mLog.info("setWifiEnabled not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        // If Airplane mode is enabled, only privileged apps are allowed to toggle Wifi
        if (mSettingsStore.isAirplaneModeOn() && !isPrivileged) {
            mLog.err("setWifiEnabled in Airplane mode: only Settings can toggle wifi").flush();
            return false;
        }

        // If SoftAp is enabled, only privileged apps are allowed to toggle wifi
        if (!isPrivileged && mTetheredSoftApTracker.getState() == WIFI_AP_STATE_ENABLED) {
            mLog.err("setWifiEnabled with SoftAp enabled: only Settings can toggle wifi").flush();
            return false;
        }

        mLog.info("setWifiEnabled package=% uid=% enable=%").c(packageName)
                .c(Binder.getCallingUid()).c(enable).flush();
        long ident = Binder.clearCallingIdentity();
        try {
            if (!mSettingsStore.handleWifiToggled(enable)) {
                // Nothing to do if wifi cannot be toggled
                return true;
            }
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
        if (mWifiPermissionsUtil.checkNetworkSettingsPermission(Binder.getCallingUid())) {
            if (enable) {
                mWifiMetrics.logUserActionEvent(UserActionEvent.EVENT_TOGGLE_WIFI_ON);
            } else {
                WifiInfo wifiInfo =
                        getPrimaryClientModeManagerBlockingThreadSafe().syncRequestConnectionInfo();
                mWifiMetrics.logUserActionEvent(UserActionEvent.EVENT_TOGGLE_WIFI_OFF,
                        wifiInfo == null ? -1 : wifiInfo.getNetworkId());
            }
        }
        mWifiMetrics.incrementNumWifiToggles(isPrivileged, enable);
        mActiveModeWarden.wifiToggled(new WorkSource(Binder.getCallingUid(), packageName));
        mLastCallerInfoManager.put(LastCallerInfoManager.WIFI_ENABLED, Process.myTid(),
                Binder.getCallingUid(), Binder.getCallingPid(), packageName, enable);
        if (enable) {
            mReconnectAfterEnable.mShouldReconnect = true;
            mReconnectAfterEnable.mReconnectCnt = 3;
            mWifiThreadRunner.postDelayed(mReconnectAfterEnable, 5000);
            // mWifiThreadRunner.postDelayed(() -> reconnect(packageName), 5000);
        }
        return true;
    }

    private class ReconnectRunnable implements Runnable {
        boolean mShouldReconnect = false;
        int mReconnectCnt;

        boolean isWifiConnected() {
            boolean ret = false;
            try {
                WifiInfo wifiInfo = mWifiThreadRunner.call(
                        () -> getClientModeManagerIfSecondaryCmmRequestedByCallerPresent(
                                UserHandle.myUserId(), mContext.getOpPackageName())
                                .syncRequestConnectionInfo(), new WifiInfo());
                if (wifiInfo != null && wifiInfo.getWifiSsid() != null) {
                    ret = true;
                }
            } finally {
                return ret;
            }
        }

        @Override
        public void run() {
            if (!mShouldReconnect || isWifiConnected()) {
                mReconnectCnt = 0;
                mShouldReconnect = false;
                Log.d("chongyuzhao", "stop reconnect");
                return;
            }
            mWifiThreadRunner.post(() -> {
                mActiveModeWarden.getPrimaryClientModeManager().reconnect(new WorkSource(UserHandle.myUserId()));
            });
            if (mReconnectCnt > 0) mReconnectCnt--;
            if (mReconnectCnt <= 0) mShouldReconnect = false;
            if (mShouldReconnect) {
                Log.d("chongyuzhao", "reconnect again");
                mWifiThreadRunner.postDelayed(this, 5000);
            }
        }
    }
    ReconnectRunnable mReconnectAfterEnable = new ReconnectRunnable();

    @RequiresApi(Build.VERSION_CODES.S)
    @Override
    public void registerSubsystemRestartCallback(ISubsystemRestartCallback callback) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("registerSubsystemRestartCallback uid=%").c(Binder.getCallingUid()).flush();
        }

        mWifiThreadRunner.post(() -> {
            if (!mActiveModeWarden.registerSubsystemRestartCallback(callback)) {
                Log.e(TAG, "registerSubsystemRestartCallback: Failed to register callback");
            }
        });
    }

    @RequiresApi(Build.VERSION_CODES.S)
    @Override
    public void unregisterSubsystemRestartCallback(ISubsystemRestartCallback callback) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("registerSubsystemRestartCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        mWifiThreadRunner.post(() -> {
            if (!mActiveModeWarden.unregisterSubsystemRestartCallback(callback)) {
                Log.e(TAG, "unregisterSubsystemRestartCallback: Failed to register callback");
            }
        });
    }

    @RequiresApi(Build.VERSION_CODES.S)
    @Override
    public void restartWifiSubsystem() {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        enforceRestartWifiSubsystemPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("restartWifiSubsystem uid=%").c(Binder.getCallingUid()).flush();
        }
        mWifiThreadRunner.post(() -> {
            WifiInfo wifiInfo =
                    mActiveModeWarden.getPrimaryClientModeManager().syncRequestConnectionInfo();
            mWifiMetrics.logUserActionEvent(UserActionEvent.EVENT_RESTART_WIFI_SUB_SYSTEM,
                    wifiInfo == null ? -1 : wifiInfo.getNetworkId());
            mWifiInjector.getSelfRecovery().trigger(REASON_API_CALL);
        });
    }

    /**
     * see {@link WifiManager#getWifiState()}
     * @return One of {@link WifiManager#WIFI_STATE_DISABLED},
     *         {@link WifiManager#WIFI_STATE_DISABLING},
     *         {@link WifiManager#WIFI_STATE_ENABLED},
     *         {@link WifiManager#WIFI_STATE_ENABLING},
     *         {@link WifiManager#WIFI_STATE_UNKNOWN}
     */
    @Override
    public int getWifiEnabledState() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiEnabledState uid=%").c(Binder.getCallingUid()).flush();
        }
        return getPrimaryClientModeManagerBlockingThreadSafe().syncGetWifiState();
    }

    /**
     * see {@link WifiManager#getWifiApState()}
     * @return One of {@link WifiManager#WIFI_AP_STATE_DISABLED},
     *         {@link WifiManager#WIFI_AP_STATE_DISABLING},
     *         {@link WifiManager#WIFI_AP_STATE_ENABLED},
     *         {@link WifiManager#WIFI_AP_STATE_ENABLING},
     *         {@link WifiManager#WIFI_AP_STATE_FAILED}
     */
    @Override
    public int getWifiApEnabledState() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiApEnabledState uid=%").c(Binder.getCallingUid()).flush();
        }
        return mTetheredSoftApTracker.getState();
    }

    /**
     * see {@link android.net.wifi.WifiManager#updateInterfaceIpState(String, int)}
     *
     * The possible modes include: {@link WifiManager#IFACE_IP_MODE_TETHERED},
     *                             {@link WifiManager#IFACE_IP_MODE_LOCAL_ONLY},
     *                             {@link WifiManager#IFACE_IP_MODE_CONFIGURATION_ERROR}
     *
     * @param ifaceName String name of the updated interface
     * @param mode new operating mode of the interface
     *
     * @throws SecurityException if the caller does not have permission to call update
     */
    @Override
    public void updateInterfaceIpState(String ifaceName, int mode) {
        // NETWORK_STACK is a signature only permission.
        enforceNetworkStackPermission();
        mLog.info("updateInterfaceIpState uid=%").c(Binder.getCallingUid()).flush();
        // hand off the work to our handler thread
        mWifiThreadRunner.post(() -> mLohsSoftApTracker.updateInterfaceIpState(ifaceName, mode));
    }

    /**
     * see {@link WifiManager#isDefaultCoexAlgorithmEnabled()}
     * @return {@code true} if the default coex algorithm is enabled. {@code false} otherwise.
     */
    @Override
    public boolean isDefaultCoexAlgorithmEnabled() {
        return mContext.getResources().getBoolean(R.bool.config_wifiDefaultCoexAlgorithmEnabled);
    }

    /**
     * see {@link android.net.wifi.WifiManager#setCoexUnsafeChannels(List, int)}
     * @param unsafeChannels List of {@link CoexUnsafeChannel} to avoid.
     * @param restrictions Bitmap of {@link CoexRestriction} specifying the mandatory
     *                     uses of the specified channels.
     */
    @Override
    @RequiresApi(Build.VERSION_CODES.S)
    public void setCoexUnsafeChannels(
            @NonNull List<CoexUnsafeChannel> unsafeChannels, int restrictions) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.WIFI_UPDATE_COEX_UNSAFE_CHANNELS, "WifiService");
        if (unsafeChannels == null) {
            throw new IllegalArgumentException("unsafeChannels cannot be null");
        }
        if (mContext.getResources().getBoolean(R.bool.config_wifiDefaultCoexAlgorithmEnabled)) {
            Log.e(TAG, "setCoexUnsafeChannels called but default coex algorithm is enabled");
            return;
        }
        mWifiThreadRunner.post(() ->
                mCoexManager.setCoexUnsafeChannels(unsafeChannels, restrictions));
    }

    /**
     * See {@link WifiManager#registerCoexCallback(WifiManager.CoexCallback)}
     */
    @RequiresApi(Build.VERSION_CODES.S)
    public void registerCoexCallback(@NonNull ICoexCallback callback) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.WIFI_ACCESS_COEX_UNSAFE_CHANNELS, "WifiService");
        if (callback == null) {
            throw new IllegalArgumentException("callback must not be null");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("registerCoexCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        mWifiThreadRunner.post(() -> mCoexManager.registerRemoteCoexCallback(callback));
    }

    /**
     * See {@link WifiManager#unregisterCoexCallback(WifiManager.CoexCallback)}
     */
    @RequiresApi(Build.VERSION_CODES.S)
    public void unregisterCoexCallback(@NonNull ICoexCallback callback) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.WIFI_ACCESS_COEX_UNSAFE_CHANNELS, "WifiService");
        if (callback == null) {
            throw new IllegalArgumentException("callback must not be null");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("unregisterCoexCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        mWifiThreadRunner.post(() -> mCoexManager.unregisterRemoteCoexCallback(callback));
    }

    /**
     * see {@link android.net.wifi.WifiManager#startSoftAp(WifiConfiguration)}
     * @param wifiConfig SSID, security and channel details as part of WifiConfiguration
     * @return {@code true} if softap start was triggered
     * @throws SecurityException if the caller does not have permission to start softap
     */
    @Override
    public boolean startSoftAp(WifiConfiguration wifiConfig, String packageName) {
        // NETWORK_STACK is a signature only permission.
        enforceNetworkStackPermission();

        mLog.info("startSoftAp uid=%").c(Binder.getCallingUid()).flush();

        SoftApConfiguration softApConfig = null;
        if (wifiConfig != null) {
            softApConfig = ApConfigUtil.fromWifiConfiguration(wifiConfig);
            if (softApConfig == null) {
                return false;
            }
        }

        if (!mTetheredSoftApTracker.setEnablingIfAllowed()) {
            mLog.err("Tethering is already active.").flush();
            return false;
        }

        WorkSource requestorWs = new WorkSource(Binder.getCallingUid(), packageName);
        if (!mWifiThreadRunner.call(
                () -> mActiveModeWarden.canRequestMoreSoftApManagers(requestorWs), false)) {
            // Take down LOHS if it is up.
            mLohsSoftApTracker.stopAll();
        }

        if (!startSoftApInternal(new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, softApConfig,
                mTetheredSoftApTracker.getSoftApCapability()), requestorWs)) {
            mTetheredSoftApTracker.setFailedWhileEnabling();
            return false;
        }
        mLastCallerInfoManager.put(LastCallerInfoManager.SOFT_AP, Process.myTid(),
                Binder.getCallingUid(), Binder.getCallingPid(), packageName, true);
        return true;
    }

    /**
     * see {@link android.net.wifi.WifiManager#startTetheredHotspot(SoftApConfiguration)}
     * @param softApConfig SSID, security and channel details as part of SoftApConfiguration
     * @return {@code true} if softap start was triggered
     * @throws SecurityException if the caller does not have permission to start softap
     */
    @Override
    public boolean startTetheredHotspot(@Nullable SoftApConfiguration softApConfig,
            @NonNull String packageName) {
        // NETWORK_STACK is a signature only permission.
        enforceNetworkStackPermission();

        mLog.info("startTetheredHotspot uid=%").c(Binder.getCallingUid()).flush();

        if (!mTetheredSoftApTracker.setEnablingIfAllowed()) {
            mLog.err("Tethering is already active.").flush();
            return false;
        }

        WorkSource requestorWs = new WorkSource(Binder.getCallingUid(), packageName);
        if (!mWifiThreadRunner.call(
                () -> mActiveModeWarden.canRequestMoreSoftApManagers(requestorWs), false)) {
            // Take down LOHS if it is up.
            mLohsSoftApTracker.stopAll();
        }

        if (!startSoftApInternal(new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, softApConfig,
                mTetheredSoftApTracker.getSoftApCapability()), requestorWs)) {
            mTetheredSoftApTracker.setFailedWhileEnabling();
            return false;
        }
        mLastCallerInfoManager.put(LastCallerInfoManager.TETHERED_HOTSPOT, Process.myTid(),
                Binder.getCallingUid(), Binder.getCallingPid(), packageName, true);
        return true;
    }

    /**
     * Internal method to start softap mode. Callers of this method should have already checked
     * proper permissions beyond the NetworkStack permission.
     */
    private boolean startSoftApInternal(SoftApModeConfiguration apConfig, WorkSource requestorWs) {
        int uid = Binder.getCallingUid();
        boolean privileged = isSettingsOrSuw(Binder.getCallingPid(), uid);
        mLog.trace("startSoftApInternal uid=% mode=%")
                .c(uid).c(apConfig.getTargetMode()).flush();

        // null wifiConfig is a meaningful input for CMD_SET_AP; it means to use the persistent
        // AP config.
        SoftApConfiguration softApConfig = apConfig.getSoftApConfiguration();
        if (softApConfig != null
                && (!WifiApConfigStore.validateApWifiConfiguration(
                    softApConfig, privileged, mContext))) {
            Log.e(TAG, "Invalid SoftApConfiguration");
            return false;
        }

        mActiveModeWarden.startSoftAp(apConfig, requestorWs);
        return true;
    }

    /**
     * see {@link android.net.wifi.WifiManager#stopSoftAp()}
     * @return {@code true} if softap stop was triggered
     * @throws SecurityException if the caller does not have permission to stop softap
     */
    @Override
    public boolean stopSoftAp() {
        // NETWORK_STACK is a signature only permission.
        enforceNetworkStackPermission();

        // only permitted callers are allowed to this point - they must have gone through
        // connectivity service since this method is protected with the NETWORK_STACK PERMISSION

        mLog.info("stopSoftAp uid=%").c(Binder.getCallingUid()).flush();

        stopSoftApInternal(WifiManager.IFACE_IP_MODE_TETHERED);
        mLastCallerInfoManager.put(LastCallerInfoManager.SOFT_AP, Process.myTid(),
                Binder.getCallingUid(), Binder.getCallingPid(), "<unknown>", false);
        return true;
    }

    /**
     * Internal method to stop softap mode.
     *
     * Callers of this method should have already checked
     * proper permissions beyond the NetworkStack permission.
     *
     * @param mode the operating mode of APs to bring down (ex,
     *             {@link WifiManager.IFACE_IP_MODE_TETHERED} or
     *             {@link WifiManager.IFACE_IP_MODE_LOCAL_ONLY}).
     *             Use {@link WifiManager.IFACE_IP_MODE_UNSPECIFIED} to stop all APs.
     */
    private void stopSoftApInternal(int mode) {
        mLog.trace("stopSoftApInternal uid=% mode=%").c(Binder.getCallingUid()).c(mode).flush();

        mActiveModeWarden.stopSoftAp(mode);
    }

    private final class CountryCodeListenerProxy implements WifiCountryCode.ChangeListener {
        @Override
        public void onDriverCountryCodeChanged(String countryCode) {
            // post operation to handler thread
            mWifiThreadRunner.post(() -> {
                Log.i(TAG, "onDriverCountryCodeChanged " + countryCode);
                mTetheredSoftApTracker.updateAvailChannelListInSoftApCapability();
                mActiveModeWarden.updateSoftApCapability(
                        mTetheredSoftApTracker.getSoftApCapability());
            });
        }
    }

    /**
     * SoftAp callback
     */
    private final class TetheredSoftApTracker implements SoftApCallbackInternal {
        /**
         * State of tethered SoftAP
         * One of:  {@link WifiManager#WIFI_AP_STATE_DISABLED},
         *          {@link WifiManager#WIFI_AP_STATE_DISABLING},
         *          {@link WifiManager#WIFI_AP_STATE_ENABLED},
         *          {@link WifiManager#WIFI_AP_STATE_ENABLING},
         *          {@link WifiManager#WIFI_AP_STATE_FAILED}
         */
        private final Object mLock = new Object();
        private int mTetheredSoftApState = WIFI_AP_STATE_DISABLED;
        private Map<String, List<WifiClient>> mTetheredSoftApConnectedClientsMap = new HashMap();
        private Map<String, SoftApInfo> mTetheredSoftApInfoMap = new HashMap();
        private boolean mIsBridgedMode = false;
        // TODO: We need to maintain two capability. One for LTE + SAP and one for WIFI + SAP
        private SoftApCapability mTetheredSoftApCapability = null;
        private boolean mIsBootComplete = false;

        public void handleBootCompleted() {
            mIsBootComplete = true;
            updateAvailChannelListInSoftApCapability();
        }

        public int getState() {
            synchronized (mLock) {
                return mTetheredSoftApState;
            }
        }

        public boolean setEnablingIfAllowed() {
            synchronized (mLock) {
                if (mTetheredSoftApState != WIFI_AP_STATE_DISABLED
                        && mTetheredSoftApState != WIFI_AP_STATE_FAILED) {
                    return false;
                }
                mTetheredSoftApState = WIFI_AP_STATE_ENABLING;
                return true;
            }
        }

        public void setFailedWhileEnabling() {
            synchronized (mLock) {
                if (mTetheredSoftApState == WIFI_AP_STATE_ENABLING) {
                    mTetheredSoftApState = WIFI_AP_STATE_FAILED;
                }
            }
        }

        public Map<String, List<WifiClient>> getConnectedClients() {
            synchronized (mLock) {
                return mTetheredSoftApConnectedClientsMap;
            }
        }

        public Map<String, SoftApInfo> getSoftApInfos() {
            synchronized (mLock) {
                return mTetheredSoftApInfoMap;
            }
        }

        public boolean getIsBridgedMode() {
            synchronized (mLock) {
                return mIsBridgedMode;
            }
        }

        public SoftApCapability getSoftApCapability() {
            synchronized (mLock) {
                if (mTetheredSoftApCapability == null) {
                    mTetheredSoftApCapability = ApConfigUtil.updateCapabilityFromResource(mContext);
                    // Default country code
                    mTetheredSoftApCapability = updateSoftApCapabilityWithAvailableChannelList(
                            mTetheredSoftApCapability);
                }
                return mTetheredSoftApCapability;
            }
        }

        private SoftApCapability updateSoftApCapabilityWithAvailableChannelList(
                @NonNull SoftApCapability softApCapability) {
            SoftApCapability newSoftApCapability = new SoftApCapability(softApCapability);
            if (!mIsBootComplete) {
                // The available channel list is from wificond.
                // It might be a failure or stuck during wificond init.
                return newSoftApCapability;
            }
            List<Integer> supportedChannelList = null;
            if (ApConfigUtil.isSoftAp24GhzSupported(mContext)) {
                supportedChannelList = ApConfigUtil.getAvailableChannelFreqsForBand(
                    SoftApConfiguration.BAND_2GHZ, mWifiNative, mContext.getResources(), false);
                if (supportedChannelList != null) {
                    newSoftApCapability.setSupportedChannelList(
                            SoftApConfiguration.BAND_2GHZ,
                            supportedChannelList.stream().mapToInt(Integer::intValue).toArray());
                }
            }
            if (ApConfigUtil.isSoftAp5GhzSupported(mContext)) {
                supportedChannelList = ApConfigUtil.getAvailableChannelFreqsForBand(
                        SoftApConfiguration.BAND_5GHZ, mWifiNative, mContext.getResources(), false);
                if (supportedChannelList != null) {
                    newSoftApCapability.setSupportedChannelList(
                            SoftApConfiguration.BAND_5GHZ,
                            supportedChannelList.stream().mapToInt(Integer::intValue).toArray());
                }
            }
            if (ApConfigUtil.isSoftAp6GhzSupported(mContext)) {
                supportedChannelList = ApConfigUtil.getAvailableChannelFreqsForBand(
                        SoftApConfiguration.BAND_6GHZ, mWifiNative, mContext.getResources(), false);
                if (supportedChannelList != null) {
                    newSoftApCapability.setSupportedChannelList(
                            SoftApConfiguration.BAND_6GHZ,
                            supportedChannelList.stream().mapToInt(Integer::intValue).toArray());
                }
            }
            if (ApConfigUtil.isSoftAp60GhzSupported(mContext)) {
                supportedChannelList = ApConfigUtil.getAvailableChannelFreqsForBand(
                        SoftApConfiguration.BAND_60GHZ, mWifiNative, mContext.getResources(),
                        false);
                if (supportedChannelList != null) {
                    newSoftApCapability.setSupportedChannelList(
                            SoftApConfiguration.BAND_60GHZ,
                            supportedChannelList.stream().mapToInt(Integer::intValue).toArray());
                }
            }
            return newSoftApCapability;
        }

        public void updateAvailChannelListInSoftApCapability() {
            onCapabilityChanged(updateSoftApCapabilityWithAvailableChannelList(
                    getSoftApCapability()));
        }

        public void updateSoftApCapabilityWhenCarrierConfigChanged(int subId) {
            CarrierConfigManager carrierConfigManager =
                    mContext.getSystemService(CarrierConfigManager.class);
            if (carrierConfigManager == null) return;
            PersistableBundle carrierConfig = carrierConfigManager.getConfigForSubId(subId);
            if (carrierConfig == null) return;
            int carrierMaxClient = carrierConfig.getInt(
                    CarrierConfigManager.Wifi.KEY_HOTSPOT_MAX_CLIENT_COUNT);
            int finalSupportedClientNumber = mContext.getResources().getInteger(
                    R.integer.config_wifiHardwareSoftapMaxClientCount);
            if (carrierMaxClient > 0) {
                finalSupportedClientNumber = Math.min(finalSupportedClientNumber,
                        carrierMaxClient);
            }
            if (finalSupportedClientNumber == getSoftApCapability().getMaxSupportedClients()) {
                return;
            }
            SoftApCapability newSoftApCapability = new SoftApCapability(mTetheredSoftApCapability);
            newSoftApCapability.setMaxSupportedClients(
                    finalSupportedClientNumber);
            onCapabilityChanged(newSoftApCapability);
        }

        private final RemoteCallbackList<ISoftApCallback> mRegisteredSoftApCallbacks =
                new RemoteCallbackList<>();

        public boolean registerSoftApCallback(ISoftApCallback callback) {
            return mRegisteredSoftApCallbacks.register(callback);
        }

        public void unregisterSoftApCallback(ISoftApCallback callback) {
            mRegisteredSoftApCallbacks.unregister(callback);
        }

        /**
         * Called when soft AP state changes.
         *
         * @param state new new AP state. One of {@link #WIFI_AP_STATE_DISABLED},
         *        {@link #WIFI_AP_STATE_DISABLING}, {@link #WIFI_AP_STATE_ENABLED},
         *        {@link #WIFI_AP_STATE_ENABLING}, {@link #WIFI_AP_STATE_FAILED}
         * @param failureReason reason when in failed state. One of
         *        {@link #SAP_START_FAILURE_GENERAL}, {@link #SAP_START_FAILURE_NO_CHANNEL}
         */
        @Override
        public void onStateChanged(int state, int failureReason) {
            synchronized (mLock) {
                mTetheredSoftApState = state;
            }

            int itemCount = mRegisteredSoftApCallbacks.beginBroadcast();
            for (int i = 0; i < itemCount; i++) {
                try {
                    mRegisteredSoftApCallbacks.getBroadcastItem(i).onStateChanged(state,
                            failureReason);
                } catch (RemoteException e) {
                    Log.e(TAG, "onStateChanged: remote exception -- " + e);
                }
            }
            mRegisteredSoftApCallbacks.finishBroadcast();
        }

        /**
         * Called when the connected clients to soft AP changes.
         *
         * @param clients connected clients to soft AP
         */
        @Override
        public void onConnectedClientsOrInfoChanged(Map<String, SoftApInfo> infos,
                Map<String, List<WifiClient>> clients, boolean isBridged) {
            synchronized (mLock) {
                mIsBridgedMode = isBridged;
                if (infos.size() == 0 && isBridged) {
                    Log.d(TAG, "ShutDown bridged mode, clear isBridged cache in Service");
                    mIsBridgedMode = false;
                }
                mTetheredSoftApConnectedClientsMap =
                        ApConfigUtil.deepCopyForWifiClientListMap(clients);
                mTetheredSoftApInfoMap = ApConfigUtil.deepCopyForSoftApInfoMap(infos);
            }
            int itemCount = mRegisteredSoftApCallbacks.beginBroadcast();
            for (int i = 0; i < itemCount; i++) {
                try {
                    mRegisteredSoftApCallbacks.getBroadcastItem(i).onConnectedClientsOrInfoChanged(
                            ApConfigUtil.deepCopyForSoftApInfoMap(mTetheredSoftApInfoMap),
                            ApConfigUtil.deepCopyForWifiClientListMap(
                                    mTetheredSoftApConnectedClientsMap), isBridged, false);
                } catch (RemoteException e) {
                    Log.e(TAG, "onConnectedClientsOrInfoChanged: remote exception -- " + e);
                }
            }
            mRegisteredSoftApCallbacks.finishBroadcast();
        }

        /**
         * Called when capability of softap changes.
         *
         * @param capability is the softap capability. {@link SoftApCapability}
         */
        @Override
        public void onCapabilityChanged(SoftApCapability capability) {
            synchronized (mLock) {
                if (Objects.equals(capability, mTetheredSoftApCapability)) {
                    return;
                }
                mTetheredSoftApCapability = new SoftApCapability(capability);
            }
            int itemCount = mRegisteredSoftApCallbacks.beginBroadcast();
            for (int i = 0; i < itemCount; i++) {
                try {
                    mRegisteredSoftApCallbacks.getBroadcastItem(i).onCapabilityChanged(
                            mTetheredSoftApCapability);
                } catch (RemoteException e) {
                    Log.e(TAG, "onCapabiliyChanged: remote exception -- " + e);
                }
            }
            mRegisteredSoftApCallbacks.finishBroadcast();
        }

        /**
         * Called when client trying to connect but device blocked the client with specific reason.
         *
         * @param client the currently blocked client.
         * @param blockedReason one of blocked reason from
         * {@link WifiManager.SapClientBlockedReason}
         */
        @Override
        public void onBlockedClientConnecting(WifiClient client, int blockedReason) {
            int itemCount = mRegisteredSoftApCallbacks.beginBroadcast();
            for (int i = 0; i < itemCount; i++) {
                try {
                    mRegisteredSoftApCallbacks.getBroadcastItem(i).onBlockedClientConnecting(client,
                            blockedReason);
                } catch (RemoteException e) {
                    Log.e(TAG, "onBlockedClientConnecting: remote exception -- " + e);
                }
            }
            mRegisteredSoftApCallbacks.finishBroadcast();
        }
    }

    /**
     * Implements LOHS behavior on top of the existing SoftAp API.
     */
    private final class LohsSoftApTracker implements SoftApCallbackInternal {
        @GuardedBy("mLocalOnlyHotspotRequests")
        private final HashMap<Integer, LocalOnlyHotspotRequestInfo>
                mLocalOnlyHotspotRequests = new HashMap<>();

        /** Currently-active config, to be sent to shared clients registering later. */
        @GuardedBy("mLocalOnlyHotspotRequests")
        private SoftApModeConfiguration mActiveConfig = null;

        /**
         * Whether we are currently operating in exclusive mode (i.e. whether a custom config is
         * active).
         */
        @GuardedBy("mLocalOnlyHotspotRequests")
        private boolean mIsExclusive = false;

        @GuardedBy("mLocalOnlyHotspotRequests")
        private String mLohsInterfaceName;

        /**
         * State of local-only hotspot
         * One of:  {@link WifiManager#WIFI_AP_STATE_DISABLED},
         *          {@link WifiManager#WIFI_AP_STATE_DISABLING},
         *          {@link WifiManager#WIFI_AP_STATE_ENABLED},
         *          {@link WifiManager#WIFI_AP_STATE_ENABLING},
         *          {@link WifiManager#WIFI_AP_STATE_FAILED}
         */
        @GuardedBy("mLocalOnlyHotspotRequests")
        private int mLohsState = WIFI_AP_STATE_DISABLED;

        @GuardedBy("mLocalOnlyHotspotRequests")
        private int mLohsInterfaceMode = WifiManager.IFACE_IP_MODE_UNSPECIFIED;

        private SoftApCapability mLohsSoftApCapability = null;

        public SoftApCapability getSoftApCapability() {
            if (mLohsSoftApCapability == null) {
                mLohsSoftApCapability =  ApConfigUtil.updateCapabilityFromResource(mContext);
            }
            return mLohsSoftApCapability;
        }

        public void updateInterfaceIpState(String ifaceName, int mode) {
            // update interface IP state related to local-only hotspot
            synchronized (mLocalOnlyHotspotRequests) {
                Log.d(TAG, "updateInterfaceIpState: ifaceName=" + ifaceName + " mode=" + mode
                        + " previous LOHS mode= " + mLohsInterfaceMode);

                switch (mode) {
                    case WifiManager.IFACE_IP_MODE_LOCAL_ONLY:
                        // first make sure we have registered requests.
                        if (mLocalOnlyHotspotRequests.isEmpty()) {
                            // we don't have requests...  stop the hotspot
                            Log.wtf(TAG, "Starting LOHS without any requests?");
                            stopSoftApInternal(WifiManager.IFACE_IP_MODE_LOCAL_ONLY);
                            return;
                        }
                        // LOHS is ready to go!  Call our registered requestors!
                        mLohsInterfaceName = ifaceName;
                        mLohsInterfaceMode = mode;
                        sendHotspotStartedMessageToAllLOHSRequestInfoEntriesLocked();
                        break;
                    case WifiManager.IFACE_IP_MODE_TETHERED:
                        if (mLohsInterfaceName != null
                                && mLohsInterfaceName.equals(ifaceName)) {
                            /* This shouldn't happen except in a race, but if it does, tear down
                             * the LOHS and let tethering win.
                             *
                             * If concurrent SAPs are allowed, the interface names will differ,
                             * so we don't have to check the config here.
                             */
                            Log.e(TAG, "Unexpected IP mode change on " + ifaceName);
                            mLohsInterfaceName = null;
                            mLohsInterfaceMode = WifiManager.IFACE_IP_MODE_UNSPECIFIED;
                            sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(
                                    LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE);
                        }
                        break;
                    case WifiManager.IFACE_IP_MODE_CONFIGURATION_ERROR:
                        if (ifaceName == null) {
                            // All softAps
                            mLohsInterfaceName = null;
                            mLohsInterfaceMode = mode;
                            sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(
                                    LocalOnlyHotspotCallback.ERROR_GENERIC);
                            stopSoftApInternal(WifiManager.IFACE_IP_MODE_UNSPECIFIED);
                        } else if (ifaceName.equals(mLohsInterfaceName)) {
                            mLohsInterfaceName = null;
                            mLohsInterfaceMode = mode;
                            sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(
                                    LocalOnlyHotspotCallback.ERROR_GENERIC);
                            stopSoftApInternal(WifiManager.IFACE_IP_MODE_LOCAL_ONLY);
                        } else {
                            // Not for LOHS. This is the wrong place to do this, but...
                            stopSoftApInternal(WifiManager.IFACE_IP_MODE_TETHERED);
                        }
                        break;
                    case WifiManager.IFACE_IP_MODE_UNSPECIFIED:
                        if (ifaceName == null || ifaceName.equals(mLohsInterfaceName)) {
                            mLohsInterfaceName = null;
                            mLohsInterfaceMode = mode;
                        }
                        break;
                    default:
                        mLog.warn("updateInterfaceIpState: unknown mode %").c(mode).flush();
                }
            }
        }

        /**
         * Helper method to send a HOTSPOT_FAILED message to all registered LocalOnlyHotspotRequest
         * callers and clear the registrations.
         *
         * Callers should already hold the mLocalOnlyHotspotRequests lock.
         */
        @GuardedBy("mLocalOnlyHotspotRequests")
        private void sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(int reason) {
            for (LocalOnlyHotspotRequestInfo requestor : mLocalOnlyHotspotRequests.values()) {
                try {
                    requestor.sendHotspotFailedMessage(reason);
                    requestor.unlinkDeathRecipient();
                } catch (RemoteException e) {
                    // This will be cleaned up by binder death handling
                }
            }

            // Since all callers were notified, now clear the registrations.
            mLocalOnlyHotspotRequests.clear();
        }

        /**
         * Helper method to send a HOTSPOT_STOPPED message to all registered LocalOnlyHotspotRequest
         * callers and clear the registrations.
         *
         * Callers should already hold the mLocalOnlyHotspotRequests lock.
         */
        @GuardedBy("mLocalOnlyHotspotRequests")
        private void sendHotspotStoppedMessageToAllLOHSRequestInfoEntriesLocked() {
            for (LocalOnlyHotspotRequestInfo requestor : mLocalOnlyHotspotRequests.values()) {
                try {
                    requestor.sendHotspotStoppedMessage();
                    requestor.unlinkDeathRecipient();
                } catch (RemoteException e) {
                    // This will be cleaned up by binder death handling
                }
            }

            // Since all callers were notified, now clear the registrations.
            mLocalOnlyHotspotRequests.clear();
        }

        /**
         * Add a new LOHS client
         */
        private int start(int pid, LocalOnlyHotspotRequestInfo request) {
            synchronized (mLocalOnlyHotspotRequests) {
                // does this caller already have a request?
                if (mLocalOnlyHotspotRequests.get(pid) != null) {
                    mLog.trace("caller already has an active request").flush();
                    throw new IllegalStateException(
                            "Caller already has an active LocalOnlyHotspot request");
                }

                // Never accept exclusive requests (with custom configuration) at the same time as
                // shared requests.
                if (!mLocalOnlyHotspotRequests.isEmpty()) {
                    boolean requestIsExclusive = request.getCustomConfig() != null;
                    if (mIsExclusive || requestIsExclusive) {
                        mLog.trace("Cannot share with existing LOHS request due to custom config")
                                .flush();
                        return LocalOnlyHotspotCallback.ERROR_GENERIC;
                    }
                }

                // At this point, the request is accepted.
                if (mLocalOnlyHotspotRequests.isEmpty()) {
                    mWifiThreadRunner.post(() -> {
                        startForFirstRequestLocked(request);
                    });

                } else if (mLohsInterfaceMode == WifiManager.IFACE_IP_MODE_LOCAL_ONLY) {
                    // LOHS has already started up for an earlier request, so we can send the
                    // current config to the incoming request right away.
                    try {
                        mLog.trace("LOHS already up, trigger onStarted callback").flush();
                        request.sendHotspotStartedMessage(mActiveConfig.getSoftApConfiguration());
                    } catch (RemoteException e) {
                        return LocalOnlyHotspotCallback.ERROR_GENERIC;
                    }
                }

                mLocalOnlyHotspotRequests.put(pid, request);
                return LocalOnlyHotspotCallback.REQUEST_REGISTERED;
            }
        }

        @GuardedBy("mLocalOnlyHotspotRequests")
        private void startForFirstRequestLocked(LocalOnlyHotspotRequestInfo request) {
            int band = WifiApConfigStore.generateDefaultBand(mContext);

            // For auto only
            if (hasAutomotiveFeature(mContext)) {
                if (mContext.getResources().getBoolean(R.bool.config_wifiLocalOnlyHotspot6ghz)
                        && ApConfigUtil.isBandSupported(SoftApConfiguration.BAND_6GHZ, mContext)) {
                    band = SoftApConfiguration.BAND_6GHZ;
                } else if (mContext.getResources().getBoolean(
                        R.bool.config_wifi_local_only_hotspot_5ghz)
                        && ApConfigUtil.isBandSupported(SoftApConfiguration.BAND_5GHZ, mContext)) {
                    band = SoftApConfiguration.BAND_5GHZ;
                }
            }
            SoftApConfiguration softApConfig = mWifiApConfigStore.generateLocalOnlyHotspotConfig(
                    mContext, band, request.getCustomConfig());

            mActiveConfig = new SoftApModeConfiguration(
                    WifiManager.IFACE_IP_MODE_LOCAL_ONLY,
                    softApConfig, mLohsSoftApTracker.getSoftApCapability());
            mIsExclusive = (request.getCustomConfig() != null);

            startSoftApInternal(mActiveConfig, request.getWorkSource());
        }

        /**
         * Requests that any local-only hotspot be stopped.
         */
        public void stopAll() {
            synchronized (mLocalOnlyHotspotRequests) {
                if (!mLocalOnlyHotspotRequests.isEmpty()) {
                    // This is used to take down LOHS when tethering starts, and in that
                    // case we send failed instead of stopped.
                    // TODO check if that is right. Calling onFailed instead of onStopped when the
                    // hotspot is already started does not seem to match the documentation
                    sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(
                            LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE);
                    stopIfEmptyLocked();
                }
            }
        }

        /**
         * Unregisters the LOHS request from the given process and stops LOHS if no other clients.
         */
        public void stopByPid(int pid) {
            synchronized (mLocalOnlyHotspotRequests) {
                LocalOnlyHotspotRequestInfo requestInfo = mLocalOnlyHotspotRequests.remove(pid);
                if (requestInfo == null) return;
                requestInfo.unlinkDeathRecipient();
                stopIfEmptyLocked();
            }
        }

        /**
         * Unregisters LocalOnlyHotspot request and stops the hotspot if needed.
         */
        public void stopByRequest(LocalOnlyHotspotRequestInfo request) {
            synchronized (mLocalOnlyHotspotRequests) {
                if (mLocalOnlyHotspotRequests.remove(request.getPid()) == null) {
                    mLog.trace("LocalOnlyHotspotRequestInfo not found to remove").flush();
                    return;
                }
                stopIfEmptyLocked();
            }
        }

        @GuardedBy("mLocalOnlyHotspotRequests")
        private void stopIfEmptyLocked() {
            if (mLocalOnlyHotspotRequests.isEmpty()) {
                mActiveConfig = null;
                mIsExclusive = false;
                mLohsInterfaceName = null;
                mLohsInterfaceMode = WifiManager.IFACE_IP_MODE_UNSPECIFIED;
                stopSoftApInternal(WifiManager.IFACE_IP_MODE_LOCAL_ONLY);
            }
        }

        /**
         * Helper method to send a HOTSPOT_STARTED message to all registered LocalOnlyHotspotRequest
         * callers.
         *
         * Callers should already hold the mLocalOnlyHotspotRequests lock.
         */
        @GuardedBy("mLocalOnlyHotspotRequests")
        private void sendHotspotStartedMessageToAllLOHSRequestInfoEntriesLocked() {
            for (LocalOnlyHotspotRequestInfo requestor : mLocalOnlyHotspotRequests.values()) {
                try {
                    requestor.sendHotspotStartedMessage(mActiveConfig.getSoftApConfiguration());
                } catch (RemoteException e) {
                    // This will be cleaned up by binder death handling
                }
            }
        }

        @Override
        public void onStateChanged(int state, int failureReason) {
            // The AP state update from ClientModeImpl for softap
            synchronized (mLocalOnlyHotspotRequests) {
                Log.d(TAG, "lohs.onStateChanged: currentState=" + state
                        + " previousState=" + mLohsState + " errorCode= " + failureReason
                        + " ifaceName=" + mLohsInterfaceName);

                // check if we have a failure - since it is possible (worst case scenario where
                // WifiController and ClientModeImpl are out of sync wrt modes) to get two FAILED
                // notifications in a row, we need to handle this first.
                if (state == WIFI_AP_STATE_FAILED) {
                    // update registered LOHS callbacks if we see a failure
                    int errorToReport = ERROR_GENERIC;
                    if (failureReason == SAP_START_FAILURE_NO_CHANNEL) {
                        errorToReport = ERROR_NO_CHANNEL;
                    }
                    // holding the required lock: send message to requestors and clear the list
                    sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(errorToReport);
                    // also need to clear interface ip state
                    updateInterfaceIpState(mLohsInterfaceName,
                            WifiManager.IFACE_IP_MODE_UNSPECIFIED);
                } else if (state == WIFI_AP_STATE_DISABLING || state == WIFI_AP_STATE_DISABLED) {
                    // softap is shutting down or is down...  let requestors know via the
                    // onStopped call
                    // if we are currently in hotspot mode, then trigger onStopped for registered
                    // requestors, otherwise something odd happened and we should clear state
                    if (mLohsInterfaceName != null
                            && mLohsInterfaceMode == WifiManager.IFACE_IP_MODE_LOCAL_ONLY) {
                        // holding the required lock: send message to requestors and clear the list
                        sendHotspotStoppedMessageToAllLOHSRequestInfoEntriesLocked();
                    } else {
                        // LOHS not active: report an error (still holding the required lock)
                        sendHotspotFailedMessageToAllLOHSRequestInfoEntriesLocked(ERROR_GENERIC);
                    }
                    // also clear interface ip state
                    updateInterfaceIpState(mLohsInterfaceName,
                            WifiManager.IFACE_IP_MODE_UNSPECIFIED);
                }
                // For enabling and enabled, just record the new state
                mLohsState = state;
            }
        }
    }

    /**
     * see {@link android.net.wifi.WifiManager#registerSoftApCallback(Executor,
     * WifiManager.SoftApCallback)}
     *
     * @param callback Soft AP callback to register
     *
     * @throws SecurityException if the caller does not have permission to register a callback
     * @throws RemoteException if remote exception happens
     * @throws IllegalArgumentException if the arguments are null or invalid
     */
    @Override
    public void registerSoftApCallback(ISoftApCallback callback) {
        // verify arguments
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }

        int uid = Binder.getCallingUid();
        int pid = Binder.getCallingPid();
        if (!mWifiPermissionsUtil.checkConfigOverridePermission(uid)
                && !checkNetworkSettingsPermission(pid, uid)
                && !checkMainlineNetworkStackPermission(pid, uid)) {
            // random apps should not be allowed to read the user specified config
            throw new SecurityException("App not allowed to read  WiFi Ap information "
                    + "(uid/pid = " + uid + "/" + pid + ")");
        }

        if (isVerboseLoggingEnabled()) {
            mLog.info("registerSoftApCallback uid=%").c(Binder.getCallingUid()).flush();
        }

        // post operation to handler thread
        mWifiThreadRunner.post(() -> {
            if (!mTetheredSoftApTracker.registerSoftApCallback(callback)) {
                Log.e(TAG, "registerSoftApCallback: Failed to add callback");
                return;
            }
            // Update the client about the current state immediately after registering the callback
            try {
                callback.onStateChanged(mTetheredSoftApTracker.getState(), 0);
                callback.onConnectedClientsOrInfoChanged(mTetheredSoftApTracker.getSoftApInfos(),
                        mTetheredSoftApTracker.getConnectedClients(),
                        mTetheredSoftApTracker.getIsBridgedMode(), true);
                callback.onCapabilityChanged(mTetheredSoftApTracker.getSoftApCapability());
            } catch (RemoteException e) {
                Log.e(TAG, "registerSoftApCallback: remote exception -- " + e);
            }
        });
    }

    /**
     * see {@link android.net.wifi.WifiManager#unregisterSoftApCallback(WifiManager.SoftApCallback)}
     *
     * @param callback Soft AP callback to unregister
     *
     * @throws SecurityException if the caller does not have permission to register a callback
     */
    @Override
    public void unregisterSoftApCallback(ISoftApCallback callback) {
        int uid = Binder.getCallingUid();
        int pid = Binder.getCallingPid();
        if (!mWifiPermissionsUtil.checkConfigOverridePermission(uid)
                && !checkNetworkSettingsPermission(pid, uid)
                && !checkMainlineNetworkStackPermission(pid, uid)) {
            // random apps should not be allowed to read the user specified config
            throw new SecurityException("App not allowed to read  WiFi Ap information "
                    + "(uid/pid = " + uid + "/" + pid + ")");
        }

        if (isVerboseLoggingEnabled()) {
            mLog.info("unregisterSoftApCallback uid=%").c(Binder.getCallingUid()).flush();
        }

        // post operation to handler thread
        mWifiThreadRunner.post(() ->
                mTetheredSoftApTracker.unregisterSoftApCallback(callback));
    }

    /**
     * Temporary method used for testing while start is not fully implemented.  This
     * method allows unit tests to register callbacks directly for testing mechanisms triggered by
     * softap mode changes.
     */
    @VisibleForTesting
    void registerLOHSForTest(int pid, LocalOnlyHotspotRequestInfo request) {
        mLohsSoftApTracker.start(pid, request);
    }

    /**
     * Method to start LocalOnlyHotspot.  In this method, permissions, settings and modes are
     * checked to verify that we can enter softapmode.  This method returns
     * {@link LocalOnlyHotspotCallback#REQUEST_REGISTERED} if we will attempt to start, otherwise,
     * possible startup erros may include tethering being disallowed failure reason {@link
     * LocalOnlyHotspotCallback#ERROR_TETHERING_DISALLOWED} or an incompatible mode failure reason
     * {@link LocalOnlyHotspotCallback#ERROR_INCOMPATIBLE_MODE}.
     *
     * see {@link WifiManager#startLocalOnlyHotspot(LocalOnlyHotspotCallback)}
     *
     * @param callback Callback to communicate with WifiManager and allow cleanup if the app dies.
     * @param packageName String name of the calling package.
     * @param featureId The feature in the package
     * @param customConfig Custom configuration to be applied to the hotspot, or null for a shared
     *                     hotspot with framework-generated config.
     *
     * @return int return code for attempt to start LocalOnlyHotspot.
     *
     * @throws SecurityException if the caller does not have permission to start a Local Only
     * Hotspot.
     * @throws IllegalStateException if the caller attempts to start the LocalOnlyHotspot while they
     * have an outstanding request.
     */
    @Override
    public int startLocalOnlyHotspot(ILocalOnlyHotspotCallback callback, String packageName,
            String featureId, SoftApConfiguration customConfig) {
        // first check if the caller has permission to start a local only hotspot
        // need to check for WIFI_STATE_CHANGE and location permission
        final int uid = Binder.getCallingUid();
        final int pid = Binder.getCallingPid();

        mLog.info("start uid=% pid=%").c(uid).c(pid).flush();

        final WorkSource requestorWs;
        // Permission requirements are different with/without custom config.
        if (customConfig == null) {
            if (enforceChangePermission(packageName) != MODE_ALLOWED) {
                return LocalOnlyHotspotCallback.ERROR_GENERIC;
            }
            enforceLocationPermission(packageName, featureId, uid);
            long ident = Binder.clearCallingIdentity();
            try {
                // also need to verify that Locations services are enabled.
                if (!mWifiPermissionsUtil.isLocationModeEnabled()) {
                    throw new SecurityException("Location mode is not enabled.");
                }
                // TODO(b/162344695): Exception added for LOHS. This exception is need to avoid
                // breaking existing LOHS behavior: LOHS AP iface is allowed to delete STA iface
                // (even if LOHS app has lower priority than user toggled on STA iface). This does
                // not fit in with the new context based concurrency priority in HalDeviceManager,
                // but we cannot break existing API's. So, we artificially boost the priority of
                // the request by "faking" the requestor context as settings app.
                // We probably need some UI dialog to allow the user to grant the app's LOHS
                // request. Once that UI dialog is added, we can get rid of this hack and use the UI
                // to elevate the priority of LOHS request only if user approves the request to
                // toggle wifi off for LOHS.
                requestorWs = mFrameworkFacade.getSettingsWorkSource(mContext);
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        } else {
            if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
                throw new SecurityException(TAG + ": Permission denied");
            }
            // Already privileged, no need to fake.
            requestorWs = new WorkSource(uid, packageName);
        }

        // verify that tethering is not disabled
        if (mUserManager.hasUserRestrictionForUser(
                UserManager.DISALLOW_CONFIG_TETHERING, UserHandle.getUserHandleForUid(uid))) {
            return LocalOnlyHotspotCallback.ERROR_TETHERING_DISALLOWED;
        }

        // the app should be in the foreground
        long ident = Binder.clearCallingIdentity();
        try {
            // also need to verify that Locations services are enabled.
            if (!mFrameworkFacade.isAppForeground(mContext, uid)) {
                return LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE;
            }
        } finally {
            Binder.restoreCallingIdentity(ident);
        }

        // check if we are currently tethering
        if (!mActiveModeWarden.canRequestMoreSoftApManagers(requestorWs)
                && mTetheredSoftApTracker.getState() == WIFI_AP_STATE_ENABLED) {
            // Tethering is enabled, cannot start LocalOnlyHotspot
            mLog.info("Cannot start localOnlyHotspot when WiFi Tethering is active.")
                    .flush();
            return LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE;
        }

        // now create the new LOHS request info object
        LocalOnlyHotspotRequestInfo request = new LocalOnlyHotspotRequestInfo(
                requestorWs, callback, new LocalOnlyRequestorCallback(), customConfig);

        return mLohsSoftApTracker.start(pid, request);
    }

    /**
     * see {@link WifiManager#stopLocalOnlyHotspot()}
     *
     * @throws SecurityException if the caller does not have permission to stop a Local Only
     * Hotspot.
     */
    @Override
    public void stopLocalOnlyHotspot() {
        // don't do a permission check here. if the app's permission to change the wifi state is
        // revoked, we still want them to be able to stop a previously created hotspot (otherwise
        // it could cost the user money). When the app created the hotspot, its permission was
        // checked.
        final int uid = Binder.getCallingUid();
        final int pid = Binder.getCallingPid();

        mLog.info("stopLocalOnlyHotspot uid=% pid=%").c(uid).c(pid).flush();

        mLohsSoftApTracker.stopByPid(pid);
    }

    /**
     * see {@link WifiManager#watchLocalOnlyHotspot(LocalOnlyHotspotObserver)}
     *
     * This call requires the android.permission.NETWORK_SETTINGS permission.
     *
     * @param callback Callback to communicate with WifiManager and allow cleanup if the app dies.
     *
     * @throws SecurityException if the caller does not have permission to watch Local Only Hotspot
     * status updates.
     * @throws IllegalStateException if the caller attempts to watch LocalOnlyHotspot updates with
     * an existing subscription.
     */
    @Override
    public void startWatchLocalOnlyHotspot(ILocalOnlyHotspotCallback callback) {
        // NETWORK_SETTINGS is a signature only permission.
        enforceNetworkSettingsPermission();

        throw new UnsupportedOperationException("LocalOnlyHotspot is still in development");
    }

    /**
     * see {@link WifiManager#unregisterLocalOnlyHotspotObserver()}
     */
    @Override
    public void stopWatchLocalOnlyHotspot() {
        // NETWORK_STACK is a signature only permission.
        enforceNetworkSettingsPermission();
        throw new UnsupportedOperationException("LocalOnlyHotspot is still in development");
    }

    /**
     * see {@link WifiManager#getWifiApConfiguration()}
     * @return soft access point configuration
     * @throws SecurityException if the caller does not have permission to retrieve the softap
     * config
     */
    @Nullable
    @Override
    public WifiConfiguration getWifiApConfiguration() {
        enforceAccessPermission();
        int uid = Binder.getCallingUid();
        // only allow Settings UI to get the saved SoftApConfig
        if (!mWifiPermissionsUtil.checkConfigOverridePermission(uid)) {
            // random apps should not be allowed to read the user specified config
            throw new SecurityException("App not allowed to read or update stored WiFi Ap config "
                    + "(uid = " + uid + ")");
        }

        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiApConfiguration uid=%").c(uid).flush();
        }

        // hand off work to the ClientModeImpl handler thread to sync work between calls
        // and SoftApManager starting up softap
        return (mWifiThreadRunner.call(mWifiApConfigStore::getApConfiguration,
                new SoftApConfiguration.Builder().build())).toWifiConfiguration();
    }

    /**
     * see {@link WifiManager#getSoftApConfiguration()}
     * @return soft access point configuration {@link SoftApConfiguration}
     * @throws SecurityException if the caller does not have permission to retrieve the softap
     * config
     */
    @NonNull
    @Override
    public SoftApConfiguration getSoftApConfiguration() {
        int uid = Binder.getCallingUid();
        if (!mWifiPermissionsUtil.checkConfigOverridePermission(uid)
                && !mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
            // random apps should not be allowed to read the user specified config
            throw new SecurityException("App not allowed to read or update stored WiFi Ap config "
                    + "(uid = " + uid + ")");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getSoftApConfiguration uid=%").c(uid).flush();
        }

        // hand off work to the ClientModeImpl handler thread to sync work between calls
        // and SoftApManager starting up softap
        return mWifiThreadRunner.call(mWifiApConfigStore::getApConfiguration,
                new SoftApConfiguration.Builder().build());
    }

    /**
     * see {@link WifiManager#setWifiApConfiguration(WifiConfiguration)}
     * @param wifiConfig WifiConfiguration details for soft access point
     * @return boolean indicating success or failure of the operation
     * @throws SecurityException if the caller does not have permission to write the softap config
     */
    @Override
    public boolean setWifiApConfiguration(WifiConfiguration wifiConfig, String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        int uid = Binder.getCallingUid();
        // only allow Settings UI to write the stored SoftApConfig
        if (!mWifiPermissionsUtil.checkConfigOverridePermission(uid)) {
            // random apps should not be allowed to read the user specified config
            throw new SecurityException("App not allowed to read or update stored WiFi AP config "
                    + "(uid = " + uid + ")");
        }
        mLog.info("setWifiApConfiguration uid=%").c(uid).flush();
        if (wifiConfig == null)
            return false;
        SoftApConfiguration softApConfig = ApConfigUtil.fromWifiConfiguration(wifiConfig);
        if (softApConfig == null) return false;
        if (WifiApConfigStore.validateApWifiConfiguration(
                softApConfig, false, mContext)) {
            mWifiThreadRunner.post(() -> mWifiApConfigStore.setApConfiguration(softApConfig));
            return true;
        } else {
            Log.e(TAG, "Invalid WifiConfiguration");
            return false;
        }
    }

    /**
     * see {@link WifiManager#setSoftApConfiguration(SoftApConfiguration)}
     * @param softApConfig {@link SoftApConfiguration} details for soft access point
     * @return boolean indicating success or failure of the operation
     * @throws SecurityException if the caller does not have permission to write the softap config
     */
    @Override
    public boolean setSoftApConfiguration(
            @NonNull SoftApConfiguration softApConfig, @NonNull String packageName) {
        int uid = Binder.getCallingUid();
        boolean privileged = mWifiPermissionsUtil.checkNetworkSettingsPermission(uid);
        if (!mWifiPermissionsUtil.checkConfigOverridePermission(uid)
                && !privileged) {
            // random apps should not be allowed to read the user specified config
            throw new SecurityException("App not allowed to read or update stored WiFi Ap config "
                    + "(uid = " + uid + ")");
        }
        mLog.info("setSoftApConfiguration uid=%").c(uid).flush();
        if (softApConfig == null) return false;
        if (WifiApConfigStore.validateApWifiConfiguration(softApConfig, privileged, mContext)) {
            mActiveModeWarden.updateSoftApConfiguration(softApConfig);
            mWifiThreadRunner.post(() -> mWifiApConfigStore.setApConfiguration(softApConfig));
            return true;
        } else {
            Log.e(TAG, "Invalid SoftAp Configuration");
            return false;
        }
    }

    /**
     * see {@link android.net.wifi.WifiManager#setScanAlwaysAvailable(boolean)}
     */
    @Override
    public void setScanAlwaysAvailable(boolean isAvailable, String packageName) {
        enforceNetworkSettingsPermission();
        mLog.info("setScanAlwaysAvailable uid=% package=% isAvailable=%")
                .c(Binder.getCallingUid())
                .c(packageName)
                .c(isAvailable)
                .flush();
        mSettingsStore.handleWifiScanAlwaysAvailableToggled(isAvailable);
        long ident = Binder.clearCallingIdentity();
        try {
            mWifiInjector.getWifiScanAlwaysAvailableSettingsCompatibility()
                    .handleWifiScanAlwaysAvailableToggled(isAvailable);
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
        mActiveModeWarden.scanAlwaysModeChanged();
    }

    /**
     * see {@link android.net.wifi.WifiManager#isScanAlwaysAvailable()}
     */
    @Override
    public boolean isScanAlwaysAvailable() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("isScanAlwaysAvailable uid=%").c(Binder.getCallingUid()).flush();
        }
        return mSettingsStore.isScanAlwaysAvailableToggleEnabled();
    }

    /**
     * see {@link android.net.wifi.WifiManager#disconnect()}
     */
    @Override
    public boolean disconnect(String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        if (!isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), Binder.getCallingUid())) {
            mLog.info("disconnect not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        mLog.info("disconnect uid=%").c(Binder.getCallingUid()).flush();
        mWifiThreadRunner.post(() -> mActiveModeWarden.getPrimaryClientModeManager().disconnect());
        return true;
    }

    /**
     * see {@link android.net.wifi.WifiManager#reconnect()}
     */
    @Override
    public boolean reconnect(String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        int callingUid = Binder.getCallingUid();
        if (!isTargetSdkLessThanQOrPrivileged(packageName, Binder.getCallingPid(), callingUid)) {
            mLog.info("reconnect not allowed for uid=%").c(callingUid).flush();
            return false;
        }
        mLog.info("reconnect uid=%").c(callingUid).flush();

        mWifiThreadRunner.post(() -> {
            mActiveModeWarden.getPrimaryClientModeManager().reconnect(new WorkSource(callingUid));
        });
        return true;
    }

    /**
     * see {@link android.net.wifi.WifiManager#reassociate()}
     */
    @Override
    public boolean reassociate(String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        if (!isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), Binder.getCallingUid())) {
            mLog.info("reassociate not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        mLog.info("reassociate uid=%").c(Binder.getCallingUid()).flush();
        mWifiThreadRunner.post(() -> mActiveModeWarden.getPrimaryClientModeManager().reassociate());
        return true;
    }

    /**
     * Returns true if we should log the call to getSupportedFeatures.
     *
     * Because of the way getSupportedFeatures is used in WifiManager, there are
     * often clusters of several back-to-back calls; avoid repeated logging if
     * the feature set has not changed and the time interval is short.
     */
    private boolean needToLogSupportedFeatures(long features) {
        if (isVerboseLoggingEnabled()) {
            long now = mClock.getElapsedSinceBootMillis();
            synchronized (this) {
                if (now > mLastLoggedSupportedFeaturesTimestamp + A_FEW_MILLISECONDS
                        || features != mLastLoggedSupportedFeatures) {
                    mLastLoggedSupportedFeaturesTimestamp = now;
                    mLastLoggedSupportedFeatures = features;
                    return true;
                }
            }
        }
        return false;
    }
    private static final int A_FEW_MILLISECONDS = 250;
    private long mLastLoggedSupportedFeatures = -1;
    private long mLastLoggedSupportedFeaturesTimestamp = 0;

    /**
     * see {@link android.net.wifi.WifiManager#getSupportedFeatures}
     */
    @Override
    public long getSupportedFeatures() {
        enforceAccessPermission();
        long features = getSupportedFeaturesInternal();
        if (needToLogSupportedFeatures(features)) {
            mLog.info("getSupportedFeatures uid=% returns %")
                    .c(Binder.getCallingUid())
                    .c(Long.toHexString(features))
                    .flush();
        }
        return features;
    }

    @Override
    public void getWifiActivityEnergyInfoAsync(IOnWifiActivityEnergyInfoListener listener) {
        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiActivityEnergyInfoAsync uid=%")
                    .c(Binder.getCallingUid())
                    .flush();
        }
        // getWifiActivityEnergyInfo() performs permission checking
        WifiActivityEnergyInfo info = getWifiActivityEnergyInfo();
        try {
            listener.onWifiActivityEnergyInfo(info);
        } catch (RemoteException e) {
            Log.e(TAG, "onWifiActivityEnergyInfo: RemoteException -- ", e);
        }
    }

    private WifiActivityEnergyInfo getWifiActivityEnergyInfo() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiActivityEnergyInfo uid=%").c(Binder.getCallingUid()).flush();
        }
        if ((getSupportedFeatures() & WifiManager.WIFI_FEATURE_LINK_LAYER_STATS) == 0) {
            return null;
        }
        WifiLinkLayerStats stats = mWifiThreadRunner.call(
                () -> mActiveModeWarden.getPrimaryClientModeManager().getWifiLinkLayerStats(),
                null);
        if (stats == null) {
            return null;
        }

        final long rxIdleTimeMillis = stats.on_time - stats.tx_time - stats.rx_time;
        if (VDBG || rxIdleTimeMillis < 0 || stats.on_time < 0 || stats.tx_time < 0
                || stats.rx_time < 0 || stats.on_time_scan < 0) {
            Log.d(TAG, " getWifiActivityEnergyInfo: "
                    + " on_time_millis=" + stats.on_time
                    + " tx_time_millis=" + stats.tx_time
                    + " rx_time_millis=" + stats.rx_time
                    + " rxIdleTimeMillis=" + rxIdleTimeMillis
                    + " scan_time_millis=" + stats.on_time_scan);
        }

        // Convert the LinkLayerStats into WifiActivityEnergyInfo
        return new WifiActivityEnergyInfo(
                mClock.getElapsedSinceBootMillis(),
                WifiActivityEnergyInfo.STACK_STATE_STATE_IDLE,
                stats.tx_time,
                stats.rx_time,
                stats.on_time_scan,
                rxIdleTimeMillis);
    }

    /**
     * see {@link android.net.wifi.WifiManager#getConfiguredNetworks()}
     *
     * @param packageName String name of the calling package
     * @param featureId The feature in the package
     * @param callerNetworksOnly Whether to only return networks created by the caller
     * @return the list of configured networks
     */
    @Override
    public ParceledListSlice<WifiConfiguration> getConfiguredNetworks(String packageName,
            String featureId, boolean callerNetworksOnly) {
        enforceAccessPermission();
        int callingUid = Binder.getCallingUid();
        boolean isDeviceOrProfileOwner = isDeviceOrProfileOwner(callingUid, packageName);
        boolean isCarrierApp = mWifiInjector.makeTelephonyManager()
                .checkCarrierPrivilegesForPackageAnyPhone(packageName)
                == TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS;
        boolean isPrivileged = isPrivileged(getCallingPid(), callingUid);
        // Only DO, PO, carrier app or system app can use callerNetworksOnly argument
        if (callerNetworksOnly) {
            if (!isDeviceOrProfileOwner && !isCarrierApp && !isPrivileged) {
                throw new SecurityException(
                        "Not a DO, PO, carrier or privileged app");
            }
        }
        // bypass shell: can get various pkg name
        // also bypass if caller is only retrieving networks added by itself
        if (callingUid != Process.SHELL_UID && callingUid != Process.ROOT_UID
                && !callerNetworksOnly) {
            long ident = Binder.clearCallingIdentity();
            try {
                mWifiPermissionsUtil.enforceCanAccessScanResults(packageName, featureId,
                        callingUid, null);
            } catch (SecurityException e) {
                Log.w(TAG, "Permission violation - getConfiguredNetworks not allowed for uid="
                        + callingUid + ", packageName=" + packageName + ", reason=" + e);
                return new ParceledListSlice<>(new ArrayList<>());
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        }
        boolean isTargetSdkLessThanQOrPrivileged = isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), callingUid);
        if (!isTargetSdkLessThanQOrPrivileged && !isCarrierApp) {
            mLog.info("getConfiguredNetworks not allowed for uid=%")
                    .c(callingUid).flush();
            return new ParceledListSlice<>(new ArrayList<>());
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getConfiguredNetworks uid=%").c(callingUid).flush();
        }

        int targetConfigUid = Process.INVALID_UID; // don't expose any MAC addresses
        if (isPrivileged || isDeviceOrProfileOwner) {
            targetConfigUid = Process.WIFI_UID; // expose all MAC addresses
        } else if (isCarrierApp) {
            targetConfigUid = callingUid; // expose only those configs created by the Carrier App
        }
        int finalTargetConfigUid = targetConfigUid;
        List<WifiConfiguration> configs = mWifiThreadRunner.call(
                () -> mWifiConfigManager.getSavedNetworks(finalTargetConfigUid),
                Collections.emptyList());
        if (isTargetSdkLessThanQOrPrivileged && !callerNetworksOnly) {
            return new ParceledListSlice<>(
                    WifiConfigurationUtil.convertMultiTypeConfigsToLegacyConfigs(configs));
        }
        // Should only get its own configs
        List<WifiConfiguration> creatorConfigs = new ArrayList<>();
        for (WifiConfiguration config : configs) {
            if (config.creatorUid == callingUid) {
                creatorConfigs.add(config);
            }
        }
        return new ParceledListSlice<>(
                WifiConfigurationUtil.convertMultiTypeConfigsToLegacyConfigs(creatorConfigs));
    }

    /**
     * see {@link android.net.wifi.WifiManager#getPrivilegedConfiguredNetworks()}
     *
     * @param packageName String name of the calling package
     * @param featureId The feature in the package
     * @return the list of configured networks with real preSharedKey
     */
    @Override
    public ParceledListSlice<WifiConfiguration> getPrivilegedConfiguredNetworks(
            String packageName, String featureId) {
        enforceReadCredentialPermission();
        enforceAccessPermission();
        int callingUid = Binder.getCallingUid();
        long ident = Binder.clearCallingIdentity();
        try {
            mWifiPermissionsUtil.enforceCanAccessScanResults(packageName, featureId, callingUid,
                    null);
        } catch (SecurityException e) {
            Log.w(TAG, "Permission violation - getPrivilegedConfiguredNetworks not allowed for"
                    + " uid=" + callingUid + ", packageName=" + packageName + ", reason=" + e);
            return null;
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getPrivilegedConfiguredNetworks uid=%").c(callingUid).flush();
        }
        List<WifiConfiguration> configs = mWifiThreadRunner.call(
                () -> mWifiConfigManager.getConfiguredNetworksWithPasswords(),
                Collections.emptyList());
        return new ParceledListSlice<>(
                WifiConfigurationUtil.convertMultiTypeConfigsToLegacyConfigs(configs));
    }

    /**
     * Return a map of all matching configurations keys with corresponding scanResults (or an empty
     * map if none).
     *
     * @param scanResults The list of scan results
     * @return Map that consists of FQDN (Fully Qualified Domain Name) and corresponding
     * scanResults per network type({@link WifiManager#PASSPOINT_HOME_NETWORK} and {@link
     * WifiManager#PASSPOINT_ROAMING_NETWORK}).
     */
    @Override
    public Map<String, Map<Integer, List<ScanResult>>>
            getAllMatchingPasspointProfilesForScanResults(List<ScanResult> scanResults) {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getMatchingPasspointConfigurations uid=%").c(Binder.getCallingUid()).flush();
        }
        if (!ScanResultUtil.validateScanResultList(scanResults)) {
            Log.e(TAG, "Attempt to retrieve passpoint with invalid scanResult List");
            return Collections.emptyMap();
        }
        return mWifiThreadRunner.call(
            () -> mPasspointManager.getAllMatchingPasspointProfilesForScanResults(scanResults),
                Collections.emptyMap());
    }

    /**
     * Returns list of OSU (Online Sign-Up) providers associated with the given list of ScanResult.
     *
     * @param scanResults a list of ScanResult that has Passpoint APs.
     * @return Map that consists of {@link OsuProvider} and a matching list of {@link ScanResult}.
     */
    @Override
    public Map<OsuProvider, List<ScanResult>> getMatchingOsuProviders(
            List<ScanResult> scanResults) {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getMatchingOsuProviders uid=%").c(Binder.getCallingUid()).flush();
        }

        if (!ScanResultUtil.validateScanResultList(scanResults)) {
            Log.w(TAG, "Attempt to retrieve OsuProviders with invalid scanResult List");
            return Collections.emptyMap();
        }
        return mWifiThreadRunner.call(
            () -> mPasspointManager.getMatchingOsuProviders(scanResults), Collections.emptyMap());
    }

    /**
     * Returns the matching Passpoint configurations for given OSU(Online Sign-Up) providers.
     *
     * @param osuProviders a list of {@link OsuProvider}
     * @return Map that consists of {@link OsuProvider} and matching {@link PasspointConfiguration}.
     */
    @Override
    public Map<OsuProvider, PasspointConfiguration> getMatchingPasspointConfigsForOsuProviders(
            List<OsuProvider> osuProviders) {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getMatchingPasspointConfigsForOsuProviders uid=%").c(
                    Binder.getCallingUid()).flush();
        }
        if (osuProviders == null) {
            Log.e(TAG, "Attempt to retrieve Passpoint configuration with null osuProviders");
            return new HashMap<>();
        }
        return mWifiThreadRunner.call(
            () -> mPasspointManager.getMatchingPasspointConfigsForOsuProviders(osuProviders),
                Collections.emptyMap());
    }

    /**
     * Returns the corresponding wifi configurations for given FQDN (Fully Qualified Domain Name)
     * list.
     *
     * An empty list will be returned when no match is found.
     *
     * @param fqdnList a list of FQDN
     * @return List of {@link WifiConfiguration} converted from {@link PasspointProvider}
     */
    @Override
    public List<WifiConfiguration> getWifiConfigsForPasspointProfiles(List<String> fqdnList) {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiConfigsForPasspointProfiles uid=%").c(
                    Binder.getCallingUid()).flush();
        }
        if (fqdnList == null) {
            Log.e(TAG, "Attempt to retrieve WifiConfiguration with null fqdn List");
            return new ArrayList<>();
        }
        return mWifiThreadRunner.call(
            () -> mPasspointManager.getWifiConfigsForPasspointProfiles(fqdnList),
                Collections.emptyList());
    }

    /**
     * Returns a list of Wifi configurations for matched available WifiNetworkSuggestion
     * corresponding to the given scan results.
     *
     * An empty list will be returned when no match is found or all matched suggestions is not
     * available(not allow user manually connect, user not approved or open network).
     *
     * @param scanResults a list of {@link ScanResult}.
     * @return a list of {@link WifiConfiguration} from matched {@link WifiNetworkSuggestion}.
     */
    @Override
    public List<WifiConfiguration> getWifiConfigForMatchedNetworkSuggestionsSharedWithUser(
            List<ScanResult> scanResults) {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getWifiConfigsForMatchedNetworkSuggestions uid=%").c(
                    Binder.getCallingUid()).flush();
        }
        if (!ScanResultUtil.validateScanResultList(scanResults)) {
            Log.w(TAG, "Attempt to retrieve WifiConfiguration with invalid scanResult List");
            return new ArrayList<>();
        }
        return mWifiThreadRunner.call(
                () -> mWifiNetworkSuggestionsManager
                        .getWifiConfigForMatchedNetworkSuggestionsSharedWithUser(scanResults),
                Collections.emptyList());
    }

    /**
     * see {@link WifiManager#addNetworkPrivileged(WifiConfiguration)}
     * @return WifiManager.AddNetworkResult Object.
     */
    @Override
    public @NonNull WifiManager.AddNetworkResult addOrUpdateNetworkPrivileged(
            WifiConfiguration config, String packageName) {
        int pid = Binder.getCallingPid();
        int uid = Binder.getCallingUid();
        mWifiPermissionsUtil.checkPackage(uid, packageName);
        boolean hasPermission = isPrivileged(pid, uid)
                || isDeviceOrProfileOwner(uid, packageName)
                || mWifiPermissionsUtil.isSystem(packageName, uid);
        if (!hasPermission) {
            throw new SecurityException("Caller is not a device owner, profile owner, system app,"
                    + " or privileged app");
        }
        if (config != null) {
            config.networkId = removeSecurityTypeFromNetworkId(config.networkId);
        }
        return addOrUpdateNetworkInternal(config, packageName, uid);
    }

    /**
     * see {@link android.net.wifi.WifiManager#addOrUpdateNetwork(WifiConfiguration)}
     * @return the supplicant-assigned identifier for the new or updated
     * network if the operation succeeds, or {@code -1} if it fails
     */
    @Override
    public int addOrUpdateNetwork(WifiConfiguration config, String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return -1;
        }
        int callingUid = Binder.getCallingUid();
        if (!isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), callingUid)) {
            mLog.info("addOrUpdateNetwork not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return -1;
        }
        if (config != null) {
            config.networkId = removeSecurityTypeFromNetworkId(config.networkId);
        }
        mLog.info("addOrUpdateNetwork uid=%").c(Binder.getCallingUid()).flush();
        return addOrUpdateNetworkInternal(config, packageName, callingUid).networkId;
    }

    private @NonNull AddNetworkResult addOrUpdateNetworkInternal(WifiConfiguration config,
            String packageName, int callingUid) {
        if (config == null) {
            Log.e(TAG, "bad network configuration");
            return new AddNetworkResult(
                    AddNetworkResult.STATUS_INVALID_CONFIGURATION, -1);
        }
        mWifiMetrics.incrementNumAddOrUpdateNetworkCalls();

        // Previously, this API is overloaded for installing Passpoint profiles.  Now
        // that we have a dedicated API for doing it, redirect the call to the dedicated API.
        if (config.isPasspoint()) {
            PasspointConfiguration passpointConfig =
                    PasspointProvider.convertFromWifiConfig(config);
            if (passpointConfig == null || passpointConfig.getCredential() == null) {
                Log.e(TAG, "Missing credential for Passpoint profile");
                return new AddNetworkResult(
                        AddNetworkResult.STATUS_ADD_PASSPOINT_FAILURE, -1);
            }

            // Copy over certificates and keys.
            X509Certificate[] x509Certificates = null;
            if (config.enterpriseConfig.getCaCertificate() != null) {
                x509Certificates =
                        new X509Certificate[]{config.enterpriseConfig.getCaCertificate()};
            }
            passpointConfig.getCredential().setCaCertificates(x509Certificates);
            passpointConfig.getCredential().setClientCertificateChain(
                    config.enterpriseConfig.getClientCertificateChain());
            passpointConfig.getCredential().setClientPrivateKey(
                    config.enterpriseConfig.getClientPrivateKey());
            if (!addOrUpdatePasspointConfiguration(passpointConfig, packageName)) {
                Log.e(TAG, "Failed to add Passpoint profile");
                return new AddNetworkResult(
                        AddNetworkResult.STATUS_ADD_PASSPOINT_FAILURE, -1);
            }
            // There is no network ID associated with a Passpoint profile.
            return new AddNetworkResult(AddNetworkResult.STATUS_SUCCESS, 0);
        }

        if (config.isEnterprise() && config.enterpriseConfig.isEapMethodServerCertUsed()
                && !config.enterpriseConfig.isMandatoryParameterSetForServerCertValidation()) {
            if (!(mContext.getResources().getBoolean(
                    R.bool.config_wifiAllowInsecureEnterpriseConfigurationsForSettingsAndSUW)
                    && isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid()))) {
                Log.e(TAG, "Enterprise network configuration is missing either a Root CA "
                        + "or a domain name");
                return new AddNetworkResult(
                        AddNetworkResult.STATUS_INVALID_CONFIGURATION_ENTERPRISE, -1);
            }
            Log.w(TAG, "Insecure Enterprise network " + config.SSID
                    + " configured by Settings/SUW");
        }

        Log.i("addOrUpdateNetworkInternal", " uid = " + Binder.getCallingUid()
                + " SSID " + config.SSID
                + " nid=" + config.networkId);
        // TODO: b/171981339, add more detailed failure reason into
        //  WifiConfigManager.NetworkUpdateResult, and plumb that reason up.
        int networkId =  mWifiThreadRunner.call(
                () -> mWifiConfigManager.addOrUpdateNetwork(config, callingUid, packageName)
                        .getNetworkId(),
                WifiConfiguration.INVALID_NETWORK_ID);
        if (networkId >= 0) {
            return new AddNetworkResult(AddNetworkResult.STATUS_SUCCESS, addSecurityTypeToNetworkId(
                    networkId, config.getDefaultSecurityParams().getSecurityType()));
        }
        return new AddNetworkResult(
                AddNetworkResult.STATUS_ADD_WIFI_CONFIG_FAILURE, -1);
    }

    public static void verifyCert(X509Certificate caCert)
            throws GeneralSecurityException, IOException {
        CertificateFactory factory = CertificateFactory.getInstance("X.509");
        CertPathValidator validator =
                CertPathValidator.getInstance(CertPathValidator.getDefaultType());
        CertPath path = factory.generateCertPath(
                Arrays.asList(caCert));
        KeyStore ks = KeyStore.getInstance("AndroidCAStore");
        ks.load(null, null);
        PKIXParameters params = new PKIXParameters(ks);
        params.setRevocationEnabled(false);
        validator.validate(path, params);
    }

    /**
     * See {@link android.net.wifi.WifiManager#removeNetwork(int)}
     * @param netId the integer that identifies the network configuration
     * to the supplicant
     * @return {@code true} if the operation succeeded
     */
    @Override
    public boolean removeNetwork(int netId, String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        if (!isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), Binder.getCallingUid())) {
            mLog.info("removeNetwork not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        final int internalNetId = removeSecurityTypeFromNetworkId(netId);
        int callingUid = Binder.getCallingUid();
        mLog.info("removeNetwork uid=%").c(callingUid).flush();
        return mWifiThreadRunner.call(
                () -> mWifiConfigManager.removeNetwork(internalNetId, callingUid, packageName),
                false);
    }

    @Override
    public boolean removeNonCallerConfiguredNetworks(String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            throw new SecurityException("Caller does not hold CHANGE_WIFI_STATE permission");
        }
        final int callingUid = Binder.getCallingUid();
        if (!mWifiPermissionsUtil.isDeviceOwner(callingUid, packageName)) {
            throw new SecurityException("Caller is not device owner");
        }
        return mWifiThreadRunner.call(
                () -> mWifiConfigManager.removeNonCallerConfiguredNetwork(callingUid), false);
    }

    /**
     * Trigger a connect request and wait for the callback to return status.
     * This preserves the legacy connect API behavior, i.e. {@link WifiManager#enableNetwork(
     * int, true)}
     * @return
     */
    private boolean triggerConnectAndReturnStatus(int netId, int callingUid) {
        final CountDownLatch countDownLatch = new CountDownLatch(1);
        final Mutable<Boolean> success = new Mutable<>(false);
        IActionListener.Stub connectListener = new IActionListener.Stub() {
            @Override
            public void onSuccess() {
                success.value = true;
                countDownLatch.countDown();
            }
            @Override
            public void onFailure(int reason) {
                success.value = false;
                countDownLatch.countDown();
            }
        };
        mWifiThreadRunner.post(() ->
                mMakeBeforeBreakManager.stopAllSecondaryTransientClientModeManagers(() ->
                        mConnectHelper.connectToNetwork(
                                new NetworkUpdateResult(netId),
                                new ActionListenerWrapper(connectListener),
                                callingUid)
                )
        );
        // now wait for response.
        try {
            countDownLatch.await(RUN_WITH_SCISSORS_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Failed to retrieve connect status");
        }
        return success.value;
    }

    /**
     * See {@link android.net.wifi.WifiManager#enableNetwork(int, boolean)}
     * @param netId the integer that identifies the network configuration
     * to the supplicant
     * @param disableOthers if true, disable all other networks.
     * @return {@code true} if the operation succeeded
     */
    @Override
    public boolean enableNetwork(int netId, boolean disableOthers, String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        if (!isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), Binder.getCallingUid())) {
            mLog.info("enableNetwork not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        final int internalNetId = removeSecurityTypeFromNetworkId(netId);
        int callingUid = Binder.getCallingUid();
        // TODO b/33807876 Log netId
        mLog.info("enableNetwork uid=% disableOthers=%")
                .c(callingUid)
                .c(disableOthers).flush();

        mWifiMetrics.incrementNumEnableNetworkCalls();
        if (disableOthers) {
            return triggerConnectAndReturnStatus(internalNetId, callingUid);
        } else {
            return mWifiThreadRunner.call(
                    () -> mWifiConfigManager.enableNetwork(
                            internalNetId, false, callingUid, packageName),
                    false);
        }
    }

    /**
     * See {@link android.net.wifi.WifiManager#disableNetwork(int)}
     * @param netId the integer that identifies the network configuration
     * to the supplicant
     * @return {@code true} if the operation succeeded
     */
    @Override
    public boolean disableNetwork(int netId, String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        if (!isTargetSdkLessThanQOrPrivileged(
                packageName, Binder.getCallingPid(), Binder.getCallingUid())) {
            mLog.info("disableNetwork not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        final int internalNetId = removeSecurityTypeFromNetworkId(netId);
        int callingUid = Binder.getCallingUid();
        mLog.info("disableNetwork uid=%").c(callingUid).flush();
        return mWifiThreadRunner.call(
                () -> mWifiConfigManager.disableNetwork(
                        internalNetId, callingUid, packageName), false);
    }

    /**
     * See
     * {@link android.net.wifi.WifiManager#startRestrictingAutoJoinToSubscriptionId(int)}
     * @param subscriptionId the subscription ID of the carrier whose merged wifi networks won't be
     *                       disabled.
     */
    @Override
    @RequiresApi(Build.VERSION_CODES.S)
    public void startRestrictingAutoJoinToSubscriptionId(int subscriptionId) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }

        mLog.info("startRestrictingAutoJoinToSubscriptionId=% uid=%").c(subscriptionId)
                .c(Binder.getCallingUid()).flush();
        mWifiThreadRunner.post(() -> {
            mWifiConfigManager
                    .startRestrictingAutoJoinToSubscriptionId(subscriptionId);
            // always disconnect here and rely on auto-join to find the appropriate carrier network
            // to join. Even if we are currently connected to the carrier-merged wifi, it's still
            // better to disconnect here because it's possible that carrier wifi offload is
            // disabled.
            mActiveModeWarden.getPrimaryClientModeManager().disconnect();
        });
    }

    /**
     * See {@link android.net.wifi.WifiManager#stopRestrictingAutoJoinToSubscriptionId()}
     */
    @Override
    @RequiresApi(Build.VERSION_CODES.S)
    public void stopRestrictingAutoJoinToSubscriptionId() {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }

        mLog.info("stopRestrictingAutoJoinToSubscriptionId uid=%")
                .c(Binder.getCallingUid()).flush();
        mWifiThreadRunner.post(() ->
                mWifiConfigManager.stopRestrictingAutoJoinToSubscriptionId());
    }

    /**
     * See {@link android.net.wifi.WifiManager#allowAutojoinGlobal(boolean)}
     * @param choice the OEM's choice to allow auto-join
     */
    @Override
    public void allowAutojoinGlobal(boolean choice) {
        enforceNetworkSettingsPermission();

        int callingUid = Binder.getCallingUid();
        mLog.info("allowAutojoinGlobal=% uid=%").c(choice).c(callingUid).flush();
        mWifiThreadRunner.post(() -> mWifiConnectivityManager.setAutoJoinEnabledExternal(choice));
        mLastCallerInfoManager.put(LastCallerInfoManager.AUTOJOIN_GLOBAL, Process.myTid(),
                callingUid, Binder.getCallingPid(), "<unknown>", choice);
    }

    /**
     * See {@link android.net.wifi.WifiManager#allowAutojoin(int, boolean)}
     * @param netId the integer that identifies the network configuration
     * @param choice the user's choice to allow auto-join
     */
    @Override
    public void allowAutojoin(int netId, boolean choice) {
        enforceNetworkSettingsPermission();

        final int internalNetId = removeSecurityTypeFromNetworkId(netId);
        int callingUid = Binder.getCallingUid();
        mLog.info("allowAutojoin=% uid=%").c(choice).c(callingUid).flush();
        mWifiThreadRunner.post(() -> {
            WifiConfiguration config = mWifiConfigManager.getConfiguredNetwork(internalNetId);
            if (config == null) {
                return;
            }
            if (config.fromWifiNetworkSpecifier) {
                Log.e(TAG, "Auto-join configuration is not permitted for NetworkSpecifier "
                        + "connections: " + config);
                return;
            }
            if (config.isPasspoint() && !config.isEphemeral()) {
                Log.e(TAG,
                        "Auto-join configuration for a non-ephemeral Passpoint network should be "
                                + "configured using FQDN: "
                                + config);
                return;
            }
            // If the network is a suggestion, store the auto-join configure to the
            // WifiNetWorkSuggestionsManager.
            if (config.fromWifiNetworkSuggestion) {
                if (!mWifiNetworkSuggestionsManager
                        .allowNetworkSuggestionAutojoin(config, choice)) {
                    return;
                }
            }
            // even for Suggestion, modify the current ephemeral configuration so that
            // existing configuration auto-connection is updated correctly
            if (choice != config.allowAutojoin) {
                mWifiConfigManager.allowAutojoin(internalNetId, choice);
                // do not log this metrics for passpoint networks again here since it's already
                // logged in PasspointManager.
                if (!config.isPasspoint()) {
                    mWifiMetrics.logUserActionEvent(choice
                            ? UserActionEvent.EVENT_CONFIGURE_AUTO_CONNECT_ON
                            : UserActionEvent.EVENT_CONFIGURE_AUTO_CONNECT_OFF, internalNetId);
                }
            }
        });
    }

    /**
     * See {@link android.net.wifi.WifiManager#allowAutojoinPasspoint(String, boolean)}
     * @param fqdn the FQDN that identifies the passpoint configuration
     * @param enableAutojoin true to enable auto-join, false to disable
     */
    @Override
    public void allowAutojoinPasspoint(String fqdn, boolean enableAutojoin) {
        enforceNetworkSettingsPermission();
        if (fqdn == null) {
            throw new IllegalArgumentException("FQDN cannot be null");
        }

        int callingUid = Binder.getCallingUid();
        mLog.info("allowAutojoinPasspoint=% uid=%").c(enableAutojoin).c(callingUid).flush();
        mWifiThreadRunner.post(
                () -> mPasspointManager.enableAutojoin(null, fqdn, enableAutojoin));
    }

    /**
     * See {@link android.net.wifi.WifiManager
     * #setMacRandomizationSettingPasspointEnabled(String, boolean)}
     * @param fqdn the FQDN that identifies the passpoint configuration
     * @param enable true to enable mac randomization, false to disable
     */
    @Override
    public void setMacRandomizationSettingPasspointEnabled(String fqdn, boolean enable) {
        enforceNetworkSettingsPermission();
        if (fqdn == null) {
            throw new IllegalArgumentException("FQDN cannot be null");
        }

        int callingUid = Binder.getCallingUid();
        mLog.info("setMacRandomizationSettingPasspointEnabled=% uid=%")
                .c(enable).c(callingUid).flush();
        mWifiThreadRunner.post(
                () -> mPasspointManager.enableMacRandomization(fqdn, enable));
    }

    /**
     * See {@link android.net.wifi.WifiManager#setPasspointMeteredOverride(String, boolean)}
     * @param fqdn the FQDN that identifies the passpoint configuration
     * @param meteredOverride One of the values in {@link MeteredOverride}
     */
    @Override
    public void setPasspointMeteredOverride(String fqdn, int meteredOverride) {
        enforceNetworkSettingsPermission();
        if (fqdn == null) {
            throw new IllegalArgumentException("FQDN cannot be null");
        }

        int callingUid = Binder.getCallingUid();
        mLog.info("setPasspointMeteredOverride=% uid=%")
                .c(meteredOverride).c(callingUid).flush();
        mWifiThreadRunner.post(
                () -> mPasspointManager.setMeteredOverride(fqdn, meteredOverride));
    }

    /**
     * Provides backward compatibility for apps using
     * {@link WifiManager#getConnectionInfo()}, {@link WifiManager#getDhcpInfo()} when a
     * secondary STA is created as a result of a request from their app (peer to peer
     * WifiNetworkSpecifier request or oem paid/private suggestion).
     */
    private ClientModeManager getClientModeManagerIfSecondaryCmmRequestedByCallerPresent(
            int callingUid, @NonNull String callingPackageName) {
        List<ConcreteClientModeManager> secondaryCmms =
                mActiveModeWarden.getClientModeManagersInRoles(
                        ROLE_CLIENT_LOCAL_ONLY, ROLE_CLIENT_SECONDARY_LONG_LIVED);
        for (ConcreteClientModeManager cmm : secondaryCmms) {
            WorkSource reqWs = cmm.getRequestorWs();
            // If there are more than 1 secondary CMM for same app, return any one (should not
            // happen currently since we don't support 3 STA's concurrently).
            if (reqWs.equals(new WorkSource(callingUid, callingPackageName))) {
                mLog.info("getConnectionInfo providing secondary CMM info").flush();
                return cmm;
            }
        }
        // No secondary CMM's created for the app, return primary CMM.
        return mActiveModeWarden.getPrimaryClientModeManager();
    }

    /**
     * See {@link android.net.wifi.WifiManager#getConnectionInfo()}
     * @return the Wi-Fi information, contained in {@link WifiInfo}.
     */
    @Override
    public WifiInfo getConnectionInfo(String callingPackage, String callingFeatureId) {
        enforceAccessPermission();
        int uid = Binder.getCallingUid();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getConnectionInfo uid=%").c(uid).flush();
        }
        long ident = Binder.clearCallingIdentity();
        try {
            WifiInfo wifiInfo = mWifiThreadRunner.call(
                    () -> getClientModeManagerIfSecondaryCmmRequestedByCallerPresent(
                            uid, callingPackage)
                            .syncRequestConnectionInfo(), new WifiInfo());
            long redactions = wifiInfo.getApplicableRedactions();
            if (mWifiPermissionsUtil.checkLocalMacAddressPermission(uid)) {
                if (isVerboseLoggingEnabled()) {
                    Log.v(TAG, "Clearing REDACT_FOR_LOCAL_MAC_ADDRESS for " + callingPackage
                            + "(uid=" + uid + ")");
                }
                redactions &= ~NetworkCapabilities.REDACT_FOR_LOCAL_MAC_ADDRESS;
            }
            if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
                if (isVerboseLoggingEnabled()) {
                    Log.v(TAG, "Clearing REDACT_FOR_NETWORK_SETTINGS for " + callingPackage
                            + "(uid=" + uid + ")");
                }
                redactions &= ~NetworkCapabilities.REDACT_FOR_NETWORK_SETTINGS;
            }
            try {
                if (isVerboseLoggingEnabled()) {
                    Log.v(TAG, "Clearing REDACT_FOR_ACCESS_FINE_LOCATION for " + callingPackage
                            + "(uid=" + uid + ")");
                }
                mWifiPermissionsUtil.enforceCanAccessScanResults(callingPackage, callingFeatureId,
                        uid, null);
                redactions &= ~NetworkCapabilities.REDACT_FOR_ACCESS_FINE_LOCATION;
            } catch (SecurityException ignored) {
                if (isVerboseLoggingEnabled()) {
                    Log.v(TAG, "Keeping REDACT_FOR_ACCESS_FINE_LOCATION:" + ignored);
                }
            }
            WifiInfo wifiInfoCopy = wifiInfo.makeCopy(redactions);
            wifiInfoCopy.setNetworkId(addSecurityTypeToNetworkId(wifiInfoCopy.getNetworkId(),
                    convertWifiInfoSecurityTypeToWifiConfiguration(
                            wifiInfoCopy.getCurrentSecurityType())));
            return wifiInfoCopy;
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
    }

    /**
     * Return the results of the most recent access point scan, in the form of
     * a list of {@link ScanResult} objects.
     * @return the list of results
     */
    @Override
    public List<ScanResult> getScanResults(String callingPackage, String callingFeatureId) {
        enforceAccessPermission();
        int uid = Binder.getCallingUid();
        long ident = Binder.clearCallingIdentity();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getScanResults uid=%").c(uid).flush();
        }
        try {
            mWifiPermissionsUtil.enforceCanAccessScanResults(callingPackage, callingFeatureId,
                    uid, null);
            List<ScanResult> scanResults = mWifiThreadRunner.call(
                    mScanRequestProxy::getScanResults, Collections.emptyList());
            return scanResults;
        } catch (SecurityException e) {
            Log.w(TAG, "Permission violation - getScanResults not allowed for uid="
                    + uid + ", packageName=" + callingPackage + ", reason=" + e);
            return new ArrayList<>();
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
    }

    /**
     * Return the filtered ScanResults which may be authenticated by the suggested network
     * configurations.
     * @return The map of {@link WifiNetworkSuggestion} and the list of {@link ScanResult} which
     * may be authenticated by the corresponding network configuration.
     */
    @Override
    @NonNull
    public Map<WifiNetworkSuggestion, List<ScanResult>> getMatchingScanResults(
            @NonNull List<WifiNetworkSuggestion> networkSuggestions,
            @Nullable List<ScanResult> scanResults,
            String callingPackage, String callingFeatureId) {
        enforceAccessPermission();
        int uid = Binder.getCallingUid();
        long ident = Binder.clearCallingIdentity();
        try {
            mWifiPermissionsUtil.enforceCanAccessScanResults(callingPackage, callingFeatureId,
                    uid, null);

            return mWifiThreadRunner.call(
                    () -> {
                        if (!ScanResultUtil.validateScanResultList(scanResults)) {
                            return mWifiNetworkSuggestionsManager.getMatchingScanResults(
                                    networkSuggestions, mScanRequestProxy.getScanResults());
                        } else {
                            return mWifiNetworkSuggestionsManager.getMatchingScanResults(
                                    networkSuggestions, scanResults);
                        }
                    },
                    Collections.emptyMap());
        } catch (SecurityException e) {
            Log.w(TAG, "Permission violation - getMatchingScanResults not allowed for uid="
                    + uid + ", packageName=" + callingPackage + ", reason + e");
        } finally {
            Binder.restoreCallingIdentity(ident);
        }

        return Collections.emptyMap();
    }

    /**
     * Add or update a Passpoint configuration.
     *
     * @param config The Passpoint configuration to be added
     * @return true on success or false on failure
     */
    @Override
    public boolean addOrUpdatePasspointConfiguration(
            PasspointConfiguration config, String packageName) {
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return false;
        }
        int callingUid = Binder.getCallingUid();
        if (!isTargetSdkLessThanROrPrivileged(
                packageName, Binder.getCallingPid(), callingUid)) {
            mLog.info("addOrUpdatePasspointConfiguration not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return false;
        }
        mLog.info("addorUpdatePasspointConfiguration uid=%").c(callingUid).flush();
        return mWifiThreadRunner.call(
                () -> mPasspointManager.addOrUpdateProvider(config, callingUid, packageName,
                        false, true), false);
    }

    /**
     * Remove the Passpoint configuration identified by its FQDN (Fully Qualified Domain Name).
     *
     * @param fqdn The FQDN of the Passpoint configuration to be removed
     * @return true on success or false on failure
     */
    @Override
    public boolean removePasspointConfiguration(String fqdn, String packageName) {
        return removePasspointConfigurationInternal(fqdn, null);
    }

    /**
     * Remove a Passpoint profile based on either FQDN (multiple matching profiles) or a unique
     * identifier (one matching profile).
     *
     * @param fqdn The FQDN of the Passpoint configuration to be removed
     * @param uniqueId The unique identifier of the Passpoint configuration to be removed
     * @return true on success or false on failure
     */
    private boolean removePasspointConfigurationInternal(String fqdn, String uniqueId) {
        final int uid = Binder.getCallingUid();
        boolean privileged = false;
        if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)
                || mWifiPermissionsUtil.checkNetworkCarrierProvisioningPermission(uid)) {
            privileged = true;
        }
        mLog.info("removePasspointConfigurationInternal uid=%").c(Binder.getCallingUid()).flush();
        final boolean privilegedFinal = privileged;
        return mWifiThreadRunner.call(
                () -> mPasspointManager.removeProvider(uid, privilegedFinal, uniqueId, fqdn),
                false);
    }

    /**
     * Return the list of the installed Passpoint configurations.
     *
     * An empty list will be returned when no configuration is installed.
     * @param packageName String name of the calling package
     * @return A list of {@link PasspointConfiguration}.
     */
    @Override
    public List<PasspointConfiguration> getPasspointConfigurations(String packageName) {
        final int uid = Binder.getCallingUid();
        boolean privileged = false;
        if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)
                || mWifiPermissionsUtil.checkNetworkSetupWizardPermission(uid)) {
            privileged = true;
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getPasspointConfigurations uid=%").c(Binder.getCallingUid()).flush();
        }
        final boolean privilegedFinal = privileged;
        return mWifiThreadRunner.call(
            () -> mPasspointManager.getProviderConfigs(uid, privilegedFinal),
            Collections.emptyList());
    }

    /**
     * Query for a Hotspot 2.0 release 2 OSU icon
     * @param bssid The BSSID of the AP
     * @param fileName Icon file name
     */
    @Override
    public void queryPasspointIcon(long bssid, String fileName) {
        enforceAccessPermission();
        mLog.info("queryPasspointIcon uid=%").c(Binder.getCallingUid()).flush();
        mWifiThreadRunner.post(() -> {
            mActiveModeWarden.getPrimaryClientModeManager().syncQueryPasspointIcon(bssid, fileName);
        });
    }

    /**
     * Match the currently associated network against the SP matching the given FQDN
     * @param fqdn FQDN of the SP
     * @return ordinal [HomeProvider, RoamingProvider, Incomplete, None, Declined]
     */
    @Override
    public int matchProviderWithCurrentNetwork(String fqdn) {
        mLog.info("matchProviderWithCurrentNetwork uid=%").c(Binder.getCallingUid()).flush();
        return 0;
    }

     /**
     * Get the country code
     * @return Get the best choice country code for wifi, regardless of if it was set or
     * not.
     * Returns null when there is no country code available.
     */
    @Override
    public String getCountryCode() {
        enforceNetworkSettingsPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getCountryCode uid=%").c(Binder.getCallingUid()).flush();
        }
        return mCountryCode.getCountryCode();
    }

    /**
     * Set the Wifi country code. This call will override the country code set by telephony.
     * @param countryCode A 2-Character alphanumeric country code.
     *
     */
    @RequiresApi(Build.VERSION_CODES.S)
    @Override
    public void setOverrideCountryCode(@NonNull String countryCode) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.MANAGE_WIFI_COUNTRY_CODE, "WifiService");
        if (!WifiCountryCode.isValid(countryCode)) {
            throw new IllegalArgumentException("Country code must be a 2-Character alphanumeric"
                    + " code. But got countryCode " + countryCode
                    + " instead");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("setOverrideCountryCode uid=% countryCode=%")
                    .c(Binder.getCallingUid()).c(countryCode).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> mCountryCode.setOverrideCountryCode(countryCode));
    }

    /**
     * Clear the country code previously set through setOverrideCountryCode method.
     *
     */
    @RequiresApi(Build.VERSION_CODES.S)
    @Override
    public void clearOverrideCountryCode() {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.MANAGE_WIFI_COUNTRY_CODE, "WifiService");
        if (isVerboseLoggingEnabled()) {
            mLog.info("clearCountryCode uid=%").c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> mCountryCode.clearOverrideCountryCode());
    }

    /**
     * Change the default country code previously set from ro.boot.wificountrycode.
     * @param countryCode A 2-Character alphanumeric country code.
     *
     */
    @RequiresApi(Build.VERSION_CODES.S)
    @Override
    public void setDefaultCountryCode(@NonNull String countryCode) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.MANAGE_WIFI_COUNTRY_CODE, "WifiService");
        if (!WifiCountryCode.isValid(countryCode)) {
            throw new IllegalArgumentException("Country code must be a 2-Character alphanumeric"
                    + " code. But got countryCode " + countryCode
                    + " instead");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("setDefaultCountryCode uid=% countryCode=%")
                    .c(Binder.getCallingUid()).c(countryCode).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> mCountryCode.setDefaultCountryCode(countryCode));
    }

    @Override
    public boolean is24GHzBandSupported() {
        if (isVerboseLoggingEnabled()) {
            mLog.info("is24GHzBandSupported uid=%").c(Binder.getCallingUid()).flush();
        }

        return is24GhzBandSupportedInternal();
    }

    private boolean is24GhzBandSupportedInternal() {
        if (mContext.getResources().getBoolean(R.bool.config_wifi24ghzSupport)) {
            return true;
        }
        return mWifiThreadRunner.call(
                () -> mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ).length > 0,
                false);
    }


    @Override
    public boolean is5GHzBandSupported() {
        if (isVerboseLoggingEnabled()) {
            mLog.info("is5GHzBandSupported uid=%").c(Binder.getCallingUid()).flush();
        }

        return is5GhzBandSupportedInternal();
    }

    private boolean is5GhzBandSupportedInternal() {
        if (mContext.getResources().getBoolean(R.bool.config_wifi5ghzSupport)) {
            return true;
        }
        return mWifiThreadRunner.call(
                () -> mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ).length > 0,
                false);
    }

    @Override
    public boolean is6GHzBandSupported() {
        if (isVerboseLoggingEnabled()) {
            mLog.info("is6GHzBandSupported uid=%").c(Binder.getCallingUid()).flush();
        }

        return is6GhzBandSupportedInternal();
    }

    private boolean is6GhzBandSupportedInternal() {
        if (mContext.getResources().getBoolean(R.bool.config_wifi6ghzSupport)) {
            return true;
        }
        return mWifiThreadRunner.call(
                () -> mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_6_GHZ).length > 0,
                false);
    }

    @Override
    public boolean is60GHzBandSupported() {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }

        if (isVerboseLoggingEnabled()) {
            mLog.info("is60GHzBandSupported uid=%").c(Binder.getCallingUid()).flush();
        }

        return is60GhzBandSupportedInternal();
    }

    private boolean is60GhzBandSupportedInternal() {
        if (mContext.getResources().getBoolean(R.bool.config_wifi60ghzSupport)) {
            return true;
        }
        return mWifiThreadRunner.call(
                () -> mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_60_GHZ).length > 0,
                false);
    }

    @Override
    public boolean isWifiStandardSupported(@WifiStandard int standard) {
        return mWifiThreadRunner.call(
                () -> mActiveModeWarden.getPrimaryClientModeManager().isWifiStandardSupported(
                        standard), false);
    }

    /**
     * Return the DHCP-assigned addresses from the last successful DHCP request,
     * if any.
     * @return the DHCP information
     * @deprecated
     */
    @Override
    public DhcpInfo getDhcpInfo(@NonNull String packageName) {
        enforceAccessPermission();
        int callingUid = Binder.getCallingUid();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getDhcpInfo uid=%").c(callingUid).flush();
        }
        DhcpResultsParcelable dhcpResults = mWifiThreadRunner.call(
                () -> getClientModeManagerIfSecondaryCmmRequestedByCallerPresent(
                        callingUid, packageName)
                        .syncGetDhcpResultsParcelable(), new DhcpResultsParcelable());

        DhcpInfo info = new DhcpInfo();

        if (dhcpResults.baseConfiguration != null) {
            if (dhcpResults.baseConfiguration.getIpAddress() != null
                    && dhcpResults.baseConfiguration.getIpAddress().getAddress()
                    instanceof Inet4Address) {
                info.ipAddress = Inet4AddressUtils.inet4AddressToIntHTL(
                        (Inet4Address) dhcpResults.baseConfiguration.getIpAddress().getAddress());
            }

            if (dhcpResults.baseConfiguration.getGateway() != null) {
                info.gateway = Inet4AddressUtils.inet4AddressToIntHTL(
                        (Inet4Address) dhcpResults.baseConfiguration.getGateway());
            }

            int dnsFound = 0;
            for (InetAddress dns : dhcpResults.baseConfiguration.getDnsServers()) {
                if (dns instanceof Inet4Address) {
                    if (dnsFound == 0) {
                        info.dns1 = Inet4AddressUtils.inet4AddressToIntHTL((Inet4Address) dns);
                    } else {
                        info.dns2 = Inet4AddressUtils.inet4AddressToIntHTL((Inet4Address) dns);
                    }
                    if (++dnsFound > 1) break;
                }
            }
        }
        String serverAddress = dhcpResults.serverAddress;
        if (serverAddress != null) {
            InetAddress serverInetAddress = InetAddresses.parseNumericAddress(serverAddress);
            info.serverAddress =
                    Inet4AddressUtils.inet4AddressToIntHTL((Inet4Address) serverInetAddress);
        }
        info.leaseDuration = dhcpResults.leaseDuration;

        return info;
    }

    /**
     * enable TDLS for the local NIC to remote NIC
     * The APPs don't know the remote MAC address to identify NIC though,
     * so we need to do additional work to find it from remote IP address
     */

    private static class TdlsTaskParams {
        String mRemoteIpAddress;
        boolean mEnable;
    }

    private class TdlsTask extends AsyncTask<TdlsTaskParams, Integer, Integer> {
        @Override
        protected Integer doInBackground(TdlsTaskParams... params) {

            // Retrieve parameters for the call
            TdlsTaskParams param = params[0];
            String remoteIpAddress = param.mRemoteIpAddress.trim();
            boolean enable = param.mEnable;

            // Get MAC address of Remote IP
            String macAddress = null;

            try (BufferedReader reader = new BufferedReader(new FileReader("/proc/net/arp"))) {
                // Skip over the line bearing column titles
                reader.readLine();

                String line;
                while ((line = reader.readLine()) != null) {
                    String[] tokens = line.split("[ ]+");
                    if (tokens.length < 6) {
                        continue;
                    }

                    // ARP column format is
                    // Address HWType HWAddress Flags Mask IFace
                    String ip = tokens[0];
                    String mac = tokens[3];

                    if (remoteIpAddress.equals(ip)) {
                        macAddress = mac;
                        break;
                    }
                }

                if (macAddress == null) {
                    Log.w(TAG, "Did not find remoteAddress {" + remoteIpAddress + "} in "
                            + "/proc/net/arp");
                } else {
                    enableTdlsWithMacAddress(macAddress, enable);
                }

            } catch (FileNotFoundException e) {
                Log.e(TAG, "Could not open /proc/net/arp to lookup mac address");
            } catch (IOException e) {
                Log.e(TAG, "Could not read /proc/net/arp to lookup mac address");
            }
            return 0;
        }
    }

    @Override
    public void enableTdls(String remoteAddress, boolean enable) {
        if (remoteAddress == null) {
          throw new IllegalArgumentException("remoteAddress cannot be null");
        }
        mLog.info("enableTdls uid=% enable=%").c(Binder.getCallingUid()).c(enable).flush();
        TdlsTaskParams params = new TdlsTaskParams();
        params.mRemoteIpAddress = remoteAddress;
        params.mEnable = enable;
        new TdlsTask().execute(params);
    }


    @Override
    public void enableTdlsWithMacAddress(String remoteMacAddress, boolean enable) {
        mLog.info("enableTdlsWithMacAddress uid=% enable=%")
                .c(Binder.getCallingUid())
                .c(enable)
                .flush();
        if (remoteMacAddress == null) {
          throw new IllegalArgumentException("remoteMacAddress cannot be null");
        }
        mWifiThreadRunner.post(() ->
                mActiveModeWarden.getPrimaryClientModeManager().enableTdls(
                        remoteMacAddress, enable));
    }

    /**
     * Temporarily disable a network, should be trigger when user disconnect a network
     */
    @Override
    public void disableEphemeralNetwork(String network, String packageName) {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.CHANGE_WIFI_STATE,
                "WifiService");
        if (!isPrivileged(Binder.getCallingPid(), Binder.getCallingUid())) {
            mLog.info("disableEphemeralNetwork not allowed for uid=%")
                    .c(Binder.getCallingUid()).flush();
            return;
        }
        mLog.info("disableEphemeralNetwork uid=%").c(Binder.getCallingUid()).flush();
        mWifiThreadRunner.post(() -> mWifiConfigManager.userTemporarilyDisabledNetwork(network,
                Binder.getCallingUid()));
    }

    private void removeAppStateInternal(int uid, @NonNull String pkgName) {
        ApplicationInfo ai = new ApplicationInfo();
        ai.packageName = pkgName;
        ai.uid = uid;
        mWifiConfigManager.removeNetworksForApp(ai);
        mScanRequestProxy.clearScanRequestTimestampsForApp(pkgName, uid);

        // Remove all suggestions from the package.
        mWifiNetworkSuggestionsManager.removeApp(pkgName);
        mWifiInjector.getWifiNetworkFactory().removeUserApprovedAccessPointsForApp(
                pkgName);

        // Remove all Passpoint profiles from package.
        mWifiInjector.getPasspointManager().removePasspointProviderWithPackage(
                pkgName);
    }

    private void registerForBroadcasts() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_PACKAGE_FULLY_REMOVED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        intentFilter.addDataScheme("package");
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                int uid = intent.getIntExtra(Intent.EXTRA_UID, -1);
                Uri uri = intent.getData();
                if (uid == -1 || uri == null) {
                    Log.e(TAG, "Uid or Uri is missing for action:" + intent.getAction());
                    return;
                }
                String pkgName = uri.getSchemeSpecificPart();
                PackageManager pm = context.getPackageManager();
                PackageInfo packageInfo = null;
                try {
                    packageInfo = pm.getPackageInfo(pkgName, 0);
                } catch (PackageManager.NameNotFoundException e) {
                    Log.w(TAG, "Couldn't get PackageInfo for package:" + pkgName);
                }
                // If package is not removed or disabled, just ignore.
                if (packageInfo != null
                        && packageInfo.applicationInfo != null
                        && packageInfo.applicationInfo.enabled) {
                    return;
                }
                Log.d(TAG, "Remove settings for package:" + pkgName);
                // Call the method in the main Wifi thread.
                mWifiThreadRunner.post(() -> {
                    removeAppStateInternal(uid, pkgName);
                });
            }
        }, intentFilter);
    }

    private void registerForCarrierConfigChange() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final int subId = SubscriptionManager.getActiveDataSubscriptionId();
                // post operation to handler thread
                mWifiThreadRunner.post(() -> {
                    Log.d(TAG, "ACTION_CARRIER_CONFIG_CHANGED, active subId: " + subId);
                    mTetheredSoftApTracker.updateSoftApCapabilityWhenCarrierConfigChanged(subId);
                    mActiveModeWarden.updateSoftApCapability(
                            mTetheredSoftApTracker.getSoftApCapability());
                });
            }
        }, filter);

        WifiPhoneStateListener phoneStateListener = new WifiPhoneStateListener(
                mWifiInjector.getWifiHandlerThread().getLooper());

        mContext.getSystemService(TelephonyManager.class).listen(
                phoneStateListener, PhoneStateListener.LISTEN_ACTIVE_DATA_SUBSCRIPTION_ID_CHANGE);
    }

    @Override
    public int handleShellCommand(@NonNull ParcelFileDescriptor in,
            @NonNull ParcelFileDescriptor out, @NonNull ParcelFileDescriptor err,
            @NonNull String[] args) {
        WifiShellCommand shellCommand =  new WifiShellCommand(mWifiInjector, this, mContext,
                mWifiGlobals, mWifiThreadRunner);
        return shellCommand.exec(this, in.getFileDescriptor(), out.getFileDescriptor(),
                err.getFileDescriptor(), args);
    }

    private void updateWifiMetrics() {
        mWifiThreadRunner.run(() -> {
            mWifiMetrics.updateSavedNetworks(
                    mWifiConfigManager.getSavedNetworks(Process.WIFI_UID));
            mActiveModeWarden.updateMetrics();
            mPasspointManager.updateMetrics();
        });
        boolean isEnhancedMacRandEnabled = mFrameworkFacade.getIntegerSetting(mContext,
                WifiConfigManager.ENHANCED_MAC_RANDOMIZATION_FEATURE_FORCE_ENABLE_FLAG, 0) == 1
                ? true : false;
        mWifiMetrics.setEnhancedMacRandomizationForceEnabled(isEnhancedMacRandEnabled);
        mWifiMetrics.setIsScanningAlwaysEnabled(
                mSettingsStore.isScanAlwaysAvailableToggleEnabled());
        mWifiMetrics.setVerboseLoggingEnabled(isVerboseLoggingEnabled());
        mWifiMetrics.setWifiWakeEnabled(mWifiInjector.getWakeupController().isEnabled());
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (mContext.checkCallingOrSelfPermission(android.Manifest.permission.DUMP)
                != PERMISSION_GRANTED) {
            pw.println("Permission Denial: can't dump WifiService from from pid="
                    + Binder.getCallingPid()
                    + ", uid=" + Binder.getCallingUid());
            return;
        }
        if (args != null && args.length > 0 && WifiMetrics.PROTO_DUMP_ARG.equals(args[0])) {
            // WifiMetrics proto bytes were requested. Dump only these.
            updateWifiMetrics();
            mWifiMetrics.dump(fd, pw, args);
        } else if (args != null && args.length > 0 && IpClientUtil.DUMP_ARG.equals(args[0])) {
            // IpClient dump was requested. Pass it along and take no further action.
            String[] ipClientArgs = new String[args.length - 1];
            System.arraycopy(args, 1, ipClientArgs, 0, ipClientArgs.length);
            mActiveModeWarden.getPrimaryClientModeManager().dumpIpClient(fd, pw, ipClientArgs);
        } else if (args != null && args.length > 0 && WifiScoreReport.DUMP_ARG.equals(args[0])) {
            mActiveModeWarden.getPrimaryClientModeManager().dumpWifiScoreReport(fd, pw, args);
        } else if (args != null && args.length > 0 && WifiScoreCard.DUMP_ARG.equals(args[0])) {
            WifiScoreCard wifiScoreCard = mWifiInjector.getWifiScoreCard();
            String networkListBase64 = mWifiThreadRunner.call(() ->
                    wifiScoreCard.getNetworkListBase64(true), "");
            pw.println(networkListBase64);
        } else {
            pw.println("Verbose logging is " + (isVerboseLoggingEnabled() ? "on" : "off"));
            pw.println("Stay-awake conditions: " +
                    mFacade.getIntegerSetting(mContext,
                            Settings.Global.STAY_ON_WHILE_PLUGGED_IN, 0));
            pw.println("mInIdleMode " + mInIdleMode);
            pw.println("mScanPending " + mScanPending);
            pw.println("SettingsStore:");
            mSettingsStore.dump(fd, pw, args);
            mActiveModeWarden.dump(fd, pw, args);
            mMakeBeforeBreakManager.dump(fd, pw, args);
            pw.println();
            mWifiTrafficPoller.dump(fd, pw, args);
            pw.println();
            pw.println("Locks held:");
            mWifiLockManager.dump(pw);
            pw.println();
            mWifiMulticastLockManager.dump(pw);
            pw.println();
            WifiScoreCard wifiScoreCard = mWifiInjector.getWifiScoreCard();
            String networkListBase64 = mWifiThreadRunner.call(() ->
                    wifiScoreCard.getNetworkListBase64(true), "");
            pw.println("WifiScoreCard:");
            pw.println(networkListBase64);

            updateWifiMetrics();
            mWifiMetrics.dump(fd, pw, args);

            pw.println();
            mWifiThreadRunner.run(() -> mWifiNetworkSuggestionsManager.dump(fd, pw, args));
            pw.println();
            mWifiBackupRestore.dump(fd, pw, args);
            pw.println();
            pw.println("ScoringParams: " + mWifiInjector.getScoringParams());
            pw.println();
            mWifiThreadRunner.run(() -> {
                mWifiInjector.getWifiNetworkScoreCache().dumpWithLatestScanResults(
                        fd, pw, args, mScanRequestProxy.getScanResults());
                mWifiInjector.getSettingsConfigStore().dump(fd, pw, args);
            });
            pw.println();
            mCountryCode.dump(fd, pw, args);
            mWifiInjector.getWifiNetworkFactory().dump(fd, pw, args);
            mWifiInjector.getUntrustedWifiNetworkFactory().dump(fd, pw, args);
            mWifiInjector.getOemWifiNetworkFactory().dump(fd, pw, args);
            pw.println("Wlan Wake Reasons:" + mWifiNative.getWlanWakeReasonCount());
            pw.println();
            mWifiConfigManager.dump(fd, pw, args);
            pw.println();
            mPasspointManager.dump(pw);
            pw.println();
            mWifiInjector.getWifiDiagnostics().captureBugReportData(
                    WifiDiagnostics.REPORT_REASON_USER_ACTION);
            mWifiInjector.getWifiDiagnostics().dump(fd, pw, args);
            mWifiConnectivityManager.dump(fd, pw, args);
            mWifiThreadRunner.run(() -> {
                mWifiHealthMonitor.dump(fd, pw, args);
            });
            mWifiThreadRunner.run(() -> {
                mWifiScoreCard.dump(fd, pw, args);
            });
            mWifiInjector.getWakeupController().dump(fd, pw, args);
            mWifiInjector.getWifiLastResortWatchdog().dump(fd, pw, args);
            mWifiInjector.getAdaptiveConnectivityEnabledSettingObserver().dump(fd, pw, args);
            mWifiInjector.getWifiGlobals().dump(fd, pw, args);
            mWifiInjector.getSarManager().dump(fd, pw, args);
            pw.println();
            mLastCallerInfoManager.dump(pw);
            pw.println();
            mWifiInjector.getLinkProbeManager().dump(fd, pw, args);
        }
    }

    @Override
    public boolean acquireWifiLock(IBinder binder, int lockMode, String tag, WorkSource ws) {
        mLog.info("acquireWifiLock uid=% lockMode=%")
                .c(Binder.getCallingUid())
                .c(lockMode).flush();

        // Check on permission to make this call
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.WAKE_LOCK, null);

        // If no UID is provided in worksource, use the calling UID
        WorkSource updatedWs = (ws == null || ws.isEmpty())
                ? new WorkSource(Binder.getCallingUid()) : ws;

        if (!WifiLockManager.isValidLockMode(lockMode)) {
            throw new IllegalArgumentException("lockMode =" + lockMode);
        }

        return mWifiThreadRunner.call(() ->
                mWifiLockManager.acquireWifiLock(lockMode, tag, binder, updatedWs), false);
    }

    @Override
    public void updateWifiLockWorkSource(IBinder binder, WorkSource ws) {
        mLog.info("updateWifiLockWorkSource uid=%").c(Binder.getCallingUid()).flush();

        // Check on permission to make this call
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.UPDATE_DEVICE_STATS, null);

        // If no UID is provided in worksource, use the calling UID
        WorkSource updatedWs = (ws == null || ws.isEmpty())
                ? new WorkSource(Binder.getCallingUid()) : ws;

        mWifiThreadRunner.run(() ->
                mWifiLockManager.updateWifiLockWorkSource(binder, updatedWs));
    }

    @Override
    public boolean releaseWifiLock(IBinder binder) {
        mLog.info("releaseWifiLock uid=%").c(Binder.getCallingUid()).flush();

        // Check on permission to make this call
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.WAKE_LOCK, null);

        return mWifiThreadRunner.call(() ->
                mWifiLockManager.releaseWifiLock(binder), false);
    }

    @Override
    public void initializeMulticastFiltering() {
        enforceMulticastChangePermission();
        mLog.info("initializeMulticastFiltering uid=%").c(Binder.getCallingUid()).flush();
        mWifiMulticastLockManager.initializeFiltering();
    }

    @Override
    public void acquireMulticastLock(IBinder binder, String tag) {
        enforceMulticastChangePermission();
        mLog.info("acquireMulticastLock uid=%").c(Binder.getCallingUid()).flush();
        mWifiMulticastLockManager.acquireLock(binder, tag);
    }

    @Override
    public void releaseMulticastLock(String tag) {
        enforceMulticastChangePermission();
        mLog.info("releaseMulticastLock uid=%").c(Binder.getCallingUid()).flush();
        mWifiMulticastLockManager.releaseLock(tag);
    }

    @Override
    public boolean isMulticastEnabled() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("isMulticastEnabled uid=%").c(Binder.getCallingUid()).flush();
        }
        return mWifiMulticastLockManager.isMulticastEnabled();
    }

    @Override
    public void enableVerboseLogging(int verbose) {
        enforceAccessPermission();
        enforceNetworkSettingsPermission();
        mLog.info("enableVerboseLogging uid=% verbose=%")
                .c(Binder.getCallingUid())
                .c(verbose).flush();
        boolean enabled = verbose > 0;
        mWifiInjector.getSettingsConfigStore().put(WIFI_VERBOSE_LOGGING_ENABLED, enabled);
        onVerboseLoggingStatusChanged(enabled);
        enableVerboseLoggingInternal(verbose);
    }

    private void onVerboseLoggingStatusChanged(boolean enabled) {
        int itemCount = mRegisteredWifiLoggingStatusListeners.beginBroadcast();
        for (int i = 0; i < itemCount; i++) {
            try {
                mRegisteredWifiLoggingStatusListeners.getBroadcastItem(i)
                        .onStatusChanged(enabled);
            } catch (RemoteException e) {
                Log.e(TAG, "onVerboseLoggingStatusChanged: RemoteException -- ", e);
            }

        }
        mRegisteredWifiLoggingStatusListeners.finishBroadcast();
    }

    private boolean isVerboseLoggingEnabled() {
        return WifiManager.VERBOSE_LOGGING_LEVEL_DISABLED != mVerboseLoggingLevel;
    }

    private void enableVerboseLoggingInternal(int verbose) {
        if (verbose > WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED
                && mBuildProperties.isUserBuild()) {
            throw new SecurityException(TAG + ": Not allowed for the user build.");
        }
        mVerboseLoggingLevel = verbose;

        // Update wifi globals before sending the verbose logging change.
        mWifiThreadRunner.removeCallbacks(mAutoDisableShowKeyVerboseLoggingModeRunnable);
        if (WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED_SHOW_KEY == mVerboseLoggingLevel) {
            mWifiGlobals.setShowKeyVerboseLoggingModeEnabled(true);
            mWifiThreadRunner.postDelayed(mAutoDisableShowKeyVerboseLoggingModeRunnable,
                    AUTO_DISABLE_SHOW_KEY_COUNTDOWN_MILLIS);
        } else {
            // Ensure the show key mode is disabled.
            mWifiGlobals.setShowKeyVerboseLoggingModeEnabled(false);
        }

        mActiveModeWarden.enableVerboseLogging(isVerboseLoggingEnabled());
        mWifiLockManager.enableVerboseLogging(verbose);
        mWifiMulticastLockManager.enableVerboseLogging(verbose);
        mWifiInjector.enableVerboseLogging(verbose);
        mWifiInjector.getSarManager().enableVerboseLogging(verbose);
    }

    @Override
    public int getVerboseLoggingLevel() {
        if (isVerboseLoggingEnabled()) {
            mLog.info("getVerboseLoggingLevel uid=%").c(Binder.getCallingUid()).flush();
        }
        return mVerboseLoggingLevel;
    }

    private Runnable mAutoDisableShowKeyVerboseLoggingModeRunnable = new Runnable() {
        @Override
        public void run() {
            // If still enabled, fallback to the regular verbose logging mode.
            if (isVerboseLoggingEnabled()) {
                enableVerboseLoggingInternal(WifiManager.VERBOSE_LOGGING_LEVEL_ENABLED);
            }
        }
    };

    @Override
    public void factoryReset(String packageName) {
        enforceNetworkSettingsPermission();
        if (enforceChangePermission(packageName) != MODE_ALLOWED) {
            return;
        }
        mLog.info("factoryReset uid=%").c(Binder.getCallingUid()).flush();
        if (mUserManager.hasUserRestrictionForUser(
                UserManager.DISALLOW_NETWORK_RESET,
                UserHandle.getUserHandleForUid(Binder.getCallingUid()))) {
            return;
        }
        if (!mUserManager.hasUserRestrictionForUser(
                UserManager.DISALLOW_CONFIG_TETHERING,
                UserHandle.getUserHandleForUid(Binder.getCallingUid()))) {
            // Turn mobile hotspot off
            stopSoftApInternal(WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        }

        if (mUserManager.hasUserRestrictionForUser(
                UserManager.DISALLOW_CONFIG_WIFI,
                UserHandle.getUserHandleForUid(Binder.getCallingUid()))) {
            return;
        }
        // Delete all Wifi SSIDs
        List<WifiConfiguration> networks = mWifiThreadRunner.call(
                () -> mWifiConfigManager.getSavedNetworks(Process.WIFI_UID),
                Collections.emptyList());
        for (WifiConfiguration network : networks) {
            removeNetwork(network.networkId, packageName);
        }
        // Delete all Passpoint configurations
        List<PasspointConfiguration> configs = mWifiThreadRunner.call(
                () -> mPasspointManager.getProviderConfigs(Process.WIFI_UID /* ignored */, true),
                Collections.emptyList());
        for (PasspointConfiguration config : configs) {
            removePasspointConfigurationInternal(null, config.getUniqueId());
        }
        mWifiThreadRunner.post(() -> {
            mPasspointManager.clearAnqpRequestsAndFlushCache();
            mWifiConfigManager.clearUserTemporarilyDisabledList();
            mWifiConfigManager.removeAllEphemeralOrPasspointConfiguredNetworks();
            mWifiInjector.getWifiNetworkFactory().clear();
            mWifiNetworkSuggestionsManager.clear();
            mWifiInjector.getWifiScoreCard().clear();
            mWifiHealthMonitor.clear();
            mWifiCarrierInfoManager.clear();
            notifyFactoryReset();
        });
    }

    /**
     * Notify the Factory Reset Event to application who may installed wifi configurations.
     */
    private void notifyFactoryReset() {
        Intent intent = new Intent(WifiManager.ACTION_NETWORK_SETTINGS_RESET);

        // Retrieve list of broadcast receivers for this broadcast & send them directed broadcasts
        // to wake them up (if they're in background).
        List<ResolveInfo> resolveInfos =
                mContext.getPackageManager().queryBroadcastReceiversAsUser(
                        intent, 0,
                        UserHandle.of(mWifiInjector.getWifiPermissionsWrapper().getCurrentUser()));
        if (resolveInfos == null || resolveInfos.isEmpty()) return; // No need to send broadcast.

        for (ResolveInfo resolveInfo : resolveInfos) {
            Intent intentToSend = new Intent(intent);
            intentToSend.setComponent(new ComponentName(
                    resolveInfo.activityInfo.applicationInfo.packageName,
                    resolveInfo.activityInfo.name));
            mContext.sendBroadcastAsUser(intentToSend, UserHandle.CURRENT,
                    android.Manifest.permission.NETWORK_CARRIER_PROVISIONING);
        }
    }

    @Override
    public Network getCurrentNetwork() {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("getCurrentNetwork uid=%").c(Binder.getCallingUid()).flush();
        }
        return getPrimaryClientModeManagerBlockingThreadSafe().syncGetCurrentNetwork();
    }

    public static String toHexString(String s) {
        if (s == null) {
            return "null";
        }
        StringBuilder sb = new StringBuilder();
        sb.append('\'').append(s).append('\'');
        for (int n = 0; n < s.length(); n++) {
            sb.append(String.format(" %02x", s.charAt(n) & 0xffff));
        }
        return sb.toString();
    }

    /**
     * Retrieve the data to be backed to save the current state.
     *
     * @return  Raw byte stream of the data to be backed up.
     */
    @Override
    public byte[] retrieveBackupData() {
        enforceNetworkSettingsPermission();
        mLog.info("retrieveBackupData uid=%").c(Binder.getCallingUid()).flush();
        Log.d(TAG, "Retrieving backup data");
        List<WifiConfiguration> wifiConfigurations = mWifiThreadRunner.call(
                () -> mWifiConfigManager.getConfiguredNetworksWithPasswords(), null);
        byte[] backupData =
                mWifiBackupRestore.retrieveBackupDataFromConfigurations(wifiConfigurations);
        Log.d(TAG, "Retrieved backup data");
        return backupData;
    }

    /**
     * Helper method to restore networks retrieved from backup data.
     *
     * @param configurations list of WifiConfiguration objects parsed from the backup data.
     */
    private void restoreNetworks(List<WifiConfiguration> configurations) {
        if (configurations == null) {
            Log.e(TAG, "Backup data parse failed");
            return;
        }
        int callingUid = Binder.getCallingUid();
        mWifiThreadRunner.run(
                () -> {
                    for (WifiConfiguration configuration : configurations) {
                        int networkId =
                                mWifiConfigManager.addOrUpdateNetwork(configuration, callingUid)
                                        .getNetworkId();
                        if (networkId == WifiConfiguration.INVALID_NETWORK_ID) {
                            Log.e(TAG, "Restore network failed: "
                                    + configuration.getProfileKey());
                            continue;
                        }
                        // Enable all networks restored.
                        mWifiConfigManager.enableNetwork(networkId, false, callingUid, null);
                        // Restore auto-join param.
                        mWifiConfigManager.allowAutojoin(networkId, configuration.allowAutojoin);
                    }
                });
    }

    /**
     * Restore state from the backed up data.
     *
     * @param data Raw byte stream of the backed up data.
     */
    @Override
    public void restoreBackupData(byte[] data) {
        enforceNetworkSettingsPermission();
        mLog.info("restoreBackupData uid=%").c(Binder.getCallingUid()).flush();
        Log.d(TAG, "Restoring backup data");
        List<WifiConfiguration> wifiConfigurations =
                mWifiBackupRestore.retrieveConfigurationsFromBackupData(data);
        restoreNetworks(wifiConfigurations);
        Log.d(TAG, "Restored backup data");
    }

    /**
     * Retrieve the soft ap config data to be backed to save current config data.
     *
     * @return  Raw byte stream of the data to be backed up.
     */
    @Override
    public byte[] retrieveSoftApBackupData() {
        enforceNetworkSettingsPermission();
        mLog.info("retrieveSoftApBackupData uid=%").c(Binder.getCallingUid()).flush();
        SoftApConfiguration config = mWifiThreadRunner.call(mWifiApConfigStore::getApConfiguration,
                new SoftApConfiguration.Builder().build());
        byte[] backupData =
                mSoftApBackupRestore.retrieveBackupDataFromSoftApConfiguration(config);
        Log.d(TAG, "Retrieved soft ap backup data");
        return backupData;
    }

    /**
     * Restore soft ap config from the backed up data.
     *
     * @param data Raw byte stream of the backed up data.
     * @return restored SoftApConfiguration or Null if data is invalid.
     */
    @Override
    public SoftApConfiguration restoreSoftApBackupData(byte[] data) {
        enforceNetworkSettingsPermission();
        mLog.info("restoreSoftApBackupData uid=%").c(Binder.getCallingUid()).flush();
        SoftApConfiguration softApConfig =
                mSoftApBackupRestore.retrieveSoftApConfigurationFromBackupData(data);
        if (softApConfig != null) {
            mWifiThreadRunner.post(() -> mWifiApConfigStore.setApConfiguration(
                    mWifiApConfigStore.resetToDefaultForUnsupportedConfig(
                    mWifiApConfigStore.upgradeSoftApConfiguration(softApConfig))));
            Log.d(TAG, "Restored soft ap backup data");
        }
        return softApConfig;
    }


    /**
     * Restore state from the older supplicant back up data.
     * The old backup data was essentially a backup of wpa_supplicant.conf & ipconfig.txt file.
     *
     * @param supplicantData Raw byte stream of wpa_supplicant.conf
     * @param ipConfigData Raw byte stream of ipconfig.txt
     */
    public void restoreSupplicantBackupData(byte[] supplicantData, byte[] ipConfigData) {
        enforceNetworkSettingsPermission();
        mLog.trace("restoreSupplicantBackupData uid=%").c(Binder.getCallingUid()).flush();
        Log.d(TAG, "Restoring supplicant backup data");
        List<WifiConfiguration> wifiConfigurations =
                mWifiBackupRestore.retrieveConfigurationsFromSupplicantBackupData(
                        supplicantData, ipConfigData);
        restoreNetworks(wifiConfigurations);
        Log.d(TAG, "Restored supplicant backup data");
    }

    /**
     * Starts subscription provisioning with a provider.
     *
     * @param provider {@link OsuProvider} the provider to provision with
     * @param callback {@link IProvisioningCallback} the callback object to inform status
     */
    @Override
    public void startSubscriptionProvisioning(OsuProvider provider,
            IProvisioningCallback callback) {
        if (provider == null) {
            throw new IllegalArgumentException("Provider must not be null");
        }
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        final int uid = Binder.getCallingUid();
        mLog.trace("startSubscriptionProvisioning uid=%").c(uid).flush();
        if (getPrimaryClientModeManagerBlockingThreadSafe()
                .syncStartSubscriptionProvisioning(uid, provider, callback)) {
            mLog.trace("Subscription provisioning started with %")
                    .c(provider.toString()).flush();
        }
    }

    /**
     * See
     * {@link WifiManager#registerTrafficStateCallback(Executor, WifiManager.TrafficStateCallback)}
     *
     * @param callback Traffic State callback to register
     *
     * @throws SecurityException if the caller does not have permission to register a callback
     * @throws RemoteException if remote exception happens
     * @throws IllegalArgumentException if the arguments are null or invalid
     */
    @Override
    public void registerTrafficStateCallback(ITrafficStateCallback callback) {
        // verify arguments
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }
        enforceNetworkSettingsPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("registerTrafficStateCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> mWifiTrafficPoller.addCallback(callback));
    }

    /**
     * see {@link android.net.wifi.WifiManager#unregisterTrafficStateCallback(
     * WifiManager.TrafficStateCallback)}
     *
     * @param callback Traffic State callback to unregister
     *
     * @throws SecurityException if the caller does not have permission to register a callback
     */
    @Override
    public void unregisterTrafficStateCallback(ITrafficStateCallback callback) {
        enforceNetworkSettingsPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("unregisterTrafficStateCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> mWifiTrafficPoller.removeCallback(callback));
    }

    private long getSupportedFeaturesInternal() {
        long supportedFeatureSet = mWifiThreadRunner.call(
                () -> mActiveModeWarden.getPrimaryClientModeManager().getSupportedFeatures(),
                0L);
        // Mask the feature set against system properties.
        boolean rttSupported = mContext.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_WIFI_RTT);
        if (!rttSupported) {
            // flags filled in by vendor HAL, remove if overlay disables it.
            supportedFeatureSet &=
                    ~(WifiManager.WIFI_FEATURE_D2D_RTT | WifiManager.WIFI_FEATURE_D2AP_RTT);
        }
        if (!mContext.getResources().getBoolean(
                R.bool.config_wifi_p2p_mac_randomization_supported)) {
            // flags filled in by vendor HAL, remove if overlay disables it.
            supportedFeatureSet &= ~WifiManager.WIFI_FEATURE_P2P_RAND_MAC;
        }
        if (mContext.getResources().getBoolean(
                R.bool.config_wifi_connected_mac_randomization_supported)) {
            // no corresponding flags in vendor HAL, set if overlay enables it.
            supportedFeatureSet |= WifiManager.WIFI_FEATURE_CONNECTED_RAND_MAC;
        }
        if (ApConfigUtil.isApMacRandomizationSupported(mContext)) {
            // no corresponding flags in vendor HAL, set if overlay enables it.
            supportedFeatureSet |= WifiManager.WIFI_FEATURE_AP_RAND_MAC;
        }
        if (SdkLevel.isAtLeastS()) {
            if (ApConfigUtil.isBridgedModeSupported(mContext)) {
                // The bridged mode requires the kernel network modules support.
                // It doesn't relate the vendor HAL, set if overlay enables it.
                supportedFeatureSet |= WifiManager.WIFI_FEATURE_BRIDGED_AP;
            }
            if (mContext.getResources().getBoolean(
                    R.bool.config_wifiStaWithBridgedSoftApConcurrencySupported)) {
                // The bridged mode requires the kernel network modules support.
                // It doesn't relate the vendor HAL, set if overlay enables it.
                supportedFeatureSet |= WifiManager.WIFI_FEATURE_STA_BRIDGED_AP;
            }
        }
        supportedFeatureSet |= mWifiThreadRunner.call(
                () -> {
                    long concurrencyFeatureSet = 0L;
                    if (mActiveModeWarden.isStaApConcurrencySupported()) {
                        concurrencyFeatureSet |= WifiManager.WIFI_FEATURE_AP_STA;
                    }
                    if (mActiveModeWarden.isStaStaConcurrencySupportedForLocalOnlyConnections()) {
                        concurrencyFeatureSet |= WifiManager.WIFI_FEATURE_ADDITIONAL_STA_LOCAL_ONLY;
                    }
                    if (mActiveModeWarden.isStaStaConcurrencySupportedForMbb()) {
                        concurrencyFeatureSet |= WifiManager.WIFI_FEATURE_ADDITIONAL_STA_MBB;
                    }
                    if (mActiveModeWarden.isStaStaConcurrencySupportedForRestrictedConnections()) {
                        concurrencyFeatureSet |= WifiManager.WIFI_FEATURE_ADDITIONAL_STA_RESTRICTED;
                    }
                    return concurrencyFeatureSet;
                }, 0L);
        return supportedFeatureSet;
    }

    private static boolean hasAutomotiveFeature(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
    }

    /**
     * See
     * {@link WifiManager#registerNetworkRequestMatchCallback(
     * Executor, WifiManager.NetworkRequestMatchCallback)}
     *
     * @param callback Network Request Match callback to register
     *
     * @throws SecurityException if the caller does not have permission to register a callback
     * @throws RemoteException if remote exception happens
     * @throws IllegalArgumentException if the arguments are null or invalid
     */
    @Override
    public void registerNetworkRequestMatchCallback(INetworkRequestMatchCallback callback) {
        // verify arguments
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }
        enforceNetworkSettingsPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("registerNetworkRequestMatchCallback uid=%")
                    .c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() ->
                mWifiInjector.getWifiNetworkFactory().addCallback(callback));
    }

    /**
     * see {@link android.net.wifi.WifiManager#unregisterNetworkRequestMatchCallback(
     * WifiManager.NetworkRequestMatchCallback)}
     *
     * @param callback Network Request Match callback to unregister
     *
     * @throws SecurityException if the caller does not have permission to register a callback
     */
    @Override
    public void unregisterNetworkRequestMatchCallback(INetworkRequestMatchCallback callback) {
        enforceNetworkSettingsPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("unregisterNetworkRequestMatchCallback uid=%")
                    .c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() ->
                mWifiInjector.getWifiNetworkFactory().removeCallback(callback));
    }

    /**
     * See {@link android.net.wifi.WifiManager#addNetworkSuggestions(List)}
     *
     * @param networkSuggestions List of network suggestions to be added.
     * @param callingPackageName Package Name of the app adding the suggestions.
     * @param callingFeatureId Feature in the calling package
     * @throws SecurityException if the caller does not have permission.
     * @return One of status codes from {@link WifiManager.NetworkSuggestionsStatusCode}.
     */
    @Override
    public int addNetworkSuggestions(
            List<WifiNetworkSuggestion> networkSuggestions, String callingPackageName,
            String callingFeatureId) {
        if (enforceChangePermission(callingPackageName) != MODE_ALLOWED) {
            return WifiManager.STATUS_NETWORK_SUGGESTIONS_ERROR_APP_DISALLOWED;
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("addNetworkSuggestions uid=%").c(Binder.getCallingUid()).flush();
        }
        int callingUid = Binder.getCallingUid();

        int success = mWifiThreadRunner.call(() -> mWifiNetworkSuggestionsManager.add(
                networkSuggestions, callingUid, callingPackageName, callingFeatureId),
                WifiManager.STATUS_NETWORK_SUGGESTIONS_ERROR_INTERNAL);
        if (success != WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS) {
            Log.e(TAG, "Failed to add network suggestions");
        }
        return success;
    }

    /**
     * See {@link android.net.wifi.WifiManager#removeNetworkSuggestions(List)}
     *
     * @param networkSuggestions List of network suggestions to be removed.
     * @param callingPackageName Package Name of the app removing the suggestions.
     * @throws SecurityException if the caller does not have permission.
     * @return One of status codes from {@link WifiManager.NetworkSuggestionsStatusCode}.
     */
    @Override
    public int removeNetworkSuggestions(
            List<WifiNetworkSuggestion> networkSuggestions, String callingPackageName) {
        if (enforceChangePermission(callingPackageName) != MODE_ALLOWED) {
            return WifiManager.STATUS_NETWORK_SUGGESTIONS_ERROR_APP_DISALLOWED;
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("removeNetworkSuggestions uid=%").c(Binder.getCallingUid()).flush();
        }
        int callingUid = Binder.getCallingUid();

        int success = mWifiThreadRunner.call(() -> mWifiNetworkSuggestionsManager.remove(
                networkSuggestions, callingUid, callingPackageName),
                WifiManager.STATUS_NETWORK_SUGGESTIONS_ERROR_INTERNAL);
        if (success != WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS) {
            Log.e(TAG, "Failed to remove network suggestions");
        }
        return success;
    }

    /**
     * See {@link android.net.wifi.WifiManager#getNetworkSuggestions()}
     * @param callingPackageName Package Name of the app getting the suggestions.
     * @return a list of network suggestions suggested by this app
     */
    @Override
    public List<WifiNetworkSuggestion> getNetworkSuggestions(String callingPackageName) {
        int callingUid = Binder.getCallingUid();
        mAppOps.checkPackage(callingUid, callingPackageName);
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getNetworkSuggestionList uid=%").c(Binder.getCallingUid()).flush();
        }
        return mWifiThreadRunner.call(() ->
                mWifiNetworkSuggestionsManager.get(callingPackageName, callingUid),
                Collections.emptyList());
    }

    /**
     * Gets the factory Wi-Fi MAC addresses.
     * @throws SecurityException if the caller does not have permission.
     * @return Array of String representing Wi-Fi MAC addresses, or empty array if failed.
     */
    @Override
    public String[] getFactoryMacAddresses() {
        final int uid = Binder.getCallingUid();
        if (!mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
            throw new SecurityException("App not allowed to get Wi-Fi factory MAC address "
                    + "(uid = " + uid + ")");
        }
        String result = mWifiThreadRunner.call(
                () -> mActiveModeWarden.getPrimaryClientModeManager().getFactoryMacAddress(),
                null);
        // result can be empty array if either: WifiThreadRunner.call() timed out, or
        // ClientModeImpl.getFactoryMacAddress() returned null.
        // In this particular instance, we don't differentiate the two types of nulls.
        if (result == null) {
            return new String[0];
        }
        return new String[]{result};
    }

    /**
     * Sets the current device mobility state.
     * @param state the new device mobility state
     */
    @Override
    public void setDeviceMobilityState(@DeviceMobilityState int state) {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.WIFI_SET_DEVICE_MOBILITY_STATE, "WifiService");

        if (isVerboseLoggingEnabled()) {
            mLog.info("setDeviceMobilityState uid=% state=%")
                    .c(Binder.getCallingUid())
                    .c(state)
                    .flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> {
            mWifiConnectivityManager.setDeviceMobilityState(state);
            mWifiHealthMonitor.setDeviceMobilityState(state);
            mWifiDataStall.setDeviceMobilityState(state);
        });
    }

    /**
     * Proxy for the final native call of the parent class. Enables mocking of
     * the function.
     */
    public int getMockableCallingUid() {
        return getCallingUid();
    }

    /**
     * Start DPP in Configurator-Initiator role. The current device will initiate DPP bootstrapping
     * with a peer, and send the SSID and password of the selected network.
     *
     * @param binder Caller's binder context
     * @param packageName Package name of the calling app
     * @param enrolleeUri URI of the Enrollee obtained externally (e.g. QR code scanning)
     * @param selectedNetworkId Selected network ID to be sent to the peer
     * @param netRole The network role of the enrollee
     * @param callback Callback for status updates
     */
    @Override
    public void startDppAsConfiguratorInitiator(IBinder binder, @NonNull String packageName,
            String enrolleeUri, int selectedNetworkId, int netRole, IDppCallback callback) {
        // verify arguments
        if (binder == null) {
            throw new IllegalArgumentException("Binder must not be null");
        }
        if (TextUtils.isEmpty(enrolleeUri)) {
            throw new IllegalArgumentException("Enrollee URI must not be null or empty");
        }
        if (selectedNetworkId < 0) {
            throw new IllegalArgumentException("Selected network ID invalid");
        }
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }

        final int uid = getMockableCallingUid();

        int callingUid = Binder.getCallingUid();
        mAppOps.checkPackage(callingUid, packageName);
        if (!isSettingsOrSuw(Binder.getCallingPid(), callingUid)) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        // Stop MBB (if in progress) when DPP is initiated. Otherwise, DPP operation will fail
        // when the previous primary iface is removed after MBB completion.
        mWifiThreadRunner.post(() ->
                mMakeBeforeBreakManager.stopAllSecondaryTransientClientModeManagers(() ->
                        mDppManager.startDppAsConfiguratorInitiator(
                                uid, packageName,
                                mActiveModeWarden.getPrimaryClientModeManager().getInterfaceName(),
                                binder, enrolleeUri,
                                removeSecurityTypeFromNetworkId(selectedNetworkId), netRole,
                                callback)));
    }

    /**
     * Start DPP in Enrollee-Initiator role. The current device will initiate DPP bootstrapping
     * with a peer, and receive the SSID and password from the peer configurator.
     *
     * @param binder Caller's binder context
     * @param configuratorUri URI of the Configurator obtained externally (e.g. QR code scanning)
     * @param callback Callback for status updates
     */
    @Override
    public void startDppAsEnrolleeInitiator(IBinder binder, String configuratorUri,
            IDppCallback callback) {
        // verify arguments
        if (binder == null) {
            throw new IllegalArgumentException("Binder must not be null");
        }
        if (TextUtils.isEmpty(configuratorUri)) {
            throw new IllegalArgumentException("Enrollee URI must not be null or empty");
        }
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }

        final int uid = getMockableCallingUid();

        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }

        // Stop MBB (if in progress) when DPP is initiated. Otherwise, DPP operation will fail
        // when the previous primary iface is removed after MBB completion.
        mWifiThreadRunner.post(() ->
                mMakeBeforeBreakManager.stopAllSecondaryTransientClientModeManagers(() ->
                        mDppManager.startDppAsEnrolleeInitiator(uid,
                                mActiveModeWarden.getPrimaryClientModeManager().getInterfaceName(),
                                binder, configuratorUri, callback)));
    }

    /**
     * Start DPP in Enrollee-Responder role. The current device will generate the
     * bootstrap code and wait for the peer device to start the DPP authentication process.
     *
     * @param binder Caller's binder context
     * @param deviceInfo Device specific info to display in QR code(e.g. Easy_connect_demo)
     * @param curve Elliptic curve cryptography type used to generate DPP public/private key pair.
     * @param callback Callback for status updates
     */
    @Override
    @RequiresApi(Build.VERSION_CODES.S)
    public void startDppAsEnrolleeResponder(IBinder binder, @Nullable String deviceInfo,
            @WifiManager.EasyConnectCryptographyCurve int curve, IDppCallback callback) {
        if (!SdkLevel.isAtLeastS()) {
            throw new UnsupportedOperationException();
        }
        // verify arguments
        if (binder == null) {
            throw new IllegalArgumentException("Binder must not be null");
        }
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null");
        }

        final int uid = getMockableCallingUid();

        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }

        if (deviceInfo != null) {
            int deviceInfoLen = deviceInfo.length();
            if (deviceInfoLen > WifiManager.getEasyConnectMaxAllowedResponderDeviceInfoLength()) {
                throw new IllegalArgumentException("Device info length: " + deviceInfoLen
                        + " must be less than "
                        + WifiManager.getEasyConnectMaxAllowedResponderDeviceInfoLength());
            }
            char c;
            for (int i = 0; i < deviceInfoLen; i++) {
                c = deviceInfo.charAt(i);
                if (c < '!' || c > '~' || c == ';') {
                    throw new IllegalArgumentException("Allowed Range of ASCII characters in"
                            + "deviceInfo - %x20-7E; semicolon and space are not allowed!"
                            + "Found c: " + c);
                }
            }
        }

        // Stop MBB (if in progress) when DPP is initiated. Otherwise, DPP operation will fail
        // when the previous primary iface is removed after MBB completion.
        mWifiThreadRunner.post(() ->
                mMakeBeforeBreakManager.stopAllSecondaryTransientClientModeManagers(() ->
                        mDppManager.startDppAsEnrolleeResponder(uid,
                                mActiveModeWarden.getPrimaryClientModeManager().getInterfaceName(),
                                binder, deviceInfo, curve, callback)));
    }

    /**
     * Stop or abort a current DPP session.
     */
    @Override
    public void stopDppSession() throws RemoteException {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        final int uid = getMockableCallingUid();

        mWifiThreadRunner.post(() -> mDppManager.stopDppSession(uid));
    }

    /**
     * see {@link android.net.wifi.WifiManager#addWifiVerboseLoggingStatusChangedListener(Executor,
     * WifiManager.WifiVerboseLoggingStatusChangedListener)}
     *
     * @param listener IWifiVerboseLoggingStatusChangedListener listener to add
     *
     * @throws SecurityException if the caller does not have permission to add a listener.
     * @throws IllegalArgumentException if the argument is null.
     */
    @Override
    public void addWifiVerboseLoggingStatusChangedListener(
            IWifiVerboseLoggingStatusChangedListener listener) {
        if (listener == null) {
            throw new IllegalArgumentException("Listener must not be null");
        }
        enforceAccessPermission();
        // Post operation to handler thread
        mWifiThreadRunner.post(() ->
                mRegisteredWifiLoggingStatusListeners.register(listener));
    }

    /**
     * see {@link android.net.wifi.WifiManager#unregisterWifiVerboseLoggingStatusCallback
     * (WifiManager.WifiVerboseLoggingStatusCallback)}
     *
     * @param listener the listener to be removed.
     *
     * @throws SecurityException if the caller does not have permission to add a listener.
     * @throws IllegalArgumentException if the argument is null.
     */
    @Override
    public void removeWifiVerboseLoggingStatusChangedListener(
            IWifiVerboseLoggingStatusChangedListener listener) {
        if (listener == null) {
            throw new IllegalArgumentException("Listener must not be null");
        }
        enforceAccessPermission();
        // Post operation to handler thread
        mWifiThreadRunner.post(() ->
                mRegisteredWifiLoggingStatusListeners.unregister(listener));
    }

    /**
     * see {@link android.net.wifi.WifiManager#addOnWifiUsabilityStatsListener(Executor,
     * WifiManager.OnWifiUsabilityStatsListener)}
     *
     * @param listener WifiUsabilityStatsEntry listener to add
     *
     * @throws SecurityException if the caller does not have permission to add a listener
     * @throws RemoteException if remote exception happens
     * @throws IllegalArgumentException if the arguments are null or invalid
     */
    @Override
    public void addOnWifiUsabilityStatsListener(IOnWifiUsabilityStatsListener listener) {
        if (listener == null) {
            throw new IllegalArgumentException("Listener must not be null");
        }
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE, "WifiService");
        if (isVerboseLoggingEnabled()) {
            mLog.info("addOnWifiUsabilityStatsListener uid=%")
                .c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() ->
                mWifiMetrics.addOnWifiUsabilityListener(listener));
    }

    /**
     * see {@link android.net.wifi.WifiManager#removeOnWifiUsabilityStatsListener
     * (WifiManager.OnWifiUsabilityStatsListener)}
     *
     * @param listener listener to be removed.
     *
     * @throws SecurityException if the caller does not have permission to add a listener
     */
    @Override
    public void removeOnWifiUsabilityStatsListener(IOnWifiUsabilityStatsListener listener) {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE, "WifiService");
        if (isVerboseLoggingEnabled()) {
            mLog.info("removeOnWifiUsabilityStatsListener uid=%")
                    .c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() ->
                mWifiMetrics.removeOnWifiUsabilityListener(listener));
    }

    /**
     * Updates the Wi-Fi usability score.
     * @param seqNum Sequence number of the Wi-Fi usability score.
     * @param score The Wi-Fi usability score.
     * @param predictionHorizonSec Prediction horizon of the Wi-Fi usability score in second.
     */
    @Override
    public void updateWifiUsabilityScore(int seqNum, int score, int predictionHorizonSec) {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE, "WifiService");

        if (isVerboseLoggingEnabled()) {
            mLog.info("updateWifiUsabilityScore uid=% seqNum=% score=% predictionHorizonSec=%")
                    .c(Binder.getCallingUid())
                    .c(seqNum)
                    .c(score)
                    .c(predictionHorizonSec)
                    .flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> {
            String ifaceName = mActiveModeWarden.getPrimaryClientModeManager().getInterfaceName();
            mWifiMetrics.incrementWifiUsabilityScoreCount(
                    ifaceName, seqNum, score, predictionHorizonSec);
        });
    }

    /**
     * Notify interested parties if a wifi config has been changed.
     *
     * @param wifiCredentialEventType WIFI_CREDENTIAL_SAVED or WIFI_CREDENTIAL_FORGOT
     * @param config Must have a WifiConfiguration object to succeed
     */
    private void broadcastWifiCredentialChanged(int wifiCredentialEventType,
            WifiConfiguration config) {
        Intent intent = new Intent(WifiManager.WIFI_CREDENTIAL_CHANGED_ACTION);
        if (config != null && config.SSID != null && mWifiPermissionsUtil.isLocationModeEnabled()) {
            intent.putExtra(WifiManager.EXTRA_WIFI_CREDENTIAL_SSID, config.SSID);
        }
        intent.putExtra(WifiManager.EXTRA_WIFI_CREDENTIAL_EVENT_TYPE,
                wifiCredentialEventType);
        mContext.createContextAsUser(UserHandle.CURRENT, 0)
                .sendBroadcastWithMultiplePermissions(
                        intent,
                        new String[]{
                                android.Manifest.permission.RECEIVE_WIFI_CREDENTIAL_CHANGE,
                                android.Manifest.permission.ACCESS_FINE_LOCATION,
                        });
    }

    /**
     * Connects to a network.
     *
     * If the supplied config is not null, then the netId argument will be ignored and the config
     * will be saved (or updated if its networkId or profile key already exist) and connected to.
     *
     * If the supplied config is null, then the netId argument will be matched to a saved config to
     * be connected to.
     *
     * @param config New or existing config to add/update and connect to
     * @param netId Network ID of existing config to connect to if the supplied config is null
     * @param callback Listener to notify action result
     *
     * see: {@link WifiManager#connect(WifiConfiguration, WifiManager.ActionListener)}
     *      {@link WifiManager#connect(int, WifiManager.ActionListener)}
     */
    @Override
    public void connect(WifiConfiguration config, int netId, @Nullable IActionListener callback) {
        int uid = Binder.getCallingUid();
        if (!isPrivileged(Binder.getCallingPid(), uid)) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (config != null) {
            config.networkId = removeSecurityTypeFromNetworkId(config.networkId);
        }
        final int netIdArg = removeSecurityTypeFromNetworkId(netId);
        mLog.info("connect uid=%").c(uid).flush();
        mWifiThreadRunner.post(() -> {
            ActionListenerWrapper wrapper = new ActionListenerWrapper(callback);
            final NetworkUpdateResult result;
            // if connecting using WifiConfiguration, save the network first
            if (config != null) {
                if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
                    mWifiMetrics.logUserActionEvent(
                            UserActionEvent.EVENT_ADD_OR_UPDATE_NETWORK, config.networkId);
                }
                result = mWifiConfigManager.addOrUpdateNetwork(config, uid);
                if (!result.isSuccess()) {
                    Log.e(TAG, "connect adding/updating config=" + config + " failed");
                    wrapper.sendFailure(WifiManager.ERROR);
                    return;
                }
                broadcastWifiCredentialChanged(WifiManager.WIFI_CREDENTIAL_SAVED, config);
            } else {
                if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
                    mWifiMetrics.logUserActionEvent(
                            UserActionEvent.EVENT_MANUAL_CONNECT, netIdArg);
                }
                result = new NetworkUpdateResult(netIdArg);
            }
            WifiConfiguration configuration = mWifiConfigManager
                    .getConfiguredNetwork(result.getNetworkId());
            if (configuration == null) {
                Log.e(TAG, "connect to Invalid network Id=" + netIdArg);
                wrapper.sendFailure(WifiManager.ERROR);
                return;
            }
            if (configuration.enterpriseConfig != null
                    && configuration.enterpriseConfig.isAuthenticationSimBased()) {
                int subId = mWifiCarrierInfoManager.getBestMatchSubscriptionId(configuration);
                if (!mWifiCarrierInfoManager.isSimReady(subId)) {
                    Log.e(TAG, "connect to SIM-based config=" + configuration
                            + "while SIM is absent");
                    wrapper.sendFailure(WifiManager.ERROR);
                    return;
                }
                if (mWifiCarrierInfoManager.requiresImsiEncryption(subId)
                        && !mWifiCarrierInfoManager.isImsiEncryptionInfoAvailable(subId)) {
                    Log.e(TAG, "Imsi protection required but not available for Network="
                            + configuration);
                    wrapper.sendFailure(WifiManager.ERROR);
                    return;
                }
            }
            mMakeBeforeBreakManager.stopAllSecondaryTransientClientModeManagers(() ->
                    mConnectHelper.connectToNetwork(result, wrapper, uid));
        });
    }

    /**
     * see {@link android.net.wifi.WifiManager#save(WifiConfiguration,
     * WifiManager.ActionListener)}
     */
    @Override
    public void save(WifiConfiguration config, @Nullable IActionListener callback) {
        int uid = Binder.getCallingUid();
        if (!isPrivileged(Binder.getCallingPid(), uid)) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (config != null) {
            config.networkId = removeSecurityTypeFromNetworkId(config.networkId);
        }
        mLog.info("save uid=%").c(uid).flush();
        mWifiThreadRunner.post(() -> {
            ActionListenerWrapper wrapper = new ActionListenerWrapper(callback);
            NetworkUpdateResult result =
                    mWifiConfigManager.updateBeforeSaveNetwork(config, uid);
            if (result.isSuccess()) {
                broadcastWifiCredentialChanged(WifiManager.WIFI_CREDENTIAL_SAVED, config);
                mMakeBeforeBreakManager.stopAllSecondaryTransientClientModeManagers(() ->
                        mActiveModeWarden.getPrimaryClientModeManager()
                                .saveNetwork(result, wrapper, uid));
                if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
                    mWifiMetrics.logUserActionEvent(
                            UserActionEvent.EVENT_ADD_OR_UPDATE_NETWORK, config.networkId);
                }
            } else {
                wrapper.sendFailure(WifiManager.ERROR);
            }
        });
    }

    /**
     * see {@link android.net.wifi.WifiManager#forget(int, WifiManager.ActionListener)}
     */
    @Override
    public void forget(int netId, @Nullable IActionListener callback) {
        int uid = Binder.getCallingUid();
        if (!isPrivileged(Binder.getCallingPid(), uid)) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        final int internalNetId = removeSecurityTypeFromNetworkId(netId);
        mLog.info("forget uid=%").c(Binder.getCallingUid()).flush();
        if (mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)) {
            // It's important to log this metric before the actual forget executes because
            // the netId becomes invalid after the forget operation.
            mWifiMetrics.logUserActionEvent(UserActionEvent.EVENT_FORGET_WIFI, internalNetId);
        }
        mWifiThreadRunner.post(() -> {
            WifiConfiguration config = mWifiConfigManager.getConfiguredNetwork(internalNetId);
            boolean success = mWifiConfigManager.removeNetwork(internalNetId, uid, null);
            ActionListenerWrapper wrapper = new ActionListenerWrapper(callback);
            if (success) {
                wrapper.sendSuccess();
                broadcastWifiCredentialChanged(WifiManager.WIFI_CREDENTIAL_FORGOT, config);
            } else {
                Log.e(TAG, "Failed to remove network");
                wrapper.sendFailure(WifiManager.ERROR);
            }
        });
    }

    /**
     * See {@link WifiManager#registerScanResultsCallback(WifiManager.ScanResultsCallback)}
     */
    public void registerScanResultsCallback(@NonNull IScanResultsCallback callback) {
        if (callback == null) {
            throw new IllegalArgumentException("callback must not be null");
        }
        enforceAccessPermission();

        if (isVerboseLoggingEnabled()) {
            mLog.info("registerScanResultsCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        mWifiThreadRunner.post(() -> {
            if (!mWifiInjector.getScanRequestProxy().registerScanResultsCallback(callback)) {
                Log.e(TAG, "registerScanResultsCallback: Failed to register callback");
            }
        });
    }

    /**
     * See {@link WifiManager#registerScanResultsCallback(WifiManager.ScanResultsCallback)}
     */
    public void unregisterScanResultsCallback(@NonNull IScanResultsCallback callback) {
        if (isVerboseLoggingEnabled()) {
            mLog.info("unregisterScanResultCallback uid=%").c(Binder.getCallingUid()).flush();
        }
        enforceAccessPermission();
        // post operation to handler thread
        mWifiThreadRunner.post(() -> mWifiInjector.getScanRequestProxy()
                        .unregisterScanResultsCallback(callback));

    }

    /**
     * See {@link WifiManager#addSuggestionConnectionStatusListener(Executor,
     * SuggestionConnectionStatusListener)}
     */
    public void registerSuggestionConnectionStatusListener(
            @NonNull ISuggestionConnectionStatusListener listener, String packageName,
            @Nullable String featureId) {
        if (listener == null) {
            throw new IllegalArgumentException("listener must not be null");
        }
        final int uid = Binder.getCallingUid();
        enforceAccessPermission();
        enforceLocationPermission(packageName, featureId, uid);
        if (isVerboseLoggingEnabled()) {
            mLog.info("registerSuggestionConnectionStatusListener uid=%").c(uid).flush();
        }
        mWifiThreadRunner.post(() ->
                mWifiNetworkSuggestionsManager
                        .registerSuggestionConnectionStatusListener(listener, packageName, uid));
    }

    /**
     * See {@link WifiManager#removeSuggestionConnectionStatusListener(
     * SuggestionConnectionStatusListener)}
     */
    public void unregisterSuggestionConnectionStatusListener(
            @NonNull ISuggestionConnectionStatusListener listener, String packageName) {
        enforceAccessPermission();
        int uid = Binder.getCallingUid();
        if (isVerboseLoggingEnabled()) {
            mLog.info("unregisterSuggestionConnectionStatusListener uid=%")
                    .c(uid).flush();
        }
        mWifiThreadRunner.post(() ->
                mWifiNetworkSuggestionsManager
                        .unregisterSuggestionConnectionStatusListener(listener, packageName, uid));
    }

    @Override
    public int calculateSignalLevel(int rssi) {
        return RssiUtil.calculateSignalLevel(mContext, rssi);
    }

    /**
     * See {@link android.net.wifi.WifiManager#setWifiConnectedNetworkScorer(Executor,
     * WifiManager.WifiConnectedNetworkScorer)}
     *
     * @param binder IBinder instance to allow cleanup if the app dies.
     * @param scorer Wifi connected network scorer to set.
     * @return true Scorer is set successfully.
     *
     * @throws RemoteException if remote exception happens
     * @throws IllegalArgumentException if the arguments are null or invalid
     */
    @Override
    public boolean setWifiConnectedNetworkScorer(IBinder binder,
            IWifiConnectedNetworkScorer scorer) {
        if (binder == null) {
            throw new IllegalArgumentException("Binder must not be null");
        }
        if (scorer == null) {
            throw new IllegalArgumentException("Scorer must not be null");
        }
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE, "WifiService");
        if (isVerboseLoggingEnabled()) {
            mLog.info("setWifiConnectedNetworkScorer uid=%").c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        return mWifiThreadRunner.call(
                () -> mActiveModeWarden.setWifiConnectedNetworkScorer(binder, scorer), false);
    }

    /**
     * See {@link WifiManager#clearWifiConnectedNetworkScorer()}
     */
    @Override
    public void clearWifiConnectedNetworkScorer() {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.WIFI_UPDATE_USABILITY_STATS_SCORE, "WifiService");
        if (isVerboseLoggingEnabled()) {
            mLog.info("clearWifiConnectedNetworkScorer uid=%").c(Binder.getCallingUid()).flush();
        }
        // Post operation to handler thread
        mWifiThreadRunner.post(() -> mActiveModeWarden.clearWifiConnectedNetworkScorer());
    }

    /**
     * See {@link android.net.wifi.WifiManager#setScanThrottleEnabled(boolean)}
     */
    @Override
    public void setScanThrottleEnabled(boolean enable) {
        enforceNetworkSettingsPermission();
        mLog.info("setScanThrottleEnabled uid=% verbose=%")
                .c(Binder.getCallingUid())
                .c(enable).flush();
        mWifiThreadRunner.post(()-> mScanRequestProxy.setScanThrottleEnabled(enable));
    }

    /**
     * See {@link android.net.wifi.WifiManager#isScanThrottleEnabled()}
     */
    @Override
    public boolean isScanThrottleEnabled() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("isScanThrottleEnabled uid=%").c(Binder.getCallingUid()).flush();
        }
        return mWifiThreadRunner.call(()-> mScanRequestProxy.isScanThrottleEnabled(), true);
    }

    /**
     * See {@link android.net.wifi.WifiManager#setAutoWakeupEnabled(boolean)}
     */
    @Override
    public void setAutoWakeupEnabled(boolean enable) {
        enforceNetworkSettingsPermission();
        mLog.info("setWalkeupEnabled uid=% verbose=%")
                .c(Binder.getCallingUid())
                .c(enable).flush();
        mWifiThreadRunner.post(()-> mWifiInjector.getWakeupController().setEnabled(enable));
    }

    /**
     * See {@link android.net.wifi.WifiManager#isAutoWakeupEnabled()}
     */
    @Override
    public boolean isAutoWakeupEnabled() {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("isAutoWakeupEnabled uid=%").c(Binder.getCallingUid()).flush();
        }
        return mWifiThreadRunner.call(()-> mWifiInjector.getWakeupController().isEnabled(), false);
    }

    /**
     * See {@link android.net.wifi.WifiManager#setCarrierNetworkOffloadEnabled(int, boolean, boolean)}
     */
    @Override
    public void setCarrierNetworkOffloadEnabled(int subscriptionId, boolean merged,
            boolean enabled) {
        if (!isSettingsOrSuw(Binder.getCallingPid(), Binder.getCallingUid())) {
            throw new SecurityException(TAG + ": Permission denied");
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("setCarrierNetworkOffloadEnabled uid=%").c(Binder.getCallingUid()).flush();
        }
        mWifiThreadRunner.post(() ->
                mWifiCarrierInfoManager.setCarrierNetworkOffloadEnabled(subscriptionId, merged, enabled));
    }

    /**
     * See {@link android.net.wifi.WifiManager#isCarrierNetworkOffloadEnabled(int, boolean)}
     */
    @Override
    public boolean isCarrierNetworkOffloadEnabled(int subId, boolean merged) {
        enforceAccessPermission();
        if (isVerboseLoggingEnabled()) {
            mLog.info("isCarrierNetworkOffload uid=%").c(Binder.getCallingUid()).flush();
        }

        return mWifiThreadRunner.call(()->
                mWifiCarrierInfoManager.isCarrierNetworkOffloadEnabled(subId, merged), true);
    }

    /**
     * See {@link android.net.wifi.WifiManager#addSuggestionUserApprovalStatusListener(Executor,
     * WifiManager.SuggestionUserApprovalStatusListener)}
     */
    @Override
    public void addSuggestionUserApprovalStatusListener(
            ISuggestionUserApprovalStatusListener listener, String packageName) {
        if (listener == null) {
            throw new NullPointerException("listener must not be null");
        }
        final int uid = Binder.getCallingUid();
        enforceAccessPermission();
        long callingIdentity = Binder.clearCallingIdentity();
        try {
            if (!mWifiPermissionsUtil.doesUidBelongToCurrentUser(uid)) {
                Log.e(TAG, "UID " + uid + " not visible to the current user");
                throw new SecurityException("UID " + uid + " not visible to the current user");
            }
        } finally {
            // restore calling identity
            Binder.restoreCallingIdentity(callingIdentity);
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("addSuggestionUserApprovalStatusListener uid=%").c(uid).flush();
        }
        mWifiThreadRunner.post(() -> mWifiNetworkSuggestionsManager
                .addSuggestionUserApprovalStatusListener(listener, packageName, uid));
    }

    /**
     * See {@link android.net.wifi.WifiManager#removeSuggestionUserApprovalStatusListener(
     * WifiManager.SuggestionUserApprovalStatusListener)}
     */
    @Override
    public void removeSuggestionUserApprovalStatusListener(
            ISuggestionUserApprovalStatusListener listener, String packageName) {
        enforceAccessPermission();
        int uid = Binder.getCallingUid();
        long callingIdentity = Binder.clearCallingIdentity();
        try {
            if (!mWifiPermissionsUtil.doesUidBelongToCurrentUser(uid)) {
                Log.e(TAG, "UID " + uid + " not visible to the current user");
                throw new SecurityException("UID " + uid + " not visible to the current user");
            }
        } finally {
            // restore calling identity
            Binder.restoreCallingIdentity(callingIdentity);
        }
        if (isVerboseLoggingEnabled()) {
            mLog.info("removeSuggestionUserApprovalStatusListener uid=%")
                    .c(uid).flush();
        }
        mWifiThreadRunner.post(() ->
                mWifiNetworkSuggestionsManager
                        .removeSuggestionUserApprovalStatusListener(listener, packageName, uid));
    }

    /**
     * See {@link android.net.wifi.WifiManager#setEmergencyScanRequestInProgress(boolean)}.
     */
    @Override
    public void setEmergencyScanRequestInProgress(boolean inProgress) {
        enforceNetworkStackPermission();
        int uid = Binder.getCallingUid();
        mLog.info("setEmergencyScanRequestInProgress uid=%").c(uid).flush();
        mActiveModeWarden.setEmergencyScanRequestInProgress(inProgress);
    }

    /**
     * See {@link android.net.wifi.WifiManager#removeAppState(int, String)}.
     */
    @Override
    public void removeAppState(int targetAppUid, @NonNull String targetAppPackageName) {
        enforceNetworkSettingsPermission();
        mLog.info("removeAppState uid=%").c(Binder.getCallingUid()).flush();

        mWifiThreadRunner.post(() -> {
            removeAppStateInternal(targetAppUid, targetAppPackageName);
        });
    }

    /**
     * See {@link android.net.wifi.WifiManager#setWifiScoringEnabled(boolean)}.
     */
    @Override
    public boolean setWifiScoringEnabled(boolean enabled) {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.NETWORK_SETTINGS, "WifiService");
        // Post operation to handler thread
        return mWifiThreadRunner.call(
                () -> mSettingsStore.handleWifiScoringEnabled(enabled), false);
    }

    @VisibleForTesting
    static boolean isValidBandForGetUsableChannels(@WifiScanner.WifiBand int band) {
        switch (band) {
            case WifiScanner.WIFI_BAND_UNSPECIFIED:
            case WifiScanner.WIFI_BAND_24_GHZ:
            case WifiScanner.WIFI_BAND_5_GHZ_WITH_DFS:
            case WifiScanner.WIFI_BAND_BOTH_WITH_DFS:
            case WifiScanner.WIFI_BAND_6_GHZ:
            case WifiScanner.WIFI_BAND_24_5_WITH_DFS_6_GHZ:
            case WifiScanner.WIFI_BAND_60_GHZ:
            case WifiScanner.WIFI_BAND_24_5_WITH_DFS_6_60_GHZ:
                return true;
            default:
                return false;
        }
    }

    /**
     * See {@link android.net.wifi.WifiManager#getUsableChannels(int, int) and
     * See {@link android.net.wifi.WifiManager#getAllowedChannels(int, int).
     *
     * @throws SecurityException if the caller does not have permission
     * or IllegalArgumentException if the band is invalid for this method.
     */
    @Override
    public List<WifiAvailableChannel> getUsableChannels(@WifiScanner.WifiBand int band,
            @WifiAvailableChannel.OpMode int mode, @WifiAvailableChannel.Filter int filter) {
        // Location mode must be enabled
        if (!mWifiPermissionsUtil.isLocationModeEnabled()) {
            throw new SecurityException("Location mode is disabled for the device");
        }
        final int uid = Binder.getCallingUid();
        if (isVerboseLoggingEnabled()) {
            mLog.info("getUsableChannels uid=%").c(Binder.getCallingUid()).flush();
        }
        if (!mWifiPermissionsUtil.checkCallersHardwareLocationPermission(uid)) {
            throw new SecurityException("UID " + uid + " does not have location h/w permission");
        }
        if (!isValidBandForGetUsableChannels(band)) {
            throw new IllegalArgumentException("Unsupported band: " + band);
        }
        List<WifiAvailableChannel> channels = mWifiThreadRunner.call(
                () -> mWifiNative.getUsableChannels(band, mode, filter), null);
        if (channels == null) {
            throw new UnsupportedOperationException();
        }
        return channels;
    }

    private void resetNotificationManager() {
        mWifiInjector.getWifiNotificationManager().createNotificationChannels();
        mWifiInjector.getOpenNetworkNotifier().clearPendingNotification(false);
        mWifiCarrierInfoManager.resetNotification();
        mWifiNetworkSuggestionsManager.resetNotification();
        mWifiInjector.getWakeupController().resetNotification();
    }

    /**
     * See {@link android.net.wifi.WifiManager#flushPasspointAnqpCache()}.
     */
    @Override
    public void flushPasspointAnqpCache(@NonNull String packageName) {
        mWifiPermissionsUtil.checkPackage(Binder.getCallingUid(), packageName);

        if (!isDeviceOrProfileOwner(Binder.getCallingUid(), packageName)) {
            enforceAnyPermissionOf(android.Manifest.permission.NETWORK_SETTINGS,
                    android.Manifest.permission.NETWORK_MANAGED_PROVISIONING,
                    android.Manifest.permission.NETWORK_CARRIER_PROVISIONING);
        }
        mWifiThreadRunner.post(mPasspointManager::clearAnqpRequestsAndFlushCache);
    }
}
