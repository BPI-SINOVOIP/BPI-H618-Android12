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
import android.platform.helpers.exceptions.UnknownUiException;
import android.platform.test.scenario.annotation.Scenario;

import androidx.test.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * This is a test to search and open an App on Play Store. It assumes the App is already installed,
 * otherwise the test will throw exception.
 */
@Scenario
@RunWith(JUnit4.class)
public class OpenAppOnPlayStore {

    private static HelperAccessor<IAutoGooglePlayHelper> sHelper =
            new HelperAccessor<>(IAutoGooglePlayHelper.class);

    private static String mAppName;

    public OpenAppOnPlayStore() {
        mAppName =
                InstrumentationRegistry.getArguments()
                        .getString(InstallApp.APP_NAME_PARAM, InstallApp.DEFAULT_APP_NAME);
    }

    @Test
    public void openApp() {
        try {
            sHelper.get().openApp();
        } catch (UnknownUiException e) {
            // If app can not be opened directly, try search for the app first then open it.
            sHelper.get().searchAndClick(mAppName);
            sHelper.get().openApp();
        }
    }
}
