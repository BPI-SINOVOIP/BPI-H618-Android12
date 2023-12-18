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

import static android.car.drivingstate.CarDrivingStateEvent.DRIVING_STATE_PARKED;

import static com.android.car.admin.NotificationHelper.FACTORY_RESET_NOTIFICATION_ID;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.car.Car;
import android.car.drivingstate.CarDrivingStateEvent;
import android.car.drivingstate.CarDrivingStateManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.UserHandle;
import android.util.Slog;
import android.widget.Button;
import android.widget.Toast;

import com.android.car.CarLog;
import com.android.car.R;
import com.android.internal.os.IResultReceiver;

// TODO(b/171603586): STOPSHIP move this class to CarSettings
/**
 * Activity shown when a factory request is imminent, it gives the user the option to reset now or
 * wait until the device is rebooted / resumed from suspend.
 */
public final class FactoryResetActivity extends Activity {

    private static final String TAG = CarLog.tagFor(FactoryResetActivity.class);

    public static final String EXTRA_CALLBACK = "factory_reset_callback";

    private Button mNowButton;
    private Button mLaterButton;
    private IResultReceiver mCallback;

    private Car mCar;
    private CarDrivingStateManager mCarDrivingStateManager;
    /**
     * Sends the notification warning the user about the factory reset.
     */
    public static void sendNotification(Context context, IResultReceiver callback) {
        // The factory request is received by CarService - which runs on system user - but the
        // notification will be sent to all users.
        UserHandle currentUser = UserHandle.of(ActivityManager.getCurrentUser());

        @SuppressWarnings("deprecation")
        Intent intent = new Intent(context, FactoryResetActivity.class)
                .putExtra(EXTRA_CALLBACK, callback.asBinder());
        PendingIntent pendingIntent = PendingIntent.getActivityAsUser(context,
                FACTORY_RESET_NOTIFICATION_ID, intent, PendingIntent.FLAG_IMMUTABLE,
                /* options= */ null, currentUser);

        // TODO (b/13679261) allows OEM to customize the package name shown in notification
        Notification notification = NotificationHelper
                .newNotificationBuilder(context, NotificationManager.IMPORTANCE_HIGH)
                .setSmallIcon(R.drawable.car_ic_warning)
                .setColor(context.getColor(R.color.red_warning))
                .setContentTitle(context.getString(R.string.factory_reset_notification_title))
                .setContentText(context.getString(R.string.factory_reset_notification_text))
                .setCategory(Notification.CATEGORY_CAR_WARNING)
                .setOngoing(true)
                .addAction(/* icon= */ 0,
                        context.getString(R.string.factory_reset_notification_button),
                        pendingIntent)
                .build();

        Slog.i(TAG, "Showing factory reset notification on all users");
        context.getSystemService(NotificationManager.class)
                .notifyAsUser(TAG, FACTORY_RESET_NOTIFICATION_ID, notification, UserHandle.ALL);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        Object binder = null;

        try {
            binder = intent.getExtra(EXTRA_CALLBACK);
            mCallback = IResultReceiver.Stub.asInterface((IBinder) binder);
        } catch (Exception e) {
            Slog.w(TAG, "error getting IResultReveiver from " + EXTRA_CALLBACK + " extra ("
                    + binder + ") on " + intent, e);
        }

        if (mCallback == null) {
            Slog.wtf(TAG, "no IResultReceiver / " + EXTRA_CALLBACK + " extra  on " + intent);
            finish();
            return;
        }

        // Connect to car service
        mCar = Car.createCar(this);
        mCarDrivingStateManager = (CarDrivingStateManager) mCar.getCarManager(
                Car.CAR_DRIVING_STATE_SERVICE);
        showMore();
    }

    @Override
    protected void onStop() {
        super.onStop();
        finish();
    }

    private void showMore() {
        CarDrivingStateEvent state = mCarDrivingStateManager.getCurrentCarDrivingState();
        switch (state.eventValue) {
            case DRIVING_STATE_PARKED:
                showFactoryResetDialog();
                break;
            default:
                showFactoryResetToast();
        }
    }

    private void showFactoryResetDialog() {
        // TODO(b/171603586): STOPSHIP use Chassis library after moving this class to CarSettings
        AlertDialog dialog = new AlertDialog.Builder(/* context= */ this,
                        com.android.internal.R.style.Theme_DeviceDefault_Dialog_Alert)
                .setTitle(R.string.factory_reset_parked_title)
                .setMessage(R.string.factory_reset_parked_text)
                .setPositiveButton(R.string.factory_reset_later_button,
                        (d, which) -> factoryResetLater())
                .setNegativeButton(R.string.factory_reset_now_button,
                        (d, which) -> factoryResetNow())
                .setCancelable(false)
                .setOnDismissListener((d) -> finish())
                .create();

        dialog.show();
    }

    private void showFactoryResetToast() {
        showToast(R.string.factory_reset_driving_text);
        finish();
    }

    private void factoryResetNow() {
        Slog.i(TAG, "Factory reset acknowledged; finishing it");

        try {
            mCallback.send(/* resultCode= */ 0, /* resultData= */ null);

            // Cancel pending intent and notification
            getSystemService(NotificationManager.class).cancel(FACTORY_RESET_NOTIFICATION_ID);
            PendingIntent.getActivity(this, FACTORY_RESET_NOTIFICATION_ID, getIntent(),
                    PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT).cancel();
        } catch (Exception e) {
            Slog.e(TAG, "error factory resetting or cancelling notification / intent", e);
            return;
        } finally {
            finish();
        }
    }

    private void factoryResetLater() {
        Slog.i(TAG, "Delaying factory reset.");
        showToast(R.string.factory_reset_later_text);
        finish();
    }

    private void showToast(int resId) {
        Toast.makeText(this, resId, Toast.LENGTH_LONG).show();
    }
}
