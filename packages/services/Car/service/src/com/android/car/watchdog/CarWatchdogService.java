/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.watchdog;

import static android.car.user.CarUserManager.USER_LIFECYCLE_EVENT_TYPE_STARTING;
import static android.car.user.CarUserManager.USER_LIFECYCLE_EVENT_TYPE_STOPPED;

import static com.android.car.CarLog.TAG_WATCHDOG;

import android.annotation.NonNull;
import android.automotive.watchdog.internal.GarageMode;
import android.automotive.watchdog.internal.ICarWatchdogServiceForSystem;
import android.automotive.watchdog.internal.PackageInfo;
import android.automotive.watchdog.internal.PackageIoOveruseStats;
import android.automotive.watchdog.internal.PowerCycle;
import android.automotive.watchdog.internal.StateType;
import android.automotive.watchdog.internal.UserState;
import android.car.Car;
import android.car.hardware.power.CarPowerManager.CarPowerStateListener;
import android.car.hardware.power.ICarPowerStateListener;
import android.car.watchdog.CarWatchdogManager;
import android.car.watchdog.ICarWatchdogService;
import android.car.watchdog.ICarWatchdogServiceCallback;
import android.car.watchdog.IResourceOveruseListener;
import android.car.watchdog.PackageKillableState;
import android.car.watchdog.ResourceOveruseConfiguration;
import android.car.watchdog.ResourceOveruseStats;
import android.car.watchdoglib.CarWatchdogDaemonHelper;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.UserInfo;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.ArraySet;
import android.util.IndentingPrintWriter;

import com.android.car.CarLocalServices;
import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.CarServiceUtils;
import com.android.car.ICarImpl;
import com.android.car.power.CarPowerManagementService;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.ArrayUtils;
import com.android.server.utils.Slogf;

import java.lang.ref.WeakReference;
import java.util.List;

/**
 * Service to implement CarWatchdogManager API.
 */
public final class CarWatchdogService extends ICarWatchdogService.Stub implements CarServiceBase {
    static final boolean DEBUG = false; // STOPSHIP if true
    static final String TAG = CarLog.tagFor(CarWatchdogService.class);
    static final String ACTION_GARAGE_MODE_ON =
            "com.android.server.jobscheduler.GARAGE_MODE_ON";
    static final String ACTION_GARAGE_MODE_OFF =
            "com.android.server.jobscheduler.GARAGE_MODE_OFF";

    private final Context mContext;
    private final ICarWatchdogServiceForSystemImpl mWatchdogServiceForSystem;
    private final PackageInfoHandler mPackageInfoHandler;
    private final WatchdogProcessHandler mWatchdogProcessHandler;
    private final WatchdogPerfHandler mWatchdogPerfHandler;
    private final CarWatchdogDaemonHelper mCarWatchdogDaemonHelper;
    private final CarWatchdogDaemonHelper.OnConnectionChangeListener mConnectionListener;
    /*
     * TODO(b/192481350): Listen for GarageMode change notification rather than depending on the
     *  system_server broadcast when the CarService internal API for listening GarageMode change is
     *  implemented.
     */
    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            boolean isGarageMode = false;
            if (action.equals(ACTION_GARAGE_MODE_ON)) {
                isGarageMode = true;
            } else if (!action.equals(ACTION_GARAGE_MODE_OFF)) {
                return;
            }
            try {
                mCarWatchdogDaemonHelper.notifySystemStateChange(StateType.GARAGE_MODE,
                        isGarageMode ? GarageMode.GARAGE_MODE_ON : GarageMode.GARAGE_MODE_OFF,
                        /* arg2= */ -1);
                if (DEBUG) {
                    Slogf.d(TAG, "Notified car watchdog daemon of garage mode(%s)",
                            isGarageMode ? "ON" : "OFF");
                }
            } catch (RemoteException | RuntimeException e) {
                Slogf.w(TAG, "Notifying garage mode state change failed: %s", e);
            }
        }
    };

    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private boolean mReadyToRespond;
    @GuardedBy("mLock")
    private boolean mIsConnected;

    public CarWatchdogService(Context context) {
        mContext = context;
        mPackageInfoHandler = new PackageInfoHandler(mContext.getPackageManager());
        mCarWatchdogDaemonHelper = new CarWatchdogDaemonHelper(TAG_WATCHDOG);
        mWatchdogServiceForSystem = new ICarWatchdogServiceForSystemImpl(this);
        mWatchdogProcessHandler = new WatchdogProcessHandler(mWatchdogServiceForSystem,
                mCarWatchdogDaemonHelper);
        mWatchdogPerfHandler = new WatchdogPerfHandler(mContext, mCarWatchdogDaemonHelper,
                mPackageInfoHandler);
        mConnectionListener = (isConnected) -> {
            mWatchdogPerfHandler.onDaemonConnectionChange(isConnected);
            synchronized (mLock) {
                mIsConnected = isConnected;
            }
            registerToDaemon();
        };
    }

    @Override
    public void init() {
        mWatchdogProcessHandler.init();
        subscribePowerCycleChange();
        subscribeUserStateChange();
        subscribeBroadcastReceiver();
        mCarWatchdogDaemonHelper.addOnConnectionChangeListener(mConnectionListener);
        mCarWatchdogDaemonHelper.connect();
        mWatchdogPerfHandler.init();
        // To make sure the main handler is ready for responding to car watchdog daemon, registering
        // to the daemon is done through the main handler. Once the registration is completed, we
        // can assume that the main handler is not too busy handling other stuffs.
        postRegisterToDaemonMessage();
        if (DEBUG) {
            Slogf.d(TAG, "CarWatchdogService is initialized");
        }
    }

    @Override
    public void release() {
        mContext.unregisterReceiver(mBroadcastReceiver);
        mWatchdogPerfHandler.release();
        unregisterFromDaemon();
        mCarWatchdogDaemonHelper.disconnect();
    }

    @Override
    public void dump(IndentingPrintWriter writer) {
        writer.println("*CarWatchdogService*");
        mWatchdogProcessHandler.dump(writer);
        mWatchdogPerfHandler.dump(writer);
    }

    /**
     * Registers {@link android.car.watchdog.ICarWatchdogServiceCallback} to
     * {@link CarWatchdogService}.
     */
    @Override
    public void registerClient(ICarWatchdogServiceCallback client, int timeout) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_WATCHDOG);
        mWatchdogProcessHandler.registerClient(client, timeout);
    }

    /**
     * Unregisters {@link android.car.watchdog.ICarWatchdogServiceCallback} from
     * {@link CarWatchdogService}.
     */
    @Override
    public void unregisterClient(ICarWatchdogServiceCallback client) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_WATCHDOG);
        mWatchdogProcessHandler.unregisterClient(client);
    }

    /**
     * Tells {@link CarWatchdogService} that the client is alive.
     */
    @Override
    public void tellClientAlive(ICarWatchdogServiceCallback client, int sessionId) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_WATCHDOG);
        mWatchdogProcessHandler.tellClientAlive(client, sessionId);
    }

    @VisibleForTesting
    int getClientCount(int timeout) {
        return mWatchdogProcessHandler.getClientCount(timeout);
    }

    /** Returns {@link android.car.watchdog.ResourceOveruseStats} for the calling package. */
    @Override
    @NonNull
    public ResourceOveruseStats getResourceOveruseStats(
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag,
            @CarWatchdogManager.StatsPeriod int maxStatsPeriod) {
        return mWatchdogPerfHandler.getResourceOveruseStats(resourceOveruseFlag, maxStatsPeriod);
    }

    /**
      *  Returns {@link android.car.watchdog.ResourceOveruseStats} for all packages for the maximum
      *  specified period, and the specified resource types with stats greater than or equal to the
      *  minimum specified stats.
      */
    @Override
    @NonNull
    public List<ResourceOveruseStats> getAllResourceOveruseStats(
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag,
            @CarWatchdogManager.MinimumStatsFlag int minimumStatsFlag,
            @CarWatchdogManager.StatsPeriod int maxStatsPeriod) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_COLLECT_CAR_WATCHDOG_METRICS);
        return mWatchdogPerfHandler.getAllResourceOveruseStats(resourceOveruseFlag,
                minimumStatsFlag, maxStatsPeriod);
    }

    /** Returns {@link android.car.watchdog.ResourceOveruseStats} for the specified user package. */
    @Override
    @NonNull
    public ResourceOveruseStats getResourceOveruseStatsForUserPackage(
            @NonNull String packageName, @NonNull UserHandle userHandle,
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag,
            @CarWatchdogManager.StatsPeriod int maxStatsPeriod) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_COLLECT_CAR_WATCHDOG_METRICS);
        return mWatchdogPerfHandler.getResourceOveruseStatsForUserPackage(packageName, userHandle,
                resourceOveruseFlag, maxStatsPeriod);
    }

    /**
     * Adds {@link android.car.watchdog.IResourceOveruseListener} for the calling package's resource
     * overuse notifications.
     */
    @Override
    public void addResourceOveruseListener(
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag,
            @NonNull IResourceOveruseListener listener) {
        mWatchdogPerfHandler.addResourceOveruseListener(resourceOveruseFlag, listener);
    }

    /**
     * Removes the previously added {@link android.car.watchdog.IResourceOveruseListener} for the
     * calling package's resource overuse notifications.
     */
    @Override
    public void removeResourceOveruseListener(@NonNull IResourceOveruseListener listener) {
        mWatchdogPerfHandler.removeResourceOveruseListener(listener);
    }

    /**
     * Adds {@link android.car.watchdog.IResourceOveruseListener} for all packages' resource overuse
     * notifications.
     */
    @Override
    public void addResourceOveruseListenerForSystem(
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag,
            @NonNull IResourceOveruseListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_COLLECT_CAR_WATCHDOG_METRICS);
        mWatchdogPerfHandler.addResourceOveruseListenerForSystem(resourceOveruseFlag, listener);
    }

    /**
     * Removes the previously added {@link android.car.watchdog.IResourceOveruseListener} for all
     * packages' resource overuse notifications.
     */
    @Override
    public void removeResourceOveruseListenerForSystem(@NonNull IResourceOveruseListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_COLLECT_CAR_WATCHDOG_METRICS);
        mWatchdogPerfHandler.removeResourceOveruseListenerForSystem(listener);
    }

    /** Sets whether or not a user package is killable on resource overuse. */
    @Override
    public void setKillablePackageAsUser(String packageName, UserHandle userHandle,
            boolean isKillable) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG);
        mWatchdogPerfHandler.setKillablePackageAsUser(packageName, userHandle, isKillable);
    }

    /**
     * Returns all {@link android.car.watchdog.PackageKillableState} on resource overuse for
     * the specified user.
     */
    @Override
    @NonNull
    public List<PackageKillableState> getPackageKillableStatesAsUser(UserHandle userHandle) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG);
        return mWatchdogPerfHandler.getPackageKillableStatesAsUser(userHandle);
    }

    /**
     * Sets {@link android.car.watchdog.ResourceOveruseConfiguration} for the specified resources.
     */
    @Override
    @CarWatchdogManager.ReturnCode
    public int setResourceOveruseConfigurations(
            List<ResourceOveruseConfiguration> configurations,
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag)
            throws RemoteException {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG);
        return mWatchdogPerfHandler.setResourceOveruseConfigurations(configurations,
                resourceOveruseFlag);
    }

    /** Returns the available {@link android.car.watchdog.ResourceOveruseConfiguration}. */
    @Override
    @NonNull
    public List<ResourceOveruseConfiguration> getResourceOveruseConfigurations(
            @CarWatchdogManager.ResourceOveruseFlag int resourceOveruseFlag) {
        ICarImpl.assertAnyPermission(mContext, Car.PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG,
                Car.PERMISSION_COLLECT_CAR_WATCHDOG_METRICS);
        return mWatchdogPerfHandler.getResourceOveruseConfigurations(resourceOveruseFlag);
    }

    private void postRegisterToDaemonMessage() {
        CarServiceUtils.runOnMain(() -> {
            synchronized (mLock) {
                mReadyToRespond = true;
            }
            registerToDaemon();
        });
    }

    private void registerToDaemon() {
        synchronized (mLock) {
            if (!mIsConnected || !mReadyToRespond) {
                return;
            }
        }
        try {
            mCarWatchdogDaemonHelper.registerCarWatchdogService(mWatchdogServiceForSystem);
            if (DEBUG) {
                Slogf.d(TAG, "CarWatchdogService registers to car watchdog daemon");
            }
        } catch (RemoteException | RuntimeException e) {
            Slogf.w(TAG, "Cannot register to car watchdog daemon: %s", e);
        }
        UserManager userManager = UserManager.get(mContext);
        List<UserInfo> users = userManager.getUsers();
        try {
            // TODO(b/152780162): reduce the number of RPC calls(isUserRunning).
            for (UserInfo info : users) {
                int userState = userManager.isUserRunning(info.id)
                        ? UserState.USER_STATE_STARTED : UserState.USER_STATE_STOPPED;
                mCarWatchdogDaemonHelper.notifySystemStateChange(StateType.USER_STATE, info.id,
                        userState);
                if (userState == UserState.USER_STATE_STOPPED) {
                    mWatchdogProcessHandler.updateUserState(info.id, /*isStopped=*/true);
                } else {
                    mWatchdogProcessHandler.updateUserState(info.id, /*isStopped=*/false);
                }
            }
        } catch (RemoteException | RuntimeException e) {
            Slogf.w(TAG, "Notifying system state change failed: %s", e);
        }
    }

    private void unregisterFromDaemon() {
        try {
            mCarWatchdogDaemonHelper.unregisterCarWatchdogService(mWatchdogServiceForSystem);
            if (DEBUG) {
                Slogf.d(TAG, "CarWatchdogService unregisters from car watchdog daemon");
            }
        } catch (RemoteException | RuntimeException e) {
            Slogf.w(TAG, "Cannot unregister from car watchdog daemon: %s", e);
        }
    }

    private void subscribePowerCycleChange() {
        CarPowerManagementService powerService =
                CarLocalServices.getService(CarPowerManagementService.class);
        if (powerService == null) {
            Slogf.w(TAG, "Cannot get CarPowerManagementService");
            return;
        }
        powerService.registerListener(new ICarPowerStateListener.Stub() {
            @Override
            public void onStateChanged(int state) {
                int powerCycle;
                switch (state) {
                    // SHUTDOWN_PREPARE covers suspend and shutdown.
                    case CarPowerStateListener.SHUTDOWN_PREPARE:
                        powerCycle = PowerCycle.POWER_CYCLE_SHUTDOWN_PREPARE;
                        break;
                    case CarPowerStateListener.SHUTDOWN_ENTER:
                    case CarPowerStateListener.SUSPEND_ENTER:
                        powerCycle = PowerCycle.POWER_CYCLE_SHUTDOWN_ENTER;
                    // ON covers resume.
                    case CarPowerStateListener.ON:
                        powerCycle = PowerCycle.POWER_CYCLE_RESUME;
                        // There might be outdated & incorrect info. We should reset them before
                        // starting to do health check.
                        mWatchdogProcessHandler.prepareHealthCheck();
                        break;
                    default:
                        return;
                }
                try {
                    mCarWatchdogDaemonHelper.notifySystemStateChange(StateType.POWER_CYCLE,
                            powerCycle, /* arg2= */ -1);
                    if (DEBUG) {
                        Slogf.d(TAG, "Notified car watchdog daemon of power cycle(%d)", powerCycle);
                    }
                } catch (RemoteException | RuntimeException e) {
                    Slogf.w(TAG, "Notifying power cycle state change failed: %s", e);
                }
            }
        });
    }

    private void subscribeUserStateChange() {
        CarUserService userService = CarLocalServices.getService(CarUserService.class);
        if (userService == null) {
            Slogf.w(TAG, "Cannot get CarUserService");
            return;
        }
        userService.addUserLifecycleListener((event) -> {
            int userId = event.getUserHandle().getIdentifier();
            int userState;
            String userStateDesc;
            switch (event.getEventType()) {
                case USER_LIFECYCLE_EVENT_TYPE_STARTING:
                    mWatchdogProcessHandler.updateUserState(userId, /*isStopped=*/false);
                    userState = UserState.USER_STATE_STARTED;
                    userStateDesc = "STARTING";
                    break;
                case USER_LIFECYCLE_EVENT_TYPE_STOPPED:
                    mWatchdogProcessHandler.updateUserState(userId, /*isStopped=*/true);
                    userState = UserState.USER_STATE_STOPPED;
                    userStateDesc = "STOPPING";
                    break;
                default:
                    return;
            }
            try {
                mCarWatchdogDaemonHelper.notifySystemStateChange(StateType.USER_STATE, userId,
                        userState);
                if (DEBUG) {
                    Slogf.d(TAG, "Notified car watchdog daemon user %d's user state, %s",
                            userId, userStateDesc);
                }
            } catch (RemoteException | RuntimeException e) {
                Slogf.w(TAG, "Notifying user state change failed: %s", e);
            }
        });
    }

    private void subscribeBroadcastReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_GARAGE_MODE_ON);
        filter.addAction(ACTION_GARAGE_MODE_OFF);

        mContext.registerReceiverForAllUsers(mBroadcastReceiver, filter, null, null);
    }

    private static final class ICarWatchdogServiceForSystemImpl
            extends ICarWatchdogServiceForSystem.Stub {
        private final WeakReference<CarWatchdogService> mService;

        ICarWatchdogServiceForSystemImpl(CarWatchdogService service) {
            mService = new WeakReference<>(service);
        }

        @Override
        public void checkIfAlive(int sessionId, int timeout) {
            CarWatchdogService service = mService.get();
            if (service == null) {
                Slogf.w(TAG, "CarWatchdogService is not available");
                return;
            }
            service.mWatchdogProcessHandler.postHealthCheckMessage(sessionId);
        }

        @Override
        public void prepareProcessTermination() {
            Slogf.w(TAG, "CarWatchdogService is about to be killed by car watchdog daemon");
        }

        @Override
        public List<PackageInfo> getPackageInfosForUids(
                int[] uids, List<String> vendorPackagePrefixes) {
            if (ArrayUtils.isEmpty(uids)) {
                Slogf.w(TAG, "UID list is empty");
                return null;
            }
            CarWatchdogService service = mService.get();
            if (service == null) {
                Slogf.w(TAG, "CarWatchdogService is not available");
                return null;
            }
            return service.mPackageInfoHandler.getPackageInfosForUids(uids, vendorPackagePrefixes);
        }

        @Override
        public void latestIoOveruseStats(List<PackageIoOveruseStats> packageIoOveruseStats) {
            if (packageIoOveruseStats.isEmpty()) {
                Slogf.w(TAG, "Latest I/O overuse stats is empty");
                return;
            }
            CarWatchdogService service = mService.get();
            if (service == null) {
                Slogf.w(TAG, "CarWatchdogService is not available");
                return;
            }
            service.mWatchdogPerfHandler.latestIoOveruseStats(packageIoOveruseStats);
        }

        @Override
        public void resetResourceOveruseStats(List<String> packageNames) {
            if (packageNames.isEmpty()) {
                Slogf.w(TAG, "Provided an empty package name to reset resource overuse stats");
                return;
            }
            CarWatchdogService service = mService.get();
            if (service == null) {
                Slogf.w(TAG, "CarWatchdogService is not available");
                return;
            }
            service.mWatchdogPerfHandler.resetResourceOveruseStats(new ArraySet<>(packageNames));
        }
    }
}
