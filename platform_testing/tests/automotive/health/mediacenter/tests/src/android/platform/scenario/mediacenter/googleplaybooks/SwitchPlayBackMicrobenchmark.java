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
package android.platform.test.scenario.mediacenter.googleplaybooks;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoGenericAppHelper;
import android.platform.test.microbenchmark.Microbenchmark;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.runner.RunWith;

@RunWith(Microbenchmark.class)
public class SwitchPlayBackMicrobenchmark extends SwitchPlayBack {
    private static final String MEDIA_CENTER_PACKAGE = "com.android.car.media";

    private static HelperAccessor<IAutoGenericAppHelper> sHelper =
            new HelperAccessor<>(IAutoGenericAppHelper.class);

    static {
        sHelper.get().setPackage(MEDIA_CENTER_PACKAGE);
    }

    @BeforeClass
    public static void openApp() {
        sHelper.get().open();
    }

    @AfterClass
    public static void closeApp() {
        sHelper.get().exit();
    }
}
