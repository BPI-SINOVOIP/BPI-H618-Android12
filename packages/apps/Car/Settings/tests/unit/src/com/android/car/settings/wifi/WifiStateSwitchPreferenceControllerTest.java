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

package com.android.car.settings.wifi;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.net.wifi.WifiManager;

import androidx.lifecycle.Lifecycle;
import androidx.preference.SwitchPreference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.ClickableWhileDisabledSwitchPreference;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class WifiStateSwitchPreferenceControllerTest {
    private Context mContext = ApplicationProvider.getApplicationContext();
    private SwitchPreference mSwitchPreference;
    private WifiStateSwitchPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;
    private CarWifiManager mCarWifiManager;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private Lifecycle mMockLifecycle;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mSwitchPreference = new ClickableWhileDisabledSwitchPreference(mContext);
        when(mFragmentController.getSettingsLifecycle()).thenReturn(mMockLifecycle);
        mPreferenceController = new WifiStateSwitchPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        mCarWifiManager = new CarWifiManager(mContext, mMockLifecycle);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mSwitchPreference);
    }

    @Test
    public void onWifiStateChanged_disabled_setsSwitchUnchecked() {
        initializePreference(/* checked= */ true, /* enabled= */ true);
        mPreferenceController.onWifiStateChanged(WifiManager.WIFI_STATE_DISABLED);

        assertThat(mSwitchPreference.isChecked()).isFalse();
    }

    @Test
    public void onWifiStateChanged_enabled_setsSwitchChecked() {
        initializePreference(/* checked= */ false, /* enabled= */ true);
        mPreferenceController.onWifiStateChanged(WifiManager.WIFI_STATE_ENABLED);

        assertThat(mSwitchPreference.isChecked()).isTrue();
    }

    @Test
    public void onWifiStateChanged_enabling_setsSwitchChecked() {
        initializePreference(/* checked= */ false, /* enabled= */ true);
        mPreferenceController.onWifiStateChanged(WifiManager.WIFI_STATE_ENABLING);

        assertThat(mSwitchPreference.isChecked()).isTrue();
    }

    @Test
    public void onPolicyChanged_enabled_setsSwitchEnabled() {
        initializePreference(/* checked= */ false, /* enabled= */ false);

        mPreferenceController.mPowerPolicyListener.getPolicyChangeHandler()
                .handlePolicyChange(/* isOn= */ true);

        assertThat(mSwitchPreference.isEnabled()).isTrue();
    }

    @Test
    public void onPolicyChanged_disabled_setsSwitchDisabled() {
        initializePreference(/* checked= */ false, /* enabled= */ true);

        mPreferenceController.mPowerPolicyListener.getPolicyChangeHandler()
                .handlePolicyChange(/* isOn= */ false);

        assertThat(mSwitchPreference.isEnabled()).isFalse();
    }

    private void initializePreference(boolean checked, boolean enabled) {
        mCarWifiManager.setWifiEnabled(checked);
        mSwitchPreference.setChecked(checked);
        mSwitchPreference.setEnabled(enabled);
    }
}
