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

import android.app.admin.DeviceAdminInfo;
import android.app.admin.DevicePolicyManager;
import android.car.drivingstate.CarUxRestrictions;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.UserManager;

import androidx.preference.Preference;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.Logger;
import com.android.car.settings.common.PreferenceController;

import java.util.Objects;

/**
 * Base class for controllers in the device admin details screen.
 */
abstract class BaseDeviceAdminAddPreferenceController<P extends Preference>
        extends PreferenceController<P> {

    protected final Logger mLogger = new Logger(getClass());

    protected final DevicePolicyManager mDpm;
    protected final PackageManager mPm;
    protected final UserManager mUm;

    protected DeviceAdminInfo mDeviceAdminInfo;

    protected BaseDeviceAdminAddPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);

        mDpm = context.getSystemService(DevicePolicyManager.class);
        mPm = context.getPackageManager();
        mUm = context.getSystemService(UserManager.class);
    }

    @Override
    protected Class<P> getPreferenceType() {
        return (Class<P>) Preference.class;
    }

    @Override
    protected final int getAvailabilityStatus() {
        return mDeviceAdminInfo == null ? CONDITIONALLY_UNAVAILABLE : getRealAvailabilityStatus();
    }

    /**
     * Method that can be overridden to define the real value of {@link #getAvailabilityStatus()}
     * when the {@link #setDeviceAdmin(DeviceAdminInfo) DeviceAdminInfo} is set.
     */
    protected int getRealAvailabilityStatus() {
        return AVAILABLE;
    }

    final <T extends BaseDeviceAdminAddPreferenceController<P>> T setDeviceAdmin(
            DeviceAdminInfo deviceAdminInfo) {
        mDeviceAdminInfo = Objects.requireNonNull(deviceAdminInfo);
        @SuppressWarnings("unchecked")
        T safeCast = (T) this;
        return safeCast;
    }

    final boolean isProfileOrDeviceOwner() {
        return isProfileOrDeviceOwner(mDeviceAdminInfo.getComponent());
    }

    final boolean isProfileOrDeviceOwner(ComponentName admin) {
        return admin.equals(mDpm.getProfileOwner())
                || admin.equals(mDpm.getDeviceOwnerComponentOnCallingUser());
    }
}
