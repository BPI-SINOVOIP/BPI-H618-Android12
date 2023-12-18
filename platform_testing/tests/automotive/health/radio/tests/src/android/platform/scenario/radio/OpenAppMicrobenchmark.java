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
package android.platform.test.scenario.radio;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoGenericAppHelper;
import android.platform.test.scenario.AppStartupRunRule;
import android.platform.test.microbenchmark.Microbenchmark;

import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.runner.RunWith;

@RunWith(Microbenchmark.class)
public class OpenAppMicrobenchmark extends OpenApp {
    @Rule public AppStartupRunRule mAppStartupRunRule = new AppStartupRunRule<>(sHelper.get());

    private static final String RADIO_PACKAGE = "com.android.car.radio";
    private static final String MEDIA_TEMPLATE = "android.car.intent.action.MEDIA_TEMPLATE";
    private static final Map<String, String> RADIO_SERVICE_EXTRA_ARGS =
            Stream.of(
                            new Object[][] {
                                {
                                    "android.car.intent.extra.MEDIA_COMPONENT",
                                    "com.android.car.radio/com.android.car.radio.service.RadioAppService"
                                },
                            })
                    .collect(Collectors.toMap(data -> (String) data[0], data -> (String) data[1]));

    static HelperAccessor<IAutoGenericAppHelper> sRadioServiceHelper =
            new HelperAccessor<>(IAutoGenericAppHelper.class);

    static {
        sRadioServiceHelper.get().setPackage(RADIO_PACKAGE);
        sRadioServiceHelper.get().setLaunchAction(MEDIA_TEMPLATE, RADIO_SERVICE_EXTRA_ARGS);
    }

    @BeforeClass
    public static void setUp() {
        // Open radio via com.android.car.radio.service.RadioAppService once otherwise the
        // foreground app could be stuck at some other media apps(e.g. Bluetooth).
        sRadioServiceHelper.get().open();
    }
}
