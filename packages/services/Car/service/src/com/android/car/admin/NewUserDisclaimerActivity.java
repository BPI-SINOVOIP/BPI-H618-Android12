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

import static com.android.car.admin.CarDevicePolicyService.DEBUG;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Slog;
import android.widget.Button;

import com.android.car.CarLog;
import com.android.car.R;
import com.android.car.admin.ui.ManagedDeviceTextView;
import com.android.internal.annotations.VisibleForTesting;

// TODO(b/171603586): STOPSHIP move UI related activities to CarSettings
/**
 * Shows a disclaimer when a new user is added in a device that is managed by a device owner.
 */
public final class NewUserDisclaimerActivity extends Activity {

    private static final String TAG = CarLog.tagFor(NewUserDisclaimerActivity.class);
    private static final int NOTIFICATION_ID =
            NotificationHelper.NEW_USER_DISCLAIMER_NOTIFICATION_ID;

    private Button mAcceptButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.new_user_disclaimer);

        mAcceptButton = findViewById(R.id.accept_button);
        mAcceptButton.setOnClickListener((v) -> accept());
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (DEBUG) Slog.d(TAG, "showing UI");

        PerUserCarDevicePolicyService.getInstance(this).setShown();

        // TODO(b/175057848): automotically finish the activity at x ms if the user doesn't ack it
        // and/or integrate it with UserNoticeService
    }

    @VisibleForTesting
    Button getAcceptButton() {
        return mAcceptButton;
    }

    private void accept() {
        if (DEBUG) Slog.d(TAG, "user accepted");

        PerUserCarDevicePolicyService.getInstance(this).setAcknowledged();
        finish();
    }

    static void showNotification(Context context) {
        PendingIntent pendingIntent = getPendingIntent(context, /* extraFlags= */ 0);

        Notification notification = NotificationHelper
                .newNotificationBuilder(context, NotificationManager.IMPORTANCE_DEFAULT)
                // TODO(b/175057848): proper icon?
                .setSmallIcon(R.drawable.car_ic_mode)
                .setContentTitle(context.getString(R.string.new_user_managed_notification_title))
                .setContentText(ManagedDeviceTextView.getManagedDeviceText(context))
                .setCategory(Notification.CATEGORY_CAR_INFORMATION)
                .setContentIntent(pendingIntent)
                .setOngoing(true)
                .build();

        if (DEBUG) {
            Slog.d(TAG, "Showing new managed notification (id " + NOTIFICATION_ID + " on user "
                    + context.getUserId());
        }
        context.getSystemService(NotificationManager.class).notify(NOTIFICATION_ID, notification);
    }

    static void cancelNotification(Context context) {
        if (DEBUG) {
            Slog.d(TAG, "Canceling notification " + NOTIFICATION_ID + " for user "
                    + context.getUserId());
        }
        context.getSystemService(NotificationManager.class).cancel(NOTIFICATION_ID);
        getPendingIntent(context, PendingIntent.FLAG_UPDATE_CURRENT).cancel();
    }

    @VisibleForTesting
    static PendingIntent getPendingIntent(Context context, int extraFlags) {
        return PendingIntent.getActivity(context, NOTIFICATION_ID,
                new Intent(context, NewUserDisclaimerActivity.class),
                PendingIntent.FLAG_IMMUTABLE | extraFlags);
    }
}
