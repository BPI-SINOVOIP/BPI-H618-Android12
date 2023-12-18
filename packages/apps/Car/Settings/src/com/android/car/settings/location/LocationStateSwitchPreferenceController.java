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

package com.android.car.settings.location;

import static android.car.hardware.power.PowerComponent.LOCATION;

import android.car.drivingstate.CarUxRestrictions;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.os.UserHandle;
import android.provider.Settings;
import android.widget.Toast;

import androidx.annotation.VisibleForTesting;

import com.android.car.settings.R;
import com.android.car.settings.common.ClickableWhileDisabledSwitchPreference;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PowerPolicyListener;
import com.android.car.settings.common.PreferenceController;
import com.android.settingslib.Utils;

/**
 * Enables/disables location state via SwitchPreference.
 */
public class LocationStateSwitchPreferenceController extends
        PreferenceController<ClickableWhileDisabledSwitchPreference> {

    private static final IntentFilter INTENT_FILTER_LOCATION_MODE_CHANGED =
            new IntentFilter(LocationManager.MODE_CHANGED_ACTION);

    private final Context mContext;
    private final LocationManager mLocationManager;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            refreshUi();
        }
    };

    @VisibleForTesting
    final PowerPolicyListener mPowerPolicyListener;

    public LocationStateSwitchPreferenceController(Context context,
            String preferenceKey,
            FragmentController fragmentController,
            CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mContext = context;
        mLocationManager = context.getSystemService(LocationManager.class);
        mPowerPolicyListener = new PowerPolicyListener(context, LOCATION,
                isOn -> {
                    enableSwitchPreference(getPreference(), isOn);
                });
    }

    @Override
    protected Class<ClickableWhileDisabledSwitchPreference> getPreferenceType() {
        return ClickableWhileDisabledSwitchPreference.class;
    }

    @Override
    protected void updateState(ClickableWhileDisabledSwitchPreference preference) {
        updateSwitchPreference(preference, mLocationManager.isLocationEnabled());
    }

    @Override
    protected boolean handlePreferenceChanged(ClickableWhileDisabledSwitchPreference preference,
            Object newValue) {
        boolean locationEnabled = (Boolean) newValue;
        Utils.updateLocationEnabled(
                mContext,
                locationEnabled,
                UserHandle.myUserId(),
                Settings.Secure.LOCATION_CHANGER_SYSTEM_SETTINGS);
        return true;
    }

    @Override
    protected void onCreateInternal() {
        getPreference().setContentDescription(
                getContext().getString(R.string.location_state_switch_content_description));
        getPreference().setDisabledClickListener(p ->
                Toast.makeText(getContext(),
                        getContext().getString(R.string.power_component_disabled),
                        Toast.LENGTH_LONG).show());
    }

    @Override
    protected void onStartInternal() {
        mContext.registerReceiver(mReceiver, INTENT_FILTER_LOCATION_MODE_CHANGED);
    }

    @Override
    protected void onResumeInternal() {
        mPowerPolicyListener.handleCurrentPolicy();
    }

    @Override
    protected void onStopInternal() {
        mContext.unregisterReceiver(mReceiver);
    }

    @Override
    protected void onDestroyInternal() {
        mPowerPolicyListener.release();
    }

    private void updateSwitchPreference(ClickableWhileDisabledSwitchPreference preference,
            boolean enabled) {
        preference.setTitle(enabled ? R.string.car_ui_preference_switch_on
                : R.string.car_ui_preference_switch_off);
        preference.setChecked(enabled);
    }

    private void enableSwitchPreference(ClickableWhileDisabledSwitchPreference preference,
            boolean enabled) {
        preference.setEnabled(enabled);
    }
}
