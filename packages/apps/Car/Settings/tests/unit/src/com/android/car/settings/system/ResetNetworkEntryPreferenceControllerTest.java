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

package com.android.car.settings.system;

import static android.os.UserManager.DISALLOW_NETWORK_RESET;

import static com.android.car.settings.common.PreferenceController.AVAILABLE;
import static com.android.car.settings.common.PreferenceController.DISABLED_FOR_PROFILE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.UserManager;

import androidx.preference.Preference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.dx.mockito.inline.extended.ExtendedMockito;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;

/** Unit test for {@link ResetNetworkEntryPreferenceController}. */
@RunWith(AndroidJUnit4.class)
public class ResetNetworkEntryPreferenceControllerTest {
    private Context mContext = ApplicationProvider.getApplicationContext();
    private Preference mPreference;
    private ResetNetworkEntryPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;
    private MockitoSession mSession;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private UserManager mMockUserManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mSession = ExtendedMockito.mockitoSession()
                .mockStatic(UserManager.class, withSettings().lenient())
                .startMocking();
        when(UserManager.get(mContext)).thenReturn(mMockUserManager);

        mPreference = new Preference(mContext);
        mPreferenceController = new ResetNetworkEntryPreferenceController(mContext,
                "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreference);
    }

    @After
    public void tearDown() {
        if (mSession != null) {
            mSession.finishMocking();
        }
    }

    @Test
    public void getAvailabilityStatus_nonAdminUser_disabledForUser() {
        when(mMockUserManager.isAdminUser()).thenReturn(false);

        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(DISABLED_FOR_PROFILE);
    }

    @Test
    public void getAvailabilityStatus_adminUser_available() {
        when(mMockUserManager.isAdminUser()).thenReturn(true);

        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void getAvailabilityStatus_adminUser_restricted_disabledForUser() {
        when(mMockUserManager.isAdminUser()).thenReturn(true);
        when(mMockUserManager.hasUserRestriction(DISALLOW_NETWORK_RESET)).thenReturn(true);

        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(DISABLED_FOR_PROFILE);
    }
}
