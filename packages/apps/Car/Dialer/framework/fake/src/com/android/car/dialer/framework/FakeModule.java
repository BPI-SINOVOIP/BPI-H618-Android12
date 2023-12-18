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

package com.android.car.dialer.framework;

import android.bluetooth.BluetoothDevice;
import android.telecom.TelecomManager;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;

import java.util.List;
import java.util.Set;

import javax.inject.Named;
import javax.inject.Singleton;

import dagger.Binds;
import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.components.SingletonComponent;

/** Dependencies used in testing app. */
@InstallIn(SingletonComponent.class)
@Module
public abstract class FakeModule {

    @Singleton
    @Named("Bluetooth")
    @Provides
    static LiveData<Integer> provideBluetoothStateLiveData(
            FakeHfpManager fakeHfpManager) {
        return fakeHfpManager.getBluetoothStateLiveData();
    }

    @Singleton
    @Named("Bluetooth")
    @Provides
    static LiveData<Set<BluetoothDevice>> provideBluetoothPairListLiveData(
            FakeHfpManager fakeHfpManager) {
        return fakeHfpManager.getBluetoothPairListLiveData();
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<List<BluetoothDevice>> provideHfpDeviceListLiveData(
            FakeHfpManager fakeHfpManager) {
        return fakeHfpManager.getHfpDeviceListLiveData();
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<BluetoothDevice> provideCurrentHfpDeviceLiveData(
            @Named("Hfp") LiveData<List<BluetoothDevice>> hfpDeviceListLiveData) {
        LiveData<BluetoothDevice> currentHfpDevice = Transformations.map(hfpDeviceListLiveData,
                devices -> devices != null && !devices.isEmpty() ? devices.get(0) : null);
        return currentHfpDevice;
    }

    @Provides
    static TelecomManager provideTelecomManager(FakeTelecomManager fakeTelecomManager) {
        return fakeTelecomManager.getTelecomManager();
    }

    @Binds
    abstract AndroidFramework bindAndroidFramework(AndroidFrameworkImpl androidFramework);
}
