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

package com.android.car.power;

import static android.car.Car.PERMISSION_CAR_POWER;
import static android.car.Car.PERMISSION_CONTROL_CAR_POWER_POLICY;
import static android.car.Car.PERMISSION_READ_CAR_POWER_POLICY;
import static android.car.Car.POWER_SERVICE;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.expectThrows;

import android.car.Car;
import android.car.hardware.power.CarPowerManager;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.content.Context;
import android.frameworks.automotive.powerpolicy.PowerComponent;
import android.os.Handler;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Objects;
import java.util.concurrent.Executor;

/**
 * This class contains security permission tests for the
 * {@link android.car.hardware.power#CarPowerManager}'s system APIs.
 */
@RunWith(AndroidJUnit4.class)
public final class CarPowerManagerPermissionTest {
    private final Context mContext =
            InstrumentationRegistry.getInstrumentation().getTargetContext();

    private CarPowerManager mCarPowerManager;

    @Before
    public void setUp() {
        Car car = Objects.requireNonNull(Car.createCar(mContext, (Handler) null));
        mCarPowerManager = (CarPowerManager) car.getCarManager(POWER_SERVICE);
    }

    @Test
    public void testGetPowerState() throws Exception {
        Exception e = expectThrows(SecurityException.class, () -> mCarPowerManager.getPowerState());

        assertThat(e.getMessage()).contains(PERMISSION_CAR_POWER);
    }

    @Test
    public void testRequestShutdownOnNextSuspend() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.requestShutdownOnNextSuspend());

        assertThat(e.getMessage()).contains(PERMISSION_CAR_POWER);
    }

    @Test
    public void testScheduleNextWakeupTime() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.scheduleNextWakeupTime(100));

        assertThat(e.getMessage()).contains(PERMISSION_CAR_POWER);
    }

    @Test
    public void testSetListener() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.setListener(null));

        assertThat(e.getMessage()).contains(PERMISSION_CAR_POWER);
    }

    @Test
    public void testSetListenerWithCompletion() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.setListenerWithCompletion(null));

        assertThat(e.getMessage()).contains(PERMISSION_CAR_POWER);
    }

    @Test
    public void testGetCurrentPowerPolicy() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.getCurrentPowerPolicy());

        assertThat(e.getMessage()).contains(PERMISSION_READ_CAR_POWER_POLICY);
    }

    @Test
    public void testApplyPowerPolicy() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.applyPowerPolicy("policy_id"));

        assertThat(e.getMessage()).contains(PERMISSION_CONTROL_CAR_POWER_POLICY);
    }

    @Test
    public void testAddPowerPolicyChangeListener() throws Exception {
        Executor executor = mContext.getMainExecutor();
        CarPowerPolicyFilter filter = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();
        Exception e = expectThrows(SecurityException.class,
                () -> mCarPowerManager.addPowerPolicyListener(executor, filter, (p) -> { }));

        assertThat(e.getMessage()).contains(PERMISSION_READ_CAR_POWER_POLICY);
    }
}
