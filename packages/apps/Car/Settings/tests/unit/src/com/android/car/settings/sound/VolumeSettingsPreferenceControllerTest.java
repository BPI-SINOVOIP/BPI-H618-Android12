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

package com.android.car.settings.sound;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.drivingstate.CarUxRestrictions;
import android.car.media.CarAudioManager;
import android.content.Context;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.PreferenceGroup;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;
import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.settings.R;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.LogicalPreferenceGroup;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.common.SeekBarPreference;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class VolumeSettingsPreferenceControllerTest {

    private static final int ZONE_ID = 1;
    private static final int GROUP_ID = 0;
    private static final int TEST_MIN_VOLUME = 0;
    private static final int TEST_VOLUME = 40;
    private static final int TEST_NEW_VOLUME = 80;
    private static final int TEST_MAX_VOLUME = 100;

    private Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private PreferenceGroup mPreferenceGroup;
    private VolumeSettingsPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private Car mCar;
    @Mock
    private CarAudioManager mCarAudioManager;
    @Mock
    private VolumeSettingsRingtoneManager mRingtoneManager;

    @Before
    @UiThreadTest
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        when(mCar.getCarManager(Car.AUDIO_SERVICE)).thenReturn(mCarAudioManager);
        when(mCarAudioManager.getVolumeGroupCount()).thenReturn(1);
        when(mCarAudioManager.getUsagesForVolumeGroupId(GROUP_ID)).thenReturn(new int[]{1, 2});
        when(mCarAudioManager.getGroupMinVolume(GROUP_ID)).thenReturn(TEST_MIN_VOLUME);
        when(mCarAudioManager.getGroupVolume(GROUP_ID)).thenReturn(TEST_VOLUME);
        when(mCarAudioManager.getGroupMaxVolume(GROUP_ID)).thenReturn(TEST_MAX_VOLUME);

        PreferenceManager preferenceManager = new PreferenceManager(mContext);
        PreferenceScreen screen = preferenceManager.createPreferenceScreen(mContext);
        mPreferenceGroup = new LogicalPreferenceGroup(mContext);
        screen.addPreference(mPreferenceGroup);
        mPreferenceController = new TestVolumeSettingsPreferenceController(mContext,
                "key", mFragmentController, mCarUxRestrictions, mCar, mRingtoneManager);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreferenceGroup);
    }

    @Test
    public void testRefreshUi_serviceNotStarted() {
        mPreferenceController.refreshUi();
        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(0);
    }

    @Test
    public void testRefreshUi_serviceStarted() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();
        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(1);
    }

    @Test
    public void onServiceConnected_registersVolumeCallback() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();

        verify(mCarAudioManager).registerCarVolumeCallback(
                mPreferenceController.mVolumeChangeCallback);
    }

    @Test
    public void testRefreshUi_serviceStarted_multipleCalls() {
        mPreferenceController.onCreate(mLifecycleOwner);

        // Calling this multiple times shouldn't increase the number of elements.
        mPreferenceController.refreshUi();
        mPreferenceController.refreshUi();
        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(1);
    }

    @Test
    public void testRefreshUi_createdPreferenceHasMinMax() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();
        SeekBarPreference preference = (SeekBarPreference) mPreferenceGroup.getPreference(0);
        assertThat(preference.getMin()).isEqualTo(TEST_MIN_VOLUME);
        assertThat(preference.getValue()).isEqualTo(TEST_VOLUME);
        assertThat(preference.getMax()).isEqualTo(TEST_MAX_VOLUME);
    }

    @Test
    public void testOnPreferenceChange_ringtonePlays() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();
        SeekBarPreference preference = (SeekBarPreference) mPreferenceGroup.getPreference(0);
        preference.getOnPreferenceChangeListener().onPreferenceChange(preference, TEST_NEW_VOLUME);
        verify(mRingtoneManager).playAudioFeedback(anyInt(), anyInt());
    }

    @Test
    public void testOnPreferenceChange_audioManagerSet() throws CarNotConnectedException {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();
        SeekBarPreference preference = (SeekBarPreference) mPreferenceGroup.getPreference(0);
        preference.getOnPreferenceChangeListener().onPreferenceChange(preference, TEST_NEW_VOLUME);
        verify(mCarAudioManager).setGroupVolume(GROUP_ID, TEST_NEW_VOLUME, 0);
    }

    @Test
    public void onGroupVolumeChanged_sameValue_doesNotUpdateVolumeSeekbar() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();
        SeekBarPreference preference = spy((SeekBarPreference) mPreferenceGroup.getPreference(0));
        mPreferenceController.mVolumeChangeCallback.onGroupVolumeChanged(ZONE_ID,
                GROUP_ID, /* flags= */ 0);

        verify(preference, never()).setValue(any(Integer.class));
    }

    @Test
    public void onGroupVolumeChanged_differentValue_updatesVolumeSeekbar() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.refreshUi();
        when(mCarAudioManager.getGroupVolume(GROUP_ID)).thenReturn(TEST_NEW_VOLUME);
        mPreferenceController.mVolumeChangeCallback.onGroupVolumeChanged(ZONE_ID,
                GROUP_ID, /* flags= */ 0);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        SeekBarPreference preference = (SeekBarPreference) mPreferenceGroup.getPreference(0);
        assertThat(preference.getValue()).isEqualTo(TEST_NEW_VOLUME);
    }

    private static class TestVolumeSettingsPreferenceController extends
            VolumeSettingsPreferenceController {

        TestVolumeSettingsPreferenceController(Context context, String preferenceKey,
                FragmentController fragmentController,
                CarUxRestrictions uxRestrictions, Car car,
                VolumeSettingsRingtoneManager ringtoneManager) {
            super(context, preferenceKey, fragmentController, uxRestrictions, car, ringtoneManager);
        }

        @Override
        public int carVolumeItemsXml() {
            return R.xml.test_car_volume_items;
        }
    }
}
