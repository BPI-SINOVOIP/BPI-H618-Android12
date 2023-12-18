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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.accounts.Account;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.UserHandle;

import androidx.lifecycle.LifecycleOwner;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.ConfirmationDialogFragment;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.profiles.ProfileHelper;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.car.ui.preference.CarUiTwoActionIconPreference;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class AccountDetailsPreferenceControllerTest {
    private static final String ACCOUNT_NAME = "Name";
    private static final String ACCOUNT_TYPE = "com.acct";
    private final Account mAccount = new Account(ACCOUNT_NAME, ACCOUNT_TYPE);
    private final UserHandle mUserHandle = new UserHandle(0);

    private Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private CarUiTwoActionIconPreference mPreference;
    private AccountDetailsPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private ProfileHelper mMockProfileHelper;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mPreference = new CarUiTwoActionIconPreference(mContext);
        mPreferenceController = new TestAccountDetailsPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        mPreferenceController.setAccount(mAccount);
        mPreferenceController.setUserHandle(mUserHandle);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreference);
    }

    @Test
    public void cannotModifyUsers_removeAccountButtonShouldNotBeVisible() {
        when(mMockProfileHelper.canCurrentProcessModifyAccounts()).thenReturn(false);

        mPreferenceController.onCreate(mLifecycleOwner);

        assertThat(mPreference.isSecondaryActionVisible()).isFalse();
    }

    @Test
    public void canModifyUsers_removeAccountButtonShouldBeVisible() {
        when(mMockProfileHelper.canCurrentProcessModifyAccounts()).thenReturn(true);

        mPreferenceController.onCreate(mLifecycleOwner);

        assertThat(mPreference.isSecondaryActionVisible()).isTrue();
    }

    @Test
    public void onRemoveAccountButtonClicked_canModifyUsers_shouldShowConfirmRemoveAccountDialog() {
        when(mMockProfileHelper.canCurrentProcessModifyAccounts()).thenReturn(true);

        mPreferenceController.onCreate(mLifecycleOwner);

        mPreference.performSecondaryActionClick();

        verify(mFragmentController).showDialog(any(ConfirmationDialogFragment.class),
                eq(ConfirmationDialogFragment.TAG));
    }

    private class TestAccountDetailsPreferenceController
            extends AccountDetailsPreferenceController {

        TestAccountDetailsPreferenceController(Context context, String preferenceKey,
                FragmentController fragmentController,
                CarUxRestrictions uxRestrictions) {
            super(context, preferenceKey, fragmentController, uxRestrictions);
        }

        @Override
        ProfileHelper getProfileHelper() {
            return mMockProfileHelper;
        }
    }
}
