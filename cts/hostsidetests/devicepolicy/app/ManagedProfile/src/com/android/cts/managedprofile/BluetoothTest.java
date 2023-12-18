/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.cts.managedprofile;

import static com.google.common.truth.Truth.assertThat;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.content.Intent;
import android.test.AndroidTestCase;

import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import java.io.IOException;
import java.util.UUID;

/**
 * Test that the basic bluetooth API is callable in managed profiles.
 * These tests should only be executed if the device supports bluetooth,
 * i.e. if it has the {@link android.content.pm.PackageManager#FEATURE_BLUETOOTH} feature.
 *
 * This includes tests for the {@link BluetoothAdapter}.
 * The corresponding CTS tests in the primary profile are in
 * {@link android.bluetooth.cts.BasicAdapterTest}.
 * TODO: Merge the primary and managed profile tests into one.
 */
public class BluetoothTest extends AndroidTestCase {
    private BluetoothAdapter mAdapter;
    private boolean mBtWasEnabled;

    public void setUp() throws Exception {
        super.setUp();
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        assertNotNull(mAdapter);
        mBtWasEnabled = mAdapter.isEnabled();
    }

    public void tearDown() throws Exception {
        if (mBtWasEnabled != mAdapter.isEnabled()) {
            if (mBtWasEnabled) {
                enable();
            } else {
                disable();
            }
        }
        super.tearDown();
    }

    /**
     * Checks enable(), disable(), getState(), isEnabled()
     */
    public void testEnableDisable() {
        disable();
        enable();
    }

    /**
     * Test the getAddress() function.
     */
    public void testGetAddress() {
        assertTrue(BluetoothAdapter.checkBluetoothAddress(mAdapter.getAddress()));
    }

    /**
     * Tests the listenUsingRfcommWithServiceRecord function.
     */
    public void testListenUsingRfcommWithServiceRecord() throws IOException {
        enable();
        BluetoothServerSocket socket = mAdapter.listenUsingRfcommWithServiceRecord(
                "test", UUID.randomUUID());
        assertNotNull(socket);
        socket.close();
    }

    /**
     * Test the getRemoteDevice() function.
     */
    public void testGetRemoteDevice() {
        // getRemoteDevice() should work even with Bluetooth disabled
        disable();

        // test bad addresses
        try {
            mAdapter.getRemoteDevice((String)null);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
        }
        try {
            mAdapter.getRemoteDevice("00:00:00:00:00:00:00:00");
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
        }
        try {
            mAdapter.getRemoteDevice((byte[])null);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
        }
        try {
            mAdapter.getRemoteDevice(new byte[] {0x00, 0x00, 0x00, 0x00, 0x00});
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
        }

        // test success
        BluetoothDevice device = mAdapter.getRemoteDevice("00:11:22:AA:BB:CC");
        assertNotNull(device);
        assertEquals("00:11:22:AA:BB:CC", device.getAddress());
        device = mAdapter.getRemoteDevice(
                new byte[] {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
        assertNotNull(device);
        assertEquals("01:02:03:04:05:06", device.getAddress());
    }

    /**
     * Helper to turn BT off.
     * This method will either fail on an assert, or return with BT turned off.
     * Behavior of getState() and isEnabled() are validated along the way.
     */
    private void disable() {
        if (mAdapter.getState() == BluetoothAdapter.STATE_OFF) {
            assertFalse(mAdapter.isEnabled());
            return;
        }

        assertThat(mAdapter.getState()).isEqualTo(BluetoothAdapter.STATE_ON);
        assertThat(mAdapter.isEnabled()).isTrue();
        assertThat(mAdapter.disable()).isTrue();
        try (BlockingBroadcastReceiver r = new BlockingBroadcastReceiver(
                mContext,
                BluetoothAdapter.ACTION_STATE_CHANGED,
                this::isStateDisabled).register()) {
            assertThat(mAdapter.disable()).isTrue();
        }
        assertThat(mAdapter.isEnabled()).isFalse();
    }

    /**
     * Helper to turn BT on.
     * This method will either fail on an assert, or return with BT turned on.
     * Behavior of getState() and isEnabled() are validated along the way.
     */
    private void enable() {
        if (mAdapter.getState() == BluetoothAdapter.STATE_ON) {
            assertTrue(mAdapter.isEnabled());
            return;
        }

        assertThat(mAdapter.getState()).isEqualTo(BluetoothAdapter.STATE_OFF);
        assertThat(mAdapter.isEnabled()).isFalse();
        try (BlockingBroadcastReceiver r = new BlockingBroadcastReceiver(
                mContext,
                BluetoothAdapter.ACTION_STATE_CHANGED,
                this::isStateEnabled).register()) {
            assertThat(mAdapter.enable()).isTrue();
        }
        assertThat(mAdapter.isEnabled()).isTrue();
    }

    private boolean isStateEnabled(Intent intent) {
        return intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1)
                == BluetoothAdapter.STATE_ON;
    }

    private boolean isStateDisabled(Intent intent) {
        return intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1)
                == BluetoothAdapter.STATE_OFF;
    }
}
