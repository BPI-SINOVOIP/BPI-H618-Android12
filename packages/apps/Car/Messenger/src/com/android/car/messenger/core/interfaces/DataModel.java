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

package com.android.car.messenger.core.interfaces;

import androidx.lifecycle.LiveData;

import androidx.annotation.NonNull;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.models.UserAccount;

import java.util.Collection;

/**
 * This interface allows the UI to communicate with the host app. The methods provides the data and
 * actions needed by the UI library. Message Interface Channel should be implemented by the host
 * app. Method calls are done on the main thread. Extensive data gathering work should be delegated
 * to a background thread and the UI library can be notified once the data is ready via the change
 * listener.
 */
public interface DataModel {

    /**
     * Get list of accounts. Here an account can refer to actual accounts or separate user accounts.
     * Data will be separated in the UI by user accounts.
     */
    @NonNull
    LiveData<Collection<UserAccount>> getAccounts();

    /**
     * Call this to reload user account live data. This is useful when resuming an activity, to
     * ensure no account changes was missed.
     */
    void refreshUserAccounts();

    /**
     * Get collection of conversations for the given account.
     *
     * @param userAccount The account to which data is being queried. This could be the subscription
     *     id matching a sim in multi-account setting or account id with multi-user account
     */
    @NonNull
    LiveData<Collection<Conversation>> getConversations(@NonNull UserAccount userAccount);

    /**
     * Callback is called when a conversation is removed from the telephony database.
     *
     * <p>All cached data specific to this conversation should be removed, including notifications,
     * mute status and more.
     */
    @NonNull
    LiveData<String> onConversationRemoved();

    /**
     * Returns an observable conversation item, holding only unread messages. since the last known
     * {@link UserAccount#getConnectionTime}.
     *
     * <p>If no unread messages are found for the conversation id, the live data emits no data.
     */
    LiveData<Conversation> getUnreadMessages();

    /**
     * Called by UI to mute all notifications for this conversation
     *
     * @param conversationId The unique id for the conversation
     * @param mute The requested mute action, false is to unmute, true is to mute
     */
    void muteConversation(@NonNull String conversationId, boolean mute);

    /**
     * Called by UI to mark conversation as read
     *
     * @param conversationId The unique id for the conversation
     */
    void markAsRead(@NonNull String conversationId);

    /**
     * Called by UI to reply to a conversation
     *
     * @param accountId The user account/device id to send the message from
     * @param conversationId The phone number to send message
     * @param message The desired message to send to conversation thread
     */
    void replyConversation(
            @NonNull int accountId, @NonNull String conversationId, @NonNull String message);

    /**
     * Called by UI to send a message to a phone number on a device
     *
     * @param accountId The user account/device id to send the message from
     * @param phoneNumber The desired phone number to send message to
     * @param message The desired message to send to conversation thread
     */
    void sendMessage(int accountId, @NonNull String phoneNumber, @NonNull String message);

    /**
     * Called by UI to send a message to a phone number on a device
     *
     * @param iccId The {@link UserAccount#getIccId()} belonging to the device/user account to send
     *     the message from
     * @param phoneNumber The desired phone number to send message to
     * @param message The desired message to send to conversation thread
     */
    void sendMessage(@NonNull String iccId, @NonNull String phoneNumber, @NonNull String message);
}
