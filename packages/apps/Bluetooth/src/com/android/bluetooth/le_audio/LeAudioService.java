/*
 * Copyright 2020 HIMSA II K/S - www.himsa.com.
 * Represented by EHIMA - www.ehima.com
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

package com.android.bluetooth.le_audio;

import static android.bluetooth.IBluetoothLeAudio.LE_AUDIO_GROUP_ID_INVALID;

import android.annotation.RequiresPermission;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothLeAudio;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.IBluetoothLeAudio;
import android.content.Attributable;
import android.content.AttributionSource;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.HandlerThread;
import android.os.ParcelUuid;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.ProfileService;
import com.android.bluetooth.btservice.storage.DatabaseManager;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.ArrayUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Provides Bluetooth LeAudio profile, as a service in the Bluetooth application.
 * @hide
 */
public class LeAudioService extends ProfileService {
    private static final boolean DBG = true;
    private static final String TAG = "LeAudioService";

    // Upper limit of all LeAudio devices: Bonded or Connected
    private static final int MAX_LE_AUDIO_STATE_MACHINES = 10;
    private static LeAudioService sLeAudioService;

    private AdapterService mAdapterService;
    private DatabaseManager mDatabaseManager;
    private HandlerThread mStateMachinesThread;
    private BluetoothDevice mPreviousAudioDevice;

    LeAudioNativeInterface mLeAudioNativeInterface;
    AudioManager mAudioManager;

    private final Map<BluetoothDevice, LeAudioStateMachine> mStateMachines = new HashMap<>();

    private final Map<BluetoothDevice, Integer> mDeviceGroupIdMap = new ConcurrentHashMap<>();
    private final Map<Integer, Boolean> mGroupIdConnectedMap = new HashMap<>();
    private int mActiveDeviceGroupId = LE_AUDIO_GROUP_ID_INVALID;

    private BroadcastReceiver mBondStateChangedReceiver;
    private BroadcastReceiver mConnectionStateChangedReceiver;

    @Override
    protected IProfileServiceBinder initBinder() {
        return new BluetoothLeAudioBinder(this);
    }

    @Override
    protected void create() {
        Log.i(TAG, "create()");
    }

    @Override
    protected boolean start() {
        Log.i(TAG, "start()");
        if (sLeAudioService != null) {
            throw new IllegalStateException("start() called twice");
        }

        mAdapterService = Objects.requireNonNull(AdapterService.getAdapterService(),
                "AdapterService cannot be null when LeAudioService starts");
        mLeAudioNativeInterface = Objects.requireNonNull(LeAudioNativeInterface.getInstance(),
                "LeAudioNativeInterface cannot be null when LeAudioService starts");
        mDatabaseManager = Objects.requireNonNull(mAdapterService.getDatabase(),
                "DatabaseManager cannot be null when A2dpService starts");
        mAudioManager = getSystemService(AudioManager.class);
        Objects.requireNonNull(mAudioManager,
                "AudioManager cannot be null when LeAudioService starts");

        // Start handler thread for state machines
        mStateMachines.clear();
        mStateMachinesThread = new HandlerThread("LeAudioService.StateMachines");
        mStateMachinesThread.start();

        mDeviceGroupIdMap.clear();
        mGroupIdConnectedMap.clear();

        // Setup broadcast receivers
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        mBondStateChangedReceiver = new BondStateChangedReceiver();
        registerReceiver(mBondStateChangedReceiver, filter);
        filter = new IntentFilter();
        filter.addAction(BluetoothLeAudio.ACTION_LE_AUDIO_CONNECTION_STATE_CHANGED);
        mConnectionStateChangedReceiver = new ConnectionStateChangedReceiver();
        registerReceiver(mConnectionStateChangedReceiver, filter);

        // Mark service as started
        setLeAudioService(this);

        mLeAudioNativeInterface.init();

        return true;
    }

    @Override
    protected boolean stop() {
        Log.i(TAG, "stop()");
        if (sLeAudioService == null) {
            Log.w(TAG, "stop() called before start()");
            return true;
        }

        // Cleanup native interfaces
        mLeAudioNativeInterface.cleanup();
        mLeAudioNativeInterface = null;

        // Set the service and BLE devices as inactive
        setLeAudioService(null);

        // Unregister broadcast receivers
        unregisterReceiver(mBondStateChangedReceiver);
        mBondStateChangedReceiver = null;
        unregisterReceiver(mConnectionStateChangedReceiver);
        mConnectionStateChangedReceiver = null;

        // Destroy state machines and stop handler thread
        synchronized (mStateMachines) {
            for (LeAudioStateMachine sm : mStateMachines.values()) {
                sm.doQuit();
                sm.cleanup();
            }
            mStateMachines.clear();
        }

        mDeviceGroupIdMap.clear();
        mGroupIdConnectedMap.clear();

        if (mStateMachinesThread != null) {
            mStateMachinesThread.quitSafely();
            mStateMachinesThread = null;
        }

        mAudioManager = null;
        mAdapterService = null;
        return true;
    }

    @Override
    protected void cleanup() {
        Log.i(TAG, "cleanup()");
    }

    public static synchronized LeAudioService getLeAudioService() {
        if (sLeAudioService == null) {
            Log.w(TAG, "getLeAudioService(): service is NULL");
            return null;
        }
        if (!sLeAudioService.isAvailable()) {
            Log.w(TAG, "getLeAudioService(): service is not available");
            return null;
        }
        return sLeAudioService;
    }

    private static synchronized void setLeAudioService(LeAudioService instance) {
        if (DBG) {
            Log.d(TAG, "setLeAudioService(): set to: " + instance);
        }
        sLeAudioService = instance;
    }

    public boolean connect(BluetoothDevice device) {
        if (DBG) {
            Log.d(TAG, "connect(): " + device);
        }

        if (getConnectionPolicy(device) == BluetoothProfile.CONNECTION_POLICY_FORBIDDEN) {
            Log.e(TAG, "Cannot connect to " + device + " : CONNECTION_POLICY_FORBIDDEN");
            return false;
        }
        ParcelUuid[] featureUuids = mAdapterService.getRemoteUuids(device);
        if (!ArrayUtils.contains(featureUuids, BluetoothUuid.LE_AUDIO)) {
            Log.e(TAG, "Cannot connect to " + device + " : Remote does not have LE_AUDIO UUID");
            return false;
        }

        int groupId = getGroupId(device);

        //TODO: disconnect active device if it's not in groupId

        if (DBG) {
            Log.d(TAG, "connect(): " + device + "group id: " + groupId);
        }

        synchronized (mStateMachines) {
            LeAudioStateMachine sm = getOrCreateStateMachine(device);
            if (sm == null) {
                Log.e(TAG, "Ignored connect request for " + device + " : no state machine");
                return false;
            }
            sm.sendMessage(LeAudioStateMachine.CONNECT, groupId);
        }

        // Connect other devices from this group
        if (groupId != LE_AUDIO_GROUP_ID_INVALID) {
            for (BluetoothDevice storedDevice : mDeviceGroupIdMap.keySet()) {
                if (device.equals(storedDevice)) {
                    continue;
                }
                if (getGroupId(storedDevice) != groupId) {
                    continue;
                }
                synchronized (mStateMachines) {
                    LeAudioStateMachine sm = getOrCreateStateMachine(storedDevice);
                    if (sm == null) {
                        Log.e(TAG, "Ignored connect request for " + storedDevice
                                + " : no state machine");
                        continue;
                    }
                    sm.sendMessage(LeAudioStateMachine.CONNECT, groupId);
                }
            }
        }
        return true;
    }

    /**
     * Disconnects LE Audio for the remote bluetooth device
     *
     * @param device is the device with which we would like to disconnect LE Audio
     * @return true if profile disconnected, false if device not connected over LE Audio
     */
    public boolean disconnect(BluetoothDevice device) {
        if (DBG) {
            Log.d(TAG, "disconnect(): " + device);
        }

        // Disconnect this device
        synchronized (mStateMachines) {
            LeAudioStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                Log.e(TAG, "Ignored disconnect request for " + device
                        + " : no state machine");
                return false;
            }
            sm.sendMessage(LeAudioStateMachine.DISCONNECT);
        }

        // Disconnect other devices from this group
        int groupId = getGroupId(device);
        if (groupId != LE_AUDIO_GROUP_ID_INVALID) {
            for (BluetoothDevice storedDevice : mDeviceGroupIdMap.keySet()) {
                if (device.equals(storedDevice)) {
                    continue;
                }
                if (getGroupId(storedDevice) != groupId) {
                    continue;
                }
                synchronized (mStateMachines) {
                    LeAudioStateMachine sm = mStateMachines.get(storedDevice);
                    if (sm == null) {
                        Log.e(TAG, "Ignored disconnect request for " + storedDevice
                                + " : no state machine");
                        continue;
                    }
                    sm.sendMessage(LeAudioStateMachine.DISCONNECT);
                }
            }
        }
        return true;
    }

    List<BluetoothDevice> getConnectedDevices() {
        synchronized (mStateMachines) {
            List<BluetoothDevice> devices = new ArrayList<>();
            for (LeAudioStateMachine sm : mStateMachines.values()) {
                if (sm.isConnected()) {
                    devices.add(sm.getDevice());
                }
            }
            return devices;
        }
    }

    List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
        ArrayList<BluetoothDevice> devices = new ArrayList<>();
        if (states == null) {
            return devices;
        }
        final BluetoothDevice[] bondedDevices = mAdapterService.getBondedDevices();
        if (bondedDevices == null) {
            return devices;
        }
        synchronized (mStateMachines) {
            for (BluetoothDevice device : bondedDevices) {
                final ParcelUuid[] featureUuids = device.getUuids();
                if (!ArrayUtils.contains(featureUuids, BluetoothUuid.LE_AUDIO)) {
                    continue;
                }
                int connectionState = BluetoothProfile.STATE_DISCONNECTED;
                LeAudioStateMachine sm = mStateMachines.get(device);
                if (sm != null) {
                    connectionState = sm.getConnectionState();
                }
                for (int state : states) {
                    if (connectionState == state) {
                        devices.add(device);
                        break;
                    }
                }
            }
            return devices;
        }
    }

    /**
     * Get the list of devices that have state machines.
     *
     * @return the list of devices that have state machines
     */
    @VisibleForTesting
    List<BluetoothDevice> getDevices() {
        List<BluetoothDevice> devices = new ArrayList<>();
        synchronized (mStateMachines) {
            for (LeAudioStateMachine sm : mStateMachines.values()) {
                devices.add(sm.getDevice());
            }
            return devices;
        }
    }

    /**
     * Get the current connection state of the profile
     *
     * @param device is the remote bluetooth device
     * @return {@link BluetoothProfile#STATE_DISCONNECTED} if this profile is disconnected,
     * {@link BluetoothProfile#STATE_CONNECTING} if this profile is being connected,
     * {@link BluetoothProfile#STATE_CONNECTED} if this profile is connected, or
     * {@link BluetoothProfile#STATE_DISCONNECTING} if this profile is being disconnected
     */
    public int getConnectionState(BluetoothDevice device) {
        synchronized (mStateMachines) {
            LeAudioStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
            return sm.getConnectionState();
        }
    }

    /**
     * Set the active device.
     *
     * @param device the new active device
     * @return true on success, otherwise false
     */
    public boolean setActiveDevice(BluetoothDevice device) {
        if (DBG) {
            Log.d(TAG, "setActiveDevice:" + device);
        }

        return false;
    }

    /**
     * Get the connected physical LeAudio devices that are active.
     *
     * @return the list of active devices.
     */
    List<BluetoothDevice> getActiveDevices() {
        if (DBG) {
            Log.d(TAG, "getActiveDevices");
        }
        ArrayList<BluetoothDevice> activeDevices = new ArrayList<>();
        return activeDevices;
    }

    // Suppressed since this is part of a local process
    @SuppressLint("AndroidFrameworkRequiresPermission")
    void messageFromNative(LeAudioStackEvent stackEvent) {
        Log.d(TAG, "Message from native: " + stackEvent);
        BluetoothDevice device = stackEvent.device;

        if (stackEvent.type == LeAudioStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED) {
        // Some events require device state machine
            synchronized (mStateMachines) {
                LeAudioStateMachine sm = mStateMachines.get(device);
                if (sm == null) {
                    switch (stackEvent.valueInt1) {
                        case LeAudioStackEvent.CONNECTION_STATE_CONNECTED:
                        case LeAudioStackEvent.CONNECTION_STATE_CONNECTING:
                            sm = getOrCreateStateMachine(device);
                            break;
                        default:
                            break;
                    }
                }

                if (sm == null) {
                    Log.e(TAG, "Cannot process stack event: no state machine: " + stackEvent);
                    return;
                }

                sm.sendMessage(LeAudioStateMachine.STACK_EVENT, stackEvent);
                return;
            }
        }

        // Some events do not require device state machine
        if (stackEvent.type == LeAudioStackEvent.EVENT_TYPE_GROUP_STATUS_CHANGED) {
            int group_id = stackEvent.valueInt1;
            int group_status = stackEvent.valueInt2;
            int group_flags = stackEvent.valueInt3;

            // TODO: Handle Stream events
            switch (group_status) {
                case LeAudioStackEvent.GROUP_STATUS_IDLE:
                case LeAudioStackEvent.GROUP_STATUS_RECONFIGURED:
                case LeAudioStackEvent.GROUP_STATUS_DESTROYED:
                case LeAudioStackEvent.GROUP_STATUS_SUSPENDED:
                    setActiveDevice(null);
                    // TODO: Get all devices with a group and Unassign? or they are already unasigned
                    // This event may come after removing all the nodes from a certain group but only that.
                    break;
                case LeAudioStackEvent.GROUP_STATUS_STREAMING:
                    BluetoothDevice streaming_device = getConnectedPeerDevices(group_id).get(0);
                    setActiveDevice(streaming_device);
                    break;
                default:
                    break;
            }

        }
    }

    private LeAudioStateMachine getOrCreateStateMachine(BluetoothDevice device) {
        if (device == null) {
            Log.e(TAG, "getOrCreateStateMachine failed: device cannot be null");
            return null;
        }
        synchronized (mStateMachines) {
            LeAudioStateMachine sm = mStateMachines.get(device);
            if (sm != null) {
                return sm;
            }
            // Limit the maximum number of state machines to avoid DoS attack
            if (mStateMachines.size() >= MAX_LE_AUDIO_STATE_MACHINES) {
                Log.e(TAG, "Maximum number of LeAudio state machines reached: "
                        + MAX_LE_AUDIO_STATE_MACHINES);
                return null;
            }
            if (DBG) {
                Log.d(TAG, "Creating a new state machine for " + device);
            }
            sm = LeAudioStateMachine.make(device, this,
                    mLeAudioNativeInterface, mStateMachinesThread.getLooper());
            mStateMachines.put(device, sm);
            return sm;
        }
    }

    // Remove state machine if the bonding for a device is removed
    private class BondStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction())) {
                return;
            }
            int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,
                                           BluetoothDevice.ERROR);
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            Objects.requireNonNull(device, "ACTION_BOND_STATE_CHANGED with no EXTRA_DEVICE");
            bondStateChanged(device, state);
        }
    }

    /**
     * Process a change in the bonding state for a device.
     *
     * @param device the device whose bonding state has changed
     * @param bondState the new bond state for the device. Possible values are:
     * {@link BluetoothDevice#BOND_NONE},
     * {@link BluetoothDevice#BOND_BONDING},
     * {@link BluetoothDevice#BOND_BONDED}.
     */
    @VisibleForTesting
    void bondStateChanged(BluetoothDevice device, int bondState) {
        if (DBG) {
            Log.d(TAG, "Bond state changed for device: " + device + " state: " + bondState);
        }
        // Remove state machine if the bonding for a device is removed
        if (bondState != BluetoothDevice.BOND_NONE) {
            return;
        }
        mDeviceGroupIdMap.remove(device);
        synchronized (mStateMachines) {
            LeAudioStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                return;
            }
            if (sm.getConnectionState() != BluetoothProfile.STATE_DISCONNECTED) {
                return;
            }
            removeStateMachine(device);
        }
    }

    private void removeStateMachine(BluetoothDevice device) {
        synchronized (mStateMachines) {
            LeAudioStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                Log.w(TAG, "removeStateMachine: device " + device
                        + " does not have a state machine");
                return;
            }
            Log.i(TAG, "removeStateMachine: removing state machine for device: " + device);
            sm.doQuit();
            sm.cleanup();
            mStateMachines.remove(device);
        }
    }

    private List<BluetoothDevice> getConnectedPeerDevices(int groupId) {
        List<BluetoothDevice> result = new ArrayList<>();
        for (BluetoothDevice peerDevice : getConnectedDevices()) {
            if (getGroupId(peerDevice) == groupId) {
                result.add(peerDevice);
            }
        }
        return result;
    }

    @VisibleForTesting
    synchronized void connectionStateChanged(BluetoothDevice device, int fromState,
                                                     int toState) {
        if ((device == null) || (fromState == toState)) {
            Log.e(TAG, "connectionStateChanged: unexpected invocation. device=" + device
                    + " fromState=" + fromState + " toState=" + toState);
            return;
        }
        if (toState == BluetoothProfile.STATE_CONNECTED) {
            int myGroupId = getGroupId(device);
            if (myGroupId == LE_AUDIO_GROUP_ID_INVALID
                    || getConnectedPeerDevices(myGroupId).size() == 1) {
                // Log LE Audio connection event if we are the first device in a set
                // Or when the GroupId has not been found
                // MetricsLogger.logProfileConnectionEvent(
                //         BluetoothMetricsProto.ProfileId.LE_AUDIO);
            }
            if (!mGroupIdConnectedMap.getOrDefault(myGroupId, false)) {
                mGroupIdConnectedMap.put(myGroupId, true);
            }
        }
        if (fromState == BluetoothProfile.STATE_CONNECTED && getConnectedDevices().isEmpty()) {
            setActiveDevice(null);
            int myGroupId = getGroupId(device);
            mGroupIdConnectedMap.put(myGroupId, false);
        }
        // Check if the device is disconnected - if unbond, remove the state machine
        if (toState == BluetoothProfile.STATE_DISCONNECTED) {
            int bondState = mAdapterService.getBondState(device);
            if (bondState == BluetoothDevice.BOND_NONE) {
                if (DBG) {
                    Log.d(TAG, device + " is unbond. Remove state machine");
                }
                removeStateMachine(device);
            }
        }
    }

    private class ConnectionStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!BluetoothLeAudio.ACTION_LE_AUDIO_CONNECTION_STATE_CHANGED.equals(intent.getAction())) {
                return;
            }
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            int toState = intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1);
            int fromState = intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1);
            connectionStateChanged(device, fromState, toState);
        }
    }

   /**
     * Check whether can connect to a peer device.
     * The check considers a number of factors during the evaluation.
     *
     * @param device the peer device to connect to
     * @return true if connection is allowed, otherwise false
     */
    public boolean okToConnect(BluetoothDevice device) {
        // Check if this is an incoming connection in Quiet mode.
        if (mAdapterService.isQuietModeEnabled()) {
            Log.e(TAG, "okToConnect: cannot connect to " + device + " : quiet mode enabled");
            return false;
        }
        // Check connectionPolicy and accept or reject the connection.
        int connectionPolicy = getConnectionPolicy(device);
        int bondState = mAdapterService.getBondState(device);
        // Allow this connection only if the device is bonded. Any attempt to connect while
        // bonding would potentially lead to an unauthorized connection.
        if (bondState != BluetoothDevice.BOND_BONDED) {
            Log.w(TAG, "okToConnect: return false, bondState=" + bondState);
            return false;
        } else if (connectionPolicy != BluetoothProfile.CONNECTION_POLICY_UNKNOWN
                && connectionPolicy != BluetoothProfile.CONNECTION_POLICY_ALLOWED) {
            // Otherwise, reject the connection if connectionPolicy is not valid.
            Log.w(TAG, "okToConnect: return false, connectionPolicy=" + connectionPolicy);
            return false;
        }
        return true;
    }

    /**
     * Set connection policy of the profile and connects it if connectionPolicy is
     * {@link BluetoothProfile#CONNECTION_POLICY_ALLOWED} or disconnects if connectionPolicy is
     * {@link BluetoothProfile#CONNECTION_POLICY_FORBIDDEN}
     *
     * <p> The device should already be paired.
     * Connection policy can be one of:
     * {@link BluetoothProfile#CONNECTION_POLICY_ALLOWED},
     * {@link BluetoothProfile#CONNECTION_POLICY_FORBIDDEN},
     * {@link BluetoothProfile#CONNECTION_POLICY_UNKNOWN}
     *
     * @param device the remote device
     * @param connectionPolicy is the connection policy to set to for this profile
     * @return true on success, otherwise false
     */
    @RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)
    public boolean setConnectionPolicy(BluetoothDevice device, int connectionPolicy) {
        enforceCallingOrSelfPermission(BLUETOOTH_PRIVILEGED,
                "Need BLUETOOTH_PRIVILEGED permission");
        if (DBG) {
            Log.d(TAG, "Saved connectionPolicy " + device + " = " + connectionPolicy);
        }

        if (!mDatabaseManager.setProfileConnectionPolicy(device, BluetoothProfile.LE_AUDIO,
                  connectionPolicy)) {
            return false;
        }
        if (connectionPolicy == BluetoothProfile.CONNECTION_POLICY_ALLOWED) {
            connect(device);
        } else if (connectionPolicy == BluetoothProfile.CONNECTION_POLICY_FORBIDDEN) {
            disconnect(device);
        }
        return true;
    }

    /**
     * Get the connection policy of the profile.
     *
     * <p> The connection policy can be any of:
     * {@link BluetoothProfile#CONNECTION_POLICY_ALLOWED},
     * {@link BluetoothProfile#CONNECTION_POLICY_FORBIDDEN},
     * {@link BluetoothProfile#CONNECTION_POLICY_UNKNOWN}
     *
     * @param device Bluetooth device
     * @return connection policy of the device
     * @hide
     */
    public int getConnectionPolicy(BluetoothDevice device) {
        return mDatabaseManager
                .getProfileConnectionPolicy(device, BluetoothProfile.LE_AUDIO);
    }

    /**
     * Get device group id. Devices with same group id belong to same group (i.e left and right
     * earbud)
     * @param device LE Audio capable device
     * @return group id that this device currently belongs to
     */
    public int getGroupId(BluetoothDevice device) {
        if (device == null) {
            return LE_AUDIO_GROUP_ID_INVALID;
        }
        //TODO: implement
        return LE_AUDIO_GROUP_ID_INVALID;
    }

    /**
     * Binder object: must be a static class or memory leak may occur
     */
    @VisibleForTesting
    static class BluetoothLeAudioBinder extends IBluetoothLeAudio.Stub
            implements IProfileServiceBinder {
        private LeAudioService mService;

        @RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)
        private LeAudioService getService(AttributionSource source) {
            if (!Utils.checkCallerIsSystemOrActiveUser(TAG)
                    || !Utils.checkServiceAvailable(mService, TAG)
                    || !Utils.checkConnectPermissionForDataDelivery(mService, source, TAG)) {
                return null;
            }
            return mService;
        }

        BluetoothLeAudioBinder(LeAudioService svc) {
            mService = svc;
        }

        @Override
        public void cleanup() {
            mService = null;
        }

        @Override
        public boolean connect(BluetoothDevice device, AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return false;
            }
            return service.connect(device);
        }

        @Override
        public boolean disconnect(BluetoothDevice device, AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return false;
            }
            return service.disconnect(device);
        }

        @Override
        public List<BluetoothDevice> getConnectedDevices(AttributionSource source) {
            LeAudioService service = getService(source);
            if (service == null) {
                return new ArrayList<>(0);
            }
            return service.getConnectedDevices();
        }

        @Override
        public List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states,
                AttributionSource source) {
            LeAudioService service = getService(source);
            if (service == null) {
                return new ArrayList<>(0);
            }
            return service.getDevicesMatchingConnectionStates(states);
        }

        @Override
        public int getConnectionState(BluetoothDevice device, AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
            return service.getConnectionState(device);
        }

        @Override
        public boolean setActiveDevice(BluetoothDevice device, AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return false;
            }
            return service.setActiveDevice(device);
        }

        @Override
        public List<BluetoothDevice> getActiveDevices(AttributionSource source) {
            LeAudioService service = getService(source);
            if (service == null) {
                return new ArrayList<>();
            }
            return service.getActiveDevices();
        }

        @Override
        public boolean setConnectionPolicy(BluetoothDevice device, int connectionPolicy,
                AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return false;
            }
            return service.setConnectionPolicy(device, connectionPolicy);
        }

        @Override
        public int getConnectionPolicy(BluetoothDevice device, AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return BluetoothProfile.CONNECTION_POLICY_UNKNOWN;
            }
            return service.getConnectionPolicy(device);
        }

        @Override
        public int getGroupId(BluetoothDevice device, AttributionSource source) {
            Attributable.setAttributionSource(device, source);
            LeAudioService service = getService(source);
            if (service == null) {
                return LE_AUDIO_GROUP_ID_INVALID;
            }

            return service.getGroupId(device);
        }
    }

    @Override
    public void dump(StringBuilder sb) {
        super.dump(sb);
        // TODO: Dump all state machines
    }
}
