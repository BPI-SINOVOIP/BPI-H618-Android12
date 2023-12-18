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

package com.android.car.settings.display;

import static com.google.common.truth.Truth.assertThat;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.provider.Settings;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.SwitchPreference;
import androidx.preference.TwoStatePreference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class AdaptiveBrightnessTogglePreferenceControllerTest {

    private Context mContext;
    private AdaptiveBrightnessTogglePreferenceController mPreferenceController;
    private TwoStatePreference mTwoStatePreference;

    @Mock
    private FragmentController mFragmentController;

    @Before
    public void setUp() {
        LifecycleOwner lifecycleOwner = new TestLifecycleOwner();
        MockitoAnnotations.initMocks(this);

        mContext = ApplicationProvider.getApplicationContext();
        mTwoStatePreference = new SwitchPreference(mContext);

        CarUxRestrictions carUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();
        mPreferenceController = new AdaptiveBrightnessTogglePreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, carUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mTwoStatePreference);

        mPreferenceController.onCreate(lifecycleOwner);
    }

    @Test
    public void testRefreshUi_manualMode_isNotChecked() {
        Settings.System.putInt(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS_MODE,
                Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);

        mPreferenceController.refreshUi();
        assertThat(mTwoStatePreference.isChecked()).isFalse();
    }

    @Test
    public void testRefreshUi_automaticMode_isChecked() {
        Settings.System.putInt(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS_MODE,
                Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC);

        mPreferenceController.refreshUi();
        assertThat(mTwoStatePreference.isChecked()).isTrue();
    }

    @Test
    public void testHandlePreferenceChanged_setFalse() {
        mTwoStatePreference.callChangeListener(false);
        int brightnessMode = Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS_MODE,
                Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC);
        assertThat(brightnessMode).isEqualTo(Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
    }

    @Test
    public void testHandlePreferenceChanged_setTrue() {
        mTwoStatePreference.callChangeListener(true);
        int brightnessMode = Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS_MODE,
                Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
        assertThat(brightnessMode).isEqualTo(Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC);
    }
}
