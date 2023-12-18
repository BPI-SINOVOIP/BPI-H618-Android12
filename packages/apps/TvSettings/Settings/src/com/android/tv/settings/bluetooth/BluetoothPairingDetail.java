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
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.Bundle;
import android.os.Bundle;
import android.util.Log;
import androidx.fragment.app.FragmentManager;
import androidx.annotation.Keep;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.tv.settings.R;
import com.android.settingslib.bluetooth.BluetoothDeviceFilter;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.core.AbstractPreferenceController;

import java.util.ArrayList;
import java.util.List;

/**
 * BluetoothPairingDetail is a page to scan bluetooth devices and pair them.
 */
@Keep
public class BluetoothPairingDetail extends DeviceListPreferenceFragment {
    private static final String TAG = "BluetoothPairingDetail";

    static final String KEY_AVAIL_DEVICES = "available_devices";
    BluetoothDeviceNamePreferenceController mDeviceNamePrefController;
    BluetoothProgressCategory mAvailableDevicesCategory;
    AlwaysDiscoverable mAlwaysDiscoverable;

    private boolean mInitialScanStarted;

    public static BluetoothPairingDetail newInstance() {
        return new BluetoothPairingDetail();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.bluetooth_pairing_detail, null);

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mInitialScanStarted = false;
        mAlwaysDiscoverable = new AlwaysDiscoverable(getContext());
    }

    @Override
    public void onStart() {
        super.onStart();

        updateContent(mBluetoothAdapter.getState());
        mAvailableDevicesCategory.setProgress(mBluetoothAdapter.isDiscovering());
    }

    @Override
    public void onStop() {
        super.onStop();

        // Make the device only visible to connected devices.
        mAlwaysDiscoverable.stop();
        disableScanning();
    }

    @Override
    void initPreferencesFromPreferenceScreen() {
        mAvailableDevicesCategory = (BluetoothProgressCategory) findPreference(KEY_AVAIL_DEVICES);
    }

    @Deprecated
    public int getMetricsCategory() {
        return MetricsEvent.BLUETOOTH_PAIRING;
    }

    @Override
    void enableScanning() {
        // Clear all device states before first scan
        if (!mInitialScanStarted) {
            if (mAvailableDevicesCategory != null) {
                removeAllDevices();
            }
            mLocalManager.getCachedDeviceManager().clearNonBondedDevices();
            mInitialScanStarted = true;
        ///AW CODE: [feat] change position of super.enableScanning
            super.enableScanning();
        }
        //super.enableScanning();
        ///AW:add end
    }

    @Override
    void onDevicePreferenceClick(BluetoothDevicePreference btPreference) {
        disableScanning();
        super.onDevicePreferenceClick(btPreference);
    }

    @Override
    public void onScanningStateChanged(boolean started) {
        super.onScanningStateChanged(started);
        started |= mScanEnabled;
        mAvailableDevicesCategory.setProgress(started);
    }

    void updateContent(int bluetoothState) {
        switch (bluetoothState) {
            case BluetoothAdapter.STATE_ON:
                mDevicePreferenceMap.clear();
                mBluetoothAdapter.enable();

                addDeviceCategory(mAvailableDevicesCategory,
                        R.string.bluetooth_preference_found_devices,
                        BluetoothDeviceFilter.UNBONDED_DEVICE_FILTER, mInitialScanStarted);
                mAlwaysDiscoverable.start();
                enableScanning();
                break;

            case BluetoothAdapter.STATE_OFF:
                finish();
                getActivity().onBackPressed();
                break;
        }
    }

	public void finish() {
        FragmentManager fm = getFragmentManager();
        fm.popBackStack();
	}

    @Override
    public void onBluetoothStateChanged(int bluetoothState) {
        super.onBluetoothStateChanged(bluetoothState);
        updateContent(bluetoothState);
    }

    @Override
    public void onDeviceBondStateChanged(CachedBluetoothDevice cachedDevice, int bondState) {
        if (bondState == BluetoothDevice.BOND_BONDED) {
            // If one device is connected(bonded), then close this fragment.
            finish();
            return;
        }
        if (mSelectedDevice != null && cachedDevice != null) {
            BluetoothDevice device = cachedDevice.getDevice();
            if (device != null && mSelectedDevice.equals(device)
                    && bondState == BluetoothDevice.BOND_NONE) {
                // If currently selected device failed to bond, restart scanning
                enableScanning();
            }
        }
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.bluetooth_pairing_detail;
    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
        List<AbstractPreferenceController> controllers = new ArrayList<>();
        // getLifecycle() has been renamed to getSettingsLifecycle() in SettingsPreferenceFrament.java
        mDeviceNamePrefController = new BluetoothDeviceNamePreferenceController(context,
                getSettingsLifecycle());
        controllers.add(mDeviceNamePrefController);

        return controllers;
    }

    @Override
    public String getDeviceListKey() {
        return KEY_AVAIL_DEVICES;
    }
}
