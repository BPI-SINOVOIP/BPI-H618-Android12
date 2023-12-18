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

package com.android.cts.verifier.tv.audio;

import static android.media.AudioFormat.*;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.view.View;

import com.android.cts.verifier.R;
import com.android.cts.verifier.tv.TestSequence;
import com.android.cts.verifier.tv.TestStepBase;
import com.android.cts.verifier.tv.TvAppVerifierActivity;

import com.google.common.collect.ImmutableList;

import java.util.ArrayList;
import java.util.List;

/**
 * Test to verify Audio Capabilities APIs are correctly implemented.
 *
 * <p>This test checks if <br>
 * <a
 * href="https://developer.android.com/reference/android/media/AudioTrack#isDirectPlaybackSupported(android.media.AudioFormat,%20android.media.AudioAttributes)">
 * AudioTrack.isDirectPlaybackSupported()</a> <br>
 * return correct results when
 *
 * <ol>
 *   <li>No receiver or soundbar is connected
 *   <li>Receiver or soundbar is connected
 * </ol>
 */
public class AudioCapabilitiesTestActivity extends TvAppVerifierActivity
        implements View.OnClickListener {

    private final ImmutableList<AudioFormat> ATMOS_FORMATS =
            ImmutableList.of(
                    // EAC3_JOC formats - ignoring channel count for Atmos
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 1),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 2),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 3),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 4),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 5),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 6),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 7),
                    makeAudioFormat(ENCODING_E_AC3_JOC, 44100, 8));


    private TestSequence mTestSequence;
    private View mSupportDolbyAtmosYesItem;
    private View mSupportDolbyAtmosNoItem;

    @Override
    protected void setInfoResources() {
        setInfoResources(
                R.string.tv_audio_capabilities_test, R.string.tv_audio_capabilities_test_info, -1);
    }

    @Override
    public void onClick(View v) {
        if (containsButton(mSupportDolbyAtmosYesItem, v)) {
            setPassState(mSupportDolbyAtmosYesItem, true);
            setButtonEnabled(mSupportDolbyAtmosNoItem, false);
            List<TestStepBase> testSteps = new ArrayList<>();
            testSteps.add(new TvTestStep(this));
            testSteps.add(new ReceiverTestStep(this));
            testSteps.add(new TvTestStep(this));
            mTestSequence = new TestSequence(this, testSteps);
            mTestSequence.init();
            return;
        } else if (containsButton(mSupportDolbyAtmosNoItem, v)) {
            setPassState(mSupportDolbyAtmosYesItem, true);
            setButtonEnabled(mSupportDolbyAtmosNoItem, false);
            List<TestStepBase> testSteps = new ArrayList<>();
            testSteps.add(new TvTestStep(this));
            mTestSequence = new TestSequence(this, testSteps);
            mTestSequence.init();
            return;
        }
    }

    @Override
    protected void createTestItems() {
        mSupportDolbyAtmosYesItem =
                createAndAttachUserItem(
                        R.string.tv_audio_capabilities_atmos_supported, R.string.tv_yes, this);
        setButtonEnabled(mSupportDolbyAtmosYesItem, true);
        mSupportDolbyAtmosNoItem = createAndAttachButtonItem(R.string.tv_no, this);
        setButtonEnabled(mSupportDolbyAtmosNoItem, true);
    }

    private class TvTestStep extends TestStep {

        public TvTestStep(TvAppVerifierActivity context) {
            super(
                    context,
                    getString(R.string.tv_audio_capabilities_no_receiver_connected),
                    R.string.tv_start_test);
        }

        @Override
        public void runTest() {
            AudioAttributes audioAttributes =
                    new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_MEDIA)
                            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                            .build();

            getAsserter()
                    .withMessage("AudioTrack.isDirectPlaybackSupported is expected to return true"
                            + " for PCM 2 channel")
                    .that(AudioTrack.isDirectPlaybackSupported(
                            makeAudioFormat(ENCODING_PCM_16BIT, 44100, 2), audioAttributes))
                    .isTrue();
        }
    }

    private class ReceiverTestStep extends TestStep {

        public ReceiverTestStep(TvAppVerifierActivity context) {
            super(
                    context,
                    getString(R.string.tv_audio_capabilities_receiver_connected),
                    R.string.tv_start_test);
        }

        @Override
        public void runTest() {
            AudioAttributes audioAttributes =
                    new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_MEDIA)
                            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                            .build();

            getAsserter()
                    .withMessage("AudioTrack.isDirectPlaybackSupported is expected to return true"
                            + " for PCM 6 channel")
                    .that(AudioTrack.isDirectPlaybackSupported(
                            makeAudioFormat(ENCODING_PCM_16BIT, 44100, 6), audioAttributes))
                    .isTrue();

            getAsserter()
                    .withMessage("AudioTrack.isDirectPlaybackSupported is expected to return true "
                            + "for EAC3 6 channel")
                    .that(AudioTrack.isDirectPlaybackSupported(
                            makeAudioFormat(ENCODING_E_AC3, 44100, 6), audioAttributes))
                    .isTrue();

            ImmutableList.Builder<String> actualAtmosFormatStrings = ImmutableList.builder();
            for (AudioFormat audioFormat : ATMOS_FORMATS) {
                if (AudioTrack.isDirectPlaybackSupported(audioFormat, audioAttributes)) {
                    actualAtmosFormatStrings.add(toStr(audioFormat));
                }
            }

            // check that Atmos should be supported
            getAsserter()
                    .withMessage(
                            "AudioTrack.isDirectPlaybackSupported is expected to return true for"
                                + " EAC3_JOC")
                    .that(actualAtmosFormatStrings.build())
                    .isNotEmpty();
        }
    }

    /** Returns channel mask for {@code channelCount}. */
    private static int channelCountToMask(int channelCount) {
        switch (channelCount) {
            case 1:
                return CHANNEL_OUT_MONO;
            case 2:
                return CHANNEL_OUT_STEREO;
            case 3:
                return CHANNEL_OUT_STEREO | CHANNEL_OUT_FRONT_CENTER;
            case 4:
                return CHANNEL_OUT_QUAD;
            case 5:
                return CHANNEL_OUT_QUAD | CHANNEL_OUT_FRONT_CENTER;
            case 6:
                return CHANNEL_OUT_5POINT1;
            case 7:
                return CHANNEL_OUT_5POINT1 | CHANNEL_OUT_BACK_CENTER;
            case 8:
                return CHANNEL_OUT_7POINT1_SURROUND;
            default:
                return 0;
        }
    }

    /** Returns a displayable String message for {@code encodingCode}. */
    private static String encodingToString(int encodingCode) {
        switch (encodingCode) {
            case ENCODING_E_AC3:
                return "E_AC3";
            case ENCODING_E_AC3_JOC:
                return "E_AC3_JOC";
            case ENCODING_PCM_16BIT:
                return "PCM_16BIT";
            default:
                return "Unknown Encoding";
        }
    }

    /** Converts Audio format object to string */
    private static String toStr(AudioFormat audioFormat) {
        return String.format(
                "{encoding: %s, channel count: %d}",
                encodingToString(audioFormat.getEncoding()), audioFormat.getChannelCount());
    }

    private static AudioFormat makeAudioFormat(int encoding, int sampleRate, int channelCount) {
        return new AudioFormat.Builder()
                .setEncoding(encoding)
                .setSampleRate(sampleRate)
                .setChannelMask(channelCountToMask(channelCount))
                .build();
    }

    private static boolean containsButton(View item, View button) {
        return item == null ? false : item.findViewById(R.id.user_action_button) == button;
    }
}
