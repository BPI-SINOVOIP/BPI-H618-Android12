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

package com.android.car.dialer.ui.settings;

import android.bluetooth.BluetoothDevice;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;
import androidx.lifecycle.ViewModel;

import com.android.car.dialer.Constants;

import javax.inject.Inject;
import javax.inject.Named;

import dagger.hilt.android.lifecycle.HiltViewModel;

/**
 * ViewModel for {@link DialerSettingsFragment}
 */
@HiltViewModel
public class DialerSettingsViewModel extends ViewModel {

    private final LiveData<BluetoothDevice> mCurrentHfpDeviceLiveData;
    private final LiveData<Boolean> mHasHfpDeviceConnectedLiveData;

    @Inject
    public DialerSettingsViewModel(
            @Named("Hfp") LiveData<BluetoothDevice> currentHfpDeviceLiveData,
            @Named("Hfp") LiveData<Boolean> hasHfpDeviceConnectedLiveData) {
        mCurrentHfpDeviceLiveData = currentHfpDeviceLiveData;
        mHasHfpDeviceConnectedLiveData = hasHfpDeviceConnectedLiveData;
    }

    /**
     * Returns the LiveData for the current HFP device's name. Returns an empty string if there's no
     * device connected.
     */
    public LiveData<String> getCurrentHfpConnectedDeviceName() {
        return Transformations.map(mCurrentHfpDeviceLiveData, (device) ->
                device != null ? device.getName() : Constants.EMPTY_STRING);
    }

    /** Returns a {@link LiveData} which monitors if there are any connected HFP devices. */
    public LiveData<Boolean> hasHfpDeviceConnected() {
        return mHasHfpDeviceConnectedLiveData;
    }

}
