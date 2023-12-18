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

package com.android.car.messenger.impl.datamodels;

import static android.provider.Telephony.TextBasedSmsColumns.THREAD_ID;

import static com.android.car.messenger.impl.datamodels.util.ConversationFetchUtil.fetchConversation;
import static com.android.car.messenger.impl.datamodels.util.ConversationFetchUtil.loadMutedList;

import static java.util.Comparator.comparingLong;

import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.CursorIndexOutOfBoundsException;
import android.provider.Telephony;

import androidx.annotation.NonNull;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.core.shared.MessageConstants;
import com.android.car.messenger.core.util.ConversationUtil;
import com.android.car.messenger.core.util.L;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Set;
import java.util.stream.Collectors;

/** Publishes a list of {@link Conversation} for a {@link UserAccount} to subscribers */
class ConversationListLiveData extends ContentProviderLiveData<Collection<Conversation>> {
    @NonNull private final UserAccount mUserAccount;

    @NonNull
    private static final Comparator<Conversation> sConversationComparator =
            comparingLong(ConversationUtil::getConversationTimestamp).reversed();

    @NonNull
    private final SharedPreferences.OnSharedPreferenceChangeListener mPreferenceChangeListener =
            (sharedPreferences, key) -> onSharedPreferenceChanged(key);

    ConversationListLiveData(@NonNull UserAccount userAccount) {
        super(Telephony.MmsSms.CONTENT_URI);
        mUserAccount = userAccount;
    }

    @Override
    protected void onActive() {
        super.onActive();
        SharedPreferences sharedPrefs = AppFactory.get().getSharedPreferences();
        sharedPrefs.registerOnSharedPreferenceChangeListener(mPreferenceChangeListener);
        if (getValue() == null) {
            onDataChange();
        }
    }

    @Override
    protected void onInactive() {
        super.onInactive();
        SharedPreferences sharedPrefs = AppFactory.get().getSharedPreferences();
        sharedPrefs.unregisterOnSharedPreferenceChangeListener(mPreferenceChangeListener);
    }

    @Override
    public void onDataChange() {
        Cursor cursor = ConversationsPerDeviceFetchManager.getCursor(mUserAccount.getId());
        ArrayList<Conversation> conversations = new ArrayList<>();
        while (cursor != null && cursor.moveToNext()) {
            String conversationId = cursor.getString(cursor.getColumnIndex(THREAD_ID));
            Conversation conversation = null;
            try {
                conversation = fetchConversation(conversationId);
            } catch (CursorIndexOutOfBoundsException e) {
                L.w("Error occurred fetching conversation Id " + conversationId);
            } finally {
                if (conversation != null) {
                    conversations.add(conversation);
                }
            }
        }
        Collections.sort(conversations, sConversationComparator);
        postValue(conversations);
    }

    private void onSharedPreferenceChanged(@NonNull String key) {
        Collection<Conversation> conversations = getValue();
        if (!MessageConstants.KEY_MUTED_CONVERSATIONS.equals(key) || conversations == null) {
            return;
        }
        Set<String> mutedList = loadMutedList();
        ArrayList<Conversation> finalConversations = new ArrayList<>();
        boolean muteChange = false;
        for (Conversation conversation : conversations) {
            String conversationId = conversation.getId();
            boolean wasPreviouslyMuted = conversation.isMuted();
            boolean isMuted = mutedList.contains(conversationId);
            if (isMuted == wasPreviouslyMuted) {
                finalConversations.add(conversation);
                continue;
            }
            Conversation.Builder builder = conversation.toBuilder();
            builder.setMuted(isMuted);
            finalConversations.add(builder.build());
            muteChange = true;
        }

        if (muteChange) {
            postValue(
                    finalConversations.stream()
                            .sorted(sConversationComparator)
                            .collect(Collectors.toList()));
        }
    }
}
