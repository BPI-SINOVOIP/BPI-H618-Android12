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

package com.android.car.dialer.ui.common;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;
import androidx.lifecycle.ViewModel;

import com.android.car.dialer.R;
import com.android.car.dialer.livedata.SharedPreferencesLiveData;
import com.android.car.dialer.ui.common.entity.ContactSortingInfo;

import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * Superclass View Model. Monitors the sort method used for lists.
 */
public abstract class DialerListViewModel extends ViewModel {

    private final SharedPreferencesLiveData mSharedPreferencesLiveData;
    private final LiveData<Integer> mSortOrderLiveData;

    public DialerListViewModel(
            @ApplicationContext Context context,
            SharedPreferencesLiveData.Factory sharedPreferencesFactory) {
        mSharedPreferencesLiveData = sharedPreferencesFactory.create(R.string.sort_order_key);
        mSortOrderLiveData = Transformations.map(mSharedPreferencesLiveData,
                sharedPreferences -> ContactSortingInfo.getSortingInfo(context,
                        mSharedPreferencesLiveData).second);
    }

    /**
     * Returns SharedPreferences live data.
     */
    public SharedPreferencesLiveData getSharedPreferencesLiveData() {
        return mSharedPreferencesLiveData;
    }

    /**
     * Returns sort method live data.
     */
    public LiveData<Integer> getSortOrderLiveData() {
        return mSortOrderLiveData;
    }
}
