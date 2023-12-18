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

package com.android.car.dialer.ui.activecall;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.withEffectiveVisibility;
import static androidx.test.espresso.matcher.ViewMatchers.withId;

import static org.mockito.Mockito.when;

import android.telecom.Call;
import android.telecom.CallAudioState;

import androidx.core.util.Pair;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModelProvider;
import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.matcher.ViewMatchers;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.R;
import com.android.car.dialer.livedata.CallDetailLiveData;
import com.android.car.dialer.testing.TestActivity;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Collections;

@RunWith(AndroidJUnit4.class)
public class OngoingCallFragmentTest {

    private OngoingCallFragment mOngoingCallFragment;
    private Call mMockCall;
    @Mock
    private CallAudioState mMockCallAudioState;
    private CallDetailLiveData mMockCallDetailLiveData;
    private LiveData<Pair<Integer, Long>> mCallStateAndConnectTimeLiveData;
    private LiveData<Boolean> mShouldShowOnHoldCall;

    private ActivityScenario<TestActivity> mActivityScenario;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        startFragment();
    }

    @Test
    public void testOnCreateView() {
        onView(withId(R.id.incall_dialpad_fragment))
                .check(matches(withEffectiveVisibility(ViewMatchers.Visibility.GONE)));
        onView(withId(R.id.user_profile_container))
                .check(matches(withEffectiveVisibility(ViewMatchers.Visibility.VISIBLE)));
    }

    @Test
    public void testOnOpenDialpad() {
        mActivityScenario.onActivity(activity -> {
            mOngoingCallFragment.onOpenDialpad();
        });

        onView(withId(R.id.incall_dialpad_fragment))
                .check(matches(withEffectiveVisibility(ViewMatchers.Visibility.VISIBLE)));
        onView(withId(R.id.user_profile_container))
                .check(matches(withEffectiveVisibility(ViewMatchers.Visibility.GONE)));
    }

    @Test
    public void testOnCloseDialpad() {
        mActivityScenario.onActivity(activity -> {
            mOngoingCallFragment.onCloseDialpad();
        });

        onView(withId(R.id.incall_dialpad_fragment))
                .check(matches(withEffectiveVisibility(ViewMatchers.Visibility.GONE)));
        onView(withId(R.id.user_profile_container))
                .check(matches(withEffectiveVisibility(ViewMatchers.Visibility.VISIBLE)));
    }

    private void startFragment() {
        mActivityScenario = ActivityScenario.launch(TestActivity.class);
        mActivityScenario.onActivity(activity -> {
            InCallViewModel mockInCallViewModel = new ViewModelProvider(activity).get(
                    InCallViewModel.class);
            mMockCallDetailLiveData = new CallDetailLiveData();
            mMockCallDetailLiveData.setTelecomCall(mMockCall);
            mShouldShowOnHoldCall = new MutableLiveData<>(false);
            mCallStateAndConnectTimeLiveData =
                    new MutableLiveData<>(new Pair<>(Call.STATE_ACTIVE, 1000L));
            when(mockInCallViewModel.getPrimaryCallState())
                    .thenReturn(new MutableLiveData<>(Call.STATE_ACTIVE));
            when(mockInCallViewModel.getPrimaryCallDetail()).thenReturn(mMockCallDetailLiveData);
            when(mockInCallViewModel.getCallStateAndConnectTime())
                    .thenReturn(mCallStateAndConnectTimeLiveData);
            when(mockInCallViewModel.shouldShowOnholdCall()).thenReturn(mShouldShowOnHoldCall);
            when(mockInCallViewModel.getCallAudioState())
                    .thenReturn(new MutableLiveData<>(mMockCallAudioState));
            when(mockInCallViewModel.getAudioRoute()).thenReturn(new MutableLiveData<>(1));
            when(mockInCallViewModel.getSupportedAudioRoutes())
                    .thenReturn(new MutableLiveData<>(Collections.EMPTY_LIST));
            when(mockInCallViewModel.getSecondaryCallDetail()).thenReturn(mMockCallDetailLiveData);
            when(mockInCallViewModel.getPrimaryCall()).thenReturn(new MutableLiveData<>(mMockCall));
            when(mockInCallViewModel.getSecondaryCallConnectTime())
                    .thenReturn(new MutableLiveData<>(1000L));
            when(mockInCallViewModel.getAllCallList())
                    .thenReturn(new MutableLiveData<>(Collections.EMPTY_LIST));
            when(mockInCallViewModel.getOngoingCallPair())
                    .thenReturn(new MutableLiveData<>(new Pair<>(mMockCall, mMockCall)));
            when(mockInCallViewModel.getOngoingCallList())
                    .thenReturn(new MutableLiveData<>(Collections.EMPTY_LIST));
            when(mockInCallViewModel.getDialpadOpenState())
                    .thenReturn(new MutableLiveData<>(false));

            mOngoingCallFragment = new OngoingCallFragment();
            activity.getSupportFragmentManager().beginTransaction().add(
                    R.id.test_fragment_container, mOngoingCallFragment).commit();
        });
    }
}
