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

import static android.car.hardware.property.CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarPropertyEventListener;
import android.os.Bundle;

import com.android.car.CarPropertyService;
import com.android.car.telemetry.TelemetryProto;
import com.android.car.telemetry.databroker.DataSubscriber;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.Collections;
import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public class VehiclePropertyPublisherTest {
    private static final int PROP_ID_1 = 100;
    private static final int PROP_ID_2 = 102;
    private static final int AREA_ID = 20;
    private static final float PROP_READ_RATE = 0.0f;
    private static final CarPropertyEvent PROP_EVENT_1 =
            new CarPropertyEvent(PROPERTY_EVENT_PROPERTY_CHANGE,
                    new CarPropertyValue<>(PROP_ID_1, AREA_ID, /* value= */ 1));

    private static final TelemetryProto.Publisher PUBLISHER_PARAMS_1 =
            TelemetryProto.Publisher.newBuilder()
                    .setVehicleProperty(TelemetryProto.VehiclePropertyPublisher.newBuilder()
                            .setReadRate(PROP_READ_RATE)
                            .setVehiclePropertyId(PROP_ID_1))
                    .build();
    private static final TelemetryProto.Publisher PUBLISHER_PARAMS_INVALID =
            TelemetryProto.Publisher.newBuilder()
                    .setVehicleProperty(TelemetryProto.VehiclePropertyPublisher.newBuilder()
                            .setReadRate(PROP_READ_RATE)
                            .setVehiclePropertyId(-200))
                    .build();

    // mMockCarPropertyService supported car property list.
    private static final CarPropertyConfig<Integer> PROP_CONFIG_1 =
            CarPropertyConfig.newBuilder(Integer.class, PROP_ID_1, AREA_ID).setAccess(
                    CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ).build();
    private static final CarPropertyConfig<Integer> PROP_CONFIG_2_WRITE_ONLY =
            CarPropertyConfig.newBuilder(Integer.class, PROP_ID_2, AREA_ID).setAccess(
                    CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_WRITE).build();

    @Mock
    private DataSubscriber mMockDataSubscriber;

    @Mock
    private CarPropertyService mMockCarPropertyService;

    @Captor
    private ArgumentCaptor<ICarPropertyEventListener> mCarPropertyCallbackCaptor;
    @Captor
    private ArgumentCaptor<Bundle> mBundleCaptor;

    private VehiclePropertyPublisher mVehiclePropertyPublisher;

    @Before
    public void setUp() {
        when(mMockDataSubscriber.getPublisherParam()).thenReturn(PUBLISHER_PARAMS_1);
        when(mMockCarPropertyService.getPropertyList())
                .thenReturn(List.of(PROP_CONFIG_1, PROP_CONFIG_2_WRITE_ONLY));
        mVehiclePropertyPublisher = new VehiclePropertyPublisher(mMockCarPropertyService);
    }

    @Test
    public void testAddDataSubscriber_registersNewCallback() {
        mVehiclePropertyPublisher.addDataSubscriber(mMockDataSubscriber);

        verify(mMockCarPropertyService).registerListener(eq(PROP_ID_1), eq(PROP_READ_RATE), any());
        assertThat(mVehiclePropertyPublisher.getDataSubscribers()).hasSize(1);
    }

    @Test
    public void testAddDataSubscriber_failsIfInvalidCarProperty() {
        DataSubscriber invalidDataSubscriber = mock(DataSubscriber.class);
        when(invalidDataSubscriber.getPublisherParam()).thenReturn(
                TelemetryProto.Publisher.newBuilder()
                        .setVehicleProperty(TelemetryProto.VehiclePropertyPublisher.newBuilder()
                                .setReadRate(PROP_READ_RATE)
                                .setVehiclePropertyId(PROP_ID_2))
                        .build());

        Throwable error = assertThrows(IllegalArgumentException.class,
                () -> mVehiclePropertyPublisher.addDataSubscriber(invalidDataSubscriber));

        assertThat(error).hasMessageThat().contains("No access.");
        assertThat(mVehiclePropertyPublisher.getDataSubscribers()).isEmpty();
    }

    @Test
    public void testAddDataSubscriber_failsIfNoReadAccess() {
        DataSubscriber invalidDataSubscriber = mock(DataSubscriber.class);
        when(invalidDataSubscriber.getPublisherParam()).thenReturn(PUBLISHER_PARAMS_INVALID);

        Throwable error = assertThrows(IllegalArgumentException.class,
                () -> mVehiclePropertyPublisher.addDataSubscriber(invalidDataSubscriber));

        assertThat(error).hasMessageThat().contains("not found");
        assertThat(mVehiclePropertyPublisher.getDataSubscribers()).isEmpty();
    }

    @Test
    public void testRemoveDataSubscriber_succeeds() {
        mVehiclePropertyPublisher.addDataSubscriber(mMockDataSubscriber);

        mVehiclePropertyPublisher.removeDataSubscriber(mMockDataSubscriber);
        // TODO(b/189143814): add proper verification
    }

    @Test
    public void testRemoveDataSubscriber_failsIfNotFound() {
        Throwable error = assertThrows(IllegalArgumentException.class,
                () -> mVehiclePropertyPublisher.removeDataSubscriber(mMockDataSubscriber));

        assertThat(error).hasMessageThat().contains("subscriber not found");
    }

    @Test
    public void testRemoveAllDataSubscribers_succeeds() {
        mVehiclePropertyPublisher.removeAllDataSubscribers();
        // TODO(b/189143814): add tests
    }

    @Test
    public void testOnNewCarPropertyEvent_pushesValueToDataSubscriber() throws Exception {
        doNothing().when(mMockCarPropertyService).registerListener(
                anyInt(), anyFloat(), mCarPropertyCallbackCaptor.capture());
        mVehiclePropertyPublisher.addDataSubscriber(mMockDataSubscriber);

        mCarPropertyCallbackCaptor.getValue().onEvent(Collections.singletonList(PROP_EVENT_1));

        verify(mMockDataSubscriber).push(mBundleCaptor.capture());
        CarPropertyEvent event = mBundleCaptor.getValue().getParcelable(
                VehiclePropertyPublisher.CAR_PROPERTY_EVENT_KEY);
        assertThat(event).isEqualTo(PROP_EVENT_1);
    }
}
