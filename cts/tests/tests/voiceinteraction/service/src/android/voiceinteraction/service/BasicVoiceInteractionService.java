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

import static android.Manifest.permission.CAPTURE_AUDIO_HOTWORD;
import static android.Manifest.permission.RECORD_AUDIO;

import static com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity;

import android.Manifest;
import android.app.UiAutomation;
import android.content.Intent;
import android.media.AudioFormat;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;
import android.os.PersistableBundle;
import android.os.SharedMemory;
import android.service.voice.AlwaysOnHotwordDetector;
import android.service.voice.HotwordDetectionService;
import android.service.voice.HotwordDetector;
import android.service.voice.HotwordRejectedResult;
import android.service.voice.VoiceInteractionService;
import android.system.ErrnoException;
import android.util.Log;
import android.voiceinteraction.common.Utils;

import androidx.annotation.NonNull;
import androidx.test.platform.app.InstrumentationRegistry;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Locale;

/**
 * This service included a basic HotwordDetectionService for testing.
 */
public class BasicVoiceInteractionService extends VoiceInteractionService {
    // TODO: (b/182236586) Refactor the voice interaction service logic
    static final String TAG = "BasicVoiceInteractionService";

    public static String KEY_FAKE_DATA = "fakeData";
    public static String VALUE_FAKE_DATA = "fakeData";
    public static byte[] FAKE_BYTE_ARRAY_DATA = new byte[] {1, 2, 3};
    public static byte[] FAKE_HOTWORD_AUDIO_DATA =
            new byte[] {'h', 'o', 't', 'w', 'o', 'r', 'd', '!'};

    private boolean mReady = false;
    private AlwaysOnHotwordDetector mAlwaysOnHotwordDetector = null;
    private HotwordDetector mSoftwareHotwordDetector = null;
    private ParcelFileDescriptor[] mTempParcelFileDescriptor = null;

    @Override
    public void onReady() {
        super.onReady();
        mReady = true;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand received");

        if (intent == null || !mReady) {
            Log.wtf(TAG, "Can't start because either intent is null or onReady() "
                    + "is not called yet. intent = " + intent + ", mReady = " + mReady);
            return START_NOT_STICKY;
        }

        UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        // Drop any identity adopted earlier.
        uiAutomation.dropShellPermissionIdentity();

        final int testEvent = intent.getIntExtra(Utils.KEY_TEST_EVENT, -1);
        Log.i(TAG, "testEvent = " + testEvent);
        if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST) {
            runWithShellPermissionIdentity(() -> {
                mAlwaysOnHotwordDetector = callCreateAlwaysOnHotwordDetector();
            }, Manifest.permission.MANAGE_HOTWORD_DETECTION);
        } else if (testEvent == Utils.VIS_WITHOUT_MANAGE_HOTWORD_DETECTION_PERMISSION_TEST) {
            runWithShellPermissionIdentity(() -> callCreateAlwaysOnHotwordDetector(),
                    Manifest.permission.BIND_HOTWORD_DETECTION_SERVICE);
        } else if (testEvent == Utils.VIS_HOLD_BIND_HOTWORD_DETECTION_PERMISSION_TEST) {
            runWithShellPermissionIdentity(() -> callCreateAlwaysOnHotwordDetector());
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_DSP_ONDETECT_TEST) {
            // need to retain the identity until the callback is triggered
            uiAutomation.adoptShellPermissionIdentity(RECORD_AUDIO, CAPTURE_AUDIO_HOTWORD);
            if (mAlwaysOnHotwordDetector != null) {
                mAlwaysOnHotwordDetector.triggerHardwareRecognitionEventForTest(/* status */ 0,
                        /* soundModelHandle */ 100, /* captureAvailable */ true,
                        /* captureSession */ 101, /* captureDelayMs */ 1000,
                        /* capturePreambleMs */ 1001, /* triggerInData */ true,
                        createFakeAudioFormat(), new byte[1024]);
            }
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_DSP_ONREJECT_TEST) {
            runWithShellPermissionIdentity(() -> {
                if (mAlwaysOnHotwordDetector != null) {
                    mAlwaysOnHotwordDetector.triggerHardwareRecognitionEventForTest(/* status */ 0,
                            /* soundModelHandle */ 100, /* captureAvailable */ true,
                            /* captureSession */ 101, /* captureDelayMs */ 1000,
                            /* capturePreambleMs */ 1001, /* triggerInData */ true,
                            createFakeAudioFormat(), null);
                }
            });
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_EXTERNAL_SOURCE_ONDETECT_TEST) {
            uiAutomation.adoptShellPermissionIdentity(RECORD_AUDIO, CAPTURE_AUDIO_HOTWORD);
            if (mAlwaysOnHotwordDetector != null) {
                ParcelFileDescriptor audioStream = createFakeAudioStream();
                if (audioStream != null) {
                    mAlwaysOnHotwordDetector.startRecognition(
                            audioStream,
                            createFakeAudioFormat(),
                            createFakePersistableBundleData());
                }
            }
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_FROM_SOFTWARE_TRIGGER_TEST) {
            runWithShellPermissionIdentity(() -> {
                mSoftwareHotwordDetector = callCreateSoftwareHotwordDetector();
            }, Manifest.permission.MANAGE_HOTWORD_DETECTION);
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_MIC_ONDETECT_TEST) {
            uiAutomation.adoptShellPermissionIdentity(RECORD_AUDIO, CAPTURE_AUDIO_HOTWORD);
            if (mSoftwareHotwordDetector != null) {
                mSoftwareHotwordDetector.startRecognition();
            }
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_CALL_STOP_RECOGNITION) {
            if (mSoftwareHotwordDetector != null) {
                mSoftwareHotwordDetector.stopRecognition();
            }
        } else if (testEvent == Utils.HOTWORD_DETECTION_SERVICE_PROCESS_DIED_TEST) {
            runWithShellPermissionIdentity(() -> {
                if (mAlwaysOnHotwordDetector != null) {
                    PersistableBundle persistableBundle = new PersistableBundle();
                    persistableBundle.putInt(Utils.KEY_TEST_SCENARIO,
                            Utils.HOTWORD_DETECTION_SERVICE_ON_UPDATE_STATE_CRASH);
                    mAlwaysOnHotwordDetector.updateState(
                            persistableBundle,
                            createFakeSharedMemoryData());
                }
            }, Manifest.permission.MANAGE_HOTWORD_DETECTION);
        }

        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        closeFakeAudioStream();
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();
    }

    private AlwaysOnHotwordDetector callCreateAlwaysOnHotwordDetector() {
        Log.i(TAG, "callCreateAlwaysOnHotwordDetector()");
        try {
            return createAlwaysOnHotwordDetector(/* keyphrase */ "Hello Android",
                    Locale.forLanguageTag("en-US"),
                    createFakePersistableBundleData(),
                    createFakeSharedMemoryData(),
                    new AlwaysOnHotwordDetector.Callback() {
                        @Override
                        public void onAvailabilityChanged(int status) {
                            Log.i(TAG, "onAvailabilityChanged(" + status + ")");
                        }

                        @Override
                        public void onDetected(AlwaysOnHotwordDetector.EventPayload eventPayload) {
                            Log.i(TAG, "onDetected");
                            broadcastIntentWithResult(
                                    Utils.HOTWORD_DETECTION_SERVICE_ONDETECT_RESULT_INTENT,
                                    new EventPayloadParcelable(eventPayload));
                        }

                        @Override
                        public void onRejected(@NonNull HotwordRejectedResult result) {
                            super.onRejected(result);
                            Log.i(TAG, "onRejected");
                            broadcastIntentWithResult(
                                    Utils.HOTWORD_DETECTION_SERVICE_ONDETECT_RESULT_INTENT,
                                    result);
                        }

                        @Override
                        public void onError() {
                            Log.i(TAG, "onError");
                            broadcastIntentWithResult(
                                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                                    Utils.HOTWORD_DETECTION_SERVICE_GET_ERROR);
                        }

                        @Override
                        public void onRecognitionPaused() {
                            Log.i(TAG, "onRecognitionPaused");
                        }

                        @Override
                        public void onRecognitionResumed() {
                            Log.i(TAG, "onRecognitionResumed");
                        }

                        @Override
                        public void onHotwordDetectionServiceInitialized(int status) {
                            super.onHotwordDetectionServiceInitialized(status);
                            Log.i(TAG, "onHotwordDetectionServiceInitialized");
                            verifyHotwordDetectionServiceInitializedStatus(status);
                        }

                        @Override
                        public void onHotwordDetectionServiceRestarted() {
                            super.onHotwordDetectionServiceRestarted();
                            Log.i(TAG, "onHotwordDetectionServiceRestarted");
                        }
                    });
        } catch (IllegalStateException e) {
            Log.w(TAG, "callCreateAlwaysOnHotwordDetector() exception: " + e);
            broadcastIntentWithResult(
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_ILLEGAL_STATE_EXCEPTION);
        } catch (SecurityException e) {
            Log.w(TAG, "callCreateAlwaysOnHotwordDetector() exception: " + e);
            broadcastIntentWithResult(
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SECURITY_EXCEPTION);
        }
        return null;
    }

    private HotwordDetector callCreateSoftwareHotwordDetector() {
        Log.i(TAG, "callCreateSoftwareHotwordDetector()");
        try {
            return createHotwordDetector(
                    createFakePersistableBundleData(),
                    createFakeSharedMemoryData(),
                    new HotwordDetector.Callback() {
                        @Override
                        public void onDetected(AlwaysOnHotwordDetector.EventPayload eventPayload) {
                            Log.i(TAG, "onDetected");
                            broadcastIntentWithResult(
                                    Utils.HOTWORD_DETECTION_SERVICE_ONDETECT_RESULT_INTENT,
                                    new EventPayloadParcelable(eventPayload));
                        }

                        @Override
                        public void onError() {
                            Log.i(TAG, "onError");
                            broadcastIntentWithResult(
                                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                                    Utils.HOTWORD_DETECTION_SERVICE_GET_ERROR);
                        }

                        @Override
                        public void onRecognitionPaused() {
                            Log.i(TAG, "onRecognitionPaused");
                        }

                        @Override
                        public void onRecognitionResumed() {
                            Log.i(TAG, "onRecognitionResumed");
                        }

                        @Override
                        public void onRejected(HotwordRejectedResult result) {
                            Log.i(TAG, "onRejected");
                        }

                        @Override
                        public void onHotwordDetectionServiceInitialized(int status) {
                            verifyHotwordDetectionServiceInitializedStatus(status);
                        }

                        @Override
                        public void onHotwordDetectionServiceRestarted() {
                            Log.i(TAG, "onHotwordDetectionServiceRestarted");
                        }
                    });
        } catch (Exception e) {
            Log.w(TAG, "callCreateSoftwareHotwordDetector() exception: " + e);
        }
        return null;
    }

    private void broadcastIntentWithResult(String intentName, int result) {
        Intent intent = new Intent(intentName)
                .addFlags(Intent.FLAG_RECEIVER_FOREGROUND | Intent.FLAG_RECEIVER_REGISTERED_ONLY)
                .putExtra(Utils.KEY_TEST_RESULT, result);
        Log.d(TAG, "broadcast intent = " + intent + ", result = " + result);
        sendBroadcast(intent);
    }

    private void broadcastIntentWithResult(String intentName, Parcelable result) {
        Intent intent = new Intent(intentName)
                .addFlags(Intent.FLAG_RECEIVER_FOREGROUND | Intent.FLAG_RECEIVER_REGISTERED_ONLY)
                .putExtra(Utils.KEY_TEST_RESULT, result);
        Log.d(TAG, "broadcast intent = " + intent + ", result = " + result);
        sendBroadcast(intent);
    }

    private SharedMemory createFakeSharedMemoryData() {
        try {
            SharedMemory sharedMemory = SharedMemory.create("SharedMemory", 3);
            ByteBuffer byteBuffer = sharedMemory.mapReadWrite();
            byteBuffer.put(FAKE_BYTE_ARRAY_DATA);
            return sharedMemory;
        } catch (ErrnoException e) {
            Log.w(TAG, "createFakeSharedMemoryData ErrnoException : " + e);
            throw new RuntimeException(e.getMessage());
        }
    }

    private PersistableBundle createFakePersistableBundleData() {
        // TODO : Add more data for testing
        PersistableBundle persistableBundle = new PersistableBundle();
        persistableBundle.putString(KEY_FAKE_DATA, VALUE_FAKE_DATA);
        return persistableBundle;
    }

    private AudioFormat createFakeAudioFormat() {
        return new AudioFormat.Builder()
                .setSampleRate(32000)
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setChannelMask(AudioFormat.CHANNEL_IN_MONO).build();
    }

    private ParcelFileDescriptor createFakeAudioStream() {
        try {
            mTempParcelFileDescriptor = ParcelFileDescriptor.createPipe();
            try (OutputStream fos =
                         new ParcelFileDescriptor.AutoCloseOutputStream(
                                 mTempParcelFileDescriptor[1])) {
                fos.write(FAKE_HOTWORD_AUDIO_DATA, 0, 8);
            } catch (IOException e) {
                Log.w(TAG, "Failed to pipe audio data : ", e);
                return null;
            }
            return mTempParcelFileDescriptor[0];
        } catch (IOException e) {
            Log.w(TAG, "Failed to create a pipe : " + e);
        }
        return null;
    }

    private void closeFakeAudioStream() {
        if (mTempParcelFileDescriptor != null) {
            try {
                mTempParcelFileDescriptor[0].close();
                mTempParcelFileDescriptor[1].close();
            } catch (IOException e) {
                Log.w(TAG, "Failed closing : " + e);
            }
            mTempParcelFileDescriptor = null;
        }
    }

    private void verifyHotwordDetectionServiceInitializedStatus(int status) {
        if (status == HotwordDetectionService.INITIALIZATION_STATUS_SUCCESS) {
            broadcastIntentWithResult(
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);
        }
    }
}
