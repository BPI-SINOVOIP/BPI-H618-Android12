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

package com.android.networkstack.tethering;

import static android.net.NetworkStats.DEFAULT_NETWORK_NO;
import static android.net.NetworkStats.METERED_NO;
import static android.net.NetworkStats.ROAMING_NO;
import static android.net.NetworkStats.SET_DEFAULT;
import static android.net.NetworkStats.TAG_NONE;
import static android.net.NetworkStats.UID_ALL;
import static android.net.NetworkStats.UID_TETHERING;
import static android.net.ip.ConntrackMonitor.ConntrackEvent;
import static android.net.netlink.ConntrackMessage.DYING_MASK;
import static android.net.netlink.ConntrackMessage.ESTABLISHED_MASK;
import static android.net.netlink.ConntrackMessage.Tuple;
import static android.net.netlink.ConntrackMessage.TupleIpv4;
import static android.net.netlink.ConntrackMessage.TupleProto;
import static android.net.netlink.NetlinkConstants.IPCTNL_MSG_CT_DELETE;
import static android.net.netlink.NetlinkConstants.IPCTNL_MSG_CT_NEW;
import static android.net.netstats.provider.NetworkStatsProvider.QUOTA_UNLIMITED;
import static android.system.OsConstants.ETH_P_IP;
import static android.system.OsConstants.ETH_P_IPV6;
import static android.system.OsConstants.IPPROTO_TCP;
import static android.system.OsConstants.IPPROTO_UDP;
import static android.system.OsConstants.NETLINK_NETFILTER;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.staticMockMarker;
import static com.android.networkstack.tethering.BpfCoordinator.NF_CONNTRACK_TCP_TIMEOUT_ESTABLISHED;
import static com.android.networkstack.tethering.BpfCoordinator.NF_CONNTRACK_UDP_TIMEOUT_STREAM;
import static com.android.networkstack.tethering.BpfCoordinator.POLLING_CONNTRACK_TIMEOUT_MS;
import static com.android.networkstack.tethering.BpfCoordinator.StatsType;
import static com.android.networkstack.tethering.BpfCoordinator.StatsType.STATS_PER_IFACE;
import static com.android.networkstack.tethering.BpfCoordinator.StatsType.STATS_PER_UID;
import static com.android.networkstack.tethering.BpfUtils.DOWNSTREAM;
import static com.android.networkstack.tethering.BpfUtils.UPSTREAM;
import static com.android.networkstack.tethering.TetheringConfiguration.DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.argThat;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.usage.NetworkStatsManager;
import android.net.INetd;
import android.net.InetAddresses;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.MacAddress;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkStats;
import android.net.TetherOffloadRuleParcel;
import android.net.TetherStatsParcel;
import android.net.ip.ConntrackMonitor;
import android.net.ip.ConntrackMonitor.ConntrackEventConsumer;
import android.net.ip.IpServer;
import android.net.netlink.ConntrackMessage;
import android.net.netlink.NetlinkConstants;
import android.net.netlink.NetlinkSocket;
import android.net.util.InterfaceParams;
import android.net.util.SharedLog;
import android.os.Build;
import android.os.Handler;
import android.os.test.TestLooper;
import android.system.ErrnoException;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.dx.mockito.inline.extended.ExtendedMockito;
import com.android.net.module.util.NetworkStackConstants;
import com.android.net.module.util.Struct;
import com.android.networkstack.tethering.BpfCoordinator.BpfConntrackEventConsumer;
import com.android.networkstack.tethering.BpfCoordinator.ClientInfo;
import com.android.networkstack.tethering.BpfCoordinator.Ipv6ForwardingRule;
import com.android.testutils.DevSdkIgnoreRule;
import com.android.testutils.DevSdkIgnoreRule.IgnoreAfter;
import com.android.testutils.DevSdkIgnoreRule.IgnoreUpTo;
import com.android.testutils.TestableNetworkStatsProviderCbBinder;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.function.BiConsumer;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class BpfCoordinatorTest {
    @Rule
    public final DevSdkIgnoreRule mIgnoreRule = new DevSdkIgnoreRule();

    private static final int TEST_NET_ID = 24;

    private static final int UPSTREAM_IFINDEX = 1001;
    private static final int DOWNSTREAM_IFINDEX = 1002;

    private static final String UPSTREAM_IFACE = "rmnet0";

    private static final MacAddress DOWNSTREAM_MAC = MacAddress.fromString("12:34:56:78:90:ab");
    private static final MacAddress MAC_A = MacAddress.fromString("00:00:00:00:00:0a");
    private static final MacAddress MAC_B = MacAddress.fromString("11:22:33:00:00:0b");

    private static final InetAddress NEIGH_A = InetAddresses.parseNumericAddress("2001:db8::1");
    private static final InetAddress NEIGH_B = InetAddresses.parseNumericAddress("2001:db8::2");

    private static final InterfaceParams UPSTREAM_IFACE_PARAMS = new InterfaceParams(
            UPSTREAM_IFACE, UPSTREAM_IFINDEX, null /* macAddr, rawip */,
            NetworkStackConstants.ETHER_MTU);

    // The test fake BPF map class is needed because the test has no privilege to access the BPF
    // map. All member functions which eventually call JNI to access the real native BPF map need
    // to be overridden.
    // TODO: consider moving to an individual file.
    private class TestBpfMap<K extends Struct, V extends Struct> extends BpfMap<K, V> {
        private final HashMap<K, V> mMap = new HashMap<K, V>();

        TestBpfMap(final Class<K> key, final Class<V> value) {
            super(key, value);
        }

        @Override
        public void forEach(BiConsumer<K, V> action) throws ErrnoException {
            // TODO: consider using mocked #getFirstKey and #getNextKey to iterate. It helps to
            // implement the entry deletion in the iteration if required.
            for (Map.Entry<K, V> entry : mMap.entrySet()) {
                action.accept(entry.getKey(), entry.getValue());
            }
        }

        @Override
        public void updateEntry(K key, V value) throws ErrnoException {
            mMap.put(key, value);
        }

        @Override
        public void insertEntry(K key, V value) throws ErrnoException,
                IllegalArgumentException {
            // The entry is created if and only if it doesn't exist. See BpfMap#insertEntry.
            if (mMap.get(key) != null) {
                throw new IllegalArgumentException(key + " already exist");
            }
            mMap.put(key, value);
        }

        @Override
        public boolean deleteEntry(Struct key) throws ErrnoException {
            return mMap.remove(key) != null;
        }

        @Override
        public V getValue(@NonNull K key) throws ErrnoException {
            // Return value for a given key. Otherwise, return null without an error ENOENT.
            // BpfMap#getValue treats that the entry is not found as no error.
            return mMap.get(key);
        }

        @Override
        public void clear() throws ErrnoException {
            // TODO: consider using mocked #getFirstKey and #deleteEntry to implement.
            mMap.clear();
        }
    };

    @Mock private NetworkStatsManager mStatsManager;
    @Mock private INetd mNetd;
    @Mock private IpServer mIpServer;
    @Mock private IpServer mIpServer2;
    @Mock private TetheringConfiguration mTetherConfig;
    @Mock private ConntrackMonitor mConntrackMonitor;
    @Mock private BpfMap<Tether4Key, Tether4Value> mBpfDownstream4Map;
    @Mock private BpfMap<Tether4Key, Tether4Value> mBpfUpstream4Map;
    @Mock private BpfMap<TetherDownstream6Key, Tether6Value> mBpfDownstream6Map;
    @Mock private BpfMap<TetherUpstream6Key, Tether6Value> mBpfUpstream6Map;
    @Mock private BpfMap<TetherDevKey, TetherDevValue> mBpfDevMap;

    // Late init since methods must be called by the thread that created this object.
    private TestableNetworkStatsProviderCbBinder mTetherStatsProviderCb;
    private BpfCoordinator.BpfTetherStatsProvider mTetherStatsProvider;

    // Late init since the object must be initialized by the BPF coordinator instance because
    // it has to access the non-static function of BPF coordinator.
    private BpfConntrackEventConsumer mConsumer;

    private long mElapsedRealtimeNanos = 0;
    private final ArgumentCaptor<ArrayList> mStringArrayCaptor =
            ArgumentCaptor.forClass(ArrayList.class);
    private final TestLooper mTestLooper = new TestLooper();
    private final TestBpfMap<TetherStatsKey, TetherStatsValue> mBpfStatsMap =
            spy(new TestBpfMap<>(TetherStatsKey.class, TetherStatsValue.class));
    private final TestBpfMap<TetherLimitKey, TetherLimitValue> mBpfLimitMap =
            spy(new TestBpfMap<>(TetherLimitKey.class, TetherLimitValue.class));
    private BpfCoordinator.Dependencies mDeps =
            spy(new BpfCoordinator.Dependencies() {
                    @NonNull
                    public Handler getHandler() {
                        return new Handler(mTestLooper.getLooper());
                    }

                    @NonNull
                    public INetd getNetd() {
                        return mNetd;
                    }

                    @NonNull
                    public NetworkStatsManager getNetworkStatsManager() {
                        return mStatsManager;
                    }

                    @NonNull
                    public SharedLog getSharedLog() {
                        return new SharedLog("test");
                    }

                    @Nullable
                    public TetheringConfiguration getTetherConfig() {
                        return mTetherConfig;
                    }

                    @NonNull
                    public ConntrackMonitor getConntrackMonitor(ConntrackEventConsumer consumer) {
                        return mConntrackMonitor;
                    }

                    public long elapsedRealtimeNanos() {
                        return mElapsedRealtimeNanos;
                    }

                    @Nullable
                    public BpfMap<Tether4Key, Tether4Value> getBpfDownstream4Map() {
                        return mBpfDownstream4Map;
                    }

                    @Nullable
                    public BpfMap<Tether4Key, Tether4Value> getBpfUpstream4Map() {
                        return mBpfUpstream4Map;
                    }

                    @Nullable
                    public BpfMap<TetherDownstream6Key, Tether6Value> getBpfDownstream6Map() {
                        return mBpfDownstream6Map;
                    }

                    @Nullable
                    public BpfMap<TetherUpstream6Key, Tether6Value> getBpfUpstream6Map() {
                        return mBpfUpstream6Map;
                    }

                    @Nullable
                    public BpfMap<TetherStatsKey, TetherStatsValue> getBpfStatsMap() {
                        return mBpfStatsMap;
                    }

                    @Nullable
                    public BpfMap<TetherLimitKey, TetherLimitValue> getBpfLimitMap() {
                        return mBpfLimitMap;
                    }

                    @Nullable
                    public BpfMap<TetherDevKey, TetherDevValue> getBpfDevMap() {
                        return mBpfDevMap;
                    }
            });

    @Before public void setUp() {
        MockitoAnnotations.initMocks(this);
        when(mTetherConfig.isBpfOffloadEnabled()).thenReturn(true /* default value */);
    }

    private void waitForIdle() {
        mTestLooper.dispatchAll();
    }

    // TODO: Remove unnecessary calling on R because the BPF map accessing has been moved into
    // module.
    private void setupFunctioningNetdInterface() throws Exception {
        when(mNetd.tetherOffloadGetStats()).thenReturn(new TetherStatsParcel[0]);
    }

    @NonNull
    private BpfCoordinator makeBpfCoordinator() throws Exception {
        final BpfCoordinator coordinator = new BpfCoordinator(mDeps);

        mConsumer = coordinator.getBpfConntrackEventConsumerForTesting();
        final ArgumentCaptor<BpfCoordinator.BpfTetherStatsProvider>
                tetherStatsProviderCaptor =
                ArgumentCaptor.forClass(BpfCoordinator.BpfTetherStatsProvider.class);
        verify(mStatsManager).registerNetworkStatsProvider(anyString(),
                tetherStatsProviderCaptor.capture());
        mTetherStatsProvider = tetherStatsProviderCaptor.getValue();
        assertNotNull(mTetherStatsProvider);
        mTetherStatsProviderCb = new TestableNetworkStatsProviderCbBinder();
        mTetherStatsProvider.setProviderCallbackBinder(mTetherStatsProviderCb);

        return coordinator;
    }

    @NonNull
    private static NetworkStats.Entry buildTestEntry(@NonNull StatsType how,
            @NonNull String iface, long rxBytes, long rxPackets, long txBytes, long txPackets) {
        return new NetworkStats.Entry(iface, how == STATS_PER_IFACE ? UID_ALL : UID_TETHERING,
                SET_DEFAULT, TAG_NONE, METERED_NO, ROAMING_NO, DEFAULT_NETWORK_NO, rxBytes,
                rxPackets, txBytes, txPackets, 0L);
    }

    @NonNull
    private static TetherStatsParcel buildTestTetherStatsParcel(@NonNull Integer ifIndex,
            long rxBytes, long rxPackets, long txBytes, long txPackets) {
        final TetherStatsParcel parcel = new TetherStatsParcel();
        parcel.ifIndex = ifIndex;
        parcel.rxBytes = rxBytes;
        parcel.rxPackets = rxPackets;
        parcel.txBytes = txBytes;
        parcel.txPackets = txPackets;
        return parcel;
    }

    // Update a stats entry or create if not exists.
    private void updateStatsEntryToStatsMap(@NonNull TetherStatsParcel stats) throws Exception {
        final TetherStatsKey key = new TetherStatsKey(stats.ifIndex);
        final TetherStatsValue value = new TetherStatsValue(stats.rxPackets, stats.rxBytes,
                0L /* rxErrors */, stats.txPackets, stats.txBytes, 0L /* txErrors */);
        mBpfStatsMap.updateEntry(key, value);
    }

    private void updateStatsEntry(@NonNull TetherStatsParcel stats) throws Exception {
        if (mDeps.isAtLeastS()) {
            updateStatsEntryToStatsMap(stats);
        } else {
            when(mNetd.tetherOffloadGetStats()).thenReturn(new TetherStatsParcel[] {stats});
        }
    }

    // Update specific tether stats list and wait for the stats cache is updated by polling thread
    // in the coordinator. Beware of that it is only used for the default polling interval.
    // Note that the mocked tetherOffloadGetStats of netd replaces all stats entries because it
    // doesn't store the previous entries.
    private void updateStatsEntriesAndWaitForUpdate(@NonNull TetherStatsParcel[] tetherStatsList)
            throws Exception {
        if (mDeps.isAtLeastS()) {
            for (TetherStatsParcel stats : tetherStatsList) {
                updateStatsEntry(stats);
            }
        } else {
            when(mNetd.tetherOffloadGetStats()).thenReturn(tetherStatsList);
        }

        mTestLooper.moveTimeForward(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS);
        waitForIdle();
    }

    // In tests, the stats need to be set before deleting the last rule.
    // The reason is that BpfCoordinator#tetherOffloadRuleRemove reads the stats
    // of the deleting interface after the last rule deleted. #tetherOffloadRuleRemove
    // does the interface cleanup failed if there is no stats for the deleting interface.
    // Note that the mocked tetherOffloadGetAndClearStats of netd replaces all stats entries
    // because it doesn't store the previous entries.
    private void updateStatsEntryForTetherOffloadGetAndClearStats(TetherStatsParcel stats)
            throws Exception {
        if (mDeps.isAtLeastS()) {
            updateStatsEntryToStatsMap(stats);
        } else {
            when(mNetd.tetherOffloadGetAndClearStats(stats.ifIndex)).thenReturn(stats);
        }
    }

    private void clearStatsInvocations() {
        if (mDeps.isAtLeastS()) {
            clearInvocations(mBpfStatsMap);
        } else {
            clearInvocations(mNetd);
        }
    }

    private <T> T verifyWithOrder(@Nullable InOrder inOrder, @NonNull T t) {
        if (inOrder != null) {
            return inOrder.verify(t);
        } else {
            return verify(t);
        }
    }

    private void verifyTetherOffloadGetStats() throws Exception {
        if (mDeps.isAtLeastS()) {
            verify(mBpfStatsMap).forEach(any());
        } else {
            verify(mNetd).tetherOffloadGetStats();
        }
    }

    private void verifyNeverTetherOffloadGetStats() throws Exception {
        if (mDeps.isAtLeastS()) {
            verify(mBpfStatsMap, never()).forEach(any());
        } else {
            verify(mNetd, never()).tetherOffloadGetStats();
        }
    }

    private void verifyStartUpstreamIpv6Forwarding(@Nullable InOrder inOrder, int downstreamIfIndex,
            MacAddress downstreamMac, int upstreamIfindex) throws Exception {
        if (!mDeps.isAtLeastS()) return;
        final TetherUpstream6Key key = new TetherUpstream6Key(downstreamIfIndex, downstreamMac);
        final Tether6Value value = new Tether6Value(upstreamIfindex,
                MacAddress.ALL_ZEROS_ADDRESS, MacAddress.ALL_ZEROS_ADDRESS,
                ETH_P_IPV6, NetworkStackConstants.ETHER_MTU);
        verifyWithOrder(inOrder, mBpfUpstream6Map).insertEntry(key, value);
    }

    private void verifyStopUpstreamIpv6Forwarding(@Nullable InOrder inOrder, int downstreamIfIndex,
            MacAddress downstreamMac)
            throws Exception {
        if (!mDeps.isAtLeastS()) return;
        final TetherUpstream6Key key = new TetherUpstream6Key(downstreamIfIndex, downstreamMac);
        verifyWithOrder(inOrder, mBpfUpstream6Map).deleteEntry(key);
    }

    private void verifyNoUpstreamIpv6ForwardingChange(@Nullable InOrder inOrder) throws Exception {
        if (!mDeps.isAtLeastS()) return;
        if (inOrder != null) {
            inOrder.verify(mBpfUpstream6Map, never()).deleteEntry(any());
            inOrder.verify(mBpfUpstream6Map, never()).insertEntry(any(), any());
            inOrder.verify(mBpfUpstream6Map, never()).updateEntry(any(), any());
        } else {
            verify(mBpfUpstream6Map, never()).deleteEntry(any());
            verify(mBpfUpstream6Map, never()).insertEntry(any(), any());
            verify(mBpfUpstream6Map, never()).updateEntry(any(), any());
        }
    }

    private void verifyTetherOffloadRuleAdd(@Nullable InOrder inOrder,
            @NonNull Ipv6ForwardingRule rule) throws Exception {
        if (mDeps.isAtLeastS()) {
            verifyWithOrder(inOrder, mBpfDownstream6Map).updateEntry(
                    rule.makeTetherDownstream6Key(), rule.makeTether6Value());
        } else {
            verifyWithOrder(inOrder, mNetd).tetherOffloadRuleAdd(matches(rule));
        }
    }

    private void verifyNeverTetherOffloadRuleAdd() throws Exception {
        if (mDeps.isAtLeastS()) {
            verify(mBpfDownstream6Map, never()).updateEntry(any(), any());
        } else {
            verify(mNetd, never()).tetherOffloadRuleAdd(any());
        }
    }

    private void verifyTetherOffloadRuleRemove(@Nullable InOrder inOrder,
            @NonNull final Ipv6ForwardingRule rule) throws Exception {
        if (mDeps.isAtLeastS()) {
            verifyWithOrder(inOrder, mBpfDownstream6Map).deleteEntry(
                    rule.makeTetherDownstream6Key());
        } else {
            verifyWithOrder(inOrder, mNetd).tetherOffloadRuleRemove(matches(rule));
        }
    }

    private void verifyNeverTetherOffloadRuleRemove() throws Exception {
        if (mDeps.isAtLeastS()) {
            verify(mBpfDownstream6Map, never()).deleteEntry(any());
        } else {
            verify(mNetd, never()).tetherOffloadRuleRemove(any());
        }
    }

    private void verifyTetherOffloadSetInterfaceQuota(@Nullable InOrder inOrder, int ifIndex,
            long quotaBytes, boolean isInit) throws Exception {
        if (mDeps.isAtLeastS()) {
            final TetherStatsKey key = new TetherStatsKey(ifIndex);
            verifyWithOrder(inOrder, mBpfStatsMap).getValue(key);
            if (isInit) {
                verifyWithOrder(inOrder, mBpfStatsMap).insertEntry(key, new TetherStatsValue(
                        0L /* rxPackets */, 0L /* rxBytes */, 0L /* rxErrors */,
                        0L /* txPackets */, 0L /* txBytes */, 0L /* txErrors */));
            }
            verifyWithOrder(inOrder, mBpfLimitMap).updateEntry(new TetherLimitKey(ifIndex),
                    new TetherLimitValue(quotaBytes));
        } else {
            verifyWithOrder(inOrder, mNetd).tetherOffloadSetInterfaceQuota(ifIndex, quotaBytes);
        }
    }

    private void verifyNeverTetherOffloadSetInterfaceQuota(@NonNull InOrder inOrder)
            throws Exception {
        if (mDeps.isAtLeastS()) {
            inOrder.verify(mBpfStatsMap, never()).getValue(any());
            inOrder.verify(mBpfStatsMap, never()).insertEntry(any(), any());
            inOrder.verify(mBpfLimitMap, never()).updateEntry(any(), any());
        } else {
            inOrder.verify(mNetd, never()).tetherOffloadSetInterfaceQuota(anyInt(), anyLong());
        }
    }

    private void verifyTetherOffloadGetAndClearStats(@NonNull InOrder inOrder, int ifIndex)
            throws Exception {
        if (mDeps.isAtLeastS()) {
            inOrder.verify(mBpfStatsMap).getValue(new TetherStatsKey(ifIndex));
            inOrder.verify(mBpfStatsMap).deleteEntry(new TetherStatsKey(ifIndex));
            inOrder.verify(mBpfLimitMap).deleteEntry(new TetherLimitKey(ifIndex));
        } else {
            inOrder.verify(mNetd).tetherOffloadGetAndClearStats(ifIndex);
        }
    }

    // S+ and R api minimum tests.
    // The following tests are used to provide minimum checking for the APIs on different flow.
    // The auto merge is not enabled on mainline prod. The code flow R may be verified at the
    // late stage by manual cherry pick. It is risky if the R code flow has broken and be found at
    // the last minute.
    // TODO: remove once presubmit tests on R even the code is submitted on S.
    private void checkTetherOffloadRuleAddAndRemove(boolean usingApiS) throws Exception {
        setupFunctioningNetdInterface();

        // Replace Dependencies#isAtLeastS() for testing R and S+ BPF map apis. Note that |mDeps|
        // must be mocked before calling #makeBpfCoordinator which use |mDeps| to initialize the
        // coordinator.
        doReturn(usingApiS).when(mDeps).isAtLeastS();
        final BpfCoordinator coordinator = makeBpfCoordinator();

        final String mobileIface = "rmnet_data0";
        final Integer mobileIfIndex = 100;
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        // InOrder is required because mBpfStatsMap may be accessed by both
        // BpfCoordinator#tetherOffloadRuleAdd and BpfCoordinator#tetherOffloadGetAndClearStats.
        // The #verifyTetherOffloadGetAndClearStats can't distinguish who has ever called
        // mBpfStatsMap#getValue and get a wrong calling count which counts all.
        final InOrder inOrder = inOrder(mNetd, mBpfDownstream6Map, mBpfLimitMap, mBpfStatsMap);
        final Ipv6ForwardingRule rule = buildTestForwardingRule(mobileIfIndex, NEIGH_A, MAC_A);
        coordinator.tetherOffloadRuleAdd(mIpServer, rule);
        verifyTetherOffloadRuleAdd(inOrder, rule);
        verifyTetherOffloadSetInterfaceQuota(inOrder, mobileIfIndex, QUOTA_UNLIMITED,
                true /* isInit */);

        // Removing the last rule on current upstream immediately sends the cleanup stuff to netd.
        updateStatsEntryForTetherOffloadGetAndClearStats(
                buildTestTetherStatsParcel(mobileIfIndex, 0, 0, 0, 0));
        coordinator.tetherOffloadRuleRemove(mIpServer, rule);
        verifyTetherOffloadRuleRemove(inOrder, rule);
        verifyTetherOffloadGetAndClearStats(inOrder, mobileIfIndex);
    }

    // TODO: remove once presubmit tests on R even the code is submitted on S.
    @Test
    public void testTetherOffloadRuleAddAndRemoveSdkR() throws Exception {
        checkTetherOffloadRuleAddAndRemove(false /* R */);
    }

    // TODO: remove once presubmit tests on R even the code is submitted on S.
    @Test
    public void testTetherOffloadRuleAddAndRemoveAtLeastSdkS() throws Exception {
        checkTetherOffloadRuleAddAndRemove(true /* S+ */);
    }

    // TODO: remove once presubmit tests on R even the code is submitted on S.
    private void checkTetherOffloadGetStats(boolean usingApiS) throws Exception {
        setupFunctioningNetdInterface();

        doReturn(usingApiS).when(mDeps).isAtLeastS();
        final BpfCoordinator coordinator = makeBpfCoordinator();
        coordinator.startPolling();

        final String mobileIface = "rmnet_data0";
        final Integer mobileIfIndex = 100;
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        updateStatsEntriesAndWaitForUpdate(new TetherStatsParcel[] {
                buildTestTetherStatsParcel(mobileIfIndex, 1000, 100, 2000, 200)});

        final NetworkStats expectedIfaceStats = new NetworkStats(0L, 1)
                .addEntry(buildTestEntry(STATS_PER_IFACE, mobileIface, 1000, 100, 2000, 200));

        final NetworkStats expectedUidStats = new NetworkStats(0L, 1)
                .addEntry(buildTestEntry(STATS_PER_UID, mobileIface, 1000, 100, 2000, 200));

        mTetherStatsProvider.pushTetherStats();
        mTetherStatsProviderCb.expectNotifyStatsUpdated(expectedIfaceStats, expectedUidStats);
    }

    // TODO: remove once presubmit tests on R even the code is submitted on S.
    @Test
    public void testTetherOffloadGetStatsSdkR() throws Exception {
        checkTetherOffloadGetStats(false /* R */);
    }

    // TODO: remove once presubmit tests on R even the code is submitted on S.
    @Test
    public void testTetherOffloadGetStatsAtLeastSdkS() throws Exception {
        checkTetherOffloadGetStats(true /* S+ */);
    }

    @Test
    public void testGetForwardedStats() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();
        coordinator.startPolling();

        final String wlanIface = "wlan0";
        final Integer wlanIfIndex = 100;
        final String mobileIface = "rmnet_data0";
        final Integer mobileIfIndex = 101;

        // Add interface name to lookup table. In realistic case, the upstream interface name will
        // be added by IpServer when IpServer has received with a new IPv6 upstream update event.
        coordinator.addUpstreamNameToLookupTable(wlanIfIndex, wlanIface);
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        // [1] Both interface stats are changed.
        // Setup the tether stats of wlan and mobile interface. Note that move forward the time of
        // the looper to make sure the new tether stats has been updated by polling update thread.
        updateStatsEntriesAndWaitForUpdate(new TetherStatsParcel[] {
                buildTestTetherStatsParcel(wlanIfIndex, 1000, 100, 2000, 200),
                buildTestTetherStatsParcel(mobileIfIndex, 3000, 300, 4000, 400)});

        final NetworkStats expectedIfaceStats = new NetworkStats(0L, 2)
                .addEntry(buildTestEntry(STATS_PER_IFACE, wlanIface, 1000, 100, 2000, 200))
                .addEntry(buildTestEntry(STATS_PER_IFACE, mobileIface, 3000, 300, 4000, 400));

        final NetworkStats expectedUidStats = new NetworkStats(0L, 2)
                .addEntry(buildTestEntry(STATS_PER_UID, wlanIface, 1000, 100, 2000, 200))
                .addEntry(buildTestEntry(STATS_PER_UID, mobileIface, 3000, 300, 4000, 400));

        // Force pushing stats update to verify the stats reported.
        // TODO: Perhaps make #expectNotifyStatsUpdated to use test TetherStatsParcel object for
        // verifying the notification.
        mTetherStatsProvider.pushTetherStats();
        mTetherStatsProviderCb.expectNotifyStatsUpdated(expectedIfaceStats, expectedUidStats);

        // [2] Only one interface stats is changed.
        // The tether stats of mobile interface is accumulated and The tether stats of wlan
        // interface is the same.
        updateStatsEntriesAndWaitForUpdate(new TetherStatsParcel[] {
                buildTestTetherStatsParcel(wlanIfIndex, 1000, 100, 2000, 200),
                buildTestTetherStatsParcel(mobileIfIndex, 3010, 320, 4030, 440)});

        final NetworkStats expectedIfaceStatsDiff = new NetworkStats(0L, 2)
                .addEntry(buildTestEntry(STATS_PER_IFACE, wlanIface, 0, 0, 0, 0))
                .addEntry(buildTestEntry(STATS_PER_IFACE, mobileIface, 10, 20, 30, 40));

        final NetworkStats expectedUidStatsDiff = new NetworkStats(0L, 2)
                .addEntry(buildTestEntry(STATS_PER_UID, wlanIface, 0, 0, 0, 0))
                .addEntry(buildTestEntry(STATS_PER_UID, mobileIface, 10, 20, 30, 40));

        // Force pushing stats update to verify that only diff of stats is reported.
        mTetherStatsProvider.pushTetherStats();
        mTetherStatsProviderCb.expectNotifyStatsUpdated(expectedIfaceStatsDiff,
                expectedUidStatsDiff);

        // [3] Stop coordinator.
        // Shutdown the coordinator and clear the invocation history, especially the
        // tetherOffloadGetStats() calls.
        coordinator.stopPolling();
        clearStatsInvocations();

        // Verify the polling update thread stopped.
        mTestLooper.moveTimeForward(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS);
        waitForIdle();
        verifyNeverTetherOffloadGetStats();
    }

    @Test
    public void testOnSetAlert() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();
        coordinator.startPolling();

        final String mobileIface = "rmnet_data0";
        final Integer mobileIfIndex = 100;
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        // Verify that set quota to 0 will immediately triggers a callback.
        mTetherStatsProvider.onSetAlert(0);
        waitForIdle();
        mTetherStatsProviderCb.expectNotifyAlertReached();

        // Verify that notifyAlertReached never fired if quota is not yet reached.
        updateStatsEntry(buildTestTetherStatsParcel(mobileIfIndex, 0, 0, 0, 0));
        mTetherStatsProvider.onSetAlert(100);
        mTestLooper.moveTimeForward(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS);
        waitForIdle();
        mTetherStatsProviderCb.assertNoCallback();

        // Verify that notifyAlertReached fired when quota is reached.
        updateStatsEntry(buildTestTetherStatsParcel(mobileIfIndex, 50, 0, 50, 0));
        mTestLooper.moveTimeForward(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS);
        waitForIdle();
        mTetherStatsProviderCb.expectNotifyAlertReached();

        // Verify that set quota with UNLIMITED won't trigger any callback.
        mTetherStatsProvider.onSetAlert(QUOTA_UNLIMITED);
        mTestLooper.moveTimeForward(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS);
        waitForIdle();
        mTetherStatsProviderCb.assertNoCallback();
    }

    // The custom ArgumentMatcher simply comes from IpServerTest.
    // TODO: move both of them into a common utility class for reusing the code.
    private static class TetherOffloadRuleParcelMatcher implements
            ArgumentMatcher<TetherOffloadRuleParcel> {
        public final int upstreamIfindex;
        public final int downstreamIfindex;
        public final Inet6Address address;
        public final MacAddress srcMac;
        public final MacAddress dstMac;

        TetherOffloadRuleParcelMatcher(@NonNull Ipv6ForwardingRule rule) {
            upstreamIfindex = rule.upstreamIfindex;
            downstreamIfindex = rule.downstreamIfindex;
            address = rule.address;
            srcMac = rule.srcMac;
            dstMac = rule.dstMac;
        }

        public boolean matches(@NonNull TetherOffloadRuleParcel parcel) {
            return upstreamIfindex == parcel.inputInterfaceIndex
                    && (downstreamIfindex == parcel.outputInterfaceIndex)
                    && Arrays.equals(address.getAddress(), parcel.destination)
                    && (128 == parcel.prefixLength)
                    && Arrays.equals(srcMac.toByteArray(), parcel.srcL2Address)
                    && Arrays.equals(dstMac.toByteArray(), parcel.dstL2Address);
        }

        public String toString() {
            return String.format("TetherOffloadRuleParcelMatcher(%d, %d, %s, %s, %s",
                    upstreamIfindex, downstreamIfindex, address.getHostAddress(), srcMac, dstMac);
        }
    }

    @NonNull
    private TetherOffloadRuleParcel matches(@NonNull Ipv6ForwardingRule rule) {
        return argThat(new TetherOffloadRuleParcelMatcher(rule));
    }

    @NonNull
    private static Ipv6ForwardingRule buildTestForwardingRule(
            int upstreamIfindex, @NonNull InetAddress address, @NonNull MacAddress dstMac) {
        return new Ipv6ForwardingRule(upstreamIfindex, DOWNSTREAM_IFINDEX, (Inet6Address) address,
                DOWNSTREAM_MAC, dstMac);
    }

    @Test
    public void testRuleMakeTetherDownstream6Key() throws Exception {
        final Integer mobileIfIndex = 100;
        final Ipv6ForwardingRule rule = buildTestForwardingRule(mobileIfIndex, NEIGH_A, MAC_A);

        final TetherDownstream6Key key = rule.makeTetherDownstream6Key();
        assertEquals(key.iif, (long) mobileIfIndex);
        assertEquals(key.dstMac, MacAddress.ALL_ZEROS_ADDRESS);  // rawip upstream
        assertTrue(Arrays.equals(key.neigh6, NEIGH_A.getAddress()));
        // iif (4) + dstMac(6) + padding(2) + neigh6 (16) = 28.
        assertEquals(28, key.writeToBytes().length);
    }

    @Test
    public void testRuleMakeTether6Value() throws Exception {
        final Integer mobileIfIndex = 100;
        final Ipv6ForwardingRule rule = buildTestForwardingRule(mobileIfIndex, NEIGH_A, MAC_A);

        final Tether6Value value = rule.makeTether6Value();
        assertEquals(value.oif, DOWNSTREAM_IFINDEX);
        assertEquals(value.ethDstMac, MAC_A);
        assertEquals(value.ethSrcMac, DOWNSTREAM_MAC);
        assertEquals(value.ethProto, ETH_P_IPV6);
        assertEquals(value.pmtu, NetworkStackConstants.ETHER_MTU);
        // oif (4) + ethDstMac (6) + ethSrcMac (6) + ethProto (2) + pmtu (2) = 20.
        assertEquals(20, value.writeToBytes().length);
    }

    @Test
    public void testSetDataLimit() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        final String mobileIface = "rmnet_data0";
        final Integer mobileIfIndex = 100;
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        // [1] Default limit.
        // Set the unlimited quota as default if the service has never applied a data limit for a
        // given upstream. Note that the data limit only be applied on an upstream which has rules.
        final Ipv6ForwardingRule rule = buildTestForwardingRule(mobileIfIndex, NEIGH_A, MAC_A);
        final InOrder inOrder = inOrder(mNetd, mBpfDownstream6Map, mBpfLimitMap, mBpfStatsMap);
        coordinator.tetherOffloadRuleAdd(mIpServer, rule);
        verifyTetherOffloadRuleAdd(inOrder, rule);
        verifyTetherOffloadSetInterfaceQuota(inOrder, mobileIfIndex, QUOTA_UNLIMITED,
                true /* isInit */);
        inOrder.verifyNoMoreInteractions();

        // [2] Specific limit.
        // Applying the data limit boundary {min, 1gb, max, infinity} on current upstream.
        for (final long quota : new long[] {0, 1048576000, Long.MAX_VALUE, QUOTA_UNLIMITED}) {
            mTetherStatsProvider.onSetLimit(mobileIface, quota);
            waitForIdle();
            verifyTetherOffloadSetInterfaceQuota(inOrder, mobileIfIndex, quota,
                    false /* isInit */);
            inOrder.verifyNoMoreInteractions();
        }

        // [3] Invalid limit.
        // The valid range of quota is 0..max_int64 or -1 (unlimited).
        final long invalidLimit = Long.MIN_VALUE;
        try {
            mTetherStatsProvider.onSetLimit(mobileIface, invalidLimit);
            waitForIdle();
            fail("No exception thrown for invalid limit " + invalidLimit + ".");
        } catch (IllegalArgumentException expected) {
            assertEquals(expected.getMessage(), "invalid quota value " + invalidLimit);
        }
    }

    // TODO: Test the case in which the rules are changed from different IpServer objects.
    @Test
    public void testSetDataLimitOnRule6Change() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        final String mobileIface = "rmnet_data0";
        final Integer mobileIfIndex = 100;
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        // Applying a data limit to the current upstream does not take any immediate action.
        // The data limit could be only set on an upstream which has rules.
        final long limit = 12345;
        final InOrder inOrder = inOrder(mNetd, mBpfDownstream6Map, mBpfLimitMap, mBpfStatsMap);
        mTetherStatsProvider.onSetLimit(mobileIface, limit);
        waitForIdle();
        verifyNeverTetherOffloadSetInterfaceQuota(inOrder);

        // Adding the first rule on current upstream immediately sends the quota to netd.
        final Ipv6ForwardingRule ruleA = buildTestForwardingRule(mobileIfIndex, NEIGH_A, MAC_A);
        coordinator.tetherOffloadRuleAdd(mIpServer, ruleA);
        verifyTetherOffloadRuleAdd(inOrder, ruleA);
        verifyTetherOffloadSetInterfaceQuota(inOrder, mobileIfIndex, limit, true /* isInit */);
        inOrder.verifyNoMoreInteractions();

        // Adding the second rule on current upstream does not send the quota to netd.
        final Ipv6ForwardingRule ruleB = buildTestForwardingRule(mobileIfIndex, NEIGH_B, MAC_B);
        coordinator.tetherOffloadRuleAdd(mIpServer, ruleB);
        verifyTetherOffloadRuleAdd(inOrder, ruleB);
        verifyNeverTetherOffloadSetInterfaceQuota(inOrder);

        // Removing the second rule on current upstream does not send the quota to netd.
        coordinator.tetherOffloadRuleRemove(mIpServer, ruleB);
        verifyTetherOffloadRuleRemove(inOrder, ruleB);
        verifyNeverTetherOffloadSetInterfaceQuota(inOrder);

        // Removing the last rule on current upstream immediately sends the cleanup stuff to netd.
        updateStatsEntryForTetherOffloadGetAndClearStats(
                buildTestTetherStatsParcel(mobileIfIndex, 0, 0, 0, 0));
        coordinator.tetherOffloadRuleRemove(mIpServer, ruleA);
        verifyTetherOffloadRuleRemove(inOrder, ruleA);
        verifyTetherOffloadGetAndClearStats(inOrder, mobileIfIndex);
        inOrder.verifyNoMoreInteractions();
    }

    @Test
    public void testTetherOffloadRuleUpdateAndClear() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        final String ethIface = "eth1";
        final String mobileIface = "rmnet_data0";
        final Integer ethIfIndex = 100;
        final Integer mobileIfIndex = 101;
        coordinator.addUpstreamNameToLookupTable(ethIfIndex, ethIface);
        coordinator.addUpstreamNameToLookupTable(mobileIfIndex, mobileIface);

        final InOrder inOrder = inOrder(mNetd, mBpfDownstream6Map, mBpfUpstream6Map, mBpfLimitMap,
                mBpfStatsMap);

        // Before the rule test, here are the additional actions while the rules are changed.
        // - After adding the first rule on a given upstream, the coordinator adds a data limit.
        //   If the service has never applied the data limit, set an unlimited quota as default.
        // - After removing the last rule on a given upstream, the coordinator gets the last stats.
        //   Then, it clears the stats and the limit entry from BPF maps.
        // See tetherOffloadRule{Add, Remove, Clear, Clean}.

        // [1] Adding rules on the upstream Ethernet.
        // Note that the default data limit is applied after the first rule is added.
        final Ipv6ForwardingRule ethernetRuleA = buildTestForwardingRule(
                ethIfIndex, NEIGH_A, MAC_A);
        final Ipv6ForwardingRule ethernetRuleB = buildTestForwardingRule(
                ethIfIndex, NEIGH_B, MAC_B);

        coordinator.tetherOffloadRuleAdd(mIpServer, ethernetRuleA);
        verifyTetherOffloadRuleAdd(inOrder, ethernetRuleA);
        verifyTetherOffloadSetInterfaceQuota(inOrder, ethIfIndex, QUOTA_UNLIMITED,
                true /* isInit */);
        verifyStartUpstreamIpv6Forwarding(inOrder, DOWNSTREAM_IFINDEX, DOWNSTREAM_MAC, ethIfIndex);
        coordinator.tetherOffloadRuleAdd(mIpServer, ethernetRuleB);
        verifyTetherOffloadRuleAdd(inOrder, ethernetRuleB);

        // [2] Update the existing rules from Ethernet to cellular.
        final Ipv6ForwardingRule mobileRuleA = buildTestForwardingRule(
                mobileIfIndex, NEIGH_A, MAC_A);
        final Ipv6ForwardingRule mobileRuleB = buildTestForwardingRule(
                mobileIfIndex, NEIGH_B, MAC_B);
        updateStatsEntryForTetherOffloadGetAndClearStats(
                buildTestTetherStatsParcel(ethIfIndex, 10, 20, 30, 40));

        // Update the existing rules for upstream changes. The rules are removed and re-added one
        // by one for updating upstream interface index by #tetherOffloadRuleUpdate.
        coordinator.tetherOffloadRuleUpdate(mIpServer, mobileIfIndex);
        verifyTetherOffloadRuleRemove(inOrder, ethernetRuleA);
        verifyTetherOffloadRuleRemove(inOrder, ethernetRuleB);
        verifyStopUpstreamIpv6Forwarding(inOrder, DOWNSTREAM_IFINDEX, DOWNSTREAM_MAC);
        verifyTetherOffloadGetAndClearStats(inOrder, ethIfIndex);
        verifyTetherOffloadRuleAdd(inOrder, mobileRuleA);
        verifyTetherOffloadSetInterfaceQuota(inOrder, mobileIfIndex, QUOTA_UNLIMITED,
                true /* isInit */);
        verifyStartUpstreamIpv6Forwarding(inOrder, DOWNSTREAM_IFINDEX, DOWNSTREAM_MAC,
                mobileIfIndex);
        verifyTetherOffloadRuleAdd(inOrder, mobileRuleB);

        // [3] Clear all rules for a given IpServer.
        updateStatsEntryForTetherOffloadGetAndClearStats(
                buildTestTetherStatsParcel(mobileIfIndex, 50, 60, 70, 80));
        coordinator.tetherOffloadRuleClear(mIpServer);
        verifyTetherOffloadRuleRemove(inOrder, mobileRuleA);
        verifyTetherOffloadRuleRemove(inOrder, mobileRuleB);
        verifyStopUpstreamIpv6Forwarding(inOrder, DOWNSTREAM_IFINDEX, DOWNSTREAM_MAC);
        verifyTetherOffloadGetAndClearStats(inOrder, mobileIfIndex);

        // [4] Force pushing stats update to verify that the last diff of stats is reported on all
        // upstreams.
        mTetherStatsProvider.pushTetherStats();
        mTetherStatsProviderCb.expectNotifyStatsUpdated(
                new NetworkStats(0L, 2)
                .addEntry(buildTestEntry(STATS_PER_IFACE, ethIface, 10, 20, 30, 40))
                .addEntry(buildTestEntry(STATS_PER_IFACE, mobileIface, 50, 60, 70, 80)),
                new NetworkStats(0L, 2)
                .addEntry(buildTestEntry(STATS_PER_UID, ethIface, 10, 20, 30, 40))
                .addEntry(buildTestEntry(STATS_PER_UID, mobileIface, 50, 60, 70, 80)));
    }

    private void checkBpfDisabled() throws Exception {
        // The caller may mock the global dependencies |mDeps| which is used in
        // #makeBpfCoordinator for testing.
        // See #testBpfDisabledbyNoBpfDownstream6Map.
        final BpfCoordinator coordinator = makeBpfCoordinator();
        coordinator.startPolling();

        // The tether stats polling task should not be scheduled.
        mTestLooper.moveTimeForward(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS);
        waitForIdle();
        verifyNeverTetherOffloadGetStats();

        // The interface name lookup table can't be added.
        final String iface = "rmnet_data0";
        final Integer ifIndex = 100;
        coordinator.addUpstreamNameToLookupTable(ifIndex, iface);
        assertEquals(0, coordinator.getInterfaceNamesForTesting().size());

        // The rule can't be added.
        final InetAddress neigh = InetAddresses.parseNumericAddress("2001:db8::1");
        final MacAddress mac = MacAddress.fromString("00:00:00:00:00:0a");
        final Ipv6ForwardingRule rule = buildTestForwardingRule(ifIndex, neigh, mac);
        coordinator.tetherOffloadRuleAdd(mIpServer, rule);
        verifyNeverTetherOffloadRuleAdd();
        LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules =
                coordinator.getForwardingRulesForTesting().get(mIpServer);
        assertNull(rules);

        // The rule can't be removed. This is not a realistic case because adding rule is not
        // allowed. That implies no rule could be removed, cleared or updated. Verify these
        // cases just in case.
        rules = new LinkedHashMap<Inet6Address, Ipv6ForwardingRule>();
        rules.put(rule.address, rule);
        coordinator.getForwardingRulesForTesting().put(mIpServer, rules);
        coordinator.tetherOffloadRuleRemove(mIpServer, rule);
        verifyNeverTetherOffloadRuleRemove();
        rules = coordinator.getForwardingRulesForTesting().get(mIpServer);
        assertNotNull(rules);
        assertEquals(1, rules.size());

        // The rule can't be cleared.
        coordinator.tetherOffloadRuleClear(mIpServer);
        verifyNeverTetherOffloadRuleRemove();
        rules = coordinator.getForwardingRulesForTesting().get(mIpServer);
        assertNotNull(rules);
        assertEquals(1, rules.size());

        // The rule can't be updated.
        coordinator.tetherOffloadRuleUpdate(mIpServer, rule.upstreamIfindex + 1 /* new */);
        verifyNeverTetherOffloadRuleRemove();
        verifyNeverTetherOffloadRuleAdd();
        rules = coordinator.getForwardingRulesForTesting().get(mIpServer);
        assertNotNull(rules);
        assertEquals(1, rules.size());
    }

    @Test
    public void testBpfDisabledbyConfig() throws Exception {
        setupFunctioningNetdInterface();
        when(mTetherConfig.isBpfOffloadEnabled()).thenReturn(false);

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfDisabledbyNoBpfDownstream6Map() throws Exception {
        setupFunctioningNetdInterface();
        doReturn(null).when(mDeps).getBpfDownstream6Map();

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfDisabledbyNoBpfUpstream6Map() throws Exception {
        setupFunctioningNetdInterface();
        doReturn(null).when(mDeps).getBpfUpstream6Map();

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfDisabledbyNoBpfDownstream4Map() throws Exception {
        setupFunctioningNetdInterface();
        doReturn(null).when(mDeps).getBpfDownstream4Map();

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfDisabledbyNoBpfUpstream4Map() throws Exception {
        setupFunctioningNetdInterface();
        doReturn(null).when(mDeps).getBpfUpstream4Map();

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfDisabledbyNoBpfStatsMap() throws Exception {
        setupFunctioningNetdInterface();
        doReturn(null).when(mDeps).getBpfStatsMap();

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfDisabledbyNoBpfLimitMap() throws Exception {
        setupFunctioningNetdInterface();
        doReturn(null).when(mDeps).getBpfLimitMap();

        checkBpfDisabled();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testBpfMapClear() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();
        verify(mBpfDownstream4Map).clear();
        verify(mBpfUpstream4Map).clear();
        verify(mBpfDownstream6Map).clear();
        verify(mBpfUpstream6Map).clear();
        verify(mBpfStatsMap).clear();
        verify(mBpfLimitMap).clear();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testAttachDetachBpfProgram() throws Exception {
        setupFunctioningNetdInterface();

        // Static mocking for BpfUtils.
        MockitoSession mockSession = ExtendedMockito.mockitoSession()
                .mockStatic(BpfUtils.class)
                .startMocking();
        try {
            final String intIface1 = "wlan1";
            final String intIface2 = "rndis0";
            final String extIface = "rmnet_data0";
            final String virtualIface = "ipsec0";
            final BpfUtils mockMarkerBpfUtils = staticMockMarker(BpfUtils.class);
            final BpfCoordinator coordinator = makeBpfCoordinator();

            // [1] Add the forwarding pair <wlan1, rmnet_data0>. Expect that attach both wlan1 and
            // rmnet_data0.
            coordinator.maybeAttachProgram(intIface1, extIface);
            ExtendedMockito.verify(() -> BpfUtils.attachProgram(extIface, DOWNSTREAM));
            ExtendedMockito.verify(() -> BpfUtils.attachProgram(intIface1, UPSTREAM));
            ExtendedMockito.verifyNoMoreInteractions(mockMarkerBpfUtils);
            ExtendedMockito.clearInvocations(mockMarkerBpfUtils);

            // [2] Add the forwarding pair <wlan1, rmnet_data0> again. Expect no more action.
            coordinator.maybeAttachProgram(intIface1, extIface);
            ExtendedMockito.verifyNoMoreInteractions(mockMarkerBpfUtils);
            ExtendedMockito.clearInvocations(mockMarkerBpfUtils);

            // [3] Add the forwarding pair <rndis0, rmnet_data0>. Expect that attach rndis0 only.
            coordinator.maybeAttachProgram(intIface2, extIface);
            ExtendedMockito.verify(() -> BpfUtils.attachProgram(intIface2, UPSTREAM));
            ExtendedMockito.verifyNoMoreInteractions(mockMarkerBpfUtils);
            ExtendedMockito.clearInvocations(mockMarkerBpfUtils);

            // [4] Remove the forwarding pair <rndis0, rmnet_data0>. Expect detach rndis0 only.
            coordinator.maybeDetachProgram(intIface2, extIface);
            ExtendedMockito.verify(() -> BpfUtils.detachProgram(intIface2));
            ExtendedMockito.verifyNoMoreInteractions(mockMarkerBpfUtils);
            ExtendedMockito.clearInvocations(mockMarkerBpfUtils);

            // [5] Remove the forwarding pair <wlan1, rmnet_data0>. Expect that detach both wlan1
            // and rmnet_data0.
            coordinator.maybeDetachProgram(intIface1, extIface);
            ExtendedMockito.verify(() -> BpfUtils.detachProgram(extIface));
            ExtendedMockito.verify(() -> BpfUtils.detachProgram(intIface1));
            ExtendedMockito.verifyNoMoreInteractions(mockMarkerBpfUtils);
            ExtendedMockito.clearInvocations(mockMarkerBpfUtils);

            // [6] Skip attaching if upstream is virtual interface.
            coordinator.maybeAttachProgram(intIface1, virtualIface);
            ExtendedMockito.verify(() -> BpfUtils.attachProgram(extIface, DOWNSTREAM), never());
            ExtendedMockito.verify(() -> BpfUtils.attachProgram(intIface1, UPSTREAM), never());
            ExtendedMockito.verifyNoMoreInteractions(mockMarkerBpfUtils);
            ExtendedMockito.clearInvocations(mockMarkerBpfUtils);

        } finally {
            mockSession.finishMocking();
        }
    }

    @Test
    public void testTetheringConfigSetPollingInterval() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        // [1] The default polling interval.
        coordinator.startPolling();
        assertEquals(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS, coordinator.getPollingInterval());
        coordinator.stopPolling();

        // [2] Expect the invalid polling interval isn't applied. The valid range of interval is
        // DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS..max_long.
        for (final int interval
                : new int[] {0, 100, DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS - 1}) {
            when(mTetherConfig.getOffloadPollInterval()).thenReturn(interval);
            coordinator.startPolling();
            assertEquals(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS, coordinator.getPollingInterval());
            coordinator.stopPolling();
        }

        // [3] Set a specific polling interval which is larger than default value.
        // Use a large polling interval to avoid flaky test because the time forwarding
        // approximation is used to verify the scheduled time of the polling thread.
        final int pollingInterval = 100_000;
        when(mTetherConfig.getOffloadPollInterval()).thenReturn(pollingInterval);
        coordinator.startPolling();

        // Expect the specific polling interval to be applied.
        assertEquals(pollingInterval, coordinator.getPollingInterval());

        // Start on a new polling time slot.
        mTestLooper.moveTimeForward(pollingInterval);
        waitForIdle();
        clearStatsInvocations();

        // Move time forward to 90% polling interval time. Expect that the polling thread has not
        // scheduled yet.
        mTestLooper.moveTimeForward((long) (pollingInterval * 0.9));
        waitForIdle();
        verifyNeverTetherOffloadGetStats();

        // Move time forward to the remaining 10% polling interval time. Expect that the polling
        // thread has scheduled.
        mTestLooper.moveTimeForward((long) (pollingInterval * 0.1));
        waitForIdle();
        verifyTetherOffloadGetStats();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testStartStopConntrackMonitoring() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        // [1] Don't stop monitoring if it has never started.
        coordinator.stopMonitoring(mIpServer);
        verify(mConntrackMonitor, never()).start();

        // [2] Start monitoring.
        coordinator.startMonitoring(mIpServer);
        verify(mConntrackMonitor).start();
        clearInvocations(mConntrackMonitor);

        // [3] Stop monitoring.
        coordinator.stopMonitoring(mIpServer);
        verify(mConntrackMonitor).stop();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.Q)
    @IgnoreAfter(Build.VERSION_CODES.R)
    // Only run this test on Android R.
    public void testStartStopConntrackMonitoring_R() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        coordinator.startMonitoring(mIpServer);
        verify(mConntrackMonitor, never()).start();

        coordinator.stopMonitoring(mIpServer);
        verify(mConntrackMonitor, never()).stop();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testStartStopConntrackMonitoringWithTwoDownstreamIfaces() throws Exception {
        setupFunctioningNetdInterface();

        final BpfCoordinator coordinator = makeBpfCoordinator();

        // [1] Start monitoring at the first IpServer adding.
        coordinator.startMonitoring(mIpServer);
        verify(mConntrackMonitor).start();
        clearInvocations(mConntrackMonitor);

        // [2] Don't start monitoring at the second IpServer adding.
        coordinator.startMonitoring(mIpServer2);
        verify(mConntrackMonitor, never()).start();

        // [3] Don't stop monitoring if any downstream interface exists.
        coordinator.stopMonitoring(mIpServer2);
        verify(mConntrackMonitor, never()).stop();

        // [4] Stop monitoring if no downstream exists.
        coordinator.stopMonitoring(mIpServer);
        verify(mConntrackMonitor).stop();
    }

    // Test network topology:
    //
    //         public network (rawip)                 private network
    //                   |                 UE                |
    // +------------+    V    +------------+------------+    V    +------------+
    // |   Sever    +---------+  Upstream  | Downstream +---------+   Client   |
    // +------------+         +------------+------------+         +------------+
    // remote ip              public ip                           private ip
    // 140.112.8.116:443      100.81.179.1:62449                  192.168.80.12:62449
    //
    private static final Inet4Address REMOTE_ADDR =
            (Inet4Address) InetAddresses.parseNumericAddress("140.112.8.116");
    private static final Inet4Address PUBLIC_ADDR =
            (Inet4Address) InetAddresses.parseNumericAddress("100.81.179.1");
    private static final Inet4Address PRIVATE_ADDR =
            (Inet4Address) InetAddresses.parseNumericAddress("192.168.80.12");

    // IPv4-mapped IPv6 addresses
    // Remote addrress ::ffff:140.112.8.116
    // Public addrress ::ffff:100.81.179.1
    // Private addrress ::ffff:192.168.80.12
    private static final byte[] REMOTE_ADDR_V4MAPPED_BYTES = new byte[] {
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0xff, (byte) 0xff,
            (byte) 0x8c, (byte) 0x70, (byte) 0x08, (byte) 0x74 };
    private static final byte[] PUBLIC_ADDR_V4MAPPED_BYTES = new byte[] {
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0xff, (byte) 0xff,
            (byte) 0x64, (byte) 0x51, (byte) 0xb3, (byte) 0x01 };
    private static final byte[] PRIVATE_ADDR_V4MAPPED_BYTES = new byte[] {
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0xff, (byte) 0xff,
            (byte) 0xc0, (byte) 0xa8, (byte) 0x50, (byte) 0x0c };

    // Generally, public port and private port are the same in the NAT conntrack message.
    // TODO: consider using different private port and public port for testing.
    private static final short REMOTE_PORT = (short) 443;
    private static final short PUBLIC_PORT = (short) 62449;
    private static final short PRIVATE_PORT = (short) 62449;

    @NonNull
    private Tether4Key makeUpstream4Key(int proto) {
        if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
            fail("Not support protocol " + proto);
        }
        return new Tether4Key(DOWNSTREAM_IFINDEX, DOWNSTREAM_MAC, (short) proto,
            PRIVATE_ADDR.getAddress(), REMOTE_ADDR.getAddress(), PRIVATE_PORT, REMOTE_PORT);
    }

    @NonNull
    private Tether4Key makeDownstream4Key(int proto) {
        if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
            fail("Not support protocol " + proto);
        }
        return new Tether4Key(UPSTREAM_IFINDEX,
                MacAddress.ALL_ZEROS_ADDRESS /* dstMac (rawip) */, (short) proto,
                REMOTE_ADDR.getAddress(), PUBLIC_ADDR.getAddress(), REMOTE_PORT, PUBLIC_PORT);
    }

    @NonNull
    private Tether4Value makeUpstream4Value() {
        return new Tether4Value(UPSTREAM_IFINDEX,
                MacAddress.ALL_ZEROS_ADDRESS /* ethDstMac (rawip) */,
                MacAddress.ALL_ZEROS_ADDRESS /* ethSrcMac (rawip) */, ETH_P_IP,
                NetworkStackConstants.ETHER_MTU, PUBLIC_ADDR_V4MAPPED_BYTES,
                REMOTE_ADDR_V4MAPPED_BYTES, PUBLIC_PORT, REMOTE_PORT, 0 /* lastUsed */);
    }

    @NonNull
    private Tether4Value makeDownstream4Value() {
        return new Tether4Value(DOWNSTREAM_IFINDEX, MAC_A /* client mac */, DOWNSTREAM_MAC,
                ETH_P_IP, NetworkStackConstants.ETHER_MTU, REMOTE_ADDR_V4MAPPED_BYTES,
                PRIVATE_ADDR_V4MAPPED_BYTES, REMOTE_PORT, PRIVATE_PORT, 0 /* lastUsed */);
    }

    @NonNull
    private Tether4Key makeDownstream4Key() {
        return makeDownstream4Key(IPPROTO_TCP);
    }

    @NonNull
    private ConntrackEvent makeTestConntrackEvent(short msgType, int proto) {
        if (msgType != IPCTNL_MSG_CT_NEW && msgType != IPCTNL_MSG_CT_DELETE) {
            fail("Not support message type " + msgType);
        }
        if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
            fail("Not support protocol " + proto);
        }

        final int status = (msgType == IPCTNL_MSG_CT_NEW) ? ESTABLISHED_MASK : DYING_MASK;
        final int timeoutSec = (msgType == IPCTNL_MSG_CT_NEW) ? 100 /* nonzero, new */
                : 0 /* unused, delete */;
        return new ConntrackEvent(
                (short) (NetlinkConstants.NFNL_SUBSYS_CTNETLINK << 8 | msgType),
                new Tuple(new TupleIpv4(PRIVATE_ADDR, REMOTE_ADDR),
                        new TupleProto((byte) proto, PRIVATE_PORT, REMOTE_PORT)),
                new Tuple(new TupleIpv4(REMOTE_ADDR, PUBLIC_ADDR),
                        new TupleProto((byte) proto, REMOTE_PORT, PUBLIC_PORT)),
                status,
                timeoutSec);
    }

    private void setUpstreamInformationTo(final BpfCoordinator coordinator) {
        final LinkProperties lp = new LinkProperties();
        lp.setInterfaceName(UPSTREAM_IFACE);
        lp.addLinkAddress(new LinkAddress(PUBLIC_ADDR, 32 /* prefix length */));
        final NetworkCapabilities capabilities = new NetworkCapabilities()
                .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR);
        coordinator.updateUpstreamNetworkState(new UpstreamNetworkState(lp, capabilities,
                new Network(TEST_NET_ID)));
    }

    private void setDownstreamAndClientInformationTo(final BpfCoordinator coordinator) {
        final ClientInfo clientInfo = new ClientInfo(DOWNSTREAM_IFINDEX, DOWNSTREAM_MAC,
                PRIVATE_ADDR, MAC_A /* client mac */);
        coordinator.tetherOffloadClientAdd(mIpServer, clientInfo);
    }

    private void initBpfCoordinatorForRule4(final BpfCoordinator coordinator) throws Exception {
        // Needed because addUpstreamIfindexToMap only updates upstream information when polling
        // was started.
        coordinator.startPolling();

        // Needed because two reasons: (1) BpfConntrackEventConsumer#accept only performs cleanup
        // when both upstream and downstream rules are removed. (2) tetherOffloadRuleRemove of
        // api31.BpfCoordinatorShimImpl only decreases the count while the entry is deleted.
        // In the other words, deleteEntry returns true.
        doReturn(true).when(mBpfUpstream4Map).deleteEntry(any());
        doReturn(true).when(mBpfDownstream4Map).deleteEntry(any());

        // Needed because BpfCoordinator#addUpstreamIfindexToMap queries interface parameter for
        // interface index.
        doReturn(UPSTREAM_IFACE_PARAMS).when(mDeps).getInterfaceParams(UPSTREAM_IFACE);

        coordinator.addUpstreamNameToLookupTable(UPSTREAM_IFINDEX, UPSTREAM_IFACE);
        setUpstreamInformationTo(coordinator);
        setDownstreamAndClientInformationTo(coordinator);
    }

    // TODO: Test the IPv4 and IPv6 exist concurrently.
    // TODO: Test the IPv4 rule delete failed.
    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testSetDataLimitOnRule4Change() throws Exception {
        final BpfCoordinator coordinator = makeBpfCoordinator();
        initBpfCoordinatorForRule4(coordinator);

        // Applying a data limit to the current upstream does not take any immediate action.
        // The data limit could be only set on an upstream which has rules.
        final long limit = 12345;
        final InOrder inOrder = inOrder(mNetd, mBpfUpstream4Map, mBpfDownstream4Map, mBpfLimitMap,
                mBpfStatsMap);
        mTetherStatsProvider.onSetLimit(UPSTREAM_IFACE, limit);
        waitForIdle();
        verifyNeverTetherOffloadSetInterfaceQuota(inOrder);

        // Build TCP and UDP rules for testing. Note that the values of {TCP, UDP} are the same
        // because the protocol is not an element of the value. Consider using different address
        // or port to make them different for better testing.
        // TODO: Make the values of {TCP, UDP} rules different.
        final Tether4Key expectedUpstream4KeyTcp = makeUpstream4Key(IPPROTO_TCP);
        final Tether4Key expectedDownstream4KeyTcp = makeDownstream4Key(IPPROTO_TCP);
        final Tether4Value expectedUpstream4ValueTcp = makeUpstream4Value();
        final Tether4Value expectedDownstream4ValueTcp = makeDownstream4Value();

        final Tether4Key expectedUpstream4KeyUdp = makeUpstream4Key(IPPROTO_UDP);
        final Tether4Key expectedDownstream4KeyUdp = makeDownstream4Key(IPPROTO_UDP);
        final Tether4Value expectedUpstream4ValueUdp = makeUpstream4Value();
        final Tether4Value expectedDownstream4ValueUdp = makeDownstream4Value();

        // [1] Adding the first rule on current upstream immediately sends the quota.
        mConsumer.accept(makeTestConntrackEvent(IPCTNL_MSG_CT_NEW, IPPROTO_TCP));
        verifyTetherOffloadSetInterfaceQuota(inOrder, UPSTREAM_IFINDEX, limit, true /* isInit */);
        inOrder.verify(mBpfUpstream4Map)
                .insertEntry(eq(expectedUpstream4KeyTcp), eq(expectedUpstream4ValueTcp));
        inOrder.verify(mBpfDownstream4Map)
                .insertEntry(eq(expectedDownstream4KeyTcp), eq(expectedDownstream4ValueTcp));
        inOrder.verifyNoMoreInteractions();

        // [2] Adding the second rule on current upstream does not send the quota.
        mConsumer.accept(makeTestConntrackEvent(IPCTNL_MSG_CT_NEW, IPPROTO_UDP));
        verifyNeverTetherOffloadSetInterfaceQuota(inOrder);
        inOrder.verify(mBpfUpstream4Map)
                .insertEntry(eq(expectedUpstream4KeyUdp), eq(expectedUpstream4ValueUdp));
        inOrder.verify(mBpfDownstream4Map)
                .insertEntry(eq(expectedDownstream4KeyUdp), eq(expectedDownstream4ValueUdp));
        inOrder.verifyNoMoreInteractions();

        // [3] Removing the second rule on current upstream does not send the quota.
        mConsumer.accept(makeTestConntrackEvent(IPCTNL_MSG_CT_DELETE, IPPROTO_UDP));
        verifyNeverTetherOffloadSetInterfaceQuota(inOrder);
        inOrder.verify(mBpfUpstream4Map).deleteEntry(eq(expectedUpstream4KeyUdp));
        inOrder.verify(mBpfDownstream4Map).deleteEntry(eq(expectedDownstream4KeyUdp));
        inOrder.verifyNoMoreInteractions();

        // [4] Removing the last rule on current upstream immediately sends the cleanup stuff.
        updateStatsEntryForTetherOffloadGetAndClearStats(
                buildTestTetherStatsParcel(UPSTREAM_IFINDEX, 0, 0, 0, 0));
        mConsumer.accept(makeTestConntrackEvent(IPCTNL_MSG_CT_DELETE, IPPROTO_TCP));
        inOrder.verify(mBpfUpstream4Map).deleteEntry(eq(expectedUpstream4KeyTcp));
        inOrder.verify(mBpfDownstream4Map).deleteEntry(eq(expectedDownstream4KeyTcp));
        verifyTetherOffloadGetAndClearStats(inOrder, UPSTREAM_IFINDEX);
        inOrder.verifyNoMoreInteractions();
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testAddDevMapRule6() throws Exception {
        final BpfCoordinator coordinator = makeBpfCoordinator();

        coordinator.addUpstreamNameToLookupTable(UPSTREAM_IFINDEX, UPSTREAM_IFACE);
        final Ipv6ForwardingRule ruleA = buildTestForwardingRule(UPSTREAM_IFINDEX, NEIGH_A, MAC_A);
        final Ipv6ForwardingRule ruleB = buildTestForwardingRule(UPSTREAM_IFINDEX, NEIGH_B, MAC_B);

        coordinator.tetherOffloadRuleAdd(mIpServer, ruleA);
        verify(mBpfDevMap).updateEntry(eq(new TetherDevKey(UPSTREAM_IFINDEX)),
                eq(new TetherDevValue(UPSTREAM_IFINDEX)));
        verify(mBpfDevMap).updateEntry(eq(new TetherDevKey(DOWNSTREAM_IFINDEX)),
                eq(new TetherDevValue(DOWNSTREAM_IFINDEX)));
        clearInvocations(mBpfDevMap);

        coordinator.tetherOffloadRuleAdd(mIpServer, ruleB);
        verify(mBpfDevMap, never()).updateEntry(any(), any());
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testAddDevMapRule4() throws Exception {
        final BpfCoordinator coordinator = makeBpfCoordinator();
        initBpfCoordinatorForRule4(coordinator);

        mConsumer.accept(makeTestConntrackEvent(IPCTNL_MSG_CT_NEW, IPPROTO_TCP));
        verify(mBpfDevMap).updateEntry(eq(new TetherDevKey(UPSTREAM_IFINDEX)),
                eq(new TetherDevValue(UPSTREAM_IFINDEX)));
        verify(mBpfDevMap).updateEntry(eq(new TetherDevKey(DOWNSTREAM_IFINDEX)),
                eq(new TetherDevValue(DOWNSTREAM_IFINDEX)));
        clearInvocations(mBpfDevMap);

        mConsumer.accept(makeTestConntrackEvent(IPCTNL_MSG_CT_NEW, IPPROTO_UDP));
        verify(mBpfDevMap, never()).updateEntry(any(), any());
    }

    private void setElapsedRealtimeNanos(long nanoSec) {
        mElapsedRealtimeNanos = nanoSec;
    }

    private void checkRefreshConntrackTimeout(final TestBpfMap<Tether4Key, Tether4Value> bpfMap,
            final Tether4Key tcpKey, final Tether4Value tcpValue, final Tether4Key udpKey,
            final Tether4Value udpValue) throws Exception {
        // Both system elapsed time since boot and the rule last used time are used to measure
        // the rule expiration. In this test, all test rules are fixed the last used time to 0.
        // Set the different testing elapsed time to make the rule to be valid or expired.
        //
        // Timeline:
        // 0                                       60 (seconds)
        // +---+---+---+---+--...--+---+---+---+---+---+- ..
        // |      POLLING_CONNTRACK_TIMEOUT_MS     |
        // +---+---+---+---+--...--+---+---+---+---+---+- ..
        // |<-          valid diff           ->|
        // |<-          expired diff                 ->|
        // ^                                   ^       ^
        // last used time      elapsed time (valid)    elapsed time (expired)
        final long validTime = (POLLING_CONNTRACK_TIMEOUT_MS - 1) * 1_000_000L;
        final long expiredTime = (POLLING_CONNTRACK_TIMEOUT_MS + 1) * 1_000_000L;

        // Static mocking for NetlinkSocket.
        MockitoSession mockSession = ExtendedMockito.mockitoSession()
                .mockStatic(NetlinkSocket.class)
                .startMocking();
        try {
            final BpfCoordinator coordinator = makeBpfCoordinator();
            coordinator.startPolling();
            bpfMap.insertEntry(tcpKey, tcpValue);
            bpfMap.insertEntry(udpKey, udpValue);

            // [1] Don't refresh contrack timeout.
            setElapsedRealtimeNanos(expiredTime);
            mTestLooper.moveTimeForward(POLLING_CONNTRACK_TIMEOUT_MS);
            waitForIdle();
            ExtendedMockito.verifyNoMoreInteractions(staticMockMarker(NetlinkSocket.class));
            ExtendedMockito.clearInvocations(staticMockMarker(NetlinkSocket.class));

            // [2] Refresh contrack timeout.
            setElapsedRealtimeNanos(validTime);
            mTestLooper.moveTimeForward(POLLING_CONNTRACK_TIMEOUT_MS);
            waitForIdle();
            final byte[] expectedNetlinkTcp = ConntrackMessage.newIPv4TimeoutUpdateRequest(
                    IPPROTO_TCP, PRIVATE_ADDR, (int) PRIVATE_PORT, REMOTE_ADDR,
                    (int) REMOTE_PORT, NF_CONNTRACK_TCP_TIMEOUT_ESTABLISHED);
            final byte[] expectedNetlinkUdp = ConntrackMessage.newIPv4TimeoutUpdateRequest(
                    IPPROTO_UDP, PRIVATE_ADDR, (int) PRIVATE_PORT, REMOTE_ADDR,
                    (int) REMOTE_PORT, NF_CONNTRACK_UDP_TIMEOUT_STREAM);
            ExtendedMockito.verify(() -> NetlinkSocket.sendOneShotKernelMessage(
                    eq(NETLINK_NETFILTER), eq(expectedNetlinkTcp)));
            ExtendedMockito.verify(() -> NetlinkSocket.sendOneShotKernelMessage(
                    eq(NETLINK_NETFILTER), eq(expectedNetlinkUdp)));
            ExtendedMockito.verifyNoMoreInteractions(staticMockMarker(NetlinkSocket.class));
            ExtendedMockito.clearInvocations(staticMockMarker(NetlinkSocket.class));

            // [3] Don't refresh contrack timeout if polling stopped.
            coordinator.stopPolling();
            mTestLooper.moveTimeForward(POLLING_CONNTRACK_TIMEOUT_MS);
            waitForIdle();
            ExtendedMockito.verifyNoMoreInteractions(staticMockMarker(NetlinkSocket.class));
            ExtendedMockito.clearInvocations(staticMockMarker(NetlinkSocket.class));
        } finally {
            mockSession.finishMocking();
        }
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testRefreshConntrackTimeout_Upstream4Map() throws Exception {
        // TODO: Replace the dependencies BPF map with a non-mocked TestBpfMap object.
        final TestBpfMap<Tether4Key, Tether4Value> bpfUpstream4Map =
                new TestBpfMap<>(Tether4Key.class, Tether4Value.class);
        doReturn(bpfUpstream4Map).when(mDeps).getBpfUpstream4Map();

        final Tether4Key tcpKey = makeUpstream4Key(IPPROTO_TCP);
        final Tether4Key udpKey = makeUpstream4Key(IPPROTO_UDP);
        final Tether4Value tcpValue = makeUpstream4Value();
        final Tether4Value udpValue = makeUpstream4Value();

        checkRefreshConntrackTimeout(bpfUpstream4Map, tcpKey, tcpValue, udpKey, udpValue);
    }

    @Test
    @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testRefreshConntrackTimeout_Downstream4Map() throws Exception {
        // TODO: Replace the dependencies BPF map with a non-mocked TestBpfMap object.
        final TestBpfMap<Tether4Key, Tether4Value> bpfDownstream4Map =
                new TestBpfMap<>(Tether4Key.class, Tether4Value.class);
        doReturn(bpfDownstream4Map).when(mDeps).getBpfDownstream4Map();

        final Tether4Key tcpKey = makeDownstream4Key(IPPROTO_TCP);
        final Tether4Key udpKey = makeDownstream4Key(IPPROTO_UDP);
        final Tether4Value tcpValue = makeDownstream4Value();
        final Tether4Value udpValue = makeDownstream4Value();

        checkRefreshConntrackTimeout(bpfDownstream4Map, tcpKey, tcpValue, udpKey, udpValue);
    }
}
