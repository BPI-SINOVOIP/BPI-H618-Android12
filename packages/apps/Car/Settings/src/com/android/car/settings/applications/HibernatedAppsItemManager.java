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

package com.android.car.settings.applications;

import static android.app.usage.UsageStatsManager.INTERVAL_MONTHLY;

import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.apphibernation.AppHibernationManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.provider.DeviceConfig;
import android.util.ArrayMap;

import androidx.annotation.NonNull;

import com.android.settingslib.utils.ThreadUtils;

import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Class for fetching and returning the number of hibernated apps. Largely derived from
 * {@link com.android.settings.applications.HibernatedAppsPreferenceController}.
 */
public class HibernatedAppsItemManager {
    private static final String PROPERTY_HIBERNATION_UNUSED_THRESHOLD_MILLIS =
            "auto_revoke_unused_threshold_millis2";
    private static final long DEFAULT_UNUSED_THRESHOLD_MS = TimeUnit.DAYS.toMillis(90);

    private final Context mContext;

    private HibernatedAppsCountListener mListener;

    public HibernatedAppsItemManager(Context context) {
        mContext = context;
    }

    /**
     * Starts fetching recently used apps
     */
    public void startLoading() {
        ThreadUtils.postOnBackgroundThread(() -> {
            int count = getNumHibernated();
            ThreadUtils.postOnMainThread(() -> {
                HibernatedAppsCountListener localListener = mListener;
                if (localListener != null) {
                    localListener.onHibernatedAppsCountLoaded(count);
                }
            });
        });
    }

    /**
     * Registers a listener that will be notified once the data is loaded.
     */
    public void setListener(@NonNull HibernatedAppsCountListener listener) {
        mListener = listener;
    }

    private int getNumHibernated() {
        // TODO(b/187465752): Find a way to export this logic from PermissionController module
        PackageManager pm = mContext.getPackageManager();
        AppHibernationManager ahm =
                mContext.getSystemService(AppHibernationManager.class);
        List<String> hibernatedPackages = ahm.getHibernatingPackagesForUser();
        int numHibernated = hibernatedPackages.size();

        // Also need to count packages that are auto revoked but not hibernated.
        int numAutoRevoked = 0;
        UsageStatsManager usm = mContext.getSystemService(UsageStatsManager.class);
        long now = System.currentTimeMillis();
        long unusedThreshold = DeviceConfig.getLong(DeviceConfig.NAMESPACE_PERMISSIONS,
                PROPERTY_HIBERNATION_UNUSED_THRESHOLD_MILLIS, DEFAULT_UNUSED_THRESHOLD_MS);
        List<UsageStats> usageStatsList = usm.queryUsageStats(INTERVAL_MONTHLY,
                now - unusedThreshold, now);
        Map<String, UsageStats> recentlyUsedPackages = new ArrayMap<>();
        for (UsageStats us : usageStatsList) {
            recentlyUsedPackages.put(us.mPackageName, us);
        }
        List<PackageInfo> packages = pm.getInstalledPackages(
                PackageManager.MATCH_DISABLED_COMPONENTS | PackageManager.GET_PERMISSIONS);
        for (PackageInfo pi : packages) {
            String packageName = pi.packageName;
            UsageStats usageStats = recentlyUsedPackages.get(packageName);
            // Only count packages that have not been used recently as auto-revoked permissions may
            // stay revoked even after use if the user has not regranted them.
            boolean usedRecently = (usageStats != null
                    && (now - usageStats.getLastTimeAnyComponentUsed() < unusedThreshold
                    || now - usageStats.getLastTimeVisible() < unusedThreshold));
            if (!hibernatedPackages.contains(packageName)
                    && pi.requestedPermissions != null
                    && !usedRecently) {
                for (String perm : pi.requestedPermissions) {
                    if ((pm.getPermissionFlags(perm, packageName, mContext.getUser())
                            & PackageManager.FLAG_PERMISSION_AUTO_REVOKED) != 0) {
                        numAutoRevoked++;
                        break;
                    }
                }
            }
        }
        return numHibernated + numAutoRevoked;
    }


    /**
     * Callback that is called once the count of hibernated apps has been fetched.
     */
    public interface HibernatedAppsCountListener {
        /**
         * Called when the count of hibernated apps has loaded.
         */
        void onHibernatedAppsCountLoaded(int hibernatedAppsCount);
    }
}
