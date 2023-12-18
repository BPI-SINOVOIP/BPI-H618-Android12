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
package com.android.car.audio;

import static android.media.AudioAttributes.USAGE_ALARM;
import static android.media.AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
import static android.media.AudioAttributes.USAGE_ASSISTANCE_SONIFICATION;
import static android.media.AudioAttributes.USAGE_ASSISTANT;
import static android.media.AudioAttributes.USAGE_MEDIA;

import static com.android.car.audio.CarAudioContext.ALARM;
import static com.android.car.audio.CarAudioContext.AudioContext;
import static com.android.car.audio.CarAudioContext.INVALID;
import static com.android.car.audio.CarAudioContext.MUSIC;
import static com.android.car.audio.CarAudioContext.NAVIGATION;
import static com.android.car.audio.CarAudioContext.SYSTEM_SOUND;
import static com.android.car.audio.CarAudioContext.VOICE_COMMAND;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;
import static org.testng.Assert.expectThrows;

import android.car.media.CarAudioManager;
import android.media.AudioAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioPlaybackConfiguration;
import android.util.SparseArray;

import com.google.common.collect.ImmutableList;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RunWith(MockitoJUnitRunner.class)
public class CarAudioZoneTest {
    private static final String MUSIC_ADDRESS = "bus0_music";
    private static final String NAV_ADDRESS = "bus1_nav";
    private static final String VOICE_ADDRESS = "bus3_voice";
    private static final String ASSISTANT_ADDRESS = "bus10_assistant";
    private static final String ALARM_ADDRESS = "bus11_alarm";
    private static final String ANNOUNCEMENT_ADDRESS = "bus12_announcement";

    @Mock
    private CarVolumeGroup mMockMusicGroup;
    @Mock
    private CarVolumeGroup mMockNavGroup;
    @Mock
    private CarVolumeGroup mMockVoiceGroup;
    private CarAudioZone mTestAudioZone =
            new CarAudioZone(CarAudioManager.PRIMARY_AUDIO_ZONE, "Primary zone");

    @Before
    public void setUp() {
        mMockMusicGroup = new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(MUSIC, MUSIC_ADDRESS).build();

        mMockNavGroup = new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(NAVIGATION, NAV_ADDRESS).build();

        mMockVoiceGroup = new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(VOICE_COMMAND, VOICE_ADDRESS).build();
    }

    @Test
    public void getAddressForContext_returnsExpectedDeviceAddress() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);

        String musicAddress = mTestAudioZone.getAddressForContext(MUSIC);
        assertThat(musicAddress).isEqualTo(MUSIC_ADDRESS);

        String navAddress = mTestAudioZone.getAddressForContext(NAVIGATION);
        assertThat(navAddress).matches(NAV_ADDRESS);
    }

    @Test
    public void getAddressForContext_throwsOnInvalidContext() {
        IllegalArgumentException thrown =
                expectThrows(IllegalArgumentException.class,
                        () -> mTestAudioZone.getAddressForContext(INVALID));

        assertThat(thrown).hasMessageThat().contains("audioContext 0 is invalid");
    }

    @Test
    public void getAddressForContext_throwsOnNonExistentContext() {
        IllegalStateException thrown =
                expectThrows(IllegalStateException.class,
                        () -> mTestAudioZone.getAddressForContext(MUSIC));

        assertThat(thrown).hasMessageThat().contains("Could not find output device in zone");
    }

    @Test
    public void findActiveContextsFromPlaybackConfigurations_returnsAllActiveContext() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(mMockVoiceGroup);
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new Builder().setUsage(USAGE_MEDIA).setDeviceAddress(MUSIC_ADDRESS).build(),
                new Builder().setUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .setDeviceAddress(NAV_ADDRESS).build()
        );

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).containsExactly(MUSIC, NAVIGATION);
    }

    @Test
    public void findActiveContextsFromPlaybackConfigurations_returnsNoMatchingContexts() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(mMockVoiceGroup);
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new Builder().setUsage(USAGE_ASSISTANT)
                        .setDeviceAddress(ANNOUNCEMENT_ADDRESS).build(),
                new Builder().setUsage(USAGE_ALARM)
                        .setDeviceAddress(ALARM_ADDRESS).build()
        );

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).isEmpty();
    }

    @Test
    public void findActiveContextsFromPlaybackConfigurations_withMultiDevices_returnsContexts() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(VOICE_COMMAND, ASSISTANT_ADDRESS)
                .addDeviceAddressAndContexts(ALARM, ALARM_ADDRESS)
                .build());
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new Builder().setUsage(USAGE_ASSISTANT)
                        .setDeviceAddress(ASSISTANT_ADDRESS).build(),
                new Builder().setUsage(USAGE_ALARM)
                        .setDeviceAddress(ALARM_ADDRESS).build()
        );

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).containsExactly(VOICE_COMMAND, ALARM);
    }

    @Test
    public void
            findActiveContextsFromPlaybackConfigurations_deviceWithMultiContext_returnsContext() {
        mTestAudioZone.addVolumeGroup(new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(VOICE_COMMAND, ASSISTANT_ADDRESS)
                .addDeviceAddressAndContexts(ALARM, ASSISTANT_ADDRESS)
                .addDeviceAddressAndContexts(MUSIC, ASSISTANT_ADDRESS)
                .build());
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new Builder().setUsage(USAGE_ALARM)
                        .setDeviceAddress(ASSISTANT_ADDRESS).build(),
                new Builder().setUsage(USAGE_MEDIA)
                        .setDeviceAddress(MUSIC_ADDRESS).build()
        );

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).containsExactly(ALARM);
    }

    @Test
    public void
            findActiveContextsFromPlaybackConfigurations_withNonMatchingContext_returnsContext() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(VOICE_COMMAND, ASSISTANT_ADDRESS)
                .build());
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new Builder().setUsage(USAGE_ASSISTANCE_SONIFICATION)
                        .setDeviceAddress(ASSISTANT_ADDRESS).build()
        );

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).containsExactly(SYSTEM_SOUND);
    }

    @Test
    public void findActiveContextsFromPlaybackConfigurations_withMultiGroupMatch_returnsContexts() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(new VolumeGroupBuilder()
                .addDeviceAddressAndContexts(VOICE_COMMAND, ASSISTANT_ADDRESS)
                .addDeviceAddressAndContexts(ALARM, ALARM_ADDRESS)
                .build());
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of(
                new Builder().setUsage(USAGE_ALARM)
                        .setDeviceAddress(ALARM_ADDRESS).build(),
                new Builder().setUsage(USAGE_MEDIA)
                        .setDeviceAddress(MUSIC_ADDRESS).build()
        );

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).containsExactly(ALARM, USAGE_MEDIA);
    }

    @Test
    public void
            findActiveContextsFromPlaybackConfigurations_onEmptyConfigurations_returnsNoContexts() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(mMockVoiceGroup);
        List<AudioPlaybackConfiguration> activeConfigurations = ImmutableList.of();

        List<Integer> activeContexts = mTestAudioZone
                .findActiveContextsFromPlaybackConfigurations(activeConfigurations);

        assertThat(activeContexts).isEmpty();
    }

    @Test
    public void findActiveContextsFromPlaybackConfigurations_onNullConfigurations_fails() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        mTestAudioZone.addVolumeGroup(mMockNavGroup);
        mTestAudioZone.addVolumeGroup(mMockVoiceGroup);
        List<AudioPlaybackConfiguration> activeConfigurations = null;

        assertThrows(NullPointerException.class,
                () -> mTestAudioZone
                        .findActiveContextsFromPlaybackConfigurations(activeConfigurations));
    }

    @Test
    public void isAudioDeviceInfoValidForZone_withNullAudioDeviceInfo_returnsFalse() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);

        assertThat(mTestAudioZone.isAudioDeviceInfoValidForZone(null)).isFalse();
    }

    @Test
    public void isAudioDeviceInfoValidForZone_withNullDeviceAddress_returnsFalse() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        AudioDeviceInfo nullAddressDeviceInfo = Mockito.mock(AudioDeviceInfo.class);
        when(nullAddressDeviceInfo.getAddress()).thenReturn(null);

        assertThat(mTestAudioZone.isAudioDeviceInfoValidForZone(nullAddressDeviceInfo)).isFalse();
    }

    @Test
    public void isAudioDeviceInfoValidForZone_withEmptyDeviceAddress_returnsFalse() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        AudioDeviceInfo nullAddressDeviceInfo = Mockito.mock(AudioDeviceInfo.class);
        when(nullAddressDeviceInfo.getAddress()).thenReturn("");

        assertThat(mTestAudioZone.isAudioDeviceInfoValidForZone(nullAddressDeviceInfo)).isFalse();
    }

    @Test
    public void isAudioDeviceInfoValidForZone_withDeviceAddressNotInZone_returnsFalse() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        AudioDeviceInfo nullAddressDeviceInfo = Mockito.mock(AudioDeviceInfo.class);
        when(nullAddressDeviceInfo.getAddress()).thenReturn(VOICE_ADDRESS);

        assertThat(mTestAudioZone.isAudioDeviceInfoValidForZone(nullAddressDeviceInfo)).isFalse();
    }

    @Test
    public void isAudioDeviceInfoValidForZone_withDeviceAddressInZone_returnsTrue() {
        mTestAudioZone.addVolumeGroup(mMockMusicGroup);
        AudioDeviceInfo nullAddressDeviceInfo = Mockito.mock(AudioDeviceInfo.class);
        when(nullAddressDeviceInfo.getAddress()).thenReturn(MUSIC_ADDRESS);

        assertThat(mTestAudioZone.isAudioDeviceInfoValidForZone(nullAddressDeviceInfo)).isTrue();
    }

    private static class VolumeGroupBuilder {
        private SparseArray<String> mDeviceAddresses = new SparseArray<>();

        VolumeGroupBuilder addDeviceAddressAndContexts(@AudioContext int context, String address) {
            mDeviceAddresses.put(context, address);
            return this;
        }

        CarVolumeGroup build() {
            CarVolumeGroup carVolumeGroup = mock(CarVolumeGroup.class);
            Map<String, ArrayList<Integer>> addressToContexts = new HashMap<>();
            @AudioContext int[] contexts = new int[mDeviceAddresses.size()];

            for (int index = 0; index < mDeviceAddresses.size(); index++) {
                @AudioContext int context = mDeviceAddresses.keyAt(index);
                String address = mDeviceAddresses.get(context);
                when(carVolumeGroup.getAddressForContext(context)).thenReturn(address);
                if (!addressToContexts.containsKey(address)) {
                    addressToContexts.put(address, new ArrayList<>());
                }
                addressToContexts.get(address).add(context);
                contexts[index] = context;
            }

            when(carVolumeGroup.getContexts()).thenReturn(contexts);

            for (String address : addressToContexts.keySet()) {
                when(carVolumeGroup.getContextsForAddress(address))
                        .thenReturn(ImmutableList.copyOf(addressToContexts.get(address)));
            }
            when(carVolumeGroup.getAddresses())
                    .thenReturn(ImmutableList.copyOf(addressToContexts.keySet()));
            return carVolumeGroup;
        }

    }

    private static class Builder {
        private @AudioAttributes.AttributeUsage int mUsage = USAGE_MEDIA;
        private boolean mIsActive = true;
        private String mDeviceAddress = "";

        Builder setUsage(@AudioAttributes.AttributeUsage int usage) {
            mUsage = usage;
            return this;
        }

        Builder setDeviceAddress(String deviceAddress) {
            mDeviceAddress = deviceAddress;
            return this;
        }

        Builder setInactive() {
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
