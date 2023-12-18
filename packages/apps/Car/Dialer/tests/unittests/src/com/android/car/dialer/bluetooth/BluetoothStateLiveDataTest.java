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
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
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

@RunWith(AndroidJUnit4.class)
public class BluetoothStateLiveDataTest {
    private static final String INTENT_ACTION = BluetoothAdapter.ACTION_STATE_CHANGED;

    private BluetoothStateLiveData mBluetoothStateLiveData;
    private LifecycleRegistry mLifecycleRegistry;
    @Mock
    private LifecycleOwner mMockLifecycleOwner;
    @Mock
    private Observer<Integer> mMockObserver;
    @Mock
    private Context mContext;
    @Mock
    private BluetoothAdapter mBluetoothAdapter;
    @Captor
    private ArgumentCaptor<BroadcastReceiver> mReceiverCaptor;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        mBluetoothStateLiveData = new BluetoothStateLiveData(mContext, mBluetoothAdapter);
        mLifecycleRegistry = new LifecycleRegistry(mMockLifecycleOwner);
        when(mMockLifecycleOwner.getLifecycle()).thenReturn(mLifecycleRegistry);
    }

    @Test
    @UiThreadTest
    public void testOnActive() {
        mBluetoothStateLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value));
        verify(mMockObserver, never()).onChanged(any());

        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());
        verify(mMockObserver).onChanged(any());
    }

    @Test
    @UiThreadTest
    public void testOnBluetoothAdapterStateChange() {
        ArgumentCaptor<Integer> valueCaptor = ArgumentCaptor.forClass(Integer.class);
        doNothing().when(mMockObserver).onChanged(valueCaptor.capture());

        when(mBluetoothAdapter.isEnabled()).thenReturn(false);
        mBluetoothStateLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value));
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        assertThat(valueCaptor.getValue()).isEqualTo(BluetoothState.DISABLED);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());

        when(mBluetoothAdapter.isEnabled()).thenReturn(true);
        Intent intent = new Intent();
        intent.setAction(INTENT_ACTION);
        mReceiverCaptor.getValue().onReceive(mContext, intent);
        assertThat(mBluetoothStateLiveData.getValue()).isEqualTo(BluetoothState.ENABLED);
        assertThat(valueCaptor.getValue()).isEqualTo(BluetoothState.ENABLED);
    }

    @Test
    @UiThreadTest
    public void testOnInactiveUnregister() {
        mBluetoothStateLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value));
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());

        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY);
        verify(mContext).unregisterReceiver(mReceiverCaptor.getValue());
    }
}
