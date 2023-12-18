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

package com.android.car.telemetry;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.expectThrows;

import android.car.Car;
import android.car.telemetry.CarTelemetryManager;
import android.car.telemetry.ManifestKey;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Objects;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * This class contains security permission tests for {@link CarTelemetryManager}'s system APIs.
 */
@RunWith(AndroidJUnit4.class)
public class CarTelemetryManagerPermissionTest {
    private final Context mContext =
            InstrumentationRegistry.getInstrumentation().getTargetContext();
    private final ManifestKey mManifestKey = new ManifestKey("name", 1);
    private final byte[] mManifestBytes = "manifest".getBytes();

    private Car mCar;
    private CarTelemetryManager mCarTelemetryManager;

    @Before
    public void setUp() {
        mCar = Objects.requireNonNull(Car.createCar(mContext));
        mCarTelemetryManager = (CarTelemetryManager) mCar.getCarManager(Car.CAR_TELEMETRY_SERVICE);
    }

    @After
    public void tearDown() {
        mCar.disconnect();
    }

    @Test
    public void testSetListener() throws Exception {
        Executor executor = Executors.newSingleThreadExecutor();
        CarTelemetryManager.CarTelemetryResultsListener listener =
                new FakeCarTelemetryResultsListener();

        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.setListener(executor, listener));

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testClearListener() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.clearListener());

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testAddManifest() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.addManifest(mManifestKey, mManifestBytes));

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testRemoveManifest() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.removeManifest(mManifestKey));

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testRemoveAllManifests() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.removeAllManifests());

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testSendFinishedReports() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.sendFinishedReports(mManifestKey));

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testSendAllFinishedReports() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.sendAllFinishedReports());

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    @Test
    public void testSendScriptExecutionErrors() throws Exception {
        Exception e = expectThrows(SecurityException.class,
                () -> mCarTelemetryManager.sendScriptExecutionErrors());

        assertThat(e.getMessage()).contains(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE);
    }

    private class FakeCarTelemetryResultsListener implements
            CarTelemetryManager.CarTelemetryResultsListener {
        @Override
        public void onResult(@NonNull ManifestKey key, @NonNull byte[] result) {
        }

        @Override
        public void onError(@NonNull byte[] error) {
        }
    }

}
