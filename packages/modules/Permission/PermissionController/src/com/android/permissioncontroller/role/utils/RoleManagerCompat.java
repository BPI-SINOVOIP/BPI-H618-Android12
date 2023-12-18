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

package com.android.permissioncontroller.role.utils;

import android.app.role.RoleManager;

import androidx.annotation.NonNull;

import com.android.modules.utils.build.SdkLevel;

/**
 * Helper for accessing features in {@link RoleManager}.
 */
public class RoleManagerCompat {

    private RoleManagerCompat() {}

    /**
     * @see RoleManager#isBypassingRoleQualification()
     */
    public static boolean isBypassingRoleQualification(@NonNull RoleManager roleManager) {
        if (SdkLevel.isAtLeastS()) {
            return roleManager.isBypassingRoleQualification();
        } else {
            return false;
        }
    }
}
