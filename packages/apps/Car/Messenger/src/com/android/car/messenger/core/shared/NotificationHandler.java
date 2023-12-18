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

package com.android.car.messenger.core.shared;

import static com.android.car.messenger.core.shared.MessageConstants.EXTRA_ACCOUNT_ID;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.service.notification.StatusBarNotification;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.assist.payloadhandlers.ConversationPayloadHandler;
import com.android.car.messenger.R;
import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.service.MessengerService;
import com.android.car.messenger.core.util.L;
import com.android.car.messenger.core.util.VoiceUtil;

/** Useful notification handler for posting messages */
public class NotificationHandler {
    @NonNull
    private static final String GROUP_TAP_TO_READ_NOTIFICATION =
            "com.android.car.messenger.TAP_TO_READ";

    private static final int TAP_TO_READ_SBN_ATTEMPT_LIMIT = 3;

    private NotificationHandler() {}

    /** Posts, removes or updates a notification based on a conversation */
    public static void postOrRemoveNotification(@NonNull Conversation conversation) {
        if (conversation.isMuted()) {
            removeNotification(conversation.getId());
        } else {
            postNotification(conversation);
        }
    }

    /* Posts or updates a notification based on a conversation */
    private static void postNotification(Conversation conversation) {
        int userAccountId = conversation.getExtras().getInt(EXTRA_ACCOUNT_ID, 0);
        if (userAccountId == 0) {
            L.w(
                    "posting Notification with null user account id. "
                            + "Note, reply would likely fail if user account id is not set.");
        }
        Conversation tapToReadConversation =
                VoiceUtil.createTapToReadConversation(conversation, userAccountId);
        Context context = AppFactory.get().getContext();
        NotificationManager notificationManager =
                context.getSystemService(NotificationManager.class);
        String channelId = MessengerService.MESSAGE_CHANNEL_ID;
        Notification notification =
                ConversationPayloadHandler.createNotificationFromConversation(
                        context, channelId, tapToReadConversation, R.drawable.ic_message, null);

        notificationManager.notify(tapToReadConversation.getId().hashCode(), notification);
    }

    /**
     * Posts a notification in the foreground for Tap To Read
     *
     * <p>This is useful as legacy digital assistant implementations of Tap To Read require a {@link
     * StatusBarNotification} in order to fulfill a tap to read request.
     *
     * <p>This notification is invisible to the user but accessible by digital assistants.
     *
     * @return the StatusBarNotification posted by the system for this notification, or null if not
     *     found after a limited attempt at retrieval
     */
    @Nullable
    public static StatusBarNotification postNotificationForLegacyTapToRead(
            @NonNull Conversation tapToReadConversation) {
        Context context = AppFactory.get().getContext();
        // cancel any other notifications within group.
        // There should be only notification in group at a time.
        cancelAllTapToReadNotifications(context);
        // Post as a foreground service:
        // Foreground notifications by system apps with low priority
        // are hidden from user view, which is desired
        Notification notification =
                ConversationPayloadHandler.createNotificationFromConversation(
                        context,
                        MessengerService.APP_RUNNING_CHANNEL_ID,
                        tapToReadConversation,
                        context.getApplicationInfo().icon,
                        GROUP_TAP_TO_READ_NOTIFICATION);
        int id = (GROUP_TAP_TO_READ_NOTIFICATION + tapToReadConversation.getId()).hashCode();
        NotificationManager notificationManager =
                context.getSystemService(NotificationManager.class);
        notificationManager.notify(id, notification);

        // attempt to retrieve the status bar notification based on the notification
        // limit attempts
        int tries = 0;
        StatusBarNotification sbn;
        do {
            sbn = findSBN(notificationManager, id);
            tries++;
        } while (sbn == null && tries < TAP_TO_READ_SBN_ATTEMPT_LIMIT);
        return sbn;
    }

    /** Cancels all Tap To Read Notifications */
    public static void cancelAllTapToReadNotifications(@NonNull Context context) {
        NotificationManager notificationManager =
                context.getSystemService(NotificationManager.class);
        for (StatusBarNotification sbn : notificationManager.getActiveNotifications()) {
            if (GROUP_TAP_TO_READ_NOTIFICATION.equals(sbn.getNotification().getGroup())) {
                notificationManager.cancel(sbn.getId());
            }
        }
    }

    /** Returns the {@link StatusBarNotification} with desired id, or null if none found */
    private static StatusBarNotification findSBN(
            @NonNull NotificationManager notificationManager, int id) {
        for (StatusBarNotification sbn : notificationManager.getActiveNotifications()) {
            if (sbn.getId() == id) {
                return sbn;
            }
        }
        return null;
    }

    /** Removes a notification based on a conversation */
    public static void removeNotification(@NonNull String conversationId) {
        Context context = AppFactory.get().getContext();
        NotificationManager notificationManager =
                context.getSystemService(NotificationManager.class);
        notificationManager.cancel(conversationId.hashCode());
    }
}
