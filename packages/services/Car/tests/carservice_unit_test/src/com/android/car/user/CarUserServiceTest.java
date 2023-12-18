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

import static android.car.test.mocks.AndroidMockitoHelper.mockUmCreateUser;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmGetUserInfo;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmGetUsers;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmRemoveUserOrSetEphemeral;
import static android.car.test.mocks.JavaMockitoHelper.getResult;
import static android.car.test.util.UserTestingHelper.UserInfoBuilder;
import static android.content.pm.UserInfo.FLAG_ADMIN;
import static android.content.pm.UserInfo.FLAG_EPHEMERAL;
import static android.content.pm.UserInfo.FLAG_GUEST;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.doAnswer;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doNothing;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.app.IActivityManager;
import android.car.CarOccupantZoneManager.OccupantTypeEnum;
import android.car.CarOccupantZoneManager.OccupantZoneInfo;
import android.car.drivingstate.CarUxRestrictions;
import android.car.drivingstate.ICarUxRestrictionsChangeListener;
import android.car.settings.CarSettings;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.car.test.mocks.BlockingAnswer;
import android.car.test.util.BlockingResultReceiver;
import android.car.testapi.BlockingUserLifecycleListener;
import android.car.user.CarUserManager;
import android.car.user.CarUserManager.UserLifecycleEvent;
import android.car.user.CarUserManager.UserLifecycleListener;
import android.car.user.UserCreationResult;
import android.car.user.UserIdentificationAssociationResponse;
import android.car.user.UserRemovalResult;
import android.car.user.UserStartResult;
import android.car.user.UserStopResult;
import android.car.user.UserSwitchResult;
import android.car.userlib.HalCallback;
import android.car.userlib.HalCallback.HalCallbackStatus;
import android.car.userlib.UserHalHelper;
import android.car.userlib.UserHelper;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.pm.UserInfo.UserInfoFlag;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.hardware.automotive.vehicle.V2_0.CreateUserRequest;
import android.hardware.automotive.vehicle.V2_0.CreateUserResponse;
import android.hardware.automotive.vehicle.V2_0.CreateUserStatus;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoRequestType;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoResponse;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoResponseAction;
import android.hardware.automotive.vehicle.V2_0.RemoveUserRequest;
import android.hardware.automotive.vehicle.V2_0.SwitchUserRequest;
import android.hardware.automotive.vehicle.V2_0.SwitchUserResponse;
import android.hardware.automotive.vehicle.V2_0.SwitchUserStatus;
import android.hardware.automotive.vehicle.V2_0.UserFlags;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociation;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationGetRequest;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationResponse;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationSetRequest;
import android.hardware.automotive.vehicle.V2_0.UsersInfo;
import android.location.LocationManager;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.UserManager.RemoveResult;
import android.sysprop.CarProperties;
import android.util.Log;
import android.util.SparseArray;
import android.view.Display;

import androidx.test.InstrumentationRegistry;

import com.android.car.CarServiceUtils;
import com.android.car.CarUxRestrictionsManagerService;
import com.android.car.hal.UserHalService;
import com.android.car.internal.ICarServiceHelper;
import com.android.car.internal.common.CommonConstants.UserLifecycleEventType;
import com.android.car.internal.common.UserHelperLite;
import com.android.internal.R;
import com.android.internal.infra.AndroidFuture;
import com.android.internal.os.IResultReceiver;
import com.android.internal.util.Preconditions;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.Mock;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;

/**
 * This class contains unit tests for the {@link CarUserService}.
 *
 * The following mocks are used:
 * <ol>
 * <li> {@link Context} provides system services and resources.
 * <li> {@link IActivityManager} provides current user.
 * <li> {@link UserManager} provides user creation and user info.
 * <li> {@link Resources} provides user icon.
 * <li> {@link Drawable} provides bitmap of user icon.
 * <ol/>
 */
public final class CarUserServiceTest extends AbstractExtendedMockitoTestCase {

    private static final String TAG = CarUserServiceTest.class.getSimpleName();
    private static final int NO_USER_INFO_FLAGS = 0;

    private static final int NON_EXISTING_USER = 55; // must not be on mExistingUsers

    private static final boolean HAS_CALLER_RESTRICTIONS = true;
    private static final boolean NO_CALLER_RESTRICTIONS = false;

    private static final int DEFAULT_TIMEOUT_MS = 15000;

    @Mock private Context mMockContext;
    @Mock private Context mApplicationContext;
    @Mock private LocationManager mLocationManager;
    @Mock private UserHalService mUserHal;
    @Mock private IActivityManager mMockedIActivityManager;
    @Mock private UserManager mMockedUserManager;
    @Mock private Resources mMockedResources;
    @Mock private Drawable mMockedDrawable;
    @Mock private InitialUserSetter mInitialUserSetter;
    @Mock private UserPreCreator mUserPreCreator;
    @Mock private IResultReceiver mSwitchUserUiReceiver;
    @Mock private PackageManager mPackageManager;
    @Mock private CarUxRestrictionsManagerService mCarUxRestrictionService;
    @Mock private ICarUxRestrictionsChangeListener mCarUxRestrictionsListener;
    @Mock private ICarServiceHelper mICarServiceHelper;

    private final BlockingUserLifecycleListener mUserLifecycleListener =
            BlockingUserLifecycleListener.forAnyEvent().build();

    @Captor private ArgumentCaptor<UsersInfo> mUsersInfoCaptor;

    private CarUserService mCarUserService;
    private boolean mUser0TaskExecuted;

    private final AndroidFuture<UserSwitchResult> mUserSwitchFuture = new AndroidFuture<>();
    private final AndroidFuture<UserSwitchResult> mUserSwitchFuture2 = new AndroidFuture<>();
    private final AndroidFuture<UserCreationResult> mUserCreationFuture = new AndroidFuture<>();
    private final AndroidFuture<UserRemovalResult> mUserRemovalFuture = new AndroidFuture<>();
    private final AndroidFuture<UserIdentificationAssociationResponse> mUserAssociationRespFuture =
            new AndroidFuture<>();
    private final int mAsyncCallTimeoutMs = 100;
    private final BlockingResultReceiver mReceiver =
            new BlockingResultReceiver(mAsyncCallTimeoutMs);
    private final InitialUserInfoResponse mGetUserInfoResponse = new InitialUserInfoResponse();
    private final SwitchUserResponse mSwitchUserResponse = new SwitchUserResponse();

    private final @NonNull UserInfo mAdminUser = new UserInfoBuilder(100)
            .setAdmin(true)
            .build();
    private final @NonNull UserInfo mAnotherAdminUser = new UserInfoBuilder(108)
            .setAdmin(true)
            .build();
    private final @NonNull UserInfo mGuestUser = new UserInfoBuilder(111)
            .setGuest(true)
            .setEphemeral(true)
            .build();
    private final @NonNull UserInfo mRegularUser = new UserInfoBuilder(222)
            .build();
    private final @NonNull UserInfo mAnotherRegularUser = new UserInfoBuilder(333)
            .build();
    private final List<UserInfo> mExistingUsers = Arrays
            .asList(mAdminUser, mAnotherAdminUser, mGuestUser, mRegularUser, mAnotherRegularUser);

    private final HandlerThread mHandlerThread = CarServiceUtils.getHandlerThread(
            getClass().getSimpleName());
    private final Handler mHandler = new Handler(mHandlerThread.getLooper());

    @Override
    protected void onSessionBuilder(CustomMockitoSessionBuilder builder) {
        builder
            .spyStatic(ActivityManager.class)
            // TODO(b/156299496): it cannot spy on UserManager, as it would slow down the tests
            // considerably (more than 5 minutes total, instead of just a couple seconds). So, it's
            // mocking UserHelper.isHeadlessSystemUser() (on mockIsHeadlessSystemUser()) instead...
            .spyStatic(UserHelper.class)
            .spyStatic(UserHelperLite.class)
            .spyStatic(CarProperties.class)
            .spyStatic(Binder.class);
    }

    /**
     * Initialize all of the objects with the @Mock annotation.
     */
    @Before
    public void setUpMocks() {
        doReturn(mApplicationContext).when(mMockContext).getApplicationContext();
        doReturn(mLocationManager).when(mMockContext).getSystemService(Context.LOCATION_SERVICE);
        doReturn(InstrumentationRegistry.getTargetContext().getContentResolver())
                .when(mMockContext).getContentResolver();
        doReturn(false).when(mMockedUserManager).isUserUnlockingOrUnlocked(anyInt());
        doReturn(mMockedResources).when(mMockContext).getResources();
        doReturn(mMockedDrawable).when(mMockedResources)
                .getDrawable(eq(R.drawable.ic_account_circle), eq(null));
        doReturn(mMockedDrawable).when(mMockedDrawable).mutate();
        doReturn(1).when(mMockedDrawable).getIntrinsicWidth();
        doReturn(1).when(mMockedDrawable).getIntrinsicHeight();
        mockUserHalSupported(true);
        mockUserHalUserAssociationSupported(true);
        doReturn(Optional.of(mAsyncCallTimeoutMs)).when(() -> CarProperties.user_hal_timeout());

        mCarUserService = newCarUserService(/* switchGuestUserBeforeGoingSleep= */ false);

        // TODO(b/172262561): refactor this call, which is not assigning the service to anything
        // (but without it some tests fail due to NPE).
        new FakeCarOccupantZoneService(mCarUserService);
    }

    private ICarUxRestrictionsChangeListener initService() {
        ArgumentCaptor<ICarUxRestrictionsChangeListener> listenerCaptor =
                ArgumentCaptor.forClass(ICarUxRestrictionsChangeListener.class);
        doNothing().when(mCarUxRestrictionService).registerUxRestrictionsChangeListener(
                listenerCaptor.capture(), eq(Display.DEFAULT_DISPLAY));

        mCarUserService.init();

        ICarUxRestrictionsChangeListener listener = listenerCaptor.getValue();
        assertWithMessage("init() didn't register ICarUxRestrictionsChangeListener")
                .that(listener).isNotNull();

        return listener;
    }

    @Test
    public void testInitAndRelease() {
        // init()
        ICarUxRestrictionsChangeListener listener = initService();
        assertThat(listener).isNotNull();

        // release()
        mCarUserService.release();
        verify(mCarUxRestrictionService).unregisterUxRestrictionsChangeListener(listener);
    }

    @Test
    public void testSetICarServiceHelper_withUxRestrictions() throws Exception {
        mockGetUxRestrictions(/* restricted= */ true);
        ICarUxRestrictionsChangeListener listener = initService();

        mCarUserService.setCarServiceHelper(mICarServiceHelper);
        verify(mICarServiceHelper).setSafetyMode(false);

        updateUxRestrictions(listener, /* restricted= */ false);
        verify(mICarServiceHelper).setSafetyMode(true);
    }

    @Test
    public void testSetICarServiceHelper_withoutUxRestrictions() throws Exception {
        mockGetUxRestrictions(/* restricted= */ false);
        ICarUxRestrictionsChangeListener listener = initService();

        mCarUserService.setCarServiceHelper(mICarServiceHelper);
        verify(mICarServiceHelper).setSafetyMode(true);

        updateUxRestrictions(listener, /* restricted= */ true);
        verify(mICarServiceHelper).setSafetyMode(false);
    }

    @Test
    public void testAddUserLifecycleListener_checkNullParameter() {
        assertThrows(NullPointerException.class,
                () -> mCarUserService.addUserLifecycleListener(null));
    }

    @Test
    public void testRemoveUser_binderMethod() {
        CarUserService spy = spy(mCarUserService);

        spy.removeUser(42, mUserRemovalFuture);

        verify(spy).removeUser(42, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);
    }

    @Test
    public void testRemoveUserLifecycleListener_checkNullParameter() {
        assertThrows(NullPointerException.class,
                () -> mCarUserService.removeUserLifecycleListener(null));
    }

    @Test
    public void testOnUserLifecycleEvent_legacyUserSwitch_halCalled() throws Exception {
        // Arrange
        mockExistingUsers(mExistingUsers);

        // Act
        sendUserSwitchingEvent(mAdminUser.id, mRegularUser.id);

        // Verify
        verify(mUserHal).legacyUserSwitch(any());
    }

    @Test
    public void testOnUserLifecycleEvent_legacyUserSwitch_halnotSupported() throws Exception {
        // Arrange
        mockExistingUsers(mExistingUsers);
        mockUserHalSupported(false);

        // Act
        sendUserSwitchingEvent(mAdminUser.id, mRegularUser.id);

        // Verify
        verify(mUserHal, never()).legacyUserSwitch(any());
    }

    @Test
    public void testOnUserLifecycleEvent_notifyListener() throws Exception {
        // Arrange
        mCarUserService.addUserLifecycleListener(mUserLifecycleListener);
        mockExistingUsers(mExistingUsers);

        // Act
        sendUserSwitchingEvent(mAdminUser.id, mRegularUser.id);

        // Verify
        verifyListenerOnEventInvoked(mRegularUser.id,
                CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING);
    }

    @Test
    public void testOnUserLifecycleEvent_ensureAllListenersAreNotified() throws Exception {
        // Arrange: add two listeners, one to fail on onEvent
        // Adding the failure listener first.
        UserLifecycleListener failureListener = mock(UserLifecycleListener.class);
        doThrow(new RuntimeException("Failed onEvent invocation")).when(
                failureListener).onEvent(any(UserLifecycleEvent.class));
        mCarUserService.addUserLifecycleListener(failureListener);
        mockExistingUsers(mExistingUsers);

        // Adding the non-failure listener later.
        mCarUserService.addUserLifecycleListener(mUserLifecycleListener);

        // Act
        sendUserSwitchingEvent(mAdminUser.id, mRegularUser.id);

        // Verify
        verifyListenerOnEventInvoked(mRegularUser.id,
                CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING);
    }

    private void verifyListenerOnEventInvoked(int expectedNewUserId, int expectedEventType)
            throws Exception {
        UserLifecycleEvent actualEvent = mUserLifecycleListener.waitForAnyEvent();
        assertThat(actualEvent.getEventType()).isEqualTo(expectedEventType);
        assertThat(actualEvent.getUserId()).isEqualTo(expectedNewUserId);
    }

    private void verifyLastActiveUserSet(UserInfo user) {
        verify(mInitialUserSetter).setLastActiveUser(user.id);
    }

    private void verifyLastActiveUserNotSet() {
        verify(mInitialUserSetter, never()).setLastActiveUser(any());
    }

    /**
     * Test that the {@link CarUserService} disables the location service for headless user 0 upon
     * first run.
     */
    @Test
    public void testDisableLocationForHeadlessSystemUserOnFirstRun() {
        sendUserUnlockedEvent(UserHandle.USER_SYSTEM);
        verify(mLocationManager).setLocationEnabledForUser(
                /* enabled= */ false, UserHandle.of(UserHandle.USER_SYSTEM));
    }

    /**
     * Test that the {@link CarUserService} updates last active user on user switch in non-headless
     * system user mode.
     */
    @Test
    public void testLastActiveUserUpdatedOnUserSwitch_nonHeadlessSystemUser() throws Exception {
        mockIsHeadlessSystemUser(mRegularUser.id, false);
        mockExistingUsers(mExistingUsers);

        sendUserSwitchingEvent(mAdminUser.id, mRegularUser.id);

        verifyLastActiveUserSet(mRegularUser);
    }

    /**
     * Test that the {@link CarUserService} sets default guest restrictions on first boot.
     */
    @Test
    public void testInitializeGuestRestrictions_IfNotAlreadySet() {
        sendUserUnlockedEvent(UserHandle.USER_SYSTEM);
        assertThat(getSettingsInt(CarSettings.Global.DEFAULT_USER_RESTRICTIONS_SET)).isEqualTo(1);
    }

    @Test
    public void testRunOnUser0UnlockImmediate() {
        mUser0TaskExecuted = false;
        sendUserUnlockedEvent(UserHandle.USER_SYSTEM);
        mCarUserService.runOnUser0Unlock(() -> {
            mUser0TaskExecuted = true;
        });
        assertTrue(mUser0TaskExecuted);
    }

    @Test
    public void testRunOnUser0UnlockLater() {
        mUser0TaskExecuted = false;
        mCarUserService.runOnUser0Unlock(() -> {
            mUser0TaskExecuted = true;
        });
        assertFalse(mUser0TaskExecuted);
        sendUserUnlockedEvent(UserHandle.USER_SYSTEM);
        assertTrue(mUser0TaskExecuted);
    }

    /**
     * Test is lengthy as it is testing LRU logic.
     */
    @Test
    public void testBackgroundUserList() throws RemoteException {
        int user1 = 101;
        int user2 = 102;
        int user3 = 103;
        int user4Guest = 104;
        int user5 = 105;

        UserInfo user1Info = new UserInfo(user1, "user1", NO_USER_INFO_FLAGS);
        UserInfo user2Info = new UserInfo(user2, "user2", NO_USER_INFO_FLAGS);
        UserInfo user3Info = new UserInfo(user3, "user3", NO_USER_INFO_FLAGS);
        UserInfo user4GuestInfo = new UserInfo(
                user4Guest, "user4Guest", FLAG_EPHEMERAL | FLAG_GUEST);
        UserInfo user5Info = new UserInfo(user5, "user5", NO_USER_INFO_FLAGS);

        doReturn(user1Info).when(mMockedUserManager).getUserInfo(user1);
        doReturn(user2Info).when(mMockedUserManager).getUserInfo(user2);
        doReturn(user3Info).when(mMockedUserManager).getUserInfo(user3);
        doReturn(user4GuestInfo).when(mMockedUserManager).getUserInfo(user4Guest);
        doReturn(user5Info).when(mMockedUserManager).getUserInfo(user5);

        mockGetCurrentUser(user1);
        sendUserUnlockedEvent(UserHandle.USER_SYSTEM);
        // user 0 should never go to that list.
        assertTrue(mCarUserService.getBackgroundUsersToRestart().isEmpty());

        sendUserUnlockedEvent(user1);
        assertEquals(new Integer[]{user1},
                mCarUserService.getBackgroundUsersToRestart().toArray());

        // user 2 background, ignore in restart list
        sendUserUnlockedEvent(user2);
        assertEquals(new Integer[]{user1},
                mCarUserService.getBackgroundUsersToRestart().toArray());

        mockGetCurrentUser(user3);
        sendUserUnlockedEvent(user3);
        assertEquals(new Integer[]{user3, user1},
                mCarUserService.getBackgroundUsersToRestart().toArray());

        mockGetCurrentUser(user4Guest);
        sendUserUnlockedEvent(user4Guest);
        assertEquals(new Integer[]{user3, user1},
                mCarUserService.getBackgroundUsersToRestart().toArray());

        mockGetCurrentUser(user5);
        sendUserUnlockedEvent(user5);
        assertEquals(new Integer[]{user5, user3},
                mCarUserService.getBackgroundUsersToRestart().toArray());
    }

    /**
     * Test is lengthy as it is testing LRU logic.
     */
    @Test
    public void testBackgroundUsersStartStopKeepBackgroundUserList() throws Exception {
        int user1 = 101;
        int user2 = 102;
        int user3 = 103;

        UserInfo user1Info = new UserInfo(user1, "user1", NO_USER_INFO_FLAGS);
        UserInfo user2Info = new UserInfo(user2, "user2", NO_USER_INFO_FLAGS);
        UserInfo user3Info = new UserInfo(user3, "user3", NO_USER_INFO_FLAGS);

        when(mMockedUserManager.getUserInfo(user1)).thenReturn(user1Info);
        when(mMockedUserManager.getUserInfo(user2)).thenReturn(user2Info);
        when(mMockedUserManager.getUserInfo(user3)).thenReturn(user3Info);

        mockGetCurrentUser(user1);
        sendUserUnlockedEvent(UserHandle.USER_SYSTEM);
        sendUserUnlockedEvent(user1);
        mockGetCurrentUser(user2);
        sendUserUnlockedEvent(user2);
        sendUserUnlockedEvent(user1);
        mockGetCurrentUser(user3);
        sendUserUnlockedEvent(user3);
        mockStopUserWithDelayedLocking(user3, ActivityManager.USER_OP_IS_CURRENT);

        assertThat(mCarUserService.getBackgroundUsersToRestart()).containsExactly(user2, user3);

        when(mMockedIActivityManager.startUserInBackground(user2)).thenReturn(true);
        when(mMockedIActivityManager.unlockUser(user2, null, null, null)).thenReturn(true);
        assertThat(mCarUserService.startAllBackgroundUsersInGarageMode()).containsExactly(user2);
        sendUserUnlockedEvent(user2);
        assertThat(mCarUserService.getBackgroundUsersToRestart()).containsExactly(user2, user3);

        when(mMockedIActivityManager.stopUser(user2, true, null))
                .thenReturn(ActivityManager.USER_OP_SUCCESS);
        // should not stop the current fg user
        assertThat(mCarUserService.stopBackgroundUserInGagageMode(user3)).isFalse();
        assertThat(mCarUserService.stopBackgroundUserInGagageMode(user2)).isTrue();
        assertThat(mCarUserService.getBackgroundUsersToRestart()).containsExactly(user2, user3);
        assertThat(mCarUserService.getBackgroundUsersToRestart()).containsExactly(user2, user3);
    }

    @Test
    public void testStopUser_success() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user101", NO_USER_INFO_FLAGS);
        mockStopUserWithDelayedLocking(userId, ActivityManager.USER_OP_SUCCESS);

        AndroidFuture<UserStopResult> userStopResult = new AndroidFuture<>();
        stopUser(userId, userStopResult);

        assertThat(getResult(userStopResult).getStatus())
                .isEqualTo(UserStopResult.STATUS_SUCCESSFUL);
        assertThat(getResult(userStopResult).isSuccess()).isTrue();
    }

    @Test
    public void testStopUser_permissionDenied() throws Exception {
        int userId = 101;
        mockManageUsersPermission(android.Manifest.permission.MANAGE_USERS, false);
        mockManageUsersPermission(android.Manifest.permission.CREATE_USERS, false);

        AndroidFuture<UserStopResult> userStopResult = new AndroidFuture<>();
        assertThrows(SecurityException.class, () -> stopUser(userId, userStopResult));
    }

    @Test
    public void testStopUser_fail() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user101", NO_USER_INFO_FLAGS);
        mockStopUserWithDelayedLockingThrowsRemoteException(userId);

        AndroidFuture<UserStopResult> userStopResult = new AndroidFuture<>();
        stopUser(userId, userStopResult);

        assertThat(getResult(userStopResult).getStatus())
                .isEqualTo(UserStopResult.STATUS_ANDROID_FAILURE);
        assertThat(getResult(userStopResult).isSuccess()).isFalse();
    }

    @Test
    public void testStopUser_userDoesNotExist() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user101", NO_USER_INFO_FLAGS);
        mockStopUserWithDelayedLocking(userId, ActivityManager.USER_OP_UNKNOWN_USER);

        AndroidFuture<UserStopResult> userStopResult = new AndroidFuture<>();
        stopUser(userId, userStopResult);

        assertThat(getResult(userStopResult).getStatus())
                .isEqualTo(UserStopResult.STATUS_USER_DOES_NOT_EXIST);
        assertThat(getResult(userStopResult).isSuccess()).isFalse();
    }

    @Test
    public void testStopUser_systemUser() throws Exception {
        mockStopUserWithDelayedLocking(
                UserHandle.USER_SYSTEM, ActivityManager.USER_OP_ERROR_IS_SYSTEM);

        AndroidFuture<UserStopResult> userStopResult = new AndroidFuture<>();
        stopUser(UserHandle.USER_SYSTEM, userStopResult);

        assertThat(getResult(userStopResult).getStatus())
                .isEqualTo(UserStopResult.STATUS_FAILURE_SYSTEM_USER);
        assertThat(getResult(userStopResult).isSuccess()).isFalse();
    }

    @Test
    public void testStopUser_currentUser() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user101", NO_USER_INFO_FLAGS);
        mockStopUserWithDelayedLocking(userId, ActivityManager.USER_OP_IS_CURRENT);

        AndroidFuture<UserStopResult> userStopResult = new AndroidFuture<>();
        stopUser(userId, userStopResult);

        assertThat(getResult(userStopResult).getStatus())
                .isEqualTo(UserStopResult.STATUS_FAILURE_CURRENT_USER);
        assertThat(getResult(userStopResult).isSuccess()).isFalse();
    }

    @Test
    public void testStopBackgroundUserForSystemUser() throws Exception {
        mockStopUserWithDelayedLocking(
                UserHandle.USER_SYSTEM, ActivityManager.USER_OP_ERROR_IS_SYSTEM);

        assertThat(mCarUserService.stopBackgroundUserInGagageMode(UserHandle.USER_SYSTEM))
                .isFalse();
    }

    @Test
    public void testStopBackgroundUserForFgUser() throws Exception {
        int userId = 101;
        mockStopUserWithDelayedLocking(userId, ActivityManager.USER_OP_IS_CURRENT);

        assertThat(mCarUserService.stopBackgroundUserInGagageMode(userId)).isFalse();
    }

    @Test
    public void testCreateAdminDriver_IfCurrentUserIsAdminUser() throws Exception {
        when(mMockedUserManager.isSystemUser()).thenReturn(true);
        mockUmCreateUser(mMockedUserManager, "testUser", UserManager.USER_TYPE_FULL_SECONDARY,
                UserInfo.FLAG_ADMIN, 10);
        mockHalCreateUser(HalCallback.STATUS_OK, CreateUserStatus.SUCCESS);

        AndroidFuture<UserCreationResult> future = mCarUserService.createDriver("testUser", true);
        waitForHandlerThreadToFinish();

        assertThat(getResult(future).getUser().name).isEqualTo("testUser");
        assertThat(getResult(future).getUser().id).isEqualTo(10);
    }

    @Test
    public void testCreateAdminDriver_IfCurrentUserIsNotSystemUser() throws Exception {
        when(mMockedUserManager.isSystemUser()).thenReturn(false);
        AndroidFuture<UserCreationResult> future = mCarUserService.createDriver("testUser", true);
        waitForHandlerThreadToFinish();
        assertThat(getResult(future).getStatus())
                .isEqualTo(UserCreationResult.STATUS_INVALID_REQUEST);
    }

    @Test
    public void testCreateNonAdminDriver() throws Exception {
        mockUmCreateUser(mMockedUserManager, "testUser", UserManager.USER_TYPE_FULL_SECONDARY,
                NO_USER_INFO_FLAGS, 10);
        mockHalCreateUser(HalCallback.STATUS_OK, CreateUserStatus.SUCCESS);

        AndroidFuture<UserCreationResult> future = mCarUserService.createDriver("testUser", false);
        waitForHandlerThreadToFinish();

        UserInfo userInfo = getResult(future).getUser();
        assertThat(userInfo.name).isEqualTo("testUser");
        assertThat(userInfo.id).isEqualTo(10);
    }

    @Test
    public void testCreatePassenger() {
        doNothing()
                .when(() -> UserHelper.setDefaultNonAdminRestrictions(any(), any(), anyBoolean()));
        int driverId = 90;
        int passengerId = 99;
        String userName = "testUser";
        UserInfo userInfo = new UserInfo(passengerId, userName, NO_USER_INFO_FLAGS);
        doReturn(userInfo).when(mMockedUserManager).createProfileForUser(eq(userName),
                eq(UserManager.USER_TYPE_PROFILE_MANAGED), eq(0), eq(driverId));
        UserInfo driverInfo = new UserInfo(driverId, "driver", NO_USER_INFO_FLAGS);
        doReturn(driverInfo).when(mMockedUserManager).getUserInfo(driverId);
        assertEquals(userInfo, mCarUserService.createPassenger(userName, driverId));
    }

    @Test
    public void testCreatePassenger_IfMaximumProfileAlreadyCreated() {
        int driverId = 90;
        String userName = "testUser";
        doReturn(null).when(mMockedUserManager).createProfileForUser(eq(userName),
                eq(UserManager.USER_TYPE_PROFILE_MANAGED), anyInt(), anyInt());
        UserInfo driverInfo = new UserInfo(driverId, "driver", NO_USER_INFO_FLAGS);
        doReturn(driverInfo).when(mMockedUserManager).getUserInfo(driverId);
        assertEquals(null, mCarUserService.createPassenger(userName, driverId));
    }

    @Test
    public void testCreatePassenger_IfDriverIsGuest() {
        int driverId = 90;
        String userName = "testUser";
        UserInfo driverInfo = new UserInfo(driverId, "driver", UserInfo.FLAG_GUEST);
        doReturn(driverInfo).when(mMockedUserManager).getUserInfo(driverId);
        assertEquals(null, mCarUserService.createPassenger(userName, driverId));
    }

    @Test
    public void testSwitchDriver() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mRegularUser, mSwitchUserResponse);
        mockAmSwitchUser(mRegularUser, true);
        when(mMockedUserManager.hasUserRestriction(UserManager.DISALLOW_USER_SWITCH))
                .thenReturn(false);
        mCarUserService.switchDriver(mRegularUser.id, mUserSwitchFuture);
        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
    }

    @Test
    public void testSwitchDriver_failUxRestrictions() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockGetUxRestrictions(/* restricted= */ true);
        initService();

        mCarUserService.switchDriver(mRegularUser.id, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_UX_RESTRICTION_FAILURE);
        verifyNoUserSwitch();
        assertNoHalUserSwitch();
    }

    @Test
    public void testSwitchDriver_IfUserSwitchIsNotAllowed() throws Exception {
        when(mMockedUserManager.getUserSwitchability())
                .thenReturn(UserManager.SWITCHABILITY_STATUS_USER_SWITCH_DISALLOWED);
        mCarUserService.switchDriver(mRegularUser.id, mUserSwitchFuture);
        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_INVALID_REQUEST);
    }

    @Test
    public void testSwitchDriver_IfSwitchedToCurrentUser() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        when(mMockedUserManager.hasUserRestriction(UserManager.DISALLOW_USER_SWITCH))
                .thenReturn(false);
        mCarUserService.switchDriver(mAdminUser.id, mUserSwitchFuture);
        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_OK_USER_ALREADY_IN_FOREGROUND);
    }

    @Test
    public void testStartPassenger() throws RemoteException {
        int passenger1Id = 91;
        int passenger2Id = 92;
        int passenger3Id = 93;
        int zone1Id = 1;
        int zone2Id = 2;
        doReturn(true).when(mMockedIActivityManager)
                .startUserInBackgroundWithListener(anyInt(), eq(null));
        assertTrue(mCarUserService.startPassenger(passenger1Id, zone1Id));
        assertTrue(mCarUserService.startPassenger(passenger2Id, zone2Id));
        assertFalse(mCarUserService.startPassenger(passenger3Id, zone2Id));
    }

    @Test
    public void testStopPassenger() throws RemoteException {
        int user1Id = 11;
        int passenger1Id = 91;
        int passenger2Id = 92;
        int zoneId = 1;
        UserInfo user1Info = new UserInfo(user1Id, "user1", NO_USER_INFO_FLAGS);
        UserInfo passenger1Info =
                new UserInfo(passenger1Id, "passenger1", UserInfo.FLAG_MANAGED_PROFILE);
        associateParentChild(user1Info, passenger1Info);
        doReturn(passenger1Info).when(mMockedUserManager).getUserInfo(passenger1Id);
        doReturn(null).when(mMockedUserManager).getUserInfo(passenger2Id);
        mockGetCurrentUser(user1Id);
        doReturn(true).when(mMockedIActivityManager)
                .startUserInBackgroundWithListener(anyInt(), eq(null));
        assertTrue(mCarUserService.startPassenger(passenger1Id, zoneId));
        assertTrue(mCarUserService.stopPassenger(passenger1Id));
        // Test of stopping an already stopped passenger.
        assertTrue(mCarUserService.stopPassenger(passenger1Id));
        // Test of stopping a non-existing passenger.
        assertFalse(mCarUserService.stopPassenger(passenger2Id));
    }

    private static void associateParentChild(UserInfo parent, UserInfo child) {
        parent.profileGroupId = parent.id;
        child.profileGroupId = parent.id;
    }

    private static List<UserInfo> prepareUserList() {
        List<UserInfo> users = new ArrayList<>(Arrays.asList(
                new UserInfo(10, "test10", UserInfo.FLAG_PRIMARY | UserInfo.FLAG_ADMIN),
                new UserInfo(11, "test11", NO_USER_INFO_FLAGS),
                new UserInfo(12, "test12", UserInfo.FLAG_MANAGED_PROFILE),
                new UserInfo(13, "test13", NO_USER_INFO_FLAGS),
                new UserInfo(14, "test14", UserInfo.FLAG_GUEST),
                new UserInfo(15, "test15", UserInfo.FLAG_EPHEMERAL),
                new UserInfo(16, "test16", UserInfo.FLAG_DISABLED),
                new UserInfo(17, "test17", UserInfo.FLAG_MANAGED_PROFILE),
                new UserInfo(18, "test18", UserInfo.FLAG_MANAGED_PROFILE),
                new UserInfo(19, "test19", NO_USER_INFO_FLAGS)
        ));
        // Parent: test10, child: test12
        associateParentChild(users.get(0), users.get(2));
        // Parent: test13, child: test17
        associateParentChild(users.get(3), users.get(7));
        // Parent: test13, child: test18
        associateParentChild(users.get(3), users.get(8));
        return users;
    }

    @Test
    public void testGetAllPossibleDrivers() {
        Set<Integer> expected = new HashSet<Integer>(Arrays.asList(10, 11, 13, 14));
        when(mMockedUserManager.getAliveUsers()).thenReturn(prepareUserList());
        mockIsHeadlessSystemUser(19, true);
        for (UserInfo user : mCarUserService.getAllDrivers()) {
            assertThat(expected).contains(user.id);
            expected.remove(user.id);
        }
        assertThat(expected).isEmpty();
    }

    @Test
    public void testGetAllPassengers() {
        SparseArray<HashSet<Integer>> testCases = new SparseArray<HashSet<Integer>>() {
            {
                put(0, new HashSet<Integer>());
                put(10, new HashSet<Integer>(Arrays.asList(12)));
                put(11, new HashSet<Integer>());
                put(13, new HashSet<Integer>(Arrays.asList(17)));
            }
        };
        mockIsHeadlessSystemUser(18, true);
        for (int i = 0; i < testCases.size(); i++) {
            when(mMockedUserManager.getAliveUsers()).thenReturn(prepareUserList());
            List<UserInfo> passengers = mCarUserService.getPassengers(testCases.keyAt(i));
            HashSet<Integer> expected = testCases.valueAt(i);
            for (UserInfo user : passengers) {
                assertThat(expected).contains(user.id);
                expected.remove(user.id);
            }
            assertThat(expected).isEmpty();
        }
    }

    @Test
    public void testRemoveUser_currentUser_successSetEphemeral() throws Exception {
        UserInfo currentUser = mRegularUser;
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        UserInfo removeUser = mRegularUser;
        mockRemoveUserNoCallback(removeUser, UserManager.REMOVE_RESULT_SET_EPHEMERAL);

        removeUser(removeUser.id, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_SUCCESSFUL_SET_EPHEMERAL);
        assertNoHalUserRemoval();
    }

    @Test
    public void testRemoveUser_alreadyBeingRemoved_success() throws Exception {
        UserInfo currentUser = mRegularUser;
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        UserInfo removeUser = mRegularUser;
        mockRemoveUser(removeUser, UserManager.REMOVE_RESULT_ALREADY_BEING_REMOVED);

        removeUser(removeUser.id, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(), UserRemovalResult.STATUS_SUCCESSFUL);
        assertHalRemove(currentUser, removeUser);
    }

    @Test
    public void testRemoveUser_currentLastAdmin_successSetEphemeral() throws Exception {
        UserInfo currentUser = mAdminUser;
        List<UserInfo> existingUsers = Arrays.asList(mAdminUser, mRegularUser);
        mockExistingUsersAndCurrentUser(existingUsers, currentUser);
        UserInfo removeUser = mAdminUser;
        mockRemoveUserNoCallback(removeUser, UserManager.REMOVE_RESULT_SET_EPHEMERAL);

        removeUser(mAdminUser.id, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_SUCCESSFUL_LAST_ADMIN_SET_EPHEMERAL);
        assertNoHalUserRemoval();
    }

    @Test
    public void testRemoveUser_userNotExist() throws Exception {
        removeUser(15, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_USER_DOES_NOT_EXIST);
    }

    @Test
    public void testRemoveUser_lastAdminUser_success() throws Exception {
        UserInfo currentUser = mRegularUser;
        UserInfo removeUser = mAdminUser;
        List<UserInfo> existingUsers = Arrays.asList(mAdminUser, mRegularUser);

        mockExistingUsersAndCurrentUser(existingUsers, currentUser);
        mockRemoveUser(removeUser);

        removeUser(mAdminUser.id, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_SUCCESSFUL_LAST_ADMIN_REMOVED);
        assertHalRemove(currentUser, removeUser);
    }

    @Test
    public void testRemoveUser_notLastAdminUser_success() throws Exception {
        UserInfo currentUser = mRegularUser;
        // Give admin rights to current user.
        currentUser.flags = currentUser.flags | FLAG_ADMIN;
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        UserInfo removeUser = mAdminUser;
        mockRemoveUser(removeUser);

        removeUser(removeUser.id, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(), UserRemovalResult.STATUS_SUCCESSFUL);
        assertHalRemove(currentUser, removeUser);
    }

    @Test
    public void testRemoveUser_success() throws Exception {
        UserInfo currentUser = mAdminUser;
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        UserInfo removeUser = mRegularUser;
        mockRemoveUser(removeUser);

        removeUser(removeUser.id, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);
        UserRemovalResult result = getUserRemovalResult();

        assertUserRemovalResultStatus(result, UserRemovalResult.STATUS_SUCCESSFUL);
        assertHalRemove(currentUser, removeUser);
    }

    @Test
    public void testRemoveUser_halNotSupported() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        UserInfo removeUser = mRegularUser;
        mockUserHalSupported(false);
        mockRemoveUser(removeUser);

        removeUser(removeUser.id, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(), UserRemovalResult.STATUS_SUCCESSFUL);
        verify(mUserHal, never()).removeUser(any());
    }

    @Test
    public void testRemoveUser_androidFailure() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int targetUserId = mRegularUser.id;
        mockRemoveUser(new UserInfoBuilder(targetUserId).build(), UserManager.REMOVE_RESULT_ERROR);

        removeUser(targetUserId, NO_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_ANDROID_FAILURE);
    }

    @Test
    public void testRemoveUserWithRestriction_nonAdminRemovingAdmin() throws Exception {
        UserInfo currentUser = mRegularUser;
        UserInfo removeUser = mAdminUser;
        mockGetCallingUserHandle(currentUser.id);
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        mockRemoveUser(removeUser, /* evenWhenDisallowed= */ true);

        assertThrows(SecurityException.class,
                () -> removeUser(removeUser.id, HAS_CALLER_RESTRICTIONS, mUserRemovalFuture));
    }

    @Test
    public void testRemoveUserWithRestriction_nonAdminRemovingNonAdmin() throws Exception {
        UserInfo currentUser = mRegularUser;
        UserInfo removeUser = mAnotherRegularUser;
        mockGetCallingUserHandle(currentUser.id);
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        mockRemoveUser(removeUser, /* evenWhenDisallowed= */ true);

        assertThrows(SecurityException.class,
                () -> removeUser(removeUser.id, HAS_CALLER_RESTRICTIONS, mUserRemovalFuture));
    }

    @Test
    public void testRemoveUserWithRestriction_nonAdminRemovingItself() throws Exception {
        UserInfo currentUser = mRegularUser;
        UserInfo removeUser = mRegularUser;
        mockGetCallingUserHandle(currentUser.id);
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        mockRemoveUserNoCallback(removeUser, /* evenWhenDisallowed= */ true,
                UserManager.REMOVE_RESULT_SET_EPHEMERAL);

        removeUser(removeUser.id, HAS_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_SUCCESSFUL_SET_EPHEMERAL);
        assertNoHalUserRemoval();
    }

    @Test
    public void testRemoveUserWithRestriction_adminRemovingAdmin() throws Exception {
        UserInfo currentUser = mAdminUser;
        UserInfo removeUser = mAnotherAdminUser;
        mockGetCallingUserHandle(currentUser.id);
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        mockRemoveUser(removeUser, /* evenWhenDisallowed= */ true);

        removeUser(removeUser.id, HAS_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(), UserRemovalResult.STATUS_SUCCESSFUL);
        assertHalRemove(currentUser, removeUser, /* evenWhenDisallowed= */ true);
    }

    @Test
    public void testRemoveUserWithRestriction_adminRemovingNonAdmin() throws Exception {
        UserInfo currentUser = mAdminUser;
        UserInfo removeUser = mRegularUser;
        mockGetCallingUserHandle(currentUser.id);
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        mockRemoveUser(removeUser, /* evenWhenDisallowed= */ true);

        removeUser(removeUser.id, HAS_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(), UserRemovalResult.STATUS_SUCCESSFUL);
        assertHalRemove(currentUser, removeUser, /* evenWhenDisallowed= */ true);
    }

    @Test
    public void testRemoveUserWithRestriction_adminRemovingItself() throws Exception {
        UserInfo currentUser = mAdminUser;
        UserInfo removeUser = mAdminUser;
        mockGetCallingUserHandle(currentUser.id);
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        mockRemoveUserNoCallback(removeUser, /* evenWhenDisallowed= */ true,
                UserManager.REMOVE_RESULT_SET_EPHEMERAL);

        removeUser(removeUser.id,
                HAS_CALLER_RESTRICTIONS, mUserRemovalFuture);

        assertUserRemovalResultStatus(getUserRemovalResult(),
                UserRemovalResult.STATUS_SUCCESSFUL_SET_EPHEMERAL);
        assertNoHalUserRemoval();
    }

    @Test
    public void testSwitchUser_nullReceiver() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);

        assertThrows(NullPointerException.class,
                () -> switchUser(mAdminUser.id, mAsyncCallTimeoutMs, null));
    }

    @Test
    public void testSwitchUser_nonExistingTarget() throws Exception {
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .switchUser(NON_EXISTING_USER, mAsyncCallTimeoutMs, mUserSwitchFuture));
    }

    @Test
    public void testSwitchUser_noUserSwitchability() throws Exception {
        UserInfo currentUser = mAdminUser;
        mockExistingUsersAndCurrentUser(mExistingUsers, currentUser);
        doReturn(UserManager.SWITCHABILITY_STATUS_SYSTEM_USER_LOCKED).when(mMockedUserManager)
                .getUserSwitchability();

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_NOT_SWITCHABLE);
    }

    @Test
    public void testSwitchUser_targetSameAsCurrentUser() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);

        switchUser(mAdminUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_OK_USER_ALREADY_IN_FOREGROUND);

        verifyNoUserSwitch();
    }

    @Test
    public void testSwitchUser_halNotSupported_success() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockUserHalSupported(false);
        mockAmSwitchUser(mRegularUser, true);

        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        verify(mUserHal, never()).switchUser(any(), anyInt(), any());

        // update current user due to successful user switch
        mockCurrentUser(mRegularUser);
        sendUserUnlockedEvent(mRegularUser.id);
        assertNoHalUserSwitch();
        assertNoPostSwitch();
    }

    @Test
    public void testSwitchUser_halNotSupported_failure() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockUserHalSupported(false);

        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_ANDROID_FAILURE);
        assertNoHalUserSwitch();
    }

    @Test
    public void testSwitchUser_HalSuccessAndroidSuccess() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus()).isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);

        // update current user due to successful user switch
        mockCurrentUser(mGuestUser);
        sendUserUnlockedEvent(mGuestUser.id);
        assertPostSwitch(requestId, mGuestUser.id, mGuestUser.id);
    }

    @Test
    public void testSwitchUser_HalSuccessAndroidFailure() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, false);

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_ANDROID_FAILURE);
        assertPostSwitch(requestId, mAdminUser.id, mGuestUser.id);
    }

    @Test
    public void testSwitchUser_HalFailure() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mSwitchUserResponse.status = SwitchUserStatus.FAILURE;
        mSwitchUserResponse.errorMessage = "Error Message";
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        UserSwitchResult result = getUserSwitchResult();
        assertThat(result.getStatus()).isEqualTo(UserSwitchResult.STATUS_HAL_FAILURE);
        assertThat(result.getErrorMessage()).isEqualTo(mSwitchUserResponse.errorMessage);
        verifyNoUserSwitch();
    }

    @Test
    public void testSwitchUser_error_badCallbackStatus() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mockHalSwitch(mAdminUser.id, HalCallback.STATUS_WRONG_HAL_RESPONSE, mSwitchUserResponse,
                mGuestUser);

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_HAL_INTERNAL_FAILURE);
        verifyNoUserSwitch();
    }

    @Test
    public void testSwitchUser_failUxRestrictedOnInit() throws Exception {
        mockGetUxRestrictions(/*restricted= */ true);
        mockExistingUsersAndCurrentUser(mAdminUser);

        initService();
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        assertThat(getUserSwitchResult().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_UX_RESTRICTION_FAILURE);
        assertNoHalUserSwitch();
        verifyNoUserSwitch();
    }

    @Test
    public void testSwitchUser_failUxRestrictionsChanged() throws Exception {
        mockGetUxRestrictions(/*restricted= */ false); // not restricted when CarService init()s
        mockExistingUsersAndCurrentUser(mAdminUser);
        mSwitchUserResponse.requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);

        // Should be ok first time...
        ICarUxRestrictionsChangeListener listener = initService();
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);
        assertThat(getUserSwitchResult().getStatus()).isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);

        // ...but then fail after the state changed
        mockCurrentUser(mGuestUser);
        updateUxRestrictions(listener, /* restricted= */ true); // changed state
        switchUser(mAdminUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);
        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_UX_RESTRICTION_FAILURE);

        // Verify only initial call succeeded (if second was also called the mocks, verify() would
        // fail because it was called more than once()
        assertHalSwitchAnyUser();
        verifyAnyUserSwitch();
    }

    @Test
    public void testSwitchUser_multipleCallsDifferentUser_beforeFirstUserUnlocked()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // calling another user switch before unlock
        int newRequestId = 43;
        SwitchUserResponse switchUserResponse = new SwitchUserResponse();
        switchUserResponse.status = SwitchUserStatus.SUCCESS;
        switchUserResponse.requestId = newRequestId;
        mockHalSwitch(mAdminUser.id, mRegularUser, switchUserResponse);
        mockAmSwitchUser(mRegularUser, true);
        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);

        assertThat(getUserSwitchResult().getStatus()).isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertNoPostSwitch();
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
        assertHalSwitch(mAdminUser.id, mRegularUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsDifferentUser_beforeFirstUserUnlock_abandonFirstCall()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // calling another user switch before unlock
        int newRequestId = 43;
        SwitchUserResponse switchUserResponse = new SwitchUserResponse();
        switchUserResponse.status = SwitchUserStatus.SUCCESS;
        switchUserResponse.requestId = newRequestId;
        mockHalSwitch(mAdminUser.id, mRegularUser, switchUserResponse);
        mockAmSwitchUser(mRegularUser, true);
        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);
        mockCurrentUser(mRegularUser);
        sendUserUnlockedEvent(mRegularUser.id);

        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertPostSwitch(newRequestId, mRegularUser.id, mRegularUser.id);
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
        assertHalSwitch(mAdminUser.id, mRegularUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsDifferentUser_beforeHALResponded()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // calling another user switch before unlock
        int newRequestId = 43;
        SwitchUserResponse switchUserResponse = new SwitchUserResponse();
        switchUserResponse.status = SwitchUserStatus.SUCCESS;
        switchUserResponse.requestId = newRequestId;
        mockHalSwitch(mAdminUser.id, mRegularUser, switchUserResponse);
        mockAmSwitchUser(mRegularUser, true);
        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);

        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertNoPostSwitch();
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
        assertHalSwitch(mAdminUser.id, mRegularUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsDifferentUser_beforeHALResponded_abandonFirstCall()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // calling another user switch before unlock
        int newRequestId = 43;
        SwitchUserResponse switchUserResponse = new SwitchUserResponse();
        switchUserResponse.status = SwitchUserStatus.SUCCESS;
        switchUserResponse.requestId = newRequestId;
        mockHalSwitch(mAdminUser.id, mRegularUser, switchUserResponse);
        mockAmSwitchUser(mRegularUser, true);
        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);

        mockCurrentUser(mRegularUser);
        sendUserUnlockedEvent(mRegularUser.id);

        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertPostSwitch(newRequestId, mRegularUser.id, mRegularUser.id);
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
        assertHalSwitch(mAdminUser.id, mRegularUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsDifferentUser_HALRespondedLate_abandonFirstCall()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        BlockingAnswer<Void> blockingAnswer = mockHalSwitchLateResponse(mAdminUser.id, mGuestUser,
                mSwitchUserResponse);
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // calling another user switch before unlock
        int newRequestId = 43;
        SwitchUserResponse switchUserResponse = new SwitchUserResponse();
        switchUserResponse.status = SwitchUserStatus.SUCCESS;
        switchUserResponse.requestId = newRequestId;
        mockHalSwitch(mAdminUser.id, mRegularUser, switchUserResponse);
        mockAmSwitchUser(mRegularUser, true);
        switchUser(mRegularUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);

        mockCurrentUser(mRegularUser);
        sendUserUnlockedEvent(mRegularUser.id);
        blockingAnswer.unblock();

        UserSwitchResult result = getUserSwitchResult();
        assertThat(result.getStatus())
                .isEqualTo(UserSwitchResult.STATUS_TARGET_USER_ABANDONED_DUE_TO_A_NEW_REQUEST);
        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertPostSwitch(newRequestId, mRegularUser.id, mRegularUser.id);
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
        assertHalSwitch(mAdminUser.id, mRegularUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsSameUser_beforeHALResponded() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // calling another user switch before unlock
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);

        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_TARGET_USER_ALREADY_BEING_SWITCHED_TO);
        assertNoPostSwitch();
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsSameUser_beforeFirstUserUnlocked() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);
        // calling another user switch before unlock
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);

        assertThat(getUserSwitchResult().getStatus()).isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_TARGET_USER_ALREADY_BEING_SWITCHED_TO);
        assertNoPostSwitch();
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
    }

    @Test
    public void testSwitchUser_multipleCallsSameUser_beforeFirstUserUnlocked_noAffectOnFirstCall()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);

        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);
        int newRequestId = 43;
        mSwitchUserResponse.requestId = newRequestId;

        // calling another user switch before unlock
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture2);
        mockCurrentUser(mGuestUser);
        sendUserUnlockedEvent(mGuestUser.id);

        assertThat(getUserSwitchResult().getStatus()).isEqualTo(UserSwitchResult.STATUS_SUCCESSFUL);
        assertThat(getUserSwitchResult2().getStatus())
                .isEqualTo(UserSwitchResult.STATUS_TARGET_USER_ALREADY_BEING_SWITCHED_TO);
        assertPostSwitch(requestId, mGuestUser.id, mGuestUser.id);
        assertHalSwitch(mAdminUser.id, mGuestUser.id);
    }

    @Test
    public void testSwitchUser_InvalidPermission() throws Exception {
        mockManageUsersPermission(android.Manifest.permission.MANAGE_USERS, false);
        assertThrows(SecurityException.class, () -> mCarUserService
                .switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture));
    }

    @Test
    public void testLegacyUserSwitch_ok() throws Exception {
        mockExistingUsers(mExistingUsers);

        sendUserSwitchingEvent(mAdminUser.id, mRegularUser.id);

        verify(mUserHal).legacyUserSwitch(isSwitchUserRequest(0, mAdminUser.id, mRegularUser.id));
    }

    @Test
    public void testLegacyUserSwitch_notCalledAfterNormalSwitch() throws Exception {
        // Arrange - emulate normal switch
        mockExistingUsersAndCurrentUser(mAdminUser);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // Act - trigger legacy switch
        sendUserSwitchingEvent(mAdminUser.id, mGuestUser.id);

        // Assert
        verify(mUserHal, never()).legacyUserSwitch(any());
    }

    @Test
    public void testSetSwitchUserUI_receiverSetAndCalled() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int callerId = Binder.getCallingUid();
        mockCallerUid(callerId, true);
        int requestId = 42;
        mSwitchUserResponse.status = SwitchUserStatus.SUCCESS;
        mSwitchUserResponse.requestId = requestId;
        mockHalSwitch(mAdminUser.id, mGuestUser, mSwitchUserResponse);
        mockAmSwitchUser(mGuestUser, true);

        mCarUserService.setUserSwitchUiCallback(mSwitchUserUiReceiver);
        switchUser(mGuestUser.id, mAsyncCallTimeoutMs, mUserSwitchFuture);

        // update current user due to successful user switch
        verify(mSwitchUserUiReceiver).send(mGuestUser.id, null);
    }

    @Test
    public void testSetSwitchUserUI_nonCarSysUiCaller() throws Exception {
        int callerId = Binder.getCallingUid();
        mockCallerUid(callerId, false);

        assertThrows(SecurityException.class,
                () -> mCarUserService.setUserSwitchUiCallback(mSwitchUserUiReceiver));
    }

    @Test
    public void testSwitchUser_OEMRequest_success() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockAmSwitchUser(mRegularUser, true);
        int requestId = -1;

        mCarUserService.switchAndroidUserFromHal(requestId, mRegularUser.id);
        mockCurrentUser(mRegularUser);
        sendUserUnlockedEvent(mRegularUser.id);

        assertPostSwitch(requestId, mRegularUser.id, mRegularUser.id);
    }

    @Test
    public void testSwitchUser_OEMRequest_failure() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockAmSwitchUser(mRegularUser, false);
        int requestId = -1;

        mCarUserService.switchAndroidUserFromHal(requestId, mRegularUser.id);

        assertPostSwitch(requestId, mAdminUser.id, mRegularUser.id);
    }

    @Test
    public void testCreateUser_nullType() throws Exception {
        assertThrows(NullPointerException.class, () -> mCarUserService
                .createUser("dude", null, 108, mAsyncCallTimeoutMs, mUserCreationFuture,
                        NO_CALLER_RESTRICTIONS));
    }

    @Test
    public void testCreateUser_nullReceiver() throws Exception {
        assertThrows(NullPointerException.class, () -> mCarUserService
                .createUser("dude", "TypeONegative", 108, mAsyncCallTimeoutMs, null,
                        NO_CALLER_RESTRICTIONS));
    }

    @Test
    public void testCreateUser_umCreateReturnsNull() throws Exception {
        // No need to mock um.createUser() to return null

        createUser("dude", "TypeONegative", 108, mAsyncCallTimeoutMs,
                mUserCreationFuture, NO_CALLER_RESTRICTIONS);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_ANDROID_FAILURE);
        assertThat(result.getUser()).isNull();
        assertThat(result.getErrorMessage()).isNull();
        assertNoHalUserCreation();
        verifyNoUserRemoved();
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_umCreateThrowsException() throws Exception {
        mockUmCreateUser(mMockedUserManager, "dude", "TypeONegative", 108,
                new RuntimeException("D'OH!"));

        createUser("dude", "TypeONegative", 108, mAsyncCallTimeoutMs,
                mUserCreationFuture, NO_CALLER_RESTRICTIONS);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_ANDROID_FAILURE);
        assertThat(result.getUser()).isNull();
        assertThat(result.getErrorMessage()).isNull();
        assertNoHalUserCreation();
        verifyNoUserRemoved();
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_internalHalFailure() throws Exception {
        UserInfo newUser = mockUmCreateUser(mMockedUserManager, "dude", "TypeONegative", 108, 42);
        mockHalCreateUser(HalCallback.STATUS_INVALID, /* not_used_status= */ -1);
        mockRemoveUser(newUser);

        createUser("dude", "TypeONegative", 108, mAsyncCallTimeoutMs,
                mUserCreationFuture, NO_CALLER_RESTRICTIONS);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_HAL_INTERNAL_FAILURE);
        assertThat(result.getUser()).isNull();
        assertThat(result.getErrorMessage()).isNull();
        verifyUserRemoved(newUser.id);
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_halFailure() throws Exception {
        UserInfo newUser = mockUmCreateUser(mMockedUserManager, "dude", "TypeONegative", 108, 42);
        mockHalCreateUser(HalCallback.STATUS_OK, CreateUserStatus.FAILURE);
        mockRemoveUser(newUser);

        createUser("dude", "TypeONegative", 108, mAsyncCallTimeoutMs,
                mUserCreationFuture, NO_CALLER_RESTRICTIONS);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_HAL_FAILURE);
        assertThat(result.getUser()).isNull();
        assertThat(result.getErrorMessage()).isNull();

        verifyUserRemoved(newUser.id);
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_halServiceThrowsRuntimeException() throws Exception {
        UserInfo newUser = mockUmCreateUser(mMockedUserManager, "dude", "TypeONegative", 108, 42);
        mockHalCreateUserThrowsRuntimeException();
        mockRemoveUser(newUser);

        createUser("dude", "TypeONegative", 108, mAsyncCallTimeoutMs,
                mUserCreationFuture, NO_CALLER_RESTRICTIONS);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_HAL_INTERNAL_FAILURE);
        assertThat(result.getUser()).isNull();
        assertThat(result.getErrorMessage()).isNull();

        verifyUserRemoved(newUser.id);
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_halNotSupported_success() throws Exception {
        mockUserHalSupported(false);
        mockExistingUsersAndCurrentUser(mAdminUser);
        int userId = mGuestUser.id;
        mockUmCreateUser(mMockedUserManager, "dude", UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL, userId);

        createUser("dude", UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL, mAsyncCallTimeoutMs, mUserCreationFuture,
                NO_CALLER_RESTRICTIONS);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_SUCCESSFUL);
        assertNoHalUserCreation();
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_success() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        int userId = mGuestUser.id;
        mockUmCreateUser(mMockedUserManager, "dude", UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL, userId);
        ArgumentCaptor<CreateUserRequest> requestCaptor =
                mockHalCreateUser(HalCallback.STATUS_OK, CreateUserStatus.SUCCESS);

        createUser("dude", UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL, mAsyncCallTimeoutMs, mUserCreationFuture,
                NO_CALLER_RESTRICTIONS);

        // Assert request
        CreateUserRequest request = requestCaptor.getValue();
        Log.d(TAG, "createUser() request: " + request);
        assertThat(request.newUserName).isEqualTo("dude");
        assertThat(request.newUserInfo.userId).isEqualTo(userId);
        assertThat(request.newUserInfo.flags).isEqualTo(UserFlags.GUEST | UserFlags.EPHEMERAL);
        assertDefaultUsersInfo(request.usersInfo, mAdminUser);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_SUCCESSFUL);
        assertThat(result.getErrorMessage()).isNull();
        UserInfo newUser = result.getUser();
        assertThat(newUser).isNotNull();
        assertThat(newUser.id).isEqualTo(userId);
        assertThat(newUser.name).isEqualTo("dude");
        assertThat(newUser.userType).isEqualTo(UserManager.USER_TYPE_FULL_GUEST);
        assertThat(newUser.flags).isEqualTo(UserInfo.FLAG_EPHEMERAL);

        verifyNoUserRemoved();
        assertNoHalUserRemoval();
    }

    @Test
    public void testCreateUser_success_nullName() throws Exception {
        String nullName = null;
        mockExistingUsersAndCurrentUser(mAdminUser);
        int userId = mGuestUser.id;
        mockUmCreateUser(mMockedUserManager, nullName, UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL, userId);
        ArgumentCaptor<CreateUserRequest> requestCaptor =
                mockHalCreateUser(HalCallback.STATUS_OK, CreateUserStatus.SUCCESS);

        createUser(nullName, UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL, mAsyncCallTimeoutMs, mUserCreationFuture,
                NO_CALLER_RESTRICTIONS);

        // Assert request
        CreateUserRequest request = requestCaptor.getValue();
        Log.d(TAG, "createUser() request: " + request);
        assertThat(request.newUserName).isEmpty();
        assertThat(request.newUserInfo.userId).isEqualTo(userId);
        assertThat(request.newUserInfo.flags).isEqualTo(UserFlags.GUEST | UserFlags.EPHEMERAL);
        assertDefaultUsersInfo(request.usersInfo, mAdminUser);

        UserCreationResult result = getUserCreationResult();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_SUCCESSFUL);
        assertThat(result.getErrorMessage()).isNull();

        UserInfo newUser = result.getUser();
        assertThat(newUser).isNotNull();
        assertThat(newUser.id).isEqualTo(userId);
        assertThat(newUser.name).isNull();
        assertThat(newUser.userType).isEqualTo(UserManager.USER_TYPE_FULL_GUEST);
        assertThat(newUser.flags).isEqualTo(UserInfo.FLAG_EPHEMERAL);

        verifyNoUserRemoved();
        verify(mUserHal, never()).removeUser(any());
    }

    @Test
    public void testCreateUser_binderMethod() {
        CarUserService spy = spy(mCarUserService);
        AndroidFuture<UserCreationResult> receiver = new AndroidFuture<>();
        int flags = 42;
        int timeoutMs = 108;

        spy.createUser("name", "type", flags, timeoutMs, receiver);

        verify(spy).createUser("name", "type", flags, timeoutMs, receiver,
                NO_CALLER_RESTRICTIONS);
    }

    @Test
    public void testCreateUserWithRestrictions_nonAdminCreatingAdmin() throws Exception {
        UserInfo currentUser = mRegularUser;
        mockExistingUsersAndCurrentUser(currentUser);
        mockGetCallingUserHandle(currentUser.id);

        createUser("name", UserManager.USER_TYPE_FULL_SECONDARY,
                UserInfo.FLAG_ADMIN, mAsyncCallTimeoutMs,
                mUserCreationFuture, HAS_CALLER_RESTRICTIONS);
        assertInvalidArgumentsFailure();
    }

    private void assertInvalidArgumentsFailure() throws Exception {
        UserCreationResult result = getUserCreationResult();
        assertThat(result).isNotNull();
        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_INVALID_REQUEST);
        assertThat(result.getUser()).isNull();
    }

    @Test
    public void testCreateUserWithRestrictions_invalidTypes() throws Exception {
        createUserWithRestrictionsInvalidTypes(UserManager.USER_TYPE_FULL_DEMO);
        createUserWithRestrictionsInvalidTypes(UserManager.USER_TYPE_FULL_RESTRICTED);
        createUserWithRestrictionsInvalidTypes(UserManager.USER_TYPE_FULL_SYSTEM);
        createUserWithRestrictionsInvalidTypes(UserManager.USER_TYPE_PROFILE_MANAGED);
        createUserWithRestrictionsInvalidTypes(UserManager.USER_TYPE_SYSTEM_HEADLESS);
    }


    private void createUserWithRestrictionsInvalidTypes(@NonNull String type) throws Exception {
        mCarUserService.createUser("name", type, /* flags= */ 0, mAsyncCallTimeoutMs,
                mUserCreationFuture, HAS_CALLER_RESTRICTIONS);
        waitForHandlerThreadToFinish();
        assertInvalidArgumentsFailure();
    }

    @Test
    public void testCreateUserWithRestrictions_invalidFlags() throws Exception {
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_DEMO);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_DISABLED);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_EPHEMERAL);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_FULL);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_INITIALIZED);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_MANAGED_PROFILE);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_PRIMARY);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_QUIET_MODE);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_RESTRICTED);
        createUserWithRestrictionsInvalidTypes(UserInfo.FLAG_SYSTEM);
    }

    private void createUserWithRestrictionsInvalidTypes(int flags) throws Exception {
        mCarUserService.createUser("name", UserManager.USER_TYPE_FULL_SECONDARY, flags,
                mAsyncCallTimeoutMs, mUserCreationFuture, HAS_CALLER_RESTRICTIONS);
        waitForHandlerThreadToFinish();
        assertInvalidArgumentsFailure();
    }

    @Test
    @ExpectWtf
    public void testCreateUserEvenWhenDisallowed_noHelper() throws Exception {
        UserInfo userInfo = mCarUserService.createUserEvenWhenDisallowed("name",
                UserManager.USER_TYPE_FULL_SECONDARY, UserInfo.FLAG_ADMIN);
        waitForHandlerThreadToFinish();

        assertThat(userInfo).isNull();
    }

    @Test
    public void testCreateUserEvenWhenDisallowed_remoteException() throws Exception {
        mCarUserService.setCarServiceHelper(mICarServiceHelper);
        when(mICarServiceHelper.createUserEvenWhenDisallowed(any(), any(), anyInt()))
                .thenThrow(new RemoteException("D'OH!"));

        UserInfo userInfo = mCarUserService.createUserEvenWhenDisallowed("name",
                UserManager.USER_TYPE_FULL_SECONDARY, UserInfo.FLAG_ADMIN);
        waitForHandlerThreadToFinish();

        assertThat(userInfo).isNull();
    }

    @Test
    public void testCreateUserEvenWhenDisallowed_success() throws Exception {
        UserInfo user = new UserInfoBuilder(100)
                .setName("name")
                .setType(UserManager.USER_TYPE_FULL_SECONDARY)
                .setFlags(UserInfo.FLAG_ADMIN)
                .build();
        mCarUserService.setCarServiceHelper(mICarServiceHelper);
        when(mICarServiceHelper.createUserEvenWhenDisallowed("name",
                UserManager.USER_TYPE_FULL_SECONDARY, UserInfo.FLAG_ADMIN)).thenReturn(user);

        UserInfo actualUser = mCarUserService.createUserEvenWhenDisallowed("name",
                UserManager.USER_TYPE_FULL_SECONDARY, UserInfo.FLAG_ADMIN);
        waitForHandlerThreadToFinish();

        assertThat(actualUser).isNotNull();
        assertThat(actualUser.id).isEqualTo(100);
        assertThat(actualUser.name).isEqualTo("name");
        assertThat(actualUser.userType).isEqualTo(UserManager.USER_TYPE_FULL_SECONDARY);
        assertThat(actualUser.flags).isEqualTo(UserInfo.FLAG_ADMIN);
    }

    @Test
    public void testStartUserInBackground_success() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user1", NO_USER_INFO_FLAGS);
        mockCurrentUser(mRegularUser);
        mockUmGetUserInfo(mMockedUserManager, userInfo);
        mockAmStartUserInBackground(userId, true);

        AndroidFuture<UserStartResult> userStartResult = new AndroidFuture<>();
        startUserInBackground(userId, userStartResult);

        assertThat(getResult(userStartResult).getStatus())
                .isEqualTo(UserStartResult.STATUS_SUCCESSFUL);
        assertThat(getResult(userStartResult).isSuccess()).isTrue();
    }

    @Test
    public void testStartUserInBackground_permissionDenied() throws Exception {
        int userId = 101;
        mockManageUsersPermission(android.Manifest.permission.MANAGE_USERS, false);
        mockManageUsersPermission(android.Manifest.permission.CREATE_USERS, false);

        AndroidFuture<UserStartResult> userStartResult = new AndroidFuture<>();
        assertThrows(SecurityException.class,
                () -> startUserInBackground(userId, userStartResult));
    }

    @Test
    public void testStartUserInBackground_fail() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user1", NO_USER_INFO_FLAGS);
        mockCurrentUser(mRegularUser);
        mockUmGetUserInfo(mMockedUserManager, userInfo);
        mockAmStartUserInBackground(userId, false);

        AndroidFuture<UserStartResult> userStartResult = new AndroidFuture<>();
        startUserInBackground(userId, userStartResult);

        assertThat(getResult(userStartResult).getStatus())
                .isEqualTo(UserStartResult.STATUS_ANDROID_FAILURE);
        assertThat(getResult(userStartResult).isSuccess()).isFalse();
    }

    @Test
    public void testStartUserInBackground_currentUser() throws Exception {
        int userId = 101;
        UserInfo userInfo = new UserInfo(userId, "user1", NO_USER_INFO_FLAGS);
        mockGetCurrentUser(userId);
        mockUmGetUserInfo(mMockedUserManager, userInfo);
        mockAmStartUserInBackground(userId, true);

        AndroidFuture<UserStartResult> userStartResult = new AndroidFuture<>();
        startUserInBackground(userId, userStartResult);

        assertThat(getResult(userStartResult).getStatus())
                .isEqualTo(UserStartResult.STATUS_SUCCESSFUL_USER_IS_CURRENT_USER);
        assertThat(getResult(userStartResult).isSuccess()).isTrue();
    }

    @Test
    public void testStartUserInBackground_userDoesNotExist() throws Exception {
        int userId = 101;
        mockCurrentUser(mRegularUser);
        when(mMockedUserManager.getUserInfo(userId)).thenReturn(null);
        mockAmStartUserInBackground(userId, true);

        AndroidFuture<UserStartResult> userStartResult = new AndroidFuture<>();
        startUserInBackground(userId, userStartResult);

        assertThat(getResult(userStartResult).getStatus())
                .isEqualTo(UserStartResult.STATUS_USER_DOES_NOT_EXIST);
        assertThat(getResult(userStartResult).isSuccess()).isFalse();
    }

    @Test
    public void testIsHalSupported() throws Exception {
        when(mUserHal.isSupported()).thenReturn(true);
        assertThat(mCarUserService.isUserHalSupported()).isTrue();
    }

    @Test
    public void testGetUserIdentificationAssociation_nullTypes() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> mCarUserService.getUserIdentificationAssociation(null));
    }

    @Test
    public void testGetUserIdentificationAssociation_emptyTypes() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> mCarUserService.getUserIdentificationAssociation(new int[] {}));
    }

    @Test
    public void testGetUserIdentificationAssociation_noPermission() throws Exception {
        mockManageUsersPermission(android.Manifest.permission.MANAGE_USERS, false);
        assertThrows(SecurityException.class,
                () -> mCarUserService.getUserIdentificationAssociation(new int[] { 42 }));
    }

    @Test
    public void testGetUserIdentificationAssociation_noSuchUser() throws Exception {
        // Should fail because we're not mocking UserManager.getUserInfo() to set the flag
        assertThrows(IllegalArgumentException.class,
                () -> mCarUserService.getUserIdentificationAssociation(new int[] { 42 }));
    }

    @Test
    public void testGetUserIdentificationAssociation_service_returnNull() throws Exception {
        mockCurrentUserForBinderCalls();

        UserIdentificationAssociationResponse response = mCarUserService
                .getUserIdentificationAssociation(new int[] { 108 });

        assertThat(response.isSuccess()).isFalse();
        assertThat(response.getValues()).isNull();
        assertThat(response.getErrorMessage()).isNull();
    }

    @Test
    public void testGetUserIdentificationAssociation_halNotSupported() throws Exception {
        mockUserHalUserAssociationSupported(false);

        UserIdentificationAssociationResponse response = mCarUserService
                .getUserIdentificationAssociation(new int[] { });

        assertThat(response.isSuccess()).isFalse();
        assertThat(response.getValues()).isNull();
        assertThat(response.getErrorMessage()).isEqualTo(CarUserService.VEHICLE_HAL_NOT_SUPPORTED);
        verify(mUserHal, never()).getUserAssociation(any());
    }

    @Test
    public void testGetUserIdentificationAssociation_ok() throws Exception {
        UserInfo currentUser = mockCurrentUserForBinderCalls();

        int[] types = new int[] { 1, 2, 3 };
        int[] values = new int[] { 10, 20, 30 };
        mockHalGetUserIdentificationAssociation(currentUser, types, values, "D'OH!");

        UserIdentificationAssociationResponse response = mCarUserService
                .getUserIdentificationAssociation(types);

        assertThat(response.isSuccess()).isTrue();
        assertThat(response.getValues()).asList().containsAtLeast(10, 20, 30).inOrder();
        assertThat(response.getErrorMessage()).isEqualTo("D'OH!");
    }

    @Test
    public void testSetUserIdentificationAssociation_nullTypes() throws Exception {
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        null, new int[] {42}, mUserAssociationRespFuture));
    }

    @Test
    public void testSetUserIdentificationAssociation_emptyTypes() throws Exception {
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        new int[0], new int[] {42}, mUserAssociationRespFuture));
    }

    @Test
    public void testSetUserIdentificationAssociation_nullValues() throws Exception {
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        new int[] {42}, null, mUserAssociationRespFuture));
    }
    @Test
    public void testSetUserIdentificationAssociation_sizeMismatch() throws Exception {
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        new int[] {1}, new int[] {2, 2}, mUserAssociationRespFuture));
    }

    @Test
    public void testSetUserIdentificationAssociation_nullFuture() throws Exception {
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        new int[] {42}, new int[] {42}, null));
    }

    @Test
    public void testSetUserIdentificationAssociation_noPermission() throws Exception {
        mockManageUsersPermission(android.Manifest.permission.MANAGE_USERS, false);
        assertThrows(SecurityException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        new int[] {42}, new int[] {42}, mUserAssociationRespFuture));
    }

    @Test
    public void testSetUserIdentificationAssociation_noCurrentUser() throws Exception {
        // Should fail because we're not mocking UserManager.getUserInfo() to set the flag
        assertThrows(IllegalArgumentException.class, () -> mCarUserService
                .setUserIdentificationAssociation(mAsyncCallTimeoutMs,
                        new int[] {42}, new int[] {42}, mUserAssociationRespFuture));
    }

    @Test
    public void testSetUserIdentificationAssociation_halNotSupported() throws Exception {
        int[] types = new int[] { 1, 2, 3 };
        int[] values = new int[] { 10, 20, 30 };
        mockUserHalUserAssociationSupported(false);

        mCarUserService.setUserIdentificationAssociation(mAsyncCallTimeoutMs, types, values,
                mUserAssociationRespFuture);
        UserIdentificationAssociationResponse response = getUserAssociationRespResult();

        assertThat(response.isSuccess()).isFalse();
        assertThat(response.getValues()).isNull();
        assertThat(response.getErrorMessage()).isEqualTo(CarUserService.VEHICLE_HAL_NOT_SUPPORTED);
        verify(mUserHal, never()).setUserAssociation(anyInt(), any(), any());
    }

    @Test
    public void testSetUserIdentificationAssociation_halFailedWithErrorMessage() throws Exception {
        mockCurrentUserForBinderCalls();
        mockHalSetUserIdentificationAssociationFailure("D'OH!");
        int[] types = new int[] { 1, 2, 3 };
        int[] values = new int[] { 10, 20, 30 };
        mCarUserService.setUserIdentificationAssociation(mAsyncCallTimeoutMs, types, values,
                mUserAssociationRespFuture);

        UserIdentificationAssociationResponse response = getUserAssociationRespResult();

        assertThat(response.isSuccess()).isFalse();
        assertThat(response.getValues()).isNull();
        assertThat(response.getErrorMessage()).isEqualTo("D'OH!");

    }

    @Test
    public void testSetUserIdentificationAssociation_halFailedWithoutErrorMessage()
            throws Exception {
        mockCurrentUserForBinderCalls();
        mockHalSetUserIdentificationAssociationFailure(/* errorMessage= */ null);
        int[] types = new int[] { 1, 2, 3 };
        int[] values = new int[] { 10, 20, 30 };
        mCarUserService.setUserIdentificationAssociation(mAsyncCallTimeoutMs, types, values,
                mUserAssociationRespFuture);

        UserIdentificationAssociationResponse response = getUserAssociationRespResult();

        assertThat(response.isSuccess()).isFalse();
        assertThat(response.getValues()).isNull();
        assertThat(response.getErrorMessage()).isNull();
    }

    @Test
    public void testSetUserIdentificationAssociation_ok() throws Exception {
        UserInfo currentUser = mockCurrentUserForBinderCalls();

        int[] types = new int[] { 1, 2, 3 };
        int[] values = new int[] { 10, 20, 30 };
        mockHalSetUserIdentificationAssociationSuccess(currentUser, types, values, "D'OH!");

        mCarUserService.setUserIdentificationAssociation(mAsyncCallTimeoutMs, types, values,
                mUserAssociationRespFuture);

        UserIdentificationAssociationResponse response = getUserAssociationRespResult();

        assertThat(response.isSuccess()).isTrue();
        assertThat(response.getValues()).asList().containsAtLeast(10, 20, 30).inOrder();
        assertThat(response.getErrorMessage()).isEqualTo("D'OH!");
    }

    @Test
    public void testInitBootUser_halNotSupported() {
        mockUserHalSupported(false);

        mCarUserService.initBootUser();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_DEFAULT_BEHAVIOR
                    && info.userLocales == null;
        }));
    }

    @Test
    public void testInitBootUser_halNullResponse() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockHalGetInitialInfo(mAdminUser.id, null);

        mCarUserService.initBootUser();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_DEFAULT_BEHAVIOR;
        }));
    }

    @Test
    public void testInitBootUser_halDefaultResponse() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mGetUserInfoResponse.action = InitialUserInfoResponseAction.DEFAULT;
        mGetUserInfoResponse.userLocales = "LOL";
        mockHalGetInitialInfo(mAdminUser.id, mGetUserInfoResponse);

        mCarUserService.initBootUser();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_DEFAULT_BEHAVIOR
                    && info.userLocales.equals("LOL");
        }));
    }

    @Test
    public void testInitBootUser_halSwitchResponse() throws Exception {
        int switchUserId = mGuestUser.id;
        mockExistingUsersAndCurrentUser(mAdminUser);
        mGetUserInfoResponse.action = InitialUserInfoResponseAction.SWITCH;
        mGetUserInfoResponse.userToSwitchOrCreate.userId = switchUserId;
        mockHalGetInitialInfo(mAdminUser.id, mGetUserInfoResponse);

        mCarUserService.initBootUser();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_SWITCH
                    && info.switchUserId == switchUserId;
        }));
    }

    @Test
    public void testInitBootUser_halCreateResponse() throws Exception {
        int newUserFlags = 42;
        String newUserName = "TheDude";
        mockExistingUsersAndCurrentUser(mAdminUser);
        mGetUserInfoResponse.action = InitialUserInfoResponseAction.CREATE;
        mGetUserInfoResponse.userToSwitchOrCreate.flags = newUserFlags;
        mGetUserInfoResponse.userNameToCreate = newUserName;
        mockHalGetInitialInfo(mAdminUser.id, mGetUserInfoResponse);

        mCarUserService.initBootUser();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_CREATE
                    && info.newUserFlags == newUserFlags
                    && info.newUserName == newUserName;
        }));
    }

    @Test
    public void testUpdatePreCreatedUser_success() throws Exception {
        mCarUserService.updatePreCreatedUsers();
        waitForHandlerThreadToFinish();

        verify(mUserPreCreator).managePreCreatedUsers();
    }

    @Test
    @ExpectWtf
    public void testSetInitialUser_nullUser() throws Exception {
        mCarUserService.setInitialUser(null);

        mockInteractAcrossUsersPermission(true);
        assertThat(mCarUserService.getInitialUser()).isNull();
    }

    @Test
    public void testOnSuspend_replace() throws Exception {
        mockExistingUsersAndCurrentUser(mGuestUser);
        when(mInitialUserSetter.canReplaceGuestUser(any())).thenReturn(true);

        CarUserService service = newCarUserService(/* switchGuestUserBeforeGoingSleep= */ true);
        service.onSuspend();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_REPLACE_GUEST;
        }));
        verify(mUserPreCreator).managePreCreatedUsers();
    }

    @Test
    public void testOnSuspend_notReplace() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);

        CarUserService service = newCarUserService(/* switchGuestUserBeforeGoingSleep= */ true);
        service.onSuspend();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter, never()).set(any());
        verify(mUserPreCreator).managePreCreatedUsers();
    }

    @Test
    public void testOnResume_halNullResponse_replaceTrue() throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mockHalGetInitialInfo(mAdminUser.id, null);

        mCarUserService.onResume();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_DEFAULT_BEHAVIOR
                    && info.replaceGuest;
        }));
    }

    @Test
    public void testOnResume_halDefaultResponse_replaceGuest()
            throws Exception {
        mockExistingUsersAndCurrentUser(mAdminUser);
        mGetUserInfoResponse.action = InitialUserInfoResponseAction.DEFAULT;
        mGetUserInfoResponse.userLocales = "LOL";
        mockHalGetInitialInfo(mAdminUser.id, mGetUserInfoResponse);

        mCarUserService.onResume();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_DEFAULT_BEHAVIOR && info.replaceGuest
                    && info.userLocales.equals("LOL");
        }));
    }

    @Test
    public void testOnResume_halSwitchResponse_replaceGuest()
            throws Exception {
        int switchUserId = mGuestUser.id;
        mockExistingUsersAndCurrentUser(mAdminUser);
        mGetUserInfoResponse.action = InitialUserInfoResponseAction.SWITCH;
        mGetUserInfoResponse.userToSwitchOrCreate.userId = switchUserId;
        mockHalGetInitialInfo(mAdminUser.id, mGetUserInfoResponse);

        mCarUserService.onResume();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_SWITCH && info.replaceGuest
                    && info.switchUserId == switchUserId;
        }));
    }

    @Test
    public void testOnResume_halDisabled()
            throws Exception {
        mockUserHalSupported(false);

        mCarUserService.onResume();
        waitForHandlerThreadToFinish();

        verify(mInitialUserSetter).set(argThat((info) -> {
            return info.type == InitialUserSetter.TYPE_DEFAULT_BEHAVIOR && info.replaceGuest;
        }));
    }

    @Test
    public void testInitialUserInfoRequestType_FirstBoot() throws Exception {
        when(mInitialUserSetter.hasInitialUser()).thenReturn(false);
        when(mMockContext.getPackageManager()).thenReturn(mPackageManager);
        when(mPackageManager.isDeviceUpgrading()).thenReturn(true);

        assertThat(mCarUserService.getInitialUserInfoRequestType())
                .isEqualTo(InitialUserInfoRequestType.FIRST_BOOT);
    }

    @Test
    public void testInitialUserInfoRequestType_FirstBootAfterOTA() throws Exception {
        when(mInitialUserSetter.hasInitialUser()).thenReturn(true);
        when(mMockContext.getPackageManager()).thenReturn(mPackageManager);
        when(mPackageManager.isDeviceUpgrading()).thenReturn(true);

        assertThat(mCarUserService.getInitialUserInfoRequestType())
                .isEqualTo(InitialUserInfoRequestType.FIRST_BOOT_AFTER_OTA);
    }

    @Test
    public void testInitialUserInfoRequestType_ColdBoot() throws Exception {
        when(mInitialUserSetter.hasInitialUser()).thenReturn(true);
        when(mMockContext.getPackageManager()).thenReturn(mPackageManager);
        when(mPackageManager.isDeviceUpgrading()).thenReturn(false);

        assertThat(mCarUserService.getInitialUserInfoRequestType())
                .isEqualTo(InitialUserInfoRequestType.COLD_BOOT);
    }

    private void waitForHandlerThreadToFinish() {
        assertThat(mHandler.runWithScissors(() -> {}, DEFAULT_TIMEOUT_MS)).isTrue();
    }

    private void createUser(@Nullable String name, @NonNull String userType,
            @UserInfoFlag int flags,
            int timeoutMs, @NonNull AndroidFuture<UserCreationResult> receiver,
            boolean hasCallerRestrictions) {
        mCarUserService.createUser(name, userType, flags, timeoutMs, receiver,
                hasCallerRestrictions);
        waitForHandlerThreadToFinish();
    }

    private void switchUser(@UserIdInt int userId, int timeoutMs,
            @NonNull AndroidFuture<UserSwitchResult> receiver) {
        mCarUserService.switchUser(userId, timeoutMs, receiver);
        waitForHandlerThreadToFinish();
    }

    private void removeUser(@UserIdInt int userId,
            @NonNull AndroidFuture<UserRemovalResult> userRemovalFuture) {
        mCarUserService.removeUser(userId, userRemovalFuture);
        waitForHandlerThreadToFinish();
    }

    private void removeUser(@UserIdInt int userId, boolean hasCallerRestrictions,
            @NonNull AndroidFuture<UserRemovalResult> userRemovalFuture) {
        mCarUserService.removeUser(userId, hasCallerRestrictions, userRemovalFuture);
        waitForHandlerThreadToFinish();
    }

    private void startUserInBackground(@UserIdInt int userId,
            @NonNull AndroidFuture<UserStartResult> userStartResultFuture) {
        mCarUserService.startUserInBackground(userId, userStartResultFuture);
        waitForHandlerThreadToFinish();
    }

    private void stopUser(@UserIdInt int userId,
            @NonNull AndroidFuture<UserStopResult> userStopResultFuture) {
        mCarUserService.stopUser(userId, userStopResultFuture);
        waitForHandlerThreadToFinish();
    }

    @NonNull
    private UserSwitchResult getUserSwitchResult() throws Exception {
        return getResult(mUserSwitchFuture);
    }

    @NonNull
    private UserSwitchResult getUserSwitchResult2() throws Exception {
        return getResult(mUserSwitchFuture2);
    }

    @NonNull
    private UserCreationResult getUserCreationResult() throws Exception {
        return getResult(mUserCreationFuture);
    }

    @NonNull
    private UserRemovalResult getUserRemovalResult() throws Exception {
        return getResult(mUserRemovalFuture);
    }

    @NonNull
    private UserIdentificationAssociationResponse getUserAssociationRespResult()
            throws Exception {
        return getResult(mUserAssociationRespFuture);
    }

    private CarUserService newCarUserService(boolean switchGuestUserBeforeGoingSleep) {
        when(mMockedResources
                .getBoolean(com.android.car.R.bool.config_switchGuestUserBeforeGoingSleep))
                        .thenReturn(switchGuestUserBeforeGoingSleep);

        return new CarUserService(
                mMockContext,
                mUserHal,
                mMockedUserManager,
                mMockedIActivityManager,
                /* maxRunningUsers= */ 3,
                mInitialUserSetter,
                mUserPreCreator,
                mCarUxRestrictionService,
                mHandler);
    }

    /**
     * This method must be called for cases where the service infers the user id of the caller
     * using Binder - it's not worth the effort of mocking such (native) calls.
     */
    @NonNull
    private UserInfo mockCurrentUserForBinderCalls() {
        int currentUserId = ActivityManager.getCurrentUser();
        Log.d(TAG, "testetUserIdentificationAssociation_ok(): current user is " + currentUserId);
        UserInfo currentUser = mockUmGetUserInfo(mMockedUserManager, currentUserId,
                UserInfo.FLAG_ADMIN);
        return currentUser;
    }

    /**
     * Mock calls that generate a {@code UsersInfo}.
     */
    private void mockExistingUsersAndCurrentUser(@NonNull UserInfo user)
            throws Exception {
        mockExistingUsers(mExistingUsers);
        mockCurrentUser(user);
    }

    private void mockExistingUsersAndCurrentUser(@NonNull List<UserInfo> existingUsers,
            @NonNull UserInfo currentUser) throws Exception {
        mockExistingUsers(existingUsers);
        mockCurrentUser(currentUser);
    }

    private void mockExistingUsers(@NonNull List<UserInfo> existingUsers) {
        mockUmGetUsers(mMockedUserManager, /* excludePartial= */ false, /* excludeDying= */ false,
                /* excludePreCreated= */ true, existingUsers);
        for (UserInfo user : existingUsers) {
            mockUmGetUserInfo(mMockedUserManager, user);
        }
    }

    private void mockCurrentUser(@NonNull UserInfo user) throws Exception {
        when(mMockedIActivityManager.getCurrentUser()).thenReturn(user);
        mockGetCurrentUser(user.id);
    }

    private void mockAmStartUserInBackground(@UserIdInt int userId, boolean result)
            throws Exception {
        when(mMockedIActivityManager.startUserInBackground(userId)).thenReturn(result);
    }

    private void mockAmSwitchUser(@NonNull UserInfo user, boolean result) throws Exception {
        when(mMockedIActivityManager.switchUser(user.id)).thenReturn(result);
    }

    private void mockRemoveUser(@NonNull UserInfo user) {
        mockRemoveUser(user, UserManager.REMOVE_RESULT_REMOVED);
    }

    private void mockRemoveUser(@NonNull UserInfo user, @RemoveResult int result) {
        mockRemoveUser(user, /* evenWhenDisallowed= */ false, result);
    }

    private void mockRemoveUser(@NonNull UserInfo user, boolean evenWhenDisallowed) {
        mockRemoveUser(user, evenWhenDisallowed, UserManager.REMOVE_RESULT_REMOVED);
    }

    private void mockRemoveUser(@NonNull UserInfo user, boolean evenWhenDisallowed,
            @RemoveResult int result) {
        mockUmRemoveUserOrSetEphemeral(mMockedUserManager, user, evenWhenDisallowed, result,
                (u) -> mCarUserService.onUserRemoved(u));
    }

    private void mockRemoveUserNoCallback(@NonNull UserInfo user, @RemoveResult int result) {
        mockRemoveUserNoCallback(user, /* evenWhenDisallowed= */ false, result);
    }

    private void mockRemoveUserNoCallback(@NonNull UserInfo user, boolean evenWhenDisallowed,
            @RemoveResult int result) {
        mockUmRemoveUserOrSetEphemeral(mMockedUserManager, user, evenWhenDisallowed, result,
                /* listener= */ null);
    }

    private void mockStopUserWithDelayedLocking(@UserIdInt int userId, int result)
            throws Exception {
        when(mMockedIActivityManager.stopUserWithDelayedLocking(userId, true, null))
                .thenReturn(result);
    }

    private void mockStopUserWithDelayedLockingThrowsRemoteException(@UserIdInt int userId)
            throws Exception {
        when(mMockedIActivityManager.stopUserWithDelayedLocking(userId, true, null))
                .thenThrow(new RemoteException());
    }

    private void mockHalGetInitialInfo(@UserIdInt int currentUserId,
            @NonNull InitialUserInfoResponse response) {
        UsersInfo usersInfo = newUsersInfo(currentUserId);
        doAnswer((invocation) -> {
            Log.d(TAG, "Answering " + invocation + " with " + response);
            @SuppressWarnings("unchecked")
            HalCallback<InitialUserInfoResponse> callback =
                    (HalCallback<InitialUserInfoResponse>) invocation.getArguments()[3];
            callback.onResponse(HalCallback.STATUS_OK, response);
            return null;
        }).when(mUserHal).getInitialUserInfo(anyInt(), eq(mAsyncCallTimeoutMs),
                eq(usersInfo), notNull());
    }

    private void mockIsHeadlessSystemUser(@UserIdInt int userId, boolean mode) {
        doReturn(mode).when(() -> UserHelperLite.isHeadlessSystemUser(userId));
    }

    private void mockHalSwitch(@UserIdInt int currentUserId, @NonNull UserInfo androidTargetUser,
            @Nullable SwitchUserResponse response) {
        mockHalSwitch(currentUserId, HalCallback.STATUS_OK, response, androidTargetUser);
    }

    @NonNull
    private ArgumentCaptor<CreateUserRequest> mockHalCreateUser(
            @HalCallbackStatus int callbackStatus, int responseStatus) {
        CreateUserResponse response = new CreateUserResponse();
        response.status = responseStatus;
        ArgumentCaptor<CreateUserRequest> captor = ArgumentCaptor.forClass(CreateUserRequest.class);
        doAnswer((invocation) -> {
            Log.d(TAG, "Answering " + invocation + " with " + response);
            @SuppressWarnings("unchecked")
            HalCallback<CreateUserResponse> callback =
                    (HalCallback<CreateUserResponse>) invocation.getArguments()[2];
            callback.onResponse(callbackStatus, response);
            return null;
        }).when(mUserHal).createUser(captor.capture(), eq(mAsyncCallTimeoutMs), notNull());
        return captor;
    }

    private void mockHalCreateUserThrowsRuntimeException() {
        doThrow(new RuntimeException("D'OH!"))
                .when(mUserHal).createUser(any(), eq(mAsyncCallTimeoutMs), notNull());
    }

    private void mockCallerUid(int uid, boolean returnCorrectUid) throws Exception {
        String packageName = "packageName";
        String className = "className";
        when(mMockedResources.getString(anyInt())).thenReturn(packageName + "/" + className);
        when(mMockContext.createContextAsUser(any(), anyInt())).thenReturn(mMockContext);
        when(mMockContext.getPackageManager()).thenReturn(mPackageManager);

        if (returnCorrectUid) {
            when(mPackageManager.getPackageUid(any(), anyInt())).thenReturn(uid);
        } else {
            when(mPackageManager.getPackageUid(any(), anyInt())).thenReturn(uid + 1);
        }
    }

    private BlockingAnswer<Void> mockHalSwitchLateResponse(@UserIdInt int currentUserId,
            @NonNull UserInfo androidTargetUser, @Nullable SwitchUserResponse response) {
        android.hardware.automotive.vehicle.V2_0.UserInfo halTargetUser =
                new android.hardware.automotive.vehicle.V2_0.UserInfo();
        halTargetUser.userId = androidTargetUser.id;
        halTargetUser.flags = UserHalHelper.convertFlags(androidTargetUser);
        UsersInfo usersInfo = newUsersInfo(currentUserId);
        SwitchUserRequest request = new SwitchUserRequest();
        request.targetUser = halTargetUser;
        request.usersInfo = usersInfo;

        BlockingAnswer<Void> blockingAnswer = BlockingAnswer.forVoidReturn(10_000, (invocation) -> {
            Log.d(TAG, "Answering " + invocation + " with " + response);
            @SuppressWarnings("unchecked")
            HalCallback<SwitchUserResponse> callback = (HalCallback<SwitchUserResponse>) invocation
                    .getArguments()[2];
            callback.onResponse(HalCallback.STATUS_OK, response);
        });
        doAnswer(blockingAnswer).when(mUserHal).switchUser(eq(request), eq(mAsyncCallTimeoutMs),
                notNull());
        return blockingAnswer;
    }

    private void mockHalSwitch(@UserIdInt int currentUserId,
            @HalCallback.HalCallbackStatus int callbackStatus,
            @Nullable SwitchUserResponse response, @NonNull UserInfo androidTargetUser) {
        android.hardware.automotive.vehicle.V2_0.UserInfo halTargetUser =
                new android.hardware.automotive.vehicle.V2_0.UserInfo();
        halTargetUser.userId = androidTargetUser.id;
        halTargetUser.flags = UserHalHelper.convertFlags(androidTargetUser);
        UsersInfo usersInfo = newUsersInfo(currentUserId);
        SwitchUserRequest request = new SwitchUserRequest();
        request.targetUser = halTargetUser;
        request.usersInfo = usersInfo;

        doAnswer((invocation) -> {
            Log.d(TAG, "Answering " + invocation + " with " + response);
            @SuppressWarnings("unchecked")
            HalCallback<SwitchUserResponse> callback =
                    (HalCallback<SwitchUserResponse>) invocation.getArguments()[2];
            callback.onResponse(callbackStatus, response);
            return null;
        }).when(mUserHal).switchUser(eq(request), eq(mAsyncCallTimeoutMs), notNull());
    }

    private void mockHalGetUserIdentificationAssociation(@NonNull UserInfo user,
            @NonNull int[] types, @NonNull int[] values,  @Nullable String errorMessage) {
        assertWithMessage("mismatch on number of types and values").that(types.length)
                .isEqualTo(values.length);

        UserIdentificationResponse response = new UserIdentificationResponse();
        response.numberAssociation = types.length;
        response.errorMessage = errorMessage;
        for (int i = 0; i < types.length; i++) {
            UserIdentificationAssociation association = new UserIdentificationAssociation();
            association.type = types[i];
            association.value = values[i];
            response.associations.add(association);
        }

        when(mUserHal.getUserAssociation(isUserIdentificationGetRequest(user, types)))
                .thenReturn(response);
    }

    private void mockHalSetUserIdentificationAssociationSuccess(@NonNull UserInfo user,
            @NonNull int[] types, @NonNull int[] values,  @Nullable String errorMessage) {
        assertWithMessage("mismatch on number of types and values").that(types.length)
                .isEqualTo(values.length);

        UserIdentificationResponse response = new UserIdentificationResponse();
        response.numberAssociation = types.length;
        response.errorMessage = errorMessage;
        for (int i = 0; i < types.length; i++) {
            UserIdentificationAssociation association = new UserIdentificationAssociation();
            association.type = types[i];
            association.value = values[i];
            response.associations.add(association);
        }

        doAnswer((invocation) -> {
            Log.d(TAG, "Answering " + invocation + " with " + response);
            @SuppressWarnings("unchecked")
            UserIdentificationSetRequest request =
                    (UserIdentificationSetRequest) invocation.getArguments()[1];
            assertWithMessage("Wrong user on %s", request)
                    .that(request.userInfo.userId)
                    .isEqualTo(user.id);
            assertWithMessage("Wrong flags on %s", request)
                    .that(UserHalHelper.toUserInfoFlags(request.userInfo.flags))
                    .isEqualTo(user.flags);
            @SuppressWarnings("unchecked")
            HalCallback<UserIdentificationResponse> callback =
                    (HalCallback<UserIdentificationResponse>) invocation.getArguments()[2];
            callback.onResponse(HalCallback.STATUS_OK, response);
            return null;
        }).when(mUserHal).setUserAssociation(eq(mAsyncCallTimeoutMs), notNull(), notNull());
    }

    private void mockHalSetUserIdentificationAssociationFailure(@NonNull String errorMessage) {
        UserIdentificationResponse response = new UserIdentificationResponse();
        response.errorMessage = errorMessage;
        doAnswer((invocation) -> {
            Log.d(TAG, "Answering " + invocation + " with " + response);
            @SuppressWarnings("unchecked")
            HalCallback<UserIdentificationResponse> callback =
                    (HalCallback<UserIdentificationResponse>) invocation.getArguments()[2];
            callback.onResponse(HalCallback.STATUS_WRONG_HAL_RESPONSE, response);
            return null;
        }).when(mUserHal).setUserAssociation(eq(mAsyncCallTimeoutMs), notNull(), notNull());
    }

    private void mockInteractAcrossUsersPermission(boolean granted) {
        int result = granted ? android.content.pm.PackageManager.PERMISSION_GRANTED
                : android.content.pm.PackageManager.PERMISSION_DENIED;

        doReturn(result).when(() -> ActivityManager.checkComponentPermission(
                eq(android.Manifest.permission.INTERACT_ACROSS_USERS),
                anyInt(), anyInt(), eq(true)));
        doReturn(result).when(() -> ActivityManager.checkComponentPermission(
                eq(android.Manifest.permission.INTERACT_ACROSS_USERS_FULL),
                anyInt(), anyInt(), eq(true)));
    }

    private void mockManageUsersPermission(String permission, boolean granted) {
        int result = granted ? android.content.pm.PackageManager.PERMISSION_GRANTED
                : android.content.pm.PackageManager.PERMISSION_DENIED;

        doReturn(result).when(() -> ActivityManager.checkComponentPermission(eq(permission),
                anyInt(), anyInt(), eq(true)));
    }

    private void mockUserHalSupported(boolean result) {
        when(mUserHal.isSupported()).thenReturn(result);
    }

    private void mockUserHalUserAssociationSupported(boolean result) {
        when(mUserHal.isUserAssociationSupported()).thenReturn(result);
    }

    private CarUxRestrictions getUxRestrictions(boolean restricted) {
        int restrictions = CarUxRestrictions.UX_RESTRICTIONS_BASELINE;
        if (restricted) {
            restrictions |= CarUxRestrictions.UX_RESTRICTIONS_NO_SETUP;
        }
        return new CarUxRestrictions.Builder(/* reqOpt= */ false, restrictions,
                System.currentTimeMillis()).build();
    }

    private void updateUxRestrictions(ICarUxRestrictionsChangeListener listener, boolean restricted)
            throws RemoteException {
        CarUxRestrictions restrictions = getUxRestrictions(restricted);
        Log.v(TAG, "updateUxRestrictions(" + restricted + "): sending UX restrictions ("
                + restrictions + ") to " + listener);
        listener.onUxRestrictionsChanged(restrictions);
    }


    private void mockGetUxRestrictions(boolean restricted) {
        CarUxRestrictions restrictions = getUxRestrictions(restricted);
        Log.v(TAG, "mockUxRestrictions(" + restricted + ") mocking getCurrentUxRestrictions() to "
                + "return " + restrictions);
        when(mCarUxRestrictionService.getCurrentUxRestrictions()).thenReturn(restrictions);
    }

    /**
     * Asserts a {@link UsersInfo} that was created based on {@link #mockCurrentUsers(UserInfo)}.
     */
    private void assertDefaultUsersInfo(UsersInfo actual, UserInfo currentUser) {
        // TODO(b/150413515): figure out why this method is not called in other places
        assertThat(actual).isNotNull();
        assertSameUser(actual.currentUser, currentUser);
        assertThat(actual.numberUsers).isEqualTo(mExistingUsers.size());
        for (int i = 0; i < actual.numberUsers; i++) {
            assertSameUser(actual.existingUsers.get(i), mExistingUsers.get(i));
        }
    }

    private void assertSameUser(android.hardware.automotive.vehicle.V2_0.UserInfo halUser,
            UserInfo androidUser) {
        assertThat(halUser.userId).isEqualTo(androidUser.id);
        assertWithMessage("flags mismatch: hal=%s, android=%s",
                UserInfo.flagsToString(androidUser.flags),
                UserHalHelper.userFlagsToString(halUser.flags))
            .that(halUser.flags).isEqualTo(UserHalHelper.convertFlags(androidUser));
    }

    private void verifyUserRemoved(@UserIdInt int userId) {
        verify(mMockedUserManager).removeUser(userId);
    }

    private void verifyNoUserRemoved() {
        verify(mMockedUserManager, never()).removeUserOrSetEphemeral(anyInt(), anyBoolean());
        verify(mMockedUserManager, never()).removeUser(anyInt());
    }

    private void verifyAnyUserSwitch() throws Exception {
        verify(mMockedIActivityManager).switchUser(anyInt());
    }

    private void verifyNoUserSwitch() throws Exception {
        verify(mMockedIActivityManager, never()).switchUser(anyInt());
    }

    @NonNull
    private UsersInfo newUsersInfo(@UserIdInt int currentUserId) {
        UsersInfo infos = new UsersInfo();
        infos.numberUsers = mExistingUsers.size();
        boolean foundCurrentUser = false;
        for (UserInfo info : mExistingUsers) {
            android.hardware.automotive.vehicle.V2_0.UserInfo existingUser =
                    new android.hardware.automotive.vehicle.V2_0.UserInfo();
            int flags = UserFlags.NONE;
            if (info.id == UserHandle.USER_SYSTEM) {
                flags |= UserFlags.SYSTEM;
            }
            if (info.isAdmin()) {
                flags |= UserFlags.ADMIN;
            }
            if (info.isGuest()) {
                flags |= UserFlags.GUEST;
            }
            if (info.isEphemeral()) {
                flags |= UserFlags.EPHEMERAL;
            }
            existingUser.userId = info.id;
            existingUser.flags = flags;
            if (info.id == currentUserId) {
                foundCurrentUser = true;
                infos.currentUser.userId = info.id;
                infos.currentUser.flags = flags;
            }
            infos.existingUsers.add(existingUser);
        }
        Preconditions.checkArgument(foundCurrentUser,
                "no user with id " + currentUserId + " on " + mExistingUsers);
        return infos;
    }

    private void assertNoPostSwitch() {
        verify(mUserHal, never()).postSwitchResponse(any());
    }

    private void assertPostSwitch(int requestId, int currentId, int targetId) {
        verify(mUserHal).postSwitchResponse(isSwitchUserRequest(requestId, currentId, targetId));
    }

    private void assertHalSwitch(int currentId, int targetId) {
        verify(mUserHal).switchUser(isSwitchUserRequest(0, currentId, targetId),
                eq(mAsyncCallTimeoutMs), any());
    }

    private void assertHalSwitchAnyUser() {
        verify(mUserHal).switchUser(any(), eq(mAsyncCallTimeoutMs), any());
    }

    private void assertNoHalUserSwitch() {
        verify(mUserHal, never()).switchUser(any(), anyInt(), any());
    }

    private void assertNoHalUserCreation() {
        verify(mUserHal, never()).createUser(any(), anyInt(), any());
    }

    private void assertNoHalUserRemoval() {
        verify(mUserHal, never()).removeUser(any());
    }

    private void assertHalRemove(@NonNull UserInfo currentUser, @NonNull UserInfo removeUser) {
        assertHalRemove(currentUser, removeUser, /* evenWhenDisallowed= */ false);
    }

    private void assertHalRemove(@NonNull UserInfo currentUser, @NonNull UserInfo removeUser,
            boolean evenWhenDisallowed) {
        verify(mMockedUserManager).removeUserOrSetEphemeral(removeUser.id, evenWhenDisallowed);
        ArgumentCaptor<RemoveUserRequest> requestCaptor =
                ArgumentCaptor.forClass(RemoveUserRequest.class);
        verify(mUserHal).removeUser(requestCaptor.capture());
        RemoveUserRequest request = requestCaptor.getValue();
        assertThat(request.removedUserInfo.userId).isEqualTo(removeUser.id);
        assertThat(request.removedUserInfo.flags).isEqualTo(UserHalHelper.convertFlags(removeUser));
        assertThat(request.usersInfo.currentUser.userId).isEqualTo(currentUser.id);
    }

    private void assertUserRemovalResultStatus(UserRemovalResult result,
            @UserRemovalResult.Status int expectedStatus) {
        int actualStatus = result.getStatus();
        if (actualStatus != expectedStatus) {
            fail(String.format("wrong UserRemovalResult: expected %s, got %s",
                    UserRemovalResult.statusToString(expectedStatus),
                    UserRemovalResult.statusToString(actualStatus)));
        }
    }

    @NonNull
    private static SwitchUserRequest isSwitchUserRequest(int requestId,
            @UserIdInt int currentUserId, @UserIdInt int targetUserId) {
        return argThat(new SwitchUserRequestMatcher(requestId, currentUserId, targetUserId));
    }

    static final class FakeCarOccupantZoneService {
        private final SparseArray<Integer> mZoneUserMap = new SparseArray<Integer>();
        private final CarUserService.ZoneUserBindingHelper mZoneUserBindigHelper =
                new CarUserService.ZoneUserBindingHelper() {
                    @Override
                    @NonNull
                    public List<OccupantZoneInfo> getOccupantZones(
                            @OccupantTypeEnum int occupantType) {
                        return null;
                    }

                    @Override
                    public boolean assignUserToOccupantZone(@UserIdInt int userId, int zoneId) {
                        if (mZoneUserMap.get(zoneId) != null) {
                            return false;
                        }
                        mZoneUserMap.put(zoneId, userId);
                        return true;
                    }

                    @Override
                    public boolean unassignUserFromOccupantZone(@UserIdInt int userId) {
                        for (int index = 0; index < mZoneUserMap.size(); index++) {
                            if (mZoneUserMap.valueAt(index) == userId) {
                                mZoneUserMap.removeAt(index);
                                break;
                            }
                        }
                        return true;
                    }

                    @Override
                    public boolean isPassengerDisplayAvailable() {
                        return true;
                    }
                };

        FakeCarOccupantZoneService(CarUserService carUserService) {
            carUserService.setZoneUserBindingHelper(mZoneUserBindigHelper);
        }
    }

    private void sendUserLifecycleEvent(@UserIdInt int fromUserId, @UserIdInt int toUserId,
            @UserLifecycleEventType int eventType) {
        mCarUserService.onUserLifecycleEvent(eventType, fromUserId,
                toUserId);
    }

    private void sendUserUnlockedEvent(@UserIdInt int userId) {
        sendUserLifecycleEvent(/* fromUser */ 0, userId,
                CarUserManager.USER_LIFECYCLE_EVENT_TYPE_UNLOCKED);
    }

    private void sendUserSwitchingEvent(@UserIdInt int fromUserId, @UserIdInt int toUserId) {
        sendUserLifecycleEvent(fromUserId, toUserId,
                CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING);
    }

    @NonNull
    private static UserIdentificationGetRequest isUserIdentificationGetRequest(
            @NonNull UserInfo user, @NonNull int[] types) {
        return argThat(new UserIdentificationGetRequestMatcher(user, types));
    }

    private static class UserIdentificationGetRequestMatcher implements
            ArgumentMatcher<UserIdentificationGetRequest> {

        private static final String MY_TAG =
                UserIdentificationGetRequestMatcher.class.getSimpleName();

        private final @UserIdInt int mUserId;
        private final int mHalFlags;
        private final @NonNull int[] mTypes;

        private UserIdentificationGetRequestMatcher(@NonNull UserInfo user, @NonNull int[] types) {
            mUserId = user.id;
            mHalFlags = UserHalHelper.convertFlags(user);
            mTypes = types;
        }

        @Override
        public boolean matches(UserIdentificationGetRequest argument) {
            if (argument == null) {
                Log.w(MY_TAG, "null argument");
                return false;
            }
            if (argument.userInfo.userId != mUserId) {
                Log.w(MY_TAG, "wrong user id on " + argument + "; expected " + mUserId);
                return false;
            }
            if (argument.userInfo.flags != mHalFlags) {
                Log.w(MY_TAG, "wrong flags on " + argument + "; expected " + mHalFlags);
                return false;
            }
            if (argument.numberAssociationTypes != mTypes.length) {
                Log.w(MY_TAG, "wrong numberAssociationTypes on " + argument + "; expected "
                        + mTypes.length);
                return false;
            }
            if (argument.associationTypes.size() != mTypes.length) {
                Log.w(MY_TAG, "wrong associationTypes size on " + argument + "; expected "
                        + mTypes.length);
                return false;
            }
            for (int i = 0; i < mTypes.length; i++) {
                if (argument.associationTypes.get(i) != mTypes[i]) {
                    Log.w(MY_TAG, "wrong association type on index " + i + " on " + argument
                            + "; expected types: " + Arrays.toString(mTypes));
                    return false;
                }
            }
            Log.d(MY_TAG, "Good News, Everyone! " + argument + " matches " + this);
            return true;
        }

        @Override
        public String toString() {
            return "isUserIdentificationGetRequest(userId=" + mUserId + ", flags="
                    + UserHalHelper.userFlagsToString(mHalFlags) + ", types="
                    + Arrays.toString(mTypes) + ")";
        }
    }

    private static final class SwitchUserRequestMatcher
            implements ArgumentMatcher<SwitchUserRequest> {
        private static final String MY_TAG = UsersInfo.class.getSimpleName();

        private final int mRequestId;
        private final @UserIdInt int mCurrentUserId;
        private final @UserIdInt int mTargetUserId;


        private SwitchUserRequestMatcher(int requestId, @UserIdInt int currentUserId,
                @UserIdInt int targetUserId) {
            mCurrentUserId = currentUserId;
            mTargetUserId = targetUserId;
            mRequestId = requestId;
        }

        @Override
        public boolean matches(SwitchUserRequest argument) {
            if (argument == null) {
                Log.w(MY_TAG, "null argument");
                return false;
            }
            if (argument.usersInfo.currentUser.userId != mCurrentUserId) {
                Log.w(MY_TAG,
                        "wrong current user id on " + argument + "; expected " + mCurrentUserId);
                return false;
            }

            if (argument.targetUser.userId != mTargetUserId) {
                Log.w(MY_TAG,
                        "wrong target user id on " + argument + "; expected " + mTargetUserId);
                return false;
            }

            if (argument.requestId != mRequestId) {
                Log.w(MY_TAG,
                        "wrong request Id on " + argument + "; expected " + mTargetUserId);
                return false;
            }

            Log.d(MY_TAG, "Good News, Everyone! " + argument + " matches " + this);
            return true;
        }
    }
}
