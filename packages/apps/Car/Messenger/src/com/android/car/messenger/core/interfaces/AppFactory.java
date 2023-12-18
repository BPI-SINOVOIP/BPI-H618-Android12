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

package com.android.car.messenger.core.interfaces;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;

/**
 * The AppFactory provides singleton instances to be used throughout the app.
 *
 * <p>The Factory implementation points to the UI core library interfaces.
 *
 * <p>Once the Factory implementation is initialized with {@link #setInstance(AppFactory)}, the
 * library interface implementations can be accessed anywhere throughout the application.
 */
public abstract class AppFactory {
    @NonNull private static AppFactory sInstance;
    protected static boolean sRegistered;
    protected static boolean sInitialized;

    /** Returns the Factory instance for the Application. */
    @NonNull
    public static AppFactory get() {
        return sInstance;
    }

    /**
     * Sets the Factory instance.
     *
     * <p>This is called when the application starts, in onCreate of the custom Application class
     */
    protected static void setInstance(@NonNull final AppFactory factory) {
        // Not allowed to call this after real application initialization is complete
        if (sRegistered && sInitialized) {
            return;
        }
        sInstance = factory;
    }

    /** Returns context most appropriate for UI context-requiring tasks. */
    @NonNull
    public abstract Context getContext();

    /**
     * Perhaps the single most important methods to implement, this provides the data source for the
     * app.
     */
    @NonNull
    public abstract DataModel getDataModel();

    /** Returns the shared preference instance for the app */
    @NonNull
    public abstract SharedPreferences getSharedPreferences();
}
