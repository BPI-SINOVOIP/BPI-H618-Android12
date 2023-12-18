/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.car.settings.bluetooth;

import static android.os.UserManager.DISALLOW_CONFIG_BLUETOOTH;

import android.bluetooth.BluetoothProfile;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.UserManager;

import androidx.annotation.VisibleForTesting;
import androidx.preference.PreferenceGroup;

import com.android.car.settings.R;
import com.android.car.settings.common.CarUxRestrictionsHelper;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.MultiActionPreference;
import com.android.car.settings.common.ToggleButtonActionItem;
import com.android.settingslib.bluetooth.BluetoothDeviceFilter;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.bluetooth.LocalBluetoothManager;
import com.android.settingslib.bluetooth.LocalBluetoothProfile;


/**
 * Displays a list of bonded (paired) Bluetooth devices. Clicking on a device launch the device
 * details page. Additional buttons to will connect/disconnect from the device, toggle phone calls,
 * and toggle media audio.
 */
public class BluetoothBondedDevicesPreferenceController extends
        BluetoothDevicesGroupPreferenceController implements
        BluetoothDevicePreference.UpdateToggleButtonListener {

    private static final MultiActionPreference.ActionItem BLUETOOTH_BUTTON =
            MultiActionPreference.ActionItem.ACTION_ITEM1;
    private static final MultiActionPreference.ActionItem PHONE_BUTTON =
            MultiActionPreference.ActionItem.ACTION_ITEM2;
    private static final MultiActionPreference.ActionItem MEDIA_BUTTON =
            MultiActionPreference.ActionItem.ACTION_ITEM3;

    private boolean mShowDeviceDetails = true;

    public BluetoothBondedDevicesPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    @VisibleForTesting
    BluetoothBondedDevicesPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions,
            LocalBluetoothManager localBluetoothManager, UserManager userManager) {
        super(context, preferenceKey, fragmentController, uxRestrictions, localBluetoothManager,
                userManager);
    }

    @Override
    protected BluetoothDeviceFilter.Filter getDeviceFilter() {
        return BluetoothDeviceFilter.BONDED_DEVICE_FILTER;
    }

    @Override
    protected BluetoothDevicePreference createDevicePreference(CachedBluetoothDevice cachedDevice) {
        BluetoothDevicePreference pref = super.createDevicePreference(cachedDevice);
        pref.getActionItem(BLUETOOTH_BUTTON).setVisible(true);
        pref.getActionItem(PHONE_BUTTON).setVisible(true);
        pref.getActionItem(MEDIA_BUTTON).setVisible(true);
        pref.setToggleButtonUpdateListener(this);
        updateBluetoothActionItemAvailability(pref);
        updateActionAvailability(pref, true);

        return pref;
    }

    @Override
    protected void onDeviceClicked(CachedBluetoothDevice cachedDevice) {
        if (mShowDeviceDetails) {
            getFragmentController().launchFragment(
                    BluetoothDeviceDetailsFragment.newInstance(cachedDevice));
        }
    }

    @Override
    public void onDeviceBondStateChanged(CachedBluetoothDevice cachedDevice, int bondState) {
        refreshUi();
    }

    @Override
    protected void updateState(PreferenceGroup preferenceGroup) {
        super.updateState(preferenceGroup);

        boolean hasUserRestriction = getUserManager()
                .hasUserRestriction(DISALLOW_CONFIG_BLUETOOTH);
        updateActionAvailability(preferenceGroup, hasUserRestriction);
    }

    @Override
    protected void onApplyUxRestrictions(CarUxRestrictions uxRestrictions) {
        super.onApplyUxRestrictions(uxRestrictions);

        if (CarUxRestrictionsHelper.isNoSetup(uxRestrictions)) {
            updateActionAvailability(getPreference(), /* isRestricted= */ true);
        }
    }

    @Override
    public void updateToggleButtonState(BluetoothDevicePreference preference) {
        boolean hasUserRestriction = getUserManager()
                .hasUserRestriction(DISALLOW_CONFIG_BLUETOOTH);
        updateActionAvailability(preference, hasUserRestriction);
    }

    private void updateActionAvailability(PreferenceGroup group, boolean isRestricted) {
        for (int i = 0; i < group.getPreferenceCount(); i++) {
            BluetoothDevicePreference preference =
                    (BluetoothDevicePreference) group.getPreference(i);
            updateActionAvailability(preference, isRestricted);
        }
    }

    private void updateActionAvailability(BluetoothDevicePreference preference,
            boolean isRestricted) {
        if (!isRestricted) {
            setButtonsCheckedAndListeners(preference);
            mShowDeviceDetails = true;
        } else {
            updatePhoneActionItemAvailability(preference, true);
            updateMediaActionItemAvailability(preference, true);
            mShowDeviceDetails = false;
        }
    }

    private void toggleBluetoothConnectivity(boolean connect, CachedBluetoothDevice cachedDevice) {
        if (connect) {
            cachedDevice.connect();
        } else if (cachedDevice.isConnected()) {
            cachedDevice.disconnect();
        }
    }

    private void setButtonsCheckedAndListeners(BluetoothDevicePreference preference) {
        CachedBluetoothDevice cachedDevice = preference.getCachedDevice();

        // If device is currently attempting to connect/disconnect, disable further actions
        if (cachedDevice.isBusy()) {
            disableAllActionItems(preference);
            // There is a case where on creation the cached device will try to automatically connect
            // but does not report itself as busy yet. This ensures that the bluetooth button state
            // is correct (should be checked in either connecting or disconnecting states).
            preference.getActionItem(BLUETOOTH_BUTTON).setChecked(true);
            return;
        }

        LocalBluetoothProfile phoneProfile = null;
        LocalBluetoothProfile mediaProfile = null;
        for (LocalBluetoothProfile profile : cachedDevice.getProfiles()) {
            if (profile.getProfileId() == BluetoothProfile.HEADSET_CLIENT) {
                phoneProfile = profile;
            } else if (profile.getProfileId() == BluetoothProfile.A2DP_SINK) {
                mediaProfile = profile;
            }
        }
        LocalBluetoothProfile finalPhoneProfile = phoneProfile;
        LocalBluetoothProfile finalMediaProfile = mediaProfile;
        boolean isConnected = cachedDevice.isConnected();

        // Setup up bluetooth button
        updateBluetoothActionItemAvailability(preference);
        ToggleButtonActionItem bluetoothItem = preference.getActionItem(BLUETOOTH_BUTTON);
        bluetoothItem.setChecked(isConnected);
        bluetoothItem.setOnClickListener(
                isChecked -> {
                    if (cachedDevice.isBusy()) {
                        return;
                    }
                    // If trying to connect and both phone and media are disabled, connecting will
                    // always fail. In this case force both profiles on.
                    if (isChecked && finalPhoneProfile != null && finalMediaProfile != null
                            && !finalPhoneProfile.isEnabled(cachedDevice.getDevice())
                            && !finalMediaProfile.isEnabled(cachedDevice.getDevice())) {
                        finalPhoneProfile.setEnabled(cachedDevice.getDevice(), true);
                        finalMediaProfile.setEnabled(cachedDevice.getDevice(), true);
                    }
                    toggleBluetoothConnectivity(isChecked, cachedDevice);
                });

        if (phoneProfile == null || !isConnected) {
            // Disable phone button
            updatePhoneActionItemAvailability(preference, true);
        } else {
            // Enable phone button
            updatePhoneActionItemAvailability(preference, false);
            ToggleButtonActionItem phoneItem = preference.getActionItem(PHONE_BUTTON);
            boolean phoneEnabled = phoneProfile.isEnabled(cachedDevice.getDevice());
            phoneItem.setChecked(phoneEnabled);
            phoneItem.setOnClickListener(isChecked ->
                    finalPhoneProfile.setEnabled(cachedDevice.getDevice(), isChecked));
        }

        if (mediaProfile == null || !isConnected) {
            // Disable media button
            updateMediaActionItemAvailability(preference, true);
        } else {
            // Enable media button
            updateMediaActionItemAvailability(preference, false);
            ToggleButtonActionItem mediaItem = preference.getActionItem(MEDIA_BUTTON);
            boolean mediaEnabled = mediaProfile.isEnabled(cachedDevice.getDevice());
            mediaItem.setChecked(mediaEnabled);
            mediaItem.setOnClickListener(isChecked ->
                    finalMediaProfile.setEnabled(cachedDevice.getDevice(), isChecked));
        }
    }

    private void updateBluetoothActionItemAvailability(BluetoothDevicePreference preference) {
        // Run on main thread because recyclerview may still be computing layout
        getContext().getMainExecutor().execute(() -> {
            ToggleButtonActionItem bluetoothItem = preference.getActionItem(BLUETOOTH_BUTTON);
            bluetoothItem.setEnabled(true);
            bluetoothItem.setDrawable(getContext(), R.drawable.ic_bluetooth_button);
        });
    }

    private void updatePhoneActionItemAvailability(BluetoothDevicePreference preference,
            boolean isRestricted) {
        // Run on main thread because recyclerview may still be computing layout
        getContext().getMainExecutor().execute(() -> {
            ToggleButtonActionItem phoneItem = preference.getActionItem(PHONE_BUTTON);
            phoneItem.setEnabled(!isRestricted);
            phoneItem.setDrawable(getContext(), isRestricted
                    ? R.drawable.ic_bluetooth_phone_unavailable : R.drawable.ic_bluetooth_phone);
        });
    }

    private void updateMediaActionItemAvailability(BluetoothDevicePreference preference,
            boolean isRestricted) {
        // Run on main thread because recyclerview may still be computing layout
        getContext().getMainExecutor().execute(() -> {
            ToggleButtonActionItem mediaItem = preference.getActionItem(MEDIA_BUTTON);
            mediaItem.setEnabled(!isRestricted);
            mediaItem.setDrawable(getContext(), isRestricted
                    ? R.drawable.ic_bluetooth_media_unavailable : R.drawable.ic_bluetooth_media);
        });
    }

    private void disableAllActionItems(BluetoothDevicePreference preference) {
        // Run on main thread because recyclerview may still be computing layout
        getContext().getMainExecutor().execute(() -> {
            preference.getActionItem(BLUETOOTH_BUTTON).setEnabled(false);
            preference.getActionItem(PHONE_BUTTON).setEnabled(false);
            preference.getActionItem(MEDIA_BUTTON).setEnabled(false);
        });
    }
}
