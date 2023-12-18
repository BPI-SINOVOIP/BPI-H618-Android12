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

package com.android.car.internal.common;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.BOILERPLATE_CODE;

import android.annotation.IntDef;

import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Provides common constants for Car library, Car Service, and System Server.
 *
 * @hide
 */
public final class CommonConstants {

    @ExcludeFromCodeCoverageGeneratedReport(reason = BOILERPLATE_CODE)
    private CommonConstants() {
        throw new UnsupportedOperationException("contains only static constants");
    }

    // CarUserManagerConstants

    public static final int USER_LIFECYCLE_EVENT_TYPE_STARTING = 1;
    public static final int USER_LIFECYCLE_EVENT_TYPE_SWITCHING = 2;
    public static final int USER_LIFECYCLE_EVENT_TYPE_UNLOCKING = 3;
    public static final int USER_LIFECYCLE_EVENT_TYPE_UNLOCKED = 4;
    public static final int USER_LIFECYCLE_EVENT_TYPE_STOPPING = 5;
    public static final int USER_LIFECYCLE_EVENT_TYPE_STOPPED = 6;

    // CarService Constants
    public static final String CAR_SERVICE_INTERFACE = "android.car.ICar";

    @IntDef(prefix = { "USER_LIFECYCLE_EVENT_TYPE_" }, value = {
            USER_LIFECYCLE_EVENT_TYPE_STARTING,
            USER_LIFECYCLE_EVENT_TYPE_SWITCHING,
            USER_LIFECYCLE_EVENT_TYPE_UNLOCKING,
            USER_LIFECYCLE_EVENT_TYPE_UNLOCKED,
            USER_LIFECYCLE_EVENT_TYPE_STOPPING,
            USER_LIFECYCLE_EVENT_TYPE_STOPPED,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface UserLifecycleEventType{}
}
