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

package com.android.car.hal;

import static android.car.VehiclePropertyIds.CLUSTER_DISPLAY_STATE;
import static android.car.VehiclePropertyIds.CLUSTER_NAVIGATION_STATE;
import static android.car.VehiclePropertyIds.CLUSTER_REPORT_STATE;
import static android.car.VehiclePropertyIds.CLUSTER_REQUEST_DISPLAY;
import static android.car.VehiclePropertyIds.CLUSTER_SWITCH_UI;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.NonNull;
import android.graphics.Insets;
import android.graphics.Rect;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.os.ServiceSpecificException;
import android.os.SystemClock;
import android.util.IntArray;

import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.GuardedBy;
import com.android.server.utils.Slogf;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;

/**
 * Translates HAL input events to higher-level semantic information.
 */
public final class ClusterHalService extends HalServiceBase {
    private static final String TAG = ClusterHalService.class.getSimpleName();
    private static final boolean DBG = false;
    public static final int DISPLAY_OFF = 0;
    public static final int DISPLAY_ON = 1;
    public static final int DONT_CARE = -1;

    /**
     * Interface to receive incoming Cluster HAL events.
     */
    public interface ClusterHalEventCallback {
        /**
         * Called when CLUSTER_SWITCH_UI message is received.
         *
         * @param uiType uiType ClusterOS wants to switch to
         */
        void onSwitchUi(int uiType);

        /**
         * Called when CLUSTER_DISPLAY_STATE message is received.
         *
         * @param onOff  0 - off, 1 - on
         * @param bounds the area to render the cluster Activity in pixel
         * @param insets Insets of the cluster display
         */
        void onDisplayState(int onOff, Rect bounds, Insets insets);
    }

    ;

    private static final int[] SUPPORTED_PROPERTIES = new int[]{
            CLUSTER_SWITCH_UI,
            CLUSTER_DISPLAY_STATE,
            CLUSTER_REPORT_STATE,
            CLUSTER_REQUEST_DISPLAY,
            CLUSTER_NAVIGATION_STATE,
    };

    private static final int[] CORE_PROPERTIES = new int[]{
            CLUSTER_SWITCH_UI,
            CLUSTER_REPORT_STATE,
            CLUSTER_DISPLAY_STATE,
            CLUSTER_REQUEST_DISPLAY,
    };

    private static final int[] SUBSCRIBABLE_PROPERTIES = new int[]{
            CLUSTER_SWITCH_UI,
            CLUSTER_DISPLAY_STATE,
    };

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private ClusterHalEventCallback mCallback;

    private final VehicleHal mHal;

    private volatile boolean mIsCoreSupported;
    private volatile boolean mIsNavigationStateSupported;

    public ClusterHalService(VehicleHal hal) {
        mHal = hal;
    }

    @Override
    public void init() {
        Slogf.d(TAG, "initClusterHalService");
        if (!isCoreSupported()) return;

        for (int property : SUBSCRIBABLE_PROPERTIES) {
            mHal.subscribeProperty(this, property);
        }
    }

    @Override
    public void release() {
        Slogf.d(TAG, "releaseClusterHalService");
        synchronized (mLock) {
            mCallback = null;
        }
    }

    /**
     * Sets the event callback to receive Cluster HAL events.
     */
    public void setCallback(ClusterHalEventCallback callback) {
        LinkedList<VehiclePropValue> eventsToDispatch = null;
        synchronized (mLock) {
            mCallback = callback;
        }
    }

    @NonNull
    @Override
    public int[] getAllSupportedProperties() {
        return SUPPORTED_PROPERTIES;
    }

    @Override
    public void takeProperties(@NonNull Collection<VehiclePropConfig> properties) {
        IntArray supportedProperties = new IntArray(properties.size());
        for (VehiclePropConfig property : properties) {
            supportedProperties.add(property.prop);
        }
        mIsCoreSupported = true;
        for (int coreProperty : CORE_PROPERTIES) {
            if (supportedProperties.indexOf(coreProperty) < 0) {
                mIsCoreSupported = false;
                break;
            }
        }
        mIsNavigationStateSupported = supportedProperties.indexOf(CLUSTER_NAVIGATION_STATE) >= 0;
        Slogf.d(TAG, "takeProperties: coreSupported=%s, navigationStateSupported=%s",
                mIsCoreSupported, mIsNavigationStateSupported);
    }

    public boolean isCoreSupported() {
        return mIsCoreSupported;
    }

    public boolean isNavigationStateSupported() {
        return mIsNavigationStateSupported;
    }

    @Override
    public void onHalEvents(List<VehiclePropValue> values) {
        Slogf.d(TAG, "handleHalEvents(): %s", values);
        ClusterHalEventCallback callback;
        synchronized (mLock) {
            callback = mCallback;
        }
        if (callback == null || !isCoreSupported()) return;

        for (VehiclePropValue value : values) {
            switch (value.prop) {
                case CLUSTER_SWITCH_UI:
                    int uiType = value.value.int32Values.get(0);
                    callback.onSwitchUi(uiType);
                    break;
                case CLUSTER_DISPLAY_STATE:
                    int onOff = value.value.int32Values.get(0);
                    Rect bounds = null;
                    if (hasNoDontCare(value.value.int32Values, 1, 4, "bounds")) {
                        bounds = new Rect(
                                value.value.int32Values.get(1), value.value.int32Values.get(2),
                                value.value.int32Values.get(3), value.value.int32Values.get(4));
                    }
                    Insets insets = null;
                    if (hasNoDontCare(value.value.int32Values, 5, 4, "insets")) {
                        insets = Insets.of(
                                value.value.int32Values.get(5), value.value.int32Values.get(6),
                                value.value.int32Values.get(7), value.value.int32Values.get(8));
                    }
                    callback.onDisplayState(onOff, bounds, insets);
                    break;
                default:
                    Slogf.w(TAG, "received unsupported event from HAL: %s", value);
            }
        }
    }

    private static boolean hasNoDontCare(ArrayList<Integer> values, int start, int length,
            String fieldName) {
        int count = 0;
        for (int i = start; i < start + length; ++i) {
            if (values.get(i) == DONT_CARE) ++count;
        }
        if (count == 0) return true;
        if (count != length) {
            Slogf.w(TAG, "Don't care should be set in the whole %s.", fieldName);
        }
        return false;
    }

    /**
     * Reports the current display state and ClusterUI state.
     *
     * @param onOff          0 - off, 1 - on
     * @param bounds         the area to render the cluster Activity in pixel
     * @param insets         Insets of the cluster display
     * @param uiTypeMain     uiType that ClusterHome tries to show in main area
     * @param uiTypeSub      uiType that ClusterHome tries to show in sub area
     * @param uiAvailability the byte array to represent the availability of ClusterUI.
     */
    public void reportState(int onOff, Rect bounds, Insets insets,
            int uiTypeMain, int uiTypeSub, byte[] uiAvailability) {
        if (!isCoreSupported()) return;
        VehiclePropValue request = createVehiclePropValue(CLUSTER_REPORT_STATE);
        request.value.int32Values.add(onOff);
        request.value.int32Values.add(bounds.left);
        request.value.int32Values.add(bounds.top);
        request.value.int32Values.add(bounds.right);
        request.value.int32Values.add(bounds.bottom);
        request.value.int32Values.add(insets.left);
        request.value.int32Values.add(insets.top);
        request.value.int32Values.add(insets.right);
        request.value.int32Values.add(insets.bottom);
        request.value.int32Values.add(uiTypeMain);
        request.value.int32Values.add(uiTypeSub);
        fillByteList(request.value.bytes, uiAvailability);
        send(request);
    }

    /**
     * Requests to turn the cluster display on to show some ClusterUI.
     *
     * @param uiType uiType that ClusterHome tries to show in main area
     */
    public void requestDisplay(int uiType) {
        if (!isCoreSupported()) return;
        VehiclePropValue request = createVehiclePropValue(CLUSTER_REQUEST_DISPLAY);
        request.value.int32Values.add(uiType);
        send(request);
    }


    /**
     * Informs the current navigation state.
     *
     * @param navigateState the serialized message of {@code NavigationStateProto}
     */
    public void sendNavigationState(byte[] navigateState) {
        if (!isNavigationStateSupported()) return;
        VehiclePropValue request = createVehiclePropValue(CLUSTER_NAVIGATION_STATE);
        fillByteList(request.value.bytes, navigateState);
        send(request);
    }

    private void send(VehiclePropValue request) {
        try {
            mHal.set(request);
        } catch (ServiceSpecificException e) {
            Slogf.e(TAG, "Failed to send request: " + request, e);
        }
    }

    private static void fillByteList(ArrayList<Byte> byteList, byte[] bytesArray) {
        byteList.ensureCapacity(bytesArray.length);
        for (byte b : bytesArray) {
            byteList.add(b);
        }
    }

    private static VehiclePropValue createVehiclePropValue(int property) {
        VehiclePropValue value = new VehiclePropValue();
        value.prop = property;
        value.timestamp = SystemClock.elapsedRealtime();
        return value;
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(PrintWriter writer) {
        writer.println("*Cluster HAL*");
        writer.println("mIsCoreSupported:" + isCoreSupported());
        writer.println("mIsNavigationStateSupported:" + isNavigationStateSupported());
    }
}
