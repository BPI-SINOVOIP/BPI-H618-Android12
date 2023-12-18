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

package com.android.car.settings.wifi;

import static com.android.car.settings.common.PreferenceController.AVAILABLE;
import static com.android.car.settings.common.PreferenceController.AVAILABLE_FOR_VIEWING;
import static com.android.car.settings.common.PreferenceController.UNSUPPORTED_ON_DEVICE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.net.TetheringManager;

import androidx.lifecycle.Lifecycle;
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
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.concurrent.Executor;

@RunWith(AndroidJUnit4.class)
public class WifiTetherPreferenceControllerTest {
    private Context mContext = spy(ApplicationProvider.getApplicationContext());
    private LifecycleOwner mLifecycleOwner;
    private CarUiTwoActionSwitchPreference mPreference;
    private WifiTetherPreferenceController mController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private Lifecycle mMockLifecycle;
    @Mock
    private TetheringManager mTetheringManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        when(mContext.getSystemService(TetheringManager.class)).thenReturn(mTetheringManager);
        when(mFragmentController.getSettingsLifecycle()).thenReturn(mMockLifecycle);

        mPreference = new CarUiTwoActionSwitchPreference(mContext);
        mController = new WifiTetherPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mController, mPreference);
    }

    @Test
    public void onStart_isAvailableForViewing() {
        mController.onCreate(mLifecycleOwner);
        mController.onStart(mLifecycleOwner);

        assertThat(mController.getAvailabilityStatus()).isEqualTo(
                AVAILABLE_FOR_VIEWING);
    }

    @Test
    public void onStart_registersTetheringEventCallback() {
        mController.onCreate(mLifecycleOwner);
        mController.onStart(mLifecycleOwner);

        verify(mTetheringManager).registerTetheringEventCallback(
                any(Executor.class), any(TetheringManager.TetheringEventCallback.class));
    }

    @Test
    public void onStop_unregistersTetheringEventCallback() {
        mController.onCreate(mLifecycleOwner);
        mController.onStart(mLifecycleOwner);
        mController.onStop(mLifecycleOwner);

        verify(mTetheringManager).unregisterTetheringEventCallback(
                any(TetheringManager.TetheringEventCallback.class));
    }

    @Test
    public void onTetheringSupported_false_isUnsupportedOnDevice() {
        mController.onCreate(mLifecycleOwner);
        mController.onStart(mLifecycleOwner);

        ArgumentCaptor<TetheringManager.TetheringEventCallback> captor =
                ArgumentCaptor.forClass(TetheringManager.TetheringEventCallback.class);
        verify(mTetheringManager).registerTetheringEventCallback(
                any(Executor.class), captor.capture());

        captor.getValue().onTetheringSupported(false);

        assertThat(mController.getAvailabilityStatus()).isEqualTo(
                UNSUPPORTED_ON_DEVICE);
    }

    @Test
    public void onTetheringSupported_true_isAvailable() {
        mController.onCreate(mLifecycleOwner);
        mController.onStart(mLifecycleOwner);

        ArgumentCaptor<TetheringManager.TetheringEventCallback> captor =
                ArgumentCaptor.forClass(TetheringManager.TetheringEventCallback.class);
        verify(mTetheringManager).registerTetheringEventCallback(
                any(Executor.class), captor.capture());
        captor.getValue().onTetheringSupported(false);

        captor.getValue().onTetheringSupported(true);

        assertThat(mController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }
}
