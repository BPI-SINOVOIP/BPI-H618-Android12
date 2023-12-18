/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package android.telecom.cts;

import static android.media.AudioAttributes.USAGE_NOTIFICATION_RINGTONE;
import static android.telecom.cts.TestUtils.COMPONENT;
import static android.telecom.cts.TestUtils.PACKAGE;
import static android.telecom.cts.TestUtils.WAIT_FOR_STATE_CHANGE_TIMEOUT_MS;
import static android.telephony.TelephonyManager.CALL_STATE_RINGING;

import android.content.ComponentName;
import android.media.AudioManager;
import android.media.AudioPlaybackConfiguration;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.telecom.Call;
import android.telecom.Connection;
import android.telecom.ConnectionRequest;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;
import android.telephony.PhoneStateListener;

import com.android.compatibility.common.util.ShellIdentityUtils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Tests valid/invalid incoming calls that are received from the ConnectionService
 * and registered through TelecomManager
 */
public class IncomingCallTest extends BaseTelecomTestWithMockServices {
    private static final long STATE_CHANGE_DELAY = 1000;

    private static final PhoneAccountHandle TEST_INVALID_HANDLE = new PhoneAccountHandle(
            new ComponentName(PACKAGE, COMPONENT), "WRONG_ID");

    public void testVerstatPassed() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(MockConnectionService.VERSTAT_PASSED_NUMBER, null);
        verifyConnectionForIncomingCall();

        Call call = mInCallCallbacks.getService().getLastCall();
        assertEquals(Connection.VERIFICATION_STATUS_PASSED,
                call.getDetails().getCallerNumberVerificationStatus());
    }

    public void testVerstatFailed() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(MockConnectionService.VERSTAT_FAILED_NUMBER, null);
        verifyConnectionForIncomingCall();

        Call call = mInCallCallbacks.getService().getLastCall();
        assertEquals(Connection.VERIFICATION_STATUS_FAILED,
                call.getDetails().getCallerNumberVerificationStatus());
    }

    public void testVerstatNotVerified() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(MockConnectionService.VERSTAT_NOT_VERIFIED_NUMBER, null);
        verifyConnectionForIncomingCall();

        Call call = mInCallCallbacks.getService().getLastCall();
        assertEquals(Connection.VERIFICATION_STATUS_NOT_VERIFIED,
                call.getDetails().getCallerNumberVerificationStatus());
    }

    public void testAddNewIncomingCall_CorrectPhoneAccountHandle() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(createTestNumber(), null);
        final Connection connection3 = verifyConnectionForIncomingCall();
        Collection<Connection> connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(1, connections.size());
        assertTrue(connections.contains(connection3));
    }

    public void testPhoneStateListenerInvokedOnIncomingCall() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        Uri testNumber = createTestNumber();
        addAndVerifyNewIncomingCall(testNumber, null);
        verifyConnectionForIncomingCall();
        verifyPhoneStateListenerCallbacksForCall(CALL_STATE_RINGING,
                testNumber.getSchemeSpecificPart());
        verifyCallStateListener(CALL_STATE_RINGING);
    }

    /**
     * This test verifies that when a default dialer is incapable of playing a ringtone that the
     * platform still plays a ringtone.
     * <p>
     * Given that the default {@link MockInCallService} defined in the CTS tests does not declare
     * {@link TelecomManager#METADATA_IN_CALL_SERVICE_RINGING}, we expect the Telecom framework to
     * play a ringtone for an incoming call.
     * @throws Exception
     */
    public void testRingOnIncomingCall() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        ShellIdentityUtils.invokeStaticMethodWithShellPermissions(
                (ShellIdentityUtils.StaticShellPermissionMethodHelper<Void>) () -> {
                    RingtoneManager.setActualDefaultRingtoneUri(mContext,
                            RingtoneManager.TYPE_RINGTONE,
                            Settings.System.DEFAULT_RINGTONE_URI);
                    return null;
                });
        LinkedBlockingQueue<Boolean> queue = new LinkedBlockingQueue(1);
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        AudioManager audioManager = mContext.getSystemService(AudioManager.class);
        AudioManager.AudioPlaybackCallback callback = new AudioManager.AudioPlaybackCallback() {
            @Override
            public void onPlaybackConfigChanged(List<AudioPlaybackConfiguration> configs) {
                super.onPlaybackConfigChanged(configs);
                boolean isPlayingRingtone = configs.stream()
                        .anyMatch(c -> c.getAudioAttributes().getUsage()
                                == USAGE_NOTIFICATION_RINGTONE);
                if (isPlayingRingtone && queue.isEmpty()) {
                    queue.add(isPlayingRingtone);
                }
            }
        };
        audioManager.registerAudioPlaybackCallback(callback, new Handler(Looper.getMainLooper()));
        Uri testNumber = createTestNumber();
        addAndVerifyNewIncomingCall(testNumber, null);
        verifyConnectionForIncomingCall();
        verifyPhoneStateListenerCallbacksForCall(CALL_STATE_RINGING,
                testNumber.getSchemeSpecificPart());
        Boolean ringing = queue.poll(WAIT_FOR_STATE_CHANGE_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        assertNotNull("Telecom should have played a ringtone, timed out waiting for state change",
                ringing);
        assertTrue("Telecom should have played a ringtone.", ringing);
        audioManager.unregisterAudioPlaybackCallback(callback);
    }

    /**
     * Tests to be sure that new incoming calls can only be added using a valid PhoneAccountHandle
     * (b/26864502). If a PhoneAccount has not been registered for the PhoneAccountHandle, then
     * a SecurityException will be thrown.
     */
    public void testAddNewIncomingCall_IncorrectPhoneAccountHandle() {
        if (!mShouldTestTelecom) {
            return;
        }

        Bundle extras = new Bundle();
        extras.putParcelable(TelecomManager.EXTRA_INCOMING_CALL_ADDRESS, createTestNumber());
        try {
            mTelecomManager.addNewIncomingCall(TEST_INVALID_HANDLE, extras);
            fail();
        } catch (SecurityException e) {
            // This should create a security exception!
        }

        assertFalse(CtsConnectionService.isServiceRegisteredToTelecom());
    }

    /**
     * Tests to be sure that new incoming calls can only be added if a PhoneAccount is enabled
     * (b/26864502). If a PhoneAccount is not enabled for the PhoneAccountHandle, then
     * a SecurityException will be thrown.
     */
    public void testAddNewIncomingCall_PhoneAccountNotEnabled() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }

        // Do not enable PhoneAccount
        setupConnectionService(null, FLAG_REGISTER);
        assertFalse(mTelecomManager.getPhoneAccount(TestUtils.TEST_PHONE_ACCOUNT_HANDLE)
                .isEnabled());
        try {
            addAndVerifyNewIncomingCall(createTestNumber(), null);
            fail();
        } catch (SecurityException e) {
            // This should create a security exception!
        }

        assertFalse(CtsConnectionService.isServiceRegisteredToTelecom());
    }

    /**
     * Ensure {@link Call.Details#PROPERTY_VOIP_AUDIO_MODE} is set for a ringing call which uses
     * voip audio mode.
     * @throws Exception
     */
    public void testAddNewIncomingCallVoipState() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(new MockConnectionService() {
            @Override
            public Connection onCreateIncomingConnection(
                    PhoneAccountHandle connectionManagerPhoneAccount,
                    ConnectionRequest request) {
                Connection connection = super.onCreateIncomingConnection(
                        connectionManagerPhoneAccount,
                        request);
                connection.setAudioModeIsVoip(true);
                lock.release();
                return connection;
            }
        }, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(createTestNumber(), null);
        verifyConnectionForIncomingCall();

        assertTrue((mInCallCallbacks.getService().getLastCall().getDetails().getCallProperties()
                & Call.Details.PROPERTY_VOIP_AUDIO_MODE) != 0);
    }

    /**
     * Ensure the {@link android.telephony.PhoneStateListener#onCallStateChanged(int, String)}
     * called in an expected way and phone state is correct.
     * @throws Exception
     */
    public void testPhoneStateChangeAsExpected() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        Uri testNumber = createTestNumber();
        addAndVerifyNewIncomingCall(testNumber, null);

        CountDownLatch count = new CountDownLatch(1);
        Executor executor = (Runnable command)->count.countDown();
        PhoneStateListener listener = new PhoneStateListener(executor);
        mTelephonyManager.listen(listener, PhoneStateListener.LISTEN_CALL_STATE);

        count.await(TestUtils.WAIT_FOR_PHONE_STATE_LISTENER_REGISTERED_TIMEOUT_S,
                TimeUnit.SECONDS);

        Thread.sleep(STATE_CHANGE_DELAY);
        assertEquals(CALL_STATE_RINGING, mTelephonyManager.getCallState());
    }

    /**
     * Verifies that a call to {@link android.telecom.Call#answer(int)} with a passed video state of
     * {@link android.telecom.VideoProfile#STATE_AUDIO_ONLY} will result in a call to
     * {@link Connection#onAnswer()}.
     * @throws Exception
     */
    public void testConnectionOnAnswerForAudioCall() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        // Get a new incoming call.
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(createTestNumber(), null);
        Call call = mInCallCallbacks.getService().getLastCall();
        final MockConnection connection = verifyConnectionForIncomingCall();
        TestUtils.InvokeCounter audioInvoke = connection.getInvokeCounter(
                MockConnection.ON_ANSWER_CALLED);

        // Answer as audio-only.
        call.answer(VideoProfile.STATE_AUDIO_ONLY);

        // Make sure we get a call to {@link Connection#onAnswer()}.
        audioInvoke.waitForCount(1, WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
    }

    /**
     * Verifies that a call to {@link android.telecom.Call#answer(int)} with a passed video state of
     * {@link android.telecom.VideoProfile#STATE_AUDIO_ONLY} will result in a call to
     * {@link Connection#onAnswer()} where overridden.
     * @throws Exception
     */
    public void testConnectionOnAnswerForVideoCallAnsweredAsAudio() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        // Get a new incoming call.
        Bundle extras = new Bundle();
        extras.putInt(TelecomManager.EXTRA_START_CALL_WITH_VIDEO_STATE,
                VideoProfile.STATE_BIDIRECTIONAL);
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(createTestNumber(), extras);
        Call call = mInCallCallbacks.getService().getLastCall();
        final MockConnection connection = verifyConnectionForIncomingCall();
        TestUtils.InvokeCounter audioInvoke = connection.getInvokeCounter(
                MockConnection.ON_ANSWER_CALLED);

        // Answer as audio-only.
        call.answer(VideoProfile.STATE_AUDIO_ONLY);

        // Make sure we get a call to {@link Connection#onAnswer()}.
        audioInvoke.waitForCount(1, WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
    }

    /**
     * Verifies that a call to {@link android.telecom.Call#answer(int)} with a passed video state of
     * {@link android.telecom.VideoProfile#STATE_BIDIRECTIONAL} will result in a call to
     * {@link Connection#onAnswer(int)}.
     * @throws Exception
     */
    public void testConnectionOnAnswerIntForVideoCallAnsweredAsVideo() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        // Get a new incoming call.
        Bundle extras = new Bundle();
        extras.putInt(TelecomManager.EXTRA_START_CALL_WITH_VIDEO_STATE,
                VideoProfile.STATE_BIDIRECTIONAL);
        setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        addAndVerifyNewIncomingCall(createTestNumber(), extras);
        Call call = mInCallCallbacks.getService().getLastCall();
        final MockConnection connection = verifyConnectionForIncomingCall();
        TestUtils.InvokeCounter audioInvoke = connection.getInvokeCounter(
                MockConnection.ON_ANSWER_VIDEO_CALLED);

        // Answer as audio-only.
        call.answer(VideoProfile.STATE_BIDIRECTIONAL);

        // Make sure we get a call to {@link Connection#onAnswer(int)}.
        audioInvoke.waitForCount(1, WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
    }
}
