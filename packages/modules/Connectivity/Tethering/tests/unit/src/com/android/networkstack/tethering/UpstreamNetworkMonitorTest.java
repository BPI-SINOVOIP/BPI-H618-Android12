/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.networkstack.tethering;

import static android.net.ConnectivityManager.TYPE_MOBILE_DUN;
import static android.net.ConnectivityManager.TYPE_MOBILE_HIPRI;
import static android.net.ConnectivityManager.TYPE_WIFI;
import static android.net.NetworkCapabilities.NET_CAPABILITY_DUN;
import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.NetworkCapabilities.TRANSPORT_CELLULAR;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;

import static com.android.modules.utils.build.SdkLevel.isAtLeastS;
import static com.android.networkstack.tethering.UpstreamNetworkMonitor.TYPE_NONE;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.IConnectivityManager;
import android.net.IpPrefix;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.util.SharedLog;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.test.TestLooper;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.networkstack.tethering.TestConnectivityManager.NetworkRequestInfo;
import com.android.networkstack.tethering.TestConnectivityManager.TestNetworkAgent;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class UpstreamNetworkMonitorTest {
    private static final int EVENT_UNM_UPDATE = 1;

    private static final boolean INCLUDES = true;
    private static final boolean EXCLUDES = false;

    private static final NetworkCapabilities CELL_CAPABILITIES = new NetworkCapabilities.Builder()
            .addTransportType(TRANSPORT_CELLULAR).addCapability(NET_CAPABILITY_INTERNET).build();
    private static final NetworkCapabilities DUN_CAPABILITIES = new NetworkCapabilities.Builder()
            .addTransportType(TRANSPORT_CELLULAR).addCapability(NET_CAPABILITY_DUN).build();
    private static final NetworkCapabilities WIFI_CAPABILITIES = new NetworkCapabilities.Builder()
            .addTransportType(TRANSPORT_WIFI).addCapability(NET_CAPABILITY_INTERNET).build();

    @Mock private Context mContext;
    @Mock private EntitlementManager mEntitleMgr;
    @Mock private IConnectivityManager mCS;
    @Mock private SharedLog mLog;

    private TestStateMachine mSM;
    private TestConnectivityManager mCM;
    private UpstreamNetworkMonitor mUNM;

    private final TestLooper mLooper = new TestLooper();

    @Before public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        reset(mContext);
        reset(mCS);
        reset(mLog);
        when(mLog.forSubComponent(anyString())).thenReturn(mLog);
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(true);

        mCM = spy(new TestConnectivityManager(mContext, mCS));
        when(mContext.getSystemService(eq(Context.CONNECTIVITY_SERVICE))).thenReturn(mCM);
        mSM = new TestStateMachine(mLooper.getLooper());
        mUNM = new UpstreamNetworkMonitor(mContext, mSM, mLog, EVENT_UNM_UPDATE);
    }

    @After public void tearDown() throws Exception {
        if (mSM != null) {
            mSM.quit();
            mSM = null;
        }
    }

    @Test
    public void testStopWithoutStartIsNonFatal() {
        mUNM.stop();
        mUNM.stop();
        mUNM.stop();
    }

    @Test
    public void testDoesNothingBeforeTrackDefaultAndStarted() throws Exception {
        assertTrue(mCM.hasNoCallbacks());
        assertFalse(mUNM.mobileNetworkRequested());

        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        assertTrue(mCM.hasNoCallbacks());
        mUNM.setUpstreamConfig(false /* autoUpstream */, false /* dunRequired */);
        assertTrue(mCM.hasNoCallbacks());
    }

    @Test
    public void testDefaultNetworkIsTracked() throws Exception {
        assertTrue(mCM.hasNoCallbacks());
        mUNM.startTrackDefaultNetwork(mEntitleMgr);

        mUNM.startObserveAllNetworks();
        assertEquals(1, mCM.mTrackingDefault.size());

        mUNM.stop();
        assertTrue(mCM.onlyHasDefaultCallbacks());
    }

    @Test
    public void testListensForAllNetworks() throws Exception {
        assertTrue(mCM.mListening.isEmpty());

        mUNM.startTrackDefaultNetwork(mEntitleMgr);
        mUNM.startObserveAllNetworks();
        assertFalse(mCM.mListening.isEmpty());
        assertTrue(mCM.isListeningForAll());

        mUNM.stop();
        assertTrue(mCM.onlyHasDefaultCallbacks());
    }

    @Test
    public void testCallbacksRegistered() {
        mUNM.startTrackDefaultNetwork(mEntitleMgr);
        // Verify the fired default request matches expectation.
        final ArgumentCaptor<NetworkRequest> requestCaptor =
                ArgumentCaptor.forClass(NetworkRequest.class);

        if (isAtLeastS()) {
            verify(mCM).registerSystemDefaultNetworkCallback(any(), any());
        } else {
            verify(mCM).requestNetwork(
                    requestCaptor.capture(), any(NetworkCallback.class), any(Handler.class));
            // For R- devices, Tethering will invoke this function in 2 cases, one is to
            // request mobile network, the other is to track system default network. Verify
            // the request is the one tracks default network.
            assertTrue(TestConnectivityManager.looksLikeDefaultRequest(requestCaptor.getValue()));
        }

        mUNM.startObserveAllNetworks();
        verify(mCM, times(1)).registerNetworkCallback(
                any(NetworkRequest.class), any(NetworkCallback.class), any(Handler.class));

        mUNM.stop();
        verify(mCM, times(1)).unregisterNetworkCallback(any(NetworkCallback.class));
    }

    @Test
    public void testRequestsMobileNetwork() throws Exception {
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.startObserveAllNetworks();
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.setUpstreamConfig(false /* autoUpstream */, false /* dunRequired */);
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.setTryCell(true);
        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_HIPRI);
        assertFalse(isDunRequested());

        mUNM.stop();
        assertFalse(mUNM.mobileNetworkRequested());
        assertTrue(mCM.hasNoCallbacks());
    }

    @Test
    public void testDuplicateMobileRequestsIgnored() throws Exception {
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.startObserveAllNetworks();
        verify(mCM, times(1)).registerNetworkCallback(
                any(NetworkRequest.class), any(NetworkCallback.class), any(Handler.class));
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        mUNM.setTryCell(true);
        verify(mCM, times(1)).requestNetwork(
                any(NetworkRequest.class), anyInt(), anyInt(), any(Handler.class),
                any(NetworkCallback.class));

        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_DUN);
        assertTrue(isDunRequested());

        // Try a few things that must not result in any state change.
        mUNM.setTryCell(true);
        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        mUNM.setTryCell(true);

        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_DUN);
        assertTrue(isDunRequested());

        mUNM.stop();
        verify(mCM, times(2)).unregisterNetworkCallback(any(NetworkCallback.class));

        verifyNoMoreInteractions(mCM);
    }

    @Test
    public void testRequestsDunNetwork() throws Exception {
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.startObserveAllNetworks();
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        assertFalse(mUNM.mobileNetworkRequested());
        assertEquals(0, mCM.mRequested.size());

        mUNM.setTryCell(true);
        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_DUN);
        assertTrue(isDunRequested());

        mUNM.stop();
        assertFalse(mUNM.mobileNetworkRequested());
        assertTrue(mCM.hasNoCallbacks());
    }

    @Test
    public void testUpdateMobileRequiresDun() throws Exception {
        mUNM.startObserveAllNetworks();

        // Test going from no-DUN to DUN correctly re-registers callbacks.
        mUNM.setUpstreamConfig(false /* autoUpstream */, false /* dunRequired */);
        mUNM.setTryCell(true);
        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_HIPRI);
        assertFalse(isDunRequested());
        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_DUN);
        assertTrue(isDunRequested());

        // Test going from DUN to no-DUN correctly re-registers callbacks.
        mUNM.setUpstreamConfig(false /* autoUpstream */, false /* dunRequired */);
        assertTrue(mUNM.mobileNetworkRequested());
        assertUpstreamTypeRequested(TYPE_MOBILE_HIPRI);
        assertFalse(isDunRequested());

        mUNM.stop();
        assertFalse(mUNM.mobileNetworkRequested());
    }

    @Test
    public void testSelectPreferredUpstreamType() throws Exception {
        final Collection<Integer> preferredTypes = new ArrayList<>();
        preferredTypes.add(TYPE_WIFI);

        mUNM.startTrackDefaultNetwork(mEntitleMgr);
        mUNM.startObserveAllNetworks();
        // There are no networks, so there is nothing to select.
        assertSatisfiesLegacyType(TYPE_NONE, mUNM.selectPreferredUpstreamType(preferredTypes));

        final TestNetworkAgent wifiAgent = new TestNetworkAgent(mCM, WIFI_CAPABILITIES);
        wifiAgent.fakeConnect();
        mLooper.dispatchAll();
        // WiFi is up, we should prefer it.
        assertSatisfiesLegacyType(TYPE_WIFI, mUNM.selectPreferredUpstreamType(preferredTypes));
        wifiAgent.fakeDisconnect();
        mLooper.dispatchAll();
        // There are no networks, so there is nothing to select.
        assertSatisfiesLegacyType(TYPE_NONE, mUNM.selectPreferredUpstreamType(preferredTypes));

        final TestNetworkAgent cellAgent = new TestNetworkAgent(mCM, CELL_CAPABILITIES);
        cellAgent.fakeConnect();
        mLooper.dispatchAll();
        assertSatisfiesLegacyType(TYPE_NONE, mUNM.selectPreferredUpstreamType(preferredTypes));

        preferredTypes.add(TYPE_MOBILE_DUN);
        // This is coupled with preferred types in TetheringConfiguration.
        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        // DUN is available, but only use regular cell: no upstream selected.
        assertSatisfiesLegacyType(TYPE_NONE, mUNM.selectPreferredUpstreamType(preferredTypes));
        preferredTypes.remove(TYPE_MOBILE_DUN);
        // No WiFi, but our preferred flavour of cell is up.
        preferredTypes.add(TYPE_MOBILE_HIPRI);
        // This is coupled with preferred types in TetheringConfiguration.
        mUNM.setUpstreamConfig(false /* autoUpstream */, false /* dunRequired */);
        assertSatisfiesLegacyType(TYPE_MOBILE_HIPRI,
                mUNM.selectPreferredUpstreamType(preferredTypes));
        // mobile is not permitted, we should not use HIPRI.
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(false);
        assertSatisfiesLegacyType(TYPE_NONE, mUNM.selectPreferredUpstreamType(preferredTypes));
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(true);
        assertSatisfiesLegacyType(TYPE_MOBILE_HIPRI,
                mUNM.selectPreferredUpstreamType(preferredTypes));

        wifiAgent.fakeConnect();
        mLooper.dispatchAll();
        // WiFi is up, and we should prefer it over cell.
        assertSatisfiesLegacyType(TYPE_WIFI, mUNM.selectPreferredUpstreamType(preferredTypes));

        preferredTypes.remove(TYPE_MOBILE_HIPRI);
        preferredTypes.add(TYPE_MOBILE_DUN);
        // This is coupled with preferred types in TetheringConfiguration.
        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        assertSatisfiesLegacyType(TYPE_WIFI, mUNM.selectPreferredUpstreamType(preferredTypes));

        final TestNetworkAgent dunAgent = new TestNetworkAgent(mCM, DUN_CAPABILITIES);
        dunAgent.fakeConnect();
        mLooper.dispatchAll();

        // WiFi is still preferred.
        assertSatisfiesLegacyType(TYPE_WIFI, mUNM.selectPreferredUpstreamType(preferredTypes));

        // WiFi goes down, cell and DUN are still up but only DUN is preferred.
        wifiAgent.fakeDisconnect();
        mLooper.dispatchAll();
        assertSatisfiesLegacyType(TYPE_MOBILE_DUN,
                mUNM.selectPreferredUpstreamType(preferredTypes));
        // mobile is not permitted, we should not use DUN.
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(false);
        assertSatisfiesLegacyType(TYPE_NONE, mUNM.selectPreferredUpstreamType(preferredTypes));
        // mobile change back to permitted, DUN should come back
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(true);
        assertSatisfiesLegacyType(TYPE_MOBILE_DUN,
                mUNM.selectPreferredUpstreamType(preferredTypes));
    }

    @Test
    public void testGetCurrentPreferredUpstream() throws Exception {
        mUNM.startTrackDefaultNetwork(mEntitleMgr);
        mUNM.startObserveAllNetworks();
        mUNM.setUpstreamConfig(true /* autoUpstream */, false /* dunRequired */);
        mUNM.setTryCell(true);

        // [0] Mobile connects, DUN not required -> mobile selected.
        final TestNetworkAgent cellAgent = new TestNetworkAgent(mCM, CELL_CAPABILITIES);
        cellAgent.fakeConnect();
        mCM.makeDefaultNetwork(cellAgent);
        mLooper.dispatchAll();
        assertEquals(cellAgent.networkId, mUNM.getCurrentPreferredUpstream().network);
        assertEquals(0, mCM.mRequested.size());

        // [1] Mobile connects but not permitted -> null selected
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(false);
        assertEquals(null, mUNM.getCurrentPreferredUpstream());
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(true);
        assertEquals(0, mCM.mRequested.size());

        // [2] WiFi connects but not validated/promoted to default -> mobile selected.
        final TestNetworkAgent wifiAgent = new TestNetworkAgent(mCM, WIFI_CAPABILITIES);
        wifiAgent.fakeConnect();
        mLooper.dispatchAll();
        assertEquals(cellAgent.networkId, mUNM.getCurrentPreferredUpstream().network);
        assertEquals(0, mCM.mRequested.size());

        // [3] WiFi validates and is promoted to the default network -> WiFi selected.
        mCM.makeDefaultNetwork(wifiAgent);
        mLooper.dispatchAll();
        assertEquals(wifiAgent.networkId, mUNM.getCurrentPreferredUpstream().network);
        assertEquals(0, mCM.mRequested.size());

        // [4] DUN required, no other changes -> WiFi still selected
        mUNM.setUpstreamConfig(false /* autoUpstream */, true /* dunRequired */);
        assertEquals(wifiAgent.networkId, mUNM.getCurrentPreferredUpstream().network);
        assertEquals(1, mCM.mRequested.size());
        assertTrue(isDunRequested());

        // [5] WiFi no longer validated, mobile becomes default, DUN required -> null selected.
        mCM.makeDefaultNetwork(cellAgent);
        mLooper.dispatchAll();
        assertEquals(null, mUNM.getCurrentPreferredUpstream());
        assertEquals(1, mCM.mRequested.size());
        assertTrue(isDunRequested());

        // [6] DUN network arrives -> DUN selected
        final TestNetworkAgent dunAgent = new TestNetworkAgent(mCM, CELL_CAPABILITIES);
        dunAgent.networkCapabilities.addCapability(NET_CAPABILITY_DUN);
        dunAgent.networkCapabilities.removeCapability(NET_CAPABILITY_INTERNET);
        dunAgent.fakeConnect();
        mLooper.dispatchAll();
        assertEquals(dunAgent.networkId, mUNM.getCurrentPreferredUpstream().network);
        assertEquals(1, mCM.mRequested.size());

        // [7] Mobile is not permitted -> null selected
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(false);
        assertEquals(null, mUNM.getCurrentPreferredUpstream());
        assertEquals(1, mCM.mRequested.size());

        // [7] Mobile is permitted again -> DUN selected
        when(mEntitleMgr.isCellularUpstreamPermitted()).thenReturn(true);
        assertEquals(dunAgent.networkId, mUNM.getCurrentPreferredUpstream().network);
        assertEquals(1, mCM.mRequested.size());

        // [8] DUN no longer required -> request is withdrawn
        mUNM.setUpstreamConfig(true /* autoUpstream */, false /* dunRequired */);
        assertEquals(0, mCM.mRequested.size());
        assertFalse(isDunRequested());
    }

    @Test
    public void testLocalPrefixes() throws Exception {
        mUNM.startTrackDefaultNetwork(mEntitleMgr);
        mUNM.startObserveAllNetworks();

        // [0] Test minimum set of local prefixes.
        Set<IpPrefix> local = mUNM.getLocalPrefixes();
        assertTrue(local.isEmpty());

        final Set<String> alreadySeen = new HashSet<>();

        // [1] Pretend Wi-Fi connects.
        final TestNetworkAgent wifiAgent = new TestNetworkAgent(mCM, WIFI_CAPABILITIES);
        final LinkProperties wifiLp = wifiAgent.linkProperties;
        wifiLp.setInterfaceName("wlan0");
        final String[] wifi_addrs = {
                "fe80::827a:bfff:fe6f:374d", "100.112.103.18",
                "2001:db8:4:fd00:827a:bfff:fe6f:374d",
                "2001:db8:4:fd00:6dea:325a:fdae:4ef4",
                "fd6a:a640:60bf:e985::123",  // ULA address for good measure.
        };
        for (String addrStr : wifi_addrs) {
            final String cidr = addrStr.contains(":") ? "/64" : "/20";
            wifiLp.addLinkAddress(new LinkAddress(addrStr + cidr));
        }
        wifiAgent.fakeConnect();
        wifiAgent.sendLinkProperties();
        mLooper.dispatchAll();

        local = mUNM.getLocalPrefixes();
        assertPrefixSet(local, INCLUDES, alreadySeen);
        final String[] wifiLinkPrefixes = {
                // Link-local prefixes are excluded and dealt with elsewhere.
                "100.112.96.0/20", "2001:db8:4:fd00::/64", "fd6a:a640:60bf:e985::/64",
        };
        assertPrefixSet(local, INCLUDES, wifiLinkPrefixes);
        Collections.addAll(alreadySeen, wifiLinkPrefixes);
        assertEquals(alreadySeen.size(), local.size());

        // [2] Pretend mobile connects.
        final TestNetworkAgent cellAgent = new TestNetworkAgent(mCM, CELL_CAPABILITIES);
        final LinkProperties cellLp = cellAgent.linkProperties;
        cellLp.setInterfaceName("rmnet_data0");
        final String[] cell_addrs = {
                "10.102.211.48", "2001:db8:0:1:b50e:70d9:10c9:433d",
        };
        for (String addrStr : cell_addrs) {
            final String cidr = addrStr.contains(":") ? "/64" : "/27";
            cellLp.addLinkAddress(new LinkAddress(addrStr + cidr));
        }
        cellAgent.fakeConnect();
        cellAgent.sendLinkProperties();
        mLooper.dispatchAll();

        local = mUNM.getLocalPrefixes();
        assertPrefixSet(local, INCLUDES, alreadySeen);
        final String[] cellLinkPrefixes = { "10.102.211.32/27", "2001:db8:0:1::/64" };
        assertPrefixSet(local, INCLUDES, cellLinkPrefixes);
        Collections.addAll(alreadySeen, cellLinkPrefixes);
        assertEquals(alreadySeen.size(), local.size());

        // [3] Pretend DUN connects.
        final TestNetworkAgent dunAgent = new TestNetworkAgent(mCM, DUN_CAPABILITIES);
        final LinkProperties dunLp = dunAgent.linkProperties;
        dunLp.setInterfaceName("rmnet_data1");
        final String[] dun_addrs = {
                "192.0.2.48", "2001:db8:1:2:b50e:70d9:10c9:433d",
        };
        for (String addrStr : dun_addrs) {
            final String cidr = addrStr.contains(":") ? "/64" : "/27";
            dunLp.addLinkAddress(new LinkAddress(addrStr + cidr));
        }
        dunAgent.fakeConnect();
        dunAgent.sendLinkProperties();
        mLooper.dispatchAll();

        local = mUNM.getLocalPrefixes();
        assertPrefixSet(local, INCLUDES, alreadySeen);
        final String[] dunLinkPrefixes = { "192.0.2.32/27", "2001:db8:1:2::/64" };
        assertPrefixSet(local, INCLUDES, dunLinkPrefixes);
        Collections.addAll(alreadySeen, dunLinkPrefixes);
        assertEquals(alreadySeen.size(), local.size());

        // [4] Pretend Wi-Fi disconnected.  It's addresses/prefixes should no
        // longer be included (should be properly removed).
        wifiAgent.fakeDisconnect();
        mLooper.dispatchAll();
        local = mUNM.getLocalPrefixes();
        assertPrefixSet(local, EXCLUDES, wifiLinkPrefixes);
        assertPrefixSet(local, INCLUDES, cellLinkPrefixes);
        assertPrefixSet(local, INCLUDES, dunLinkPrefixes);

        // [5] Pretend mobile disconnected.
        cellAgent.fakeDisconnect();
        mLooper.dispatchAll();
        local = mUNM.getLocalPrefixes();
        assertPrefixSet(local, EXCLUDES, wifiLinkPrefixes);
        assertPrefixSet(local, EXCLUDES, cellLinkPrefixes);
        assertPrefixSet(local, INCLUDES, dunLinkPrefixes);

        // [6] Pretend DUN disconnected.
        dunAgent.fakeDisconnect();
        mLooper.dispatchAll();
        local = mUNM.getLocalPrefixes();
        assertTrue(local.isEmpty());
    }

    @Test
    public void testSelectMobileWhenMobileIsNotDefault() {
        final Collection<Integer> preferredTypes = new ArrayList<>();
        // Mobile has higher pirority than wifi.
        preferredTypes.add(TYPE_MOBILE_HIPRI);
        preferredTypes.add(TYPE_WIFI);
        mUNM.startTrackDefaultNetwork(mEntitleMgr);
        mUNM.startObserveAllNetworks();
        // Setup wifi and make wifi as default network.
        final TestNetworkAgent wifiAgent = new TestNetworkAgent(mCM, WIFI_CAPABILITIES);
        wifiAgent.fakeConnect();
        mCM.makeDefaultNetwork(wifiAgent);
        // Setup mobile network.
        final TestNetworkAgent cellAgent = new TestNetworkAgent(mCM, CELL_CAPABILITIES);
        cellAgent.fakeConnect();
        mLooper.dispatchAll();

        assertSatisfiesLegacyType(TYPE_MOBILE_HIPRI,
                mUNM.selectPreferredUpstreamType(preferredTypes));
        verify(mEntitleMgr, times(1)).maybeRunProvisioning();
    }

    private void assertSatisfiesLegacyType(int legacyType, UpstreamNetworkState ns) {
        if (legacyType == TYPE_NONE) {
            assertTrue(ns == null);
            return;
        }

        final NetworkCapabilities nc =
                UpstreamNetworkMonitor.networkCapabilitiesForType(legacyType);
        assertTrue(nc.satisfiedByNetworkCapabilities(ns.networkCapabilities));
    }

    private void assertUpstreamTypeRequested(int upstreamType) throws Exception {
        assertEquals(1, mCM.mRequested.size());
        assertEquals(1, mCM.mLegacyTypeMap.size());
        assertEquals(Integer.valueOf(upstreamType),
                mCM.mLegacyTypeMap.values().iterator().next());
    }

    private boolean isDunRequested() {
        for (NetworkRequestInfo nri : mCM.mRequested.values()) {
            if (nri.request.networkCapabilities.hasCapability(NET_CAPABILITY_DUN)) {
                return true;
            }
        }
        return false;
    }

    public static class TestStateMachine extends StateMachine {
        public final ArrayList<Message> messages = new ArrayList<>();
        private final State mLoggingState = new LoggingState();

        class LoggingState extends State {
            @Override public void enter() {
                messages.clear();
            }

            @Override public void exit() {
                messages.clear();
            }

            @Override public boolean processMessage(Message msg) {
                messages.add(msg);
                return true;
            }
        }

        public TestStateMachine(Looper looper) {
            super("UpstreamNetworkMonitor.TestStateMachine", looper);
            addState(mLoggingState);
            setInitialState(mLoggingState);
            super.start();
        }
    }

    static void assertPrefixSet(Set<IpPrefix> prefixes, boolean expectation, String... expected) {
        final Set<String> expectedSet = new HashSet<>();
        Collections.addAll(expectedSet, expected);
        assertPrefixSet(prefixes, expectation, expectedSet);
    }

    static void assertPrefixSet(Set<IpPrefix> prefixes, boolean expectation, Set<String> expected) {
        for (String expectedPrefix : expected) {
            final String errStr = expectation ? "did not find" : "found";
            assertEquals(
                    String.format("Failed expectation: %s prefix: %s", errStr, expectedPrefix),
                    expectation, prefixes.contains(new IpPrefix(expectedPrefix)));
        }
    }
}
