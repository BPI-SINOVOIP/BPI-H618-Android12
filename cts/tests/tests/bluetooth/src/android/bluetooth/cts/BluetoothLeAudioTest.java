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

package android.bluetooth.cts;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothLeAudio;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Build;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.compatibility.common.util.ApiLevelUtil;

import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class BluetoothLeAudioTest extends AndroidTestCase {
    private static final String TAG = BluetoothLeAudioTest.class.getSimpleName();

    private static final int PROXY_CONNECTION_TIMEOUT_MS = 500;  // ms timeout for Proxy Connect
    private static final String PROFILE_SUPPORTED_LE_AUDIO = "profile_supported_leaudio";
    private static final int LE_AUDIO_PROFILE_CONSTANT = 22;

    private boolean mHasBluetooth;
    private BluetoothAdapter mAdapter;

    private BluetoothLeAudio mBluetoothLeAudio;
    private boolean mIsLeAudioSupported;
    private boolean mIsProfileReady;
    private Condition mConditionProfileIsConnected;
    private ReentrantLock mProfileConnectedlock;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        if (ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S)) {
            mHasBluetooth = getContext().getPackageManager().hasSystemFeature(
                    PackageManager.FEATURE_BLUETOOTH);

            if (!mHasBluetooth) return;
            BluetoothManager manager = getContext().getSystemService(BluetoothManager.class);
            mAdapter = manager.getAdapter();
            assertTrue(BTAdapterUtils.enableAdapter(mAdapter, mContext));

            mProfileConnectedlock = new ReentrantLock();
            mConditionProfileIsConnected  = mProfileConnectedlock.newCondition();
            mIsProfileReady = false;
            mBluetoothLeAudio = null;

            Resources bluetoothResources = mContext.getPackageManager().getResourcesForApplication(
                    "com.android.bluetooth");
            int leAudioSupportId = bluetoothResources.getIdentifier(
                    PROFILE_SUPPORTED_LE_AUDIO, "bool", "com.android.bluetooth");
            if (leAudioSupportId == 0) return;
            mIsLeAudioSupported = bluetoothResources.getBoolean(leAudioSupportId);
            if (!mIsLeAudioSupported) return;

            mAdapter.getProfileProxy(getContext(), new BluetoothLeAudioServiceListener(),
                    LE_AUDIO_PROFILE_CONSTANT);
        }
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        if (mHasBluetooth) {
            if (mAdapter != null && mBluetoothLeAudio != null) {
                mBluetoothLeAudio.close();
                mBluetoothLeAudio = null;
                mIsProfileReady = false;
            }
            assertTrue(BTAdapterUtils.disableAdapter(mAdapter, mContext));
            mAdapter = null;
        }
    }

    public void testGetConnectedDevices() {
        if (!(mHasBluetooth && mIsLeAudioSupported)) return;

        assertTrue(waitForProfileConnect());
        assertNotNull(mBluetoothLeAudio);

        assertTrue(BTAdapterUtils.disableAdapter(mAdapter, mContext));

        // Verify returns empty list if bluetooth is not enabled
        List<BluetoothDevice> connectedDevices = mBluetoothLeAudio.getConnectedDevices();
        assertTrue(connectedDevices.isEmpty());
    }

    public void testGetDevicesMatchingConnectionStates() {
        if (!(mHasBluetooth && mIsLeAudioSupported)) return;

        assertTrue(waitForProfileConnect());
        assertNotNull(mBluetoothLeAudio);

        assertTrue(BTAdapterUtils.disableAdapter(mAdapter, mContext));

        // Verify returns empty list if bluetooth is not enabled
        List<BluetoothDevice> connectedDevices =
                mBluetoothLeAudio.getDevicesMatchingConnectionStates(null);
        assertTrue(connectedDevices.isEmpty());
    }

    public void testGetConnectionState() {
        if (!(mHasBluetooth && mIsLeAudioSupported)) return;

        assertTrue(waitForProfileConnect());
        assertNotNull(mBluetoothLeAudio);

        BluetoothDevice testDevice = mAdapter.getRemoteDevice("00:11:22:AA:BB:CC");

        // Verify returns false when invalid input is given
        assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mBluetoothLeAudio.getConnectionState(null));

        assertTrue(BTAdapterUtils.disableAdapter(mAdapter, mContext));

        // Verify returns false if bluetooth is not enabled
        assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mBluetoothLeAudio.getConnectionState(testDevice));
    }

    private boolean waitForProfileConnect() {
        mProfileConnectedlock.lock();
        try {
            // Wait for the Adapter to be disabled
            while (!mIsProfileReady) {
                if (!mConditionProfileIsConnected.await(
                        PROXY_CONNECTION_TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
                    // Timeout
                    Log.e(TAG, "Timeout while waiting for Profile Connect");
                    break;
                } // else spurious wakeups
            }
        } catch (InterruptedException e) {
            Log.e(TAG, "waitForProfileConnect: interrrupted");
        } finally {
            mProfileConnectedlock.unlock();
        }
        return mIsProfileReady;
    }

    private final class BluetoothLeAudioServiceListener implements
            BluetoothProfile.ServiceListener {

        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            mProfileConnectedlock.lock();
            mBluetoothLeAudio = (BluetoothLeAudio) proxy;
            mIsProfileReady = true;
            try {
                mConditionProfileIsConnected.signal();
            } finally {
                mProfileConnectedlock.unlock();
            }
        }

        @Override
        public void onServiceDisconnected(int profile) {
        }
    }
}
