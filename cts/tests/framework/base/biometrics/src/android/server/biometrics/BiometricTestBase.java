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

package android.server.biometrics;

import static android.os.PowerManager.FULL_WAKE_LOCK;
import static android.server.biometrics.SensorStates.SensorState;
import static android.server.biometrics.SensorStates.UserState;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static com.android.server.biometrics.nano.BiometricServiceStateProto.STATE_AUTH_IDLE;
import static com.android.server.biometrics.nano.BiometricServiceStateProto.STATE_AUTH_PENDING_CONFIRM;
import static com.android.server.biometrics.nano.BiometricServiceStateProto.STATE_AUTH_STARTED_UI_SHOWING;
import static com.android.server.biometrics.nano.BiometricServiceStateProto.STATE_SHOWING_DEVICE_CREDENTIAL;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;
import static org.mockito.Mockito.mock;

import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.hardware.biometrics.BiometricManager;
import android.hardware.biometrics.BiometricManager.Authenticators;
import android.hardware.biometrics.BiometricPrompt;
import android.hardware.biometrics.BiometricTestSession;
import android.hardware.biometrics.SensorProperties;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.Handler;
import android.os.Looper;
import android.os.PowerManager;
import android.server.wm.ActivityManagerTestBase;
import android.server.wm.TestJournalProvider.TestJournal;
import android.server.wm.UiDeviceUtils;
import android.server.wm.WindowManagerState;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.server.biometrics.nano.BiometricServiceStateProto;

import org.junit.After;
import org.junit.Before;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executor;

/**
 * Base class containing useful functionality. Actual tests should be done in subclasses.
 */
abstract class BiometricTestBase extends ActivityManagerTestBase {

    private static final String TAG = "BiometricTestBase";
    private static final String DUMPSYS_BIOMETRIC = Utils.DUMPSYS_BIOMETRIC;
    private static final String FLAG_CLEAR_SCHEDULER_LOG = " --clear-scheduler-buffer";

    // Negative-side (left) buttons
    protected static final String BUTTON_ID_NEGATIVE = "button_negative";
    protected static final String BUTTON_ID_USE_CREDENTIAL = "button_use_credential";

    // Positive-side (right) buttons
    protected static final String BUTTON_ID_CONFIRM = "button_confirm";
    protected static final String BUTTON_ID_TRY_AGAIN = "button_try_again";

    // Biometric text contents
    protected static final String TITLE_VIEW = "title";
    protected static final String SUBTITLE_VIEW = "subtitle";
    protected static final String DESCRIPTION_VIEW = "description";

    protected static final String VIEW_ID_PASSWORD_FIELD = "lockPassword";

    @NonNull protected Instrumentation mInstrumentation;
    @NonNull protected BiometricManager mBiometricManager;
    @NonNull protected List<SensorProperties> mSensorProperties;
    @Nullable private PowerManager.WakeLock mWakeLock;
    @NonNull protected UiDevice mDevice;
    protected boolean mHasStrongBox;

    /**
     * Expose this functionality to our package, since ActivityManagerTestBase's is `protected`.
     * @param componentName
     */
    void launchActivity(@NonNull ComponentName componentName) {
        super.launchActivity(componentName);
    }

    /** @see Utils#getBiometricServiceCurrentState() */
    @NonNull
    protected BiometricServiceState getCurrentState() throws Exception {
        return Utils.getBiometricServiceCurrentState();
    }

    @NonNull
    protected BiometricServiceState getCurrentStateAndClearSchedulerLog() throws Exception {
        final byte[] dump = Utils.executeShellCommand(DUMPSYS_BIOMETRIC
                + FLAG_CLEAR_SCHEDULER_LOG);
        final BiometricServiceStateProto proto = BiometricServiceStateProto.parseFrom(dump);
        return BiometricServiceState.parseFrom(proto);
    }

    @Nullable
    protected UiObject2 findView(String id) {
        Log.d(TAG, "Finding view: " + id);
        return mDevice.findObject(By.res(mBiometricManager.getUiPackage(), id));
    }

    protected void findAndPressButton(String id) {
        final UiObject2 button = findView(id);
        assertNotNull(button);
        Log.d(TAG, "Clicking button: " + id);
        button.click();
    }

    protected SensorStates getSensorStates() throws Exception {
        return getCurrentState().mSensorStates;
    }

    protected void waitForState(@BiometricServiceState.AuthSessionState int state)
            throws Exception {
        for (int i = 0; i < 20; i++) {
            final BiometricServiceState serviceState = getCurrentState();
            if (serviceState.mState != state) {
                Log.d(TAG, "Not in state " + state + " yet, current: " + serviceState.mState);
                Thread.sleep(300);
            } else {
                return;
            }
        }
        Log.d(TAG, "Timed out waiting for state to become: " + state);
    }

    private void waitForStateNotEqual(@BiometricServiceState.AuthSessionState int state)
            throws Exception {
        for (int i = 0; i < 20; i++) {
            final BiometricServiceState serviceState = getCurrentState();
            if (serviceState.mState == state) {
                Log.d(TAG, "Not out of state yet, current: " + serviceState.mState);
                Thread.sleep(300);
            } else {
                return;
            }
        }
        Log.d(TAG, "Timed out waiting for state to not equal: " + state);
    }

    private boolean anyEnrollmentsExist() throws Exception {
        final BiometricServiceState serviceState = getCurrentState();

        for (SensorState sensorState : serviceState.mSensorStates.sensorStates.values()) {
            for (UserState userState : sensorState.getUserStates().values()) {
                if (userState.numEnrolled != 0) {
                    Log.d(TAG, "Enrollments still exist: " + serviceState);
                    return true;
                }
            }
        }
        return false;
    }

    protected void successfullyAuthenticate(@NonNull BiometricTestSession session, int userId)
            throws Exception {
        session.acceptAuthentication(userId);
        mInstrumentation.waitForIdleSync();
        waitForStateNotEqual(STATE_AUTH_STARTED_UI_SHOWING);
        BiometricServiceState state = getCurrentState();
        Log.d(TAG, "State after acceptAuthentication: " + state);
        if (state.mState == STATE_AUTH_PENDING_CONFIRM) {
            findAndPressButton(BUTTON_ID_CONFIRM);
            mInstrumentation.waitForIdleSync();
            waitForState(STATE_AUTH_IDLE);
        } else {
            waitForState(STATE_AUTH_IDLE);
        }

        assertEquals("Failed to become idle after authenticating",
                STATE_AUTH_IDLE, getCurrentState().mState);
    }

    protected void successfullyEnterCredential() throws Exception {
        waitForState(STATE_SHOWING_DEVICE_CREDENTIAL);
        BiometricServiceState state = getCurrentState();
        assertTrue(state.toString(), state.mSensorStates.areAllSensorsIdle());
        assertEquals(state.toString(), STATE_SHOWING_DEVICE_CREDENTIAL, state.mState);

        // Wait for any animations to complete. Ideally, this should be reflected in
        // STATE_SHOWING_DEVICE_CREDENTIAL, but SysUI and BiometricService are different processes
        // so we'd need to add some additional plumbing. We can improve this in the future.
        // TODO(b/152240892)
        Thread.sleep(1000);

        // Enter credential. AuthSession done, authentication callback received
        final UiObject2 passwordField = findView(VIEW_ID_PASSWORD_FIELD);
        Log.d(TAG, "Focusing, entering, submitting credential");
        passwordField.click();
        passwordField.setText(LOCK_CREDENTIAL);
        mDevice.pressEnter();
        waitForState(STATE_AUTH_IDLE);

        state = getCurrentState();
        assertEquals(state.toString(), STATE_AUTH_IDLE, state.mState);
    }

    protected void cancelAuthentication(@NonNull CancellationSignal cancel) throws Exception {
        cancel.cancel();
        mInstrumentation.waitForIdleSync();
        waitForState(STATE_AUTH_IDLE);

        //TODO(b/152240892): Currently BiometricService does not get a signal from SystemUI
        //  when the dialog finishes animating away.
        Thread.sleep(1000);

        BiometricServiceState state = getCurrentState();
        assertEquals("Not idle after requesting cancellation", state.mState, STATE_AUTH_IDLE);
    }

    protected void waitForAllUnenrolled() throws Exception {
        for (int i = 0; i < 20; i++) {
            if (anyEnrollmentsExist()) {
                Log.d(TAG, "Enrollments still exist..");
                Thread.sleep(300);
            } else {
                return;
            }
        }
        fail("Some sensors still have enrollments. State: " + getCurrentState());
    }

    /**
     * Shows a BiometricPrompt that specifies {@link Authenticators#DEVICE_CREDENTIAL}.
     */
    protected void showCredentialOnlyBiometricPrompt(
            @NonNull BiometricPrompt.AuthenticationCallback callback,
            @NonNull CancellationSignal cancellationSignal,
            boolean shouldShow) throws Exception {
        showCredentialOnlyBiometricPromptWithContents(callback, cancellationSignal, shouldShow,
                "Title", "Subtitle", "Description");
    }

    /**
     * Shows a BiometricPrompt that specifies {@link Authenticators#DEVICE_CREDENTIAL}
     * and the specified contents.
     */
    protected void showCredentialOnlyBiometricPromptWithContents(
            @NonNull BiometricPrompt.AuthenticationCallback callback,
            @NonNull CancellationSignal cancellationSignal, boolean shouldShow,
            @NonNull String title, @NonNull String subtitle,
            @NonNull String description) throws Exception {
        final Handler handler = new Handler(Looper.getMainLooper());
        final Executor executor = handler::post;
        final BiometricPrompt prompt = new BiometricPrompt.Builder(mContext)
                .setTitle(title)
                .setSubtitle(subtitle)
                .setDescription(description)
                .setAllowedAuthenticators(Authenticators.DEVICE_CREDENTIAL)
                .setAllowBackgroundAuthentication(true)
                .build();

        prompt.authenticate(cancellationSignal, executor, callback);
        mInstrumentation.waitForIdleSync();

        // Wait for any animations to complete. Ideally, this should be reflected in
        // STATE_SHOWING_DEVICE_CREDENTIAL, but SysUI and BiometricService are different processes
        // so we'd need to add some additional plumbing. We can improve this in the future.
        // TODO(b/152240892)
        Thread.sleep(1000);

        if (shouldShow) {
            waitForState(STATE_SHOWING_DEVICE_CREDENTIAL);
            BiometricServiceState state = getCurrentState();
            assertEquals(state.toString(), STATE_SHOWING_DEVICE_CREDENTIAL, state.mState);
        } else {
            Utils.waitForIdleService(this::getSensorStates);
        }
    }

    /**
     * SHows a BiometricPrompt that sets
     * {@link BiometricPrompt.Builder#setDeviceCredentialAllowed(boolean)} to true.
     */
    protected void showDeviceCredentialAllowedBiometricPrompt(
            @NonNull BiometricPrompt.AuthenticationCallback callback,
            @NonNull CancellationSignal cancellationSignal,
            boolean shouldShow) throws Exception {
        final Handler handler = new Handler(Looper.getMainLooper());
        final Executor executor = handler::post;
        final BiometricPrompt prompt = new BiometricPrompt.Builder(mContext)
                .setTitle("Title")
                .setSubtitle("Subtitle")
                .setDescription("Description")
                .setDeviceCredentialAllowed(true)
                .setAllowBackgroundAuthentication(true)
                .build();

        prompt.authenticate(cancellationSignal, executor, callback);
        mInstrumentation.waitForIdleSync();

        // Wait for any animations to complete. Ideally, this should be reflected in
        // STATE_SHOWING_DEVICE_CREDENTIAL, but SysUI and BiometricService are different processes
        // so we'd need to add some additional plumbing. We can improve this in the future.
        // TODO(b/152240892)
        Thread.sleep(1000);

        if (shouldShow) {
            waitForState(STATE_SHOWING_DEVICE_CREDENTIAL);
            BiometricServiceState state = getCurrentState();
            assertEquals(state.toString(), STATE_SHOWING_DEVICE_CREDENTIAL, state.mState);
        } else {
            Utils.waitForIdleService(this::getSensorStates);
        }
    }

    protected void showDefaultBiometricPrompt(int sensorId, int userId,
            boolean requireConfirmation, @NonNull BiometricPrompt.AuthenticationCallback callback,
            @NonNull CancellationSignal cancellationSignal) throws Exception {
        final Handler handler = new Handler(Looper.getMainLooper());
        final Executor executor = handler::post;
        final BiometricPrompt prompt = new BiometricPrompt.Builder(mContext)
                .setTitle("Title")
                .setSubtitle("Subtitle")
                .setDescription("Description")
                .setConfirmationRequired(requireConfirmation)
                .setNegativeButton("Negative Button", executor, (dialog, which) -> {
                    Log.d(TAG, "Negative button pressed");
                })
                .setAllowBackgroundAuthentication(true)
                .setAllowedSensorIds(new ArrayList<>(Collections.singletonList(sensorId)))
                .build();
        prompt.authenticate(cancellationSignal, executor, callback);

        waitForState(STATE_AUTH_STARTED_UI_SHOWING);
    }

    /**
     * Shows the default BiometricPrompt (sensors meeting BIOMETRIC_WEAK) with a negative button,
     * but does not complete authentication. In other words, the dialog will stay on the screen.
     */
    protected void showDefaultBiometricPromptWithContents(int sensorId, int userId,
            boolean requireConfirmation, @NonNull BiometricPrompt.AuthenticationCallback callback,
            @NonNull String title, @NonNull String subtitle, @NonNull String description,
            @NonNull String negativeButtonText) throws Exception {
        final Handler handler = new Handler(Looper.getMainLooper());
        final Executor executor = handler::post;
        final BiometricPrompt prompt = new BiometricPrompt.Builder(mContext)
                .setTitle(title)
                .setSubtitle(subtitle)
                .setDescription(description)
                .setConfirmationRequired(requireConfirmation)
                .setNegativeButton(negativeButtonText, executor, (dialog, which) -> {
                    Log.d(TAG, "Negative button pressed");
                })
                .setAllowBackgroundAuthentication(true)
                .setAllowedSensorIds(new ArrayList<>(Collections.singletonList(sensorId)))
                .build();
        prompt.authenticate(new CancellationSignal(), executor,
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        Log.d(TAG, "onAuthenticationError: " + errorCode);
                    }

                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        Log.d(TAG, "onAuthenticationSucceeded");
                    }
                });

        waitForState(STATE_AUTH_STARTED_UI_SHOWING);
    }

    /**
     * Shows the default BiometricPrompt (sensors meeting BIOMETRIC_WEAK) with a negative button,
     * and fakes successful authentication via TestApis.
     */
    protected void showDefaultBiometricPromptAndAuth(@NonNull BiometricTestSession session,
            int sensorId, int userId) throws Exception {
        BiometricPrompt.AuthenticationCallback callback = mock(
                BiometricPrompt.AuthenticationCallback.class);
        showDefaultBiometricPromptWithContents(sensorId, userId, false /* requireConfirmation */,
                callback, "Title", "Subtitle", "Description", "Negative Button");
        successfullyAuthenticate(session, userId);
    }

    protected void showBiometricPromptWithAuthenticators(int authenticators) {
        final Handler handler = new Handler(Looper.getMainLooper());
        final Executor executor = handler::post;
        final BiometricPrompt prompt = new BiometricPrompt.Builder(mContext)
                .setTitle("Title")
                .setSubtitle("Subtitle")
                .setDescription("Description")
                .setNegativeButton("Negative Button", executor, (dialog, which) -> {
                    Log.d(TAG, "Negative button pressed");
                })
                .setAllowBackgroundAuthentication(true)
                .setAllowedAuthenticators(authenticators)
                .build();
        prompt.authenticate(new CancellationSignal(), executor,
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        Log.d(TAG, "onAuthenticationError: " + errorCode);
                    }

                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        Log.d(TAG, "onAuthenticationSucceeded");
                    }
                });
    }

    protected void launchActivityAndWaitForResumed(@NonNull ActivitySession activitySession) {
        activitySession.start();
        mWmState.waitForActivityState(activitySession.getComponentName(),
                WindowManagerState.STATE_RESUMED);
        mInstrumentation.waitForIdleSync();
    }

    protected void closeActivity(@NonNull ActivitySession activitySession) throws Exception {
        activitySession.close();
        mInstrumentation.waitForIdleSync();
    }

    protected int getCurrentStrength(int sensorId) throws Exception {
        final BiometricServiceState serviceState = getCurrentState();
        return serviceState.mSensorStates.sensorStates.get(sensorId).getCurrentStrength();
    }

    protected List<Integer> getSensorsOfTargetStrength(int targetStrength) {
        final List<Integer> sensors = new ArrayList<>();
        for (SensorProperties prop : mSensorProperties) {
            if (prop.getSensorStrength() == targetStrength) {
                sensors.add(prop.getSensorId());
            }
        }
        Log.d(TAG, "getSensorsOfTargetStrength: num of target sensors=" + sensors.size());
        return sensors;
    }

    @NonNull
    protected static BiometricCallbackHelper.State getCallbackState(@NonNull TestJournal journal) {
        Utils.waitFor("Waiting for authentication callback",
                () -> journal.extras.containsKey(BiometricCallbackHelper.KEY),
                (lastResult) -> fail("authentication callback never received - died waiting"));

        final Bundle bundle = journal.extras.getBundle(BiometricCallbackHelper.KEY);
        final BiometricCallbackHelper.State state =
                BiometricCallbackHelper.State.fromBundle(bundle);

        // Clear the extras since we want to wait for the journal to sync any new info the next
        // time it's read
        journal.extras.clear();

        return state;
    }

    @Before
    public void setUp() throws Exception {
        mInstrumentation = getInstrumentation();
        mBiometricManager = mInstrumentation.getContext().getSystemService(BiometricManager.class);

        mInstrumentation.getUiAutomation().adoptShellPermissionIdentity();
        mDevice = UiDevice.getInstance(mInstrumentation);
        mSensorProperties = mBiometricManager.getSensorProperties();

        assumeTrue(mInstrumentation.getContext().getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_SECURE_LOCK_SCREEN));

        mHasStrongBox = mContext.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_STRONGBOX_KEYSTORE);

        // Keep the screen on for the duration of each test, since BiometricPrompt goes away
        // when screen turns off.
        final PowerManager pm = mInstrumentation.getContext().getSystemService(PowerManager.class);
        mWakeLock = pm.newWakeLock(FULL_WAKE_LOCK, TAG);
        mWakeLock.acquire();

        // Turn screen on and dismiss keyguard
        UiDeviceUtils.pressWakeupButton();
        UiDeviceUtils.pressUnlockButton();
    }

    @After
    public void cleanup() {
        mInstrumentation.waitForIdleSync();

        try {
            Utils.waitForIdleService(this::getSensorStates);
        } catch (Exception e) {
            Log.e(TAG, "Exception when waiting for idle", e);
        }

        try {
            final BiometricServiceState state = getCurrentState();

            for (Map.Entry<Integer, SensorState> sensorEntry
                    : state.mSensorStates.sensorStates.entrySet()) {
                for (Map.Entry<Integer, UserState> userEntry
                        : sensorEntry.getValue().getUserStates().entrySet()) {
                    if (userEntry.getValue().numEnrolled != 0) {
                        Log.w(TAG, "Cleaning up for sensor: " + sensorEntry.getKey()
                                + ", user: " + userEntry.getKey());
                        BiometricTestSession session = mBiometricManager.createTestSession(
                                sensorEntry.getKey());
                        session.cleanupInternalState(userEntry.getKey());
                        session.close();
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Unable to get current state in cleanup()");
        }

        // Authentication lifecycle is done
        try {
            Utils.waitForIdleService(this::getSensorStates);
        } catch (Exception e) {
            Log.e(TAG, "Exception when waiting for idle", e);
        }

        if (mWakeLock != null) {
            mWakeLock.release();
        }
        mInstrumentation.getUiAutomation().dropShellPermissionIdentity();
    }

    protected void enrollForSensor(@NonNull BiometricTestSession session, int sensorId)
            throws Exception {
        Log.d(TAG, "Enrolling for sensor: " + sensorId);
        final int userId = 0;

        session.startEnroll(userId);
        mInstrumentation.waitForIdleSync();
        Utils.waitForBusySensor(sensorId, this::getSensorStates);

        session.finishEnroll(userId);
        mInstrumentation.waitForIdleSync();
        Utils.waitForIdleService(this::getSensorStates);

        final BiometricServiceState state = getCurrentState();
        assertEquals("Sensor: " + sensorId + " should have exactly one enrollment",
                1, state.mSensorStates.sensorStates
                .get(sensorId).getUserStates().get(userId).numEnrolled);
    }
}
