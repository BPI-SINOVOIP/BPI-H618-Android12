/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.car;

import android.annotation.Nullable;
import android.app.Service;
import android.car.ICarBluetoothUserService;
import android.car.ILocationManagerProxy;
import android.car.IPerUserCarService;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.IBinder;
import android.util.IndentingPrintWriter;

import com.android.car.admin.PerUserCarDevicePolicyService;
import com.android.server.utils.Slogf;

import java.io.FileDescriptor;
import java.io.PrintWriter;

/**
 * {@link CarService} process runs as the System User. When logged in as a different user, some
 * services do not provide an API to register or bind as a User, hence CarService doesn't receive
 * the events from services/processes running as a non-system user.
 *
 * This Service is run as the Current User on every User Switch and components of CarService can
 * use this service to communicate with services/processes running as the current (non-system) user.
 */
public class PerUserCarService extends Service {
    private static final boolean DBG = true;
    private static final String TAG = CarLog.tagFor(PerUserCarService.class);

    private CarBluetoothUserService mCarBluetoothUserService;
    private LocationManagerProxy mLocationManagerProxy;
    private @Nullable PerUserCarDevicePolicyService mPerUserCarDevicePolicyService;
    private PerUserCarServiceBinder mPerUserCarServiceBinder;

    @Override
    public IBinder onBind(Intent intent) {
        if (DBG) Slogf.d(TAG, "onBind()");

        if (mPerUserCarServiceBinder == null) {
            Slogf.e(TAG, "PerUserCarServiceBinder null");
        }
        return mPerUserCarServiceBinder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DBG) Slogf.d(TAG, "onStart()");

        return START_STICKY;
    }

    @Override
    public void onCreate() {
        Context context = getApplicationContext();
        Slogf.i(TAG, "created for user %d", context.getUserId());

        mPerUserCarServiceBinder = new PerUserCarServiceBinder();
        mCarBluetoothUserService = new CarBluetoothUserService(this);

        if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_DEVICE_ADMIN)) {
            mPerUserCarDevicePolicyService = PerUserCarDevicePolicyService.getInstance(context);
            mPerUserCarDevicePolicyService.onCreate();
        } else if (DBG) {
            Slogf.d(TAG, "Not setting PerUserCarDevicePolicyService because device doesn't have %s",
                    PackageManager.FEATURE_DEVICE_ADMIN);
        }

        mLocationManagerProxy = new LocationManagerProxy(this);
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        Slogf.i(TAG, "destroyed for user %d", getApplicationContext().getUserId());

        if (mPerUserCarDevicePolicyService != null) {
            mPerUserCarDevicePolicyService.onDestroy();
        }
        mPerUserCarServiceBinder = null;
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        try (IndentingPrintWriter pw = new IndentingPrintWriter(writer)) {
            pw.println("CarBluetoothUserService");
            pw.increaseIndent();
            mCarBluetoothUserService.dump(pw);
            pw.decreaseIndent();
            pw.println();

            if (mPerUserCarDevicePolicyService != null) {
                pw.println("PerUserCarDevicePolicyService");
                pw.increaseIndent();
                mPerUserCarDevicePolicyService.dump(pw);
                pw.decreaseIndent();
            } else {
                pw.println("PerUserCarDevicePolicyService not needed");
            }
            pw.println();

            pw.println("LocationManagerProxy");
            pw.increaseIndent();
            mLocationManagerProxy.dump(pw);
            pw.decreaseIndent();
            pw.println();
        }
    }

    /**
     * Other Services in CarService can create their own Binder interface and receive that interface
     * through this PerUserCarService binder.
     */
    private final class PerUserCarServiceBinder extends IPerUserCarService.Stub {
        @Override
        public ICarBluetoothUserService getBluetoothUserService() {
            return mCarBluetoothUserService;
        }

        @Override
        public ILocationManagerProxy getLocationManagerProxy() {
            return mLocationManagerProxy;
        }
    }
}
