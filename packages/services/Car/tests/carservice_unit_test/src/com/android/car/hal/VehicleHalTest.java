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

package com.android.car.hal;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.hardware.property.CarPropertyManager;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.IVehicleCallback;
import android.hardware.automotive.vehicle.V2_0.SubscribeOptions;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyAccess;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyChangeMode;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public final class VehicleHalTest {

    private static final int SOME_READ_ON_CHANGE_PROPERTY = 0x01;
    private static final int SOME_READ_WRITE_STATIC_PROPERTY = 0x02;
    private static final int UNSUPPORTED_PROPERTY = -1;

    private static final float ANY_SAMPLING_RATE = 60f;
    private static final int NO_FLAGS = 0;

    @Mock private IVehicle mIVehicle;

    @Mock private PowerHalService mPowerHalService;
    @Mock private PropertyHalService mPropertyHalService;
    @Mock private InputHalService mInputHalService;
    @Mock private VmsHalService mVmsHalService;
    @Mock private UserHalService mUserHalService;
    @Mock private DiagnosticHalService mDiagnosticHalService;
    @Mock private ClusterHalService mClusterHalService;
    @Mock private HalClient mHalClient;

    private VehicleHal mVehicleHal;

    /** Hal services configurations */
    private final ArrayList<VehiclePropConfig> mConfigs = new ArrayList<>();

    @Before
    public void setUp() throws Exception {
        mVehicleHal = new VehicleHal(mPowerHalService,
                mPropertyHalService, mInputHalService, mVmsHalService, mUserHalService,
                mDiagnosticHalService, mClusterHalService, mHalClient);

        mConfigs.clear();

        // Initialize PowerHAL service with a READ_WRITE and ON_CHANGE property
        when(mPowerHalService.getAllSupportedProperties()).thenReturn(
                new int[]{SOME_READ_ON_CHANGE_PROPERTY});
        VehiclePropConfig powerHalConfig = new VehiclePropConfig();
        powerHalConfig.prop = SOME_READ_ON_CHANGE_PROPERTY;
        powerHalConfig.access = VehiclePropertyAccess.READ_WRITE;
        powerHalConfig.changeMode = VehiclePropertyChangeMode.ON_CHANGE;
        mConfigs.add(powerHalConfig);
        assertThat(VehicleHal.isPropertySubscribable(powerHalConfig)).isTrue();

        // Initialize PropertyHAL service with a READ_WRITE and STATIC property
        when(mPropertyHalService.getAllSupportedProperties()).thenReturn(
                new int[]{SOME_READ_WRITE_STATIC_PROPERTY});
        VehiclePropConfig propertyHalConfig = new VehiclePropConfig();
        propertyHalConfig.prop = SOME_READ_WRITE_STATIC_PROPERTY;
        propertyHalConfig.access = VehiclePropertyAccess.READ_WRITE;
        propertyHalConfig.changeMode = VehiclePropertyChangeMode.STATIC;
        mConfigs.add(propertyHalConfig);
        assertThat(VehicleHal.isPropertySubscribable(propertyHalConfig)).isFalse();

        // Initialize the remaining services with empty properties
        when(mInputHalService.getAllSupportedProperties()).thenReturn(new int[0]);
        when(mVmsHalService.getAllSupportedProperties()).thenReturn(new int[0]);
        when(mUserHalService.getAllSupportedProperties()).thenReturn(new int[0]);
        when(mDiagnosticHalService.getAllSupportedProperties()).thenReturn(new int[0]);

        when(mHalClient.getAllPropConfigs()).thenReturn(mConfigs);
        mVehicleHal.init();
    }

    @Test
    public void testSubscribeProperty_registeringReadWriteAndOnChangeProperty() throws Exception {
        // Act
        mVehicleHal.subscribeProperty(mPowerHalService, SOME_READ_ON_CHANGE_PROPERTY,
                ANY_SAMPLING_RATE, NO_FLAGS);

        // Assert
        SubscribeOptions expectedOptions = new SubscribeOptions();
        expectedOptions.propId = SOME_READ_ON_CHANGE_PROPERTY;
        expectedOptions.sampleRate = ANY_SAMPLING_RATE;
        expectedOptions.flags = NO_FLAGS;

        verify(mHalClient).subscribe(eq(expectedOptions));
    }

    @Test
    public void testUnsubscribeProperty_subscribedProperty() throws Exception {
        //Act
        mVehicleHal.unsubscribeProperty(mPowerHalService, SOME_READ_ON_CHANGE_PROPERTY);

        // Assert
        verify(mHalClient).unsubscribe(eq(SOME_READ_ON_CHANGE_PROPERTY));
    }

    @Test
    public void testUnsubscribeProperty_unsubscribedProperty() throws Exception {
        //Act
        mVehicleHal.unsubscribeProperty(mPowerHalService, UNSUPPORTED_PROPERTY);

        // Assert
        verify(mHalClient, never()).unsubscribe(anyInt());
    }

    @Test
    public void testSubscribeProperty_registeringStaticProperty() throws Exception {
        // Act
        mVehicleHal.subscribeProperty(mPowerHalService, SOME_READ_WRITE_STATIC_PROPERTY,
                ANY_SAMPLING_RATE, NO_FLAGS);

        // Assert
        verify(mHalClient, never()).subscribe(any(SubscribeOptions.class));
    }

    @Test
    public void testGetSampleRate_unsupportedProperty() {
        assertThat(mVehicleHal.getSampleRate(UNSUPPORTED_PROPERTY)).isEqualTo(
                VehicleHal.NO_SAMPLE_RATE);
    }

    @Test
    public void testGetSampleRate_supportedAndRegisteredProperty() {
        // Act
        mVehicleHal.subscribeProperty(mPowerHalService, SOME_READ_ON_CHANGE_PROPERTY,
                ANY_SAMPLING_RATE, NO_FLAGS);

        // Assert
        assertThat(mVehicleHal.getSampleRate(SOME_READ_ON_CHANGE_PROPERTY)).isEqualTo(
                ANY_SAMPLING_RATE);
    }

    @Test
    public void testOnPropertyEvent() {
        // Arrange
        List<VehiclePropValue> dispatchList = mock(List.class);
        when(mPowerHalService.getDispatchList()).thenReturn(dispatchList);

        VehiclePropValue propValue = new VehiclePropValue();
        propValue.prop = SOME_READ_ON_CHANGE_PROPERTY;
        propValue.areaId = VehicleHal.NO_AREA;
        ArrayList<VehiclePropValue> propValues = new ArrayList<>();
        propValues.add(propValue);

        // Act
        mVehicleHal.onPropertyEvent(propValues);

        // Assert
        verify(dispatchList).add(propValue);
        verify(mPowerHalService).onHalEvents(dispatchList);
        verify(dispatchList).clear();
    }

    @Test
    public void testOnPropertySetError() {
        // Arrange
        int errorCode = CarPropertyManager.CAR_SET_PROPERTY_ERROR_CODE_TRY_AGAIN;
        int propId = SOME_READ_ON_CHANGE_PROPERTY;
        int areaId = VehicleHal.NO_AREA;

        // Act
        mVehicleHal.onPropertySetError(errorCode, propId, areaId);

        // Assert
        verify(mPowerHalService).onPropertySetError(propId, areaId, errorCode);
    }

    @Test
    public void testInjectOnPropertySetError() {
        // Arrange
        int errorCode = CarPropertyManager.CAR_SET_PROPERTY_ERROR_CODE_TRY_AGAIN;
        int propId = SOME_READ_ON_CHANGE_PROPERTY;
        int areaId = VehicleHal.NO_AREA;

        // Act
        mVehicleHal.onPropertySetError(errorCode, propId, areaId);

        // Assert
        verify(mPowerHalService).onPropertySetError(propId, areaId, errorCode);
    }

    @Test
    public void testVehicleHalReconnected() throws Exception {
        // Arrange
        IVehicle mockVehicle = mock(IVehicle.class);

        // Act
        mVehicleHal.vehicleHalReconnected(mockVehicle);

        // Assert
        ArgumentCaptor<IVehicleCallback> vehicleCallbackCaptor = ArgumentCaptor.forClass(
                IVehicleCallback.class);
        ArgumentCaptor<ArrayList> optionsCaptor = ArgumentCaptor.forClass(ArrayList.class);
        verify(mockVehicle, times(1)).subscribe(vehicleCallbackCaptor.capture(),
                optionsCaptor.capture());
        assertThat(vehicleCallbackCaptor.getValue()).isNotNull();
        assertThat(optionsCaptor.getValue()).isEmpty();
    }

    @Test
    public void testGetIfAvailableOrFail() {
        // Arrange
        VehiclePropValue propValue = new VehiclePropValue();
        propValue.prop = SOME_READ_ON_CHANGE_PROPERTY;
        propValue.areaId = VehicleHal.NO_AREA;
        when(mHalClient.getValue(any(VehiclePropValue.class))).thenReturn(propValue);

        // Act
        VehiclePropValue actual = mVehicleHal.getIfAvailableOrFail(SOME_READ_ON_CHANGE_PROPERTY,
                /* numberOfRetries= */ 1);

        // Assert
        assertThat(actual).isEqualTo(propValue);
    }

    // Testing dump methods

    @Test
    public void testDump() {
        // Arrange
        StringWriter writer = new StringWriter();
        PrintWriter printWriter = new PrintWriter(writer);

        // Act
        mVehicleHal.dump(printWriter);

        // Assert
        String actual = writer.toString();
        assertThat(actual).contains("Property Id: 1 // 0x1 name: 0x1, service: mPowerHalService");
        assertThat(actual).contains(
                "Property Id: 2 // 0x2 name: 0x2, service: mPropertyHalService");
    }

    @Test
    public void testDumpListHals() {
        // Arrange
        StringWriter writer = new StringWriter();
        PrintWriter printWriter = new PrintWriter(writer);

        // Act
        mVehicleHal.dumpListHals(printWriter);

        // Assert
        assertServiceNamesAreDumped(writer.toString());
    }

    @Test
    public void testDumpSpecificHals() {
        // Arrange
        StringWriter writer = new StringWriter();
        PrintWriter printWriter = new PrintWriter(writer);

        // Act
        mVehicleHal.dumpListHals(printWriter);

        // Assert
        assertServiceNamesAreDumped(writer.toString());
    }

    private void assertServiceNamesAreDumped(String actual) {
        assertThat(actual).contains("com.android.car.hal.PowerHalService");
        assertThat(actual).contains("com.android.car.hal.InputHalService");
        assertThat(actual).contains("com.android.car.hal.DiagnosticHalService");
        assertThat(actual).contains("com.android.car.hal.VmsHalService");
        assertThat(actual).contains("com.android.car.hal.UserHalService");
        assertThat(actual).contains("com.android.car.hal.PropertyHalService");
    }

    @Test
    public void testDumpPropertyValueByCommend() {
        // Arrange
        StringWriter writer = new StringWriter();
        PrintWriter printWriter = new PrintWriter(writer);

        VehiclePropValue propValue = new VehiclePropValue();
        propValue.value = new VehiclePropValue.RawValue();
        propValue.value.stringValue = "some_value";
        when(mHalClient.getValue(any(VehiclePropValue.class))).thenReturn(propValue);

        // Act
        mVehicleHal.dumpPropertyValueByCommend(printWriter, /* propId= */ -1, /* areaId= */-1);

        // Assert
        assertThat(writer.toString()).contains("string: some_value");
    }

    // Testing vehicle hal property getters

    @Test
    public void testGetForPropertyIdAndAreaId() {
        // Arrange
        int propertyId = 123;  // Any property id
        int areaId = 456;  // Any area id

        // Act
        mVehicleHal.get(propertyId, areaId);

        // Assert
        VehiclePropValue expectedPropValue = new VehiclePropValue();
        expectedPropValue.prop = propertyId;
        expectedPropValue.areaId = areaId;
        verify(mHalClient).getValue(eq(expectedPropValue));
    }

    @Test
    public void testGet_VehiclePropValue() {
        // Arrange
        VehiclePropValue propValue = new VehiclePropValue();

        // Act
        mVehicleHal.get(propValue);

        // Assert
        verify(mHalClient).getValue(propValue);
    }
}
