/*
 * Copyright (C) 2018 The Android Open Source Project
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

import android.annotation.SuppressLint;
import android.app.Application;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.interfaces.DataModel;
import com.android.car.messenger.core.models.UserAccount;

import java.util.List;
import java.util.stream.Collectors;

/** View model for ConversationLogFragment which provides message history live data. */
public class ConversationListViewModel extends AndroidViewModel {
    @SuppressLint("StaticFieldLeak")
    @NonNull
    private final DataModel mDataModel;

    @Nullable private UserAccount mUserAccount;
    @Nullable private LiveData<UIConversationLog> mUIConversationLogLiveData;

    public ConversationListViewModel(@NonNull Application application) {
        super(application);
        mDataModel = AppFactory.get().getDataModel();
    }

    /**
     * Gets an observable {@link UIConversationLog} for the connected account
     *
     * <p>The observable emits the following: - {@link UIConversationLog#isLoading()} returns true
     * when loading - {@link UIConversationLog#getConnectionStatus()} returns appropriate connection
     * status, such as connected or disconnected - {@link UIConversationLog#getData()} returns a
     * non-null list of {@link UIConversationItem}, or empty if no items found
     */
    @NonNull
    public LiveData<UIConversationLog> getConversations(@Nullable UserAccount userAccount) {
        if (userAccount == null) {
            MediatorLiveData<UIConversationLog> mutableLiveData = new MediatorLiveData<>();
            mutableLiveData.postValue(UIConversationLog.getDisconnectedState());
            return mutableLiveData;
        }
        if (mUserAccount != null
                && mUserAccount.getId() == userAccount.getId()
                && mUIConversationLogLiveData != null) {
            return mUIConversationLogLiveData;
        }
        mUserAccount = userAccount;
        mUIConversationLogLiveData = createUIConversationLog(mUserAccount);
        return mUIConversationLogLiveData;
    }

    private LiveData<UIConversationLog> createUIConversationLog(@NonNull UserAccount userAccount) {
        MediatorLiveData<UIConversationLog> mutableLiveData = new MediatorLiveData<>();
        mutableLiveData.postValue(UIConversationLog.getLoadingState());
        mutableLiveData.addSource(
                mDataModel.getConversations(userAccount),
                list -> {
                    List<UIConversationItem> data =
                            list.stream()
                                    .map(UIConversationItemConverter::convertToUIConversationItem)
                                    .collect(Collectors.toList());
                    UIConversationLog log = UIConversationLog.getLoadedState(data);
                    mutableLiveData.postValue(log);
                });
        return mutableLiveData;
    }
}
