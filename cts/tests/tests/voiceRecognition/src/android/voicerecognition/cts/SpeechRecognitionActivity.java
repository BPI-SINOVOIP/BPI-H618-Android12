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
 * limitations under the License
 */

package android.voicerecognition.cts;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

/**
 * An activity that uses SpeechRecognition APIs. SpeechRecognition will bind the RecognitionService
 * to provide the voice recognition functions.
 */
public class SpeechRecognitionActivity extends Activity {
    private final String TAG = "SpeechRecognitionActivity";

    SpeechRecognizer mRecognizer;

    private Handler mHandler;
    private SpeechRecognizerListener mListener;

    final List<CallbackMethod> mCallbackMethodsInvoked = new ArrayList<>();

    public boolean mStartListeningCalled;
    public CountDownLatch mCountDownLatch;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    }

    @Override
    protected void onDestroy() {
        if (mRecognizer != null) {
            mRecognizer.destroy();
            mRecognizer = null;
        }
        super.onDestroy();
    }

    public void startListening() {
        final Intent recognizerIntent =
                new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        recognizerIntent.putExtra(
                RecognizerIntent.EXTRA_CALLING_PACKAGE, this.getPackageName());
        startListening(recognizerIntent);
    }

    public void startListening(Intent intent) {
        mHandler.post(() -> mRecognizer.startListening(intent));
    }

    public void stopListening() {
        mHandler.post(mRecognizer::stopListening);
    }

    public void cancel() {
        mHandler.post(mRecognizer::cancel);
    }

    public void destroyRecognizer() {
        mHandler.post(mRecognizer::destroy);
    }

    public void init(boolean onDevice, String customRecognizerComponent) {
        mHandler = new Handler(getMainLooper());
        mHandler.post(() -> {
            if (onDevice) {
                mRecognizer = SpeechRecognizer.createOnDeviceTestingSpeechRecognizer(this);
            } else if (customRecognizerComponent != null) {
                mRecognizer = SpeechRecognizer.createSpeechRecognizer(this,
                        ComponentName.unflattenFromString(customRecognizerComponent));
            } else {
                mRecognizer = SpeechRecognizer.createSpeechRecognizer(this);
            }

            mListener = new SpeechRecognizerListener();
            mRecognizer.setRecognitionListener(mListener);
            mRecognizer.setRecognitionListener(mListener);
            mStartListeningCalled = false;
            mCountDownLatch = new CountDownLatch(1);
        });
    }

    private class SpeechRecognizerListener implements RecognitionListener {

        @Override
        public void onReadyForSpeech(Bundle params) {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_READY_FOR_SPEECH);
        }

        @Override
        public void onBeginningOfSpeech() {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_BEGINNING_OF_SPEECH);
        }

        @Override
        public void onRmsChanged(float rmsdB) {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_RMS_CHANGED);
        }

        @Override
        public void onBufferReceived(byte[] buffer) {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_BUFFER_RECEIVED);
        }

        @Override
        public void onEndOfSpeech() {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_END_OF_SPEECH);
        }

        @Override
        public void onError(int error) {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_ERROR);
        }

        @Override
        public void onResults(Bundle results) {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_RESULTS);
            mStartListeningCalled = true;
            mCountDownLatch.countDown();
        }

        @Override
        public void onPartialResults(Bundle partialResults) {
            mCallbackMethodsInvoked.add(CallbackMethod.CALLBACK_METHOD_PARTIAL_RESULTS);
        }

        @Override
        public void onEvent(int eventType, Bundle params) {
        }
    }
}
