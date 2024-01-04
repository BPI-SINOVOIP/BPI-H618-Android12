/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.softwinner.utils;

import android.util.Log;
import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

import com.softwinner.utils.HdmiCtsUtils;
import com.softwinner.utils.HdmiCapability;
import com.softwinner.utils.HdmiOutputConfig;

public class hdmictsDemo extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        HdmiOutputConfig testconfig = new HdmiOutputConfig();
        testconfig.deviceType = 1234;
        testconfig.aspectRatio = 4;
        HdmiCtsUtils.setHdmiOutputConfig(testconfig);


        HdmiCapability cap = HdmiCtsUtils.getHdmiCapability();
        Log.d("XXX", "HdmiCapability deviceType: " + cap.deviceType);
        Log.d("XXX", "HdmiCapability supportedHdrMode: " + cap.supportedHdrMode);
        Log.d("XXX", "HdmiCapability supportedColorSpace: " + cap.supportedColorSpace);
        Log.d("XXX", "HdmiCapability supportedColorDepth: " + cap.supportedColorDepth);
        Log.d("XXX", "HdmiCapability supportedColorimetry: " + cap.supportedColorimetry);
        Log.d("XXX", "HdmiCapability supportedValueRange: " + cap.supportedValueRange);
        Log.d("XXX", "HdmiCapability supportedScanMode: " + cap.supportedScanMode);

        int[] supportmodes = HdmiCtsUtils.getHdmiSupportMode();

        Log.d("XXX", "Supported Mode cnt: " + supportmodes.length);
        for (int i = 0; i < supportmodes.length; i++) {
            Log.d("XXX", "   " + supportmodes[i]);
        }

    }
}
