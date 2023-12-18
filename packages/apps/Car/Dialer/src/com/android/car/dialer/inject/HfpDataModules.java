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
import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;
import androidx.lifecycle.ViewModel;

import com.android.car.arch.common.LiveDataFunctions;
import com.android.car.dialer.livedata.CallHistoryLiveData;
import com.android.car.dialer.storage.FavoriteNumberRepository;
import com.android.car.dialer.ui.favorite.BluetoothFavoriteContactsLiveData;
import com.android.car.telephony.common.Contact;
import com.android.car.telephony.common.InMemoryPhoneBook;
import com.android.car.telephony.common.PhoneCallLog;

import java.util.List;

import javax.inject.Named;
import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.components.ActivityRetainedComponent;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.android.scopes.ActivityRetainedScoped;
import dagger.hilt.components.SingletonComponent;

/**
 * Module providing dependencies for single hfp connection.
 * This module is excluded in the emulator build variant.
 */
public final class HfpDataModules {
    /** Single hfp application level dependencies. */
    @InstallIn(SingletonComponent.class)
    @Module
    public static final class AppModule {
        /**
         * This {@link LiveData} for call logs will be always be active. See {@link
         * com.android.car.dialer.bluetooth.CallHistoryManager}.
         */
        @Singleton
        @Provides
        static LiveData<List<PhoneCallLog>> provideCallHistoryLiveData(
                @ApplicationContext Context context,
                @Named("Hfp") LiveData<BluetoothDevice> currentHfpDevice) {
            return LiveDataFunctions.switchMapNonNull(currentHfpDevice,
                    device -> CallHistoryLiveData.newInstance(context, device.getAddress()));
        }

        @Singleton
        @Provides
        static LiveData<List<Contact>> provideContactListLiveData(
                @Named("Hfp") LiveData<BluetoothDevice> currentHfpDevice) {
            return LiveDataFunctions.switchMapNonNull(currentHfpDevice,
                    device -> InMemoryPhoneBook.get().getContactsLiveDataByAccount(
                            device.getAddress()));
        }
    }

    /** {@link LiveData} instances that are shared across various {@link ViewModel}s. */
    @InstallIn(ActivityRetainedComponent.class)
    @Module
    public static final class ActivityRetainedModule {
        @ActivityRetainedScoped
        @Named("Hfp")
        @Provides
        static LiveData<Boolean> hasHfpDeviceConnectedLiveData(
                @Named("Hfp") LiveData<List<BluetoothDevice>> hfpDeviceListLiveData) {
            return Transformations.map(hfpDeviceListLiveData,
                    devices -> devices != null && !devices.isEmpty());
        }

        @ActivityRetainedScoped
        @Provides
        @Named("BluetoothFavorite")
        static LiveData<List<Contact>> provideBluetoothFavoriteContactListLiveData(
                @Named("Hfp") LiveData<BluetoothDevice> currentHfpDevice,
                BluetoothFavoriteContactsLiveData.Factory bleFavoriteContactsLiveDataFactory) {
            return LiveDataFunctions.switchMapNonNull(currentHfpDevice,
                    device -> bleFavoriteContactsLiveDataFactory.create(device.getAddress()));
        }

        @ActivityRetainedScoped
        @Provides
        @Named("LocalFavorite")
        static LiveData<List<Contact>> provideLocalFavoriteContactListLiveData(
                @Named("Hfp") LiveData<BluetoothDevice> currentHfpDevice,
                FavoriteNumberRepository favoriteNumberRepository) {
            return LiveDataFunctions.switchMapNonNull(currentHfpDevice,
                    device -> favoriteNumberRepository.getFavoriteContacts(device.getAddress()));
        }
    }
}
