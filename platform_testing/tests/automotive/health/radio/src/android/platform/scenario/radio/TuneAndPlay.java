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

import static org.junit.Assume.assumeTrue;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoRadioHelper;
import android.platform.test.scenario.annotation.Scenario;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import androidx.test.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** This scenario will tune and play an FM station, assuming radio app is open. */
@Scenario
@RunWith(JUnit4.class)
public class TuneAndPlay {

    private static final long OPEN_RADIO_TIMEOUT_MS = 10000;
    private static HelperAccessor<IAutoRadioHelper> sHelper =
            new HelperAccessor<>(IAutoRadioHelper.class);

    @Before
    public void checkApp() {
        // TODO: Remove the checking logic and use default helper open() method after migrating to Q
        UiDevice device = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        String pkgName = sHelper.get().getPackage();
        assumeTrue(device.wait(Until.hasObject(By.pkg(pkgName).depth(0)), OPEN_RADIO_TIMEOUT_MS));
    }

    @Test
    public void testTuneAndPlay() {
        sHelper.get().setStation("FM", 92.3);
        sHelper.get().playRadio();
    }
}
