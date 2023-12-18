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

package com.android.internal.net.ipsec.test.ike;

import static com.android.internal.net.ipsec.test.ike.IkeLocalRequestScheduler.LOCAL_REQUEST_WAKE_LOCK_TAG;
import static com.android.internal.net.ipsec.test.ike.IkeSessionStateMachine.BUSY_WAKE_LOCK_TAG;

import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;

import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.InetAddresses;
import android.net.IpSecManager;
import android.net.IpSecManager.UdpEncapsulationSocket;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.SocketKeepalive;
import android.os.Handler;
import android.os.PowerManager;

import com.android.internal.net.ipsec.test.ike.testutils.MockIpSecTestUtils;
import com.android.internal.net.ipsec.test.ike.utils.IkeAlarmReceiver;
import com.android.internal.net.ipsec.test.ike.utils.RandomnessFactory;

import org.junit.Before;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.util.concurrent.Executor;

public abstract class IkeSessionTestBase {
    protected static final Inet4Address LOCAL_ADDRESS =
            (Inet4Address) InetAddresses.parseNumericAddress("192.0.2.200");
    protected static final Inet4Address UPDATED_LOCAL_ADDRESS =
            (Inet4Address) InetAddresses.parseNumericAddress("192.0.2.201");
    protected static final Inet4Address REMOTE_ADDRESS =
            (Inet4Address) InetAddresses.parseNumericAddress("127.0.0.1");
    protected static final Inet6Address LOCAL_ADDRESS_V6 =
            (Inet6Address) InetAddresses.parseNumericAddress("2001:db8::200");
    protected static final Inet6Address UPDATED_LOCAL_ADDRESS_V6 =
            (Inet6Address) InetAddresses.parseNumericAddress("2001:db8::201");
    protected static final Inet6Address REMOTE_ADDRESS_V6 =
            (Inet6Address) InetAddresses.parseNumericAddress("::1");
    protected static final String REMOTE_HOSTNAME = "ike.test.android.com";

    protected PowerManager.WakeLock mMockBusyWakelock;
    protected PowerManager.WakeLock mMockLocalRequestWakelock;

    protected MockIpSecTestUtils mMockIpSecTestUtils;
    protected Context mSpyContext;
    protected IpSecManager mIpSecManager;
    protected PowerManager mPowerManager;

    protected ConnectivityManager mMockConnectManager;
    protected Network mMockDefaultNetwork;
    protected SocketKeepalive mMockSocketKeepalive;
    protected NetworkCapabilities mMockNetworkCapabilities;

    @Before
    public void setUp() throws Exception {
        mMockIpSecTestUtils = MockIpSecTestUtils.setUpMockIpSec();
        mIpSecManager = mMockIpSecTestUtils.getIpSecManager();

        mSpyContext = spy(mMockIpSecTestUtils.getContext());
        doReturn(null)
                .when(mSpyContext)
                .registerReceiver(
                        any(IkeAlarmReceiver.class),
                        any(IntentFilter.class),
                        any(),
                        any(Handler.class));
        doNothing().when(mSpyContext).unregisterReceiver(any(IkeAlarmReceiver.class));

        mPowerManager = mock(PowerManager.class);
        mMockBusyWakelock = mock(PowerManager.WakeLock.class);
        mMockLocalRequestWakelock = mock(PowerManager.WakeLock.class);
        doReturn(mPowerManager).when(mSpyContext).getSystemService(eq(PowerManager.class));
        doReturn(mMockBusyWakelock)
                .when(mPowerManager)
                .newWakeLock(anyInt(), argThat(tag -> tag.contains(BUSY_WAKE_LOCK_TAG)));
        // Only in test that all local requests will get the same WakeLock instance but in function
        // code each local request will have a separate WakeLock.
        doReturn(mMockLocalRequestWakelock)
                .when(mPowerManager)
                .newWakeLock(anyInt(), argThat(tag -> tag.contains(LOCAL_REQUEST_WAKE_LOCK_TAG)));

        mMockDefaultNetwork = mock(Network.class);
        resetDefaultNetwork();

        mMockSocketKeepalive = mock(SocketKeepalive.class);

        mMockNetworkCapabilities = mock(NetworkCapabilities.class);
        doReturn(false)
                .when(mMockNetworkCapabilities)
                .hasTransport(RandomnessFactory.TRANSPORT_TEST);

        mMockConnectManager = mock(ConnectivityManager.class);
        doReturn(mMockConnectManager)
                .when(mSpyContext)
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        resetMockConnectManager();
    }

    protected void resetMockConnectManager() {
        reset(mMockConnectManager);
        doReturn(mMockDefaultNetwork).when(mMockConnectManager).getActiveNetwork();
        doReturn(mMockSocketKeepalive)
                .when(mMockConnectManager)
                .createSocketKeepalive(
                        any(Network.class),
                        any(UdpEncapsulationSocket.class),
                        any(Inet4Address.class),
                        any(Inet4Address.class),
                        any(Executor.class),
                        any(SocketKeepalive.Callback.class));
        doReturn(mMockNetworkCapabilities)
                .when(mMockConnectManager)
                .getNetworkCapabilities(any(Network.class));
    }

    protected void resetDefaultNetwork() throws Exception {
        reset(mMockDefaultNetwork);
        doReturn(new InetAddress[] {REMOTE_ADDRESS})
                .when(mMockDefaultNetwork)
                .getAllByName(REMOTE_HOSTNAME);
        doReturn(new InetAddress[] {REMOTE_ADDRESS})
                .when(mMockDefaultNetwork)
                .getAllByName(REMOTE_ADDRESS.getHostAddress());
    }
}
