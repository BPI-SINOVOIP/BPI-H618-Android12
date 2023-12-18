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
package com.android.car.evs;

import android.annotation.SystemApi;
import android.car.evs.CarEvsManager;
import android.car.evs.CarEvsManager.CarEvsStreamEvent;
import android.util.Slog;

/**
 * Utility class for CarEvsService
 *
 * @hide
 */
@SystemApi
public final class CarEvsServiceUtils {
    private static final String TAG = CarEvsServiceUtils.class.getSimpleName();
    private CarEvsServiceUtils() {}

    /**
     * Converts EvsEvent to CarEvsManager.CarEvsStreamEvent.
     *
     * See the definition of EvsEventType in hardware/interfaces/automotive/evs/1.1/types.hal
     */
    static @CarEvsStreamEvent int convertToStreamEvent(int inputEvent) {
        @CarEvsStreamEvent int outputStatus = CarEvsManager.STREAM_EVENT_NONE;

        switch (inputEvent) {
            case 0: // EvsEventType.STREAM_STARTED
                outputStatus = CarEvsManager.STREAM_EVENT_STREAM_STARTED;
                break;
            case 1: // EvsEventType.STREAM_STOPPED
                outputStatus = CarEvsManager.STREAM_EVENT_STREAM_STOPPED;
                break;
            case 2: // EvsEventType.FRAME_DROPPED
                outputStatus = CarEvsManager.STREAM_EVENT_FRAME_DROPPED;
                break;
            case 3: // EvsEventType.TIMEOUT
                outputStatus = CarEvsManager.STREAM_EVENT_TIMEOUT;
                break;
            case 4: // EvsEventType.PARAMETER_CHANGED
                outputStatus = CarEvsManager.STREAM_EVENT_PARAMETER_CHANGED;
                break;
            case 5: // EvsEventType.MASTER_RELEASED
                outputStatus = CarEvsManager.STREAM_EVENT_PRIMARY_OWNER_CHANGED;
                break;
            case 6: // EvsEventType.STREAM_ERROR
                outputStatus = CarEvsManager.STREAM_EVENT_OTHER_ERRORS;
                break;
            default:
                Slog.w(TAG, "Invalid event type: " + inputEvent);
                break;
        }

        return outputStatus;
    }
}
