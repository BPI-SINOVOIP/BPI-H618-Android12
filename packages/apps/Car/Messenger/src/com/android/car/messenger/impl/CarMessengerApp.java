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

package com.android.car.messenger.impl;

import android.app.Application;
import android.os.Handler;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.util.L;

import java.lang.Thread.UncaughtExceptionHandler;

/** The application object */
public class CarMessengerApp extends Application implements UncaughtExceptionHandler {
    @Nullable private static UncaughtExceptionHandler sSystemUncaughtExceptionHandler;

    @Override
    public void onCreate() {
        super.onCreate();
        AppFactoryImpl.register(this);
        sSystemUncaughtExceptionHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(this);
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
        L.d("onLowMemory");
    }

    @Override
    public void uncaughtException(final Thread thread, final Throwable ex) {
        final boolean background = getMainLooper().getThread() != thread;
        if (background) {
            L.e("Uncaught exception in background thread " + thread, ex);
            final Handler handler = new Handler(getMainLooper());
            handler.post(() -> nullSafeUncaughtException(thread, ex));
        } else {
            nullSafeUncaughtException(thread, ex);
        }
    }

    private static void nullSafeUncaughtException(@NonNull Thread thread, @NonNull Throwable ex) {
        if (sSystemUncaughtExceptionHandler != null) {
            sSystemUncaughtExceptionHandler.uncaughtException(thread, ex);
        }
    }
}
