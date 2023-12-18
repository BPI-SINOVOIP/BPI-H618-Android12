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

package com.android.car.messenger.core.ui.conversationlist;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.format.DateFormat;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.R;
import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.util.ConversationUtil;

import java.util.Objects;

/** Util class that converts Conversation Item to UIConversationItem */
public class UIConversationItemConverter {

    private UIConversationItemConverter() {}

    /** Converts Conversation Item to UIConversationItem */
    public static UIConversationItem convertToUIConversationItem(Conversation conversation) {
        Context context = AppFactory.get().getContext();
        boolean isUnread = conversation.getUnreadCount() > 0;
        long timestamp = ConversationUtil.getConversationTimestamp(conversation);
        boolean isReplied = ConversationUtil.isReplied(conversation);

        String subtitle = "";
        Drawable subtitleIcon = null;
        if (isReplied) {
            subtitle = context.getString(R.string.replied);
            subtitleIcon = context.getDrawable(R.drawable.car_ui_icon_reply);
        } else if (isUnread) {
            subtitle = getNumberOfUnreadMessages(context, conversation.getUnreadCount());
            subtitleIcon = context.getDrawable(R.drawable.ic_play);
        }

        return new UIConversationItem(
                conversation.getId(),
                Objects.requireNonNull(conversation.getConversationTitle()),
                subtitle,
                subtitleIcon,
                toHumanDisplay(timestamp),
                getConversationAvatar(context, conversation),
                /* showMuteIcon= */ true,
                /* showReplyIcon= */ true,
                isUnread,
                conversation.isMuted(),
                conversation);
    }

    @NonNull
    private static String getNumberOfUnreadMessages(
            @NonNull Context context, int noOfUnreadMessages) {
        if (noOfUnreadMessages == 1) {
            return context.getResources().getQuantityString(R.plurals.new_message, 1);
        } else {
            return context.getResources()
                    .getQuantityString(
                            R.plurals.new_message, noOfUnreadMessages, noOfUnreadMessages);
        }
    }

    @NonNull
    private static String toHumanDisplay(long timeInMillis) {
        String delegate = "hh:mm aaa";
        return (String) DateFormat.format(delegate, timeInMillis);
    }

    @Nullable
    private static Drawable getConversationAvatar(
            @NonNull Context context, @NonNull Conversation conversation) {
        return (conversation.getConversationIcon() != null)
                ? conversation.getConversationIcon().loadDrawable(context)
                : null;
    }
}
