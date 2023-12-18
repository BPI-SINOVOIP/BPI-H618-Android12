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

import static com.android.car.settings.common.PreferenceController.AVAILABLE;
import static com.android.car.settings.common.PreferenceController.CONDITIONALLY_UNAVAILABLE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.Intent;
import android.os.UserManager;
import android.provider.Settings;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.Preference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.development.DevelopmentSettingsUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.dx.mockito.inline.extended.ExtendedMockito;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;

@RunWith(AndroidJUnit4.class)
public class DeveloperOptionsEntryPreferenceControllerTest {
    private Context mContext = spy(ApplicationProvider.getApplicationContext());
    private LifecycleOwner mLifecycleOwner;
    private Preference mPreference;
    private DeveloperOptionsEntryPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;
    private MockitoSession mSession;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private UserManager mMockUserManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mSession = ExtendedMockito.mockitoSession()
                .mockStatic(UserManager.class, withSettings().lenient())
                .startMocking();
        when(UserManager.get(mContext)).thenReturn(mMockUserManager);
        when(mMockUserManager.isAdminUser()).thenReturn(true);
        when(mMockUserManager.hasUserRestriction(UserManager.DISALLOW_DEBUGGING_FEATURES))
                .thenReturn(false);
        doNothing().when(mContext).startActivity(any());

        mPreference = new Preference(mContext);
        mPreference.setIntent(new Intent(Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS));
        mPreferenceController = new DeveloperOptionsEntryPreferenceController(mContext,
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
    public void testGetAvailabilityStatus_devOptionsEnabled_isAvailable() {
        setDeveloperOptionsEnabled(true);
        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void testGetAvailabilityStatus_devOptionsDisabled_isUnavailable() {
        setDeveloperOptionsEnabled(false);
        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(
                CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testGetAvailabilityStatus_devOptionsEnabled_hasUserRestriction_isUnavailable() {
        setDeveloperOptionsEnabled(true);
        when(mMockUserManager.hasUserRestriction(UserManager.DISALLOW_DEBUGGING_FEATURES))
                .thenReturn(true);
        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(
                CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void performClick_startsActivity() {
        setDeveloperOptionsEnabled(true);
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreference.performClick();

        ArgumentCaptor<Intent> captor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext).startActivity(captor.capture());

        Intent intent = captor.getValue();
        assertThat(intent.getAction()).isEqualTo(Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS);
    }

    private void setDeveloperOptionsEnabled(boolean enabled) {
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, enabled ? 1 : 0);
        DevelopmentSettingsUtil.setDevelopmentSettingsEnabled(mContext, enabled);
    }
}
