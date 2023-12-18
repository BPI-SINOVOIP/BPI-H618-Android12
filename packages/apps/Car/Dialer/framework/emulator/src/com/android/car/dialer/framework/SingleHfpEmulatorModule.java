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

package com.android.car.dialer.framework;

import android.bluetooth.BluetoothDevice;
import android.content.Context;

import androidx.lifecycle.LiveData;

import com.android.car.arch.common.LiveDataFunctions;
import com.android.car.dialer.livedata.CallHistoryLiveData;
import com.android.car.dialer.storage.FavoriteNumberRepository;
import com.android.car.dialer.ui.favorite.BluetoothFavoriteContactsLiveData;
import com.android.car.telephony.common.Contact;
import com.android.car.telephony.common.InMemoryPhoneBook;
import com.android.car.telephony.common.PhoneCallLog;

import java.util.Collections;
import java.util.List;
import java.util.Set;

import javax.inject.Named;
import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.components.SingletonComponent;

/** Module providing dependencies for single hfp connection running on the emulator. */
@InstallIn(SingletonComponent.class)
@Module
public final class SingleHfpEmulatorModule {
    @Singleton
    @Named("Bluetooth")
    @Provides
    static LiveData<Integer> provideBluetoothStateLiveData() {
        return LiveDataFunctions.dataOf(/*enabled*/2);
    }

    @Singleton
    @Named("Bluetooth")
    @Provides
    static LiveData<Set<BluetoothDevice>> provideBluetoothPairListLiveData() {
        return LiveDataFunctions.dataOf(Collections.emptySet());
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<List<BluetoothDevice>> provideHfpDeviceListLiveData() {
        return LiveDataFunctions.dataOf(Collections.emptyList());
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<BluetoothDevice> provideCurrentHfpDeviceLiveData() {
        return LiveDataFunctions.dataOf(null);
    }

    @Singleton
    @Named("Hfp")
    @Provides
    static LiveData<Boolean> hasHfpDeviceConnectedLiveData() {
        return LiveDataFunctions.dataOf(true);
    }

    @Provides
    static LiveData<List<PhoneCallLog>> provideCallHistoryLiveData(
            @ApplicationContext Context context) {
        return CallHistoryLiveData.newInstance(context, null);
    }

    @Provides
    static LiveData<List<Contact>> provideContactListLiveData() {
        return InMemoryPhoneBook.get().getContactsLiveDataByAccount(null);
    }

    @Provides
    @Named("BluetoothFavorite")
    static LiveData<List<Contact>> provideBluetoothFavoriteContactListLiveData(
            BluetoothFavoriteContactsLiveData.Factory bluetoothFavoriteContactsLiveDataFactory) {
        return bluetoothFavoriteContactsLiveDataFactory.create(null);
    }

    @Provides
    @Named("LocalFavorite")
    static LiveData<List<Contact>> provideLocalFavoriteContactListLiveData(
            FavoriteNumberRepository favoriteNumberRepository) {
        return favoriteNumberRepository.getFavoriteContacts(null);
    }
}
