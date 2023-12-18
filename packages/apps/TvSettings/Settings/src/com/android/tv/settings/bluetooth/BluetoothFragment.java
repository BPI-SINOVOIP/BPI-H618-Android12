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

package com.android.tv.settings.bluetooth;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.text.TextUtils;
import android.text.BidiFormatter;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;
import androidx.preference.TwoStatePreference;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.tv.settings.R;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.bluetooth.BluetoothDeviceFilter;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.tv.settings.overlay.FeatureFactory;
import com.android.settingslib.core.lifecycle.Lifecycle;

import java.util.ArrayList;
import java.util.List;

/**
 * The "bluetooth" screen in TV settings.
 */
@Keep
public class BluetoothFragment extends DeviceListPreferenceFragment {
    private static final String TAG = "BluetoothFragment";
    private static final int PAIRED_DEVICE_ORDER = 1;

    static final String KEY_DEVICE_NAME = "device_name";
    static final String KEY_PAIRED_DEVICES = "paired_devices";
    static final String KEY_BLUETOOTH_ENABLE = "bluetooth_enable";
    static final String KEY_PAIRED_MANAGEMENT = "paired_device_management";
    static final String KEY_BT_PAIR = "bt_pair";
   	static final String KEY_BT_RENAME_DEVICE = "bt_rename_device";

    PreferenceGroup mPairedDevicesCategory;
    private Preference mDeviceNamePreference;
    private Preference mPairingPreference;
    private Preference mRenamePreference;
    private Preference mPairedManagementPreference;
    private TwoStatePreference mEnableBtPref;

    private AlwaysDiscoverable mAlwaysDiscoverable;
    BluetoothAdapter mBluetoothAdapter;
	IntentFilter filter;

    private BluetoothDeviceNamePreferenceController mDeviceNamePrefController;

    public static BluetoothFragment newInstance() {
        return new BluetoothFragment();
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Broadcast receiver is always running on the UI thread here,
            // so we don't need consider thread synchronization.
            String action = intent.getAction();
            if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                handleStateChanged(state);
            }
            if (BluetoothAdapter.ACTION_LOCAL_NAME_CHANGED.equals(action)) {
                if (mBluetoothAdapter != null && mBluetoothAdapter.isEnabled()) {
                    final CharSequence summary = TextUtils.expandTemplate(
                            getContext().getText(R.string.bluetooth_device_name_summary),
                            BidiFormatter.getInstance().unicodeWrap(mBluetoothAdapter.getName()));
                    mDeviceNamePreference.setSummary(summary);
                }
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mAlwaysDiscoverable = new AlwaysDiscoverable(getContext());
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_LOCAL_NAME_CHANGED);
        getContext().registerReceiver(mReceiver, filter);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.bluetooth_settings, null);
        mEnableBtPref = (TwoStatePreference) findPreference(KEY_BLUETOOTH_ENABLE);
        mPairingPreference = findPreference(KEY_BT_PAIR);
        mDeviceNamePreference = findPreference(KEY_DEVICE_NAME);
        mRenamePreference = findPreference(KEY_BT_RENAME_DEVICE);
        mPairedManagementPreference = findPreference(KEY_PAIRED_MANAGEMENT);

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }

    @Override
	public String getDeviceListKey() {
        return KEY_PAIRED_DEVICES;
	}

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.bluetooth_settings;
    }

    private void removePreference(@Nullable Preference preference) {
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
    }

    @Override
    public void onStart() {
        if (mBluetoothAdapter != null) {
            handleStateChanged(mBluetoothAdapter.getState());
        }
        super.onStart();
        mShowDevicesWithoutNames = true;
        if (mBluetoothAdapter != null) {
            updateContent(mBluetoothAdapter.getState());
        }
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onStop() {
        super.onStop();
        if (mBluetoothAdapter == null) {
            return;
        }
        // Make the device only visible to connected devices.
        if (mAlwaysDiscoverable != null) {
            mAlwaysDiscoverable.stop();
        }

    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        getContext().unregisterReceiver(mReceiver);
        if (mAlwaysDiscoverable != null) {
            mAlwaysDiscoverable.stop();
        }
    }

	@Override
	public boolean onPreferenceTreeClick(Preference preference) {
		switch (preference.getKey()) {
			case KEY_BLUETOOTH_ENABLE:
                if (mBluetoothAdapter != null) {
                    boolean status = setBluetoothEnabled(mEnableBtPref.isChecked());
					if (mEnableBtPref.isChecked() && !status) {
					    mEnableBtPref.setChecked(false);
					    mEnableBtPref.setEnabled(true);
					}
				} else {
				    Log.e(TAG, "BluetoothAdapter is null!!");
				}
				break;
		}
		return super.onPreferenceTreeClick(preference);
	}

    void handleStateChanged(int state) {
        switch (state) {
            case BluetoothAdapter.STATE_TURNING_ON:
                break;
            case BluetoothAdapter.STATE_ON:
				mEnableBtPref.setChecked(true);
                mEnableBtPref.setEnabled(true);
                break;
            case BluetoothAdapter.STATE_TURNING_OFF:
                break;
            case BluetoothAdapter.STATE_OFF:
				mEnableBtPref.setChecked(false);
                mEnableBtPref.setEnabled(true);
                break;
            default:
				mEnableBtPref.setChecked(false);
                mEnableBtPref.setEnabled(true);
        }
    }

    @Override
    void initPreferencesFromPreferenceScreen() {
        mPairedDevicesCategory = (PreferenceGroup) findPreference(KEY_PAIRED_DEVICES);
    }

    @Override
    public void onBluetoothStateChanged(int bluetoothState) {
        super.onBluetoothStateChanged(bluetoothState);
        updateContent(bluetoothState);
    }

    @Override
    public void onDeviceBondStateChanged(CachedBluetoothDevice cachedDevice, int bondState) {
        updateContent(mBluetoothAdapter.getState());
    }

    private void updateContent(int bluetoothState) {
        int messageId = 0;

        switch (bluetoothState) {
            case BluetoothAdapter.STATE_ON:
                mDevicePreferenceMap.clear();
                mDeviceNamePreference.setVisible(true);
                mPairingPreference.setVisible(true);
                mRenamePreference.setVisible(true);
                mPairedManagementPreference.setVisible(true);
                mPairedDevicesCategory.setVisible(true);
                addDeviceCategory(mPairedDevicesCategory,
                        R.string.bluetooth_preference_paired_devices,
                        BluetoothDeviceFilter.BONDED_DEVICE_FILTER, true);

                if (mAlwaysDiscoverable != null) {
                    mAlwaysDiscoverable.start();
                }
                return; // not break

            case BluetoothAdapter.STATE_TURNING_OFF:
                if (mBluetoothAdapter.isDiscovering()) {
                    mBluetoothAdapter.cancelDiscovery();
                }
                break;

            case BluetoothAdapter.STATE_OFF:
                mDeviceNamePreference.setVisible(false);
                mPairingPreference.setVisible(false);
                mRenamePreference.setVisible(false);
                mPairedManagementPreference.setVisible(false);
                mDevicePreferenceMap.clear();
                mPairedDevicesCategory.setVisible(false);
                break;

            case BluetoothAdapter.STATE_TURNING_ON:
                break;
        }
    }

    /**
     * Add a listener, which enables the advanced settings icon.
     *
     * @param preference the newly added preference
     */
    @Override
    void initDevicePreference(BluetoothDevicePreference preference) {
        preference.setOrder(PAIRED_DEVICE_ORDER);
    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
        final List<AbstractPreferenceController> controllers = new ArrayList<>(3);
        // getLifecycle() has been renamed to getSettingsLifecycle() in SettingsPreferenceFrament.java
        final Lifecycle lifecycle = getSettingsLifecycle();
        mDeviceNamePrefController = new BluetoothDeviceNamePreferenceController(context, lifecycle);
        controllers.add(mDeviceNamePrefController);
        controllers.add(new BluetoothDeviceRenamePreferenceController(context));
        controllers.add(new BluetoothFilesPreferenceController(context));

        return controllers;
    }

    @Deprecated
    public int getMetricsCategory() {
        return MetricsEvent.BLUETOOTH;
    }

    private boolean setBluetoothEnabled(boolean isEnabled) {
        return isEnabled ? mBluetoothAdapter.enable() : mBluetoothAdapter.disable();
    }
}
