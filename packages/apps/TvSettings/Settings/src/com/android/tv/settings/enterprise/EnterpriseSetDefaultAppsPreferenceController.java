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

package com.android.tv.settings.enterprise;

import android.content.Context;
import android.os.UserHandle;
import android.os.UserManager;

import androidx.preference.Preference;

import com.android.settingslib.core.AbstractPreferenceController;
import com.android.tv.settings.R;
import com.android.tv.settings.enterprise.apps.ApplicationFeatureProvider;
import com.android.tv.settings.enterprise.apps.EnterpriseDefaultApps;
import com.android.tv.settings.overlay.FlavorUtils;

/**
 * Forked from:
 * Settings/src/com/android/settings/enterprise/EnterpriseSetDefaultAppsPreferenceController.java
 */
public class EnterpriseSetDefaultAppsPreferenceController extends AbstractPreferenceController {

    private static final String KEY_DEFAULT_APPS = "number_enterprise_set_default_apps";
    private final ApplicationFeatureProvider mApplicationFeatureProvider;
    private final UserManager mUserManager;

    public EnterpriseSetDefaultAppsPreferenceController(Context context) {
        super(context);
        mApplicationFeatureProvider = FlavorUtils.getFeatureFactory(
                context).getApplicationFeatureProvider(
                context);
        mUserManager = context.getSystemService(UserManager.class);
    }

    @Override
    public void updateState(Preference preference) {
        final int num = getNumberOfEnterpriseSetDefaultApps();
        preference.setSummary(mContext.getResources().getQuantityString(
                R.plurals.enterprise_privacy_number_packages, num, num));
    }

    @Override
    public boolean isAvailable() {
        return getNumberOfEnterpriseSetDefaultApps() > 0;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_DEFAULT_APPS;
    }

    private int getNumberOfEnterpriseSetDefaultApps() {
        int num = 0;

        for (UserHandle user : mUserManager.getUserProfiles()) {
            for (EnterpriseDefaultApps app : EnterpriseDefaultApps.values()) {
                num += mApplicationFeatureProvider
                        .findPersistentPreferredActivities(user.getIdentifier(),
                                app.getIntents()).size();
            }
        }
        return num;
    }
}
