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

package com.android.bedstead.harrier.annotations.enterprise;

import static android.content.pm.PackageManager.FEATURE_DEVICE_ADMIN;

import static com.android.bedstead.harrier.DeviceState.UserType.SYSTEM_USER;

import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.FailureMode;
import com.android.bedstead.harrier.annotations.RequireFeature;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Mark that a test requires that a device owner is available on the device.
 *
 * <p>Your test configuration may be configured so that this test is only run on a device which has
 * a device owner. Otherwise, you can use {@link DeviceState} to ensure that the device enters
 * the correct state for the method. If using {@link DeviceState}, you can use
 * {@link DeviceState#deviceOwner()} to interact with the device owner.
 *
 * <p>If {@link DeviceState} is required to set the device owner (because there isn't one already)
 * then all users and accounts may be removed from the device.
 */
@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@RequireFeature(FEATURE_DEVICE_ADMIN)
public @interface EnsureHasDeviceOwner {
    /** Which user type the device owner should be installed on. */
    DeviceState.UserType onUser() default SYSTEM_USER;

    /** Behaviour if the device owner cannot be set. */
    FailureMode failureMode() default FailureMode.FAIL;

    /**
     * Whether this DPC should be returned by calls to {@link DeviceState#dpc()}.
     *
     * <p>Only one device policy controller per test should be marked as primary.
     */
    boolean isPrimary() default false;

    /**
     * Affiliation ids to be set for the device owner.
     */
    String[] affiliationIds() default {};
}
