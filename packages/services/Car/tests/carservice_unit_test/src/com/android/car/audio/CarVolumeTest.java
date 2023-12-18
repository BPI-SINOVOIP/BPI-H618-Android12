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

import static android.media.AudioAttributes.USAGE_ANNOUNCEMENT;
import static android.media.AudioAttributes.USAGE_ASSISTANT;
import static android.media.AudioAttributes.USAGE_MEDIA;
import static android.media.AudioAttributes.USAGE_VIRTUAL_SOURCE;
import static android.telephony.TelephonyManager.CALL_STATE_IDLE;
import static android.telephony.TelephonyManager.CALL_STATE_OFFHOOK;
import static android.telephony.TelephonyManager.CALL_STATE_RINGING;

import static com.android.car.audio.CarAudioContext.ALARM;
import static com.android.car.audio.CarAudioContext.CALL;
import static com.android.car.audio.CarAudioContext.CALL_RING;
import static com.android.car.audio.CarAudioContext.INVALID;
import static com.android.car.audio.CarAudioContext.MUSIC;
import static com.android.car.audio.CarAudioContext.NAVIGATION;
import static com.android.car.audio.CarAudioContext.NOTIFICATION;
import static com.android.car.audio.CarAudioContext.VOICE_COMMAND;
import static com.android.car.audio.CarAudioService.DEFAULT_AUDIO_CONTEXT;
import static com.android.car.audio.CarAudioService.SystemClockWrapper;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;
import static org.testng.Assert.expectThrows;

import android.media.AudioAttributes.AttributeUsage;

import com.android.car.audio.CarAudioContext.AudioContext;

import com.google.common.collect.ImmutableList;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.ArrayList;
import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public class CarVolumeTest {

    private static final @CarVolume.CarVolumeListVersion int VERSION_ZERO = 0;
    private static final @CarVolume.CarVolumeListVersion int VERSION_ONE = 1;
    private static final @CarVolume.CarVolumeListVersion int VERSION_TWO = 2;
    private static final @CarVolume.CarVolumeListVersion int VERSION_THREE = 3;
    private static final long START_TIME = 10000;
    private static final long START_TIME_ONE_SECOND = 11000;
    private static final long START_TIME_FOUR_SECOND = 14000;
    private static final int KEY_EVENT_TIMEOUT_MS = 3000;
    private static final int TRIAL_COUNTS = 10;

    @Mock
    private SystemClockWrapper mMockClock;

    private CarVolume mCarVolume;

    @Before
    public void setUp() throws Exception {
        when(mMockClock.uptimeMillis()).thenReturn(START_TIME);
        mCarVolume = new CarVolume(mMockClock, VERSION_TWO, KEY_EVENT_TIMEOUT_MS);

    }

    @Test
    public void constructor_withVersionLessThanOne_failsTooLow() {
        IllegalArgumentException thrown = expectThrows(IllegalArgumentException.class, () -> {
            new CarVolume(mMockClock, VERSION_ZERO, KEY_EVENT_TIMEOUT_MS);
        });

        assertWithMessage("Constructor Exception")
                .that(thrown).hasMessageThat().contains("too low");
    }

    @Test
    public void constructor_withVersionGreaterThanTwo_failsTooHigh() {
        IllegalArgumentException thrown = expectThrows(IllegalArgumentException.class, () -> {
            new CarVolume(mMockClock, VERSION_THREE, KEY_EVENT_TIMEOUT_MS);
        });

        assertWithMessage("Constructor Exception")
                .that(thrown).hasMessageThat().contains("too high");
    }

    @Test
    public void constructor_withNullSystemClock_fails() {
        NullPointerException thrown = expectThrows(NullPointerException.class, () -> {
            new CarVolume(null, VERSION_ONE, KEY_EVENT_TIMEOUT_MS);
        });

        assertWithMessage("Constructor Exception")
                .that(thrown).hasMessageThat().contains("Clock");
    }

    @Test
    public void getSuggestedAudioContext_withNullActivePlayback_fails() {
        assertThrows(NullPointerException.class,
                () -> mCarVolume.getSuggestedAudioContextAndSaveIfFound(
                null, CALL_STATE_IDLE, new int[0]));
    }

    @Test
    public void getSuggestedAudioContext_withNullHallUsages_fails() {
        assertThrows(NullPointerException.class,
                () -> mCarVolume.getSuggestedAudioContextAndSaveIfFound(
                new ArrayList<>(), CALL_STATE_IDLE, null));
    }

    @Test
    public void getSuggestedAudioContext_withNoActivePlaybackAndIdleTelephony_returnsDefault() {
        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_IDLE, new int[0]);

        assertThat(suggestedContext).isEqualTo(CarAudioService.DEFAULT_AUDIO_CONTEXT);
    }

    @Test
    public void getSuggestedAudioContext_withOneConfiguration_returnsAssociatedContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        new int[0]);

        assertThat(suggestedContext).isEqualTo(VOICE_COMMAND);
    }

    @Test
    public void getSuggestedAudioContext_withCallStateOffHook_returnsCallContext() {
        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                        CALL_STATE_OFFHOOK, new int[0]);

        assertThat(suggestedContext).isEqualTo(CALL);
    }

    @Test

    public void getSuggestedAudioContext_withV1AndCallStateRinging_returnsCallRingContext() {
        CarVolume carVolume = new CarVolume(mMockClock, VERSION_ONE, KEY_EVENT_TIMEOUT_MS);

        @AudioContext int suggestedContext =
                carVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_RINGING, new int[0]);

        assertThat(suggestedContext).isEqualTo(CALL_RING);
    }

    @Test
    public void getSuggestedAudioContext_withActivePlayback_returnsHighestPriorityContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(ALARM, CALL, NOTIFICATION);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        new int[0]);

        assertThat(suggestedContext).isEqualTo(CALL);
    }

    @Test
    public void getSuggestedAudioContext_withLowerPriorityActivePlaybackAndCall_returnsCall() {
        List<Integer> activePlaybackContexts = ImmutableList.of(ALARM, NOTIFICATION);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_OFFHOOK,
                        new int[0]);

        assertThat(suggestedContext).isEqualTo(CALL);
    }

    @Test
    public void getSuggestedAudioContext_withV1AndNavigationConfigurationAndCall_returnsNav() {
        CarVolume carVolume = new CarVolume(mMockClock, VERSION_ONE, KEY_EVENT_TIMEOUT_MS);
        List<Integer> activePlaybackContexts = ImmutableList.of(NAVIGATION);

        @AudioContext int suggestedContext = carVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_OFFHOOK,
                        new int[0]);

        assertThat(suggestedContext).isEqualTo(NAVIGATION);
    }

    @Test
    public void getSuggestedAudioContext_withV2AndNavigationConfigurationAndCall_returnsCall() {
        List<Integer> activePlaybackContexts = ImmutableList.of(NAVIGATION);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_OFFHOOK,
                        new int[0]);

        assertThat(suggestedContext).isEqualTo(CALL);
    }

    @Test
    public void getSuggestedAudioContext_withUnprioritizedUsage_returnsDefault() {
        List<Integer> activePlaybackContexts = ImmutableList.of(INVALID);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        new int[0]);

        assertThat(suggestedContext).isEqualTo(DEFAULT_AUDIO_CONTEXT);
    }

    @Test
    public void getSuggestedAudioContext_withHalActiveUsage_returnsHalActive() {
        int[] activeHalUsages = new int[] {USAGE_ASSISTANT};

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_IDLE, activeHalUsages);

        assertThat(suggestedContext).isEqualTo(VOICE_COMMAND);
    }

    @Test
    public void getSuggestedAudioContext_withHalUnprioritizedUsage_returnsDefault() {
        int[] activeHalUsages = new int[] {USAGE_VIRTUAL_SOURCE};

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_IDLE, activeHalUsages);

        assertThat(suggestedContext).isEqualTo(DEFAULT_AUDIO_CONTEXT);
    }

    @Test
    public void getSuggestedAudioContext_withConfigAndHalActiveUsage_returnsConfigActive() {
        int[] activeHalUsages = new int[] {USAGE_ASSISTANT};
        List<Integer> activePlaybackContexts = ImmutableList.of(MUSIC);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        activeHalUsages);

        assertThat(suggestedContext).isEqualTo(MUSIC);
    }

    @Test
    public void getSuggestedAudioContext_withConfigAndHalActiveUsage_returnsHalActive() {
        int[] activeHalUsages = new int[] {USAGE_MEDIA};
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        activeHalUsages);

        assertThat(suggestedContext).isEqualTo(MUSIC);
    }

    @Test
    public void getSuggestedAudioContext_withHalActiveUsageAndActiveCall_returnsCall() {
        int[] activeHalUsages = new int[] {USAGE_MEDIA};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        @AudioContext int suggestedContext = mCarVolume.getSuggestedAudioContextAndSaveIfFound(
                activePlaybackContexts, CALL_STATE_OFFHOOK, activeHalUsages);

        assertThat(suggestedContext).isEqualTo(CALL);
    }

    @Test
    public void getSuggestedAudioContext_withMultipleHalActiveUsages_returnsMusic() {
        int[] activeHalUsages = new int[] {USAGE_MEDIA, USAGE_ANNOUNCEMENT, USAGE_ASSISTANT};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        @AudioContext int suggestedContext = mCarVolume
                .getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        activeHalUsages);

        assertThat(suggestedContext).isEqualTo(MUSIC);
    }

    @Test
    public void getSuggestedAudioContext_withStillActiveContext_returnsPrevActiveContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                new int[0]);

        when(mMockClock.uptimeMillis()).thenReturn(START_TIME_ONE_SECOND);

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_IDLE, new int[0]);

        assertThat(suggestedContext).isEqualTo(VOICE_COMMAND);
    }

    @Test
    public void
            getSuggestedAudioContext_withStillActiveContext_retPrevActiveContextMultipleTimes() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                new int[0]);

        long deltaTime = KEY_EVENT_TIMEOUT_MS - 1;
        for (int volumeCounter = 1; volumeCounter < TRIAL_COUNTS; volumeCounter++) {
            when(mMockClock.uptimeMillis())
                    .thenReturn(START_TIME + (volumeCounter * deltaTime));

            @AudioContext int suggestedContext =
                    mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                            CALL_STATE_IDLE, new int[0]);
            assertThat(suggestedContext).isEqualTo(VOICE_COMMAND);
        }
    }

    @Test
    public void
            getSuggestedAudioContext_withActContextAndNewHigherPrioContext_returnPrevActContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                new int[0]);

        when(mMockClock.uptimeMillis()).thenReturn(START_TIME_ONE_SECOND);

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_OFFHOOK, new int[0]);

        assertThat(suggestedContext).isEqualTo(VOICE_COMMAND);
    }

    @Test
    public void getSuggestedAudioContext_afterActiveContextTimeout_returnsDefaultContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                        new int[0]);

        when(mMockClock.uptimeMillis()).thenReturn(START_TIME_FOUR_SECOND);

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_IDLE, new int[0]);

        assertThat(suggestedContext).isEqualTo(DEFAULT_AUDIO_CONTEXT);
    }

    @Test
    public void
            getSuggestedAudioContext_afterActiveContextTimeoutAndNewContext_returnsNewContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                new int[0]);

        when(mMockClock.uptimeMillis()).thenReturn(START_TIME_FOUR_SECOND);

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_OFFHOOK, new int[0]);

        assertThat(suggestedContext).isEqualTo(CALL);
    }

    @Test
    public void
            getSuggestedAudioContext_afterMultipleQueriesAndNewContextCall_returnsNewContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                new int[0]);


        long deltaTime = KEY_EVENT_TIMEOUT_MS - 1;

        for (int volumeCounter = 1; volumeCounter < TRIAL_COUNTS; volumeCounter++) {
            when(mMockClock.uptimeMillis()).thenReturn(START_TIME + volumeCounter * deltaTime);

            mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(), CALL_STATE_IDLE,
                            new int[0]);
        }

        when(mMockClock.uptimeMillis())
                .thenReturn(START_TIME + (TRIAL_COUNTS * deltaTime) + KEY_EVENT_TIMEOUT_MS);

        @AudioContext int newContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                CALL_STATE_OFFHOOK, new int[0]);

        assertThat(newContext).isEqualTo(CALL);
    }

    @Test
    public void getSuggestedAudioContext_afterResetSelectedVolumeContext_returnsDefaultContext() {
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        mCarVolume.getSuggestedAudioContextAndSaveIfFound(activePlaybackContexts, CALL_STATE_IDLE,
                new int[0]);

        when(mMockClock.uptimeMillis()).thenReturn(START_TIME_ONE_SECOND);

        mCarVolume.resetSelectedVolumeContext();

        @AudioContext int suggestedContext =
                mCarVolume.getSuggestedAudioContextAndSaveIfFound(new ArrayList<>(),
                        CALL_STATE_IDLE, new int[0]);

        assertThat(suggestedContext).isEqualTo(DEFAULT_AUDIO_CONTEXT);
    }


    @Test
    public void isAnyContextActive_withOneConfigurationAndMatchedContext_returnsTrue() {
        @AudioContext int[] activeContexts = {VOICE_COMMAND};
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_IDLE,
                new int[0])).isTrue();
    }

    @Test
    public void isAnyContextActive_withOneConfigurationAndMismatchedContext_returnsFalse() {
        @AudioContext int[] activeContexts = {ALARM};
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_IDLE,
                new int[0])).isFalse();
    }

    @Test
    public void isAnyContextActive_withOneConfigurationAndMultipleContexts_returnsTrue() {
        @AudioContext int[] activeContexts = {ALARM, MUSIC, VOICE_COMMAND};
        List<Integer> activePlaybackContexts = ImmutableList.of(VOICE_COMMAND);

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_IDLE,
                new int[0])).isTrue();
    }

    @Test
    public void isAnyContextActive_withOneConfigurationAndMultipleContexts_returnsFalse() {
        @AudioContext int[] activeContexts = {ALARM, MUSIC, VOICE_COMMAND};
        List<Integer> activePlaybackContexts = ImmutableList.of(NOTIFICATION);

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_IDLE,
                new int[0])).isFalse();
    }

    @Test
    public void isAnyContextActive_withActiveHalUsagesAndMatchedContext_returnsTrue() {
        @AudioContext int[] activeContexts = {VOICE_COMMAND};
        @AttributeUsage int[] activeHalUsages = {USAGE_MEDIA, USAGE_ANNOUNCEMENT, USAGE_ASSISTANT};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_IDLE,
                activeHalUsages)).isTrue();
    }

    @Test
    public void isAnyContextActive_withActiveHalUsagesAndMismatchedContext_returnsFalse() {
        @AudioContext int[] activeContexts = {ALARM};
        @AttributeUsage int[] activeHalUsages = {USAGE_MEDIA, USAGE_ANNOUNCEMENT, USAGE_ASSISTANT};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_IDLE,
                activeHalUsages)).isFalse();
    }

    @Test
    public void isAnyContextActive_withActiveCallAndMatchedContext_returnsTrue() {
        @AudioContext int[] activeContexts = {CALL};
        @AttributeUsage int[] activeHalUsages = {};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_OFFHOOK,
                activeHalUsages)).isTrue();
    }

    @Test
    public void isAnyContextActive_withActiveCallAndMismatchedContext_returnsFalse() {
        @AudioContext int[] activeContexts = {VOICE_COMMAND};
        @AttributeUsage int[] activeHalUsages = {};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThat(CarVolume
                .isAnyContextActive(activeContexts, activePlaybackContexts, CALL_STATE_OFFHOOK,
                activeHalUsages)).isFalse();
    }

    @Test
    public void isAnyContextActive_withNullContexts_fails() {
        @AudioContext int[] activeContexts = null;
        @AttributeUsage int[] activeHalUsages = {};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThrows(NullPointerException.class,
                () -> CarVolume.isAnyContextActive(activeContexts,
                        activePlaybackContexts, CALL_STATE_OFFHOOK, activeHalUsages));
    }

    @Test
    public void isAnyContextActive_withEmptyContexts_fails() {
        @AudioContext int[] activeContexts = {};
        @AttributeUsage int[] activeHalUsages = {};
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThrows(IllegalArgumentException.class,
                () -> CarVolume.isAnyContextActive(activeContexts,
                        activePlaybackContexts, CALL_STATE_OFFHOOK, activeHalUsages));
    }

    @Test
    public void isAnyContextActive_withNullActivePlayback_fails() {
        @AudioContext int[] activeContexts = {ALARM};
        @AttributeUsage int[] activeHalUsages = {};
        List<Integer> activePlaybackContexts = null;

        assertThrows(NullPointerException.class,
                () -> CarVolume.isAnyContextActive(activeContexts,
                        activePlaybackContexts, CALL_STATE_OFFHOOK, activeHalUsages));
    }

    @Test
    public void isAnyContextActive_withNullHalUsages_fails() {
        @AudioContext int[] activeContexts = {ALARM};
        @AttributeUsage int[] activeHalUsages = null;
        List<Integer> activePlaybackContexts = new ArrayList<>();

        assertThrows(NullPointerException.class,
                () -> CarVolume.isAnyContextActive(activeContexts,
                        activePlaybackContexts, CALL_STATE_OFFHOOK, activeHalUsages));
    }
}
