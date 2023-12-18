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
package android.platform.test.scenario.mediacenter.testmedia;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoGenericAppHelper;
import android.platform.test.scenario.annotation.Scenario;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Scroll down and up in Google play Books' content list. */
@Scenario
@RunWith(JUnit4.class)
public class Scroll {
    private static final String MEDIA_CENTER_PACKAGE = "com.android.car.media";

    static HelperAccessor<IAutoGenericAppHelper> sAutoGenericHelper =
            new HelperAccessor<>(IAutoGenericAppHelper.class);

    static {
        sAutoGenericHelper.get().setPackage(MEDIA_CENTER_PACKAGE);
        // Media center has some UI components overlays the scrollable region at both the top and
        // bottom section, and hence a large margin is required.
        sAutoGenericHelper.get().setScrollableMargin(0, 200, 0, 200);
    }

    @Test
    public void testScrollDownAndUp() {
        sAutoGenericHelper.get().scrollDownOnePage(2500);
        sAutoGenericHelper.get().scrollUpOnePage(2500);
    }
}
