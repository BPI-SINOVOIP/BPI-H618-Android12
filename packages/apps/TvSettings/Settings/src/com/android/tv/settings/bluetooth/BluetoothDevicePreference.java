/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.app.AlertDialog;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.UserManager;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import android.text.Html;
import android.text.TextUtils;
import android.util.Pair;
import android.util.TypedValue;
import android.widget.ImageView;

import com.android.tv.settings.R;
import com.android.tv.settings.overlay.FeatureFactory;
import com.android.settingslib.RestrictedPreference;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;

import static android.os.UserManager.DISALLOW_CONFIG_BLUETOOTH;

/**
 * BluetoothDevicePreference is the preference type used to display each remote
 * Bluetooth device in the Bluetooth Settings screen.
 */

public final class BluetoothDevicePreference extends Preference implements
        CachedBluetoothDevice.Callback {
    private static final String TAG = "BluetoothDevicePref";

    private final CachedBluetoothDevice mCachedDevice;

    private AlertDialog mDisconnectDialog;
    private String contentDescription = null;
    private DeviceListPreferenceFragment mDeviceListPreferenceFragment;
    /* Talk-back descriptions for various BT icons */
    Resources mResources;

    public BluetoothDevicePreference(Context context, CachedBluetoothDevice cachedDevice,
            DeviceListPreferenceFragment deviceListPreferenceFragment) {
        super(context, null);
        mResources = getContext().getResources();
        mDeviceListPreferenceFragment = deviceListPreferenceFragment;

        mCachedDevice = cachedDevice;
        mCachedDevice.registerCallback(this);

        onDeviceAttributesChanged();
    }

    void rebind() {
        notifyChanged();
    }

    CachedBluetoothDevice getCachedDevice() {
        return mCachedDevice;
    }

    @Override
    protected void onPrepareForRemoval() {
        super.onPrepareForRemoval();
        mCachedDevice.unregisterCallback(this);
        if (mDisconnectDialog != null) {
            mDisconnectDialog.dismiss();
            mDisconnectDialog = null;
        }
    }

    public CachedBluetoothDevice getBluetoothDevice() {
        return mCachedDevice;
    }

    public void onDeviceAttributesChanged() {
        /*
         * The preference framework takes care of making sure the value has
         * changed before proceeding. It will also call notifyChanged() if
         * any preference info has changed from the previous value.
         */
        setTitle(mCachedDevice.getName());
        // Null check is done at the framework
        if (mCachedDevice.isConnectedA2dpDevice()) {
            setSummary(mCachedDevice.getConnectionSummary());
        } else {
            final String desc = mCachedDevice.isConnected() ? "Connected" : null;
            setSummary(desc);
        }

        final Pair<Drawable, String> pair = Utils.getBtClassDrawableWithDescription(getContext(),
                mCachedDevice);
        if (pair.first != null) {
            setIcon(pair.first);
            contentDescription = pair.second;
        }

        // Used to gray out the item
        setEnabled(!mCachedDevice.isBusy());

        // Device is only visible in the UI if it has a valid name besides MAC address or when user
        // allows showing devices without user-friendly name in developer settings
        setVisible(mDeviceListPreferenceFragment.shouldShowDevicesWithoutNames()
                || mCachedDevice.hasHumanReadableName());

        // This could affect ordering, so notify that
        notifyHierarchyChanged();
    }

    @Override
    public boolean equals(Object o) {
        if ((o == null) || !(o instanceof BluetoothDevicePreference)) {
            return false;
        }
        return mCachedDevice.equals(
                ((BluetoothDevicePreference) o).mCachedDevice);
    }

    @Override
    public int hashCode() {
        return mCachedDevice.hashCode();
    }

    @Override
    public int compareTo(Preference another) {
        if (!(another instanceof BluetoothDevicePreference)) {
            // Rely on default sort
            return super.compareTo(another);
        }

        return mCachedDevice
                .compareTo(((BluetoothDevicePreference) another).mCachedDevice);
    }

    void onClicked() {
        Context context = getContext();
        int bondState = mCachedDevice.getBondState();

        if (mCachedDevice.isConnected()) {
            askDisconnect();
        } else if (bondState == BluetoothDevice.BOND_BONDED) {
            mCachedDevice.connect(true);
        } else if (bondState == BluetoothDevice.BOND_NONE) {
            pair();
        }
    }

    // Show disconnect confirmation dialog for a device.
    private void askDisconnect() {
        Context context = getContext();
        String name = mCachedDevice.getName();
        if (TextUtils.isEmpty(name)) {
            name = context.getString(R.string.bluetooth_device);
        }
        String message = context.getString(R.string.bluetooth_disconnect_all_profiles, name);
        String title = context.getString(R.string.bluetooth_disconnect_title);

        DialogInterface.OnClickListener disconnectListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                mCachedDevice.disconnect();
            }
        };

        mDisconnectDialog = Utils.showDisconnectDialog(context,
                mDisconnectDialog, disconnectListener, title, Html.fromHtml(message));
    }

    private void pair() {
        if (!mCachedDevice.startPairing()) {
            Utils.showError(getContext(), mCachedDevice.getName(),
                    R.string.bluetooth_pairing_error_message);
        }
    }

}
