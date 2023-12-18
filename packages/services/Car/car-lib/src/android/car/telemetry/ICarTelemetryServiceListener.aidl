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

package android.car.telemetry;

import android.car.telemetry.ManifestKey;
import java.util.List;

/**
 * Binder interface implemented by {@code CarTelemetryManager}. Enables sending results from
 * {@code CarTelemetryService} to {@code CarTelemetryManager}.
 *
 * @hide
 */
oneway interface ICarTelemetryServiceListener {

    /**
     * Called by {@code CarTelemetryService} to provide telemetry results. This call is destructive.
     * The parameter will no longer be stored in {@code CarTelemetryService}.
     *
     * @param key the key that the result is associated with.
     * @param result the serialized bytes of the script execution result message.
     */
    void onResult(in ManifestKey key, in byte[] result);

    /**
     * Called by {@code CarTelemetryService} to provide telemetry errors. This call is destrutive.
     * The parameter will no longer be stored in {@code CarTelemetryService}.
     *
     * @param error the serialized bytes of an error message.
     */
    void onError(in byte[] error);
}