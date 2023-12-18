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
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.telecom.TelecomManager;

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

import java.util.Collections;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class HfpDeviceListLiveDataTest {
    private static final String INTENT_ACTION_REGISTERED =
            TelecomManager.ACTION_PHONE_ACCOUNT_REGISTERED;

    private HfpDeviceListLiveData mHfpDeviceListLiveData;
    private LifecycleRegistry mLifecycleRegistry;
    @Mock
    private LifecycleOwner mMockLifecycleOwner;
    @Mock
    private Observer<Integer> mMockObserver;
    @Mock
    private PhoneAccountManager mPhoneAccountManager;
    private BluetoothAdapter mBluetoothAdapter;
    private List<BluetoothDevice> mBluetoothDeviceList;
    @Mock
    private Context mContext;
    @Captor
    private ArgumentCaptor<BroadcastReceiver> mReceiverCaptor;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        mBluetoothDeviceList = Collections.singletonList(null);
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        mHfpDeviceListLiveData = new HfpDeviceListLiveData(
                mContext, mBluetoothAdapter, mPhoneAccountManager);
        mLifecycleRegistry = new LifecycleRegistry(mMockLifecycleOwner);
        when(mMockLifecycleOwner.getLifecycle()).thenReturn(mLifecycleRegistry);
        when(mPhoneAccountManager.getHfpDeviceList()).thenReturn(mBluetoothDeviceList);
    }

    @Test
    @UiThreadTest
    public void testOnActive() {
        mHfpDeviceListLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value.size()));
        verify(mMockObserver, never()).onChanged(any());
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());
        verify(mMockObserver).onChanged(any());
    }

    @Test
    @UiThreadTest
    public void testOnBluetoothHfpStateChange() {
        ArgumentCaptor<Integer> valueCaptor = ArgumentCaptor.forClass(Integer.class);
        doNothing().when(mMockObserver).onChanged(valueCaptor.capture());

        when(mPhoneAccountManager.getHfpDeviceList()).thenReturn(mBluetoothDeviceList);
        mHfpDeviceListLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value.size()));
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());
        assertThat(mHfpDeviceListLiveData.getValue()).isNotEmpty();

        when(mPhoneAccountManager.getHfpDeviceList()).thenReturn(Collections.emptyList());
        Intent intent = new Intent();
        intent.setAction(INTENT_ACTION_REGISTERED);
        mReceiverCaptor.getValue().onReceive(mContext, intent);
        assertThat(mHfpDeviceListLiveData.getValue()).isEmpty();
    }

    @Test
    @UiThreadTest
    public void testOnInactiveUnregister() {
        mHfpDeviceListLiveData.observe(mMockLifecycleOwner,
                (value) -> mMockObserver.onChanged(value.size()));
        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START);
        verify(mContext).registerReceiver(mReceiverCaptor.capture(), any());

        mLifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY);
        verify(mContext).unregisterReceiver(mReceiverCaptor.getValue());
    }
}
