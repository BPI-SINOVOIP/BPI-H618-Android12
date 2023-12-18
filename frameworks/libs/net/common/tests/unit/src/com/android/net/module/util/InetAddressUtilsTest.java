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

package com.android.net.module.util;

import static junit.framework.Assert.assertEquals;

import android.os.Parcel;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.net.Inet6Address;
import java.net.InetAddress;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class InetAddressUtilsTest {

    private InetAddress parcelUnparcelAddress(InetAddress addr) {
        Parcel p = Parcel.obtain();
        InetAddressUtils.parcelInetAddress(p, addr, 0 /* flags */);
        p.setDataPosition(0);
        byte[] marshalled = p.marshall();
        p.recycle();
        p = Parcel.obtain();
        p.unmarshall(marshalled, 0, marshalled.length);
        p.setDataPosition(0);
        InetAddress out = InetAddressUtils.unparcelInetAddress(p);
        p.recycle();
        return out;
    }

    @Test
    public void testParcelUnparcelIpv4Address() throws Exception {
        InetAddress ipv4 = InetAddress.getByName("192.0.2.1");
        assertEquals(ipv4, parcelUnparcelAddress(ipv4));
    }

    @Test
    public void testParcelUnparcelIpv6Address() throws Exception {
        InetAddress ipv6 = InetAddress.getByName("2001:db8::1");
        assertEquals(ipv6, parcelUnparcelAddress(ipv6));
    }

    @Test
    public void testParcelUnparcelScopedIpv6Address() throws Exception {
        InetAddress ipv6 = InetAddress.getByName("fe80::1%42");
        assertEquals(42, ((Inet6Address) ipv6).getScopeId());
        Inet6Address out = (Inet6Address) parcelUnparcelAddress(ipv6);
        assertEquals(ipv6, out);
        assertEquals(42, out.getScopeId());
    }
}
