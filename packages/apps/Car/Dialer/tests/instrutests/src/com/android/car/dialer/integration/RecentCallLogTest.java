/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.dialer.integration;

import static android.app.Activity.RESULT_OK;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.intent.Intents.intending;
import static androidx.test.espresso.intent.matcher.IntentMatchers.hasComponent;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.allOf;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Instrumentation;
import android.content.Intent;
import android.net.Uri;
import android.telecom.TelecomManager;
import android.view.View;

import androidx.lifecycle.MutableLiveData;
import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.intent.Intents;
import androidx.test.espresso.matcher.BoundedMatcher;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.dialer.R;
import com.android.car.dialer.bluetooth.CallHistoryManager;
import com.android.car.dialer.framework.FakeHfpManager;
import com.android.car.dialer.livedata.CallHistoryLiveData;
import com.android.car.dialer.ui.TelecomActivity;
import com.android.car.dialer.ui.activecall.InCallActivity;
import com.android.car.dialer.widget.CallTypeIconsView;
import com.android.car.telephony.common.PhoneCallLog;

import org.hamcrest.Description;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import javax.inject.Inject;

import dagger.hilt.android.testing.BindValue;
import dagger.hilt.android.testing.HiltAndroidRule;
import dagger.hilt.android.testing.HiltAndroidTest;

@SmallTest
@RunWith(AndroidJUnit4.class)
@HiltAndroidTest
public class RecentCallLogTest {
    private static final String HEADER = "Today";
    private static final String RELATIVE_TIME = "0 min. ago";
    private static final String PHONE_NUMBER = "511";
    private static final Uri CALL_URI = Uri.fromParts("tel", PHONE_NUMBER, null);

    @Inject
    FakeHfpManager mFakeHfpManager;
    @Inject
    TelecomManager mTelecomManager;
    @BindValue
    @Mock
    CallHistoryManager mMockCallHistoryManager;
    @Mock
    PhoneCallLog mMockPhoneCallLog;
    private PhoneCallLog.Record mIncomingRecord;
    private PhoneCallLog.Record mOutgoingRecord;

    @Rule
    public final HiltAndroidRule mHiltAndroidRule = new HiltAndroidRule(this);

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(RecentCallLogTest.this);

        Intents.init();
        // CardView error, suppressing InCallActivity by responding with RESULT_OK.
        intending(hasComponent(InCallActivity.class.getName())).respondWith(
                new Instrumentation.ActivityResult(RESULT_OK, null));

        mIncomingRecord = new PhoneCallLog.Record(System.currentTimeMillis(),
                CallHistoryLiveData.CallType.INCOMING_TYPE);
        mOutgoingRecord = new PhoneCallLog.Record(System.currentTimeMillis() - 10000,
                CallHistoryLiveData.CallType.OUTGOING_TYPE);
        MutableLiveData<List<PhoneCallLog>> callLogLiveData = new MutableLiveData<>();
        callLogLiveData.postValue(Collections.singletonList(mMockPhoneCallLog));
        when(mMockCallHistoryManager.getCallHistoryLiveData()).thenReturn(callLogLiveData);
        when(mMockPhoneCallLog.getAllCallRecords()).thenReturn(
                Arrays.asList(mIncomingRecord, mOutgoingRecord));
        when(mMockPhoneCallLog.getLastCallEndTimestamp()).thenReturn(
                mIncomingRecord.getCallEndTimestamp());

        mHiltAndroidRule.inject();
        mFakeHfpManager.connectHfpDevice();
    }

    @Test
    public void verifyRecentCallScreen() {
        when(mMockPhoneCallLog.getPhoneNumberString()).thenReturn(PHONE_NUMBER);

        ActivityScenario.launch(
                new Intent(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                        TelecomActivity.class));
        onView(withText(R.string.call_history_title)).check(matches(isDisplayed()));

        onView(allOf(withText(HEADER), withId(R.id.title))).check(matches(isDisplayed()));
        onView(allOf(withText(PHONE_NUMBER), withId(R.id.title))).check(matches(isDisplayed()));
        onView(allOf(withText(RELATIVE_TIME), withId(R.id.text))).check(matches(isDisplayed()));

        onView(withId(R.id.call_type_icons))
                .check(matches(
                        new CallTypeIconMatcher(0, CallHistoryLiveData.CallType.INCOMING_TYPE)))
                .check(matches(
                        new CallTypeIconMatcher(1, CallHistoryLiveData.CallType.OUTGOING_TYPE)));

        onView(withId(R.id.call_action_id)).perform(click());
        verify(mTelecomManager).placeCall(eq(CALL_URI), isNull());
    }

    @Test
    public void emptyPhoneNumber_showAsUnknownCall() {
        when(mMockPhoneCallLog.getPhoneNumberString()).thenReturn("");

        ActivityScenario.launch(
                new Intent(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                        TelecomActivity.class));
        // Verify there is no loading issue and the call is displayed as unknown calls.
        onView(allOf(withText("Unknown"), withId(R.id.title))).check(matches(isDisplayed()));
    }

    @After
    public void tearDown() {
        Intents.release();
    }

    private static class CallTypeIconMatcher extends BoundedMatcher<View, CallTypeIconsView> {
        private final int mIndex;
        private final int mExpectedIconType;

        CallTypeIconMatcher(int index, int expectedIconType) {
            super(CallTypeIconsView.class);
            mIndex = index;
            mExpectedIconType = expectedIconType;
        }

        @Override
        protected boolean matchesSafely(CallTypeIconsView callTypeIconsView) {
            return callTypeIconsView.getCallType(mIndex) == mExpectedIconType;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("Call icon type at " + mIndex + " is " + mExpectedIconType);
        }
    }
}
