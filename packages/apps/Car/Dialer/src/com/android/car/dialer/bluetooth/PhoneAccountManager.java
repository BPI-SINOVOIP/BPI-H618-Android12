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

package com.android.car.dialer.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.dialer.Constants;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import javax.inject.Inject;
import javax.inject.Singleton;

/** User phone account handle management. */
@Singleton
public class PhoneAccountManager {
    private final TelecomManager mTelecomManager;
    private final BluetoothAdapter mBluetoothAdapter;

    @Inject
    public PhoneAccountManager(
            TelecomManager telecomManager,
            @Nullable BluetoothAdapter bluetoothAdapter) {
        mTelecomManager = telecomManager;
        mBluetoothAdapter = bluetoothAdapter;
    }

    /**
     * Sets the {@link PhoneAccountHandle} associated with the {@link BluetoothDevice} as the
     * user selected outgoing phone account.
     */
    public void setUserSelectedOutgoingDevice(BluetoothDevice device) {
        PhoneAccountHandle phoneAccountHandle = getMatchingPhoneAccount(device);
        mTelecomManager.setUserSelectedOutgoingPhoneAccount(phoneAccountHandle);
    }

    /**
     * Returns the {@link BluetoothDevice} for the given {@link PhoneAccountHandle} if the account
     * is for hfp connection.
     */
    public BluetoothDevice getMatchingDevice(
            @Nullable PhoneAccountHandle phoneAccountHandle) {
        Set<BluetoothDevice> bondedDevices =
                mBluetoothAdapter == null ? null : mBluetoothAdapter.getBondedDevices();
        if (bondedDevices == null) {
            return null;
        }
        if (isHfpConnectionService(phoneAccountHandle)) {
            for (BluetoothDevice bluetoothDevice : bondedDevices) {
                if (TextUtils.equals(bluetoothDevice.getAddress(), phoneAccountHandle.getId())) {
                    return bluetoothDevice;
                }
            }
        }
        return null;
    }

    /** Returns the list of hfp {@link BluetoothDevice}s for current callable phone accounts. */
    @NonNull
    public List<BluetoothDevice> getHfpDeviceList() {
        List<PhoneAccountHandle> phoneAccountHandles =
                mTelecomManager.getCallCapablePhoneAccounts(true);
        List<BluetoothDevice> hfpDeviceList = new ArrayList<>();
        for (PhoneAccountHandle phoneAccountHandle : phoneAccountHandles) {
            BluetoothDevice bluetoothDevice = getMatchingDevice(phoneAccountHandle);
            if (bluetoothDevice != null) {
                hfpDeviceList.add(bluetoothDevice);
            }
        }
        return hfpDeviceList;
    }

    /** Returns the {@link PhoneAccountHandle} for the given bluetooth device. */
    public PhoneAccountHandle getMatchingPhoneAccount(@Nullable BluetoothDevice bluetoothDevice) {
        if (bluetoothDevice == null) {
            return null;
        }

        List<PhoneAccountHandle> phoneAccountHandleList =
                mTelecomManager.getCallCapablePhoneAccounts(/* includeDisabledAccounts= */true);
        for (PhoneAccountHandle phoneAccountHandle : phoneAccountHandleList) {
            if (isHfpConnectionService(phoneAccountHandle)) {
                if (TextUtils.equals(phoneAccountHandle.getId(), bluetoothDevice.getAddress())) {
                    return phoneAccountHandle;
                }
            }
        }
        return null;
    }

    /** Returns if the {@link PhoneAccountHandle} is from a hfp connection for bluetooth call. */
    public boolean isHfpConnectionService(@Nullable PhoneAccountHandle phoneAccountHandle) {
        return phoneAccountHandle != null
                && Constants.HFP_CLIENT_CONNECTION_SERVICE_CLASS_NAME.equals(
                phoneAccountHandle.getComponentName().getClassName());
    }
}
