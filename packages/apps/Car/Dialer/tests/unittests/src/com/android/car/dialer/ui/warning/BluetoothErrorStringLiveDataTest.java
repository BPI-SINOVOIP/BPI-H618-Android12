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

package com.android.car.dialer.ui.warning;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;

import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.dialer.R;
import com.android.car.dialer.bluetooth.BluetoothState;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Collections;
import java.util.List;
import java.util.Set;

@RunWith(AndroidJUnit4.class)
public class BluetoothErrorStringLiveDataTest {

    @Mock
    private Context mContext;
    @Mock
    private Observer<String> mMockObserver;
    private BluetoothAdapter mBluetoothAdapter;
    private MutableLiveData<List<BluetoothDevice>> mHfpDeviceListLiveData;
    private MutableLiveData<Set<BluetoothDevice>> mPairedListLiveData;
    private MutableLiveData<Integer> mBluetoothStateLiveData;
    private BluetoothErrorStringLiveData mBluetoothErrorStringLiveData;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mHfpDeviceListLiveData = new MutableLiveData<>(Collections.emptyList());
        mPairedListLiveData = new MutableLiveData<>(Collections.emptySet());
        mBluetoothStateLiveData = new MutableLiveData<>(0);
    }

    @Test
    @UiThreadTest
    public void testDialerAppState_defaultBluetoothAdapterIsNull_bluetoothError() {
        mBluetoothAdapter = null;

        initializeBluetoothErrorStringLiveData();

        assertThat(mBluetoothErrorStringLiveData.getValue()).isEqualTo(
                mContext.getString(R.string.bluetooth_unavailable));
    }

    @Test
    @UiThreadTest
    public void testDialerAppState_bluetoothNotEnabled_bluetoothError() {
        mBluetoothAdapter = mock(BluetoothAdapter.class);
        mBluetoothStateLiveData.setValue(BluetoothState.DISABLED);

        initializeBluetoothErrorStringLiveData();

        assertThat(mBluetoothErrorStringLiveData.getValue()).isEqualTo(
                mContext.getString(R.string.bluetooth_disabled));
    }

    @Test
    @UiThreadTest
    public void testDialerAppState_noPairedDevices_bluetoothError() {
        mBluetoothAdapter = mock(BluetoothAdapter.class);
        mBluetoothStateLiveData.setValue(BluetoothState.ENABLED);
        mPairedListLiveData.setValue(Collections.emptySet());

        initializeBluetoothErrorStringLiveData();

        assertThat(mBluetoothErrorStringLiveData.getValue()).isEqualTo(
                mContext.getString(R.string.bluetooth_unpaired));
    }

    @Test
    @UiThreadTest
    public void testDialerAppState_hfpNoConnected_bluetoothError() {
        mBluetoothAdapter = mock(BluetoothAdapter.class);
        mBluetoothStateLiveData.setValue(BluetoothState.ENABLED);
        BluetoothDevice mockBluetoothDevice = mock(BluetoothDevice.class);
        mPairedListLiveData.setValue(Collections.singleton(mockBluetoothDevice));
        mHfpDeviceListLiveData.setValue(Collections.emptyList());

        initializeBluetoothErrorStringLiveData();

        assertThat(mBluetoothErrorStringLiveData.getValue()).isEqualTo(
                mContext.getString(R.string.no_hfp));
    }

    @Test
    @UiThreadTest
    public void testDialerAppState_bluetoothAllSet_dialerAppNoError() {
        mBluetoothAdapter = mock(BluetoothAdapter.class);
        mBluetoothStateLiveData.setValue(BluetoothState.ENABLED);
        BluetoothDevice mockBluetoothDevice = mock(BluetoothDevice.class);
        mPairedListLiveData.setValue(Collections.singleton(mockBluetoothDevice));
        mHfpDeviceListLiveData.setValue(Collections.singletonList(mockBluetoothDevice));

        initializeBluetoothErrorStringLiveData();

        assertThat(mBluetoothErrorStringLiveData.getValue()).isEqualTo(
                BluetoothErrorStringLiveData.NO_BT_ERROR);
    }

    private void initializeBluetoothErrorStringLiveData() {
        mBluetoothErrorStringLiveData = new BluetoothErrorStringLiveData(mContext,
                mHfpDeviceListLiveData, mPairedListLiveData, mBluetoothStateLiveData,
                mBluetoothAdapter);
        // Observers needed so that the liveData's internal initialization is triggered
        mBluetoothStateLiveData.observeForever(error -> {
        });
        mHfpDeviceListLiveData.observeForever(error -> {
        });
        mPairedListLiveData.observeForever(error -> {
        });
        mBluetoothErrorStringLiveData.observeForever(mMockObserver);
    }
}
