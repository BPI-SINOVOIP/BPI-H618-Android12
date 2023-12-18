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

package com.android.car.settings.applications;

import static android.provider.DeviceConfig.NAMESPACE_APP_HIBERNATION;

import static com.android.car.settings.applications.ApplicationsUtils.PROPERTY_APP_HIBERNATION_ENABLED;
import static com.android.car.settings.common.PreferenceController.AVAILABLE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.apphibernation.AppHibernationManager;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.provider.DeviceConfig;

import androidx.preference.Preference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class HibernatedAppsPreferenceControllerTest {

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private AppHibernationManager mAppHibernationManager;
    @Mock
    private Preference mPreference;
    private static final String KEY = "key";
    private Context mContext;
    private HibernatedAppsPreferenceController mController;
    private CarUxRestrictions mCarUxRestrictions;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();
        DeviceConfig.setProperty(NAMESPACE_APP_HIBERNATION, PROPERTY_APP_HIBERNATION_ENABLED,
                "true", false);
        mContext = spy(ApplicationProvider.getApplicationContext());
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(AppHibernationManager.class))
                .thenReturn(mAppHibernationManager);
        mController = new HibernatedAppsPreferenceController(mContext, KEY, mFragmentController,
                mCarUxRestrictions);
    }

    @Test
    public void getAvailabilityStatus_featureDisabled_shouldNotReturnAvailable() {
        DeviceConfig.setProperty(NAMESPACE_APP_HIBERNATION, PROPERTY_APP_HIBERNATION_ENABLED,
                /* value= */ "false", /* makeDefault= */ true);

        assertThat((mController).getAvailabilityStatus()).isNotEqualTo(AVAILABLE);
    }

    @Test
    public void getAvailabilityStatus_featureEnabled_shouldReturnAvailable() {
        DeviceConfig.setProperty(NAMESPACE_APP_HIBERNATION, PROPERTY_APP_HIBERNATION_ENABLED,
                /* value= */ "true", /* makeDefault= */ true);

        assertThat((mController).getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void onHibernatedAppsCountCallback_setsSummary() {
        assignPreference();
        when(mContext.getResources()).thenReturn(mock(Resources.class));
        int totalHibernated = 2;

        mController.onHibernatedAppsCountLoaded(totalHibernated);

        verify(mContext.getResources()).getQuantityString(
                anyInt(), eq(totalHibernated), eq(totalHibernated));
    }

    private void assignPreference() {
        PreferenceControllerTestUtil.assignPreference(mController,
                mPreference);
    }
}
