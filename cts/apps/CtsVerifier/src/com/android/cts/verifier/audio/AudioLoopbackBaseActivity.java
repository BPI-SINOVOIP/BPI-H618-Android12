/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.app.AlertDialog;
import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;

import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.cts.verifier.audio.audiolib.AudioSystemFlags;
import com.android.cts.verifier.audio.audiolib.StatUtils;
import com.android.cts.verifier.audio.audiolib.AudioUtils;
import com.android.cts.verifier.CtsVerifierReportLog;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import static com.android.cts.verifier.TestListActivity.sCurrentDisplayMode;
import static com.android.cts.verifier.TestListAdapter.setTestNameSuffix;

/**
 * Base class for testing activitiees that require audio loopback hardware..
 */
public class AudioLoopbackBaseActivity extends PassFailButtons.Activity {
    private static final String TAG = "AudioLoopbackBaseActivity";

    // JNI load
    static {
        try {
            System.loadLibrary("audioloopback_jni");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Error loading Audio Loopback JNI library");
            Log.e(TAG, "e: " + e);
            e.printStackTrace();
        }

        /* TODO: gracefully fail/notify if the library can't be loaded */
    }
    protected AudioManager mAudioManager;

    // UI
    TextView mInputDeviceTxt;
    TextView mOutputDeviceTxt;

    TextView mAudioLevelText;
    SeekBar mAudioLevelSeekbar;

    TextView mTestPathTxt;

    TextView mResultText;
    ProgressBar mProgressBar;
    int mMaxLevel;

    String mYesString;
    String mNoString;

    // These flags determine the maximum allowed latency
    private boolean mClaimsProAudio;
    private boolean mClaimsOutput;
    private boolean mClaimsInput;

    // Useful info
    private boolean mSupportsMMAP = AudioUtils.isMMapSupported();
    private boolean mSupportsMMAPExclusive = AudioUtils.isMMapExclusiveSupported();

    // Peripheral(s)
    boolean mIsPeripheralAttached;  // CDD ProAudio section C-1-3
    AudioDeviceInfo mOutputDevInfo;
    AudioDeviceInfo mInputDevInfo;

    protected static final int TESTPERIPHERAL_NONE          = 0;
    protected static final int TESTPERIPHERAL_ANALOG_JACK   = 1;
    protected static final int TESTPERIPHERAL_USB           = 2;
    protected static final int TESTPERIPHERAL_DEVICE        = 3; // device speaker + mic
    protected int mTestPeripheral = TESTPERIPHERAL_NONE;

    // Loopback Logic
    NativeAnalyzerThread mNativeAnalyzerThread = null;

    protected static final int NUM_TEST_PHASES = 5;
    protected int mTestPhase = 0;

    protected double[] mLatencyMillis = new double[NUM_TEST_PHASES];
    protected double[] mConfidence = new double[NUM_TEST_PHASES];

    protected double mMeanLatencyMillis;
    protected double mMeanAbsoluteDeviation;
    protected double mMeanConfidence;

    protected static final double CONFIDENCE_THRESHOLD = 0.6;
    // impossibly low latencies (indicating something in the test went wrong).
    protected static final float EPSILON = 1.0f;
    protected static final double PROAUDIO_RECOMMENDED_LATENCY_MS = 20.0;
    protected static final double PROAUDIO_RECOMMENDED_USB_LATENCY_MS = 25.0;
    protected static final double PROAUDIO_MUST_LATENCY_MS = 20.0;
    protected static final double BASIC_RECOMMENDED_LATENCY_MS = 50.0;
    protected static final double BASIC_MUST_LATENCY_MS = 800.0;
    protected double mMustLatency;
    protected double mRecommendedLatency;

    // The audio stream callback threads should stop and close
    // in less than a few hundred msec. This is a generous timeout value.
    private static final int STOP_TEST_TIMEOUT_MSEC = 2 * 1000;

    //
    // Common UI Handling
    //
    private void connectLoopbackUI() {
        // Connected Device
        mInputDeviceTxt = ((TextView)findViewById(R.id.audioLoopbackInputLbl));
        mOutputDeviceTxt = ((TextView)findViewById(R.id.audioLoopbackOutputLbl));

        mAudioLevelText = (TextView)findViewById(R.id.audio_loopback_level_text);
        mAudioLevelSeekbar = (SeekBar)findViewById(R.id.audio_loopback_level_seekbar);
        mMaxLevel = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        mAudioLevelSeekbar.setMax(mMaxLevel);
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, (int)(0.7 * mMaxLevel), 0);
        refreshLevel();

        mAudioLevelSeekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC,
                        progress, 0);
                Log.i(TAG,"Level set to: " + progress);
                refreshLevel();
            }
        });

        mResultText = (TextView)findViewById(R.id.audio_loopback_results_text);
        mProgressBar = (ProgressBar)findViewById(R.id.audio_loopback_progress_bar);
        showWait(false);
    }

    //
    // Peripheral Connection Logic
    //
    protected void scanPeripheralList(AudioDeviceInfo[] devices) {
        // CDD Section C-1-3: USB port, host-mode support

        // Can't just use the first record because then we will only get
        // Source OR sink, not both even on devices that are both.
        mOutputDevInfo = null;
        mInputDevInfo = null;

        // Any valid peripherals
        // Do we leave in the Headset test to support a USB-Dongle?
        for (AudioDeviceInfo devInfo : devices) {
            if (devInfo.getType() == AudioDeviceInfo.TYPE_USB_DEVICE || // USB Peripheral
                    devInfo.getType() == AudioDeviceInfo.TYPE_USB_HEADSET || // USB dongle+LBPlug
                    devInfo.getType() == AudioDeviceInfo.TYPE_WIRED_HEADSET || // Loopback Plug?
                    devInfo.getType() == AudioDeviceInfo.TYPE_AUX_LINE) { // Aux-cable loopback?
                if (devInfo.isSink()) {
                    mOutputDevInfo = devInfo;
                }
                if (devInfo.isSource()) {
                    mInputDevInfo = devInfo;
                }
            }  else {
                handleDeviceConnection(devInfo);
            }
        }

        // need BOTH input and output to test
        mIsPeripheralAttached = mOutputDevInfo != null && mInputDevInfo != null;
        calculateTestPeripheral();
        showConnectedAudioPeripheral();
        calculateLatencyThresholds();
        displayLatencyThresholds();
    }

    protected void handleDeviceConnection(AudioDeviceInfo deviceInfo) {
        // NOP
    }

    private class ConnectListener extends AudioDeviceCallback {
        /*package*/ ConnectListener() {}

        //
        // AudioDevicesManager.OnDeviceConnectionListener
        //
        @Override
        public void onAudioDevicesAdded(AudioDeviceInfo[] addedDevices) {
            scanPeripheralList(mAudioManager.getDevices(AudioManager.GET_DEVICES_ALL));
        }

        @Override
        public void onAudioDevicesRemoved(AudioDeviceInfo[] removedDevices) {
            scanPeripheralList(mAudioManager.getDevices(AudioManager.GET_DEVICES_ALL));
        }
    }

    protected void calculateTestPeripheral() {
        if (!mIsPeripheralAttached) {
            mTestPeripheral = TESTPERIPHERAL_DEVICE;
        } else if (mInputDevInfo.getType() == AudioDeviceInfo.TYPE_USB_DEVICE ||
                mInputDevInfo.getType() == AudioDeviceInfo.TYPE_USB_HEADSET) {
            mTestPeripheral = TESTPERIPHERAL_USB;
        } else if (mInputDevInfo.getType() == AudioDeviceInfo.TYPE_WIRED_HEADSET ||
                mInputDevInfo.getType() == AudioDeviceInfo.TYPE_AUX_LINE) {
            mTestPeripheral = TESTPERIPHERAL_ANALOG_JACK;
        } else {
            // Huh?
            Log.e(TAG, "No valid peripheral found!?");
            mTestPeripheral = TESTPERIPHERAL_NONE;
        }
    }

    protected boolean isPeripheralValidForTest() {
        return mTestPeripheral == TESTPERIPHERAL_ANALOG_JACK
                || mTestPeripheral == TESTPERIPHERAL_USB;

    }
    protected void showConnectedAudioPeripheral() {
        mInputDeviceTxt.setText(
                mInputDevInfo != null ? mInputDevInfo.getProductName().toString()
                        : "");
        mOutputDeviceTxt.setText(
                mOutputDevInfo != null ? mOutputDevInfo.getProductName().toString()
                        : "");

        String pathName;
        switch (mTestPeripheral) {
            case TESTPERIPHERAL_ANALOG_JACK:
                pathName = "Headset Jack";
                break;

            case TESTPERIPHERAL_USB:
                pathName = "USB";
                break;

            case TESTPERIPHERAL_DEVICE:
                pathName = "Device Speaker + Microphone";
                break;

            case TESTPERIPHERAL_NONE:
            default:
                pathName = "Error. Unknown Test Path";
                break;
        }
        mTestPathTxt.setText(pathName);
    }

    private void calculateLatencyThresholds() {
        switch (mTestPeripheral) {
            case TESTPERIPHERAL_ANALOG_JACK:
                mRecommendedLatency = mClaimsProAudio
                        ? PROAUDIO_RECOMMENDED_LATENCY_MS : BASIC_RECOMMENDED_LATENCY_MS;
                mMustLatency =  mClaimsProAudio
                        ? PROAUDIO_RECOMMENDED_LATENCY_MS : BASIC_MUST_LATENCY_MS;
                break;

            case TESTPERIPHERAL_USB:
                mRecommendedLatency = mClaimsProAudio
                        ? PROAUDIO_RECOMMENDED_USB_LATENCY_MS : BASIC_RECOMMENDED_LATENCY_MS;
                mMustLatency = mClaimsProAudio
                        ? PROAUDIO_RECOMMENDED_USB_LATENCY_MS : BASIC_MUST_LATENCY_MS;
                break;

            case TESTPERIPHERAL_DEVICE:
                // This isn't a valid case so we won't pass it, but it can be run
                mRecommendedLatency = mClaimsProAudio
                        ? PROAUDIO_RECOMMENDED_LATENCY_MS : BASIC_RECOMMENDED_LATENCY_MS;
                mMustLatency = mClaimsProAudio
                        ? PROAUDIO_RECOMMENDED_LATENCY_MS :BASIC_MUST_LATENCY_MS;
                break;

            case TESTPERIPHERAL_NONE:
            default:
                mRecommendedLatency = BASIC_RECOMMENDED_LATENCY_MS;
                mMustLatency = BASIC_MUST_LATENCY_MS;
                break;
        }
    }

    private void displayLatencyThresholds() {
        if (isPeripheralValidForTest()) {
            ((TextView) findViewById(R.id.audio_loopback_must_latency)).setText("" + mMustLatency);
            ((TextView) findViewById(R.id.audio_loopback_recommended_latency)).setText(
                    "" + mRecommendedLatency);
        } else {
            String naStr = getResources().getString(R.string.audio_proaudio_NA);
            ((TextView) findViewById(R.id.audio_loopback_must_latency)).setText(naStr);
            ((TextView) findViewById(R.id.audio_loopback_recommended_latency)).setText(naStr);
        }
    }

    /**
     * refresh Audio Level seekbar and text
     */
    private void refreshLevel() {
        int currentLevel = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        mAudioLevelSeekbar.setProgress(currentLevel);

        String levelText = String.format("%s: %d/%d",
                getResources().getString(R.string.audio_loopback_level_text),
                currentLevel, mMaxLevel);
        mAudioLevelText.setText(levelText);
    }

    //
    // show active progress bar
    //
    protected void showWait(boolean show) {
        mProgressBar.setVisibility(show ? View.VISIBLE : View.INVISIBLE);
    }

    //
    // Common loging
    //
    // Schema
    private static final String KEY_LATENCY = "latency";
    private static final String KEY_CONFIDENCE = "confidence";
    private static final String KEY_SAMPLE_RATE = "sample_rate";
    private static final String KEY_IS_PRO_AUDIO = "is_pro_audio";
    private static final String KEY_IS_LOW_LATENCY = "is_low_latency";
    private static final String KEY_IS_PERIPHERAL_ATTACHED = "is_peripheral_attached";
    private static final String KEY_INPUT_PERIPHERAL_NAME = "input_peripheral";
    private static final String KEY_OUTPUT_PERIPHERAL_NAME = "output_peripheral";
    private static final String KEY_TEST_PERIPHERAL = "test_peripheral";
    private static final String KEY_TEST_MMAP = "supports_mmap";
    private static final String KEY_TEST_MMAPEXCLUSIVE = "supports_mmap_exclusive";

    @Override
    public String getTestId() {
        return setTestNameSuffix(sCurrentDisplayMode, getClass().getName());
    }

    //
    // Subclasses should call this explicitly. SubClasses should call submit() after their logs
    //
    @Override
    public void recordTestResults() {
        if (mNativeAnalyzerThread == null) {
            return; // no results to report
        }

        CtsVerifierReportLog reportLog = getReportLog();
        reportLog.addValue(
                KEY_LATENCY,
                mMeanLatencyMillis,
                ResultType.LOWER_BETTER,
                ResultUnit.MS);

        reportLog.addValue(
                KEY_CONFIDENCE,
                mMeanConfidence,
                ResultType.HIGHER_BETTER,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_SAMPLE_RATE,
                mNativeAnalyzerThread.getSampleRate(),
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_IS_LOW_LATENCY,
                mNativeAnalyzerThread.isLowLatencyStream(),
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_IS_PERIPHERAL_ATTACHED,
                mIsPeripheralAttached,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_IS_PRO_AUDIO,
                mClaimsProAudio,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_TEST_PERIPHERAL,
                mTestPeripheral,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_TEST_MMAP,
                mSupportsMMAP,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        reportLog.addValue(
                KEY_TEST_MMAPEXCLUSIVE ,
                mSupportsMMAPExclusive,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        if (mIsPeripheralAttached) {
            reportLog.addValue(
                    KEY_INPUT_PERIPHERAL_NAME,
                    mInputDevInfo != null ? mInputDevInfo.getProductName().toString() : "None",
                    ResultType.NEUTRAL,
                    ResultUnit.NONE);

            reportLog.addValue(
                    KEY_OUTPUT_PERIPHERAL_NAME,
                    mOutputDevInfo != null ? mOutputDevInfo.getProductName().toString() : "None",
                    ResultType.NEUTRAL,
                    ResultUnit.NONE);
        }
    }

    private static final String KEY_LOOPBACK_AVAILABLE = "loopback_available";
    private void recordLoopbackStatus(boolean has) {
        getReportLog().addValue(
                KEY_LOOPBACK_AVAILABLE,
                has,
                ResultType.NEUTRAL,
                ResultUnit.NONE);
    }

    protected void startAudioTest(Handler messageHandler) {
        getPassButton().setEnabled(false);

        mTestPhase = 0;
        java.util.Arrays.fill(mLatencyMillis, 0.0);
        java.util.Arrays.fill(mConfidence, 0.0);

        mNativeAnalyzerThread = new NativeAnalyzerThread(this);
        if (mNativeAnalyzerThread != null) {
            mNativeAnalyzerThread.setMessageHandler(messageHandler);
            // This value matches AAUDIO_INPUT_PRESET_VOICE_RECOGNITION
            mNativeAnalyzerThread.setInputPreset(MediaRecorder.AudioSource.VOICE_RECOGNITION);
            startTestPhase();
        } else {
            Log.e(TAG, "Couldn't allocate native analyzer thread");
            mResultText.setText(getResources().getString(R.string.audio_loopback_failure));
        }
    }

    private void startTestPhase() {
        if (mNativeAnalyzerThread != null) {
            mNativeAnalyzerThread.startTest();

            // what is this for?
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    protected void handleTestCompletion() {
        mMeanLatencyMillis = StatUtils.calculateMean(mLatencyMillis);
        mMeanAbsoluteDeviation =
                StatUtils.calculateMeanAbsoluteDeviation(mMeanLatencyMillis, mLatencyMillis);
        mMeanConfidence = StatUtils.calculateMean(mConfidence);


        String result;
        if (mMeanConfidence < CONFIDENCE_THRESHOLD) {
            result = String.format(
                    "Test Finished\nInsufficient Confidence (%.2f < %.2f). No Results.",
                    mMeanConfidence, CONFIDENCE_THRESHOLD);
        } else {
            result = String.format(
                    "Test Finished\nMean Latency:%.2f ms (required:%.2f)\n" +
                            "Mean Absolute Deviation: %.2f\n" +
                            " Confidence: %.2f\n" +
                            " Low Latency Path: %s",
                    mMeanLatencyMillis,
                    mMustLatency,
                    mMeanAbsoluteDeviation,
                    mMeanConfidence,
                    mNativeAnalyzerThread.isLowLatencyStream() ? mYesString : mNoString);
        }

        // Make sure the test thread is finished. It should already be done.
        if (mNativeAnalyzerThread != null) {
            try {
                mNativeAnalyzerThread.stopTest(STOP_TEST_TIMEOUT_MSEC);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        mResultText.setText(result);
    }

    protected void handleTestPhaseCompletion() {
        if (mNativeAnalyzerThread != null && mTestPhase < NUM_TEST_PHASES) {
            mLatencyMillis[mTestPhase] = mNativeAnalyzerThread.getLatencyMillis();
            mConfidence[mTestPhase] = mNativeAnalyzerThread.getConfidence();

            String result = String.format(
                    "Test %d Finished\nLatency: %.2f ms\nConfidence: %.2f\n",
                    mTestPhase,
                    mLatencyMillis[mTestPhase],
                    mConfidence[mTestPhase]);

            mResultText.setText(result);
            try {
                mNativeAnalyzerThread.stopTest(STOP_TEST_TIMEOUT_MSEC);
                // Thread.sleep(/*STOP_TEST_TIMEOUT_MSEC*/500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            mTestPhase++;
            if (mTestPhase >= NUM_TEST_PHASES) {
                handleTestCompletion();
            } else {
                startTestPhase();
            }
        }
    }

    /**
     * handler for messages from audio thread
     */
    protected Handler mMessageHandler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch(msg.what) {
                case NativeAnalyzerThread.NATIVE_AUDIO_THREAD_MESSAGE_REC_STARTED:
                    Log.v(TAG,"got message native rec started!!");
                    showWait(true);
                    mResultText.setText(String.format("[phase: %d] - Test Running...",
                            (mTestPhase + 1)));
                    break;
                case NativeAnalyzerThread.NATIVE_AUDIO_THREAD_MESSAGE_OPEN_ERROR:
                    Log.v(TAG,"got message native rec can't start!!");
                    mResultText.setText("Test Error opening streams.");
                    handleTestCompletion();
                    break;
                case NativeAnalyzerThread.NATIVE_AUDIO_THREAD_MESSAGE_REC_ERROR:
                    Log.v(TAG,"got message native rec can't start!!");
                    mResultText.setText("Test Error while recording.");
                    handleTestCompletion();
                    break;
                case NativeAnalyzerThread.NATIVE_AUDIO_THREAD_MESSAGE_REC_COMPLETE_ERRORS:
                    mResultText.setText("Test FAILED due to errors.");
                    handleTestCompletion();
                    break;
                case NativeAnalyzerThread.NATIVE_AUDIO_THREAD_MESSAGE_ANALYZING:
                    Log.i(TAG, "NATIVE_AUDIO_THREAD_MESSAGE_ANALYZING");
                    mResultText.setText(String.format("[phase: %d] - Analyzing ...",
                            mTestPhase + 1));
                    break;
                case NativeAnalyzerThread.NATIVE_AUDIO_THREAD_MESSAGE_REC_COMPLETE:
                    Log.i(TAG, "NATIVE_AUDIO_THREAD_MESSAGE_REC_COMPLETE");
                    handleTestPhaseCompletion();
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mClaimsOutput = AudioSystemFlags.claimsOutput(this);
        mClaimsInput = AudioSystemFlags.claimsInput(this);
        mClaimsProAudio = AudioSystemFlags.claimsProAudio(this);

        mYesString = getResources().getString(R.string.audio_general_yes);
        mNoString = getResources().getString(R.string.audio_general_no);

        // Pro Audio
        ((TextView)findViewById(R.id.audio_loopback_pro_audio)).setText(
                "" + (mClaimsProAudio ? mYesString : mNoString));

        // MMAP
        ((TextView)findViewById(R.id.audio_loopback_mmap)).setText(
                "" + (mSupportsMMAP ? mYesString : mNoString));
        ((TextView)findViewById(R.id.audio_loopback_mmap_exclusive)).setText(
                "" + (mSupportsMMAPExclusive ? mYesString : mNoString));

        // Low Latency
        ((TextView)findViewById(R.id.audio_loopback_low_latency)).setText(
                "" + (AudioSystemFlags.claimsLowLatencyAudio(this) ? mYesString : mNoString));

        mTestPathTxt = ((TextView)findViewById(R.id.audio_loopback_testpath));

        mAudioManager = (AudioManager)getSystemService(AUDIO_SERVICE);

        mAudioManager.registerAudioDeviceCallback(new ConnectListener(), new Handler());

        connectLoopbackUI();

        calculateLatencyThresholds();
        displayLatencyThresholds();
    }
}
