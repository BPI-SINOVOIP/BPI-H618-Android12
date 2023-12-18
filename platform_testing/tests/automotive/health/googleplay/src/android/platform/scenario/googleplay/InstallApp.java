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
package android.platform.test.scenario.googleplay;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoGooglePlayHelper;
import android.platform.test.scenario.annotation.Scenario;

import androidx.test.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * This is test scenario to install app from Google Play Store. It assumes the Play Store app is
 * already open
 */
@Scenario
@RunWith(JUnit4.class)
public class InstallApp {
    // The argument keys are accessible to other class in the same package
    static final String APP_NAME_PARAM = "app_name_on_play_store";
    static final String DEFAULT_APP_NAME = "NPR One";

    private static HelperAccessor<IAutoGooglePlayHelper> sHelper =
            new HelperAccessor<>(IAutoGooglePlayHelper.class);

    private String mAppName;

    public InstallApp() {
        mAppName =
                InstrumentationRegistry.getArguments().getString(APP_NAME_PARAM, DEFAULT_APP_NAME);
    }

    @Test
    public void testSearchAndInstall() {
        sHelper.get().searchAndClick(mAppName);
        sHelper.get().installApp();
    }
}
