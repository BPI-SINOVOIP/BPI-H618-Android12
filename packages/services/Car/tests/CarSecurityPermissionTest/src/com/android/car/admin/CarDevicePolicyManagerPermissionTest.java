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

import static android.Manifest.permission.CREATE_USERS;
import static android.Manifest.permission.MANAGE_USERS;
import static android.car.Car.CAR_DEVICE_POLICY_SERVICE;
import static android.car.Car.createCar;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.expectThrows;

import android.app.Instrumentation;
import android.car.Car;
import android.car.admin.CarDevicePolicyManager;
import android.content.Context;
import android.os.Handler;
import android.os.UserHandle;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Objects;

/**
 * This class contains security permission tests for the {@link CarDevicePolicyManager}'s APIs.
 */
@RunWith(AndroidJUnit4.class)
public final class CarDevicePolicyManagerPermissionTest {

    private CarDevicePolicyManager mManager;
    private Context mContext;
    private Instrumentation mInstrumentation;

    @Before
    public void setUp() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mContext = mInstrumentation.getTargetContext();
        Car car = Objects.requireNonNull(createCar(mContext, (Handler) null));
        mManager = (CarDevicePolicyManager) car.getCarManager(CAR_DEVICE_POLICY_SERVICE);
    }

    @Test
    public void testRemoveUserPermission() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mManager.removeUser(UserHandle.of(100)));
        assertThat(e.getMessage()).contains(CREATE_USERS);
        assertThat(e.getMessage()).contains(MANAGE_USERS);
    }

    @Test
    public void testCreateUserPermission() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mManager.createUser("DaUser", CarDevicePolicyManager.USER_TYPE_REGULAR));
        assertThat(e.getMessage()).contains(CREATE_USERS);
        assertThat(e.getMessage()).contains(MANAGE_USERS);
    }

    @Test
    public void testStartUserInBackgroundPermission() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mManager.startUserInBackground(UserHandle.of(100)));
        assertThat(e.getMessage()).contains(CREATE_USERS);
        assertThat(e.getMessage()).contains(MANAGE_USERS);
    }

    @Test
    public void testStopUserPermission() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mManager.stopUser(UserHandle.of(100)));
        assertThat(e.getMessage()).contains(CREATE_USERS);
        assertThat(e.getMessage()).contains(MANAGE_USERS);
    }
}
