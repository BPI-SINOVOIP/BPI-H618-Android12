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

import static com.android.car.messenger.impl.datamodels.util.ConversationFetchUtil.fetchConversation;
import static com.android.car.messenger.impl.datamodels.util.CursorUtils.DEFAULT_SORT_ORDER;

import android.content.Context;
import android.database.Cursor;
import android.database.CursorIndexOutOfBoundsException;
import android.net.Uri;
import android.provider.Telephony;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.core.shared.MessageConstants;
import com.android.car.messenger.core.util.ConversationUtil;
import com.android.car.messenger.core.util.L;
import com.android.car.messenger.impl.common.ProjectionStateListener;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Locale;
import java.util.Objects;

/**
 * Publishes a stream of {@link Conversation} with unread messages that was received on the user
 * device after the car's connection to the{@link UserAccount}.
 */
public class NewMessageLiveData extends ContentProviderLiveData<Conversation> {
    @NonNull
    private final UserAccountLiveData mUserAccountLiveData = UserAccountLiveData.getInstance();

    @NonNull private Collection<UserAccount> mUserAccounts = new ArrayList<>();
    @NonNull private final HashMap<Integer, Instant> mOffsetMap = new HashMap<>();

    @NonNull
    private static final String MESSAGE_QUERY =
            Telephony.TextBasedSmsColumns.DATE
                    + " > %d AND "
                    + Telephony.TextBasedSmsColumns.SUBSCRIPTION_ID
                    + " = %d";

    @NonNull
    private final ProjectionStateListener mProjectionStateListener =
            new ProjectionStateListener(AppFactory.get().getContext());

    NewMessageLiveData() {
        super(Telephony.Sms.CONTENT_URI, Telephony.Mms.CONTENT_URI, Telephony.MmsSms.CONTENT_URI);
    }

    @Override
    protected void onActive() {
        super.onActive();
        addSource(
                mUserAccountLiveData,
                it -> {
                    mUserAccounts = it.getAccounts();
                    it.getRemovedAccounts()
                            .forEach(userAccount -> mOffsetMap.remove(userAccount.getId()));
                });
        if (getValue() == null) {
            onDataChange();
        }
    }

    @Override
    protected void onInactive() {
        super.onInactive();
        removeSource(mUserAccountLiveData);
        mUserAccounts.clear();
        mOffsetMap.clear();
    }

    @Override
    public void onDataChange() {
        for (UserAccount userAccount : mUserAccounts) {
            if (hasProjectionInForeground(userAccount)) {
                continue;
            }
            Instant offset =
                    Objects.requireNonNull(
                            mOffsetMap.getOrDefault(
                                    userAccount.getId(), userAccount.getConnectionTime()));
            Cursor mmsCursor = getMmsCursor(userAccount, offset);
            boolean foundNewMms = postNewMessageIfFound(mmsCursor, userAccount);
            Cursor smsCursor = getSmsCursor(userAccount, offset);
            boolean foundNewSms = postNewMessageIfFound(smsCursor, userAccount);
            if (foundNewMms || foundNewSms) {
                // onDataChange is called per one message insert,
                // so once a new message is found we can exit early
                break;
            }
        }
    }

    /** Post a new message if one is found, and returns true if so, false otherwise */
    private boolean postNewMessageIfFound(
            @Nullable Cursor cursor, @NonNull UserAccount userAccount) {
        if (cursor == null || !cursor.moveToFirst()) {
            return false;
        }
        String conversationId =
                cursor.getString(cursor.getColumnIndex(Telephony.TextBasedSmsColumns.THREAD_ID));

        Conversation conversation;
        try {
            conversation = fetchConversation(conversationId);
            conversation.getExtras().putInt(MessageConstants.EXTRA_ACCOUNT_ID, userAccount.getId());
        } catch (CursorIndexOutOfBoundsException e) {
            L.w("Error occurred fetching conversation Id " + conversationId);
            return false;
        }
        Instant offset =
                Instant.ofEpochMilli(ConversationUtil.getConversationTimestamp(conversation));
        mOffsetMap.put(userAccount.getId(), offset);
        postValue(conversation);
        return true;
    }

    /** Get the last message cursor, taking into account the last message posted */
    @Nullable
    private Cursor getMmsCursor(@NonNull UserAccount userAccount, @NonNull Instant offset) {
        return getCursor(Telephony.Mms.Inbox.CONTENT_URI, userAccount, offset.getEpochSecond());
    }

    /** Get the last message cursor, taking into account the last message posted */
    @Nullable
    private Cursor getSmsCursor(@NonNull UserAccount userAccount, @NonNull Instant offset) {
        return getCursor(Telephony.Sms.Inbox.CONTENT_URI, userAccount, offset.toEpochMilli());
    }
    /** Get the last message cursor, taking into account an offset and subscription id */
    @Nullable
    private Cursor getCursor(Uri uri, @NonNull UserAccount userAccount, long offset) {
        Context context = AppFactory.get().getContext();
        String query = String.format(Locale.ENGLISH, MESSAGE_QUERY, offset, userAccount.getId());
        return context.getContentResolver()
                .query(
                        uri,
                        new String[] {Telephony.TextBasedSmsColumns.THREAD_ID},
                        query,
                        /* selectionArgs= */ null,
                        DEFAULT_SORT_ORDER + " LIMIT 1");
    }

    private boolean hasProjectionInForeground(@NonNull UserAccount userAccount) {
        return mProjectionStateListener.isProjectionInActiveForeground(userAccount.getIccId());
    }
}
