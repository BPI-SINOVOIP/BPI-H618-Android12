/*
 * Copyright (C) 2017 The Android Open Source Project
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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.car.VehicleAreaSeat;
import android.car.VehicleAreaType;
import android.car.VehiclePropertyIds;
import android.car.VehicleSeatOccupancyState;
import android.car.drivingstate.CarDrivingStateEvent;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.car.hardware.power.ICarPowerPolicyListener;
import android.car.hardware.power.PowerComponent;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.CarPropertyManager;
import android.car.hardware.property.ICarPropertyEventListener;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.util.Log;
import android.util.Slog;

import com.android.car.power.CarPowerManagementService;
import com.android.internal.annotations.VisibleForTesting;

import java.io.PrintWriter;
import java.util.List;
import java.util.Objects;

/**
 * A Bluetooth Device Connection policy that is specific to the use cases of a Car. Contains policy
 * for deciding when to trigger connection and disconnection events.
 */
public class BluetoothDeviceConnectionPolicy {
    private static final String TAG = CarLog.tagFor(BluetoothDeviceConnectionPolicy.class);
    private static final boolean DBG = Log.isLoggable(TAG, Log.DEBUG);

    private final int mUserId;
    private final Context mContext;
    private final BluetoothAdapter mBluetoothAdapter;
    private final CarBluetoothService mCarBluetoothService;
    private final CarServicesHelper mCarHelper;
    private final UserManager mUserManager;

    private final ICarPowerPolicyListener mPowerPolicyListener =
            new ICarPowerPolicyListener.Stub() {
                @Override
                public void onPolicyChanged(CarPowerPolicy appliedPolicy,
                        CarPowerPolicy accumulatedPolicy) {
                    boolean isOn = accumulatedPolicy.isComponentEnabled(PowerComponent.BLUETOOTH);
                    if (!mUserManager.isUserUnlocked(mUserId)) {
                        logd("User " + mUserId + " is locked, ignoring bluetooth power change "
                                + (isOn ? "on" : "off"));
                        return;
                    }
                    if (isOn) {
                        if (isBluetoothPersistedOn()) {
                            enableBluetooth();
                        }
                        // The above isBluetoothPersistedOn() call is always true when the
                        // adapter is on, but can be true or false if the adapter is off. If we
                        // turned the adapter back on then this connectDevices() call would fail
                        // at first here but be caught by the following adapter on broadcast
                        // below. We'll only do this if the adapter is on.
                        if (mBluetoothAdapter.getState() == BluetoothAdapter.STATE_ON) {
                            connectDevices();
                        }
                    } else {
                        // we'll turn off Bluetooth to disconnect devices and better the "off"
                        // illusion
                        logd("Car power policy turns off bluetooth. Disable bluetooth adapter");
                        disableBluetooth();
                    }
                }
    };

    @VisibleForTesting
    public ICarPowerPolicyListener getPowerPolicyListener() {
        return mPowerPolicyListener;
    }

    /**
     * A BroadcastReceiver that listens specifically for actions related to the profile we're
     * tracking and uses them to update the status.
     *
     * On BluetoothAdapter.ACTION_STATE_CHANGED:
     *    If the adapter is going into the ON state, then commit trigger auto connection.
     */
    private class BluetoothBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1);
                logd("Bluetooth Adapter state changed: ", Utils.getAdapterStateName(state));
                if (state == BluetoothAdapter.STATE_ON) {
                    connectDevices();
                }
            }
        }
    }
    private final BluetoothBroadcastReceiver mBluetoothBroadcastReceiver;

    /**
     * A helper class to interact with the VHAL and the rest of the car.
     */
    final class CarServicesHelper {
        private final CarPropertyService mCarPropertyService;
        private final CarDrivingStateService mCarDrivingStateService;

        // Location of the driver's seat, e.g., left or right side.
        private final int mDriverSeat;

        CarServicesHelper() {
            mCarPropertyService = CarLocalServices.getService(CarPropertyService.class);
            if (mCarPropertyService == null) Slog.w(TAG, "Cannot find CarPropertyService");
            mDriverSeat = getDriverSeatLocationFromVhal();
            mCarDrivingStateService = CarLocalServices.getService(CarDrivingStateService.class);
            if (mCarDrivingStateService == null) Slog.w(TAG, "Cannot find mCarDrivingStateService");
        }

        /**
         * Set up vehicle event listeners. Remember to call {@link release()} when done.
         */
        public void init() {
            if (mCarPropertyService != null) {
                mCarPropertyService.registerListener(VehiclePropertyIds.SEAT_OCCUPANCY,
                        CarPropertyManager.SENSOR_RATE_ONCHANGE, mSeatOnOccupiedListener);
            }
        }

        public void release() {
            if (mCarPropertyService != null) {
                mCarPropertyService.unregisterListener(VehiclePropertyIds.SEAT_OCCUPANCY,
                        mSeatOnOccupiedListener);
            }
        }

        /**
         * A {@code ICarPropertyEventListener} that triggers the auto-connection process when
         * {@code SEAT_OCCUPANCY} is {@code OCCUPIED}.
         */
        private final ICarPropertyEventListener mSeatOnOccupiedListener =
                new ICarPropertyEventListener.Stub() {
                    @Override
                    public void onEvent(List<CarPropertyEvent> events) throws RemoteException {
                        for (CarPropertyEvent event : events) {
                            onSeatOccupancyCarPropertyEvent(event);
                        }
                    }
                };

        /**
         * Acts on {@link CarPropertyEvent} events marked with
         * {@link CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE} and marked with {@link
         * VehiclePropertyIds.SEAT_OCCUPANCY} by calling {@link connectDevices}.
         * <p>
         * Default implementation filters on driver's seat only, but can change to trigger on
         * any front row seat, or any seat in the car.
         * <p>
         * Default implementation also restricts this trigger to when the car is in the
         * parked state, to discourage drivers from exploiting to connect while driving, and to
         * also filter out spurious seat sensor signals while driving.
         * <p>
         * This method does nothing if the event parameter is {@code null}.
         *
         * @param event - The {@link CarPropertyEvent} to be handled.
         */
        private void onSeatOccupancyCarPropertyEvent(CarPropertyEvent event) {
            if ((event == null)
                    || (event.getEventType() != CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE)) {
                return;
            }
            CarPropertyValue value = event.getCarPropertyValue();
            logd("Car property changed: ", value.toString());
            if (mBluetoothAdapter.isEnabled()
                    && (value.getPropertyId() == VehiclePropertyIds.SEAT_OCCUPANCY)
                    && ((int) value.getValue() == VehicleSeatOccupancyState.OCCUPIED)
                    && (value.getAreaId() == mDriverSeat)
                    && isParked()) {
                connectDevices();
            }
        }

        /**
         * Gets the location of the driver's seat (e.g., front-left, front-right) from the VHAL.
         * <p>
         * Default implementation sets the driver's seat to front-left if mCarPropertyService is
         * not found.
         * <p>
         * Note, comments for {@link CarPropertyManager#getIntProperty(int, int)} indicate it may
         * take a couple of seconds to complete, whereas there are no such comments for
         * {@link CarPropertyService#getPropertySafe(int, int)}, but we assume there is also similar
         * latency in querying VHAL properties.
         *
         * @return An {@code int} representing driver's seat location.
         */
        private int getDriverSeatLocationFromVhal() {
            int defaultLocation = VehicleAreaSeat.SEAT_ROW_1_LEFT;

            if (mCarPropertyService == null) {
                return defaultLocation;
            }
            CarPropertyValue value = mCarPropertyService.getPropertySafe(
                    VehiclePropertyIds.INFO_DRIVER_SEAT, VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL);
            if (value == null) {
                // Distinguish between two possible causes for null, based on
                // {@code mConfigs.get(prop)} in {@link CarPropertyService#getProperty} and
                // {@link CarPropertyService#getPropertyConfigList}
                List<CarPropertyConfig> availableProp = mCarPropertyService.getPropertyConfigList(
                        new int[] {VehiclePropertyIds.INFO_DRIVER_SEAT});
                if (availableProp.isEmpty() || availableProp.get(0) == null) {
                    logd("Driver seat location property is not in config list.");
                } else {
                    logd("Driver seat location property is not ready yet.");
                }
                return defaultLocation;
            }
            return (int) value.getValue();
        }

        public int getDriverSeatLocation() {
            return mDriverSeat;
        }

        /**
         * Returns {@code true} if the car is in parked gear.
         * <p>
         * We are being conservative and only want to trigger when car is in parked state. Extending
         * this conservative approach, we default return false if {@code mCarDrivingStateService}
         * is not found, or if we otherwise can't get the value.
         */
        public boolean isParked() {
            if (mCarDrivingStateService == null) {
                return false;
            }
            CarDrivingStateEvent event = mCarDrivingStateService.getCurrentDrivingState();
            if (event == null) {
                return false;
            }
            return event.eventValue == CarDrivingStateEvent.DRIVING_STATE_PARKED;
        }
    }

    /**
     * Create a new BluetoothDeviceConnectionPolicy object, responsible for encapsulating the
     * default policy for when to initiate device connections given the list of prioritized devices
     * for each profile.
     *
     * @param context - The context of the creating application
     * @param userId - The user ID we're operating as
     * @param bluetoothService - A reference to CarBluetoothService so we can connect devices
     * @return A new instance of a BluetoothProfileDeviceManager, or null on any error
     */
    public static BluetoothDeviceConnectionPolicy create(Context context, int userId,
            CarBluetoothService bluetoothService) {
        try {
            return new BluetoothDeviceConnectionPolicy(context, userId, bluetoothService);
        } catch (NullPointerException e) {
            return null;
        }
    }

    /**
     * Create a new BluetoothDeviceConnectionPolicy object, responsible for encapsulating the
     * default policy for when to initiate device connections given the list of prioritized devices
     * for each profile.
     *
     * @param context - The context of the creating application
     * @param userId - The user ID we're operating as
     * @param bluetoothService - A reference to CarBluetoothService so we can connect devices
     * @return A new instance of a BluetoothProfileDeviceManager
     */
    private BluetoothDeviceConnectionPolicy(Context context, int userId,
            CarBluetoothService bluetoothService) {
        mUserId = userId;
        mContext = Objects.requireNonNull(context);
        mCarBluetoothService = bluetoothService;
        mBluetoothBroadcastReceiver = new BluetoothBroadcastReceiver();
        mBluetoothAdapter = Objects.requireNonNull(BluetoothAdapter.getDefaultAdapter());
        mCarHelper = new CarServicesHelper();
        mUserManager = mContext.getSystemService(UserManager.class);
    }

    /**
     * Setup the Bluetooth profile service connections and Vehicle Event listeners.
     * and start the state machine -{@link BluetoothAutoConnectStateMachine}
     */
    public void init() {
        logd("init()");
        IntentFilter profileFilter = new IntentFilter();
        profileFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        mContext.registerReceiverAsUser(mBluetoothBroadcastReceiver, UserHandle.CURRENT,
                profileFilter, null, null);
        CarPowerManagementService cpms = CarLocalServices.getService(
                CarPowerManagementService.class);
        if (cpms != null) {
            CarPowerPolicyFilter filter = new CarPowerPolicyFilter.Builder()
                    .setComponents(PowerComponent.BLUETOOTH).build();
            cpms.addPowerPolicyListener(filter, mPowerPolicyListener);
        } else {
            Slog.w(TAG, "Cannot find CarPowerManagementService");
        }
        mCarHelper.init();

        // Since we do this only on start up and on user switch, it's safe to kick off a connect on
        // init. If we have a connect in progress, this won't hurt anything. If we already have
        // devices connected, this will add on top of it. We _could_ enter this from a crash
        // recovery, but that would at worst cause more devices to connect and wouldn't change the
        // existing devices.
        if (mBluetoothAdapter.getState() == BluetoothAdapter.STATE_ON) {
            // CarPowerManager doesn't provide a getState() or that would go here too.
            connectDevices();
        }
    }

    /**
     * Clean up slate. Close the Bluetooth profile service connections and quit the state machine -
     * {@link BluetoothAutoConnectStateMachine}
     */
    public void release() {
        logd("release()");
        CarPowerManagementService cpms =
                CarLocalServices.getService(CarPowerManagementService.class);
        if (cpms != null) {
            cpms.removePowerPolicyListener(mPowerPolicyListener);
        }
        if (mBluetoothBroadcastReceiver != null) {
            mContext.unregisterReceiver(mBluetoothBroadcastReceiver);
        }
        mCarHelper.release();
    }

    /**
     * Tell each Profile device manager that its time to begin auto connecting devices
     */
    public void connectDevices() {
        logd("Connect devices for each profile");
        mCarBluetoothService.connectDevices();
    }

    /**
     * Get the persisted Bluetooth state from Settings
     *
     * @return True if the persisted Bluetooth state is on, false otherwise
     */
    private boolean isBluetoothPersistedOn() {
        return (Settings.Global.getInt(
                mContext.getContentResolver(), Settings.Global.BLUETOOTH_ON, -1) != 0);
    }

    /**
     * Turn on the Bluetooth Adapter.
     */
    private void enableBluetooth() {
        logd("Enable bluetooth adapter");
        if (mBluetoothAdapter == null) {
            Slog.e(TAG, "Cannot enable Bluetooth adapter. The object is null.");
            return;
        }
        mBluetoothAdapter.enable();
    }

    /**
     * Turn off the Bluetooth Adapter.
     *
     * Tells BluetoothAdapter to shut down _without_ persisting the off state as the desired state
     * of the Bluetooth adapter for next start up.
     */
    private void disableBluetooth() {
        logd("Disable bluetooth, do not persist state across reboot");
        if (mBluetoothAdapter == null) {
            Slog.e(TAG, "Cannot disable Bluetooth adapter. The object is null.");
            return;
        }
        mBluetoothAdapter.disable(false);
    }

    /**
     * Print the verbose status of the object
     */
    public void dump(PrintWriter writer, String indent) {
        writer.println(indent + TAG + ":");
        writer.println(indent + "\tUserId: " + mUserId);
    }

    /**
     * Print to debug if debug is enabled
     */
    private static void logd(String... msgParts) {
        if (DBG) {
            Slog.d(TAG, String.join(" ", msgParts));
        }
    }
}
