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

package com.android.car.audio;

import static android.media.AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
import static android.media.AudioAttributes.USAGE_EMERGENCY;
import static android.media.AudioAttributes.USAGE_GAME;
import static android.media.AudioAttributes.USAGE_MEDIA;
import static android.media.AudioAttributes.USAGE_UNKNOWN;
import static android.media.AudioAttributes.USAGE_VIRTUAL_SOURCE;

import static com.android.car.audio.CarAudioContext.EMERGENCY;
import static com.android.car.audio.CarAudioContext.INVALID;
import static com.android.car.audio.CarAudioContext.MUSIC;
import static com.android.car.audio.CarAudioContext.NAVIGATION;
import static com.android.car.audio.CarAudioContext.isCriticalAudioContext;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.testng.Assert.assertThrows;

import android.media.AudioAttributes;
import android.media.AudioAttributes.AttributeUsage;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.audio.CarAudioContext.AudioContext;

import com.google.common.primitives.Ints;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.HashSet;
import java.util.Set;

@RunWith(AndroidJUnit4.class)
public class CarAudioContextTest {
    private static final int INVALID_USAGE = -5;
    private static final int INVALID_CONTEXT = -5;

    @Test
    public void getContextForUsage_forValidUsage_returnsContext() {
        assertThat(CarAudioContext.getContextForUsage(USAGE_MEDIA))
                .isEqualTo(MUSIC);
    }

    @Test
    public void getContextForUsage_withInvalidUsage_returnsInvalidContext() {
        assertThat(CarAudioContext.getContextForUsage(INVALID_USAGE)).isEqualTo(
                INVALID);
    }

    @Test
    public void getContextForAudioAttributes_forAttributeWithValidUsage_returnsContext() {
        AudioAttributes attributes = new AudioAttributes.Builder().setUsage(USAGE_MEDIA).build();

        assertThat(CarAudioContext.getContextForAttributes(attributes)).isEqualTo(MUSIC);
    }

    @Test
    public void getContextForAudioAttributes_forAttributesWithInvalidUsage_returnsInvalidContext() {
        AudioAttributes attributes = new AudioAttributes.Builder().setUsage(USAGE_VIRTUAL_SOURCE)
                .build();

        assertThat(CarAudioContext.getContextForAttributes(attributes)).isEqualTo(INVALID);
    }

    @Test
    public void getUsagesForContext_withValidContext_returnsUsages() {
        int[] usages = CarAudioContext.getUsagesForContext(MUSIC);
        assertThat(usages).asList().containsExactly(USAGE_UNKNOWN, USAGE_GAME, USAGE_MEDIA);
    }

    @Test
    public void getUsagesForContext_withInvalidContext_throws() {
        assertThrows(IllegalArgumentException.class, () -> {
            CarAudioContext.getUsagesForContext(INVALID_CONTEXT);
        });
    }

    @Test
    public void getUsagesForContext_returnsUniqueValuesForAllContexts() {
        Set<Integer> allUsages = new HashSet<>();
        for (@AudioContext int audioContext : CarAudioContext.CONTEXTS) {
            @AttributeUsage int[] usages = CarAudioContext.getUsagesForContext(audioContext);
            assertThat(allUsages.addAll(Ints.asList(usages))).isTrue();
        }
    }

    @Test
    public void getUniqueContextsForUsages_withEmptyArray_returnsEmptySet() {
        Set<Integer> result = CarAudioContext.getUniqueContextsForUsages(new int[0]);

        assertThat(result).isEmpty();
    }

    @Test
    public void getUniqueContextsForUsages_withMultipleUsages_filtersDuplicateContexts() {
        int[] usages = {USAGE_GAME, USAGE_MEDIA};

        Set<Integer> result = CarAudioContext.getUniqueContextsForUsages(usages);

        assertThat(result).containsExactly(MUSIC);
    }

    @Test
    public void getUniqueContextsForUsages_withMultipleUsages_returnsAllUniqueContexts() {
        int[] usages = {USAGE_MEDIA, USAGE_ASSISTANCE_NAVIGATION_GUIDANCE, USAGE_EMERGENCY};

        Set<Integer> result = CarAudioContext.getUniqueContextsForUsages(usages);

        assertThat(result).containsExactly(MUSIC, NAVIGATION, EMERGENCY);
    }

    @Test
    public void isCriticalAudioContext_forNonCritialContexts_returnsFalse() {
        assertWithMessage("Non-critical context INVALID")
                .that(isCriticalAudioContext(CarAudioContext.INVALID)).isFalse();
        assertWithMessage("Non-critical context MUSIC")
                .that(isCriticalAudioContext(CarAudioContext.MUSIC)).isFalse();
        assertWithMessage("Non-critical context NAVIGATION")
                .that(isCriticalAudioContext(CarAudioContext.NAVIGATION)).isFalse();
        assertWithMessage("Non-critical context VOICE_COMMAND")
                .that(isCriticalAudioContext(CarAudioContext.VOICE_COMMAND)).isFalse();
        assertWithMessage("Non-critical context CALL_RING")
                .that(isCriticalAudioContext(CarAudioContext.CALL_RING)).isFalse();
        assertWithMessage("Non-critical context CALL")
                .that(isCriticalAudioContext(CarAudioContext.CALL)).isFalse();
        assertWithMessage("Non-critical context ALARM")
                .that(isCriticalAudioContext(CarAudioContext.ALARM)).isFalse();
        assertWithMessage("Non-critical context NOTIFICATION")
                .that(isCriticalAudioContext(CarAudioContext.NOTIFICATION)).isFalse();
        assertWithMessage("Non-critical context SYSTEM_SOUND")
                .that(isCriticalAudioContext(CarAudioContext.SYSTEM_SOUND)).isFalse();
        assertWithMessage("Non-critical context VEHICLE_STATUS")
                .that(isCriticalAudioContext(CarAudioContext.VEHICLE_STATUS)).isFalse();
        assertWithMessage("Non-critical context ANNOUNCEMENT")
                .that(isCriticalAudioContext(CarAudioContext.ANNOUNCEMENT)).isFalse();
    }

    @Test
    public void isCriticalAudioContext_forCriticalContexts_returnsTrue() {
        assertWithMessage("Critical context EMERGENCY")
                .that(isCriticalAudioContext(CarAudioContext.EMERGENCY)).isTrue();
        assertWithMessage("Critical context SAFETY")
                .that(isCriticalAudioContext(CarAudioContext.SAFETY)).isTrue();
    }
}
