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

package com.android.car.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertisingSet;
import android.bluetooth.le.AdvertisingSetCallback;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.os.ParcelUuid;
import android.util.IndentingPrintWriter;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/**
 * The FastPairAdvertiser is responsible for the BLE advertisement of either the model ID while
 * in pairing mode or the stored account keys while not in pairing mode.
 *
 * Note that two different advertisers should be created and only one should be advertising at a
 * time.
 */
class FastPairAdvertiser {
    // Service ID assigned for FastPair.
    public static final ParcelUuid FastPairServiceUuid = ParcelUuid
            .fromString("0000FE2C-0000-1000-8000-00805f9b34fb");
    private static final String TAG = FastPairAdvertiser.class.getSimpleName();
    private static final boolean DBG = FastPairUtils.DBG;

    private final Callbacks mCallbacks;
    private final Context mContext;
    private final byte[] mFastPairModelData;

    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    private AdvertisingSetParameters mAdvertisingSetParameters;
    private AdvertiseData mData;
    private boolean mAdvertising = false;

    interface Callbacks {
        /**
         * Notify the Resolvable Private Address of the BLE advertiser.
         *
         * @param device The current LE address
         */
        void onRpaUpdated(BluetoothDevice device);
    }

    FastPairAdvertiser(Context context, int modelId, Callbacks callbacks) {
        mContext = context;
        mCallbacks = callbacks;
        ByteBuffer modelIdBytes = ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(
                modelId);
        mFastPairModelData = Arrays.copyOfRange(modelIdBytes.array(), 1, 4);
        initializeBluetoothLeAdvertiser();
    }

    /**
     * Advertise the model id when in pairing mode.
     */
    void advertiseModelId() {
        if (DBG) {
            Log.d(TAG, "AdvertiseModelId");
        }
        mAdvertisingSetParameters = new AdvertisingSetParameters.Builder()
                .setLegacyMode(true)
                .setInterval(AdvertisingSetParameters.INTERVAL_LOW)
                .setScannable(true)
                .setConnectable(true)
                .build();
        mData = new AdvertiseData.Builder()
                .addServiceUuid(FastPairServiceUuid)
                .addServiceData(FastPairServiceUuid, mFastPairModelData)
                .setIncludeTxPowerLevel(true)
                .build();
        startAdvertising();
    }

    /**
     * Advertise the stored account keys while not in pairing mode
     */
    void advertiseAccountKeys() {
        if (DBG) {
            Log.d(TAG, "AdvertiseAccountKeys");
        }
        mAdvertisingSetParameters = new AdvertisingSetParameters.Builder()
                .setLegacyMode(true)
                .setInterval(AdvertisingSetParameters.INTERVAL_MEDIUM)
                .setScannable(true)
                .setConnectable(true)
                .build();
        mData = new AdvertiseData.Builder()
                .addServiceUuid(FastPairServiceUuid)
                .addServiceData(FastPairServiceUuid,
                        FastPairUtils.getAccountKeyAdvertisement(mContext))
                .setIncludeTxPowerLevel(true)
                .build();
        startAdvertising();
    }

    /**
     * Stop advertising when it is time to shut down.
     */
    void stopAdvertising() {
        if (DBG) {
            Log.d(TAG, "stoppingAdvertising");
        }
        if (mBluetoothLeAdvertiser == null) return;
        mBluetoothLeAdvertiser.stopAdvertisingSet(mAdvertisingSetCallback);
    }

    /**
     * Attempt to set mBluetoothLeAdvertiser from the BluetoothAdapter
     *
     * Returns
     *      true if mBluetoothLeAdvertiser is set
     *      false if mBluetoothLeAdvertiser is still null
     */
    private boolean initializeBluetoothLeAdvertiser() {
        if (mBluetoothLeAdvertiser != null) return true;

        BluetoothManager bluetoothManager = mContext.getSystemService(BluetoothManager.class);
        if (bluetoothManager == null) return false;

        mBluetoothAdapter = bluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) return false;

        mBluetoothLeAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
        return (mBluetoothLeAdvertiser != null);
    }

    /**
     * Acquire the LE advertiser from the Bluetooth adapter, and if available start the configured
     * advertiser.
     */
    private void startAdvertising() {
        if (!initializeBluetoothLeAdvertiser()) return;
        if (!mAdvertising) {
            if (DBG) Log.d(TAG, "startingAdvertising");
            mBluetoothLeAdvertiser.startAdvertisingSet(mAdvertisingSetParameters, mData, null, null,
                    null, mAdvertisingSetCallback);
        }
    }

    /* Callback to handle changes in advertising. */
    private AdvertisingSetCallback mAdvertisingSetCallback = new AdvertisingSetCallback() {
        @Override
        public void onAdvertisingSetStarted(AdvertisingSet advertisingSet, int txPower,
                int status) {
            if (DBG) {
                Log.d(TAG, "onAdvertisingSetStarted(): txPower:" + txPower + " , status: "
                        + status);
            }
            mAdvertising = true;
            if (advertisingSet == null) return;
            advertisingSet.getOwnAddress();
        }

        @Override
        public void onAdvertisingSetStopped(AdvertisingSet advertisingSet) {
            if (DBG) Log.d(TAG, "onAdvertisingSetStopped():");
            mAdvertising = false;
        }

        @Override
        public void onOwnAddressRead(AdvertisingSet advertisingSet, int addressType,
                String address) {
            if (DBG) Log.d(TAG, "onOwnAddressRead Type= " + addressType + " Address= " + address);
            mCallbacks.onRpaUpdated(mBluetoothAdapter.getRemoteDevice(address));
        }
    };

    public void dump(IndentingPrintWriter writer) {
        if (mAdvertising) {
            writer.println("Currently advertising         : " + mData);
        }
    }
}
