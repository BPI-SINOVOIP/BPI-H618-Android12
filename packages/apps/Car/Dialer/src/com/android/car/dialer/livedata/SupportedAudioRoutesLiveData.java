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

import android.telecom.PhoneAccountHandle;

import androidx.lifecycle.MediatorLiveData;

import com.android.car.dialer.bluetooth.PhoneAccountManager;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.telephony.common.CallDetail;

import java.util.List;

import dagger.assisted.Assisted;
import dagger.assisted.AssistedFactory;
import dagger.assisted.AssistedInject;

/**
 * Returns a list of {@link android.telecom.CallAudioState.CallAudioRoute}s for the primary ongoing
 * call.
 */
public class SupportedAudioRoutesLiveData extends MediatorLiveData<List<Integer>> {
    private boolean mIsHfpConnection;
    private final UiCallManager mUiCallManager;
    private final PhoneAccountManager mPhoneAccountManager;

    @AssistedInject
    public SupportedAudioRoutesLiveData(
            @Assisted CallDetailLiveData primaryCallDetailLiveData,
            PhoneAccountManager phoneAccountManager,
            UiCallManager uiCallManager) {
        mPhoneAccountManager = phoneAccountManager;
        mUiCallManager = uiCallManager;

        mIsHfpConnection = false;
        addSource(primaryCallDetailLiveData, this::updateOngoingCallSupportedAudioRoutes);
    }

    private void updateOngoingCallSupportedAudioRoutes(CallDetail callDetail) {
        if (callDetail == null) {
            // Phone call might have disconnected, no action.
            return;
        }
        PhoneAccountHandle phoneAccountHandle = callDetail.getPhoneAccountHandle();
        boolean isHfpConnection = mPhoneAccountManager.isHfpConnectionService(phoneAccountHandle);
        // If it is the same type of phone account with a previous call, do nothing.
        if (getValue() != null && isHfpConnection == mIsHfpConnection) {
            return;
        }
        mIsHfpConnection = isHfpConnection;
        List<Integer> audioRoutes = mUiCallManager.getSupportedAudioRoute(phoneAccountHandle);
        setValue(audioRoutes);
    }

    /**
     * Factory to create {@link SupportedAudioRoutesLiveData} instances via the {@link
     * AssistedInject} constructor.
     */
    @AssistedFactory
    public interface Factory {
        /** Creates a {@link SupportedAudioRoutesLiveData} instance. */
        SupportedAudioRoutesLiveData create(CallDetailLiveData primaryCallDetailLiveData);
    }
}
