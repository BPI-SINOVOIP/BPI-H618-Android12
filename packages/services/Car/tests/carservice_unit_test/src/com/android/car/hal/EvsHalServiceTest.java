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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.evs.CarEvsManager;
import android.hardware.automotive.vehicle.V2_0.EvsServiceRequestIndex;
import android.hardware.automotive.vehicle.V2_0.EvsServiceState;
import android.hardware.automotive.vehicle.V2_0.EvsServiceType;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.util.Log;

import com.android.car.vehiclehal.test.VehiclePropConfigBuilder;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Set;

@RunWith(MockitoJUnitRunner.class)
public class EvsHalServiceTest {
    @Mock VehicleHal mVehicleHal;
    @Mock EvsHalService.EvsHalEventListener mListener;

    private static final VehiclePropConfig EVS_SERVICE_REQUEST =
            VehiclePropConfigBuilder.newBuilder(VehicleProperty.EVS_SERVICE_REQUEST).build();

    private EvsHalService mEvsHalService;

    private final int TRUE = 1;
    private final int FALSE = 0;

    @Before
    public void setUp() {
        mEvsHalService = new EvsHalService(mVehicleHal);
        mEvsHalService.init();
    }

    @After
    public void tearDown() {
        mEvsHalService.release();
        mEvsHalService = null;
    }

    @Test
    public void takesEvsServiceRequestProperty() {
        Set<VehiclePropConfig> offeredProps = ImmutableSet.of(
                EVS_SERVICE_REQUEST);

        mEvsHalService.takeProperties(offeredProps);

        assertThat(mEvsHalService.isEvsServiceRequestSupported()).isTrue();
    }

    @Test
    public void requestToStartRearviewViaEvsServiceRequest() {
        subscribeListener(ImmutableSet.of(EVS_SERVICE_REQUEST));

        List<Integer> events = new ArrayList<>();
        doAnswer(invocation -> {
                events.add(invocation.getArgument(0));
                events.add(invocation.getArgument(1) ? TRUE : FALSE);
                return null;
        }).when(mListener).onEvent(anyInt(), anyBoolean());

        dispatchEvsServiceRequest(EvsServiceType.REARVIEW, EvsServiceState.ON);

        assertThat(events.get(0)).isEqualTo(CarEvsManager.SERVICE_TYPE_REARVIEW);
        assertThat(events.get(1)).isEqualTo(TRUE);
    }

    @Test
    public void requestToStopRearviewViaEvsServiceRequest() {
        subscribeListener(ImmutableSet.of(EVS_SERVICE_REQUEST));

        List<Integer> events = new ArrayList<>();
        doAnswer(invocation -> {
                events.add(invocation.getArgument(0));
                events.add(invocation.getArgument(1) ? TRUE : FALSE);
                return null;
        }).when(mListener).onEvent(anyInt(), anyBoolean());

        dispatchEvsServiceRequest(EvsServiceType.REARVIEW, EvsServiceState.OFF);

        assertThat(events.get(0)).isEqualTo(CarEvsManager.SERVICE_TYPE_REARVIEW);
        assertThat(events.get(1)).isEqualTo(FALSE);
    }

    // TODO(b/179029031): Adds more tests to verify the surround view service integration.

    private void dispatchEvsServiceRequest(int type, int state) {
        mEvsHalService.onHalEvents(
                ImmutableList.of(buildEvsServiceRequestProp(type, state)));
    }

    private VehiclePropValue buildEvsServiceRequestProp(int type, int state) {
        VehiclePropValue v = new VehiclePropValue();
        v.prop = VehicleProperty.EVS_SERVICE_REQUEST;
        v.value.int32Values.add(type);
        v.value.int32Values.add(state);

        return v;
    }

    private void subscribeListener(Collection<VehiclePropConfig> properties) {
        reset(mListener);
        mEvsHalService.takeProperties(properties);
        mEvsHalService.setListener(mListener);

        for (VehiclePropConfig config : properties) {
            verify(mVehicleHal).subscribeProperty(mEvsHalService, config.prop);
        }
    }
}
