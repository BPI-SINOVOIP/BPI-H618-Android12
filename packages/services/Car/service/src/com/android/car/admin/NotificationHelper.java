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

package com.android.car.admin;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.BOILERPLATE_CODE;

import android.annotation.NonNull;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Bundle;

import com.android.car.R;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.VisibleForTesting;

import java.util.Objects;

// TODO: move this class to CarSettings or at least to some common package (not admin)

/**
 * Helper for notification-related tasks
 */
public final class NotificationHelper {

    static final int FACTORY_RESET_NOTIFICATION_ID = 42;

    static final int NEW_USER_DISCLAIMER_NOTIFICATION_ID = 108;

    @VisibleForTesting
    static final String CHANNEL_ID_DEFAULT = "channel_id_default";
    @VisibleForTesting
    static final String CHANNEL_ID_HIGH = "channel_id_high";

    /**
     * Creates a notification (and its notification channel) for the given importance type, setting
     * its name to be {@code Android System}.
     *
     * @param context context for showing the notification
     * @param importance notification importance. Currently only
     * {@link NotificationManager.IMPORTANCE_HIGH} is supported.
     */
    @NonNull
    public static Notification.Builder newNotificationBuilder(Context context,
            @NotificationManager.Importance int importance) {
        Objects.requireNonNull(context, "context cannot be null");

        String channelId, importanceName;
        switch (importance) {
            case NotificationManager.IMPORTANCE_DEFAULT:
                channelId = CHANNEL_ID_DEFAULT;
                importanceName = context.getString(R.string.importance_default);
                break;
            case NotificationManager.IMPORTANCE_HIGH:
                channelId = CHANNEL_ID_HIGH;
                importanceName = context.getString(R.string.importance_high);
                break;
            default:
                throw new IllegalArgumentException("Unsupported importance: " + importance);
        }
        NotificationManager notificationMgr = context.getSystemService(NotificationManager.class);
        notificationMgr.createNotificationChannel(
                new NotificationChannel(channelId, importanceName, importance));

        Bundle extras = new Bundle();
        extras.putString(Notification.EXTRA_SUBSTITUTE_APP_NAME,
                context.getString(com.android.internal.R.string.android_system_label));

        return new Notification.Builder(context, channelId).addExtras(extras);
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = BOILERPLATE_CODE,
            details = "private constructor")
    private NotificationHelper() {
        throw new UnsupportedOperationException("Contains only static methods");
    }
}
