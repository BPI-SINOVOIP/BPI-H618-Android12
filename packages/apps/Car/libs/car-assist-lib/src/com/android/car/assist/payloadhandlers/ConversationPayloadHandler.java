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

package com.android.car.assist.payloadhandlers;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.RemoteAction;
import android.app.RemoteInput;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationCompat.Action;
import androidx.core.app.NotificationCompat.Action.SemanticAction;
import androidx.core.app.NotificationCompat.MessagingStyle;
import androidx.core.graphics.drawable.IconCompat;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.common.Conversation.ConversationAction.ActionType;

/**
 * Util class that provides a conversion between {@link Conversation} and {@link
 * Notification}
 */
public class ConversationPayloadHandler {

    private ConversationPayloadHandler() {
    }

    /**
     * Returns the package name from the pending intent
     */
    @SuppressWarnings("PendingIntentCreator")
    @NonNull
    public static String getPackageName(@NonNull PendingIntent pendingIntent) {
        return pendingIntent.getCreatorPackage();
    }

    /**
     * Creates a notification from {@link Conversation}
     */
    @NonNull
    public static Notification createNotificationFromConversation(
            @NonNull Context context,
            @NonNull String channelId,
            @NonNull Conversation conversation,
            @DrawableRes int iconRes,
            @Nullable String group) {
        MessagingStyle messagingStyle = getMessagingStyle(conversation);
        Action muteAction = getNotificationAction(context, conversation,
                ActionType.ACTION_TYPE_MUTE);
        Action markAsReadAction = getNotificationAction(context, conversation,
                ActionType.ACTION_TYPE_MARK_AS_READ);
        Action replyAction = getNotificationAction(context, conversation,
                ActionType.ACTION_TYPE_REPLY);

        return new NotificationCompat.Builder(context, channelId)
                .setStyle(messagingStyle)
                .setSmallIcon(iconRes)
                .setLargeIcon(getBitmap(conversation.getConversationIcon(), context))
                .addAction(replyAction)
                .addAction(markAsReadAction)
                .addAction(muteAction)
                .setCategory(NotificationCompat.CATEGORY_MESSAGE)
                .setGroup(group)
                .build();
    }

    @Nullable
    private static Action getNotificationAction(
            @NonNull Context context,
            @NonNull Conversation conversation,
            @ActionType int actionType) {
        Conversation.ConversationAction conversationAction =
                getConversationAction(conversation, actionType);
        Action notificationAction = null;
        if (conversationAction != null) {
            RemoteAction remoteAction = conversationAction.getRemoteAction();
            IconCompat icon =
                    IconCompat.createFromIcon(context, remoteAction.getIcon());
            Action.Builder builder =
                    new Action.Builder(
                            /* icon= */ icon,
                            /* charSequence= */ remoteAction.getTitle(),
                            remoteAction.getActionIntent())
                            .setShowsUserInterface(false)
                            .setSemanticAction(getSemanticAction(actionType));
            if (conversationAction.getRemoteInput() != null) {
                builder.addRemoteInput(toCompat(conversationAction.getRemoteInput()));
            }
            notificationAction = builder.build();
        }
        return notificationAction;
    }

    @SemanticAction
    private static int getSemanticAction(@ActionType int actionType) {
        switch (actionType) {
            case ActionType.ACTION_TYPE_MUTE:
                return Action.SEMANTIC_ACTION_MUTE;
            case ActionType.ACTION_TYPE_MARK_AS_READ:
                return Action.SEMANTIC_ACTION_MARK_AS_READ;
            case ActionType.ACTION_TYPE_REPLY:
                return Action.SEMANTIC_ACTION_REPLY;
            default:
                return Action.SEMANTIC_ACTION_NONE;
        }
    }

    private static MessagingStyle getMessagingStyle(
            @NonNull Conversation conversation) {
        MessagingStyle messagingStyle =
                new MessagingStyle(conversation.getUser());
        messagingStyle.setGroupConversation(isGroupConversation(conversation));
        messagingStyle.setConversationTitle(conversation.getConversationTitle());
        for (Conversation.Message message : conversation.getMessages()) {
            messagingStyle.addMessage(
                    new MessagingStyle.Message(
                            message.getText(), message.getTimestamp(), message.getPerson()));
        }
        return messagingStyle;
    }

    @Nullable
    private static Bitmap getBitmap(@Nullable IconCompat icon, Context context) {
        if (icon == null) {
            return null;
        }
        Drawable drawable = icon.loadDrawable(context);
        if (drawable instanceof BitmapDrawable) {
            return ((BitmapDrawable) drawable).getBitmap();
        } else {
            return null;
        }
    }

    private static boolean isGroupConversation(Conversation conversation) {
        if (conversation.getParticipants().contains(conversation.getUser())) {
            return conversation.getParticipants().size() > 2;
        } else {
            return conversation.getParticipants().size() > 1;
        }
    }

    @Nullable
    private static Conversation.ConversationAction getConversationAction(
            Conversation conversation, @ActionType int action) {
        if (conversation.getActions() == null) {
            return null;
        }
        return conversation.getActions().stream()
                .filter(it -> it.getActionType() == action)
                .findFirst()
                .orElse(null);
    }

    // Conversation classes use android.app.RemoteInput
    // instead of androidx.core.app.RemoteInput in order to support
    // being parcelable. While Notification uses androidx.core.app.RemoteInput.
    // We used these methods to convert between the two.
    private static androidx.core.app.RemoteInput toCompat(RemoteInput src) {
        androidx.core.app.RemoteInput.Builder builder =
                new androidx.core.app.RemoteInput.Builder(src.getResultKey())
                        .setLabel(src.getLabel())
                        .addExtras(src.getExtras());
        return builder.build();
    }
}
