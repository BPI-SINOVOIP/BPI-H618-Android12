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

import com.android.bedstead.harrier.DeviceState;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Mark that a test method should be run using GMS Instrumentation for certain versions.
 *
 * <p>This will apply {@link RequireSdkVersion} to ensure that on the given versions, this test
 * only runs when the instrumented package is `com.google.android.gms`. It will also skip the test
 * if it is run with gms instrumentation on a version which does not require it.
 *
 * <p>This allows for two test configurations to be set up, one which instruments GMS and one
 * which does not - and both pointed at the same test sources.
 *
 * <p>Your test configuration may be configured so that this test is only run on a device with the
 * given state. Otherwise, you can use {@link DeviceState} to ensure that the test is
 * not run when the sdk version is not correct.
 */
@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
public @interface RequireGmsInstrumentation {
    int min() default 1;
    int max() default Integer.MAX_VALUE;
}
