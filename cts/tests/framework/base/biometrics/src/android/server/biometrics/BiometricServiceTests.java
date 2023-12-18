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

package android.server.biometrics;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.hardware.biometrics.BiometricTestSession;
import android.hardware.biometrics.SensorProperties;
import android.platform.test.annotations.Presubmit;
import android.util.Log;

import androidx.annotation.NonNull;

import com.android.server.biometrics.nano.BiometricsProto;

import org.junit.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Tests for system server logic.
 */
@Presubmit
public class BiometricServiceTests extends BiometricTestBase {
    private static final String TAG = "BiometricTests/Service";

    @Test
    public void testAuthenticatorIdsInvalidated() throws Exception {
        // On devices with multiple strong sensors, adding enrollments to one strong sensor
        // must cause authenticatorIds for all other strong sensors to be invalidated, if they
        // (the other strong sensors) have enrollments.
        final List<Integer> strongSensors = new ArrayList<>();
        for (SensorProperties prop : mSensorProperties) {
            if (prop.getSensorStrength() == SensorProperties.STRENGTH_STRONG) {
                strongSensors.add(prop.getSensorId());
            }
        }
        assumeTrue("numStrongSensors: " + strongSensors.size(), strongSensors.size() >= 2);

        Log.d(TAG, "testAuthenticatorIdsInvalidated, numStrongSensors: " + strongSensors.size());

        for (Integer sensorId : strongSensors) {
            testAuthenticatorIdsInvalidated_forSensor(sensorId, strongSensors);
        }
    }

    /**
     * Tests that the specified sensorId's authenticatorId when any other strong sensor adds
     * an enrollment.
     */
    private void testAuthenticatorIdsInvalidated_forSensor(int sensorId,
            @NonNull List<Integer> strongSensors) throws Exception {
        Log.d(TAG, "testAuthenticatorIdsInvalidated_forSensor: " + sensorId);
        final List<BiometricTestSession> biometricSessions = new ArrayList<>();

        final BiometricTestSession targetSensorTestSession =
                mBiometricManager.createTestSession(sensorId);

        // Get the state once. This intentionally clears the scheduler's recent operations dump.
        BiometricServiceState state = getCurrentStateAndClearSchedulerLog();

        waitForAllUnenrolled();
        Log.d(TAG, "Enrolling for: " + sensorId);
        enrollForSensor(targetSensorTestSession, sensorId);
        biometricSessions.add(targetSensorTestSession);
        state = getCurrentStateAndClearSchedulerLog();

        // Target sensorId has never been requested to invalidate authenticatorId yet.
        assertEquals(0, Utils.numberOfSpecifiedOperations(state, sensorId,
                BiometricsProto.CM_INVALIDATE));

        // Add enrollments for all other sensors. Upon each enrollment, the authenticatorId for
        // the above sensor should be invalidated.
        for (Integer id : strongSensors) {
            if (id != sensorId) {
                final BiometricTestSession session = mBiometricManager.createTestSession(id);
                biometricSessions.add(session);
                Log.d(TAG, "Sensor " + id + " should request invalidation");
                enrollForSensor(session, id);
                state = getCurrentStateAndClearSchedulerLog();
                assertEquals(1, Utils.numberOfSpecifiedOperations(state, sensorId,
                        BiometricsProto.CM_INVALIDATE));

                // In addition, the sensor that should have enrolled should have been the one that
                // requested invalidation.
                assertEquals(1, Utils.numberOfSpecifiedOperations(state, id,
                        BiometricsProto.CM_INVALIDATION_REQUESTER));
            }
        }

        // Cleanup
        for (BiometricTestSession session : biometricSessions) {
            session.close();
        }
    }

    @Test
    public void testLockoutResetRequestedAfterCredentialUnlock() throws Exception {
        // ResetLockout only really needs to be applied when enrollments exist. Furthermore, some
        // interfaces may take this a step further and ignore resetLockout requests when no
        // enrollments exist.
        List<BiometricTestSession> biometricSessions = new ArrayList<>();
        for (SensorProperties prop : mSensorProperties) {
            BiometricTestSession session = mBiometricManager.createTestSession(prop.getSensorId());
            enrollForSensor(session, prop.getSensorId());
            biometricSessions.add(session);
        }

        try (CredentialSession credentialSession = new CredentialSession()) {
            credentialSession.setCredential();

            // Explicitly clear the state so we can check exact number below
            final BiometricServiceState clearState = getCurrentStateAndClearSchedulerLog();
            credentialSession.verifyCredential();

            Utils.waitFor("Waiting for password verification and resetLockout completion", () -> {
                try {
                    BiometricServiceState state = getCurrentState();
                    // All sensors have processed exactly one resetLockout request. Use a boolean
                    // to track this so we have better logging
                    boolean allResetOnce = true;
                    for (SensorProperties prop : mSensorProperties) {
                        final int numResetLockouts = Utils.numberOfSpecifiedOperations(state,
                                prop.getSensorId(), BiometricsProto.CM_RESET_LOCKOUT);
                        Log.d(TAG, "Sensor: " + prop.getSensorId()
                                + ", numResetLockouts: " + numResetLockouts);
                        if (numResetLockouts != 1) {
                            allResetOnce = false;
                        }
                    }
                    return allResetOnce;
                } catch (Exception e) {
                    return false;
                }
            }, unused -> fail("All sensors must receive and process exactly one resetLockout"));
        }

        for (BiometricTestSession session : biometricSessions) {
            session.close();
        }
    }

    @Test
    public void testLockoutResetRequestedAfterBiometricUnlock_whenStrong() throws Exception {
        assumeTrue(mSensorProperties.size() > 1);

        // ResetLockout only really needs to be applied when enrollments exist. Furthermore, some
        // interfaces may take this a step further and ignore resetLockout requests when no
        // enrollments exist.
        Map<Integer, BiometricTestSession> biometricSessions = new HashMap<>();
        for (SensorProperties prop : mSensorProperties) {
            BiometricTestSession session = mBiometricManager.createTestSession(prop.getSensorId());
            enrollForSensor(session, prop.getSensorId());
            biometricSessions.put(prop.getSensorId(), session);
        }

        // When a strong biometric sensor authenticates, all other biometric sensors that:
        //  1) Do not require HATs for resetLockout (e.g. IBiometricsFingerprint@2.1) or
        //  2) Require HATs but do not require challenges (e.g. IFingerprint@1.0, IFace@1.0)
        // schedule and complete a resetLockout operation.
        //
        // To be more explicit, sensors that require HATs AND challenges (IBiometricsFace@1.0)
        // do not schedule resetLockout, since the interface has no way of generating multiple
        // HATs with a single authentication (e.g. if the user requested to unlock an auth-bound
        // key, the only HAT returned would have the keystore operationId within).
        for (SensorProperties prop : mSensorProperties) {
            if (prop.getSensorStrength() != SensorProperties.STRENGTH_STRONG) {
                Log.d(TAG, "Skipping sensor: " + prop.getSensorId()
                        + ", strength: " + prop.getSensorStrength());
                continue;
            }
            testLockoutResetRequestedAfterBiometricUnlock_whenStrong_forSensor(
                    prop.getSensorId(), biometricSessions.get(prop.getSensorId()));
        }

        for (BiometricTestSession session : biometricSessions.values()) {
            session.close();
        }
    }

    private void testLockoutResetRequestedAfterBiometricUnlock_whenStrong_forSensor(int sensorId,
            @NonNull BiometricTestSession session)
            throws Exception {
        Log.d(TAG, "testLockoutResetRequestedAfterBiometricUnlock_whenStrong_forSensor: "
                + sensorId);
        final int userId = 0;

        BiometricServiceState state = getCurrentState();
        final List<Integer> eligibleSensorsToReset = new ArrayList<>();
        final List<Integer> ineligibleSensorsToReset = new ArrayList<>();
        for (SensorProperties prop : mSensorProperties) {
            if (prop.getSensorId() == sensorId) {
                // Do not need to resetLockout for self
                continue;
            }

            SensorStates.SensorState sensorState = state.mSensorStates.sensorStates
                    .get(prop.getSensorId());
            final boolean supportsChallengelessHat =
                    sensorState.isResetLockoutRequiresHardwareAuthToken()
                            && !sensorState.isResetLockoutRequiresChallenge();
            final boolean doesNotRequireHat =
                    !sensorState.isResetLockoutRequiresHardwareAuthToken();
            Log.d(TAG, "SensorId: " + prop.getSensorId()
                    + ", supportsChallengelessHat: " + supportsChallengelessHat
                    + ", doesNotRequireHat: " + doesNotRequireHat);
            if (supportsChallengelessHat || doesNotRequireHat) {
                Log.d(TAG, "Adding eligible sensor: " + prop.getSensorId());
                eligibleSensorsToReset.add(prop.getSensorId());
            } else {
                Log.d(TAG, "Adding ineligible sensor: " + prop.getSensorId());
                ineligibleSensorsToReset.add(prop.getSensorId());
            }
        }

        // Explicitly clear the log so that we can check the exact number of resetLockout operations
        // below.
        state = getCurrentStateAndClearSchedulerLog();

        // Request authentication with the specified sensorId that was passed in
        showDefaultBiometricPromptAndAuth(session, sensorId, userId);

        // Check that all eligible sensors have resetLockout in their scheduler history
        state = getCurrentState();
        for (Integer id : eligibleSensorsToReset) {
            assertEquals("Sensor: " + id + " should have exactly one resetLockout", 1,
                    Utils.numberOfSpecifiedOperations(state, id, BiometricsProto.CM_RESET_LOCKOUT));
        }

        // Check that all ineligible sensors do not have resetLockout in their scheduler history
        for (Integer id : ineligibleSensorsToReset) {
            assertEquals("Sensor: " + id + " should have no resetLockout", 0,
                    Utils.numberOfSpecifiedOperations(state, id, BiometricsProto.CM_RESET_LOCKOUT));
        }
    }

    @Test
    public void testLockoutResetNotRequestedAfterBiometricUnlock_whenNotStrong() throws Exception {
        assumeTrue(mSensorProperties.size() > 1);

        // ResetLockout only really needs to be applied when enrollments exist. Furthermore, some
        // interfaces may take this a step further and ignore resetLockout requests when no
        // enrollments exist.
        Map<Integer, BiometricTestSession> biometricSessions = new HashMap<>();
        for (SensorProperties prop : mSensorProperties) {
            BiometricTestSession session = mBiometricManager.createTestSession(prop.getSensorId());
            enrollForSensor(session, prop.getSensorId());
            biometricSessions.put(prop.getSensorId(), session);
        }

        // Sensors that do not meet BIOMETRIC_STRONG are not allowed to resetLockout for other
        // sensors.
        // TODO: Note that we are only testing STRENGTH_WEAK for now, since STRENGTH_CONVENIENCE is
        //  not exposed to BiometricPrompt. In other words, we currently do not have a way to
        //  request and finish authentication for STRENGTH_CONVENIENCE sensors.
        for (SensorProperties prop : mSensorProperties) {
            if (prop.getSensorStrength() != SensorProperties.STRENGTH_WEAK) {
                Log.d(TAG, "Skipping sensor: " + prop.getSensorId()
                        + ", strength: " + prop.getSensorStrength());
                continue;
            }

            testLockoutResetNotRequestedAfterBiometricUnlock_whenNotStrong_forSensor(
                    prop.getSensorId(), biometricSessions.get(prop.getSensorId()));
        }

        // Cleanup
        for (BiometricTestSession s : biometricSessions.values()) {
            s.close();
        }
    }

    private void testLockoutResetNotRequestedAfterBiometricUnlock_whenNotStrong_forSensor(
            int sensorId, @NonNull BiometricTestSession session) throws Exception {
        Log.d(TAG, "testLockoutResetNotRequestedAfterBiometricUnlock_whenNotStrong_forSensor: "
                + sensorId);
        final int userId = 0;

        // Explicitly clear the log so that we can check the exact number of resetLockout operations
        // below.
        BiometricServiceState state = getCurrentStateAndClearSchedulerLog();

        // Request authentication with the specified sensorId that was passed in
        showDefaultBiometricPromptAndAuth(session, sensorId, userId);

        // Check that no other sensors have resetLockout in their queue
        for (SensorProperties prop : mSensorProperties) {
            if (prop.getSensorId() == sensorId) {
                continue;
            }
            state = getCurrentState();
            assertEquals("Sensor: " + prop.getSensorId() + " should have no resetLockout", 0,
                    Utils.numberOfSpecifiedOperations(state, prop.getSensorId(),
                            BiometricsProto.CM_RESET_LOCKOUT));
        }
    }

    @Test
    public void testBiometricsRemovedWhenCredentialRemoved() throws Exception {
        // Manually keep track of sessions and do not use autocloseable, since we do not want the
        // test session to automatically cleanup and remove enrollments once we leave scope.
        final List<BiometricTestSession> biometricSessions = new ArrayList<>();

        try (CredentialSession session = new CredentialSession()) {
            session.setCredential();
            for (SensorProperties prop : mSensorProperties) {
                BiometricTestSession biometricSession =
                        mBiometricManager.createTestSession(prop.getSensorId());
                biometricSessions.add(biometricSession);
                enrollForSensor(biometricSession, prop.getSensorId());
            }
        }

        // All biometrics should now be removed, since CredentialSession removes device credential
        // after losing scope.
        waitForAllUnenrolled();
        // In case any additional cleanup needs to be done in the future, aside from un-enrollment
        for (BiometricTestSession session : biometricSessions) {
            session.close();
        }
    }
}
