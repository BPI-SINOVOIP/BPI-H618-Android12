/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.car.dialer.inject;

import android.bluetooth.BluetoothDevice;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;

import com.android.car.dialer.bluetooth.PhoneAccountManager;
import com.android.car.dialer.bluetooth.UiBluetoothMonitor;

import java.util.List;
import java.util.Set;

import javax.inject.Named;
import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.components.SingletonComponent;

/**
 * Module providing dependencies that are overridden by car.dialer.framework modules.
 * This module is excluded in the test and emulator build variants.
 */
@InstallIn(SingletonComponent.class)
@Module
public final class BluetoothSingleHfpModule {
    @Singleton
    @Named("Bluetooth")
    @Provides
    static LiveData<Integer> provideBluetoothStateLiveData(
            UiBluetoothMonitor uiBluetoothMonitor) {
        return uiBluetoothMonitor.getBluetoothStateLiveData();
    }

    @Singleton
    @Named("Bluetooth")
    @Provides
    static LiveData<Set<BluetoothDevice>> provideBluetoothPairListLiveData(
            UiBluetoothMonitor uiBluetoothMonitor) {
        return uiBluetoothMonitor.getPairListLiveData();
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<List<BluetoothDevice>> provideHfpDeviceListLiveData(
            UiBluetoothMonitor uiBluetoothMonitor) {
        return uiBluetoothMonitor.getHfpDeviceListLiveData();
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<BluetoothDevice> provideCurrentHfpDeviceLiveData(
            @Named("Hfp") LiveData<List<BluetoothDevice>> hfpDeviceListLiveData,
            PhoneAccountManager phoneAccountManager) {
        LiveData<BluetoothDevice> currentHfpDevice = Transformations.map(hfpDeviceListLiveData,
                devices -> devices != null && !devices.isEmpty() ? devices.get(0) : null);
        currentHfpDevice.observeForever(
                device -> phoneAccountManager.setUserSelectedOutgoingDevice(device));
        return currentHfpDevice;
    }
}
