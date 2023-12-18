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

package com.android.car.messenger.core.ui.launcher;

import android.app.Application;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.interfaces.DataModel;
import com.android.car.messenger.core.models.UserAccount;

import java.util.List;
import java.util.stream.Collectors;

/** View model for MessageLauncherActivity */
public class MessageLauncherViewModel extends AndroidViewModel {
    @NonNull private final DataModel mDataSource;
    @Nullable private LiveData<List<UserAccount>> mAccountsLiveData;
    // We currently only support the primary account until multi-account support is added
    private static final int DEVICE_LIMIT = 1;

    public MessageLauncherViewModel(@NonNull Application application) {
        super(application);
        mDataSource = AppFactory.get().getDataModel();
    }

    /** Get observable data with list of accounts/user accounts */
    @NonNull
    public LiveData<List<UserAccount>> getAccounts() {
        if (mAccountsLiveData == null) {
            mAccountsLiveData = getAccountList();
        }
        return mAccountsLiveData;
    }

    private LiveData<List<UserAccount>> getAccountList() {
        return Transformations.map(
                mDataSource.getAccounts(),
                accountList ->
                        accountList.stream().limit(DEVICE_LIMIT).collect(Collectors.toList()));
    }
}
