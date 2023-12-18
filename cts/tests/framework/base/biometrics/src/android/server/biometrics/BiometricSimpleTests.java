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
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import android.content.pm.PackageManager;
import android.hardware.biometrics.BiometricManager;
import android.hardware.biometrics.BiometricManager.Authenticators;
import android.hardware.biometrics.BiometricPrompt;
import android.hardware.biometrics.BiometricTestSession;
import android.hardware.biometrics.SensorProperties;
import android.os.CancellationSignal;
import android.platform.test.annotations.Presubmit;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import com.android.server.biometrics.nano.SensorStateProto;

import org.junit.Test;

import java.util.Random;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Simple tests.
 */
@Presubmit
public class BiometricSimpleTests extends BiometricTestBase {
    private static final String TAG = "BiometricTests/Simple";

    /**
     * Tests that enrollments created via {@link BiometricTestSession} show up in the
     * biometric dumpsys.
     */
    @Test
    public void testEnroll() throws Exception {
        for (SensorProperties prop : mSensorProperties) {
            try (BiometricTestSession session =
                         mBiometricManager.createTestSession(prop.getSensorId())){
                enrollForSensor(session, prop.getSensorId());
            }
        }
    }

    /**
     * Tests that the sensorIds retrieved via {@link BiometricManager#getSensorProperties()} and
     * the dumpsys are consistent with each other.
     */
    @Test
    public void testSensorPropertiesAndDumpsysMatch() throws Exception {
        final BiometricServiceState state = getCurrentState();

        assertEquals(mSensorProperties.size(), state.mSensorStates.sensorStates.size());
        for (SensorProperties prop : mSensorProperties) {
            assertTrue(state.mSensorStates.sensorStates.containsKey(prop.getSensorId()));
        }
    }

    /**
     * Tests that the PackageManager features and biometric dumpsys are consistent with each other.
     */
    @Test
    public void testPackageManagerAndDumpsysMatch() throws Exception {
        final BiometricServiceState state = getCurrentState();
        if (mSensorProperties.isEmpty()) {
            assertTrue(state.mSensorStates.sensorStates.isEmpty());
        } else {
            final PackageManager pm = mContext.getPackageManager();

            assertEquals(pm.hasSystemFeature(PackageManager.FEATURE_FINGERPRINT),
                    state.mSensorStates.containsModality(SensorStateProto.FINGERPRINT));
            assertEquals(pm.hasSystemFeature(PackageManager.FEATURE_FACE),
                    state.mSensorStates.containsModality(SensorStateProto.FACE));
            assertEquals(pm.hasSystemFeature(PackageManager.FEATURE_IRIS),
                    state.mSensorStates.containsModality(SensorStateProto.IRIS));
        }
    }

    @Test
    public void testCanAuthenticate_whenNoSensors() {
        if (mSensorProperties.isEmpty()) {
            assertEquals(BiometricManager.BIOMETRIC_ERROR_NO_HARDWARE,
                    mBiometricManager.canAuthenticate(Authenticators.BIOMETRIC_WEAK));
            assertEquals(BiometricManager.BIOMETRIC_ERROR_NO_HARDWARE,
                    mBiometricManager.canAuthenticate(Authenticators.BIOMETRIC_STRONG));
        }
    }

    @Test
    public void testInvalidInputs() {
        for (int i = 0; i < 32; i++) {
            final int authenticator = 1 << i;
            // If it's a public constant, no need to test
            if (Utils.isPublicAuthenticatorConstant(authenticator)) {
                continue;
            }

            // Test canAuthenticate(int)
            assertThrows("Invalid authenticator in canAuthenticate must throw exception: "
                            + authenticator,
                    Exception.class,
                    () -> mBiometricManager.canAuthenticate(authenticator));

            // Test BiometricPrompt
            assertThrows("Invalid authenticator in authenticate must throw exception: "
                            + authenticator,
                    Exception.class,
                    () -> showBiometricPromptWithAuthenticators(authenticator));
        }
    }

    /**
     * When device credential is not enrolled, check the behavior for
     * 1) BiometricManager#canAuthenticate(DEVICE_CREDENTIAL)
     * 2) BiometricPrompt#setAllowedAuthenticators(DEVICE_CREDENTIAL)
     * 3) @deprecated BiometricPrompt#setDeviceCredentialAllowed(true)
     */
    @Test
    public void testWhenCredentialNotEnrolled() throws Exception {
        // First case above
        final int result = mBiometricManager.canAuthenticate(BiometricManager
                .Authenticators.DEVICE_CREDENTIAL);
        assertEquals(BiometricManager.BIOMETRIC_ERROR_NONE_ENROLLED, result);

        // Second case above
        BiometricPrompt.AuthenticationCallback callback =
                mock(BiometricPrompt.AuthenticationCallback.class);
        showCredentialOnlyBiometricPrompt(callback, new CancellationSignal(),
                false /* shouldShow */);
        verify(callback).onAuthenticationError(
                eq(BiometricPrompt.BIOMETRIC_ERROR_NO_DEVICE_CREDENTIAL),
                any());

        // Third case above. Since the deprecated API is intended to allow credential in addition
        // to biometrics, we should be receiving BIOMETRIC_ERROR_NO_BIOMETRICS.
        final boolean noSensors = mSensorProperties.isEmpty();
        callback = mock(BiometricPrompt.AuthenticationCallback.class);
        showDeviceCredentialAllowedBiometricPrompt(callback, new CancellationSignal(),
                false /* shouldShow */);
        verify(callback).onAuthenticationError(
                eq(noSensors ? BiometricPrompt.BIOMETRIC_ERROR_NO_DEVICE_CREDENTIAL
                        : BiometricPrompt.BIOMETRIC_ERROR_NO_BIOMETRICS),
                any());
    }

    /**
     * When device credential is enrolled, check the behavior for
     * 1) BiometricManager#canAuthenticate(DEVICE_CREDENTIAL)
     * 2a) Successfully authenticating BiometricPrompt#setAllowedAuthenticators(DEVICE_CREDENTIAL)
     * 2b) Cancelling authentication for the above
     * 3a) @deprecated BiometricPrompt#setDeviceCredentialALlowed(true)
     * 3b) Cancelling authentication for the above
     * 4) Cancelling auth for options 2) and 3)
     */
    @Test
    public void testWhenCredentialEnrolled() throws Exception {
        try (CredentialSession session = new CredentialSession()) {
            session.setCredential();

            // First case above
            final int result = mBiometricManager.canAuthenticate(BiometricManager
                    .Authenticators.DEVICE_CREDENTIAL);
            assertEquals(BiometricManager.BIOMETRIC_SUCCESS, result);

            // 2a above
            BiometricPrompt.AuthenticationCallback callback =
                    mock(BiometricPrompt.AuthenticationCallback.class);
            showCredentialOnlyBiometricPrompt(callback, new CancellationSignal(),
                    true /* shouldShow */);
            successfullyEnterCredential();
            verify(callback).onAuthenticationSucceeded(any());

            // 2b above
            CancellationSignal cancel = new CancellationSignal();
            callback = mock(BiometricPrompt.AuthenticationCallback.class);
            showCredentialOnlyBiometricPrompt(callback, cancel, true /* shouldShow */);
            cancelAuthentication(cancel);
            verify(callback).onAuthenticationError(eq(BiometricPrompt.BIOMETRIC_ERROR_CANCELED),
                    any());

            // 3a above
            callback = mock(BiometricPrompt.AuthenticationCallback.class);
            showDeviceCredentialAllowedBiometricPrompt(callback, new CancellationSignal(),
                    true /* shouldShow */);
            successfullyEnterCredential();
            verify(callback).onAuthenticationSucceeded(any());

            // 3b above
            cancel = new CancellationSignal();
            callback = mock(BiometricPrompt.AuthenticationCallback.class);
            showDeviceCredentialAllowedBiometricPrompt(callback, cancel, true /* shouldShow */);
            cancelAuthentication(cancel);
            verify(callback).onAuthenticationError(eq(BiometricPrompt.BIOMETRIC_ERROR_CANCELED),
                    any());
        }
    }

    /**
     * Tests that the values specified through the public APIs are shown on the BiometricPrompt UI
     * when biometric auth is requested.
     *
     * Upon successful authentication, checks that the result is
     * {@link BiometricPrompt#AUTHENTICATION_RESULT_TYPE_BIOMETRIC}
     */
    @Test
    public void testSimpleBiometricAuth() throws Exception {
        for (SensorProperties props : mSensorProperties) {

            Log.d(TAG, "testSimpleBiometricAuth, sensor: " + props.getSensorId());

            try (BiometricTestSession session =
                         mBiometricManager.createTestSession(props.getSensorId())) {

                final int authenticatorStrength =
                        Utils.testApiStrengthToAuthenticatorStrength(props.getSensorStrength());

                assertEquals("Sensor: " + props.getSensorId()
                                + ", strength: " + props.getSensorStrength(),
                        BiometricManager.BIOMETRIC_ERROR_NONE_ENROLLED,
                        mBiometricManager.canAuthenticate(authenticatorStrength));

                enrollForSensor(session, props.getSensorId());

                assertEquals("Sensor: " + props.getSensorId()
                                + ", strength: " + props.getSensorStrength(),
                        BiometricManager.BIOMETRIC_SUCCESS,
                        mBiometricManager.canAuthenticate(authenticatorStrength));

                final Random random = new Random();
                final String randomTitle = String.valueOf(random.nextInt(10000));
                final String randomSubtitle = String.valueOf(random.nextInt(10000));
                final String randomDescription = String.valueOf(random.nextInt(10000));
                final String randomNegativeButtonText = String.valueOf(random.nextInt(10000));

                CountDownLatch latch = new CountDownLatch(1);
                BiometricPrompt.AuthenticationCallback callback =
                        new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        assertEquals("Must be TYPE_BIOMETRIC",
                                BiometricPrompt.AUTHENTICATION_RESULT_TYPE_BIOMETRIC,
                                result.getAuthenticationType());
                        latch.countDown();
                    }
                };

                showDefaultBiometricPromptWithContents(props.getSensorId(), 0 /* userId */,
                        true /* requireConfirmation */, callback, randomTitle, randomSubtitle,
                        randomDescription, randomNegativeButtonText);

                final UiObject2 actualTitle = findView(TITLE_VIEW);
                final UiObject2 actualSubtitle = findView(SUBTITLE_VIEW);
                final UiObject2 actualDescription = findView(DESCRIPTION_VIEW);
                final UiObject2 actualNegativeButton = findView(BUTTON_ID_NEGATIVE);
                assertEquals(randomTitle, actualTitle.getText());
                assertEquals(randomSubtitle, actualSubtitle.getText());
                assertEquals(randomDescription, actualDescription.getText());
                assertEquals(randomNegativeButtonText, actualNegativeButton.getText());

                // Finish auth
                successfullyAuthenticate(session, 0 /* userId */);
                latch.await(3, TimeUnit.SECONDS);
            }
        }
    }

    /**
     * Tests that the values specified through the public APIs are shown on the BiometricPrompt UI
     * when credential auth is requested.
     *
     * Upon successful authentication, checks that the result is
     * {@link BiometricPrompt#AUTHENTICATION_RESULT_TYPE_BIOMETRIC}
     */
    @Test
    public void testSimpleCredentialAuth() throws Exception {
        try (CredentialSession session = new CredentialSession()){
            session.setCredential();

            final Random random = new Random();
            final String randomTitle = String.valueOf(random.nextInt(10000));
            final String randomSubtitle = String.valueOf(random.nextInt(10000));
            final String randomDescription = String.valueOf(random.nextInt(10000));

            CountDownLatch latch = new CountDownLatch(1);
            BiometricPrompt.AuthenticationCallback callback =
                    new BiometricPrompt.AuthenticationCallback() {
                @Override
                public void onAuthenticationSucceeded(
                        BiometricPrompt.AuthenticationResult result) {
                    assertEquals("Must be TYPE_CREDENTIAL",
                            BiometricPrompt.AUTHENTICATION_RESULT_TYPE_DEVICE_CREDENTIAL,
                            result.getAuthenticationType());
                    latch.countDown();
                }
            };
            showCredentialOnlyBiometricPromptWithContents(callback, new CancellationSignal(),
                    true /* shouldShow */, randomTitle, randomSubtitle, randomDescription);

            final UiObject2 actualTitle = findView(TITLE_VIEW);
            final UiObject2 actualSubtitle = findView(SUBTITLE_VIEW);
            final UiObject2 actualDescription = findView(DESCRIPTION_VIEW);
            assertEquals(randomTitle, actualTitle.getText());
            assertEquals(randomSubtitle, actualSubtitle.getText());
            assertEquals(randomDescription, actualDescription.getText());

            // Finish auth
            successfullyEnterCredential();
            latch.await(3, TimeUnit.SECONDS);
        }
    }

    /**
     * Tests that cancelling auth succeeds, and that ERROR_CANCELED is received.
     */
    @Test
    public void testBiometricCancellation() throws Exception {
        for (SensorProperties props : mSensorProperties) {
            try (BiometricTestSession session =
                         mBiometricManager.createTestSession(props.getSensorId())) {
                enrollForSensor(session, props.getSensorId());

                BiometricPrompt.AuthenticationCallback callback =
                        mock(BiometricPrompt.AuthenticationCallback.class);
                CancellationSignal cancellationSignal = new CancellationSignal();

                showDefaultBiometricPrompt(props.getSensorId(), 0 /* userId */,
                        true /* requireConfirmation */, callback, cancellationSignal);

                cancelAuthentication(cancellationSignal);
                verify(callback).onAuthenticationError(eq(BiometricPrompt.BIOMETRIC_ERROR_CANCELED),
                        any());
                verifyNoMoreInteractions(callback);
            }
        }
    }
}
