/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.bluetooth.BluetoothDevice;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Observer;

import com.android.car.dialer.log.L;

import java.util.List;
import java.util.Set;

import javax.inject.Inject;
import javax.inject.Singleton;

/**
 * Class that responsible for getting status of bluetooth connections.
 */
@Singleton
public final class UiBluetoothMonitor {
    private static final String TAG = "CD.BtMonitor";

    private BluetoothPairListLiveData mPairListLiveData;
    private BluetoothStateLiveData mBluetoothStateLiveData;
    private HfpDeviceListLiveData mHfpDeviceListLiveData;

    private Observer mPairListObserver;
    private Observer mBluetoothStateObserver;
    private Observer<List<BluetoothDevice>> mHfpDeviceListObserver;

    @Inject
    public UiBluetoothMonitor(
            BluetoothPairListLiveData bluetoothPairListLiveData,
            BluetoothStateLiveData bluetoothStateLiveData,
            HfpDeviceListLiveData hfpDeviceListLiveData) {
        mPairListLiveData = bluetoothPairListLiveData;
        mBluetoothStateLiveData = bluetoothStateLiveData;
        mHfpDeviceListLiveData = hfpDeviceListLiveData;

        mPairListObserver = o -> L.i(TAG, "PairList is updated");
        mBluetoothStateObserver = o -> L.i(TAG, "BluetoothState is updated");
        mHfpDeviceListObserver = deviceList -> L.i(TAG, "HfpDeviceList is updated");

        mPairListLiveData.observeForever(mPairListObserver);
        mBluetoothStateLiveData.observeForever(mBluetoothStateObserver);
        mHfpDeviceListLiveData.observeForever(mHfpDeviceListObserver);
    }

    /**
     * Stops the {@link UiBluetoothMonitor}. Call this function when Dialer goes to background.
     */
    public void tearDown() {
        removeObserver(mPairListLiveData, mPairListObserver);
        removeObserver(mBluetoothStateLiveData, mBluetoothStateObserver);
        removeObserver(mHfpDeviceListLiveData, mHfpDeviceListObserver);
    }

    /**
     * Returns a LiveData which monitors the paired device list changes.
     */
    public LiveData<Set<BluetoothDevice>> getPairListLiveData() {
        return mPairListLiveData;
    }

    /**
     * Returns a LiveData which monitors the Bluetooth state changes.
     */
    public LiveData<Integer> getBluetoothStateLiveData() {
        return mBluetoothStateLiveData;
    }

    /**
     * Returns a SingleLiveEvent which monitors whether to refresh Dialer.
     */
    public LiveData<List<BluetoothDevice>> getHfpDeviceListLiveData() {
        return mHfpDeviceListLiveData;
    }

    private void removeObserver(LiveData liveData, Observer observer) {
        if (liveData != null && liveData.hasObservers()) {
            liveData.removeObserver(observer);
        }
    }
}
