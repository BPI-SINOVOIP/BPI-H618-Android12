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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.os.UserHandle;
import android.provider.Settings;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceController;
import com.android.car.ui.preference.CarUiTwoActionSwitchPreference;
import com.android.internal.annotations.VisibleForTesting;
import com.android.settingslib.Utils;

/** Handles a {@link CarUiTwoActionSwitchPreference} which shows the current status of Location. */
public class LocationTogglePreferenceController
        extends PreferenceController<CarUiTwoActionSwitchPreference> {

    @VisibleForTesting
    static final IntentFilter INTENT_FILTER_LOCATION_MODE_CHANGED =
            new IntentFilter(LocationManager.MODE_CHANGED_ACTION);

    private final Context mContext;
    private final LocationManager mLocationManager;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            refreshUi();
        }
    };

    public LocationTogglePreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mContext = context;
        mLocationManager = context.getSystemService(LocationManager.class);
    }

    @Override
    protected Class<CarUiTwoActionSwitchPreference> getPreferenceType() {
        return CarUiTwoActionSwitchPreference.class;
    }

    @Override
    protected void onCreateInternal() {
        getPreference().setOnSecondaryActionClickListener(locationEnabled -> {
            Utils.updateLocationEnabled(
                    mContext,
                    locationEnabled,
                    UserHandle.myUserId(),
                    Settings.Secure.LOCATION_CHANGER_SYSTEM_SETTINGS);
        });
    }

    @Override
    protected void onStartInternal() {
        mContext.registerReceiver(mReceiver, INTENT_FILTER_LOCATION_MODE_CHANGED);
    }

    @Override
    protected void onStopInternal() {
        mContext.unregisterReceiver(mReceiver);
    }

    @Override
    protected void updateState(CarUiTwoActionSwitchPreference preference) {
        preference.setSecondaryActionChecked(mLocationManager.isLocationEnabled());
    }
}
