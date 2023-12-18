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

package com.android.cts.verifier.audio;

import static com.android.cts.verifier.TestListActivity.sCurrentDisplayMode;
import static com.android.cts.verifier.TestListAdapter.setTestNameSuffix;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.android.cts.verifier.R;
import com.android.cts.verifier.audio.audiolib.AudioSystemParams;

import org.hyphonate.megaaudio.recorder.AudioSinkProvider;
import org.hyphonate.megaaudio.recorder.Recorder;
import org.hyphonate.megaaudio.recorder.RecorderBuilder;
import org.hyphonate.megaaudio.recorder.sinks.AppCallback;
import org.hyphonate.megaaudio.recorder.sinks.AppCallbackAudioSink;
import org.hyphonate.megaaudio.recorder.sinks.AppCallbackAudioSinkProvider;

/**
 * CTS-Test for cold-start latency measurements
 */
public class AudioInColdStartLatencyActivity
        extends AudioColdStartBaseActivity {
    private static final String TAG = "AudioInColdStartLatencyActivity";
    private static final boolean DEBUG = false;

    private static final int LATENCY_MS_MUST     = 500; // CDD C-3-2
    private static final int LATENCY_MS_RECOMMEND = 100; // CDD C-SR

    // MegaAudio
    private Recorder mRecorder;

    private TextView mCallbackDeltaTxt;

    private long mPreviousCallbackTime;
    private long mCallbackDeltaTime;

    private long mNominalCallbackDelta;
    private long mCallbackThresholdTime;
    private long mAccumulatedTime;
    private long mNumCallbacks;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setContentView(R.layout.audio_coldstart_in_activity);
        super.onCreate(savedInstanceState);

//        mCallbackDeltaTxt = (TextView) findViewById(R.id.coldstart_inCallbackDeltaTxt);

        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
        setInfoResources(
                R.string.audio_coldstart_inputlbl, R.string.audio_coldstart_input_info, -1);
    }

    @Override
    public String getTestId() {
        return setTestNameSuffix(sCurrentDisplayMode, getClass().getName());
    }

    boolean calcTestResult() {
        boolean pass = mColdStartlatencyMS <= LATENCY_MS_MUST;
        getPassButton().setEnabled(pass);
        return pass;
    }

    double calcColdStartLatency() {
        mColdStartlatencyMS = nanosToMs(mPreviousCallbackTime - mPreOpenTime);
        return mColdStartlatencyMS;
    }

    void showInResults() {
        showColdStartLatency();

        calcTestResult();
    }

    protected void stopAudio() {
        stopAudioTest();
    }

    //
    // Audio Streaming
    //
    @Override
    boolean startAudioTest() {
        AudioSystemParams audioSystemParams = new AudioSystemParams();
        audioSystemParams.init(this);

        mSampleRate = audioSystemParams.getSystemSampleRate();
        mNumBufferFrames = audioSystemParams.getSystemBurstFrames();

        mAccumulatedTime = 0;
        mNumCallbacks = 0;

        AudioSinkProvider sinkProvider =
                new AppCallbackAudioSinkProvider(new ColdStartAppCallback());
        try {
            mPreOpenTime = System.nanoTime();
            mRecorder = (new RecorderBuilder())
                    .setRecorderType(mAudioApi)
                    .setAudioSinkProvider(sinkProvider)
                    .build();
            mRecorder.setupStream(NUM_CHANNELS, mSampleRate, mNumBufferFrames);
            mPostOpenTime = System.nanoTime();

            mIsTestRunning = true;
        } catch (RecorderBuilder.BadStateException badStateException) {
            mResultsTxt.setText("Can't Start Recorder.");
            Log.e(TAG, "BadStateException: " + badStateException);
            mIsTestRunning = false;
        }

        mPreStartTime = System.nanoTime();
        mRecorder.startStream();
        mPostStartTime = System.nanoTime();

        showOpenTime();
        showStartTime();

        if (mIsTestRunning) {
            mStartBtn.setEnabled(false);
            mStopBtn.setEnabled(true);
        }
        return mIsTestRunning;
    }

    @Override
    void stopAudioTest() {
        if (!mIsTestRunning) {
            return;
        }

        mRecorder.stopStream();

        mIsTestRunning = false;

        mStartBtn.setEnabled(true);
        mStopBtn.setEnabled(false);

        calcColdStartLatency();

        showInResults();
    }

    // Callback for Recorder
    /*
     * Monitor callbacks until they become consistent (i.e. delta between callbacks is below
     * some threshold like 1/8 the "nominal" callback time. This is defined as the "cold start
     * latency". Calculate that time and display the results.
     */
    class ColdStartAppCallback implements AppCallback {
        public void onDataReady(float[] audioData, int numFrames) {
            mNumCallbacks++;

            long time = System.nanoTime();
            if (mPreviousCallbackTime == 0) {
                mNumBufferFrames = numFrames;
                mNominalCallbackDelta
                        = (long)((1000000000.0 * (double) mNumBufferFrames) / (double) mSampleRate);
                mCallbackThresholdTime = mNominalCallbackDelta + (mNominalCallbackDelta / 8);
                // update attributes with actual buffer size
                // showAttributes();
                mPreviousCallbackTime = time;
            } else {
                mCallbackDeltaTime = time - mPreviousCallbackTime;

                mPreviousCallbackTime = time;
                mAccumulatedTime += mCallbackDeltaTime;

                if (mCallbackDeltaTime < mCallbackThresholdTime) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            stopAudioTest();
                        }
                    });
                }
            }
        }
    }
}
