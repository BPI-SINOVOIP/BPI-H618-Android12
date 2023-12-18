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
package com.android.car.dialer.livedata;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.text.format.DateUtils;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.Observer;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.testing.TestActivity;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class HeartBeatLiveDataTest {

    private HeartBeatLiveData mHeartBeatLiveData;
    @Mock
    private Observer<Boolean> mMockObserver;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        mHeartBeatLiveData = new HeartBeatLiveData(DateUtils.SECOND_IN_MILLIS);
    }

    @Test
    public void active_onLifecycleStart() {
        ActivityScenario<TestActivity> activityScenario = ActivityScenario.launch(
                TestActivity.class);
        activityScenario.moveToState(Lifecycle.State.CREATED);
        activityScenario.onActivity(activity ->
                mHeartBeatLiveData.observe(activity, (value) -> mMockObserver.onChanged(value)));
        verify(mMockObserver, never()).onChanged(any());

        activityScenario.moveToState(Lifecycle.State.STARTED);
        verify(mMockObserver).onChanged(any());
    }
}
