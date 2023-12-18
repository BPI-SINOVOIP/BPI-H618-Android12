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

package com.android.permissioncontroller.role.model;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.UserHandle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Class for behavior of the system shell role.
 */
public class SystemShellRoleBehavior implements RoleBehavior {
    @Nullable
    @Override
    public Boolean isPackageQualified(@NonNull Role role, @NonNull String packageName,
            @NonNull Context context) {
        PackageManager packageManager = context.getPackageManager();
        int uid;
        try {
            uid = packageManager.getPackageUid(packageName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
        int appId = UserHandle.getAppId(uid);
        return appId == Process.SHELL_UID;
    }
}
