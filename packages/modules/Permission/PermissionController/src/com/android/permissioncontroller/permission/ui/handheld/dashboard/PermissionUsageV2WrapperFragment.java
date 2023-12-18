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

package com.android.permissioncontroller.permission.ui.handheld.dashboard;

import static com.android.permissioncontroller.Constants.EXTRA_SESSION_ID;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.preference.PreferenceFragmentCompat;

import com.android.permissioncontroller.permission.ui.handheld.PermissionsCollapsingToolbarBaseFragment;

/**
 * Wrapper over PermissionUsageV2Fragment
 */
@RequiresApi(Build.VERSION_CODES.S)
public class PermissionUsageV2WrapperFragment extends PermissionsCollapsingToolbarBaseFragment{
    @NonNull
    @Override
    public PreferenceFragmentCompat createPreferenceFragment() {
        return new PermissionUsageV2Fragment();
    }

    /**
     * @return A new fragment
     */
    public static @NonNull PermissionUsageV2WrapperFragment newInstance(@Nullable String groupName,
            long numMillis, long sessionId) {
        PermissionUsageV2WrapperFragment fragment = new PermissionUsageV2WrapperFragment();
        Bundle arguments = new Bundle();
        if (groupName != null) {
            arguments.putString(Intent.EXTRA_PERMISSION_GROUP_NAME, groupName);
        }
        arguments.putLong(Intent.EXTRA_DURATION_MILLIS, numMillis);
        arguments.putLong(EXTRA_SESSION_ID, sessionId);
        fragment.setArguments(arguments);
        return fragment;
    }
}
