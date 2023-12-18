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

package android.server.biometrics.fingerprint;

import static android.server.biometrics.SensorStates.SensorState;
import static android.server.biometrics.SensorStates.UserState;
import static android.server.biometrics.fingerprint.Components.AUTH_ON_CREATE_ACTIVITY;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.app.Instrumentation;
import android.hardware.biometrics.BiometricTestSession;
import android.hardware.biometrics.SensorProperties;
import android.hardware.fingerprint.FingerprintManager;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.server.biometrics.BiometricServiceState;
import android.server.biometrics.SensorStates;
import android.server.biometrics.Utils;
import android.server.wm.ActivityManagerTestBase;
import android.server.wm.TestJournalProvider.TestJournal;
import android.server.wm.TestJournalProvider.TestJournalContainer;
import android.server.wm.UiDeviceUtils;
import android.server.wm.WindowManagerState;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.server.biometrics.nano.SensorServiceStateProto;
import com.android.server.biometrics.nano.SensorStateProto;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@SuppressWarnings("deprecation")
@Presubmit
public class FingerprintServiceTest extends ActivityManagerTestBase {
    private static final String TAG = "FingerprintServiceTest";

    private static final String DUMPSYS_FINGERPRINT = "dumpsys fingerprint --proto --state";

    private SensorStates getSensorStates() throws Exception {
        final byte[] dump = Utils.executeShellCommand(DUMPSYS_FINGERPRINT);
        SensorServiceStateProto proto = SensorServiceStateProto.parseFrom(dump);
        return SensorStates.parseFrom(proto);
    }

    @Nullable
    private static FingerprintCallbackHelper.State getCallbackState(@NonNull TestJournal journal) {
        Utils.waitFor("Waiting for authentication callback",
                () -> journal.extras.containsKey(FingerprintCallbackHelper.KEY));

        final Bundle bundle = journal.extras.getBundle(FingerprintCallbackHelper.KEY);
        if (bundle == null) {
            return null;
        }

        final FingerprintCallbackHelper.State state =
                FingerprintCallbackHelper.State.fromBundle(bundle);

        // Clear the extras since we want to wait for the journal to sync any new info the next
        // time it's read
        journal.extras.clear();

        return state;
    }

    @NonNull private Instrumentation mInstrumentation;
    @Nullable private FingerprintManager mFingerprintManager;
    @NonNull private List<SensorProperties> mSensorProperties;

    @Before
    public void setUp() throws Exception {
        mInstrumentation = getInstrumentation();
        mFingerprintManager = mInstrumentation.getContext()
                .getSystemService(FingerprintManager.class);

        // Tests can be skipped on devices without FingerprintManager
        assumeTrue(mFingerprintManager != null);

        mInstrumentation.getUiAutomation().adoptShellPermissionIdentity();

        mSensorProperties = mFingerprintManager.getSensorProperties();

        // Tests can be skipped on devices without fingerprint sensors
        assumeTrue(!mSensorProperties.isEmpty());
    }

    @After
    public void cleanup() throws Exception {
        if (mFingerprintManager == null || mSensorProperties.isEmpty()) {
            // The tests were skipped anyway, nothing to clean up. Maybe we can use JUnit test
            // annotations in the future.
            return;
        }


        mInstrumentation.waitForIdleSync();
        Utils.waitForIdleService(this::getSensorStates);

        final SensorStates sensorStates = getSensorStates();
        for (Map.Entry<Integer, SensorState> sensorEntry : sensorStates.sensorStates.entrySet()) {
            for (Map.Entry<Integer, UserState> userEntry
                    : sensorEntry.getValue().getUserStates().entrySet()) {
                if (userEntry.getValue().numEnrolled != 0) {
                    Log.w(TAG, "Cleaning up for sensor: " + sensorEntry.getKey()
                            + ", user: " + userEntry.getKey());
                    BiometricTestSession session =
                            mFingerprintManager.createTestSession(sensorEntry.getKey());
                    session.cleanupInternalState(userEntry.getKey());
                    session.close();
                }
            }
        }

        mInstrumentation.getUiAutomation().dropShellPermissionIdentity();
    }

    @Test
    public void testEnroll() throws Exception {
        for (SensorProperties prop : mSensorProperties) {
            try (BiometricTestSession session
                         = mFingerprintManager.createTestSession(prop.getSensorId())){
                testEnrollForSensor(session, prop.getSensorId());
            }
        }
    }

    private void testEnrollForSensor(BiometricTestSession session, int sensorId) throws Exception {
        final int userId = 0;

        session.startEnroll(userId);
        mInstrumentation.waitForIdleSync();
        Utils.waitForIdleService(this::getSensorStates);

        session.finishEnroll(userId);
        mInstrumentation.waitForIdleSync();
        Utils.waitForIdleService(this::getSensorStates);

        final SensorStates sensorStates = getSensorStates();

        // The (sensorId, userId) has one finger enrolled.
        assertEquals(1, sensorStates.sensorStates
                .get(sensorId).getUserStates().get(userId).numEnrolled);
    }

    @Test
    public void testAuthenticateFromForegroundActivity() throws Exception {
        // Turn screen on and dismiss keyguard
        UiDeviceUtils.pressWakeupButton();
        UiDeviceUtils.pressUnlockButton();

        // Manually keep track and close the sessions, since we want to enroll all sensors before
        // requesting auth.
        final List<BiometricTestSession> testSessions = new ArrayList<>();

        final int userId = 0;
        for (SensorProperties prop : mSensorProperties) {
            BiometricTestSession session =
                    mFingerprintManager.createTestSession(prop.getSensorId());
            testSessions.add(session);

            session.startEnroll(userId);
            mInstrumentation.waitForIdleSync();
            Utils.waitForIdleService(this::getSensorStates);

            session.finishEnroll(userId);
            mInstrumentation.waitForIdleSync();
            Utils.waitForIdleService(this::getSensorStates);
        }

        final TestJournal journal = TestJournalContainer.get(AUTH_ON_CREATE_ACTIVITY);

        // Launch test activity
        launchActivity(AUTH_ON_CREATE_ACTIVITY);
        mWmState.waitForActivityState(AUTH_ON_CREATE_ACTIVITY, WindowManagerState.STATE_RESUMED);
        mInstrumentation.waitForIdleSync();

        // At least one sensor should be authenticating
        assertFalse(getSensorStates().areAllSensorsIdle());

        // Nothing happened yet
        FingerprintCallbackHelper.State callbackState = getCallbackState(journal);
        assertNotNull(callbackState);
        assertEquals(0, callbackState.mNumAuthRejected);
        assertEquals(0, callbackState.mNumAuthAccepted);
        assertEquals(0, callbackState.mAcquiredReceived.size());
        assertEquals(0, callbackState.mErrorsReceived.size());

        // Auth and check again now
        testSessions.get(0).acceptAuthentication(userId);
        mInstrumentation.waitForIdleSync();
        callbackState = getCallbackState(journal);
        assertNotNull(callbackState);
        assertTrue(callbackState.mErrorsReceived.isEmpty());
        assertTrue(callbackState.mAcquiredReceived.isEmpty());
        assertEquals(1, callbackState.mNumAuthAccepted);
        assertEquals(0, callbackState.mNumAuthRejected);

        // Cleanup
        for (BiometricTestSession session : testSessions) {
            session.close();
        }
    }

    @Test
    public void testRejectThenErrorFromForegroundActivity() throws Exception {
        // Turn screen on and dismiss keyguard
        UiDeviceUtils.pressWakeupButton();
        UiDeviceUtils.pressUnlockButton();

        // Manually keep track and close the sessions, since we want to enroll all sensors before
        // requesting auth.
        final List<BiometricTestSession> testSessions = new ArrayList<>();

        final int userId = 0;
        for (SensorProperties prop : mSensorProperties) {
            BiometricTestSession session =
                    mFingerprintManager.createTestSession(prop.getSensorId());
            testSessions.add(session);

            session.startEnroll(userId);
            mInstrumentation.waitForIdleSync();
            Utils.waitForIdleService(this::getSensorStates);

            session.finishEnroll(userId);
            mInstrumentation.waitForIdleSync();
            Utils.waitForIdleService(this::getSensorStates);
        }

        final TestJournal journal = TestJournalContainer.get(AUTH_ON_CREATE_ACTIVITY);

        // Launch test activity
        launchActivity(AUTH_ON_CREATE_ACTIVITY);
        mWmState.waitForActivityState(AUTH_ON_CREATE_ACTIVITY, WindowManagerState.STATE_RESUMED);
        mInstrumentation.waitForIdleSync();
        FingerprintCallbackHelper.State callbackState = getCallbackState(journal);
        assertNotNull(callbackState);

        // Fingerprint rejected
        testSessions.get(0).rejectAuthentication(userId);
        mInstrumentation.waitForIdleSync();
        callbackState = getCallbackState(journal);
        assertNotNull(callbackState);
        assertEquals(1, callbackState.mNumAuthRejected);
        assertEquals(0, callbackState.mNumAuthAccepted);
        assertEquals(0, callbackState.mAcquiredReceived.size());
        assertEquals(0, callbackState.mErrorsReceived.size());

        // Send an acquire message
        // skip this check on devices with UDFPS because they prompt to try again
        // and do not dispatch an acquired event via BiometricPrompt
        final boolean verifyPartial = !hasUdfps();
        if (verifyPartial) {
            testSessions.get(0).notifyAcquired(userId,
                    FingerprintManager.FINGERPRINT_ACQUIRED_PARTIAL);
            mInstrumentation.waitForIdleSync();
            callbackState = getCallbackState(journal);
            assertNotNull(callbackState);
            assertEquals(1, callbackState.mNumAuthRejected);
            assertEquals(0, callbackState.mNumAuthAccepted);
            assertEquals(1, callbackState.mAcquiredReceived.size());
            assertEquals(FingerprintManager.FINGERPRINT_ACQUIRED_PARTIAL,
                    (int) callbackState.mAcquiredReceived.get(0));
            assertEquals(0, callbackState.mErrorsReceived.size());
        }

        // Send an error
        testSessions.get(0).notifyError(userId,
                FingerprintManager.FINGERPRINT_ERROR_CANCELED);
        mInstrumentation.waitForIdleSync();
        callbackState = getCallbackState(journal);
        assertNotNull(callbackState);
        assertEquals(1, callbackState.mNumAuthRejected);
        assertEquals(0, callbackState.mNumAuthAccepted);
        if (verifyPartial) {
            assertEquals(1, callbackState.mAcquiredReceived.size());
            assertEquals(FingerprintManager.FINGERPRINT_ACQUIRED_PARTIAL,
                    (int) callbackState.mAcquiredReceived.get(0));
        } else {
            assertEquals(0, callbackState.mAcquiredReceived.size());
        }
        assertEquals(1, callbackState.mErrorsReceived.size());
        assertEquals(FingerprintManager.FINGERPRINT_ERROR_CANCELED,
                (int) callbackState.mErrorsReceived.get(0));

        // Authentication lifecycle is done
        assertTrue(getSensorStates().areAllSensorsIdle());

        // Cleanup
        for (BiometricTestSession session : testSessions) {
            session.close();
        }
    }

    private boolean hasUdfps() throws Exception {
        final BiometricServiceState state = Utils.getBiometricServiceCurrentState();
        return state.mSensorStates.containsModalityFlag(SensorStateProto.FINGERPRINT_UDFPS);
    }
}