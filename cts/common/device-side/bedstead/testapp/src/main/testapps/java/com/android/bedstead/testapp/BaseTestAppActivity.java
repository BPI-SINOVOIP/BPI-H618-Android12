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

package com.android.bedstead.testapp;

import android.app.Activity;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.util.Log;

import com.android.eventlib.premade.EventLibActivity;

import java.util.Map;
import java.util.WeakHashMap;

import javax.annotation.Nullable;

/**
 * An {@link Activity} which logs events for all lifecycle events and supports TestApp Features.
 */
public class BaseTestAppActivity extends EventLibActivity {

    private static Map<String, BaseTestAppActivity> sActivities = new WeakHashMap<>();

    /**
     * Find an activity for the given class name.
     *
     * <p>This will return {@code null} if there is no existing activity for the class name.
     *
     * <p>This method is thread-safe
     */
    @Nullable
    public static BaseTestAppActivity findActivity(String activityClassName) {
        synchronized (BaseTestAppActivity.class) {
            return sActivities.get(activityClassName);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        synchronized (BaseTestAppActivity.class) {
            sActivities.put(getClassName(), this);
        }
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreate(Bundle savedInstanceState, PersistableBundle persistentState) {
        synchronized (BaseTestAppActivity.class) {
            sActivities.put(getClassName(), this);
        }
        super.onCreate(savedInstanceState, persistentState);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onRestart() {
        super.onRestart();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
