/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.softwinner.display;

import android.os.Looper;
import android.util.Log;
import android.os.ServiceManager;
import android.os.DisplayOutputManagerService;

public final class DisplayDaemon {
    private static final String TAG = "DisplayDaemon";
    private static final boolean DEBUG = true;
    private Looper mLooper;
    private DisplayOutputManagerService mService;

    public static void main(String[] args) {
        if (DEBUG) Log.d(TAG, "DisplayDaemon.main " + args);
        int exitCode = 1;
        try {
            exitCode = new DisplayDaemon().run(args);
        } catch (Exception e) {
            Log.e(TAG, "Error", e);
        }
        System.exit(exitCode);
    }

    public int run(String[] args)  {
        Looper.prepare();
        mLooper = Looper.myLooper();
        mService = new DisplayOutputManagerService();

        Log.d(TAG, "Starting DisplayOutputManagerService");
        ServiceManager.addService("display_output", mService);
        Looper.loop();
        Log.w(TAG, "DisplayDaemon exit!");
        return 0;
    }
}

