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

import static com.android.settingslib.display.BrightnessUtils.GAMMA_SPACE_MAX;
import static com.android.settingslib.display.BrightnessUtils.convertLinearToGamma;

import static com.google.common.truth.Truth.assertThat;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.os.UserHandle;
import android.provider.Settings;

import androidx.lifecycle.LifecycleOwner;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.common.SeekBarPreference;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class BrightnessLevelPreferenceControllerTest {
    private Context mContext;
    private BrightnessLevelPreferenceController mController;
    private SeekBarPreference mSeekBarPreference;
    private int mMin;
    private int mMax;
    private int mMid;

    @Mock
    private FragmentController mFragmentController;

    @Before
    public void setUp() {
        LifecycleOwner lifecycleOwner = new TestLifecycleOwner();
        MockitoAnnotations.initMocks(this);

        mContext = ApplicationProvider.getApplicationContext();
        mMin = mContext.getResources().getInteger(
                com.android.internal.R.integer.config_screenBrightnessSettingMinimum);
        mMax = mContext.getResources().getInteger(
                com.android.internal.R.integer.config_screenBrightnessSettingMaximum);
        mMid = (mMax + mMin) / 2;

        mSeekBarPreference = new SeekBarPreference(mContext);
        CarUxRestrictions carUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();
        mController = new BrightnessLevelPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, carUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mController, mSeekBarPreference);

        mController.onCreate(lifecycleOwner);
    }

    @Test
    public void testRefreshUi_maxSet() {
        mController.refreshUi();
        assertThat(mSeekBarPreference.getMax()).isEqualTo(GAMMA_SPACE_MAX);
    }

    @Test
    public void testRefreshUi_minValue() {
        Settings.System.putIntForUser(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, mMin, UserHandle.myUserId());

        mController.refreshUi();
        assertThat(mSeekBarPreference.getValue()).isEqualTo(0);
    }

    @Test
    public void testRefreshUi_maxValue() {
        Settings.System.putIntForUser(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, mMax, UserHandle.myUserId());

        mController.refreshUi();
        assertThat(mSeekBarPreference.getValue()).isEqualTo(GAMMA_SPACE_MAX);
    }

    @Test
    public void testRefreshUi_midValue() {
        Settings.System.putIntForUser(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, mMid, UserHandle.myUserId());

        mController.refreshUi();
        assertThat(mSeekBarPreference.getValue()).isEqualTo(
                convertLinearToGamma(mMid,
                        mMin, mMax));
    }

    @Test
    public void testHandlePreferenceChanged_minValue() throws Settings.SettingNotFoundException {
        mSeekBarPreference.callChangeListener(0);
        int currentSettingsVal = Settings.System.getIntForUser(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, UserHandle.myUserId());
        assertThat(currentSettingsVal).isEqualTo(mMin);
    }

    @Test
    public void testHandlePreferenceChanged_maxValue() throws Settings.SettingNotFoundException {
        mSeekBarPreference.callChangeListener(GAMMA_SPACE_MAX);
        int currentSettingsVal = Settings.System.getIntForUser(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, UserHandle.myUserId());
        assertThat(currentSettingsVal).isEqualTo(mMax);
    }

    @Test
    public void testHandlePreferenceChanged_midValue() throws Settings.SettingNotFoundException {
        mSeekBarPreference.callChangeListener(convertLinearToGamma(mMid, mMin, mMax));
        int currentSettingsVal = Settings.System.getIntForUser(mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, UserHandle.myUserId());
        assertThat(currentSettingsVal).isEqualTo(mMid);
    }
}
