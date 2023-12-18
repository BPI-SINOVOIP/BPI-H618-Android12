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

package com.android.media.audiotestharness.client.core;

import com.android.media.audiotestharness.common.Defaults;
import com.android.media.audiotestharness.proto.AudioFormatOuterClass.AudioFormat;

import java.io.InputStream;

/**
 * {@link InputStream} that provides access to raw audio samples returned by a Audio Capture Session
 * while also providing access to helper methods to interact with the session.
 */
public abstract class AudioCaptureStream extends InputStream {

    /**
     * Returns the {@link AudioFormat} corresponding to this {@link AudioCaptureStream}, thus the
     * raw data exposed by this stream will be raw PCM samples matching this format.
     */
    public AudioFormat getAudioFormat() {
        return Defaults.AUDIO_FORMAT;
    }
}
