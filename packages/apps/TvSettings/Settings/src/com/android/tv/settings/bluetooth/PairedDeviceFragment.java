/*
 * Copyright (C) 2018 The Android Open Source Project
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
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;

import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.Set;

/**
 * The "Paired Device Management Fragment" screen in bt settings.
 */
@Keep
public class PairedDeviceFragment extends SettingsPreferenceFragment {
    private static final String TAG = "PairedDeviceFragment";

    private BluetoothAdapter mBtAdapter;
    private final BroadcastReceiver mBCMReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            updateAccessories();
        }
    };

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.paired_device_management, null);
    }

    @Deprecated
    public int getMetricsCategory() {
        return  MetricsProto.MetricsEvent.BLUETOOTH;
    }

    private void updateAccessories() {
        PreferenceScreen preferenceScreen = getPreferenceScreen();
        if (preferenceScreen == null) {
            return;
        }

        if (mBtAdapter == null) {
            preferenceScreen.removeAll();
            return;
        }

        final Set<BluetoothDevice> bondedDevices = mBtAdapter.getBondedDevices();
        if (bondedDevices == null) {
            preferenceScreen.removeAll();
            return;
        }

        final Context themedContext = getPreferenceManager().getContext();

        final Set<String> touchedKeys = new ArraySet<>(bondedDevices.size() + 1);

        for (final BluetoothDevice device : bondedDevices) {
            final String deviceAddress = device.getAddress();
            if (TextUtils.isEmpty(deviceAddress)) {
                Log.w(TAG, "Skipping mysteriously empty bluetooth device");
                continue;
            }

            final String desc = device.isConnected() ? getString(R.string.accessory_connected) :
                    null;
            final String key = "BluetoothDevice:" + deviceAddress;
            touchedKeys.add(key);
            Preference preference = preferenceScreen.findPreference(key);
            if (preference == null) {
                preference = new Preference(themedContext);
                preference.setKey(key);
            }
            final String deviceName = device.getAlias();
            preference.setTitle(deviceName);
            preference.setSummary(desc);
            final int deviceImgId = Utils.getImageIdForDevice(device);
            preference.setIcon(deviceImgId);
            preference.setFragment(PairedDeviceOperationFragment.class.getName());
            PairedDeviceOperationFragment.prepareArgs(
                    preference.getExtras(),
                    deviceAddress,
                    deviceName,
                    deviceImgId);
            preferenceScreen.addPreference(preference);
        }

        for (int i = 0; i < preferenceScreen.getPreferenceCount();) {
            final Preference preference = preferenceScreen.getPreference(i);
            if (touchedKeys.contains(preference.getKey())) {
                i++;
            } else {
                preferenceScreen.removePreference(preference);
            }
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        IntentFilter btChangeFilter = new IntentFilter();
        btChangeFilter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        btChangeFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        btChangeFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        getContext().registerReceiver(mBCMReceiver, btChangeFilter);
    }

    @Override
    public void onStop() {
        super.onStop();
        getContext().unregisterReceiver(mBCMReceiver);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateAccessories();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mBtAdapter = BluetoothAdapter.getDefaultAdapter();
        super.onCreate(savedInstanceState);
    }
}
