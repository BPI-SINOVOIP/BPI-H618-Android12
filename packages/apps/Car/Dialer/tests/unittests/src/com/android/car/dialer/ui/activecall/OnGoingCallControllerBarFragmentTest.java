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
import static androidx.test.espresso.matcher.ViewMatchers.isClickable;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isEnabled;
import static androidx.test.espresso.matcher.ViewMatchers.isNotClickable;
import static androidx.test.espresso.matcher.ViewMatchers.isNotEnabled;
import static androidx.test.espresso.matcher.ViewMatchers.withEffectiveVisibility;
import static androidx.test.espresso.matcher.ViewMatchers.withId;

import static com.android.car.dialer.testing.TestViewActions.selfClick;
import static com.android.car.dialer.testing.TestViewMatchers.isActivated;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.telecom.Call;
import android.telecom.CallAudioState;

import androidx.core.util.Pair;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;
import androidx.lifecycle.ViewModelProvider;
import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.matcher.ViewMatchers;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.R;
import com.android.car.dialer.testing.TestActivity;
import com.android.car.telephony.common.CallDetail;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class OnGoingCallControllerBarFragmentTest {
    private List<Integer> mAudioRouteList = new ArrayList<>();
    private MutableLiveData<Call> mPrimaryCallLiveData;
    private MutableLiveData<Integer> mCallStateLiveData;
    private MutableLiveData<CallDetail> mCallDetailLiveData;
    private MutableLiveData<Boolean> mDialpadStateLiveData;
    private MutableLiveData<List<Call>> mCallListLiveData;
    private MutableLiveData<CallAudioState> mCallAudioStateLiveData;
    private LiveData<Pair<Call, Call>> mOngoingCallPairLiveData;
    private List<Call> mCallList;
    @Mock
    private Call mMockCall;
    @Mock
    private Call.Details mMockDetails;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        when(mMockCall.getDetails()).thenReturn(mMockDetails);
        mPrimaryCallLiveData = new MutableLiveData<>(mMockCall);
        mCallList = new ArrayList<>();
        mCallList.add(mMockCall);
        mCallListLiveData = new MutableLiveData<>(mCallList);
        mOngoingCallPairLiveData = Transformations.map(mCallListLiveData,
                calls -> calls.size() > 1 ? new Pair<>(mMockCall, calls.get(1))
                        : new Pair<>(mMockCall, null));

        mCallStateLiveData = new MutableLiveData<>();
        mCallDetailLiveData = new MutableLiveData<>();
        mDialpadStateLiveData = new MutableLiveData<>(false);
        mCallAudioStateLiveData = new MutableLiveData<>();
    }

    @Test
    public void testMuteButton() {
        addFragment(Call.STATE_ACTIVE);

        // Test initial state
        onView(withId(R.id.mute_button)).check(matches(isClickable())).check(
                matches(isActivated(false)));

        // Mute
        onView(withId(R.id.mute_button)).perform(selfClick()).check(matches(isActivated(true)));
        // Unmute
        onView(withId(R.id.mute_button)).perform(selfClick()).check(matches(isActivated(false)));
    }

    @Test
    public void testDialpadButton() {
        addFragment(Call.STATE_ACTIVE);

        // Test initial state
        onView(withId(R.id.toggle_dialpad_button)).check(matches(isClickable())).check(
                matches(isActivated(false)));
        assertThat(mDialpadStateLiveData.getValue()).isFalse();
        // On open dialpad
        onView(withId(R.id.toggle_dialpad_button)).perform(selfClick()).check(
                matches(isActivated(true)));
        assertThat(mDialpadStateLiveData.getValue()).isTrue();
        // On close dialpad
        onView(withId(R.id.toggle_dialpad_button)).perform(selfClick()).check(
                matches(isActivated(false)));
        assertThat(mDialpadStateLiveData.getValue()).isFalse();
    }

    @Test
    public void testEndCallButton() {
        addFragment(Call.STATE_ACTIVE);

        onView(withId(R.id.end_call_button)).check(matches(isClickable()));
        // onEndCall
        onView(withId(R.id.end_call_button)).perform(selfClick());
        verify(mMockCall).disconnect();
    }

    @Test
    public void testAudioRouteButton_withOneAudioRoute() {
        addFragment(Call.STATE_ACTIVE);

        onView(withId(R.id.voice_channel_view)).check(matches(isNotClickable()));
    }

    @Test
    public void testAudioRouteButtonView_withMultipleAudioRoutes() {
        mAudioRouteList.add(CallAudioState.ROUTE_EARPIECE);
        addFragment(Call.STATE_ACTIVE);

        onView(withId(R.id.voice_channel_view)).check(matches(isClickable()));
    }

    @Test
    public void testClickPauseButton_activeCall() {
        addFragment(Call.STATE_ACTIVE);

        onView(withId(R.id.pause_button)).check(matches(isClickable())).check(
                matches(isEnabled())).check(matches(isActivated(false)));

        // onHoldCall
        onView(withId(R.id.pause_button)).perform(selfClick());
        verify(mMockCall).hold();
    }

    @Test
    public void testClickPauseButton_onholdCall() {
        addFragment(Call.STATE_HOLDING);

        onView(withId(R.id.pause_button)).check(matches(isClickable())).check(
                matches(isEnabled())).check(matches(isActivated(true)));

        // onUnholdCall
        onView(withId(R.id.pause_button)).perform(selfClick());
        verify(mMockCall).unhold();
    }

    @Test
    public void testClickPauseButton_connectingCall() {
        addFragment(Call.STATE_DIALING);
        onView(withId(R.id.pause_button)).check(matches(isNotEnabled()));

        onView(withId(R.id.pause_button)).perform(selfClick());
        verify(mMockCall, never()).hold();
        verify(mMockCall, never()).unhold();
    }

    @Test
    public void testPauseButton_MultipleCall() {
        Call secondCall = mock(Call.class);
        mCallList.add(secondCall);
        addFragment(Call.STATE_ACTIVE);

        onView(withId(R.id.pause_button)).check(matches(withEffectiveVisibility(
                ViewMatchers.Visibility.GONE)));
        onView(withId(R.id.merge_button)).check(matches(isDisplayed()));
    }

    private void addFragment(int callState) {
        ActivityScenario<TestActivity> activityScenario = ActivityScenario.launch(
                TestActivity.class);
        activityScenario.onActivity(activity -> {
            InCallViewModel mockInCallViewModel = new ViewModelProvider(activity).get(
                    InCallViewModel.class);

            mCallStateLiveData.setValue(callState);
            when(mockInCallViewModel.getPrimaryCall()).thenReturn(mPrimaryCallLiveData);
            when(mockInCallViewModel.getPrimaryCallDetail()).thenReturn(mCallDetailLiveData);
            when(mockInCallViewModel.getPrimaryCallState()).thenReturn(mCallStateLiveData);
            when(mockInCallViewModel.getDialpadOpenState()).thenReturn(mDialpadStateLiveData);
            when(mockInCallViewModel.getAllCallList()).thenReturn(mCallListLiveData);
            when(mockInCallViewModel.getCallAudioState()).thenReturn(mCallAudioStateLiveData);
            when(mockInCallViewModel.getOngoingCallPair()).thenReturn(mOngoingCallPairLiveData);
            when(mockInCallViewModel.getOngoingCallList()).thenReturn(mCallListLiveData);

            mAudioRouteList.add(CallAudioState.ROUTE_BLUETOOTH);
            when(mockInCallViewModel.getSupportedAudioRoutes()).thenReturn(
                    new MutableLiveData<>(mAudioRouteList));
            MutableLiveData<Integer> audioRouteLiveData = new MutableLiveData<>(
                    CallAudioState.ROUTE_BLUETOOTH);
            when(mockInCallViewModel.getAudioRoute()).thenReturn(audioRouteLiveData);

            MutableLiveData<Pair<Integer, Long>> stateAndConnectTimeLiveData =
                    new MutableLiveData<>();
            when(mockInCallViewModel.getCallStateAndConnectTime())
                    .thenReturn(stateAndConnectTimeLiveData);

            activity.getSupportFragmentManager().beginTransaction().add(
                    R.id.test_fragment_container,
                    new OnGoingCallControllerBarFragment()).commit();
        });
    }
}
