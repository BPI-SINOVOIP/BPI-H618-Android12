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

package com.android.car.messenger.impl.datamodels.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Bitmap;
import androidx.core.graphics.drawable.IconCompat;
import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.core.app.Person;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.shared.MessageConstants;
import com.android.car.messenger.core.util.ConversationUtil;
import com.android.car.messenger.core.util.L;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.BiConsumer;

/** Utility class for retrieving and setting conversation items. */
public class ConversationFetchUtil {

    private static final int MESSAGE_LIMIT = 10;
    private static final String COMMA_DELIMITER = ", ";

    private ConversationFetchUtil() {}

    /** Fetches a conversation item based on a provided conversation id */
    public static Conversation fetchConversation(@NonNull String conversationId) {
        L.d("Fetching latest data for Conversation " + conversationId);
        Conversation.Builder conversationBuilder = initConversationBuilder(conversationId);
        Cursor messagesCursor =
                CursorUtils.getMessagesCursor(conversationId, MESSAGE_LIMIT, /* offset= */ 0);
        // messages to read: first get unread messages
        List<Conversation.Message> messagesToRead = MessageUtils.getUnreadMessages(messagesCursor);
        int unreadCount = messagesToRead.size();
        long lastReplyTimestamp = 0L;

        // if no unread messages, get read messages
        if (messagesToRead.isEmpty()) {
            Pair<List<Conversation.Message>, Long> readMessagesAndReplyTimestamp =
                    MessageUtils.getReadMessagesAndReplyTimestamp(messagesCursor);
            messagesToRead = readMessagesAndReplyTimestamp.first;
            lastReplyTimestamp = readMessagesAndReplyTimestamp.second;
        }

        conversationBuilder.setMessages(messagesToRead).setUnreadCount(unreadCount);
        ConversationUtil.setReplyTimestampAsAnExtra(
                conversationBuilder, /* extras= */ null, lastReplyTimestamp);
        return conversationBuilder.build();
    }

    @NonNull
    private static Conversation.Builder initConversationBuilder(@NonNull String conversationId) {
        Context context = AppFactory.get().getContext();
        String userName = ContactUtils.DRIVER_NAME;
        Conversation.Builder builder =
                new Conversation.Builder(
                        new Person.Builder().setName(userName).build(), conversationId);
        List<Person> participants =
                fetchParticipants(
                        conversationId,
                        (names, icons) -> {
                            builder.setConversationTitle(TextUtils.join(COMMA_DELIMITER, names));
                            Bitmap bitmap = AvatarUtil.createGroupAvatar(context, icons);
                            if (bitmap != null) {
                                builder.setConversationIcon(IconCompat.createWithBitmap(bitmap));
                            }
                        });
        builder.setParticipants(participants);
        builder.setMuted(loadMutedList().contains(conversationId));
        return builder;
    }

    /**
     * Fetches participants and allows caller to process names and icons before returning.
     *
     * <p>For context, a conversation often holds multiple messages, which holds multiple
     * participant contacts, which each in turn could hold an avatar.
     *
     * <p>This leads to a very heavy conversation class and leads to problems down the road when
     * sending this conversation as a bundle.
     *
     * <p>To mitigate this, {@link Person} classes do not hold an avatar. Instead, each contact
     * avatar is channeled up to the caller during a fetch to make one avatar for the entire
     * conversation.
     *
     * @param conversationId, id for conversation to fetch participants information
     * @param processNamesAndIcons the method to process the names and icons of the participants
     * @return list of participants as {@link Person}. For performance reasons, the objects do not
     *     contain an avatar, and a functional interface is needed in order to process the various
     *     participant icons nto one conversation icon.
     */
    private static List<Person> fetchParticipants(
            @NonNull String conversationId,
            @NonNull BiConsumer<List<CharSequence>, List<Bitmap>> processNamesAndIcons) {
        List<CharSequence> participantNames = new ArrayList<>();
        List<Bitmap> participantIcons = new ArrayList<>();
        List<Person> participants =
                ContactUtils.getRecipients(
                        conversationId,
                        (name, bitmap) -> {
                            participantNames.add(name);
                            participantIcons.add(bitmap);
                        });
        processNamesAndIcons.accept(participantNames, participantIcons);
        return participants;
    }

    /** Returns a set of muted conversation items */
    @NonNull
    public static Set<String> loadMutedList() {
        SharedPreferences sharedPreferences = AppFactory.get().getSharedPreferences();
        return sharedPreferences.getStringSet(
                MessageConstants.KEY_MUTED_CONVERSATIONS, new HashSet<>());
    }
}
