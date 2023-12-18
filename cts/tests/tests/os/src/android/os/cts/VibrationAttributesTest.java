/*
 * Copyright (C) 2019 The Android Open Source Project
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
import static org.junit.Assert.assertNotEquals;

import android.media.AudioAttributes;
import android.os.VibrationAttributes;
import android.os.VibrationEffect;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class VibrationAttributesTest {
    private static final int TEST_USAGE = AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY;

    private static final int TEST_AMPLITUDE = 100;
    private static final long TEST_TIMING_LONG = 5000;
    private static final long TEST_TIMING_SHORT = 4999;
    private static final long[] TEST_TIMINGS_LONG = new long[] { 100, 100, 4800 };
    private static final long[] TEST_TIMINGS_SHORT = new long[] { 100, 100, 4799 };


    private static final VibrationEffect TEST_ONE_SHOT_LONG =
            VibrationEffect.createOneShot(TEST_TIMING_LONG, TEST_AMPLITUDE);
    private static final VibrationEffect TEST_ONE_SHOT_SHORT =
            VibrationEffect.createOneShot(TEST_TIMING_SHORT, TEST_AMPLITUDE);
    private static final VibrationEffect TEST_WAVEFORM_LONG =
            VibrationEffect.createWaveform(TEST_TIMINGS_LONG, -1);
    private static final VibrationEffect TEST_WAVEFORM_SHORT =
            VibrationEffect.createWaveform(TEST_TIMINGS_SHORT, -1);
    private static final VibrationEffect TEST_PREBAKED =
            VibrationEffect.get(VibrationEffect.EFFECT_CLICK, true);

    @Test
    public void testCreate() {
        AudioAttributes tmp = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ALARM)
                .build();
        VibrationAttributes attr = new VibrationAttributes.Builder(tmp, null).build();
        assertEquals(attr.getUsage(), VibrationAttributes.USAGE_ALARM);
        assertEquals(attr.getUsageClass(), VibrationAttributes.USAGE_CLASS_ALARM);
        assertEquals(attr.getFlags(), 0);
        assertEquals(attr.getAudioUsage(), AudioAttributes.USAGE_ALARM);
    }

    @Test
    public void testGetAudioUsageReturnOriginalUsage() {
        AudioAttributes tmp = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                .build();
        VibrationAttributes attr = new VibrationAttributes.Builder(tmp, null).build();
        assertEquals(attr.getUsage(), VibrationAttributes.USAGE_COMMUNICATION_REQUEST);
        assertEquals(attr.getAudioUsage(), AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY);
    }

    @Test
    public void testGetAudioUsageUnknownReturnsBasedOnVibrationUsage() {
        VibrationAttributes attr = new VibrationAttributes.Builder()
                .setUsage(VibrationAttributes.USAGE_NOTIFICATION).build();
        assertEquals(attr.getUsage(), VibrationAttributes.USAGE_NOTIFICATION);
        assertEquals(attr.getAudioUsage(), AudioAttributes.USAGE_NOTIFICATION);
    }

    @Test
    public void testEquals() {
        AudioAttributes tmp = createAudioAttributes(TEST_USAGE);
        VibrationAttributes attr = new VibrationAttributes.Builder(tmp, null).build();
        VibrationAttributes attr2 = new VibrationAttributes.Builder(tmp, null).build();
        assertEquals(attr, attr2);
    }

    @Test
    public void testNotEqualsDifferentAudioUsage() {
        AudioAttributes tmp = createAudioAttributes(
                AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_INSTANT);
        VibrationAttributes attr = new VibrationAttributes.Builder(tmp, null).build();
        AudioAttributes tmp2 = createAudioAttributes(
                AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_DELAYED);
        VibrationAttributes attr2 = new VibrationAttributes.Builder(tmp2, null).build();
        assertEquals(attr.getUsage(), attr2.getUsage());
        assertNotEquals(attr, attr2);
    }

    @Test
    public void testNotEqualsDifferentVibrationUsage() {
        VibrationAttributes attr = new VibrationAttributes.Builder()
                .setUsage(VibrationAttributes.USAGE_TOUCH)
                .build();
        VibrationAttributes attr2 = new VibrationAttributes.Builder()
                .setUsage(VibrationAttributes.USAGE_NOTIFICATION)
                .build();
        assertNotEquals(attr, attr2);
    }

    @Test
    public void testNotEqualsDifferentFlags() {
        AudioAttributes tmp = createAudioAttributes(TEST_USAGE);
        VibrationAttributes attr = new VibrationAttributes.Builder(tmp, null).build();
        VibrationAttributes attr2 = new VibrationAttributes.Builder(tmp, null).setFlags(1, 1)
                .build();
        assertNotEquals(attr, attr2);
    }

    @Test
    public void testHeuristics() {
        AudioAttributes tmp = createAudioAttributes(AudioAttributes.USAGE_UNKNOWN);
        VibrationAttributes oneShotLong =
            new VibrationAttributes.Builder(tmp, TEST_ONE_SHOT_LONG).build();
        VibrationAttributes oneShotShort =
            new VibrationAttributes.Builder(tmp, TEST_ONE_SHOT_SHORT).build();
        VibrationAttributes waveformLong =
            new VibrationAttributes.Builder(tmp, TEST_WAVEFORM_LONG).build();
        VibrationAttributes waveformShort =
            new VibrationAttributes.Builder(tmp, TEST_WAVEFORM_SHORT).build();
        VibrationAttributes prebaked =
            new VibrationAttributes.Builder(tmp, TEST_PREBAKED).build();
        assertEquals(oneShotShort.getUsage(), VibrationAttributes.USAGE_TOUCH);
        assertEquals(oneShotShort.getAudioUsage(), AudioAttributes.USAGE_ASSISTANCE_SONIFICATION);
        assertEquals(waveformShort.getUsage(), VibrationAttributes.USAGE_TOUCH);
        assertEquals(waveformShort.getAudioUsage(), AudioAttributes.USAGE_ASSISTANCE_SONIFICATION);
        assertEquals(oneShotLong.getUsage(), VibrationAttributes.USAGE_UNKNOWN);
        assertEquals(oneShotLong.getAudioUsage(), AudioAttributes.USAGE_UNKNOWN);
        assertEquals(waveformLong.getUsage(), VibrationAttributes.USAGE_UNKNOWN);
        assertEquals(waveformLong.getAudioUsage(), AudioAttributes.USAGE_UNKNOWN);
        assertEquals(prebaked.getUsage(), VibrationAttributes.USAGE_TOUCH);
        assertEquals(prebaked.getAudioUsage(), AudioAttributes.USAGE_ASSISTANCE_SONIFICATION);
    }

    private static AudioAttributes createAudioAttributes(int usage) {
        return new AudioAttributes.Builder().setUsage(usage).build();
    }
}

