/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.accessories;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;

import java.util.concurrent.TimeUnit;

public class BluetoothRequestPermissionActivity extends FragmentActivity {

    private static final String TAG = "BluetoothRequestPermissionActivity";

    private static final int DEFAULT_DISCOVERABLE_TIMEOUT_SECONDS = 30;
    private static final int MIN_DISCOVERABLE_TIMEOUT_SECONDS = 1;
    private static final int MAX_DISCOVERABLE_TIMEOUT_SECONDS = 300;

    private BluetoothAdapter mBluetoothAdapter;
    private int mTimeoutSeconds = DEFAULT_DISCOVERABLE_TIMEOUT_SECONDS;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Error: there's a problem starting Bluetooth");
            setResult(Activity.RESULT_CANCELED);
            finish();
            return;
        }

        if (!parseIntent()) {
            setResult(Activity.RESULT_CANCELED);
            finish();
            return;
        }

        mBluetoothAdapter.setScanMode(BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE,
                TimeUnit.SECONDS.toMillis(mTimeoutSeconds));

        setResult(Activity.RESULT_OK);
        finish();
    }

    private boolean parseIntent() {
        Intent intent = getIntent();
        if (intent == null) {
            return false;
        }

        if (intent.getAction().equals(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE)
                || intent.getAction().equals(BluetoothAdapter.ACTION_REQUEST_ENABLE)) {
            mTimeoutSeconds = intent.getIntExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION,
                    DEFAULT_DISCOVERABLE_TIMEOUT_SECONDS);
            if (mTimeoutSeconds < MIN_DISCOVERABLE_TIMEOUT_SECONDS
                    || mTimeoutSeconds > MAX_DISCOVERABLE_TIMEOUT_SECONDS) {
                mTimeoutSeconds = DEFAULT_DISCOVERABLE_TIMEOUT_SECONDS;
            }
            Log.d(TAG, "Setting Bluetooth Discoverable Timeout = " + mTimeoutSeconds);
        } else {
            Log.e(TAG, "Error: this activity may be started only with intent "
                    + BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
            return false;
        }

        return true;
    }
}
