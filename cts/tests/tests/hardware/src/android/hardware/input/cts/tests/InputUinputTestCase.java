/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.input.cts.tests;

import com.android.cts.input.UinputDevice;
import com.android.cts.input.UinputTestData;

import java.util.List;

public class InputUinputTestCase extends InputTestCase {
    private static final String TAG = "InputUinputTestCase";
    private UinputDevice mUinputDevice;

    InputUinputTestCase(int registerResourceId) {
        super(registerResourceId);
    }

    @Override
    protected void setUpDevice(int id, int vendorId, int productId, int sources,
            String registerCommand) {
        mUinputDevice = new UinputDevice(mInstrumentation, id, vendorId, productId, sources,
                registerCommand);
    }

    @Override
    protected void tearDownDevice() {
        if (mUinputDevice != null) {
            mUinputDevice.close();
        }
    }

    @Override
    protected void testInputDeviceEvents(int resourceId) {
        List<UinputTestData> tests = mParser.getUinputTestData(resourceId);

        for (UinputTestData testData: tests) {
            mCurrentTestCase = testData.name;

            // Send all of the evdev Events
            for (int i = 0; i < testData.evdevEvents.size(); i++) {
                final String injections = testData.evdevEvents.get(i);
                mUinputDevice.injectEvents(injections);
            }
            verifyEvents(testData.events);
        }
    }
}
