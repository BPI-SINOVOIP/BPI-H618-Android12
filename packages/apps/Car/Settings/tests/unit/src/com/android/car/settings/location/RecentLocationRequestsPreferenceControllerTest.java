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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.PreferenceGroup;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;
import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.R;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.LogicalPreferenceGroup;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.settingslib.location.RecentLocationApps;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class RecentLocationRequestsPreferenceControllerTest {
    private Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private PreferenceGroup mPreferenceGroup;
    private RecentLocationRequestsPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private RecentLocationApps mRecentLocationApps;

    @Before
    @UiThreadTest
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();
        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        PreferenceManager preferenceManager = new PreferenceManager(mContext);
        PreferenceScreen screen = preferenceManager.createPreferenceScreen(mContext);
        mPreferenceGroup = new LogicalPreferenceGroup(mContext);
        screen.addPreference(mPreferenceGroup);
        mPreferenceController = new RecentLocationRequestsPreferenceController(mContext,
                "key", mFragmentController, mCarUxRestrictions);
        mPreferenceController.setRecentLocationApps(mRecentLocationApps);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreferenceGroup);
        mPreferenceController.onCreate(mLifecycleOwner);
    }

    @Test
    public void refreshUi_noRecentRequests_messageDisplayed() {
        when(mRecentLocationApps.getAppListSorted(true)).thenReturn(Collections.emptyList());
        mPreferenceController.refreshUi();

        assertThat(mPreferenceGroup.getPreference(0).getTitle()).isEqualTo(
                mContext.getString(R.string.location_settings_recent_requests_empty_message));
    }

    @Test
    public void refreshUi_someRecentRequests_preferencesAddedToScreen() {
        List<RecentLocationApps.Request> list = Arrays.asList(
                mock(RecentLocationApps.Request.class),
                mock(RecentLocationApps.Request.class),
                mock(RecentLocationApps.Request.class));
        when(mRecentLocationApps.getAppListSorted(true)).thenReturn(list);
        mPreferenceController.refreshUi();

        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(list.size());
    }

    @Test
    public void refreshUi_newRecentRequests_listIsUpdated() {
        List<RecentLocationApps.Request> list1 = Arrays.asList(
                mock(RecentLocationApps.Request.class),
                mock(RecentLocationApps.Request.class),
                mock(RecentLocationApps.Request.class));
        when(mRecentLocationApps.getAppListSorted(true)).thenReturn(list1);

        List<RecentLocationApps.Request> list2 = new ArrayList<>(list1);
        list2.add(mock(RecentLocationApps.Request.class));

        mPreferenceController.refreshUi();
        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(list1.size());

        when(mRecentLocationApps.getAppListSorted(true)).thenReturn(list2);
        mPreferenceController.refreshUi();

        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(list2.size());
    }
}
