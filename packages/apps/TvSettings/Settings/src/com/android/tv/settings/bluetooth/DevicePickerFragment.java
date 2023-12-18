/*
 * Copyright (C) 2011 The Android Open Source Project
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

import android.app.settings.SettingsEnums;
import android.app.FragmentManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothDevicePicker;
import android.content.SharedPreferences;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;

import androidx.annotation.Keep;

import com.android.tv.settings.R;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.core.AbstractPreferenceController;

import java.util.List;

@Keep
public final class DevicePickerFragment extends DeviceListPreferenceFragment {
    private static final String KEY_BT_DEVICE_LIST = "bt_device_list";
    private static final String TAG = "DevicePickerFragment";
    private static final String SHARED_PREFERENCES_NAME = "bluetooth_settings";
    private static final String KEY_LAST_SELECTED_DEVICE = "last_selected_device";
    private static final String KEY_LAST_SELECTED_DEVICE_TIME = "last_selected_device_time";

    BluetoothProgressCategory mAvailableDevicesCategory;

    private boolean mNeedAuth;
    private String mLaunchPackage;
    private String mLaunchClass;

    public DevicePickerFragment newInstance() {
        return new DevicePickerFragment();
    }

    @Override
    void initPreferencesFromPreferenceScreen() {
        Intent intent = getActivity().getIntent();
        mNeedAuth = intent.getBooleanExtra(BluetoothDevicePicker.EXTRA_NEED_AUTH, false);
        setFilter(intent.getIntExtra(BluetoothDevicePicker.EXTRA_FILTER_TYPE,
                BluetoothDevicePicker.FILTER_TYPE_ALL));
        mLaunchPackage = intent.getStringExtra(BluetoothDevicePicker.EXTRA_LAUNCH_PACKAGE);
        mLaunchClass = intent.getStringExtra(BluetoothDevicePicker.EXTRA_LAUNCH_CLASS);
        mAvailableDevicesCategory = (BluetoothProgressCategory) findPreference(KEY_BT_DEVICE_LIST);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
    }

    @Deprecated
    public int getMetricsCategory() {
        return SettingsEnums.BLUETOOTH_DEVICE_PICKER;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
        addCachedDevices();
        mSelectedDevice = null;
            enableScanning();
            mAvailableDevicesCategory.setProgress(mBluetoothAdapter.isDiscovering());
    }

    @Override
    public void onStop() {
        // Try disable scanning no matter what, no effect if enableScanning has not been called
        disableScanning();
        super.onStop();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        /* Check if any device was selected, if no device selected
         * send  ACTION_DEVICE_SELECTED with a null device, otherwise
         * don't do anything */
        if (mSelectedDevice == null) {
            sendDevicePickedIntent(null);
        }
    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
		return null;
    }

    @Override
    void onDevicePreferenceClick(BluetoothDevicePreference btPreference) {

        disableScanning();
        persistSelectedDeviceInPicker(
                getActivity(), mSelectedDevice.getAddress());
        if ((btPreference.getCachedDevice().getBondState() ==
                BluetoothDevice.BOND_BONDED) || !mNeedAuth) {
            sendDevicePickedIntent(mSelectedDevice);
            finish();
        } else {
            super.onDevicePreferenceClick(btPreference);
        }
    }

    @Override
    public void onScanningStateChanged(boolean started) {
        super.onScanningStateChanged(started);
        started |= mScanEnabled;
        mAvailableDevicesCategory.setProgress(started);
    }

    public void onDeviceBondStateChanged(CachedBluetoothDevice cachedDevice,
            int bondState) {
        BluetoothDevice device = cachedDevice.getDevice();
        if (!device.equals(mSelectedDevice)) {
            return;
        }
        if (bondState == BluetoothDevice.BOND_BONDED) {
            sendDevicePickedIntent(device);
            finish();
        } else if (bondState == BluetoothDevice.BOND_NONE) {
            enableScanning();
        }
    }

    @Override
    protected void initDevicePreference(BluetoothDevicePreference preference) {
        super.initDevicePreference(preference);
    }

    @Override
    public void onBluetoothStateChanged(int bluetoothState) {
        super.onBluetoothStateChanged(bluetoothState);

        if (bluetoothState == BluetoothAdapter.STATE_ON) {
            enableScanning();
        }
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.device_picker;
    }

    @Override
    public String getDeviceListKey() {
        return KEY_BT_DEVICE_LIST;
    }

    private void sendDevicePickedIntent(BluetoothDevice device) {
        Intent intent = new Intent(BluetoothDevicePicker.ACTION_DEVICE_SELECTED);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        if (mLaunchPackage != null && mLaunchClass != null) {
            intent.setClassName(mLaunchPackage, mLaunchClass);
        }
        getActivity().sendBroadcast(intent);
    }

    static void persistSelectedDeviceInPicker(Context context, String deviceAddress) {
        SharedPreferences.Editor editor = getSharedPreferences(context).edit();
        editor.putString(KEY_LAST_SELECTED_DEVICE,
                deviceAddress);
        editor.putLong(KEY_LAST_SELECTED_DEVICE_TIME,
                System.currentTimeMillis());
        editor.apply();
    }

    private static SharedPreferences getSharedPreferences(Context context) {
        return context.getSharedPreferences(SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
    }

    public void finish() {
        getActivity().finish();
    }
}
