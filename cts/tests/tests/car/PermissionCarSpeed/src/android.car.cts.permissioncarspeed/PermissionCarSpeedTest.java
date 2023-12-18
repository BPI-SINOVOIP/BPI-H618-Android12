/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.car.cts.permissioncarspeed;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.car.Car;
import android.car.VehiclePropertyIds;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.property.CarPropertyManager;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.RequiresDevice;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.CddTest;

import com.google.common.collect.ImmutableList;

import org.junit.Test;
import org.junit.Before;
import org.junit.runner.RunWith;

@RequiresDevice
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "Instant apps cannot get car related permissions.")
public final class PermissionCarSpeedTest {
    private static final ImmutableList<Integer> PERMISSION_CAR_SPEED_PROPERTIES =
            ImmutableList.<Integer>builder().add(
                    VehiclePropertyIds.PERF_VEHICLE_SPEED,
                    VehiclePropertyIds.PERF_VEHICLE_SPEED_DISPLAY, VehiclePropertyIds.WHEEL_TICK)
                    .build();

    private CarPropertyManager mCarPropertyManager;

    @Before
    public void setUp() {
        mCarPropertyManager =
                (CarPropertyManager) Car.createCar(
                        InstrumentationRegistry.getInstrumentation().getContext()).getCarManager(
                        Car.PROPERTY_SERVICE);
        assertThat(mCarPropertyManager).isNotNull();
    }

    @Test
    public void testPermissionCarSpeedGranted() {
        for (CarPropertyConfig<?> carPropertyConfig : mCarPropertyManager.getPropertyList()) {
            assertWithMessage("%s", VehiclePropertyIds.toString(carPropertyConfig.getPropertyId()))
                    .that(carPropertyConfig.getPropertyId()).isIn(PERMISSION_CAR_SPEED_PROPERTIES);
        }
    }

    @CddTest(requirement = "2.5.1")
    @Test
    public void testRequiredPropertyAvailableWithPermissionCarSpeedGranted() {
        assertWithMessage("%s", VehiclePropertyIds.toString(VehiclePropertyIds.PERF_VEHICLE_SPEED))
                .that(mCarPropertyManager.getCarPropertyConfig(
                        VehiclePropertyIds.PERF_VEHICLE_SPEED)).isNotNull();
    }
}