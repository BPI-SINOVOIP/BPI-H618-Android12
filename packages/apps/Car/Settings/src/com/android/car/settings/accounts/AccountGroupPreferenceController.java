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

package com.android.car.settings.accounts;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;

import androidx.annotation.VisibleForTesting;
import androidx.preference.PreferenceGroup;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.profiles.ProfileDetailsBasePreferenceController;
import com.android.car.settings.profiles.ProfileHelper;

/**
 * Controller for displaying accounts and associated actions.
 * Ensures changes can only be made by the appropriate users.
 */
public class AccountGroupPreferenceController extends
        ProfileDetailsBasePreferenceController<PreferenceGroup> {

    public AccountGroupPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController, CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    @Override
    protected Class<PreferenceGroup> getPreferenceType() {
        return PreferenceGroup.class;
    }

    @Override
    protected int getAvailabilityStatus() {
        boolean isCurrentUser = getProfileHelper().isCurrentProcessUser(getUserInfo());
        boolean canModifyAccounts = getProfileHelper().canCurrentProcessModifyAccounts();
        return (isCurrentUser && canModifyAccounts) ? AVAILABLE : DISABLED_FOR_PROFILE;
    }

    @VisibleForTesting
    ProfileHelper getProfileHelper() {
        return ProfileHelper.getInstance(getContext());
    }
}
