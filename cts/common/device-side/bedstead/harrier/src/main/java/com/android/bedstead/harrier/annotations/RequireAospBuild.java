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

import static com.android.bedstead.harrier.annotations.RequireAospBuild.GMS_CORE_PACKAGE;
import static com.android.bedstead.harrier.annotations.RequireAospBuild.GSF_PACKAGE;
import static com.android.bedstead.harrier.annotations.RequireAospBuild.PLAY_STORE_PACKAGE;

import com.android.bedstead.harrier.DeviceState;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@RequirePackageNotInstalled(value = GMS_CORE_PACKAGE, onUser = DeviceState.UserType.ANY)
@RequirePackageNotInstalled(value = PLAY_STORE_PACKAGE, onUser = DeviceState.UserType.ANY)
@RequirePackageNotInstalled(value = GSF_PACKAGE, onUser = DeviceState.UserType.ANY)
public @interface RequireAospBuild {
    String GMS_CORE_PACKAGE = "com.google.android.gms";
    String PLAY_STORE_PACKAGE = "com.android.vending";
    String GSF_PACKAGE = "com.google.android.gsf";
}
