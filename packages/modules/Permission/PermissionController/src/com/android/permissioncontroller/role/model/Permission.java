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

import android.os.Build;

import androidx.annotation.NonNull;

import com.android.modules.utils.build.SdkLevel;

import java.util.Objects;

/**
 * A permission to be granted or revoke by a {@link Role}.
 */
public class Permission {

    /**
     * The name of the permission.
     */
    @NonNull
    private final String mName;

    /**
     * The minimum SDK version for this permission to be granted.
     */
    private final int mMinSdkVersion;

    public Permission(@NonNull String name, int minSdkVersion) {
        mName = name;
        mMinSdkVersion = minSdkVersion;
    }

    @NonNull
    public String getName() {
        return mName;
    }

    public int getMinSdkVersion() {
        return mMinSdkVersion;
    }

    /**
     * Check whether this permission is available.
     *
     * @return whether this permission is available
     */
    public boolean isAvailable() {
        // Workaround to match the value 33+ for T+ in roles.xml before SDK finalization.
        if (mMinSdkVersion >= 33) {
            return SdkLevel.isAtLeastT();
        } else {
            return Build.VERSION.SDK_INT >= mMinSdkVersion;
        }
    }

    @Override
    public String toString() {
        return "Permission{"
                + "mName='" + mName + '\''
                + ", mMinSdkVersion=" + mMinSdkVersion
                + '}';
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }
        if (object == null || getClass() != object.getClass()) {
            return false;
        }
        Permission that = (Permission) object;
        return mMinSdkVersion == that.mMinSdkVersion
                && mName.equals(that.mName);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mName, mMinSdkVersion);
    }
}
