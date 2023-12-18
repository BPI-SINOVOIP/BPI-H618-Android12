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

import static android.content.pm.PackageManager.FEATURE_DEVICE_ADMIN;

import static com.android.bedstead.harrier.OptionalBoolean.ANY;

import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.OptionalBoolean;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasProfileOwner;
import com.android.bedstead.harrier.annotations.meta.RequireRunOnProfileAnnotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Mark that a test method should run within a work profile.
 *
 * <p>Your test configuration should be such that this test is only run where a work profile is
 * created and the test is being run within that user.
 *
 * <p>Optionally, you can guarantee that these methods do not run outside of a work
 * profile by using {@link DeviceState}.
 *
 * <p>This annotation by default opts a test into multi-user presubmit. New tests should also be
 * annotated {@link Postsubmit} until they are shown to meet the multi-user presubmit requirements.
 */
@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@RequireRunOnProfileAnnotation(value = "android.os.usertype.profile.MANAGED", hasProfileOwner = true)
@EnsureHasProfileOwner
@RequireFeature(FEATURE_DEVICE_ADMIN)
public @interface RequireRunOnWorkProfile {
    OptionalBoolean installInstrumentedAppInParent() default ANY;

    /**
     * Whether the profile owner's DPC should be returned by calls to {@link DeviceState#dpc()}.
     *
     * <p>Only one device policy controller per test should be marked as primary.
     */
    boolean dpcIsPrimary() default false;

    /**
     * Affiliation ids to be set for the profile owner.
     */
    String[] affiliationIds() default {};
}