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

import static com.google.common.truth.Truth.assertThat;

import android.content.Intent;
import android.platform.test.annotations.AppModeFull;
import android.voiceinteraction.common.Utils;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for using the VoiceInteractionService without a basic HotwordDetectionService.
 */
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "No real use case for instant mode hotword detection service")
public final class HotwordDetectionServiceNonExistenceTest
        extends AbstractVoiceInteractionBasicTestCase {
    static final String TAG = "HotwordDetectionServiceNonExistenceTest";

    @Test
    public void testHotwordDetectionService_noHotwordDetectionComponentName_triggerFailure()
            throws Throwable {
        final BlockingBroadcastReceiver receiver = new BlockingBroadcastReceiver(mContext,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT);
        receiver.register();

        mActivityTestRule.getScenario().onActivity(activity -> {
            activity.triggerHotwordDetectionServiceTest(
                    Utils.HOTWORD_DETECTION_SERVICE_NONE,
                    Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST);
        });

        final Intent intent = receiver.awaitForBroadcast(TIMEOUT_MS);
        assertThat(intent).isNotNull();
        assertThat(intent.getIntExtra(Utils.KEY_TEST_RESULT, -1)).isEqualTo(
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_ILLEGAL_STATE_EXCEPTION);

        receiver.unregisterQuietly();
    }

    @Override
    public String getVoiceInteractionService() {
        return "android.voiceinteraction.cts/"
                + "android.voiceinteraction.service.MainInteractionService";
    }
}
