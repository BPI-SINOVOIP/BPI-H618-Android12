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

package com.android.car.dialer.telecom;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNotNull;
import static org.mockito.Matchers.isNull;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothDevice;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.telecom.CallAudioState;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.bluetooth.PhoneAccountManager;
import com.android.car.telephony.common.CallDetail;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.stubbing.Answer;

import java.util.Collections;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class UiCallManagerTest {

    private static final String TEL_SCHEME = "tel";

    private UiCallManager mUiCallManager;

    @Mock
    private PhoneAccountManager mMockPhoneAccountManager;
    @Mock
    private BluetoothDevice mMockBluetoothDevice;
    @Mock
    private TelecomManager mMockTelecomManager;
    @Mock
    private Context mMockContext;
    @Mock
    private InCallServiceImpl.LocalBinder mMockIBinder;
    @Mock
    private InCallServiceImpl mMockInCallService;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        when(mMockIBinder.getService()).thenReturn(mMockInCallService);
        doAnswer((Answer<Void>) invocation -> {
            ServiceConnection serviceConnection = invocation.getArgument(1);
            serviceConnection.onServiceConnected(
                    new ComponentName(mMockContext, InCallServiceImpl.class), mMockIBinder);
            return null;
        }).when(mMockContext).bindService(any(Intent.class), any(ServiceConnection.class),
                anyInt());

        when(mMockPhoneAccountManager.getMatchingDevice(isNull())).thenReturn(null);
        when(mMockPhoneAccountManager.getMatchingDevice(isNotNull())).thenReturn(
                mMockBluetoothDevice);

        mUiCallManager = new UiCallManager(mMockContext, mMockTelecomManager,
                mMockPhoneAccountManager);
    }

    @Test
    public void testPlaceCall() {
        String[] phoneNumbers = {
                "6505551234", // US Number
                "511", // Special number
                "911", // Emergency number
                "122", // Emergency number
                "#77" // Emergency number
        };

        for (int i = 0; i < phoneNumbers.length; i++) {
            checkPlaceCall(phoneNumbers[i], i + 1);
        }
    }

    private void checkPlaceCall(String phoneNumber, int timesCalled) {
        ArgumentCaptor<Uri> uriCaptor = ArgumentCaptor.forClass(Uri.class);

        assertThat(mUiCallManager.placeCall(phoneNumber)).isTrue();
        verify(mMockTelecomManager, times(timesCalled)).placeCall(uriCaptor.capture(), isNull());
        assertThat(uriCaptor.getValue().getScheme()).isEqualTo(TEL_SCHEME);
        assertThat(uriCaptor.getValue().getSchemeSpecificPart()).isEqualTo(phoneNumber);
        assertThat(uriCaptor.getValue().getFragment()).isNull();
    }

    @Test
    public void testGetMuted_isMuted() {
        CallAudioState callAudioState = new CallAudioState(true,
                CallAudioState.ROUTE_BLUETOOTH, CallAudioState.ROUTE_ALL);
        when(mMockInCallService.getCallAudioState()).thenReturn(callAudioState);

        assertThat(mUiCallManager.getMuted()).isTrue();
    }

    @Test
    public void testGetMuted_audioRouteIsNull() {
        when(mMockInCallService.getCallAudioState()).thenReturn(null);

        assertThat(mUiCallManager.getMuted()).isFalse();
    }

    @Test
    public void testGetMuted_InCallServiceIsNull() {
        when(mMockIBinder.getService()).thenReturn(null);
        mUiCallManager = new UiCallManager(mMockContext, mMockTelecomManager,
                mMockPhoneAccountManager);

        assertThat(mUiCallManager.getMuted()).isFalse();
    }

    @Test
    public void testSetMuted() {
        mUiCallManager.setMuted(true);

        ArgumentCaptor<Boolean> captor = ArgumentCaptor.forClass(Boolean.class);
        verify(mMockInCallService).setMuted(captor.capture());
        assertThat(captor.getValue()).isTrue();
    }

    @Test
    public void testGetSupportedAudioRouteMask() {
        CallAudioState callAudioState = new CallAudioState(
                true, CallAudioState.ROUTE_BLUETOOTH, CallAudioState.ROUTE_ALL);
        when(mMockInCallService.getCallAudioState()).thenReturn(callAudioState);

        assertThat(mUiCallManager.getSupportedAudioRouteMask()).isEqualTo(CallAudioState
                .ROUTE_ALL);
    }

    @Test
    public void testGetSupportedAudioRouteMask_InCallServiceIsNull() {
        when(mMockIBinder.getService()).thenReturn(null);
        mUiCallManager = new UiCallManager(mMockContext, mMockTelecomManager,
                mMockPhoneAccountManager);

        assertThat(mUiCallManager.getSupportedAudioRouteMask()).isEqualTo(0);
    }

    @Test
    public void testGetSupportedAudioRoute_supportedAudioRouteMaskIs0() {
        // SupportedAudioRouteMask is 0.
        assertThat(mUiCallManager.getSupportedAudioRoute(null).size()).isEqualTo(0);
    }

    @Test
    public void testGetSupportedAudioRoute_supportedAudioRouteMaskIsRouteAll() {
        // SupportedAudioRouteMask is CallAudioState.ROUTE_ALL.
        CallAudioState callAudioState = new CallAudioState(
                true, CallAudioState.ROUTE_BLUETOOTH, CallAudioState.ROUTE_ALL);
        when(mMockInCallService.getCallAudioState()).thenReturn(callAudioState);

        List<Integer> audioRoutes = mUiCallManager.getSupportedAudioRoute(null);
        assertThat(audioRoutes.size()).isEqualTo(3);
        assertThat(audioRoutes.contains(CallAudioState.ROUTE_EARPIECE)).isTrue();
        assertThat(audioRoutes.contains(CallAudioState.ROUTE_SPEAKER)).isTrue();
        assertThat(audioRoutes.contains(CallAudioState.ROUTE_BLUETOOTH)).isTrue();
    }

    @Test
    public void testGetSupportedAudioRoute_supportedAudioRouteMaskIsRouteSpeaker() {
        // SupportedAudioRouteMask is CallAudioState.ROUTE_SPEAKER.
        CallAudioState callAudioState = new CallAudioState(
                true, CallAudioState.ROUTE_SPEAKER, CallAudioState.ROUTE_SPEAKER);
        when(mMockInCallService.getCallAudioState()).thenReturn(callAudioState);

        List<Integer> audioRoutes = mUiCallManager.getSupportedAudioRoute(null);
        assertThat(audioRoutes.size()).isEqualTo(1);
        assertThat(audioRoutes.get(0)).isEqualTo(CallAudioState.ROUTE_SPEAKER);
    }

    @Test
    public void bluetoothCall_getSupportedAudioRoute() {
        PhoneAccountHandle mockPhoneAccountHandle = mock(PhoneAccountHandle.class);

        List<Integer> supportedAudioRoute = mUiCallManager.getSupportedAudioRoute(
                mockPhoneAccountHandle);
        assertThat(supportedAudioRoute.get(0)).isEqualTo(CallAudioState.ROUTE_BLUETOOTH);
        assertThat(supportedAudioRoute.get(1)).isEqualTo(CallAudioState.ROUTE_EARPIECE);
    }

    @Test
    public void noPhoneAccounts_emergencyCallNotSupported() {
        when(mMockTelecomManager.getCallCapablePhoneAccounts()).thenReturn(Collections.EMPTY_LIST);

        assertThat(mUiCallManager.isEmergencyCallSupported()).isFalse();
    }

    @Test
    public void hasPhoneAccountWithEmergencyCallCapability_emergencyCallSupported() {
        PhoneAccountHandle mockPhoneAccountHandle = mock(PhoneAccountHandle.class);
        PhoneAccount mockPhoneAccount = mock(PhoneAccount.class);

        when(mMockTelecomManager.getCallCapablePhoneAccounts()).thenReturn(
                Collections.singletonList(mockPhoneAccountHandle));
        when(mMockTelecomManager.getPhoneAccount(eq(mockPhoneAccountHandle))).thenReturn(
                mockPhoneAccount);
        when(mockPhoneAccount.hasCapabilities(
                eq(PhoneAccount.CAPABILITY_PLACE_EMERGENCY_CALLS))).thenReturn(true);

        assertThat(mUiCallManager.isEmergencyCallSupported()).isTrue();
    }

    @Test
    public void scoStateToAudioRoute() {
        CallAudioState callAudioState = new CallAudioState(
                true, CallAudioState.ROUTE_SPEAKER, CallAudioState.ROUTE_ALL);
        when(mMockInCallService.getCallAudioState()).thenReturn(callAudioState);

        assertThat(mUiCallManager.getAudioRoute(CallDetail.STATE_AUDIO_CONNECTED)).isEqualTo(
                CallAudioState.ROUTE_BLUETOOTH);
        assertThat(mUiCallManager.getAudioRoute(CallDetail.STATE_AUDIO_CONNECTING)).isEqualTo(
                CallAudioState.ROUTE_EARPIECE);
        assertThat(mUiCallManager.getAudioRoute(CallDetail.STATE_AUDIO_DISCONNECTED)).isEqualTo(
                CallAudioState.ROUTE_EARPIECE);
        assertThat(mUiCallManager.getAudioRoute(CallDetail.STATE_AUDIO_ERROR)).isEqualTo(
                CallAudioState.ROUTE_SPEAKER);
    }

    @After
    public void tearDown() {
        mUiCallManager.tearDown();
    }
}
