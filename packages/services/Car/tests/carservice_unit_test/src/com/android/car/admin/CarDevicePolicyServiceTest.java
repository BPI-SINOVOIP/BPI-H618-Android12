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
package com.android.car.admin;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;

import android.annotation.NonNull;
import android.car.admin.CarDevicePolicyManager;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.car.user.UserCreationResult;
import android.car.user.UserRemovalResult;
import android.car.user.UserStartResult;
import android.car.user.UserStopResult;
import android.content.pm.UserInfo;
import android.content.pm.UserInfo.UserInfoFlag;
import android.os.UserManager;

import com.android.car.user.CarUserService;
import com.android.internal.infra.AndroidFuture;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

public final class CarDevicePolicyServiceTest extends AbstractExtendedMockitoTestCase {

    @Mock
    private CarUserService mCarUserService;

    private CarDevicePolicyService mService;

    private AndroidFuture<UserRemovalResult> mUserRemovalResult = new AndroidFuture<>();

    private AndroidFuture<UserCreationResult> mUserCreationResult = new AndroidFuture<>();

    private AndroidFuture<UserStartResult> mUserStartResult = new AndroidFuture<>();

    private AndroidFuture<UserStopResult> mUserStopResult = new AndroidFuture<>();

    @Before
    public void setFixtures() {
        mService = new CarDevicePolicyService(mCarUserService);
    }

    @Test
    public void testRemoveUser() {
        mService.removeUser(42, mUserRemovalResult);

        verify(mCarUserService).removeUser(eq(42), /* hasCallerRestrictions= */ eq(true),
                eq(mUserRemovalResult));
    }

    @Test
    public void testCreateUser_failure_invalidTypes() throws Exception {
        invalidCreateUserTypeTest(CarDevicePolicyManager.FIRST_USER_TYPE - 1);
        invalidCreateUserTypeTest(CarDevicePolicyManager.LAST_USER_TYPE + 1);
    }

    private void invalidCreateUserTypeTest(@CarDevicePolicyManager.UserType int type)
            throws Exception {
        mService.createUser("name", type, mUserCreationResult);
        UserCreationResult result = mUserCreationResult.get();
        assertThat(result).isNotNull();
        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStatus()).isEqualTo(UserCreationResult.STATUS_INVALID_REQUEST);
        assertThat(result.getUser()).isNull();
    }

    @Test
    public void testCreateUser_ok_normalUser() {
        createUserOkTest(/* userInfoFlags=*/ 0, CarDevicePolicyManager.USER_TYPE_REGULAR,
                UserManager.USER_TYPE_FULL_SECONDARY);
    }

    @Test
    public void testCreateUser_ok_admin() {
        createUserOkTest(UserInfo.FLAG_ADMIN, CarDevicePolicyManager.USER_TYPE_ADMIN,
                UserManager.USER_TYPE_FULL_SECONDARY);
    }

    @Test
    public void testCreateUser_ok_guest() {
        createUserOkTest(/* userInfoFlags=*/ 0, CarDevicePolicyManager.USER_TYPE_GUEST,
                UserManager.USER_TYPE_FULL_GUEST);
    }

    private void createUserOkTest(@UserInfoFlag int flags,
            @CarDevicePolicyManager.UserType int carDpmUserType, @NonNull String userType) {
        mService.createUser("name", carDpmUserType, mUserCreationResult);

        verify(mCarUserService).createUser(eq("name"), eq(userType), eq(flags),
                /* timeoutMs= */ anyInt(), eq(mUserCreationResult));
    }

    @Test
    public void testStartUserInBackground() {
        mService.startUserInBackground(42, mUserStartResult);

        verify(mCarUserService).startUserInBackground(42, mUserStartResult);
    }

    @Test
    public void testStopUser() {
        mService.stopUser(42, mUserStopResult);

        verify(mCarUserService).stopUser(42, mUserStopResult);
    }
}
