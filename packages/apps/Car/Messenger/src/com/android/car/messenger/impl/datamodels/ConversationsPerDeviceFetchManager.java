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

import static android.provider.Telephony.MmsSms.CONTENT_CONVERSATIONS_URI;
import static android.provider.Telephony.TextBasedSmsColumns.SUBSCRIPTION_ID;
import static android.provider.Telephony.TextBasedSmsColumns.THREAD_ID;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.Observer;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.telephony.SubscriptionInfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.models.UserAccount;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Holds the information on any changes made to a conversation list per device/user account
 *
 * <p>To listen for specific changes such as removed conversations, observable data is also
 * provided.
 */
class ConversationsPerDeviceFetchManager {
    @Nullable private static ConversationsPerDeviceFetchManager sInstance;

    @NonNull
    private final MediatorLiveData<String> mRemovedConversationLiveData = new MediatorLiveData<>();

    @NonNull
    private final HashMap<Integer, ConversationIdChangeList> mCachedResults = new HashMap<>();

    @NonNull private static final Uri URI = CONTENT_CONVERSATIONS_URI;

    @NonNull
    private static final String[] PROJECTION = {
        SUBSCRIPTION_ID, THREAD_ID,
    };

    @NonNull private final Context mContext;

    @NonNull
    private final ContentObserver mObserver =
            new ContentObserver(null) {
                @Override
                public void onChange(boolean selfChange) {
                    onDataChange();
                }
            };

    private ConversationsPerDeviceFetchManager() {
        mContext = AppFactory.get().getContext();
        mContext.getContentResolver()
                .registerContentObserver(URI, /* notifyForDescendants= */ false, mObserver);
        mRemovedConversationLiveData.addSource(
                UserAccountLiveData.getInstance(), onUserAccountRemovedObserver());
    }

    /**
     * Returns a cursor that searches the {@link android.provider.Telephony.MmsSms} database for a
     * list of all conversations, based on the accountId provided
     *
     * @param accountId searches for conversations based on id provided
     */
    @Nullable
    public static Cursor getCursor(int accountId) {
        Context context = AppFactory.get().getContext();
        return context.getContentResolver()
                .query(
                        URI,
                        PROJECTION,
                        /* selection= */ SUBSCRIPTION_ID + "=" + accountId,
                        /* selectionArgs= */ null,
                        /* sortOrder= */ null,
                        /* cancellationSignal= */ null);
    }

    private void onDataChange() {
        UserAccountLiveData.UserAccountChangeList changeList =
                UserAccountLiveData.getInstance().getValue();
        if (changeList == null) {
            return;
        }
        Collection<UserAccount> userAccounts = changeList.getAccounts();
        for (UserAccount userAccount : userAccounts) {
            boolean changeDetected = postChangeIfFound(userAccount.getId());
            // one change is posted per onDataUri call
            if (changeDetected) {
                break;
            }
        }
    }

    /**
     * Post a changelist if one is found for the id provided
     *
     * @return true if a change was posted and false, otherwise.
     */
    private boolean postChangeIfFound(int userAccountId) {
        Cursor cursor = getCursor(userAccountId);
        ArrayList<String> currentConversationIds = new ArrayList<>();
        while (cursor != null && cursor.moveToNext()) {
            String conversationId = cursor.getString(cursor.getColumnIndex(THREAD_ID));
            currentConversationIds.add(conversationId);
        }

        // get updated changes
        Collection<String> prevConversationIds =
                getValueOrEmpty(userAccountId).getAllConversationIds();
        Set<String> newConversations = getDifference(currentConversationIds, prevConversationIds);
        Set<String> removedConversations =
                getDifference(prevConversationIds, currentConversationIds);

        if (newConversations.isEmpty() && removedConversations.isEmpty()) {
            // Return early if no new conversations were added or removed since last change list.
            // However, if no conversations is found, post an empty changelist to allow
            // the subscriber update the UI with "no new conversations found"
            if (currentConversationIds.isEmpty()) {
                postValueInternal(new ConversationIdChangeList(userAccountId));
            }
            return false;
        }

        ConversationIdChangeList changeList = new ConversationIdChangeList(userAccountId);
        changeList.mConversationIds = currentConversationIds;
        changeList.mAddedConversationIds = newConversations;
        changeList.mRemovedConversationIds = removedConversations;

        postValueInternal(changeList);
        return true;
    }

    private void postValueInternal(ConversationIdChangeList changeList) {
        mCachedResults.put(changeList.mUserAccountId, changeList);
        changeList.getRemovedConversationIds().forEach(mRemovedConversationLiveData::postValue);
    }

    /** Returns a live data that emits removed conversation ids */
    public LiveData<String> getRemovedConversationLiveData() {
        return mRemovedConversationLiveData;
    }

    @NonNull
    private ConversationIdChangeList getValueOrEmpty(int userAccountId) {
        ConversationIdChangeList cache = mCachedResults.get(userAccountId);
        if (cache == null) {
            return new ConversationIdChangeList(userAccountId);
        }
        return cache;
    }

    /**
     * Returns a set that contains a difference between the two lists - firstList - secondList =
     * result
     *
     * <p>This essentially points out which items or changes are not present in firstList.
     */
    @NonNull
    private static Set<String> getDifference(
            @NonNull Collection<String> firstList, @NonNull Collection<String> secondList) {
        return firstList.stream()
                .filter(it -> !secondList.contains(it))
                .collect(Collectors.toSet());
    }

    /** Gets the instance of {@link ConversationsPerDeviceFetchManager} */
    @NonNull
    public static ConversationsPerDeviceFetchManager getInstance() {
        if (sInstance == null) {
            sInstance = new ConversationsPerDeviceFetchManager();
        }
        return sInstance;
    }

    @NonNull
    private Observer<UserAccountLiveData.UserAccountChangeList> onUserAccountRemovedObserver() {
        return userAccountChangeList -> {
            if (userAccountChangeList == null) {
                return;
            }
            userAccountChangeList
                    .getRemovedAccounts()
                    .forEach(
                            removedAccount -> {
                                ConversationIdChangeList conversationIdInfo =
                                        mCachedResults.get(removedAccount.getId());
                                if (conversationIdInfo == null) {
                                    return;
                                }
                                conversationIdInfo.getAllConversationIds().stream()
                                        .forEach(mRemovedConversationLiveData::postValue);
                                mCachedResults.remove(removedAccount.getId());
                            });
        };
    }

    /**
     * Holds the list of conversation ids per {@link SubscriptionInfo#getSubscriptionId()}
     * Additional information such as which specific conversation ids have changed is also provided.
     */
    public static class ConversationIdChangeList {
        private final int mUserAccountId;
        @NonNull private Collection<String> mConversationIds = new ArrayList<>();
        @NonNull private Collection<String> mRemovedConversationIds = new ArrayList<>();
        @NonNull private Collection<String> mAddedConversationIds = new ArrayList<>();

        private ConversationIdChangeList(int userAccountId) {
            mUserAccountId = userAccountId;
        }

        /* Returns the list of added conversation Ids */
        @NonNull
        public Collection<String> getAllConversationIds() {
            return mConversationIds;
        }

        /* Returns the list of added conversation Ids */
        @NonNull
        public Stream<String> getRemovedConversationIds() {
            return mRemovedConversationIds.stream();
        }

        /* Returns the list of removed conversation Ids */
        @NonNull
        public Stream<String> getAddedConversationIds() {
            return mAddedConversationIds.stream();
        }
    }
}
