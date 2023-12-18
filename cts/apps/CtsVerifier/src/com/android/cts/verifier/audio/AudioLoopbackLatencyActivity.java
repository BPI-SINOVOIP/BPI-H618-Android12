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

package com.android.cts.verifier.audio;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.cts.verifier.CtsVerifierReportLog;
import com.android.cts.verifier.R;

import static com.android.cts.verifier.TestListActivity.sCurrentDisplayMode;
import static com.android.cts.verifier.TestListAdapter.setTestNameSuffix;

/**
 * Tests Audio Device roundtrip latency by using a loopback plug.
 */
public class AudioLoopbackLatencyActivity extends AudioLoopbackBaseActivity {
    private static final String TAG = AudioLoopbackLatencyActivity.class.getSimpleName();

//    public static final int BYTES_PER_FRAME = 2;

    private int mMinBufferSizeInFrames = 0;

    OnBtnClickListener mBtnClickListener = new OnBtnClickListener();

    Button mTestButton;

    // ReportLog Schema
    private static final String KEY_LEVEL = "level";
    private static final String KEY_BUFFER_SIZE = "buffer_size_in_frames";

    private class OnBtnClickListener implements OnClickListener {
        @Override
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.audio_loopback_test_btn:
                    Log.i(TAG, "audio loopback test");
                    startAudioTest();
                    break;
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // we need to do this first so that the layout is inplace when the super-class inits
        setContentView(R.layout.audio_loopback_latency_activity);

        mTestButton =(Button)findViewById(R.id.audio_loopback_test_btn);
        mTestButton.setOnClickListener(mBtnClickListener);

        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
        setInfoResources(R.string.audio_loopback_latency_test, R.string.audio_loopback_info, -1);

        super.onCreate(savedInstanceState);
    }

    protected void startAudioTest() {
        mTestButton.setEnabled(false);
        super.startAudioTest(mMessageHandler);
    }

    protected void handleTestCompletion() {
        super.handleTestCompletion();

        // We are not enforcing the latency target (PROAUDIO_LATENCY_MS_LIMIT)
        // test is allowed to pass as long as an analysis is done.
        boolean resultValid =
                isPeripheralValidForTest()
                && mMeanConfidence >= CONFIDENCE_THRESHOLD
                && mMeanLatencyMillis > EPSILON
                && mMeanLatencyMillis < mMustLatency;

        getPassButton().setEnabled(resultValid);

        recordTestResults();

        showWait(false);
        mTestButton.setEnabled(true);
    }

    /**
     * Store test results in log
     */
    @Override
    public String getTestId() {
        return setTestNameSuffix(sCurrentDisplayMode, getClass().getName());
    }

    @Override
    public void recordTestResults() {
        Log.d(TAG, "recordTestResults()");
        super.recordTestResults();

        CtsVerifierReportLog reportLog = getReportLog();
        int audioLevel = mAudioLevelSeekbar.getProgress();
        reportLog.addValue(
                KEY_LEVEL,
                audioLevel,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_BUFFER_SIZE,
                mMinBufferSizeInFrames,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.submit();
    }
}
