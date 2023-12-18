/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.user;

import static android.Manifest.permission.CREATE_USERS;
import static android.Manifest.permission.MANAGE_USERS;
import static android.car.drivingstate.CarUxRestrictions.UX_RESTRICTIONS_NO_SETUP;

import static com.android.car.PermissionHelper.checkHasAtLeastOnePermissionGranted;
import static com.android.car.PermissionHelper.checkHasDumpPermissionGranted;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DEPRECATED_CODE;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.app.ActivityTaskManager.RootTaskInfo;
import android.app.IActivityManager;
import android.car.CarOccupantZoneManager;
import android.car.CarOccupantZoneManager.OccupantTypeEnum;
import android.car.CarOccupantZoneManager.OccupantZoneInfo;
import android.car.ICarUserService;
import android.car.drivingstate.CarUxRestrictions;
import android.car.drivingstate.ICarUxRestrictionsChangeListener;
import android.car.settings.CarSettings;
import android.car.user.CarUserManager;
import android.car.user.CarUserManager.UserIdentificationAssociationSetValue;
import android.car.user.CarUserManager.UserIdentificationAssociationType;
import android.car.user.CarUserManager.UserLifecycleEvent;
import android.car.user.CarUserManager.UserLifecycleListener;
import android.car.user.UserCreationResult;
import android.car.user.UserIdentificationAssociationResponse;
import android.car.user.UserRemovalResult;
import android.car.user.UserStartResult;
import android.car.user.UserStopResult;
import android.car.user.UserSwitchResult;
import android.car.userlib.HalCallback;
import android.car.userlib.UserHalHelper;
import android.car.userlib.UserHelper;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.UserInfo;
import android.content.pm.UserInfo.UserInfoFlag;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.hardware.automotive.vehicle.V2_0.CreateUserRequest;
import android.hardware.automotive.vehicle.V2_0.CreateUserStatus;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoRequestType;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoResponseAction;
import android.hardware.automotive.vehicle.V2_0.RemoveUserRequest;
import android.hardware.automotive.vehicle.V2_0.SwitchUserRequest;
import android.hardware.automotive.vehicle.V2_0.SwitchUserStatus;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationGetRequest;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationResponse;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationSetAssociation;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationSetRequest;
import android.hardware.automotive.vehicle.V2_0.UsersInfo;
import android.location.LocationManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.os.RemoteException;
import android.os.Trace;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.sysprop.CarProperties;
import android.text.TextUtils;
import android.util.EventLog;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.TimingsTraceLog;
import android.view.Display;

import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.CarServiceUtils;
import com.android.car.CarUxRestrictionsManagerService;
import com.android.car.R;
import com.android.car.hal.UserHalService;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.car.internal.ICarServiceHelper;
import com.android.car.internal.common.CommonConstants.UserLifecycleEventType;
import com.android.car.internal.common.EventLogTags;
import com.android.car.internal.common.UserHelperLite;
import com.android.car.power.CarPowerManagementService;
import com.android.car.user.InitialUserSetter.InitialUserInfo;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.infra.AndroidFuture;
import com.android.internal.os.IResultReceiver;
import com.android.internal.util.ArrayUtils;
import com.android.internal.util.FunctionalUtils;
import com.android.internal.util.Preconditions;
import com.android.internal.util.UserIcons;
import com.android.server.utils.Slogf;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * User service for cars. Manages users at boot time. Including:
 *
 * <ol>
 *   <li> Creates a user used as driver.
 *   <li> Creates a user used as passenger.
 *   <li> Creates a secondary admin user on first run.
 *   <li> Switch drivers.
 * <ol/>
 */
public final class CarUserService extends ICarUserService.Stub implements CarServiceBase {

    private static final String TAG = CarLog.tagFor(CarUserService.class);

    /** {@code int} extra used to represent a user id in a {@link IResultReceiver} response. */
    public static final String BUNDLE_USER_ID = "user.id";
    /** {@code int} extra used to represent user flags in a {@link IResultReceiver} response. */
    public static final String BUNDLE_USER_FLAGS = "user.flags";
    /** {@code String} extra used to represent a user name in a {@link IResultReceiver} response. */
    public static final String BUNDLE_USER_NAME = "user.name";
    /**
     * {@code int} extra used to represent the user locales in a {@link IResultReceiver} response.
     */
    public static final String BUNDLE_USER_LOCALES = "user.locales";
    /**
     * {@code int} extra used to represent the info action in a {@link IResultReceiver} response.
     */
    public static final String BUNDLE_INITIAL_INFO_ACTION = "initial_info.action";

    public static final String VEHICLE_HAL_NOT_SUPPORTED = "Vehicle Hal not supported.";

    public static final String HANDLER_THREAD_NAME = "UserService";

    private final Context mContext;
    private final IActivityManager mAm;
    private final UserManager mUserManager;
    private final int mMaxRunningUsers;
    private final InitialUserSetter mInitialUserSetter;
    private final boolean mEnablePassengerSupport;
    private final UserPreCreator mUserPreCreator;

    private final Object mLockUser = new Object();
    @GuardedBy("mLockUser")
    private boolean mUser0Unlocked;
    @GuardedBy("mLockUser")
    private final ArrayList<Runnable> mUser0UnlockTasks = new ArrayList<>();
    // Only one passenger is supported.
    @GuardedBy("mLockUser")
    private @UserIdInt int mLastPassengerId;
    /**
     * Background users that will be restarted in garage mode. This list can include the
     * current foreground user but the current foreground user should not be restarted.
     */
    @GuardedBy("mLockUser")
    private final ArrayList<Integer> mBackgroundUsersToRestart = new ArrayList<>();
    /**
     * Keep the list of background users started here. This is wholly for debugging purpose.
     */
    @GuardedBy("mLockUser")
    private final ArrayList<Integer> mBackgroundUsersRestartedHere = new ArrayList<>();

    private final UserHalService mHal;

    private final HandlerThread mHandlerThread = CarServiceUtils.getHandlerThread(
            HANDLER_THREAD_NAME);
    private final Handler mHandler;

    /**
     * List of listeners to be notified on new user activities events.
     * This collection should be accessed and manipulated by mHandlerThread only.
     */
    private final List<UserLifecycleListener> mUserLifecycleListeners = new ArrayList<>();

    /**
     * List of lifecycle listeners by uid.
     * This collection should be accessed and manipulated by mHandlerThread only.
     */
    private final SparseArray<IResultReceiver> mAppLifecycleListeners = new SparseArray<>();

    /**
     * User Id for the user switch in process, if any.
     */
    @GuardedBy("mLockUser")
    private int mUserIdForUserSwitchInProcess = UserHandle.USER_NULL;
    /**
     * Request Id for the user switch in process, if any.
     */
    @GuardedBy("mLockUser")
    private int mRequestIdForUserSwitchInProcess;
    private final int mHalTimeoutMs = CarProperties.user_hal_timeout().orElse(5_000);

    private final CopyOnWriteArrayList<PassengerCallback> mPassengerCallbacks =
            new CopyOnWriteArrayList<>();

    // TODO(b/163566866): Use mSwitchGuestUserBeforeSleep for new create guest request
    private final boolean mSwitchGuestUserBeforeSleep;

    @Nullable
    @GuardedBy("mLockUser")
    private UserInfo mInitialUser;

    private IResultReceiver mUserSwitchUiReceiver;

    private final CarUxRestrictionsManagerService mCarUxRestrictionService;

    /**
     * Whether some operations - like user switch - are restricted by driving safety constraints.
     */
    @GuardedBy("mLockUser")
    private boolean mUxRestricted;

    /**
     * If {@code false}, garage mode operations (background users start at garage mode entry and
     * background users stop at garage mode exit) will be skipped. Controlled using car shell
     * command {@code adb shell set-start-bg-users-on-garage-mode [true|false]}
     * Purpose: Garage mode testing and simulation
     */
    @GuardedBy("mLockUser")
    private boolean mStartBackgroundUsersOnGarageMode = true;

    /**
     * Callback to notify {@code CarServiceHelper} about driving safety changes (through
     * {@link ICarServiceHelper#setSafetyMode(boolean).
     *
     * <p>NOTE: in theory, that logic should belong to {@code CarDevicePolicyService}, but it's
     * simpler to do it here (and that service already depends on this one).
     */
    @GuardedBy("mLockUser")
    private ICarServiceHelper mICarServiceHelper;

    private final ICarUxRestrictionsChangeListener mCarUxRestrictionsChangeListener =
            new ICarUxRestrictionsChangeListener.Stub() {
        @Override
        public void onUxRestrictionsChanged(CarUxRestrictions restrictions) {
            setUxRestrictions(restrictions);
        }
    };

    /** Interface for callbaks related to passenger activities. */
    public interface PassengerCallback {
        /** Called when passenger is started at a certain zone. */
        void onPassengerStarted(@UserIdInt int passengerId, int zoneId);
        /** Called when passenger is stopped. */
        void onPassengerStopped(@UserIdInt int passengerId);
    }

    /** Interface for delegating zone-related implementation to CarOccupantZoneService. */
    public interface ZoneUserBindingHelper {
        /** Gets occupant zones corresponding to the occupant type. */
        @NonNull
        List<OccupantZoneInfo> getOccupantZones(@OccupantTypeEnum int occupantType);
        /** Assigns the user to the occupant zone. */
        boolean assignUserToOccupantZone(@UserIdInt int userId, int zoneId);
        /** Makes the occupant zone unoccupied. */
        boolean unassignUserFromOccupantZone(@UserIdInt int userId);
        /** Returns whether there is a passenger display. */
        boolean isPassengerDisplayAvailable();
    }

    private final Object mLockHelper = new Object();
    @GuardedBy("mLockHelper")
    private ZoneUserBindingHelper mZoneUserBindingHelper;

    /** Map used to avoid calling UserHAL when a user was removed because HAL creation failed. */
    @GuardedBy("mLockUser")
    private final SparseBooleanArray mFailedToCreateUserIds = new SparseBooleanArray(1);

    public CarUserService(@NonNull Context context, @NonNull UserHalService hal,
            @NonNull UserManager userManager,
            @NonNull IActivityManager am, int maxRunningUsers,
            @NonNull CarUxRestrictionsManagerService uxRestrictionService) {
        this(context, hal, userManager, am, maxRunningUsers,
                /* initialUserSetter= */ null, /* userPreCreator= */ null, uxRestrictionService,
                null);
    }

    @VisibleForTesting
    CarUserService(@NonNull Context context, @NonNull UserHalService hal,
            @NonNull UserManager userManager,
            @NonNull IActivityManager am, int maxRunningUsers,
            @Nullable InitialUserSetter initialUserSetter,
            @Nullable UserPreCreator userPreCreator,
            @NonNull CarUxRestrictionsManagerService uxRestrictionService,
            @Nullable Handler handler) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "constructed for user " + context.getUserId());
        }
        mContext = context;
        mHal = hal;
        mAm = am;
        mMaxRunningUsers = maxRunningUsers;
        mUserManager = userManager;
        mLastPassengerId = UserHandle.USER_NULL;
        mHandler = handler == null ? new Handler(mHandlerThread.getLooper()) : handler;
        mInitialUserSetter =
                initialUserSetter == null ? new InitialUserSetter(context, this,
                        (u) -> setInitialUser(u)) : initialUserSetter;
        mUserPreCreator =
                userPreCreator == null ? new UserPreCreator(mUserManager) : userPreCreator;
        Resources resources = context.getResources();
        mEnablePassengerSupport = resources.getBoolean(R.bool.enablePassengerSupport);
        mSwitchGuestUserBeforeSleep = resources.getBoolean(
                R.bool.config_switchGuestUserBeforeGoingSleep);
        mCarUxRestrictionService = uxRestrictionService;
    }

    @Override
    public void init() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "init()");
        }
        mCarUxRestrictionService.registerUxRestrictionsChangeListener(
                mCarUxRestrictionsChangeListener, Display.DEFAULT_DISPLAY);

        setUxRestrictions(mCarUxRestrictionService.getCurrentUxRestrictions());
    }

    @Override
    public void release() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "release()");
        }
        mCarUxRestrictionService
                .unregisterUxRestrictionsChangeListener(mCarUxRestrictionsChangeListener);
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(@NonNull IndentingPrintWriter writer) {
        checkHasDumpPermissionGranted("dump()");

        writer.println("*CarUserService*");
        String indent = "  ";
        handleDumpListeners(writer, indent);
        writer.printf("User switch UI receiver %s\n", mUserSwitchUiReceiver);
        synchronized (mLockUser) {
            writer.println("User0Unlocked: " + mUser0Unlocked);
            writer.println("BackgroundUsersToRestart: " + mBackgroundUsersToRestart);
            writer.println("BackgroundUsersRestarted: " + mBackgroundUsersRestartedHere);
            if (mFailedToCreateUserIds.size() > 0) {
                writer.println("FailedToCreateUserIds: " + mFailedToCreateUserIds);
            }
            writer.printf("Is UX restricted: %b\n", mUxRestricted);
            writer.printf("Start Background Users On Garage Mode=%s\n",
                    mStartBackgroundUsersOnGarageMode);
        }

        writer.println("SwitchGuestUserBeforeSleep: " + mSwitchGuestUserBeforeSleep);

        writer.println("MaxRunningUsers: " + mMaxRunningUsers);
        List<UserInfo> allDrivers = getAllDrivers();
        int driversSize = allDrivers.size();
        writer.println("NumberOfDrivers: " + driversSize);
        for (int i = 0; i < driversSize; i++) {
            int driverId = allDrivers.get(i).id;
            writer.print(indent + "#" + i + ": id=" + driverId);
            List<UserInfo> passengers = getPassengers(driverId);
            int passengersSize = passengers.size();
            writer.print(" NumberPassengers: " + passengersSize);
            if (passengersSize > 0) {
                writer.print(" [");
                for (int j = 0; j < passengersSize; j++) {
                    writer.print(passengers.get(j).id);
                    if (j < passengersSize - 1) {
                        writer.print(" ");
                    }
                }
                writer.print("]");
            }
            writer.println();
        }
        writer.printf("EnablePassengerSupport: %s\n", mEnablePassengerSupport);
        writer.printf("User HAL timeout: %dms\n",  mHalTimeoutMs);
        writer.printf("Initial user: %s\n", mInitialUser);

        writer.println("Relevant overlayable properties");
        Resources res = mContext.getResources();
        writer.printf("%sowner_name=%s\n", indent,
                res.getString(com.android.internal.R.string.owner_name));
        writer.printf("%sdefault_guest_name=%s\n", indent,
                res.getString(R.string.default_guest_name));
        writer.printf("User switch in process=%d\n", mUserIdForUserSwitchInProcess);
        writer.printf("Request Id for the user switch in process=%d\n ",
                    mRequestIdForUserSwitchInProcess);
        writer.printf("System UI package name=%s\n", getSystemUiPackageName());

        writer.println("Relevant Global settings");
        dumpGlobalProperty(writer, indent, CarSettings.Global.LAST_ACTIVE_USER_ID);
        dumpGlobalProperty(writer, indent, CarSettings.Global.LAST_ACTIVE_PERSISTENT_USER_ID);

        mInitialUserSetter.dump(writer);
    }

    private void dumpGlobalProperty(PrintWriter writer, String indent, String property) {
        String value = Settings.Global.getString(mContext.getContentResolver(), property);
        writer.printf("%s%s=%s\n", indent, property, value);
    }

    private void handleDumpListeners(@NonNull PrintWriter writer, String indent) {
        CountDownLatch latch = new CountDownLatch(1);
        mHandler.post(() -> {
            handleDumpServiceLifecycleListeners(writer);
            handleDumpAppLifecycleListeners(writer, indent);
            latch.countDown();
        });
        int timeout = 5;
        try {
            if (!latch.await(timeout, TimeUnit.SECONDS)) {
                writer.printf("Handler thread didn't respond in %ds when dumping listeners\n",
                        timeout);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            writer.println("Interrupted waiting for handler thread to dump app and user listeners");
        }
    }

    private void handleDumpServiceLifecycleListeners(@NonNull PrintWriter writer) {
        if (mUserLifecycleListeners.isEmpty()) {
            writer.println("No lifecycle listeners for internal services");
            return;
        }
        int size = mUserLifecycleListeners.size();
        writer.printf("%d lifecycle listener%s for services\n", size, size == 1 ? "" : "s");
        String indent = "  ";
        for (UserLifecycleListener listener : mUserLifecycleListeners) {
            writer.printf("%s%s\n", indent, FunctionalUtils.getLambdaName(listener));
        }
    }

    private void handleDumpAppLifecycleListeners(@NonNull PrintWriter writer, String indent) {
        int size = mAppLifecycleListeners.size();
        if (size == 0) {
            writer.println("No lifecycle listeners for apps");
            return;
        }
        writer.printf("%d lifecycle listener%s for apps \n", size, size == 1 ? "" : "s");
        for (int i = 0; i < size; i++) {
            int uid = mAppLifecycleListeners.keyAt(i);
            IResultReceiver listener = mAppLifecycleListeners.valueAt(i);
            writer.printf("%suid: %d listener: %s\n", indent, uid,
                    FunctionalUtils.getLambdaName(listener));
        }
    }

    /**
     * @see ExperimentalCarUserManager.createDriver
     */
    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DEPRECATED_CODE,
            details = "TODO(b/172262561) remove annotation after refactoring")
    public AndroidFuture<UserCreationResult> createDriver(@NonNull String name, boolean admin) {
        checkManageUsersPermission("createDriver");
        Objects.requireNonNull(name, "name cannot be null");

        AndroidFuture<UserCreationResult> future = new AndroidFuture<UserCreationResult>() {
            @Override
            protected void onCompleted(UserCreationResult result, Throwable err) {
                if (result == null) {
                    Slog.w(TAG, "createDriver(" + name + "," + admin + ") failed: " + err);
                } else {
                    if (result.getStatus() == UserCreationResult.STATUS_SUCCESSFUL) {
                        assignDefaultIcon(result.getUser());
                    }
                }
                super.onCompleted(result, err);
            };
        };
        int flags = 0;
        if (admin) {
            if (!(mUserManager.isAdminUser() || mUserManager.isSystemUser())) {
                Slog.e(TAG, "Only admin users and system user can create other admins.");
                sendUserCreationResultFailure(future, UserCreationResult.STATUS_INVALID_REQUEST);
                return future;
            }
            flags = UserInfo.FLAG_ADMIN;
        }
        createUser(name, UserInfo.getDefaultUserType(flags), flags, mHalTimeoutMs, future);
        return future;
    }

    /**
     * @see ExperimentalCarUserManager.createPassenger
     */
    @Override
    @Nullable
    @ExcludeFromCodeCoverageGeneratedReport(reason = DEPRECATED_CODE,
            details = "TODO(b/172262561) remove annotation after refactoring")
    public UserInfo createPassenger(@NonNull String name, @UserIdInt int driverId) {
        checkManageUsersPermission("createPassenger");
        Objects.requireNonNull(name, "name cannot be null");
        UserInfo driver = mUserManager.getUserInfo(driverId);
        if (driver == null) {
            Slog.w(TAG, "the driver is invalid");
            return null;
        }
        if (driver.isGuest()) {
            Slog.w(TAG, "a guest driver cannot create a passenger");
            return null;
        }
        // createPassenger doesn't use user HAL because user HAL doesn't support profile user yet.
        UserInfo user = mUserManager.createProfileForUser(name,
                UserManager.USER_TYPE_PROFILE_MANAGED, /* flags */ 0, driverId);
        if (user == null) {
            // Couldn't create user, most likely because there are too many.
            Slog.w(TAG, "can't create a profile for user" + driverId);
            return null;
        }
        // Passenger user should be a non-admin user.
        UserHelper.setDefaultNonAdminRestrictions(mContext, user, /* enable= */ true);
        assignDefaultIcon(user);
        return user;
    }

    /**
     * @see ExperimentalCarUserManager.switchDriver
     */
    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DEPRECATED_CODE,
            details = "TODO(b/172262561) remove annotation after refactoring")
    public void switchDriver(@UserIdInt int driverId, AndroidFuture<UserSwitchResult> receiver) {
        checkManageUsersPermission("switchDriver");
        if (UserHelperLite.isHeadlessSystemUser(driverId)) {
            // System user doesn't associate with real person, can not be switched to.
            Slog.w(TAG, "switching to system user in headless system user mode is not allowed");
            sendUserSwitchResult(receiver, UserSwitchResult.STATUS_INVALID_REQUEST);
            return;
        }
        int userSwitchable = mUserManager.getUserSwitchability();
        if (userSwitchable != UserManager.SWITCHABILITY_STATUS_OK) {
            Slog.w(TAG, "current process is not allowed to switch user");
            sendUserSwitchResult(receiver, UserSwitchResult.STATUS_INVALID_REQUEST);
            return;
        }
        switchUser(driverId, mHalTimeoutMs, receiver);
    }

    /**
     * Returns all drivers who can occupy the driving zone. Guest users are included in the list.
     *
     * @return the list of {@link UserInfo} who can be a driver on the device.
     */
    @Override
    @NonNull
    public List<UserInfo> getAllDrivers() {
        checkManageUsersOrDumpPermission("getAllDrivers");
        return getUsers((user) -> !UserHelperLite.isHeadlessSystemUser(user.id) && user.isEnabled()
                && !user.isManagedProfile() && !user.isEphemeral());
    }

    /**
     * Returns all passengers under the given driver.
     *
     * @param driverId User id of a driver.
     * @return the list of {@link UserInfo} who is a passenger under the given driver.
     */
    @Override
    @NonNull
    public List<UserInfo> getPassengers(@UserIdInt int driverId) {
        checkManageUsersOrDumpPermission("getPassengers");
        return getUsers((user) -> {
            return !UserHelperLite.isHeadlessSystemUser(user.id) && user.isEnabled()
                    && user.isManagedProfile() && user.profileGroupId == driverId;
        });
    }

    /**
     * @see CarUserManager.startPassenger
     */
    @Override
    public boolean startPassenger(@UserIdInt int passengerId, int zoneId) {
        checkManageUsersPermission("startPassenger");
        synchronized (mLockUser) {
            try {
                if (!mAm.startUserInBackgroundWithListener(passengerId, null)) {
                    Slog.w(TAG, "could not start passenger");
                    return false;
                }
            } catch (RemoteException e) {
                // ignore
                Slog.w(TAG, "error while starting passenger", e);
                return false;
            }
            if (!assignUserToOccupantZone(passengerId, zoneId)) {
                Slog.w(TAG, "could not assign passenger to zone");
                return false;
            }
            mLastPassengerId = passengerId;
        }
        for (PassengerCallback callback : mPassengerCallbacks) {
            callback.onPassengerStarted(passengerId, zoneId);
        }
        return true;
    }

    /**
     * @see CarUserManager.stopPassenger
     */
    @Override
    public boolean stopPassenger(@UserIdInt int passengerId) {
        checkManageUsersPermission("stopPassenger");
        return stopPassengerInternal(passengerId, true);
    }

    private boolean stopPassengerInternal(@UserIdInt int passengerId, boolean checkCurrentDriver) {
        synchronized (mLockUser) {
            UserInfo passenger = mUserManager.getUserInfo(passengerId);
            if (passenger == null) {
                Slog.w(TAG, "passenger " + passengerId + " doesn't exist");
                return false;
            }
            if (mLastPassengerId != passengerId) {
                Slog.w(TAG, "passenger " + passengerId + " hasn't been started");
                return true;
            }
            if (checkCurrentDriver) {
                int currentUser = ActivityManager.getCurrentUser();
                if (passenger.profileGroupId != currentUser) {
                    Slog.w(TAG, "passenger " + passengerId
                            + " is not a profile of the current user");
                    return false;
                }
            }
            // Passenger is a profile, so cannot be stopped through activity manager.
            // Instead, activities started by the passenger are stopped and the passenger is
            // unassigned from the zone.
            stopAllTasks(passengerId);
            if (!unassignUserFromOccupantZone(passengerId)) {
                Slog.w(TAG, "could not unassign user from occupant zone");
                return false;
            }
            mLastPassengerId = UserHandle.USER_NULL;
        }
        for (PassengerCallback callback : mPassengerCallbacks) {
            callback.onPassengerStopped(passengerId);
        }
        return true;
    }

    private void stopAllTasks(@UserIdInt int userId) {
        try {
            for (RootTaskInfo info : mAm.getAllRootTaskInfos()) {
                for (int i = 0; i < info.childTaskIds.length; i++) {
                    if (info.childTaskUserIds[i] == userId) {
                        int taskId = info.childTaskIds[i];
                        if (!mAm.removeTask(taskId)) {
                            Slog.w(TAG, "could not remove task " + taskId);
                        }
                    }
                }
            }
        } catch (RemoteException e) {
            Slog.e(TAG, "could not get stack info", e);
        }
    }

    @Override
    public void setLifecycleListenerForUid(IResultReceiver listener) {
        int uid = Binder.getCallingUid();
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SET_LIFECYCLE_LISTENER, uid);
        checkInteractAcrossUsersPermission("setLifecycleListenerForUid" + uid);

        try {
            listener.asBinder().linkToDeath(() -> onListenerDeath(uid), 0);
        } catch (RemoteException e) {
            Slog.wtf(TAG, "Cannot listen to death of " + uid);
        }
        mHandler.post(() -> mAppLifecycleListeners.append(uid, listener));
    }

    private void onListenerDeath(int uid) {
        Slog.i(TAG, "Removing listeners for uid " + uid + " on binder death");
        mHandler.post(() -> mAppLifecycleListeners.remove(uid));
    }

    @Override
    public void resetLifecycleListenerForUid() {
        int uid = Binder.getCallingUid();
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_RESET_LIFECYCLE_LISTENER, uid);
        checkInteractAcrossUsersPermission("resetLifecycleListenerForUid-" + uid);
        mHandler.post(() -> mAppLifecycleListeners.remove(uid));
    }

    /**
     * Gets the initial foreground user after the device boots or resumes from suspension.
     *
     * <p>When the OEM supports the User HAL, the initial user won't be available until the HAL
     * returns the initial value to {@code CarService} - if HAL takes too long or times out, this
     * method returns {@code null}.
     *
     * <p>If the HAL eventually times out, {@code CarService} will fallback to its default behavior
     * (like switching to the last active user), and this method will return the result of such
     * operation.
     *
     * <p>Notice that if {@code CarService} crashes, subsequent calls to this method will return
     * {@code null}.
     *
     * @hide
     */
    @Nullable
    public UserInfo getInitialUser() {
        checkInteractAcrossUsersPermission("getInitialUser");
        synchronized (mLockUser) {
            return mInitialUser;
        }
    }

    /**
     * Sets the initial foreground user after the device boots or resumes from suspension.
     */
    public void setInitialUser(@Nullable UserInfo user) {
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SET_INITIAL_USER,
                user == null ? UserHandle.USER_NULL : user.id);
        synchronized (mLockUser) {
            mInitialUser = user;
        }
        if (user == null) {
            // This mean InitialUserSetter failed and could not fallback, so the initial user was
            // not switched (and most likely is SYSTEM_USER).
            // TODO(b/153104378): should we set it to ActivityManager.getCurrentUser() instead?
            Slog.wtf(TAG, "Initial user set to null");
        }
    }

    private void initResumeReplaceGuest() {
        int currentUserId = ActivityManager.getCurrentUser();
        UserInfo currentUser = mUserManager.getUserInfo(currentUserId);

        if (!mInitialUserSetter.canReplaceGuestUser(currentUser)) return; // Not a guest

        InitialUserInfo info =
                new InitialUserSetter.Builder(InitialUserSetter.TYPE_REPLACE_GUEST).build();

        mInitialUserSetter.set(info);
    }

    /**
     * Calls to switch user at the power suspend.
     *
     * <p><b>Note:</b> Should be used only by {@link CarPowerManagementService}
     *
     */
    public void onSuspend() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "onSuspend called.");
        }

        if (mSwitchGuestUserBeforeSleep) {
            initResumeReplaceGuest();
        }

        preCreateUsersInternal();
    }

    /**
     * Calls to switch user at the power resume.
     *
     * <p>
     * <b>Note:</b> Should be used only by {@link CarPowerManagementService}
     *
     */
    public void onResume() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "onResume called.");
        }

        mHandler.post(() -> initBootUser(InitialUserInfoRequestType.RESUME));
    }

    /**
     * Calls to start user at the android startup.
     */
    public void initBootUser() {
        mHandler.post(() -> initBootUser(getInitialUserInfoRequestType()));
    }

    private void initBootUser(int requestType) {
        boolean replaceGuest =
                requestType == InitialUserInfoRequestType.RESUME && !mSwitchGuestUserBeforeSleep;
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_INITIAL_USER_INFO_REQ, requestType,
                mHalTimeoutMs);
        checkManageUsersPermission("startInitialUser");

        if (!isUserHalSupported()) {
            fallbackToDefaultInitialUserBehavior(/* userLocales= */ null, replaceGuest);
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_INITIAL_USER_INFO_REQ_COMPLETE,
                    requestType);
            return;
        }

        UsersInfo usersInfo = UserHalHelper.newUsersInfo(mUserManager);
        mHal.getInitialUserInfo(requestType, mHalTimeoutMs, usersInfo, (status, resp) -> {
            if (resp != null) {
                EventLog.writeEvent(EventLogTags.CAR_USER_SVC_INITIAL_USER_INFO_RESP,
                        status, resp.action, resp.userToSwitchOrCreate.userId,
                        resp.userToSwitchOrCreate.flags, resp.userNameToCreate, resp.userLocales);

                String userLocales = resp.userLocales;
                InitialUserInfo info;
                switch (resp.action) {
                    case InitialUserInfoResponseAction.SWITCH:
                        int userId = resp.userToSwitchOrCreate.userId;
                        if (userId <= 0) {
                            Slog.w(TAG, "invalid (or missing) user id sent by HAL: " + userId);
                            fallbackToDefaultInitialUserBehavior(userLocales, replaceGuest);
                            break;
                        }
                        info = new InitialUserSetter.Builder(InitialUserSetter.TYPE_SWITCH)
                                .setUserLocales(userLocales)
                                .setSwitchUserId(userId)
                                .setReplaceGuest(replaceGuest)
                                .build();
                        mInitialUserSetter.set(info);
                        break;

                    case InitialUserInfoResponseAction.CREATE:
                        int halFlags = resp.userToSwitchOrCreate.flags;
                        String userName =  resp.userNameToCreate;
                        info = new InitialUserSetter.Builder(InitialUserSetter.TYPE_CREATE)
                                .setUserLocales(userLocales)
                                .setNewUserName(userName)
                                .setNewUserFlags(halFlags)
                                .build();
                        mInitialUserSetter.set(info);
                        break;

                    case InitialUserInfoResponseAction.DEFAULT:
                        fallbackToDefaultInitialUserBehavior(userLocales, replaceGuest);
                        break;
                    default:
                        Slog.w(TAG, "invalid response action on " + resp);
                        fallbackToDefaultInitialUserBehavior(/* user locale */ null, replaceGuest);
                        break;

                }
            } else {
                EventLog.writeEvent(EventLogTags.CAR_USER_SVC_INITIAL_USER_INFO_RESP, status);
                fallbackToDefaultInitialUserBehavior(/* user locale */ null, replaceGuest);
            }
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_INITIAL_USER_INFO_REQ_COMPLETE,
                    requestType);
        });
    }

    private void fallbackToDefaultInitialUserBehavior(String userLocales, boolean replaceGuest) {
        InitialUserInfo info = new InitialUserSetter.Builder(
                InitialUserSetter.TYPE_DEFAULT_BEHAVIOR)
                .setUserLocales(userLocales)
                .setReplaceGuest(replaceGuest)
                .build();
        mInitialUserSetter.set(info);
    }

    @VisibleForTesting
    int getInitialUserInfoRequestType() {
        if (!mInitialUserSetter.hasInitialUser()) {
            return InitialUserInfoRequestType.FIRST_BOOT;
        }
        if (mContext.getPackageManager().isDeviceUpgrading()) {
            return InitialUserInfoRequestType.FIRST_BOOT_AFTER_OTA;
        }
        return InitialUserInfoRequestType.COLD_BOOT;
    }

    /**
     * Sets the {@link ICarServiceHelper} so it can receive UX restriction updates.
     */
    public void setCarServiceHelper(ICarServiceHelper helper) {
        boolean restricted;
        synchronized (mLockUser) {
            mICarServiceHelper = helper;
            restricted = mUxRestricted;
        }
        updateSafetyMode(helper, restricted);
    }

    private void updateSafetyMode(@Nullable ICarServiceHelper helper, boolean restricted) {
        if (helper == null) return;

        boolean isSafe = !restricted;
        try {
            helper.setSafetyMode(isSafe);
        } catch (Exception e) {
            Slog.e(TAG, "Exception calling helper.setDpmSafetyMode(" + isSafe + ")", e);
        }
    }

    private void setUxRestrictions(@Nullable CarUxRestrictions restrictions) {
        boolean restricted = restrictions != null
                && (restrictions.getActiveRestrictions() & UX_RESTRICTIONS_NO_SETUP)
                        == UX_RESTRICTIONS_NO_SETUP;
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "setUxRestrictions(" + restrictions + "): restricted=" + restricted);
        } else {
            Slog.i(TAG, "Setting UX restricted to " + restricted);
        }

        ICarServiceHelper helper = null;

        synchronized (mLockUser) {
            mUxRestricted = restricted;
            if (mICarServiceHelper == null) {
                Slog.e(TAG, "onUxRestrictionsChanged(): no mICarServiceHelper");
            }
            helper = mICarServiceHelper;
        }
        updateSafetyMode(helper, restricted);
    }

    private boolean isUxRestricted() {
        synchronized (mLockUser) {
            return mUxRestricted;
        }
    }

    /**
     * Calls the {@link UserHalService} and {@link IActivityManager} for user switch.
     *
     * <p>
     * When everything works well, the workflow is:
     * <ol>
     *   <li> {@link UserHalService} is called for HAL user switch with ANDROID_SWITCH request
     *   type, current user id, target user id, and a callback.
     *   <li> HAL called back with SUCCESS.
     *   <li> {@link IActivityManager} is called for Android user switch.
     *   <li> Receiver would receive {@code STATUS_SUCCESSFUL}.
     *   <li> Once user is unlocked, {@link UserHalService} is again called with ANDROID_POST_SWITCH
     *   request type, current user id, and target user id. In this case, the current and target
     *   user IDs would be same.
     * <ol/>
     *
     * <p>
     * Corner cases:
     * <ul>
     *   <li> If target user is already the current user, no user switch is performed and receiver
     *   would receive {@code STATUS_OK_USER_ALREADY_IN_FOREGROUND} right away.
     *   <li> If HAL user switch call fails, no Android user switch. Receiver would receive
     *   {@code STATUS_HAL_INTERNAL_FAILURE}.
     *   <li> If HAL user switch call is successful, but android user switch call fails,
     *   {@link UserHalService} is again called with request type POST_SWITCH, current user id, and
     *   target user id, but in this case the current and target user IDs would be different.
     *   <li> If another user switch request for the same target user is received while previous
     *   request is in process, receiver would receive
     *   {@code STATUS_TARGET_USER_ALREADY_BEING_SWITCHED_TO} for the new request right away.
     *   <li> If a user switch request is received while another user switch request for different
     *   target user is in process, the previous request would be abandoned and new request will be
     *   processed. No POST_SWITCH would be sent for the previous request.
     * <ul/>
     *
     * @param targetUserId - target user Id
     * @param timeoutMs - timeout for HAL to wait
     * @param receiver - receiver for the results
     */
    @Override
    public void switchUser(@UserIdInt int targetUserId, int timeoutMs,
            @NonNull AndroidFuture<UserSwitchResult> receiver) {
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SWITCH_USER_REQ, targetUserId, timeoutMs);
        checkManageOrCreateUsersPermission("switchUser");
        Objects.requireNonNull(receiver);
        UserInfo targetUser = mUserManager.getUserInfo(targetUserId);
        Preconditions.checkArgument(targetUser != null, "Target user doesn't exist");
        if (mUserManager.getUserSwitchability() != UserManager.SWITCHABILITY_STATUS_OK) {
            sendUserSwitchResult(receiver, UserSwitchResult.STATUS_NOT_SWITCHABLE);
            return;
        }
        mHandler.post(() -> handleSwitchUser(targetUser, timeoutMs, receiver));
    }

    private void handleSwitchUser(@NonNull UserInfo targetUser, int timeoutMs,
            @NonNull AndroidFuture<UserSwitchResult> receiver) {
        int currentUser = ActivityManager.getCurrentUser();
        int targetUserId = targetUser.id;
        if (currentUser == targetUserId) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "Current user is same as requested target user: " + targetUserId);
            }
            int resultStatus = UserSwitchResult.STATUS_OK_USER_ALREADY_IN_FOREGROUND;
            sendUserSwitchResult(receiver, resultStatus);
            return;
        }

        if (isUxRestricted()) {
            sendUserSwitchResult(receiver, UserSwitchResult.STATUS_UX_RESTRICTION_FAILURE);
            return;
        }

        // If User Hal is not supported, just android user switch.
        if (!isUserHalSupported()) {
            try {
                if (mAm.switchUser(targetUserId)) {
                    sendUserSwitchResult(receiver, UserSwitchResult.STATUS_SUCCESSFUL);
                    return;
                }
            } catch (RemoteException e) {
                // ignore
                Slog.w(TAG, "error while switching user " + targetUser.toFullString(), e);
            }
            sendUserSwitchResult(receiver, UserSwitchResult.STATUS_ANDROID_FAILURE);
            return;
        }

        synchronized (mLockUser) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "switchUser(" + targetUserId + "): currentuser=" + currentUser
                        + ", mUserIdForUserSwitchInProcess=" + mUserIdForUserSwitchInProcess);
            }

            // If there is another request for the same target user, return another request in
            // process, else {@link mUserIdForUserSwitchInProcess} is updated and {@link
            // mRequestIdForUserSwitchInProcess} is reset. It is possible that there may be another
            // user switch request in process for different target user, but that request is now
            // ignored.
            if (mUserIdForUserSwitchInProcess == targetUserId) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG,
                            "Another user switch request in process for the requested target user: "
                                    + targetUserId);
                }

                int resultStatus = UserSwitchResult.STATUS_TARGET_USER_ALREADY_BEING_SWITCHED_TO;
                sendUserSwitchResult(receiver, resultStatus);
                return;
            } else {
                mUserIdForUserSwitchInProcess = targetUserId;
                mRequestIdForUserSwitchInProcess = 0;
            }
        }

        UsersInfo usersInfo = UserHalHelper.newUsersInfo(mUserManager);
        SwitchUserRequest request = createUserSwitchRequest(targetUserId, usersInfo);

        mHal.switchUser(request, timeoutMs, (halCallbackStatus, resp) -> {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "switch response: status="
                        + UserHalHelper.halCallbackStatusToString(halCallbackStatus)
                        + ", resp=" + resp);
            }

            int resultStatus = UserSwitchResult.STATUS_HAL_INTERNAL_FAILURE;

            synchronized (mLockUser) {
                if (halCallbackStatus != HalCallback.STATUS_OK) {
                    Slog.w(TAG, "invalid callback status ("
                            + UserHalHelper.halCallbackStatusToString(halCallbackStatus)
                            + ") for response " + resp);
                    sendUserSwitchResult(receiver, resultStatus);
                    mUserIdForUserSwitchInProcess = UserHandle.USER_NULL;
                    return;
                }

                if (mUserIdForUserSwitchInProcess != targetUserId) {
                    // Another user switch request received while HAL responded. No need to process
                    // this request further
                    if (Log.isLoggable(TAG, Log.DEBUG)) {
                        Slog.d(TAG, "Another user switch received while HAL responsed. Request"
                                + " abondoned for : " + targetUserId + ". Current user in process: "
                                + mUserIdForUserSwitchInProcess);
                    }
                    resultStatus =
                            UserSwitchResult.STATUS_TARGET_USER_ABANDONED_DUE_TO_A_NEW_REQUEST;
                    sendUserSwitchResult(receiver, resultStatus);
                    mUserIdForUserSwitchInProcess = UserHandle.USER_NULL;
                    return;
                }

                switch (resp.status) {
                    case SwitchUserStatus.SUCCESS:
                        boolean switched;
                        try {
                            switched = mAm.switchUser(targetUserId);
                            if (switched) {
                                sendUserSwitchUiCallback(targetUserId);
                                resultStatus = UserSwitchResult.STATUS_SUCCESSFUL;
                                mRequestIdForUserSwitchInProcess = resp.requestId;
                            } else {
                                resultStatus = UserSwitchResult.STATUS_ANDROID_FAILURE;
                                postSwitchHalResponse(resp.requestId, targetUserId);
                            }
                        } catch (RemoteException e) {
                            // ignore
                            Slog.w(TAG,
                                    "error while switching user " + targetUser.toFullString(), e);
                        }
                        break;
                    case SwitchUserStatus.FAILURE:
                        // HAL failed to switch user
                        resultStatus = UserSwitchResult.STATUS_HAL_FAILURE;
                        break;
                    default:
                        // Shouldn't happen because UserHalService validates the status
                        Slog.wtf(TAG, "Received invalid user switch status from HAL: " + resp);
                }

                if (mRequestIdForUserSwitchInProcess == 0) {
                    mUserIdForUserSwitchInProcess = UserHandle.USER_NULL;
                }
            }
            sendUserSwitchResult(receiver, halCallbackStatus, resultStatus, resp.errorMessage);
        });
    }

    @Override
    public void removeUser(@UserIdInt int userId, AndroidFuture<UserRemovalResult> receiver) {
        removeUser(userId, /* hasCallerRestrictions= */ false, receiver);
    }

    /**
     * Internal implementation of {@code removeUser()}, which is used by both
     * {@code ICarUserService} and {@code ICarDevicePolicyService}.
     *
     * @param userId user to be removed
     * @param hasCallerRestrictions when {@code true}, if the caller user is not an admin, it can
     * only remove itself.
     * @param receiver to post results
     */
    public void removeUser(@UserIdInt int userId, boolean hasCallerRestrictions,
            AndroidFuture<UserRemovalResult> receiver) {
        checkManageOrCreateUsersPermission("removeUser");
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_REMOVE_USER_REQ, userId,
                hasCallerRestrictions ? 1 : 0);

        if (hasCallerRestrictions) {
            // Restrictions: non-admin user can only remove itself, admins have no restrictions
            int callingUserId = Binder.getCallingUserHandle().getIdentifier();
            UserInfo callingUser = mUserManager.getUserInfo(callingUserId);
            if (!callingUser.isAdmin() && userId != callingUserId) {
                throw new SecurityException("Non-admin user " + callingUserId
                        + " can only remove itself");
            }
        }
        mHandler.post(() -> handleRemoveUser(userId, hasCallerRestrictions, receiver));
    }

    private void handleRemoveUser(@UserIdInt int userId, boolean hasCallerRestrictions,
            AndroidFuture<UserRemovalResult> receiver) {
        UserInfo userInfo = mUserManager.getUserInfo(userId);
        if (userInfo == null) {
            sendUserRemovalResult(userId, UserRemovalResult.STATUS_USER_DOES_NOT_EXIST, receiver);
            return;
        }
        android.hardware.automotive.vehicle.V2_0.UserInfo halUser =
                new android.hardware.automotive.vehicle.V2_0.UserInfo();
        halUser.userId = userInfo.id;
        halUser.flags = UserHalHelper.convertFlags(userInfo);
        UsersInfo usersInfo = UserHalHelper.newUsersInfo(mUserManager);

        // check if the user is last admin user.
        boolean isLastAdmin = false;
        if (UserHalHelper.isAdmin(halUser.flags)) {
            int size = usersInfo.existingUsers.size();
            int totalAdminUsers = 0;
            for (int i = 0; i < size; i++) {
                if (UserHalHelper.isAdmin(usersInfo.existingUsers.get(i).flags)) {
                    totalAdminUsers++;
                }
            }
            if (totalAdminUsers == 1) {
                isLastAdmin = true;
            }
        }

        // First remove user from android and then remove from HAL because HAL remove user is one
        // way call.
        // TODO(b/170887769): rename hasCallerRestrictions to fromCarDevicePolicyManager (or use an
        // int / enum to indicate if it's called from CarUserManager or CarDevicePolicyManager), as
        // it's counter-intuitive that it's "allowed even when disallowed" when it
        // "has caller restrictions"
        boolean evenWhenDisallowed = hasCallerRestrictions;
        int result = mUserManager.removeUserOrSetEphemeral(userId, evenWhenDisallowed);
        if (result == UserManager.REMOVE_RESULT_ERROR) {
            sendUserRemovalResult(userId, UserRemovalResult.STATUS_ANDROID_FAILURE, receiver);
            return;
        }

        if (isLastAdmin) {
            Slog.w(TAG,
                    "Last admin user successfully removed or set ephemeral. User Id: " + userId);
        }

        switch (result) {
            case UserManager.REMOVE_RESULT_REMOVED:
            case UserManager.REMOVE_RESULT_ALREADY_BEING_REMOVED:
                sendUserRemovalResult(userId,
                        isLastAdmin ? UserRemovalResult.STATUS_SUCCESSFUL_LAST_ADMIN_REMOVED
                                : UserRemovalResult.STATUS_SUCCESSFUL, receiver);
            case UserManager.REMOVE_RESULT_SET_EPHEMERAL:
                sendUserRemovalResult(userId,
                        isLastAdmin ? UserRemovalResult.STATUS_SUCCESSFUL_LAST_ADMIN_SET_EPHEMERAL
                                : UserRemovalResult.STATUS_SUCCESSFUL_SET_EPHEMERAL, receiver);
            default:
                sendUserRemovalResult(userId, UserRemovalResult.STATUS_ANDROID_FAILURE, receiver);
        }
    }

    /**
     * Should be called by {@code ICarImpl} only.
     */
    public void onUserRemoved(@NonNull UserInfo user) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "onUserRemoved: " + user.toFullString());
        }
        notifyHalUserRemoved(user);
    }

    private void notifyHalUserRemoved(@NonNull UserInfo user) {
        if (!isUserHalSupported()) return;

        if (user == null) {
            Slog.wtf(TAG, "notifyHalUserRemoved() called for null user");
            return;
        }

        int userId = user.id;

        if (userId == UserHandle.USER_NULL) {
            Slog.wtf(TAG, "notifyHalUserRemoved() called for UserHandle.USER_NULL");
            return;
        }

        synchronized (mLockUser) {
            if (mFailedToCreateUserIds.get(userId)) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG, "notifyHalUserRemoved(): skipping " + userId);
                }
                mFailedToCreateUserIds.delete(userId);
                return;
            }
        }

        android.hardware.automotive.vehicle.V2_0.UserInfo halUser =
                new android.hardware.automotive.vehicle.V2_0.UserInfo();
        halUser.userId = userId;
        halUser.flags = UserHalHelper.convertFlags(user);

        RemoveUserRequest request = new RemoveUserRequest();
        request.removedUserInfo = halUser;
        request.usersInfo = UserHalHelper.newUsersInfo(mUserManager);
        mHal.removeUser(request);
    }

    private void sendUserRemovalResult(@UserIdInt int userId, @UserRemovalResult.Status int result,
            AndroidFuture<UserRemovalResult> receiver) {
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_REMOVE_USER_RESP, userId, result);
        receiver.complete(new UserRemovalResult(result));
    }

    private void sendUserSwitchUiCallback(@UserIdInt int targetUserId) {
        if (mUserSwitchUiReceiver == null) {
            Slog.w(TAG, "No User switch UI receiver.");
            return;
        }

        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SWITCH_USER_UI_REQ, targetUserId);
        try {
            mUserSwitchUiReceiver.send(targetUserId, null);
        } catch (RemoteException e) {
            Slog.e(TAG, "Error calling user switch UI receiver.", e);
        }
    }

    /**
     * Used to create the initial user, even when it's disallowed by {@code DevicePolicyManager}.
     */
    @Nullable
    UserInfo createUserEvenWhenDisallowed(@Nullable String name, @NonNull String userType,
            @UserInfoFlag int flags) {
        if (mICarServiceHelper == null) {
            Slog.wtf(TAG, "createUserEvenWhenDisallowed(): mICarServiceHelper not set yet",
                    new Exception());
            return null;
        }
        try {
            return mICarServiceHelper.createUserEvenWhenDisallowed(name, userType, flags);
        } catch (RemoteException e) {
            Slog.e(TAG, "createUserEvenWhenDisallowed(" + UserHelperLite.safeName(name) + ", "
                    + userType + ", " + UserInfo.flagsToString(flags) + ") failed", e);
            return null;
        }
    }

    @Override
    public void createUser(@Nullable String name, @NonNull String userType, @UserInfoFlag int flags,
            int timeoutMs, @NonNull AndroidFuture<UserCreationResult> receiver) {
        createUser(name, userType, flags, timeoutMs, receiver, /* hasCallerRestrictions= */ false);
    }

    /**
     * Internal implementation of {@code createUser()}, which is used by both
     * {@code ICarUserService} and {@code ICarDevicePolicyService}.
     *
     * @param hasCallerRestrictions when {@code true}, if the caller user is not an admin, it can
     * only create admin users
     */
    public void createUser(@Nullable String name, @NonNull String userType, @UserInfoFlag int flags,
            int timeoutMs, @NonNull AndroidFuture<UserCreationResult> receiver,
            boolean hasCallerRestrictions) {
        Objects.requireNonNull(userType, "user type cannot be null");
        Objects.requireNonNull(receiver, "receiver cannot be null");
        checkManageOrCreateUsersPermission(flags);
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_CREATE_USER_REQ,
                UserHelperLite.safeName(name), userType, flags, timeoutMs,
                hasCallerRestrictions ? 1 : 0);
        mHandler.post(() -> handleCreateUser(name, userType, flags, timeoutMs, receiver,
                hasCallerRestrictions));

    }

    private void handleCreateUser(@Nullable String name, @NonNull String userType,
            @UserInfoFlag int flags, int timeoutMs,
            @NonNull AndroidFuture<UserCreationResult> receiver,
            boolean hasCallerRestrictions) {
        if (hasCallerRestrictions) {
            // Restrictions:
            // - type/flag can only be normal user, admin, or guest
            // - non-admin user can only create non-admin users

            boolean validCombination;
            switch (userType) {
                case UserManager.USER_TYPE_FULL_SECONDARY:
                    validCombination = flags == 0
                        || (flags & UserInfo.FLAG_ADMIN) == UserInfo.FLAG_ADMIN;
                    break;
                case UserManager.USER_TYPE_FULL_GUEST:
                    validCombination = flags == 0;
                    break;
                default:
                    validCombination = false;
            }
            if (!validCombination) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG, "Invalid combination of user type(" + userType
                            + ") and flags (" + UserInfo.flagsToString(flags)
                            + ") for caller with restrictions");
                }
                sendUserCreationResultFailure(receiver, UserCreationResult.STATUS_INVALID_REQUEST);
                return;

            }

            int callingUserId = Binder.getCallingUserHandle().getIdentifier();
            UserInfo callingUser = mUserManager.getUserInfo(callingUserId);
            if (!callingUser.isAdmin() && (flags & UserInfo.FLAG_ADMIN) == UserInfo.FLAG_ADMIN) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG, "Non-admin user " + callingUserId
                            + " can only create non-admin users");
                }
                sendUserCreationResultFailure(receiver, UserCreationResult.STATUS_INVALID_REQUEST);
                return;
            }

        }

        UserInfo newUser;
        try {
            newUser = mUserManager.createUser(name, userType, flags);
            if (newUser == null) {
                Slog.w(TAG, "um.createUser() returned null for user of type " + userType
                        + " and flags " + UserInfo.flagsToString(flags));
                sendUserCreationResultFailure(receiver, UserCreationResult.STATUS_ANDROID_FAILURE);
                return;
            }
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "Created user: " + newUser.toFullString());
            }
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_CREATE_USER_USER_CREATED, newUser.id,
                    UserHelperLite.safeName(newUser.name), newUser.userType, newUser.flags);
        } catch (RuntimeException e) {
            Slog.e(TAG, "Error creating user of type " + userType + " and flags"
                    + UserInfo.flagsToString(flags), e);
            sendUserCreationResultFailure(receiver, UserCreationResult.STATUS_ANDROID_FAILURE);
            return;
        }

        if (!isUserHalSupported()) {
            sendUserCreationResult(receiver, UserCreationResult.STATUS_SUCCESSFUL, newUser, null);
            return;
        }

        CreateUserRequest request = new CreateUserRequest();
        request.usersInfo = UserHalHelper.newUsersInfo(mUserManager);
        if (!TextUtils.isEmpty(name)) {
            request.newUserName = name;
        }
        request.newUserInfo.userId = newUser.id;
        request.newUserInfo.flags = UserHalHelper.convertFlags(newUser);
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "Create user request: " + request);
        }

        try {
            mHal.createUser(request, timeoutMs, (status, resp) -> {
                int resultStatus = UserCreationResult.STATUS_HAL_INTERNAL_FAILURE;
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG, "createUserResponse: status="
                            + UserHalHelper.halCallbackStatusToString(status) + ", resp=" + resp);
                }
                UserInfo user = null; // user returned in the result
                if (status != HalCallback.STATUS_OK) {
                    Slog.w(TAG, "invalid callback status ("
                            + UserHalHelper.halCallbackStatusToString(status) + ") for response "
                            + resp);
                    EventLog.writeEvent(EventLogTags.CAR_USER_SVC_CREATE_USER_RESP, status,
                            resultStatus, resp.errorMessage);
                    removeCreatedUser(newUser, "HAL call failed with "
                            + UserHalHelper.halCallbackStatusToString(status));
                    sendUserCreationResult(receiver, resultStatus, user, /* errorMsg= */ null);
                    return;
                }

                switch (resp.status) {
                    case CreateUserStatus.SUCCESS:
                        resultStatus = UserCreationResult.STATUS_SUCCESSFUL;
                        user = newUser;
                        break;
                    case CreateUserStatus.FAILURE:
                        // HAL failed to switch user
                        resultStatus = UserCreationResult.STATUS_HAL_FAILURE;
                        break;
                    default:
                        // Shouldn't happen because UserHalService validates the status
                        Slog.wtf(TAG, "Received invalid user switch status from HAL: " + resp);
                }
                EventLog.writeEvent(EventLogTags.CAR_USER_SVC_CREATE_USER_RESP, status,
                        resultStatus, resp.errorMessage);
                if (user == null) {
                    removeCreatedUser(newUser, "HAL returned "
                            + UserCreationResult.statusToString(resultStatus));
                }
                sendUserCreationResult(receiver, resultStatus, user, resp.errorMessage);
            });
        } catch (Exception e) {
            Slog.w(TAG, "mHal.createUser(" + request + ") failed", e);
            removeCreatedUser(newUser, "mHal.createUser() failed");
            sendUserCreationResultFailure(receiver, UserCreationResult.STATUS_HAL_INTERNAL_FAILURE);
        }
    }

    private void removeCreatedUser(@NonNull UserInfo user, @NonNull String reason) {
        Slog.i(TAG, "removing " + user.toFullString() + "; reason: " + reason);

        int userId = user.id;
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_CREATE_USER_USER_REMOVED, userId, reason);

        synchronized (mLockUser) {
            mFailedToCreateUserIds.put(userId, true);
        }

        try {
            if (!mUserManager.removeUser(userId)) {
                Slog.w(TAG, "Failed to remove user " + user.toFullString());
            }
        } catch (Exception e) {
            Slog.e(TAG, "Failed to remove user " + user.toFullString(), e);
        }
    }

    @Override
    public UserIdentificationAssociationResponse getUserIdentificationAssociation(
            @UserIdentificationAssociationType int[] types) {
        if (!isUserHalUserAssociationSupported()) {
            return UserIdentificationAssociationResponse.forFailure(VEHICLE_HAL_NOT_SUPPORTED);
        }

        Preconditions.checkArgument(!ArrayUtils.isEmpty(types), "must have at least one type");
        checkManageOrCreateUsersPermission("getUserIdentificationAssociation");

        int uid = getCallingUid();
        int userId = UserHandle.getUserId(uid);
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_GET_USER_AUTH_REQ, uid, userId);

        UserIdentificationGetRequest request = new UserIdentificationGetRequest();
        request.userInfo.userId = userId;
        request.userInfo.flags = getHalUserInfoFlags(userId);

        request.numberAssociationTypes = types.length;
        for (int i = 0; i < types.length; i++) {
            request.associationTypes.add(types[i]);
        }

        UserIdentificationResponse halResponse = mHal.getUserAssociation(request);
        if (halResponse == null) {
            Slog.w(TAG, "getUserIdentificationAssociation(): HAL returned null for "
                    + Arrays.toString(types));
            return UserIdentificationAssociationResponse.forFailure();
        }

        int[] values = new int[halResponse.associations.size()];
        for (int i = 0; i < values.length; i++) {
            values[i] = halResponse.associations.get(i).value;
        }
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_GET_USER_AUTH_RESP, values.length);

        return UserIdentificationAssociationResponse.forSuccess(values, halResponse.errorMessage);
    }

    @Override
    public void setUserIdentificationAssociation(int timeoutMs,
            @UserIdentificationAssociationType int[] types,
            @UserIdentificationAssociationSetValue int[] values,
            AndroidFuture<UserIdentificationAssociationResponse> result) {
        if (!isUserHalUserAssociationSupported()) {
            result.complete(
                    UserIdentificationAssociationResponse.forFailure(VEHICLE_HAL_NOT_SUPPORTED));
            return;
        }

        Preconditions.checkArgument(!ArrayUtils.isEmpty(types), "must have at least one type");
        Preconditions.checkArgument(!ArrayUtils.isEmpty(values), "must have at least one value");
        if (types.length != values.length) {
            throw new IllegalArgumentException("types (" + Arrays.toString(types) + ") and values ("
                    + Arrays.toString(values) + ") should have the same length");
        }
        checkManageOrCreateUsersPermission("setUserIdentificationAssociation");

        int uid = getCallingUid();
        int userId = UserHandle.getUserId(uid);
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SET_USER_AUTH_REQ, uid, userId, types.length);

        UserIdentificationSetRequest request = new UserIdentificationSetRequest();
        request.userInfo.userId = userId;
        request.userInfo.flags = getHalUserInfoFlags(userId);

        request.numberAssociations = types.length;
        for (int i = 0; i < types.length; i++) {
            UserIdentificationSetAssociation association = new UserIdentificationSetAssociation();
            association.type = types[i];
            association.value = values[i];
            request.associations.add(association);
        }

        mHal.setUserAssociation(timeoutMs, request, (status, resp) -> {
            if (status != HalCallback.STATUS_OK) {
                Slog.w(TAG, "setUserIdentificationAssociation(): invalid callback status ("
                        + UserHalHelper.halCallbackStatusToString(status) + ") for response "
                        + resp);
                if (resp == null || TextUtils.isEmpty(resp.errorMessage)) {
                    EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SET_USER_AUTH_RESP, 0);
                    result.complete(UserIdentificationAssociationResponse.forFailure());
                    return;
                }
                EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SET_USER_AUTH_RESP, 0,
                        resp.errorMessage);
                result.complete(
                        UserIdentificationAssociationResponse.forFailure(resp.errorMessage));
                return;
            }
            int respSize = resp.associations.size();
            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SET_USER_AUTH_RESP, respSize,
                    resp.errorMessage);

            int[] responseTypes = new int[respSize];
            for (int i = 0; i < respSize; i++) {
                responseTypes[i] = resp.associations.get(i).value;
            }
            UserIdentificationAssociationResponse response = UserIdentificationAssociationResponse
                    .forSuccess(responseTypes, resp.errorMessage);
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "setUserIdentificationAssociation(): resp= " + resp
                        + ", converted=" + response);
            }
            result.complete(response);
        });
    }

    /**
     * Gets the User HAL flags for the given user.
     *
     * @throws IllegalArgumentException if the user does not exist.
     */
    private int getHalUserInfoFlags(@UserIdInt int userId) {
        UserInfo user = mUserManager.getUserInfo(userId);
        Preconditions.checkArgument(user != null, "no user for id %d", userId);
        return UserHalHelper.convertFlags(user);
    }

    private void sendResult(@NonNull IResultReceiver receiver, int resultCode,
            @Nullable Bundle resultData) {
        try {
            receiver.send(resultCode, resultData);
        } catch (RemoteException e) {
            // ignore
            Slog.w(TAG, "error while sending results", e);
        }
    }

    private void sendUserSwitchResult(@NonNull AndroidFuture<UserSwitchResult> receiver,
            @UserSwitchResult.Status int userSwitchStatus) {
        sendUserSwitchResult(receiver, HalCallback.STATUS_INVALID, userSwitchStatus,
                /* errorMessage= */ null);
    }

    private void sendUserSwitchResult(@NonNull AndroidFuture<UserSwitchResult> receiver,
            @HalCallback.HalCallbackStatus int halCallbackStatus,
            @UserSwitchResult.Status int userSwitchStatus, @Nullable String errorMessage) {
        if (errorMessage != null) {
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SWITCH_USER_RESP, halCallbackStatus,
                    userSwitchStatus, errorMessage);
        } else {
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SWITCH_USER_RESP, halCallbackStatus,
                    userSwitchStatus);
        }
        receiver.complete(new UserSwitchResult(userSwitchStatus, errorMessage));
    }

    private void sendUserCreationResultFailure(@NonNull AndroidFuture<UserCreationResult> receiver,
            @UserCreationResult.Status int status) {
        sendUserCreationResult(receiver, status, /* user= */ null, /* errorMessage= */ null);
    }

    private void sendUserCreationResult(@NonNull AndroidFuture<UserCreationResult> receiver,
            @UserCreationResult.Status int status, @NonNull UserInfo user,
            @Nullable String errorMessage) {
        if (TextUtils.isEmpty(errorMessage)) {
            errorMessage = null;
        }
        receiver.complete(new UserCreationResult(status, user, errorMessage));
    }

    /**
     * Calls activity manager for user switch.
     *
     * <p><b>NOTE</b> This method is meant to be called just by UserHalService.
     *
     * @param requestId for the user switch request
     * @param targetUserId of the target user
     *
     * @hide
     */
    public void switchAndroidUserFromHal(int requestId, @UserIdInt int targetUserId) {
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_SWITCH_USER_FROM_HAL_REQ, requestId,
                targetUserId);
        Slog.i(TAG, "User hal requested a user switch. Target user id " + targetUserId);

        try {
            boolean result = mAm.switchUser(targetUserId);
            if (result) {
                updateUserSwitchInProcess(requestId, targetUserId);
            } else {
                postSwitchHalResponse(requestId, targetUserId);
            }
        } catch (RemoteException e) {
            // ignore
            Slog.w(TAG, "error while switching user " + targetUserId, e);
        }
    }

    private void updateUserSwitchInProcess(int requestId, @UserIdInt int targetUserId) {
        synchronized (mLockUser) {
            if (mUserIdForUserSwitchInProcess != UserHandle.USER_NULL) {
                // Some other user switch is in process.
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG, "User switch for user: " + mUserIdForUserSwitchInProcess
                            + " is in process. Abandoning it as a new user switch is requested"
                            + " for the target user: " + targetUserId);
                }
            }
            mUserIdForUserSwitchInProcess = targetUserId;
            mRequestIdForUserSwitchInProcess = requestId;
        }
    }

    private void postSwitchHalResponse(int requestId, @UserIdInt int targetUserId) {
        if (!isUserHalSupported()) return;

        UsersInfo usersInfo = UserHalHelper.newUsersInfo(mUserManager);
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_POST_SWITCH_USER_REQ, requestId,
                targetUserId, usersInfo.currentUser.userId);
        SwitchUserRequest request = createUserSwitchRequest(targetUserId, usersInfo);
        request.requestId = requestId;
        mHal.postSwitchResponse(request);
    }

    private SwitchUserRequest createUserSwitchRequest(@UserIdInt int targetUserId,
            @NonNull UsersInfo usersInfo) {
        UserInfo targetUser = mUserManager.getUserInfo(targetUserId);
        android.hardware.automotive.vehicle.V2_0.UserInfo halTargetUser =
                new android.hardware.automotive.vehicle.V2_0.UserInfo();
        halTargetUser.userId = targetUser.id;
        halTargetUser.flags = UserHalHelper.convertFlags(targetUser);
        SwitchUserRequest request = new SwitchUserRequest();
        request.targetUser = halTargetUser;
        request.usersInfo = usersInfo;
        return request;
    }

    /**
     * Checks if the User HAL is supported.
     */
    public boolean isUserHalSupported() {
        return mHal.isSupported();
    }

    /**
     * Checks if the User HAL user association is supported.
     */
    @Override
    public boolean isUserHalUserAssociationSupported() {
        return mHal.isUserAssociationSupported();
    }

    /**
     * Sets a callback which is invoked before user switch.
     *
     * <p>
     * This method should only be called by the Car System UI. The purpose of this call is to notify
     * Car System UI to show the user switch UI before the user switch.
     */
    @Override
    public void setUserSwitchUiCallback(@NonNull IResultReceiver receiver) {
        checkManageUsersPermission("setUserSwitchUiCallback");

        // Confirm that caller is system UI.
        String systemUiPackageName = getSystemUiPackageName();
        if (systemUiPackageName == null) {
            throw new IllegalStateException("System UI package not found.");
        }

        try {
            int systemUiUid = mContext
                    .createContextAsUser(UserHandle.SYSTEM, /* flags= */ 0).getPackageManager()
                    .getPackageUid(systemUiPackageName, PackageManager.MATCH_SYSTEM_ONLY);
            int callerUid = Binder.getCallingUid();
            if (systemUiUid != callerUid) {
                throw new SecurityException("Invalid caller. Only" + systemUiPackageName
                        + " is allowed to make this call");
            }
        } catch (NameNotFoundException e) {
            throw new IllegalStateException("Package " + systemUiPackageName + " not found.");
        }

        mUserSwitchUiReceiver = receiver;
    }

    // TODO(157082995): This information can be taken from
    // PackageManageInternalImpl.getSystemUiServiceComponent
    @Nullable
    private String getSystemUiPackageName() {
        try {
            ComponentName componentName = ComponentName.unflattenFromString(mContext.getResources()
                    .getString(com.android.internal.R.string.config_systemUIServiceComponent));
            return componentName.getPackageName();
        } catch (RuntimeException e) {
            Slog.w(TAG, "error while getting system UI package name.", e);
            return null;
        }
    }

    private void updateDefaultUserRestriction() {
        // We want to set restrictions on system and guest users only once. These are persisted
        // onto disk, so it's sufficient to do it once + we minimize the number of disk writes.
        if (Settings.Global.getInt(mContext.getContentResolver(),
                CarSettings.Global.DEFAULT_USER_RESTRICTIONS_SET, /* default= */ 0) != 0) {
            return;
        }
        // Only apply the system user restrictions if the system user is headless.
        if (UserManager.isHeadlessSystemUserMode()) {
            setSystemUserRestrictions();
        }
        Settings.Global.putInt(mContext.getContentResolver(),
                CarSettings.Global.DEFAULT_USER_RESTRICTIONS_SET, 1);
    }

    private boolean isPersistentUser(@UserIdInt int userId) {
        return !mUserManager.getUserInfo(userId).isEphemeral();
    }

    /**
     * Adds a new {@link UserLifecycleListener} to listen to user activity events.
     */
    public void addUserLifecycleListener(@NonNull UserLifecycleListener listener) {
        Objects.requireNonNull(listener, "listener cannot be null");
        mHandler.post(() -> mUserLifecycleListeners.add(listener));
    }

    /**
     * Removes previously added {@link UserLifecycleListener}.
     */
    public void removeUserLifecycleListener(@NonNull UserLifecycleListener listener) {
        Objects.requireNonNull(listener, "listener cannot be null");
        mHandler.post(() -> mUserLifecycleListeners.remove(listener));
    }

    /** Adds callback to listen to passenger activity events. */
    public void addPassengerCallback(@NonNull PassengerCallback callback) {
        Objects.requireNonNull(callback, "callback cannot be null");
        mPassengerCallbacks.add(callback);
    }

    /** Removes previously added callback to listen passenger events. */
    public void removePassengerCallback(@NonNull PassengerCallback callback) {
        Objects.requireNonNull(callback, "callback cannot be null");
        mPassengerCallbacks.remove(callback);
    }

    /** Sets the implementation of ZoneUserBindingHelper. */
    public void setZoneUserBindingHelper(@NonNull ZoneUserBindingHelper helper) {
        synchronized (mLockHelper) {
            mZoneUserBindingHelper = helper;
        }
    }

    private void onUserUnlocked(@UserIdInt int userId) {
        ArrayList<Runnable> tasks = null;
        synchronized (mLockUser) {
            sendPostSwitchToHalLocked(userId);
            if (userId == UserHandle.USER_SYSTEM) {
                if (!mUser0Unlocked) { // user 0, unlocked, do this only once
                    updateDefaultUserRestriction();
                    tasks = new ArrayList<>(mUser0UnlockTasks);
                    mUser0UnlockTasks.clear();
                    mUser0Unlocked = true;
                }
            } else { // none user0
                Integer user = userId;
                if (isPersistentUser(userId)) {
                    // current foreground user should stay in top priority.
                    if (userId == ActivityManager.getCurrentUser()) {
                        mBackgroundUsersToRestart.remove(user);
                        mBackgroundUsersToRestart.add(0, user);
                    }
                    // -1 for user 0
                    if (mBackgroundUsersToRestart.size() > (mMaxRunningUsers - 1)) {
                        int userToDrop = mBackgroundUsersToRestart.get(
                                mBackgroundUsersToRestart.size() - 1);
                        Slog.i(TAG, "New user unlocked:" + userId
                                + ", dropping least recently user from restart list:" + userToDrop);
                        // Drop the least recently used user.
                        mBackgroundUsersToRestart.remove(mBackgroundUsersToRestart.size() - 1);
                    }
                }
            }
        }
        if (tasks != null && tasks.size() > 0) {
            Slog.d(TAG, "User0 unlocked, run queued tasks:" + tasks.size());
            for (Runnable r : tasks) {
                r.run();
            }
        }
    }

    /**
     * Starts the specified user in the background.
     *
     * @param userId user to start in background
     * @param receiver to post results
     */
    public void startUserInBackground(@UserIdInt int userId,
            @NonNull AndroidFuture<UserStartResult> receiver) {
        checkManageOrCreateUsersPermission("startUserInBackground");
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_START_USER_IN_BACKGROUND_REQ, userId);

        mHandler.post(() -> handleStartUserInBackground(userId, receiver));
    }

    private void handleStartUserInBackground(@UserIdInt int userId,
            @NonNull AndroidFuture<UserStartResult> receiver) {
        // If the requested user is the current user, do nothing and return success.
        if (ActivityManager.getCurrentUser() == userId) {
            sendUserStartResult(
                    userId, UserStartResult.STATUS_SUCCESSFUL_USER_IS_CURRENT_USER, receiver);
            return;
        }
        // If requested user does not exist, return error.
        if (mUserManager.getUserInfo(userId) == null) {
            Slogf.w(TAG, "User %d does not exist", userId);
            sendUserStartResult(userId, UserStartResult.STATUS_USER_DOES_NOT_EXIST, receiver);
            return;
        }

        try {
            if (!mAm.startUserInBackground(userId)) {
                Slogf.w(TAG, "Failed to start user %d in background", userId);
                sendUserStartResult(userId, UserStartResult.STATUS_ANDROID_FAILURE, receiver);
                return;
            }
        } catch (RemoteException e) {
            Slogf.w(TAG, e, "Failed to start user %d in background", userId);
        }

        // TODO(b/181331178): We are not updating mBackgroundUsersToRestart or
        // mBackgroundUsersRestartedHere, which were only used for the garage mode. Consider
        // renaming them to make it more clear.
        sendUserStartResult(userId, UserStartResult.STATUS_SUCCESSFUL, receiver);
    }

    private void sendUserStartResult(@UserIdInt int userId, @UserStartResult.Status int result,
            @NonNull AndroidFuture<UserStartResult> receiver) {
        EventLog.writeEvent(
                EventLogTags.CAR_USER_SVC_START_USER_IN_BACKGROUND_RESP, userId, result);
        receiver.complete(new UserStartResult(result));
    }

    /**
     * Starts all background users that were active in system.
     *
     * @return list of background users started successfully.
     */
    @NonNull
    public ArrayList<Integer> startAllBackgroundUsersInGarageMode() {
        synchronized (mLockUser) {
            if (!mStartBackgroundUsersOnGarageMode) {
                Slogf.i(TAG, "Background users are not started as mStartBackgroundUsersOnGarageMode"
                        + " is false.");
                return new ArrayList<>();
            }
        }

        ArrayList<Integer> users;
        synchronized (mLockUser) {
            users = new ArrayList<>(mBackgroundUsersToRestart);
            mBackgroundUsersRestartedHere.clear();
            mBackgroundUsersRestartedHere.addAll(mBackgroundUsersToRestart);
        }
        ArrayList<Integer> startedUsers = new ArrayList<>();
        for (Integer user : users) {
            if (user == ActivityManager.getCurrentUser()) {
                continue;
            }
            try {
                if (mAm.startUserInBackground(user)) {
                    if (mUserManager.isUserUnlockingOrUnlocked(user)) {
                        // already unlocked / unlocking. No need to unlock.
                        startedUsers.add(user);
                    } else if (mAm.unlockUser(user, null, null, null)) {
                        startedUsers.add(user);
                    } else { // started but cannot unlock
                        Slog.w(TAG, "Background user started but cannot be unlocked:" + user);
                        if (mUserManager.isUserRunning(user)) {
                            // add to started list so that it can be stopped later.
                            startedUsers.add(user);
                        }
                    }
                }
            } catch (RemoteException e) {
                // ignore
                Slog.w(TAG, "error while starting user in background", e);
            }
        }
        // Keep only users that were re-started in mBackgroundUsersRestartedHere
        synchronized (mLockUser) {
            ArrayList<Integer> usersToRemove = new ArrayList<>();
            for (Integer user : mBackgroundUsersToRestart) {
                if (!startedUsers.contains(user)) {
                    usersToRemove.add(user);
                }
            }
            mBackgroundUsersRestartedHere.removeAll(usersToRemove);
        }
        return startedUsers;
    }

    /**
     * Stops the specified background user.
     *
     * @param userId user to stop
     * @param receiver to post results
     */
    public void stopUser(@UserIdInt int userId, @NonNull AndroidFuture<UserStopResult> receiver) {
        checkManageOrCreateUsersPermission("stopUser");
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_STOP_USER_REQ, userId);

        mHandler.post(() -> handleStopUser(userId, receiver));
    }

    private void handleStopUser(
            @UserIdInt int userId, @NonNull AndroidFuture<UserStopResult> receiver) {
        @UserStopResult.Status int userStopStatus = stopBackgroundUserInternal(userId);
        sendUserStopResult(userId, userStopStatus, receiver);
    }

    private void sendUserStopResult(@UserIdInt int userId, @UserStopResult.Status int result,
            @NonNull AndroidFuture<UserStopResult> receiver) {
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_STOP_USER_RESP, userId, result);
        receiver.complete(new UserStopResult(result));
    }

    private @UserStopResult.Status int stopBackgroundUserInternal(@UserIdInt int userId) {
        try {
            int r = mAm.stopUserWithDelayedLocking(userId, true, null);
            switch(r) {
                case ActivityManager.USER_OP_SUCCESS:
                    return UserStopResult.STATUS_SUCCESSFUL;
                case ActivityManager.USER_OP_ERROR_IS_SYSTEM:
                    Slogf.w(TAG, "Cannot stop the system user: %d", userId);
                    return UserStopResult.STATUS_FAILURE_SYSTEM_USER;
                case ActivityManager.USER_OP_IS_CURRENT:
                    Slogf.w(TAG, "Cannot stop the current user: %d", userId);
                    return UserStopResult.STATUS_FAILURE_CURRENT_USER;
                case ActivityManager.USER_OP_UNKNOWN_USER:
                    Slogf.w(TAG, "Cannot stop the user that does not exist: %d", userId);
                    return UserStopResult.STATUS_USER_DOES_NOT_EXIST;
                default:
                    Slogf.w(TAG, "stopUser failed, user: %d, err: %d", userId, r);
            }
        } catch (RemoteException e) {
            // ignore the exception
            Slogf.w(TAG, e, "error while stopping user: %d", userId);
        }
        return UserStopResult.STATUS_ANDROID_FAILURE;
    }

    /**
     * Sets boolean to control background user operations during garage mode.
     */
    public void setStartBackgroundUsersOnGarageMode(boolean enable) {
        synchronized (mLockUser) {
            mStartBackgroundUsersOnGarageMode = enable;
        }
    }

    /**
     * Stops a background user.
     *
     * @return whether stopping succeeds.
     */
    public boolean stopBackgroundUserInGagageMode(@UserIdInt int userId) {
        synchronized (mLockUser) {
            if (!mStartBackgroundUsersOnGarageMode) {
                Slogf.i(TAG, "Background users are not stopped as mStartBackgroundUsersOnGarageMode"
                        + " is false.");
                return false;
            }
        }

        @UserStopResult.Status int userStopStatus = stopBackgroundUserInternal(userId);
        if (UserStopResult.isSuccess(userStopStatus)) {
            // Remove the stopped user from the mBackgroundUserRestartedHere list.
            synchronized (mLockUser) {
                mBackgroundUsersRestartedHere.remove(Integer.valueOf(userId));
            }
            return true;
        }
        return false;
    }

    /**
     * Notifies all registered {@link UserLifecycleListener} with the event passed as argument.
     */
    public void onUserLifecycleEvent(@UserLifecycleEventType int eventType,
            @UserIdInt int fromUserId, @UserIdInt int toUserId) {
        int userId = toUserId;

        // Handle special cases first...
        if (eventType == CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING) {
            onUserSwitching(fromUserId, toUserId);
        } else if (eventType == CarUserManager.USER_LIFECYCLE_EVENT_TYPE_UNLOCKED) {
            onUserUnlocked(userId);
        }

        // ...then notify listeners.
        UserLifecycleEvent event = new UserLifecycleEvent(eventType, fromUserId, userId);

        mHandler.post(() -> {
            handleNotifyServiceUserLifecycleListeners(event);
            handleNotifyAppUserLifecycleListeners(event);
        });
    }

    private void sendPostSwitchToHalLocked(@UserIdInt int userId) {
        if (mUserIdForUserSwitchInProcess == UserHandle.USER_NULL
                || mUserIdForUserSwitchInProcess != userId
                || mRequestIdForUserSwitchInProcess == 0) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "No user switch request Id. No android post switch sent.");
            }
            return;
        }
        postSwitchHalResponse(mRequestIdForUserSwitchInProcess, mUserIdForUserSwitchInProcess);
        mUserIdForUserSwitchInProcess = UserHandle.USER_NULL;
        mRequestIdForUserSwitchInProcess = 0;
    }

    private void handleNotifyAppUserLifecycleListeners(UserLifecycleEvent event) {
        int listenersSize = mAppLifecycleListeners.size();
        if (listenersSize == 0) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "No app listener to be notified of " + event);
            }
            return;
        }
        // Must use a different TimingsTraceLog because it's another thread
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "Notifying " + listenersSize + " app listeners of " + event);
        }
        int userId = event.getUserId();
        TimingsTraceLog t = new TimingsTraceLog(TAG, Trace.TRACE_TAG_SYSTEM_SERVER);
        int eventType = event.getEventType();
        t.traceBegin("notify-app-listeners-user-" + userId + "-event-" + eventType);
        for (int i = 0; i < listenersSize; i++) {
            int uid = mAppLifecycleListeners.keyAt(i);

            IResultReceiver listener = mAppLifecycleListeners.valueAt(i);
            Bundle data = new Bundle();
            data.putInt(CarUserManager.BUNDLE_PARAM_ACTION, eventType);

            int fromUserId = event.getPreviousUserId();
            if (fromUserId != UserHandle.USER_NULL) {
                data.putInt(CarUserManager.BUNDLE_PARAM_PREVIOUS_USER_ID, fromUserId);
            }

            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "Notifying listener for uid " + uid);
            }
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_NOTIFY_APP_LIFECYCLE_LISTENER,
                    uid, eventType, fromUserId, userId);
            try {
                t.traceBegin("notify-app-listener-uid-" + uid);
                listener.send(userId, data);
            } catch (RemoteException e) {
                Slog.e(TAG, "Error calling lifecycle listener", e);
            } finally {
                t.traceEnd();
            }
        }
        t.traceEnd(); // notify-app-listeners-user-USERID-event-EVENT_TYPE
    }

    private void handleNotifyServiceUserLifecycleListeners(UserLifecycleEvent event) {
        TimingsTraceLog t = new TimingsTraceLog(TAG, Trace.TRACE_TAG_SYSTEM_SERVER);
        if (mUserLifecycleListeners.isEmpty()) {
            Slog.w(TAG, "Not notifying internal UserLifecycleListeners");
            return;
        } else if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "Notifying " + mUserLifecycleListeners.size()
                    + " service listeners of " + event);
        }

        int userId = event.getUserId();
        int eventType = event.getEventType();
        t.traceBegin("notify-listeners-user-" + userId + "-event-" + eventType);
        for (UserLifecycleListener listener : mUserLifecycleListeners) {
            String listenerName = FunctionalUtils.getLambdaName(listener);
            EventLog.writeEvent(EventLogTags.CAR_USER_SVC_NOTIFY_INTERNAL_LIFECYCLE_LISTENER,
                    listenerName, eventType, event.getPreviousUserId(), userId);
            try {
                t.traceBegin("notify-listener-" + listenerName);
                listener.onEvent(event);
            } catch (RuntimeException e) {
                Slog.e(TAG,
                        "Exception raised when invoking onEvent for " + listenerName, e);
            } finally {
                t.traceEnd();
            }
        }
        t.traceEnd(); // notify-listeners-user-USERID-event-EVENT_TYPE
    }

    private void onUserSwitching(@UserIdInt int fromUserId, @UserIdInt int toUserId) {
        Slog.i(TAG, "onUserSwitching() callback for user " + toUserId);
        TimingsTraceLog t = new TimingsTraceLog(TAG, Trace.TRACE_TAG_SYSTEM_SERVER);
        t.traceBegin("onUserSwitching-" + toUserId);

        // Switch HAL users if user switch is not requested by CarUserService
        notifyHalLegacySwitch(fromUserId, toUserId);

        mInitialUserSetter.setLastActiveUser(toUserId);

        if (mLastPassengerId != UserHandle.USER_NULL) {
            stopPassengerInternal(mLastPassengerId, false);
        }
        if (mEnablePassengerSupport && isPassengerDisplayAvailable()) {
            setupPassengerUser();
            startFirstPassenger(toUserId);
        }
        t.traceEnd();
    }

    private void notifyHalLegacySwitch(@UserIdInt int fromUserId, @UserIdInt int toUserId) {
        synchronized (mLockUser) {
            if (mUserIdForUserSwitchInProcess != UserHandle.USER_NULL) {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Slog.d(TAG, "notifyHalLegacySwitch(" + fromUserId + ", " + toUserId
                            + "): not needed, normal switch for " + mUserIdForUserSwitchInProcess);
                }
                return;
            }
        }

        if (!isUserHalSupported()) return;

        // switch HAL user
        UsersInfo usersInfo = UserHalHelper.newUsersInfo(mUserManager, fromUserId);
        SwitchUserRequest request = createUserSwitchRequest(toUserId, usersInfo);
        mHal.legacyUserSwitch(request);
    }

    /**
     * Runs the given runnable when user 0 is unlocked. If user 0 is already unlocked, it is
     * run inside this call.
     *
     * @param r Runnable to run.
     */
    public void runOnUser0Unlock(@NonNull Runnable r) {
        Objects.requireNonNull(r, "runnable cannot be null");
        boolean runNow = false;
        synchronized (mLockUser) {
            if (mUser0Unlocked) {
                runNow = true;
            } else {
                mUser0UnlockTasks.add(r);
            }
        }
        if (runNow) {
            r.run();
        }
    }

    @VisibleForTesting
    @NonNull
    ArrayList<Integer> getBackgroundUsersToRestart() {
        ArrayList<Integer> backgroundUsersToRestart = null;
        synchronized (mLockUser) {
            backgroundUsersToRestart = new ArrayList<>(mBackgroundUsersToRestart);
        }
        return backgroundUsersToRestart;
    }

    private void setSystemUserRestrictions() {
        // Disable Location service for system user.
        LocationManager locationManager =
                (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
        locationManager.setLocationEnabledForUser(
                /* enabled= */ false, UserHandle.of(UserHandle.USER_SYSTEM));
    }

    /**
     * Assigns a default icon to a user according to the user's id.
     *
     * @param userInfo User whose avatar is set to default icon.
     */
    private void assignDefaultIcon(UserInfo userInfo) {
        int idForIcon = userInfo.isGuest() ? UserHandle.USER_NULL : userInfo.id;
        Bitmap bitmap = UserIcons.convertToBitmap(
                UserIcons.getDefaultUserIcon(mContext.getResources(), idForIcon, false));
        mUserManager.setUserIcon(userInfo.id, bitmap);
    }

    private interface UserFilter {
        boolean isEligibleUser(UserInfo user);
    }

    /** Returns all users who are matched by the given filter. */
    private List<UserInfo> getUsers(UserFilter filter) {
        List<UserInfo> users = mUserManager.getAliveUsers();

        for (Iterator<UserInfo> iterator = users.iterator(); iterator.hasNext(); ) {
            UserInfo user = iterator.next();
            if (!filter.isEligibleUser(user)) {
                iterator.remove();
            }
        }
        return users;
    }

    private static void checkManageUsersOrDumpPermission(String message) {
        checkHasAtLeastOnePermissionGranted(message,
                android.Manifest.permission.MANAGE_USERS,
                android.Manifest.permission.DUMP);
    }

    private void checkInteractAcrossUsersPermission(String message) {
        checkHasAtLeastOnePermissionGranted(message,
                android.Manifest.permission.INTERACT_ACROSS_USERS,
                android.Manifest.permission.INTERACT_ACROSS_USERS_FULL);
    }

    private int getNumberOfManagedProfiles(@UserIdInt int userId) {
        List<UserInfo> users = mUserManager.getAliveUsers();
        // Count all users that are managed profiles of the given user.
        int managedProfilesCount = 0;
        for (UserInfo user : users) {
            if (user.isManagedProfile() && user.profileGroupId == userId) {
                managedProfilesCount++;
            }
        }
        return managedProfilesCount;
    }

    /**
     * Starts the first passenger of the given driver and assigns the passenger to the front
     * passenger zone.
     *
     * @param driverId User id of the driver.
     * @return whether it succeeds.
     */
    private boolean startFirstPassenger(@UserIdInt int driverId) {
        int zoneId = getAvailablePassengerZone();
        if (zoneId == OccupantZoneInfo.INVALID_ZONE_ID) {
            Slog.w(TAG, "passenger occupant zone is not found");
            return false;
        }
        List<UserInfo> passengers = getPassengers(driverId);
        if (passengers.size() < 1) {
            Slog.w(TAG, "passenger is not found");
            return false;
        }
        // Only one passenger is supported. If there are two or more passengers, the first passenger
        // is chosen.
        int passengerId = passengers.get(0).id;
        if (!startPassenger(passengerId, zoneId)) {
            Slog.w(TAG, "cannot start passenger " + passengerId);
            return false;
        }
        return true;
    }

    private int getAvailablePassengerZone() {
        int[] occupantTypes = new int[] {CarOccupantZoneManager.OCCUPANT_TYPE_FRONT_PASSENGER,
                CarOccupantZoneManager.OCCUPANT_TYPE_REAR_PASSENGER};
        for (int occupantType : occupantTypes) {
            int zoneId = getZoneId(occupantType);
            if (zoneId != OccupantZoneInfo.INVALID_ZONE_ID) {
                return zoneId;
            }
        }
        return OccupantZoneInfo.INVALID_ZONE_ID;
    }

    /**
     * Creates a new passenger user when there is no passenger user.
     */
    private void setupPassengerUser() {
        int currentUser = ActivityManager.getCurrentUser();
        int profileCount = getNumberOfManagedProfiles(currentUser);
        if (profileCount > 0) {
            Slog.w(TAG, "max profile of user" + currentUser
                    + " is exceeded: current profile count is " + profileCount);
            return;
        }
        // TODO(b/140311342): Use resource string for the default passenger name.
        UserInfo passenger = createPassenger("Passenger", currentUser);
        if (passenger == null) {
            // Couldn't create user, most likely because there are too many.
            Slog.w(TAG, "cannot create a passenger user");
            return;
        }
    }

    @NonNull
    private List<OccupantZoneInfo> getOccupantZones(@OccupantTypeEnum int occupantType) {
        ZoneUserBindingHelper helper = null;
        synchronized (mLockHelper) {
            if (mZoneUserBindingHelper == null) {
                Slog.w(TAG, "implementation is not delegated");
                return new ArrayList<OccupantZoneInfo>();
            }
            helper = mZoneUserBindingHelper;
        }
        return helper.getOccupantZones(occupantType);
    }

    private boolean assignUserToOccupantZone(@UserIdInt int userId, int zoneId) {
        ZoneUserBindingHelper helper = null;
        synchronized (mLockHelper) {
            if (mZoneUserBindingHelper == null) {
                Slog.w(TAG, "implementation is not delegated");
                return false;
            }
            helper = mZoneUserBindingHelper;
        }
        return helper.assignUserToOccupantZone(userId, zoneId);
    }

    private boolean unassignUserFromOccupantZone(@UserIdInt int userId) {
        ZoneUserBindingHelper helper = null;
        synchronized (mLockHelper) {
            if (mZoneUserBindingHelper == null) {
                Slog.w(TAG, "implementation is not delegated");
                return false;
            }
            helper = mZoneUserBindingHelper;
        }
        return helper.unassignUserFromOccupantZone(userId);
    }

    private boolean isPassengerDisplayAvailable() {
        ZoneUserBindingHelper helper = null;
        synchronized (mLockHelper) {
            if (mZoneUserBindingHelper == null) {
                Slog.w(TAG, "implementation is not delegated");
                return false;
            }
            helper = mZoneUserBindingHelper;
        }
        return helper.isPassengerDisplayAvailable();
    }

    /**
     * Gets the zone id of the given occupant type. If there are two or more zones, the first found
     * zone is returned.
     *
     * @param occupantType The type of an occupant.
     * @return The zone id of the given occupant type. {@link OccupantZoneInfo.INVALID_ZONE_ID},
     *         if not found.
     */
    private int getZoneId(@OccupantTypeEnum int occupantType) {
        List<OccupantZoneInfo> zoneInfos = getOccupantZones(occupantType);
        return (zoneInfos.size() > 0) ? zoneInfos.get(0).zoneId : OccupantZoneInfo.INVALID_ZONE_ID;
    }

    /**
     * Manages the required number of pre-created users.
     */
    @Override
    public void updatePreCreatedUsers() {
        checkManageOrCreateUsersPermission("preCreateUsers");
        preCreateUsersInternal();
    }

    private void preCreateUsersInternal() {
        mHandler.post(() -> mUserPreCreator.managePreCreatedUsers());
    }

    // TODO(b/167698977): members below were copied from UserManagerService; it would be better to
    // move them to some internal android.os class instead.

    private static final int ALLOWED_FLAGS_FOR_CREATE_USERS_PERMISSION =
            UserInfo.FLAG_MANAGED_PROFILE
            | UserInfo.FLAG_PROFILE
            | UserInfo.FLAG_EPHEMERAL
            | UserInfo.FLAG_RESTRICTED
            | UserInfo.FLAG_GUEST
            | UserInfo.FLAG_DEMO
            | UserInfo.FLAG_FULL;

    private static void checkManageUsersPermission(String message) {
        if (!hasManageUsersPermission()) {
            throw new SecurityException("You need " + MANAGE_USERS + " permission to: " + message);
        }
    }

    private static void checkManageOrCreateUsersPermission(String message) {
        if (!hasManageOrCreateUsersPermission()) {
            throw new SecurityException(
                    "You either need " + MANAGE_USERS + " or " + CREATE_USERS + " permission to: "
            + message);
        }
    }

    private static void checkManageOrCreateUsersPermission(int creationFlags) {
        if ((creationFlags & ~ALLOWED_FLAGS_FOR_CREATE_USERS_PERMISSION) == 0) {
            if (!hasManageOrCreateUsersPermission()) {
                throw new SecurityException("You either need " + MANAGE_USERS + " or "
                        + CREATE_USERS + "permission to create a user with flags "
                        + creationFlags);
            }
        } else if (!hasManageUsersPermission()) {
            throw new SecurityException("You need " + MANAGE_USERS + " permission to create a user"
                    + " with flags " + creationFlags);
        }
    }

    private static boolean hasManageUsersPermission() {
        final int callingUid = Binder.getCallingUid();
        return UserHandle.isSameApp(callingUid, Process.SYSTEM_UID)
                || callingUid == Process.ROOT_UID
                || hasPermissionGranted(MANAGE_USERS, callingUid);
    }

    private static boolean hasManageUsersOrPermission(String alternativePermission) {
        final int callingUid = Binder.getCallingUid();
        return UserHandle.isSameApp(callingUid, Process.SYSTEM_UID)
                || callingUid == Process.ROOT_UID
                || hasPermissionGranted(MANAGE_USERS, callingUid)
                || hasPermissionGranted(alternativePermission, callingUid);
    }

    private static boolean hasManageOrCreateUsersPermission() {
        return hasManageUsersOrPermission(CREATE_USERS);
    }

    private static boolean hasPermissionGranted(String permission, int uid) {
        return ActivityManager.checkComponentPermission(permission, uid, /* owningUid= */ -1,
                /* exported= */ true) == PackageManager.PERMISSION_GRANTED;
    }
}
