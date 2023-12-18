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

import static com.android.car.messenger.core.shared.MessageConstants.LAST_REPLY_TIMESTAMP_EXTRA;

import static java.lang.Math.max;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.common.Conversation.Message;
import com.android.car.messenger.common.Conversation.Message.MessageStatus;

/** Conversation Util class for the {@link Conversation} DAO */
public class ConversationUtil {
    private ConversationUtil() {}

    /**
     * Get the last timestamp for the conversation. This could be a reply timestamp or last received
     * message timestamp, whichever is last.
     */
    public static long getConversationTimestamp(@Nullable Conversation conversation) {
        if (conversation == null) {
            return 0L;
        }
        long replyTimestamp = conversation.getExtras().getLong(LAST_REPLY_TIMESTAMP_EXTRA, 0L);
        Message lastMessage = getLastMessage(conversation);
        long lastMessageTimestamp = lastMessage == null ? 0L : lastMessage.getTimestamp();
        return max(replyTimestamp, lastMessageTimestamp);
    }

    /** Returns if the {@link Conversation} has been last responded to. */
    public static boolean isReplied(@Nullable Conversation conversation) {
        if (conversation == null) {
            return false;
        }
        long lastReplyTimestamp = getReplyTimestamp(conversation);
        long lastMessageTimestamp = 0L;
        Message lastMessageGroup = ConversationUtil.getLastMessage(conversation);
        if (lastMessageGroup != null) {
            lastMessageTimestamp = lastMessageGroup.getTimestamp();
        }
        return lastReplyTimestamp > lastMessageTimestamp;
    }

    /**
     * Returns the last message in the conversation, or null if {@link Conversation#getMessages} is
     * empty
     */
    @Nullable
    public static Message getLastMessage(@Nullable Conversation conversation) {
        if (conversation == null || conversation.getMessages().isEmpty()) {
            return null;
        }
        int size = conversation.getMessages().size();
        return conversation.getMessages().get(size - 1);
    }

    /**
     * Gets the conversation status of the last messages Returns {@link
     * MessageStatus#MESSAGE_STATUS_NONE} when no known message status or last message is a reply
     */
    @MessageStatus
    public static int getConversationStatus(@Nullable Conversation conversation) {
        Message lastMessage = getLastMessage(conversation);
        return isReplied(conversation) || lastMessage == null
                ? MessageStatus.MESSAGE_STATUS_NONE
                : lastMessage.getMessageStatus();
    }

    /**
     * Sets the Reply timestamp to a {@link Conversation.Builder}
     *
     * @param extras optional, pass an existing bundle to which the reply timestamp will be added
     *     to. If no extra is passed, a new one will be created. The final extras will be added to
     *     the {@link Conversation#getExtras()}
     */
    public static void setReplyTimestampAsAnExtra(
            @NonNull Conversation.Builder conversationBuilder,
            @Nullable Bundle extras,
            long lastReplyTimestamp) {
        if (lastReplyTimestamp > 0L) {
            if (extras == null) {
                extras = new Bundle();
            }
            extras.putLong(LAST_REPLY_TIMESTAMP_EXTRA, lastReplyTimestamp);
            conversationBuilder.setExtras(extras);
        }
    }

    /** Gets reply timestamp */
    private static long getReplyTimestamp(@Nullable Conversation conversation) {
        if (conversation == null) {
            return 0L;
        }
        return conversation.getExtras().getLong(LAST_REPLY_TIMESTAMP_EXTRA, 0L);
    }
}
