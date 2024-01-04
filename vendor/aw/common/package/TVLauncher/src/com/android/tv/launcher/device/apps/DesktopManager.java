/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.launcher.device.apps;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;

/**
 * Handles opening an application.
 */
class DesktopManager {

    private final Context mContext;
    private AppInfo mAppInfo;
    private Intent mLaunchIntent;
    private final String SHARED_PREFERENCES_NAME = "fav_app";
    private final String FAV_APP_PREFERENCE_TOTAL_NO = "total_no"; 

    DesktopManager(Context context, AppInfo appInfo) {
        mContext = context;
        mAppInfo = appInfo;
    }

    public boolean canSetDesktop() {
        PackageManager pm = mContext.getPackageManager();
        mLaunchIntent = pm.getLeanbackLaunchIntentForPackage(mAppInfo.getPackageName());
        if (mLaunchIntent == null) {
            mLaunchIntent = pm.getLaunchIntentForPackage(mAppInfo.getPackageName());
        }
        return mLaunchIntent != null;
    }

    public void set(ApplicationsState state) {
        if (canSetDesktop()) {
            SharedPreferences prefs = mContext.getSharedPreferences(SHARED_PREFERENCES_NAME,Context.MODE_PRIVATE);
	    int no = prefs.getInt(FAV_APP_PREFERENCE_TOTAL_NO,0);
	    SharedPreferences.Editor editor = prefs.edit();
	    editor.putString(Integer.toString(no + 1),mAppInfo.getPackageName());
	    editor.putInt(FAV_APP_PREFERENCE_TOTAL_NO,no + 1);
	    editor.apply();
        }
    }
}
