/*
 * Copyright (C) 2008 The Android Open Source Project
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
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

import android.media.AudioAttributes;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.Vibrator.OnVibratorStateChangedListener;

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
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.util.concurrent.Executors;

@RunWith(AndroidJUnit4.class)
public class VibratorTest {
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

    private static final AudioAttributes AUDIO_ATTRIBUTES =
            new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build();
    private static final long CALLBACK_TIMEOUT_MILLIS = 5000;
    private static final int[] PREDEFINED_EFFECTS = new int[]{
            VibrationEffect.EFFECT_CLICK,
            VibrationEffect.EFFECT_DOUBLE_CLICK,
            VibrationEffect.EFFECT_TICK,
            VibrationEffect.EFFECT_THUD,
            VibrationEffect.EFFECT_POP,
            VibrationEffect.EFFECT_HEAVY_CLICK,
            VibrationEffect.EFFECT_TEXTURE_TICK,
    };
    private static final int[] PRIMITIVE_EFFECTS = new int[]{
            VibrationEffect.Composition.PRIMITIVE_CLICK,
            VibrationEffect.Composition.PRIMITIVE_TICK,
            VibrationEffect.Composition.PRIMITIVE_LOW_TICK,
            VibrationEffect.Composition.PRIMITIVE_QUICK_RISE,
            VibrationEffect.Composition.PRIMITIVE_QUICK_FALL,
            VibrationEffect.Composition.PRIMITIVE_SLOW_RISE,
            VibrationEffect.Composition.PRIMITIVE_SPIN,
            VibrationEffect.Composition.PRIMITIVE_THUD,
    };

    private Vibrator mVibrator;
    @Mock
    private OnVibratorStateChangedListener mStateListener;

    @Before
    public void setUp() {
        mVibrator = InstrumentationRegistry.getInstrumentation().getContext().getSystemService(
                Vibrator.class);

        mVibrator.addVibratorStateListener(mStateListener);
        reset(mStateListener);
    }

    @After
    public void cleanUp() {
        mVibrator.cancel();
    }

    @Test
    public void testVibratorCancel() {
        mVibrator.vibrate(10_000);
        assertStartsVibrating();

        mVibrator.cancel();
        assertStopsVibrating();
    }

    @Test
    public void testVibratePattern() {
        long[] pattern = {100, 200, 400, 800, 1600};
        mVibrator.vibrate(pattern, 3);
        assertStartsVibrating();

        try {
            mVibrator.vibrate(pattern, 10);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException expected) {
        }
    }

    @Test
    public void testVibrateMultiThread() {
        new Thread(() -> {
            try {
                mVibrator.vibrate(500);
            } catch (Exception e) {
                fail("MultiThread fail1");
            }
        }).start();
        new Thread(() -> {
            try {
                // This test only get two threads to run vibrator at the same time for a functional
                // test, but it can not verify if the second thread get the precedence.
                mVibrator.vibrate(1000);
            } catch (Exception e) {
                fail("MultiThread fail2");
            }
        }).start();
        assertStartsVibrating();
    }

    @LargeTest
    @Test
    public void testVibrateOneShot() {
        VibrationEffect oneShot =
                VibrationEffect.createOneShot(300, VibrationEffect.DEFAULT_AMPLITUDE);
        mVibrator.vibrate(oneShot);
        assertStartsThenStopsVibrating(300);

        oneShot = VibrationEffect.createOneShot(10_000, 255 /* Max amplitude */);
        mVibrator.vibrate(oneShot);
        assertStartsVibrating();

        mVibrator.cancel();
        assertStopsVibrating();

        oneShot = VibrationEffect.createOneShot(300, 1 /* Min amplitude */);
        mVibrator.vibrate(oneShot, AUDIO_ATTRIBUTES);
        assertStartsVibrating();
    }

    @LargeTest
    @Test
    public void testVibrateWaveform() {
        final long[] timings = new long[] {100, 200, 300, 400, 500};
        final int[] amplitudes = new int[] {64, 128, 255, 128, 64};
        VibrationEffect waveform = VibrationEffect.createWaveform(timings, amplitudes, -1);
        mVibrator.vibrate(waveform);
        assertStartsThenStopsVibrating(1500);

        waveform = VibrationEffect.createWaveform(timings, amplitudes, 0);
        mVibrator.vibrate(waveform, AUDIO_ATTRIBUTES);
        assertStartsVibrating();

        SystemClock.sleep(2000);
        assertTrue(!mVibrator.hasVibrator() || mVibrator.isVibrating());

        mVibrator.cancel();
        assertStopsVibrating();
    }

    @Test
    public void testVibratePredefined() {
        int[] supported = mVibrator.areEffectsSupported(PREDEFINED_EFFECTS);
        for (int i = 0; i < PREDEFINED_EFFECTS.length; i++) {
            mVibrator.vibrate(VibrationEffect.createPredefined(PREDEFINED_EFFECTS[i]));
            if (supported[i] == Vibrator.VIBRATION_EFFECT_SUPPORT_YES) {
                assertStartsVibrating();
            }
        }
    }

    @Test
    public void testVibrateComposed() {
        boolean[] supported = mVibrator.arePrimitivesSupported(PRIMITIVE_EFFECTS);
        for (int i = 0; i < PRIMITIVE_EFFECTS.length; i++) {
            mVibrator.vibrate(VibrationEffect.startComposition()
                    .addPrimitive(PRIMITIVE_EFFECTS[i])
                    .addPrimitive(PRIMITIVE_EFFECTS[i], 0.5f)
                    .addPrimitive(PRIMITIVE_EFFECTS[i], 0.8f, 10)
                    .compose());
            if (supported[i]) {
                assertStartsVibrating();
            }
        }
    }

    @Test
    public void testGetId() {
        // The system vibrator should not be mapped to any physical vibrator and use a default id.
        assertEquals(-1, mVibrator.getId());
    }

    @Test
    public void testHasVibrator() {
        // Just make sure it doesn't crash when this is called; we don't really have a way to test
        // if the device has vibrator or not.
        mVibrator.hasVibrator();
    }

    @Test
    public void testVibratorHasAmplitudeControl() {
        // Just make sure it doesn't crash when this is called; we don't really have a way to test
        // if the amplitude control works or not.
        mVibrator.hasAmplitudeControl();
    }

    @Test
    public void testVibratorEffectsAreSupported() {
        // Just make sure it doesn't crash when this is called and that it returns all queries;
        // We don't really have a way to test if the device supports each effect or not.
        assertEquals(PREDEFINED_EFFECTS.length,
                mVibrator.areEffectsSupported(PREDEFINED_EFFECTS).length);
        assertEquals(0, mVibrator.areEffectsSupported().length);
    }

    @Test
    public void testVibratorAllEffectsAreSupported() {
        // Just make sure it doesn't crash when this is called;
        // We don't really have a way to test if the device supports each effect or not.
        mVibrator.areAllEffectsSupported(PREDEFINED_EFFECTS);
        assertEquals(Vibrator.VIBRATION_EFFECT_SUPPORT_YES, mVibrator.areAllEffectsSupported());
    }

    @Test
    public void testVibratorPrimitivesAreSupported() {
        // Just make sure it doesn't crash when this is called;
        // We don't really have a way to test if the device supports each effect or not.
        assertEquals(PRIMITIVE_EFFECTS.length,
                mVibrator.arePrimitivesSupported(PRIMITIVE_EFFECTS).length);
        assertEquals(0, mVibrator.arePrimitivesSupported().length);
    }

    @Test
    public void testVibratorAllPrimitivesAreSupported() {
        // Just make sure it doesn't crash when this is called;
        // We don't really have a way to test if the device supports each effect or not.
        mVibrator.areAllPrimitivesSupported(PRIMITIVE_EFFECTS);
        assertTrue(mVibrator.areAllPrimitivesSupported());
    }

    @Test
    public void testVibratorPrimitivesDurations() {
        int[] durations = mVibrator.getPrimitiveDurations(PRIMITIVE_EFFECTS);
        boolean[] supported = mVibrator.arePrimitivesSupported(PRIMITIVE_EFFECTS);
        assertEquals(PRIMITIVE_EFFECTS.length, durations.length);
        for (int i = 0; i < durations.length; i++) {
            assertEquals("Primitive " + PRIMITIVE_EFFECTS[i]
                            + " expected to have " + (supported[i] ? "positive" : "zero")
                            + " duration, found " + durations[i] + "ms",
                    supported[i], durations[i] > 0);
        }
        assertEquals(0, mVibrator.getPrimitiveDurations().length);
    }

    @Test
    public void testVibratorIsVibrating() {
        if (!mVibrator.hasVibrator()) {
            return;
        }

        assertFalse(mVibrator.isVibrating());

        mVibrator.vibrate(5000);
        assertStartsVibrating();
        assertTrue(mVibrator.isVibrating());

        mVibrator.cancel();
        assertStopsVibrating();
        assertFalse(mVibrator.isVibrating());
    }

    @LargeTest
    @Test
    public void testVibratorVibratesNoLongerThanDuration() {
        if (!mVibrator.hasVibrator()) {
            return;
        }

        mVibrator.vibrate(1000);
        assertStartsVibrating();

        SystemClock.sleep(1500);
        assertFalse(mVibrator.isVibrating());
    }

    @LargeTest
    @Test
    public void testVibratorStateCallback() {
        if (!mVibrator.hasVibrator()) {
            return;
        }

        OnVibratorStateChangedListener listener1 = mock(OnVibratorStateChangedListener.class);
        OnVibratorStateChangedListener listener2 = mock(OnVibratorStateChangedListener.class);
        // Add listener1 on executor
        mVibrator.addVibratorStateListener(Executors.newSingleThreadExecutor(), listener1);
        // Add listener2 on main thread.
        mVibrator.addVibratorStateListener(listener2);
        verify(listener1, timeout(CALLBACK_TIMEOUT_MILLIS).times(1)).onVibratorStateChanged(false);
        verify(listener2, timeout(CALLBACK_TIMEOUT_MILLIS).times(1)).onVibratorStateChanged(false);

        mVibrator.vibrate(1000);

        verify(listener1, timeout(CALLBACK_TIMEOUT_MILLIS).times(1)).onVibratorStateChanged(true);
        verify(listener2, timeout(CALLBACK_TIMEOUT_MILLIS).times(1)).onVibratorStateChanged(true);

        mVibrator.cancel();
        assertStopsVibrating();

        // Remove listener1 & listener2
        mVibrator.removeVibratorStateListener(listener1);
        mVibrator.removeVibratorStateListener(listener2);
        reset(listener1);
        reset(listener2);

        mVibrator.vibrate(1000);
        assertStartsVibrating();

        verify(listener1, timeout(CALLBACK_TIMEOUT_MILLIS).times(0))
                .onVibratorStateChanged(anyBoolean());
        verify(listener2, timeout(CALLBACK_TIMEOUT_MILLIS).times(0))
                .onVibratorStateChanged(anyBoolean());
    }

    private void assertStartsThenStopsVibrating(long duration) {
        if (mVibrator.hasVibrator()) {
            verify(mStateListener, timeout(CALLBACK_TIMEOUT_MILLIS).atLeastOnce())
                    .onVibratorStateChanged(true);
            SystemClock.sleep(duration);
            assertVibratorState(false);
        }
    }

    private void assertStartsVibrating() {
        assertVibratorState(true);
    }

    private void assertStopsVibrating() {
        assertVibratorState(false);
    }

    private void assertVibratorState(boolean expected) {
        if (mVibrator.hasVibrator()) {
            verify(mStateListener, timeout(CALLBACK_TIMEOUT_MILLIS).atLeastOnce())
                    .onVibratorStateChanged(eq(expected));
            reset(mStateListener);
        }
    }
}
