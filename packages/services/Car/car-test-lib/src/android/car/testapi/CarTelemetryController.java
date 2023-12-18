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

package android.car.testapi;

import android.car.telemetry.ManifestKey;

/**
 * Controller to manipulate and verify {@link android.car.telemetry.CarTelemetryManager} in
 * unit tests.
 */
public interface CarTelemetryController {
    /**
     * Returns {@code true} if a {@link
     * android.car.telemetry.CarTelemetryManager.CarTelemetryResultsListener} is
     * registered with the manager, otherwise returns {@code false}.
     */
    boolean isListenerSet();

    /**
     * Returns the number of valid manifests registered with the manager.
     */
    int getValidManifestsCount();

    /**
     * Associate a blob of data with the given key, used for testing the flush reports APIs.
     */
    void addDataForKey(ManifestKey key, byte[] data);

    /**
     * Configure the blob of data to be flushed with the
     * {@code FakeCarTelemetryService#flushScriptExecutionErrors()} API.
     */
    void setErrorData(byte[] error);
}
