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

package android.voiceinteraction.cts;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.util.Log;
import android.voiceinteraction.common.Utils;
import android.voiceinteraction.service.BasicVoiceInteractionService;
import android.voiceinteraction.service.MainInteractionService;

public class TestVoiceInteractionServiceActivity extends Activity {
    static final String TAG = "TestVoiceInteractionServiceActivity";

    void triggerHotwordDetectionServiceTest(int serviceType, int testEvent) {
        Intent serviceIntent = new Intent();
        if (serviceType == Utils.HOTWORD_DETECTION_SERVICE_NONE) {
            serviceIntent.setComponent(new ComponentName(this,
                    "android.voiceinteraction.service.MainInteractionService"));
        } else if (serviceType == Utils.HOTWORD_DETECTION_SERVICE_BASIC) {
            serviceIntent.setComponent(new ComponentName(this,
                    "android.voiceinteraction.service.BasicVoiceInteractionService"));
        } else {
            Log.w(TAG, "Never here");
            finish();
            return;
        }
        serviceIntent.putExtra(Utils.KEY_TEST_EVENT, testEvent);
        ComponentName serviceName = startService(serviceIntent);
        Log.i(TAG, "triggerHotwordDetectionServiceTest Started service: " + serviceName);
    }
}
