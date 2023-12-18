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
import com.android.bedstead.harrier.annotations.meta.EnsureHasNoUserAnnotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Mark that a test method should run on a device which has no secondary user that is not the
 * current user.
 *
 * <p>Your test configuration may be configured so that this test is only run on a device which
 * has no secondary user that is not the current user. Otherwise, you can use {@link DeviceState}
 * to ensure that the device enters the correct state for the method.
 */
@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@EnsureHasNoUserAnnotation("android.os.usertype.full.SECONDARY")
public @interface EnsureHasNoSecondaryUser {
}
