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

package com.android.car.dialer.livedata;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.telecom.Call;
import android.telecom.CallAudioState;

import androidx.lifecycle.Observer;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.telecom.UiCallManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class AudioRouteLiveDataTest {
    private AudioRouteLiveData mAudioRouteLiveData;
    private CallDetailLiveData mCallDetailLiveData;

    @Mock
    private Observer<Integer> mMockObserver;
    @Mock
    private UiCallManager mMockUiCallManager;
    @Mock
    Call mMockCall;
    @Mock
    Call.Details mMockCallDetails;


    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        when(mMockCall.getDetails()).thenReturn(mMockCallDetails);
        mCallDetailLiveData = new CallDetailLiveData();

        when(mMockUiCallManager.getAudioRoute(anyInt())).thenReturn(CallAudioState.ROUTE_EARPIECE);

        mAudioRouteLiveData = new AudioRouteLiveData(mCallDetailLiveData, mMockUiCallManager);
    }

    @Test
    @UiThreadTest
    public void onActive_updateAudioRoute() {
        verify(mMockObserver, never()).onChanged(any());

        mCallDetailLiveData.setTelecomCall(mMockCall);
        mAudioRouteLiveData.observeForever(mMockObserver);

        verify(mMockObserver).onChanged(CallAudioState.ROUTE_EARPIECE);
    }

    @Test
    @UiThreadTest
    public void callDetailChange_updateAudioRoute() {
        mCallDetailLiveData.setTelecomCall(mMockCall);
        mAudioRouteLiveData.observeForever(mMockObserver);
        assertThat(mAudioRouteLiveData.getValue()).isEqualTo(CallAudioState.ROUTE_EARPIECE);

        when(mMockUiCallManager.getAudioRoute(anyInt())).thenReturn(CallAudioState.ROUTE_BLUETOOTH);
        mCallDetailLiveData.setTelecomCall(mMockCall);

        verify(mMockObserver).onChanged(CallAudioState.ROUTE_BLUETOOTH);
        assertThat(mAudioRouteLiveData.getValue()).isEqualTo(CallAudioState.ROUTE_BLUETOOTH);
    }
}
