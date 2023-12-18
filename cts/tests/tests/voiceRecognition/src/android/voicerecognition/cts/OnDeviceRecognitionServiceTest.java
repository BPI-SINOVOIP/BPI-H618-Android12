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

import static androidx.test.InstrumentationRegistry.getInstrumentation;

import android.content.ComponentName;
import android.speech.SpeechRecognizer;
import android.util.Log;

import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.runner.RunWith;

/** Recognition service tests for a default speech recognition service. */
@RunWith(AndroidJUnit4.class)
public final class OnDeviceRecognitionServiceTest extends AbstractRecognitionServiceTest {
    private static final String TAG = OnDeviceRecognitionServiceTest.class.getSimpleName();

    private SpeechRecognizer mRecognizer;

    @Before
    public void setUp() {
        getInstrumentation().getUiAutomation().adoptShellPermissionIdentity(
                "android.permission.MANAGE_SPEECH_RECOGNITION");
    }

    @After
    public void tearDown() {
        if (mRecognizer != null) {
            mRecognizer.setTemporaryOnDeviceRecognizer(null);
            mRecognizer = null;
        }

        getInstrumentation().getUiAutomation().dropShellPermissionIdentity();
    }

    @Override
    protected void setCurrentRecognizer(SpeechRecognizer recognizer, String component) {
        Log.i(TAG, "Setting recognizer to " + component);
        mRecognizer = recognizer;
        recognizer.setTemporaryOnDeviceRecognizer(ComponentName.unflattenFromString(component));
    }

    @Override
    boolean isOnDeviceTest() {
        return true;
    }

    @Override
    String customRecognizer() {
        // We will use the default one (specified in config).
        return null;
    }
}
