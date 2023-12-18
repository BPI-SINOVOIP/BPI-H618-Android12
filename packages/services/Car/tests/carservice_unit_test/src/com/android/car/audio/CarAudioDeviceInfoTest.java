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

import static android.media.AudioFormat.CHANNEL_OUT_MONO;
import static android.media.AudioFormat.CHANNEL_OUT_QUAD;
import static android.media.AudioFormat.CHANNEL_OUT_STEREO;
import static android.media.AudioFormat.ENCODING_PCM_16BIT;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.media.AudioDeviceInfo;
import android.media.AudioDevicePort;
import android.media.AudioGain;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class CarAudioDeviceInfoTest {

    private static final String TEST_ADDRESS = "test address";
    private static final int MIN_GAIN = 0;
    private static final int MAX_GAIN = 100;
    private static final int DEFAULT_GAIN = 50;
    private static final int STEP_SIZE = 2;

    @Test
    public void constructor_requiresNonNullGain() {
        AudioDeviceInfo audioDeviceInfo = mock(AudioDeviceInfo.class);
        when(audioDeviceInfo.getPort()).thenReturn(null);

        assertThrows(NullPointerException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void constructor_requiresJointModeGain() {
        AudioGain gainWithChannelMode = new GainBuilder().setMode(AudioGain.MODE_CHANNELS).build();
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo(
                new AudioGain[]{gainWithChannelMode});

        assertThrows(NullPointerException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void constructor_requiresMaxGainLargerThanMin() {
        AudioGain gainWithChannelMode = new GainBuilder().setMaxValue(10).setMinValue(20).build();
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo(
                new AudioGain[]{gainWithChannelMode});

        assertThrows(IllegalArgumentException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void constructor_requiresDefaultGainLargerThanMin() {
        AudioGain gainWithChannelMode = new GainBuilder().setDefaultValue(10).setMinValue(
                20).build();
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo(
                new AudioGain[]{gainWithChannelMode});

        assertThrows(IllegalArgumentException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void constructor_requiresDefaultGainSmallerThanMax() {
        AudioGain gainWithChannelMode = new GainBuilder().setDefaultValue(15).setMaxValue(
                10).build();
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo(
                new AudioGain[]{gainWithChannelMode});

        assertThrows(IllegalArgumentException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void constructor_requiresGainStepSizeFactorOfRange() {
        AudioGain gainWithChannelMode = new GainBuilder().setStepSize(7).build();
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo(
                new AudioGain[]{gainWithChannelMode});

        assertThrows(IllegalArgumentException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void constructor_requiresGainStepSizeFactorOfRangeToDefault() {
        AudioGain gainWithChannelMode = new GainBuilder().setStepSize(7).setMaxValue(98).build();
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo(
                new AudioGain[]{gainWithChannelMode});

        assertThrows(IllegalArgumentException.class, () -> new CarAudioDeviceInfo(audioDeviceInfo));
    }

    @Test
    public void getSampleRate_withMultipleSampleRates_returnsMax() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        int[] sampleRates = new int[]{48000, 96000, 16000, 8000};
        when(audioDeviceInfo.getSampleRates()).thenReturn(sampleRates);
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        int sampleRate = info.getSampleRate();

        assertThat(sampleRate).isEqualTo(96000);
    }

    @Test
    public void getSampleRate_withNullSampleRate_returnsDefault() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        when(audioDeviceInfo.getSampleRates()).thenReturn(null);
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        int sampleRate = info.getSampleRate();

        assertThat(sampleRate).isEqualTo(CarAudioDeviceInfo.DEFAULT_SAMPLE_RATE);
    }

    @Test
    public void getAudioDevicePort_returnsValueFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getAudioDevicePort()).isEqualTo(audioDeviceInfo.getPort());
    }

    @Test
    public void getAddress_returnsValueFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getAddress()).isEqualTo(TEST_ADDRESS);
    }

    @Test
    public void getMaxGain_returnsValueFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getMaxGain()).isEqualTo(MAX_GAIN);
    }

    @Test
    public void getMinGain_returnsValueFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getMinGain()).isEqualTo(MIN_GAIN);
    }

    @Test
    public void getDefaultGain_returnsValueFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getDefaultGain()).isEqualTo(DEFAULT_GAIN);
    }

    @Test
    public void getStepValue_returnsValueFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getStepValue()).isEqualTo(STEP_SIZE);
    }

    @Test
    public void getChannelCount_withNoChannelMasks_returnsOne() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        int channelCount = info.getChannelCount();

        assertThat(channelCount).isEqualTo(1);
    }

    @Test
    public void getChannelCount_withMultipleChannels_returnsHighestCount() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        when(audioDeviceInfo.getChannelMasks()).thenReturn(new int[]{CHANNEL_OUT_STEREO,
                CHANNEL_OUT_QUAD, CHANNEL_OUT_MONO});
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        int channelCount = info.getChannelCount();

        assertThat(channelCount).isEqualTo(4);
    }

    @Test
    public void getAudioDeviceInfo_returnsConstructorParameter() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getAudioDeviceInfo()).isEqualTo(audioDeviceInfo);
    }

    @Test
    public void getEncodingFormat_returnsPCM16() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);

        assertThat(info.getEncodingFormat()).isEqualTo(ENCODING_PCM_16BIT);
    }

    @Test
    public void getAudioGain_returnsGainFromDeviceInfo() {
        AudioDeviceInfo audioDeviceInfo = getMockAudioDeviceInfo();
        CarAudioDeviceInfo info = new CarAudioDeviceInfo(audioDeviceInfo);
        AudioGain expectedGain = audioDeviceInfo.getPort().gains()[0];

        assertThat(info.getAudioGain()).isEqualTo(expectedGain);
    }

    private AudioDeviceInfo getMockAudioDeviceInfo() {
        AudioGain mockGain = new GainBuilder().build();
        return getMockAudioDeviceInfo(new AudioGain[]{mockGain});
    }

    private AudioDeviceInfo getMockAudioDeviceInfo(AudioGain[] gains) {
        AudioDeviceInfo mockInfo = mock(AudioDeviceInfo.class);
        AudioDevicePort mockPort = mock(AudioDevicePort.class);
        when(mockInfo.getPort()).thenReturn(mockPort);
        when(mockPort.gains()).thenReturn(gains);
        when(mockInfo.getAddress()).thenReturn(TEST_ADDRESS);
        return mockInfo;
    }

    private static class GainBuilder {
        int mMode = AudioGain.MODE_JOINT;
        int mMaxValue = MAX_GAIN;
        int mMinValue = MIN_GAIN;
        int mDefaultValue = DEFAULT_GAIN;
        int mStepSize = STEP_SIZE;

        GainBuilder setMode(int mode) {
            mMode = mode;
            return this;
        }

        GainBuilder setMaxValue(int maxValue) {
            mMaxValue = maxValue;
            return this;
        }

        GainBuilder setMinValue(int minValue) {
            mMinValue = minValue;
            return this;
        }

        GainBuilder setDefaultValue(int defaultValue) {
            mDefaultValue = defaultValue;
            return this;
        }

        GainBuilder setStepSize(int stepSize) {
            mStepSize = stepSize;
            return this;
        }

        AudioGain build() {
            AudioGain mockGain = mock(AudioGain.class);
            when(mockGain.mode()).thenReturn(mMode);
            when(mockGain.maxValue()).thenReturn(mMaxValue);
            when(mockGain.minValue()).thenReturn(mMinValue);
            when(mockGain.defaultValue()).thenReturn(mDefaultValue);
            when(mockGain.stepValue()).thenReturn(mStepSize);
            return mockGain;
        }
    }
}
