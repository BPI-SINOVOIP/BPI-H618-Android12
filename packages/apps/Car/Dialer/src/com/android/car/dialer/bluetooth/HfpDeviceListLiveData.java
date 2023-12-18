/*
 * Copyright (C) 2019 The Android Open Source Project
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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.telecom.TelecomManager;

import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;

import java.util.List;

import javax.inject.Inject;
import javax.inject.Singleton;

import dagger.hilt.android.qualifiers.ApplicationContext;

/** {@link LiveData} that monitors the hfp connected devices. */
@Singleton
class HfpDeviceListLiveData extends MediatorLiveData<List<BluetoothDevice>> {
    private final Context mContext;
    private final BluetoothAdapter mBluetoothAdapter;
    private final PhoneAccountManager mPhoneAccountManager;
    private final IntentFilter mIntentFilter;

    private BroadcastReceiver mPhoneAccountUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            update();
        }
    };

    /** Creates a new {@link HfpDeviceListLiveData}. Call on main thread. */
    @Inject
    HfpDeviceListLiveData(
            @ApplicationContext Context context,
            @Nullable BluetoothAdapter bluetoothAdapter,
            PhoneAccountManager phoneAccountManager) {
        mContext = context;
        mBluetoothAdapter = bluetoothAdapter;
        mPhoneAccountManager = phoneAccountManager;

        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction(TelecomManager.ACTION_PHONE_ACCOUNT_REGISTERED);
        mIntentFilter.addAction(TelecomManager.ACTION_PHONE_ACCOUNT_UNREGISTERED);
        mIntentFilter.addAction(TelecomManager.ACTION_DEFAULT_DIALER_CHANGED);
    }

    @Override
    protected void onActive() {
        super.onActive();
        if (mBluetoothAdapter != null) {
            update();
            mContext.registerReceiver(mPhoneAccountUpdateReceiver, mIntentFilter);
        }
    }

    @Override
    protected void onInactive() {
        if (mBluetoothAdapter != null) {
            mContext.unregisterReceiver(mPhoneAccountUpdateReceiver);
        }
        super.onInactive();
    }

    private void update() {
        setValue(mPhoneAccountManager.getHfpDeviceList());
    }
}
