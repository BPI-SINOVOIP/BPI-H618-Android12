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

package android.voiceinteraction.service;

import static android.media.AudioFormat.CHANNEL_IN_FRONT;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.PersistableBundle;
import android.os.Process;
import android.os.SharedMemory;
import android.service.voice.AlwaysOnHotwordDetector;
import android.service.voice.HotwordDetectedResult;
import android.service.voice.HotwordDetectionService;
import android.service.voice.HotwordRejectedResult;
import android.system.ErrnoException;
import android.text.TextUtils;
import android.util.Log;
import android.voiceinteraction.common.Utils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.IOException;
import java.io.InputStream;
import java.util.function.IntConsumer;

import javax.annotation.concurrent.GuardedBy;

public class MainHotwordDetectionService extends HotwordDetectionService {
    static final String TAG = "MainHotwordDetectionService";

    public static final HotwordDetectedResult DETECTED_RESULT =
            new HotwordDetectedResult.Builder()
                    .setAudioChannel(CHANNEL_IN_FRONT)
                    .setConfidenceLevel(HotwordDetectedResult.CONFIDENCE_LEVEL_HIGH)
                    .setHotwordDetectionPersonalized(true)
                    .setHotwordDurationMillis(1000)
                    .setHotwordOffsetMillis(500)
                    .setHotwordPhraseId(5)
                    .setPersonalizedScore(10)
                    .setScore(15)
                    .build();
    public static final HotwordDetectedResult DETECTED_RESULT_AFTER_STOP_DETECTION =
            new HotwordDetectedResult.Builder()
                    .setScore(57)
                    .build();
    public static final HotwordDetectedResult DETECTED_RESULT_FOR_MIC_FAILURE =
            new HotwordDetectedResult.Builder()
                    .setScore(58)
                    .build();
    public static final HotwordRejectedResult REJECTED_RESULT =
            new HotwordRejectedResult.Builder()
                    .setConfidenceLevel(HotwordRejectedResult.CONFIDENCE_LEVEL_MEDIUM)
                    .build();

    private Handler mHandler;
    @NonNull
    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private boolean mStopDetectionCalled;

    @GuardedBy("mLock")
    @Nullable
    private Runnable mDetectionJob;

    @Override
    public void onCreate() {
        super.onCreate();
        mHandler = Handler.createAsync(Looper.getMainLooper());
    }

    @Override
    public void onDetect(@NonNull AlwaysOnHotwordDetector.EventPayload eventPayload,
            long timeoutMillis, @NonNull Callback callback) {
        Log.d(TAG, "onDetect for DSP source");

        if (!canReadAudio()) {
            callback.onDetected(DETECTED_RESULT_FOR_MIC_FAILURE);
            return;
        }

        // TODO: Check the capture session (needs to be reflectively accessed).
        byte[] data = eventPayload.getTriggerAudio();
        if (data != null && data.length > 0) {
            // Create the unaccepted HotwordDetectedResult first to test the protection in the
            // onDetected callback function of HotwordDetectionService. When the bundle data of
            // HotwordDetectedResult is larger than max bundle size, it will throw the
            // IllegalArgumentException.
            PersistableBundle persistableBundle = new PersistableBundle();
            HotwordDetectedResult hotwordDetectedResult =
                    new HotwordDetectedResult.Builder()
                            .setExtras(persistableBundle)
                            .build();
            int key = 0;
            do {
                persistableBundle.putInt(Integer.toString(key), 0);
                key++;
            } while (Utils.getParcelableSize(persistableBundle)
                    <= HotwordDetectedResult.getMaxBundleSize());

            try {
                callback.onDetected(hotwordDetectedResult);
            } catch (IllegalArgumentException e) {
                callback.onDetected(DETECTED_RESULT);
            }
        } else {
            callback.onRejected(REJECTED_RESULT);
        }
    }

    @Override
    public void onDetect(
            @NonNull ParcelFileDescriptor audioStream,
            @NonNull AudioFormat audioFormat,
            @Nullable PersistableBundle options,
            @NonNull Callback callback) {
        Log.d(TAG, "onDetect for external source");

        if (callback == null) {
            Log.w(TAG, "callback is null");
            return;
        }
        if (audioStream == null) {
            Log.w(TAG, "audioStream is null");
            return;
        }

        long startTime = System.currentTimeMillis();
        try (InputStream fis =
                     new ParcelFileDescriptor.AutoCloseInputStream(audioStream)) {

            // We added the fake audio data and set "hotword!" string at the head. Then we simulated
            // to verify the audio data with "hotword!" in HotwordDetectionService. If the audio
            // data includes "hotword!", it means that the hotword is valid.
            while (fis.available() < 8) {
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    // Nothing
                }
                if (System.currentTimeMillis() - startTime > 3000) {
                    Log.w(TAG, "Over timeout");
                    return;
                }
            }
            Log.d(TAG, "fis.available() = " + fis.available());
            byte[] buffer = new byte[8];
            fis.read(buffer, 0, 8);
            if(isSame(buffer, BasicVoiceInteractionService.FAKE_HOTWORD_AUDIO_DATA,
                    buffer.length)) {
                Log.d(TAG, "call callback.onDetected");
                callback.onDetected(DETECTED_RESULT);
            }
        } catch (IOException e) {
            Log.w(TAG, "Failed to read data : ", e);
        }
    }

    @Override
    public void onDetect(@NonNull Callback callback) {
        Log.d(TAG, "onDetect for Mic source");
        synchronized (mLock) {
            if (mDetectionJob != null) {
                throw new IllegalStateException("onDetect called while already detecting");
            }
            if (!mStopDetectionCalled) {
                // Delaying this allows us to test other flows, such as stopping detection. It's
                // also more realistic to schedule it onto another thread.
                mDetectionJob = () -> {
                    Log.d(TAG, "Sending detected result");

                    if (canReadAudio()) {
                        callback.onDetected(DETECTED_RESULT);
                    } else {
                        callback.onDetected(DETECTED_RESULT_FOR_MIC_FAILURE);
                    }
                };
                mHandler.postDelayed(mDetectionJob, 1500);
            } else {
                Log.d(TAG, "Sending detected result after stop detection");
                // We can't store and use this callback in onStopDetection (not valid anymore there), so
                // instead we trigger detection again to report the event.
                callback.onDetected(DETECTED_RESULT_AFTER_STOP_DETECTION);
            }
        }
    }

    @Override
    public void onStopDetection() {
        super.onStopDetection();
        synchronized (mLock) {
            mHandler.removeCallbacks(mDetectionJob);
            mDetectionJob = null;
            mStopDetectionCalled = true;
        }
    }

    @Override
    public void onUpdateState(
            @Nullable PersistableBundle options,
            @Nullable SharedMemory sharedMemory,
            long callbackTimeoutMillis,
            @Nullable IntConsumer statusCallback) {
        super.onUpdateState(options, sharedMemory, callbackTimeoutMillis, statusCallback);
        Log.d(TAG, "onUpdateState");

        if (options != null) {
            if (options.getInt(Utils.KEY_TEST_SCENARIO, -1)
                    == Utils.HOTWORD_DETECTION_SERVICE_ON_UPDATE_STATE_CRASH) {
                Log.d(TAG, "Crash itself. Pid: " + Process.myPid());
                Process.killProcess(Process.myPid());
                return;
            }
            String fakeData = options.getString(BasicVoiceInteractionService.KEY_FAKE_DATA);
            if (!TextUtils.equals(fakeData, BasicVoiceInteractionService.VALUE_FAKE_DATA)) {
                Log.d(TAG, "options : data is not the same");
                return;
            }
        }

        if (sharedMemory != null) {
            try {
                sharedMemory.mapReadWrite();
                Log.d(TAG, "sharedMemory : is not read-only");
                return;
            } catch (ErrnoException e) {
                // For read-only case
            } finally {
                sharedMemory.close();
            }
        }

        // Report success
        Log.d(TAG, "onUpdateState success");
        if (statusCallback != null) {
            statusCallback.accept(INITIALIZATION_STATUS_SUCCESS);
        }
    }

    private boolean isSame(byte[] array1, byte[] array2, int length) {
        if (length <= 0) {
            return false;
        }
        if (array1 == null || array2 == null || array1.length < length || array2.length < length) {
            return false;
        }
        for (int i = 0; i < length; i++) {
            if (array1[i] != array2[i]) {
                return false;
            }
        }
        return true;
    }

    private boolean canReadAudio() {
        int bytesPerSample = 2; // for ENCODING_PCM_16BIT
        int sampleRate = 16000;
        int bytesPerSecond = bytesPerSample * sampleRate; // for single channel
        AudioRecord record =
                new AudioRecord.Builder()
                        .setAudioAttributes(
                                new AudioAttributes.Builder()
                                        .setInternalCapturePreset(MediaRecorder.AudioSource.HOTWORD)
                                        .build())
                        .setAudioFormat(
                                new AudioFormat.Builder()
                                        .setChannelMask(AudioFormat.CHANNEL_IN_MONO)
                                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                                        .setSampleRate(sampleRate)
                                        .build())
                        .setBufferSizeInBytes(bytesPerSecond)
                        .build();
        if (record.getState() != AudioRecord.STATE_INITIALIZED) {
            Log.e(TAG, "Failed to initialize AudioRecord");
            record.release();
            return false;
        }

        record.startRecording();
        try {
            byte[] buffer = new byte[bytesPerSecond]; // read 1 second of audio
            int numBytes = 0;
            while (numBytes < buffer.length) {
                int bytesRead =
                        record.read(buffer, numBytes, Math.min(1024, buffer.length - numBytes));
                if (bytesRead < 0) {
                    Log.e(TAG, "Error reading from mic: " + bytesRead);
                    return false;
                }
                numBytes += bytesRead;
            }
            for (byte b : buffer) {
                // TODO: Maybe check that some portion of the bytes are non-zero.
                if (b != 0) {
                    return true;
                }
            }
            return false;
        } finally {
            record.release();
        }
    }
}
