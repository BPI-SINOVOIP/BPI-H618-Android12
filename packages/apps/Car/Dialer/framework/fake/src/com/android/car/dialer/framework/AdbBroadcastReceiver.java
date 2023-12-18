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

package com.android.car.dialer.framework;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import javax.inject.Inject;
import javax.inject.Singleton;

import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * Broadcast receiver for receiving Dialer debug commands from ADB.
 */
@Singleton
public class AdbBroadcastReceiver extends BroadcastReceiver {

    private static final String TAG = "CD.ADBHandler";
    private static final String INTENT_ACTION = "com.android.car.dialer.intent.action.adb";
    private static final String ACTION_TAG = "action";
    private static final String ACTION_CONNECT = "connect";
    private static final String ACTION_DISCONNECT = "disconnect";
    private static final String ACTION_ADDCALL = "addCall";
    private static final String ACTION_RECEIVECALL = "rcvCall";
    private static final String ACTION_ENDCALL = "endCall";
    private static final String ACTION_CLEARALL = "clearAll";
    private static final String ACTION_MERGE = "merge";
    private static final String EXTRA_CALL_ID = "id";
    private static final String EXTRA_DEVICE_ID = "device_id";

    private final FakeTelecomManager mFakeTelecomManager;
    private final FakeHfpManager mFakeHfpManager;

    @Inject
    AdbBroadcastReceiver(FakeTelecomManager fakeTelecomManager, FakeHfpManager fakeHfpManager) {
        mFakeTelecomManager = fakeTelecomManager;
        mFakeHfpManager = fakeHfpManager;
    }

    /**
     * Registers this class to an application context
     */
    public void registerReceiver(@ApplicationContext Context context) {
        Log.d(TAG, "Registered to " + context);
        IntentFilter filter = new IntentFilter();
        filter.addAction(INTENT_ACTION);
        context.registerReceiver(this, filter);
    }

    /**
     * Unregisters this class from the application context
     */
    public void unregisterReceiver(@ApplicationContext Context context) {
        Log.d(TAG, "Unregistered from " + context);
        context.unregisterReceiver(this);
    }

    /**
     * Handles received broadcast intent
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getStringExtra(ACTION_TAG);
        String id;

        switch (action) {
            case ACTION_ADDCALL:
                id = intent.getStringExtra(EXTRA_CALL_ID);
                Log.d(TAG, action + id);
                mFakeTelecomManager.placeCall(id);
                break;
            case ACTION_ENDCALL:
                id = intent.getStringExtra(EXTRA_CALL_ID);
                Log.d(TAG, action + id);
                mFakeTelecomManager.endCall(id);
                break;
            case ACTION_RECEIVECALL:
                id = intent.getStringExtra(EXTRA_CALL_ID);
                Log.d(TAG, action + id);
                mFakeTelecomManager.receiveCall(id);
                break;
            case ACTION_CLEARALL:
                Log.d(TAG, action);
                mFakeTelecomManager.clearCalls();
                break;
            case ACTION_MERGE:
                Log.d(TAG, action);
                mFakeTelecomManager.mergeCalls();
                break;
            case ACTION_CONNECT:
                Log.d(TAG, action);
                mFakeHfpManager.connectHfpDevice();
                break;
            case ACTION_DISCONNECT:
                id = intent.getStringExtra(EXTRA_DEVICE_ID);
                Log.d(TAG, action + id);
                mFakeHfpManager.disconnectHfpDevice(id);
                break;
            default:
                Log.d(TAG, "Unknown command " + action);
        }
    }
}
