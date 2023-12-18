/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.car.user;

import static android.Manifest.permission.INTERACT_ACROSS_USERS;
import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.os.Process.myUid;

import static com.android.internal.util.FunctionalUtils.getLambdaName;

import android.annotation.CallbackExecutor;
import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.RequiresPermission;
import android.annotation.SystemApi;
import android.annotation.TestApi;
import android.annotation.UserIdInt;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.ICarUserService;
import android.car.util.concurrent.AndroidAsyncFuture;
import android.car.util.concurrent.AsyncFuture;
import android.content.pm.UserInfo;
import android.content.pm.UserInfo.UserInfoFlag;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.sysprop.CarProperties;
import android.util.ArrayMap;
import android.util.EventLog;
import android.util.Log;

import com.android.car.internal.common.CommonConstants;
import com.android.car.internal.common.CommonConstants.UserLifecycleEventType;
import com.android.car.internal.common.EventLogTags;
import com.android.car.internal.common.UserHelperLite;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.infra.AndroidFuture;
import com.android.internal.os.IResultReceiver;
import com.android.internal.util.ArrayUtils;
import com.android.internal.util.Preconditions;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.stream.Collectors;

/**
 * API to manage users related to car.
 *
 * @hide
 */
@SystemApi
@TestApi
public final class CarUserManager extends CarManagerBase {

    private static final String TAG = CarUserManager.class.getSimpleName();
    private static final int HAL_TIMEOUT_MS = CarProperties.user_hal_timeout().orElse(5_000);
    private static final int REMOVE_USER_CALL_TIMEOUT_MS = 60_000;

    private static final boolean DBG = false;

    /**
     * {@link UserLifecycleEvent} called when the user is starting, for components to initialize
     * any per-user state they maintain for running users.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final int USER_LIFECYCLE_EVENT_TYPE_STARTING =
            CommonConstants.USER_LIFECYCLE_EVENT_TYPE_STARTING;

    /**
     * {@link UserLifecycleEvent} called when switching to a different foreground user, for
     * components that have special behavior for whichever user is currently in the foreground.
     *
     * <p>This is called before any application processes are aware of the new user.
     *
     * <p>Notice that internal system services might not have handled user switching yet, so be
     * careful with interaction with them.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final int USER_LIFECYCLE_EVENT_TYPE_SWITCHING =
            CommonConstants.USER_LIFECYCLE_EVENT_TYPE_SWITCHING;

    /**
     * {@link UserLifecycleEvent} called when an existing user is in the process of being unlocked.
     *
     * <p>This means the credential-encrypted storage for that user is now available, and
     * encryption-aware component filtering is no longer in effect.
     *
     * <p>Notice that internal system services might not have handled unlock yet, so most components
     * should ignore this callback and rely on {@link #USER_LIFECYCLE_EVENT_TYPE_UNLOCKED} instead.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final int USER_LIFECYCLE_EVENT_TYPE_UNLOCKING =
            CommonConstants.USER_LIFECYCLE_EVENT_TYPE_UNLOCKING;

    /**
     * {@link UserLifecycleEvent} called after an existing user is unlocked.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final int USER_LIFECYCLE_EVENT_TYPE_UNLOCKED =
            CommonConstants.USER_LIFECYCLE_EVENT_TYPE_UNLOCKED;

    /**
     * {@link UserLifecycleEvent} called when an existing user is stopping, for components to
     * finalize any per-user state they maintain for running users.
     *
     * <p>This is called prior to sending the {@code SHUTDOWN} broadcast to the user; it is a good
     * place to stop making use of any resources of that user (such as binding to a service running
     * in the user).
     *
     * <p><b>Note:</b> this is the last callback where the callee may access the target user's CE
     * storage.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final int USER_LIFECYCLE_EVENT_TYPE_STOPPING =
            CommonConstants.USER_LIFECYCLE_EVENT_TYPE_STOPPING;

    /**
     * {@link UserLifecycleEvent} called after an existing user is stopped.
     *
     * <p>This is called after all application process teardown of the user is complete.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final int USER_LIFECYCLE_EVENT_TYPE_STOPPED =
            CommonConstants.USER_LIFECYCLE_EVENT_TYPE_STOPPED;

    /** @hide */
    public static final String BUNDLE_PARAM_ACTION = "action";
    /** @hide */
    public static final String BUNDLE_PARAM_PREVIOUS_USER_ID = "previous_user";

    /**
     * {@link UserIdentificationAssociationType} for key fob.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_TYPE_KEY_FOB = 1;

    /**
     * {@link UserIdentificationAssociationType} for custom type 1.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_1 = 101;

    /**
     * {@link UserIdentificationAssociationType} for custom type 2.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_2 = 102;

    /**
     * {@link UserIdentificationAssociationType} for custom type 3.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_3 = 103;

    /**
     * {@link UserIdentificationAssociationType} for custom type 4.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_4 = 104;

    /**
     *  User HAL's user identification association types
     *
     * @hide
     */
    @IntDef(prefix = { "USER_IDENTIFICATION_ASSOCIATION_TYPE_" }, value = {
            USER_IDENTIFICATION_ASSOCIATION_TYPE_KEY_FOB,
            USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_1,
            USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_2,
            USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_3,
            USER_IDENTIFICATION_ASSOCIATION_TYPE_CUSTOM_4,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface UserIdentificationAssociationType{}

    /**
     * {@link UserIdentificationAssociationSetValue} to associate the identification type with the
     * current foreground Android user.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_ASSOCIATE_CURRENT_USER = 1;

    /**
     * {@link UserIdentificationAssociationSetValue} to disassociate the identification type from
     * the current foreground Android user.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_DISASSOCIATE_CURRENT_USER = 2;

    /**
     * {@link UserIdentificationAssociationSetValue} to disassociate the identification type from
     * all Android users.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_DISASSOCIATE_ALL_USERS = 3;

    /**
     * User HAL's user identification association types
     *
     * @hide
     */
    @IntDef(prefix = { "USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_" }, value = {
            USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_ASSOCIATE_CURRENT_USER,
            USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_DISASSOCIATE_CURRENT_USER,
            USER_IDENTIFICATION_ASSOCIATION_SET_VALUE_DISASSOCIATE_ALL_USERS,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface UserIdentificationAssociationSetValue{}

    /**
     * {@link UserIdentificationAssociationValue} when the status of an association could not be
     * determined.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_VALUE_UNKNOWN = 1;

    /**
     * {@link UserIdentificationAssociationValue} when the identification type is associated with
     * the current foreground Android user.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_VALUE_ASSOCIATE_CURRENT_USER = 2;

    /**
     * {@link UserIdentificationAssociationValue} when the identification type is associated with
     * another Android user.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_VALUE_ASSOCIATED_ANOTHER_USER = 3;

    /**
     * {@link UserIdentificationAssociationValue} when the identification type is not associated
     * with any Android user.
     *
     * @hide
     */
    public static final int USER_IDENTIFICATION_ASSOCIATION_VALUE_NOT_ASSOCIATED_ANY_USER = 4;

    /**
     * User HAL's user identification association types
     *
     * @hide
     */
    @IntDef(prefix = { "USER_IDENTIFICATION_ASSOCIATION_VALUE_" }, value = {
            USER_IDENTIFICATION_ASSOCIATION_VALUE_UNKNOWN,
            USER_IDENTIFICATION_ASSOCIATION_VALUE_ASSOCIATE_CURRENT_USER,
            USER_IDENTIFICATION_ASSOCIATION_VALUE_ASSOCIATED_ANOTHER_USER,
            USER_IDENTIFICATION_ASSOCIATION_VALUE_NOT_ASSOCIATED_ANY_USER,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface UserIdentificationAssociationValue{}

    private final Object mLock = new Object();
    private final ICarUserService mService;
    private final UserManager mUserManager;

    @Nullable
    @GuardedBy("mLock")
    private ArrayMap<UserLifecycleListener, Executor> mListeners;

    @Nullable
    @GuardedBy("mLock")
    private LifecycleResultReceiver mReceiver;

    /**
     * @hide
     */
    public CarUserManager(@NonNull Car car, @NonNull IBinder service) {
        this(car, ICarUserService.Stub.asInterface(service), UserManager.get(car.getContext()));
    }

    /**
     * @hide
     */
    @VisibleForTesting
    public CarUserManager(@NonNull Car car, @NonNull ICarUserService service,
            @NonNull UserManager userManager) {
        super(car);
        mService = service;
        mUserManager = userManager;
    }

    /**
     * Switches the foreground user to the given target user.
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public AsyncFuture<UserSwitchResult> switchUser(@UserIdInt int targetUserId) {
        int uid = myUid();
        try {
            AndroidFuture<UserSwitchResult> future = new AndroidFuture<UserSwitchResult>() {
                @Override
                protected void onCompleted(UserSwitchResult result, Throwable err) {
                    if (result != null) {
                        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SWITCH_USER_RESP, uid,
                                result.getStatus(), result.getErrorMessage());
                    } else {
                        Log.w(TAG, "switchUser(" + targetUserId + ") failed: " + err);
                    }
                    super.onCompleted(result, err);
                }
            };
            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SWITCH_USER_REQ, uid, targetUserId);
            mService.switchUser(targetUserId, HAL_TIMEOUT_MS, future);
            return new AndroidAsyncFuture<>(future);
        } catch (SecurityException e) {
            throw e;
        } catch (RemoteException | RuntimeException e) {
            AsyncFuture<UserSwitchResult> future =
                    newSwitchResuiltForFailure(UserSwitchResult.STATUS_HAL_INTERNAL_FAILURE);
            return handleExceptionFromCarService(e, future);
        }
    }

    private AndroidAsyncFuture<UserSwitchResult> newSwitchResuiltForFailure(
            @UserSwitchResult.Status int status) {
        AndroidFuture<UserSwitchResult> future = new AndroidFuture<>();
        future.complete(new UserSwitchResult(status, null));
        return new AndroidAsyncFuture<>(future);
    }

    /**
     * Creates a new Android user.
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
                        android.Manifest.permission.CREATE_USERS})
    public AsyncFuture<UserCreationResult> createUser(@Nullable String name,
            @NonNull String userType, @UserInfoFlag int flags) {
        int uid = myUid();
        try {
            AndroidFuture<UserCreationResult> future = new AndroidFuture<UserCreationResult>() {
                @Override
                protected void onCompleted(UserCreationResult result, Throwable err) {
                    if (result != null) {
                        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_CREATE_USER_RESP, uid,
                                result.getStatus(), result.getErrorMessage());
                        UserInfo user = result.getUser();
                        if (result.isSuccess() && user != null && user.isGuest()) {
                            onGuestCreated(user);
                        }
                    } else {
                        Log.w(TAG, "createUser(" + userType + "," + UserInfo.flagsToString(flags)
                                + ") failed: " + err);
                    }
                    super.onCompleted(result, err);
                };
            };
            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_CREATE_USER_REQ, uid,
                    UserHelperLite.safeName(name), userType, flags);
            mService.createUser(name, userType, flags, HAL_TIMEOUT_MS, future);
            return new AndroidAsyncFuture<>(future);
        } catch (SecurityException e) {
            throw e;
        } catch (RemoteException | RuntimeException e) {
            AndroidFuture<UserCreationResult> future = new AndroidFuture<>();
            future.complete(new UserCreationResult(UserCreationResult.STATUS_HAL_INTERNAL_FAILURE));
            return handleExceptionFromCarService(e, new AndroidAsyncFuture<>(future));
        }
    }

    /**
     * Creates a new guest Android user.
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public AsyncFuture<UserCreationResult> createGuest(@Nullable String name) {
        return createUser(name, UserManager.USER_TYPE_FULL_GUEST, /* flags= */ 0);
    }

    /**
     * Creates a new Android user.
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public AsyncFuture<UserCreationResult> createUser(@Nullable String name,
            @UserInfoFlag int flags) {
        return createUser(name, UserManager.USER_TYPE_FULL_SECONDARY, flags);
    }

    /**
     * Updates pre-created users.
     * <p>
     * Updates pre-created users based on the car properties defined
     * {@code CarProperties.number_pre_created_guests} and (@code
     * CarProperties.number_pre_created_users}.
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public void updatePreCreatedUsers() {
        int uid = myUid();
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_PRE_CREATE_USER_REQ, uid);
        try {
            mService.updatePreCreatedUsers();
        } catch (SecurityException e) {
            throw e;
        } catch (RemoteException | RuntimeException e) {
            handleExceptionFromCarService(e, null);
        }
    }

    // TODO(b/159283854): move to UserManager
    private void onGuestCreated(UserInfo user) {
        Settings.Secure.putStringForUser(getContext().getContentResolver(),
                Settings.Secure.SKIP_FIRST_USE_HINTS, "1", user.id);
    }

    /**
     * Removes the given user.
     *
     * @param userId identification of the user to be removed.
     *
     * @return whether the user was successfully removed.
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    @NonNull
    public UserRemovalResult removeUser(@UserIdInt int userId) {
        int uid = myUid();
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_REMOVE_USER_REQ, uid, userId);
        int status = UserRemovalResult.STATUS_ANDROID_FAILURE;
        try {
            AndroidFuture<UserRemovalResult> future = new AndroidFuture<UserRemovalResult>();
            mService.removeUser(userId, future);
            UserRemovalResult result = future.get(REMOVE_USER_CALL_TIMEOUT_MS,
                    TimeUnit.MILLISECONDS);
            status = result.getStatus();
            return result;
        } catch (SecurityException e) {
            throw e;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return new UserRemovalResult(UserRemovalResult.STATUS_ANDROID_FAILURE);
        } catch (ExecutionException | TimeoutException e) {
            return new UserRemovalResult(UserRemovalResult.STATUS_ANDROID_FAILURE);
        } catch (RemoteException | RuntimeException e) {
            return handleExceptionFromCarService(e,
                    new UserRemovalResult(UserRemovalResult.STATUS_ANDROID_FAILURE));
        } finally {
            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_REMOVE_USER_RESP, uid, status);
        }
    }

    /**
     * Adds a listener for {@link UserLifecycleEvent user lifecycle events}.
     *
     * @throws IllegalStateException if the listener was already added.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    @RequiresPermission(anyOf = {INTERACT_ACROSS_USERS, INTERACT_ACROSS_USERS_FULL})
    public void addListener(@NonNull @CallbackExecutor Executor executor,
            @NonNull UserLifecycleListener listener) {
        Objects.requireNonNull(executor, "executor cannot be null");
        Objects.requireNonNull(listener, "listener cannot be null");

        int uid = myUid();
        synchronized (mLock) {
            Preconditions.checkState(mListeners == null || !mListeners.containsKey(listener),
                    "already called for this listener");
            if (mReceiver == null) {
                mReceiver = new LifecycleResultReceiver();
                try {
                    EventLog.writeEvent(EventLogTags.CAR_USER_MGR_ADD_LISTENER, uid);
                    if (DBG) Log.d(TAG, "Setting lifecycle receiver for uid " + uid);
                    mService.setLifecycleListenerForUid(mReceiver);
                } catch (RemoteException e) {
                    handleRemoteExceptionFromCarService(e);
                }
            } else {
                if (DBG) Log.d(TAG, "Already set receiver for uid " + uid);
            }

            if (mListeners == null) {
                mListeners = new ArrayMap<>(1); // Most likely app will have just one listener
            } else if (DBG) {
                Log.d(TAG, "addListener(" + getLambdaName(listener) + "): context " + getContext()
                        + " already has " + mListeners.size() + " listeners: "
                        + mListeners.keySet().stream()
                                .map((l) -> getLambdaName(l))
                                .collect(Collectors.toList()), new Exception());
            }
            if (DBG) Log.d(TAG, "Adding listener: " + listener);
            mListeners.put(listener, executor);
        }
    }

    /**
     * Removes a listener for {@link UserLifecycleEvent user lifecycle events}.
     *
     * @throws IllegalStateException if the listener was not added beforehand.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    @RequiresPermission(anyOf = {INTERACT_ACROSS_USERS, INTERACT_ACROSS_USERS_FULL})
    public void removeListener(@NonNull UserLifecycleListener listener) {
        Objects.requireNonNull(listener, "listener cannot be null");

        int uid = myUid();
        synchronized (mLock) {
            Preconditions.checkState(mListeners != null && mListeners.containsKey(listener),
                    "not called for this listener yet");
            mListeners.remove(listener);

            if (!mListeners.isEmpty()) {
                if (DBG) Log.d(TAG, "removeListeners(): still " + mListeners.size() + " left");
                return;
            }
            mListeners = null;

            if (mReceiver == null) {
                Log.wtf(TAG, "removeListener(): receiver already null");
                return;
            }

            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_REMOVE_LISTENER, uid);
            if (DBG) Log.d(TAG, "Removing lifecycle receiver for uid=" + uid);
            try {
                mService.resetLifecycleListenerForUid();
                mReceiver = null;
            } catch (RemoteException e) {
                handleRemoteExceptionFromCarService(e);
            }
        }
    }

    /**
     * Check if user hal supports user association.
     *
     * @hide
     */
    public boolean isUserHalUserAssociationSupported() {
        try {
            return mService.isUserHalUserAssociationSupported();
        } catch (RemoteException | RuntimeException e) {
            return handleExceptionFromCarService(e, false);
        }
    }

    /**
     * Gets the user authentication types associated with this manager's user.
     *
     * @hide
     */
    @NonNull
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public UserIdentificationAssociationResponse getUserIdentificationAssociation(
            @UserIdentificationAssociationType int... types) {
        Preconditions.checkArgument(!ArrayUtils.isEmpty(types), "must have at least one type");
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_GET_USER_AUTH_REQ, types.length);
        try {
            UserIdentificationAssociationResponse response =
                    mService.getUserIdentificationAssociation(types);
            if (response != null) {
                int[] values = response.getValues();
                EventLog.writeEvent(EventLogTags.CAR_USER_MGR_GET_USER_AUTH_RESP,
                        values != null ? values.length : 0);
            }
            return response;
        } catch (SecurityException e) {
            throw e;
        } catch (RemoteException | RuntimeException e) {
            return handleExceptionFromCarService(e,
                    UserIdentificationAssociationResponse.forFailure(e.getMessage()));
        }
    }

    /**
     * Sets the user authentication types associated with this manager's user.
     *
     * @hide
     */
    @NonNull
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public AsyncFuture<UserIdentificationAssociationResponse> setUserIdentificationAssociation(
            @UserIdentificationAssociationType int[] types,
            @UserIdentificationAssociationSetValue int[] values) {
        Preconditions.checkArgument(!ArrayUtils.isEmpty(types), "must have at least one type");
        Preconditions.checkArgument(!ArrayUtils.isEmpty(values), "must have at least one value");
        if (types.length != values.length) {
            throw new IllegalArgumentException("types (" + Arrays.toString(types) + ") and values ("
                    + Arrays.toString(values) + ") should have the same length");
        }
        // TODO(b/153900032): move this logic to a common helper
        Object[] loggedValues = new Integer[types.length * 2];
        for (int i = 0; i < types.length; i++) {
            loggedValues[i * 2] = types[i];
            loggedValues[i * 2 + 1 ] = values[i];
        }
        EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SET_USER_AUTH_REQ, loggedValues);

        try {
            AndroidFuture<UserIdentificationAssociationResponse> future =
                    new AndroidFuture<UserIdentificationAssociationResponse>() {
                @Override
                protected void onCompleted(UserIdentificationAssociationResponse result,
                        Throwable err) {
                    if (result != null) {
                        int[] rawValues = result.getValues();
                        // TODO(b/153900032): move this logic to a common helper
                        if (rawValues != null) {
                            Object[] loggedValues = new Object[rawValues.length];
                            for (int i = 0; i < rawValues.length; i++) {
                                loggedValues[i] = rawValues[i];
                            }
                            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_SET_USER_AUTH_RESP,
                                    loggedValues);
                        }
                    } else {
                        Log.w(TAG, "setUserIdentificationAssociation(" + Arrays.toString(types)
                                + ", " + Arrays.toString(values) + ") failed: " + err);
                    }
                    super.onCompleted(result, err);
                };
            };
            mService.setUserIdentificationAssociation(HAL_TIMEOUT_MS, types, values, future);
            return new AndroidAsyncFuture<>(future);
        } catch (SecurityException e) {
            throw e;
        } catch (RemoteException | RuntimeException e) {
            AndroidFuture<UserIdentificationAssociationResponse> future = new AndroidFuture<>();
            future.complete(UserIdentificationAssociationResponse.forFailure());
            return handleExceptionFromCarService(e, new AndroidAsyncFuture<>(future));
        }
    }

    /**
     * Sets a callback to be notified before user switch. It should only be used by Car System UI.
     *
     * @hide
     */
    @RequiresPermission(android.Manifest.permission.MANAGE_USERS)
    public void setUserSwitchUiCallback(@NonNull UserSwitchUiCallback callback) {
        Preconditions.checkArgument(callback != null, "Null callback");
        UserSwitchUiCallbackReceiver userSwitchUiCallbackReceiver =
                new UserSwitchUiCallbackReceiver(callback);
        try {
            mService.setUserSwitchUiCallback(userSwitchUiCallbackReceiver);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * {@code IResultReceiver} used to receive user switch UI Callback.
     */
    // TODO(b/154958003): use mReceiver instead as now there are two binder objects
    private final class UserSwitchUiCallbackReceiver extends IResultReceiver.Stub {

        private final UserSwitchUiCallback mUserSwitchUiCallback;

        UserSwitchUiCallbackReceiver(UserSwitchUiCallback callback) {
            mUserSwitchUiCallback = callback;
        }

        @Override
        public void send(int userId, Bundle unused) throws RemoteException {
            mUserSwitchUiCallback.showUserSwitchDialog(userId);
        }
    }

    /**
     * {@code IResultReceiver} used to receive lifecycle events and dispatch to the proper listener.
     */
    private class LifecycleResultReceiver extends IResultReceiver.Stub {
        @Override
        public void send(int resultCode, Bundle resultData) {
            if (resultData == null) {
                Log.w(TAG, "Received result (" + resultCode + ") without data");
                return;
            }
            int from = resultData.getInt(BUNDLE_PARAM_PREVIOUS_USER_ID, UserHandle.USER_NULL);
            int to = resultCode;
            int eventType = resultData.getInt(BUNDLE_PARAM_ACTION);
            UserLifecycleEvent event = new UserLifecycleEvent(eventType, from, to);
            ArrayMap<UserLifecycleListener, Executor> listeners;
            synchronized (mLock) {
                listeners = mListeners;
            }
            if (listeners == null) {
                Log.w(TAG, "No listeners for event " + event);
                return;
            }
            int size = listeners.size();
            EventLog.writeEvent(EventLogTags.CAR_USER_MGR_NOTIFY_LIFECYCLE_LISTENER,
                    size, eventType, from, to);
            for (int i = 0; i < size; i++) {
                UserLifecycleListener listener = listeners.keyAt(i);
                Executor executor = listeners.valueAt(i);
                if (DBG) {
                    Log.d(TAG, "Calling " + getLambdaName(listener) + " for event " + event);
                }
                executor.execute(() -> listener.onEvent(event));
            }
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        // nothing to do
    }

    /**
     * @hide
     */
    @TestApi
    public static String lifecycleEventTypeToString(@UserLifecycleEventType int type) {
        switch (type) {
            case USER_LIFECYCLE_EVENT_TYPE_STARTING:
                return "STARTING";
            case USER_LIFECYCLE_EVENT_TYPE_SWITCHING:
                return "SWITCHING";
            case USER_LIFECYCLE_EVENT_TYPE_UNLOCKING:
                return "UNLOCKING";
            case USER_LIFECYCLE_EVENT_TYPE_UNLOCKED:
                return "UNLOCKED";
            case USER_LIFECYCLE_EVENT_TYPE_STOPPING:
                return "STOPPING";
            case USER_LIFECYCLE_EVENT_TYPE_STOPPED:
                return "STOPPED";
            default:
                return "UNKNOWN-" + type;
        }
    }

    // NOTE: this method is called by ExperimentalCarUserManager, so it can get the mService.
    // "Real" ExperimentalCarUserManager instances should be obtained through
    //    ExperimentalCarUserManager.from(mCarUserManager)
    // instead.
    ExperimentalCarUserManager newExperimentalCarUserManager() {
        return new ExperimentalCarUserManager(mCar, mService);
    }

    /**
     * Checks if the given {@code userId} represents a valid user.
     *
     * <p>A "valid" user:
     *
     * <ul>
     *   <li>Must exist in the device.
     *   <li>Is not in the process of being deleted.
     *   <li>Cannot be the {@link UserHandle#isSystem() system} user on devices that use
     *   {@link UserManager#isHeadlessSystemUserMode() headless system mode}.
     * </ul>
     *
     * @hide
     */
    @RequiresPermission(anyOf = {android.Manifest.permission.MANAGE_USERS,
            android.Manifest.permission.CREATE_USERS})
    public boolean isValidUser(@UserIdInt int userId) {
        List<UserInfo> allUsers = mUserManager.getAliveUsers();
        for (int i = 0; i < allUsers.size(); i++) {
            UserInfo user = allUsers.get(i);
            if (user.id == userId && (userId != UserHandle.USER_SYSTEM
                    || !UserManager.isHeadlessSystemUserMode())) {
                return true;
            }
        }
        return false;
    }

    /**
     * Defines a lifecycle event for an Android user.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public static final class UserLifecycleEvent {
        private final @UserLifecycleEventType int mEventType;
        private final @UserIdInt int mUserId;
        private final @UserIdInt int mPreviousUserId;

        /** @hide */
        public UserLifecycleEvent(@UserLifecycleEventType int eventType,
                @UserIdInt int from, @UserIdInt int to) {
            mEventType = eventType;
            mPreviousUserId = from;
            mUserId = to;
        }

        /** @hide */
        public UserLifecycleEvent(@UserLifecycleEventType int eventType, @UserIdInt int to) {
            this(eventType, UserHandle.USER_NULL, to);
        }

        /**
         * Gets the event type.
         *
         * @return either {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_STARTING},
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_SWITCHING},
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_UNLOCKING},
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_UNLOCKED},
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_STOPPING}, or
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_STOPPED}.
         */
        @UserLifecycleEventType
        public int getEventType() {
            return mEventType;
        }

        /**
         * Gets the id of the user whose event is being reported.
         *
         * @hide
         */
        @UserIdInt
        public int getUserId() {
            return mUserId;
        }

        /**
         * Gets the handle of the user whose event is being reported.
         */
        @NonNull
        public UserHandle getUserHandle() {
            return UserHandle.of(mUserId);
        }

        /**
         * Gets the id of the user being switched from.
         *
         * <p>This method returns {@link UserHandle#USER_NULL} for all event types but
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_SWITCHING}.
         *
         * @hide
         */
        @UserIdInt
        public int getPreviousUserId() {
            return mPreviousUserId;
        }

        /**
         * Gets the handle of the user being switched from.
         *
         * <p>This method returns {@code null} for all event types but
         * {@link CarUserManager#USER_LIFECYCLE_EVENT_TYPE_SWITCHING}.
         */
        @Nullable
        public UserHandle getPreviousUserHandle() {
            return mPreviousUserId == UserHandle.USER_NULL ? null : UserHandle.of(mPreviousUserId);
        }

        @Override
        public String toString() {
            StringBuilder builder = new StringBuilder("Event[type=")
                    .append(lifecycleEventTypeToString(mEventType));
            if (mPreviousUserId != UserHandle.USER_NULL) {
                builder
                    .append(",from=").append(mPreviousUserId)
                    .append(",to=").append(mUserId);
            } else {
                builder.append(",user=").append(mUserId);
            }

            return builder.append(']').toString();
        }
    }

    /**
     * Listener for Android User lifecycle events.
     *
     * <p>Must be registered using {@link CarUserManager#addListener(UserLifecycleListener)} and
     * unregistered through {@link CarUserManager#removeListener(UserLifecycleListener)}.
     *
     * @hide
     */
    @SystemApi
    @TestApi
    public interface UserLifecycleListener {

        /**
         * Called to notify the given {@code event}.
         */
        void onEvent(@NonNull UserLifecycleEvent event);
    }

    /**
     * Callback for notifying user switch before switch started.
     *
     * <p> It should only be user by Car System UI. The purpose of this callback is notify the
     * Car System UI to display the user switch UI.
     *
     * @hide
     */
    public interface UserSwitchUiCallback {

        /**
         * Called to notify that user switch dialog should be shown now.
         */
        void showUserSwitchDialog(@UserIdInt int userId);
    }
}
