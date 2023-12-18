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

import static android.app.NotificationManager.IMPORTANCE_NONE;
import static android.app.NotificationManager.IMPORTANCE_UNSPECIFIED;

import android.app.INotificationManager;
import android.app.NotificationChannel;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.ServiceManager;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.Logger;
import com.android.car.settings.common.PreferenceController;

/**
 * Base notifications class that handles checking and changing notification availability
 *
 * @param <T> The upper bound on the type of {@link Preference} on which the controller
 *            expects to operate.
 */
public abstract class BaseNotificationsPreferenceController<T extends Preference> extends
        PreferenceController<T> {

    private static final Logger LOG = new Logger(BaseNotificationsPreferenceController.class);

    @VisibleForTesting
    public INotificationManager mNotificationManager = INotificationManager.Stub.asInterface(
            ServiceManager.getService(Context.NOTIFICATION_SERVICE));

    public BaseNotificationsPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    /**
     * Changes the notifications availability of the specified app
     *
     * @param packageName Package name of app
     * @param uid Uid of app
     * @param enabled Whether to enable or disable the notification
     * @return Whether changing the notification availability succeeded or not
     */
    public boolean toggleNotificationsSetting(String packageName, int uid, boolean enabled) {
        try {
            if (mNotificationManager.onlyHasDefaultChannel(packageName, uid)) {
                NotificationChannel defaultChannel =
                        mNotificationManager.getNotificationChannelForPackage(
                                packageName,
                                uid,
                                NotificationChannel.DEFAULT_CHANNEL_ID,
                                /* conversationId= */ null,
                                /* includeDeleted= */ true);
                defaultChannel.setImportance(enabled ? IMPORTANCE_UNSPECIFIED : IMPORTANCE_NONE);
                mNotificationManager
                        .updateNotificationChannelForPackage(packageName, uid, defaultChannel);
            }
            mNotificationManager.setNotificationsEnabledForPackage(packageName, uid, enabled);
        } catch (Exception e) {
            LOG.w("Error querying notification setting for package");
            return false;
        }
        return true;
    }

    /**
     * Checks whether notifications are enabled for specified app
     *
     * @param packageName Package name of the app
     * @param uid Uid of the app
     * @return Whether notifications are enabled for the specified app
     */
    public boolean areNotificationsEnabled(String packageName, int uid) {
        try {
            return mNotificationManager.areNotificationsEnabledForPackage(packageName, uid);
        } catch (Exception e) {
            LOG.w("Error querying notification setting for package");
            return false;
        }
    }
}
