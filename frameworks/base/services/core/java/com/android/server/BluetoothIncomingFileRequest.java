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
package com.android.server;

import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public final class BluetoothIncomingFileRequest extends BroadcastReceiver {
    private static final String TAG = "BluetoothIncomingFileRequest";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (!action.equals(BluetoothDevice.ACTION_INCOMINGFILE_REQUEST)) {
            return;
        }
        Log.d(TAG, "receive ACTION_INCOMINGFIL_REQUEST!");
        // convert broadcast intent into activity intent (same action string)
        BluetoothDevice device =
                intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        int type = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT,
                BluetoothDevice.ERROR);
        Intent incomingIntent = new Intent();
        incomingIntent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        incomingIntent.setAction(BluetoothDevice.ACTION_INCOMINGFILE_CONFIRM_REQUEST);
        incomingIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        context.startActivity(incomingIntent);
    }
}
