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

package com.android.car.dialer.bluetooth;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.LifecycleRegistry;
import androidx.lifecycle.Observer;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.HashSet;
import java.util.Set;

@RunWith(AndroidJUnit4.class)
public class BluetoothPairListLiveDataTest {
    private static final String INTENT_ACTION = BluetoothDevice.ACTION_BOND_STATE_CHANGED;

    private BluetoothPairListLiveData mBluetoothPairListLiveData;
    @Mock
    private BluetoothAdapter mBluetoothAdapter;
    private LifecycleRegistry mLifecycleRegistry;
    @Mock
    private LifecycleOwner mMockLifecycleOwner;
    @Mock
    private Observer<Set<BluetoothDevice>> mMockObserver;
    @Captor
    private ArgumentCaptor<Set<BluetoothDevice>> mValueCaptor;
    @Captor
    private ArgumentCaptor<BroadcastReceiver> mReceiverCaptor;
    @Mock
    private Context mContext;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        mBluetoothPairListLiveData = new BluetoothPairListLiveData(mContext, mBluetoothAdapter);
        mLifecycleRegistry = new LifecycleRegistry(mMockLifecycleOwner);
        when(mMockLifecycleOwner.getLifecycle()).thenReturn(mLifecycleRegistry);
    }

    @Test
    @UiThreadTest
    public void testOnActive() {
        mBluetoothPairListLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value));
        verify(mMockObserver, never()).onChanged(any());

        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());
        verify(mMockObserver).onChanged(any());
    }

    @Test
    @UiThreadTest
    public void testOnBluetoothConnected() {
        // Set up Bluetooth devices
        Set<BluetoothDevice> bondedDevices = new HashSet<BluetoothDevice>();
        bondedDevices.add(mock(BluetoothDevice.class));
        when(mBluetoothAdapter.getBondedDevices()).thenReturn(bondedDevices);
        doNothing().when(mMockObserver).onChanged(mValueCaptor.capture());
        mBluetoothPairListLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value));
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verifyBondedDevices(bondedDevices);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());

        // Update Bluetooth devices
        bondedDevices.add(mock(BluetoothDevice.class));
        Intent intent = new Intent();
        intent.setAction(INTENT_ACTION);
        mReceiverCaptor.getValue().onReceive(mContext, intent);
        verifyBondedDevices(bondedDevices);
    }

    @Test
    @UiThreadTest
    public void testOnInactiveUnregister() {
        mBluetoothPairListLiveData.observe(mMockLifecycleOwner,
                value -> mMockObserver.onChanged(value));
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());

        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY);
        verify(mContext).unregisterReceiver(mReceiverCaptor.getValue());
    }

    private void verifyBondedDevices(Set bondedDevices) {
        // Verify Bonded Devices for BluetoothAdapter
        assertThat(mBluetoothAdapter.getBondedDevices().containsAll(bondedDevices)).isTrue();
        assertThat(mBluetoothAdapter.getBondedDevices().size()).isEqualTo(bondedDevices.size());
        // Verify Bonded Devices for LiveData
        assertThat(mBluetoothPairListLiveData.getValue().containsAll(bondedDevices)).isTrue();
        assertThat(mBluetoothPairListLiveData.getValue().size()).isEqualTo(bondedDevices.size());
        // Verify Bonded Devices for Observer
        assertThat(mValueCaptor.getValue().containsAll(bondedDevices)).isTrue();
        assertThat(mValueCaptor.getValue().size()).isEqualTo(bondedDevices.size());
    }
}
