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

package com.android.car.dialer.bluetooth;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Observer;

import com.android.car.dialer.log.L;
import com.android.car.telephony.common.PhoneCallLog;

import java.util.List;

import javax.inject.Inject;
import javax.inject.Singleton;

/**
 * A class that monitors the call history data change.
 */
@Singleton
public final class CallHistoryManager {
    private static final String TAG = "CD.CallHistoryManager";

    private LiveData<List<PhoneCallLog>> mCallHistoryLiveData;

    private Observer mCallHistoryObserver;

    @Inject
    CallHistoryManager(LiveData<List<PhoneCallLog>> callHistoryLiveData) {
        mCallHistoryLiveData = callHistoryLiveData;

        mCallHistoryObserver = o -> L.i(TAG, "Call history is updated");

        // Call history live data is observed forever to avoid race condition between new call
        // log insertion timing and data change registering.
        mCallHistoryLiveData.observeForever(mCallHistoryObserver);
    }

    /**
     * Tears down the {@link CallHistoryManager}.
     */
    public void tearDown() {
        if (mCallHistoryLiveData != null && mCallHistoryLiveData.hasObservers()) {
            mCallHistoryLiveData.removeObserver(mCallHistoryObserver);
        }
    }

    /**
     * Returns a LiveData which monitors the call history data for the first connected device.
     */
    public LiveData<List<PhoneCallLog>> getCallHistoryLiveData() {
        return mCallHistoryLiveData;
    }
}
