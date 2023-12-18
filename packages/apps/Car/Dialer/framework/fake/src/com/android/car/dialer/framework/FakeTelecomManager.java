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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.spy;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.telecom.Call;
import android.telecom.InCallService;
import android.telecom.TelecomManager;
import android.util.Log;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import javax.inject.Inject;
import javax.inject.Singleton;

import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * A fake TelecomManager implementation
 */
@Singleton
public class FakeTelecomManager {
    private static final String TAG = "CD.FakeTelecomManager";
    private static final String ACTION_BIND = "proxy_bind";

    private TelecomManager mSpiedTelecomManager;
    private InCallServiceProxy mInCallService;

    private final ServiceConnection mInCallServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            Log.d(TAG, "fake telecom manager connected");
            mInCallService = ((InCallServiceProxy.LocalBinder) binder).getService();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.d(TAG, "onServiceDisconnected");
            mInCallService = null;
        }
    };

    @Inject
    public FakeTelecomManager(@ApplicationContext Context context) {
        Log.d(TAG, "Create FakeTelecomManager");

        mSpiedTelecomManager = spy(context.getSystemService(TelecomManager.class));

        // Mock telecomManager.placeCall()
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock inv) {
                Log.d(TAG, "placeCall");

                Uri uri = inv.getArgument(0);
                String number = uri.getSchemeSpecificPart();
                placeCall(number);

                return null;
            }
        }).when(mSpiedTelecomManager).placeCall(any(Uri.class), any(Bundle.class));

        bindService(context);
    }

    /**
     * Places a call.
     */
    public void placeCall(String id) {
        if (mInCallService != null) {
            mInCallService.addCall(id, Call.Details.DIRECTION_OUTGOING, Call.STATE_ACTIVE);
        } else {
            Log.d(TAG, "null service");
        }
    }

    /**
     * Receive a call.
     */
    public void receiveCall(String id) {
        if (mInCallService != null) {
            mInCallService.addCall(id, Call.Details.DIRECTION_INCOMING, Call.STATE_RINGING);
        } else {
            Log.d(TAG, "null service");
        }
    }

    /**
     * Ends a call.
     */
    public void endCall(String id) {
        if (mInCallService != null) {
            mInCallService.endCall(id);
        } else {
            Log.d(TAG, "null service");
        }
    }

    /**
     * Removes all current calls.
     */
    public void clearCalls() {
        if (mInCallService != null) {
            mInCallService.clearCalls();
        } else {
            Log.d(TAG, "null service");
        }
    }

    /**
     * Merges the primary and secondary calls.
     */
    public void mergeCalls() {
        if (mInCallService != null) {
            mInCallService.mergeCalls();
        } else {
            Log.d(TAG, "null service");
        }
    }

    /**
     * Binds this class to the fake InCallService.
     */
    public void bindService(@ApplicationContext Context context) {
        Log.d(TAG, "binding to InCallService");

        String defaultPackage = mSpiedTelecomManager.getDefaultDialerPackage();

        Intent serviceIntent = new Intent();
        serviceIntent.setAction(InCallService.SERVICE_INTERFACE);
        serviceIntent.setPackage(defaultPackage);

        PackageManager pm = context.getPackageManager();
        ResolveInfo info = pm.resolveService(serviceIntent, 0);

        if (info == null) {
            Log.e(TAG, "Could not resolve InCallServiceImpl to bind");
        }

        Intent intent = new Intent();
        intent.setComponent(new ComponentName(info.serviceInfo.packageName, info.serviceInfo.name));
        intent.setAction(ACTION_BIND);
        context.bindService(intent, mInCallServiceConnection, Context.BIND_AUTO_CREATE);
    }

    /**
     * Gets a fake TelecomManager.
     */
    public TelecomManager getTelecomManager() {
        return mSpiedTelecomManager;
    }
}
