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

import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioTimestamp;
import android.media.AudioTrack;
import android.util.Log;

import org.hyphonate.megaaudio.common.StreamBase;

/**
 * Implementation of abstract Player class implemented for the Android Java-based audio playback
 * API, i.e. AudioTrack.
 */
public class JavaPlayer extends Player {
    @SuppressWarnings("unused") private static String TAG = JavaPlayer.class.getSimpleName();
    @SuppressWarnings("unused") private static final boolean LOG = false;

    /*
     * Player infrastructure
     */
    /* The AudioTrack for playing the audio stream */
    private AudioTrack mAudioTrack;

    private AudioSource mAudioSource;

    // Playback state
    /** <code>true</code> if currently playing audio data */
    private boolean mPlaying;

    /*
     * Data buffers
     */
    /** Number of FRAMES of audio data in a burst buffer */
    private int mNumBufferFrames;

    /** The Burst Buffer. This is the buffer we fill with audio and feed into the AudioTrack. */
    private float[] mAudioBuffer;

    // Player-specific extension
    public AudioTrack getAudioTrack() { return mAudioTrack; }

    public JavaPlayer(AudioSourceProvider sourceProvider) {
        super(sourceProvider);
        mNumBufferFrames = -1;   // TODO need error defines
    }

    @Override
    public AudioSource getAudioSource() {
        return mAudioSource;
    }

    //
    // Status
    //
    @Override
    public boolean isPlaying() {
        return mPlaying;
    }

    /**
     * Allocates the array for the burst buffer.
     */
    private void allocBurstBuffer() {
        // pad it by 1 frame. This allows some sources to not have to worry about
        // handling the end-of-buffer edge case. i.e. a "Guard Point" for interpolation.
        mAudioBuffer = new float[(mNumBufferFrames + 1) * mChannelCount];
    }

    //
    // Attributes
    //
    /**
     * @return The number of frames of audio data contained in the internal buffer.
     */
    @Override
    public int getNumBufferFrames() {
        return mNumBufferFrames;
    }

    @Override
    public int getRoutedDeviceId() {
        if (mAudioTrack != null) {
            AudioDeviceInfo routedDevice = mAudioTrack.getRoutedDevice();
            return routedDevice != null ? routedDevice.getId() : ROUTED_DEVICE_ID_INVALID;
        } else {
            return ROUTED_DEVICE_ID_INVALID;
        }
    }

    /*
     * State
     */
    @Override
    public int setupStream(int channelCount, int sampleRate, int numBufferFrames) {
        if (LOG) {
            Log.i(TAG, "setupStream(chans:" + channelCount + ", rate:" + sampleRate +
                    ", frames:" + numBufferFrames);
        }

        mChannelCount = channelCount;
        mSampleRate = sampleRate;
        mNumBufferFrames = numBufferFrames;

        mAudioSource = mSourceProvider.getJavaSource();
        mAudioSource.init(mNumBufferFrames, mChannelCount);

        try {
            int bufferSizeInBytes = mNumBufferFrames * mChannelCount
                    * sampleSizeInBytes(AudioFormat.ENCODING_PCM_FLOAT);
            mAudioTrack = new AudioTrack.Builder()
                    .setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)
                    .setAudioFormat(new AudioFormat.Builder()
                            .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                            .setSampleRate(mSampleRate)
                            .setChannelIndexMask(StreamBase.channelCountToIndexMask(mChannelCount))
                            // .setChannelMask(channelMask)
                            .build())
                    .setBufferSizeInBytes(bufferSizeInBytes)
                    .build();

            allocBurstBuffer();
            mAudioTrack.setPreferredDevice(mRouteDevice);
        }  catch (UnsupportedOperationException ex) {
            if (LOG) {
                Log.i(TAG, "Couldn't open AudioTrack: " + ex);
            }
            mAudioTrack = null;
            return ERROR_UNSUPPORTED;
        }

        return OK;
    }

    @Override
    public int teardownStream() {
        stopStream();

        waitForStreamThreadToExit();

        if (mAudioTrack != null) {
            mAudioTrack.release();
            mAudioTrack = null;
        }

        mChannelCount = 0;
        mSampleRate = 0;

        //TODO - Retrieve errors from above
        return OK;
    }

    /**
     * Allocates the underlying AudioTrack and begins Playback.
     * @return True if the stream is successfully started.
     *
     * This method returns when the start operation is complete, but before the first
     * call to the AudioSource.pull() method.
     */
    @Override
    public int startStream() {
        if (mAudioTrack == null) {
            return ERROR_INVALID_STATE;
        }
        waitForStreamThreadToExit(); // just to be sure.

        mStreamThread = new Thread(new StreamPlayerRunnable(), "StreamPlayer Thread");
        mPlaying = true;
        mStreamThread.start();

        return OK;
    }

    /**
     * Marks the stream for stopping on the next callback from the underlying system.
     *
     * Returns immediately, though a call to AudioSource.pull() may be in progress.
     */
    @Override
    public int stopStream() {
        mPlaying = false;
        return OK;
    }

    /**
     * Gets a timestamp from the audio stream
     * @param timestamp
     * @return
     */
    public boolean getTimestamp(AudioTimestamp timestamp) {
        return mPlaying ? mAudioTrack.getTimestamp(timestamp) : false;
    }

    //
    // StreamPlayerRunnable
    //
    /**
     * Implements the <code>run</code> method for the playback thread.
     * Gets initial audio data and starts the AudioTrack. Then continuously provides audio data
     * until the flag <code>mPlaying</code> is set to false (in the stop() method).
     */
    private class StreamPlayerRunnable implements Runnable {
        @Override
        public void run() {
            final int numBufferSamples = mNumBufferFrames * mChannelCount;

            mAudioTrack.play();
            while (mPlaying) {
                mAudioSource.pull(mAudioBuffer, mNumBufferFrames, mChannelCount);

                onPull();

                int numSamplesWritten = mAudioTrack.write(
                        mAudioBuffer,0, numBufferSamples, AudioTrack.WRITE_BLOCKING);
                if (numSamplesWritten < 0) {
                    // error
                    if (LOG) {
                        Log.i(TAG, "AudioTrack write error: " + numSamplesWritten);
                    }
                    stopStream();
                } else if (numSamplesWritten < numBufferSamples) {
                    // end of stream
                    if (LOG) {
                        Log.i(TAG, "Stream Complete.");
                    }
                    stopStream();
                }
            }
        }
    }
}
