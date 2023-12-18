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

package com.android.car.settings.wifi;

import static android.car.hardware.power.PowerComponent.WIFI;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.widget.Toast;

import androidx.annotation.VisibleForTesting;

import com.android.car.settings.R;
import com.android.car.settings.common.ClickableWhileDisabledSwitchPreference;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PowerPolicyListener;
import com.android.car.settings.common.PreferenceController;

/**
 * Enables/disables Wifi state via SwitchPreference.
 */
public class WifiStateSwitchPreferenceController extends
        PreferenceController<ClickableWhileDisabledSwitchPreference>
        implements CarWifiManager.Listener {

    private final CarWifiManager mCarWifiManager;

    @VisibleForTesting
    final PowerPolicyListener mPowerPolicyListener;

    public WifiStateSwitchPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController,
            CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mPowerPolicyListener = new PowerPolicyListener(context, WIFI,
                isOn -> {
                    enableSwitchPreference(getPreference(), isOn);
                });
        mCarWifiManager = new CarWifiManager(context,
                getFragmentController().getSettingsLifecycle());
    }

    @Override
    protected Class<ClickableWhileDisabledSwitchPreference> getPreferenceType() {
        return ClickableWhileDisabledSwitchPreference.class;
    }

    @Override
    protected void updateState(ClickableWhileDisabledSwitchPreference preference) {
        updateSwitchPreference(preference, mCarWifiManager.isWifiEnabled());
    }

    @Override
    protected boolean handlePreferenceChanged(ClickableWhileDisabledSwitchPreference preference,
            Object newValue) {
        boolean wifiEnabled = (Boolean) newValue;
        mCarWifiManager.setWifiEnabled(wifiEnabled);
        return true;
    }

    @Override
    protected void onCreateInternal() {
        getPreference().setContentDescription(
                getContext().getString(R.string.wifi_state_switch_content_description));
        getPreference().setDisabledClickListener(p ->
                Toast.makeText(getContext(),
                        getContext().getString(R.string.power_component_disabled),
                        Toast.LENGTH_LONG).show());
    }

    @Override
    protected void onStartInternal() {
        mCarWifiManager.addListener(this);
        onWifiStateChanged(mCarWifiManager.getWifiState());
    }

    @Override
    protected void onResumeInternal() {
        mPowerPolicyListener.handleCurrentPolicy();
    }

    @Override
    protected void onStopInternal() {
        mCarWifiManager.removeListener(this);
    }

    @Override
    protected void onDestroyInternal() {
        mPowerPolicyListener.release();
    }

    @Override
    public void onWifiEntriesChanged() {
        // intentional no-op
    }

    @Override
    public void onWifiStateChanged(int state) {
        updateSwitchPreference(getPreference(), state == WifiManager.WIFI_STATE_ENABLED
                || state == WifiManager.WIFI_STATE_ENABLING);
    }

    private void updateSwitchPreference(ClickableWhileDisabledSwitchPreference preference,
            boolean enabled) {
        preference.setChecked(enabled);
    }

    private void enableSwitchPreference(ClickableWhileDisabledSwitchPreference preference,
            boolean enabled) {
        preference.setEnabled(enabled);
    }
}
