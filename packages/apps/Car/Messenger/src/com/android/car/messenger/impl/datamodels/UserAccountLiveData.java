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

import androidx.lifecycle.LiveData;
import android.content.Context;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionManager.OnSubscriptionsChangedListener;
import android.telephony.TelephonyManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.impl.datamodels.UserAccountLiveData.UserAccountChangeList;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Get the UserAccount of the currently active remote bluetooth SIM(s). The records will be sorted
 * by {@link SubscriptionInfo#getSimSlotIndex} then by {@link SubscriptionInfo#getSubscriptionId}.
 *
 * <p>Requires Permission: {@link android.Manifest.permission#READ_PHONE_STATE READ_PHONE_STATE} or
 * that the calling app has carrier privileges (see {@link TelephonyManager#hasCarrierPrivileges}).
 * In the latter case, only records accessible to the calling app are returned.
 *
 * <p>Listens for changes via {@link OnSubscriptionsChangedListener} and the latest data can be
 * observed and retrieved via {@link LiveData#getValue()}.
 *
 * <p>Sorts list of the currently {@link SubscriptionInfo} records available on the device
 *
 * <ul>
 *   <li>If the list is empty then there are no {@link SubscriptionInfo} records currently
 *       available.
 *   <li>if the list is non-empty the list is sorted by {@link SubscriptionInfo#getSimSlotIndex}
 *       then by {@link SubscriptionInfo#getSubscriptionId}.
 * </ul>
 */
public class UserAccountLiveData extends LiveData<UserAccountChangeList> {
    @NonNull private final SubscriptionManager mSubscriptionManager;

    @NonNull
    private final OnSubscriptionsChangedListener mOnChangeListener =
            new OnSubscriptionsChangedListener() {
                @Override
                public void onSubscriptionsChanged() {
                    loadValue();
                }
            };

    @Nullable private static UserAccountLiveData sInstance;

    private UserAccountLiveData() {
        Context context = AppFactory.get().getContext();
        mSubscriptionManager = context.getSystemService(SubscriptionManager.class);
        mSubscriptionManager.addOnSubscriptionsChangedListener(mOnChangeListener);
        loadValue();
    }

    /**
     * Refresh the user accounts. Updates listeners if a change is found. Useful to call when
     * something occurs that indicates a change in accounts, such as empty messages. This is useful
     * as there are occasions when the subscription on change listener is not called after a
     * subscription is deleted.
     */
    public void refresh() {
        loadValue();
    }

    /** Gets the instance of {@link UserAccountLiveData} */
    @NonNull
    public static UserAccountLiveData getInstance() {
        if (sInstance == null) {
            sInstance = new UserAccountLiveData();
        }
        return sInstance;
    }

    private void loadValue() {
        List<UserAccount> accounts =
                getNullSafeSubscriptionInfoList().stream()
                        .map(
                                it -> {
                                    int subscriptionId = it.getSubscriptionId();
                                    String iccId = it.getIccId();
                                    String displayName =
                                            it.getDisplayName() != null
                                                    ? it.getDisplayName().toString()
                                                    : "";
                                    return new UserAccount(
                                            subscriptionId, displayName, iccId, Instant.now());
                                })
                        .collect(Collectors.toList());

        // get the removed accounts and added accounts.
        Collection<UserAccount> prevUserAccounts = getValueOrEmpty().mAccounts;
        Set<UserAccount> addedAccounts = getDifference(accounts, prevUserAccounts);
        Set<UserAccount> removedAccounts = getDifference(prevUserAccounts, accounts);

        if (addedAccounts.isEmpty() && removedAccounts.isEmpty()) {
            // Return early if no new accounts were added or removed since last change list.
            // However, if no account is found, post an empty changelist to allow
            // the subscriber update the UI with "no account found or all accounts disconnected"
            if (accounts.isEmpty()) {
                postValue(new UserAccountChangeList());
            }
            return;
        }

        UserAccountChangeList newAccountChangeList = new UserAccountChangeList();
        newAccountChangeList.mAccounts = accounts;
        newAccountChangeList.mAddedAccounts = addedAccounts;
        newAccountChangeList.mRemovedAccounts = removedAccounts;
        postValue(newAccountChangeList);
    }

    /**
     * Returns User Account with the given iccId
     *
     * @param iccId Maps to the {@link SubscriptionInfo#getIccId()}
     */
    @Nullable
    public static UserAccount getUserAccount(@NonNull String iccId) {
        Collection<UserAccount> userAccounts = getValueOrEmpty().getAccounts();
        for (UserAccount account : userAccounts) {
            if (iccId.equals(account.getIccId())) {
                return account;
            }
        }
        return null;
    }

    /**
     * Returns a list that contains a difference between the two lists - firstList - secondList =
     * result This essentially points out which items or changes are not present in firstList.
     */
    @NonNull
    public static Set<UserAccount> getDifference(
            @NonNull Collection<UserAccount> firstList,
            @NonNull Collection<UserAccount> secondList) {
        return firstList.stream()
                .filter(it -> secondList.stream().noneMatch(item -> item.getId() == it.getId()))
                .collect(Collectors.toSet());
    }

    /** A list of {@link UserAccount} with information on what changed */
    public static class UserAccountChangeList {
        @NonNull private Collection<UserAccount> mAccounts = new ArrayList<>();
        @NonNull private Collection<UserAccount> mRemovedAccounts = new ArrayList<>();
        @NonNull private Collection<UserAccount> mAddedAccounts = new ArrayList<>();

        /** Get all user accounts */
        @NonNull
        public Collection<UserAccount> getAccounts() {
            return mAccounts;
        }

        /** Get removed accounts for this change list */
        @NonNull
        public Stream<UserAccount> getRemovedAccounts() {
            return mRemovedAccounts.stream();
        }

        /** Gets added accounts for this change list */
        @NonNull
        public Stream<UserAccount> getAddedAccounts() {
            return mAddedAccounts.stream();
        }
    }

    /** Returns the value or empty changelist, if value is null */
    @NonNull
    private static UserAccountChangeList getValueOrEmpty() {
        UserAccountChangeList value = sInstance != null ? sInstance.getValue() : null;
        if (value == null) {
            value = new UserAccountChangeList();
        }
        return value;
    }

    /** Returns null safe subscription info list */
    @NonNull
    private List<SubscriptionInfo> getNullSafeSubscriptionInfoList() {
        List<SubscriptionInfo> subscriptionInfos =
                mSubscriptionManager.getActiveSubscriptionInfoList();
        if (subscriptionInfos == null) {
            return new ArrayList<>();
        }
        // The last added subscription is more likely the last device connection made
        // and more likely relevant to the user.
        // Reverse the subscription list to prioritize the last connected device.
        Collections.reverse(subscriptionInfos);
        return subscriptionInfos;
    }
}
