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

package android.car.cts.permissionreadcardisplayunits;

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

import com.google.common.collect.ImmutableList;

import org.junit.Test;
import org.junit.Before;
import org.junit.runner.RunWith;

@RequiresDevice
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "Instant apps cannot get car related permissions.")
public final class PermissionReadCarDisplayUnitsTest {
    private static final ImmutableList<Integer> PERMISSION_READ_CAR_DISPLAY_UNITS_PROPERTIES =
            ImmutableList.<Integer>builder().add(
                    VehiclePropertyIds.DISTANCE_DISPLAY_UNITS,
                    VehiclePropertyIds.FUEL_VOLUME_DISPLAY_UNITS,
                    VehiclePropertyIds.TIRE_PRESSURE_DISPLAY_UNITS,
                    VehiclePropertyIds.EV_BATTERY_DISPLAY_UNITS,
                    /*VehiclePropertyIds.VEHICLE_SPEED_DISPLAY_UNITS=*/ 289408516,
                    VehiclePropertyIds.FUEL_CONSUMPTION_UNITS_DISTANCE_OVER_VOLUME)
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
    public void testPermissionReadCarDisplayUnitsGranted() {
        for (CarPropertyConfig<?> carPropertyConfig : mCarPropertyManager.getPropertyList()) {
            assertWithMessage("%s",
                    VehiclePropertyIds.toString(carPropertyConfig.getPropertyId())).that(
                    carPropertyConfig.getPropertyId()).isIn(
                    PERMISSION_READ_CAR_DISPLAY_UNITS_PROPERTIES);
        }
    }
}