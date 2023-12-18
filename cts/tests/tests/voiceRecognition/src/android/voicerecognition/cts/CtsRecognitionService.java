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

package android.voicerecognition.cts;

import static android.voicerecognition.cts.TestObjects.ERROR_CODE;
import static android.voicerecognition.cts.TestObjects.PARTIAL_RESULTS_BUNDLE;
import static android.voicerecognition.cts.TestObjects.READY_FOR_SPEECH_BUNDLE;
import static android.voicerecognition.cts.TestObjects.RESULTS_BUNDLE;
import static android.voicerecognition.cts.TestObjects.RMS_CHANGED_VALUE;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.fail;

import android.content.Intent;
import android.os.RemoteException;
import android.speech.RecognitionService;
import android.util.Log;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;

public class CtsRecognitionService extends RecognitionService {
    private static final String TAG = CtsRecognitionService.class.getSimpleName();

    public static List<RecognizerMethod> sInvokedRecognizerMethods = new ArrayList<>();
    public static Queue<CallbackMethod> sInstructedCallbackMethods = new ArrayDeque<>();
    public static AtomicBoolean sIsActive = new AtomicBoolean(false);

    private final Random mRandom = new Random();

    @Override
    protected void onStartListening(Intent recognizerIntent, Callback listener) {
        sIsActive.set(true);
        assertThat(listener.getCallingUid()).isEqualTo(android.os.Process.myUid());

        sInvokedRecognizerMethods.add(RecognizerMethod.RECOGNIZER_METHOD_START_LISTENING);

        maybeRespond(listener);
        sIsActive.set(false);
    }

    @Override
    protected void onStopListening(Callback listener) {
        sIsActive.set(true);
        assertThat(listener.getCallingUid()).isEqualTo(android.os.Process.myUid());

        sInvokedRecognizerMethods.add(RecognizerMethod.RECOGNIZER_METHOD_STOP_LISTENING);

        maybeRespond(listener);
        sIsActive.set(false);
    }

    @Override
    protected void onCancel(Callback listener) {
        sIsActive.set(true);
        assertThat(listener.getCallingUid()).isEqualTo(android.os.Process.myUid());

        sInvokedRecognizerMethods.add(RecognizerMethod.RECOGNIZER_METHOD_CANCEL);

        maybeRespond(listener);
        sIsActive.set(false);
    }

    private void maybeRespond(Callback listener) {
        if (sInstructedCallbackMethods.isEmpty()) {
            return;
        }

        CallbackMethod callbackMethod = sInstructedCallbackMethods.poll();

        Log.i(TAG, "Responding with callback method " + callbackMethod.name());

        try {
            switch (callbackMethod) {
                case CALLBACK_METHOD_UNSPECIFIED:
                    // ignore
                    break;
                case CALLBACK_METHOD_BEGINNING_OF_SPEECH:
                    listener.beginningOfSpeech();
                    break;
                case CALLBACK_METHOD_BUFFER_RECEIVED:
                    byte[] buffer = new byte[100];
                    mRandom.nextBytes(buffer);
                    listener.bufferReceived(buffer);
                    break;
                case CALLBACK_METHOD_END_OF_SPEECH:
                    listener.endOfSpeech();
                    break;
                case CALLBACK_METHOD_ERROR:
                    listener.error(ERROR_CODE);
                    break;
                case CALLBACK_METHOD_RESULTS:
                    listener.results(RESULTS_BUNDLE);
                    break;
                case CALLBACK_METHOD_PARTIAL_RESULTS:
                    listener.partialResults(PARTIAL_RESULTS_BUNDLE);
                    break;
                case CALLBACK_METHOD_READY_FOR_SPEECH:
                    listener.readyForSpeech(READY_FOR_SPEECH_BUNDLE);
                    break;
                case CALLBACK_METHOD_RMS_CHANGED:
                    listener.rmsChanged(RMS_CHANGED_VALUE);
                    break;
                default:
                    fail();
            }
        } catch (RemoteException e) {
            fail();
        }
    }
}
