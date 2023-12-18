/*
 * Copyright 2020 The Android Open Source Project
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

package com.google.android.iwlan.epdg;

import android.net.LinkAddress;
import android.telephony.data.NetworkSliceInfo;

import java.net.InetAddress;
import java.util.LinkedList;
import java.util.List;

public class TunnelLinkPropertiesTest {

    private static final String IP_ADDRESS = "192.0.2.1";
    private static final String DNS_ADDRESS = "8.8.8.8";
    private static final String PSCF_ADDRESS = "10.159.204.230";
    private static final String INTERFACE_NAME = "ipsec6";
    private static final NetworkSliceInfo SLICE_INFO =
            NetworkSliceSelectionAssistanceInformation.getSliceInfo(new byte[] {1});

    public static TunnelLinkProperties createTestTunnelLinkProperties() throws Exception {
        List<LinkAddress> mInternalAddressList = new LinkedList<>();
        List<InetAddress> mDNSAddressList = new LinkedList<>();
        List<InetAddress> mPCSFAddressList = new LinkedList<>();

        InetAddress mDNSAddress = InetAddress.getByName(DNS_ADDRESS);
        InetAddress mPCSFAddress = InetAddress.getByName(PSCF_ADDRESS);

        mInternalAddressList.add(new LinkAddress(InetAddress.getByName(IP_ADDRESS), 3));
        mDNSAddressList.add(mDNSAddress);
        mPCSFAddressList.add(mPCSFAddress);

        return TunnelLinkProperties.builder()
                .setInternalAddresses(mInternalAddressList)
                .setDnsAddresses(mDNSAddressList)
                .setPcscfAddresses(mPCSFAddressList)
                .setIfaceName(INTERFACE_NAME)
                .setSliceInfo(SLICE_INFO)
                .build();
    }
}
