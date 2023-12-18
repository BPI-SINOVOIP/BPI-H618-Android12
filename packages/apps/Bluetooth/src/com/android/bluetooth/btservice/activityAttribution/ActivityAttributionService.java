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

package com.android.bluetooth.btservice.activityattribution;

import android.util.Log;

import java.util.Objects;

/**
 * Service used for attributes wakeup, wakelock and Bluetooth traffic into per-app and per-device
 * based activities.
 */
public class ActivityAttributionService {
    private boolean mCleaningUp;
    private static ActivityAttributionService sActivityAttributionService;
    private static final boolean DBG = false;
    private static final String TAG = "ActivityAttributionService";

    ActivityAttributionNativeInterface mActivityAttributionNativeInterface;

    /** Start and initialize the Activity Attribution service. */
    public void start() {
        debugLog("start()");

        if (sActivityAttributionService != null) {
            Log.e(TAG, "start() called twice");
            return;
        }

        mActivityAttributionNativeInterface =
                Objects.requireNonNull(
                        ActivityAttributionNativeInterface.getInstance(),
                        "ActivityAttributionNativeInterface "
                                + "cannot be null when ActivityAttributionService starts");

        // Mark service as started
        setActivityAttributionService(this);
    }

    /** Cleans up the Activity Attribution service. */
    public void cleanup() {
        debugLog("cleanup");
        if (mCleaningUp) {
            debugLog("already doing cleanup");
            return;
        }

        mCleaningUp = true;

        if (sActivityAttributionService == null) {
            debugLog("cleanup() called before start()");
            return;
        }

        // Mark service as stopped
        setActivityAttributionService(null);

        // Cleanup native interface
        mActivityAttributionNativeInterface.cleanup();
        mActivityAttributionNativeInterface = null;
    }

    /** Get the ActivityAttributionService instance */
    public static synchronized ActivityAttributionService getActivityAttributionService() {
        if (sActivityAttributionService == null) {
            Log.w(TAG, "getActivityAttributionService(): service is NULL");
            return null;
        }

        if (!sActivityAttributionService.isAvailable()) {
            Log.w(TAG, "getActivityAttributionService(): service is not available");
            return null;
        }
        return sActivityAttributionService;
    }

    /** Init JNI */
    public void initJni() {
        debugLog("initJni()");
        // Initialize native interface
        mActivityAttributionNativeInterface.init();
    }

    private boolean isAvailable() {
        return !mCleaningUp;
    }

    private static synchronized void setActivityAttributionService(
            ActivityAttributionService instance) {
        debugLog("setActivityAttributionService(): set to: " + instance);
        sActivityAttributionService = instance;
    }

    private static void debugLog(String msg) {
        if (DBG) {
            Log.d(TAG, msg);
        }
    }
}
