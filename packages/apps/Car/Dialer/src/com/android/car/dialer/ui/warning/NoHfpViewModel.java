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

package com.android.car.dialer.ui.warning;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModel;

import javax.inject.Inject;
import javax.inject.Named;

import dagger.hilt.android.lifecycle.HiltViewModel;

/** View model for {@link NoHfpFragment} */
@HiltViewModel
public class NoHfpViewModel extends ViewModel {

    private final LiveData<Boolean> mHasHfpDeviceConnectedLiveData;
    private final BluetoothErrorStringLiveData mBluetoothErrorStringLiveData;

    @Inject
    NoHfpViewModel(
            @Named("Hfp") LiveData<Boolean> hasHfpDeviceConnectedLiveData,
            BluetoothErrorStringLiveData bluetoothErrorStringLiveData) {
        mHasHfpDeviceConnectedLiveData = hasHfpDeviceConnectedLiveData;
        mBluetoothErrorStringLiveData = bluetoothErrorStringLiveData;
    }

    public LiveData<String> getBluetoothErrorStringLiveData() {
        return mBluetoothErrorStringLiveData;
    }

    /** Returns a {@link LiveData} which monitors if there are any connected HFP devices. */
    public LiveData<Boolean> hasHfpDeviceConnected() {
        return mHasHfpDeviceConnectedLiveData;
    }
}
