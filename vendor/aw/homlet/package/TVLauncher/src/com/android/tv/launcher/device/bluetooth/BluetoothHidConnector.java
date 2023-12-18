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

package com.android.tv.launcher.device.bluetooth;

import android.bluetooth.BluetoothHidHost;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;


public class BluetoothHidConnector {

    public static final String TAG = "BluetoothHidConnector";

    private static final boolean DEBUG = false;

    private Context mContext;
    private BluetoothDevice mTarget;
    private BluetoothHidHost mHidProfile;
    private boolean mConnectionStateReceiverRegistered = false;

    private BroadcastReceiver mConnectionStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            BluetoothDevice device = (BluetoothDevice) intent.getParcelableExtra(
                    BluetoothDevice.EXTRA_DEVICE);
            if (DEBUG) {
                Log.d(TAG, "There was a connection status change for: " + device.getAddress());
            }

            if (device.equals(mTarget)) {
                int previousState = intent.getIntExtra(
                        BluetoothProfile.EXTRA_PREVIOUS_STATE,
                        BluetoothProfile.STATE_CONNECTING);
                int state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE,
                        BluetoothProfile.STATE_CONNECTING);

                if (DEBUG) {
                    Log.d(TAG, "Connection states: old = " + previousState + ", new = " + state);
                }

                if (previousState == BluetoothProfile.STATE_CONNECTING) {
                    if (state == BluetoothProfile.STATE_CONNECTED) {
                        Log.v(TAG, "Connect succeed");
                    } else if (state == BluetoothProfile.STATE_DISCONNECTED) {
                        Log.d(TAG, "Failed to connect");
                    }

                    unregisterConnectionStateReceiver();
                    closeHidProfileProxy();
                }
            }
        }
    };

    private BluetoothProfile.ServiceListener mServiceConnection =
            new BluetoothProfile.ServiceListener() {

        @Override
        public void onServiceDisconnected(int profile) {
            Log.w(TAG, "Service disconnected, perhaps unexpectedly");
            unregisterConnectionStateReceiver();
            closeHidProfileProxy();
        }

        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (DEBUG) {
                Log.d(TAG, "Connection made to bluetooth proxy." );
            }
            BluetoothHidHost mHidProfile = (BluetoothHidHost) proxy;
            if (DEBUG) {
                Log.d(TAG, "Connecting to target: " + mTarget.getAddress());
            }

            registerConnectionStateReceiver();

            // TODO need to start a timer, otherwise if the connection fails we might be
            // stuck here forever
            mHidProfile.connect(mTarget);

            // must set PRIORITY_AUTO_CONNECT or auto-connection will not
            // occur, however this setting does not appear to be sticky
            // across a reboot
            mHidProfile.setPriority(mTarget, BluetoothProfile.PRIORITY_AUTO_CONNECT);
        }
    };

    public BluetoothHidConnector(Context context, BluetoothDevice target) {
        mContext = context;
        mTarget = target;
    }

    public void openConnection(BluetoothAdapter adapter) {
        if (DEBUG) {
            Log.d(TAG, "opening connection");
        }
        if (!adapter.getProfileProxy(mContext, mServiceConnection, BluetoothProfile.HID_HOST)) {
            Log.v(TAG, "openConnection failed.");
        }
    }

    private void closeHidProfileProxy() {
        if (mHidProfile != null) {
            try {
                BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
                adapter.closeProfileProxy(BluetoothProfile.HID_HOST, mHidProfile);
                mHidProfile = null;
            } catch (Throwable t) {
                Log.w(TAG, "Error cleaning up HID proxy", t);
            }
        }
    }

    private void registerConnectionStateReceiver() {
        if (DEBUG) Log.d(TAG, "registerConnectionStateReceiver()");
        IntentFilter filter = new IntentFilter(BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED);
        mContext.registerReceiver(mConnectionStateReceiver, filter);
        mConnectionStateReceiverRegistered = true;
    }

    private void unregisterConnectionStateReceiver() {
        if (mConnectionStateReceiverRegistered) {
            if (DEBUG) Log.d(TAG, "unregisterConnectionStateReceiver()");
            mContext.unregisterReceiver(mConnectionStateReceiver);
            mConnectionStateReceiverRegistered = false;
        }
    }

}
