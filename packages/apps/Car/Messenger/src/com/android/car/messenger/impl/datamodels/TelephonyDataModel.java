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

package com.android.car.messenger.impl.datamodels;

import static com.android.car.messenger.core.shared.MessageConstants.KEY_MUTED_CONVERSATIONS;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.provider.Telephony;
import android.telephony.SmsManager;

import androidx.annotation.NonNull;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.interfaces.DataModel;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.core.util.L;
import com.android.car.messenger.impl.datamodels.UserAccountLiveData.UserAccountChangeList;
import com.android.car.messenger.impl.datamodels.util.CursorUtils;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/** Queries the telephony data model to retrieve the SMS/MMS messages */
public class TelephonyDataModel implements DataModel {

    @NonNull
    @Override
    public LiveData<Collection<UserAccount>> getAccounts() {
        return Transformations.map(
                UserAccountLiveData.getInstance(), UserAccountChangeList::getAccounts);
    }

    @Override
    public void refreshUserAccounts() {
        UserAccountLiveData.getInstance().refresh();
    }

    @NonNull
    @Override
    public LiveData<Collection<Conversation>> getConversations(@NonNull UserAccount userAccount) {
        return new ConversationListLiveData(userAccount);
    }

    @NonNull
    @Override
    public LiveData<Conversation> getUnreadMessages() {
        return new NewMessageLiveData();
    }

    @Override
    public void muteConversation(@NonNull String conversationId, boolean mute) {
        SharedPreferences sharedPreferences = AppFactory.get().getSharedPreferences();
        Set<String> mutedConversations =
                sharedPreferences.getStringSet(KEY_MUTED_CONVERSATIONS, new HashSet<>());
        Set<String> finalSet = new HashSet<>(mutedConversations);
        if (mute) {
            finalSet.add(conversationId);
        } else {
            finalSet.remove(conversationId);
        }
        sharedPreferences.edit().putStringSet(KEY_MUTED_CONVERSATIONS, finalSet).apply();
    }

    @Override
    public void markAsRead(@NonNull String conversationId) {
        L.d("markAsRead for conversationId: " + conversationId);
        Context context = AppFactory.get().getContext();
        ContentValues values = new ContentValues();
        values.put(Telephony.ThreadsColumns.READ, 1);
        context.getContentResolver()
                .update(CursorUtils.getConversationUri(conversationId), values, /* extras= */ null);
    }

    @Override
    public void replyConversation(
            int accountId, @NonNull String conversationId, @NonNull String message) {
        if (accountId <= 0) {
            L.e("Invalid user account id when replying conversation, dropping message");
            return;
        }
        L.d("Sending a message to a conversation");
        String destination =
                Uri.withAppendedPath(Telephony.Threads.CONTENT_URI, conversationId).toString();
        SmsManager.getSmsManagerForSubscriptionId(accountId)
                .sendTextMessage(
                        destination,
                        /* scAddress= */ null,
                        message,
                        /* sentIntent= */ null,
                        /* deliveryIntent= */ null);
    }

    @Override
    public void sendMessage(int accountId, @NonNull String phoneNumber, @NonNull String message) {
        L.d("Sending a message to a phone number");
        SmsManager.getSmsManagerForSubscriptionId(accountId)
                .sendTextMessage(
                        phoneNumber,
                        /* scAddress= */ null,
                        message,
                        /* sentIntent= */ null,
                        /* deliveryIntent= */ null);
    }

    @Override
    public void sendMessage(
            @NonNull String iccId, @NonNull String phoneNumber, @NonNull String message) {
        UserAccount userAccount = UserAccountLiveData.getUserAccount(iccId);
        if (userAccount == null) {
            L.d("Could not find User Account with specified iccId. Unable to send message");
            return;
        }
        sendMessage(userAccount.getId(), phoneNumber, message);
    }

    @NonNull
    @Override
    public LiveData<String> onConversationRemoved() {
        return Transformations.map(
                ConversationsPerDeviceFetchManager.getInstance().getRemovedConversationLiveData(),
                id -> {
                    muteConversation(id, false);
                    return id;
                });
    }
}
