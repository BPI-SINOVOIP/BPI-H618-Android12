/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.settings.system;

import static android.os.UserManager.DISALLOW_FACTORY_RESET;

import static com.android.car.settings.enterprise.ActionDisabledByAdminDialogFragment.DISABLED_BY_ADMIN_CONFIRM_DIALOG_TAG;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.UserHandle;
import android.os.UserManager;
import android.widget.Toast;

import com.android.car.settings.R;
import com.android.car.settings.common.ClickableWhileDisabledPreference;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceController;
import com.android.car.settings.enterprise.ActionDisabledByAdminDialogFragment;

/**
 * Controller which determines if factory clear (aka "factory reset") should be displayed based on
 * user status.
 */
public class FactoryResetEntryPreferenceController
        extends PreferenceController<ClickableWhileDisabledPreference> {

    private final UserManager mUserManager;

    public FactoryResetEntryPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
        mUserManager = UserManager.get(context);
    }

    @Override
    protected Class<ClickableWhileDisabledPreference> getPreferenceType() {
        return ClickableWhileDisabledPreference.class;
    }

    @Override
    protected void onCreateInternal() {
        super.onCreateInternal();
        getPreference().setDisabledClickListener(p ->
                Toast.makeText(getContext(), getContext().getString(R.string.action_unavailable),
                        Toast.LENGTH_LONG).show());
    }

    @Override
    protected boolean handlePreferenceClicked(ClickableWhileDisabledPreference preference) {
        if (mUserManager.hasUserRestriction(DISALLOW_FACTORY_RESET)) {
            getFragmentController().showDialog(ActionDisabledByAdminDialogFragment.newInstance(
                    DISALLOW_FACTORY_RESET, UserHandle.USER_SYSTEM),
                    DISABLED_BY_ADMIN_CONFIRM_DIALOG_TAG);
            return true;
        }
        return super.handlePreferenceClicked(preference);
    }

    @Override
    public int getAvailabilityStatus() {
        return shouldDisable() ? AVAILABLE_FOR_VIEWING : AVAILABLE;
    }

    private boolean shouldDisable() {
        if (!mUserManager.isAdminUser() && !isDemoUser()) {
            // Disable for non-admin and non-demo users.
            return true;
        }
        UserHandle userHandle = UserHandle.of(getContext().getUserId());
        return mUserManager.hasBaseUserRestriction(DISALLOW_FACTORY_RESET, userHandle);
    }

    private boolean isDemoUser() {
        return UserManager.isDeviceInDemoMode(getContext()) && mUserManager.isDemoUser();
    }
}
