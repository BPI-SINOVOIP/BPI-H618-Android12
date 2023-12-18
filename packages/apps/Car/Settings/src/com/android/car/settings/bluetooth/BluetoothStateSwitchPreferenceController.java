/*
 * Copyright (C) 2020 The Android Open Source Project
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

import static android.car.hardware.power.PowerComponent.BLUETOOTH;
import static android.os.UserManager.DISALLOW_BLUETOOTH;

import android.bluetooth.BluetoothAdapter;
import android.car.drivingstate.CarUxRestrictions;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserManager;
import android.widget.Toast;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import com.android.car.settings.R;
import com.android.car.settings.common.ClickableWhileDisabledSwitchPreference;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PowerPolicyListener;
import com.android.car.settings.common.PreferenceController;
import com.android.settingslib.bluetooth.LocalBluetoothManager;

import java.util.function.Consumer;

/**
 * Enables/disables bluetooth state via SwitchPreference.
 */
public class BluetoothStateSwitchPreferenceController extends
        PreferenceController<ClickableWhileDisabledSwitchPreference> {

    private final Context mContext;
    private final IntentFilter mIntentFilter = new IntentFilter(
            BluetoothAdapter.ACTION_STATE_CHANGED);
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            handleStateChanged(state);
        }
    };
    private BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private LocalBluetoothManager mLocalBluetoothManager;
    private UserManager mUserManager;
    private boolean mUpdating = false;

    @VisibleForTesting
    final PowerPolicyListener mPowerPolicyListener;

    public BluetoothStateSwitchPreferenceController(Context context,
            String preferenceKey,
            FragmentController fragmentController,
            CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mContext = context;
        mPowerPolicyListener = new PowerPolicyListener(context, BLUETOOTH,
                isOn -> {
                    enableSwitchPreference(getPreference(), isOn, /* forPowerPolicy= */ true);
                });
    }

    @Override
    protected Class<ClickableWhileDisabledSwitchPreference> getPreferenceType() {
        return ClickableWhileDisabledSwitchPreference.class;
    }

    @Override
    protected void updateState(ClickableWhileDisabledSwitchPreference preference) {
        updateSwitchPreference(mBluetoothAdapter.getState() == BluetoothAdapter.STATE_ON
                || mBluetoothAdapter.getState() == BluetoothAdapter.STATE_TURNING_ON);
    }

    @Override
    protected boolean handlePreferenceChanged(ClickableWhileDisabledSwitchPreference preference,
            Object newValue) {
        if (mUpdating) {
            return false;
        }
        enableSwitchPreference(preference, /* enabled= */ false, /* forPowerPolicy= */ false);
        boolean bluetoothEnabled = (Boolean) newValue;
        if (bluetoothEnabled) {
            mBluetoothAdapter.enable();
        } else {
            mBluetoothAdapter.disable();
        }
        return true;
    }

    @Override
    protected void onCreateInternal() {
        mUserManager = UserManager.get(mContext);
        mLocalBluetoothManager = BluetoothUtils.getLocalBtManager(mContext);
        if (mLocalBluetoothManager == null) {
            getFragmentController().goBack();
        }
        getPreference().setContentDescription(
                mContext.getString(R.string.bluetooth_state_switch_content_description));
    }

    @Override
    protected void onStartInternal() {
        mContext.registerReceiver(mReceiver, mIntentFilter);
        mLocalBluetoothManager.setForegroundActivity(mContext);
        handleStateChanged(mBluetoothAdapter.getState());
    }

    @Override
    protected void onResumeInternal() {
        mPowerPolicyListener.handleCurrentPolicy();
    }

    @Override
    protected void onStopInternal() {
        mContext.unregisterReceiver(mReceiver);
        mLocalBluetoothManager.setForegroundActivity(null);
    }

    @Override
    protected void onDestroyInternal() {
        mPowerPolicyListener.release();
    }

    private boolean isUserRestricted() {
        return mUserManager.hasUserRestriction(DISALLOW_BLUETOOTH);
    }

    @VisibleForTesting
    void handleStateChanged(int state) {
        // Set updating state to prevent additional updates while trying to reflect the new
        // adapter state.
        mUpdating = true;
        switch (state) {
            case BluetoothAdapter.STATE_TURNING_ON:
                enableSwitchPreference(getPreference(), /* enabled= */ false,
                        /* forPowerPolicy= */ false);
                updateSwitchPreference(true);
                break;
            case BluetoothAdapter.STATE_ON:
                enableSwitchPreference(getPreference(), !isUserRestricted(),
                        /* forPowerPolicy= */ false);
                updateSwitchPreference(true);
                break;
            case BluetoothAdapter.STATE_TURNING_OFF:
                enableSwitchPreference(getPreference(), /* enabled= */ false,
                        /* forPowerPolicy= */ false);
                updateSwitchPreference(false);
                break;
            case BluetoothAdapter.STATE_OFF:
            default:
                enableSwitchPreference(getPreference(), !isUserRestricted(),
                        /* forPowerPolicy= */ false);
                updateSwitchPreference(false);
        }
        mUpdating = false;
    }

    @VisibleForTesting
    void setUserManager(UserManager userManager) {
        mUserManager = userManager;
    }

    private void updateSwitchPreference(boolean enabled) {
        String bluetoothName = BluetoothAdapter.getDefaultAdapter().getName();
        getPreference().setSummary(enabled ? getContext()
                .getString(R.string.bluetooth_state_switch_summary, bluetoothName) : null);
        getPreference().setChecked(enabled);
    }

    private void enableSwitchPreference(ClickableWhileDisabledSwitchPreference preference,
            boolean enabled, boolean forPowerPolicy) {
        Consumer<Preference> listener = !forPowerPolicy ? null : p ->
                Toast.makeText(getContext(),
                        getContext().getString(R.string.power_component_disabled),
                        Toast.LENGTH_LONG).show();
        preference.setDisabledClickListener(listener);
        preference.setEnabled(enabled);
    }
}
