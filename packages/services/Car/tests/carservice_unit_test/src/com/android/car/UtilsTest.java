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

package com.android.car;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothDevice;
import android.text.TextUtils;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.UUID;

@RunWith(MockitoJUnitRunner.class)
public final class UtilsTest {

    @Mock
    private BluetoothDevice mMockBluetoothDevice;

    @Test
    public void testGetDeviceDebugInfo() {
        when(mMockBluetoothDevice.getName()).thenReturn("deviceName");
        when(mMockBluetoothDevice.getAddress()).thenReturn("deviceAddress");

        assertThat(Utils.getDeviceDebugInfo(mMockBluetoothDevice))
            .isEqualTo("(name = deviceName, addr = deviceAddress)");
    }

    @Test
    public void testGetDeviceDebugInfo_nullDevice() {
        assertThat(Utils.getDeviceDebugInfo(null)).isEqualTo("(null)");
    }

    @Test
    public void testTransitionLogToString() {
        Utils.TransitionLog transitionLog =
                new Utils.TransitionLog("serviceName", "state1", "state2", 1623777864000L);
        String result = transitionLog.toString();

        assertThat(result).startsWith("06-15 17:24:24");
        assertThat(result).contains("serviceName:");
        assertThat(result).contains("from state1 to state2");
    }

    @Test
    public void testTransitionLogToString_withExtra() {
        Utils.TransitionLog transitionLog =
                new Utils.TransitionLog("serviceName", "state1", "state2", 1623777864000L, "extra");
        String result = transitionLog.toString();

        assertThat(result).startsWith("06-15 17:24:24");
        assertThat(result).contains("serviceName:");
        assertThat(result).contains("extra");
        assertThat(result).contains("from state1 to state2");
    }

    @Test
    public void testLongToBytes() {
        long longValue = 1234567890L;
        byte[] expected = new byte[] {0, 0, 0, 0, 73, -106, 2, -46};

        assertThat(Utils.longToBytes(longValue)).isEqualTo(expected);
    }

    @Test
    public void testBytesToLong() {
        byte[] bytes = new byte[] {0, 0, 0, 0, 73, -106, 2, -46};
        long expected = 1234567890L;

        assertThat(Utils.bytesToLong(bytes)).isEqualTo(expected);
    }

    @Test
    public void testByteArrayToHexString() {
        assertThat(Utils.byteArrayToHexString(new byte[] {0, 1, 2, -3})).isEqualTo("000102fd");
    }

    @Test
    public void testUuidToBytes() {
        UUID uuid = new UUID(123456789L, 987654321L);
        byte[] expected = new byte[] {0, 0, 0, 0, 7, 91, -51, 21, 0, 0, 0, 0, 58, -34, 104, -79};

        assertThat(Utils.uuidToBytes(uuid)).isEqualTo(expected);
    }

    @Test
    public void testBytesToUUID() {
        byte[] bytes = new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, -9, -8, -7, -6, -5, -4, -3};
        UUID expected = new UUID(72623859790382856L, 718316418130246909L);

        assertThat(Utils.bytesToUUID(bytes).getLeastSignificantBits())
                .isEqualTo(718316418130246909L);
        assertThat(Utils.bytesToUUID(bytes).getMostSignificantBits()).isEqualTo(72623859790382856L);
        assertThat(Utils.bytesToUUID(bytes)).isEqualTo(expected);
    }

    @Test
    public void testBytesToUUID_invalidLength() {
        byte[] bytes = new byte[] {0};

        assertThat(Utils.bytesToUUID(bytes)).isNull();
    }

    @Test
    public void testGenerateRandomNumberString() {
        String result = Utils.generateRandomNumberString(25);

        assertThat(result).hasLength(25);
        assertThat(TextUtils.isDigitsOnly(result)).isTrue();
    }

    @Test
    public void testConcatByteArrays() {
        byte[] bytes1 = new byte[] {1, 2, 3};
        byte[] bytes2 = new byte[] {4, 5, 6};
        Byte[] expected = new Byte[] {1, 2, 3, 4, 5, 6};

        assertThat(Utils.concatByteArrays(bytes1, bytes2)).asList()
                .containsExactlyElementsIn(expected).inOrder();
    }
}
