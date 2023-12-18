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

package com.android.car.messenger.core.util;

import android.util.Log;

import androidx.annotation.NonNull;

/** Util class for logging. */
public class L {
    @NonNull private static final String TAG = "CarMessenger";

    private L() {}

    /**
     * Logs verbose level logs if loggable.
     *
     * @param msg the message to log, as a format string
     */
    public static void v(@NonNull String msg) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, msg);
        }
    }

    /**
     * Logs debug level logs if loggable.
     *
     * @param msg the message to log, as a format string
     */
    public static void d(@NonNull String msg) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, msg);
        }
    }

    /**
     * Logs info level logs if loggable.
     *
     * @param msg the message to log, as a format string
     */
    public static void i(@NonNull String msg) {
        if (Log.isLoggable(TAG, Log.INFO)) {
            Log.i(TAG, msg);
        }
    }

    /**
     * Logs warning level logs if loggable.
     *
     * @param msg the message to log, as a format string
     */
    public static void w(@NonNull String msg) {
        if (Log.isLoggable(TAG, Log.WARN)) {
            Log.w(TAG, msg);
        }
    }

    /**
     * Logs error level logs.
     *
     * @param msg the message to log, as a format string
     */
    public static void e(@NonNull String msg) {
        Log.e(TAG, msg);
    }

    /**
     * Logs warning level logs.
     *
     * @param msg the message to log, as a format string
     * @param e a throwable to log
     */
    public static void e(@NonNull String msg, Throwable e) {
        Log.e(TAG, msg, e);
    }

    /**
     * Logs conditions that should never happen.
     *
     * @param msg the message to log, as a format string
     */
    public static void wtf(@NonNull String msg) {
        Log.wtf(TAG, msg);
    }

    /**
     * Logs conditions that should never happen.
     *
     * @param e an exception to log
     * @param msg the message to log, as a format string
     */
    public static void wtf(Exception e, @NonNull String msg) {
        Log.wtf(TAG, msg, e);
    }
}
