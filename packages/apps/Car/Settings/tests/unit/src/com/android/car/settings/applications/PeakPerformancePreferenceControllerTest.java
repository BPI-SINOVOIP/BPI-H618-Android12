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

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.Car;
import android.car.drivingstate.CarUxRestrictions;
import android.car.watchdog.CarWatchdogManager;
import android.car.watchdog.PackageKillableState;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.os.Bundle;
import android.os.Process;
import android.os.UserHandle;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.SwitchPreference;
import androidx.preference.TwoStatePreference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.ConfirmationDialogFragment;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;
import org.mockito.quality.Strictness;

import java.util.Collections;

@RunWith(AndroidJUnit4.class)
public class PeakPerformancePreferenceControllerTest {
    private static final String PKG_NAME = "package.name";
    private static final int UID = Process.myUid();

    private MockitoSession mMockingSession;
    private final Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private CarUxRestrictions mCarUxRestrictions;
    private UserHandle mUserHandle;
    private PeakPerformancePreferenceController mController;
    private TwoStatePreference mTwoStatePreference;

    @Captor
    ArgumentCaptor<Car.CarServiceLifecycleListener> mCarLifecycleCaptor;
    @Captor
    ArgumentCaptor<ConfirmationDialogFragment> mDialogFragment;
    @Mock
    private FragmentController mFragmentController;
    @Mock
    private Car mMockCar;
    @Mock
    private CarWatchdogManager mMockManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mMockingSession = mockitoSession()
                .initMocks(this)
                .mockStatic(Car.class)
                .strictness(Strictness.LENIENT)
                .startMocking();
        mLifecycleOwner = new TestLifecycleOwner();
        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();
        mUserHandle = UserHandle.getUserHandleForUid(UID);

        mTwoStatePreference = new SwitchPreference(mContext);

        mController = new PeakPerformancePreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mController, mTwoStatePreference);

        when(Car.createCar(any(), any(), anyLong(), mCarLifecycleCaptor.capture())).then(
                invocation -> {
                    Car.CarServiceLifecycleListener listener = mCarLifecycleCaptor.getValue();
                    listener.onLifecycleChanged(mMockCar, true);
                    return mMockCar;
                });
        when(mMockCar.getCarManager(Car.CAR_WATCHDOG_SERVICE)).thenReturn(mMockManager);

        PackageInfo packageInfo = new PackageInfo();
        packageInfo.packageName = PKG_NAME;

        ApplicationInfo applicationInfo = new ApplicationInfo();
        applicationInfo.packageName = PKG_NAME;
        packageInfo.applicationInfo = applicationInfo;
        packageInfo.applicationInfo.uid = UID;
        mController.setPackageInfo(packageInfo);
    }

    @After
    public void tearDown() {
        mMockingSession.finishMocking();
    }

    @Test
    public void onCreate_peakPerformance_withKillableStateYes() {
        when(mMockManager.getPackageKillableStatesAsUser(any())).thenReturn(
                Collections.singletonList(
                        new PackageKillableState(PKG_NAME, mUserHandle.getIdentifier(),
                                PackageKillableState.KILLABLE_STATE_YES)));

        mController.onCreate(mLifecycleOwner);

        assertThat(mTwoStatePreference.isChecked()).isTrue();
        assertThat(mTwoStatePreference.isEnabled()).isTrue();
    }

    @Test
    public void onCreate_peakPerformance_withKillableStateNo() {
        when(mMockManager.getPackageKillableStatesAsUser(any())).thenReturn(
                Collections.singletonList(
                        new PackageKillableState(PKG_NAME, mUserHandle.getIdentifier(),
                                PackageKillableState.KILLABLE_STATE_NO)));

        mController.onCreate(mLifecycleOwner);

        assertThat(mTwoStatePreference.isChecked()).isFalse();
        assertThat(mTwoStatePreference.isEnabled()).isTrue();
    }

    @Test
    public void onCreate_peakPerformance_withKillableStateNever() {
        when(mMockManager.getPackageKillableStatesAsUser(any())).thenReturn(
                Collections.singletonList(
                        new PackageKillableState(PKG_NAME, mUserHandle.getIdentifier(),
                                PackageKillableState.KILLABLE_STATE_NEVER)));

        mController.onCreate(mLifecycleOwner);

        assertThat(mTwoStatePreference.isChecked()).isFalse();
        assertThat(mTwoStatePreference.isEnabled()).isFalse();
    }

    @Test
    public void callChangeListener_enablingPeakPerformance() {
        when(mMockManager.getPackageKillableStatesAsUser(any())).thenReturn(
                Collections.singletonList(
                        new PackageKillableState(PKG_NAME, mUserHandle.getIdentifier(),
                                PackageKillableState.KILLABLE_STATE_NO)));
        mController.onCreate(mLifecycleOwner);
        assertThat(mTwoStatePreference.isChecked()).isFalse();

        mTwoStatePreference.callChangeListener(true);

        verify(mMockManager).setKillablePackageAsUser(PKG_NAME, mUserHandle, true);
        assertThat(mTwoStatePreference.isChecked()).isTrue();
        assertThat(mTwoStatePreference.isEnabled()).isTrue();
    }

    @Test
    public void callChangeListener_disablingPeakPerformance() {
        when(mMockManager.getPackageKillableStatesAsUser(any())).thenReturn(
                Collections.singletonList(
                        new PackageKillableState(PKG_NAME, mUserHandle.getIdentifier(),
                                PackageKillableState.KILLABLE_STATE_YES)));
        mController.onCreate(mLifecycleOwner);
        assertThat(mTwoStatePreference.isChecked()).isTrue();

        mTwoStatePreference.callChangeListener(false);
        verify(mFragmentController).showDialog(mDialogFragment.capture(),
                eq(PeakPerformancePreferenceController.TURN_OFF_PEAK_PERFORMANCE_DIALOG_TAG));

        mDialogFragment.getValue().getConfirmListener().onConfirm(new Bundle());

        verify(mMockManager).setKillablePackageAsUser(PKG_NAME, mUserHandle, false);
        assertThat(mTwoStatePreference.isChecked()).isFalse();
        assertThat(mTwoStatePreference.isEnabled()).isTrue();
    }
}
