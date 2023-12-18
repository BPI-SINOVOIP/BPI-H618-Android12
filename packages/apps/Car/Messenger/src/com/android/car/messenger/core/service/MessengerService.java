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

package com.android.car.messenger.core.service;

import static com.android.car.messenger.core.shared.MessageConstants.ACTION_DIRECT_SEND;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_MARK_AS_READ;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_MUTE;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_REPLY;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.media.AudioAttributes;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.provider.Settings;
import androidx.core.app.NotificationCompat;
import android.telephony.TelephonyManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.R;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.interfaces.DataModel;
import com.android.car.messenger.core.shared.NotificationHandler;
import com.android.car.messenger.core.util.L;
import com.android.car.messenger.core.util.VoiceUtil;

import java.time.Duration;

/** Service responsible for handling messaging events. */
public class MessengerService extends Service {
    /* ACTIONS */
    /** Used to start this service at boot-complete. Takes no arguments. */
    @NonNull public static final String ACTION_START = "com.android.car.messenger.ACTION_START";

    /* EXTRAS */
    /* NOTIFICATIONS */
    @NonNull public static final String MESSAGE_CHANNEL_ID = "MESSAGE_CHANNEL_ID";
    @NonNull public static final String SILENT_MESSAGE_CHANNEL_ID = "SILENT_MESSAGE_CHANNEL_ID";
    @NonNull public static final String APP_RUNNING_CHANNEL_ID = "APP_RUNNING_CHANNEL_ID";
    private static final int SERVICE_STARTED_NOTIFICATION_ID = Integer.MAX_VALUE;

    /* Binding boilerplate */
    @NonNull private final IBinder mBinder = new LocalBinder();

    /* Delay fetching to give time for the system to start up on boot */
    private static final Duration DELAY_FETCH_DURATION = Duration.ofSeconds(3);

    /** Local Binder For {@link MessengerService} */
    public class LocalBinder extends Binder {
        /** Returns {@link MessengerService} */
        @NonNull
        public MessengerService getService() {
            return MessengerService.this;
        }
    }

    @Override
    @NonNull
    public IBinder onBind(@NonNull Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        L.d("MessengerService - onCreate");
        Handler handler = new Handler();
        handler.postDelayed(this::subscribeToNotificationUpdates, DELAY_FETCH_DURATION.toMillis());

        sendServiceRunningNotification();
    }

    private void subscribeToNotificationUpdates() {
        DataModel dataModel = AppFactory.get().getDataModel();
        dataModel.getUnreadMessages().observeForever(NotificationHandler::postOrRemoveNotification);
        dataModel.onConversationRemoved().observeForever(NotificationHandler::removeNotification);
    }

    private void sendServiceRunningNotification() {
        NotificationManager notificationManager = getSystemService(NotificationManager.class);

        if (notificationManager == null) {
            L.e("Failed to get NotificationManager instance");
            return;
        }

        // Create notification channel for app running notification
        {
            NotificationChannel appRunningNotificationChannel =
                    new NotificationChannel(
                            APP_RUNNING_CHANNEL_ID,
                            getString(R.string.app_running_msg_notification_title),
                            NotificationManager.IMPORTANCE_LOW);
            notificationManager.createNotificationChannel(appRunningNotificationChannel);
        }

        // Create notification channel for notifications that should be posted silently in the
        // notification center, without a heads up notification.
        {
            NotificationChannel silentNotificationChannel =
                    new NotificationChannel(
                            SILENT_MESSAGE_CHANNEL_ID,
                            getString(R.string.message_channel_description),
                            NotificationManager.IMPORTANCE_LOW);
            notificationManager.createNotificationChannel(silentNotificationChannel);
        }

        {
            AudioAttributes attributes =
                    new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_NOTIFICATION)
                            .build();
            NotificationChannel channel =
                    new NotificationChannel(
                            MESSAGE_CHANNEL_ID,
                            getString(R.string.message_channel_name),
                            NotificationManager.IMPORTANCE_HIGH);
            channel.setDescription(getString(R.string.message_channel_description));
            channel.setSound(Settings.System.DEFAULT_NOTIFICATION_URI, attributes);
            notificationManager.createNotificationChannel(channel);
        }

        final Notification notification =
                new NotificationCompat.Builder(this, APP_RUNNING_CHANNEL_ID)
                        .setSmallIcon(R.drawable.ic_message)
                        .setContentTitle(getString(R.string.app_running_msg_notification_title))
                        .setContentText(getString(R.string.app_running_msg_notification_content))
                        .build();

        startForeground(SERVICE_STARTED_NOTIFICATION_ID, notification);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        L.d("onDestroy");
    }

    @Override
    public int onStartCommand(@Nullable Intent intent, int flags, int startId) {
        final int result = START_STICKY;

        if (intent == null || intent.getAction() == null) {
            return result;
        }

        final String action = intent.getAction();
        switch (action) {
            case ACTION_START:
                // NO-OP
                break;
            case ACTION_REPLY:
                VoiceUtil.voiceReply(intent);
                break;
            case ACTION_MUTE:
                VoiceUtil.mute(intent);
                break;
            case ACTION_MARK_AS_READ:
                VoiceUtil.markAsRead(intent);
                break;
            case ACTION_DIRECT_SEND:
                VoiceUtil.directSend(intent);
                break;
            case TelephonyManager.ACTION_RESPOND_VIA_MESSAGE:
                // Not currently supported. This was added to allow CarMessenger become the default
                // SMS app.
                break;
            default:
                L.w("Unsupported action: " + action);
        }
        return result;
    }
}
