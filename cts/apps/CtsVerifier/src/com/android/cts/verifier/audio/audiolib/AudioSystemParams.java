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

package com.android.cts.verifier.audio.audiolib;

import android.content.Context;
import android.media.AudioManager;

public class AudioSystemParams {
    // This value will be calculated in init()
    private int mSystemSampleRate;

    // This value will be calculated in init()
    private int mSystemBufferFrames;

    // The system burst buffer size as reported by AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER
    // This value will be calculated in init()
    private int mSystemBurstFrames;

    public void init(Context context) {
        AudioManager audioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);

        String framesPerBuffer = audioManager.getProperty(
                AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        mSystemBurstFrames = Integer.parseInt(framesPerBuffer, 10);

        mSystemBufferFrames = mSystemBurstFrames;

        mSystemSampleRate = Integer.parseInt(
                audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
    }

    public int getSystemSampleRate() {
        return mSystemSampleRate;
    }

    public int getSystemBufferFrames() {
        return mSystemBufferFrames;
    }

    public int getSystemBurstFrames()  {
        return mSystemBurstFrames;
    }
}