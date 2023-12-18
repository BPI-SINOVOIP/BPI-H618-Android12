/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.car.power;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.ActivityManager;
import android.car.Car;
import android.car.hardware.power.CarPowerManager.CarPowerStateListener;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.car.hardware.power.ICarPower;
import android.car.hardware.power.ICarPowerPolicyListener;
import android.car.hardware.power.ICarPowerStateListener;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.frameworks.automotive.powerpolicy.internal.ICarPowerPolicySystemNotification;
import android.frameworks.automotive.powerpolicy.internal.PolicyState;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReport;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReq;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateShutdownParam;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.AtomicFile;
import android.util.IndentingPrintWriter;
import android.util.SparseArray;

import com.android.car.CarLocalServices;
import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.CarServiceUtils;
import com.android.car.CarStatsLogHelper;
import com.android.car.ICarImpl;
import com.android.car.R;
import com.android.car.am.ContinuousBlankActivity;
import com.android.car.hal.PowerHalService;
import com.android.car.hal.PowerHalService.PowerState;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.user.CarUserNoticeService;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.os.IResultReceiver;
import com.android.internal.util.Preconditions;
import com.android.internal.util.function.pooled.PooledLambda;
import com.android.server.utils.Slogf;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Power Management service class for cars. Controls the power states and interacts with other
 * parts of the system to ensure its own state.
 */
public class CarPowerManagementService extends ICarPower.Stub implements
        CarServiceBase, PowerHalService.PowerEventListener {
    public static final String SILENT_MODE_FORCED_SILENT =
            SilentModeHandler.SILENT_MODE_FORCED_SILENT;
    public static final String SILENT_MODE_FORCED_NON_SILENT =
            SilentModeHandler.SILENT_MODE_FORCED_NON_SILENT;
    public static final String SILENT_MODE_NON_FORCED = SilentModeHandler.SILENT_MODE_NON_FORCED;

    private static final String TAG = CarLog.tagFor(CarPowerManagementService.class);
    private static final String WIFI_STATE_FILENAME = "wifi_state";
    private static final String WIFI_STATE_MODIFIED = "forcibly_disabled";
    private static final String WIFI_STATE_ORIGINAL = "original";
    // If Suspend to RAM fails, we retry with an exponential back-off:
    // The wait interval will be 10 msec, 20 msec, 40 msec, ...
    // Once the wait interval goes beyond 100 msec, it is fixed at 100 msec.
    private static final long INITIAL_SUSPEND_RETRY_INTERVAL_MS = 10;
    private static final long MAX_RETRY_INTERVAL_MS = 100;
    // Minimum and maximum wait duration before the system goes into Suspend to RAM.
    private static final long MIN_SUSPEND_WAIT_DURATION_MS = 0;
    private static final long MAX_SUSPEND_WAIT_DURATION_MS = 3 * 60 * 1000;

    private static final long CAR_POWER_POLICY_DAEMON_FIND_MARGINAL_TIME_MS = 300;
    private static final long CAR_POWER_POLICY_DAEMON_BIND_RETRY_INTERVAL_MS = 500;
    private static final int CAR_POWER_POLICY_DAEMON_BIND_MAX_RETRY = 3;
    private static final String CAR_POWER_POLICY_DAEMON_INTERFACE =
            "carpowerpolicy_system_notification";

    // TODO:  Make this OEM configurable.
    private static final int SHUTDOWN_POLLING_INTERVAL_MS = 2000;
    private static final int SHUTDOWN_EXTEND_MAX_MS = 5000;

    // maxGarageModeRunningDurationInSecs should be equal or greater than this. 15 min for now.
    private static final int MIN_MAX_GARAGE_MODE_DURATION_MS = 15 * 60 * 1000;

    // in secs
    private static final String PROP_MAX_GARAGE_MODE_DURATION_OVERRIDE =
            "android.car.garagemodeduration";

    private final Object mLock = new Object();
    private final Object mSimulationWaitObject = new Object();

    private final Context mContext;
    private final PowerHalService mHal;
    private final SystemInterface mSystemInterface;
    // The listeners that complete simply by returning from onStateChanged()
    private final PowerManagerCallbackList<ICarPowerStateListener> mPowerManagerListeners =
            new PowerManagerCallbackList<>(
                    l -> CarPowerManagementService.this.doUnregisterListener(l));
    // The listeners that must indicate asynchronous completion by calling finished().
    private final PowerManagerCallbackList<ICarPowerStateListener>
            mPowerManagerListenersWithCompletion = new PowerManagerCallbackList<>(
                    l -> CarPowerManagementService.this.doUnregisterListener(l));

    @GuardedBy("mLock")
    private final Set<IBinder> mListenersWeAreWaitingFor = new HashSet<>();
    @GuardedBy("mLock")
    private final LinkedList<CpmsState> mPendingPowerStates = new LinkedList<>();
    private final HandlerThread mHandlerThread = CarServiceUtils.getHandlerThread(
            getClass().getSimpleName());
    private final PowerHandler mHandler = new PowerHandler(mHandlerThread.getLooper(), this);

    private final UserManager mUserManager;
    private final CarUserService mUserService;

    private final WifiManager mWifiManager;
    private final AtomicFile mWifiStateFile;
    private final boolean mWifiAdjustmentForSuspend;

    // This is a temp work-around to reduce user switching delay after wake-up.
    private final boolean mSwitchGuestUserBeforeSleep;

    // CPMS tries to enter Suspend to RAM within the duration specified at
    // mMaxSuspendWaitDurationMs. The default max duration is MAX_SUSPEND_WAIT_DRATION, and can be
    // overridden by setting config_maxSuspendWaitDuration in an overrlay resource.
    // The valid range is MIN_SUSPEND_WAIT_DRATION to MAX_SUSPEND_WAIT_DURATION.
    private final long mMaxSuspendWaitDurationMs;

    @GuardedBy("mSimulationWaitObject")
    private boolean mWakeFromSimulatedSleep;
    @GuardedBy("mSimulationWaitObject")
    private boolean mInSimulatedDeepSleepMode;

    @GuardedBy("mLock")
    private CpmsState mCurrentState;
    @GuardedBy("mLock")
    private Timer mTimer;
    @GuardedBy("mLock")
    private long mProcessingStartTime;
    @GuardedBy("mLock")
    private long mLastSleepEntryTime;

    @GuardedBy("mLock")
    private boolean mTimerActive;
    @GuardedBy("mLock")
    private int mNextWakeupSec;
    @GuardedBy("mLock")
    private boolean mShutdownOnFinish;
    @GuardedBy("mLock")
    private boolean mShutdownOnNextSuspend;
    @GuardedBy("mLock")
    private boolean mIsBooting = true;
    @GuardedBy("mLock")
    private int mShutdownPrepareTimeMs = MIN_MAX_GARAGE_MODE_DURATION_MS;
    @GuardedBy("mLock")
    private int mShutdownPollingIntervalMs = SHUTDOWN_POLLING_INTERVAL_MS;
    @GuardedBy("mLock")
    private boolean mRebootAfterGarageMode;
    @GuardedBy("mLock")
    private boolean mGarageModeShouldExitImmediately;

    @GuardedBy("mLock")
    private ICarPowerPolicySystemNotification mCarPowerPolicyDaemon;
    @GuardedBy("mLock")
    private boolean mConnectionInProgress;
    private BinderHandler mBinderHandler;
    @GuardedBy("mLock")
    private String mCurrentPowerPolicyId;
    @GuardedBy("mLock")
    private String mPendingPowerPolicyId;
    @GuardedBy("mLock")
    private String mCurrentPowerPolicyGroupId;
    @GuardedBy("mLock")
    private boolean mIsPowerPolicyLocked;
    @GuardedBy("mLock")
    private boolean mHasControlOverDaemon;

    @GuardedBy("mLock")
    @Nullable
    private IResultReceiver mFactoryResetCallback;

    private final PowerManagerCallbackList<ICarPowerPolicyListener> mPowerPolicyListeners =
            new PowerManagerCallbackList<>(
                    l -> CarPowerManagementService.this.mPowerPolicyListeners.unregister(l));

    private final PowerComponentHandler mPowerComponentHandler;
    private final PolicyReader mPolicyReader = new PolicyReader();
    private final SilentModeHandler mSilentModeHandler;

    interface ActionOnDeath<T extends IInterface> {
        void take(T listener);
    }

    private final class PowerManagerCallbackList<T extends IInterface> extends
            RemoteCallbackList<T> {
        private ActionOnDeath<T> mActionOnDeath;

        PowerManagerCallbackList(ActionOnDeath<T> action) {
            mActionOnDeath = action;
        }

        /**
         * Old version of {@link #onCallbackDied(E, Object)} that
         * does not provide a cookie.
         */
        @Override
        public void onCallbackDied(T listener) {
            Slogf.i(TAG, "binderDied %s", listener.asBinder());
            mActionOnDeath.take(listener);
        }
    }

    public CarPowerManagementService(Context context, PowerHalService powerHal,
            SystemInterface systemInterface, CarUserService carUserService,
            ICarPowerPolicySystemNotification powerPolicyDaemon) {
        this(context, context.getResources(), powerHal, systemInterface, UserManager.get(context),
                carUserService, powerPolicyDaemon,
                new PowerComponentHandler(context, systemInterface),
                /* silentModeHwStatePath= */ null, /* silentModeKernelStatePath= */ null,
                /* bootReason= */ null);
    }

    @VisibleForTesting
    public CarPowerManagementService(Context context, Resources resources, PowerHalService powerHal,
            SystemInterface systemInterface, UserManager userManager, CarUserService carUserService,
            ICarPowerPolicySystemNotification powerPolicyDaemon,
            PowerComponentHandler powerComponentHandler, @Nullable String silentModeHwStatePath,
            @Nullable String silentModeKernelStatePath, @Nullable String bootReason) {
        mContext = context;
        mHal = powerHal;
        mSystemInterface = systemInterface;
        mUserManager = userManager;
        mShutdownPrepareTimeMs = resources.getInteger(
                R.integer.maxGarageModeRunningDurationInSecs) * 1000;
        mSwitchGuestUserBeforeSleep = resources.getBoolean(
                R.bool.config_switchGuestUserBeforeGoingSleep);
        if (mShutdownPrepareTimeMs < MIN_MAX_GARAGE_MODE_DURATION_MS) {
            Slogf.w(TAG,
                    "maxGarageModeRunningDurationInSecs smaller than minimum required, "
                    + "resource:%d(ms) while should exceed:%d(ms), Ignore resource.",
                    mShutdownPrepareTimeMs, MIN_MAX_GARAGE_MODE_DURATION_MS);
            mShutdownPrepareTimeMs = MIN_MAX_GARAGE_MODE_DURATION_MS;
        }
        mUserService = carUserService;
        mCarPowerPolicyDaemon = powerPolicyDaemon;
        if (powerPolicyDaemon != null) {
            // For testing purpose
            mHasControlOverDaemon = true;
        }
        mWifiManager = context.getSystemService(WifiManager.class);
        mWifiStateFile = new AtomicFile(
                new File(mSystemInterface.getSystemCarDir(), WIFI_STATE_FILENAME));
        mWifiAdjustmentForSuspend = getWifiAdjustmentForSuspendConfig();
        mPowerComponentHandler = powerComponentHandler;
        mSilentModeHandler = new SilentModeHandler(this, silentModeHwStatePath,
                silentModeKernelStatePath, bootReason);
        mMaxSuspendWaitDurationMs = Math.max(MIN_SUSPEND_WAIT_DURATION_MS,
                Math.min(getMaxSuspendWaitDurationConfig(), MAX_SUSPEND_WAIT_DURATION_MS));
    }

    /**
     * Overrides timers to keep testing time short.
     *
     * <p>Passing in {@code 0} resets the value to the default.
     */
    @VisibleForTesting
    public void setShutdownTimersForTest(int pollingIntervalMs, int shutdownTimeoutMs) {
        synchronized (mLock) {
            mShutdownPollingIntervalMs =
                    (pollingIntervalMs == 0) ? SHUTDOWN_POLLING_INTERVAL_MS : pollingIntervalMs;
            mShutdownPrepareTimeMs =
                    (shutdownTimeoutMs == 0) ? SHUTDOWN_EXTEND_MAX_MS : shutdownTimeoutMs;
        }
    }

    @VisibleForTesting
    protected HandlerThread getHandlerThread() {
        return mHandlerThread;
    }

    @Override
    public void init() {
        mPolicyReader.init();
        mPowerComponentHandler.init();
        mHal.setListener(this);
        if (mHal.isPowerStateSupported()) {
            // Initialize CPMS in WAIT_FOR_VHAL state
            onApPowerStateChange(CpmsState.WAIT_FOR_VHAL, CarPowerStateListener.WAIT_FOR_VHAL);
        } else {
            Slogf.w(TAG, "Vehicle hal does not support power state yet.");
            onApPowerStateChange(CpmsState.ON, CarPowerStateListener.ON);
        }
        mSystemInterface.startDisplayStateMonitoring(this);
        connectToPowerPolicyDaemon();
    }

    @Override
    public void release() {
        if (mBinderHandler != null) {
            mBinderHandler.unlinkToDeath();
        }
        synchronized (mLock) {
            releaseTimerLocked();
            mCurrentState = null;
            mCarPowerPolicyDaemon = null;
            mHandler.cancelAll();
            mListenersWeAreWaitingFor.clear();
        }
        mSystemInterface.stopDisplayStateMonitoring();
        mPowerManagerListeners.kill();
        mPowerPolicyListeners.kill();
        mSystemInterface.releaseAllWakeLocks();
    }

    @Override
    public void dump(IndentingPrintWriter writer) {
        synchronized (mLock) {
            writer.println("*PowerManagementService*");
            writer.printf("mCurrentState: %s\n", mCurrentState);
            writer.printf("mProcessingStartTime: %d\n", mProcessingStartTime);
            writer.printf("mLastSleepEntryTime: %d\n", mLastSleepEntryTime);
            writer.printf("mNextWakeupSec: %d\n", mNextWakeupSec);
            writer.printf("mShutdownOnNextSuspend: %b\n", mShutdownOnNextSuspend);
            writer.printf("mShutdownOnFinish: %b\n", mShutdownOnFinish);
            writer.printf("mShutdownPollingIntervalMs: %d\n", mShutdownPollingIntervalMs);
            writer.printf("mShutdownPrepareTimeMs: %d\n", mShutdownPrepareTimeMs);
            writer.printf("mRebootAfterGarageMode: %b\n", mRebootAfterGarageMode);
            writer.printf("mSwitchGuestUserBeforeSleep: %b\n", mSwitchGuestUserBeforeSleep);
            writer.printf("mCurrentPowerPolicyId: %s\n", mCurrentPowerPolicyId);
            writer.printf("mPendingPowerPolicyId: %s\n", mPendingPowerPolicyId);
            writer.printf("mCurrentPowerPolicyGroupId: %s\n", mCurrentPowerPolicyGroupId);
            writer.printf("mIsPowerPolicyLocked: %b\n", mIsPowerPolicyLocked);
            writer.printf("mMaxSuspendWaitDurationMs: %d\n", mMaxSuspendWaitDurationMs);
            writer.printf("config_maxSuspendWaitDuration: %d\n", getMaxSuspendWaitDurationConfig());
            writer.printf("# of power policy change listener: %d\n",
                    mPowerPolicyListeners.getRegisteredCallbackCount());
            writer.printf("mFactoryResetCallback: %s\n", mFactoryResetCallback);
        }
        mPolicyReader.dump(writer);
        mPowerComponentHandler.dump(writer);
        mSilentModeHandler.dump(writer);
    }

    @Override
    public void onApPowerStateChange(PowerState state) {
        synchronized (mLock) {
            mPendingPowerStates.addFirst(new CpmsState(state));
            mLock.notify();
        }
        mHandler.handlePowerStateChange();
    }

    @VisibleForTesting
    void setStateForWakeUp() {
        mSilentModeHandler.init();
        synchronized (mLock) {
            mIsBooting = false;
        }
        handleWaitForVhal(new CpmsState(CpmsState.WAIT_FOR_VHAL,
                CarPowerStateListener.WAIT_FOR_VHAL));
        Slogf.d(TAG, "setStateForTesting(): mIsBooting is set to false and power state is switched "
                + "to Wait For Vhal");
    }

    /**
     * Initiate state change from CPMS directly.
     */
    private void onApPowerStateChange(int apState, int carPowerStateListenerState) {
        CpmsState newState = new CpmsState(apState, carPowerStateListenerState);
        synchronized (mLock) {
            if (newState.mState == CpmsState.WAIT_FOR_FINISH) {
                // We are ready to shut down. Suppress this transition if
                // there is a request to cancel the shutdown (WAIT_FOR_VHAL).
                for (int idx = 0; idx < mPendingPowerStates.size(); idx++) {
                    if (mPendingPowerStates.get(idx).mState == CpmsState.WAIT_FOR_VHAL) {
                        // Completely ignore this WAIT_FOR_FINISH
                        return;
                    }
                }
            }
            mPendingPowerStates.addFirst(newState);
            mLock.notify();
        }
        mHandler.handlePowerStateChange();
    }

    private void doHandlePowerStateChange() {
        CpmsState state;
        synchronized (mLock) {
            state = mPendingPowerStates.peekFirst();
            mPendingPowerStates.clear();
            if (state == null) {
                Slogf.e(TAG, "Null power state was requested");
                return;
            }
            Slogf.i(TAG, "doHandlePowerStateChange: newState=%s", state.name());
            if (!needPowerStateChangeLocked(state)) {
                return;
            }
            // now real power change happens. Whatever was queued before should be all cancelled.
            releaseTimerLocked();
            mCurrentState = state;
        }
        mHandler.cancelProcessingComplete();
        Slogf.i(TAG, "setCurrentState %s", state);
        CarStatsLogHelper.logPowerState(state.mState);
        switch (state.mState) {
            case CpmsState.WAIT_FOR_VHAL:
                handleWaitForVhal(state);
                break;
            case CpmsState.ON:
                handleOn();
                break;
            case CpmsState.SHUTDOWN_PREPARE:
                handleShutdownPrepare(state);
                break;
            case CpmsState.SIMULATE_SLEEP:
                simulateShutdownPrepare();
                break;
            case CpmsState.WAIT_FOR_FINISH:
                handleWaitForFinish(state);
                break;
            case CpmsState.SUSPEND:
                // Received FINISH from VHAL
                handleFinish();
                break;
            default:
                // Illegal state
                // TODO:  Throw exception?
                break;
        }
    }

    private void handleWaitForVhal(CpmsState state) {
        int carPowerStateListenerState = state.mCarPowerStateListenerState;
        // TODO(b/177478420): Restore Wifi, Audio, Location, and Bluetooth, if they are artificially
        // modified for S2R.
        mSilentModeHandler.querySilentModeHwState();
        sendPowerManagerEvent(carPowerStateListenerState);
        // Inspect CarPowerStateListenerState to decide which message to send via VHAL
        switch (carPowerStateListenerState) {
            case CarPowerStateListener.WAIT_FOR_VHAL:
                mHal.sendWaitForVhal();
                break;
            case CarPowerStateListener.SHUTDOWN_CANCELLED:
                mShutdownOnNextSuspend = false; // This cancels the "NextSuspend"
                mHal.sendShutdownCancel();
                break;
            case CarPowerStateListener.SUSPEND_EXIT:
                mHal.sendSleepExit();
                break;
        }
        if (mWifiAdjustmentForSuspend) restoreWifi();
    }

    private void updateCarUserNoticeServiceIfNecessary() {
        try {
            int currentUserId = ActivityManager.getCurrentUser();
            UserInfo currentUserInfo = mUserManager.getUserInfo(currentUserId);
            CarUserNoticeService carUserNoticeService =
                    CarLocalServices.getService(CarUserNoticeService.class);
            if (currentUserInfo != null && currentUserInfo.isGuest()
                    && carUserNoticeService != null) {
                Slogf.i(TAG, "Car user notice service will ignore all messages before user "
                        + "switch.");
                Intent intent = new Intent();
                intent.setComponent(new ComponentName(mContext.getPackageName(),
                        ContinuousBlankActivity.class.getName()));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mContext.startActivityAsUser(intent, UserHandle.CURRENT);
                carUserNoticeService.ignoreUserNotice(currentUserId);
            }
        } catch (Exception e) {
            Slogf.w(TAG, e, "Cannot ignore user notice for current user");
        }
    }

    @VisibleForTesting
    void handleOn() {
        if (factoryResetIfNeeded()) return;

        // If current user is a Guest User, we want to inform CarUserNoticeService not to show
        // notice for current user, and show user notice only for the target user.
        if (!mSwitchGuestUserBeforeSleep) {
            updateCarUserNoticeServiceIfNecessary();
        }

        boolean isPreemptive;
        synchronized (mLock) {
            isPreemptive = mPolicyReader.isPreemptivePowerPolicy(mCurrentPowerPolicyId);
        }
        if (!mSilentModeHandler.isSilentMode() && isPreemptive) {
            cancelPreemptivePowerPolicy();
        } else {
            applyDefaultPowerPolicyForState(VehicleApPowerStateReport.ON,
                    PolicyReader.POWER_POLICY_ID_ALL_ON);
        }

        sendPowerManagerEvent(CarPowerStateListener.ON);

        mHal.sendOn();

        synchronized (mLock) {
            if (mIsBooting) {
                Slogf.d(TAG, "handleOn(): called on boot");
                mIsBooting = false;
                return;
            }
        }

        try {
            mUserService.onResume();
        } catch (Exception e) {
            Slogf.e(TAG, e, "Could not switch user on resume");
        }
    }

    private boolean factoryResetIfNeeded() {
        IResultReceiver callback;
        synchronized (mLock) {
            if (mFactoryResetCallback == null) return false;
            callback = mFactoryResetCallback;
        }

        try {
            Slogf.i(TAG, "Factory resetting as it was delayed by user");
            callback.send(/* resultCode= */ 0, /* resultData= */ null);
            return true;
        } catch (Exception e) {
            Slogf.wtf(TAG, e, "Should have factory reset, but failed");
            return false;
        }
    }

    private void applyDefaultPowerPolicyForState(int state, @Nullable String fallbackPolicyId) {
        CarPowerPolicy policy;
        synchronized (mLock) {
            policy = mPolicyReader
                    .getDefaultPowerPolicyForState(mCurrentPowerPolicyGroupId, state);
        }
        if (policy == null && fallbackPolicyId == null) {
            Slogf.w(TAG, "No default power policy for %s is found",
                    PolicyReader.powerStateToString(state));
            return;
        }
        String policyId = policy == null ? fallbackPolicyId : policy.getPolicyId();
        applyPowerPolicy(policyId, /* upToDaemon= */ true);
    }

    /**
     * Sets the callback used to factory reset the device on resume when the user delayed it.
     */
    public void setFactoryResetCallback(IResultReceiver callback) {
        synchronized (mLock) {
            mFactoryResetCallback = callback;
        }
    }

    /**
     * Tells Garage Mode if it should run normally, or just
     * exit immediately without indicating 'idle'
     * @return True if no idle jobs should be run
     * @hide
     */
    public boolean garageModeShouldExitImmediately() {
        synchronized (mLock) {
            return mGarageModeShouldExitImmediately;
        }
    }

    private void handleShutdownPrepare(CpmsState newState) {
        // Shutdown on finish if the system doesn't support deep sleep or doesn't allow it.
        synchronized (mLock) {
            mShutdownOnFinish = mShutdownOnNextSuspend
                    || !mHal.isDeepSleepAllowed()
                    || !mSystemInterface.isSystemSupportingDeepSleep()
                    || !newState.mCanSleep;
            mGarageModeShouldExitImmediately = !newState.mCanPostpone;
        }
        Slogf.i(TAG,
                (newState.mCanPostpone
                ? "starting shutdown prepare with Garage Mode"
                        : "starting shutdown prepare without Garage Mode"));
        makeSureNoUserInteraction();
        sendPowerManagerEvent(CarPowerStateListener.SHUTDOWN_PREPARE);
        mHal.sendShutdownPrepare();
        doHandlePreprocessing();
    }

    // Simulate system shutdown to Deep Sleep
    private void simulateShutdownPrepare() {
        Slogf.i(TAG, "starting shutdown prepare");
        makeSureNoUserInteraction();
        sendPowerManagerEvent(CarPowerStateListener.SHUTDOWN_PREPARE);
        mHal.sendShutdownPrepare();
        doHandlePreprocessing();
    }

    private void handleWaitForFinish(CpmsState state) {
        sendPowerManagerEvent(state.mCarPowerStateListenerState);
        int wakeupSec;
        synchronized (mLock) {
            // If we're shutting down immediately, don't schedule
            // a wakeup time.
            wakeupSec = mGarageModeShouldExitImmediately ? 0 : mNextWakeupSec;
        }
        switch (state.mCarPowerStateListenerState) {
            case CarPowerStateListener.SUSPEND_ENTER:
                mHal.sendSleepEntry(wakeupSec);
                break;
            case CarPowerStateListener.SHUTDOWN_ENTER:
                mHal.sendShutdownStart(wakeupSec);
                break;
        }
    }

    private void handleFinish() {
        boolean simulatedMode;
        synchronized (mSimulationWaitObject) {
            simulatedMode = mInSimulatedDeepSleepMode;
        }
        boolean mustShutDown;
        boolean forceReboot;
        synchronized (mLock) {
            mustShutDown = mShutdownOnFinish && !simulatedMode;
            forceReboot = mRebootAfterGarageMode;
            mRebootAfterGarageMode = false;
        }
        if (forceReboot) {
            PowerManager powerManager = mContext.getSystemService(PowerManager.class);
            if (powerManager == null) {
                Slogf.wtf(TAG, "No PowerManager. Cannot reboot.");
            } else {
                Slogf.i(TAG, "GarageMode has completed. Forcing reboot.");
                powerManager.reboot("GarageModeReboot");
                throw new AssertionError("Should not return from PowerManager.reboot()");
            }
        }
        // To make Kernel implementation simpler when going into sleep.
        if (mWifiAdjustmentForSuspend) disableWifi();

        if (mustShutDown) {
            // shutdown HU
            mSystemInterface.shutdown();
        } else {
            doHandleDeepSleep(simulatedMode);
        }
        mShutdownOnNextSuspend = false;
    }

    private void restoreWifi() {
        boolean needToRestore = readWifiModifiedState();
        if (needToRestore) {
            if (!mWifiManager.isWifiEnabled()) {
                Slogf.i(TAG, "Wifi has been enabled to restore the last setting");
                mWifiManager.setWifiEnabled(true);
            }
            // Update the persistent data as wifi is not modified by car framework.
            saveWifiModifiedState(false);
        }
    }

    private void disableWifi() {
        boolean wifiEnabled = mWifiManager.isWifiEnabled();
        boolean wifiModifiedState = readWifiModifiedState();
        if (wifiEnabled != wifiModifiedState) {
            saveWifiModifiedState(wifiEnabled);
        }
        if (!wifiEnabled) return;

        mWifiManager.setWifiEnabled(false);
        Slogf.i(TAG, "Wifi has been disabled and the last setting was saved");
    }

    private void saveWifiModifiedState(boolean forciblyDisabled) {
        FileOutputStream fos;
        try {
            fos = mWifiStateFile.startWrite();
        } catch (IOException e) {
            Slogf.e(TAG, e, "Cannot create %s", WIFI_STATE_FILENAME);
            return;
        }

        try (BufferedWriter writer = new BufferedWriter(
                new OutputStreamWriter(fos, StandardCharsets.UTF_8))) {
            writer.write(forciblyDisabled ? WIFI_STATE_MODIFIED : WIFI_STATE_ORIGINAL);
            writer.newLine();
            writer.flush();
            mWifiStateFile.finishWrite(fos);
        } catch (IOException e) {
            mWifiStateFile.failWrite(fos);
            Slogf.e(TAG, e, "Writing %s failed", WIFI_STATE_FILENAME);
        }
    }

    private boolean readWifiModifiedState() {
        boolean needToRestore = false;
        boolean invalidState = false;

        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(mWifiStateFile.openRead(), StandardCharsets.UTF_8))) {
            String line = reader.readLine();
            if (line == null) {
                needToRestore = false;
                invalidState = true;
            } else {
                line = line.trim();
                needToRestore = WIFI_STATE_MODIFIED.equals(line);
                invalidState = !(needToRestore || WIFI_STATE_ORIGINAL.equals(line));
            }
        } catch (IOException e) {
            // If a file named wifi_state doesn't exist, we will not modify Wifi at system start.
            Slogf.w(TAG, "Failed to read %s: %s", WIFI_STATE_FILENAME, e);
            return false;
        }
        if (invalidState) {
            mWifiStateFile.delete();
        }

        return needToRestore;
    }

    @GuardedBy("mLock")
    private void releaseTimerLocked() {
        if (mTimer != null) {
            mTimer.cancel();
        }
        mTimer = null;
        mTimerActive = false;
    }

    private void doHandlePreprocessing() {
        int intervalMs;
        int pollingCount;
        synchronized (mLock) {
            intervalMs = mShutdownPollingIntervalMs;
            pollingCount = (mShutdownPrepareTimeMs / mShutdownPollingIntervalMs) + 1;
        }
        if (Build.IS_USERDEBUG || Build.IS_ENG) {
            int shutdownPrepareTimeOverrideInSecs =
                    SystemProperties.getInt(PROP_MAX_GARAGE_MODE_DURATION_OVERRIDE, -1);
            if (shutdownPrepareTimeOverrideInSecs >= 0) {
                pollingCount =
                        (shutdownPrepareTimeOverrideInSecs * 1000 / intervalMs)
                                + 1;
                Slogf.i(TAG, "Garage mode duration overridden secs: %d",
                        shutdownPrepareTimeOverrideInSecs);
            }
        }
        Slogf.i(TAG, "processing before shutdown expected for: %dms, adding polling:%d",
                mShutdownPrepareTimeMs, pollingCount);
        boolean allAreComplete;
        synchronized (mLock) {
            mProcessingStartTime = SystemClock.elapsedRealtime();
            releaseTimerLocked();
            allAreComplete = mListenersWeAreWaitingFor.isEmpty();
            if (allAreComplete) {
                Slogf.i(TAG, "Listener queue is empty, don't start polling");
            } else {
                mTimer = new Timer();
                mTimerActive = true;
                mTimer.scheduleAtFixedRate(
                        new ShutdownProcessingTimerTask(pollingCount),
                        /* delay= */ 0,
                        intervalMs);
            }
        }
        if (allAreComplete) {
            signalComplete();
        }
        // allowUserSwitch value doesn't matter for onSuspend = true
        mUserService.onSuspend();
    }

    private void sendPowerManagerEvent(int newState) {
        // Broadcast to the listeners that do not signal completion
        notifyListeners(mPowerManagerListeners, newState);

        // SHUTDOWN_PREPARE is the only state where we need
        // to maintain callbacks from listener components.
        boolean allowCompletion = (newState == CarPowerStateListener.SHUTDOWN_PREPARE);

        // Fully populate mListenersWeAreWaitingFor before calling any onStateChanged()
        // for the listeners that signal completion.
        // Otherwise, if the first listener calls finish() synchronously, we will
        // see the list go empty and we will think that we are done.
        boolean haveSomeCompleters = false;
        PowerManagerCallbackList<ICarPowerStateListener> completingListeners =
                new PowerManagerCallbackList(l -> { });
        synchronized (mLock) {
            mListenersWeAreWaitingFor.clear();
            int idx = mPowerManagerListenersWithCompletion.beginBroadcast();
            while (idx-- > 0) {
                ICarPowerStateListener listener =
                        mPowerManagerListenersWithCompletion.getBroadcastItem(idx);
                completingListeners.register(listener);
                if (allowCompletion) {
                    mListenersWeAreWaitingFor.add(listener.asBinder());
                    haveSomeCompleters = true;
                }
            }
            mPowerManagerListenersWithCompletion.finishBroadcast();
        }
        // Broadcast to the listeners that DO signal completion
        notifyListeners(completingListeners, newState);

        if (allowCompletion && !haveSomeCompleters) {
            // No jobs need to signal completion. So we are now complete.
            signalComplete();
        }
    }

    private void notifyListeners(PowerManagerCallbackList<ICarPowerStateListener> listenerList,
            int newState) {
        int idx = listenerList.beginBroadcast();
        while (idx-- > 0) {
            ICarPowerStateListener listener = listenerList.getBroadcastItem(idx);
            try {
                listener.onStateChanged(newState);
            } catch (RemoteException e) {
                // It's likely the connection snapped. Let binder death handle the situation.
                Slogf.e(TAG, e, "onStateChanged() call failed");
            }
        }
        listenerList.finishBroadcast();
    }

    private void doHandleDeepSleep(boolean simulatedMode) {
        int status = applyPreemptivePowerPolicy(PolicyReader.POWER_POLICY_ID_SUSPEND_TO_RAM);
        if (status != PolicyOperationStatus.OK) {
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(status));
        }
        // keep holding partial wakelock to prevent entering sleep before enterDeepSleep call
        // enterDeepSleep should force sleep entry even if wake lock is kept.
        mSystemInterface.switchToPartialWakeLock();
        mHandler.cancelProcessingComplete();
        synchronized (mLock) {
            mLastSleepEntryTime = SystemClock.elapsedRealtime();
        }
        int nextListenerState;
        if (simulatedMode) {
            simulateSleepByWaiting();
            nextListenerState = CarPowerStateListener.SHUTDOWN_CANCELLED;
        } else {
            boolean sleepSucceeded = suspendWithRetries();
            if (!sleepSucceeded) {
                // Suspend failed and we shut down instead.
                // We either won't get here at all or we will power off very soon.
                return;
            }
            // We suspended and have now resumed
            nextListenerState = CarPowerStateListener.SUSPEND_EXIT;
        }
        synchronized (mLock) {
            // Any wakeup time from before is no longer valid.
            mNextWakeupSec = 0;
        }
        Slogf.i(TAG, "Resuming after suspending");
        mSystemInterface.refreshDisplayBrightness();
        onApPowerStateChange(CpmsState.WAIT_FOR_VHAL, nextListenerState);
    }

    private boolean needPowerStateChangeLocked(@NonNull CpmsState newState) {
        if (mCurrentState == null) {
            return true;
        } else if (mCurrentState.equals(newState)) {
            Slogf.d(TAG, "Requested state is already in effect: %s", newState.name());
            return false;
        }

        // The following switch/case enforces the allowed state transitions.
        boolean transitionAllowed = false;
        switch (mCurrentState.mState) {
            case CpmsState.WAIT_FOR_VHAL:
                transitionAllowed = (newState.mState == CpmsState.ON)
                    || (newState.mState == CpmsState.SHUTDOWN_PREPARE);
                break;
            case CpmsState.SUSPEND:
                transitionAllowed =  newState.mState == CpmsState.WAIT_FOR_VHAL;
                break;
            case CpmsState.ON:
                transitionAllowed = (newState.mState == CpmsState.SHUTDOWN_PREPARE)
                    || (newState.mState == CpmsState.SIMULATE_SLEEP);
                break;
            case CpmsState.SHUTDOWN_PREPARE:
                // If VHAL sends SHUTDOWN_IMMEDIATELY or SLEEP_IMMEDIATELY while in
                // SHUTDOWN_PREPARE state, do it.
                transitionAllowed =
                        ((newState.mState == CpmsState.SHUTDOWN_PREPARE) && !newState.mCanPostpone)
                                || (newState.mState == CpmsState.WAIT_FOR_FINISH)
                                || (newState.mState == CpmsState.WAIT_FOR_VHAL);
                break;
            case CpmsState.SIMULATE_SLEEP:
                transitionAllowed = true;
                break;
            case CpmsState.WAIT_FOR_FINISH:
                transitionAllowed = (newState.mState == CpmsState.SUSPEND
                        || newState.mState == CpmsState.WAIT_FOR_VHAL);
                break;
            default:
                Slogf.e(TAG, "Unexpected current state: currentState=%s, newState=%s",
                        mCurrentState.name(), newState.name());
                transitionAllowed = true;
        }
        if (!transitionAllowed) {
            Slogf.e(TAG, "Requested power transition is not allowed: %s --> %s",
                    mCurrentState.name(), newState.name());
        }
        return transitionAllowed;
    }

    private void doHandleProcessingComplete() {
        int listenerState;
        synchronized (mLock) {
            releaseTimerLocked();
            if (!mShutdownOnFinish && mLastSleepEntryTime > mProcessingStartTime) {
                // entered sleep after processing start. So this could be duplicate request.
                Slogf.w(TAG, "Duplicate sleep entry request, ignore");
                return;
            }
            listenerState = mShutdownOnFinish
                    ? CarPowerStateListener.SHUTDOWN_ENTER : CarPowerStateListener.SUSPEND_ENTER;
        }

        onApPowerStateChange(CpmsState.WAIT_FOR_FINISH, listenerState);
    }

    @Override
    public void onDisplayBrightnessChange(int brightness) {
        mHandler.handleDisplayBrightnessChange(brightness);
    }

    private void doHandleDisplayBrightnessChange(int brightness) {
        mSystemInterface.setDisplayBrightness(brightness);
    }

    private void doHandleMainDisplayStateChange(boolean on) {
        Slogf.w(TAG, "Unimplemented:  doHandleMainDisplayStateChange() - on = %b", on);
    }

    /**
     * Handles when a main display changes.
     */
    public void handleMainDisplayChanged(boolean on) {
        mHandler.handleMainDisplayStateChange(on);
    }

    /**
     * Send display brightness to VHAL.
     * @param brightness value 0-100%
     */
    public void sendDisplayBrightness(int brightness) {
        mHal.sendDisplayBrightness(brightness);
    }

    /**
     * Get the PowerHandler that we use to change power states
     */
    public Handler getHandler() {
        return mHandler;

    }

    // Binder interface for general use.
    // The listener is not required (or allowed) to call finished().
    @Override
    public void registerListener(ICarPowerStateListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        mPowerManagerListeners.register(listener);
    }

    // Binder interface for Car services only.
    // After the listener completes its processing, it must call finished().
    @Override
    public void registerListenerWithCompletion(ICarPowerStateListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        ICarImpl.assertCallingFromSystemProcessOrSelf();

        mPowerManagerListenersWithCompletion.register(listener);
        // TODO: Need to send current state to newly registered listener? If so, need to handle
        //       completion for SHUTDOWN_PREPARE state
    }

    @Override
    public void unregisterListener(ICarPowerStateListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        doUnregisterListener(listener);
    }

    @Override
    public void requestShutdownOnNextSuspend() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        synchronized (mLock) {
            mShutdownOnNextSuspend = true;
        }
    }

    @Override
    public void finished(ICarPowerStateListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        ICarImpl.assertCallingFromSystemProcessOrSelf();
        finishedImpl(listener.asBinder());
    }

    @Override
    public void scheduleNextWakeupTime(int seconds) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        if (seconds < 0) {
            Slogf.w(TAG, "Next wake up time is negative. Ignoring!");
            return;
        }
        boolean timedWakeupAllowed = mHal.isTimedWakeupAllowed();
        synchronized (mLock) {
            if (!timedWakeupAllowed) {
                Slogf.w(TAG, "Setting timed wakeups are disabled in HAL. Skipping");
                mNextWakeupSec = 0;
                return;
            }
            if (mNextWakeupSec == 0 || mNextWakeupSec > seconds) {
                // The new value is sooner than the old value. Take the new value.
                mNextWakeupSec = seconds;
            } else {
                Slogf.d(TAG, "Tried to schedule next wake up, but already had shorter "
                        + "scheduled time");
            }
        }
    }

    @Override
    public int getPowerState() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        synchronized (mLock) {
            return (mCurrentState == null) ? CarPowerStateListener.INVALID
                    : mCurrentState.mCarPowerStateListenerState;
        }
    }

    /**
     * @see android.car.hardware.power.CarPowerManager#getCurrentPowerPolicy
     */
    @Override
    public CarPowerPolicy getCurrentPowerPolicy() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_READ_CAR_POWER_POLICY);
        return mPowerComponentHandler.getAccumulatedPolicy();
    }

    /**
     * @see android.car.hardware.power.CarPowerManager#applyPowerPolicy
     */
    @Override
    public void applyPowerPolicy(String policyId) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CONTROL_CAR_POWER_POLICY);
        Preconditions.checkArgument(policyId != null, "policyId cannot be null");
        Preconditions.checkArgument(!policyId.startsWith(PolicyReader.SYSTEM_POWER_POLICY_PREFIX),
                "System power policy cannot be applied by apps");
        int status = applyPowerPolicy(policyId, true);
        if (status != PolicyOperationStatus.OK) {
            throw new IllegalArgumentException(PolicyOperationStatus.errorCodeToString(status));
        }
    }

    /**
     * @see android.car.hardware.power.CarPowerManager#setPowerPolicyGroup
     */
    @Override
    public void setPowerPolicyGroup(String policyGroupId) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CONTROL_CAR_POWER_POLICY);
        Preconditions.checkArgument(policyGroupId != null, "policyGroupId cannot be null");
        int status = setCurrentPowerPolicyGroup(policyGroupId);
        if (status != PolicyOperationStatus.OK) {
            throw new IllegalArgumentException(PolicyOperationStatus.errorCodeToString(status));
        }
    }

    /**
     * @see android.car.hardware.power.CarPowerManager#addPowerPolicyListener
     */
    @Override
    public void addPowerPolicyListener(CarPowerPolicyFilter filter,
            ICarPowerPolicyListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_READ_CAR_POWER_POLICY);
        mPowerPolicyListeners.register(listener, filter);
    }

    /**
     * @see android.car.hardware.power.CarPowerManager#removePowerPolicyListener
     */
    @Override
    public void removePowerPolicyListener(ICarPowerPolicyListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_READ_CAR_POWER_POLICY);
        mPowerPolicyListeners.unregister(listener);
    }

    void notifySilentModeChange(boolean silent) {
        Slogf.i(TAG, "Silent mode is set to %b", silent);
        if (silent) {
            applyPreemptivePowerPolicy(PolicyReader.POWER_POLICY_ID_NO_USER_INTERACTION);
        } else {
            cancelPreemptivePowerPolicy();
        }
    }

    private void doUnregisterListener(ICarPowerStateListener listener) {
        mPowerManagerListeners.unregister(listener);
        boolean found = mPowerManagerListenersWithCompletion.unregister(listener);
        if (found) {
            // Remove this from the completion list (if it's there)
            finishedImpl(listener.asBinder());
        }
    }

    private void finishedImpl(IBinder binder) {
        boolean allAreComplete;
        synchronized (mLock) {
            mListenersWeAreWaitingFor.remove(binder);
            allAreComplete = mListenersWeAreWaitingFor.isEmpty();
        }
        if (allAreComplete) {
            signalComplete();
        }
    }

    private void signalComplete() {
        if (mCurrentState.mState == CpmsState.SHUTDOWN_PREPARE
                || mCurrentState.mState == CpmsState.SIMULATE_SLEEP) {
            PowerHandler powerHandler;
            // All apps are ready to shutdown/suspend.
            synchronized (mLock) {
                if (!mShutdownOnFinish) {
                    if (mLastSleepEntryTime > mProcessingStartTime
                            && mLastSleepEntryTime < SystemClock.elapsedRealtime()) {
                        Slogf.i(TAG, "signalComplete: Already slept!");
                        return;
                    }
                }
                powerHandler = mHandler;
            }
            Slogf.i(TAG, "Apps are finished, call handleProcessingComplete()");
            powerHandler.handleProcessingComplete();
        }
    }

    private void initializePowerPolicy() {
        Slogf.i(TAG, "CPMS is taking control from carpowerpolicyd");
        ICarPowerPolicySystemNotification daemon;
        synchronized (mLock) {
            daemon = mCarPowerPolicyDaemon;
        }
        PolicyState state;
        if (daemon != null) {
            try {
                state = daemon.notifyCarServiceReady();
            } catch (RemoteException e) {
                Slogf.e(TAG, e, "Failed to tell car power policy daemon that CarService is ready");
                return;
            }
        } else {
            Slogf.w(TAG, "Failed to notify car service is ready. car power policy daemon is not "
                    + "available");
            return;
        }

        String currentPowerPolicyId;
        String currentPolicyGroupId;
        synchronized (mLock) {
            mHasControlOverDaemon = true;
            currentPowerPolicyId = mCurrentPowerPolicyId;
            currentPolicyGroupId = mCurrentPowerPolicyGroupId;
        }
        // If the current power policy or the policy group has been modified by CPMS, we ignore
        // the power policy or the policy group passed from car power policy daemon, and notifies
        // the current power policy to the daemon.
        if (currentPowerPolicyId == null || currentPowerPolicyId.isEmpty()) {
            int status = applyPowerPolicy(state.policyId, false);
            if (status != PolicyOperationStatus.OK) {
                Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(status));
            }
        } else {
            notifyPowerPolicyChangeToDaemon(currentPowerPolicyId);
        }
        if (currentPolicyGroupId == null || currentPolicyGroupId.isEmpty()) {
            int status = setCurrentPowerPolicyGroup(state.policyGroupId);
            if (status != PolicyOperationStatus.OK) {
                Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(status));
            }
        }
        mSilentModeHandler.init();
    }

    @PolicyOperationStatus.ErrorCode
    private int setCurrentPowerPolicyGroup(String policyGroupId) {
        if (!mPolicyReader.isPowerPolicyGroupAvailable(policyGroupId)) {
            int error = PolicyOperationStatus.ERROR_SET_POWER_POLICY_GROUP;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error,
                    policyGroupId + " is not registered"));
            return error;
        }
        synchronized (mLock) {
            mCurrentPowerPolicyGroupId = policyGroupId;
        }
        return PolicyOperationStatus.OK;
    }

    @PolicyOperationStatus.ErrorCode
    private int applyPowerPolicy(@Nullable String policyId, boolean upToDaemon) {
        CarPowerPolicy policy = mPolicyReader.getPowerPolicy(policyId);
        if (policy == null) {
            int error = PolicyOperationStatus.ERROR_APPLY_POWER_POLICY;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, policyId));
            return error;
        }
        synchronized (mLock) {
            if (mIsPowerPolicyLocked) {
                Slogf.i(TAG, "Power policy is locked. The request policy(%s) will be applied when "
                        + "power policy becomes unlocked", policyId);
                mPendingPowerPolicyId = policyId;
                return PolicyOperationStatus.OK;
            }
            mCurrentPowerPolicyId = policyId;
        }
        mPowerComponentHandler.applyPowerPolicy(policy);
        notifyPowerPolicyChange(policyId, upToDaemon);
        Slogf.i(TAG, "The current power policy is %s", policyId);
        return PolicyOperationStatus.OK;
    }

    @PolicyOperationStatus.ErrorCode
    private int applyPreemptivePowerPolicy(String policyId) {
        CarPowerPolicy policy = mPolicyReader.getPreemptivePowerPolicy(policyId);
        if (policy == null) {
            int error = PolicyOperationStatus.ERROR_NOT_REGISTERED_POWER_POLICY_ID;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, policyId));
            return error;
        }
        synchronized (mLock) {
            mIsPowerPolicyLocked = true;
            if (!mPolicyReader.isPreemptivePowerPolicy(mCurrentPowerPolicyId)) {
                mPendingPowerPolicyId = mCurrentPowerPolicyId;
            }
            mCurrentPowerPolicyId = policyId;
        }
        mPowerComponentHandler.applyPowerPolicy(policy);
        notifyPowerPolicyChange(policyId, true);
        Slogf.i(TAG, "The current power policy is %s", policyId);
        return PolicyOperationStatus.OK;
    }

    private void cancelPreemptivePowerPolicy() {
        String policyId;
        synchronized (mLock) {
            if (!mIsPowerPolicyLocked) {
                Slogf.w(TAG, "Failed to cancel system power policy: the current policy is not the "
                        + "system power policy");
                return;
            }
            mIsPowerPolicyLocked = false;
            policyId = mPendingPowerPolicyId;
            mPendingPowerPolicyId = null;
        }
        int status = applyPowerPolicy(policyId, true);
        if (status != PolicyOperationStatus.OK) {
            Slogf.w(TAG, "Failed to cancel system power policy: %s",
                    PolicyOperationStatus.errorCodeToString(status));
        }
    }

    private void notifyPowerPolicyChangeToDaemon(String policyId) {
        ICarPowerPolicySystemNotification daemon;
        boolean hadPendingPolicyNotification;
        synchronized (mLock) {
            daemon = mCarPowerPolicyDaemon;
            if (daemon == null) {
                Slogf.e(TAG, "Failed to notify car power policy daemon: the daemon is not ready");
                return;
            }
            if (!mHasControlOverDaemon) {
                Slogf.w(TAG, "Notifying policy change is deferred: CPMS has not yet taken control");
                return;
            }
        }
        try {
            daemon.notifyPowerPolicyChange(policyId);
        } catch (RemoteException | IllegalStateException e) {
            Slogf.e(TAG, e, "Failed to notify car power policy daemon of a new power policy(%s)",
                    policyId);
        }
    }

    private void notifyPowerPolicyChange(String policyId, boolean upToDaemon) {
        // Notify system clients
        if (upToDaemon) {
            notifyPowerPolicyChangeToDaemon(policyId);
        }

        // Notify Java clients
        CarPowerPolicy accumulatedPolicy = mPowerComponentHandler.getAccumulatedPolicy();
        CarPowerPolicy appliedPolicy = mPolicyReader.isPreemptivePowerPolicy(policyId)
                ? mPolicyReader.getPreemptivePowerPolicy(policyId)
                : mPolicyReader.getPowerPolicy(policyId);
        if (appliedPolicy == null) {
            Slogf.wtf(TAG, "The new power policy(%s) should exist", policyId);
        }
        int idx = mPowerPolicyListeners.beginBroadcast();
        while (idx-- > 0) {
            ICarPowerPolicyListener listener = mPowerPolicyListeners.getBroadcastItem(idx);
            CarPowerPolicyFilter filter =
                    (CarPowerPolicyFilter) mPowerPolicyListeners.getBroadcastCookie(idx);
            if (!mPowerComponentHandler.isComponentChanged(filter)) {
                continue;
            }
            try {
                listener.onPolicyChanged(appliedPolicy, accumulatedPolicy);
            } catch (RemoteException e) {
                // It's likely the connection snapped. Let binder death handle the situation.
                Slogf.e(TAG, e, "onPolicyChanged() call failed: policyId = %s", policyId);
            }
        }
        mPowerPolicyListeners.finishBroadcast();
    }

    private void makeSureNoUserInteraction() {
        mSilentModeHandler.updateKernelSilentMode(true);
        int status = applyPreemptivePowerPolicy(PolicyReader.POWER_POLICY_ID_NO_USER_INTERACTION);
        if (status != PolicyOperationStatus.OK) {
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(status));
        }
    }

    private void connectToPowerPolicyDaemon() {
        synchronized (mLock) {
            if (mCarPowerPolicyDaemon != null || mConnectionInProgress) {
                return;
            }
            mConnectionInProgress = true;
        }
        connectToDaemonHelper(CAR_POWER_POLICY_DAEMON_BIND_MAX_RETRY);
    }

    private void connectToDaemonHelper(int retryCount) {
        if (retryCount <= 0) {
            synchronized (mLock) {
                mConnectionInProgress = false;
            }
            Slogf.e(TAG, "Cannot reconnect to car power policyd daemon after retrying %d times",
                    CAR_POWER_POLICY_DAEMON_BIND_MAX_RETRY);
            return;
        }
        if (makeBinderConnection()) {
            Slogf.i(TAG, "Connected to car power policy daemon");
            initializePowerPolicy();
            return;
        }
        mHandler.sendMessageDelayed(PooledLambda.obtainMessage(
                CarPowerManagementService::connectToDaemonHelper,
                CarPowerManagementService.this, retryCount - 1),
                CAR_POWER_POLICY_DAEMON_BIND_RETRY_INTERVAL_MS);
    }

    private boolean makeBinderConnection() {
        long currentTimeMs = SystemClock.uptimeMillis();
        IBinder binder = ServiceManager.getService(CAR_POWER_POLICY_DAEMON_INTERFACE);
        if (binder == null) {
            Slogf.w(TAG, "Finding car power policy daemon failed. Power policy management is not "
                    + "supported");
            return false;
        }
        long elapsedTimeMs = SystemClock.uptimeMillis() - currentTimeMs;
        if (elapsedTimeMs > CAR_POWER_POLICY_DAEMON_FIND_MARGINAL_TIME_MS) {
            Slogf.wtf(TAG, "Finding car power policy daemon took too long(%dms)", elapsedTimeMs);
        }

        ICarPowerPolicySystemNotification daemon =
                ICarPowerPolicySystemNotification.Stub.asInterface(binder);
        if (daemon == null) {
            Slogf.w(TAG, "Getting car power policy daemon interface failed. Power policy management"
                    + " is not supported");
            return false;
        }
        synchronized (mLock) {
            mCarPowerPolicyDaemon = daemon;
            mConnectionInProgress = false;
        }
        mBinderHandler = new BinderHandler(daemon);
        mBinderHandler.linkToDeath();
        return true;
    }

    private final class BinderHandler implements IBinder.DeathRecipient {
        private ICarPowerPolicySystemNotification mDaemon;

        private BinderHandler(ICarPowerPolicySystemNotification daemon) {
            mDaemon = daemon;
        }

        @Override
        public void binderDied() {
            Slogf.w(TAG, "Car power policy daemon died: reconnecting");
            unlinkToDeath();
            mDaemon = null;
            synchronized (mLock) {
                mCarPowerPolicyDaemon = null;
                mHasControlOverDaemon = false;
            }
            mHandler.sendMessageDelayed(PooledLambda.obtainMessage(
                    CarPowerManagementService::connectToDaemonHelper,
                    CarPowerManagementService.this, CAR_POWER_POLICY_DAEMON_BIND_MAX_RETRY),
                    CAR_POWER_POLICY_DAEMON_BIND_RETRY_INTERVAL_MS);
        }

        private void linkToDeath() {
            if (mDaemon == null) {
                return;
            }
            IBinder binder = mDaemon.asBinder();
            if (binder == null) {
                Slogf.w(TAG, "Linking to binder death recipient skipped");
                return;
            }
            try {
                binder.linkToDeath(this, 0);
            } catch (RemoteException e) {
                mDaemon = null;
                Slogf.w(TAG, e, "Linking to binder death recipient failed: %s");
            }
        }

        private void unlinkToDeath() {
            if (mDaemon == null) {
                return;
            }
            IBinder binder = mDaemon.asBinder();
            if (binder == null) {
                Slogf.w(TAG, "Unlinking from binder death recipient skipped");
                return;
            }
            binder.unlinkToDeath(this, 0);
        }
    }

    private static final class PowerHandler extends Handler {
        private static final String TAG = PowerHandler.class.getSimpleName();
        private static final int MSG_POWER_STATE_CHANGE = 0;
        private static final int MSG_DISPLAY_BRIGHTNESS_CHANGE = 1;
        private static final int MSG_MAIN_DISPLAY_STATE_CHANGE = 2;
        private static final int MSG_PROCESSING_COMPLETE = 3;

        // Do not handle this immediately but with some delay as there can be a race between
        // display off due to rear view camera and delivery to here.
        private static final long MAIN_DISPLAY_EVENT_DELAY_MS = 500;

        private final WeakReference<CarPowerManagementService> mService;

        private PowerHandler(Looper looper, CarPowerManagementService service) {
            super(looper);
            mService = new WeakReference<CarPowerManagementService>(service);
        }

        private void handlePowerStateChange() {
            Message msg = obtainMessage(MSG_POWER_STATE_CHANGE);
            sendMessage(msg);
        }

        private void handleDisplayBrightnessChange(int brightness) {
            Message msg = obtainMessage(MSG_DISPLAY_BRIGHTNESS_CHANGE, brightness, 0);
            sendMessage(msg);
        }

        private void handleMainDisplayStateChange(boolean on) {
            removeMessages(MSG_MAIN_DISPLAY_STATE_CHANGE);
            Message msg = obtainMessage(MSG_MAIN_DISPLAY_STATE_CHANGE, Boolean.valueOf(on));
            sendMessageDelayed(msg, MAIN_DISPLAY_EVENT_DELAY_MS);
        }

        private void handleProcessingComplete() {
            removeMessages(MSG_PROCESSING_COMPLETE);
            Message msg = obtainMessage(MSG_PROCESSING_COMPLETE);
            sendMessage(msg);
        }

        private void cancelProcessingComplete() {
            removeMessages(MSG_PROCESSING_COMPLETE);
        }

        private void cancelAll() {
            removeMessages(MSG_POWER_STATE_CHANGE);
            removeMessages(MSG_DISPLAY_BRIGHTNESS_CHANGE);
            removeMessages(MSG_MAIN_DISPLAY_STATE_CHANGE);
            removeMessages(MSG_PROCESSING_COMPLETE);
        }

        @Override
        public void handleMessage(Message msg) {
            CarPowerManagementService service = mService.get();
            if (service == null) {
                Slogf.i(TAG, "handleMessage null service");
                return;
            }
            switch (msg.what) {
                case MSG_POWER_STATE_CHANGE:
                    service.doHandlePowerStateChange();
                    break;
                case MSG_DISPLAY_BRIGHTNESS_CHANGE:
                    service.doHandleDisplayBrightnessChange(msg.arg1);
                    break;
                case MSG_MAIN_DISPLAY_STATE_CHANGE:
                    service.doHandleMainDisplayStateChange((Boolean) msg.obj);
                    break;
                case MSG_PROCESSING_COMPLETE:
                    service.doHandleProcessingComplete();
                    break;
            }
        }
    }

    private class ShutdownProcessingTimerTask extends TimerTask {
        private final int mExpirationCount;
        private int mCurrentCount;

        private ShutdownProcessingTimerTask(int expirationCount) {
            mExpirationCount = expirationCount;
            mCurrentCount = 0;
        }

        @Override
        public void run() {
            synchronized (mLock) {
                if (!mTimerActive) {
                    // Ignore timer expiration since we got cancelled
                    return;
                }
                mCurrentCount++;
                if (mCurrentCount > mExpirationCount) {
                    PowerHandler handler;
                    releaseTimerLocked();
                    handler = mHandler;
                    handler.handleProcessingComplete();
                } else {
                    mHal.sendShutdownPostpone(SHUTDOWN_EXTEND_MAX_MS);
                }
            }
        }
    }

    // Send the command to enter Suspend to RAM.
    // If the command is not successful, try again with an exponential back-off.
    // If it fails repeatedly, send the command to shut down.
    // If we decide to go to a different power state, abort this retry mechanism.
    // Returns true if we successfully suspended.
    private boolean suspendWithRetries() {
        long retryIntervalMs = INITIAL_SUSPEND_RETRY_INTERVAL_MS;
        long totalWaitDurationMs = 0;

        while (true) {
            Slogf.i(TAG, "Entering Suspend to RAM");
            boolean suspendSucceeded = mSystemInterface.enterDeepSleep();
            if (suspendSucceeded) {
                return true;
            }
            if (totalWaitDurationMs >= mMaxSuspendWaitDurationMs) {
                break;
            }
            // We failed to suspend. Block the thread briefly and try again.
            synchronized (mLock) {
                if (mPendingPowerStates.isEmpty()) {
                    Slogf.w(TAG, "Failed to Suspend; will retry after %dms", retryIntervalMs);
                    try {
                        mLock.wait(retryIntervalMs);
                    } catch (InterruptedException ignored) {
                        Thread.currentThread().interrupt();
                    }
                    totalWaitDurationMs += retryIntervalMs;
                    retryIntervalMs = Math.min(retryIntervalMs * 2, MAX_RETRY_INTERVAL_MS);
                }
                // Check for a new power state now, before going around the loop again
                if (!mPendingPowerStates.isEmpty()) {
                    Slogf.i(TAG, "Terminating the attempt to Suspend to RAM");
                    return false;
                }
            }
        }
        // Too many failures trying to suspend. Shut down.
        Slogf.w(TAG, "Could not Suspend to RAM after %dms long trial. Shutting down.",
                totalWaitDurationMs);
        mSystemInterface.shutdown();
        return false;
    }

    private static class CpmsState {
        // NOTE: When modifying states below, make sure to update CarPowerStateChanged.State in
        //   frameworks/proto_logging/stats/atoms.proto also.
        public static final int WAIT_FOR_VHAL = 0;
        public static final int ON = 1;
        public static final int SHUTDOWN_PREPARE = 2;
        public static final int WAIT_FOR_FINISH = 3;
        public static final int SUSPEND = 4;
        public static final int SIMULATE_SLEEP = 5;

        /* Config values from AP_POWER_STATE_REQ */
        public final boolean mCanPostpone;
        public final boolean mCanSleep;
        /* Message sent to CarPowerStateListener in response to this state */
        public final int mCarPowerStateListenerState;
        /* One of the above state variables */
        public final int mState;

        /**
          * This constructor takes a PowerHalService.PowerState object and creates the corresponding
          * CPMS state from it.
          */
        CpmsState(PowerState halPowerState) {
            switch (halPowerState.mState) {
                case VehicleApPowerStateReq.ON:
                    this.mCanPostpone = false;
                    this.mCanSleep = false;
                    this.mCarPowerStateListenerState = cpmsStateToPowerStateListenerState(ON);
                    this.mState = ON;
                    break;
                case VehicleApPowerStateReq.SHUTDOWN_PREPARE:
                    this.mCanPostpone = halPowerState.canPostponeShutdown();
                    this.mCanSleep = halPowerState.canEnterDeepSleep();
                    this.mCarPowerStateListenerState = cpmsStateToPowerStateListenerState(
                            SHUTDOWN_PREPARE);
                    this.mState = SHUTDOWN_PREPARE;
                    break;
                case VehicleApPowerStateReq.CANCEL_SHUTDOWN:
                    this.mCanPostpone = false;
                    this.mCanSleep = false;
                    this.mCarPowerStateListenerState = CarPowerStateListener.SHUTDOWN_CANCELLED;
                    this.mState = WAIT_FOR_VHAL;
                    break;
                case VehicleApPowerStateReq.FINISHED:
                    this.mCanPostpone = false;
                    this.mCanSleep = false;
                    this.mCarPowerStateListenerState = cpmsStateToPowerStateListenerState(SUSPEND);
                    this.mState = SUSPEND;
                    break;
                default:
                    // Illegal state from PowerState.  Throw an exception?
                    this.mCanPostpone = false;
                    this.mCanSleep = false;
                    this.mCarPowerStateListenerState = 0;
                    this.mState = 0;
                    break;
            }
        }

        CpmsState(int state, int carPowerStateListenerState) {
            this.mCanPostpone = (state == SIMULATE_SLEEP);
            this.mCanSleep = (state == SIMULATE_SLEEP);
            this.mCarPowerStateListenerState = carPowerStateListenerState;
            this.mState = state;
        }

        public String name() {
            String baseName;
            switch(mState) {
                case WAIT_FOR_VHAL:     baseName = "WAIT_FOR_VHAL";    break;
                case ON:                baseName = "ON";               break;
                case SHUTDOWN_PREPARE:  baseName = "SHUTDOWN_PREPARE"; break;
                case WAIT_FOR_FINISH:   baseName = "WAIT_FOR_FINISH";  break;
                case SUSPEND:           baseName = "SUSPEND";          break;
                case SIMULATE_SLEEP:    baseName = "SIMULATE_SLEEP";   break;
                default:                baseName = "<unknown>";        break;
            }
            return baseName + "(" + mState + ")";
        }

        private static int cpmsStateToPowerStateListenerState(int state) {
            int powerStateListenerState = 0;

            // Set the CarPowerStateListenerState based on current state
            switch (state) {
                case ON:
                    powerStateListenerState = CarPowerStateListener.ON;
                    break;
                case SHUTDOWN_PREPARE:
                    powerStateListenerState = CarPowerStateListener.SHUTDOWN_PREPARE;
                    break;
                case SUSPEND:
                    powerStateListenerState = CarPowerStateListener.SUSPEND_ENTER;
                    break;
                case WAIT_FOR_VHAL:
                case WAIT_FOR_FINISH:
                default:
                    // Illegal state for this constructor.  Throw an exception?
                    break;
            }
            return powerStateListenerState;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof CpmsState)) {
                return false;
            }
            CpmsState that = (CpmsState) o;
            return this.mState == that.mState
                    && this.mCanSleep == that.mCanSleep
                    && this.mCanPostpone == that.mCanPostpone
                    && this.mCarPowerStateListenerState == that.mCarPowerStateListenerState;
        }

        @Override
        public String toString() {
            return "CpmsState canSleep:" + mCanSleep + ", canPostpone=" + mCanPostpone
                    + ", carPowerStateListenerState=" + mCarPowerStateListenerState
                    + ", CpmsState=" + this.name();
        }
    }

    /**
     * Resume after a manually-invoked suspend.
     * Invoked using "adb shell dumpsys activity service com.android.car resume".
     */
    public void forceSimulatedResume() {
        PowerHandler handler;
        synchronized (mLock) {
            // Cancel Garage Mode in case it's running
            mPendingPowerStates.addFirst(new CpmsState(CpmsState.WAIT_FOR_VHAL,
                                                       CarPowerStateListener.SHUTDOWN_CANCELLED));
            mLock.notify();
            handler = mHandler;
        }
        handler.handlePowerStateChange();

        synchronized (mSimulationWaitObject) {
            mWakeFromSimulatedSleep = true;
            mSimulationWaitObject.notify();
        }
    }

    /**
     * Manually enter simulated suspend (Deep Sleep) mode, trigging Garage mode.
     * If the parameter is 'true', reboot the system when Garage Mode completes.
     *
     * Invoked using "adb shell dumpsys activity service com.android.car suspend" or
     * "adb shell dumpsys activity service com.android.car garage-mode reboot".
     * This is similar to 'onApPowerStateChange()' except that it needs to create a CpmsState
     * that is not directly derived from a VehicleApPowerStateReq.
     */
    public void forceSuspendAndMaybeReboot(boolean shouldReboot) {
        synchronized (mSimulationWaitObject) {
            mInSimulatedDeepSleepMode = true;
            mWakeFromSimulatedSleep = false;
            mGarageModeShouldExitImmediately = false;
        }
        PowerHandler handler;
        synchronized (mLock) {
            mRebootAfterGarageMode = shouldReboot;
            mPendingPowerStates.addFirst(new CpmsState(CpmsState.SIMULATE_SLEEP,
                                                       CarPowerStateListener.SHUTDOWN_PREPARE));
            handler = mHandler;
        }
        handler.handlePowerStateChange();
    }

    /**
     * Manually defines a power policy.
     *
     * <p>If the given ID already exists or specified power components are invalid, it fails.
     *
     * @return {@code true}, if successful. Otherwise, {@code false}.
     */
    public boolean definePowerPolicyFromCommand(String[] args, IndentingPrintWriter writer) {
        if (args.length < 2) {
            writer.println("Too few arguments");
            return false;
        }
        String powerPolicyId = args[1];
        int index = 2;
        String[] enabledComponents = new String[0];
        String[] disabledComponents = new String[0];
        while (index < args.length) {
            switch (args[index]) {
                case "--enable":
                    if (index == args.length - 1) {
                        writer.println("No components for --enable");
                        return false;
                    }
                    enabledComponents = args[index + 1].split(",");
                    break;
                case "--disable":
                    if (index == args.length - 1) {
                        writer.println("No components for --disabled");
                        return false;
                    }
                    disabledComponents = args[index + 1].split(",");
                    break;
                default:
                    writer.printf("Unrecognized argument: %s\n", args[index]);
                    return false;
            }
            index += 2;
        }
        int status = definePowerPolicy(powerPolicyId, enabledComponents, disabledComponents);
        if (status != PolicyOperationStatus.OK) {
            writer.println(PolicyOperationStatus.errorCodeToString(status));
            return false;
        }
        writer.printf("Power policy(%s) is successfully defined.\n", powerPolicyId);
        return true;
    }

    /**
     * Defines a power policy with the given id and components.
     *
     * <p> A policy defined with this method is valid until the system is rebooted/restarted.
     */
    @VisibleForTesting
    @PolicyOperationStatus.ErrorCode
    public int definePowerPolicy(String powerPolicyId, String[] enabledComponents,
            String[] disabledComponents) {
        int status = mPolicyReader.definePowerPolicy(powerPolicyId,
                enabledComponents, disabledComponents);
        if (status != PolicyOperationStatus.OK) {
            int error = PolicyOperationStatus.ERROR_DEFINE_POWER_POLICY;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error));
            return error;
        }
        ICarPowerPolicySystemNotification daemon;
        synchronized (mLock) {
            daemon = mCarPowerPolicyDaemon;
        }
        try {
            daemon.notifyPowerPolicyDefinition(powerPolicyId, enabledComponents,
                    disabledComponents);
        } catch (RemoteException e) {
            int error = PolicyOperationStatus.ERROR_DEFINE_POWER_POLICY;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error));
            return error;
        }
        return PolicyOperationStatus.OK;
    }

    /**
     * Manually applies a power policy.
     *
     * <p>If the given ID is not defined, it fails.
     *
     * @return {@code true}, if successful. Otherwise, {@code false}.
     */
    public boolean applyPowerPolicyFromCommand(String[] args, IndentingPrintWriter writer) {
        if (args.length != 2) {
            writer.println("Power policy ID should be given");
            return false;
        }
        String powerPolicyId = args[1];
        if (powerPolicyId == null) {
            writer.println("Policy ID cannot be null");
            return false;
        }
        boolean isPreemptive = mPolicyReader.isPreemptivePowerPolicy(powerPolicyId);
        int status = isPreemptive ? applyPreemptivePowerPolicy(powerPolicyId)
                : applyPowerPolicy(powerPolicyId, true);
        if (status != PolicyOperationStatus.OK) {
            writer.println(PolicyOperationStatus.errorCodeToString(status));
            return false;
        }
        writer.printf("Power policy(%s) is successfully applied.\n", powerPolicyId);
        return true;
    }

    /**
     * Manually defines a power policy group.
     *
     * <p>If the given ID already exists, a wrong power state is given, or specified power policy ID
     * doesn't exist, it fails.
     *
     * @return {@code true}, if successful. Otherwise, {@code false}.
     */
    public boolean definePowerPolicyGroupFromCommand(String[] args, IndentingPrintWriter writer) {
        if (args.length < 3 || args.length > 4) {
            writer.println("Invalid syntax");
            return false;
        }
        String policyGroupId = args[1];
        int index = 2;
        SparseArray<String> defaultPolicyPerState = new SparseArray<>();
        while (index < args.length) {
            String[] tokens = args[index].split(":");
            if (tokens.length != 2) {
                writer.println("Invalid syntax");
                return false;
            }
            int state = PolicyReader.toPowerState(tokens[0]);
            if (state == PolicyReader.INVALID_POWER_STATE) {
                writer.printf("Invalid power state: %s\n", tokens[0]);
                return false;
            }
            defaultPolicyPerState.put(state, tokens[1]);
            index++;
        }
        int status = mPolicyReader.definePowerPolicyGroup(policyGroupId,
                defaultPolicyPerState);
        if (status != PolicyOperationStatus.OK) {
            writer.println(PolicyOperationStatus.errorCodeToString(status));
            return false;
        }
        writer.printf("Power policy group(%s) is successfully defined.\n", policyGroupId);
        return true;
    }

    /**
     * Manually sets a power policy group.
     *
     * <p>If the given ID is not defined, it fails.
     *
     * @return {@code true}, if successful. Otherwise, {@code false}.
     */
    public boolean setPowerPolicyGroupFromCommand(String[] args, IndentingPrintWriter writer) {
        if (args.length != 2) {
            writer.println("Power policy group ID should be given");
            return false;
        }
        String policyGroupId = args[1];
        int status = setCurrentPowerPolicyGroup(policyGroupId);
        if (status != PolicyOperationStatus.OK) {
            writer.println(PolicyOperationStatus.errorCodeToString(status));
            return false;
        }
        writer.printf("Setting power policy group(%s) is successful.\n", policyGroupId);
        return true;
    }

    /**
     * Powers off the device, considering the given options.
     *
     * <p>The final state can be "suspend-to-RAM" or "shutdown". Attempting to go to suspend-to-RAM
     * on devices which do not support it may lead to an unexpected system state.
     */
    public void powerOffFromCommand(boolean skipGarageMode, boolean shutdown) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        int param = 0;
        if (shutdown) {
            param = skipGarageMode ? VehicleApPowerStateShutdownParam.SHUTDOWN_IMMEDIATELY
                    : VehicleApPowerStateShutdownParam.SHUTDOWN_ONLY;
        } else {
            param = skipGarageMode ? VehicleApPowerStateShutdownParam.SLEEP_IMMEDIATELY
                    : VehicleApPowerStateShutdownParam.CAN_SLEEP;
        }
        PowerState state = new PowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE, param);
        synchronized (mLock) {
            mRebootAfterGarageMode = false;
            mPendingPowerStates.addFirst(new CpmsState(state));
            mLock.notify();
        }
        mHandler.handlePowerStateChange();
    }

    /**
     * Changes Silent Mode to the given mode.
     */
    public void setSilentMode(String silentMode) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_POWER);
        mSilentModeHandler.setSilentMode(silentMode);
    }

    /**
     * Dumps the current Silent Mode.
     */
    public void dumpSilentMode(IndentingPrintWriter writer) {
        mSilentModeHandler.dump(writer);
    }

    // In a real Deep Sleep, the hardware removes power from the CPU (but retains power
    // on the RAM). This puts the processor to sleep. Upon some external signal, power
    // is re-applied to the CPU, and processing resumes right where it left off.
    // We simulate this behavior by calling wait().
    // We continue from wait() when forceSimulatedResume() is called.
    private void simulateSleepByWaiting() {
        Slogf.i(TAG, "Starting to simulate Deep Sleep by waiting");
        synchronized (mSimulationWaitObject) {
            while (!mWakeFromSimulatedSleep) {
                try {
                    mSimulationWaitObject.wait();
                } catch (InterruptedException ignored) {
                    Thread.currentThread().interrupt(); // Restore interrupted status
                }
            }
            mInSimulatedDeepSleepMode = false;
        }
        Slogf.i(TAG, "Exit Deep Sleep simulation");
    }

    private int getMaxSuspendWaitDurationConfig() {
        return mContext.getResources().getInteger(R.integer.config_maxSuspendWaitDuration);
    }

    private boolean getWifiAdjustmentForSuspendConfig() {
        return mContext.getResources().getBoolean(R.bool.config_wifiAdjustmentForSuspend);
    }
}
