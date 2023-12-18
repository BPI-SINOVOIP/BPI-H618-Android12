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

package com.android.car.settings.notifications;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.graphics.drawable.Drawable;

import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import com.android.car.settings.applications.ApplicationDetailsFragment;
import com.android.car.settings.applications.ApplicationListItemManager;
import com.android.car.settings.common.FragmentController;
import com.android.car.ui.preference.CarUiTwoActionSwitchPreference;
import com.android.settingslib.applications.ApplicationsState;

import java.util.ArrayList;

/**
 * Controller for of list of preferences that enable / disable showing notifications for an
 * application.
 */
public class NotificationsAppListPreferenceController extends
        BaseNotificationsPreferenceController<PreferenceCategory> implements
        ApplicationListItemManager.AppListItemListener {

    private NotificationsFragment.NotificationSwitchListener mNotificationSwitchListener;

    public NotificationsAppListPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    public void setNotificationSwitchListener(
            NotificationsFragment.NotificationSwitchListener listener) {
        mNotificationSwitchListener = listener;
    }

    @Override
    protected Class<PreferenceCategory> getPreferenceType() {
        return PreferenceCategory.class;
    }

    @Override
    public void onDataLoaded(ArrayList<ApplicationsState.AppEntry> apps) {
        getPreference().removeAll();
        for (ApplicationsState.AppEntry appEntry : apps) {
            getPreference().addPreference(
                    createPreference(appEntry.label, appEntry.icon,
                            appEntry.info.packageName, appEntry.info.uid));
        }
    }

    private Preference createPreference(String title, Drawable icon, String packageName, int uid) {
        CarUiTwoActionSwitchPreference preference =
                new CarUiTwoActionSwitchPreference(getContext());
        preference.setTitle(title);
        preference.setIcon(icon);
        preference.setKey(packageName);
        preference.setOnPreferenceClickListener(p -> {
            getFragmentController().launchFragment(
                    ApplicationDetailsFragment.getInstance(packageName));
            return true;
        });

        preference.setOnSecondaryActionClickListener((newValue) -> {
            toggleNotificationsSetting(packageName, uid, newValue);
            if (mNotificationSwitchListener != null) {
                mNotificationSwitchListener.onSwitchChanged();
            }
        });
        preference.setSecondaryActionChecked(areNotificationsEnabled(packageName, uid));

        return preference;
    }
}
