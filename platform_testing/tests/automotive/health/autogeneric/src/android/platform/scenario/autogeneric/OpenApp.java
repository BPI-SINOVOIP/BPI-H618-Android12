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

package android.platform.test.scenario.autogeneric;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoGenericAppHelper;
import android.platform.test.option.StringOption;
import android.platform.test.scenario.annotation.Scenario;

import org.junit.ClassRule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Open an app by specifying either launch activity or package. */
@Scenario
@RunWith(JUnit4.class)
public class OpenApp {
    @ClassRule
    public static StringOption mPackageOption = new StringOption("pkg").setRequired(false);

    @ClassRule
    public static StringOption mLaunchActivityOption =
            new StringOption("launch-activity").setRequired(false);

    static HelperAccessor<IAutoGenericAppHelper> sHelper =
            new HelperAccessor<>(IAutoGenericAppHelper.class);

    public OpenApp() {
        if (mLaunchActivityOption.get() != null) {
            sHelper.get().setLaunchActivity(mLaunchActivityOption.get());
        } else if (mPackageOption.get() != null) {
            sHelper.get().setPackage(mPackageOption.get());
        } else {
            throw new IllegalStateException("Neither launch activity nor package is set.");
        }
    }

    @Test
    public void testOpen() {
        sHelper.get().open();
    }
}
