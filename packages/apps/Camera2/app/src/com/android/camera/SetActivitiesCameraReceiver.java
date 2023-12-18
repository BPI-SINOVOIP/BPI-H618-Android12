/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.camera;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera.CameraInfo;

import com.android.camera.debug.Log;

// Enable or disable camera-related activities based on "camera hal" in every boot up.
// This receiver runs when BOOT_COMPLETED intent is received.
public class SetActivitiesCameraReceiver extends BroadcastReceiver {
    private static final Log.Tag TAG = new Log.Tag("SetActivitiesCameraReceiver");
    private static final boolean CHECK_BACK_CAMERA_ONLY = false;
    private static final String ACTIVITIES[] = {
        "com.android.camera.CameraLauncher",
    };

    @Override
    public void onReceive(Context context, Intent intent) {
        // Disable camera-related activities if there is no camera.
        int component_state = (CHECK_BACK_CAMERA_ONLY
            ? hasBackCamera()
            : (hasCamera() /*|| supportExternalCamera(context)*/))
            ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
            : PackageManager.COMPONENT_ENABLED_STATE_DISABLED;

        Log.i(TAG, "component state is " + component_state);
        for (int i = 0; i < ACTIVITIES.length; i++) {
            setComponent(context, ACTIVITIES[i],
                component_state);
        }
    }

    private boolean supportExternalCamera(Context context) {
        PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_CAMERA_EXTERNAL);
    }

    private boolean hasCamera() {
        int n = android.hardware.Camera.getNumberOfCameras();
        Log.i(TAG, "number of camera: " + n);
        return (n > 0);
    }

    private boolean hasBackCamera() {
        int n = android.hardware.Camera.getNumberOfCameras();
        CameraInfo info = new CameraInfo();
        for (int i = 0; i < n; i++) {
            android.hardware.Camera.getCameraInfo(i, info);
            if (info.facing == CameraInfo.CAMERA_FACING_BACK) {
                Log.i(TAG, "back camera found: " + i);
                return true;
            }
        }
        Log.i(TAG, "no back camera");
        return false;
    }

    private void setComponent(Context context, String klass, final int enabledState) {
        ComponentName name = new ComponentName(context, klass);
        PackageManager pm = context.getPackageManager();

        // We need the DONT_KILL_APP flag, otherwise we will be killed
        // immediately because we are in the same app.
        pm.setComponentEnabledSetting(name,
                enabledState,
                PackageManager.SYNCHRONOUS);


    }
}
