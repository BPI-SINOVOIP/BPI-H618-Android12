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
import android.app.AppComponentFactory;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.util.Log;

import com.android.bedstead.testapp.processor.annotations.TestAppReceiver;

/**
 * An {@link AppComponentFactory} which redirects invalid class names to premade TestApp classes.
 */
@TestAppReceiver
public final class TestAppAppComponentFactory extends AppComponentFactory {

    private static final String LOG_TAG = "TestAppACF";

    @Override
    public Activity instantiateActivity(ClassLoader classLoader, String className, Intent intent)
            throws InstantiationException, IllegalAccessException, ClassNotFoundException {

        try {
            return super.instantiateActivity(classLoader, className, intent);
        } catch (ClassNotFoundException e) {
            Log.d(LOG_TAG,
                    "Activity class (" + className + ") not found, routing to TestAppActivity");
            BaseTestAppActivity activity =
                    (BaseTestAppActivity) super.instantiateActivity(
                            classLoader, BaseTestAppActivity.class.getName(), intent);
            activity.setOverrideActivityClassName(className);
            return activity;
        }
    }

    @Override
    public BroadcastReceiver instantiateReceiver(ClassLoader classLoader, String className,
            Intent intent)
            throws InstantiationException, IllegalAccessException, ClassNotFoundException {
        try {
            return super.instantiateReceiver(classLoader, className, intent);
        } catch (ClassNotFoundException e) {
            Log.d(LOG_TAG, "Broadcast Receiver class (" + className
                    + ") not found, routing to TestAppBroadcastReceiver");

            BaseTestAppBroadcastReceiver receiver = (BaseTestAppBroadcastReceiver)
                    super.instantiateReceiver(
                            classLoader, BaseTestAppBroadcastReceiver.class.getName(), intent);
            receiver.setOverrideBroadcastReceiverClassName(className);
            return receiver;
        }
    }
}
