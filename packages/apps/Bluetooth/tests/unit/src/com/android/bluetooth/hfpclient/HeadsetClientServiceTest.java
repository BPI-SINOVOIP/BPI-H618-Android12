/*
 * Copyright 2017 The Android Open Source Project
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

package com.android.bluetooth.hfpclient;

import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.MediumTest;
import androidx.test.rule.ServiceTestRule;
import androidx.test.runner.AndroidJUnit4;

import com.android.bluetooth.R;
import com.android.bluetooth.TestUtils;
import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.storage.DatabaseManager;

import org.junit.After;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class HeadsetClientServiceTest {
    private HeadsetClientService mService = null;
    private BluetoothAdapter mAdapter = null;
    private Context mTargetContext;

    private static final int STANDARD_WAIT_MILLIS = 1000;

    @Rule public final ServiceTestRule mServiceRule = new ServiceTestRule();

    @Mock private AdapterService mAdapterService;
    @Mock private HeadsetClientStateMachine mStateMachine;

    @Mock private DatabaseManager mDatabaseManager;

    @Before
    public void setUp() throws Exception {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        Assume.assumeTrue("Ignore test when HeadsetClientService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_hfpclient));
        MockitoAnnotations.initMocks(this);
        TestUtils.setAdapterService(mAdapterService);
        doReturn(mDatabaseManager).when(mAdapterService).getDatabase();
        doReturn(true, false).when(mAdapterService).isStartedProfile(anyString());
        TestUtils.startService(mServiceRule, HeadsetClientService.class);
        // At this point the service should have started so check NOT null
        mService = HeadsetClientService.getHeadsetClientService();
        Assert.assertNotNull(mService);
        // Try getting the Bluetooth adapter
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        Assert.assertNotNull(mAdapter);
    }

    @After
    public void tearDown() throws Exception {
        if (!mTargetContext.getResources().getBoolean(R.bool.profile_supported_hfpclient)) {
            return;
        }
        TestUtils.stopService(mServiceRule, HeadsetClientService.class);
        mService = HeadsetClientService.getHeadsetClientService();
        Assert.assertNull(mService);
        TestUtils.clearAdapterService(mAdapterService);
    }

    @Test
    public void testInitialize() {
        Assert.assertNotNull(HeadsetClientService.getHeadsetClientService());
    }

    @Test
    public void testSendBIEVtoStateMachineWhenBatteryChanged() {
        // Put mock state machine
        BluetoothDevice device =
                BluetoothAdapter.getDefaultAdapter().getRemoteDevice("00:01:02:03:04:05");
        mService.getStateMachineMap().put(device, mStateMachine);

        // Send battery changed intent
        Intent intent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        intent.putExtra(BatteryManager.EXTRA_LEVEL, 50);
        mService.sendBroadcast(intent);

        // Expect send BIEV to state machine
        verify(mStateMachine, timeout(STANDARD_WAIT_MILLIS).times(1))
                .sendMessage(
                    eq(HeadsetClientStateMachine.SEND_BIEV),
                    eq(2),
                    anyInt());
    }

    @Test
    public void testUpdateBatteryLevel() {
        // Put mock state machine
        BluetoothDevice device =
                BluetoothAdapter.getDefaultAdapter().getRemoteDevice("00:01:02:03:04:05");
        mService.getStateMachineMap().put(device, mStateMachine);

        mService.updateBatteryLevel();

        // Expect send BIEV to state machine
        verify(mStateMachine, timeout(STANDARD_WAIT_MILLIS).times(1))
                .sendMessage(
                    eq(HeadsetClientStateMachine.SEND_BIEV),
                    eq(2),
                    anyInt());
    }
}
