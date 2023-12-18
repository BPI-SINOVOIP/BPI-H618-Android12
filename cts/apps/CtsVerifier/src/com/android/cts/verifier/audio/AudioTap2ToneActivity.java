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
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.TextView;

import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.cts.verifier.audio.audiolib.StatUtils;
import com.android.cts.verifier.CtsVerifierReportLog;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.audio.audiolib.CircularBufferFloat;
import com.android.cts.verifier.audio.audiolib.TapLatencyAnalyser;
import com.android.cts.verifier.audio.audiolib.WaveformView;
import com.android.cts.verifier.audio.sources.BlipAudioSourceProvider;

import org.hyphonate.megaaudio.common.BuilderBase;
import org.hyphonate.megaaudio.duplex.DuplexAudioManager;
import org.hyphonate.megaaudio.player.AudioSource;
import org.hyphonate.megaaudio.player.AudioSourceProvider;
import org.hyphonate.megaaudio.player.JavaSourceProxy;
import org.hyphonate.megaaudio.recorder.AudioSinkProvider;
import org.hyphonate.megaaudio.recorder.sinks.AppCallback;
import org.hyphonate.megaaudio.recorder.sinks.AppCallbackAudioSinkProvider;

/**
 * CtsVerifier test to measure tap-to-tone latency.
 */
public class AudioTap2ToneActivity
        extends PassFailButtons.Activity
        implements View.OnClickListener, AppCallback {
    private static final String TAG = "AudioTap2ToneActivity";

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

    private boolean mIsRecording;

    private int mPlayerType = BuilderBase.TYPE_OBOE | BuilderBase.SUB_TYPE_OBOE_AAUDIO;

    private DuplexAudioManager mDuplexAudioManager;
    private AudioSource mBlipSource;

    private Button mStartBtn;
    private Button mStopBtn;

    private TextView mSpecView;
    private TextView mResultsView;
    private TextView mStatsView;
    private TextView mPhaseView;

    private WaveformView mWaveformView;

    // Test Constants are from OboeTester.AudioMidiTester
    private static final float MAX_TOUCH_LATENCY = 0.200f;
    private static final float MAX_OUTPUT_LATENCY = 0.600f;
    private static final float ANALYSIS_TIME_MARGIN = 0.250f;

    private static final float ANALYSIS_TIME_DELAY = MAX_OUTPUT_LATENCY;
    private static final float ANALYSIS_TIME_TOTAL = MAX_TOUCH_LATENCY + MAX_OUTPUT_LATENCY;
    private static final float ANALYSIS_TIME_MAX = ANALYSIS_TIME_TOTAL + ANALYSIS_TIME_MARGIN;
    private static final int ANALYSIS_SAMPLE_RATE = 48000; // need not match output rate

    private static final int NUM_RECORD_CHANNELS = 1;

    private CircularBufferFloat mInputBuffer;

    private Runnable mAnalysisTask;
    private int mTaskCountdown;

    private TapLatencyAnalyser mTapLatencyAnalyser;

    // Stats for latency
    // STRONGLY RECOMMENDED in CDD 5.6
    private static final int MAX_TAP_2_TONE_LATENCY = 80;   // ms

    // Test API (back-end) IDs
    private static final int NUM_TEST_APIS = 2;
    private static final int TEST_API_NATIVE = 0;
    private static final int TEST_API_JAVA = 1;
    private int mActiveTestAPI = TEST_API_NATIVE;

    private int[] mNumMeasurements = new int[NUM_TEST_APIS];    // ms
    private int[] mLatencySumSamples = new int[NUM_TEST_APIS];  // ms
    private double[] mLatencyMin = new double[NUM_TEST_APIS];   // ms
    private double[] mLatencyMax = new double[NUM_TEST_APIS];   // ms
    private double[] mLatencyAve = new double[NUM_TEST_APIS];   // ms

    private static final int NUM_TEST_PHASES = 5;
    private int mTestPhase;

    private double[] mLatencyMillis = new double[NUM_TEST_PHASES];

    // ReportLog Schema
    // Note that each key will be suffixed with the ID of the API tested
    private static final String KEY_LATENCY_MIN = "latency_min_";
    private static final String KEY_LATENCY_MAX = "latency_max_";
    private static final String KEY_LATENCY_AVE = "latency_max_";
    private static final String KEY_LATENCY_NUM_MEASUREMENTS = "latency_num_measurements_";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setContentView(R.layout.audio_tap2tone_activity);

        super.onCreate(savedInstanceState);

        // Setup UI
        mStartBtn = (Button) findViewById(R.id.tap2tone_startBtn);
        mStartBtn.setOnClickListener(this);
        mStopBtn = (Button) findViewById(R.id.tap2tone_stopBtn);
        mStopBtn.setOnClickListener(this);

        ((RadioButton) findViewById(R.id.audioJavaApiBtn)).setOnClickListener(this);
        RadioButton nativeApiRB = findViewById(R.id.audioNativeApiBtn);
        nativeApiRB.setChecked(true);
        nativeApiRB.setOnClickListener(this);

        ((Button) findViewById(R.id.tap2tone_clearResults)).setOnClickListener(this);

        mSpecView = (TextView) findViewById(R.id.tap2tone_specTxt);
        mResultsView = (TextView) findViewById(R.id.tap2tone_resultTxt);
        mStatsView = (TextView) findViewById(R.id.tap2tone_statsTxt);
        mPhaseView = (TextView) findViewById(R.id.tap2tone_phaseInfo);

        mWaveformView = (WaveformView) findViewById(R.id.tap2tone_waveView);
        // Start a blip test when the waveform view is tapped.
        mWaveformView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent event) {
                int action = event.getActionMasked();
                switch (action) {
                    case MotionEvent.ACTION_DOWN:
                    case MotionEvent.ACTION_POINTER_DOWN:
                        trigger();
                        break;
                    case MotionEvent.ACTION_MOVE:
                        break;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_POINTER_UP:
                        break;
                }
                // Must return true or we do not get the ACTION_MOVE and
                // ACTION_UP events.
                return true;
            }
        });

        setPassFailButtonClickListeners();
        setInfoResources(R.string.audio_tap2tone, R.string.audio_tap2tone_info, -1);

        enableAudioButtons();

        // Setup analysis
        int numBufferSamples = (int) (ANALYSIS_TIME_MAX * ANALYSIS_SAMPLE_RATE);
        mInputBuffer = new CircularBufferFloat(numBufferSamples);
        mTapLatencyAnalyser = new TapLatencyAnalyser();

        JavaSourceProxy.initN();

        calculateTestPass();
    }

    private void startAudio() {
        if (mIsRecording) {
            return;
        }

        if (mDuplexAudioManager == null) {
            AudioSourceProvider sourceProvider = new BlipAudioSourceProvider();
            AudioSinkProvider sinkProvider = new AppCallbackAudioSinkProvider(this);
            mDuplexAudioManager = new DuplexAudioManager(sourceProvider, sinkProvider);
            mDuplexAudioManager.setNumRecorderChannels(NUM_RECORD_CHANNELS);
        }

        mDuplexAudioManager.setupStreams(mPlayerType, BuilderBase.TYPE_JAVA);
        mDuplexAudioManager.start();

        mBlipSource = (AudioSource) mDuplexAudioManager.getAudioSource();

        mIsRecording = true;
        enableAudioButtons();
    }

    private void stopAudio() {
        if (mIsRecording) {
            mDuplexAudioManager.stop();
            // is there a teardown method here?
            mIsRecording = false;
            enableAudioButtons();
        }
    }

    private void resetStats() {
        mNumMeasurements[mActiveTestAPI] = 0;
        mLatencySumSamples[mActiveTestAPI] = 0;
        mLatencyMin[mActiveTestAPI] =
            mLatencyMax[mActiveTestAPI] =
            mLatencyAve[mActiveTestAPI] = 0;

        java.util.Arrays.fill(mLatencyMillis, 0.0);

        mTestPhase = 0;
    }

    private void clearResults() {
        resetStats();
        mSpecView.setText(getResources().getString(R.string.audio_tap2tone_spec));
        mResultsView.setText("");
        mStatsView.setText("");
    }

    private void enableAudioButtons() {
        mStartBtn.setEnabled(!mIsRecording);
        mStopBtn.setEnabled(mIsRecording);
    }

    private void calculateTestPass() {
        // 80ms is currently STRONGLY RECOMMENDED, so pass the test as long as they have run it.
        boolean testCompleted = mTestPhase >= NUM_TEST_PHASES;
        boolean pass = mLatencyAve[mActiveTestAPI] != 0
                && mLatencyAve[mActiveTestAPI] <= MAX_TAP_2_TONE_LATENCY;

        if (testCompleted) {
            if (pass) {
                mSpecView.setText("Ave: " + mLatencyAve[mActiveTestAPI] + " ms <= "
                        + MAX_TAP_2_TONE_LATENCY + " ms -- PASS");
            } else {
                mSpecView.setText("Ave: " + mLatencyAve[mActiveTestAPI] + " ms > "
                        + MAX_TAP_2_TONE_LATENCY + " ms -- DOES NOT MEET STRONGLY RECOMMENDED");
            }
        }
        getPassButton().setEnabled(testCompleted);
    }

    private void recordTestStatus() {
        CtsVerifierReportLog reportLog = getReportLog();
        for (int api = TEST_API_NATIVE; api <= TEST_API_JAVA; api++) {
            reportLog.addValue(
                    KEY_LATENCY_MIN + api,
                    mLatencyMin[api],
                    ResultType.NEUTRAL,
                    ResultUnit.NONE);
            reportLog.addValue(
                    KEY_LATENCY_MAX + api,
                    mLatencyMax[api],
                    ResultType.NEUTRAL,
                    ResultUnit.NONE);
            reportLog.addValue(
                    KEY_LATENCY_AVE + api,
                    mLatencyAve[api],
                    ResultType.NEUTRAL,
                    ResultUnit.NONE);
            reportLog.addValue(
                    KEY_LATENCY_NUM_MEASUREMENTS + api,
                    mNumMeasurements[api],
                    ResultType.NEUTRAL,
                    ResultUnit.NONE);
        }

        reportLog.submit();
    }

    private void trigger() {
        if (mIsRecording) {
            mBlipSource.trigger();

            // schedule an analysis to start in the near future
            mAnalysisTask = new Runnable() {
                public void run() {
                    new Thread() {
                        public void run() {
                            analyzeCapturedAudio();
                        }
                    }.start();
                }
            };
            mTaskCountdown =
                    (int) (mDuplexAudioManager.getRecorder().getSampleRate() * ANALYSIS_TIME_DELAY);
        }
    }

    /**
     * A holder for analysis results/
     */
    public static class TestResult {
        public float[] samples;
        public float[] filtered;
        public int frameRate;
        public TapLatencyAnalyser.TapLatencyEvent[] events;
    }

    private void processTest(TestResult result) {
        if (mTestPhase == NUM_TEST_PHASES) {
            mTestPhase--;
        }

        int[] cursors = new int[2];
        cursors[0] = result.events[0].sampleIndex;
        cursors[1] = result.events[1].sampleIndex;
        mWaveformView.setCursorData(cursors);

        int latencySamples = cursors[1] - cursors[0];
        mLatencySumSamples[mActiveTestAPI] += latencySamples;
        mNumMeasurements[mActiveTestAPI]++;

        double latencyMillis = 1000 * latencySamples / result.frameRate;
        mLatencyMillis[mTestPhase] = latencyMillis;

        if (mLatencyMin[mActiveTestAPI] == 0
                || mLatencyMin[mActiveTestAPI] > latencyMillis) {
            mLatencyMin[mActiveTestAPI] = latencyMillis;
        }
        if (mLatencyMax[mActiveTestAPI] == 0
                || mLatencyMax[mActiveTestAPI] < latencyMillis) {
            mLatencyMax[mActiveTestAPI] = latencyMillis;
        }

        mLatencyAve[mActiveTestAPI] = StatUtils.calculateMean(mLatencyMillis);
        double meanAbsoluteDeviation = StatUtils.calculateMeanAbsoluteDeviation(
                mLatencyAve[mActiveTestAPI], mLatencyMillis);

        mTestPhase++;

        mLatencyAve[mActiveTestAPI] = 1000
                * (mLatencySumSamples[mActiveTestAPI] / mNumMeasurements[mActiveTestAPI])
                / result.frameRate;
        mResultsView.setText("Phase: " + mTestPhase + " : " + latencyMillis
                + " ms, Ave: " + mLatencyAve[mActiveTestAPI] + " ms");
        mStatsView.setText("Deviation: " + String.format("%.2f",meanAbsoluteDeviation));

        mPhaseView.setText("" + mTestPhase + " of " + NUM_TEST_PHASES + " completed.");
    }

    private void analyzeCapturedAudio() {
        if (!mIsRecording) {
            return;
        }
        int sampleRate = mDuplexAudioManager.getRecorder().getSampleRate();
        int numSamples = (int) (sampleRate * ANALYSIS_TIME_TOTAL);
        float[] buffer = new float[numSamples];

        int numRead = mInputBuffer.readMostRecent(buffer);

        TestResult result = new TestResult();
        result.samples = buffer;
        result.frameRate = sampleRate;
        result.events = mTapLatencyAnalyser.analyze(buffer, 0, numRead);
        result.filtered = mTapLatencyAnalyser.getFilteredBuffer();

        // This will come in on a background thread, so switch to the UI thread to update the UI.
        runOnUiThread(new Runnable() {
            public void run() {
                if (result.events.length < 2) {
                    mResultsView.setText(
                            getResources().getString(R.string.audio_tap2tone_too_few));
                    mStatsView.setText("");
                } else if (result.events.length > 2) {
                    mResultsView.setText(
                            getResources().getString(R.string.audio_tap2tone_too_many));
                    mStatsView.setText("");
                } else {
                    processTest(result);
                }

                mWaveformView.setSampleData(result.filtered);
                mWaveformView.postInvalidate();

                calculateTestPass();
            }
        });
    }

    //
    // View.OnClickListener overrides
    //
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.tap2tone_startBtn:
                startAudio();
                break;

            case R.id.tap2tone_stopBtn:
                stopAudio();
                break;

            case R.id.audioJavaApiBtn:
                stopAudio();
                clearResults();
                mPlayerType = BuilderBase.TYPE_JAVA;
                mActiveTestAPI = TEST_API_JAVA;
                break;

            case R.id.audioNativeApiBtn:
                stopAudio();
                clearResults();
                mPlayerType = BuilderBase.TYPE_OBOE | BuilderBase.SUB_TYPE_OBOE_AAUDIO;
                mActiveTestAPI = TEST_API_NATIVE;
                break;

            case R.id.tap2tone_clearResults:
                clearResults();
                break;
        }
    }

    @Override
    public void setTestResultAndFinish(boolean passed) {
        stopAudio();
        recordTestStatus();
        super.setTestResultAndFinish(passed);
    }

    //
    // AppCallback overrides
    //
    @Override
    public void onDataReady(float[] audioData, int numFrames) {
        mInputBuffer.write(audioData);

        // Analysis?
        if (mTaskCountdown > 0) {
            mTaskCountdown -= numFrames;
            if (mTaskCountdown <= 0) {
                mTaskCountdown = 0;
                new Thread(mAnalysisTask).start(); // run asynchronously with audio thread
            }
        }
    }
}
