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

import static android.content.Intent.ACTION_MAIN;

import static com.android.car.hal.ClusterHalService.DISPLAY_OFF;
import static com.android.car.hal.ClusterHalService.DISPLAY_ON;
import static com.android.car.hal.ClusterHalService.DONT_CARE;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.app.ActivityOptions;
import android.car.Car;
import android.car.CarOccupantZoneManager;
import android.car.ICarOccupantZoneCallback;
import android.car.cluster.ClusterHomeManager;
import android.car.cluster.ClusterState;
import android.car.cluster.IClusterHomeCallback;
import android.car.cluster.IClusterHomeService;
import android.car.cluster.navigation.NavigationState.NavigationStateProto;
import android.car.navigation.CarNavigationInstrumentCluster;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Insets;
import android.graphics.Point;
import android.graphics.Rect;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.UserHandle;
import android.text.TextUtils;
import android.util.IndentingPrintWriter;
import android.view.Display;

import com.android.car.CarLog;
import com.android.car.CarOccupantZoneService;
import com.android.car.CarServiceBase;
import com.android.car.R;
import com.android.car.am.FixedActivityService;
import com.android.car.hal.ClusterHalService;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.server.utils.Slogf;

/**
 * Service responsible for interactions between ClusterOS and ClusterHome.
 */
public class ClusterHomeService extends IClusterHomeService.Stub
        implements CarServiceBase, ClusterNavigationService.ClusterNavigationServiceCallback,
        ClusterHalService.ClusterHalEventCallback {
    private static final String TAG = CarLog.TAG_CLUSTER;
    private static final int DEFAULT_MIN_UPDATE_INTERVAL_MILLIS = 1000;
    private static final String NAV_STATE_PROTO_BUNDLE_KEY = "navstate2";

    private final Context mContext;
    private final ClusterHalService mClusterHalService;
    private final ClusterNavigationService mClusterNavigationService;
    private final CarOccupantZoneService mOccupantZoneService;
    private final FixedActivityService mFixedActivityService;
    private final ComponentName mClusterHomeActivity;

    private boolean mServiceEnabled;

    private int mClusterDisplayId = Display.INVALID_DISPLAY;

    private int mOnOff = DISPLAY_OFF;
    private Rect mBounds = new Rect();
    private Insets mInsets = Insets.NONE;
    private int mUiType = ClusterHomeManager.UI_TYPE_CLUSTER_HOME;
    private Intent mLastIntent;

    private final RemoteCallbackList<IClusterHomeCallback> mClientCallbacks =
            new RemoteCallbackList<>();

    public ClusterHomeService(Context context, ClusterHalService clusterHalService,
            ClusterNavigationService navigationService,
            CarOccupantZoneService occupantZoneService,
            FixedActivityService fixedActivityService) {
        mContext = context;
        mClusterHalService = clusterHalService;
        mClusterNavigationService = navigationService;
        mOccupantZoneService = occupantZoneService;
        mFixedActivityService = fixedActivityService;
        mClusterHomeActivity = ComponentName.unflattenFromString(
                mContext.getString(R.string.config_clusterHomeActivity));
        mLastIntent = new Intent(ACTION_MAIN).setComponent(mClusterHomeActivity);
    }

    @Override
    public void init() {
        Slogf.d(TAG, "initClusterHomeService");
        if (TextUtils.isEmpty(mClusterHomeActivity.getPackageName())
                || TextUtils.isEmpty(mClusterHomeActivity.getClassName())) {
            Slogf.i(TAG, "Improper ClusterHomeActivity: %s", mClusterHomeActivity);
            return;
        }
        if (!mClusterHalService.isCoreSupported()) {
            Slogf.e(TAG, "No Cluster HAL properties");
            return;
        }

        mServiceEnabled = true;
        mClusterHalService.setCallback(this);
        mClusterNavigationService.setClusterServiceCallback(this);

        mOccupantZoneService.registerCallback(mOccupantZoneCallback);
        initClusterDisplay();
    }

    private void initClusterDisplay() {
        int clusterDisplayId = mOccupantZoneService.getDisplayIdForDriver(
                CarOccupantZoneManager.DISPLAY_TYPE_INSTRUMENT_CLUSTER);
        if (clusterDisplayId == Display.INVALID_DISPLAY) {
            Slogf.i(TAG, "No cluster display is defined");
        }
        if (clusterDisplayId == mClusterDisplayId) {
            return;  // Skip if the cluster display isn't changed.
        }
        mClusterDisplayId = clusterDisplayId;
        sendDisplayState(ClusterHomeManager.CONFIG_DISPLAY_ID);
        if (clusterDisplayId == Display.INVALID_DISPLAY) {
            return;
        }

        // Initialize mBounds only once.
        if (mBounds.right == 0 && mBounds.bottom == 0 && mBounds.left == 0 && mBounds.top == 0) {
            DisplayManager displayManager = mContext.getSystemService(DisplayManager.class);
            Display clusterDisplay = displayManager.getDisplay(clusterDisplayId);
            Point size = new Point();
            clusterDisplay.getRealSize(size);
            mBounds.right = size.x;
            mBounds.bottom = size.y;
            Slogf.d(TAG, "Found cluster displayId=%d, bounds=%s", clusterDisplayId, mBounds);
        }

        ActivityOptions activityOptions = ActivityOptions.makeBasic()
                .setLaunchDisplayId(clusterDisplayId);
        mFixedActivityService.startFixedActivityModeForDisplayAndUser(
                mLastIntent, activityOptions, clusterDisplayId, UserHandle.USER_SYSTEM);
    }

    private final ICarOccupantZoneCallback mOccupantZoneCallback =
            new ICarOccupantZoneCallback.Stub() {
                @Override
                public void onOccupantZoneConfigChanged(int flags) throws RemoteException {
                    if ((flags & CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_DISPLAY) != 0) {
                        initClusterDisplay();
                    }
                }
            };

    @Override
    public void release() {
        Slogf.d(TAG, "releaseClusterHomeService");
        mOccupantZoneService.unregisterCallback(mOccupantZoneCallback);
        mClusterHalService.setCallback(null);
        mClusterNavigationService.setClusterServiceCallback(null);
        mClientCallbacks.kill();
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        // TODO: record the latest states from both sides
    }

    // ClusterHalEventListener starts
    @Override
    public void onSwitchUi(int uiType) {
        Slogf.d(TAG, "onSwitchUi: uiType=%d", uiType);
        int changes = 0;
        if (mUiType != uiType) {
            mUiType = uiType;
            changes |= ClusterHomeManager.CONFIG_UI_TYPE;
        }
        sendDisplayState(changes);
    }

    @Override
    public void onDisplayState(int onOff, Rect bounds, Insets insets) {
        Slogf.d(TAG, "onDisplayState: onOff=%d, bounds=%s, insets=%s", onOff, bounds, insets);
        int changes = 0;
        if (onOff != DONT_CARE && mOnOff != onOff) {
            mOnOff = onOff;
            changes |= ClusterHomeManager.CONFIG_DISPLAY_ON_OFF;
        }
        if (bounds != null && !mBounds.equals(bounds)) {
            mBounds = bounds;
            changes |= ClusterHomeManager.CONFIG_DISPLAY_BOUNDS;
        }
        if (insets != null && !mInsets.equals(insets)) {
            mInsets = insets;
            changes |= ClusterHomeManager.CONFIG_DISPLAY_INSETS;
        }
        sendDisplayState(changes);
    }
    // ClusterHalEventListener ends

    private void sendDisplayState(int changes) {
        ClusterState state = createClusterState();
        int n = mClientCallbacks.beginBroadcast();
        for (int i = 0; i < n; i++) {
            IClusterHomeCallback callback = mClientCallbacks.getBroadcastItem(i);
            try {
                callback.onClusterStateChanged(state, changes);
            } catch (RemoteException ignores) {
                // ignore
            }
        }
        mClientCallbacks.finishBroadcast();
    }

    // ClusterNavigationServiceCallback starts
    @Override
    public void onNavigationStateChanged(Bundle bundle) {
        byte[] protoBytes = bundle.getByteArray(NAV_STATE_PROTO_BUNDLE_KEY);

        sendNavigationState(protoBytes);
    }

    private void sendNavigationState(byte[] protoBytes) {
        final int n = mClientCallbacks.beginBroadcast();
        for (int i = 0; i < n; i++) {
            IClusterHomeCallback callback = mClientCallbacks.getBroadcastItem(i);
            try {
                callback.onNavigationStateChanged(protoBytes);
            } catch (RemoteException ignores) {
                // ignore
            }
        }
        mClientCallbacks.finishBroadcast();

        if (!mClusterHalService.isNavigationStateSupported()) {
            Slogf.d(TAG, "No Cluster NavigationState HAL property");
            return;
        }
        mClusterHalService.sendNavigationState(protoBytes);
    }

    @Override
    public CarNavigationInstrumentCluster getInstrumentClusterInfo() {
        return CarNavigationInstrumentCluster.createCluster(DEFAULT_MIN_UPDATE_INTERVAL_MILLIS);
    }

    @Override
    public void notifyNavContextOwnerChanged(ClusterNavigationService.ContextOwner owner) {
        Slogf.d(TAG, "notifyNavContextOwnerChanged: owner=%s", owner);
        // Sends the empty NavigationStateProto to clear out the last direction
        // when the app context owner is changed or the navigation is finished.
        NavigationStateProto emptyProto = NavigationStateProto.newBuilder()
                .setServiceStatus(NavigationStateProto.ServiceStatus.NORMAL).build();
        sendNavigationState(emptyProto.toByteArray());
    }
    // ClusterNavigationServiceCallback ends

    // IClusterHomeService starts
    @Override
    public void reportState(int uiTypeMain, int uiTypeSub, byte[] uiAvailability) {
        Slogf.d(TAG, "reportState: main=%d, sub=%d", uiTypeMain, uiTypeSub);
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");

        mClusterHalService.reportState(mOnOff, mBounds, mInsets,
                uiTypeMain, uiTypeSub, uiAvailability);
    }

    @Override
    public void requestDisplay(int uiType) {
        Slogf.d(TAG, "requestDisplay: uiType=%d", uiType);
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");

        mClusterHalService.requestDisplay(uiType);
    }

    @Override
    public boolean startFixedActivityModeAsUser(Intent intent,
            Bundle activityOptionsBundle, int userId) {
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");
        if (mClusterDisplayId == Display.INVALID_DISPLAY) {
            Slogf.e(TAG, "Cluster display is not ready.");
            return false;
        }

        ActivityOptions activityOptions = ActivityOptions.fromBundle(activityOptionsBundle);
        activityOptions.setLaunchDisplayId(mClusterDisplayId);
        mLastIntent = intent;
        return mFixedActivityService.startFixedActivityModeForDisplayAndUser(
                intent, activityOptions, mClusterDisplayId, userId);
    }

    @Override
    public void stopFixedActivityMode() {
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");
        if (mClusterDisplayId == Display.INVALID_DISPLAY) {
            Slogf.e(TAG, "Cluster display is not ready.");
            return;
        }

        mFixedActivityService.stopFixedActivityMode(mClusterDisplayId);
    }

    @Override
    public void registerCallback(IClusterHomeCallback callback) {
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");

        mClientCallbacks.register(callback);
    }

    @Override
    public void unregisterCallback(IClusterHomeCallback callback) {
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");

        mClientCallbacks.unregister(callback);
    }

    @Override
    public ClusterState getClusterState() {
        Slogf.d(TAG, "getClusterState");
        enforcePermission(Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
        if (!mServiceEnabled) throw new IllegalStateException("Service is not enabled");
        return createClusterState();
    }
    // IClusterHomeService ends

    private void enforcePermission(String permissionName) {
        if (mContext.checkCallingOrSelfPermission(permissionName)
                != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException("requires permission " + permissionName);
        }
    }

    private ClusterState createClusterState() {
        ClusterState state = new ClusterState();
        state.on = mOnOff == DISPLAY_ON;
        state.bounds = mBounds;
        state.insets = mInsets;
        state.uiType = mUiType;
        state.displayId = mClusterDisplayId;
        return state;
    }
}
