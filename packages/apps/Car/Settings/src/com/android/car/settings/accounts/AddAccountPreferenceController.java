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

package com.android.car.settings.accounts;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceController;
import com.android.car.settings.profiles.ProfileHelper;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * Business Logic for preference starts the add account flow.
 */
public class AddAccountPreferenceController extends PreferenceController<Preference> {

    private String[] mAuthorities;
    private String[] mAccountTypes;

    public AddAccountPreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController,
            CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    /** Sets the account authorities that are available. */
    public AddAccountPreferenceController setAuthorities(String[] authorities) {
        mAuthorities = authorities;
        return this;
    }

    /** Sets the account authorities that are available. */
    public AddAccountPreferenceController setAccountTypes(String[] accountTypes) {
        mAccountTypes = accountTypes;
        return this;
    }

    @Override
    protected Class<Preference> getPreferenceType() {
        return Preference.class;
    }

    @Override
    protected boolean handlePreferenceClicked(Preference preference) {
        AccountTypesHelper helper = getAccountTypesHelper();

        if (mAuthorities != null) {
            helper.setAuthorities(Arrays.asList(mAuthorities));
        }
        if (mAccountTypes != null) {
            helper.setAccountTypesFilter(
                    new HashSet<>(Arrays.asList(mAccountTypes)));
        }

        Set<String> authorizedAccountTypes = helper.getAuthorizedAccountTypes();

        if (authorizedAccountTypes.size() == 1) {
            String accountType = authorizedAccountTypes.iterator().next();
            getContext().startActivity(
                    AddAccountActivity.createAddAccountActivityIntent(getContext(), accountType));
        } else {
            getFragmentController().launchFragment(new ChooseAccountFragment());
        }
        return true;
    }

    @Override
    protected int getAvailabilityStatus() {
        if (getProfileHelper().canCurrentProcessModifyAccounts()) {
            return AVAILABLE;
        }
        return DISABLED_FOR_PROFILE;
    }

    @VisibleForTesting
    ProfileHelper getProfileHelper() {
        return ProfileHelper.getInstance(getContext());
    }

    @VisibleForTesting
    AccountTypesHelper getAccountTypesHelper() {
        return new AccountTypesHelper(getContext());
    }
}
