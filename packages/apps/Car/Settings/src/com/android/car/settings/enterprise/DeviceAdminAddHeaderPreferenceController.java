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
import android.content.res.Resources;
import android.graphics.drawable.Drawable;

import androidx.preference.Preference;

import com.android.car.settings.common.FragmentController;

/**
 * Controller for the header preference the device admin details screen.
 */
public final class DeviceAdminAddHeaderPreferenceController
        extends BaseDeviceAdminAddPreferenceController<Preference> {

    public DeviceAdminAddHeaderPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    @Override
    protected void updateState(Preference preference) {
        CharSequence name = mDeviceAdminInfo.loadLabel(mPm);
        Drawable icon = mDeviceAdminInfo.loadIcon(mPm);
        CharSequence description = null;
        try {
            description = mDeviceAdminInfo.loadDescription(mPm);
        } catch (Resources.NotFoundException e) {
            mLogger.v("No description for "
                    + mDeviceAdminInfo.getComponent().flattenToShortString());
        }

        mLogger.d("updateState: name=" + name  + ", description=" + description);
        preference.setTitle(name);
        preference.setIcon(icon);
        if (description != null) {
            preference.setSummary(description);
        }
    }
}
