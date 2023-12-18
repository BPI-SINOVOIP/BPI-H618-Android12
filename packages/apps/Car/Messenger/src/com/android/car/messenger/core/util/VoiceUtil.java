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
package com.android.car.messenger.core.util;

import static com.android.car.assist.CarVoiceInteractionSession.KEY_ACTION;
import static com.android.car.assist.CarVoiceInteractionSession.KEY_CONVERSATION;
import static com.android.car.assist.CarVoiceInteractionSession.KEY_DEVICE_ADDRESS;
import static com.android.car.assist.CarVoiceInteractionSession.KEY_DEVICE_NAME;
import static com.android.car.assist.CarVoiceInteractionSession.KEY_NOTIFICATION;
import static com.android.car.assist.CarVoiceInteractionSession.KEY_PHONE_NUMBER;
import static com.android.car.assist.CarVoiceInteractionSession.KEY_SEND_PENDING_INTENT;
import static com.android.car.assist.CarVoiceInteractionSession.VOICE_ACTION_READ_CONVERSATION;
import static com.android.car.assist.CarVoiceInteractionSession.VOICE_ACTION_READ_NOTIFICATION;
import static com.android.car.assist.CarVoiceInteractionSession.VOICE_ACTION_REPLY_CONVERSATION;
import static com.android.car.assist.CarVoiceInteractionSession.VOICE_ACTION_REPLY_NOTIFICATION;
import static com.android.car.assist.CarVoiceInteractionSession.VOICE_ACTION_SEND_SMS;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_DIRECT_SEND;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_MARK_AS_READ;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_MUTE;
import static com.android.car.messenger.core.shared.MessageConstants.ACTION_REPLY;
import static com.android.car.messenger.core.shared.MessageConstants.EXTRA_ACCOUNT_ID;
import static com.android.car.messenger.core.shared.MessageConstants.EXTRA_CONVERSATION_KEY;

import android.app.Activity;
import android.app.PendingIntent;
import android.app.RemoteAction;
import android.app.RemoteInput;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.Bundle;
import android.service.notification.StatusBarNotification;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.R;
import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.common.Conversation.ConversationAction;
import com.android.car.messenger.common.Conversation.ConversationAction.ActionType;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.core.service.MessengerService;
import com.android.car.messenger.core.shared.MessageConstants;
import com.android.car.messenger.core.shared.NotificationHandler;

import java.util.ArrayList;
import java.util.List;

/** Voice Util classes for requesting voice interactions and responding to voice actions */
public class VoiceUtil {

    /** Represents a null user account id */
    private static final int NULL_ACCOUNT_ID = 0;

    private VoiceUtil() {}

    /** Requests Voice request to read a conversation */
    public static void voiceRequestReadConversation(
            @NonNull Activity activity,
            @NonNull UserAccount userAccount,
            @NonNull Conversation conversation) {
        if (conversation.getMessages().isEmpty()) {
            L.d("No messages to read from Conversation! Returning.");
            return;
        }
        voiceRequestHelper(
                activity,
                conversation,
                userAccount,
                VOICE_ACTION_READ_CONVERSATION,
                VOICE_ACTION_READ_NOTIFICATION);
    }

    /** Requests Voice request to reply to a conversation */
    public static void voiceRequestReplyConversation(
            @NonNull Activity activity,
            @NonNull UserAccount userAccount,
            @NonNull Conversation conversation) {
        voiceRequestHelper(
                activity,
                conversation,
                userAccount,
                VOICE_ACTION_REPLY_CONVERSATION,
                VOICE_ACTION_REPLY_NOTIFICATION);
    }

    private static void voiceRequestHelper(
            @NonNull Activity activity,
            @NonNull Conversation conversation,
            @NonNull UserAccount userAccount,
            @NonNull String conversationAction,
            @NonNull String notificationAction) {
        Bundle args = new Bundle();
        Conversation tapToReadConversation =
                createTapToReadConversation(conversation, userAccount.getId());
        boolean isConversationSupported =
                activity.getResources().getBoolean(R.bool.ttr_conversation_supported);
        if (isConversationSupported) {
            // New API using generic Conversation class
            // is currently limited in support by partner assistants and is being phased in.
            args.putString(KEY_ACTION, conversationAction);
            args.putBundle(KEY_CONVERSATION, tapToReadConversation.toBundle());
        } else {
            // Continue using legacy SBN
            StatusBarNotification sbn =
                    NotificationHandler.postNotificationForLegacyTapToRead(tapToReadConversation);
            if (sbn == null) {
                L.e("Failed to convert Conversation to SBN for Legacy Tap To Read.");
                return;
            }
            args.putString(KEY_ACTION, notificationAction);
            args.putParcelable(KEY_NOTIFICATION, sbn);
        }

        activity.showAssist(args);
    }

    /** Requests Voice request to start a generic compose voice interaction */
    public static void voiceRequestGenericCompose(Activity activity, UserAccount userAccount) {
        Bundle bundle = new Bundle();
        bundle.putString(KEY_ACTION, VOICE_ACTION_SEND_SMS);
        bundle.putString(KEY_DEVICE_ADDRESS, userAccount.getIccId());
        bundle.putString(KEY_DEVICE_NAME, userAccount.getName());
        PendingIntent sendIntent =
                createServiceIntent(
                        ACTION_DIRECT_SEND, /* conversationKey= */ null, userAccount.getId());
        bundle.putParcelable(KEY_SEND_PENDING_INTENT, sendIntent);
        activity.showAssist(bundle);
    }

    /**
     * Returns a new conversation containing the tap to read pending intents to be transferred over
     * to the Voice Assistant.
     *
     * <p>The conversation object returned remained unmodified.
     *
     * <p>This is important to allow the Assistant have a different instance than the one that
     * powers our UI. We can create new pending intents without modifying the instance the Assistant
     * holds.
     *
     * @return new conversation instance with the same data and pending intents for tap to read.
     */
    public static Conversation createTapToReadConversation(
            Conversation conversation, int userAccountId) {
        Context context = AppFactory.get().getContext();
        String conversationKey = conversation.getId();
        Conversation.Builder builder = conversation.toBuilder();

        final int replyIcon = R.drawable.car_ui_icon_reply;
        final String replyString = context.getString(R.string.action_reply);
        PendingIntent replyIntent =
                createServiceIntent(ACTION_REPLY, conversationKey, userAccountId);
        ConversationAction replyAction =
                new ConversationAction(
                        ActionType.ACTION_TYPE_REPLY,
                        new RemoteAction(
                                Icon.createWithResource(context, replyIcon),
                                replyString,
                                replyString,
                                replyIntent),
                        new RemoteInput.Builder(Intent.EXTRA_TEXT).build());

        final int markAsReadIcon = android.R.drawable.ic_media_play;
        final String markAsReadString = context.getString(R.string.action_mark_as_read);
        PendingIntent markAsReadIntent =
                createServiceIntent(ACTION_MARK_AS_READ, conversationKey, userAccountId);
        ConversationAction markAsReadAction =
                new ConversationAction(
                        ActionType.ACTION_TYPE_MARK_AS_READ,
                        new RemoteAction(
                                Icon.createWithResource(context, markAsReadIcon),
                                markAsReadString,
                                markAsReadString,
                                markAsReadIntent),
                        null);

        final int muteIcon = R.drawable.car_ui_icon_toggle_mute;
        final String muteString = context.getString(R.string.action_mute);
        PendingIntent muteIntent = createServiceIntent(ACTION_MUTE, conversationKey, userAccountId);
        ConversationAction muteAction =
                new ConversationAction(
                        ActionType.ACTION_TYPE_MUTE,
                        new RemoteAction(
                                Icon.createWithResource(context, muteIcon),
                                muteString,
                                muteString,
                                muteIntent),
                        null);

        List<ConversationAction> actions = new ArrayList<>();
        actions.add(replyAction);
        actions.add(markAsReadAction);
        actions.add(muteAction);
        builder.setActions(actions);
        return builder.build();
    }

    private static PendingIntent createServiceIntent(
            @NonNull String action, @Nullable String conversationKey, int userAccountId) {
        Context context = AppFactory.get().getContext();
        Bundle bundle = new Bundle();
        if (conversationKey != null) {
            bundle.putString(EXTRA_CONVERSATION_KEY, conversationKey);
        }
        bundle.putInt(EXTRA_ACCOUNT_ID, userAccountId);
        Intent intent =
                new Intent(context, MessengerService.class)
                        .setAction(action)
                        .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP)
                        .setClass(context, MessengerService.class)
                        .putExtras(bundle);

        int requestCode =
                (conversationKey == null) ? action.hashCode() : conversationKey.hashCode();
        return PendingIntent.getForegroundService(
                context,
                requestCode,
                intent,
                PendingIntent.FLAG_MUTABLE | PendingIntent.FLAG_ONE_SHOT);
    }

    /** Sends a reply, meant to be used from a caller originating from voice input. */
    public static void directSend(Intent intent) {
        final CharSequence phoneNumber = intent.getCharSequenceExtra(KEY_PHONE_NUMBER);
        final String iccId = intent.getStringExtra(KEY_DEVICE_ADDRESS);
        final CharSequence message = intent.getCharSequenceExtra(Intent.EXTRA_TEXT);
        if (iccId == null || phoneNumber == null || TextUtils.isEmpty(message)) {
            L.e("Dropping voice reply. Received no icc id, phone Number and/or empty message!");
            return;
        }
        L.d("Sending a message to specified phone number");
        AppFactory.get()
                .getDataModel()
                .sendMessage(iccId, phoneNumber.toString(), message.toString());
    }

    /** Sends a reply, meant to be used from a caller originating from voice input. */
    public static void voiceReply(Intent intent) {
        final String conversationKey = intent.getStringExtra(EXTRA_CONVERSATION_KEY);
        final int accountId =
                intent.getIntExtra(MessageConstants.EXTRA_ACCOUNT_ID, NULL_ACCOUNT_ID);
        final Bundle bundle = RemoteInput.getResultsFromIntent(intent);
        if (bundle == null || accountId == NULL_ACCOUNT_ID) {
            L.e("Dropping voice reply. Received null bundle or no user account id in bundle!");
            return;
        }
        final CharSequence message = bundle.getCharSequence(Intent.EXTRA_TEXT);
        L.d("voiceReply: " + message);
        if (!TextUtils.isEmpty(message)) {
            AppFactory.get()
                    .getDataModel()
                    .replyConversation(accountId, conversationKey, message.toString());
        }
    }

    /** Mark a conversation associated with a given sender key as read. */
    public static void mute(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras != null) {
            final String conversationKey = extras.getString(EXTRA_CONVERSATION_KEY);
            L.d("mute");
            AppFactory.get().getDataModel().muteConversation(conversationKey, true);
        }
    }

    /** Mark a conversation associated with a given sender key as read. */
    public static void markAsRead(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras != null) {
            final String conversationKey = extras.getString(EXTRA_CONVERSATION_KEY);
            L.d("marking as read");
            AppFactory.get().getDataModel().markAsRead(conversationKey);
        }
    }
}
