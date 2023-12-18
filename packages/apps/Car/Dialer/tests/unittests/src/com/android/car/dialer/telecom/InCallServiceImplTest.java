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

import static android.app.Activity.RESULT_OK;

import static androidx.test.espresso.intent.Intents.intending;
import static androidx.test.espresso.intent.matcher.IntentMatchers.anyIntent;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Instrumentation;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.IBinder;
import android.service.notification.StatusBarNotification;
import android.telecom.Call;
import android.telecom.CallAudioState;
import android.telecom.GatewayInfo;

import androidx.preference.PreferenceManager;
import androidx.test.espresso.intent.Intents;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ServiceTestRule;

import com.android.car.arch.common.LiveDataFunctions;
import com.android.car.dialer.bluetooth.PhoneAccountManager;
import com.android.car.dialer.notification.InCallNotificationController;
import com.android.car.dialer.ui.activecall.InCallActivity;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.concurrent.TimeoutException;
/**
 * Tests for {@link InCallServiceImpl}.
 */
@RunWith(AndroidJUnit4.class)
public class InCallServiceImplTest {
    private static final String TELECOM_CALL_ID = "+12345678900";

    @Rule
    public final ServiceTestRule serviceRule = new ServiceTestRule();

    private InCallServiceImpl mInCallServiceImpl;
    private Context mContext;
    private InCallNotificationController mInCallNotificationController;

    @Mock
    private Call mMockTelecomCall;
    @Mock
    private Call.Details mMockCallDetails;
    @Mock
    private CallAudioState mMockCallAudioState;
    @Mock
    private InCallServiceImpl.CallAudioStateCallback mCallAudioStateCallback;
    @Mock
    private InCallServiceImpl.ActiveCallListChangedCallback mActiveCallListChangedCallback;
    @Mock
    private PhoneAccountManager mPhoneAccountManager;
    @Mock
    private ProjectionCallHandler mProjectionCallHandler;

    @Before
    public void setUp() throws TimeoutException {
        mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();

        MockitoAnnotations.initMocks(this);

        // Create the service Intent.
        Intent serviceIntent = new Intent(mContext, InCallServiceImpl.class);
        serviceIntent.setAction(InCallServiceImpl.ACTION_LOCAL_BIND);

        // Bind the service and grab a reference to the binder.
        IBinder binder = serviceRule.bindService(serviceIntent);

        mInCallServiceImpl =
                ((InCallServiceImpl.LocalBinder) binder).getService();
        mInCallServiceImpl.mPhoneAccountManager = mPhoneAccountManager;
        mInCallNotificationController = new InCallNotificationController(mContext);
        mInCallServiceImpl.mInCallRouter = new InCallRouter(mContext,
                PreferenceManager.getDefaultSharedPreferences(mContext),
                mInCallNotificationController, mProjectionCallHandler);
        mInCallServiceImpl.mCurrentHfpDeviceLiveData = LiveDataFunctions.nullLiveData();

        mInCallServiceImpl.addActiveCallListChangedCallback(mActiveCallListChangedCallback);

        Uri uri = Uri.fromParts("tel", TELECOM_CALL_ID, null);
        GatewayInfo gatewayInfo = new GatewayInfo("", uri, uri);
        when(mMockTelecomCall.getDetails()).thenReturn(mMockCallDetails);
        when(mMockCallDetails.getHandle()).thenReturn(uri);
        when(mMockCallDetails.getGatewayInfo()).thenReturn(gatewayInfo);

        Intents.init();
        intending(anyIntent()).respondWith(new Instrumentation.ActivityResult(RESULT_OK, null));
    }

    @After
    public void tearDown() {
        Intents.release();
    }

    @Test
    public void onActiveCallAdded_startInCallActivity() {
        when(mMockTelecomCall.getState()).thenReturn(Call.STATE_ACTIVE);
        mInCallServiceImpl.onCallAdded(mMockTelecomCall);

        ArgumentCaptor<Call> callCaptor = ArgumentCaptor.forClass(Call.class);

        verify(mActiveCallListChangedCallback).onTelecomCallAdded(callCaptor.capture());
        assertThat(callCaptor.getValue()).isEqualTo(mMockTelecomCall);

        Intent intent = Intents.getIntents().get(0);
        assertThat(intent).isNotNull();
    }

    @Test
    public void onCallRemoved() {
        when(mMockTelecomCall.getState()).thenReturn(Call.STATE_ACTIVE);
        mInCallServiceImpl.onCallRemoved(mMockTelecomCall);

        ArgumentCaptor<Call> callCaptor = ArgumentCaptor.forClass(Call.class);

        verify(mActiveCallListChangedCallback).onTelecomCallRemoved(callCaptor.capture());
        assertThat(callCaptor.getValue()).isEqualTo(mMockTelecomCall);
    }

    @Test
    public void onRingingCallAdded_showNotification() {
        when(mMockTelecomCall.getState()).thenReturn(Call.STATE_RINGING);
        mInCallServiceImpl.onCallAdded(mMockTelecomCall);

        ArgumentCaptor<Call> callCaptor = ArgumentCaptor.forClass(Call.class);

        verify(mActiveCallListChangedCallback).onTelecomCallAdded(callCaptor.capture());
        assertThat(callCaptor.getValue()).isEqualTo(mMockTelecomCall);

        ArgumentCaptor<Call.Callback> callbackListCaptor = ArgumentCaptor.forClass(
                Call.Callback.class);
        verify(mMockTelecomCall).registerCallback(callbackListCaptor.capture());

        NotificationManager notificationManager = (NotificationManager) mContext.getSystemService(
                Context.NOTIFICATION_SERVICE);
        StatusBarNotification statusBarNotification =
                notificationManager.getActiveNotifications()[0];
        assertThat(statusBarNotification.getTag()).isEqualTo(TELECOM_CALL_ID);
        assertThat(statusBarNotification.getNotification()).isNotNull();
        Notification notification = statusBarNotification.getNotification();
        assertThat(notification.actions.length).isEqualTo(2);

        // Dismiss the notification to tear down properly
        mInCallNotificationController.cancelInCallNotification(mMockTelecomCall);
    }

    @Test
    public void testRemoveActiveCallListChangedCallback() {
        mInCallServiceImpl.removeActiveCallListChangedCallback(mActiveCallListChangedCallback);

        mInCallServiceImpl.onCallAdded(mMockTelecomCall);
        verify(mActiveCallListChangedCallback, never()).onTelecomCallAdded(any());

        mInCallServiceImpl.onCallRemoved(mMockTelecomCall);
        verify(mActiveCallListChangedCallback, never()).onTelecomCallRemoved(any());
    }

    @Test
    public void testAddCallAudioStateChangedCallback() {
        mInCallServiceImpl.addCallAudioStateChangedCallback(mCallAudioStateCallback);

        mInCallServiceImpl.onCallAudioStateChanged(mMockCallAudioState);
        verify(mCallAudioStateCallback).onCallAudioStateChanged(any());
    }

    @Test
    public void testOnBringToForeground() {
        mInCallServiceImpl.onCallAdded(mMockTelecomCall);
        mInCallServiceImpl.onBringToForeground(false);

        Intent intent = Intents.getIntents().get(0);
        assertThat(intent.getComponent().getClassName()).isEqualTo(InCallActivity.class.getName());
    }
}
