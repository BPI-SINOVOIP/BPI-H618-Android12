/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hdmicec.app;

import android.app.Activity;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.hardware.hdmi.HdmiTvClient;
import android.os.Bundle;
import android.util.Log;

/**
 * A simple activity that can be used to trigger actions using the HdmiControlManager. The actions
 * supported are:
 *
 * <p>
 *
 * <p>1. android.hdmicec.app.OTP: Triggers the OTP
 *
 * <p>Usage: <code>START_COMMAND -a android.hdmicec.app.OTP</code>
 *
 * <p>
 *
 * <p>2. android.hdmicec.app.SELECT_DEVICE: Selects a device to be the active source. The logical
 * address of the device that has to be made the active source has to passed as a parameter.
 *
 * <p>Usage: <code>START_COMMAND -a android.hdmicec.app.DEVICE_SELECT --ei "la" [LOGICAL_ADDRESS]
 * </code>
 *
 * <p>
 *
 * <p>where START_COMMAND is
 *
 * <p><code>
 * adb shell am start -n "android.hdmicec.app/android.hdmicec.app.HdmiControlManagerHelper"
 * </code>
 */
public class HdmiControlManagerHelper extends Activity {

    private static final String TAG = HdmiControlManagerHelper.class.getSimpleName();
    HdmiControlManager mHdmiControlManager;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mHdmiControlManager = getSystemService(HdmiControlManager.class);
        if (mHdmiControlManager == null) {
            Log.i(TAG, "Failed to get HdmiControlManager");
            return;
        }

        switch (getIntent().getAction()) {
            case "android.hdmicec.app.OTP":
                initiateOtp();
                break;
            case "android.hdmicec.app.DEVICE_SELECT":
                int logicalAddress = getIntent().getIntExtra("la", 50);
                deviceSelect(logicalAddress);
            default:
                Log.w(TAG, "Unknown intent!");
        }
    }

    private void deviceSelect(int logicalAddress) {
        HdmiTvClient client = mHdmiControlManager.getTvClient();
        if (client == null) {
            Log.e(TAG, "Failed to get the TV client");
            return;
        }

        client.deviceSelect(
                logicalAddress,
                (result) -> {
                    if (result == HdmiControlManager.RESULT_SUCCESS) {
                        Log.i(TAG, "Selected device with logical address " + logicalAddress);
                    } else {
                        Log.i(
                                TAG,
                                "Could not select device with logical address " + logicalAddress);
                    }
                    finishAndRemoveTask();
                });
    }

    private void initiateOtp() {
        HdmiPlaybackClient client = mHdmiControlManager.getPlaybackClient();
        if (client == null) {
            Log.i(TAG, "Failed to get HdmiPlaybackClient");
            return;
        }

        client.oneTouchPlay(
                (result) -> {
                    if (result == HdmiControlManager.RESULT_SUCCESS) {
                        Log.i(TAG, "OTP successful");
                    } else {
                        Log.i(TAG, "OTP failed");
                    }
                    finishAndRemoveTask();
                });
    }
}
