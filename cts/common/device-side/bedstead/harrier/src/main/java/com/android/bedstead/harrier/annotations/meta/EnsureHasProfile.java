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

package com.android.bedstead.harrier.annotations.meta;

import static com.android.bedstead.harrier.DeviceState.UserType.CURRENT_USER;

import com.android.bedstead.harrier.DeviceState;

import java.lang.annotation.Target;

/**
 * This annotation should not be used directly. It exists as a template for annotations which
 * ensure that a given user has a specific profile type.
 */
@Target({})
public @interface EnsureHasProfile {
    /** Which user type the profile should be attached to. */
    DeviceState.UserType forUser() default CURRENT_USER;

    /** Whether the test app should be installed in the profile. */
    boolean installTestApp() default true;

    /**
     * Whether the profile owner's DPC should be returned by calls to {@link DeviceState#dpc()}.
     *
     * <p>Only one device policy controller per test should be marked as primary.
     */
    // NOTE: This field is only required if hasProfileOwner=true
    boolean dpcIsPrimary() default false;
}
