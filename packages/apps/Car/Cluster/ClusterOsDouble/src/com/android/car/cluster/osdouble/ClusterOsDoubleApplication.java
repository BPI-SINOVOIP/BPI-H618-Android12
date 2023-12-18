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

package com.android.car.cluster.osdouble;

import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;

import android.app.ActivityOptions;
import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.util.Log;
import android.view.Display;
import android.view.DisplayAddress;

/**
 * Application class to start ClusterOsDoubleActivity on the physical cluster display.
 */
public class ClusterOsDoubleApplication extends Application {
    public static final String TAG = "ClusterOsDouble";

    private DisplayManager mDisplayManager;

    @Override
    public void onCreate() {
        super.onCreate();
        Context context = getApplicationContext();
        mDisplayManager = context.getSystemService(DisplayManager.class);
        int displayPort = context.getResources().getInteger(R.integer.config_clusterDisplayPort);
        if (displayPort == 0) {
            Log.e(TAG, "Invalid resource: config_clusterDisplayPort");
            // Won't throw the exception, if so, it'll restart the application continuously,
            // because this is the persistent application.
            return;
        }
        int displayId = findDisplay(displayPort);
        if (displayId == Display.INVALID_DISPLAY) {
            Log.e(TAG, "Can't find the display with portId: " + displayPort);
            return;
        }

        Intent intent = Intent.makeMainActivity(
                ComponentName.createRelative(context, ClusterOsDoubleActivity.class.getName()));
        intent.addFlags(FLAG_ACTIVITY_NEW_TASK);

        ActivityOptions options = ActivityOptions.makeBasic().setLaunchDisplayId(displayId);
        context.startActivity(intent, options.toBundle());
    }

    private int findDisplay(int displayPort) {
        for (Display display : mDisplayManager.getDisplays()) {
            DisplayAddress address = display.getAddress();
            if (!(address instanceof DisplayAddress.Physical)) {
                continue;
            }
            DisplayAddress.Physical physical = (DisplayAddress.Physical) address;
            if (physical.getPort() == displayPort) {
                return display.getDisplayId();
            }
        }
        return Display.INVALID_DISPLAY;
    }
}
