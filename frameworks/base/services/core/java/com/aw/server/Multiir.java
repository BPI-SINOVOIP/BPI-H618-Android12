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

package com.softwinner;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.provider.Settings;
import android.os.Handler;
import android.os.UserHandle;

import android.util.Log;
import java.io.File;

/**
 * Class that provides access to some of the gpio management functions.
 *
 * {@hide}
 */
public final class Multiir {
    private static final String TAG = "Multiir";
    private static boolean DEBUG = false;
    private static boolean ServiceReady = false;
    private static MouseModeSettingsObserver mObserver;

    private static class MouseModeSettingsObserver extends ContentObserver {
        Context mContext;
        MouseModeSettingsObserver(Handler handler) {
            super(handler);
        }

        void observe(Context context) {
            mContext = context;
            ContentResolver resolver = context.getContentResolver();
            resolver.registerContentObserver(Settings.System.getUriFor(
                        Settings.System.TV_MOUSEMODE_POINTERSPEED), false, this, UserHandle.USER_ALL);
            resolver.registerContentObserver(Settings.System.getUriFor(
                        Settings.System.TV_MOUSEMODE_STEPDISTANCE), false, this, UserHandle.USER_ALL);
            int pointer_speed = Settings.System.getInt(resolver, Settings.System.TV_MOUSEMODE_POINTERSPEED, -1);
            int step_distance = Settings.System.getInt(resolver, Settings.System.TV_MOUSEMODE_STEPDISTANCE, -1);
            if (pointer_speed == -1) {
                pointer_speed = _getDefaultPointerSpeed();
                Settings.System.putInt(resolver, Settings.System.TV_MOUSEMODE_POINTERSPEED, pointer_speed);
            }
            if (step_distance == -1) {
                step_distance = _getDefaultStepDistance();
                Settings.System.putInt(resolver, Settings.System.TV_MOUSEMODE_STEPDISTANCE, step_distance);
            }
        }

        @Override
        public void onChange(boolean selfChange) {
            if (mContext == null)
                return;
            ContentResolver resolver = mContext.getContentResolver();
            int pointer_speed = Settings.System.getInt(resolver, Settings.System.TV_MOUSEMODE_POINTERSPEED, -1);
            int step_distance = Settings.System.getInt(resolver, Settings.System.TV_MOUSEMODE_STEPDISTANCE, -1);
            if (pointer_speed > 0)
                _setPointerSpeed(pointer_speed);
            if (step_distance > 0)
                _setStepDistance(step_distance);
        }
    }

    // can't instantiate this class
    private Multiir() {
    }

    static {
        String lib_name = "libmultiir_jni.so";
        String lib_path[] = {"/system/lib/", "/system/lib64/", "/vendor/lib/", "/vendor/lib64/"};
        for (int i = 0; i < lib_path.length; i++) {
            File file = new File(lib_path[i], lib_name);
            if (file.exists()) {
                System.loadLibrary("multiir_jni");
                int ret = _nativeInit();
                if (ret == 0) ServiceReady = true;
                break;
            }
        }
    }

    public static int enterMouseMode() {
        if (ServiceReady) {
            _enterMouseMode();
            return 1;
        }
        return 0;
    }

    public static int exitMouseMode() {
        if (ServiceReady) {
            _exitMouseMode();
            return 1;
        }
        return 0;
    }

    public static int reportMouseKeyEvent(int scanCode, int keyState) {
        if (ServiceReady) {
            _reportMouseKeyEvent(scanCode, keyState);
            return 1;
        }
        return 0;
    }

    public static void registerObserver(Context context) {
        if (!ServiceReady || mObserver != null)
            return;
        mObserver = new MouseModeSettingsObserver(new Handler());
        mObserver.observe(context);
        IntentFilter filter = new IntentFilter("com.softwinner.tv.MOUSEMODE_RESET");
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if ("com.softwinner.tv.MOUSEMODE_RESET".equals(intent.getAction())) {
                    ContentResolver resolver = context.getContentResolver();
                    int pointer_speed = _getDefaultPointerSpeed();
                    int step_distance = _getDefaultStepDistance();
                    Settings.System.putInt(resolver, Settings.System.TV_MOUSEMODE_POINTERSPEED, pointer_speed);
                    Settings.System.putInt(resolver, Settings.System.TV_MOUSEMODE_STEPDISTANCE, step_distance);
                    context.sendBroadcast(new Intent("com.softwinner.tv.MOUSEMODE_UPDATE"));
                }
            }
        };
        context.registerReceiver(receiver, filter);
    }

    private static native int _nativeInit();
    private static native int _enterMouseMode();
    private static native int _exitMouseMode();
    private static native int _getDefaultPointerSpeed();
    private static native int _getDefaultStepDistance();
    private static native int _reset();
    private static native int _setPointerSpeed(int ms);
    private static native int _setStepDistance(int px);
    private static native int _reportMouseKeyEvent(int scanCode, int keyState);
}
