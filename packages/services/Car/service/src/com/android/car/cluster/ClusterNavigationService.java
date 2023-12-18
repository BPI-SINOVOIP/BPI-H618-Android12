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
package com.android.car.cluster;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.car.Car;
import android.car.CarAppFocusManager;
import android.car.cluster.renderer.IInstrumentClusterNavigation;
import android.car.navigation.CarNavigationInstrumentCluster;
import android.content.Context;
import android.os.Binder;
import android.os.Bundle;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;

import com.android.car.AppFocusService;
import com.android.car.AppFocusService.FocusOwnershipCallback;
import com.android.car.CarLocalServices;
import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.ICarImpl;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.GuardedBy;

import java.util.Objects;

/**
 * Service responsible for Navigation focus management and {@code NavigationState} change.
 *
 * @hide
 */
public class ClusterNavigationService extends IInstrumentClusterNavigation.Stub
        implements CarServiceBase, FocusOwnershipCallback {
    private static final String TAG = CarLog.TAG_CLUSTER;
    private static final ContextOwner NO_OWNER = new ContextOwner(0, 0);

    private final Context mContext;
    private final AppFocusService mAppFocusService;

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private ContextOwner mNavContextOwner = NO_OWNER;

    interface ClusterNavigationServiceCallback {
        void onNavigationStateChanged(Bundle bundle);
        CarNavigationInstrumentCluster getInstrumentClusterInfo();
        void notifyNavContextOwnerChanged(ContextOwner owner);
    }

    @GuardedBy("mLock")
    ClusterNavigationServiceCallback mClusterServiceCallback;

    @Override
    public void onNavigationStateChanged(Bundle bundle) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_NAVIGATION_MANAGER);
        assertNavigationFocus();
        ClusterNavigationServiceCallback callback;
        synchronized (mLock) {
            callback = mClusterServiceCallback;
        }
        if (callback == null) return;
        callback.onNavigationStateChanged(bundle);
    }

    @Override
    public CarNavigationInstrumentCluster getInstrumentClusterInfo() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CAR_NAVIGATION_MANAGER);
        ClusterNavigationServiceCallback callback;
        synchronized (mLock) {
            callback = mClusterServiceCallback;
        }
        if (callback == null) return null;
        return callback.getInstrumentClusterInfo();
    }

    public ClusterNavigationService(Context context, AppFocusService appFocusService) {
        mContext = context;
        mAppFocusService = appFocusService;
    }

    public void setClusterServiceCallback(
            ClusterNavigationServiceCallback clusterServiceCallback) {
        synchronized (mLock) {
            mClusterServiceCallback = clusterServiceCallback;
        }
    }

    @Override
    public void init() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "initClusterNavigationService");
        }
        mAppFocusService.registerContextOwnerChangedCallback(this /* FocusOwnershipCallback */);
    }

    @Override
    public void release() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "releaseClusterNavigationService");
        }
        setClusterServiceCallback(null);
        mAppFocusService.unregisterContextOwnerChangedCallback(this);
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        writer.println("**" + getClass().getSimpleName() + "**");
        synchronized (mLock) {
            writer.println("context owner: " + mNavContextOwner);
        }
    }

    @Override
    public void onFocusAcquired(int appType, int uid, int pid) {
        changeNavContextOwner(appType, uid, pid, true);
    }

    @Override
    public void onFocusAbandoned(int appType, int uid, int pid) {
        changeNavContextOwner(appType, uid, pid, false);
    }

    private void assertNavigationFocus() {
        int uid = Binder.getCallingUid();
        int pid = Binder.getCallingPid();
        synchronized (mLock) {
            if (uid == mNavContextOwner.uid && pid == mNavContextOwner.pid) {
                return;
            }
        }
        // Stored one failed. There can be a delay, so check with real one again.
        AppFocusService afs = CarLocalServices.getService(AppFocusService.class);
        if (afs != null && afs.isFocusOwner(uid, pid,
                CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION)) {
            return;
        }
        throw new IllegalStateException("Client not owning APP_FOCUS_TYPE_NAVIGATION");
    }

    private void changeNavContextOwner(int appType, int uid, int pid, boolean acquire) {
        if (appType != CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION) {
            return;
        }
        ContextOwner requester = new ContextOwner(uid, pid);
        ContextOwner newOwner = acquire ? requester : NO_OWNER;
        ClusterNavigationServiceCallback callback;
        synchronized (mLock) {
            if ((acquire && Objects.equals(mNavContextOwner, requester))
                    || (!acquire && !Objects.equals(mNavContextOwner, requester))) {
                // Nothing to do here. Either the same owner is acquiring twice, or someone is
                // abandoning a focus they didn't have.
                Slog.w(TAG, "Invalid nav context owner change (acquiring: " + acquire
                        + "), current owner: [" + mNavContextOwner
                        + "], requester: [" + requester + "]");
                return;
            }

            mNavContextOwner = newOwner;
            callback = mClusterServiceCallback;
        }
        if (callback == null) return;

        callback.notifyNavContextOwnerChanged(newOwner);
    }

    static class ContextOwner {
        final int uid;
        final int pid;

        ContextOwner(int uid, int pid) {
            this.uid = uid;
            this.pid = pid;
        }

        @Override
        public String toString() {
            return "uid: " + uid + ", pid: " + pid;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            ContextOwner that = (ContextOwner) o;
            return uid == that.uid && pid == that.pid;
        }

        @Override
        public int hashCode() {
            return Objects.hash(uid, pid);
        }
    }
}
