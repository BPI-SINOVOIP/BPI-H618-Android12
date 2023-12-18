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

package android.ipsec.ike.cts;

import static android.net.ipsec.ike.SaProposal.DH_GROUP_1024_BIT_MODP;

import static com.android.internal.util.HexDump.hexStringToByteArray;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import android.net.InetAddresses;
import android.net.ipsec.ike.exceptions.IkeInternalException;
import android.net.ipsec.ike.exceptions.IkeNetworkLostException;
import android.net.ipsec.ike.exceptions.InvalidKeException;
import android.net.ipsec.ike.exceptions.InvalidMajorVersionException;
import android.net.ipsec.ike.exceptions.InvalidSelectorsException;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.net.InetAddress;

@RunWith(AndroidJUnit4.class)
public class IkeExceptionTest extends IkeTestNetworkBase {
    @Test
    public void testIkeInternalException() throws Exception {
        final Throwable cause = new Throwable("Test Cause");
        final String errMsg = "Test Error Message";

        final IkeInternalException exception = new IkeInternalException(cause);
        final IkeInternalException exceptionWithMsg = new IkeInternalException(errMsg, cause);

        assertEquals(cause, exception.getCause());
        assertEquals(cause, exceptionWithMsg.getCause());
        assertEquals(errMsg, exceptionWithMsg.getMessage());
    }

    @Test
    public void testIkeNetworkLostException() throws Exception {
        final InetAddress testAddress = InetAddresses.parseNumericAddress("198.51.100.10");
        try (TunNetworkContext tunContext = new TunNetworkContext(testAddress)) {
            final IkeNetworkLostException exception =
                    new IkeNetworkLostException(tunContext.tunNetwork);
            assertEquals(tunContext.tunNetwork, exception.getNetwork());
        }
    }

    @Test
    public void testInvalidKeException() throws Exception {
        final InvalidKeException exception = new InvalidKeException(DH_GROUP_1024_BIT_MODP);
        assertEquals(DH_GROUP_1024_BIT_MODP, exception.getDhGroup());
    }

    @Test
    public void testInvalidMajorVersionException() throws Exception {
        final byte majorVersion = (byte) 3;
        final InvalidMajorVersionException exception =
                new InvalidMajorVersionException(majorVersion);
        assertEquals(majorVersion, exception.getMajorVersion());
    }

    @Test
    public void testInvalidSelectorsException() throws Exception {
        final byte[] packetInfo =
                hexStringToByteArray("4500009cafcd4000403208adc0a80064c0a80001c9d8d74200000001");
        final int spi = 0xc9d8d742;

        final InvalidSelectorsException exception = new InvalidSelectorsException(spi, packetInfo);
        assertArrayEquals(packetInfo, exception.getIpSecPacketInfo());
        assertEquals(spi, exception.getIpSecSpi());
    }
}
