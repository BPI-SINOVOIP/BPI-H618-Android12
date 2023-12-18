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

/*
 * Defines the native interface that is used by state machine/service to
 * send or receive messages from the native stack. This file is registered
 * for the native methods in the corresponding JNI C++ file.
 */
package com.android.bluetooth.le_audio;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.internal.annotations.GuardedBy;

/**
 * LeAudio Native Interface to/from JNI.
 */
public class LeAudioNativeInterface {
    private static final String TAG = "LeAudioNativeInterface";
    private static final boolean DBG = true;
    private BluetoothAdapter mAdapter;

    @GuardedBy("INSTANCE_LOCK")
    private static LeAudioNativeInterface sInstance;
    private static final Object INSTANCE_LOCK = new Object();

    static {
        classInitNative();
    }

    private LeAudioNativeInterface() {
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mAdapter == null) {
            Log.wtfStack(TAG, "No Bluetooth Adapter Available");
        }
    }

    /**
     * Get singleton instance.
     */
    public static LeAudioNativeInterface getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sInstance == null) {
                sInstance = new LeAudioNativeInterface();
            }
            return sInstance;
        }
    }

    private byte[] getByteAddress(BluetoothDevice device) {
        if (device == null) {
            return Utils.getBytesFromAddress("00:00:00:00:00:00");
        }
        return Utils.getBytesFromAddress(device.getAddress());
    }

    private void sendMessageToService(LeAudioStackEvent event) {
        LeAudioService service = LeAudioService.getLeAudioService();
        if (service != null) {
            service.messageFromNative(event);
        } else {
            Log.e(TAG, "Event ignored, service not available: " + event);
        }
    }

    private BluetoothDevice getDevice(byte[] address) {
        return mAdapter.getRemoteDevice(address);
    }

    // Callbacks from the native stack back into the Java framework.
    // All callbacks are routed via the Service which will disambiguate which
    // state machine the message should be routed to.
    private void onConnectionStateChanged(int state, byte[] address) {
        LeAudioStackEvent event =
                new LeAudioStackEvent(LeAudioStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        event.device = getDevice(address);
        event.valueInt1 = state;

        if (DBG) {
            Log.d(TAG, "onConnectionStateChanged: " + event);
        }
        sendMessageToService(event);
    }

    // Callbacks from the native stack back into the Java framework.
    // All callbacks are routed via the Service which will disambiguate which
    // state machine the message should be routed to.
    private void onSetMemberAvailable(byte[] address, int groupId) {
        LeAudioStackEvent event =
                new LeAudioStackEvent(LeAudioStackEvent.EVENT_TYPE_SET_MEMBER_AVAILABLE);
        event.device = getDevice(address);
        event.valueInt1 = groupId;
        if (DBG) {
            Log.d(TAG, "onSetMemberAvailable: " + event);
        }
        sendMessageToService(event);
    }

    private void onGroupStatus(int groupId, int groupStatus, int groupFlags) {
        LeAudioStackEvent event =
                new LeAudioStackEvent(LeAudioStackEvent.EVENT_TYPE_GROUP_STATUS_CHANGED);
        event.valueInt1 = groupId;
        event.valueInt2 = groupStatus;
        event.valueInt3 = groupFlags;
        event.device = null;

        if (DBG) {
            Log.d(TAG, "onGroupStatus: " + event);
        }
        sendMessageToService(event);
    }

    private void onAudioConf(int direction, int groupId, int sinkAudioLocation,
                             int sourceAudioLocation, byte[] address) {
        LeAudioStackEvent event =
                new LeAudioStackEvent(LeAudioStackEvent.EVENT_TYPE_AUDIO_CONF_CHANGED);
        event.valueInt1 = direction;
        event.valueInt2 = groupId;
        event.valueInt3 = sinkAudioLocation;
        event.valueInt4 = sourceAudioLocation;
        event.device = getDevice(address);

        if (DBG) {
            Log.d(TAG, "onAudioConf: " + event);
        }
        sendMessageToService(event);
    }

    /**
     * Initializes the native interface.
     *
     * priorities to configure.
     */
    public void init() {
        initNative();
    }

    /**
     * Cleanup the native interface.
     */
    public void cleanup() {
        cleanupNative();
    }

    /**
     * Initiates LeAudio connection to a remote device.
     *
     * @param device the remote device
     * @return true on success, otherwise false.
     */
    public boolean connectLeAudio(BluetoothDevice device) {
        return connectLeAudioNative(getByteAddress(device));
    }

    /**
     * Disconnects LeAudio from a remote device.
     *
     * @param device the remote device
     * @return true on success, otherwise false.
     */
    public boolean disconnectLeAudio(BluetoothDevice device) {
        return disconnectLeAudioNative(getByteAddress(device));
    }

    /**
     * Enable content streaming.
     * @param groupId group identifier
     * @param contentType type of content to stream
     */
    public void groupStream(int groupId, int contentType) {
        groupStreamNative(groupId, contentType);
    }

    /**
     * Suspend content streaming.
     * @param groupId  group identifier
     */
    public void groupSuspend(int groupId) {
        groupSuspendNative(groupId);
    }

    /**
     * Stop all content streaming.
     * @param groupId  group identifier
     * TODO: Maybe we should use also pass the content type argument
     */
    public void groupStop(int groupId) {
        groupStopNative(groupId);
    }

    // Native methods that call into the JNI interface
    private static native void classInitNative();
    private native void initNative();
    private native void cleanupNative();
    private native boolean connectLeAudioNative(byte[] address);
    private native boolean disconnectLeAudioNative(byte[] address);
    private native void groupStreamNative(int groupId, int contentType);
    private native void groupSuspendNative(int groupId);
    private native void groupStopNative(int groupId);
}
