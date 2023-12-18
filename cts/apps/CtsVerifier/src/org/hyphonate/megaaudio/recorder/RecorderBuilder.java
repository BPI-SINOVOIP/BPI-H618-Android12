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

import android.media.AudioDeviceInfo;

import org.hyphonate.megaaudio.common.BuilderBase;

public class RecorderBuilder extends BuilderBase {

    private AudioSinkProvider mSinkProvider;

    private int mInputPreset = Recorder.INPUT_PRESET_NONE;

    public RecorderBuilder() {

    }

    public RecorderBuilder setRecorderType(int type) {
        mType = type;
        return this;
    }

    public RecorderBuilder setAudioSinkProvider(AudioSinkProvider sinkProvider) {
        mSinkProvider = sinkProvider;
        return this;
    }

    public RecorderBuilder setInputPreset(int inputPreset) {
        mInputPreset = inputPreset;
        return this;
    }

    public Recorder build() throws BadStateException {
        if (mSinkProvider == null) {
            throw new BadStateException();
        }

        Recorder recorder = null;
        int playerType = mType & TYPE_MASK;
        switch (playerType) {
            case TYPE_NONE:
                // NOP
                break;

            case TYPE_JAVA:
                recorder = new JavaRecorder(mSinkProvider);
                break;

            case TYPE_OBOE:{
                int recorderSubType = mType & SUB_TYPE_MASK;
                recorder = new OboeRecorder(mSinkProvider, recorderSubType);
            }
            break;

            default:
                throw new BadStateException();
        }

        return recorder;
    }

    public class BadStateException extends Throwable {

    }
}
