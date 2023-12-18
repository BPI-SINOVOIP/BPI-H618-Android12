/*
 * Copyright 2020 The Android Open Source Project
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
package org.hyphonate.megaaudio.recorder;

import android.media.AudioFormat;
import android.media.AudioRecord;

import org.hyphonate.megaaudio.common.StreamBase;

public abstract class Recorder extends StreamBase {
    protected AudioSinkProvider mSinkProvider;

    // This value is to indicate that no explicit call to set an input preset in the builder
    // will be made.
    // Constants can be found here:
    // https://developer.android.com/reference/android/media/MediaRecorder.AudioSource
    public static final int INPUT_PRESET_NONE = -1;

    public Recorder(AudioSinkProvider sinkProvider) {
        mSinkProvider = sinkProvider;
    }
    public abstract void setInputPreset(int preset);

    /*
     * State
     */
    public abstract boolean isRecording();

    /*
     * Utilities
     */
    public static final int AUDIO_CHANNEL_COUNT_MAX = 30;

    public static final int AUDIO_CHANNEL_REPRESENTATION_POSITION   = 0x0;
    public static final int AUDIO_CHANNEL_REPRESENTATION_INDEX      = 0x2;

    //
    // Attributes
    //
    // This needs to be static because it is called before creating the Recorder subclass
    public static int calcMinBufferFrames(int channelCount, int sampleRate) {
        int channelMask = Recorder.channelCountToChannelMask(channelCount);
        int bufferSizeInBytes =
                AudioRecord.getMinBufferSize (sampleRate,
                        channelMask,
                        AudioFormat.ENCODING_PCM_FLOAT);
        return bufferSizeInBytes / sampleSizeInBytes(AudioFormat.ENCODING_PCM_FLOAT);
    }

    /*
     * Channel Utils
     */
    // TODO - Consider moving these into a "Utilities" library
//    /**
//     * @param chanCount The number of channels for which to generate an index mask.
//     * @return  A channel index mask corresponding to the supplied channel count.
//     *
//     * @note The generated index mask has active channels from 0 to chanCount - 1
//     */
//    public static int countToIndexMask(int chanCount) {
//        return  (1 << chanCount) - 1;
//    }

    /* Not part of public API */
    private static int audioChannelMaskFromRepresentationAndBits(int representation, int bits)
    {
        return ((representation << AUDIO_CHANNEL_COUNT_MAX) | bits);
    }

    /* Derive a channel mask for index assignment from a channel count.
     * Returns the matching channel mask,
     * or AUDIO_CHANNEL_NONE if the channel count is zero,
     * or AUDIO_CHANNEL_INVALID if the channel count exceeds AUDIO_CHANNEL_COUNT_MAX.
     */
    private static int audioChannelMaskForIndexAssignmentFromCount(int channel_count)
    {
        if (channel_count == 0) {
            return 0; // AUDIO_CHANNEL_NONE
        }
        if (channel_count > AUDIO_CHANNEL_COUNT_MAX) {
            return AudioFormat.CHANNEL_INVALID;
        }
        int bits = (1 << channel_count) - 1;
        return audioChannelMaskFromRepresentationAndBits(AUDIO_CHANNEL_REPRESENTATION_INDEX, bits);
    }

    /**
     * @param channelCount  The number of channels for which to generate an input position mask.
     * @return An input channel-position mask corresponding the supplied number of channels.
     */
    public static int channelCountToChannelMask(int channelCount) {
        int bits;
        switch (channelCount) {
            case 1:
                bits = AudioFormat.CHANNEL_IN_MONO;
                break;

            case 2:
                bits = AudioFormat.CHANNEL_IN_STEREO;
                break;

            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                // FIXME FCC_8
                return audioChannelMaskForIndexAssignmentFromCount(channelCount);

            default:
                return AudioFormat.CHANNEL_INVALID;
        }

        return audioChannelMaskFromRepresentationAndBits(AUDIO_CHANNEL_REPRESENTATION_POSITION, bits);
    }

}
