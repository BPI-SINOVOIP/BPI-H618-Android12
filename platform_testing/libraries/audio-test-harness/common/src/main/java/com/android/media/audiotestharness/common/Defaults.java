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

package com.android.media.audiotestharness.common;

import com.android.media.audiotestharness.proto.AudioDeviceOuterClass.AudioDevice;
import com.android.media.audiotestharness.proto.AudioFormatOuterClass.AudioFormat;

/**
 * Contains all the defaults for the Audio Test Harness system shared between the client and server
 * libraries.
 */
public final class Defaults {

    /**
     * The default {@link AudioFormat} used for capture.
     *
     * <p>Currently set to CD format and quality, thus audio samples are captured as single-channel,
     * signed, little-endian samples at 16-bit 44100hz.
     */
    public static final AudioFormat AUDIO_FORMAT =
            AudioFormat.newBuilder()
                    .setSampleRate(44100.0f)
                    .setSampleSizeBits(16)
                    .setChannels(1)
                    .setSigned(true)
                    .setBigEndian(false)
                    .build();

    /**
     * The default {@link AudioDevice} used for capture.
     *
     * <p>Currently set to a Dayton UMM-6 reference microphone which are commonly used by existing
     * audio boxes.
     */
    public static final AudioDevice AUDIO_DEVICE =
            AudioDevice.newBuilder()
                    .setName("UMM6 [plughw:0,0]")
                    .addCapabilities(AudioDevice.Capability.CAPTURE)
                    .build();
}
