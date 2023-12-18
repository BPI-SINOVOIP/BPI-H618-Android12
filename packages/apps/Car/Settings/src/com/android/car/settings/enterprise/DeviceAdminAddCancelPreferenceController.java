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

package com.android.car.settings.enterprise;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;

import androidx.preference.Preference;

import com.android.car.settings.common.FragmentController;

/**
 * Controller for the cancel button in the device admin details screen.
 */
public final class DeviceAdminAddCancelPreferenceController
        extends BaseDeviceAdminAddPreferenceController<Preference> {

    public DeviceAdminAddCancelPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    @Override
    protected void updateState(Preference preference) {
        preference.setTitle(com.android.internal.R.string.cancel);
    }

    @Override
    protected boolean handlePreferenceClicked(Preference preference) {
        getFragmentController().goBack();
        return true;
    }
}
