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

package android.car.cts.permissioncarinfo;

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
public final class PermissionCarInfoTest {
    private static final ImmutableList<Integer> PERMISSION_CAR_INFO_PROPERTIES =
            ImmutableList.<Integer>builder().add(
                    VehiclePropertyIds.INFO_MAKE, VehiclePropertyIds.INFO_MODEL,
                    VehiclePropertyIds.INFO_MODEL_YEAR, VehiclePropertyIds.INFO_FUEL_CAPACITY,
                    VehiclePropertyIds.INFO_FUEL_TYPE, VehiclePropertyIds.INFO_EV_BATTERY_CAPACITY,
                    VehiclePropertyIds.INFO_EV_CONNECTOR_TYPE,
                    VehiclePropertyIds.INFO_FUEL_DOOR_LOCATION,
                    VehiclePropertyIds.INFO_MULTI_EV_PORT_LOCATIONS,
                    VehiclePropertyIds.INFO_EV_PORT_LOCATION, VehiclePropertyIds.INFO_DRIVER_SEAT,
                    VehiclePropertyIds.INFO_EXTERIOR_DIMENSIONS,
                    VehiclePropertyIds.ELECTRONIC_TOLL_COLLECTION_CARD_TYPE,
                    VehiclePropertyIds.ELECTRONIC_TOLL_COLLECTION_CARD_STATUS)
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
    public void testPermissionCarInfoGranted() {
        for (CarPropertyConfig<?> carPropertyConfig : mCarPropertyManager.getPropertyList()) {
            assertWithMessage("%s",
                    VehiclePropertyIds.toString(carPropertyConfig.getPropertyId())).that(
                    carPropertyConfig.getPropertyId()).isIn(PERMISSION_CAR_INFO_PROPERTIES);
        }
    }
}
