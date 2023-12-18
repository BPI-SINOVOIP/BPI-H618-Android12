/*
 * Copyright (C) 2019 The Android Open Source Project
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

import static android.car.hardware.power.PowerComponentUtil.FIRST_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.LAST_POWER_COMPONENT;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
import android.car.AbstractExtendedMockitoCarServiceTestCase;
import android.car.VehicleAreaSeat;
import android.car.VehiclePropertyIds;
import android.car.VehicleSeatOccupancyState;
import android.car.drivingstate.CarDrivingStateEvent;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarPropertyEventListener;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.UserManager;
import android.provider.Settings;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.util.Log;

import androidx.test.filters.FlakyTest;
import androidx.test.filters.RequiresDevice;

import com.android.car.power.CarPowerManagementService;
import com.android.car.systeminterface.SystemInterface;
import com.android.internal.app.IVoiceInteractionManagerService;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.Invocation;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import java.util.Arrays;
import java.util.Collection;
import java.util.stream.Collectors;

/**
 * Unit tests for {@link BluetoothDeviceConnectionPolicy}
 *
 * Run:
 * atest BluetoothDeviceConnectionPolicyTest
 */
@RequiresDevice
@RunWith(MockitoJUnitRunner.class)
public class BluetoothDeviceConnectionPolicyTest extends AbstractExtendedMockitoCarServiceTestCase {
    private static final String TAG = BluetoothDeviceConnectionPolicyTest.class.getSimpleName();
    private static final boolean VERBOSE = false;

    private static final long WAIT_TIMEOUT_MS = 5000;

    @Mock private Context mMockContext;
    @Mock private Resources mMockResources;
    private MockContentResolver mMockContentResolver;
    private MockContentProvider mMockContentProvider;
    @Mock private PackageManager mMockPackageManager;
    @Mock private UserManager mMockUserManager;
    @Mock private CarBluetoothService mMockBluetoothService;
    @Mock private IVoiceInteractionManagerService mMockVoiceService;
    @Mock private SystemInterface mMockSystemInterface;
    @Mock private CarPowerManagementService mMockCarPowerManagementService;
    @Mock private CarPropertyService mMockCarPropertyService;
    @Mock private CarDrivingStateService mMockCarDrivingStateService;

    private final int mUserId = 10;

    private BluetoothDeviceConnectionPolicy mPolicy;
    private BluetoothAdapterHelper mBluetoothAdapterHelper;
    private BroadcastReceiver mReceiver;
    @Captor private ArgumentCaptor<ICarPropertyEventListener> mSeatListenerCaptor;

    // Can't set these programmatically in individual tests since SeatOnOccupiedListener.mDriverSeat
    // is final, and BluetoothDeviceConnectionPolicy.mSeatOnOccupiedListener is also final.
    // BluetoothDeviceConnectionPolicy is created once in setUp(), so individual tests cannot set
    // the driver's seat location programmatically.
    //
    // Please ensure the two seats are different values.
    private static final int DRIVER_SEAT = VehicleAreaSeat.SEAT_ROW_1_RIGHT;
    private static final int PASSENGER_SEAT = VehicleAreaSeat.SEAT_ROW_1_LEFT;

    @Override
    protected void onSessionBuilder(CustomMockitoSessionBuilder session) {
        session.spyStatic(CarLocalServices.class);
    }

    //--------------------------------------------------------------------------------------------//
    // Setup/TearDown                                                                             //
    //--------------------------------------------------------------------------------------------//

    @Before
    public void setUp() {
        mMockContentResolver = new MockContentResolver(null);
        mMockContentProvider = new MockContentProvider() {
            @Override
            public Bundle call(String method, String request, Bundle args) {
                return new Bundle();
            }
        };
        mMockContentResolver.addProvider(Settings.AUTHORITY, mMockContentProvider);

        when(mMockContext.getResources()).thenReturn(mMockResources);
        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);
        when(mMockContext.getApplicationContext()).thenReturn(mMockContext);
        when(mMockContext.getPackageManager()).thenReturn(mMockPackageManager);
        when(mMockContext.getSystemService(UserManager.class)).thenReturn(mMockUserManager);
        when(mMockUserManager.isUserUnlocked(mUserId)).thenReturn(true);

        mockGetCarLocalService(CarPropertyService.class, mMockCarPropertyService);
        mockGetCarLocalService(CarDrivingStateService.class, mMockCarDrivingStateService);

        // setting the driver's seat location
        when(mMockCarPropertyService
                .getPropertySafe(eq(VehiclePropertyIds.INFO_DRIVER_SEAT), anyInt()))
                .thenReturn(new CarPropertyValue<Integer>(VehiclePropertyIds.INFO_DRIVER_SEAT,
                0 /*areaId*/, new Integer(DRIVER_SEAT)));

        // Make sure we grab and store the bluetooth broadcast receiver object
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                if (arguments != null && arguments.length == 5 && arguments[0] != null) {
                    mReceiver = (BroadcastReceiver) arguments[0];
                }
                return null;
            }
        }).when(mMockContext).registerReceiverAsUser(any(BroadcastReceiver.class), any(), any(),
                any(), any());

        mBluetoothAdapterHelper = new BluetoothAdapterHelper();
        mBluetoothAdapterHelper.init();

        mPolicy = BluetoothDeviceConnectionPolicy.create(mMockContext, mUserId,
                mMockBluetoothService);
        Assert.assertTrue(mPolicy != null);

        // Get the seat occupancy listener
        doNothing().when(mMockCarPropertyService)
                .registerListener(anyInt(), anyFloat(), mSeatListenerCaptor.capture());

        CarLocalServices.addService(CarPowerManagementService.class,
                mMockCarPowerManagementService);
    }

    @After
    public void tearDown() {
        mPolicy.release();
        mBluetoothAdapterHelper.release();
        CarLocalServices.removeServiceForTest(CarPowerManagementService.class);
    }

    //--------------------------------------------------------------------------------------------//
    // Utilities                                                                                  //
    //--------------------------------------------------------------------------------------------//

    private void sendAdapterStateChanged(int newState) {
        if (mReceiver != null) {
            Intent intent = new Intent(BluetoothAdapter.ACTION_STATE_CHANGED);
            intent.putExtra(BluetoothAdapter.EXTRA_STATE, newState);
            mReceiver.onReceive(null, intent);
        }
    }

    private void sendPowerPolicyBluetoothOnOff(boolean isOn) throws Exception {
        int[] allComponents = new int[LAST_POWER_COMPONENT - FIRST_POWER_COMPONENT + 1];
        for (int component = FIRST_POWER_COMPONENT; component <= LAST_POWER_COMPONENT;
                component++) {
            allComponents[component - FIRST_POWER_COMPONENT] = component;
        }
        int[] noComponents = new int[]{};
        CarPowerPolicy policy;
        if (isOn) {
            policy = new CarPowerPolicy("bt_on", allComponents, noComponents);
        } else {
            policy = new CarPowerPolicy("bt_off", noComponents, allComponents);
        }
        mPolicy.getPowerPolicyListener().onPolicyChanged(policy, policy);
    }

    private void sendSeatOnOccupied(int seat) {
        CarPropertyValue<Integer> value = new CarPropertyValue<Integer>(
                VehiclePropertyIds.SEAT_OCCUPANCY, seat,
                new Integer(VehicleSeatOccupancyState.OCCUPIED));
        CarPropertyEvent event = new CarPropertyEvent(
                CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE, value);
        try {
            mSeatListenerCaptor.getValue().onEvent(Arrays.asList(event));
        } catch (Throwable e) {
            Log.e(TAG, "sendSeatOnOccupied: " + e);
        }

    }

    private void setDrivingState(int value) {
        when(mMockCarDrivingStateService.getCurrentDrivingState())
                .thenReturn(new CarDrivingStateEvent(value, 0 /*timeStamp*/));
    }

    private int getNumberOfConnectDevicesCalls() {
        Collection<Invocation> invocations =
                Mockito.mockingDetails(mMockBluetoothService).getInvocations();

        return invocations.stream()
                .filter(inv -> "connectDevices".equals(inv.getMethod().getName()))
                .collect(Collectors.toList())
                .size();
    }

    //--------------------------------------------------------------------------------------------//
    // Policy Init tests                                                                          //
    //--------------------------------------------------------------------------------------------//

    /**
     * Preconditions:
     * - Adapter is on
     *
     * Action:
     * - Initialize the policy
     *
     * Outcome:
     * - Because the Adapter is ON at init time, we should attempt to connect devices
     */
    @Test
    public void testInitWithAdapterOn_connectDevices() {
        mBluetoothAdapterHelper.forceAdapterOn();
        mPolicy.init();
        verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeastOnce()).connectDevices();
    }

    /**
     * Preconditions:
     * - Adapter is off
     *
     * Action:
     * - Initialize the policy
     *
     * Outcome:
     * - Because the Adapter is OFF at init time, we should not attempt to connect devices
     */
    @Test
    public void testInitWithAdapterOff_doNothing() {
        mBluetoothAdapterHelper.forceAdapterOff();
        mPolicy.init();
        verify(mMockBluetoothService, times(0)).connectDevices();
    }

    //--------------------------------------------------------------------------------------------//
    // Car Power Manager state changed event tests                                                //
    //--------------------------------------------------------------------------------------------//

    /**
     * Preconditions:
     * - Adapter is on
     *
     * Action:
     * - Receive a SHUTDOWN_PREPARE call
     *
     * Outcome:
     * - Adapter is turned off but the ON state is persisted
     */
    @Test
    public void testReceivePowerShutdownPrepare_disableBluetooth() throws Exception {
        mBluetoothAdapterHelper.forceAdapterOn();
        mPolicy.init();
        reset(mMockBluetoothService);

        sendPowerPolicyBluetoothOnOff(false);
        mBluetoothAdapterHelper.waitForAdapterOff();
        Assert.assertTrue(mBluetoothAdapterHelper.isAdapterPersistedOn());
    }

    /**
     * Preconditions:
     * - Adapter is off and is persisted off
     * - Policy is initialized
     *
     * Action:
     * - Power state ON is received
     *
     * Outcome:
     * - Because the Adapter is persisted off, we should do nothing. The adapter should remain off
     */
    @Test
    public void testReceivePowerOnBluetoothPersistedOff_doNothing() throws Exception {
        mBluetoothAdapterHelper.forceAdapterOff();
        mPolicy.init();
        reset(mMockBluetoothService);

        sendPowerPolicyBluetoothOnOff(true);
        mBluetoothAdapterHelper.waitForAdapterOff();
    }

     /**
     * Preconditions:
     * - Adapter is off and is not persisted off
     * - Policy is initialized
     *
     * Action:
     * - Power state ON is received
     *
     * Outcome:
     * - Because the Adapter is not persisted off, we should turn it back on. No attempt to connect
     *   devices is made because we're yielding to the adapter ON event.
     */
    @Test
    @FlakyTest // This occasionally fails to connect
    public void testReceivePowerOnBluetoothOffNotPersisted_BluetoothOnConnectDevices()
            throws Exception {
        mBluetoothAdapterHelper.forceAdapterOffDoNotPersist();
        mPolicy.init();
        reset(mMockBluetoothService);

        sendPowerPolicyBluetoothOnOff(true);
        verify(mMockBluetoothService, times(0)).connectDevices();
        mBluetoothAdapterHelper.waitForAdapterOn();
    }

    /**
     * Preconditions:
     * - Adapter is on
     * - Policy is initialized
     *
     * Action:
     * - Power state ON is received
     *
     * Outcome:
     * - Because the Adapter on, we should attempt to connect devices
     */
    @Test
    public void testReceivePowerOnBluetoothOn_connectDevices() throws Exception {
        mBluetoothAdapterHelper.forceAdapterOn();
        mPolicy.init();
        reset(mMockBluetoothService);

        sendPowerPolicyBluetoothOnOff(true);
        verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeastOnce()).connectDevices();
    }

    //--------------------------------------------------------------------------------------------//
    // Bluetooth stack adapter status changed event tests                                         //
    //--------------------------------------------------------------------------------------------//

    /**
     * Preconditions:
     * - Policy is initialized
     *
     * Action:
     * - Adapter state TURNING_OFF is received
     *
     * Outcome:
     * - Do nothing
     */
    @Test
    public void testReceiveAdapterTurningOff_doNothing() {
        mPolicy.init();
        reset(mMockBluetoothService);

        sendAdapterStateChanged(BluetoothAdapter.STATE_TURNING_OFF);
        verify(mMockBluetoothService, times(0)).connectDevices();
    }

    /**
     * Preconditions:
     * - Policy is initialized
     *
     * Action:
     * - Adapter state OFF is received
     *
     * Outcome:
     * - Do nothing
     */
    @Test
    public void testReceiveAdapterOff_doNothing() {
        mPolicy.init();
        reset(mMockBluetoothService);

        sendAdapterStateChanged(BluetoothAdapter.STATE_OFF);
        verify(mMockBluetoothService, times(0)).connectDevices();
    }

    /**
     * Preconditions:
     * - Policy is initialized
     *
     * Action:
     * - Adapter state TURNING_ON is received
     *
     * Outcome:
     * - Do nothing
     */
    @Test
    public void testReceiveAdapterTurningOn_doNothing() {
        mPolicy.init();
        reset(mMockBluetoothService);

        sendAdapterStateChanged(BluetoothAdapter.STATE_TURNING_ON);
        verify(mMockBluetoothService, times(0)).connectDevices();
    }

    /**
     * Preconditions:
     * - Policy is initialized
     *
     * Action:
     * - Adapter state ON is received
     *
     * Outcome:
     * - Attempt to connect devices
     */
    @Test
    public void testReceiveAdapterOn_connectDevices() {
        mPolicy.init();
        reset(mMockBluetoothService);

        sendAdapterStateChanged(BluetoothAdapter.STATE_ON);
        verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeastOnce()).connectDevices();
    }

    //--------------------------------------------------------------------------------------------//
    // Seat occupancy event tests                                                                 //
    //--------------------------------------------------------------------------------------------//

    /**
     * Preconditions:
     * - Policy is initialized
     * - Adapter is ON
     * - Car is in parked state
     *
     * Action:
     * - Driver's seat sensor OCCUPIED is received
     *
     * Outcome:
     * - Attempt to connect devices
     */
    @Test
    public void testSeatOnOccupied_driverSeat_parked_connectDevices() {
        mBluetoothAdapterHelper.forceAdapterOn();
        mBluetoothAdapterHelper.waitForAdapterOn();
        mPolicy.init();
        setDrivingState(CarDrivingStateEvent.DRIVING_STATE_PARKED);

        reset(mMockBluetoothService);
        // to catch any prior invocation of connectDevices() and remove a race condition
        verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeast(0)).connectDevices();
        int numCalls = getNumberOfConnectDevicesCalls();
        if (VERBOSE) Log.v(TAG, "prior invocations (driver's seat, parked): " + numCalls);

        sendSeatOnOccupied(DRIVER_SEAT);

        verify(mMockBluetoothService, atLeast(numCalls + 1)).connectDevices();
    }

    /**
     * Preconditions:
     * - Policy is initialized
     * - Adapter is ON
     * - Car is in parked state
     *
     * Action:
     * - Passenger's seat sensor OCCUPIED is received
     *
     * Outcome:
     * - Do nothing
     */
    @Test
    public void testSeatOnOccupied_passengerSeat_parked_doNothing() {
        mBluetoothAdapterHelper.forceAdapterOn();
        mBluetoothAdapterHelper.waitForAdapterOn();
        mPolicy.init();
        setDrivingState(CarDrivingStateEvent.DRIVING_STATE_PARKED);

        reset(mMockBluetoothService);
        // There's an apparent race condition with a prior invocation of connectDevices() which
        // causes verify(mMockBluetoothService, times(0)).connectDevices() to be flaky. Also,
        // calling verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeastOnce())
        // .connectDevices() right after reset() will pass the test consistently.
        // Instead, we check to make sure there's no change in the number of invocations between
        // before and after our call.
        verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeast(0)).connectDevices();
        int numCalls = getNumberOfConnectDevicesCalls();
        if (VERBOSE) Log.v(TAG, "prior invocations (passenger's seat, parked): " + numCalls);

        sendSeatOnOccupied(PASSENGER_SEAT);

        verify(mMockBluetoothService, times(numCalls)).connectDevices();
    }

    /**
     * Preconditions:
     * - Policy is initialized
     * - Adapter is ON
     * - Car is in driving state
     *
     * Action:
     * - Driver's seat sensor OCCUPIED is received
     *
     * Outcome:
     * - Do nothing
     */
    @Test
    public void testSeatOnOccupied_driverSeat_driving_doNothing() {
        mBluetoothAdapterHelper.forceAdapterOn();
        mBluetoothAdapterHelper.waitForAdapterOn();
        mPolicy.init();
        setDrivingState(CarDrivingStateEvent.DRIVING_STATE_MOVING);

        reset(mMockBluetoothService);
        // to catch any prior invocation of connectDevices() and remove a race condition
        verify(mMockBluetoothService, timeout(WAIT_TIMEOUT_MS).atLeast(0)).connectDevices();
        int numCalls = getNumberOfConnectDevicesCalls();
        if (VERBOSE) Log.v(TAG, "prior invocations (driver's seat, driving): " + numCalls);

        sendSeatOnOccupied(DRIVER_SEAT);

        verify(mMockBluetoothService, times(numCalls)).connectDevices();
    }

    /**
     * Tests the case where if {@link VehiclePropertyIds.INFO_DRIVER_SEAT} is not registered or is
     * not available, then car policy should still be able to proceed to be created, and not crash
     * car Bluetooth.
     *
     * Preconditions:
     * - {@code mCarPropertyService.getProperty(VehiclePropertyIds.INFO_DRIVER_SEAT,
     *   VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL)} returns {@code null}.
     *
     * Action:
     * - Create a new {@link BluetoothDeviceConnectionPolicy}.
     *
     * Outcome:
     * - A new instance of {@link BluetoothDeviceConnectionPolicy} is successfully created, e.g.,
     *   no NPE or anything else that prevents creation of policy in this case
     */
    @Test
    public void testGetDriverSeatLocationNull_policyCreated() {
        when(mMockCarPropertyService
                .getPropertySafe(eq(VehiclePropertyIds.INFO_DRIVER_SEAT), anyInt()))
                .thenReturn(null);

        BluetoothDeviceConnectionPolicy policyUnderTest = BluetoothDeviceConnectionPolicy.create(
                mMockContext, mUserId, mMockBluetoothService);
        Assert.assertTrue(policyUnderTest != null);
    }

    /**
     * Tests the case where if {@link CarDrivingStateService#getCurrentDrivingState()} returns
     * null, {@link CarServicesHelper#isParked()} should not throw a NPE.
     *
     * Preconditions:
     * - {@link CarDrivingStateService#getCurrentDrivingState()} returns {@code null}.
     *
     * Action:
     * - Call {@link CarServicesHelper#isParked()}.
     *
     * Outcome:
     * - {@link CarServicesHelper#isParked()} returns {@code false}.
     */
    @Test
    public void testGetDrivingStateNull_noNpe() {
        when(mMockCarDrivingStateService.getCurrentDrivingState()).thenReturn(null);

        BluetoothDeviceConnectionPolicy.CarServicesHelper helperUnderTest =
                mPolicy.new CarServicesHelper();

        Assert.assertFalse(helperUnderTest.isParked());
    }
}
