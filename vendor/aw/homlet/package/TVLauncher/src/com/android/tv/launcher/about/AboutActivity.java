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

package com.android.tv.launcher.about;

import com.android.tv.launcher.R;
import com.android.tv.launcher.PreferenceUtils;
import com.android.tv.launcher.dialog.old.Action;
import com.android.tv.launcher.dialog.old.ActionAdapter;
import com.android.tv.launcher.dialog.old.ActionFragment;
import com.android.tv.launcher.dialog.old.ContentFragment;
import com.android.tv.launcher.dialog.old.DialogActivity;
import com.android.tv.launcher.dialog.old.BaseActionFragment;

import com.android.tv.launcher.name.DeviceManager;

import android.app.ActivityManager;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/**
 * Activity which shows the build / model / legal info / etc.
 */
public class AboutActivity extends DialogActivity implements ActionAdapter.Listener,
        ActionAdapter.OnFocusListener {

    private static final String TAG = "AboutActivity";

    /**
     * Action keys for switching over in onActionClicked.
     */
    private static final String KEY_UPDATE = "update";
    private static final String KEY_LEGAL_INFO = "about_legal_info";
    private static final String KEY_BUILD = "build";
    private static final String KEY_VERSION = "version";
    private static final String KEY_REBOOT = "reboot";

    /**
     * Intent action of SettingsLicenseActivity (for displaying open source licenses.)
     */
    private static final String SETTINGS_LEGAL_LICENSE_INTENT_ACTION = "android.settings.LICENSE";

    /**
     * Intent action of SettingsTosActivity (for displaying terms of service.)
     */
    private static final String SETTINGS_LEGAL_TERMS_OF_SERVICE = "android.settings.TERMS";

    /**
     * Intent action of device name activity.
     */
    //名称修改一下，不然这个intent会启动到原生Settings里边去
    private static final String SETTINGS_DEVICE_NAME_INTENT_ACTION = "android.settings.DEVICE_NAME_SOFTWINNER";

    /**
     * Intent action of system update activity.
     */
    private static final String SETTINGS_UPDATE_SYSTEM = "android.settings.SYSTEM_UPDATE_SETTINGS";
    /**
     * 这个是文件管理器里边的 recovery升级的
     */
    private static final String SOFTWINNER_UPDATE_SYSTEM = "softwinner.intent.action.RECOVREY";

    /**
     * Intent to launch ads activity.
     */
    private static final String SETTINGS_ADS_ACTIVITY_PACKAGE = "com.google.android.gms";
    private static final String SETTINGS_ADS_ACTIVITY_ACTION =
            "com.google.android.gms.settings.ADS_PRIVACY";

    /**
     * Intent component to launch PlatLogo Easter egg.
     */
    private static final ComponentName mPlatLogoActivity = new ComponentName("android",
            "com.android.internal.app.PlatLogoActivity");

    /**
     * Number of clicks it takes to be a developer.
     */
    private static final int NUM_DEVELOPER_CLICKS = 7;

    private int mDeveloperClickCount;
    private PreferenceUtils mPreferenceUtils;
    private Toast mToast;
    private int mSelectedIndex;
    private long[] mHits = new long[3];
    private int mHitsIndex;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPreferenceUtils = new PreferenceUtils(this);
        setContentAndActionFragments(ContentFragment.newInstance(
                        getString(R.string.about_preference), null, null, R.drawable.ic_settings_about,
                        getResources().getColor(R.color.icon_background)),
                ActionFragment.newInstance(getActions()));
        mSelectedIndex = 0;
    }

    @Override
    public void onResume() {
        super.onResume();
        mDeveloperClickCount = 0;
		ArrayList<Action> actions = getActionFragment().getArguments().getParcelableArrayList(BaseActionFragment.EXTRA_ACTIONS);
		for(Action action:actions){
			String key = action.getKey();
			if((key!=null)&&(key.equals("name"))){
				action.setDescription(DeviceManager.getDeviceName(this));
				break;
			}
		}
    }

    @Override
    public void onActionFocused(Action action) {
        mSelectedIndex = getActions().indexOf(action);
    }

    @Override
    public void onActionClicked(Action action) {
        final String key = action.getKey();
        if (TextUtils.equals(key, KEY_BUILD)) { 
            mDeveloperClickCount++;
            if (!mPreferenceUtils.isDeveloperEnabled()) {
                int numLeft = NUM_DEVELOPER_CLICKS - mDeveloperClickCount;
                if (numLeft < 3 && numLeft > 0) {
                    showToast(getResources().getQuantityString(
                            R.plurals.show_dev_countdown, numLeft, numLeft));
                }
                if (numLeft == 0) {
                    mPreferenceUtils.setDeveloperEnabled(true);
                    showToast(getString(R.string.show_dev_on));
                    mDeveloperClickCount = 0;
                }
            } else {
                if (mDeveloperClickCount > 3) {
                    showToast(getString(R.string.show_dev_already));
                }
            }
        } else if (TextUtils.equals(key, KEY_VERSION)) { 
            mHits[mHitsIndex] = SystemClock.uptimeMillis();
            mHitsIndex = (mHitsIndex + 1) % mHits.length;
            if (mHits[mHitsIndex] >= SystemClock.uptimeMillis() - 500) {
                Intent intent = new Intent();
                intent.setComponent(mPlatLogoActivity); //快速点击可以显示一个平台logo，这可以改成咱们自己的logo
                startActivity(intent);
            }
        } else if (TextUtils.equals(key, KEY_LEGAL_INFO)) {
            ArrayList<Action> actions = getLegalActions();
            setContentAndActionFragments(ContentFragment.newInstance(
                    getString(R.string.about_legal_info), null, null),
                    ActionFragment.newInstance(actions));
        } else if (TextUtils.equals(key, KEY_REBOOT)) {
            if(isMonkeyRunning())
		return;
            PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
            pm.reboot(null);
        } else if (TextUtils.equals(key, KEY_UPDATE)) {
            if(checkPackage("com.softwinner.settingsassist")&&checkPackage("com.softwinner.TvdFileManager")){
                Intent intent = new Intent(SOFTWINNER_UPDATE_SYSTEM);
                startActivity(intent);
            }else{
                showToast("暂时不支持本地升级,请检查TvdFileManager是否支持RECOVERY");
            }
        }else {
            Intent intent = action.getIntent();
            if (intent != null) {
                try {
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Log.e(TAG, "intent for (" + action.getTitle() + ") not found:", e);
                }
            } else {
                Log.e(TAG, "null intent for: " + action.getTitle());
            }
        }
    }

    /**
     * Returns true if Monkey is running.
     */
    public static boolean isMonkeyRunning() {
        return ActivityManager.isUserAMonkey();
    }

    private ArrayList<Action> getLegalActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder()
                .intent(systemIntent(SETTINGS_LEGAL_LICENSE_INTENT_ACTION))
                .title(getString(R.string.about_legal_license))
                .build());
        actions.add(new Action.Builder()
                .intent(systemIntent(SETTINGS_LEGAL_TERMS_OF_SERVICE))
                .title(getString(R.string.about_terms_of_service))
                .build());

        return actions;
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder()
                .key(KEY_UPDATE)
                .title(getString(R.string.about_system_update))
                .build());
        actions.add(new Action.Builder()
                .key("name")
                .title(getString(R.string.device_name))
                .description(DeviceManager.getDeviceName(this))
                .intent(new Intent(SETTINGS_DEVICE_NAME_INTENT_ACTION))
                .build());
        actions.add(new Action.Builder()
                .key(KEY_REBOOT)
                .title(getString(R.string.restart_button_label))
                .build());
        actions.add(new Action.Builder()
                .key(KEY_LEGAL_INFO)
                .title(getString(R.string.about_legal_info))
                .build());
        Intent adsIntent = new Intent();
        adsIntent.setPackage(SETTINGS_ADS_ACTIVITY_PACKAGE);
        adsIntent.setAction(SETTINGS_ADS_ACTIVITY_ACTION);
        adsIntent.addCategory(Intent.CATEGORY_DEFAULT);
        List<ResolveInfo> resolveInfos = getPackageManager().queryIntentActivities(adsIntent,
                PackageManager.MATCH_DEFAULT_ONLY);
        if (!resolveInfos.isEmpty()) {
            // Launch the phone ads id activity.
            actions.add(new Action.Builder()
                    .key("ads")
                    .title(getString(R.string.about_ads))
                    .intent(adsIntent)
                    .enabled(true)
                    .build());
        }
        actions.add(new Action.Builder()
                .key("model")
                .title(getString(R.string.about_model))
                .description(Build.MODEL)
                .enabled(false)
                .build());
        actions.add(new Action.Builder()
                .key(KEY_VERSION)
                .title(getString(R.string.about_version))
                .description(SystemProperties.get("ro.product.firmware","null"))
                .enabled(true)
                .build());
        actions.add(new Action.Builder()
                .key("serial")
                .title(getString(R.string.about_serial))
                .description(Build.SERIAL)
                .enabled(false)
                .build());
        actions.add(new Action.Builder()
                .key(KEY_BUILD)
                .title(getString(R.string.about_build))
                .description(Build.DISPLAY)
                .enabled(true)
                .build());
        return actions;
    }

    private void displayFragment(Fragment fragment) {
        getFragmentManager()
            .beginTransaction()
            .setTransition(FragmentTransaction.TRANSIT_FRAGMENT_OPEN)
            .replace(android.R.id.content, fragment)
            .addToBackStack(null)
            .commit();
    }

    private void showToast(String toastString) {
        if (mToast != null) {
            mToast.cancel();
        }
        mToast = Toast.makeText(this, toastString, Toast.LENGTH_SHORT);
        mToast.show();
    }

    // Returns an Intent for the given action if a system app can handle it, otherwise null.
    private Intent systemIntent(String action) {
        final Intent intent = new Intent(action);

        // Limit the intent to an activity that is in the system image.
        final PackageManager pm = getPackageManager();
        final List<ResolveInfo> activities = pm.queryIntentActivities(intent, 0);
        for (ResolveInfo info : activities) {
            if ((info.activityInfo.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0) {
                if (info.activityInfo.isEnabled()) {
                    intent.setPackage(info.activityInfo.packageName);
                    return intent;
                }
            }
        }
        return null;  // No system image package found.
    }
    
    private boolean checkPackage(String packageName) {  
        if (packageName == null || "".equals(packageName))  
            return false;  
        try {  
            ApplicationInfo info = getPackageManager().getApplicationInfo(  
                    packageName, PackageManager.GET_UNINSTALLED_PACKAGES);  
            return true;  
        } catch (NameNotFoundException e) {  
            return false;  
        }  
    }  
}
