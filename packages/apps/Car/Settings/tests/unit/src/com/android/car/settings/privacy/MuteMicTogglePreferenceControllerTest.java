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

import static com.android.car.settings.common.PreferenceController.AVAILABLE;
import static com.android.car.settings.common.PreferenceController.UNSUPPORTED_ON_DEVICE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.hardware.SensorPrivacyManager;

import androidx.lifecycle.LifecycleOwner;
import androidx.test.annotation.UiThreadTest;
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
public class MuteMicTogglePreferenceControllerTest {
    private LifecycleOwner mLifecycleOwner;
    private Context mContext = ApplicationProvider.getApplicationContext();
    private CarUiTwoActionSwitchPreference mSwitchPreference;
    private MuteMicTogglePreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private SensorPrivacyManager mMockSensorPrivacyManager;
    @Captor
    private ArgumentCaptor<SensorPrivacyManager.OnSensorPrivacyChangedListener> mListener;

    @Before
    @UiThreadTest
    public void setUp() {
        mLifecycleOwner = new TestLifecycleOwner();
        MockitoAnnotations.initMocks(this);
        setMicMuteFeatureAvailable(true);

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mSwitchPreference = new CarUiTwoActionSwitchPreference(mContext);
        mPreferenceController = new MuteMicTogglePreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions,
                mMockSensorPrivacyManager);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mSwitchPreference);
    }

    @Test
    public void micMuteUnavailable_preferenceIsHidden() {
        setMicMuteFeatureAvailable(false);
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onStart(mLifecycleOwner);

        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(UNSUPPORTED_ON_DEVICE);
    }

    @Test
    public void micMuteAvailable_preferenceIsShown() {
        setMicMuteFeatureAvailable(true);
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onStart(mLifecycleOwner);

        assertThat(mPreferenceController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void onPreferenceClicked_clickMicEnabled_shouldSetPrivacySensor() {
        initializePreference(/* isMicEnabled= */ true);

        mSwitchPreference.performSecondaryActionClick();

        verify(mMockSensorPrivacyManager).setSensorPrivacyForProfileGroup(
                eq(SensorPrivacyManager.Sources.SETTINGS),
                eq(SensorPrivacyManager.Sensors.MICROPHONE),
                eq(true));
        setIsSensorPrivacyEnabled(true);

        mListener.getValue().onSensorPrivacyChanged(SensorPrivacyManager.Sensors.MICROPHONE, true);

        assertThat(mSwitchPreference.isSecondaryActionChecked()).isFalse();
    }

    @Test
    public void onPreferenceClicked_clickMicDisabled_shouldClearPrivacySensor() {
        initializePreference(/* isMicEnabled= */ false);

        mSwitchPreference.performSecondaryActionClick();

        verify(mMockSensorPrivacyManager).setSensorPrivacyForProfileGroup(
                eq(SensorPrivacyManager.Sources.SETTINGS),
                eq(SensorPrivacyManager.Sensors.MICROPHONE),
                eq(false));
        setIsSensorPrivacyEnabled(false);
        mListener.getValue().onSensorPrivacyChanged(SensorPrivacyManager.Sensors.MICROPHONE, false);

        assertThat(mSwitchPreference.isSecondaryActionChecked()).isTrue();
    }

    @Test
    public void onListenerUpdate_micDisabled_shouldUpdateChecked() {
        initializePreference(/* isMicEnabled= */ false);

        setIsSensorPrivacyEnabled(false);
        mListener.getValue().onSensorPrivacyChanged(SensorPrivacyManager.Sensors.MICROPHONE, false);

        assertThat(mSwitchPreference.isSecondaryActionChecked()).isTrue();
    }

    @Test
    public void onListenerUpdate_micEnabled_shouldUpdateChecked() {
        initializePreference(/* isMicEnabled= */ true);

        setIsSensorPrivacyEnabled(true);
        mListener.getValue().onSensorPrivacyChanged(SensorPrivacyManager.Sensors.MICROPHONE, true);

        assertThat(mSwitchPreference.isSecondaryActionChecked()).isFalse();
    }

    private void initializePreference(boolean isMicEnabled) {
        setIsSensorPrivacyEnabled(!isMicEnabled);
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onStart(mLifecycleOwner);
        verify(mMockSensorPrivacyManager).addSensorPrivacyListener(
                eq(SensorPrivacyManager.Sensors.MICROPHONE), mListener.capture());
    }

    private void setIsSensorPrivacyEnabled(boolean isMuted) {
        when(mMockSensorPrivacyManager.isSensorPrivacyEnabled(
                eq(SensorPrivacyManager.Sensors.MICROPHONE))).thenReturn(isMuted);
    }

    private void setMicMuteFeatureAvailable(boolean isAvailable) {
        when(mMockSensorPrivacyManager
                .supportsSensorToggle(eq(SensorPrivacyManager.Sensors.MICROPHONE)))
                .thenReturn(isAvailable);
    }
}
