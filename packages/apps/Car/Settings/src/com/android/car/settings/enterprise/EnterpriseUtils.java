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
package com.android.car.settings.enterprise;

import android.annotation.Nullable;
import android.app.admin.DeviceAdminInfo;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;

import com.android.car.settings.common.Logger;

import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.List;

/**
 * Generic helper methods for this package.
 */
final class EnterpriseUtils {

    private static final Logger LOG = new Logger(EnterpriseUtils.class);

    /**
     * Gets the active admin for the given package.
     */
    @Nullable
    public static ComponentName getAdminWithinPackage(Context context, String packageName) {
        List<ComponentName> admins = context.getSystemService(DevicePolicyManager.class)
                .getActiveAdmins();
        if (admins == null) {
            LOG.v("getAdminWithinPackage(): no active admins on context");
            return null;
        }
        return admins.stream().filter(i -> i.getPackageName().equals(packageName)).findAny()
                .orElse(null);
    }

    /**
     * Gets the active admin info for the given admin .
     */
    @Nullable
    public static DeviceAdminInfo getDeviceAdminInfo(Context context, ComponentName admin) {
        LOG.d("getDeviceAdminInfo()(): " + admin.flattenToShortString());

        ActivityInfo ai;
        try {
            ai = context.getPackageManager().getReceiverInfo(admin, PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            LOG.v("Unable to get activity info for " + admin.flattenToShortString() + ": " + e);
            return null;
        }

        try {
            return new DeviceAdminInfo(context, ai);
        } catch (XmlPullParserException | IOException e) {
            LOG.v("Unable to retrieve device policy for " + admin.flattenToShortString() + ": ",
                    e);
            return null;
        }
    }

    private EnterpriseUtils() {
        throw new UnsupportedOperationException("Provides only static methods");
    }
}
