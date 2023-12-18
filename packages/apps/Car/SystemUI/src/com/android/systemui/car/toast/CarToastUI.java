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

package com.android.systemui.car.toast;

import android.app.ActivityManager;
import android.app.ITransientNotificationCallback;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

import com.android.systemui.R;
import com.android.systemui.dagger.SysUISingleton;
import com.android.systemui.dagger.qualifiers.Main;
import com.android.systemui.statusbar.CommandQueue;
import com.android.systemui.toast.ToastFactory;
import com.android.systemui.toast.ToastLogger;
import com.android.systemui.toast.ToastUI;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.inject.Inject;

/**
 * Controls display of text toasts in AAOS.
 */
@SysUISingleton
public class CarToastUI extends ToastUI {
    private static final boolean DEBUG = false;
    private static final String TAG = "CarToastUI";

    private final PackageManager mPackageManager;
    private final Set<String> mPackageNameAllowList;

    @Inject
    public CarToastUI(Context context, @Main Resources resources, CommandQueue commandQueue,
            ToastFactory toastFactory, ToastLogger toastLogger, PackageManager packageManager) {
        super(context, commandQueue, toastFactory, toastLogger);
        mPackageManager = packageManager;

        String[] allowList = resources.getStringArray(
                R.array.config_restrictedToastsPackageNameAllowList);
        mPackageNameAllowList = new HashSet<>(Arrays.asList(allowList));
    }

    @Override
    public void showToast(int uid, String packageName, IBinder token, CharSequence text,
            IBinder windowToken, int duration, @Nullable ITransientNotificationCallback callback) {
        if (!isAllowListed(packageName) && !isSystemPrivilegedOrPlatformKey(packageName)) {
            if (DEBUG) {
                Log.w(TAG, packageName
                        + " cannot show a Toast since it is not allow listed and it isn't "
                        + "privileged.");
            }
            return;
        }
        if (DEBUG) {
            Log.d(TAG,
                    packageName
                            + " is a system privileged app or has been signed with platform key.");
        }

        super.showToast(uid, packageName, token, text, windowToken, duration, callback);
    }

    private boolean isAllowListed(String packageName) {
        return mPackageNameAllowList.contains(packageName);
    }

    private boolean isSystemPrivilegedOrPlatformKey(String packageName) {
        PackageInfo packageInfo = getPackageInfo(packageName);
        if (packageInfo == null) return false;

        return (packageInfo.applicationInfo.isSignedWithPlatformKey() || (
                packageInfo.applicationInfo.isSystemApp()
                        && packageInfo.applicationInfo.isPrivilegedApp()));
    }

    private PackageInfo getPackageInfo(String packageName) {
        PackageInfo packageInfo = null;
        try {
            packageInfo = mPackageManager.getPackageInfoAsUser(packageName, /* flags= */ 0,
                    ActivityManager.getCurrentUser());
        } catch (PackageManager.NameNotFoundException ex) {
            Log.e(TAG, "package not found: " + packageName);
        }
        return packageInfo;
    }
}
