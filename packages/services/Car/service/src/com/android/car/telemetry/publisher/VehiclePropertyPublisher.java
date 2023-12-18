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

package com.android.car.telemetry.publisher;

import android.car.VehiclePropertyIds;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarPropertyEventListener;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Slog;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.car.CarPropertyService;
import com.android.car.telemetry.TelemetryProto;
import com.android.car.telemetry.databroker.DataSubscriber;
import com.android.internal.util.Preconditions;

import java.util.Collection;
import java.util.List;

/**
 * Publisher for Vehicle Property changes, aka {@code CarPropertyService}.
 *
 * <p>TODO(b/187525360): Add car property listener logic
 */
public class VehiclePropertyPublisher extends AbstractPublisher {
    private static final boolean DEBUG = false;  // STOPSHIP if true

    /** Bundle key for {@link CarPropertyEvent}. */
    public static final String CAR_PROPERTY_EVENT_KEY = "car_property_event";

    private final CarPropertyService mCarPropertyService;
    private final SparseArray<CarPropertyConfig> mCarPropertyList;

    private final ICarPropertyEventListener mCarPropertyEventListener =
            new ICarPropertyEventListener.Stub() {
                @Override
                public void onEvent(List<CarPropertyEvent> events) throws RemoteException {
                    if (DEBUG) {
                        Slog.d(CarLog.TAG_TELEMETRY,
                                "Received " + events.size() + " vehicle property events");
                    }
                    for (CarPropertyEvent event : events) {
                        onVehicleEvent(event);
                    }
                }
            };

    public VehiclePropertyPublisher(CarPropertyService carPropertyService) {
        mCarPropertyService = carPropertyService;
        // Load car property list once, as the list doesn't change runtime.
        mCarPropertyList = new SparseArray<>();
        for (CarPropertyConfig property : mCarPropertyService.getPropertyList()) {
            mCarPropertyList.append(property.getPropertyId(), property);
        }
    }

    @Override
    protected void onDataSubscriberAdded(DataSubscriber subscriber) {
        TelemetryProto.Publisher publisherParam = subscriber.getPublisherParam();
        Preconditions.checkArgument(
                publisherParam.getPublisherCase()
                        == TelemetryProto.Publisher.PublisherCase.VEHICLE_PROPERTY,
                "Subscribers only with VehicleProperty publisher are supported by this class.");
        int propertyId = publisherParam.getVehicleProperty().getVehiclePropertyId();
        CarPropertyConfig config = mCarPropertyList.get(propertyId);
        Preconditions.checkArgument(
                config != null,
                "Vehicle property " + VehiclePropertyIds.toString(propertyId) + " not found.");
        Preconditions.checkArgument(
                config.getAccess() == CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ
                        || config.getAccess()
                        == CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                "No access. Cannot read " + VehiclePropertyIds.toString(propertyId) + ".");
        mCarPropertyService.registerListener(
                propertyId,
                publisherParam.getVehicleProperty().getReadRate(),
                mCarPropertyEventListener);
    }

    @Override
    protected void onDataSubscribersRemoved(Collection<DataSubscriber> subscribers) {
        // TODO(b/190230611): Remove car property listener
    }

    /**
     * Called when publisher receives new events. It's called on CarPropertyService's worker
     * thread.
     */
    private void onVehicleEvent(CarPropertyEvent event) {
        Bundle bundle = new Bundle();
        bundle.putParcelable(CAR_PROPERTY_EVENT_KEY, event);
        for (DataSubscriber subscriber : getDataSubscribers()) {
            TelemetryProto.Publisher publisherParam = subscriber.getPublisherParam();
            if (event.getCarPropertyValue().getPropertyId()
                    != publisherParam.getVehicleProperty().getVehiclePropertyId()) {
                continue;
            }
            subscriber.push(bundle);
        }
    }
}
