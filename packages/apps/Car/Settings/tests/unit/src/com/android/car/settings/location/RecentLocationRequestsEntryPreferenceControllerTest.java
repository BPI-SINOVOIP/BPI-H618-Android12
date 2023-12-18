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

package com.android.car.settings.location;

import static com.android.car.settings.location.RecentLocationRequestsEntryPreferenceController.INTENT_FILTER_LOCATION_MODE_CHANGED;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.car.drivingstate.CarUxRestrictions;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.os.UserHandle;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.Preference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
public class RecentLocationRequestsEntryPreferenceControllerTest {
    private static final long TIMEOUT_MS = 5000;

    private Context mContext = spy(ApplicationProvider.getApplicationContext());
    private LifecycleOwner mLifecycleOwner;
    private Preference mPreference;
    private RecentLocationRequestsEntryPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;
    private LocationManager mLocationManager;

    @Mock
    private FragmentController mFragmentController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();
        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mLocationManager = mContext.getSystemService(LocationManager.class);

        mPreference = new Preference(mContext);
        mPreferenceController = new RecentLocationRequestsEntryPreferenceController(mContext,
                "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreference);

        mPreferenceController.onCreate(mLifecycleOwner);
    }

    @Test
    public void onStart_registersBroadcastReceiver() {
        mPreferenceController.onStart(mLifecycleOwner);
        verify(mContext).registerReceiver(any(BroadcastReceiver.class),
                eq(INTENT_FILTER_LOCATION_MODE_CHANGED));
    }

    @Test
    public void onStop_unregistersBroadcastReceiver() {
        mPreferenceController.onStart(mLifecycleOwner);
        ArgumentCaptor<BroadcastReceiver> captor = ArgumentCaptor.forClass(
                BroadcastReceiver.class);
        verify(mContext).registerReceiver(captor.capture(),
                eq(INTENT_FILTER_LOCATION_MODE_CHANGED));

        mPreferenceController.onStop(mLifecycleOwner);
        verify(mContext).unregisterReceiver(captor.getValue());
    }

    @Test
    public void refreshUi_locationOn_preferenceIsEnabled() {
        setLocationEnabled(true);
        mPreferenceController.refreshUi();

        assertThat(mPreference.isEnabled()).isTrue();
    }

    @Test
    public void refreshUi_locationOff_preferenceIsDisabled() {
        setLocationEnabled(false);
        mPreferenceController.refreshUi();

        assertThat(mPreference.isEnabled()).isFalse();
    }

    @Test
    public void locationModeChangedBroadcastSent_locationOff_preferenceIsDisabled() {
        mPreferenceController.onStart(mLifecycleOwner);
        setLocationEnabled(true);
        mPreferenceController.refreshUi();
        setLocationEnabled(false);

        assertThat(mPreference.isEnabled()).isFalse();
    }

    @Test
    public void locationModeChangedBroadcastSent_locationOn_preferenceIsEnabled() {
        mPreferenceController.onStart(mLifecycleOwner);
        setLocationEnabled(false);
        mPreferenceController.refreshUi();
        setLocationEnabled(true);

        assertThat(mPreference.isEnabled()).isTrue();
    }

    private void setLocationEnabled(boolean enabled) {
        CountDownLatch latch = new CountDownLatch(1);
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                latch.countDown();
            }
        };
        mContext.registerReceiver(receiver, new IntentFilter(LocationManager.MODE_CHANGED_ACTION));
        try {
            mLocationManager.setLocationEnabledForUser(enabled,
                    UserHandle.of(UserHandle.myUserId()));
            assertWithMessage("%s intent reveiced in %sms", LocationManager.MODE_CHANGED_ACTION,
                    TIMEOUT_MS).that(latch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS)).isTrue();
        } catch (InterruptedException e) {
            assertThat(mLocationManager.isLocationEnabled()).isEqualTo(enabled);
        } finally {
            mContext.unregisterReceiver(receiver);
        }
    }
}
