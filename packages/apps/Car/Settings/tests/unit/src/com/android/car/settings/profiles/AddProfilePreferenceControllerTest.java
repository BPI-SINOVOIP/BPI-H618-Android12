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

package com.android.car.settings.profiles;

import static com.android.car.settings.profiles.AddProfilePreferenceController.MAX_PROFILES_LIMIT_REACHED_DIALOG_TAG;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.UserManager;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.Preference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.ConfirmationDialogFragment;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.ResourceTestUtils;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class AddProfilePreferenceControllerTest {

    private Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private Preference mPreference;
    private AddProfilePreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private UserManager mUserManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mPreference = new Preference(mContext);
        mPreferenceController = new AddProfilePreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        mPreferenceController.setUserManager(mUserManager);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreference);
    }

    @Test
    public void onCreate_userInDemoMode_showsExitRetailModeButton() {
        when(mUserManager.isDemoUser()).thenReturn(true);

        mPreferenceController.onCreate(mLifecycleOwner);

        assertThat(mPreference.isVisible()).isTrue();
        assertThat(mPreference.getTitle()).isEqualTo(
                ResourceTestUtils.getString(mContext, "exit_retail_button_text"));
    }

    @Test
    public void onCreate_userCanAddNewProfile_showsAddProfileButton() {
        when(mUserManager.isDemoUser()).thenReturn(false);
        when(mUserManager.hasUserRestriction(UserManager.DISALLOW_ADD_USER)).thenReturn(false);

        mPreferenceController.onCreate(mLifecycleOwner);

        assertThat(mPreference.isVisible()).isTrue();
        assertThat(mPreference.getTitle()).isEqualTo(
                ResourceTestUtils.getString(mContext, "add_profile_text"));
    }

    @Test
    public void onCreate_userRestrictedFromAddingNewProfileAndNotInDemo_doesNotShowActionButton() {
        when(mUserManager.isDemoUser()).thenReturn(false);
        when(mUserManager.hasUserRestriction(UserManager.DISALLOW_ADD_USER)).thenReturn(true);

        mPreferenceController.onCreate(mLifecycleOwner);

        assertThat(mPreference.isVisible()).isFalse();
    }

    /* Test that if the max num of profiles is reached, clicking on the button informs user of
    that. */
    @Test
    public void testCallOnClick_profileLimitReached_showErrorDialog() {
        when(mUserManager.isDemoUser()).thenReturn(false);
        when(mUserManager.hasUserRestriction(UserManager.DISALLOW_ADD_USER)).thenReturn(false);
        when(mUserManager.canAddMoreUsers()).thenReturn(false);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreference.performClick();

        verify(mFragmentController).showDialog(any(ConfirmationDialogFragment.class),
                eq(MAX_PROFILES_LIMIT_REACHED_DIALOG_TAG));
    }
}
