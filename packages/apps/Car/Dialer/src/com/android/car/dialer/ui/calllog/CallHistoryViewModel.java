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

package com.android.car.dialer.ui.calllog;

import android.content.Context;
import android.text.format.DateUtils;

import androidx.lifecycle.LiveData;

import com.android.car.arch.common.FutureData;
import com.android.car.arch.common.LiveDataFunctions;
import com.android.car.dialer.bluetooth.CallHistoryManager;
import com.android.car.dialer.livedata.HeartBeatLiveData;
import com.android.car.dialer.livedata.SharedPreferencesLiveData;
import com.android.car.dialer.ui.common.DialerListViewModel;
import com.android.car.dialer.ui.common.UiCallLogLiveData;
import com.android.car.telephony.common.InMemoryPhoneBook;

import java.util.List;

import javax.inject.Inject;

import dagger.hilt.android.lifecycle.HiltViewModel;
import dagger.hilt.android.qualifiers.ApplicationContext;

/** View model for CallHistoryFragment which provides call history live data. */
@HiltViewModel
public class CallHistoryViewModel extends DialerListViewModel {
    private final CallHistoryManager mCallHistoryManager;

    private UiCallLogLiveData mUiCallLogLiveData;
    private LiveData<FutureData<List<Object>>> mUiCallLogFutureData;

    @Inject
    public CallHistoryViewModel(
            @ApplicationContext Context context,
            SharedPreferencesLiveData.Factory sharedPreferencesFactory,
            CallHistoryManager callHistoryManager) {
        super(context, sharedPreferencesFactory);
        mCallHistoryManager = callHistoryManager;
        mUiCallLogLiveData = new UiCallLogLiveData(context,
                new HeartBeatLiveData(DateUtils.MINUTE_IN_MILLIS),
                mCallHistoryManager.getCallHistoryLiveData(),
                InMemoryPhoneBook.get().getContactsLiveData());

        mUiCallLogFutureData = LiveDataFunctions.loadingSwitchMap(mUiCallLogLiveData,
                input -> LiveDataFunctions.dataOf(input));
    }

    /** Returns the {@link LiveData} for call history list {@link FutureData}. */
    public LiveData<FutureData<List<Object>>> getCallHistory() {
        return mUiCallLogFutureData;
    }
}
