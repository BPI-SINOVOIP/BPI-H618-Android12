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

import static android.car.evs.CarEvsManager.SERVICE_TYPE_REARVIEW;
import static android.car.evs.CarEvsManager.SERVICE_TYPE_SURROUNDVIEW;
import static android.hardware.automotive.vehicle.V2_0.VehicleProperty.EVS_SERVICE_REQUEST;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.car.evs.CarEvsManager.CarEvsServiceType;
import android.hardware.automotive.vehicle.V2_0.EvsServiceRequestIndex;
import android.hardware.automotive.vehicle.V2_0.EvsServiceState;
import android.hardware.automotive.vehicle.V2_0.EvsServiceType;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.util.Log;
import android.util.Slog;
import android.util.SparseArray;

import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.GuardedBy;

import java.io.PrintWriter;
import java.util.Collection;
import java.util.List;

/*
 * Translates HAL events, CarEvsService is interested in, into the higher-level semantic
 * information.
 */
public class EvsHalService extends HalServiceBase {

    private static final String TAG = EvsHalService.class.getSimpleName();
    private static final boolean DBG = Log.isLoggable(TAG, Log.DEBUG);

    private static final int[] SUPPORTED_PROPERTIES = new int[] {
        EVS_SERVICE_REQUEST,
    };

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private final SparseArray<VehiclePropConfig> mProperties = new SparseArray();

    private final VehicleHal mHal;

    @GuardedBy("mLock")
    private EvsHalEventListener mListener;

    private @CarEvsServiceType int mLastServiceTypeRequested;

    private boolean mIsEvsServiceRequestSupported;

    public EvsHalService(VehicleHal hal) {
        mHal = hal;
    }

    /**
     * Interface to be implemented by any client that wants to get notified upon a new EVS service
     * request.
     */
    public interface EvsHalEventListener {
        /** Called when a value of monitoring VHAL property gets changed */
        void onEvent(@CarEvsServiceType int id, boolean on);
    }

    /**
     * Sets the event listener to receive Vehicle's EVS-related events.
     *
     * @param listener {@link EvsHalEventListener}
     * @throws {@link IllegalStateException} if none of required VHAL properties are not supported
     *         on this device.
     */
    public void setListener(EvsHalEventListener listener) {
        if (!mIsEvsServiceRequestSupported) {
            throw new IllegalStateException(
                    "Any of required VHAL properties are not supported.");
        }

        synchronized (mLock) {
            mListener = listener;
        }

        mHal.subscribeProperty(this, EVS_SERVICE_REQUEST);
    }

    /** Returns whether {@code EVS_SERVICE_REQUEST} is supported */
    public boolean isEvsServiceRequestSupported() {
        return mIsEvsServiceRequestSupported;
    }

    @Override
    public void init() {
        synchronized (mLock) {
            for (int i = 0; i < mProperties.size(); i++) {
                VehiclePropConfig config = mProperties.valueAt(i);
                if (VehicleHal.isPropertySubscribable(config)) {
                    mHal.subscribeProperty(this, config.prop);
                }
            }
        }
    }

    @Override
    public void release() {
        synchronized (mLock) {
            mListener = null;
            mProperties.clear();
        }
    }

    @Override
    public int[] getAllSupportedProperties() {
        return SUPPORTED_PROPERTIES;
    }

    @Override
    public void takeProperties(Collection<VehiclePropConfig> configs) {
        if (configs.isEmpty()) {
            return;
        }

        synchronized (mLock) {
            for (VehiclePropConfig config : configs) {
                mProperties.put(config.prop, config);
            }
        }

        mIsEvsServiceRequestSupported = mProperties.contains(EVS_SERVICE_REQUEST);
    }

    @Override
    public void onHalEvents(List<VehiclePropValue> values) {
        EvsHalEventListener listener;
        synchronized (mLock) {
            listener = mListener;
        }

        if (listener == null) {
            Slog.w(TAG, "EVS Hal event occurs while the listener is null.");
            return;
        }

        dispatchHalEvents(values, listener);
    }

    private void dispatchHalEvents(List<VehiclePropValue> values, EvsHalEventListener listener) {
        for (int i = 0; i < values.size(); ++i) {
            VehiclePropValue v = values.get(i);
            boolean on = false;
            switch (v.prop) {
                case EVS_SERVICE_REQUEST:
                    // Sees
                    // android.hardware.automotive.vehicle.V2_0.VehicleProperty.EVS_SERVICE_REQUEST
                    int rawServiceType =
                            v.value.int32Values.get(EvsServiceRequestIndex.TYPE);
                    @CarEvsServiceType int type = rawServiceType == EvsServiceType.REARVIEW ?
                            SERVICE_TYPE_REARVIEW : SERVICE_TYPE_SURROUNDVIEW;
                    on = v.value.int32Values.get(
                            EvsServiceRequestIndex.STATE) == EvsServiceState.ON;
                    if (DBG) {
                        Slog.d(TAG, "Received EVS_SERVICE_REQUEST: type = " + type + " on = " + on);
                    }

                    listener.onEvent(type, on);
                    break;

                default:
                    if (DBG) {
                        Slog.d(TAG, "Received unknown property change: " + v);
                    }
                    break;
            }
        }
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(PrintWriter writer) {
        writer.println("*EVSHALSERVICE*");
        writer.printf("Use EVS_SERVICE_REQUEST: %b\n", isEvsServiceRequestSupported());
    }
}
