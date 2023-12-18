/*
 * Copyright (C) 2017 The Android Open Source Project
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

import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import android.text.BidiFormatter;
import android.text.TextUtils;
import android.util.Log;

import com.android.tv.settings.R;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnStart;
import com.android.settingslib.core.lifecycle.events.OnStop;

/**
 * Controller that shows and updates the bluetooth device name
 */
public class BluetoothDeviceNamePreferenceController extends AbstractPreferenceController
        implements LifecycleObserver, OnStart, OnStop {
    private static final String TAG = "BluetoothNamePrefCtrl";

    public static final String KEY_DEVICE_NAME = "device_name";


    @VisibleForTesting
    Preference mPreference;
    protected BluetoothAdapter mBluetoothAdapter;

    public BluetoothDeviceNamePreferenceController(Context context, Lifecycle lifecycle) {
    	this(context);
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Bluetooth is not supported on this device");
            return;
        }
        lifecycle.addObserver(this);
    }

    BluetoothDeviceNamePreferenceController(Context context) {
        super(context);
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        mPreference = screen.findPreference(getPreferenceKey());
        super.displayPreference(screen);
    }

    @Override
    public void onStart() {
    }

    @Override
    public void onStop() {
    }

    @Override
    public boolean isAvailable() {
        return mBluetoothAdapter != null;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_DEVICE_NAME;
    }

    @Override
    public void updateState(Preference preference) {
        updateDeviceName(preference, mBluetoothAdapter.getName());
    }

    /**
     * Create preference to show bluetooth device name
     *
     * @param screen to add the preference in
     * @param order  to decide position of the preference
     * @return bluetooth preference that created in this method
     */
    public Preference createBluetoothDeviceNamePreference(PreferenceScreen screen, int order) {
        mPreference = new Preference(screen.getContext());
        mPreference.setOrder(order);
        mPreference.setKey(KEY_DEVICE_NAME);
        screen.addPreference(mPreference);

        return mPreference;
    }

    /**
     * Update device summary with {@code deviceName}, where {@code deviceName} has accent color
     *
     * @param preference to set the summary for
     * @param deviceName bluetooth device name to show in the summary
     */
    protected void updateDeviceName(final Preference preference, final String deviceName) {
        if (deviceName == null) {
            // TODO: show error message in preference subtitle
            return;
        }
        final CharSequence summary = TextUtils.expandTemplate(
                mContext.getText(R.string.bluetooth_device_name_summary),
                BidiFormatter.getInstance().unicodeWrap(deviceName));
        preference.setSelectable(false);
        preference.setSummary(summary);
    }
}
