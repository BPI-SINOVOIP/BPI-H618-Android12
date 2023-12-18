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

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static com.android.compatibility.common.util.ShellUtils.runShellCommand;

import android.content.Context;
import android.provider.Settings;

import com.android.compatibility.common.util.SettingsStateChangerRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.runner.RunWith;

import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;

/**
 * Base class for using the VoiceInteractionService that included a basic HotwordDetectionService.
 */
@RunWith(AndroidJUnit4.class)
abstract class AbstractVoiceInteractionBasicTestCase {
    // TODO: (b/181943521) Combine the duplicated test class

    protected static final int TIMEOUT_MS = 5 * 1000;

    protected final Context mContext = getInstrumentation().getTargetContext();

    @Rule
    public final SettingsStateChangerRule mServiceSetterRule = new SettingsStateChangerRule(
            mContext, Settings.Secure.VOICE_INTERACTION_SERVICE, getVoiceInteractionService());

    @Rule
    public final ActivityScenarioRule<TestVoiceInteractionServiceActivity> mActivityTestRule =
            new ActivityScenarioRule<>(TestVoiceInteractionServiceActivity.class);

    @Before
    public void prepareDevice() throws Exception {
        // Unlock screen.
        runShellCommand("input keyevent KEYCODE_WAKEUP");

        // Dismiss keyguard, in case it's set as "Swipe to unlock".
        runShellCommand("wm dismiss-keyguard");
    }

    public abstract String getVoiceInteractionService();
}
