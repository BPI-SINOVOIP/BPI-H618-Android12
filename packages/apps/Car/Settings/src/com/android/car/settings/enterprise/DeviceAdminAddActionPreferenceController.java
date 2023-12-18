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
import android.content.ComponentName;
import android.content.Context;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import com.android.car.settings.R;
import com.android.car.settings.common.FragmentController;

/**
 * Controller for the action (activate / deactivate).
 */
public final class DeviceAdminAddActionPreferenceController
        extends BaseDeviceAdminAddPreferenceController<Preference> {

    /*
     * 3-state status for button:
     *
     *  - null: button disabled
     *  - true: admin active, button deactivates
     *  - false: admin inactive, button activates
     */
    @Nullable
    private Boolean mIsActive;

    public DeviceAdminAddActionPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    @Override
    protected void updateState(Preference preference) {
        setIsActive();

        preference.setEnabled(mIsActive != null);
        preference.setTitle(mIsActive == null || mIsActive
                ? R.string.remove_device_admin
                : R.string.add_device_admin);
    }

    @Override
    protected boolean handlePreferenceClicked(Preference preference) {
        if (mIsActive == null) {
            mLogger.wtf("handlePreferenceClicked() called when admin is a profile / device owner");
        } else {
            ComponentName admin = mDeviceAdminInfo.getComponent();
            if (mIsActive) {
                mLogger.i("Deactivating " + admin.flattenToShortString());
                mDpm.removeActiveAdmin(admin);
            } else {
                mLogger.i("Activating " + admin.flattenToShortString());
                // TODO(b/192372143): support refreshing
                mDpm.setActiveAdmin(admin, /* refreshing= */ false);
            }
        }
        getFragmentController().goBack();
        return true;
    }

    @VisibleForTesting
    void setIsActive() {
        ComponentName admin = mDeviceAdminInfo.getComponent();
        if (isProfileOrDeviceOwner(admin)) {
            // TODO(b/170332519): once work profiles are supported, they could be removed
            mLogger.d("updateState(): " + admin.toShortString() + " is PO or DO");
            mIsActive = null;
        } else {
            mIsActive = mDpm.isAdminActive(admin);
        }

        mLogger.d("updateState(): active = " + mIsActive);
    }
}
