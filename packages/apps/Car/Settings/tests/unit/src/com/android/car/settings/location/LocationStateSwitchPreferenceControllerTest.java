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

package com.android.car.settings.location;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.car.drivingstate.CarUxRestrictions;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.location.LocationManager;
import android.os.UserHandle;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.SwitchPreference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.ClickableWhileDisabledSwitchPreference;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LocationStateSwitchPreferenceControllerTest {
    private LifecycleOwner mLifecycleOwner;
    private Context mContext = spy(ApplicationProvider.getApplicationContext());
    private SwitchPreference mSwitchPreference;
    private LocationStateSwitchPreferenceController mPreferenceController;
    private LocationManager mLocationManager;
    private CarUxRestrictions mCarUxRestrictions;
    private UserHandle mUserHandle;

    @Mock
    private FragmentController mFragmentController;

    @Before
    public void setUp() {
        mLifecycleOwner = new TestLifecycleOwner();
        MockitoAnnotations.initMocks(this);

        mLocationManager = mContext.getSystemService(LocationManager.class);

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();
        mUserHandle = UserHandle.of(UserHandle.myUserId());

        mSwitchPreference = new ClickableWhileDisabledSwitchPreference(mContext);
        mPreferenceController = new LocationStateSwitchPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mSwitchPreference);
    }

    @Test
    public void onIntentReceived_updateUi() {
        initializePreference(/* checked= */ false, /* enabled= */ true);
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onStart(mLifecycleOwner);
        ArgumentCaptor<BroadcastReceiver> broadcastReceiverArgumentCaptor = ArgumentCaptor.forClass(
                BroadcastReceiver.class);
        verify(mContext).registerReceiver(broadcastReceiverArgumentCaptor.capture(), any());

        mLocationManager.setLocationEnabledForUser(/* enabled= */ true, mUserHandle);
        broadcastReceiverArgumentCaptor.getValue().onReceive(mContext,
                new Intent(LocationManager.MODE_CHANGED_ACTION));
        assertThat(mSwitchPreference.isChecked()).isTrue();
    }

    @Test
    public void onPreferenceClicked_locationDisabled_shouldEnable() {
        initializePreference(/* checked= */ false, /* enabled= */ true);

        mSwitchPreference.performClick();

        assertThat(mSwitchPreference.isChecked()).isTrue();
        assertThat(mLocationManager.isLocationEnabledForUser(mUserHandle)).isTrue();
    }

    @Test
    public void onPreferenceClicked_locationEnabled_shouldDisable() {
        initializePreference(/* checked= */ true, /* enabled= */ true);

        mSwitchPreference.performClick();

        assertThat(mSwitchPreference.isChecked()).isFalse();
        assertThat(mLocationManager.isLocationEnabledForUser(mUserHandle)).isFalse();
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
        mLocationManager.setLocationEnabledForUser(checked, mUserHandle);
        mSwitchPreference.setChecked(checked);
        mSwitchPreference.setEnabled(enabled);
    }
}
