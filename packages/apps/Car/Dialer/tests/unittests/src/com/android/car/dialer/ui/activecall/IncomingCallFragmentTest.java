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
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.mockito.Mockito.when;

import android.telecom.Call;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModelProvider;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.R;
import com.android.car.dialer.testing.MockEntityFactory;
import com.android.car.dialer.testing.TestActivity;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;


@RunWith(AndroidJUnit4.class)
public class IncomingCallFragmentTest {
    private static final String NUMBER = "1234567890";
    @Mock
    private Call mMockCall;
    private LiveData<Call> mCallLiveData;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mMockCall.getState()).thenReturn(Call.STATE_RINGING);
    }

    @Test
    public void testCallStateIsRinging() {
        startFragment();

        onView(withId(R.id.user_profile_call_state))
                .check(matches(withText(R.string.call_state_call_ringing)))
                .check(matches(isDisplayed()));
    }

    @Test
    public void testUserProfile() {
        Call.Details mockDetails = MockEntityFactory.createMockCallDetails(NUMBER);
        when(mMockCall.getDetails()).thenReturn(mockDetails);

        startFragment();

        onView(withId(R.id.user_profile_title)).check(matches(withText(NUMBER)));
    }

    private void startFragment() {
        ActivityScenario<TestActivity> activityScenario = ActivityScenario.launch(
                TestActivity.class);
        activityScenario.onActivity(activity -> {
            InCallViewModel mockInCallViewModel = new ViewModelProvider(activity).get(
                    InCallViewModel.class);
            mCallLiveData = new MutableLiveData<>(mMockCall);
            when(mockInCallViewModel.getIncomingCall()).thenReturn(mCallLiveData);
            activity.getSupportFragmentManager().beginTransaction().add(
                    R.id.test_fragment_container,
                    new IncomingCallFragment()).commit();
        });
    }
}
