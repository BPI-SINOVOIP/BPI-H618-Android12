/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.cts.verifier.audio;

import android.content.Context;
import android.util.Log;

import com.android.cts.verifier.audio.audiolib.AudioSystemParams;

// MegaAudio imports
import org.hyphonate.megaaudio.player.AudioSource;
import org.hyphonate.megaaudio.player.AudioSourceProvider;
import org.hyphonate.megaaudio.player.JavaPlayer;
import org.hyphonate.megaaudio.player.PlayerBuilder;
import org.hyphonate.megaaudio.player.sources.SinAudioSourceProvider;

public abstract class USBAudioPeripheralPlayerActivity extends USBAudioPeripheralActivity {
    private static final String TAG = "USBAudioPeripheralPlayerActivity";

    // MegaPlayer
    static final int NUM_CHANNELS = 2;
    JavaPlayer mAudioPlayer;

    protected boolean mIsPlaying = false;

    protected boolean mOverridePlayFlag = true;

    public USBAudioPeripheralPlayerActivity(boolean requiresMandatePeripheral) {
        super(requiresMandatePeripheral); // Mandated peripheral is NOT required
    }

    protected void setupPlayer() {
        AudioSystemParams audioSystemParams = new AudioSystemParams();
        audioSystemParams.init(this);

        int systemSampleRate = audioSystemParams.getSystemSampleRate();
        int numBufferFrames = audioSystemParams.getSystemBufferFrames();

        //
        // Allocate the source provider for the sort of signal we want to play
        //
        AudioSourceProvider sourceProvider = new SinAudioSourceProvider();
        try {
            PlayerBuilder builder = new PlayerBuilder();
            mAudioPlayer = (JavaPlayer)builder
                    // choose one or the other of these for a Java or an Oboe player
                    .setPlayerType(PlayerBuilder.TYPE_JAVA)
                    // .setPlayerType(PlayerBuilder.PLAYER_OBOE)
                    .setSourceProvider(sourceProvider)
                    .build();
            mAudioPlayer.setupStream(NUM_CHANNELS, systemSampleRate, numBufferFrames);
        } catch (PlayerBuilder.BadStateException ex) {
            Log.e(TAG, "Failed MegaPlayer build.");
        }
    }

    protected void startPlay() {
        if (mOutputDevInfo != null && !mIsPlaying) {
            mAudioPlayer.startStream();

            mIsPlaying = true;
        }
    }

    protected void stopPlay() {
        if (mIsPlaying) {
            mAudioPlayer.stopStream();
            mIsPlaying = false;
        }
    }

    public boolean isPlaying() {
        return mIsPlaying;
    }

    @Override
    protected void onPause() {
        super.onPause();

        stopPlay();
    }
}
