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

package com.android.car.dialer.storage;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.Nullable;

import java.util.Collections;
import java.util.Set;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

/**
 * Broadcast receiver that monitors the bluetooth device unpair event and removes entries for
 * devices that has been unpaired.
 */
@AndroidEntryPoint(BroadcastReceiver.class)
public class BluetoothBondedListReceiver extends Hilt_BluetoothBondedListReceiver {
    @Inject FavoriteNumberRepository mFavoriteNumberRepository;
    @Nullable BluetoothAdapter mBluetoothAdapter;

    @Override
    public void onReceive(Context context, Intent intent) {
        super.onReceive(context, intent);
        if (intent.getAction() != BluetoothDevice.ACTION_BOND_STATE_CHANGED) {
            return;
        }

        if (intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.BOND_NONE)
                == BluetoothDevice.BOND_NONE) {
            Set<BluetoothDevice> pairedDevices = mBluetoothAdapter == null ? Collections.emptySet()
                    : mBluetoothAdapter.getBondedDevices();
            mFavoriteNumberRepository.cleanup(pairedDevices);
        }
    }
}
