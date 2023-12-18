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

package android.car;

import static android.car.telemetry.CarTelemetryManager.ERROR_NONE;
import static android.car.telemetry.CarTelemetryManager.ERROR_SAME_MANIFEST_EXISTS;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.verify;

import android.app.Application;
import android.car.telemetry.CarTelemetryManager;
import android.car.telemetry.ManifestKey;
import android.car.testapi.CarTelemetryController;
import android.car.testapi.FakeCar;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.internal.DoNotInstrument;

import java.util.concurrent.Executor;

@RunWith(RobolectricTestRunner.class)
@DoNotInstrument
public class CarTelemetryManagerTest {
    @Rule
    public MockitoRule rule = MockitoJUnit.rule();

    private static final byte[] ERROR_BYTES = "ERROR".getBytes();
    private static final byte[] MANIFEST_BYTES = "MANIFEST".getBytes();
    private static final byte[] SCRIPT_RESULT_BYTES = "SCRIPT RESULT".getBytes();
    private static final ManifestKey DEFAULT_MANIFEST_KEY =
            new ManifestKey("NAME", 1);
    private static final Executor DIRECT_EXECUTOR = Runnable::run;

    private CarTelemetryController mCarTelemetryController;
    private CarTelemetryManager mCarTelemetryManager;

    @Mock
    private CarTelemetryManager.CarTelemetryResultsListener mListener;



    @Before
    public void setUp() {
        Application context = ApplicationProvider.getApplicationContext();
        FakeCar fakeCar = FakeCar.createFakeCar(context);
        Car carApi = fakeCar.getCar();

        mCarTelemetryManager =
                (CarTelemetryManager) carApi.getCarManager(Car.CAR_TELEMETRY_SERVICE);
        mCarTelemetryController = fakeCar.getCarTelemetryController();
    }

    @Test
    public void setListener_shouldSucceed() {
        mCarTelemetryManager.setListener(DIRECT_EXECUTOR, mListener);

        assertThat(mCarTelemetryController.isListenerSet()).isTrue();
    }

    @Test
    public void clearListener_shouldSucceed() {
        mCarTelemetryManager.setListener(DIRECT_EXECUTOR, mListener);
        mCarTelemetryManager.clearListener();

        assertThat(mCarTelemetryController.isListenerSet()).isFalse();
    }

    @Test
    public void addManifest_whenNew_shouldSucceed() {
        int result = mCarTelemetryManager.addManifest(DEFAULT_MANIFEST_KEY, MANIFEST_BYTES);

        assertThat(result).isEqualTo(ERROR_NONE);
        assertThat(mCarTelemetryController.getValidManifestsCount()).isEqualTo(1);
    }

    @Test
    public void addManifest_whenDuplicate_shouldIgnore() {
        int firstResult =
                mCarTelemetryManager.addManifest(DEFAULT_MANIFEST_KEY, MANIFEST_BYTES);
        int secondResult =
                mCarTelemetryManager.addManifest(DEFAULT_MANIFEST_KEY, MANIFEST_BYTES);

        assertThat(firstResult).isEqualTo(ERROR_NONE);
        assertThat(secondResult).isEqualTo(ERROR_SAME_MANIFEST_EXISTS);
        assertThat(mCarTelemetryController.getValidManifestsCount()).isEqualTo(1);
    }

    @Test
    public void removeManifest_whenValid_shouldSucceed() {
        mCarTelemetryManager.addManifest(DEFAULT_MANIFEST_KEY, MANIFEST_BYTES);

        boolean result = mCarTelemetryManager.removeManifest(DEFAULT_MANIFEST_KEY);

        assertThat(result).isTrue();
        assertThat(mCarTelemetryController.getValidManifestsCount()).isEqualTo(0);
    }

    @Test
    public void removeManifest_whenInvalid_shouldIgnore() {
        mCarTelemetryManager.addManifest(DEFAULT_MANIFEST_KEY, MANIFEST_BYTES);

        boolean result = mCarTelemetryManager.removeManifest(new ManifestKey("NAME", 100));

        assertThat(result).isFalse();
        assertThat(mCarTelemetryController.getValidManifestsCount()).isEqualTo(1);
    }

    @Test
    public void removeAllManifests_shouldSucceed() {
        mCarTelemetryManager.addManifest(DEFAULT_MANIFEST_KEY, MANIFEST_BYTES);
        mCarTelemetryManager.addManifest(new ManifestKey("NAME", 100), MANIFEST_BYTES);

        mCarTelemetryManager.removeAllManifests();

        assertThat(mCarTelemetryController.getValidManifestsCount()).isEqualTo(0);
    }

    @Test
    public void sendFinishedReports_shouldSucceed() {
        mCarTelemetryManager.setListener(DIRECT_EXECUTOR, mListener);
        mCarTelemetryController.addDataForKey(DEFAULT_MANIFEST_KEY, SCRIPT_RESULT_BYTES);

        mCarTelemetryManager.sendFinishedReports(DEFAULT_MANIFEST_KEY);

        verify(mListener).onResult(DEFAULT_MANIFEST_KEY, SCRIPT_RESULT_BYTES);
    }

    @Test
    public void sendAllFinishedReports_shouldSucceed() {
        mCarTelemetryManager.setListener(DIRECT_EXECUTOR, mListener);
        mCarTelemetryController.addDataForKey(DEFAULT_MANIFEST_KEY, SCRIPT_RESULT_BYTES);
        ManifestKey key2 = new ManifestKey("key name", 1);
        mCarTelemetryController.addDataForKey(key2, SCRIPT_RESULT_BYTES);

        mCarTelemetryManager.sendAllFinishedReports();

        verify(mListener).onResult(DEFAULT_MANIFEST_KEY, SCRIPT_RESULT_BYTES);
        verify(mListener).onResult(key2, SCRIPT_RESULT_BYTES);
    }

    @Test
    public void sendScriptExecutionErrors_shouldSucceed() {
        mCarTelemetryManager.setListener(DIRECT_EXECUTOR, mListener);
        mCarTelemetryController.setErrorData(ERROR_BYTES);

        mCarTelemetryManager.sendScriptExecutionErrors();

        verify(mListener).onError(ERROR_BYTES);
    }
}
