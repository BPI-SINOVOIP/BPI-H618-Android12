/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.car.settings.privacy;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.hardware.SensorPrivacyManager;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceController;
import com.android.car.ui.preference.CarUiTwoActionSwitchPreference;
import com.android.internal.annotations.VisibleForTesting;

/** Business logic for controlling the mute mic setting. */
public class MuteMicTogglePreferenceController
        extends PreferenceController<CarUiTwoActionSwitchPreference> {

    private SensorPrivacyManager mSensorPrivacyManager;

    private SensorPrivacyManager.OnSensorPrivacyChangedListener mListener =
            new SensorPrivacyManager.OnSensorPrivacyChangedListener() {
                @Override
                public void onSensorPrivacyChanged(int sensor, boolean enabled) {
                    refreshUi();
                }
            };

    public MuteMicTogglePreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        this(context, preferenceKey, fragmentController, uxRestrictions,
                SensorPrivacyManager.getInstance(context));
    }

    @VisibleForTesting
    MuteMicTogglePreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions,
            SensorPrivacyManager sensorPrivacyManager) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mSensorPrivacyManager = sensorPrivacyManager;
    }

    @Override
    protected Class<CarUiTwoActionSwitchPreference> getPreferenceType() {
        return CarUiTwoActionSwitchPreference.class;
    }

    @Override
    protected void onCreateInternal() {
        getPreference().setOnSecondaryActionClickListener(isChecked -> {
            // Settings UX currently shows "checked means mic is enabled", but the underlying API is
            // inversely written around "is mic muted?" So we must be careful when doing
            // comparisons.
            boolean isMicMuted = mSensorPrivacyManager.isSensorPrivacyEnabled(
                    SensorPrivacyManager.Sensors.MICROPHONE);
            if (isChecked == isMicMuted) {
                // UX and underlying API state for mic do not match, so update sensor privacy
                mSensorPrivacyManager.setSensorPrivacyForProfileGroup(
                        SensorPrivacyManager.Sources.SETTINGS,
                        SensorPrivacyManager.Sensors.MICROPHONE,
                        !isChecked);
            }
        });
    }

    @Override
    protected void onStartInternal() {
        mSensorPrivacyManager.addSensorPrivacyListener(
                SensorPrivacyManager.Sensors.MICROPHONE, mListener);
    }

    @Override
    protected void onStopInternal() {
        mSensorPrivacyManager.removeSensorPrivacyListener(SensorPrivacyManager.Sensors.MICROPHONE,
                mListener);
    }

    @Override
    protected int getAvailabilityStatus() {
        boolean hasFeatureMicToggle = mSensorPrivacyManager.supportsSensorToggle(
                SensorPrivacyManager.Sensors.MICROPHONE);
        return hasFeatureMicToggle ? AVAILABLE : UNSUPPORTED_ON_DEVICE;
    }

    @Override
    protected void updateState(CarUiTwoActionSwitchPreference preference) {
        preference.setSecondaryActionChecked(!mSensorPrivacyManager.isSensorPrivacyEnabled(
                SensorPrivacyManager.Sensors.MICROPHONE));
    }
}
