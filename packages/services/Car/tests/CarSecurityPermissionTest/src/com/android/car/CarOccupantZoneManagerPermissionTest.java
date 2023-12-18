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

package com.android.car;

import static android.car.CarOccupantZoneManager.OccupantZoneInfo;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.expectThrows;

import android.car.Car;
import android.car.CarOccupantZoneManager;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

/**
 * This class contains security permission tests for {@link CarOccupantZoneManager}.
 */
@RunWith(AndroidJUnit4.class)
public class CarOccupantZoneManagerPermissionTest {

    private static final int ANY_ZONE_ID = 0;

    private Car mCar;
    private CarOccupantZoneManager mCarOccupantZoneManager;
    private OccupantZoneInfo mAnyOccupantZone;

    @Before
    public void setUp() throws Exception {
        mCar = Car.createCar(InstrumentationRegistry.getInstrumentation().getTargetContext());
        mCarOccupantZoneManager = (CarOccupantZoneManager) mCar.getCarManager(
                Car.CAR_OCCUPANT_ZONE_SERVICE);

        List<OccupantZoneInfo> occupantZones = mCarOccupantZoneManager.getAllOccupantZones();
        assertThat(occupantZones).isNotEmpty();
        mAnyOccupantZone = occupantZones.get(0);
    }

    @After
    public void tearDown() {
        mCar.disconnect();
    }

    @Test
    public void testDisplayIdForDriver() {
        SecurityException thrown = expectThrows(SecurityException.class,
                () -> mCarOccupantZoneManager.getDisplayIdForDriver(
                        CarOccupantZoneManager.DISPLAY_TYPE_MAIN));
        assertThat(thrown.getMessage()).isEqualTo(
                "requires permission " + Car.ACCESS_PRIVATE_DISPLAY_ID);
    }

    @Test
    public void testGetAudioZoneIdForOccupant() {
        SecurityException thrown = expectThrows(SecurityException.class,
                () -> mCarOccupantZoneManager.getAudioZoneIdForOccupant(mAnyOccupantZone));
        assertThat(thrown.getMessage()).isEqualTo(
                "requires permission " + Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
    }

    @Test
    public void testGetOccupantForAudioZoneId() {
        SecurityException thrown = expectThrows(SecurityException.class,
                () -> mCarOccupantZoneManager.getOccupantForAudioZoneId(/* audioZoneId = */
                        ANY_ZONE_ID));
        assertThat(thrown.getMessage()).isEqualTo(
                "requires permission " + Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
    }

    @Test
    public void testAssignProfileUserToOccupantZone() {
        SecurityException thrown = expectThrows(SecurityException.class,
                () -> mCarOccupantZoneManager.assignProfileUserToOccupantZone(
                        mAnyOccupantZone, /* audioZoneId = */ ANY_ZONE_ID));
        assertThat(thrown.getMessage()).isEqualTo(
                "requires permission " + android.Manifest.permission.MANAGE_USERS);
    }
}
