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

package android.os.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

import android.os.CombinedVibration;
import android.os.SystemClock;
import android.os.VibrationAttributes;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.Vibrator.OnVibratorStateChangedListener;
import android.os.VibratorManager;
import android.util.SparseArray;

import androidx.test.filters.LargeTest;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.AdoptShellPermissionsRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
public class VibratorManagerTest {
    @Rule
    public ActivityTestRule<SimpleTestActivity> mActivityRule = new ActivityTestRule<>(
            SimpleTestActivity.class);

    @Rule
    public final AdoptShellPermissionsRule mAdoptShellPermissionsRule =
            new AdoptShellPermissionsRule(
                    InstrumentationRegistry.getInstrumentation().getUiAutomation(),
                    android.Manifest.permission.ACCESS_VIBRATOR_STATE);
    
    @Rule
    public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    private static final long CALLBACK_TIMEOUT_MILLIS = 5000;
    private static final VibrationAttributes VIBRATION_ATTRIBUTES =
            new VibrationAttributes.Builder()
                    .setUsage(VibrationAttributes.USAGE_TOUCH)
                    .build();

    private VibratorManager mVibratorManager;
    private final SparseArray<OnVibratorStateChangedListener> mStateListeners = new SparseArray<>();

    @Before
    public void setUp() {
        mVibratorManager =
                InstrumentationRegistry.getInstrumentation().getContext().getSystemService(
                        VibratorManager.class);

        for (int vibratorId : mVibratorManager.getVibratorIds()) {
            OnVibratorStateChangedListener listener = mock(OnVibratorStateChangedListener.class);
            mVibratorManager.getVibrator(vibratorId).addVibratorStateListener(listener);
            mStateListeners.put(vibratorId, listener);
            reset(listener);
        }
    }

    @After
    public void cleanUp() {
        mVibratorManager.cancel();
    }

    @Test
    public void testCancel() {
        mVibratorManager.vibrate(CombinedVibration.createParallel(
                VibrationEffect.createOneShot(10_000, VibrationEffect.DEFAULT_AMPLITUDE)));
        assertStartsVibrating();

        mVibratorManager.cancel();
        assertStopsVibrating();
    }

    @LargeTest
    @Test
    public void testVibrateOneShot() {
        VibrationEffect oneShot =
                VibrationEffect.createOneShot(300, VibrationEffect.DEFAULT_AMPLITUDE);
        mVibratorManager.vibrate(CombinedVibration.createParallel(oneShot));
        assertStartsThenStopsVibrating(300);

        oneShot = VibrationEffect.createOneShot(500, 255 /* Max amplitude */);
        mVibratorManager.vibrate(CombinedVibration.createParallel(oneShot));
        assertStartsVibrating();

        mVibratorManager.cancel();
        assertStopsVibrating();

        oneShot = VibrationEffect.createOneShot(100, 1 /* Min amplitude */);
        mVibratorManager.vibrate(CombinedVibration.createParallel(oneShot),
                VIBRATION_ATTRIBUTES);
        assertStartsVibrating();
    }

    @LargeTest
    @Test
    public void testVibrateWaveform() {
        final long[] timings = new long[]{100, 200, 300, 400, 500};
        final int[] amplitudes = new int[]{64, 128, 255, 128, 64};
        VibrationEffect waveform = VibrationEffect.createWaveform(timings, amplitudes, -1);
        mVibratorManager.vibrate(CombinedVibration.createParallel(waveform));
        assertStartsThenStopsVibrating(1500);

        waveform = VibrationEffect.createWaveform(timings, amplitudes, 0);
        mVibratorManager.vibrate(CombinedVibration.createParallel(waveform));
        assertStartsVibrating();

        mVibratorManager.cancel();
        assertStopsVibrating();
    }

    @Test
    public void testVibrateSingleVibrator() {
        int[] vibratorIds = mVibratorManager.getVibratorIds();
        if (vibratorIds.length < 2) {
            return;
        }

        VibrationEffect oneShot =
                VibrationEffect.createOneShot(10_000, VibrationEffect.DEFAULT_AMPLITUDE);

        for (int vibratorId : vibratorIds) {
            Vibrator vibrator = mVibratorManager.getVibrator(vibratorId);
            mVibratorManager.vibrate(
                    CombinedVibration.startParallel()
                            .addVibrator(vibratorId, oneShot)
                            .combine());
            assertStartsVibrating(vibratorId);

            for (int otherVibratorId : vibratorIds) {
                if (otherVibratorId != vibratorId) {
                    assertFalse(mVibratorManager.getVibrator(otherVibratorId).isVibrating());
                }
            }

            vibrator.cancel();
            assertStopsVibrating(vibratorId);
        }
    }

    @Test
    public void testGetVibratorIds() {
        // Just make sure it doesn't crash or return null when this is called; we don't really have
        // a way to test which vibrators will be returned.
        assertNotNull(mVibratorManager.getVibratorIds());
    }

    @Test
    public void testGetNonExistentVibratorId() {
        int missingId = Arrays.stream(mVibratorManager.getVibratorIds()).max().orElse(0) + 1;
        Vibrator vibrator = mVibratorManager.getVibrator(missingId);
        assertNotNull(vibrator);
        assertFalse(vibrator.hasVibrator());
    }

    @Test
    public void testGetDefaultVibrator() {
        Vibrator systemVibrator =
                InstrumentationRegistry.getInstrumentation().getContext().getSystemService(
                        Vibrator.class);
        assertSame(systemVibrator, mVibratorManager.getDefaultVibrator());
    }

    @Test
    public void testVibrator() {
        for (int vibratorId : mVibratorManager.getVibratorIds()) {
            Vibrator vibrator = mVibratorManager.getVibrator(vibratorId);
            assertNotNull(vibrator);
            assertEquals(vibratorId, vibrator.getId());
            assertTrue(vibrator.hasVibrator());

            // Just check these methods will not crash.
            // We don't really have a way to test if the device supports each effect or not.
            vibrator.hasAmplitudeControl();

            // Just check these methods return valid support arrays.
            // We don't really have a way to test if the device supports each effect or not.
            assertEquals(2, vibrator.areEffectsSupported(
                    VibrationEffect.EFFECT_TICK, VibrationEffect.EFFECT_CLICK).length);
            assertEquals(2, vibrator.arePrimitivesSupported(
                    VibrationEffect.Composition.PRIMITIVE_CLICK,
                    VibrationEffect.Composition.PRIMITIVE_TICK).length);

            vibrator.vibrate(VibrationEffect.createOneShot(500, VibrationEffect.DEFAULT_AMPLITUDE));
            assertStartsVibrating(vibratorId);
            assertTrue(vibrator.isVibrating());

            vibrator.cancel();
            assertStopsVibrating(vibratorId);
        }
    }

    private void assertStartsThenStopsVibrating(long duration) {
        for (int i = 0; i < mStateListeners.size(); i++) {
            verify(mStateListeners.valueAt(i), timeout(CALLBACK_TIMEOUT_MILLIS).atLeastOnce())
                    .onVibratorStateChanged(true);
        }
        SystemClock.sleep(duration);
        assertVibratorState(false);
    }

    private void assertStartsVibrating() {
        assertVibratorState(true);
    }

    private void assertStartsVibrating(int vibratorId) {
        assertVibratorState(vibratorId, true);
    }

    private void assertStopsVibrating() {
        assertVibratorState(false);
    }

    private void assertStopsVibrating(int vibratorId) {
        assertVibratorState(vibratorId, false);
    }

    private void assertVibratorState(boolean expected) {
        for (int i = 0; i < mStateListeners.size(); i++) {
            assertVibratorState(mStateListeners.keyAt(i), expected);
        }
    }

    private void assertVibratorState(int vibratorId, boolean expected) {
        OnVibratorStateChangedListener listener = mStateListeners.get(vibratorId);
        verify(listener, timeout(CALLBACK_TIMEOUT_MILLIS).atLeastOnce())
                .onVibratorStateChanged(eq(expected));
        reset(listener);
    }
}
