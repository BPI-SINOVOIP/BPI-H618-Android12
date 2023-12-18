/*
 * Copyright (C) 2019 The Android Open Source Project
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

import androidx.lifecycle.MediatorLiveData;

import com.android.car.dialer.log.L;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.telephony.common.CallDetail;

import dagger.assisted.Assisted;
import dagger.assisted.AssistedFactory;
import dagger.assisted.AssistedInject;

/**
 * Provides the current connecting audio route.
 */
public class AudioRouteLiveData extends MediatorLiveData<Integer> {
    private static final String TAG = "CD.AudioRouteLiveData";

    private final UiCallManager mUiCallManager;
    private final CallDetailLiveData mPrimaryCallDetailLiveData;

    @AssistedInject
    public AudioRouteLiveData(
            @Assisted CallDetailLiveData primaryCallDetailLiveData,
            UiCallManager callManager) {
        mUiCallManager = callManager;
        mPrimaryCallDetailLiveData = primaryCallDetailLiveData;

        addSource(mPrimaryCallDetailLiveData, this::updateOngoingCallAudioRoute);
    }

    @Override
    protected void onActive() {
        super.onActive();
        updateAudioRoute();
    }

    private void updateAudioRoute() {
        CallDetail primaryCallDetail = mPrimaryCallDetailLiveData.getValue();
        updateOngoingCallAudioRoute(primaryCallDetail);
    }

    private void updateOngoingCallAudioRoute(CallDetail callDetail) {
        if (callDetail == null) {
            // Phone call might have disconnected, no action.
            return;
        }
        int state = callDetail.getScoState();
        int audioRoute = mUiCallManager.getAudioRoute(state);
        if (getValue() == null || audioRoute != getValue()) {
            L.d(TAG, "updateAudioRoute to %s", audioRoute);
            setValue(audioRoute);
        }
    }

    /**
     * Factory to create {@link AudioRouteLiveData} instances via the {@link AssistedInject}
     * constructor.
     */
    @AssistedFactory
    public interface Factory {
        /** Creates an {@link AudioRouteLiveData} instance. */
        AudioRouteLiveData create(CallDetailLiveData primaryCallDetailLiveData);
    }
}
