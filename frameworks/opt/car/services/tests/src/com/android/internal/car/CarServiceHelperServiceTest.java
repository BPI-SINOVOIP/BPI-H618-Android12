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

package com.android.internal.car;

import static com.android.car.internal.common.CommonConstants.CAR_SERVICE_INTERFACE;
import static com.android.car.internal.common.CommonConstants.USER_LIFECYCLE_EVENT_TYPE_STARTING;
import static com.android.car.internal.common.CommonConstants.USER_LIFECYCLE_EVENT_TYPE_STOPPED;
import static com.android.car.internal.common.CommonConstants.USER_LIFECYCLE_EVENT_TYPE_STOPPING;
import static com.android.car.internal.common.CommonConstants.USER_LIFECYCLE_EVENT_TYPE_SWITCHING;
import static com.android.car.internal.common.CommonConstants.USER_LIFECYCLE_EVENT_TYPE_UNLOCKING;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doNothing;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.mock;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.verify;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.when;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;

import android.annotation.UserIdInt;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.car.watchdoglib.CarWatchdogDaemonHelper;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.server.SystemService;
import com.android.server.SystemService.TargetUser;
import com.android.server.wm.CarLaunchParamsModifier;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;

/**
 * This class contains unit tests for the {@link CarServiceHelperService}.
 */
@RunWith(AndroidJUnit4.class)
public class CarServiceHelperServiceTest extends AbstractExtendedMockitoTestCase {

    private static final String TAG = CarServiceHelperServiceTest.class.getSimpleName();

    private CarServiceHelperService mHelperSpy;
    private CarServiceHelperService mHelper;

    @Mock
    private Context mMockContext;
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private CarLaunchParamsModifier mCarLaunchParamsModifier;
    @Mock
    private CarWatchdogDaemonHelper mCarWatchdogDaemonHelper;
    @Mock
    private IBinder mICarBinder;
    @Mock
    private CarServiceProxy mCarServiceProxy;

    /**
     * Initialize objects and setup testing environment.
     */
    @Override
    protected void onSessionBuilder(CustomMockitoSessionBuilder session) {
        session.spyStatic(ServiceManager.class);
    }

    @Before
    public void setTestFixtures() {
        mHelper = new CarServiceHelperService(
                mMockContext,
                mCarLaunchParamsModifier,
                mCarWatchdogDaemonHelper,
                mCarServiceProxy);
        mHelperSpy = spy(mHelper);
        when(mMockContext.getPackageManager()).thenReturn(mPackageManager);
    }

    @Test
    public void testCarServiceLaunched() throws Exception {
        mockRegisterReceiver();
        mockBindService();
        mockLoadLibrary();

        mHelperSpy.onStart();

        verifyBindService();
    }

    @Test
    public void testHandleCarServiceCrash() throws Exception {
        mockHandleCarServiceCrash();
        mockCarServiceException();

        mHelperSpy.handleCarServiceConnection(mICarBinder);

        verify(mHelperSpy).handleCarServiceCrash();
    }

    @Test
    public void testOnUserStarting_notifiesICar() throws Exception {
        int userId = 10;

        mHelper.onUserStarting(newTargetUser(userId));

        verifyICarOnUserLifecycleEventCalled(USER_LIFECYCLE_EVENT_TYPE_STARTING,
                UserHandle.USER_NULL, userId);
    }

    @Test
    public void testOnUserStarting_preCreatedDoesntNotifyICar() throws Exception {
        mHelper.onUserStarting(newTargetUser(10, /* preCreated= */ true));

        verifyICarOnUserLifecycleEventNeverCalled();
    }

    @Test
    public void testOnUserSwitching_notifiesICar() throws Exception {
        int currentUserId = 10;
        int targetUserId = 11;

        mHelper.onUserSwitching(newTargetUser(currentUserId),
                newTargetUser(targetUserId));

        verifyICarOnUserLifecycleEventCalled(USER_LIFECYCLE_EVENT_TYPE_SWITCHING,
                currentUserId, targetUserId);
    }

    @Test
    public void testOnUserSwitching_preCreatedDoesntNotifyICar() throws Exception {
        mHelper.onUserSwitching(newTargetUser(10), newTargetUser(11, /* preCreated= */ true));

        verifyICarOnUserLifecycleEventNeverCalled();
    }

    @Test
    public void testOnUserUnlocking_notifiesICar() throws Exception {
        int userId = 10;

        mHelper.onUserUnlocking(newTargetUser(userId));

        verifyICarOnUserLifecycleEventCalled(USER_LIFECYCLE_EVENT_TYPE_UNLOCKING,
                UserHandle.USER_NULL, userId);
    }

    @Test
    public void testOnUserUnlocking_preCreatedDoesntNotifyICar() throws Exception {
        mHelper.onUserUnlocking(newTargetUser(10, /* preCreated= */ true));

        verifyICarOnUserLifecycleEventNeverCalled();
    }

    @Test
    public void testOnUserStopping_notifiesICar() throws Exception {
        int userId = 10;

        mHelper.onUserStopping(newTargetUser(userId));

        verifyICarOnUserLifecycleEventCalled(USER_LIFECYCLE_EVENT_TYPE_STOPPING,
                UserHandle.USER_NULL, userId);
    }

    @Test
    public void testOnUserStopping_preCreatedDoesntNotifyICar() throws Exception {
        mHelper.onUserStopping(newTargetUser(10, /* preCreated= */ true));

        verifyICarOnUserLifecycleEventNeverCalled();
    }

    @Test
    public void testOnUserStopped_notifiesICar() throws Exception {
        int userId = 10;

        mHelper.onUserStopped(newTargetUser(userId));

        verifyICarOnUserLifecycleEventCalled(USER_LIFECYCLE_EVENT_TYPE_STOPPED,
                UserHandle.USER_NULL, userId);
    }

    @Test
    public void testOnUserStopped_preCreatedDoesntNotifyICar() throws Exception {
        mHelper.onUserStopped(newTargetUser(10, /* preCreated= */ true));

        verifyICarOnUserLifecycleEventNeverCalled();
    }

    @Test
    public void testOnBootPhase_thirdPartyCanStart_initBootUser() throws Exception {
        mHelper.onBootPhase(SystemService.PHASE_THIRD_PARTY_APPS_CAN_START);

        verifyInitBootUser();
    }

    private TargetUser newTargetUser(int userId) {
        return newTargetUser(userId, /* preCreated= */ false);
    }

    private TargetUser newTargetUser(int userId, boolean preCreated) {
        TargetUser targetUser = mock(TargetUser.class);
        when(targetUser.getUserIdentifier()).thenReturn(userId);
        when(targetUser.isPreCreated()).thenReturn(preCreated);
        return targetUser;
    }

    private void verifyBindService() throws Exception {
        verify(mMockContext).bindServiceAsUser(
                argThat(intent -> intent.getAction().equals(CAR_SERVICE_INTERFACE)),
                any(), eq(Context.BIND_AUTO_CREATE), any(), eq(UserHandle.SYSTEM));
    }

    private void mockRegisterReceiver() {
        when(mMockContext.registerReceiverForAllUsers(any(), any(), any(), any()))
                .thenReturn(new Intent());
    }

    private void mockBindService() {
        when(mMockContext.bindServiceAsUser(any(), any(),
                eq(Context.BIND_AUTO_CREATE), any(), eq(UserHandle.SYSTEM)))
                .thenReturn(true);
    }

    private void mockLoadLibrary() {
        doNothing().when(mHelperSpy).loadNativeLibrary();
    }

    private void mockCarServiceException() throws Exception {
        when(mICarBinder.transact(anyInt(), notNull(), isNull(), eq(Binder.FLAG_ONEWAY)))
                .thenThrow(new RemoteException("mock car service Crash"));
    }

    private void mockHandleCarServiceCrash() throws Exception {
        doNothing().when(mHelperSpy).handleCarServiceCrash();
    }

    enum InitialUserInfoAction {
        DEFAULT,
        DEFAULT_WITH_LOCALE,
        DO_NOT_REPLY,
        DELAYED_REPLY,
        NON_OK_RESULT_CODE,
        NULL_BUNDLE,
        SWITCH_OK,
        SWITCH_OK_WITH_LOCALE,
        SWITCH_MISSING_USER_ID
    }

    private void verifyICarOnUserLifecycleEventCalled(int eventType,
            @UserIdInt int fromId, @UserIdInt int toId) throws Exception {
        verify(mCarServiceProxy).sendUserLifecycleEvent(eq(eventType),
                isTargetUser(fromId), isTargetUser(toId));
    }

    private static TargetUser isTargetUser(@UserIdInt int userId) {
        return argThat((user) -> {
            return user == null || user.getUserIdentifier() == userId;
        });
    }

    private void verifyICarOnUserLifecycleEventNeverCalled() throws Exception {
        verify(mCarServiceProxy, never()).sendUserLifecycleEvent(anyInt(), any(), any());
    }

    private void verifyInitBootUser() throws Exception {
        verify(mCarServiceProxy).initBootUser();
    }
}
