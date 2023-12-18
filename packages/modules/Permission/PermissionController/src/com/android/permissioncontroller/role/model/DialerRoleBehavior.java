/*
 * Copyright (C) 2018 The Android Open Source Project
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
import android.content.pm.ApplicationInfo;
import android.os.UserHandle;
import android.telecom.TelecomManager;
import android.telephony.TelephonyManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;

import com.android.modules.utils.build.SdkLevel;
import com.android.permissioncontroller.R;
import com.android.permissioncontroller.role.utils.PackageUtils;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;

/**
 * Class for behavior of the dialer role.
 *
 * @see com.android.settings.applications.DefaultAppSettings
 * @see com.android.settings.applications.defaultapps.DefaultPhonePreferenceController
 * @see com.android.settings.applications.defaultapps.DefaultPhonePicker
 */
public class DialerRoleBehavior implements RoleBehavior {

    /**
     * Permissions to be granted if the application fulfilling the dialer role is also a system app.
     */
    private static final List<String> SYSTEM_DIALER_PERMISSIONS = Arrays.asList(
            android.Manifest.permission.ACCESS_RCS_USER_CAPABILITY_EXCHANGE
    );

    @Override
    public boolean isAvailableAsUser(@NonNull Role role, @NonNull UserHandle user,
            @NonNull Context context) {
        TelephonyManager telephonyManager = context.getSystemService(TelephonyManager.class);
        return telephonyManager.isVoiceCapable();
    }

    @Override
    public void prepareApplicationPreferenceAsUser(@NonNull Role role,
            @NonNull Preference preference, @NonNull ApplicationInfo applicationInfo,
            @NonNull UserHandle user, @NonNull Context context) {
        TelecomManager telecomManager = context.getSystemService(TelecomManager.class);
        String systemPackageName = telecomManager.getSystemDialerPackage();
        if (Objects.equals(applicationInfo.packageName, systemPackageName)) {
            preference.setSummary(R.string.default_app_system_default);
        } else {
            preference.setSummary(null);
        }
    }

    @Nullable
    @Override
    public CharSequence getConfirmationMessage(@NonNull Role role, @NonNull String packageName,
            @NonNull Context context) {
        return EncryptionUnawareConfirmationMixin.getConfirmationMessage(role, packageName,
                context);
    }

    @Override
    public boolean isVisibleAsUser(@NonNull Role role, @NonNull UserHandle user,
            @NonNull Context context) {
        return context.getResources().getBoolean(R.bool.config_showDialerRole);
    }

    @Override
    public void grant(@NonNull Role role, @NonNull String packageName, @NonNull Context context) {
        if (SdkLevel.isAtLeastS()) {
            if (PackageUtils.isSystemPackage(packageName, context)) {
                Permissions.grant(packageName, SYSTEM_DIALER_PERMISSIONS, false, false,
                        true, false, false, context);
            }
        }
    }

    @Override
    public void revoke(@NonNull Role role, @NonNull String packageName,
            @NonNull Context context) {
        if (SdkLevel.isAtLeastS()) {
            Permissions.revoke(packageName, SYSTEM_DIALER_PERMISSIONS, true, false, false, context);
        }
    }
}
