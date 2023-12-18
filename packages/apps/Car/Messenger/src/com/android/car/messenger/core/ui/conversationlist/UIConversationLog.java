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

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.models.ConnectionStatus;

import java.util.ArrayList;
import java.util.List;

/** UI Data Model for presenting a log of conversation items */
public class UIConversationLog {

    @NonNull private ConnectionStatus mConnectionStatus = ConnectionStatus.DISCONNECTED;
    private boolean mIsLoading = false;
    @Nullable private List<UIConversationItem> mData = new ArrayList<>();

    public static UIConversationLog getDefault() {
        return new UIConversationLog();
    }

    public static UIConversationLog getDisconnectedState() {
        return new UIConversationLog(
                ConnectionStatus.DISCONNECTED, /* isLoading= */ false, /* list= */ null);
    }

    public static UIConversationLog getLoadingState() {
        return new UIConversationLog(
                ConnectionStatus.CONNECTED, /* isLoading= */ true, new ArrayList<>());
    }

    /** Get Loaded State */
    public static UIConversationLog getLoadedState(@NonNull List<UIConversationItem> data) {
        return new UIConversationLog(ConnectionStatus.CONNECTED, /* isLoading= */ false, data);
    }

    private UIConversationLog() {}

    public UIConversationLog(
            @NonNull ConnectionStatus connectionStatus,
            boolean isLoading,
            @Nullable List<UIConversationItem> list) {
        mConnectionStatus = connectionStatus;
        mIsLoading = isLoading;
        mData = list;
    }

    @NonNull
    public ConnectionStatus getConnectionStatus() {
        return mConnectionStatus;
    }

    public boolean isLoading() {
        return mIsLoading;
    }

    @Nullable
    public List<UIConversationItem> getData() {
        return mData;
    }
}
