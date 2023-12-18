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

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.TextView;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import org.hyphonate.megaaudio.common.BuilderBase;

public abstract class AudioColdStartBaseActivity
        extends PassFailButtons.Activity
        implements View.OnClickListener {
    private static final String TAG = "AudioColdStartBaseActivity";

    // JNI load
    static {
        try {
            System.loadLibrary("megaaudio_jni");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Error loading MegaAudio JNI library");
            Log.e(TAG, "e: " + e);
            e.printStackTrace();
        }

        /* TODO: gracefully fail/notify if the library can't be loaded */
    }

    // Test State
    protected boolean mIsTestRunning;

    // Audio Attributes
    protected static final int NUM_CHANNELS = 2;
    protected int mSampleRate;
    protected int mNumBufferFrames;

    protected int mAudioApi = BuilderBase.TYPE_OBOE;

    // (all times in nanoseconds)
    protected long mPreOpenTime;
    protected long mPostOpenTime;
    protected long mPreStartTime;
    protected long mPostStartTime;

    protected double mColdStartlatencyMS;

    // Widgets
    Button mStartBtn;
    Button mStopBtn;

    TextView mAttributesTxt;
    TextView mOpenTimeTxt;
    TextView mStartTimeTxt;
    TextView mResultsTxt;

    // Time-base conversions
    protected double nanosToMs(double nanos) {
        return nanos / 1000000.0;
    }

    protected long msToNanos(double ms) {
        return (long) (ms * 1000000.0);
    }

    //
    // UI Helpers
    //
    private final String msFormat = "%.2f ms";

    protected String makeMSString(double ms) {
        return String.format(msFormat, ms);
    }

    //
    // UI
    //
    void showAttributes() {
        mAttributesTxt.setText("" + mSampleRate + " Hz " + mNumBufferFrames + " Frames");
    }

    void showOpenTime() {
        double timeMs = nanosToMs(mPostOpenTime - mPreOpenTime);
        mOpenTimeTxt.setText("Open: " + makeMSString(timeMs));
    }

    void showStartTime() {
        double timeMs = nanosToMs(mPostStartTime - mPreStartTime);
        mStartTimeTxt.setText("Start: " + makeMSString(timeMs));
    }

    void showColdStartLatency() {
        mResultsTxt.setText("latency: " + mColdStartlatencyMS);
    }

    protected void clearResults() {
        mAttributesTxt.setText("");
        mOpenTimeTxt.setText("");
        mStartTimeTxt.setText("");
        mResultsTxt.setText("");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ((RadioButton) findViewById(R.id.audioJavaApiBtn)).setOnClickListener(this);
        RadioButton nativeApiRB = findViewById(R.id.audioNativeApiBtn);
        nativeApiRB.setChecked(true);
        nativeApiRB.setOnClickListener(this);

        mStartBtn = (Button) findViewById(R.id.coldstart_start_btn);
        mStartBtn.setOnClickListener(this);
        mStopBtn = (Button) findViewById(R.id.coldstart_stop_btn);
        mStopBtn.setOnClickListener(this);
        mStopBtn.setEnabled(false);

        mAttributesTxt = ((TextView) findViewById(R.id.coldstart_attributesTxt));
        mOpenTimeTxt = ((TextView) findViewById(R.id.coldstart_openTimeTxt));
        mStartTimeTxt = ((TextView) findViewById(R.id.coldstart_startTimeTxt));
        mResultsTxt = (TextView) findViewById(R.id.coldstart_coldLatencyTxt);
    }

    abstract boolean startAudioTest();
    abstract void stopAudioTest();

    protected void updateTestStateButtons() {
        mStartBtn.setEnabled(!mIsTestRunning);
        mStopBtn.setEnabled(mIsTestRunning);
    }

    //
    // View.OnClickListener overrides
    //
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.audioJavaApiBtn:
                stopAudioTest();
                updateTestStateButtons();
                clearResults();
                mAudioApi = BuilderBase.TYPE_JAVA;
                break;

            case R.id.audioNativeApiBtn:
                stopAudioTest();
                updateTestStateButtons();
                clearResults();
                mAudioApi = BuilderBase.TYPE_OBOE;
                break;

            case R.id.coldstart_start_btn:
                startAudioTest();

                showAttributes();
                showOpenTime();
                showStartTime();

                updateTestStateButtons();
                break;

            case R.id.coldstart_stop_btn:
                stopAudioTest();

                updateTestStateButtons();
                break;
        }
    }
}
