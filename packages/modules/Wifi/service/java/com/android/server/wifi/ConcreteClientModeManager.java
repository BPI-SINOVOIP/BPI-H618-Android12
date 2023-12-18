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

import static android.net.wifi.WifiManager.WIFI_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_STATE_UNKNOWN;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.DhcpResultsParcelable;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.wifi.IWifiConnectedNetworkScorer;
import android.net.wifi.WifiAnnotations;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.hotspot2.IProvisioningCallback;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.nl80211.DeviceWiphyCapabilities;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.UserHandle;
import android.os.WorkSource;
import android.telephony.AccessNetworkConstants;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.ims.ImsException;
import android.telephony.ims.ImsMmTelManager;
import android.telephony.ims.ImsReasonInfo;
import android.telephony.ims.RegistrationManager;
import android.telephony.ims.feature.MmTelFeature;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.util.IState;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.server.wifi.WifiNative.InterfaceCallback;
import com.android.server.wifi.WifiNative.RxFateReport;
import com.android.server.wifi.WifiNative.TxFateReport;
import com.android.server.wifi.util.ActionListenerWrapper;
import com.android.server.wifi.util.StateMachineObituary;
import com.android.server.wifi.util.WifiHandler;
import com.android.wifi.resources.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayDeque;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Manage WiFi in Client Mode where we connect to configured networks and in Scan Only Mode where
 * we do not connect to configured networks but do perform scanning.
 *
 * An instance of this class is active to manage each client interface. This is in contrast to
 * {@link DefaultClientModeManager} which handles calls when no client interfaces are active.
 *
 * This class will dynamically instantiate {@link ClientModeImpl} when it enters client mode, and
 * tear it down when it exits client mode. No instance of ClientModeImpl will be active in
 * scan-only mode, instead {@link ScanOnlyModeImpl} will be used to respond to calls.
 *
 * <pre>
 *                                           ActiveModeWarden
 *                                      /                        \
 *                                     /                          \
 *                        ConcreteClientModeManager         DefaultClientModeManager
 *                      (Client Mode + Scan Only Mode)            (Wifi off)
 *                             /            \
 *                           /               \
 *                     ClientModeImpl       ScanOnlyModeImpl
 * </pre>
 */
public class ConcreteClientModeManager implements ClientModeManager {
    private static final String TAG = "WifiClientModeManager";

    private final ClientModeStateMachine mStateMachine;

    private final Context mContext;
    private final Clock mClock;
    private final WifiNative mWifiNative;
    private final WifiMetrics mWifiMetrics;
    private final WakeupController mWakeupController;
    private final WifiInjector mWifiInjector;
    private final SelfRecovery mSelfRecovery;
    private final WifiGlobals mWifiGlobals;
    private final DefaultClientModeManager mDefaultClientModeManager;
    private final ClientModeManagerBroadcastQueue mBroadcastQueue;
    private final long mId;
    private final Graveyard mGraveyard = new Graveyard();

    private String mClientInterfaceName;
    private boolean mIfaceIsUp = false;
    private boolean mShouldReduceNetworkScore = false;
    private final DeferStopHandler mDeferStopHandler;
    @Nullable
    private ClientRole mRole = null;
    @Nullable
    private ClientRole mPreviousRole = null;
    private long mLastRoleChangeSinceBootMs = 0;
    @Nullable
    private WorkSource mRequestorWs = null;
    @NonNull
    private Listener<ConcreteClientModeManager> mModeListener;
    /** Caches the latest role change request. This is needed for the IMS dereg delay */
    @Nullable
    private RoleChangeInfo mTargetRoleChangeInfo;
    private boolean mVerboseLoggingEnabled = false;
    private int mActiveSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    private boolean mWifiStateChangeBroadcastEnabled = true;
    /**
     * mClientModeImpl is only non-null when in {@link ClientModeStateMachine.ConnectModeState} -
     * it will be null in all other states
     */
    @Nullable
    private ClientModeImpl mClientModeImpl = null;

    @Nullable
    private ScanOnlyModeImpl mScanOnlyModeImpl = null;

    /**
     * One of  {@link WifiManager#WIFI_STATE_DISABLED},
     * {@link WifiManager#WIFI_STATE_DISABLING},
     * {@link WifiManager#WIFI_STATE_ENABLED},
     * {@link WifiManager#WIFI_STATE_ENABLING},
     * {@link WifiManager#WIFI_STATE_UNKNOWN}
     */
    private final AtomicInteger mWifiState = new AtomicInteger(WIFI_STATE_DISABLED);

    private boolean mIsStopped = true;

    ConcreteClientModeManager(
            Context context, @NonNull Looper looper, Clock clock,
            WifiNative wifiNative, @NonNull Listener<ConcreteClientModeManager> listener,
            WifiMetrics wifiMetrics,
            WakeupController wakeupController, WifiInjector wifiInjector,
            SelfRecovery selfRecovery, WifiGlobals wifiGlobals,
            DefaultClientModeManager defaultClientModeManager, long id,
            @NonNull WorkSource requestorWs, @NonNull ClientRole role,
            @NonNull ClientModeManagerBroadcastQueue broadcastQueue,
            boolean verboseLoggingEnabled) {
        mContext = context;
        mClock = clock;
        mWifiNative = wifiNative;
        mModeListener = listener;
        mWifiMetrics = wifiMetrics;
        mWakeupController = wakeupController;
        mWifiInjector = wifiInjector;
        mStateMachine = new ClientModeStateMachine(looper);
        mDeferStopHandler = new DeferStopHandler(looper);
        mSelfRecovery = selfRecovery;
        mWifiGlobals = wifiGlobals;
        mDefaultClientModeManager = defaultClientModeManager;
        mId = id;
        mTargetRoleChangeInfo = new RoleChangeInfo(role, requestorWs, listener);
        mBroadcastQueue = broadcastQueue;
        enableVerboseLogging(verboseLoggingEnabled);
        mStateMachine.sendMessage(ClientModeStateMachine.CMD_START, mTargetRoleChangeInfo);
    }

    private String getTag() {
        return TAG + "[" + (mClientInterfaceName == null ? "unknown" : mClientInterfaceName) + "]";
    }

    /**
     * Sets whether to send WIFI_STATE_CHANGED broadcast for this ClientModeManager.
     * @param enabled
     */
    public void setWifiStateChangeBroadcastEnabled(boolean enabled) {
        mWifiStateChangeBroadcastEnabled = enabled;
    }

    /**
     * Disconnect from any currently connected networks and stop client mode.
     */
    @Override
    public void stop() {
        Log.d(getTag(), " currentstate: " + getCurrentStateName());
        mTargetRoleChangeInfo = null;
        if (mIfaceIsUp) {
            updateConnectModeState(mRole, WifiManager.WIFI_STATE_DISABLING,
                    WifiManager.WIFI_STATE_ENABLED);
        } else {
            updateConnectModeState(mRole, WifiManager.WIFI_STATE_DISABLING,
                    WifiManager.WIFI_STATE_ENABLING);
        }
        mDeferStopHandler.start(getWifiOffDeferringTimeMs());
    }

    private class DeferStopHandler extends WifiHandler {
        private boolean mIsDeferring = false;
        private ImsMmTelManager mImsMmTelManager = null;
        private Looper mLooper = null;
        private final Runnable mRunnable = () -> continueToStopWifi();
        private int mMaximumDeferringTimeMillis = 0;
        private long mDeferringStartTimeMillis = 0;
        private ConnectivityManager mConnectivityManager = null;
        private boolean mIsImsNetworkLost = false;
        private boolean mIsImsNetworkUnregistered = false;

        private final RegistrationManager.RegistrationCallback mImsRegistrationCallback =
                new RegistrationManager.RegistrationCallback() {
                    @Override
                    public void onRegistered(int imsRadioTech) {
                        Log.d(getTag(), "on IMS registered on type " + imsRadioTech);
                        if (!mIsDeferring) return;

                        if (imsRadioTech != AccessNetworkConstants.TRANSPORT_TYPE_WLAN) {
                            continueToStopWifi();
                        }
                    }

                    @Override
                    public void onUnregistered(ImsReasonInfo imsReasonInfo) {
                        Log.d(getTag(), "on IMS unregistered");
                        mIsImsNetworkUnregistered = true;
                        checkAndContinueToStopWifi();
                    }
                };

        private final class ImsNetworkCallback extends NetworkCallback {
            private int mRegisteredImsNetworkCount = 0;

            @Override
            public void onAvailable(Network network) {
                synchronized (this) {
                    Log.d(getTag(), "IMS network available: " + network);
                    mRegisteredImsNetworkCount++;
                }
            }

            @Override
            public void onLost(Network network) {
                synchronized (this) {
                    Log.d(getTag(), "IMS network lost: " + network
                            + " ,isDeferring: " + mIsDeferring
                            + " ,registered IMS network count: " + mRegisteredImsNetworkCount);
                    mRegisteredImsNetworkCount--;
                    if (mIsDeferring && mRegisteredImsNetworkCount <= 0) {
                        mRegisteredImsNetworkCount = 0;
                        mIsImsNetworkLost = true;
                        checkAndContinueToStopWifi();
                    }
                }
            }
        }

        private NetworkCallback mImsNetworkCallback = null;

        DeferStopHandler(Looper looper) {
            super(TAG, looper);
            mLooper = looper;
        }

        public void start(int delayMs) {
            if (mIsDeferring) return;

            mMaximumDeferringTimeMillis = delayMs;
            mDeferringStartTimeMillis = mClock.getElapsedSinceBootMillis();
            // Most cases don't need delay, check it first to avoid unnecessary work.
            if (delayMs == 0) {
                continueToStopWifi();
                return;
            }

            mImsMmTelManager = ImsMmTelManager.createForSubscriptionId(mActiveSubId);
            if (mImsMmTelManager == null || !postDelayed(mRunnable, delayMs)) {
                // if no delay or failed to add runnable, stop Wifi immediately.
                continueToStopWifi();
                return;
            }

            mIsDeferring = true;
            Log.d(getTag(), "Start DeferWifiOff handler with deferring time "
                    + delayMs + " ms for subId: " + mActiveSubId);
            try {
                mImsMmTelManager.registerImsRegistrationCallback(
                        new HandlerExecutor(new Handler(mLooper)),
                        mImsRegistrationCallback);
            } catch (RuntimeException | ImsException e) {
                Log.e(getTag(), "registerImsRegistrationCallback failed", e);
                continueToStopWifi();
                return;
            }

            NetworkRequest imsRequest = new NetworkRequest.Builder()
                    .addCapability(NetworkCapabilities.NET_CAPABILITY_IMS)
                    .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR)
                    .build();

            mConnectivityManager =
                    (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);

            mImsNetworkCallback = new ImsNetworkCallback();
            mConnectivityManager.registerNetworkCallback(imsRequest, mImsNetworkCallback,
                    new Handler(mLooper));
        }

        private void checkAndContinueToStopWifi() {
            if (mIsImsNetworkLost && mIsImsNetworkUnregistered) {
                // Add delay for targets where IMS PDN down at modem takes additional delay.
                int delay = mContext.getResources()
                        .getInteger(R.integer.config_wifiDelayDisconnectOnImsLostMs);
                if (delay == 0 || !postDelayed(mRunnable, delay)) {
                    continueToStopWifi();
                }
            }
        }

        private void continueToStopWifi() {
            Log.d(getTag(), "The target role change info " + mTargetRoleChangeInfo);

            int deferringDurationMillis =
                    (int) (mClock.getElapsedSinceBootMillis() - mDeferringStartTimeMillis);
            boolean isTimedOut = mMaximumDeferringTimeMillis > 0
                    && deferringDurationMillis >= mMaximumDeferringTimeMillis;
            if (mTargetRoleChangeInfo == null) {
                Log.d(getTag(), "Continue to stop wifi");
                mStateMachine.captureObituaryAndQuitNow();
                mWifiMetrics.noteWifiOff(mIsDeferring, isTimedOut, deferringDurationMillis);
            } else if (mTargetRoleChangeInfo.role == ROLE_CLIENT_SCAN_ONLY) {
                if (!mWifiNative.switchClientInterfaceToScanMode(
                        mClientInterfaceName, mTargetRoleChangeInfo.requestorWs)) {
                    mModeListener.onStartFailure(ConcreteClientModeManager.this);
                } else {
                    mStateMachine.sendMessage(
                            ClientModeStateMachine.CMD_SWITCH_TO_SCAN_ONLY_MODE_CONTINUE,
                            mTargetRoleChangeInfo);
                    mWifiMetrics.noteWifiOff(mIsDeferring, isTimedOut, deferringDurationMillis);
                }
            } else {
                updateConnectModeState(mRole, WifiManager.WIFI_STATE_ENABLED,
                        WifiManager.WIFI_STATE_DISABLING);
            }

            if (!mIsDeferring) return;

            Log.d(getTag(), "Stop DeferWifiOff handler.");
            removeCallbacks(mRunnable);
            if (mImsMmTelManager != null) {
                try {
                    mImsMmTelManager.unregisterImsRegistrationCallback(mImsRegistrationCallback);
                } catch (RuntimeException e) {
                    Log.e(getTag(), "unregisterImsRegistrationCallback failed", e);
                }
            }

            if (mConnectivityManager != null && mImsNetworkCallback != null) {
                mConnectivityManager.unregisterNetworkCallback(mImsNetworkCallback);
                mImsNetworkCallback = null;
            }

            mIsDeferring = false;
            mIsImsNetworkLost = false;
            mIsImsNetworkUnregistered = false;
        }
    }

    /**
     * Get deferring time before turning off WiFi.
     */
    private int getWifiOffDeferringTimeMs() {
        SubscriptionManager subscriptionManager = (SubscriptionManager) mContext.getSystemService(
                Context.TELEPHONY_SUBSCRIPTION_SERVICE);
        if (subscriptionManager == null) {
            Log.d(getTag(), "SubscriptionManager not found");
            return 0;
        }

        List<SubscriptionInfo> subInfoList = subscriptionManager.getActiveSubscriptionInfoList();
        if (subInfoList == null) {
            Log.d(getTag(), "Active SubscriptionInfo list not found");
            return 0;
        }

        // Get the maximum delay for the active subscription latched on IWLAN.
        int maxDelay = 0;
        for (SubscriptionInfo subInfo : subInfoList) {
            int curDelay = getWifiOffDeferringTimeMs(subInfo.getSubscriptionId());
            if (curDelay > maxDelay) {
                maxDelay = curDelay;
                mActiveSubId = subInfo.getSubscriptionId();
            }
        }
        return maxDelay;
    }

    private int getWifiOffDeferringTimeMs(int subId) {
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            Log.d(getTag(), "Invalid Subscription ID: " + subId);
            return 0;
        }

        ImsMmTelManager imsMmTelManager = ImsMmTelManager.createForSubscriptionId(subId);
        // If no wifi calling, no delay
        try {
            if (!imsMmTelManager.isAvailable(
                    MmTelFeature.MmTelCapabilities.CAPABILITY_TYPE_VOICE,
                    ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN)) {
                Log.d(getTag(), "IMS not registered over IWLAN for subId: " + subId);
                return 0;
            }
        } catch (RuntimeException ex) {
            Log.e(TAG, "IMS Manager is not available.", ex);
            return 0;
        }

        CarrierConfigManager configManager =
                (CarrierConfigManager) mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle config = configManager.getConfigForSubId(subId);
        return (config != null)
                ? config.getInt(CarrierConfigManager.Ims.KEY_WIFI_OFF_DEFERRING_TIME_MILLIS_INT)
                : 0;
    }

    @Override
    @Nullable public ClientRole getRole() {
        return mRole;
    }

    @Override
    @Nullable public ClientRole getPreviousRole() {
        return mPreviousRole;
    }

    @Override
    public long getLastRoleChangeSinceBootMs() {
        return mLastRoleChangeSinceBootMs;
    }

    /**
     * Class to hold info needed for role change.
     */
    private static class RoleChangeInfo {
        @Nullable public final ClientRole role;
        @Nullable public final WorkSource requestorWs;
        @Nullable public final Listener<ConcreteClientModeManager> modeListener;

        RoleChangeInfo(@Nullable ClientRole role) {
            this(role, null, null);
        }

        RoleChangeInfo(@Nullable ClientRole role, @Nullable WorkSource requestorWs,
                @Nullable Listener<ConcreteClientModeManager> modeListener) {
            this.role = role;
            this.requestorWs = requestorWs;
            this.modeListener = modeListener;
        }

        @Override
        public String toString() {
            return "Role: " + role + ", RequestorWs: " + requestorWs
                    + ", ModeListener: " + modeListener;
        }
    }

    /** Set the role of this ClientModeManager */
    public void setRole(@NonNull ClientRole role, @NonNull WorkSource requestorWs) {
        setRole(role, requestorWs, null);
    }

    /** Set the role of this ClientModeManager */
    public void setRole(@NonNull ClientRole role, @NonNull WorkSource requestorWs,
            @Nullable Listener<ConcreteClientModeManager> modeListener) {
        mTargetRoleChangeInfo = new RoleChangeInfo(role, requestorWs, modeListener);
        if (role == ROLE_CLIENT_SCAN_ONLY) {
            // Switch client mode manager to scan only mode.
            mStateMachine.sendMessage(
                    ClientModeStateMachine.CMD_SWITCH_TO_SCAN_ONLY_MODE);
        } else {
            // Switch client mode manager to connect mode.
            mStateMachine.sendMessage(
                    ClientModeStateMachine.CMD_SWITCH_TO_CONNECT_MODE,
                    mTargetRoleChangeInfo);
        }
    }

    @Override
    public String getInterfaceName() {
        return mClientInterfaceName;
    }

    @Override
    public WorkSource getRequestorWs() {
        return mRequestorWs;
    }

    /**
     * Keep stopped {@link ClientModeImpl} instances so that they can be dumped to aid debugging.
     *
     * TODO(b/160283853): Find a smarter way to evict old ClientModeImpls
     */
    private static class Graveyard {
        private static final int INSTANCES_TO_KEEP = 3;

        private final ArrayDeque<ClientModeImpl> mClientModeImpls = new ArrayDeque<>();

        /**
         * Add this stopped {@link ClientModeImpl} to the graveyard, and evict the oldest
         * ClientModeImpl if the graveyard is full.
         */
        void inter(ClientModeImpl clientModeImpl) {
            if (mClientModeImpls.size() == INSTANCES_TO_KEEP) {
                mClientModeImpls.removeFirst();
            }
            mClientModeImpls.addLast(clientModeImpl);
        }

        /** Dump the contents of the graveyard. */
        void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            pw.println("Dump of ConcreteClientModeManager.Graveyard");
            pw.println("Stopped ClientModeImpls: " + mClientModeImpls.size() + " total");
            for (ClientModeImpl clientModeImpl : mClientModeImpls) {
                clientModeImpl.dump(fd, pw, args);
            }
            pw.println();
        }

        boolean hasAllClientModeImplsQuit() {
            for (ClientModeImpl cmi : mClientModeImpls) {
                if (!cmi.hasQuit()) return false;
            }
            return true;
        }
    }

    /**
     * Dump info about this ClientMode manager.
     */
    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of ClientModeManager id=" + mId);
        pw.println("current StateMachine mode: " + getCurrentStateName());
        pw.println("mRole: " + mRole);
        pw.println("mTargetRoleChangeInfo: " + mTargetRoleChangeInfo);
        pw.println("mClientInterfaceName: " + mClientInterfaceName);
        pw.println("mIfaceIsUp: " + mIfaceIsUp);
        mStateMachine.dump(fd, pw, args);
        pw.println();
        pw.println("Wi-Fi is " + syncGetWifiStateByName());
        if (mClientModeImpl == null) {
            pw.println("No active ClientModeImpl instance");
        } else {
            mClientModeImpl.dump(fd, pw, args);
        }
        mGraveyard.dump(fd, pw, args);
        pw.println();
    }

    private String getCurrentStateName() {
        IState currentState = mStateMachine.getCurrentState();

        if (currentState != null) {
            return currentState.getName();
        }

        return "StateMachine not active";
    }

    /**
     * Update Wifi state and send the broadcast.
     *
     * @param role         Target/Set role for this client mode manager instance.
     * @param newState     new Wifi state
     * @param currentState current wifi state
     */
    private void updateConnectModeState(ClientRole role, int newState, int currentState) {
        setWifiStateForApiCalls(newState);

        if (newState == WifiManager.WIFI_STATE_UNKNOWN) {
            // do not need to broadcast failure to system
            return;
        }
        if (role != ROLE_CLIENT_PRIMARY || !mWifiStateChangeBroadcastEnabled) {
            // do not raise public broadcast unless this is the primary client mode manager
            return;
        }

        // TODO(b/175839153): this broadcast should only be sent out when wifi is toggled on/off,
        //  not per CMM
        final Intent intent = new Intent(WifiManager.WIFI_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_WIFI_STATE, newState);
        intent.putExtra(WifiManager.EXTRA_PREVIOUS_WIFI_STATE, currentState);
        String summary = "broadcast=WIFI_STATE_CHANGED_ACTION"
                + " EXTRA_WIFI_STATE=" + newState
                + " EXTRA_PREVIOUS_WIFI_STATE=" + currentState;
        if (mVerboseLoggingEnabled) Log.d(getTag(), "Queuing " + summary);
        ClientModeManagerBroadcastQueue.QueuedBroadcast broadcast =
                () -> {
                    if (mVerboseLoggingEnabled) Log.d(getTag(), "Sending " + summary);
                    mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
                };
        if (mRole == null && role == ROLE_CLIENT_PRIMARY) {
            // This CMM is intended to be the primary, but has not completed the mode transition
            // yet. Need to force broadcast to be sent.
            broadcast.send();
        } else {
            mBroadcastQueue.queueOrSendBroadcast(this, broadcast);
        }
    }

    private void setWifiStateForApiCalls(int newState) {
        switch (newState) {
            case WIFI_STATE_DISABLING:
            case WIFI_STATE_DISABLED:
            case WIFI_STATE_ENABLING:
            case WIFI_STATE_ENABLED:
            case WIFI_STATE_UNKNOWN:
                if (mVerboseLoggingEnabled) {
                    Log.d(getTag(), "setting wifi state to: " + newState);
                }
                mWifiState.set(newState);
                break;
            default:
                Log.d(getTag(), "attempted to set an invalid state: " + newState);
                break;
        }
    }

    private String syncGetWifiStateByName() {
        switch (mWifiState.get()) {
            case WIFI_STATE_DISABLING:
                return "disabling";
            case WIFI_STATE_DISABLED:
                return "disabled";
            case WIFI_STATE_ENABLING:
                return "enabling";
            case WIFI_STATE_ENABLED:
                return "enabled";
            case WIFI_STATE_UNKNOWN:
                return "unknown state";
            default:
                return "[invalid state]";
        }
    }

    private class ClientModeStateMachine extends StateMachine {
        // Commands for the state machine.
        public static final int CMD_START = 0;
        public static final int CMD_SWITCH_TO_SCAN_ONLY_MODE = 1;
        public static final int CMD_SWITCH_TO_CONNECT_MODE = 2;
        public static final int CMD_INTERFACE_STATUS_CHANGED = 3;
        public static final int CMD_INTERFACE_DESTROYED = 4;
        public static final int CMD_INTERFACE_DOWN = 5;
        public static final int CMD_SWITCH_TO_SCAN_ONLY_MODE_CONTINUE = 6;
        private final State mIdleState = new IdleState();
        private final State mStartedState = new StartedState();
        private final State mScanOnlyModeState = new ScanOnlyModeState();
        private final State mConnectModeState = new ConnectModeState();
        // Workaround since we cannot use transitionTo(mScanOnlyModeState, RoleChangeInfo)
        private RoleChangeInfo mScanRoleChangeInfoToSetOnTransition = null;
        // Workaround since we cannot use transitionTo(mConnectModeState, RoleChangeInfo)
        private RoleChangeInfo mConnectRoleChangeInfoToSetOnTransition = null;

        @Nullable
        private StateMachineObituary mObituary = null;

        private final InterfaceCallback mWifiNativeInterfaceCallback = new InterfaceCallback() {
            @Override
            public void onDestroyed(String ifaceName) {
                if (mClientInterfaceName != null && mClientInterfaceName.equals(ifaceName)) {
                    Log.d(getTag(), "STA iface " + ifaceName + " was destroyed, "
                            + "stopping client mode");

                    // we must immediately clean up state in ClientModeImpl to unregister
                    // all client mode related objects
                    // Note: onDestroyed is only called from the main Wifi thread
                    if (mClientModeImpl == null) {
                        Log.w(getTag(), "Received mWifiNativeInterfaceCallback.onDestroyed "
                                + "callback when no ClientModeImpl instance is active.");
                    } else {
                        mClientModeImpl.handleIfaceDestroyed();
                    }

                    sendMessage(CMD_INTERFACE_DESTROYED);
                }
            }

            @Override
            public void onUp(String ifaceName) {
                if (mClientInterfaceName != null && mClientInterfaceName.equals(ifaceName)) {
                    sendMessage(CMD_INTERFACE_STATUS_CHANGED, 1);
                }
            }

            @Override
            public void onDown(String ifaceName) {
                if (mClientInterfaceName != null && mClientInterfaceName.equals(ifaceName)) {
                    sendMessage(CMD_INTERFACE_STATUS_CHANGED, 0);
                }
            }
        };

        ClientModeStateMachine(Looper looper) {
            super(TAG, looper);

            // CHECKSTYLE:OFF IndentationCheck
            addState(mIdleState);
                addState(mStartedState, mIdleState);
                    addState(mScanOnlyModeState, mStartedState);
                    addState(mConnectModeState, mStartedState);
            // CHECKSTYLE:ON IndentationCheck

            setInitialState(mIdleState);
            start();
        }

        void captureObituaryAndQuitNow() {
            // capture StateMachine LogRecs since we will lose them after we call quitNow()
            // This is used for debugging.
            mObituary = new StateMachineObituary(this);

            quitNow();
        }

        @Override
        public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            if (mObituary == null) {
                // StateMachine hasn't quit yet, dump `this` via StateMachineObituary's dump()
                // method for consistency with `else` branch.
                new StateMachineObituary(this).dump(fd, pw, args);
            } else {
                // StateMachine has quit and cleared all LogRecs.
                // Get them from the obituary instead.
                mObituary.dump(fd, pw, args);
            }
        }

        /**
         * Reset this ConcreteClientModeManager when its role changes, so that it can be reused for
         * another purpose.
         */
        private void reset() {
            // Therefore, the caller must ensure that the role change has been completed and these
            // settings have already reset before setting them, otherwise the new setting would be
            // lost.
            setShouldReduceNetworkScore(false);
        }

        private void setRoleInternal(@NonNull RoleChangeInfo roleChangeInfo) {
            mPreviousRole = mRole;
            mLastRoleChangeSinceBootMs = mClock.getElapsedSinceBootMillis();
            mRole = roleChangeInfo.role;
            if (roleChangeInfo.requestorWs != null) {
                mRequestorWs = roleChangeInfo.requestorWs;
            }
            if (roleChangeInfo.modeListener != null) {
                mModeListener = roleChangeInfo.modeListener;
            }
        }

        private void setRoleInternalAndInvokeCallback(@NonNull RoleChangeInfo roleChangeInfo) {
            if (roleChangeInfo.role == mRole) return;
            if (mRole == null) {
                Log.v(getTag(), "ClientModeManager started in role: " + roleChangeInfo);
                setRoleInternal(roleChangeInfo);
                mModeListener.onStarted(ConcreteClientModeManager.this);
            } else {
                Log.v(getTag(), "ClientModeManager role changed: " + roleChangeInfo);
                setRoleInternal(roleChangeInfo);
                reset();
                mModeListener.onRoleChanged(ConcreteClientModeManager.this);
            }
            if (mClientModeImpl != null) {
                mClientModeImpl.onRoleChanged();
            }
        }

        private class IdleState extends State {
            @Override
            public void enter() {
                Log.d(getTag(), "entering IdleState");
                mClientInterfaceName = null;
                mIfaceIsUp = false;
            }

            @Override
            public void exit() {
                // Sometimes the wifi handler thread may become blocked that the statemachine
                // will exit in the IdleState without first entering StartedState. Trigger a
                // cleanup here in case the above sequence happens. This the statemachine was
                // started normally this will will not send a duplicate broadcast since mIsStopped
                // will get set to false the first time the exit happens.
                cleanupOnQuitIfApplicable();
            }

            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_START:
                        // Always start in scan mode first.
                        RoleChangeInfo roleChangeInfo = (RoleChangeInfo) message.obj;
                        mClientInterfaceName = mWifiNative.setupInterfaceForClientInScanMode(
                                mWifiNativeInterfaceCallback, roleChangeInfo.requestorWs);
                        if (TextUtils.isEmpty(mClientInterfaceName)) {
                            Log.e(getTag(), "Failed to create ClientInterface. Sit in Idle");
                            mModeListener.onStartFailure(ConcreteClientModeManager.this);
                            break;
                        }
                        if (roleChangeInfo.role instanceof ClientConnectivityRole) {
                            sendMessage(CMD_SWITCH_TO_CONNECT_MODE, roleChangeInfo);
                            transitionTo(mStartedState);
                        } else {
                            mScanRoleChangeInfoToSetOnTransition = roleChangeInfo;
                            transitionTo(mScanOnlyModeState);
                        }
                        break;
                    default:
                        Log.d(getTag(), "received an invalid message: " + message);
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        private class StartedState extends State {

            private void onUpChanged(boolean isUp) {
                if (isUp == mIfaceIsUp) {
                    return;  // no change
                }
                mIfaceIsUp = isUp;
                if (!isUp) {
                    // if the interface goes down we should exit and go back to idle state.
                    Log.d(getTag(), "interface down!");
                    mStateMachine.sendMessage(CMD_INTERFACE_DOWN);
                }
            }

            @Override
            public void enter() {
                Log.d(getTag(), "entering StartedState");
                mIfaceIsUp = false;
                mIsStopped = false;
                onUpChanged(mWifiNative.isInterfaceUp(mClientInterfaceName));
            }

            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_START:
                        // Already started, ignore this command.
                        break;
                    case CMD_SWITCH_TO_CONNECT_MODE: {
                        RoleChangeInfo roleChangeInfo = (RoleChangeInfo) message.obj;
                        updateConnectModeState(roleChangeInfo.role,
                                WifiManager.WIFI_STATE_ENABLING,
                                WifiManager.WIFI_STATE_DISABLED);
                        if (!mWifiNative.switchClientInterfaceToConnectivityMode(
                                mClientInterfaceName, roleChangeInfo.requestorWs)) {
                            updateConnectModeState(roleChangeInfo.role,
                                    WifiManager.WIFI_STATE_UNKNOWN,
                                    WifiManager.WIFI_STATE_ENABLING);
                            updateConnectModeState(roleChangeInfo.role,
                                    WifiManager.WIFI_STATE_DISABLED,
                                    WifiManager.WIFI_STATE_UNKNOWN);
                            mModeListener.onStartFailure(ConcreteClientModeManager.this);
                            break;
                        }
                        // Role set in the enter of ConnectModeState.
                        mConnectRoleChangeInfoToSetOnTransition = roleChangeInfo;
                        transitionTo(mConnectModeState);
                        break;
                    }
                    case CMD_SWITCH_TO_SCAN_ONLY_MODE:
                        updateConnectModeState(mRole, WifiManager.WIFI_STATE_DISABLING,
                                WifiManager.WIFI_STATE_ENABLED);
                        mDeferStopHandler.start(getWifiOffDeferringTimeMs());
                        break;
                    case CMD_SWITCH_TO_SCAN_ONLY_MODE_CONTINUE: {
                        RoleChangeInfo roleChangeInfo = (RoleChangeInfo) message.obj;
                        mScanRoleChangeInfoToSetOnTransition = roleChangeInfo;
                        transitionTo(mScanOnlyModeState);
                        break;
                    }
                    case CMD_INTERFACE_DOWN:
                        Log.e(getTag(), "Detected an interface down, reporting failure to "
                                + "SelfRecovery");
                        mSelfRecovery.trigger(SelfRecovery.REASON_STA_IFACE_DOWN);
                        // once interface down, nothing else to do...  stop the state machine
                        captureObituaryAndQuitNow();
                        break;
                    case CMD_INTERFACE_STATUS_CHANGED:
                        boolean isUp = message.arg1 == 1;
                        onUpChanged(isUp);
                        break;
                    case CMD_INTERFACE_DESTROYED:
                        Log.e(getTag(), "interface destroyed - client mode stopping");
                        mClientInterfaceName = null;
                        // once interface destroyed, nothing else to do...  stop the state machine
                        captureObituaryAndQuitNow();
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }

            /**
             * Clean up state, unregister listeners and update wifi state.
             */
            @Override
            public void exit() {
                if (mClientInterfaceName != null) {
                    mWifiNative.teardownInterface(mClientInterfaceName);
                    mClientInterfaceName = null;
                    mIfaceIsUp = false;
                }

                Log.i(getTag(), "StartedState#exit(), setting mRole = null");
                mIsStopped = true;
                cleanupOnQuitIfApplicable();
            }
        }

        private class ScanOnlyModeState extends State {
            @Override
            public void enter() {
                Log.d(getTag(), "entering ScanOnlyModeState");

                if (mClientInterfaceName != null) {
                    mScanOnlyModeImpl = mWifiInjector.makeScanOnlyModeImpl(
                            mClientInterfaceName);
                } else {
                    Log.e(getTag(), "Entered ScanOnlyModeState with a null interface name!");
                }

                if (mScanRoleChangeInfoToSetOnTransition == null
                        || (mScanRoleChangeInfoToSetOnTransition.role != ROLE_CLIENT_SCAN_ONLY)) {
                    Log.wtf(TAG, "Unexpected mScanRoleChangeInfoToSetOnTransition: "
                            + mScanRoleChangeInfoToSetOnTransition);
                    // Should never happen, but fallback to scan only to avoid a crash.
                    mScanRoleChangeInfoToSetOnTransition =
                            new RoleChangeInfo(ROLE_CLIENT_SCAN_ONLY);
                }

                setRoleInternalAndInvokeCallback(mScanRoleChangeInfoToSetOnTransition);
                // If we're in ScanOnlyModeState, there is only 1 CMM. So it's ok to call
                // WakeupController directly, there won't be multiple CMMs trampling over each other
                mWakeupController.start();
                mWifiNative.setScanMode(mClientInterfaceName, true);
            }

            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_SWITCH_TO_SCAN_ONLY_MODE:
                        // Already in scan only mode, ignore this command.
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }

            @Override
            public void exit() {
                mScanOnlyModeImpl = null;
                mScanRoleChangeInfoToSetOnTransition = null;

                // If we're in ScanOnlyModeState, there is only 1 CMM. So it's ok to call
                // WakeupController directly, there won't be multiple CMMs trampling over each other
                mWakeupController.stop();
                mWifiNative.setScanMode(mClientInterfaceName, false);
            }
        }

        private class ConnectModeState extends State {
            @Override
            public void enter() {
                Log.d(getTag(), "entering ConnectModeState, starting ClientModeImpl");
                if (mClientInterfaceName == null) {
                    Log.e(getTag(), "Supposed to start ClientModeImpl, but iface is null!");
                } else {
                    if (mClientModeImpl != null) {
                        Log.e(getTag(), "ConnectModeState.enter(): mClientModeImpl is already "
                                + "instantiated?!");
                    }
                    mClientModeImpl = mWifiInjector.makeClientModeImpl(
                            mClientInterfaceName, ConcreteClientModeManager.this,
                            mVerboseLoggingEnabled);
                    mClientModeImpl.setShouldReduceNetworkScore(mShouldReduceNetworkScore);
                }
                if (mConnectRoleChangeInfoToSetOnTransition == null
                        || !(mConnectRoleChangeInfoToSetOnTransition.role
                        instanceof ClientConnectivityRole)) {
                    Log.wtf(TAG, "Unexpected mConnectRoleChangeInfoToSetOnTransition: "
                            + mConnectRoleChangeInfoToSetOnTransition);
                    // Should never happen, but fallback to primary to avoid a crash.
                    mConnectRoleChangeInfoToSetOnTransition =
                            new RoleChangeInfo(ROLE_CLIENT_PRIMARY);
                }

                // Could be any one of possible connect mode roles.
                setRoleInternalAndInvokeCallback(mConnectRoleChangeInfoToSetOnTransition);
                updateConnectModeState(mConnectRoleChangeInfoToSetOnTransition.role,
                        WIFI_STATE_ENABLED, WIFI_STATE_ENABLING);
            }

            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_SWITCH_TO_CONNECT_MODE:
                        RoleChangeInfo roleChangeInfo = (RoleChangeInfo) message.obj;
                        // switching to connect mode when already in connect mode, just update the
                        // requestor WorkSource.
                        boolean success = mWifiNative.replaceStaIfaceRequestorWs(
                                mClientInterfaceName, roleChangeInfo.requestorWs);
                        if (success) {
                            setRoleInternalAndInvokeCallback(roleChangeInfo);
                        } else {
                            // If this call failed, the iface would be torn down.
                            // Thus, simply abort and let the iface down handling take care of the
                            // rest.
                            Log.e(getTag(), "Failed to switch ClientModeManager="
                                    + ConcreteClientModeManager.this + "'s requestorWs");
                        }
                        break;
                    case CMD_SWITCH_TO_SCAN_ONLY_MODE:
                    case CMD_INTERFACE_DESTROYED:
                        updateConnectModeState(mRole, WifiManager.WIFI_STATE_DISABLING,
                                WifiManager.WIFI_STATE_ENABLED);
                        return NOT_HANDLED; // Handled in StartedState.
                    case CMD_INTERFACE_DOWN:
                        updateConnectModeState(mRole, WifiManager.WIFI_STATE_DISABLING,
                                WifiManager.WIFI_STATE_UNKNOWN);
                        return NOT_HANDLED; // Handled in StartedState.
                    case CMD_INTERFACE_STATUS_CHANGED:
                        boolean isUp = message.arg1 == 1;
                        if (isUp == mIfaceIsUp) {
                            break;  // no change
                        }
                        if (!isUp) {
                            if (mWifiGlobals.isConnectedMacRandomizationEnabled()
                                    && getClientMode().isConnecting()) {
                                return HANDLED; // For MAC randomization, ignore...
                            } else {
                                // Handle the error case where our underlying interface went down if
                                // we do not have mac randomization enabled (b/72459123).
                                // if the interface goes down we should exit and go back to idle
                                // state.
                                updateConnectModeState(mRole, WifiManager.WIFI_STATE_UNKNOWN,
                                        WifiManager.WIFI_STATE_ENABLED);
                            }
                        }
                        return NOT_HANDLED; // Handled in StartedState.
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }

            @Override
            public void exit() {
                updateConnectModeState(mRole, WifiManager.WIFI_STATE_DISABLED,
                        WifiManager.WIFI_STATE_DISABLING);

                if (mClientModeImpl == null) {
                    Log.w(getTag(), "ConnectModeState.exit(): mClientModeImpl is already null?!");
                } else {
                    Log.d(getTag(), "Stopping ClientModeImpl");
                    mClientModeImpl.stop();
                    mGraveyard.inter(mClientModeImpl);
                    mClientModeImpl = null;
                }

                mConnectRoleChangeInfoToSetOnTransition = null;
            }
        }
    }

    /** Called by a ClientModeImpl owned by this CMM informing it has fully stopped. */
    public void onClientModeImplQuit() {
        cleanupOnQuitIfApplicable();
    }

    /**
     * Only clean up this CMM once the CMM and all associated ClientModeImpls have been stopped.
     * This is necessary because ClientModeImpl sends broadcasts during stop, and the role must
     * remain primary for {@link ClientModeManagerBroadcastQueue} to send them out.
     */
    private void cleanupOnQuitIfApplicable() {
        if (mIsStopped && mGraveyard.hasAllClientModeImplsQuit()) {
            mPreviousRole = mRole;
            mLastRoleChangeSinceBootMs = mClock.getElapsedSinceBootMillis();
            mRole = null;
            // only call onStopped() after role has been reset to null since ActiveModeWarden
            // expects the CMM to be fully stopped before onStopped().
            mModeListener.onStopped(ConcreteClientModeManager.this);

            // reset to false so that onStopped() won't be triggered again.
            mIsStopped = false;
        }
    }

    @Override
    public int syncGetWifiState() {
        return mWifiState.get();
    }

    @NonNull
    private ClientMode getClientMode() {
        if (mClientModeImpl != null) {
            return mClientModeImpl;
        }
        if (mScanOnlyModeImpl != null) {
            return mScanOnlyModeImpl;
        }
        return mDefaultClientModeManager;
    }

    /*
     * Note: These are simple wrappers over methods to {@link ClientModeImpl}.
     */

    @Override
    public void connectNetwork(NetworkUpdateResult result, ActionListenerWrapper wrapper,
            int callingUid) {
        getClientMode().connectNetwork(result, wrapper, callingUid);
    }

    @Override
    public void saveNetwork(NetworkUpdateResult result, ActionListenerWrapper wrapper,
            int callingUid) {
        getClientMode().saveNetwork(result, wrapper, callingUid);
    }

    @Override
    public void disconnect() {
        getClientMode().disconnect();
    }

    @Override
    public void reconnect(WorkSource ws) {
        getClientMode().reconnect(ws);
    }

    @Override
    public void reassociate() {
        getClientMode().reassociate();
    }

    @Override
    public void startConnectToNetwork(int networkId, int uid, String bssid) {
        getClientMode().startConnectToNetwork(networkId, uid, bssid);
    }

    @Override
    public void startRoamToNetwork(int networkId, String bssid) {
        getClientMode().startRoamToNetwork(networkId, bssid);
    }

    @Override
    public boolean setWifiConnectedNetworkScorer(
            IBinder binder, IWifiConnectedNetworkScorer scorer) {
        return getClientMode().setWifiConnectedNetworkScorer(binder, scorer);
    }

    @Override
    public void clearWifiConnectedNetworkScorer() {
        getClientMode().clearWifiConnectedNetworkScorer();
    }

    @Override
    public void resetSimAuthNetworks(@ClientModeImpl.ResetSimReason int resetReason) {
        getClientMode().resetSimAuthNetworks(resetReason);
    }

    @Override
    public void onBluetoothConnectionStateChanged() {
        getClientMode().onBluetoothConnectionStateChanged();
    }

    @Override
    public WifiInfo syncRequestConnectionInfo() {
        return getClientMode().syncRequestConnectionInfo();
    }

    @Override
    public boolean syncQueryPasspointIcon(long bssid, String fileName) {
        return getClientMode().syncQueryPasspointIcon(bssid, fileName);
    }

    @Override
    public Network syncGetCurrentNetwork() {
        return getClientMode().syncGetCurrentNetwork();
    }

    @Override
    public DhcpResultsParcelable syncGetDhcpResultsParcelable() {
        return getClientMode().syncGetDhcpResultsParcelable();
    }

    @Override
    public long getSupportedFeatures() {
        return getClientMode().getSupportedFeatures();
    }

    @Override
    public boolean syncStartSubscriptionProvisioning(int callingUid, OsuProvider provider,
            IProvisioningCallback callback) {
        return getClientMode().syncStartSubscriptionProvisioning(
                callingUid, provider, callback);
    }

    @Override
    public boolean isWifiStandardSupported(@WifiAnnotations.WifiStandard int standard) {
        return getClientMode().isWifiStandardSupported(standard);
    }

    @Override
    public void enableTdls(String remoteMacAddress, boolean enable) {
        getClientMode().enableTdls(remoteMacAddress, enable);
    }

    @Override
    public void dumpIpClient(FileDescriptor fd, PrintWriter pw, String[] args) {
        getClientMode().dumpIpClient(fd, pw, args);
    }

    @Override
    public void dumpWifiScoreReport(FileDescriptor fd, PrintWriter pw, String[] args) {
        getClientMode().dumpWifiScoreReport(fd, pw, args);
    }

    @Override
    public void enableVerboseLogging(boolean verbose) {
        mVerboseLoggingEnabled = verbose;
        getClientMode().enableVerboseLogging(verbose);
    }

    @Override
    public String getFactoryMacAddress() {
        return getClientMode().getFactoryMacAddress();
    }

    @Override
    public WifiConfiguration getConnectedWifiConfiguration() {
        return getClientMode().getConnectedWifiConfiguration();
    }

    @Override
    public WifiConfiguration getConnectingWifiConfiguration() {
        return getClientMode().getConnectingWifiConfiguration();
    }

    @Override
    public String getConnectedBssid() {
        return getClientMode().getConnectedBssid();
    }

    @Override
    public String getConnectingBssid() {
        return getClientMode().getConnectingBssid();
    }

    @Override
    public WifiLinkLayerStats getWifiLinkLayerStats() {
        return getClientMode().getWifiLinkLayerStats();
    }

    @Override
    public boolean setPowerSave(boolean ps) {
        return getClientMode().setPowerSave(ps);
    }

    @Override
    public boolean setLowLatencyMode(boolean enabled) {
        return getClientMode().setLowLatencyMode(enabled);
    }

    @Override
    public WifiMulticastLockManager.FilterController getMcastLockManagerFilterController() {
        return getClientMode().getMcastLockManagerFilterController();
    }

    @Override
    public boolean isConnected() {
        return getClientMode().isConnected();
    }

    @Override
    public boolean isConnecting() {
        return getClientMode().isConnecting();
    }

    @Override
    public boolean isRoaming() {
        return getClientMode().isRoaming();
    }

    @Override
    public boolean isDisconnected() {
        return getClientMode().isDisconnected();
    }

    @Override
    public boolean isSupplicantTransientState() {
        return getClientMode().isSupplicantTransientState();
    }

    @Override
    public void onCellularConnectivityChanged(@WifiDataStall.CellularDataStatusCode int status) {
        getClientMode().onCellularConnectivityChanged(status);
    }

    @Override
    public void probeLink(LinkProbeCallback callback, int mcs) {
        getClientMode().probeLink(callback, mcs);
    }

    @Override
    public void sendMessageToClientModeImpl(Message msg) {
        getClientMode().sendMessageToClientModeImpl(msg);
    }

    @Override
    public long getId() {
        return mId;
    }

    @Override
    public void setMboCellularDataStatus(boolean available) {
        getClientMode().setMboCellularDataStatus(available);
    }

    @Override
    public WifiNative.RoamingCapabilities getRoamingCapabilities() {
        return getClientMode().getRoamingCapabilities();
    }

    @Override
    public boolean configureRoaming(WifiNative.RoamingConfig config) {
        return getClientMode().configureRoaming(config);
    }

    @Override
    public boolean enableRoaming(boolean enabled) {
        return getClientMode().enableRoaming(enabled);
    }

    @Override
    public boolean setCountryCode(String countryCode) {
        return getClientMode().setCountryCode(countryCode);
    }

    @Override
    public List<TxFateReport> getTxPktFates() {
        return getClientMode().getTxPktFates();
    }

    @Override
    public List<RxFateReport> getRxPktFates() {
        return getClientMode().getRxPktFates();
    }

    @Override
    public DeviceWiphyCapabilities getDeviceWiphyCapabilities() {
        return getClientMode().getDeviceWiphyCapabilities();
    }

    @Override
    public boolean requestAnqp(String bssid, Set<Integer> anqpIds, Set<Integer> hs20Subtypes) {
        return getClientMode().requestAnqp(bssid, anqpIds, hs20Subtypes);
    }

    @Override
    public boolean requestVenueUrlAnqp(String bssid) {
        return getClientMode().requestVenueUrlAnqp(bssid);
    }

    @Override
    public boolean requestIcon(String bssid, String fileName) {
        return getClientMode().requestIcon(bssid, fileName);
    }

    @Override
    public void setShouldReduceNetworkScore(boolean shouldReduceNetworkScore) {
        mShouldReduceNetworkScore = shouldReduceNetworkScore;
        getClientMode().setShouldReduceNetworkScore(shouldReduceNetworkScore);
    }

    @Override
    public String toString() {
        return "ConcreteClientModeManager{id=" + getId()
                + " iface=" + getInterfaceName()
                + " role=" + getRole()
                + "}";
    }
}
