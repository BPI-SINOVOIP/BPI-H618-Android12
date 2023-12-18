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

package com.android.car.watchdog;

import android.automotive.watchdog.internal.ApplicationCategoryType;
import android.automotive.watchdog.internal.ComponentType;
import android.automotive.watchdog.internal.PackageIdentifier;
import android.automotive.watchdog.internal.PackageInfo;
import android.automotive.watchdog.internal.UidType;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.UserHandle;
import android.util.IntArray;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.internal.annotations.GuardedBy;
import com.android.server.utils.Slogf;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Handles package info resolving */
public final class PackageInfoHandler {
    private static final String TAG = CarLog.tagFor(PackageInfoHandler.class);

    private final PackageManager mPackageManager;
    private final Object mLock = new Object();
    /* Cache of uid to package name mapping. */
    @GuardedBy("mLock")
    private final SparseArray<String> mPackageNamesByUid = new SparseArray<>();
    @GuardedBy("mLock")
    private List<String> mVendorPackagePrefixes = new ArrayList<>();

    public PackageInfoHandler(PackageManager packageManager) {
        mPackageManager = packageManager;
    }

    /**
     * Returns package names for the given UIDs.
     *
     * Some UIDs may not have package names. This may occur when a UID is being removed and the
     * internal data structures are not up-to-date. The caller should handle it.
     */
    public SparseArray<String> getPackageNamesForUids(int[] uids) {
        IntArray unmappedUids = new IntArray(uids.length);
        SparseArray<String> packageNamesByUid = new SparseArray<>();
        synchronized (mLock) {
            for (int uid : uids) {
                String packageName = mPackageNamesByUid.get(uid, null);
                if (packageName != null) {
                    packageNamesByUid.append(uid, packageName);
                } else {
                    unmappedUids.add(uid);
                }
            }
        }
        if (unmappedUids.size() == 0) {
            return packageNamesByUid;
        }
        String[] packageNames = mPackageManager.getNamesForUids(unmappedUids.toArray());
        synchronized (mLock) {
            for (int i = 0; i < unmappedUids.size(); ++i) {
                if (packageNames[i] == null || packageNames[i].isEmpty()) {
                    continue;
                }
                mPackageNamesByUid.append(unmappedUids.get(i), packageNames[i]);
                packageNamesByUid.append(unmappedUids.get(i), packageNames[i]);
            }
        }
        return packageNamesByUid;
    }

    /**
     * Returns package infos for the given UIDs.
     *
     * Some UIDs may not have package infos. This may occur when a UID is being removed and the
     * internal data structures are not up-to-date. The caller should handle it.
     */
    public List<PackageInfo> getPackageInfosForUids(int[] uids,
            List<String> vendorPackagePrefixes) {
        synchronized (mLock) {
            /*
             * Vendor package prefixes don't change frequently because it changes only when the
             * vendor configuration is updated. Thus caching this locally during this call should
             * keep the cache up-to-date because the daemon issues this call frequently.
             */
            mVendorPackagePrefixes = vendorPackagePrefixes;
        }
        SparseArray<String> packageNamesByUid = getPackageNamesForUids(uids);
        ArrayList<PackageInfo> packageInfos = new ArrayList<>(packageNamesByUid.size());
        for (int i = 0; i < packageNamesByUid.size(); ++i) {
            packageInfos.add(getPackageInfo(packageNamesByUid.keyAt(i),
                    packageNamesByUid.valueAt(i)));
        }
        return packageInfos;
    }

    private PackageInfo getPackageInfo(int uid, String packageName) {
        PackageInfo packageInfo = new PackageInfo();
        packageInfo.packageIdentifier = new PackageIdentifier();
        packageInfo.packageIdentifier.uid = uid;
        packageInfo.packageIdentifier.name = packageName;
        packageInfo.sharedUidPackages = new ArrayList<>();
        packageInfo.componentType = ComponentType.UNKNOWN;
        /* Application category type mapping is handled on the daemon side. */
        packageInfo.appCategoryType = ApplicationCategoryType.OTHERS;
        int userId = UserHandle.getUserId(uid);
        int appId = UserHandle.getAppId(uid);
        packageInfo.uidType = appId >= Process.FIRST_APPLICATION_UID ? UidType.APPLICATION :
                UidType.NATIVE;

        if (packageName.startsWith("shared:")) {
            String[] sharedUidPackages = mPackageManager.getPackagesForUid(uid);
            if (sharedUidPackages == null) {
                return packageInfo;
            }
            boolean seenVendor = false;
            boolean seenSystem = false;
            boolean seenThirdParty = false;
            /*
             * A shared UID has multiple packages associated with it and these packages may be
             * mapped to different component types. Thus map the shared UID to the most restrictive
             * component type.
             */
            for (int i = 0; i < sharedUidPackages.length; ++i) {
                int componentType = getPackageComponentType(userId, sharedUidPackages[i]);
                switch(componentType) {
                    case ComponentType.VENDOR:
                        seenVendor = true;
                        break;
                    case ComponentType.SYSTEM:
                        seenSystem = true;
                        break;
                    case ComponentType.THIRD_PARTY:
                        seenThirdParty = true;
                        break;
                    default:
                        Slogf.w(TAG, "Unknown component type %d for package '%s'", componentType,
                                sharedUidPackages[i]);
                }
            }
            packageInfo.sharedUidPackages = Arrays.asList(sharedUidPackages);
            if (seenVendor) {
                packageInfo.componentType = ComponentType.VENDOR;
            } else if (seenSystem) {
                packageInfo.componentType = ComponentType.SYSTEM;
            } else if (seenThirdParty) {
                packageInfo.componentType = ComponentType.THIRD_PARTY;
            }
        } else {
            packageInfo.componentType = getPackageComponentType(
                    userId, packageName);
        }
        return packageInfo;
    }

    private int getPackageComponentType(int userId, String packageName) {
        try {
            ApplicationInfo info = mPackageManager.getApplicationInfoAsUser(packageName,
                    /* flags= */ 0, userId);
            return getComponentType(packageName, info);
        } catch (PackageManager.NameNotFoundException e) {
            Slogf.e(TAG, "Package '%s' not found for user %d: %s", packageName, userId, e);
        }
        return ComponentType.UNKNOWN;
    }

    /** Returns the component type for the given package and its application info. */
    public int getComponentType(String packageName, ApplicationInfo info) {
        if ((info.privateFlags & ApplicationInfo.PRIVATE_FLAG_OEM) != 0
                || (info.privateFlags & ApplicationInfo.PRIVATE_FLAG_VENDOR) != 0
                || (info.privateFlags & ApplicationInfo.PRIVATE_FLAG_ODM) != 0) {
            return ComponentType.VENDOR;
        }
        if ((info.flags & ApplicationInfo.FLAG_SYSTEM) != 0
                || (info.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0
                || (info.privateFlags & ApplicationInfo.PRIVATE_FLAG_PRODUCT) != 0
                || (info.privateFlags & ApplicationInfo.PRIVATE_FLAG_SYSTEM_EXT) != 0) {
            synchronized (mLock) {
                for (String prefix : mVendorPackagePrefixes) {
                    if (packageName.startsWith(prefix)) {
                        return ComponentType.VENDOR;
                    }
                }
            }
            return ComponentType.SYSTEM;
        }
        return ComponentType.THIRD_PARTY;
    }
}
