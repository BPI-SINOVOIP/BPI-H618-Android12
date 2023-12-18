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

package com.android.car.audio;

import static android.media.AudioAttributes.AttributeUsage;
import static android.media.AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
import static android.media.AudioAttributes.USAGE_MEDIA;

import static com.android.car.audio.CarAudioContext.MUSIC;
import static com.android.car.audio.CarAudioContext.NAVIGATION;
import static com.android.car.audio.CarAudioContext.VOICE_COMMAND;
import static com.android.car.audio.CarAudioService.SystemClockWrapper;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.testng.Assert.expectThrows;

import android.media.AudioAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioPlaybackConfiguration;

import com.google.common.collect.ImmutableList;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public final class CarAudioPlaybackCallbackTest {

    private static final int PRIMARY_ZONE_ID = 0;
    private static final String PRIMARY_MEDIA_ADDRESS = "music_bus0";
    private static final String PRIMARY_NAVIGATION_ADDRESS = "navigation_bus1";
    private static final String PRIMARY_VOICE_ADDRESS = "voice_bus3";

    private static final String SECONDARY_MEDIA_ADDRESS = "music_bus100";
    private static final String SECONDARY_NAVIGATION_ADDRESS = "navigation_bus101";

    private static final long TIMER_START_TIME_MS = 100000;
    private static final int KEY_EVENT_TIMEOUT_MS = 3000;
    private static final long TIMER_BEFORE_TIMEOUT_MS =
            TIMER_START_TIME_MS + KEY_EVENT_TIMEOUT_MS - 1;
    private static final long TIMER_AFTER_TIMEOUT_MS =
            TIMER_START_TIME_MS + KEY_EVENT_TIMEOUT_MS + 1;

    @Mock
    private SystemClockWrapper mClock;

    private CarAudioZone mPrimaryZone;

    @Before
    public void setUp() {
        mPrimaryZone = generatePrimaryZone();
        when(mClock.uptimeMillis()).thenReturn(TIMER_START_TIME_MS);
    }

    @Test
    public void createCarAudioPlaybackCallback_withNullCarAudioZones_fails() throws Exception {
        expectThrows(NullPointerException.class, () -> {
            new CarAudioPlaybackCallback(null, mClock, KEY_EVENT_TIMEOUT_MS);
        });
    }

    @Test
    public void createCarAudioPlaybackCallback_withNullSystemClockWrapper_fails() throws Exception {
        expectThrows(NullPointerException.class, () -> {
            new CarAudioPlaybackCallback(mPrimaryZone, null, KEY_EVENT_TIMEOUT_MS);
        });
    }

    @Test
    public void
            createCarAudioPlaybackCallback_withNegativeKeyEventTimeout_fails() throws Exception {
        expectThrows(IllegalArgumentException.class, () -> {
            new CarAudioPlaybackCallback(mPrimaryZone, mClock, -KEY_EVENT_TIMEOUT_MS);
        });
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withNoOnPlaybackConfigChanged_returnsEmptyList() {
        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).isEmpty();
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withOneMatchingConfiguration_returnsActiveContext() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).containsExactly(MUSIC);
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withMultipleConfiguration_returnsActiveContexts() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).containsExactly(MUSIC, NAVIGATION);
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withInactiveConfigurations_returnsActiveContext() {
        List<AudioPlaybackConfiguration> configurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(configurations);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).containsExactly(MUSIC);
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withNoActiveConfigurations_returnsEmptyContexts() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .setInactive()
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).isEmpty();
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withInactiveConfig_beforeTimeout_returnsContexts() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        List<AudioPlaybackConfiguration> configurationsChanged = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setInactive()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        callback.onPlaybackConfigChanged(configurationsChanged);

        when(mClock.uptimeMillis()).thenReturn(TIMER_BEFORE_TIMEOUT_MS);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).containsExactly(MUSIC, NAVIGATION);
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withInactiveConfigs_beforeTimeout_returnsContexts() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        List<AudioPlaybackConfiguration> configurationsChanged = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .setInactive()
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        callback.onPlaybackConfigChanged(configurationsChanged);

        when(mClock.uptimeMillis()).thenReturn(TIMER_BEFORE_TIMEOUT_MS);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).containsExactly(NAVIGATION, MUSIC);
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_afterResetStillActiveContexts_returnsEmptyContext() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        List<AudioPlaybackConfiguration> configurationsChanged = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .setInactive()
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        callback.onPlaybackConfigChanged(configurationsChanged);

        callback.resetStillActiveContexts();

        when(mClock.uptimeMillis()).thenReturn(TIMER_BEFORE_TIMEOUT_MS);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).isEmpty();
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withInactiveConfig_afterTimeout_returnsContext() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        List<AudioPlaybackConfiguration> configurationsChanged = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        callback.onPlaybackConfigChanged(configurationsChanged);

        when(mClock.uptimeMillis()).thenReturn(TIMER_AFTER_TIMEOUT_MS);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).containsExactly(MUSIC);
    }

    @Test
    public void getAllActiveContextsForPrimaryZone_withInactiveConfigs_afterTimeout_returnsEmpty() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .build()
        );

        List<AudioPlaybackConfiguration> configurationsChanged = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(PRIMARY_MEDIA_ADDRESS)
                        .setInactive()
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(PRIMARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        callback.onPlaybackConfigChanged(configurationsChanged);

        when(mClock.uptimeMillis()).thenReturn(TIMER_AFTER_TIMEOUT_MS);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).isEmpty();
    }


    @Test
    public void
            getAllActiveContextsForPrimaryZone_withMultiActiveConfigs_forDiffZone_returnsEmpty() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(SECONDARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(SECONDARY_NAVIGATION_ADDRESS)
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).isEmpty();
    }

    @Test
    public void
            getAllActiveContextsForPrimaryZone_withInactiveConfigs_forDifferentZone_returnsEmpty() {
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(SECONDARY_MEDIA_ADDRESS)
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(SECONDARY_NAVIGATION_ADDRESS)
                        .build()
        );

        List<AudioPlaybackConfiguration> configurationsChanged = ImmutableList.of(
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_MEDIA)
                        .setDeviceAddress(SECONDARY_MEDIA_ADDRESS)
                        .setInactive()
                        .build(),
                new AudioPlaybackConfigurationBuilder()
                        .setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(SECONDARY_NAVIGATION_ADDRESS)
                        .setInactive()
                        .build()
        );

        CarAudioPlaybackCallback callback =
                new CarAudioPlaybackCallback(mPrimaryZone, mClock, KEY_EVENT_TIMEOUT_MS);

        callback.onPlaybackConfigChanged(activeConfigurations);

        callback.onPlaybackConfigChanged(configurationsChanged);

        List<Integer> activeContexts =
                callback.getAllActiveContextsForPrimaryZone();

        assertThat(activeContexts).isEmpty();
    }

    private CarAudioZone generatePrimaryZone() {
        return new TestCarAudioZoneBuilder("Primary zone", PRIMARY_ZONE_ID)
                .addVolumeGroup(new VolumeGroupBuilder()
                                .addDeviceAddressAndContexts(MUSIC, PRIMARY_MEDIA_ADDRESS)
                                .build())
                .addVolumeGroup(new VolumeGroupBuilder()
                        .addDeviceAddressAndContexts(NAVIGATION, PRIMARY_NAVIGATION_ADDRESS)
                        .build())
                .addVolumeGroup(new VolumeGroupBuilder()
                        .addDeviceAddressAndContexts(VOICE_COMMAND, PRIMARY_VOICE_ADDRESS)
                        .build())
                .build();
    }

    private static class AudioPlaybackConfigurationBuilder {
        private @AttributeUsage int mUsage = USAGE_MEDIA;
        private boolean mIsActive = true;
        private String mDeviceAddress = "";

        AudioPlaybackConfigurationBuilder setUsage(@AttributeUsage int usage) {
            mUsage = usage;
            return this;
        }

        AudioPlaybackConfigurationBuilder setDeviceAddress(String deviceAddress) {
            mDeviceAddress = deviceAddress;
            return this;
        }

        AudioPlaybackConfigurationBuilder setInactive() {
            mIsActive = false;
            return this;
        }

        AudioPlaybackConfiguration build() {
            AudioPlaybackConfiguration configuration = mock(AudioPlaybackConfiguration.class);
            AudioAttributes attributes = new AudioAttributes.Builder().setUsage(mUsage).build();
            AudioDeviceInfo outputDevice = generateOutAudioDeviceInfo(mDeviceAddress);
            when(configuration.getAudioAttributes()).thenReturn(attributes);
            when(configuration.getAudioDeviceInfo()).thenReturn(outputDevice);
            when(configuration.isActive()).thenReturn(mIsActive);
            return configuration;
        }

        private AudioDeviceInfo generateOutAudioDeviceInfo(String address) {
            AudioDeviceInfo audioDeviceInfo = mock(AudioDeviceInfo.class);
            when(audioDeviceInfo.getAddress()).thenReturn(address);
            when(audioDeviceInfo.getType()).thenReturn(AudioDeviceInfo.TYPE_BUS);
            when(audioDeviceInfo.isSource()).thenReturn(false);
            when(audioDeviceInfo.isSink()).thenReturn(true);
            when(audioDeviceInfo.getInternalType()).thenReturn(AudioDeviceInfo
                    .convertDeviceTypeToInternalInputDevice(AudioDeviceInfo.TYPE_BUS));
            return audioDeviceInfo;
        }
    }
}
