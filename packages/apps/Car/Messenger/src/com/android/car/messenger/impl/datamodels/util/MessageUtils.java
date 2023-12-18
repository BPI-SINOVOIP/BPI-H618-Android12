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

package com.android.car.messenger.impl.datamodels.util;

import static com.android.car.messenger.common.Conversation.Message.MessageStatus.MESSAGE_STATUS_NONE;
import static com.android.car.messenger.common.Conversation.Message.MessageStatus.MESSAGE_STATUS_READ;
import static com.android.car.messenger.common.Conversation.Message.MessageStatus.MESSAGE_STATUS_UNREAD;

import static java.util.Comparator.comparingLong;

import android.content.Context;
import android.database.Cursor;
import android.provider.Telephony.TextBasedSmsColumns;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.Person;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.common.Conversation.Message;
import com.android.car.messenger.common.Conversation.Message.MessageStatus;
import com.android.car.messenger.common.Conversation.Message.MessageType;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.util.L;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;

/** Message Parser that provides useful static methods to parse 1-1 and Group MMS messages. */
public final class MessageUtils {

    /**
     * Gets all unread messages in cursor
     *
     * @param messagesCursor The messageCursor in descending order
     */
    @NonNull
    public static List<Message> getUnreadMessages(@Nullable Cursor messagesCursor) {
        List<Message> unreadMessages = new ArrayList<>();
        MessageUtils.forEachDesc(
                messagesCursor,
                message -> {
                    if (message.getMessageStatus() == MessageStatus.MESSAGE_STATUS_UNREAD) {
                        unreadMessages.add(message);
                        return true;
                    }
                    return false;
                });
        unreadMessages.sort(comparingLong(Message::getTimestamp));
        return unreadMessages;
    }

    /**
     * Gets Read Messages and Reply Timestamp.
     *
     * @param messagesCursor MessageCursor in descending order
     */
    @NonNull
    public static Pair<List<Message>, Long> getReadMessagesAndReplyTimestamp(
            @Nullable Cursor messagesCursor) {
        List<Message> readMessages = new ArrayList<>();
        AtomicReference<Long> lastReply = new AtomicReference<>(0L);
        MessageUtils.forEachDesc(
                messagesCursor,
                message -> {
                    // Desired impact: 4. Reply -> 3. Messages -> 2. Reply -> 1 Messages (stop
                    // parsing at 2.)
                    // lastReply references 4., messages references 3.
                    // Desired impact: 3. Messages -> 2. Reply -> 1. Messages (stop parsing at 2.)
                    // lastReply references 2., messages references 3.
                    int messageStatus = message.getMessageStatus();
                    if (message.getMessageType() == MessageType.MESSAGE_TYPE_SENT) {
                        if (lastReply.get() < message.getTimestamp()) {
                            lastReply.set(message.getTimestamp());
                        }
                        return readMessages.isEmpty();
                    }

                    if (messageStatus == MessageStatus.MESSAGE_STATUS_READ
                            || messageStatus == MessageStatus.MESSAGE_STATUS_NONE) {
                        readMessages.add(message);
                        return true;
                    }
                    return false;
                });
        readMessages.sort(comparingLong(Message::getTimestamp));
        return new Pair<>(readMessages, lastReply.get());
    }

    /**
     * Parses each message in the cursor and returns the item for further processing
     *
     * @param messageCursor The message cursor to be parsed for SMS and MMS messages
     * @param processor A consumer that takes in the {@link Message} and returns true for the method
     *     to continue parsing the cursor or false to return.
     */
    public static void forEachDesc(
            @Nullable Cursor messageCursor, @NonNull Function<Message, Boolean> processor) {
        if (messageCursor == null || !messageCursor.moveToFirst()) {
            return;
        }
        Context context = AppFactory.get().getContext();
        boolean moveToNext = true;
        boolean hasBeenRepliedTo = false;
        do {
            Message message;
            try {
                message = parseMessageAtPoint(context, messageCursor, hasBeenRepliedTo);
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
                L.d("Message was not able to be parsed. Skipping.");
                continue;
            }
            if (message.getMessageType() == MessageType.MESSAGE_TYPE_SENT) {
                hasBeenRepliedTo = true;
            }
            moveToNext = processor.apply(message);
        } while (messageCursor.moveToNext() && moveToNext);
    }

    /**
     * Parses each message in the cursor and returns the item for further processing
     *
     * @param messageCursor The message cursor to be parsed for SMS and MMS messages and returns
     *     true for the method to continue parsing the cursor or false to return.
     */
    @Nullable
    public static Message parseCurrentMessage(@NonNull Cursor messageCursor) {
        Message message = null;
        Context context = AppFactory.get().getContext();
        try {
            message = parseMessageAtPoint(context, messageCursor, false);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            L.d("Message was not able to be parsed. Skipping.");
        }
        return message;
    }

    /**
     * Parses message at the point in cursor.
     *
     * @throws IllegalArgumentException if desired columns are missing.
     * @see CursorUtils#CONTENT_CONVERSATION_PROJECTION
     */
    @NonNull
    private static Conversation.Message parseMessageAtPoint(
            @NonNull Context context, @NonNull Cursor cursor, boolean userHasReplied) {
        MmsSmsMessage msg =
                MmsUtils.isMms(cursor)
                        ? MmsUtils.parseMms(context, cursor)
                        : SmsUtils.parseSms(cursor);
        Person person =
                ContactUtils.getPerson(context, msg.mPhoneNumber, /* processParticipant= */ null);
        Conversation.Message message =
                new Conversation.Message(msg.mBody, msg.mDate.toEpochMilli(), person);
        if (msg.mType == TextBasedSmsColumns.MESSAGE_TYPE_SENT) {
            message.setMessageType(MessageType.MESSAGE_TYPE_SENT);
            message.setMessageStatus(MESSAGE_STATUS_NONE);
        } else {
            int status =
                    (msg.mRead || userHasReplied) ? MESSAGE_STATUS_READ : MESSAGE_STATUS_UNREAD;
            message.setMessageType(MessageType.MESSAGE_TYPE_INBOX);
            message.setMessageStatus(status);
        }
        return message;
    }

    private MessageUtils() {}
}
