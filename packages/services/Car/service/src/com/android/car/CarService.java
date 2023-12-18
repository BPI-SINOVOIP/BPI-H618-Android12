/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static android.os.SystemClock.elapsedRealtime;

import android.annotation.Nullable;
import android.app.Service;
import android.content.Intent;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.os.Build;
import android.os.IBinder;
import android.os.IHwBinder.DeathRecipient;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.Trace;
import android.util.EventLog;
import android.util.Slog;

import com.android.car.internal.common.EventLogTags;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.util.LimitedTimingsTraceLog;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.RingBufferIndices;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.NoSuchElementException;

public class CarService extends Service {
    public static final String CAR_SERVICE_INIT_TIMING_TAG = "CAR.InitTiming";
    public static final int CAR_SERVICE_INIT_TIMING_MIN_DURATION_MS = 5;

    private static final boolean RESTART_CAR_SERVICE_WHEN_VHAL_CRASH = true;

    private static final long WAIT_FOR_VEHICLE_HAL_TIMEOUT_MS = 10_000;

    private static final boolean IS_USER_BUILD = "user".equals(Build.TYPE);

    private ICarImpl mICarImpl;
    private IVehicle mVehicle;

    private String mVehicleInterfaceName;

    private final VehicleDeathRecipient mVehicleDeathRecipient = new VehicleDeathRecipient();

    @Override
    public void onCreate() {
        LimitedTimingsTraceLog initTiming = new LimitedTimingsTraceLog(CAR_SERVICE_INIT_TIMING_TAG,
                Trace.TRACE_TAG_SYSTEM_SERVER, CAR_SERVICE_INIT_TIMING_MIN_DURATION_MS);
        initTiming.traceBegin("CarService.onCreate");

        initTiming.traceBegin("getVehicle");
        mVehicle = getVehicle();
        initTiming.traceEnd();

        EventLog.writeEvent(EventLogTags.CAR_SERVICE_CREATE, mVehicle == null ? 0 : 1);

        if (mVehicle == null) {
            throw new IllegalStateException("Vehicle HAL service is not available.");
        }
        try {
            mVehicleInterfaceName = mVehicle.interfaceDescriptor();
        } catch (RemoteException e) {
            throw new IllegalStateException("Unable to get Vehicle HAL interface descriptor", e);
        }

        Slog.i(CarLog.TAG_SERVICE, "Connected to " + mVehicleInterfaceName);
        EventLog.writeEvent(EventLogTags.CAR_SERVICE_CONNECTED, mVehicleInterfaceName);

        mICarImpl = new ICarImpl(this,
                mVehicle,
                SystemInterface.Builder.defaultSystemInterface(this).build(),
                mVehicleInterfaceName);
        mICarImpl.init();

        linkToDeath(mVehicle, mVehicleDeathRecipient);

        ServiceManager.addService("car_service", mICarImpl);
        SystemProperties.set("boot.car_service_created", "1");

        super.onCreate();

        initTiming.traceEnd(); // "CarService.onCreate"
    }

    // onDestroy is best-effort and might not get called on shutdown/reboot. As such it is not
    // suitable for permanently saving state or other need-to-happen operation. If you have a
    // cleanup task that you want to make sure happens on shutdown/reboot, see OnShutdownReboot.
    @Override
    public void onDestroy() {
        EventLog.writeEvent(EventLogTags.CAR_SERVICE_CREATE, mVehicle == null ? 0 : 1);
        Slog.i(CarLog.TAG_SERVICE, "Service onDestroy");
        mICarImpl.release();

        if (mVehicle != null) {
            try {
                mVehicle.unlinkToDeath(mVehicleDeathRecipient);
                mVehicle = null;
            } catch (RemoteException e) {
                // Ignore errors on shutdown path.
            }
        }

        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // keep it alive.
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mICarImpl;
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        // historically, the way to get a dumpsys from CarService has been to use
        // "dumpsys activity service com.android.car/.CarService" - leaving this
        // as a forward to car_service makes the previously well-known command still work
        mICarImpl.dump(fd, writer, args);
    }

    @Nullable
    private IVehicle getVehicleWithTimeout(long waitMilliseconds) {
        IVehicle vehicle = getVehicle();
        long start = elapsedRealtime();
        while (vehicle == null && (start + waitMilliseconds) > elapsedRealtime()) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                throw new RuntimeException("Sleep was interrupted", e);
            }

            vehicle = getVehicle();
        }

        return vehicle;
    }

    @Nullable
    private static IVehicle getVehicle() {
        final String instanceName = SystemProperties.get("ro.vehicle.hal", "default");

        try {
            return android.hardware.automotive.vehicle.V2_0.IVehicle.getService(instanceName);
        } catch (RemoteException e) {
            Slog.e(CarLog.TAG_SERVICE, "Failed to get IVehicle/" + instanceName + " service", e);
        } catch (NoSuchElementException e) {
            Slog.e(CarLog.TAG_SERVICE, "IVehicle/" + instanceName + " service not registered yet");
        }
        return null;
    }

    private class VehicleDeathRecipient implements DeathRecipient {

        @Override
        public void serviceDied(long cookie) {
            EventLog.writeEvent(EventLogTags.CAR_SERVICE_VHAL_DIED, cookie);
            Slog.wtf(CarLog.TAG_SERVICE, "***Vehicle HAL died. Car service will restart***");
            Process.killProcess(Process.myPid());
        }
    }

    private static void linkToDeath(IVehicle vehicle, DeathRecipient recipient) {
        try {
            vehicle.linkToDeath(recipient, 0);
        } catch (RemoteException e) {
            throw new IllegalStateException("Failed to linkToDeath Vehicle HAL");
        }
    }

    @VisibleForTesting
    static class CrashTracker {
        private final int mMaxCrashCountLimit;
        private final int mSlidingWindowMillis;

        private final long[] mCrashTimestamps;
        private final RingBufferIndices mCrashTimestampsIndices;
        private final Runnable mCallback;

        /**
         * If maxCrashCountLimit number of crashes occurred within slidingWindowMillis time frame
         * then call provided callback function.
         */
        CrashTracker(int maxCrashCountLimit, int slidingWindowMillis, Runnable callback) {
            mMaxCrashCountLimit = maxCrashCountLimit;
            mSlidingWindowMillis = slidingWindowMillis;
            mCallback = callback;

            mCrashTimestamps = new long[maxCrashCountLimit];
            mCrashTimestampsIndices = new RingBufferIndices(mMaxCrashCountLimit);
        }

        void crashDetected() {
            long lastCrash = SystemClock.elapsedRealtime();
            mCrashTimestamps[mCrashTimestampsIndices.add()] = lastCrash;

            if (mCrashTimestampsIndices.size() == mMaxCrashCountLimit) {
                long firstCrash = mCrashTimestamps[mCrashTimestampsIndices.indexOf(0)];

                if (lastCrash - firstCrash < mSlidingWindowMillis) {
                    mCallback.run();
                }
            }
        }
    }
}
