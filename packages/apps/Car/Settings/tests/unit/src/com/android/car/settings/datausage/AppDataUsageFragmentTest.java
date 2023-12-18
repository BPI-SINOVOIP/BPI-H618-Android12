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

package com.android.car.settings.datausage;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.net.NetworkPolicy;
import android.os.Bundle;
import android.text.format.DateUtils;
import android.util.Pair;

import androidx.fragment.app.FragmentManager;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;

import com.android.car.settings.R;
import com.android.car.settings.testutils.BaseCarSettingsTestActivity;
import com.android.settingslib.NetworkPolicyEditor;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.time.ZonedDateTime;
import java.util.ArrayList;
import java.util.Iterator;

/** Unit test for {@link AppDataUsageFragment}. */
@RunWith(AndroidJUnit4.class)
public class AppDataUsageFragmentTest {
    private static final String KEY_START = "start";
    private static final String KEY_END = "end";

    private Context mContext = ApplicationProvider.getApplicationContext();
    private TestAppDataUsageFragment mFragment;
    private BaseCarSettingsTestActivity mActivity;
    private FragmentManager mFragmentManager;

    private Iterator<Pair<ZonedDateTime, ZonedDateTime>> mIterator;

    @Mock
    private NetworkPolicyEditor mNetworkPolicyEditor;
    @Mock
    private NetworkPolicy mNetworkPolicy;

    @Rule
    public ActivityTestRule<BaseCarSettingsTestActivity> mActivityTestRule =
            new ActivityTestRule<>(BaseCarSettingsTestActivity.class);

    @Before
    public void setUp() throws Throwable {
        MockitoAnnotations.initMocks(this);
        mActivity = mActivityTestRule.getActivity();
        mFragmentManager = mActivityTestRule.getActivity().getSupportFragmentManager();
    }

    @Test
    public void onActivityCreated_policyIsNull_startAndEndDateShouldHaveFourWeeksDifference()
            throws Throwable {
        setUpFragment();
        Bundle bundle = mFragment.getBundle();
        long start = bundle.getLong(KEY_START);
        long end = bundle.getLong(KEY_END);
        long timeDiff = end - start;

        assertThat(timeDiff).isEqualTo(DateUtils.WEEK_IN_MILLIS * 4);
    }

    @Test
    public void onActivityCreated_iteratorIsEmpty_startAndEndDateShouldHaveFourWeeksDifference()
            throws Throwable {
        when(mNetworkPolicyEditor.getPolicy(any())).thenReturn(mNetworkPolicy);

        ArrayList<Pair<ZonedDateTime, ZonedDateTime>> list = new ArrayList<>();
        mIterator = list.iterator();
        setUpFragment();

        Bundle bundle = mFragment.getBundle();
        long start = bundle.getLong(KEY_START);
        long end = bundle.getLong(KEY_END);
        long timeDiff = end - start;

        assertThat(timeDiff).isEqualTo(DateUtils.WEEK_IN_MILLIS * 4);
    }

    @Test
    public void onActivityCreated_iteratorIsNotEmpty_startAndEndDateShouldBeLastOneInIterator()
            throws Throwable {
        when(mNetworkPolicyEditor.getPolicy(any())).thenReturn(mNetworkPolicy);

        ZonedDateTime start1 = ZonedDateTime.now();
        ZonedDateTime end1 = ZonedDateTime.now();
        ZonedDateTime start2 = ZonedDateTime.now();
        ZonedDateTime end2 = ZonedDateTime.now();

        Pair pair1 = new Pair(start1, end1);
        Pair pair2 = new Pair(start2, end2);

        ArrayList<Pair<ZonedDateTime, ZonedDateTime>> list = new ArrayList<>();
        list.add(pair1);
        list.add(pair2);

        mIterator = list.iterator();
        setUpFragment();

        Bundle bundle = mFragment.getBundle();
        long start = bundle.getLong(KEY_START);
        long end = bundle.getLong(KEY_END);

        assertThat(start).isEqualTo(start2.toInstant().toEpochMilli());
        assertThat(end).isEqualTo(end2.toInstant().toEpochMilli());
    }

    private void setUpFragment() throws Throwable {
        String appDataUsageFragmentTag = "app_data_usage_fragment";
        mActivityTestRule.runOnUiThread(() -> {
            mFragmentManager.beginTransaction()
                    .replace(R.id.fragment_container,
                            TestAppDataUsageFragment.newInstance(mNetworkPolicyEditor, mIterator),
                            appDataUsageFragmentTag)
                    .commitNow();
        });
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        mFragment = (TestAppDataUsageFragment) mFragmentManager
                .findFragmentByTag(appDataUsageFragmentTag);
    }

    public static class TestAppDataUsageFragment extends AppDataUsageFragment {

        private NetworkPolicyEditor mNetworkPolicyEditor;
        private Iterator<Pair<ZonedDateTime, ZonedDateTime>> mIterator;

        public static TestAppDataUsageFragment newInstance(NetworkPolicyEditor networkPolicyEditor,
                Iterator<Pair<ZonedDateTime, ZonedDateTime>> cycleIterator) {
            TestAppDataUsageFragment fragment = new TestAppDataUsageFragment();
            fragment.mNetworkPolicyEditor = networkPolicyEditor;
            fragment.mIterator = cycleIterator;
            return fragment;
        }

        @Override
        NetworkPolicyEditor getNetworkPolicyEditor(Context context) {
            return mNetworkPolicyEditor;
        }

        @Override
        Iterator<Pair<ZonedDateTime, ZonedDateTime>> getCycleIterator(NetworkPolicy policy) {
            if (mIterator != null) {
                return mIterator;
            }
            return super.getCycleIterator(policy);
        }
    }
}
