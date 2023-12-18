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

package com.android.car.settings.privacy;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.car.drivingstate.CarUxRestrictions;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.location.LocationManager;
import android.os.UserHandle;

import androidx.lifecycle.LifecycleOwner;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.car.ui.preference.CarUiTwoActionSwitchPreference;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LocationTogglePreferenceControllerTest {
    private LifecycleOwner mLifecycleOwner;
    private Context mContext = spy(ApplicationProvider.getApplicationContext());
    private CarUiTwoActionSwitchPreference mSwitchPreference;
    private LocationTogglePreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;
    private UserHandle mUserHandle;
    private LocationManager mLocationManager;

    @Mock
    private FragmentController mFragmentController;

    @Captor
    private ArgumentCaptor<BroadcastReceiver> mListener;

    @Before
    public void setUp() {
        mLifecycleOwner = new TestLifecycleOwner();
        MockitoAnnotations.initMocks(this);

        mLocationManager = mContext.getSystemService(LocationManager.class);

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();
        mUserHandle = UserHandle.of(UserHandle.myUserId());

        mSwitchPreference = new CarUiTwoActionSwitchPreference(mContext);
        mPreferenceController = new LocationTogglePreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mSwitchPreference);
    }

    @Test
    public void onPreferenceClicked_clickLocationEnabled_shouldDisableLocation() {
        initializePreference(/* isLocationEnabled= */ true);

        mSwitchPreference.performSecondaryActionClick();

        assertThat(mLocationManager.isLocationEnabledForUser(mUserHandle)).isFalse();
        assertThat(mSwitchPreference.isSecondaryActionChecked()).isFalse();
    }

    @Test
    public void onPreferenceClicked_clickMicDisabled_shouldEnableLocation() {
        initializePreference(/* isLocationEnabled= */ false);

        mSwitchPreference.performSecondaryActionClick();

        assertThat(mLocationManager.isLocationEnabledForUser(mUserHandle)).isTrue();
        assertThat(mSwitchPreference.isSecondaryActionChecked()).isTrue();
    }

    @Test
    public void onListenerUpdate_locationDisabled_shouldUpdateChecked() {
        initializePreference(/* isLocationEnabled= */ false);

        mLocationManager.setLocationEnabledForUser(true, mUserHandle);
        mListener.getValue().onReceive(mContext, new Intent(LocationManager.MODE_CHANGED_ACTION));

        assertThat(mSwitchPreference.isSecondaryActionChecked()).isTrue();
    }

    @Test
    public void onListenerUpdate_locationEnabled_shouldUpdateChecked() {
        initializePreference(/* isLocationEnabled= */ true);

        mLocationManager.setLocationEnabledForUser(false, mUserHandle);
        mListener.getValue().onReceive(mContext, new Intent(LocationManager.MODE_CHANGED_ACTION));

        assertThat(mSwitchPreference.isSecondaryActionChecked()).isFalse();
    }

    private void initializePreference(boolean isLocationEnabled) {
        mLocationManager.setLocationEnabledForUser(isLocationEnabled, mUserHandle);
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onStart(mLifecycleOwner);
        verify(mContext).registerReceiver(mListener.capture(),
                eq(LocationTogglePreferenceController.INTENT_FILTER_LOCATION_MODE_CHANGED));
    }}
