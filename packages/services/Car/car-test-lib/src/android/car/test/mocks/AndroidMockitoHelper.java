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
package android.car.test.mocks;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.when;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.Activity;
import android.app.ActivityManager;
import android.car.test.util.UserTestingHelper;
import android.car.test.util.UserTestingHelper.UserInfoBuilder;
import android.car.test.util.Visitor;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.pm.UserInfo.UserInfoFlag;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Looper;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.UserManager.RemoveResult;
import android.util.Log;

import java.util.List;
import java.util.Objects;
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Provides common Mockito calls for core Android classes.
 */
public final class AndroidMockitoHelper {

    private static final String TAG = AndroidMockitoHelper.class.getSimpleName();

    /**
     * Mocks a call to {@link ActivityManager#getCurrentUser()}.
     *
     * <p><b>Note: </b>it must be made inside a
     * {@link com.android.dx.mockito.inline.extended.StaticMockitoSession} built with
     * {@code spyStatic(ActivityManager.class)}.
     *
     * @param userId result of such call
     */
    public static void mockAmGetCurrentUser(@UserIdInt int userId) {
        doReturn(userId).when(() -> ActivityManager.getCurrentUser());
    }

    /**
     * Mocks a call to {@link UserManager#isHeadlessSystemUserMode()}.
     *
     * <p><b>Note: </b>it must be made inside a
     * {@linkcom.android.dx.mockito.inline.extended.StaticMockitoSession} built with
     * {@code spyStatic(UserManager.class)}.
     *
     * @param mode result of such call
     */
    public static void mockUmIsHeadlessSystemUserMode(boolean mode) {
        doReturn(mode).when(() -> UserManager.isHeadlessSystemUserMode());
    }

    /**
     * Mocks {@code UserManager#getUserInfo(userId)} to return a {@link UserInfo} with the given
     * {@code flags}.
     */
    @NonNull
    public static UserInfo mockUmGetUserInfo(@NonNull UserManager um, @UserIdInt int userId,
            @UserInfoFlag int flags) {
        Objects.requireNonNull(um);
        UserInfo user = new UserTestingHelper.UserInfoBuilder(userId).setFlags(flags).build();
        mockUmGetUserInfo(um, user);
        return user;
    }

    /**
     * Mocks {@code UserManager.getUserInfo(userId)} to return the given {@link UserInfo}.
     */
    @NonNull
    public static void mockUmGetUserInfo(@NonNull UserManager um, @NonNull UserInfo user) {
        when(um.getUserInfo(user.id)).thenReturn(user);
    }

    /**
     * Mocks {@code UserManager#getUserInfo(userId)} when the {@code userId} is the system user's.
     */
    @NonNull
    public static void mockUmGetSystemUser(@NonNull UserManager um) {
        UserInfo user = new UserTestingHelper.UserInfoBuilder(UserHandle.USER_SYSTEM)
                .setFlags(UserInfo.FLAG_SYSTEM).build();
        when(um.getUserInfo(UserHandle.USER_SYSTEM)).thenReturn(user);
    }

    /**
     * Mocks {@code UserManager#getAliveUsers()} to return the given users.
     */
    public static void mockUmGetAliveUsers(@NonNull UserManager um, @NonNull UserInfo... users) {
        Objects.requireNonNull(um);
        when(um.getAliveUsers()).thenReturn(UserTestingHelper.toList(users));
    }

    /**
     * Mocks {@code UserManager#getAliveUsers()} to return the simple users with the given ids.
     */
    public static void mockUmGetAliveUsers(@NonNull UserManager um,
            @NonNull @UserIdInt int... userIds) {
        List<UserInfo> users = UserTestingHelper.newUsers(userIds);
        when(um.getAliveUsers()).thenReturn(users);
    }

    /**
     * Mocks {@code UserManager#getUsers(excludePartial, excludeDying, excludeDying)} to return the
     * given users.
     */
    public static void mockUmGetUsers(@NonNull UserManager um, boolean excludePartial,
            boolean excludeDying, boolean excludePreCreated, @NonNull List<UserInfo> users) {
        Objects.requireNonNull(um);
        when(um.getUsers(excludePartial, excludeDying, excludePreCreated)).thenReturn(users);
    }

    /**
     * Mocks a call to {@code UserManager#getUsers()}, which includes dying users.
     */
    public static void mockUmGetAllUsers(@NonNull UserManager um, @NonNull UserInfo... userInfos) {
        when(um.getUsers()).thenReturn(UserTestingHelper.toList(userInfos));
    }

    /**
     * Mocks a call to {@code UserManager#isUserRunning(userId)}.
     */
    public static void mockUmIsUserRunning(@NonNull UserManager um, @UserIdInt int userId,
            boolean isRunning) {
        when(um.isUserRunning(userId)).thenReturn(isRunning);
    }

    /**
     * Mocks a successful call to {@code UserManager#createUser(String, String, int)}, returning
     * a user with the passed arguments.
     */
    @NonNull
    public static UserInfo mockUmCreateUser(@NonNull UserManager um, @Nullable String name,
            @NonNull String userType, @UserInfoFlag int flags, @UserIdInt int userId) {
        UserInfo userInfo = new UserInfoBuilder(userId)
                        .setName(name)
                        .setType(userType)
                        .setFlags(flags)
                        .build();
        when(um.createUser(name, userType, flags)).thenReturn(userInfo);
        return userInfo;
    }

    /**
     * Mocks a call to {@code UserManager#createUser(String, String, int)} that throws the given
     * runtime exception.
     */
    @NonNull
    public static void mockUmCreateUser(@NonNull UserManager um, @Nullable String name,
            @NonNull String userType, @UserInfoFlag int flags, @NonNull RuntimeException e) {
        when(um.createUser(name, userType, flags)).thenThrow(e);
    }

    /**
     * Mocks a successful call to {@code UserManager#removeUserOrSetEphemeral(int)}, and notifies
     * {@code listener} when it's called.
     */
    public static void mockUmRemoveUserOrSetEphemeral(@NonNull UserManager um,
            @NonNull UserInfo user, boolean evenWhenDisallowed, @RemoveResult int result,
            @Nullable Visitor<UserInfo> listener) {
        int userId = user.id;
        when(um.removeUserOrSetEphemeral(userId, evenWhenDisallowed)).thenAnswer((inv) -> {
            if (listener != null) {
                Log.v(TAG, "mockUmRemoveUserOrSetEphemeral(" + user + "): notifying " + listener);
                listener.visit(user);
            }
            return result;
        });
    }

    /**
     * Mocks a call to {@link ServiceManager#getService(name)}.
     *
     * <p><b>Note: </b>it must be made inside a
     * {@link com.android.dx.mockito.inline.extended.StaticMockitoSession} built with
     * {@code spyStatic(ServiceManager.class)}.
     *
     * @param name interface name of the service
     * @param binder result of such call
     */
    public static void mockSmGetService(@NonNull String name, @NonNull IBinder binder) {
        doReturn(binder).when(() -> ServiceManager.getService(name));
    }

    /**
     * Returns mocked binder implementation from the given interface name.
     *
     * <p><b>Note: </b>it must be made inside a
     * {@link com.android.dx.mockito.inline.extended.StaticMockitoSession} built with
     * {@code spyStatic(ServiceManager.class)}.
     *
     * @param name interface name of the service
     * @param binder mocked return of ServiceManager.getService
     * @param service binder implementation
     */
    public static <T extends IInterface> void mockQueryService(@NonNull String name,
            @NonNull IBinder binder, @NonNull T service) {
        doReturn(binder).when(() -> ServiceManager.getService(name));
        when(binder.queryLocalInterface(anyString())).thenReturn(service);
    }

    /**
     * Mocks a call to {@link Binder.getCallingUserHandle()}.
     *
     * <p><b>Note: </b>it must be made inside a
     * {@link com.android.dx.mockito.inline.extended.StaticMockitoSession} built with
     * {@code spyStatic(Binder.class)}.
     *
     * @param userId identifier of the {@link UserHandle} that will be returned.
     */
    public static void mockBinderGetCallingUserHandle(@UserIdInt int userId) {
        doReturn(UserHandle.of(userId)).when(() -> Binder.getCallingUserHandle());
    }

    /**
     * Mocks a call to {@link Context#getSystemService(Class)}.
     */
    public static <T> void mockContextGetService(@NonNull Context context,
            @NonNull Class<T> serviceClass, @NonNull T service) {
        when(context.getSystemService(serviceClass)).thenReturn(service);
        if (serviceClass.equals(PackageManager.class)) {
            when(context.getPackageManager()).thenReturn(PackageManager.class.cast(service));
        }
    }

    // TODO(b/192307581): add unit tests
    /**
     * Returns the result of the giving {@code callable} in the main thread, preparing the
     * {@link Looper} if needed and using a default timeout.
     */
    public static <T> T syncCallOnMainThread(Callable<T> c) throws Exception {
        return syncCallOnMainThread(JavaMockitoHelper.ASYNC_TIMEOUT_MS, c);
    }

    // TODO(b/192307581): add unit tests
    /**
     * Returns the result of the giving {@code callable} in the main thread, preparing the
     * {@link Looper} if needed.
     */
    public static <T> T syncCallOnMainThread(long timeoutMs, Callable<T> callable)
            throws Exception {
        boolean quitLooper = false;
        Looper looper = Looper.getMainLooper();
        if (looper == null) {
            Log.i(TAG, "preparing main looper");
            Looper.prepareMainLooper();
            looper = Looper.getMainLooper();
            assertWithMessage("Looper.getMainLooper()").that(looper).isNotNull();
            quitLooper = true;
        }
        Log.i(TAG, "looper: " + looper);
        AtomicReference<Exception> exception = new AtomicReference<>();
        AtomicReference<T> ref = new AtomicReference<>();
        try {
            Handler handler = new Handler(looper);
            CountDownLatch latch = new CountDownLatch(1);
            handler.post(() -> {
                T result = null;
                try {
                    result = callable.call();
                } catch (Exception e) {
                    exception.set(e);
                }
                ref.set(result);
                latch.countDown();
            });
            JavaMockitoHelper.await(latch, timeoutMs);
            Exception e = exception.get();
            if (e != null) throw e;
            return ref.get();
        } finally {
            if (quitLooper) {
                Log.i(TAG, "quitting looper: " + looper);
                looper.quitSafely();
            }
        }
    }

    // TODO(b/192307581): add unit tests
    /**
     * Runs the giving {@code runnable} in the activity's UI thread, using a default timeout.
     */
    public static void syncRunOnUiThread(Activity activity, Runnable runnable) throws Exception {
        syncRunOnUiThread(JavaMockitoHelper.ASYNC_TIMEOUT_MS, activity, runnable);
    }

    // TODO(b/192307581): add unit tests
    /**
     * Runs the giving {@code runnable} in the activity's UI thread.
     */
    public static void syncRunOnUiThread(long timeoutMs, Activity activity, Runnable runnable)
            throws Exception {
        CountDownLatch latch = new CountDownLatch(1);
        activity.runOnUiThread(() -> {
            runnable.run();
            latch.countDown();
        });
        JavaMockitoHelper.await(latch, timeoutMs);
    }

    private AndroidMockitoHelper() {
        throw new UnsupportedOperationException("contains only static methods");
    }
}
