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

package com.android.car.dialer.framework;

import static org.mockito.Mockito.mock;

import android.bluetooth.BluetoothDevice;

import com.android.car.dialer.framework.testdata.CallLogDataHandler;
import com.android.car.dialer.framework.testdata.ContactDataHandler;

/**
 * A class simulating the behavior of a real bluetooth device.
 */
public class SimulatedBluetoothDevice {
    private BluetoothDevice mMockBluetoothDevice;
    private String mContactDataFileName;
    private String mCallLogDataFileName;

    private ContactDataHandler mContactDataHandler;
    private CallLogDataHandler mCallLogDataHandler;

    public SimulatedBluetoothDevice(
            ContactDataHandler contactDataHandler,
            CallLogDataHandler callLogDataHandler,
            String contactDataFile,
            String callLogDataFile) {
        mMockBluetoothDevice = mock(BluetoothDevice.class);
        mContactDataHandler = contactDataHandler;
        mCallLogDataHandler = callLogDataHandler;
        mContactDataFileName = contactDataFile;
        mCallLogDataFileName = callLogDataFile;
    }

    public BluetoothDevice getBluetoothDevice() {
        return mMockBluetoothDevice;
    }

    /**
     * Connects the bluetooth device to the {@link FakeHfpManager} and imports data.
     */
    public void connect() {
        // TODO: b/186613173 figure out account name and account type.
        mContactDataHandler.addContactsAsync(mContactDataFileName, null, null);
        mCallLogDataHandler.addCallLogsAsync(mCallLogDataFileName);
    }

    /**
     * Disconnects the bluetooth device from the {@link FakeHfpManager} and remove the data.
     */
    public void disconnect() {
        mContactDataHandler.removeAddedContactsAsync();
        mCallLogDataHandler.removeAddedCalllogsAsync();
    }
}
