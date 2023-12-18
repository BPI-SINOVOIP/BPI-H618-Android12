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

package com.android.net.module.util

import android.net.NetworkCapabilities
import android.net.NetworkCapabilities.NET_CAPABILITY_CBS
import android.net.NetworkCapabilities.NET_CAPABILITY_EIMS
import android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET
import android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET
import android.net.NetworkCapabilities.NET_CAPABILITY_OEM_PAID
import android.net.NetworkCapabilities.TRANSPORT_BLUETOOTH
import android.net.NetworkCapabilities.TRANSPORT_CELLULAR
import android.net.NetworkCapabilities.TRANSPORT_ETHERNET
import android.net.NetworkCapabilities.TRANSPORT_TEST
import android.net.NetworkCapabilities.TRANSPORT_VPN
import android.net.NetworkCapabilities.TRANSPORT_WIFI
import android.net.NetworkCapabilities.TRANSPORT_WIFI_AWARE
import androidx.test.filters.SmallTest
import androidx.test.runner.AndroidJUnit4
import com.android.net.module.util.NetworkCapabilitiesUtils.RESTRICTED_CAPABILITIES
import com.android.net.module.util.NetworkCapabilitiesUtils.UNRESTRICTED_CAPABILITIES
import com.android.net.module.util.NetworkCapabilitiesUtils.getDisplayTransport
import com.android.net.module.util.NetworkCapabilitiesUtils.packBits
import com.android.net.module.util.NetworkCapabilitiesUtils.unpackBits
import org.junit.Test
import org.junit.runner.RunWith
import java.lang.IllegalArgumentException
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue

@RunWith(AndroidJUnit4::class)
@SmallTest
class NetworkCapabilitiesUtilsTest {

    @Test
    fun testGetAccountingTransport() {
        assertEquals(TRANSPORT_WIFI, getDisplayTransport(intArrayOf(TRANSPORT_WIFI)))
        assertEquals(TRANSPORT_CELLULAR, getDisplayTransport(intArrayOf(TRANSPORT_CELLULAR)))
        assertEquals(TRANSPORT_BLUETOOTH, getDisplayTransport(intArrayOf(TRANSPORT_BLUETOOTH)))
        assertEquals(TRANSPORT_ETHERNET, getDisplayTransport(intArrayOf(TRANSPORT_ETHERNET)))
        assertEquals(TRANSPORT_WIFI_AWARE, getDisplayTransport(intArrayOf(TRANSPORT_WIFI_AWARE)))

        assertEquals(TRANSPORT_VPN, getDisplayTransport(
                intArrayOf(TRANSPORT_VPN, TRANSPORT_WIFI)))
        assertEquals(TRANSPORT_VPN, getDisplayTransport(
                intArrayOf(TRANSPORT_CELLULAR, TRANSPORT_VPN)))

        assertEquals(TRANSPORT_WIFI, getDisplayTransport(
                intArrayOf(TRANSPORT_ETHERNET, TRANSPORT_WIFI)))
        assertEquals(TRANSPORT_ETHERNET, getDisplayTransport(
                intArrayOf(TRANSPORT_ETHERNET, TRANSPORT_TEST)))

        assertFailsWith(IllegalArgumentException::class) {
            getDisplayTransport(intArrayOf())
        }
    }

    @Test
    fun testBitPackingTestCase() {
        runBitPackingTestCase(0, intArrayOf())
        runBitPackingTestCase(1, intArrayOf(0))
        runBitPackingTestCase(3, intArrayOf(0, 1))
        runBitPackingTestCase(4, intArrayOf(2))
        runBitPackingTestCase(63, intArrayOf(0, 1, 2, 3, 4, 5))
        runBitPackingTestCase(Long.MAX_VALUE.inv(), intArrayOf(63))
        runBitPackingTestCase(Long.MAX_VALUE.inv() + 1, intArrayOf(0, 63))
        runBitPackingTestCase(Long.MAX_VALUE.inv() + 2, intArrayOf(1, 63))
    }

    fun runBitPackingTestCase(packedBits: Long, bits: IntArray) {
        assertEquals(packedBits, packBits(bits))
        assertTrue(bits contentEquals unpackBits(packedBits))
    }

    @Test
    fun testInferRestrictedCapability() {
        val nc = NetworkCapabilities()
        // Default capabilities don't have restricted capability.
        assertFalse(NetworkCapabilitiesUtils.inferRestrictedCapability(nc))
        // If there is a force restricted capability, then the network capabilities is restricted.
        nc.addCapability(NET_CAPABILITY_OEM_PAID)
        nc.addCapability(NET_CAPABILITY_INTERNET)
        assertTrue(NetworkCapabilitiesUtils.inferRestrictedCapability(nc))
        // Except for the force restricted capability, if there is any unrestricted capability in
        // capabilities, then the network capabilities is not restricted.
        nc.removeCapability(NET_CAPABILITY_OEM_PAID)
        nc.addCapability(NET_CAPABILITY_CBS)
        assertFalse(NetworkCapabilitiesUtils.inferRestrictedCapability(nc))
        // Except for the force restricted capability, the network capabilities will only be treated
        // as restricted when there is no any unrestricted capability.
        nc.removeCapability(NET_CAPABILITY_INTERNET)
        assertTrue(NetworkCapabilitiesUtils.inferRestrictedCapability(nc))
    }

    @Test
    fun testRestrictedUnrestrictedCapabilities() {
        // verify EIMS is restricted
        assertEquals((1 shl NET_CAPABILITY_EIMS).toLong() and RESTRICTED_CAPABILITIES,
                (1 shl NET_CAPABILITY_EIMS).toLong())

        // verify CBS is also restricted
        assertEquals((1 shl NET_CAPABILITY_CBS).toLong() and RESTRICTED_CAPABILITIES,
                (1 shl NET_CAPABILITY_CBS).toLong())

        // verify default is not restricted
        assertEquals((1 shl NET_CAPABILITY_INTERNET).toLong() and RESTRICTED_CAPABILITIES, 0)

        // just to see
        assertEquals(RESTRICTED_CAPABILITIES and UNRESTRICTED_CAPABILITIES, 0)
    }
}
