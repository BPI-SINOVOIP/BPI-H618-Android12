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

package com.android.bedstead.harrier.annotations;

import static com.android.bedstead.harrier.DeviceState.UserType.CURRENT_USER;
import static com.android.bedstead.harrier.OptionalBoolean.TRUE;

import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.OptionalBoolean;
import com.android.bedstead.harrier.annotations.meta.EnsureHasProfileAnnotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Mark that a test method should run on a user which has a Tv profile.
 *
 * <p>Your test configuration may be configured so that this test is only run on a user which has
 * a Tv profile. Otherwise, you can use {@link DeviceState} to ensure that the device enters
 * the correct state for the method.
 */
@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@EnsureHasProfileAnnotation("com.android.tv.profile")
public @interface EnsureHasTvProfile {
    /** Which user type the tv profile should be attached to. */
    DeviceState.UserType forUser() default CURRENT_USER;

    /** Whether the instrumented test app should be installed in the tv profile. */
    OptionalBoolean installInstrumentedApp() default TRUE;
}