/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.bluetooth;

import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.Intent;
import android.util.Log;

/**
 * BluetoothPairingRequest is a receiver for any Bluetooth pairing request. It
 * starts the Bluetooth Pairing activity, displaying the PIN, the passkey or a
 * confirmation entry dialog.
 */
public final class BluetoothPairingRequest extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (!action.equals(BluetoothDevice.ACTION_PAIRING_REQUEST)) {
            return;
        }

        // convert broadcast intent into activity intent (same action string)
        BluetoothDevice device =
                intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        int type = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT,
                BluetoothDevice.ERROR);
        Intent pairingIntent = new Intent();
        pairingIntent.setClass(context, BluetoothPairingDialog.class);
        pairingIntent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        pairingIntent.putExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT, type);
        if (type == BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION ||
                type == BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY ||
                type == BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN) {
            int pairingKey = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY,
                    BluetoothDevice.ERROR);
            pairingIntent.putExtra(BluetoothDevice.EXTRA_PAIRING_KEY, pairingKey);
        }
        pairingIntent.setAction(BluetoothDevice.ACTION_PAIRING_REQUEST);
        pairingIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // In Canvas, always start the pairing activity when we get the pairing broadcast,
        // as opposed to displaying a notification that will start the pairing activity.
        mDevice = device;
        onPair(type, null);
        // context.startActivity(pairingIntent);
    }

    private BluetoothDevice mDevice;
    private boolean DEBUG = true;
    private String TAG = "BluetoothPairingRequest";
    private void onPair(int type, String value) {
        if (DEBUG) {
            Log.d(TAG, "onPair: " + value);
        }
        switch (type) {
            case BluetoothDevice.PAIRING_VARIANT_PIN:
                mDevice.setPin(value);
                break;

            case BluetoothDevice.PAIRING_VARIANT_PASSKEY:
                try {
                    int passkey = Integer.parseInt(value);
                } catch (NumberFormatException e) {
                    Log.d(TAG, "pass key " + value + " is not an integer");
                }
                break;

            case BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION:
            case BluetoothDevice.PAIRING_VARIANT_CONSENT:
                mDevice.setPairingConfirmation(true);
                break;

            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY:
            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN:
                // Do nothing.
                break;

            case BluetoothDevice.PAIRING_VARIANT_OOB_CONSENT:
                break;

            default:
                Log.e(TAG, "Incorrect pairing type received");
        }
    }
}
