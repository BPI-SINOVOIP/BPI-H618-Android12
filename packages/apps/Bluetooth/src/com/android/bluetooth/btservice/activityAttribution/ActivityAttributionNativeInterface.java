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

/*
 * Defines the native interface that is used by state machine/service to
 * send or receive messages from the native stack. This file is registered
 * for the native methods in the corresponding JNI C++ file.
 */
package com.android.bluetooth.btservice.activityattribution;

import android.util.Log;

import com.android.internal.annotations.GuardedBy;

/** ActivityAttribution Native Interface to/from JNI. */
public class ActivityAttributionNativeInterface {
    private static final boolean DBG = false;
    private static final String TAG = "ActivityAttributionNativeInterface";

    @GuardedBy("INSTANCE_LOCK")
    private static ActivityAttributionNativeInterface sInstance;

    private static final Object INSTANCE_LOCK = new Object();

    static {
        classInitNative();
    }

    /** Get singleton instance. */
    public static ActivityAttributionNativeInterface getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sInstance == null) {
                sInstance = new ActivityAttributionNativeInterface();
            }
            return sInstance;
        }
    }

    /** Initializes the native interface. */
    public void init() {
        initNative();
    }

    /** Cleanup the native interface. */
    public void cleanup() {
        cleanupNative();
    }

    // Callbacks from the native stack back into the Java framework.
    // All callbacks are routed via the Service which will disambiguate which
    // state machine the message should be routed to.

    private void onWakeup(int activity, byte[] address) {
        Log.i(TAG, "onWakeup() BTAA: " + activity);
    }

    private void onActivityLogsReady(byte[] logs) {
        Log.i(TAG, "onActivityLogsReady() BTAA: " + logs);
    }

    // Native methods that call into the JNI interface
    private static native void classInitNative();

    private native void initNative();

    private native void cleanupNative();
}
