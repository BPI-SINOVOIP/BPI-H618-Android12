/*
 * Copyright (C) 2019 The Android Open Source Project
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

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.net.TetheringManager;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.Looper;

import com.android.car.settings.R;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceController;
import com.android.car.ui.preference.CarUiTwoActionSwitchPreference;

/**
 * Controls the availability of wifi tethering preference based on whether tethering is supported
 */
public class WifiTetherPreferenceController extends
        PreferenceController<CarUiTwoActionSwitchPreference>
        implements WifiTetheringHandler.WifiTetheringAvailabilityListener {

    private final TetheringManager mTetheringManager =
            getContext().getSystemService(TetheringManager.class);
    private final Handler mHandler;
    private WifiTetheringHandler mWifiTetheringHandler;
    private volatile boolean mIsTetheringSupported;
    private volatile boolean mReceivedTetheringEventCallback = false;

    private TetheringManager.TetheringEventCallback mTetheringCallback =
            new TetheringManager.TetheringEventCallback() {
                @Override
                public void onTetheringSupported(boolean supported) {
                    mReceivedTetheringEventCallback = true;
                    if (mIsTetheringSupported != supported) {
                        mIsTetheringSupported = supported;
                    }
                    refreshUi();
                }
            };

    public WifiTetherPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mHandler = new Handler(Looper.getMainLooper());
        mWifiTetheringHandler = new WifiTetheringHandler(context,
                fragmentController.getSettingsLifecycle(), this);
    }

    @Override
    protected Class<CarUiTwoActionSwitchPreference> getPreferenceType() {
        return CarUiTwoActionSwitchPreference.class;
    }

    @Override
    protected void onCreateInternal() {
        getPreference().setOnSecondaryActionClickListener(isChecked -> {
            mWifiTetheringHandler.updateWifiTetheringState(isChecked);
        });
    }

    @Override
    protected void onStartInternal() {
        mTetheringManager.registerTetheringEventCallback(
                new HandlerExecutor(mHandler), mTetheringCallback);
        boolean tetheringEnabled = mWifiTetheringHandler.isWifiTetheringEnabled();
        updateSwitchPreference(tetheringEnabled);
        mWifiTetheringHandler.onStartInternal();
    }

    @Override
    protected void onStopInternal() {
        mTetheringManager.unregisterTetheringEventCallback(mTetheringCallback);
        mWifiTetheringHandler.onStopInternal();
    }

    @Override
    protected int getAvailabilityStatus() {
        if (!mReceivedTetheringEventCallback) {
            return AVAILABLE_FOR_VIEWING;
        }
        return  mIsTetheringSupported ? AVAILABLE : UNSUPPORTED_ON_DEVICE;
    }

    @Override
    public void onWifiTetheringAvailable() {
        updateSwitchPreference(true);
    }

    @Override
    public void onWifiTetheringUnavailable() {
        updateSwitchPreference(false);
    }

    @Override
    public void enablePreference() {
        getPreference().setSecondaryActionEnabled(true);
    }

    @Override
    public void disablePreference() {
        getPreference().setSecondaryActionEnabled(false);
    }

    private void updateSwitchPreference(boolean switchOn) {
        getPreference().setSummary(switchOn ? R.string.car_ui_preference_switch_on
                : R.string.car_ui_preference_switch_off);
        getPreference().setSecondaryActionChecked(switchOn);
    }
}
