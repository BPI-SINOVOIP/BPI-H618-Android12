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
package org.hyphonate.megaaudio.player;

import android.media.AudioTimestamp;

public class OboePlayer extends Player {
    boolean mPlaying;

    private int mPlayerSubtype;
    private long mNativePlayer;

    private AudioSource mAudioSource;

    public OboePlayer(AudioSourceProvider sourceProvider, int playerSubtype) {
        super(sourceProvider);

        mPlayerSubtype = playerSubtype;
        NativeAudioSource nativeAudioSource = mSourceProvider.getNativeSource();
        if (nativeAudioSource != null) {
            mAudioSource = nativeAudioSource;
            mNativePlayer = allocNativePlayer(nativeAudioSource.getNativeObject(), mPlayerSubtype);
        } else {
            mAudioSource = mSourceProvider.getJavaSource();
            mNativePlayer = allocNativePlayer(
                    JavaSourceProxy.allocNativeSource(mAudioSource), mPlayerSubtype);
        }
    }

    @Override
    public int getNumBufferFrames() {
        return getBufferFrameCountN(mNativePlayer);
    }

    @Override
    public int getRoutedDeviceId() {
        return getRoutedDeviceIdN(mNativePlayer);
    }

    @Override
    public AudioSource getAudioSource() {
        return mAudioSource;
    }

    @Override
    public boolean isPlaying() { return mPlaying; }

    @Override
    public int setupStream(int channelCount, int sampleRate, int numBurstFrames) {
        mChannelCount = channelCount;
        mSampleRate = sampleRate;
        return setupStreamN(
                mNativePlayer, channelCount, sampleRate,
                mRouteDevice == null ? -1 : mRouteDevice.getId());
    }

    @Override
    public int teardownStream() {
        int errCode = teardownStreamN(mNativePlayer);

        mChannelCount = 0;
        mSampleRate = 0;

        return errCode;
    }

    @Override
    public int startStream() {
        return startStreamN(mNativePlayer, mPlayerSubtype);
    }

    @Override
    public int stopStream() {
        mPlaying = false;

        return stopN(mNativePlayer);
    }

    /**
     * Gets a timestamp from the audio stream
     * @param timestamp
     * @return
     */
    public boolean getTimestamp(AudioTimestamp timestamp) {
        return getTimestampN(mNativePlayer, timestamp);
    }

    private native long allocNativePlayer(long nativeSource, int playerSubtype);

    private native int setupStreamN(long nativePlayer, int channelCount, int sampleRate, int routeDeviceId);
    private native int teardownStreamN(long nativePlayer);

    private native int startStreamN(long nativePlayer, int playerSubtype);
    private native int stopN(long nativePlayer);

    private native int getBufferFrameCountN(long mNativePlayer);

    private native int getRoutedDeviceIdN(long nativePlayer);

    private native boolean getTimestampN(long nativePlayer, AudioTimestamp timestamp);
}
