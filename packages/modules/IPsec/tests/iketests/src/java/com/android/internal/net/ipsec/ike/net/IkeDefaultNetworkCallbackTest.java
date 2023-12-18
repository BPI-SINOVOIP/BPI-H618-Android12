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

package com.android.internal.net.ipsec.test.ike.net;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.net.InetAddresses;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;

import org.junit.Before;
import org.junit.Test;

import java.net.Inet4Address;
import java.net.InetAddress;

public class IkeDefaultNetworkCallbackTest {
    // Addresses in the IPv4 Documentation Address Blocks (RFC 5737 Section 3)
    private static final InetAddress CURR_ADDRESS = InetAddresses.parseNumericAddress("192.0.2.0");
    private static final InetAddress CURR_ADDRESS_V6 =
            InetAddresses.parseNumericAddress("2001:db8::2");
    private static final InetAddress UPDATED_ADDRESS =
            InetAddresses.parseNumericAddress("192.0.2.1");

    private static final int IPV4_PREFIX_LEN = 32;
    private static final int IPV6_PREFIX_LEN = 64;

    private Network mMockNetwork;
    private IkeNetworkUpdater mMockIkeNetworkUpdater;

    private InetAddress mCurrAddress;
    private IkeDefaultNetworkCallback mNetworkCallback;

    @Before
    public void setUp() throws Exception {
        mMockNetwork = mock(Network.class);
        mMockIkeNetworkUpdater = mock(IkeNetworkUpdater.class);

        mCurrAddress = CURR_ADDRESS;
        mNetworkCallback =
                new IkeDefaultNetworkCallback(mMockIkeNetworkUpdater, mMockNetwork, mCurrAddress);
    }

    @Test
    public void testOnAvailable() {
        Network updatedNetwork = mock(Network.class);

        mNetworkCallback.onAvailable(updatedNetwork);

        verify(mMockIkeNetworkUpdater).onUnderlyingNetworkUpdated(eq(updatedNetwork));
    }

    @Test
    public void testOnAvailableCurrentNetwork() {
        mNetworkCallback.onAvailable(mMockNetwork);

        verify(mMockIkeNetworkUpdater, never()).onUnderlyingNetworkUpdated(any());
    }

    @Test
    public void testOnLost() {
        mNetworkCallback.onLost(mMockNetwork);

        verify(mMockIkeNetworkUpdater).onUnderlyingNetworkDied();
    }

    @Test
    public void testOnLostWrongNetwork() {
        mNetworkCallback.onLost(mock(Network.class));

        verify(mMockIkeNetworkUpdater, never()).onUnderlyingNetworkDied();
    }

    @Test
    public void testOnLinkPropertiesChanged() throws Exception {
        mNetworkCallback.onLinkPropertiesChanged(
                mMockNetwork, getLinkPropertiesWithAddresses(UPDATED_ADDRESS));

        verify(mMockIkeNetworkUpdater).onUnderlyingNetworkUpdated(eq(mMockNetwork));
    }

    @Test
    public void testOnLinkPropertiesChangedWrongNetwork() throws Exception {
        mNetworkCallback.onLinkPropertiesChanged(
                mock(Network.class), getLinkPropertiesWithAddresses(UPDATED_ADDRESS));

        verify(mMockIkeNetworkUpdater, never()).onUnderlyingNetworkUpdated(any());
    }

    @Test
    public void testOnLinkPropertiesChangedNoAddressChange() throws Exception {
        mNetworkCallback.onLinkPropertiesChanged(
                mMockNetwork, getLinkPropertiesWithAddresses(CURR_ADDRESS));

        verify(mMockIkeNetworkUpdater, never()).onUnderlyingNetworkUpdated(any());
    }

    @Test
    public void testOnLinkPropertiesChangedNoAddressChangeIpv6() throws Exception {
        mCurrAddress = CURR_ADDRESS_V6;
        mNetworkCallback =
                new IkeDefaultNetworkCallback(mMockIkeNetworkUpdater, mMockNetwork, mCurrAddress);

        mNetworkCallback.onLinkPropertiesChanged(
                mMockNetwork, getLinkPropertiesWithAddresses(CURR_ADDRESS_V6));

        verify(mMockIkeNetworkUpdater, never()).onUnderlyingNetworkUpdated(any());
    }

    private LinkProperties getLinkPropertiesWithAddresses(InetAddress... addresses)
            throws Exception {
        LinkProperties linkProperties = new LinkProperties();

        for (InetAddress address : addresses) {
            int prefixLen = address instanceof Inet4Address ? IPV4_PREFIX_LEN : IPV6_PREFIX_LEN;
            linkProperties.addLinkAddress(new LinkAddress(address, prefixLen));
        }
        return linkProperties;
    }

    // TODO: b/194229855 Add tests for verifying stacked LinkProperties address change
}
