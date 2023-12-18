/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.server.wifi.util;

import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND_SERVICE;

import android.annotation.NonNull;
import android.app.ActivityManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.UserHandle;
import android.os.WorkSource;
import android.util.Log;

/**
 * Class for wrapping a WorkSource object and providing some (wifi specific) utility methods.
 *
 * This is primarily used in {@link com.android.server.wifi.HalDeviceManager} class.
 */
public class WorkSourceHelper {
    private static final String TAG = "WorkSourceHelper";
    private static final int APP_INFO_FLAGS_SYSTEM_APP =
            ApplicationInfo.FLAG_SYSTEM | ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;
    private final WorkSource mWorkSource;
    private final WifiPermissionsUtil mWifiPermissionsUtil;
    private final ActivityManager mActivityManager;
    private final PackageManager mPackageManager;

    public WorkSourceHelper(
            @NonNull WorkSource workSource,
            @NonNull WifiPermissionsUtil wifiPermissionsUtil,
            @NonNull ActivityManager activityManager,
            @NonNull PackageManager packageManager) {
        mWorkSource = workSource;
        mWifiPermissionsUtil = wifiPermissionsUtil;
        mActivityManager = activityManager;
        mPackageManager = packageManager;
    }

    @Override
    public String toString() {
        return mWorkSource.toString();
    }

    private boolean isPrivileged(int uid) {
        return mWifiPermissionsUtil.checkNetworkSettingsPermission(uid)
                || mWifiPermissionsUtil.checkNetworkSetupWizardPermission(uid)
                || mWifiPermissionsUtil.checkNetworkStackPermission(uid)
                || mWifiPermissionsUtil.checkMainlineNetworkStackPermission(uid);
    }

    /**
     * Returns whether any of the one or more worksource objects contains a privileged app
     * request.
     *
     * Privileged = Request from an app with NETWORK_SETTINGS, NETWORK_SETUP_WIZARD or
     * NETWORK_STACK permissions.
     */
    public boolean hasAnyPrivilegedAppRequest() {
        for (int i = 0; i < mWorkSource.size(); i++) {
            if (isPrivileged(mWorkSource.getUid(i))) return true;
        }
        return false;
    }

    private boolean isSystem(String packageName, int uid) {
        // when checking ActiveModeWarden#INTERNAL_REQUESTOR_WS
        if (packageName == null) {
            return false;
        }
        try {
            ApplicationInfo info = mPackageManager.getApplicationInfoAsUser(
                    packageName, 0, UserHandle.getUserHandleForUid(uid));
            return (info.flags & APP_INFO_FLAGS_SYSTEM_APP) != 0;
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Failed to retrieve app info for packageName=" + packageName + " uid=" + uid,
                    e);
            // In case of exception, assume unknown app (more strict checking)
            // Note: This case will never happen since checkPackage is
            // called to verify validity before checking App's version.
            return false;
        }
    }

    /**
     * Returns whether any of the one or more worksource objects contains a system app
     * request.
     */
    public boolean hasAnySystemAppRequest() {
        for (int i = 0; i < mWorkSource.size(); i++) {
            if (isSystem(mWorkSource.getPackageName(i), mWorkSource.getUid(i))) return true;
        }
        return false;
    }

    /**
     * Check if the request comes from foreground app.
     */
    private boolean isForegroundApp(@NonNull String requestorPackageName) {
        try {
            return mActivityManager.getPackageImportance(requestorPackageName)
                    <= IMPORTANCE_FOREGROUND;
        } catch (SecurityException e) {
            Log.e(TAG, "Failed to check the app state", e);
            return false;
        }
    }

    /**
     * Returns whether any of the one or more worksource objects contains a foreground app
     * request.
     */
    public boolean hasAnyForegroundAppRequest() {
        for (int i = 0; i < mWorkSource.size(); i++) {
            if (isForegroundApp(mWorkSource.getPackageName(i))) return true;
        }
        return false;
    }

    /**
     * Check if the request comes from foreground service.
     */
    private boolean isForegroundService(@NonNull String requestorPackageName) {
        try {
            int importance = mActivityManager.getPackageImportance(requestorPackageName);
            return IMPORTANCE_FOREGROUND < importance
                    && importance <= IMPORTANCE_FOREGROUND_SERVICE;
        } catch (SecurityException e) {
            Log.e(TAG, "Failed to check the app state", e);
            return false;
        }
    }

    /**
     * Returns whether any of the one or more worksource objects contains a foreground service
     * request.
     */
    public boolean hasAnyForegroundServiceRequest() {
        for (int i = 0; i < mWorkSource.size(); i++) {
            if (isForegroundService(mWorkSource.getPackageName(i))) return true;
        }
        return false;
    }

    /**
     * Returns whether any of the one or more worksource objects contains an internal
     * (i.e uid = Process.WIFI_UID) request.
     */
    public boolean hasAnyInternalRequest() {
        for (int i = 0; i < mWorkSource.size(); i++) {
            if (mWorkSource.getUid(i) == Process.WIFI_UID) return true;
        }
        return false;
    }
}
