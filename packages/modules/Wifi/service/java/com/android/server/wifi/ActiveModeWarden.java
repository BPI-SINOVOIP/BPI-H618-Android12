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

import static android.net.wifi.WifiManager.IFACE_IP_MODE_LOCAL_ONLY;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_TETHERED;

import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_LOCAL_ONLY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_PRIMARY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SCAN_ONLY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_LONG_LIVED;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_TRANSIENT;
import static com.android.server.wifi.ActiveModeManager.ROLE_SOFTAP_TETHERED;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.net.wifi.ISubsystemRestartCallback;
import android.net.wifi.IWifiConnectedNetworkScorer;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.BatteryStatsManager;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.WorkSource;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.util.Pair;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.IState;
import com.android.internal.util.Preconditions;
import com.android.internal.util.Protocol;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.server.wifi.ActiveModeManager.ClientConnectivityRole;
import com.android.server.wifi.ActiveModeManager.ClientInternetConnectivityRole;
import com.android.server.wifi.ActiveModeManager.ClientRole;
import com.android.server.wifi.ActiveModeManager.SoftApRole;
import com.android.server.wifi.util.WifiPermissionsUtil;
import com.android.wifi.resources.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * This class provides the implementation for different WiFi operating modes.
 */
public class ActiveModeWarden {
    private static final String TAG = "WifiActiveModeWarden";
    private static final String STATE_MACHINE_EXITED_STATE_NAME = "STATE_MACHINE_EXITED";
    public static final WorkSource INTERNAL_REQUESTOR_WS = new WorkSource(Process.WIFI_UID);

    // Holder for active mode managers
    private final Set<ConcreteClientModeManager> mClientModeManagers = new ArraySet<>();
    private final Set<SoftApManager> mSoftApManagers = new ArraySet<>();

    private final Set<ModeChangeCallback> mCallbacks = new ArraySet<>();
    private final Set<PrimaryClientModeManagerChangedCallback> mPrimaryChangedCallbacks =
            new ArraySet<>();
    // DefaultModeManager used to service API calls when there are no active client mode managers.
    private final DefaultClientModeManager mDefaultClientModeManager;
    private final WifiInjector mWifiInjector;
    private final Looper mLooper;
    private final Handler mHandler;
    private final Context mContext;
    private final WifiDiagnostics mWifiDiagnostics;
    private final WifiSettingsStore mSettingsStore;
    private final FrameworkFacade mFacade;
    private final WifiPermissionsUtil mWifiPermissionsUtil;
    private final BatteryStatsManager mBatteryStatsManager;
    private final ScanRequestProxy mScanRequestProxy;
    private final WifiNative mWifiNative;
    private final WifiController mWifiController;
    private final Graveyard mGraveyard;
    private final WifiMetrics mWifiMetrics;
    private final ExternalScoreUpdateObserverProxy mExternalScoreUpdateObserverProxy;
    private final DppManager mDppManager;

    private WifiServiceImpl.SoftApCallbackInternal mSoftApCallback;
    private WifiServiceImpl.SoftApCallbackInternal mLohsCallback;

    private final RemoteCallbackList<ISubsystemRestartCallback> mRestartCallbacks =
            new RemoteCallbackList<>();

    private boolean mIsShuttingdown = false;
    private boolean mVerboseLoggingEnabled = false;
    /** Cache to store the external scorer for primary and secondary (MBB) client mode manager. */
    @Nullable private Pair<IBinder, IWifiConnectedNetworkScorer> mClientModeManagerScorer;

    @Nullable
    private ConcreteClientModeManager mLastPrimaryClientModeManager = null;

    @Nullable
    private WorkSource mLastPrimaryClientModeManagerRequestorWs = null;
    @Nullable
    private WorkSource mLastScanOnlyClientModeManagerRequestorWs = null;

    /**
     * Called from WifiServiceImpl to register a callback for notifications from SoftApManager
     */
    public void registerSoftApCallback(@NonNull WifiServiceImpl.SoftApCallbackInternal callback) {
        mSoftApCallback = callback;
    }

    /**
     * Called from WifiServiceImpl to register a callback for notifications from SoftApManager
     * for local-only hotspot.
     */
    public void registerLohsCallback(@NonNull WifiServiceImpl.SoftApCallbackInternal callback) {
        mLohsCallback = callback;
    }

    /**
     * Callbacks for indicating any mode manager changes to the rest of the system.
     */
    public interface ModeChangeCallback {
        /**
         * Invoked when new mode manager is added.
         *
         * @param activeModeManager Instance of {@link ActiveModeManager}.
         */
        void onActiveModeManagerAdded(@NonNull ActiveModeManager activeModeManager);

        /**
         * Invoked when a mode manager is removed.
         *
         * @param activeModeManager Instance of {@link ActiveModeManager}.
         */
        void onActiveModeManagerRemoved(@NonNull ActiveModeManager activeModeManager);

        /**
         * Invoked when an existing mode manager's role is changed.
         *
         * @param activeModeManager Instance of {@link ActiveModeManager}.
         */
        void onActiveModeManagerRoleChanged(@NonNull ActiveModeManager activeModeManager);
    }

    /** Called when the primary ClientModeManager changes. */
    public interface PrimaryClientModeManagerChangedCallback {
        /**
         * Note: The current implementation for changing primary CMM is not atomic (due to setRole()
         * needing to go through StateMachine, which is async). Thus, when the primary CMM changes,
         * the sequence of calls looks like this:
         * 1. onChange(prevPrimaryCmm, null)
         * 2. onChange(null, newPrimaryCmm)
         * Nevertheless, at run time, these two calls should occur in rapid succession.
         *
         * @param prevPrimaryClientModeManager the previous primary ClientModeManager, or null if
         *                                     there was no previous primary (e.g. Wifi was off).
         * @param newPrimaryClientModeManager the new primary ClientModeManager, or null if there is
         *                                    no longer a primary (e.g. Wifi was turned off).
         */
        void onChange(
                @Nullable ConcreteClientModeManager prevPrimaryClientModeManager,
                @Nullable ConcreteClientModeManager newPrimaryClientModeManager);
    }

    /**
     * Keep stopped {@link ActiveModeManager} instances so that they can be dumped to aid debugging.
     *
     * TODO(b/160283853): Find a smarter way to evict old ActiveModeManagers
     */
    private static class Graveyard {
        private static final int INSTANCES_TO_KEEP = 3;

        private final ArrayDeque<ConcreteClientModeManager> mClientModeManagers =
                new ArrayDeque<>();
        private final ArrayDeque<SoftApManager> mSoftApManagers = new ArrayDeque<>();

        /**
         * Add this stopped {@link ConcreteClientModeManager} to the graveyard, and evict the oldest
         * ClientModeManager if the graveyard is full.
         */
        void inter(ConcreteClientModeManager clientModeManager) {
            if (mClientModeManagers.size() == INSTANCES_TO_KEEP) {
                mClientModeManagers.removeFirst();
            }
            mClientModeManagers.addLast(clientModeManager);
        }

        /**
         * Add this stopped {@link SoftApManager} to the graveyard, and evict the oldest
         * SoftApManager if the graveyard is full.
         */
        void inter(SoftApManager softApManager) {
            if (mSoftApManagers.size() == INSTANCES_TO_KEEP) {
                mSoftApManagers.removeFirst();
            }
            mSoftApManagers.addLast(softApManager);
        }

        /** Dump the contents of the graveyard. */
        void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            pw.println("Dump of ActiveModeWarden.Graveyard");
            pw.println("Stopped ClientModeManagers: " + mClientModeManagers.size() + " total");
            for (ConcreteClientModeManager clientModeManager : mClientModeManagers) {
                clientModeManager.dump(fd, pw, args);
            }
            pw.println("Stopped SoftApManagers: " + mSoftApManagers.size() + " total");
            for (SoftApManager softApManager : mSoftApManagers) {
                softApManager.dump(fd, pw, args);
            }
            pw.println();
        }
    }

    ActiveModeWarden(WifiInjector wifiInjector,
            Looper looper,
            WifiNative wifiNative,
            DefaultClientModeManager defaultClientModeManager,
            BatteryStatsManager batteryStatsManager,
            WifiDiagnostics wifiDiagnostics,
            Context context,
            WifiSettingsStore settingsStore,
            FrameworkFacade facade,
            WifiPermissionsUtil wifiPermissionsUtil,
            WifiMetrics wifiMetrics,
            ExternalScoreUpdateObserverProxy externalScoreUpdateObserverProxy,
            DppManager dppManager) {
        mWifiInjector = wifiInjector;
        mLooper = looper;
        mHandler = new Handler(looper);
        mContext = context;
        mWifiDiagnostics = wifiDiagnostics;
        mSettingsStore = settingsStore;
        mFacade = facade;
        mWifiPermissionsUtil = wifiPermissionsUtil;
        mDefaultClientModeManager = defaultClientModeManager;
        mBatteryStatsManager = batteryStatsManager;
        mScanRequestProxy = wifiInjector.getScanRequestProxy();
        mWifiNative = wifiNative;
        mWifiMetrics = wifiMetrics;
        mWifiController = new WifiController();
        mExternalScoreUpdateObserverProxy = externalScoreUpdateObserverProxy;
        mDppManager = dppManager;
        mGraveyard = new Graveyard();

        wifiNative.registerStatusListener(isReady -> {
            if (!isReady && !mIsShuttingdown) {
                mHandler.post(() -> {
                    Log.e(TAG, "One of the native daemons died. Triggering recovery");
                    wifiDiagnostics.triggerBugReportDataCapture(
                            WifiDiagnostics.REPORT_REASON_WIFINATIVE_FAILURE);

                    // immediately trigger SelfRecovery if we receive a notice about an
                    // underlying daemon failure
                    // Note: SelfRecovery has a circular dependency with ActiveModeWarden and is
                    // instantiated after ActiveModeWarden, so use WifiInjector to get the instance
                    // instead of directly passing in SelfRecovery in the constructor.
                    mWifiInjector.getSelfRecovery().trigger(SelfRecovery.REASON_WIFINATIVE_FAILURE);
                });
            }
        });

        registerPrimaryClientModeManagerChangedCallback(
                (prevPrimaryClientModeManager, newPrimaryClientModeManager) -> {
                    // TODO (b/181363901): We can always propagate the external scorer to all
                    // ClientModeImpl instances. WifiScoreReport already handles skipping external
                    // scorer notification for local only & restricted STA + STA use-cases. For MBB
                    // use-case, we may want the external scorer to be notified.
                    if (prevPrimaryClientModeManager != null) {
                        prevPrimaryClientModeManager.clearWifiConnectedNetworkScorer();
                    }
                    if (newPrimaryClientModeManager != null && mClientModeManagerScorer != null) {
                        newPrimaryClientModeManager.setWifiConnectedNetworkScorer(
                                mClientModeManagerScorer.first,
                                mClientModeManagerScorer.second);
                    }
                });
    }

    private void invokeOnAddedCallbacks(@NonNull ActiveModeManager activeModeManager) {
        if (mVerboseLoggingEnabled) {
            Log.v(TAG, "ModeManager added " + activeModeManager);
        }
        for (ModeChangeCallback callback : mCallbacks) {
            callback.onActiveModeManagerAdded(activeModeManager);
        }
    }

    private void invokeOnRemovedCallbacks(@NonNull ActiveModeManager activeModeManager) {
        if (mVerboseLoggingEnabled) {
            Log.v(TAG, "ModeManager removed " + activeModeManager);
        }
        for (ModeChangeCallback callback : mCallbacks) {
            callback.onActiveModeManagerRemoved(activeModeManager);
        }
    }

    private void invokeOnRoleChangedCallbacks(@NonNull ActiveModeManager activeModeManager) {
        if (mVerboseLoggingEnabled) {
            Log.v(TAG, "ModeManager role changed " + activeModeManager);
        }
        for (ModeChangeCallback callback : mCallbacks) {
            callback.onActiveModeManagerRoleChanged(activeModeManager);
        }
    }

    private void invokeOnPrimaryClientModeManagerChangedCallbacks(
            @Nullable ConcreteClientModeManager prevPrimaryClientModeManager,
            @Nullable ConcreteClientModeManager newPrimaryClientModeManager) {
        if (mVerboseLoggingEnabled) {
            Log.v(TAG, "Primary ClientModeManager changed from " + prevPrimaryClientModeManager
                    + " to " + newPrimaryClientModeManager);
        }
        for (PrimaryClientModeManagerChangedCallback callback : mPrimaryChangedCallbacks) {
            callback.onChange(prevPrimaryClientModeManager, newPrimaryClientModeManager);
        }
    }

    /**
     * Enable verbose logging.
     */
    public void enableVerboseLogging(boolean verbose) {
        mVerboseLoggingEnabled = verbose;
        for (ActiveModeManager modeManager : getActiveModeManagers()) {
            modeManager.enableVerboseLogging(verbose);
        }
    }

    /**
     * See {@link android.net.wifi.WifiManager#setWifiConnectedNetworkScorer(Executor,
     * WifiManager.WifiConnectedNetworkScorer)}
     */
    public boolean setWifiConnectedNetworkScorer(IBinder binder,
            IWifiConnectedNetworkScorer scorer) {
        try {
            scorer.onSetScoreUpdateObserver(mExternalScoreUpdateObserverProxy);
        } catch (RemoteException e) {
            Log.e(TAG, "Unable to set score update observer " + scorer, e);
            return false;
        }
        mClientModeManagerScorer = Pair.create(binder, scorer);
        return getPrimaryClientModeManager().setWifiConnectedNetworkScorer(binder, scorer);
    }

    /**
     * See {@link WifiManager#clearWifiConnectedNetworkScorer()}
     */
    public void clearWifiConnectedNetworkScorer() {
        mClientModeManagerScorer = null;
        getPrimaryClientModeManager().clearWifiConnectedNetworkScorer();
    }

    /**
     * Register for mode change callbacks.
     */
    public void registerModeChangeCallback(@NonNull ModeChangeCallback callback) {
        mCallbacks.add(Objects.requireNonNull(callback));
    }

    /**
     * Unregister mode change callback.
     */
    public void unregisterModeChangeCallback(@NonNull ModeChangeCallback callback) {
        mCallbacks.remove(Objects.requireNonNull(callback));
    }

    /** Register for primary ClientModeManager changed callbacks. */
    public void registerPrimaryClientModeManagerChangedCallback(
            @NonNull PrimaryClientModeManagerChangedCallback callback) {
        mPrimaryChangedCallbacks.add(Objects.requireNonNull(callback));
        // If there is already a primary CMM when registering, send a callback with the info.
        ConcreteClientModeManager cm = getPrimaryClientModeManagerNullable();
        if (cm != null) callback.onChange(null, cm);
    }

    /** Unregister for primary ClientModeManager changed callbacks. */
    public void unregisterPrimaryClientModeManagerChangedCallback(
            @NonNull PrimaryClientModeManagerChangedCallback callback) {
        mPrimaryChangedCallbacks.remove(Objects.requireNonNull(callback));
    }

    /**
     * Notify that device is shutting down
     * Keep it simple and don't add collection access codes
     * to avoid concurrentModificationException when it is directly called from a different thread
     */
    public void notifyShuttingDown() {
        mIsShuttingdown = true;
    }

    /**
     * @return Returns whether we can create more client mode managers or not.
     */
    public boolean canRequestMoreClientModeManagersInRole(@NonNull WorkSource requestorWs,
            @NonNull ClientRole clientRole) {
        if (!mWifiNative.isItPossibleToCreateStaIface(requestorWs)) {
            return false;
        }
        if (clientRole == ROLE_CLIENT_LOCAL_ONLY) {
            if (!mContext.getResources().getBoolean(
                    R.bool.config_wifiMultiStaLocalOnlyConcurrencyEnabled)) {
                return false;
            }
            final int uid = requestorWs.getUid(0);
            final String packageName = requestorWs.getPackageName(0);
            // For peer to peer use-case, only allow secondary STA if the app is targeting S SDK
            // or is a system app to provide backward compatibility.
            return mWifiPermissionsUtil.isSystem(packageName, uid)
                    || !mWifiPermissionsUtil.isTargetSdkLessThan(
                            packageName, Build.VERSION_CODES.S, uid);
        }
        if (clientRole == ROLE_CLIENT_SECONDARY_TRANSIENT) {
            return mContext.getResources().getBoolean(
                    R.bool.config_wifiMultiStaNetworkSwitchingMakeBeforeBreakEnabled);
        }
        if (clientRole == ROLE_CLIENT_SECONDARY_LONG_LIVED) {
            return mContext.getResources().getBoolean(
                    R.bool.config_wifiMultiStaRestrictedConcurrencyEnabled);
        }
        Log.e(TAG, "Unrecognized role=" + clientRole);
        return false;
    }

    /**
     * @return Returns whether we can create more SoftAp managers or not.
     */
    public boolean canRequestMoreSoftApManagers(@NonNull WorkSource requestorWs) {
        return mWifiNative.isItPossibleToCreateApIface(requestorWs);
    }

    /**
     * @return Returns whether the device can support at least one concurrent client mode manager &
     * softap manager.
     */
    public boolean isStaApConcurrencySupported() {
        return mWifiNative.isStaApConcurrencySupported();
    }

    /**
     * @return Returns whether the device can support at least two concurrent client mode managers
     * and the local only use-case is enabled.
     */
    public boolean isStaStaConcurrencySupportedForLocalOnlyConnections() {
        return mWifiNative.isStaStaConcurrencySupported()
                && mContext.getResources().getBoolean(
                        R.bool.config_wifiMultiStaLocalOnlyConcurrencyEnabled);
    }

    /**
     * @return Returns whether the device can support at least two concurrent client mode managers
     * and the mbb wifi switching is enabled.
     */
    public boolean isStaStaConcurrencySupportedForMbb() {
        return mWifiNative.isStaStaConcurrencySupported()
                && mContext.getResources().getBoolean(
                        R.bool.config_wifiMultiStaNetworkSwitchingMakeBeforeBreakEnabled);
    }

    /**
     * @return Returns whether the device can support at least two concurrent client mode managers
     * and the restricted use-case is enabled.
     */
    public boolean isStaStaConcurrencySupportedForRestrictedConnections() {
        return mWifiNative.isStaStaConcurrencySupported()
                && mContext.getResources().getBoolean(
                        R.bool.config_wifiMultiStaRestrictedConcurrencyEnabled);
    }

    /** Begin listening to broadcasts and start the internal state machine. */
    public void start() {
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                // Location mode has been toggled...  trigger with the scan change
                // update to make sure we are in the correct mode
                scanAlwaysModeChanged();
            }
        }, new IntentFilter(LocationManager.MODE_CHANGED_ACTION));
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (mSettingsStore.handleAirplaneModeToggled()) {
                    airplaneModeToggled();
                }
            }
        }, new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED));
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                boolean emergencyMode =
                        intent.getBooleanExtra(TelephonyManager.EXTRA_PHONE_IN_ECM_STATE, false);
                emergencyCallbackModeChanged(emergencyMode);
            }
        }, new IntentFilter(TelephonyManager.ACTION_EMERGENCY_CALLBACK_MODE_CHANGED));
        boolean trackEmergencyCallState = mContext.getResources().getBoolean(
                R.bool.config_wifi_turn_off_during_emergency_call);
        if (trackEmergencyCallState) {
            mContext.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    boolean inCall = intent.getBooleanExtra(
                            TelephonyManager.EXTRA_PHONE_IN_EMERGENCY_CALL, false);
                    emergencyCallStateChanged(inCall);
                }
            }, new IntentFilter(TelephonyManager.ACTION_EMERGENCY_CALL_STATE_CHANGED));
        }

        mWifiController.start();
    }

    /** Disable Wifi for recovery purposes. */
    public void recoveryDisableWifi() {
        mWifiController.sendMessage(WifiController.CMD_RECOVERY_DISABLE_WIFI);
    }

    /**
     * Restart Wifi for recovery purposes.
     * @param reason One of {@link SelfRecovery.RecoveryReason}
     */
    public void recoveryRestartWifi(@SelfRecovery.RecoveryReason int reason,
            @Nullable String reasonDetail, boolean requestBugReport) {
        mWifiController.sendMessage(WifiController.CMD_RECOVERY_RESTART_WIFI, reason,
                requestBugReport ? 1 : 0, reasonDetail);
    }

    /**
     * register a callback to monitor the progress of Wi-Fi subsystem operation (started/finished)
     * - started via {@link #recoveryRestartWifi(int, String, boolean)}.
     */
    public boolean registerSubsystemRestartCallback(ISubsystemRestartCallback callback) {
        return mRestartCallbacks.register(callback);
    }

    /**
     * unregister a callback to monitor the progress of Wi-Fi subsystem operation (started/finished)
     * - started via {@link #recoveryRestartWifi(int, String, boolean)}. Callback is registered via
     * {@link #registerSubsystemRestartCallback(ISubsystemRestartCallback)}.
     */
    public boolean unregisterSubsystemRestartCallback(ISubsystemRestartCallback callback) {
        return mRestartCallbacks.unregister(callback);
    }

    /** Wifi has been toggled. */
    public void wifiToggled(WorkSource requestorWs) {
        mWifiController.sendMessage(WifiController.CMD_WIFI_TOGGLED, requestorWs);
    }

    /** Airplane Mode has been toggled. */
    public void airplaneModeToggled() {
        mWifiController.sendMessage(WifiController.CMD_AIRPLANE_TOGGLED);
    }

    /** Starts SoftAp. */
    public void startSoftAp(SoftApModeConfiguration softApConfig, WorkSource requestorWs) {
        mWifiController.sendMessage(WifiController.CMD_SET_AP, 1, 0,
                Pair.create(softApConfig, requestorWs));
    }

    /** Stop SoftAp. */
    public void stopSoftAp(int mode) {
        mWifiController.sendMessage(WifiController.CMD_SET_AP, 0, mode);
    }

    /** Update SoftAp Capability. */
    public void updateSoftApCapability(SoftApCapability capability) {
        mWifiController.sendMessage(WifiController.CMD_UPDATE_AP_CAPABILITY, capability);
    }

    /** Update SoftAp Configuration. */
    public void updateSoftApConfiguration(SoftApConfiguration config) {
        mWifiController.sendMessage(WifiController.CMD_UPDATE_AP_CONFIG, config);
    }

    /** Emergency Callback Mode has changed. */
    public void emergencyCallbackModeChanged(boolean isInEmergencyCallbackMode) {
        mWifiController.sendMessage(
                WifiController.CMD_EMERGENCY_MODE_CHANGED, isInEmergencyCallbackMode ? 1 : 0);
    }

    /** Emergency Call state has changed. */
    public void emergencyCallStateChanged(boolean isInEmergencyCall) {
        mWifiController.sendMessage(
                WifiController.CMD_EMERGENCY_CALL_STATE_CHANGED, isInEmergencyCall ? 1 : 0);
    }

    /** Scan always mode has changed. */
    public void scanAlwaysModeChanged() {
        mWifiController.sendMessage(
                WifiController.CMD_SCAN_ALWAYS_MODE_CHANGED,
                // Scan only mode change is not considered a direct user interaction since user
                // is not explicitly turning on wifi scanning (side-effect of location toggle).
                // So, use the lowest priority internal requestor worksource to ensure that this
                // is treated with the lowest priority.
                INTERNAL_REQUESTOR_WS);
    }

    /** emergency scan progress indication. */
    public void setEmergencyScanRequestInProgress(boolean inProgress) {
        mWifiController.sendMessage(
                WifiController.CMD_EMERGENCY_SCAN_STATE_CHANGED,
                inProgress ? 1 : 0, 0,
                // Emergency scans should have the highest priority, so use settings worksource.
                mFacade.getSettingsWorkSource(mContext));
    }

    /**
     * Listener to request a ModeManager instance for a particular operation.
     */
    public interface ExternalClientModeManagerRequestListener {
        /**
         * Returns an instance of ClientModeManager or null if the request failed (when wifi is
         * off).
         */
        void onAnswer(@Nullable ClientModeManager modeManager);
    }

    private static class AdditionalClientModeManagerRequestInfo {
        @NonNull public final ExternalClientModeManagerRequestListener listener;
        @NonNull public final WorkSource requestorWs;
        @NonNull public final ClientConnectivityRole clientRole;
        @NonNull public final String ssid;
        @Nullable public final String bssid;

        AdditionalClientModeManagerRequestInfo(
                @NonNull  ExternalClientModeManagerRequestListener listener,
                @NonNull WorkSource requestorWs,
                @NonNull ClientConnectivityRole clientRole,
                @NonNull String ssid,
                // For some use-cases, bssid is selected by firmware.
                @Nullable String bssid) {
            this.listener = listener;
            this.requestorWs = requestorWs;
            this.clientRole = clientRole;
            this.ssid = ssid;
            this.bssid = bssid;
        }
    }

    /**
     * Request a local only client manager.
     *
     * @param listener used to receive the requested ClientModeManager. Will receive:
     *                 1. null - if Wifi is toggled off
     *                 2. The primary ClientModeManager - if a new ClientModeManager cannot be
     *                    created.
     *                 3. The new ClientModeManager - if it was created successfully.
     * @param requestorWs the WorkSource for this request
     */
    public void requestLocalOnlyClientModeManager(
            @NonNull ExternalClientModeManagerRequestListener listener,
            @NonNull WorkSource requestorWs, @NonNull String ssid, @NonNull String bssid) {
        mWifiController.sendMessage(
                WifiController.CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER,
                new AdditionalClientModeManagerRequestInfo(
                        Objects.requireNonNull(listener), Objects.requireNonNull(requestorWs),
                        ROLE_CLIENT_LOCAL_ONLY, ssid, bssid));
    }

    /**
     * Request a secondary long lived client manager.
     *
     * @param listener used to receive the requested ClientModeManager. Will receive:
     *                 1. null - if Wifi is toggled off
     *                 2. The primary ClientModeManager - if a new ClientModeManager cannot be
     *                    created.
     *                 3. The new ClientModeManager - if it was created successfully.
     * @param requestorWs the WorkSource for this request
     */
    public void requestSecondaryLongLivedClientModeManager(
            @NonNull ExternalClientModeManagerRequestListener listener,
            @NonNull WorkSource requestorWs, @NonNull String ssid, @Nullable String bssid) {
        mWifiController.sendMessage(
                WifiController.CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER,
                new AdditionalClientModeManagerRequestInfo(
                        Objects.requireNonNull(listener), Objects.requireNonNull(requestorWs),
                        ROLE_CLIENT_SECONDARY_LONG_LIVED, ssid, bssid));
    }

    /**
     * Request a secondary transient client manager.
     *
     * @param listener used to receive the requested ClientModeManager. Will receive:
     *                 1. null - if Wifi is toggled off.
     *                 2. An existing secondary transient ClientModeManager - if it already exists.
     *                 3. A new secondary transient ClientModeManager - if one doesn't exist and one
     *                    was created successfully.
     *                 4. The primary ClientModeManager - if a new ClientModeManager cannot be
     *                    created.
     * @param requestorWs the WorkSource for this request
     */
    public void requestSecondaryTransientClientModeManager(
            @NonNull ExternalClientModeManagerRequestListener listener,
            @NonNull WorkSource requestorWs, @NonNull String ssid, @Nullable String bssid) {
        mWifiController.sendMessage(
                WifiController.CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER,
                new AdditionalClientModeManagerRequestInfo(
                        Objects.requireNonNull(listener), Objects.requireNonNull(requestorWs),
                        ROLE_CLIENT_SECONDARY_TRANSIENT, ssid, bssid));
    }

    /**
     * Remove the provided client manager.
     */
    public void removeClientModeManager(ClientModeManager clientModeManager) {
        mWifiController.sendMessage(
                WifiController.CMD_REMOVE_ADDITIONAL_CLIENT_MODE_MANAGER, clientModeManager);
    }

    /**
     * Check whether we have a primary client mode manager (indicates wifi toggle on).
     */
    public boolean hasPrimaryClientModeManager() {
        return getClientModeManagerInRole(ROLE_CLIENT_PRIMARY) != null;
    }

    /**
     * Returns primary client mode manager if any, else returns null
     * This mode manager can be the default route on the device & will handle all external API
     * calls.
     * @return Instance of {@link ConcreteClientModeManager} or null.
     */
    @Nullable
    public ConcreteClientModeManager getPrimaryClientModeManagerNullable() {
        return getClientModeManagerInRole(ROLE_CLIENT_PRIMARY);
    }

    /**
     * Returns primary client mode manager if any, else returns an instance of
     * {@link ClientModeManager}.
     * This mode manager can be the default route on the device & will handle all external API
     * calls.
     * @return Instance of {@link ClientModeManager}.
     */
    @NonNull
    public ClientModeManager getPrimaryClientModeManager() {
        ClientModeManager cm = getPrimaryClientModeManagerNullable();
        if (cm != null) return cm;
        // If there is no primary client manager, return the default one.
        return mDefaultClientModeManager;
    }

    /**
     * Returns all instances of ClientModeManager in
     * {@link ActiveModeManager.ClientInternetConnectivityRole} roles.
     * @return List of {@link ClientModeManager}.
     */
    @NonNull
    public List<ClientModeManager> getInternetConnectivityClientModeManagers() {
        List<ClientModeManager> modeManagers = new ArrayList<>();
        for (ConcreteClientModeManager manager : mClientModeManagers) {
            if (manager.getRole() instanceof ClientInternetConnectivityRole) {
                modeManagers.add(manager);
            }
        }
        return modeManagers;
    }

    /** Stop all secondary transient ClientModeManagers. */
    public void stopAllClientModeManagersInRole(ClientRole role) {
        // there should only be at most one Make Before Break CMM, but check all of them to be safe.
        for (ConcreteClientModeManager manager : mClientModeManagers) {
            if (manager.getRole() == role) {
                stopAdditionalClientModeManager(manager);
            }
        }
    }

    @NonNull
    public List<ClientModeManager> getClientModeManagers() {
        return new ArrayList<>(mClientModeManagers);
    }

    /**
     * Returns scan only client mode manager, if any.
     * This mode manager will only allow scanning.
     * @return Instance of {@link ClientModeManager} or null if none present.
     */
    @Nullable
    public ClientModeManager getScanOnlyClientModeManager() {
        return getClientModeManagerInRole(ROLE_CLIENT_SCAN_ONLY);
    }

    /**
     * Returns tethered softap manager, if any.
     * @return Instance of {@link SoftApManager} or null if none present.
     */
    @Nullable
    public SoftApManager getTetheredSoftApManager() {
        return getSoftApManagerInRole(ROLE_SOFTAP_TETHERED);
    }

    /**
     * Returns LOHS softap manager, if any.
     * @return Instance of {@link SoftApManager} or null if none present.
     */
    @Nullable
    public SoftApManager getLocalOnlySoftApManager() {
        return getSoftApManagerInRole(ActiveModeManager.ROLE_SOFTAP_LOCAL_ONLY);
    }

    private boolean hasAnyModeManager() {
        return !mClientModeManagers.isEmpty() || !mSoftApManagers.isEmpty();
    }

    private boolean hasAnyClientModeManager() {
        return !mClientModeManagers.isEmpty();
    }

    private boolean hasAnyClientModeManagerInConnectivityRole() {
        for (ConcreteClientModeManager manager : mClientModeManagers) {
            if (manager.getRole() instanceof ClientConnectivityRole) return true;
        }
        return false;
    }

    private boolean hasAnySoftApManager() {
        return !mSoftApManagers.isEmpty();
    }

    /**
     * @return true if all the client mode managers are in scan only role,
     * false if there are no client mode managers present or if any of them are not in scan only
     * role.
     */
    private boolean areAllClientModeManagersInScanOnlyRole() {
        if (mClientModeManagers.isEmpty()) return false;
        for (ConcreteClientModeManager manager : mClientModeManagers) {
            if (manager.getRole() != ROLE_CLIENT_SCAN_ONLY) return false;
        }
        return true;
    }

    /** Get any client mode manager in the given role, or null if none was found. */
    @Nullable
    public ConcreteClientModeManager getClientModeManagerInRole(ClientRole role) {
        for (ConcreteClientModeManager manager : mClientModeManagers) {
            if (manager.getRole() == role) return manager;
        }
        return null;
    }

    /** Get all client mode managers in the specified roles. */
    @NonNull
    public List<ConcreteClientModeManager> getClientModeManagersInRoles(ClientRole... roles) {
        Set<ClientRole> rolesSet = Set.of(roles);
        List<ConcreteClientModeManager> result = new ArrayList<>();
        for (ConcreteClientModeManager manager : mClientModeManagers) {
            ClientRole role = manager.getRole();
            if (role != null && rolesSet.contains(role)) {
                result.add(manager);
            }
        }
        return result;
    }

    @Nullable
    private SoftApManager getSoftApManagerInRole(SoftApRole role) {
        for (SoftApManager manager : mSoftApManagers) {
            if (manager.getRole() == role) return manager;
        }
        return null;
    }

    private SoftApRole getRoleForSoftApIpMode(int ipMode) {
        return ipMode == IFACE_IP_MODE_TETHERED
                ? ROLE_SOFTAP_TETHERED
                : ActiveModeManager.ROLE_SOFTAP_LOCAL_ONLY;
    }

    /**
     * Method to enable soft ap for wifi hotspot.
     *
     * The supplied SoftApModeConfiguration includes the target softap WifiConfiguration (or null if
     * the persisted config is to be used) and the target operating mode (ex,
     * {@link WifiManager#IFACE_IP_MODE_TETHERED} {@link WifiManager#IFACE_IP_MODE_LOCAL_ONLY}).
     *
     * @param softApConfig SoftApModeConfiguration for the hostapd softap
     */
    private void startSoftApModeManager(
            @NonNull SoftApModeConfiguration softApConfig, @NonNull WorkSource requestorWs) {
        Log.d(TAG, "Starting SoftApModeManager config = " + softApConfig.getSoftApConfiguration());
        Preconditions.checkState(softApConfig.getTargetMode() == IFACE_IP_MODE_LOCAL_ONLY
                || softApConfig.getTargetMode() == IFACE_IP_MODE_TETHERED);

        WifiServiceImpl.SoftApCallbackInternal callback =
                softApConfig.getTargetMode() == IFACE_IP_MODE_LOCAL_ONLY
                        ? mLohsCallback : mSoftApCallback;
        SoftApManager manager = mWifiInjector.makeSoftApManager(
                new SoftApListener(), callback, softApConfig, requestorWs,
                getRoleForSoftApIpMode(softApConfig.getTargetMode()), mVerboseLoggingEnabled);
        mSoftApManagers.add(manager);
    }

    /**
     * Method to stop all soft ap for the specified mode.
     *
     * This method will stop any active softAp mode managers.
     *
     * @param ipMode the operating mode of APs to bring down (ex,
     *             {@link WifiManager#IFACE_IP_MODE_TETHERED} or
     *             {@link WifiManager#IFACE_IP_MODE_LOCAL_ONLY}).
     *             Use {@link WifiManager#IFACE_IP_MODE_UNSPECIFIED} to stop all APs.
     */
    private void stopSoftApModeManagers(int ipMode) {
        Log.d(TAG, "Shutting down all softap mode managers in mode " + ipMode);
        for (SoftApManager softApManager : mSoftApManagers) {
            if (ipMode == WifiManager.IFACE_IP_MODE_UNSPECIFIED
                    || getRoleForSoftApIpMode(ipMode) == softApManager.getRole()) {
                softApManager.stop();
            }
        }
    }

    private void updateCapabilityToSoftApModeManager(SoftApCapability capability) {
        for (SoftApManager softApManager : mSoftApManagers) {
            softApManager.updateCapability(capability);
        }
    }

    private void updateConfigurationToSoftApModeManager(SoftApConfiguration config) {
        for (SoftApManager softApManager : mSoftApManagers) {
            softApManager.updateConfiguration(config);
        }
    }

    /**
     * Method to enable a new primary client mode manager in scan only mode.
     */
    private boolean startScanOnlyClientModeManager(WorkSource requestorWs) {
        Log.d(TAG, "Starting primary ClientModeManager in scan only mode");
        ConcreteClientModeManager manager = mWifiInjector.makeClientModeManager(
                new ClientListener(), requestorWs, ROLE_CLIENT_SCAN_ONLY, mVerboseLoggingEnabled);
        mClientModeManagers.add(manager);
        mLastScanOnlyClientModeManagerRequestorWs = requestorWs;
        return true;
    }

    /**
     * Method to enable a new primary client mode manager in connect mode.
     */
    private boolean startPrimaryClientModeManager(WorkSource requestorWs) {
        Log.d(TAG, "Starting primary ClientModeManager in connect mode");
        ConcreteClientModeManager manager = mWifiInjector.makeClientModeManager(
                new ClientListener(), requestorWs, ROLE_CLIENT_PRIMARY, mVerboseLoggingEnabled);
        mClientModeManagers.add(manager);
        mLastPrimaryClientModeManagerRequestorWs = requestorWs;
        return true;
    }

    /**
     * Method to enable a new primary client mode manager.
     */
    private boolean startPrimaryOrScanOnlyClientModeManager(WorkSource requestorWs) {
        ActiveModeManager.ClientRole role = getRoleForPrimaryOrScanOnlyClientModeManager();
        if (role == ROLE_CLIENT_PRIMARY) {
            return startPrimaryClientModeManager(requestorWs);
        } else if (role == ROLE_CLIENT_SCAN_ONLY) {
            return startScanOnlyClientModeManager(requestorWs);
        } else {
            return false;
        }
    }

    /**
     * Method to stop all client mode mangers.
     */
    private void stopAllClientModeManagers() {
        Log.d(TAG, "Shutting down all client mode managers");
        for (ConcreteClientModeManager clientModeManager : mClientModeManagers) {
            clientModeManager.stop();
        }
    }

    /**
     * Method to switch all primary client mode manager mode of operation to ScanOnly mode.
     */
    private void switchAllPrimaryClientModeManagersToScanOnlyMode(@NonNull WorkSource requestorWs) {
        Log.d(TAG, "Switching all primary client mode managers to scan only mode");
        for (ConcreteClientModeManager clientModeManager : mClientModeManagers) {
            if (clientModeManager.getRole() != ROLE_CLIENT_PRIMARY) {
                continue;
            }
            clientModeManager.setRole(ROLE_CLIENT_SCAN_ONLY, requestorWs);
        }
    }

    /**
     * Method to switch all client mode manager mode of operation (from ScanOnly To Connect &
     * vice-versa) based on the toggle state.
     */
    private boolean switchAllPrimaryOrScanOnlyClientModeManagers() {
        Log.d(TAG, "Switching all client mode managers");
        for (ConcreteClientModeManager clientModeManager : mClientModeManagers) {
            if (clientModeManager.getRole() != ROLE_CLIENT_PRIMARY
                    && clientModeManager.getRole() != ROLE_CLIENT_SCAN_ONLY) {
                continue;
            }
            if (!switchPrimaryOrScanOnlyClientModeManagerRole(clientModeManager)) {
                return false;
            }
        }
        return true;
    }

    private ActiveModeManager.ClientRole getRoleForPrimaryOrScanOnlyClientModeManager() {
        if (mSettingsStore.isWifiToggleEnabled()) {
            return ROLE_CLIENT_PRIMARY;
        } else if (mWifiController.shouldEnableScanOnlyMode()) {
            return ROLE_CLIENT_SCAN_ONLY;
        } else {
            Log.e(TAG, "Something is wrong, no client mode toggles enabled");
            return null;
        }
    }

    /**
     * Method to switch a client mode manager mode of operation (from ScanOnly To Connect &
     * vice-versa) based on the toggle state.
     */
    private boolean switchPrimaryOrScanOnlyClientModeManagerRole(
            @NonNull ConcreteClientModeManager modeManager) {
        ActiveModeManager.ClientRole role = getRoleForPrimaryOrScanOnlyClientModeManager();
        final WorkSource lastRequestorWs;
        if (role == ROLE_CLIENT_PRIMARY) {
            lastRequestorWs = mLastPrimaryClientModeManagerRequestorWs;
        } else if (role == ROLE_CLIENT_SCAN_ONLY) {
            lastRequestorWs = mLastScanOnlyClientModeManagerRequestorWs;
        } else {
            return false;
        }
        modeManager.setRole(role, lastRequestorWs);
        return true;
    }

    /**
     * Method to start a new client mode manager.
     */
    private boolean startAdditionalClientModeManager(
            ClientConnectivityRole role,
            @NonNull ExternalClientModeManagerRequestListener externalRequestListener,
            @NonNull WorkSource requestorWs) {
        Log.d(TAG, "Starting additional ClientModeManager in role: " + role);
        ClientListener listener = new ClientListener(externalRequestListener);
        ConcreteClientModeManager manager = mWifiInjector.makeClientModeManager(
                listener, requestorWs, role, mVerboseLoggingEnabled);
        mClientModeManagers.add(manager);
        return true;
    }

    /**
     * Method to switch role for an existing non-primary client mode manager.
     */
    private boolean switchRoleForAdditionalClientModeManager(
            @NonNull ConcreteClientModeManager manager,
            @NonNull ClientConnectivityRole role,
            @NonNull ExternalClientModeManagerRequestListener externalRequestListener,
            @NonNull WorkSource requestorWs) {
        Log.d(TAG, "Switching role for additional ClientModeManager to role: " + role);
        ClientListener listener = new ClientListener(externalRequestListener);
        manager.setRole(role, requestorWs, listener);
        return true;
    }

    /**
     * Method to stop client mode manger.
     */
    private void stopAdditionalClientModeManager(ClientModeManager clientModeManager) {
        if (clientModeManager.getRole() == ROLE_CLIENT_PRIMARY
                || clientModeManager.getRole() == ROLE_CLIENT_SCAN_ONLY) return;
        Log.d(TAG, "Shutting down additional client mode manager: " + clientModeManager);
        clientModeManager.stop();
    }

    /**
     * Method to stop all active modes, for example, when toggling airplane mode.
     */
    private void shutdownWifi() {
        Log.d(TAG, "Shutting down all mode managers");
        for (ActiveModeManager manager : getActiveModeManagers()) {
            manager.stop();
        }
    }

    /**
     * Dump current state for active mode managers.
     *
     * Must be called from the main Wifi thread.
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of " + TAG);
        pw.println("Current wifi mode: " + getCurrentMode());
        pw.println("NumActiveModeManagers: " + getActiveModeManagerCount());
        mWifiController.dump(fd, pw, args);
        for (ActiveModeManager manager : getActiveModeManagers()) {
            manager.dump(fd, pw, args);
        }
        mGraveyard.dump(fd, pw, args);
        boolean isStaStaConcurrencySupported = mWifiNative.isStaStaConcurrencySupported();
        pw.println("STA + STA Concurrency Supported: " + isStaStaConcurrencySupported);
        if (isStaStaConcurrencySupported) {
            pw.println("   MBB use-case enabled: "
                    + mContext.getResources().getBoolean(
                            R.bool.config_wifiMultiStaNetworkSwitchingMakeBeforeBreakEnabled));
            pw.println("   Local only use-case enabled: "
                    + mContext.getResources().getBoolean(
                            R.bool.config_wifiMultiStaLocalOnlyConcurrencyEnabled));
            pw.println("   Restricted use-case enabled: "
                    + mContext.getResources().getBoolean(
                            R.bool.config_wifiMultiStaRestrictedConcurrencyEnabled));
        }
        pw.println("STA + AP Concurrency Supported: " + isStaApConcurrencySupported());
        mWifiInjector.getHalDeviceManager().dump(fd, pw, args);
    }

    @VisibleForTesting
    String getCurrentMode() {
        IState state = mWifiController.getCurrentState();
        return state == null ? STATE_MACHINE_EXITED_STATE_NAME : state.getName();
    }

    @VisibleForTesting
    Collection<ActiveModeManager> getActiveModeManagers() {
        ArrayList<ActiveModeManager> activeModeManagers = new ArrayList<>();
        activeModeManagers.addAll(mClientModeManagers);
        activeModeManagers.addAll(mSoftApManagers);
        return activeModeManagers;
    }

    private int getActiveModeManagerCount() {
        return mSoftApManagers.size() + mClientModeManagers.size();
    }

    @VisibleForTesting
    boolean isInEmergencyMode() {
        IState state = mWifiController.getCurrentState();
        return ((WifiController.BaseState) state).isInEmergencyMode();
    }

    private void updateBatteryStats() {
        updateBatteryStatsWifiState(hasAnyModeManager());
        if (areAllClientModeManagersInScanOnlyRole()) {
            updateBatteryStatsScanModeActive();
        }
    }

    private class SoftApListener implements ActiveModeManager.Listener<SoftApManager> {
        @Override
        public void onStarted(SoftApManager softApManager) {
            updateBatteryStats();
            invokeOnAddedCallbacks(softApManager);
        }

        @Override
        public void onRoleChanged(SoftApManager softApManager) {
            Log.w(TAG, "Role switched received on SoftApManager unexpectedly");
        }

        @Override
        public void onStopped(SoftApManager softApManager) {
            mSoftApManagers.remove(softApManager);
            mGraveyard.inter(softApManager);
            updateBatteryStats();
            mWifiController.sendMessage(WifiController.CMD_AP_STOPPED);
            invokeOnRemovedCallbacks(softApManager);
        }

        @Override
        public void onStartFailure(SoftApManager softApManager) {
            mSoftApManagers.remove(softApManager);
            mGraveyard.inter(softApManager);
            updateBatteryStats();
            mWifiController.sendMessage(WifiController.CMD_AP_START_FAILURE);
            // onStartFailure can be called when switching between roles. So, remove
            // update listeners.
            Log.e(TAG, "SoftApManager start failed!" + softApManager);
            invokeOnRemovedCallbacks(softApManager);
        }
    }

    private class ClientListener implements ActiveModeManager.Listener<ConcreteClientModeManager> {
        @Nullable
        private ExternalClientModeManagerRequestListener mExternalRequestListener; // one shot

        ClientListener() {
            this(null);
        }

        ClientListener(
                @Nullable ExternalClientModeManagerRequestListener externalRequestListener) {
            mExternalRequestListener = externalRequestListener;
        }

        @WifiNative.MultiStaUseCase
        private int getMultiStatUseCase() {
            // Note: The use-case setting finds the first non-primary client mode manager to set
            // the use-case to HAL. This does not extend to 3 STA concurrency when there are
            // 2 secondary STA client mode managers.
            for (ClientModeManager cmm : getClientModeManagers()) {
                ClientRole clientRole = cmm.getRole();
                if (clientRole == ROLE_CLIENT_LOCAL_ONLY
                        || clientRole == ROLE_CLIENT_SECONDARY_LONG_LIVED) {
                    return WifiNative.DUAL_STA_NON_TRANSIENT_UNBIASED;
                } else if (clientRole == ROLE_CLIENT_SECONDARY_TRANSIENT) {
                    return WifiNative.DUAL_STA_TRANSIENT_PREFER_PRIMARY;
                }
            }
            // if single STA, a safe default is PREFER_PRIMARY
            return WifiNative.DUAL_STA_TRANSIENT_PREFER_PRIMARY;
        }

        /**
         * Hardware needs to be configured for STA + STA before sending the callbacks to clients
         * letting them know that CM is ready for use.
         */
        private void configureHwForMultiStaIfNecessary() {
            mWifiNative.setMultiStaUseCase(getMultiStatUseCase());
            String primaryIfaceName = getPrimaryClientModeManager().getInterfaceName();
            // if no primary exists (occurs briefly during Make Before Break), don't update the
            // primary and keep the previous primary. Only update WifiNative when the new primary is
            // activated.
            if (primaryIfaceName != null) {
                mWifiNative.setMultiStaPrimaryConnection(primaryIfaceName);
            }
        }

        private void onStartedOrRoleChanged(ConcreteClientModeManager clientModeManager) {
            updateClientScanMode();
            updateBatteryStats();
            configureHwForMultiStaIfNecessary();
            if (mExternalRequestListener != null) {
                mExternalRequestListener.onAnswer(clientModeManager);
                mExternalRequestListener = null; // reset after one shot.
            }

            // Report to SarManager
            reportWifiStateToSarManager();
        }

        private void reportWifiStateToSarManager() {
            if (areAllClientModeManagersInScanOnlyRole()) {
                // Inform sar manager that scan only is being enabled
                mWifiInjector.getSarManager().setScanOnlyWifiState(WifiManager.WIFI_STATE_ENABLED);
            } else {
                // Inform sar manager that scan only is being disabled
                mWifiInjector.getSarManager().setScanOnlyWifiState(WifiManager.WIFI_STATE_DISABLED);
            }
            if (hasAnyClientModeManagerInConnectivityRole()) {
                // Inform sar manager that wifi is Enabled
                mWifiInjector.getSarManager().setClientWifiState(WifiManager.WIFI_STATE_ENABLED);
            } else {
                // Inform sar manager that wifi is being disabled
                mWifiInjector.getSarManager().setClientWifiState(WifiManager.WIFI_STATE_DISABLED);
            }
        }

        private void onPrimaryChangedDueToStartedOrRoleChanged(
                ConcreteClientModeManager clientModeManager) {
            if (clientModeManager.getRole() != ROLE_CLIENT_PRIMARY
                    && clientModeManager == mLastPrimaryClientModeManager) {
                // CMM was primary, but is no longer primary
                invokeOnPrimaryClientModeManagerChangedCallbacks(clientModeManager, null);
                mLastPrimaryClientModeManager = null;
            } else if (clientModeManager.getRole() == ROLE_CLIENT_PRIMARY
                    && clientModeManager != mLastPrimaryClientModeManager) {
                // CMM is primary, but wasn't primary before
                invokeOnPrimaryClientModeManagerChangedCallbacks(
                        mLastPrimaryClientModeManager, clientModeManager);
                mLastPrimaryClientModeManager = clientModeManager;
            }
        }

        @Override
        public void onStarted(@NonNull ConcreteClientModeManager clientModeManager) {
            onStartedOrRoleChanged(clientModeManager);
            invokeOnAddedCallbacks(clientModeManager);
            // invoke "added" callbacks before primary changed
            onPrimaryChangedDueToStartedOrRoleChanged(clientModeManager);
        }

        @Override
        public void onRoleChanged(@NonNull ConcreteClientModeManager clientModeManager) {
            onStartedOrRoleChanged(clientModeManager);
            invokeOnRoleChangedCallbacks(clientModeManager);
            onPrimaryChangedDueToStartedOrRoleChanged(clientModeManager);
        }

        private void onStoppedOrStartFailure(ConcreteClientModeManager clientModeManager) {
            mClientModeManagers.remove(clientModeManager);
            mGraveyard.inter(clientModeManager);
            updateClientScanMode();
            updateBatteryStats();
            if (clientModeManager == mLastPrimaryClientModeManager) {
                // CMM was primary, but was stopped
                invokeOnPrimaryClientModeManagerChangedCallbacks(
                        mLastPrimaryClientModeManager, null);
                mLastPrimaryClientModeManager = null;
            }
            // invoke "removed" callbacks after primary changed
            invokeOnRemovedCallbacks(clientModeManager);

            // Report to SarManager
            reportWifiStateToSarManager();
        }

        @Override
        public void onStopped(@NonNull ConcreteClientModeManager clientModeManager) {
            onStoppedOrStartFailure(clientModeManager);
            mWifiController.sendMessage(WifiController.CMD_STA_STOPPED);
        }

        @Override
        public void onStartFailure(@NonNull ConcreteClientModeManager clientModeManager) {
            Log.e(TAG, "ClientModeManager start failed!" + clientModeManager);
            // onStartFailure can be called when switching between roles. So, remove
            // update listeners.
            onStoppedOrStartFailure(clientModeManager);
            mWifiController.sendMessage(WifiController.CMD_STA_START_FAILURE);
        }
    }

    // Update the scan state based on all active mode managers.
    private void updateClientScanMode() {
        boolean scanEnabled = hasAnyClientModeManager();
        boolean scanningForHiddenNetworksEnabled;

        if (mContext.getResources().getBoolean(R.bool.config_wifiScanHiddenNetworksScanOnlyMode)) {
            scanningForHiddenNetworksEnabled = hasAnyClientModeManager();
        } else {
            scanningForHiddenNetworksEnabled = hasAnyClientModeManagerInConnectivityRole();
        }
        mScanRequestProxy.enableScanning(scanEnabled, scanningForHiddenNetworksEnabled);
    }

    /**
     *  Helper method to report wifi state as on/off (doesn't matter which mode).
     *
     *  @param enabled boolean indicating that some mode has been turned on or off
     */
    private void updateBatteryStatsWifiState(boolean enabled) {
        if (enabled) {
            if (getActiveModeManagerCount() == 1) {
                // only report wifi on if we haven't already
                mBatteryStatsManager.reportWifiOn();
            }
        } else {
            if (getActiveModeManagerCount() == 0) {
                // only report if we don't have any active modes
                mBatteryStatsManager.reportWifiOff();
            }
        }
    }

    private void updateBatteryStatsScanModeActive() {
        mBatteryStatsManager.reportWifiState(BatteryStatsManager.WIFI_STATE_OFF_SCANNING, null);
    }

    /**
     * Called to pull metrics from ActiveModeWarden to WifiMetrics when a dump is triggered, as
     * opposed to the more common push metrics which are reported to WifiMetrics as soon as they
     * occur.
     */
    public void updateMetrics() {
        mWifiMetrics.setIsMakeBeforeBreakSupported(isStaStaConcurrencySupportedForMbb());
    }

    /**
     * WifiController is the class used to manage wifi state for various operating
     * modes (normal, airplane, wifi hotspot, etc.).
     */
    private class WifiController extends StateMachine {
        private static final String TAG = "WifiController";

        // Maximum limit to use for timeout delay if the value from overlay setting is too large.
        private static final int MAX_RECOVERY_TIMEOUT_DELAY_MS = 4000;

        private static final int BASE = Protocol.BASE_WIFI_CONTROLLER;

        static final int CMD_EMERGENCY_MODE_CHANGED                 = BASE + 1;
        static final int CMD_EMERGENCY_SCAN_STATE_CHANGED           = BASE + 2;
        static final int CMD_SCAN_ALWAYS_MODE_CHANGED               = BASE + 7;
        static final int CMD_WIFI_TOGGLED                           = BASE + 8;
        static final int CMD_AIRPLANE_TOGGLED                       = BASE + 9;
        static final int CMD_SET_AP                                 = BASE + 10;
        static final int CMD_EMERGENCY_CALL_STATE_CHANGED           = BASE + 14;
        static final int CMD_AP_STOPPED                             = BASE + 15;
        static final int CMD_STA_START_FAILURE                      = BASE + 16;
        // Command used to trigger a wifi stack restart when in active mode
        static final int CMD_RECOVERY_RESTART_WIFI                  = BASE + 17;
        // Internal command used to complete wifi stack restart
        private static final int CMD_RECOVERY_RESTART_WIFI_CONTINUE = BASE + 18;
        // Command to disable wifi when SelfRecovery is throttled or otherwise not doing full
        // recovery
        static final int CMD_RECOVERY_DISABLE_WIFI                   = BASE + 19;
        static final int CMD_STA_STOPPED                             = BASE + 20;
        static final int CMD_DEFERRED_RECOVERY_RESTART_WIFI          = BASE + 22;
        static final int CMD_AP_START_FAILURE                        = BASE + 23;
        static final int CMD_UPDATE_AP_CAPABILITY                    = BASE + 24;
        static final int CMD_UPDATE_AP_CONFIG                        = BASE + 25;
        static final int CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER  = BASE + 26;
        static final int CMD_REMOVE_ADDITIONAL_CLIENT_MODE_MANAGER   = BASE + 27;

        private final EnabledState mEnabledState = new EnabledState();
        private final DisabledState mDisabledState = new DisabledState();

        private boolean mIsInEmergencyCall = false;
        private boolean mIsInEmergencyCallbackMode = false;
        private boolean mIsEmergencyScanInProgress = false;

        WifiController() {
            super(TAG, mLooper);

            DefaultState defaultState = new DefaultState();
            addState(defaultState); {
                addState(mDisabledState, defaultState);
                addState(mEnabledState, defaultState);
            }

            setLogRecSize(100);
            setLogOnlyTransitions(false);

        }

        @Override
        protected String getWhatToString(int what) {
            switch (what) {
                case CMD_AIRPLANE_TOGGLED:
                    return "CMD_AIRPLANE_TOGGLED";
                case CMD_AP_START_FAILURE:
                    return "CMD_AP_START_FAILURE";
                case CMD_AP_STOPPED:
                    return "CMD_AP_STOPPED";
                case CMD_DEFERRED_RECOVERY_RESTART_WIFI:
                    return "CMD_DEFERRED_RECOVERY_RESTART_WIFI";
                case CMD_EMERGENCY_CALL_STATE_CHANGED:
                    return "CMD_EMERGENCY_CALL_STATE_CHANGED";
                case CMD_EMERGENCY_MODE_CHANGED:
                    return "CMD_EMERGENCY_MODE_CHANGED";
                case CMD_RECOVERY_DISABLE_WIFI:
                    return "CMD_RECOVERY_DISABLE_WIFI";
                case CMD_RECOVERY_RESTART_WIFI:
                    return "CMD_RECOVERY_RESTART_WIFI";
                case CMD_RECOVERY_RESTART_WIFI_CONTINUE:
                    return "CMD_RECOVERY_RESTART_WIFI_CONTINUE";
                case CMD_REMOVE_ADDITIONAL_CLIENT_MODE_MANAGER:
                    return "CMD_REMOVE_ADDITIONAL_CLIENT_MODE_MANAGER";
                case CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER:
                    return "CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER";
                case CMD_EMERGENCY_SCAN_STATE_CHANGED:
                    return "CMD_EMERGENCY_SCAN_STATE_CHANGED";
                case CMD_SCAN_ALWAYS_MODE_CHANGED:
                    return "CMD_SCAN_ALWAYS_MODE_CHANGED";
                case CMD_SET_AP:
                    return "CMD_SET_AP";
                case CMD_STA_START_FAILURE:
                    return "CMD_STA_START_FAILURE";
                case CMD_STA_STOPPED:
                    return "CMD_STA_STOPPED";
                case CMD_UPDATE_AP_CAPABILITY:
                    return "CMD_UPDATE_AP_CAPABILITY";
                case CMD_UPDATE_AP_CONFIG:
                    return "CMD_UPDATE_AP_CONFIG";
                case CMD_WIFI_TOGGLED:
                    return "CMD_WIFI_TOGGLED";
                default:
                    return "what:" + what;
            }
        }

        @Override
        public void start() {
            boolean isAirplaneModeOn = mSettingsStore.isAirplaneModeOn();
            boolean isWifiEnabled = mSettingsStore.isWifiToggleEnabled();
            boolean isScanningAlwaysAvailable = mSettingsStore.isScanAlwaysAvailable();
            boolean isLocationModeActive = mWifiPermissionsUtil.isLocationModeEnabled();

            log("isAirplaneModeOn = " + isAirplaneModeOn
                    + ", isWifiEnabled = " + isWifiEnabled
                    + ", isScanningAvailable = " + isScanningAlwaysAvailable
                    + ", isLocationModeActive = " + isLocationModeActive);

            // Initialize these values at bootup to defaults, will be overridden by API calls
            // for further toggles.
            mLastPrimaryClientModeManagerRequestorWs = mFacade.getSettingsWorkSource(mContext);
            mLastScanOnlyClientModeManagerRequestorWs = INTERNAL_REQUESTOR_WS;
            ActiveModeManager.ClientRole role = getRoleForPrimaryOrScanOnlyClientModeManager();
            if (role == ROLE_CLIENT_PRIMARY) {
                startPrimaryClientModeManager(mLastPrimaryClientModeManagerRequestorWs);
                setInitialState(mEnabledState);
            } else if (role == ROLE_CLIENT_SCAN_ONLY) {
                startScanOnlyClientModeManager(mLastScanOnlyClientModeManagerRequestorWs);
                setInitialState(mEnabledState);
            } else {
                setInitialState(mDisabledState);
            }
            mWifiMetrics.noteWifiEnabledDuringBoot(mSettingsStore.isWifiToggleEnabled());

            // Initialize the lower layers before we start.
            mWifiNative.initialize();
            super.start();
        }

        private int readWifiRecoveryDelay() {
            int recoveryDelayMillis = mContext.getResources().getInteger(
                    R.integer.config_wifi_framework_recovery_timeout_delay);
            if (recoveryDelayMillis > MAX_RECOVERY_TIMEOUT_DELAY_MS) {
                recoveryDelayMillis = MAX_RECOVERY_TIMEOUT_DELAY_MS;
                Log.w(TAG, "Overriding timeout delay with maximum limit value");
            }
            return recoveryDelayMillis;
        }

        abstract class BaseState extends State {
            @VisibleForTesting
            boolean isInEmergencyMode() {
                return mIsInEmergencyCall || mIsInEmergencyCallbackMode;
            }

            /** Device is in emergency mode & carrier config requires wifi off in emergency mode */
            private boolean isInEmergencyModeWhichRequiresWifiDisable() {
                return isInEmergencyMode() && mFacade.getConfigWiFiDisableInECBM(mContext);
            }

            private void updateEmergencyMode(Message msg) {
                if (msg.what == CMD_EMERGENCY_CALL_STATE_CHANGED) {
                    mIsInEmergencyCall = msg.arg1 == 1;
                } else if (msg.what == CMD_EMERGENCY_MODE_CHANGED) {
                    mIsInEmergencyCallbackMode = msg.arg1 == 1;
                }
            }

            private void enterEmergencyMode() {
                stopSoftApModeManagers(WifiManager.IFACE_IP_MODE_UNSPECIFIED);
                boolean configWiFiDisableInECBM = mFacade.getConfigWiFiDisableInECBM(mContext);
                log("Entering emergency callback mode, "
                        + "CarrierConfigManager.KEY_CONFIG_WIFI_DISABLE_IN_ECBM: "
                        + configWiFiDisableInECBM);
                if (!mIsEmergencyScanInProgress) {
                    if (configWiFiDisableInECBM) {
                        shutdownWifi();
                    }
                } else {
                    if (configWiFiDisableInECBM) {
                        switchAllPrimaryClientModeManagersToScanOnlyMode(
                                mFacade.getSettingsWorkSource(mContext));
                    }
                }
            }

            private void exitEmergencyMode() {
                log("Exiting emergency callback mode");
                // may be in DisabledState or EnabledState (depending on whether Wifi was shut down
                // in enterEmergencyMode() or not based on getConfigWiFiDisableInECBM).
                // Let CMD_WIFI_TOGGLED handling decide what the next state should be, or if we're
                // already in the correct state.

                // Assumes user toggled it on from settings before.
                wifiToggled(mFacade.getSettingsWorkSource(mContext));
            }

            private boolean processMessageInEmergencyMode(Message msg) {
                // In emergency mode: Some messages need special handling in this mode,
                // all others are dropped.
                switch (msg.what) {
                    case CMD_STA_STOPPED:
                    case CMD_AP_STOPPED:
                        log("Processing message in Emergency Callback Mode: " + msg);
                        if (!hasAnyModeManager()) {
                            log("No active mode managers, return to DisabledState.");
                            transitionTo(mDisabledState);
                        }
                        break;
                    case CMD_SET_AP:
                        // arg1 == 1 => enable AP
                        if (msg.arg1 == 1) {
                            log("AP cannot be started in Emergency Callback Mode: " + msg);
                            // SoftAP was disabled upon entering emergency mode. It also cannot
                            // be re-enabled during emergency mode. Drop the message and invoke
                            // the failure callback.
                            Pair<SoftApModeConfiguration, WorkSource> softApConfigAndWs =
                                    (Pair<SoftApModeConfiguration, WorkSource>) msg.obj;
                            SoftApModeConfiguration softApConfig = softApConfigAndWs.first;
                            WifiServiceImpl.SoftApCallbackInternal callback =
                                    softApConfig.getTargetMode() == IFACE_IP_MODE_LOCAL_ONLY
                                            ? mLohsCallback : mSoftApCallback;
                            // need to notify SoftApCallback that start/stop AP failed
                            callback.onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                                    WifiManager.SAP_START_FAILURE_GENERAL);
                        }
                        break;
                    default:
                        log("Dropping message in emergency callback mode: " + msg);
                        break;

                }
                return HANDLED;
            }

            private void handleEmergencyModeStateChange(Message msg) {
                boolean wasInEmergencyMode = isInEmergencyMode();
                updateEmergencyMode(msg);
                boolean isInEmergencyMode = isInEmergencyMode();
                if (!wasInEmergencyMode && isInEmergencyMode) {
                    enterEmergencyMode();
                } else if (wasInEmergencyMode && !isInEmergencyMode) {
                    exitEmergencyMode();
                }
            }

            private void handleEmergencyScanStateChange(Message msg) {
                final boolean scanInProgress = msg.arg1 == 1;
                final WorkSource requestorWs = (WorkSource) msg.obj;
                log("Processing scan state change: " + scanInProgress);
                mIsEmergencyScanInProgress = scanInProgress;
                if (isInEmergencyModeWhichRequiresWifiDisable())  {
                    // If wifi was disabled because of emergency mode
                    // (getConfigWiFiDisableInECBM == true), don't use the
                    // generic method to handle toggle change since that may put wifi in
                    // connectivity mode (since wifi toggle may actually be on underneath)
                    if (getCurrentState() == mDisabledState && scanInProgress) {
                        // go to scan only mode.
                        startScanOnlyClientModeManager(requestorWs);
                        transitionTo(mEnabledState);
                    } else if (getCurrentState() == mEnabledState && !scanInProgress) {
                        // shut down to go back to previous state.
                        stopAllClientModeManagers();
                    }
                } else {
                    if (getCurrentState() == mDisabledState) {
                        handleStaToggleChangeInDisabledState(requestorWs);
                    } else if (getCurrentState() == mEnabledState) {
                        handleStaToggleChangeInEnabledState(requestorWs);
                    }
                }
            }

            @Override
            public final boolean processMessage(Message msg) {
                // potentially enter emergency mode
                if (msg.what == CMD_EMERGENCY_CALL_STATE_CHANGED
                        || msg.what == CMD_EMERGENCY_MODE_CHANGED) {
                    handleEmergencyModeStateChange(msg);
                    return HANDLED;
                } else if (msg.what == CMD_EMERGENCY_SCAN_STATE_CHANGED) {
                    // emergency scans need to be allowed even in emergency mode.
                    handleEmergencyScanStateChange(msg);
                    return HANDLED;
                } else if (isInEmergencyMode()) {
                    return processMessageInEmergencyMode(msg);
                } else {
                    // not in emergency mode, process messages normally
                    return processMessageFiltered(msg);
                }
            }

            protected abstract boolean processMessageFiltered(Message msg);
        }

        class DefaultState extends State {
            @Override
            public boolean processMessage(Message msg) {
                switch (msg.what) {
                    case CMD_SCAN_ALWAYS_MODE_CHANGED:
                    case CMD_EMERGENCY_SCAN_STATE_CHANGED:
                    case CMD_WIFI_TOGGLED:
                    case CMD_STA_STOPPED:
                    case CMD_STA_START_FAILURE:
                    case CMD_AP_STOPPED:
                    case CMD_AP_START_FAILURE:
                    case CMD_RECOVERY_RESTART_WIFI:
                    case CMD_RECOVERY_RESTART_WIFI_CONTINUE:
                    case CMD_DEFERRED_RECOVERY_RESTART_WIFI:
                    case CMD_REMOVE_ADDITIONAL_CLIENT_MODE_MANAGER:
                        break;
                    case CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER:
                        AdditionalClientModeManagerRequestInfo requestInfo =
                                (AdditionalClientModeManagerRequestInfo) msg.obj;
                        requestInfo.listener.onAnswer(null);
                        break;
                    case CMD_RECOVERY_DISABLE_WIFI:
                        log("Recovery has been throttled, disable wifi");
                        shutdownWifi();
                        // onStopped will move the state machine to "DisabledState".
                        break;
                    case CMD_AIRPLANE_TOGGLED:
                        if (mSettingsStore.isAirplaneModeOn()) {
                            log("Airplane mode toggled, shutdown all modes");
                            shutdownWifi();
                            // onStopped will move the state machine to "DisabledState".
                        } else {
                            log("Airplane mode disabled, determine next state");
                            if (shouldEnableSta()) {
                                startPrimaryOrScanOnlyClientModeManager(
                                        // Assumes user toggled it on from settings before.
                                        mFacade.getSettingsWorkSource(mContext));
                                transitionTo(mEnabledState);
                            }
                            // wifi should remain disabled, do not need to transition
                        }
                        break;
                    case CMD_UPDATE_AP_CAPABILITY:
                        updateCapabilityToSoftApModeManager((SoftApCapability) msg.obj);
                        break;
                    case CMD_UPDATE_AP_CONFIG:
                        updateConfigurationToSoftApModeManager((SoftApConfiguration) msg.obj);
                        break;
                    default:
                        throw new RuntimeException("WifiController.handleMessage " + msg.what);
                }
                return HANDLED;
            }
        }

        private boolean shouldEnableScanOnlyMode() {
            return (mWifiPermissionsUtil.isLocationModeEnabled()
                    && mSettingsStore.isScanAlwaysAvailable())
                    || mIsEmergencyScanInProgress;
        }

        private boolean shouldEnableSta() {
            return mSettingsStore.isWifiToggleEnabled() || shouldEnableScanOnlyMode();
        }

        private void handleStaToggleChangeInDisabledState(WorkSource requestorWs) {
            if (shouldEnableSta()) {
                startPrimaryOrScanOnlyClientModeManager(requestorWs);
                transitionTo(mEnabledState);
            }
        }

        private void handleStaToggleChangeInEnabledState(WorkSource requestorWs) {
            if (shouldEnableSta()) {
                if (hasAnyClientModeManager()) {
                    switchAllPrimaryOrScanOnlyClientModeManagers();
                } else {
                    startPrimaryOrScanOnlyClientModeManager(requestorWs);
                }
            } else {
                stopAllClientModeManagers();
            }
        }

        class DisabledState extends BaseState {
            @Override
            public void enter() {
                log("DisabledState.enter()");
                super.enter();
                if (hasAnyModeManager()) {
                    Log.e(TAG, "Entered DisabledState, but has active mode managers");
                }
            }

            @Override
            public void exit() {
                log("DisabledState.exit()");
                super.exit();
            }

            @Override
            public boolean processMessageFiltered(Message msg) {
                switch (msg.what) {
                    case CMD_WIFI_TOGGLED:
                    case CMD_SCAN_ALWAYS_MODE_CHANGED:
                        handleStaToggleChangeInDisabledState((WorkSource) msg.obj);
                        break;
                    case CMD_SET_AP:
                        // note: CMD_SET_AP is handled/dropped in ECM mode - will not start here
                        if (msg.arg1 == 1) {
                            Pair<SoftApModeConfiguration, WorkSource> softApConfigAndWs =
                                    (Pair) msg.obj;
                            startSoftApModeManager(
                                    softApConfigAndWs.first, softApConfigAndWs.second);
                            transitionTo(mEnabledState);
                        }
                        break;
                    case CMD_RECOVERY_RESTART_WIFI:
                        log("Recovery triggered, already in disabled state");
                        sendMessageDelayed(CMD_RECOVERY_RESTART_WIFI_CONTINUE,
                                Collections.emptyList(), readWifiRecoveryDelay());
                        break;
                    case CMD_DEFERRED_RECOVERY_RESTART_WIFI:
                        // wait mRecoveryDelayMillis for letting driver clean reset.
                        sendMessageDelayed(CMD_RECOVERY_RESTART_WIFI_CONTINUE,
                                msg.obj, readWifiRecoveryDelay());
                        break;
                    case CMD_RECOVERY_RESTART_WIFI_CONTINUE:
                        log("Recovery in progress, start wifi");
                        List<ActiveModeManager> modeManagersBeforeRecovery = (List) msg.obj;
                        // No user controlled mode managers before recovery, so check if wifi
                        // was toggled on.
                        if (modeManagersBeforeRecovery.isEmpty()) {
                            if (shouldEnableSta()) {
                                startPrimaryOrScanOnlyClientModeManager(
                                        // Assumes user toggled it on from settings before.
                                        mFacade.getSettingsWorkSource(mContext));
                                transitionTo(mEnabledState);
                            }
                            break;
                        }
                        for (ActiveModeManager activeModeManager : modeManagersBeforeRecovery) {
                            if (activeModeManager instanceof ConcreteClientModeManager) {
                                startPrimaryOrScanOnlyClientModeManager(
                                        activeModeManager.getRequestorWs());
                            } else if (activeModeManager instanceof SoftApManager) {
                                SoftApManager softApManager = (SoftApManager) activeModeManager;
                                startSoftApModeManager(
                                        softApManager.getSoftApModeConfiguration(),
                                        softApManager.getRequestorWs());
                            }
                        }
                        transitionTo(mEnabledState);
                        int numCallbacks = mRestartCallbacks.beginBroadcast();
                        for (int i = 0; i < numCallbacks; i++) {
                            try {
                                mRestartCallbacks.getBroadcastItem(i).onSubsystemRestarted();
                            } catch (RemoteException e) {
                                Log.e(TAG, "Failure calling onSubsystemRestarted" + e);
                            }
                        }
                        mRestartCallbacks.finishBroadcast();
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class EnabledState extends BaseState {

            private boolean mIsDisablingDueToAirplaneMode;

            @Override
            public void enter() {
                log("EnabledState.enter()");
                super.enter();
                if (!hasAnyModeManager()) {
                    Log.e(TAG, "Entered EnabledState, but no active mode managers");
                }
                mIsDisablingDueToAirplaneMode = false;
            }

            @Override
            public void exit() {
                log("EnabledState.exit()");
                if (hasAnyModeManager()) {
                    Log.e(TAG, "Exiting EnabledState, but has active mode managers");
                }
                super.exit();
            }

            private boolean isClientModeManagerConnectedOrConnectingToBssid(
                    @NonNull ClientModeManager clientModeManager,
                    @NonNull String ssid, @NonNull String bssid) {
                WifiConfiguration connectedOrConnectingWifiConfiguration = coalesce(
                        clientModeManager.getConnectingWifiConfiguration(),
                        clientModeManager.getConnectedWifiConfiguration());
                String connectedOrConnectingBssid = coalesce(
                        clientModeManager.getConnectingBssid(),
                        clientModeManager.getConnectedBssid());
                String connectedOrConnectingSsid =
                        connectedOrConnectingWifiConfiguration == null
                                ? null : connectedOrConnectingWifiConfiguration.SSID;
                return Objects.equals(ssid, connectedOrConnectingSsid)
                        && Objects.equals(bssid, connectedOrConnectingBssid);
            }

            @Nullable
            private ConcreteClientModeManager findAnyClientModeManagerConnectingOrConnectedToBssid(
                    @NonNull String ssid, @Nullable String bssid) {
                if (bssid == null) {
                    return null;
                }
                for (ConcreteClientModeManager cmm : mClientModeManagers) {
                    if (isClientModeManagerConnectedOrConnectingToBssid(cmm, ssid, bssid)) {
                        return cmm;
                    }
                }
                return null;
            }

            private void handleAdditionalClientModeManagerRequest(
                    @NonNull AdditionalClientModeManagerRequestInfo requestInfo) {
                ClientModeManager primaryManager = getPrimaryClientModeManager();
                if (requestInfo.clientRole == ROLE_CLIENT_SECONDARY_TRANSIENT
                        && mDppManager.isSessionInProgress()) {
                    // When MBB is triggered, we could end up switching the primary interface
                    // after completion. So if we have any DPP session in progress, they will fail
                    // when the previous primary iface is removed after MBB completion.
                    Log.v(TAG, "DPP session in progress, fallback to single STA behavior "
                            + "using primary ClientModeManager=" + primaryManager);
                    requestInfo.listener.onAnswer(primaryManager);
                    return;
                }
                ConcreteClientModeManager cmmForSameBssid =
                        findAnyClientModeManagerConnectingOrConnectedToBssid(
                                requestInfo.ssid, requestInfo.bssid);
                if (cmmForSameBssid != null) {
                    // Can't allow 2 client mode managers triggering connection to same bssid.
                    Log.v(TAG, "Already connected to bssid=" + requestInfo.bssid
                            + " on ClientModeManager=" + cmmForSameBssid);
                    if (cmmForSameBssid.getRole() == ROLE_CLIENT_PRIMARY) {
                        // fallback to single STA behavior.
                        requestInfo.listener.onAnswer(cmmForSameBssid);
                        return;
                    }
                    // Existing secondary CMM connected to the same ssid/bssid.
                    if (!canRequestMoreClientModeManagersInRole(
                            requestInfo.requestorWs, requestInfo.clientRole)) {
                        Log.e(TAG, "New request cannot override existing request on "
                                + "ClientModeManager=" + cmmForSameBssid);
                        // If the new request does not have priority over the existing request,
                        // reject it since we cannot have 2 CMM's connected to same ssid/bssid.
                        requestInfo.listener.onAnswer(null);
                        return;
                    }
                    // If the new request has a higher priority over the existing one, change it's
                    // role and send it to the new client.
                    // Switch role for non primary CMM & wait for it to complete before
                    // handing it to the requestor.
                    switchRoleForAdditionalClientModeManager(
                            cmmForSameBssid, requestInfo.clientRole, requestInfo.listener,
                            requestInfo.requestorWs);
                    return;
                }

                ClientModeManager cmmForSameRole =
                        getClientModeManagerInRole(requestInfo.clientRole);
                if (cmmForSameRole != null) {
                    // Already have a client mode manager in the requested role.
                    // Note: This logic results in the framework not supporting more than 1 CMM in
                    // the same role concurrently. There is no use-case for that currently &
                    // none of the clients (i.e WifiNetworkFactory, WifiConnectivityManager, etc)
                    // are ready to support that either. If this assumption changes in the future
                    // when the device supports 3 STA's for example, change this logic!
                    Log.v(TAG, "Already exists ClientModeManager for role: " + cmmForSameRole);
                    requestInfo.listener.onAnswer(cmmForSameRole);
                    return;
                }
                if (canRequestMoreClientModeManagersInRole(
                        requestInfo.requestorWs, requestInfo.clientRole)) {
                    // Can create an additional client mode manager.
                    Log.v(TAG, "Starting a new ClientModeManager");
                    startAdditionalClientModeManager(
                            requestInfo.clientRole,
                            requestInfo.listener, requestInfo.requestorWs);
                    return;
                }
                // Fall back to single STA behavior.
                Log.v(TAG, "Falling back to single STA behavior using primary ClientModeManager="
                        + primaryManager);
                requestInfo.listener.onAnswer(primaryManager);
            }

            @Override
            public boolean processMessageFiltered(Message msg) {
                switch (msg.what) {
                    case CMD_WIFI_TOGGLED:
                    case CMD_SCAN_ALWAYS_MODE_CHANGED:
                        handleStaToggleChangeInEnabledState((WorkSource) msg.obj);
                        break;
                    case CMD_REQUEST_ADDITIONAL_CLIENT_MODE_MANAGER:
                        handleAdditionalClientModeManagerRequest(
                                (AdditionalClientModeManagerRequestInfo) msg.obj);
                        break;
                    case CMD_REMOVE_ADDITIONAL_CLIENT_MODE_MANAGER:
                        stopAdditionalClientModeManager((ClientModeManager) msg.obj);
                        break;
                    case CMD_SET_AP:
                        // note: CMD_SET_AP is handled/dropped in ECM mode - will not start here
                        if (msg.arg1 == 1) {
                            Pair<SoftApModeConfiguration, WorkSource> softApConfigAndWs =
                                    (Pair) msg.obj;
                            startSoftApModeManager(
                                    softApConfigAndWs.first, softApConfigAndWs.second);
                        } else {
                            stopSoftApModeManagers(msg.arg2);
                        }
                        break;
                    case CMD_AIRPLANE_TOGGLED:
                        // airplane mode toggled on is handled in the default state
                        if (mSettingsStore.isAirplaneModeOn()) {
                            mIsDisablingDueToAirplaneMode = true;
                            return NOT_HANDLED;
                        } else {
                            if (mIsDisablingDueToAirplaneMode) {
                                // Previous airplane mode toggle on is being processed, defer the
                                // message toggle off until previous processing is completed.
                                // Once previous airplane mode toggle is complete, we should
                                // transition to DisabledState. There, we will process the deferred
                                // airplane mode toggle message to disable airplane mode.
                                deferMessage(msg);
                            } else {
                                // when airplane mode is toggled off, but wifi is on, we can keep it
                                // on
                                log("airplane mode toggled - and airplane mode is off. return "
                                        + "handled");
                            }
                            return HANDLED;
                        }
                    case CMD_AP_STOPPED:
                    case CMD_AP_START_FAILURE:
                        if (!hasAnyModeManager()) {
                            if (shouldEnableSta()) {
                                log("SoftAp disabled, start client mode");
                                startPrimaryOrScanOnlyClientModeManager(
                                        // Assumes user toggled it on from settings before.
                                        mFacade.getSettingsWorkSource(mContext));
                            } else {
                                log("SoftAp mode disabled, return to DisabledState");
                                transitionTo(mDisabledState);
                            }
                        } else {
                            log("AP disabled, remain in EnabledState.");
                        }
                        break;
                    case CMD_STA_START_FAILURE:
                    case CMD_STA_STOPPED:
                        // Client mode stopped. Head to Disabled to wait for next command if there
                        // no active mode managers.
                        if (!hasAnyModeManager()) {
                            log("STA disabled, return to DisabledState.");
                            transitionTo(mDisabledState);
                        } else {
                            log("STA disabled, remain in EnabledState.");
                        }
                        break;
                    case CMD_RECOVERY_RESTART_WIFI: {
                        final String bugTitle;
                        final String bugDetail = (String) msg.obj;
                        if (TextUtils.isEmpty(bugDetail)) {
                            bugTitle = "Wi-Fi BugReport";
                        } else {
                            bugTitle = "Wi-Fi BugReport: " + bugDetail;
                        }
                        log("Recovery triggered, disable wifi");
                        boolean bugReportRequested = msg.arg2 != 0;
                        if (bugReportRequested) {
                            mHandler.post(() ->
                                    mWifiDiagnostics.takeBugReport(bugTitle, bugDetail));
                        }
                        // Store all instances of tethered SAP + scan only/primary STA mode managers
                        List<ActiveModeManager> modeManagersBeforeRecovery = Stream.concat(
                                mClientModeManagers.stream()
                                        .filter(m -> ROLE_CLIENT_SCAN_ONLY.equals(m.getRole())
                                                || ROLE_CLIENT_PRIMARY.equals(m.getRole())),
                                mSoftApManagers.stream()
                                        .filter(m -> ROLE_SOFTAP_TETHERED.equals(m.getRole())))
                                .collect(Collectors.toList());
                        deferMessage(obtainMessage(CMD_DEFERRED_RECOVERY_RESTART_WIFI,
                                modeManagersBeforeRecovery));
                        int numCallbacks = mRestartCallbacks.beginBroadcast();
                        for (int i = 0; i < numCallbacks; i++) {
                            try {
                                mRestartCallbacks.getBroadcastItem(i).onSubsystemRestarting();
                            } catch (RemoteException e) {
                                Log.e(TAG, "Failure calling onSubsystemRestarting" + e);
                            }
                        }
                        mRestartCallbacks.finishBroadcast();
                        shutdownWifi();
                        // onStopped will move the state machine to "DisabledState".
                        break;
                    }
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }
    }

    private static <T> T coalesce(T a, T  b) {
        return a != null ? a : b;
    }
}
