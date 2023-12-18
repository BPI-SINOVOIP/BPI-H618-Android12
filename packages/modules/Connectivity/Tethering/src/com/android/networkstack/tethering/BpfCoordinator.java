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
import static android.net.netstats.provider.NetworkStatsProvider.QUOTA_UNLIMITED;
import static android.system.OsConstants.ETH_P_IP;
import static android.system.OsConstants.ETH_P_IPV6;

import static com.android.networkstack.tethering.BpfUtils.DOWNSTREAM;
import static com.android.networkstack.tethering.BpfUtils.UPSTREAM;
import static com.android.networkstack.tethering.TetheringConfiguration.DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS;
import static com.android.networkstack.tethering.UpstreamNetworkState.isVcnInterface;

import android.app.usage.NetworkStatsManager;
import android.net.INetd;
import android.net.MacAddress;
import android.net.NetworkStats;
import android.net.NetworkStats.Entry;
import android.net.TetherOffloadRuleParcel;
import android.net.ip.ConntrackMonitor;
import android.net.ip.ConntrackMonitor.ConntrackEventConsumer;
import android.net.ip.IpServer;
import android.net.netlink.ConntrackMessage;
import android.net.netlink.NetlinkConstants;
import android.net.netlink.NetlinkSocket;
import android.net.netstats.provider.NetworkStatsProvider;
import android.net.util.InterfaceParams;
import android.net.util.SharedLog;
import android.net.util.TetheringUtils.ForwardedStats;
import android.os.Handler;
import android.os.SystemClock;
import android.system.ErrnoException;
import android.system.OsConstants;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.util.SparseArray;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.IndentingPrintWriter;
import com.android.modules.utils.build.SdkLevel;
import com.android.net.module.util.NetworkStackConstants;
import com.android.net.module.util.Struct;
import com.android.networkstack.tethering.apishim.common.BpfCoordinatorShim;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

/**
 *  This coordinator is responsible for providing BPF offload relevant functionality.
 *  - Get tethering stats.
 *  - Set data limit.
 *  - Set global alert.
 *  - Add/remove forwarding rules.
 *
 * @hide
 */
public class BpfCoordinator {
    // Ensure the JNI code is loaded. In production this will already have been loaded by
    // TetherService, but for tests it needs to be either loaded here or loaded by every test.
    // TODO: is there a better way?
    static {
        System.loadLibrary("tetherutilsjni");
    }

    private static final String TAG = BpfCoordinator.class.getSimpleName();
    private static final int DUMP_TIMEOUT_MS = 10_000;
    private static final MacAddress NULL_MAC_ADDRESS = MacAddress.fromString(
            "00:00:00:00:00:00");
    private static final String TETHER_DOWNSTREAM4_MAP_PATH = makeMapPath(DOWNSTREAM, 4);
    private static final String TETHER_UPSTREAM4_MAP_PATH = makeMapPath(UPSTREAM, 4);
    private static final String TETHER_DOWNSTREAM6_FS_PATH = makeMapPath(DOWNSTREAM, 6);
    private static final String TETHER_UPSTREAM6_FS_PATH = makeMapPath(UPSTREAM, 6);
    private static final String TETHER_STATS_MAP_PATH = makeMapPath("stats");
    private static final String TETHER_LIMIT_MAP_PATH = makeMapPath("limit");
    private static final String TETHER_ERROR_MAP_PATH = makeMapPath("error");
    private static final String TETHER_DEV_MAP_PATH = makeMapPath("dev");

    /** The names of all the BPF counters defined in bpf_tethering.h. */
    public static final String[] sBpfCounterNames = getBpfCounterNames();

    private static String makeMapPath(String which) {
        return "/sys/fs/bpf/tethering/map_offload_tether_" + which + "_map";
    }

    private static String makeMapPath(boolean downstream, int ipVersion) {
        return makeMapPath((downstream ? "downstream" : "upstream") + ipVersion);
    }

    @VisibleForTesting
    static final int POLLING_CONNTRACK_TIMEOUT_MS = 60_000;
    @VisibleForTesting
    static final int NF_CONNTRACK_TCP_TIMEOUT_ESTABLISHED = 432000;
    @VisibleForTesting
    static final int NF_CONNTRACK_UDP_TIMEOUT_STREAM = 180;

    @VisibleForTesting
    enum StatsType {
        STATS_PER_IFACE,
        STATS_PER_UID,
    }

    @NonNull
    private final Handler mHandler;
    @NonNull
    private final INetd mNetd;
    @NonNull
    private final SharedLog mLog;
    @NonNull
    private final Dependencies mDeps;
    @NonNull
    private final ConntrackMonitor mConntrackMonitor;
    @Nullable
    private final BpfTetherStatsProvider mStatsProvider;
    @NonNull
    private final BpfCoordinatorShim mBpfCoordinatorShim;
    @NonNull
    private final BpfConntrackEventConsumer mBpfConntrackEventConsumer;

    // True if BPF offload is supported, false otherwise. The BPF offload could be disabled by
    // a runtime resource overlay package or device configuration. This flag is only initialized
    // in the constructor because it is hard to unwind all existing change once device
    // configuration is changed. Especially the forwarding rules. Keep the same setting
    // to make it simpler. See also TetheringConfiguration.
    private final boolean mIsBpfEnabled;

    // Tracks whether BPF tethering is started or not. This is set by tethering before it
    // starts the first IpServer and is cleared by tethering shortly before the last IpServer
    // is stopped. Note that rule updates (especially deletions, but sometimes additions as
    // well) may arrive when this is false. If they do, they must be communicated to netd.
    // Changes in data limits may also arrive when this is false, and if they do, they must
    // also be communicated to netd.
    private boolean mPollingStarted = false;

    // Tracking remaining alert quota. Unlike limit quota is subject to interface, the alert
    // quota is interface independent and global for tether offload.
    private long mRemainingAlertQuota = QUOTA_UNLIMITED;

    // Maps upstream interface index to offloaded traffic statistics.
    // Always contains the latest total bytes/packets, since each upstream was started, received
    // from the BPF maps for each interface.
    private final SparseArray<ForwardedStats> mStats = new SparseArray<>();

    // Maps upstream interface names to interface quotas.
    // Always contains the latest value received from the framework for each interface, regardless
    // of whether offload is currently running (or is even supported) on that interface. Only
    // includes interfaces that have a quota set. Note that this map is used for storing the quota
    // which is set from the service. Because the service uses the interface name to present the
    // interface, this map uses the interface name to be the mapping index.
    private final HashMap<String, Long> mInterfaceQuotas = new HashMap<>();

    // Maps upstream interface index to interface names.
    // Store all interface name since boot. Used for lookup what interface name it is from the
    // tether stats got from netd because netd reports interface index to present an interface.
    // TODO: Remove the unused interface name.
    private final SparseArray<String> mInterfaceNames = new SparseArray<>();

    // Map of downstream rule maps. Each of these maps represents the IPv6 forwarding rules for a
    // given downstream. Each map:
    // - Is owned by the IpServer that is responsible for that downstream.
    // - Must only be modified by that IpServer.
    // - Is created when the IpServer adds its first rule, and deleted when the IpServer deletes
    //   its last rule (or clears its rules).
    // TODO: Perhaps seal the map and rule operations which communicates with netd into a class.
    // TODO: Does this need to be a LinkedHashMap or can it just be a HashMap? Also, could it be
    // a ConcurrentHashMap, in order to avoid the copies in tetherOffloadRuleClear
    // and tetherOffloadRuleUpdate?
    // TODO: Perhaps use one-dimensional map and access specific downstream rules via downstream
    // index. For doing that, IpServer must guarantee that it always has a valid IPv6 downstream
    // interface index while calling function to clear all rules. IpServer may be calling clear
    // rules function without a valid IPv6 downstream interface index even if it may have one
    // before. IpServer would need to call getInterfaceParams() in the constructor instead of when
    // startIpv6() is called, and make mInterfaceParams final.
    private final HashMap<IpServer, LinkedHashMap<Inet6Address, Ipv6ForwardingRule>>
            mIpv6ForwardingRules = new LinkedHashMap<>();

    // Map of downstream client maps. Each of these maps represents the IPv4 clients for a given
    // downstream. Needed to build IPv4 forwarding rules when conntrack events are received.
    // Each map:
    // - Is owned by the IpServer that is responsible for that downstream.
    // - Must only be modified by that IpServer.
    // - Is created when the IpServer adds its first client, and deleted when the IpServer deletes
    //   its last client.
    // Note that relying on the client address for finding downstream is okay for now because the
    // client address is unique. See PrivateAddressCoordinator#requestDownstreamAddress.
    // TODO: Refactor if any possible that the client address is not unique.
    private final HashMap<IpServer, HashMap<Inet4Address, ClientInfo>>
            mTetherClients = new HashMap<>();

    // Set for which downstream is monitoring the conntrack netlink message.
    private final Set<IpServer> mMonitoringIpServers = new HashSet<>();

    // Map of upstream interface IPv4 address to interface index.
    // TODO: consider making the key to be unique because the upstream address is not unique. It
    // is okay for now because there have only one upstream generally.
    private final HashMap<Inet4Address, Integer> mIpv4UpstreamIndices = new HashMap<>();

    // Map for upstream and downstream pair.
    private final HashMap<String, HashSet<String>> mForwardingPairs = new HashMap<>();

    // Set for upstream and downstream device map. Used for caching BPF dev map status and
    // reduce duplicate adding or removing map operations. Use LinkedHashSet because the test
    // BpfCoordinatorTest needs predictable iteration order.
    private final Set<Integer> mDeviceMapSet = new LinkedHashSet<>();

    // Tracks the last IPv4 upstream index. Support single upstream only.
    // TODO: Support multi-upstream interfaces.
    private int mLastIPv4UpstreamIfindex = 0;

    // Runnable that used by scheduling next polling of stats.
    private final Runnable mScheduledPollingStats = () -> {
        updateForwardedStats();
        maybeSchedulePollingStats();
    };

    // Runnable that used by scheduling next polling of conntrack timeout.
    private final Runnable mScheduledPollingConntrackTimeout = () -> {
        maybeRefreshConntrackTimeout();
        maybeSchedulePollingConntrackTimeout();
    };

    // TODO: add BpfMap<TetherDownstream64Key, TetherDownstream64Value> retrieving function.
    @VisibleForTesting
    public abstract static class Dependencies {
        /** Get handler. */
        @NonNull public abstract Handler getHandler();

        /** Get netd. */
        @NonNull public abstract INetd getNetd();

        /** Get network stats manager. */
        @NonNull public abstract NetworkStatsManager getNetworkStatsManager();

        /** Get shared log. */
        @NonNull public abstract SharedLog getSharedLog();

        /** Get tethering configuration. */
        @Nullable public abstract TetheringConfiguration getTetherConfig();

        /** Get conntrack monitor. */
        @NonNull public ConntrackMonitor getConntrackMonitor(ConntrackEventConsumer consumer) {
            return new ConntrackMonitor(getHandler(), getSharedLog(), consumer);
        }

        /** Get interface information for a given interface. */
        @NonNull public InterfaceParams getInterfaceParams(String ifName) {
            return InterfaceParams.getByName(ifName);
        }

        /**
         * Represents an estimate of elapsed time since boot in nanoseconds.
         */
        public long elapsedRealtimeNanos() {
            return SystemClock.elapsedRealtimeNanos();
        }

        /**
         * Check OS Build at least S.
         *
         * TODO: move to BpfCoordinatorShim once the test doesn't need the mocked OS build for
         * testing different code flows concurrently.
         */
        public boolean isAtLeastS() {
            return SdkLevel.isAtLeastS();
        }

        /** Get downstream4 BPF map. */
        @Nullable public BpfMap<Tether4Key, Tether4Value> getBpfDownstream4Map() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_DOWNSTREAM4_MAP_PATH,
                    BpfMap.BPF_F_RDWR, Tether4Key.class, Tether4Value.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create downstream4 map: " + e);
                return null;
            }
        }

        /** Get upstream4 BPF map. */
        @Nullable public BpfMap<Tether4Key, Tether4Value> getBpfUpstream4Map() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_UPSTREAM4_MAP_PATH,
                    BpfMap.BPF_F_RDWR, Tether4Key.class, Tether4Value.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create upstream4 map: " + e);
                return null;
            }
        }

        /** Get downstream6 BPF map. */
        @Nullable public BpfMap<TetherDownstream6Key, Tether6Value> getBpfDownstream6Map() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_DOWNSTREAM6_FS_PATH,
                    BpfMap.BPF_F_RDWR, TetherDownstream6Key.class, Tether6Value.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create downstream6 map: " + e);
                return null;
            }
        }

        /** Get upstream6 BPF map. */
        @Nullable public BpfMap<TetherUpstream6Key, Tether6Value> getBpfUpstream6Map() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_UPSTREAM6_FS_PATH, BpfMap.BPF_F_RDWR,
                        TetherUpstream6Key.class, Tether6Value.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create upstream6 map: " + e);
                return null;
            }
        }

        /** Get stats BPF map. */
        @Nullable public BpfMap<TetherStatsKey, TetherStatsValue> getBpfStatsMap() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_STATS_MAP_PATH,
                    BpfMap.BPF_F_RDWR, TetherStatsKey.class, TetherStatsValue.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create stats map: " + e);
                return null;
            }
        }

        /** Get limit BPF map. */
        @Nullable public BpfMap<TetherLimitKey, TetherLimitValue> getBpfLimitMap() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_LIMIT_MAP_PATH,
                    BpfMap.BPF_F_RDWR, TetherLimitKey.class, TetherLimitValue.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create limit map: " + e);
                return null;
            }
        }

        /** Get dev BPF map. */
        @Nullable public BpfMap<TetherDevKey, TetherDevValue> getBpfDevMap() {
            if (!isAtLeastS()) return null;
            try {
                return new BpfMap<>(TETHER_DEV_MAP_PATH,
                    BpfMap.BPF_F_RDWR, TetherDevKey.class, TetherDevValue.class);
            } catch (ErrnoException e) {
                Log.e(TAG, "Cannot create dev map: " + e);
                return null;
            }
        }
    }

    @VisibleForTesting
    public BpfCoordinator(@NonNull Dependencies deps) {
        mDeps = deps;
        mHandler = mDeps.getHandler();
        mNetd = mDeps.getNetd();
        mLog = mDeps.getSharedLog().forSubComponent(TAG);
        mIsBpfEnabled = isBpfEnabled();

        // The conntrack consummer needs to be initialized in BpfCoordinator constructor because it
        // have to access the data members of BpfCoordinator which is not a static class. The
        // consumer object is also needed for initializing the conntrack monitor which may be
        // mocked for testing.
        mBpfConntrackEventConsumer = new BpfConntrackEventConsumer();
        mConntrackMonitor = mDeps.getConntrackMonitor(mBpfConntrackEventConsumer);

        BpfTetherStatsProvider provider = new BpfTetherStatsProvider();
        try {
            mDeps.getNetworkStatsManager().registerNetworkStatsProvider(
                    getClass().getSimpleName(), provider);
        } catch (RuntimeException e) {
            // TODO: Perhaps not allow to use BPF offload because the reregistration failure
            // implied that no data limit could be applies on a metered upstream if any.
            Log.wtf(TAG, "Cannot register offload stats provider: " + e);
            provider = null;
        }
        mStatsProvider = provider;

        mBpfCoordinatorShim = BpfCoordinatorShim.getBpfCoordinatorShim(deps);
        if (!mBpfCoordinatorShim.isInitialized()) {
            mLog.e("Bpf shim not initialized");
        }
    }

    /**
     * Start BPF tethering offload stats polling when the first upstream is started.
     * Note that this can be only called on handler thread.
     * TODO: Perhaps check BPF support before starting.
     * TODO: Start the stats polling only if there is any client on the downstream.
     */
    public void startPolling() {
        if (mPollingStarted) return;

        if (!isUsingBpf()) {
            mLog.i("BPF is not using");
            return;
        }

        mPollingStarted = true;
        maybeSchedulePollingStats();
        maybeSchedulePollingConntrackTimeout();

        mLog.i("Polling started");
    }

    /**
     * Stop BPF tethering offload stats polling.
     * The data limit cleanup and the tether stats maps cleanup are not implemented here.
     * These cleanups rely on all IpServers calling #tetherOffloadRuleRemove. After the
     * last rule is removed from the upstream, #tetherOffloadRuleRemove does the cleanup
     * functionality.
     * Note that this can be only called on handler thread.
     */
    public void stopPolling() {
        if (!mPollingStarted) return;

        // Stop scheduled polling conntrack timeout.
        if (mHandler.hasCallbacks(mScheduledPollingConntrackTimeout)) {
            mHandler.removeCallbacks(mScheduledPollingConntrackTimeout);
        }
        // Stop scheduled polling stats and poll the latest stats from BPF maps.
        if (mHandler.hasCallbacks(mScheduledPollingStats)) {
            mHandler.removeCallbacks(mScheduledPollingStats);
        }
        updateForwardedStats();
        mPollingStarted = false;

        mLog.i("Polling stopped");
    }

    private boolean isUsingBpf() {
        return mIsBpfEnabled && mBpfCoordinatorShim.isInitialized();
    }

    /**
     * Start conntrack message monitoring.
     * Note that this can be only called on handler thread.
     *
     * TODO: figure out a better logging for non-interesting conntrack message.
     * For example, the following logging is an IPCTNL_MSG_CT_GET message but looks scary.
     * +---------------------------------------------------------------------------+
     * | ERROR unparsable netlink msg: 1400000001010103000000000000000002000000    |
     * +------------------+--------------------------------------------------------+
     * |                  | struct nlmsghdr                                        |
     * | 14000000         | length = 20                                            |
     * | 0101             | type = NFNL_SUBSYS_CTNETLINK << 8 | IPCTNL_MSG_CT_GET  |
     * | 0103             | flags                                                  |
     * | 00000000         | seqno = 0                                              |
     * | 00000000         | pid = 0                                                |
     * |                  | struct nfgenmsg                                        |
     * | 02               | nfgen_family  = AF_INET                                |
     * | 00               | version = NFNETLINK_V0                                 |
     * | 0000             | res_id                                                 |
     * +------------------+--------------------------------------------------------+
     * See NetlinkMonitor#handlePacket, NetlinkMessage#parseNfMessage.
     */
    public void startMonitoring(@NonNull final IpServer ipServer) {
        // TODO: Wrap conntrackMonitor starting function into mBpfCoordinatorShim.
        if (!isUsingBpf() || !mDeps.isAtLeastS()) return;

        if (mMonitoringIpServers.contains(ipServer)) {
            Log.wtf(TAG, "The same downstream " + ipServer.interfaceName()
                    + " should not start monitoring twice.");
            return;
        }

        if (mMonitoringIpServers.isEmpty()) {
            mConntrackMonitor.start();
            mLog.i("Monitoring started");
        }

        mMonitoringIpServers.add(ipServer);
    }

    /**
     * Stop conntrack event monitoring.
     * Note that this can be only called on handler thread.
     */
    public void stopMonitoring(@NonNull final IpServer ipServer) {
        // TODO: Wrap conntrackMonitor stopping function into mBpfCoordinatorShim.
        if (!isUsingBpf() || !mDeps.isAtLeastS()) return;

        mMonitoringIpServers.remove(ipServer);

        if (!mMonitoringIpServers.isEmpty()) return;

        mConntrackMonitor.stop();
        mLog.i("Monitoring stopped");
    }

    /**
     * Add forwarding rule. After adding the first rule on a given upstream, must add the data
     * limit on the given upstream.
     * Note that this can be only called on handler thread.
     */
    public void tetherOffloadRuleAdd(
            @NonNull final IpServer ipServer, @NonNull final Ipv6ForwardingRule rule) {
        if (!isUsingBpf()) return;

        // TODO: Perhaps avoid to add a duplicate rule.
        if (!mBpfCoordinatorShim.tetherOffloadRuleAdd(rule)) return;

        if (!mIpv6ForwardingRules.containsKey(ipServer)) {
            mIpv6ForwardingRules.put(ipServer, new LinkedHashMap<Inet6Address,
                    Ipv6ForwardingRule>());
        }
        LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules = mIpv6ForwardingRules.get(ipServer);

        // Add upstream and downstream interface index to dev map.
        maybeAddDevMap(rule.upstreamIfindex, rule.downstreamIfindex);

        // When the first rule is added to an upstream, setup upstream forwarding and data limit.
        maybeSetLimit(rule.upstreamIfindex);

        if (!isAnyRuleFromDownstreamToUpstream(rule.downstreamIfindex, rule.upstreamIfindex)) {
            final int downstream = rule.downstreamIfindex;
            final int upstream = rule.upstreamIfindex;
            // TODO: support upstream forwarding on non-point-to-point interfaces.
            // TODO: get the MTU from LinkProperties and update the rules when it changes.
            if (!mBpfCoordinatorShim.startUpstreamIpv6Forwarding(downstream, upstream, rule.srcMac,
                    NULL_MAC_ADDRESS, NULL_MAC_ADDRESS, NetworkStackConstants.ETHER_MTU)) {
                mLog.e("Failed to enable upstream IPv6 forwarding from "
                        + mInterfaceNames.get(downstream) + " to " + mInterfaceNames.get(upstream));
            }
        }

        // Must update the adding rule after calling #isAnyRuleOnUpstream because it needs to
        // check if it is about adding a first rule for a given upstream.
        rules.put(rule.address, rule);
    }

    /**
     * Remove forwarding rule. After removing the last rule on a given upstream, must clear
     * data limit, update the last tether stats and remove the tether stats in the BPF maps.
     * Note that this can be only called on handler thread.
     */
    public void tetherOffloadRuleRemove(
            @NonNull final IpServer ipServer, @NonNull final Ipv6ForwardingRule rule) {
        if (!isUsingBpf()) return;

        if (!mBpfCoordinatorShim.tetherOffloadRuleRemove(rule)) return;

        LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules = mIpv6ForwardingRules.get(ipServer);
        if (rules == null) return;

        // Must remove rules before calling #isAnyRuleOnUpstream because it needs to check if
        // the last rule is removed for a given upstream. If no rule is removed, return early.
        // Avoid unnecessary work on a non-existent rule which may have never been added or
        // removed already.
        if (rules.remove(rule.address) == null) return;

        // Remove the downstream entry if it has no more rule.
        if (rules.isEmpty()) {
            mIpv6ForwardingRules.remove(ipServer);
        }

        // If no more rules between this upstream and downstream, stop upstream forwarding.
        if (!isAnyRuleFromDownstreamToUpstream(rule.downstreamIfindex, rule.upstreamIfindex)) {
            final int downstream = rule.downstreamIfindex;
            final int upstream = rule.upstreamIfindex;
            if (!mBpfCoordinatorShim.stopUpstreamIpv6Forwarding(downstream, upstream,
                    rule.srcMac)) {
                mLog.e("Failed to disable upstream IPv6 forwarding from "
                        + mInterfaceNames.get(downstream) + " to " + mInterfaceNames.get(upstream));
            }
        }

        // Do cleanup functionality if there is no more rule on the given upstream.
        maybeClearLimit(rule.upstreamIfindex);
    }

    /**
     * Clear all forwarding rules for a given downstream.
     * Note that this can be only called on handler thread.
     * TODO: rename to tetherOffloadRuleClear6 because of IPv6 only.
     */
    public void tetherOffloadRuleClear(@NonNull final IpServer ipServer) {
        if (!isUsingBpf()) return;

        final LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules = mIpv6ForwardingRules.get(
                ipServer);
        if (rules == null) return;

        // Need to build a rule list because the rule map may be changed in the iteration.
        for (final Ipv6ForwardingRule rule : new ArrayList<Ipv6ForwardingRule>(rules.values())) {
            tetherOffloadRuleRemove(ipServer, rule);
        }
    }

    /**
     * Update existing forwarding rules to new upstream for a given downstream.
     * Note that this can be only called on handler thread.
     */
    public void tetherOffloadRuleUpdate(@NonNull final IpServer ipServer, int newUpstreamIfindex) {
        if (!isUsingBpf()) return;

        final LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules = mIpv6ForwardingRules.get(
                ipServer);
        if (rules == null) return;

        // Need to build a rule list because the rule map may be changed in the iteration.
        // First remove all the old rules, then add all the new rules. This is because the upstream
        // forwarding code in tetherOffloadRuleAdd cannot support rules on two upstreams at the
        // same time. Deleting the rules first ensures that upstream forwarding is disabled on the
        // old upstream when the last rule is removed from it, and re-enabled on the new upstream
        // when the first rule is added to it.
        // TODO: Once the IPv6 client processing code has moved from IpServer to BpfCoordinator, do
        // something smarter.
        final ArrayList<Ipv6ForwardingRule> rulesCopy = new ArrayList<>(rules.values());
        for (final Ipv6ForwardingRule rule : rulesCopy) {
            // Remove the old rule before adding the new one because the map uses the same key for
            // both rules. Reversing the processing order causes that the new rule is removed as
            // unexpected.
            // TODO: Add new rule first to reduce the latency which has no rule.
            tetherOffloadRuleRemove(ipServer, rule);
        }
        for (final Ipv6ForwardingRule rule : rulesCopy) {
            tetherOffloadRuleAdd(ipServer, rule.onNewUpstream(newUpstreamIfindex));
        }
    }

    /**
     * Add upstream name to lookup table. The lookup table is used for tether stats interface name
     * lookup because the netd only reports interface index in BPF tether stats but the service
     * expects the interface name in NetworkStats object.
     * Note that this can be only called on handler thread.
     */
    public void addUpstreamNameToLookupTable(int upstreamIfindex, @NonNull String upstreamIface) {
        if (!isUsingBpf()) return;

        if (upstreamIfindex == 0 || TextUtils.isEmpty(upstreamIface)) return;

        if (isVcnInterface(upstreamIface)) return;

        // The same interface index to name mapping may be added by different IpServer objects or
        // re-added by reconnection on the same upstream interface. Ignore the duplicate one.
        final String iface = mInterfaceNames.get(upstreamIfindex);
        if (iface == null) {
            mInterfaceNames.put(upstreamIfindex, upstreamIface);
        } else if (!TextUtils.equals(iface, upstreamIface)) {
            Log.wtf(TAG, "The upstream interface name " + upstreamIface
                    + " is different from the existing interface name "
                    + iface + " for index " + upstreamIfindex);
        }
    }

    /**
     * Add downstream client.
     * Note that this can be only called on handler thread.
     */
    public void tetherOffloadClientAdd(@NonNull final IpServer ipServer,
            @NonNull final ClientInfo client) {
        if (!isUsingBpf()) return;

        if (!mTetherClients.containsKey(ipServer)) {
            mTetherClients.put(ipServer, new HashMap<Inet4Address, ClientInfo>());
        }

        HashMap<Inet4Address, ClientInfo> clients = mTetherClients.get(ipServer);
        clients.put(client.clientAddress, client);
    }

    /**
     * Remove a downstream client and its rules if any.
     * Note that this can be only called on handler thread.
     */
    public void tetherOffloadClientRemove(@NonNull final IpServer ipServer,
            @NonNull final ClientInfo client) {
        if (!isUsingBpf()) return;

        // No clients on the downstream, return early.
        HashMap<Inet4Address, ClientInfo> clients = mTetherClients.get(ipServer);
        if (clients == null) return;

        // No client is removed, return early.
        if (clients.remove(client.clientAddress) == null) return;

        // Remove the client's rules. Removing the client implies that its rules are not used
        // anymore.
        tetherOffloadRuleClear(client);

        // Remove the downstream entry if it has no more client.
        if (clients.isEmpty()) {
            mTetherClients.remove(ipServer);
        }
    }

    /**
     * Clear all downstream clients and their rules if any.
     * Note that this can be only called on handler thread.
     */
    public void tetherOffloadClientClear(@NonNull final IpServer ipServer) {
        if (!isUsingBpf()) return;

        final HashMap<Inet4Address, ClientInfo> clients = mTetherClients.get(ipServer);
        if (clients == null) return;

        // Need to build a client list because the client map may be changed in the iteration.
        for (final ClientInfo c : new ArrayList<ClientInfo>(clients.values())) {
            tetherOffloadClientRemove(ipServer, c);
        }
    }

    /**
     * Clear all forwarding IPv4 rules for a given client.
     * Note that this can be only called on handler thread.
     */
    private void tetherOffloadRuleClear(@NonNull final ClientInfo clientInfo) {
        // TODO: consider removing the rules in #tetherOffloadRuleForEach once BpfMap#forEach
        // can guarantee that deleting some pass-in rules in the BPF map iteration can still
        // walk through every entry.
        final Inet4Address clientAddr = clientInfo.clientAddress;
        final Set<Integer> upstreamIndiceSet = new ArraySet<Integer>();
        final Set<Tether4Key> deleteUpstreamRuleKeys = new ArraySet<Tether4Key>();
        final Set<Tether4Key> deleteDownstreamRuleKeys = new ArraySet<Tether4Key>();

        // Find the rules which are related with the given client.
        mBpfCoordinatorShim.tetherOffloadRuleForEach(UPSTREAM, (k, v) -> {
            if (Arrays.equals(k.src4, clientAddr.getAddress())) {
                deleteUpstreamRuleKeys.add(k);
            }
        });
        mBpfCoordinatorShim.tetherOffloadRuleForEach(DOWNSTREAM, (k, v) -> {
            if (Arrays.equals(v.dst46, toIpv4MappedAddressBytes(clientAddr))) {
                deleteDownstreamRuleKeys.add(k);
                upstreamIndiceSet.add((int) k.iif);
            }
        });

        // The rules should be paired on upstream and downstream map because they are added by
        // conntrack events which have bidirectional information.
        // TODO: Consider figuring out a way to fix. Probably delete all rules to fallback.
        if (deleteUpstreamRuleKeys.size() != deleteDownstreamRuleKeys.size()) {
            Log.wtf(TAG, "The deleting rule numbers are different on upstream4 and downstream4 ("
                    + "upstream: " + deleteUpstreamRuleKeys.size() + ", "
                    + "downstream: " + deleteDownstreamRuleKeys.size() + ").");
            return;
        }

        // Delete the rules which are related with the given client.
        for (final Tether4Key k : deleteUpstreamRuleKeys) {
            mBpfCoordinatorShim.tetherOffloadRuleRemove(UPSTREAM, k);
        }
        for (final Tether4Key k : deleteDownstreamRuleKeys) {
            mBpfCoordinatorShim.tetherOffloadRuleRemove(DOWNSTREAM, k);
        }

        // Cleanup each upstream interface by a set which avoids duplicated work on the same
        // upstream interface. Cleaning up the same interface twice (or more) here may raise
        // an exception because all related information were removed in the first deletion.
        for (final int upstreamIndex : upstreamIndiceSet) {
            maybeClearLimit(upstreamIndex);
        }
    }

    /**
     * Clear all forwarding IPv4 rules for a given downstream. Needed because the client may still
     * connect on the downstream but the existing rules are not required anymore. Ex: upstream
     * changed.
     */
    private void tetherOffloadRule4Clear(@NonNull final IpServer ipServer) {
        if (!isUsingBpf()) return;

        final HashMap<Inet4Address, ClientInfo> clients = mTetherClients.get(ipServer);
        if (clients == null) return;

        // The value should be unique as its key because currently the key was using from its
        // client address of ClientInfo. See #tetherOffloadClientAdd.
        for (final ClientInfo client : clients.values()) {
            tetherOffloadRuleClear(client);
        }
    }

    private boolean isValidUpstreamIpv4Address(@NonNull final InetAddress addr) {
        if (!(addr instanceof Inet4Address)) return false;
        Inet4Address v4 = (Inet4Address) addr;
        if (v4.isAnyLocalAddress() || v4.isLinkLocalAddress()
                || v4.isLoopbackAddress() || v4.isMulticastAddress()) {
            return false;
        }
        return true;
    }

    /**
     * Call when UpstreamNetworkState may be changed.
     * If upstream has ipv4 for tethering, update this new UpstreamNetworkState
     * to BpfCoordinator for building upstream interface index mapping. Otherwise,
     * clear the all existing rules if any.
     *
     * Note that this can be only called on handler thread.
     */
    public void updateUpstreamNetworkState(UpstreamNetworkState ns) {
        if (!isUsingBpf()) return;

        int upstreamIndex = 0;

        // This will not work on a network that is using 464xlat because hasIpv4Address will not be
        // true.
        // TODO: need to consider 464xlat.
        if (ns != null && ns.linkProperties != null && ns.linkProperties.hasIpv4Address()) {
            // TODO: support ether ip upstream interface.
            final String ifaceName = ns.linkProperties.getInterfaceName();
            final InterfaceParams params = mDeps.getInterfaceParams(ifaceName);
            final boolean isVcn = isVcnInterface(ifaceName);
            if (!isVcn && params != null && !params.hasMacAddress /* raw ip upstream only */) {
                upstreamIndex = params.index;
            }
        }
        if (mLastIPv4UpstreamIfindex == upstreamIndex) return;

        // Clear existing rules if upstream interface is changed. The existing rules should be
        // cleared before upstream index mapping is cleared. It can avoid that ipServer or
        // conntrack event may use the non-existing upstream interfeace index to build a removing
        // key while removeing the rules. Can't notify each IpServer to clear the rules as
        // IPv6TetheringCoordinator#updateUpstreamNetworkState because the IpServer may not
        // handle the upstream changing notification before changing upstream index mapping.
        if (mLastIPv4UpstreamIfindex != 0) {
            // Clear all forwarding IPv4 rules for all downstreams.
            for (final IpServer ipserver : mTetherClients.keySet()) {
                tetherOffloadRule4Clear(ipserver);
            }
        }

        // Don't update mLastIPv4UpstreamIfindex before clearing existing rules if any. Need that
        // to tell if it is required to clean the out-of-date rules.
        mLastIPv4UpstreamIfindex = upstreamIndex;

        // If link properties are valid, build the upstream information mapping. Otherwise, clear
        // the upstream interface index mapping, to ensure that any conntrack events that arrive
        // after the upstream is lost do not incorrectly add rules pointing at the upstream.
        if (upstreamIndex == 0) {
            mIpv4UpstreamIndices.clear();
            return;
        }
        Collection<InetAddress> addresses = ns.linkProperties.getAddresses();
        for (final InetAddress addr: addresses) {
            if (isValidUpstreamIpv4Address(addr)) {
                mIpv4UpstreamIndices.put((Inet4Address) addr, upstreamIndex);
            }
        }
    }

    /**
     * Attach BPF program
     *
     * TODO: consider error handling if the attach program failed.
     */
    public void maybeAttachProgram(@NonNull String intIface, @NonNull String extIface) {
        if (isVcnInterface(extIface)) return;

        if (forwardingPairExists(intIface, extIface)) return;

        boolean firstDownstreamForThisUpstream = !isAnyForwardingPairOnUpstream(extIface);
        forwardingPairAdd(intIface, extIface);

        mBpfCoordinatorShim.attachProgram(intIface, UPSTREAM);
        // Attach if the upstream is the first time to be used in a forwarding pair.
        if (firstDownstreamForThisUpstream) {
            mBpfCoordinatorShim.attachProgram(extIface, DOWNSTREAM);
        }
    }

    /**
     * Detach BPF program
     */
    public void maybeDetachProgram(@NonNull String intIface, @NonNull String extIface) {
        forwardingPairRemove(intIface, extIface);

        // Detaching program may fail because the interface has been removed already.
        mBpfCoordinatorShim.detachProgram(intIface);
        // Detach if no more forwarding pair is using the upstream.
        if (!isAnyForwardingPairOnUpstream(extIface)) {
            mBpfCoordinatorShim.detachProgram(extIface);
        }
    }

    // TODO: make mInterfaceNames accessible to the shim and move this code to there.
    private String getIfName(long ifindex) {
        return mInterfaceNames.get((int) ifindex, Long.toString(ifindex));
    }

    /**
     * Dump information.
     * Block the function until all the data are dumped on the handler thread or timed-out. The
     * reason is that dumpsys invokes this function on the thread of caller and the data may only
     * be allowed to be accessed on the handler thread.
     */
    public void dump(@NonNull IndentingPrintWriter pw) {
        pw.println("mIsBpfEnabled: " + mIsBpfEnabled);
        pw.println("Polling " + (mPollingStarted ? "started" : "not started"));
        pw.println("Stats provider " + (mStatsProvider != null
                ? "registered" : "not registered"));
        pw.println("Upstream quota: " + mInterfaceQuotas.toString());
        pw.println("Polling interval: " + getPollingInterval() + " ms");
        pw.println("Bpf shim: " + mBpfCoordinatorShim.toString());

        pw.println("Forwarding stats:");
        pw.increaseIndent();
        if (mStats.size() == 0) {
            pw.println("<empty>");
        } else {
            dumpStats(pw);
        }
        pw.decreaseIndent();

        pw.println("BPF stats:");
        pw.increaseIndent();
        dumpBpfStats(pw);
        pw.decreaseIndent();
        pw.println();

        pw.println("Forwarding rules:");
        pw.increaseIndent();
        dumpIpv6UpstreamRules(pw);
        dumpIpv6ForwardingRules(pw);
        dumpIpv4ForwardingRules(pw);
        pw.decreaseIndent();
        pw.println();

        pw.println("Device map:");
        pw.increaseIndent();
        dumpDevmap(pw);
        pw.decreaseIndent();

        pw.println("Client Information:");
        pw.increaseIndent();
        if (mTetherClients.isEmpty()) {
            pw.println("<empty>");
        } else {
            pw.println(mTetherClients.toString());
        }
        pw.decreaseIndent();

        pw.println("IPv4 Upstream Indices:");
        pw.increaseIndent();
        if (mIpv4UpstreamIndices.isEmpty()) {
            pw.println("<empty>");
        } else {
            pw.println(mIpv4UpstreamIndices.toString());
        }
        pw.decreaseIndent();

        pw.println();
        pw.println("Forwarding counters:");
        pw.increaseIndent();
        dumpCounters(pw);
        pw.decreaseIndent();
    }

    private void dumpStats(@NonNull IndentingPrintWriter pw) {
        for (int i = 0; i < mStats.size(); i++) {
            final int upstreamIfindex = mStats.keyAt(i);
            final ForwardedStats stats = mStats.get(upstreamIfindex);
            pw.println(String.format("%d(%s) - %s", upstreamIfindex, mInterfaceNames.get(
                    upstreamIfindex), stats.toString()));
        }
    }
    private void dumpBpfStats(@NonNull IndentingPrintWriter pw) {
        try (BpfMap<TetherStatsKey, TetherStatsValue> map = mDeps.getBpfStatsMap()) {
            if (map == null) {
                pw.println("No BPF stats map");
                return;
            }
            if (map.isEmpty()) {
                pw.println("<empty>");
            }
            map.forEach((k, v) -> {
                pw.println(String.format("%s: %s", k, v));
            });
        } catch (ErrnoException e) {
            pw.println("Error dumping BPF stats map: " + e);
        }
    }

    private void dumpIpv6ForwardingRules(@NonNull IndentingPrintWriter pw) {
        if (mIpv6ForwardingRules.size() == 0) {
            pw.println("No IPv6 rules");
            return;
        }

        for (Map.Entry<IpServer, LinkedHashMap<Inet6Address, Ipv6ForwardingRule>> entry :
                mIpv6ForwardingRules.entrySet()) {
            IpServer ipServer = entry.getKey();
            // The rule downstream interface index is paired with the interface name from
            // IpServer#interfaceName. See #startIPv6, #updateIpv6ForwardingRules in IpServer.
            final String downstreamIface = ipServer.interfaceName();
            pw.println("[" + downstreamIface + "]: iif(iface) oif(iface) v6addr srcmac dstmac");

            pw.increaseIndent();
            LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules = entry.getValue();
            for (Ipv6ForwardingRule rule : rules.values()) {
                final int upstreamIfindex = rule.upstreamIfindex;
                pw.println(String.format("%d(%s) %d(%s) %s %s %s", upstreamIfindex,
                        mInterfaceNames.get(upstreamIfindex), rule.downstreamIfindex,
                        downstreamIface, rule.address.getHostAddress(), rule.srcMac, rule.dstMac));
            }
            pw.decreaseIndent();
        }
    }

    private String ipv6UpstreamRuletoString(TetherUpstream6Key key, Tether6Value value) {
        return String.format("%d(%s) %s -> %d(%s) %04x %s %s",
                key.iif, getIfName(key.iif), key.dstMac, value.oif, getIfName(value.oif),
                value.ethProto, value.ethSrcMac, value.ethDstMac);
    }

    private void dumpIpv6UpstreamRules(IndentingPrintWriter pw) {
        try (BpfMap<TetherUpstream6Key, Tether6Value> map = mDeps.getBpfUpstream6Map()) {
            if (map == null) {
                pw.println("No IPv6 upstream");
                return;
            }
            if (map.isEmpty()) {
                pw.println("No IPv6 upstream rules");
                return;
            }
            map.forEach((k, v) -> pw.println(ipv6UpstreamRuletoString(k, v)));
        } catch (ErrnoException e) {
            pw.println("Error dumping IPv6 upstream map: " + e);
        }
    }

    private String ipv4RuleToString(long now, boolean downstream,
            Tether4Key key, Tether4Value value) {
        final String src4, public4, dst4;
        final int publicPort;
        try {
            src4 = InetAddress.getByAddress(key.src4).getHostAddress();
            if (downstream) {
                public4 = InetAddress.getByAddress(key.dst4).getHostAddress();
                publicPort = key.dstPort;
            } else {
                public4 = InetAddress.getByAddress(value.src46).getHostAddress();
                publicPort = value.srcPort;
            }
            dst4 = InetAddress.getByAddress(value.dst46).getHostAddress();
        } catch (UnknownHostException impossible) {
            throw new AssertionError("IP address array not valid IPv4 address!");
        }

        final String protoStr = (key.l4proto == OsConstants.IPPROTO_TCP) ? "tcp" : "udp";
        final String ageStr = (value.lastUsed == 0) ? "-"
                : String.format("%dms", (now - value.lastUsed) / 1_000_000);
        return String.format("%s [%s] %d(%s) %s:%d -> %d(%s) %s:%d -> %s:%d [%s] %s",
                protoStr, key.dstMac, key.iif, getIfName(key.iif), src4, key.srcPort,
                value.oif, getIfName(value.oif),
                public4, publicPort, dst4, value.dstPort, value.ethDstMac, ageStr);
    }

    private void dumpIpv4ForwardingRuleMap(long now, boolean downstream,
            BpfMap<Tether4Key, Tether4Value> map, IndentingPrintWriter pw) throws ErrnoException {
        if (map == null) {
            pw.println("No IPv4 support");
            return;
        }
        if (map.isEmpty()) {
            pw.println("No rules");
            return;
        }
        map.forEach((k, v) -> pw.println(ipv4RuleToString(now, downstream, k, v)));
    }

    private void dumpIpv4ForwardingRules(IndentingPrintWriter pw) {
        final long now = SystemClock.elapsedRealtimeNanos();

        try (BpfMap<Tether4Key, Tether4Value> upstreamMap = mDeps.getBpfUpstream4Map();
                BpfMap<Tether4Key, Tether4Value> downstreamMap = mDeps.getBpfDownstream4Map()) {
            pw.println("IPv4 Upstream: proto [inDstMac] iif(iface) src -> nat -> "
                    + "dst [outDstMac] age");
            pw.increaseIndent();
            dumpIpv4ForwardingRuleMap(now, UPSTREAM, upstreamMap, pw);
            pw.decreaseIndent();

            pw.println("IPv4 Downstream: proto [inDstMac] iif(iface) src -> nat -> "
                    + "dst [outDstMac] age");
            pw.increaseIndent();
            dumpIpv4ForwardingRuleMap(now, DOWNSTREAM, downstreamMap, pw);
            pw.decreaseIndent();
        } catch (ErrnoException e) {
            pw.println("Error dumping IPv4 map: " + e);
        }
    }

    /**
     * Simple struct that only contains a u32. Must be public because Struct needs access to it.
     * TODO: make this a public inner class of Struct so anyone can use it as, e.g., Struct.U32?
     */
    public static class U32Struct extends Struct {
        @Struct.Field(order = 0, type = Struct.Type.U32)
        public long val;
    }

    private void dumpCounters(@NonNull IndentingPrintWriter pw) {
        if (!mDeps.isAtLeastS()) {
            pw.println("No counter support");
            return;
        }
        try (BpfMap<U32Struct, U32Struct> map = new BpfMap<>(TETHER_ERROR_MAP_PATH,
                BpfMap.BPF_F_RDONLY, U32Struct.class, U32Struct.class)) {

            map.forEach((k, v) -> {
                String counterName;
                try {
                    counterName = sBpfCounterNames[(int) k.val];
                } catch (IndexOutOfBoundsException e) {
                    // Should never happen because this code gets the counter name from the same
                    // include file as the BPF program that increments the counter.
                    Log.wtf(TAG, "Unknown tethering counter type " + k.val);
                    counterName = Long.toString(k.val);
                }
                if (v.val > 0) pw.println(String.format("%s: %d", counterName, v.val));
            });
        } catch (ErrnoException e) {
            pw.println("Error dumping counter map: " + e);
        }
    }

    private void dumpDevmap(@NonNull IndentingPrintWriter pw) {
        try (BpfMap<TetherDevKey, TetherDevValue> map = mDeps.getBpfDevMap()) {
            if (map == null) {
                pw.println("No devmap support");
                return;
            }
            if (map.isEmpty()) {
                pw.println("<empty>");
                return;
            }
            pw.println("ifindex (iface) -> ifindex (iface)");
            pw.increaseIndent();
            map.forEach((k, v) -> {
                // Only get upstream interface name. Just do the best to make the index readable.
                // TODO: get downstream interface name because the index is either upstream or
                // downstream interface in dev map.
                pw.println(String.format("%d (%s) -> %d (%s)", k.ifIndex, getIfName(k.ifIndex),
                        v.ifIndex, getIfName(v.ifIndex)));
            });
        } catch (ErrnoException e) {
            pw.println("Error dumping dev map: " + e);
        }
        pw.decreaseIndent();
    }

    /** IPv6 forwarding rule class. */
    public static class Ipv6ForwardingRule {
        // The upstream6 and downstream6 rules are built as the following tables. Only raw ip
        // upstream interface is supported.
        // TODO: support ether ip upstream interface.
        //
        // NAT network topology:
        //
        //         public network (rawip)                 private network
        //                   |                 UE                |
        // +------------+    V    +------------+------------+    V    +------------+
        // |   Sever    +---------+  Upstream  | Downstream +---------+   Client   |
        // +------------+         +------------+------------+         +------------+
        //
        // upstream6 key and value:
        //
        // +------+-------------+
        // | TetherUpstream6Key |
        // +------+------+------+
        // |field |iif   |dstMac|
        // |      |      |      |
        // +------+------+------+
        // |value |downst|downst|
        // |      |ream  |ream  |
        // +------+------+------+
        //
        // +------+----------------------------------+
        // |      |Tether6Value                      |
        // +------+------+------+------+------+------+
        // |field |oif   |ethDst|ethSrc|ethPro|pmtu  |
        // |      |      |mac   |mac   |to    |      |
        // +------+------+------+------+------+------+
        // |value |upstre|--    |--    |ETH_P_|1500  |
        // |      |am    |      |      |IP    |      |
        // +------+------+------+------+------+------+
        //
        // downstream6 key and value:
        //
        // +------+--------------------+
        // |      |TetherDownstream6Key|
        // +------+------+------+------+
        // |field |iif   |dstMac|neigh6|
        // |      |      |      |      |
        // +------+------+------+------+
        // |value |upstre|--    |client|
        // |      |am    |      |      |
        // +------+------+------+------+
        //
        // +------+----------------------------------+
        // |      |Tether6Value                      |
        // +------+------+------+------+------+------+
        // |field |oif   |ethDst|ethSrc|ethPro|pmtu  |
        // |      |      |mac   |mac   |to    |      |
        // +------+------+------+------+------+------+
        // |value |downst|client|downst|ETH_P_|1500  |
        // |      |ream  |      |ream  |IP    |      |
        // +------+------+------+------+------+------+
        //
        public final int upstreamIfindex;
        public final int downstreamIfindex;

        // TODO: store a ClientInfo object instead of storing address, srcMac, and dstMac directly.
        @NonNull
        public final Inet6Address address;
        @NonNull
        public final MacAddress srcMac;
        @NonNull
        public final MacAddress dstMac;

        public Ipv6ForwardingRule(int upstreamIfindex, int downstreamIfIndex,
                @NonNull Inet6Address address, @NonNull MacAddress srcMac,
                @NonNull MacAddress dstMac) {
            this.upstreamIfindex = upstreamIfindex;
            this.downstreamIfindex = downstreamIfIndex;
            this.address = address;
            this.srcMac = srcMac;
            this.dstMac = dstMac;
        }

        /** Return a new rule object which updates with new upstream index. */
        @NonNull
        public Ipv6ForwardingRule onNewUpstream(int newUpstreamIfindex) {
            return new Ipv6ForwardingRule(newUpstreamIfindex, downstreamIfindex, address, srcMac,
                    dstMac);
        }

        /**
         * Don't manipulate TetherOffloadRuleParcel directly because implementing onNewUpstream()
         * would be error-prone due to generated stable AIDL classes not having a copy constructor.
         */
        @NonNull
        public TetherOffloadRuleParcel toTetherOffloadRuleParcel() {
            final TetherOffloadRuleParcel parcel = new TetherOffloadRuleParcel();
            parcel.inputInterfaceIndex = upstreamIfindex;
            parcel.outputInterfaceIndex = downstreamIfindex;
            parcel.destination = address.getAddress();
            parcel.prefixLength = 128;
            parcel.srcL2Address = srcMac.toByteArray();
            parcel.dstL2Address = dstMac.toByteArray();
            return parcel;
        }

        /**
         * Return a TetherDownstream6Key object built from the rule.
         */
        @NonNull
        public TetherDownstream6Key makeTetherDownstream6Key() {
            return new TetherDownstream6Key(upstreamIfindex, NULL_MAC_ADDRESS,
                    address.getAddress());
        }

        /**
         * Return a Tether6Value object built from the rule.
         */
        @NonNull
        public Tether6Value makeTether6Value() {
            return new Tether6Value(downstreamIfindex, dstMac, srcMac, ETH_P_IPV6,
                    NetworkStackConstants.ETHER_MTU);
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof Ipv6ForwardingRule)) return false;
            Ipv6ForwardingRule that = (Ipv6ForwardingRule) o;
            return this.upstreamIfindex == that.upstreamIfindex
                    && this.downstreamIfindex == that.downstreamIfindex
                    && Objects.equals(this.address, that.address)
                    && Objects.equals(this.srcMac, that.srcMac)
                    && Objects.equals(this.dstMac, that.dstMac);
        }

        @Override
        public int hashCode() {
            // TODO: if this is ever used in production code, don't pass ifindices
            // to Objects.hash() to avoid autoboxing overhead.
            return Objects.hash(upstreamIfindex, downstreamIfindex, address, srcMac, dstMac);
        }
    }

    /** Tethering client information class. */
    public static class ClientInfo {
        public final int downstreamIfindex;

        @NonNull
        public final MacAddress downstreamMac;
        @NonNull
        public final Inet4Address clientAddress;
        @NonNull
        public final MacAddress clientMac;

        public ClientInfo(int downstreamIfindex,
                @NonNull MacAddress downstreamMac, @NonNull Inet4Address clientAddress,
                @NonNull MacAddress clientMac) {
            this.downstreamIfindex = downstreamIfindex;
            this.downstreamMac = downstreamMac;
            this.clientAddress = clientAddress;
            this.clientMac = clientMac;
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof ClientInfo)) return false;
            ClientInfo that = (ClientInfo) o;
            return this.downstreamIfindex == that.downstreamIfindex
                    && Objects.equals(this.downstreamMac, that.downstreamMac)
                    && Objects.equals(this.clientAddress, that.clientAddress)
                    && Objects.equals(this.clientMac, that.clientMac);
        }

        @Override
        public int hashCode() {
            return Objects.hash(downstreamIfindex, downstreamMac, clientAddress, clientMac);
        }

        @Override
        public String toString() {
            return String.format("downstream: %d (%s), client: %s (%s)",
                    downstreamIfindex, downstreamMac, clientAddress, clientMac);
        }
    }

    /**
     * A BPF tethering stats provider to provide network statistics to the system.
     * Note that this class' data may only be accessed on the handler thread.
     */
    @VisibleForTesting
    class BpfTetherStatsProvider extends NetworkStatsProvider {
        // The offloaded traffic statistics per interface that has not been reported since the
        // last call to pushTetherStats. Only the interfaces that were ever tethering upstreams
        // and has pending tether stats delta are included in this NetworkStats object.
        private NetworkStats mIfaceStats = new NetworkStats(0L, 0);

        // The same stats as above, but counts network stats per uid.
        private NetworkStats mUidStats = new NetworkStats(0L, 0);

        @Override
        public void onRequestStatsUpdate(int token) {
            mHandler.post(() -> pushTetherStats());
        }

        @Override
        public void onSetAlert(long quotaBytes) {
            mHandler.post(() -> updateAlertQuota(quotaBytes));
        }

        @Override
        public void onSetLimit(@NonNull String iface, long quotaBytes) {
            if (quotaBytes < QUOTA_UNLIMITED) {
                throw new IllegalArgumentException("invalid quota value " + quotaBytes);
            }

            mHandler.post(() -> {
                final Long curIfaceQuota = mInterfaceQuotas.get(iface);

                if (null == curIfaceQuota && QUOTA_UNLIMITED == quotaBytes) return;

                if (quotaBytes == QUOTA_UNLIMITED) {
                    mInterfaceQuotas.remove(iface);
                } else {
                    mInterfaceQuotas.put(iface, quotaBytes);
                }
                maybeUpdateDataLimit(iface);
            });
        }

        @VisibleForTesting
        void pushTetherStats() {
            try {
                // The token is not used for now. See b/153606961.
                notifyStatsUpdated(0 /* token */, mIfaceStats, mUidStats);

                // Clear the accumulated tether stats delta after reported. Note that create a new
                // empty object because NetworkStats#clear is @hide.
                mIfaceStats = new NetworkStats(0L, 0);
                mUidStats = new NetworkStats(0L, 0);
            } catch (RuntimeException e) {
                mLog.e("Cannot report network stats: ", e);
            }
        }

        private void accumulateDiff(@NonNull NetworkStats ifaceDiff,
                @NonNull NetworkStats uidDiff) {
            mIfaceStats = mIfaceStats.add(ifaceDiff);
            mUidStats = mUidStats.add(uidDiff);
        }
    }

    @Nullable
    private ClientInfo getClientInfo(@NonNull Inet4Address clientAddress) {
        for (HashMap<Inet4Address, ClientInfo> clients : mTetherClients.values()) {
            for (ClientInfo client : clients.values()) {
                if (clientAddress.equals(client.clientAddress)) {
                    return client;
                }
            }
        }
        return null;
    }

    @NonNull
    private byte[] toIpv4MappedAddressBytes(Inet4Address ia4) {
        final byte[] addr4 = ia4.getAddress();
        final byte[] addr6 = new byte[16];
        addr6[10] = (byte) 0xff;
        addr6[11] = (byte) 0xff;
        addr6[12] = addr4[0];
        addr6[13] = addr4[1];
        addr6[14] = addr4[2];
        addr6[15] = addr4[3];
        return addr6;
    }

    @Nullable
    private Inet4Address ipv4MappedAddressBytesToIpv4Address(final byte[] addr46) {
        if (addr46.length != 16) return null;
        if (addr46[0] != 0 || addr46[1] != 0 || addr46[2] != 0 || addr46[3] != 0
                || addr46[4] != 0 || addr46[5] != 0 || addr46[6] != 0 || addr46[7] != 0
                || addr46[8] != 0 && addr46[9] != 0 || (addr46[10] & 0xff) != 0xff
                || (addr46[11] & 0xff) != 0xff) {
            return null;
        }

        final byte[] addr4 = new byte[4];
        addr4[0] = addr46[12];
        addr4[1] = addr46[13];
        addr4[2] = addr46[14];
        addr4[3] = addr46[15];

        return parseIPv4Address(addr4);
    }

    // TODO: parse CTA_PROTOINFO of conntrack event in ConntrackMonitor. For TCP, only add rules
    // while TCP status is established.
    @VisibleForTesting
    class BpfConntrackEventConsumer implements ConntrackEventConsumer {
        // The upstream4 and downstream4 rules are built as the following tables. Only raw ip
        // upstream interface is supported. Note that the field "lastUsed" is only updated by
        // BPF program which records the last used time for a given rule.
        // TODO: support ether ip upstream interface.
        //
        // NAT network topology:
        //
        //         public network (rawip)                 private network
        //                   |                 UE                |
        // +------------+    V    +------------+------------+    V    +------------+
        // |   Sever    +---------+  Upstream  | Downstream +---------+   Client   |
        // +------------+         +------------+------------+         +------------+
        //
        // upstream4 key and value:
        //
        // +------+------------------------------------------------+
        // |      |      TetherUpstream4Key                        |
        // +------+------+------+------+------+------+------+------+
        // |field |iif   |dstMac|l4prot|src4  |dst4  |srcPor|dstPor|
        // |      |      |      |o     |      |      |t     |t     |
        // +------+------+------+------+------+------+------+------+
        // |value |downst|downst|tcp/  |client|server|client|server|
        // |      |ream  |ream  |udp   |      |      |      |      |
        // +------+------+------+------+------+------+------+------+
        //
        // +------+---------------------------------------------------------------------+
        // |      |      TetherUpstream4Value                                           |
        // +------+------+------+------+------+------+------+------+------+------+------+
        // |field |oif   |ethDst|ethSrc|ethPro|pmtu  |src46 |dst46 |srcPor|dstPor|lastUs|
        // |      |      |mac   |mac   |to    |      |      |      |t     |t     |ed    |
        // +------+------+------+------+------+------+------+------+------+------+------+
        // |value |upstre|--    |--    |ETH_P_|1500  |upstre|server|upstre|server|--    |
        // |      |am    |      |      |IP    |      |am    |      |am    |      |      |
        // +------+------+------+------+------+------+------+------+------+------+------+
        //
        // downstream4 key and value:
        //
        // +------+------------------------------------------------+
        // |      |      TetherDownstream4Key                      |
        // +------+------+------+------+------+------+------+------+
        // |field |iif   |dstMac|l4prot|src4  |dst4  |srcPor|dstPor|
        // |      |      |      |o     |      |      |t     |t     |
        // +------+------+------+------+------+------+------+------+
        // |value |upstre|--    |tcp/  |server|upstre|server|upstre|
        // |      |am    |      |udp   |      |am    |      |am    |
        // +------+------+------+------+------+------+------+------+
        //
        // +------+---------------------------------------------------------------------+
        // |      |      TetherDownstream4Value                                         |
        // +------+------+------+------+------+------+------+------+------+------+------+
        // |field |oif   |ethDst|ethSrc|ethPro|pmtu  |src46 |dst46 |srcPor|dstPor|lastUs|
        // |      |      |mac   |mac   |to    |      |      |      |t     |t     |ed    |
        // +------+------+------+------+------+------+------+------+------+------+------+
        // |value |downst|client|downst|ETH_P_|1500  |server|client|server|client|--    |
        // |      |ream  |      |ream  |IP    |      |      |      |      |      |      |
        // +------+------+------+------+------+------+------+------+------+------+------+
        //
        @NonNull
        private Tether4Key makeTetherUpstream4Key(
                @NonNull ConntrackEvent e, @NonNull ClientInfo c) {
            return new Tether4Key(c.downstreamIfindex, c.downstreamMac,
                    e.tupleOrig.protoNum, e.tupleOrig.srcIp.getAddress(),
                    e.tupleOrig.dstIp.getAddress(), e.tupleOrig.srcPort, e.tupleOrig.dstPort);
        }

        @NonNull
        private Tether4Key makeTetherDownstream4Key(
                @NonNull ConntrackEvent e, @NonNull ClientInfo c, int upstreamIndex) {
            return new Tether4Key(upstreamIndex, NULL_MAC_ADDRESS /* dstMac (rawip) */,
                    e.tupleReply.protoNum, e.tupleReply.srcIp.getAddress(),
                    e.tupleReply.dstIp.getAddress(), e.tupleReply.srcPort, e.tupleReply.dstPort);
        }

        @NonNull
        private Tether4Value makeTetherUpstream4Value(@NonNull ConntrackEvent e,
                int upstreamIndex) {
            return new Tether4Value(upstreamIndex,
                    NULL_MAC_ADDRESS /* ethDstMac (rawip) */,
                    NULL_MAC_ADDRESS /* ethSrcMac (rawip) */, ETH_P_IP,
                    NetworkStackConstants.ETHER_MTU, toIpv4MappedAddressBytes(e.tupleReply.dstIp),
                    toIpv4MappedAddressBytes(e.tupleReply.srcIp), e.tupleReply.dstPort,
                    e.tupleReply.srcPort, 0 /* lastUsed, filled by bpf prog only */);
        }

        @NonNull
        private Tether4Value makeTetherDownstream4Value(@NonNull ConntrackEvent e,
                @NonNull ClientInfo c, int upstreamIndex) {
            return new Tether4Value(c.downstreamIfindex,
                    c.clientMac, c.downstreamMac, ETH_P_IP, NetworkStackConstants.ETHER_MTU,
                    toIpv4MappedAddressBytes(e.tupleOrig.dstIp),
                    toIpv4MappedAddressBytes(e.tupleOrig.srcIp),
                    e.tupleOrig.dstPort, e.tupleOrig.srcPort,
                    0 /* lastUsed, filled by bpf prog only */);
        }

        public void accept(ConntrackEvent e) {
            final ClientInfo tetherClient = getClientInfo(e.tupleOrig.srcIp);
            if (tetherClient == null) return;

            final Integer upstreamIndex = mIpv4UpstreamIndices.get(e.tupleReply.dstIp);
            if (upstreamIndex == null) return;

            final Tether4Key upstream4Key = makeTetherUpstream4Key(e, tetherClient);
            final Tether4Key downstream4Key = makeTetherDownstream4Key(e, tetherClient,
                    upstreamIndex);

            if (e.msgType == (NetlinkConstants.NFNL_SUBSYS_CTNETLINK << 8
                    | NetlinkConstants.IPCTNL_MSG_CT_DELETE)) {
                final boolean deletedUpstream = mBpfCoordinatorShim.tetherOffloadRuleRemove(
                        UPSTREAM, upstream4Key);
                final boolean deletedDownstream = mBpfCoordinatorShim.tetherOffloadRuleRemove(
                        DOWNSTREAM, downstream4Key);

                if (!deletedUpstream && !deletedDownstream) {
                    // The rules may have been already removed by losing client or losing upstream.
                    return;
                }

                if (deletedUpstream != deletedDownstream) {
                    Log.wtf(TAG, "The bidirectional rules should be removed concurrently ("
                            + "upstream: " + deletedUpstream
                            + ", downstream: " + deletedDownstream + ")");
                    return;
                }

                maybeClearLimit(upstreamIndex);
                return;
            }

            final Tether4Value upstream4Value = makeTetherUpstream4Value(e, upstreamIndex);
            final Tether4Value downstream4Value = makeTetherDownstream4Value(e, tetherClient,
                    upstreamIndex);

            maybeAddDevMap(upstreamIndex, tetherClient.downstreamIfindex);
            maybeSetLimit(upstreamIndex);
            mBpfCoordinatorShim.tetherOffloadRuleAdd(UPSTREAM, upstream4Key, upstream4Value);
            mBpfCoordinatorShim.tetherOffloadRuleAdd(DOWNSTREAM, downstream4Key, downstream4Value);
        }
    }

    private boolean isBpfEnabled() {
        final TetheringConfiguration config = mDeps.getTetherConfig();
        return (config != null) ? config.isBpfOffloadEnabled() : true /* default value */;
    }

    private int getInterfaceIndexFromRules(@NonNull String ifName) {
        for (LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules : mIpv6ForwardingRules
                .values()) {
            for (Ipv6ForwardingRule rule : rules.values()) {
                final int upstreamIfindex = rule.upstreamIfindex;
                if (TextUtils.equals(ifName, mInterfaceNames.get(upstreamIfindex))) {
                    return upstreamIfindex;
                }
            }
        }
        return 0;
    }

    private long getQuotaBytes(@NonNull String iface) {
        final Long limit = mInterfaceQuotas.get(iface);
        final long quotaBytes = (limit != null) ? limit : QUOTA_UNLIMITED;

        return quotaBytes;
    }

    private boolean sendDataLimitToBpfMap(int ifIndex, long quotaBytes) {
        if (ifIndex == 0) {
            Log.wtf(TAG, "Invalid interface index.");
            return false;
        }

        return mBpfCoordinatorShim.tetherOffloadSetInterfaceQuota(ifIndex, quotaBytes);
    }

    // Handle the data limit update from the service which is the stats provider registered for.
    private void maybeUpdateDataLimit(@NonNull String iface) {
        // Set data limit only on a given upstream which has at least one rule. If we can't get
        // an interface index for a given interface name, it means either there is no rule for
        // a given upstream or the interface name is not an upstream which is monitored by the
        // coordinator.
        final int ifIndex = getInterfaceIndexFromRules(iface);
        if (ifIndex == 0) return;

        final long quotaBytes = getQuotaBytes(iface);
        sendDataLimitToBpfMap(ifIndex, quotaBytes);
    }

    // Handle the data limit update while adding forwarding rules.
    private boolean updateDataLimit(int ifIndex) {
        final String iface = mInterfaceNames.get(ifIndex);
        if (iface == null) {
            mLog.e("Fail to get the interface name for index " + ifIndex);
            return false;
        }
        final long quotaBytes = getQuotaBytes(iface);
        return sendDataLimitToBpfMap(ifIndex, quotaBytes);
    }

    private void maybeSetLimit(int upstreamIfindex) {
        if (isAnyRuleOnUpstream(upstreamIfindex)
                || mBpfCoordinatorShim.isAnyIpv4RuleOnUpstream(upstreamIfindex)) {
            return;
        }

        // If failed to set a data limit, probably should not use this upstream, because
        // the upstream may not want to blow through the data limit that was told to apply.
        // TODO: Perhaps stop the coordinator.
        boolean success = updateDataLimit(upstreamIfindex);
        if (!success) {
            final String iface = mInterfaceNames.get(upstreamIfindex);
            mLog.e("Setting data limit for " + iface + " failed.");
        }
    }

    // TODO: This should be also called while IpServer wants to clear all IPv4 rules. Relying on
    // conntrack event can't cover this case.
    private void maybeClearLimit(int upstreamIfindex) {
        if (isAnyRuleOnUpstream(upstreamIfindex)
                || mBpfCoordinatorShim.isAnyIpv4RuleOnUpstream(upstreamIfindex)) {
            return;
        }

        final TetherStatsValue statsValue =
                mBpfCoordinatorShim.tetherOffloadGetAndClearStats(upstreamIfindex);
        if (statsValue == null) {
            Log.wtf(TAG, "Fail to cleanup tether stats for upstream index " + upstreamIfindex);
            return;
        }

        SparseArray<TetherStatsValue> tetherStatsList = new SparseArray<TetherStatsValue>();
        tetherStatsList.put(upstreamIfindex, statsValue);

        // Update the last stats delta and delete the local cache for a given upstream.
        updateQuotaAndStatsFromSnapshot(tetherStatsList);
        mStats.remove(upstreamIfindex);
    }

    // TODO: Rename to isAnyIpv6RuleOnUpstream and define an isAnyRuleOnUpstream method that called
    // both isAnyIpv6RuleOnUpstream and mBpfCoordinatorShim.isAnyIpv4RuleOnUpstream.
    private boolean isAnyRuleOnUpstream(int upstreamIfindex) {
        for (LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules : mIpv6ForwardingRules
                .values()) {
            for (Ipv6ForwardingRule rule : rules.values()) {
                if (upstreamIfindex == rule.upstreamIfindex) return true;
            }
        }
        return false;
    }

    private boolean isAnyRuleFromDownstreamToUpstream(int downstreamIfindex, int upstreamIfindex) {
        for (LinkedHashMap<Inet6Address, Ipv6ForwardingRule> rules : mIpv6ForwardingRules
                .values()) {
            for (Ipv6ForwardingRule rule : rules.values()) {
                if (downstreamIfindex == rule.downstreamIfindex
                        && upstreamIfindex == rule.upstreamIfindex) {
                    return true;
                }
            }
        }
        return false;
    }

    // TODO: remove the index from map while the interface has been removed because the map size
    // is 64 entries. See packages\modules\Connectivity\Tethering\bpf_progs\offload.c.
    private void maybeAddDevMap(int upstreamIfindex, int downstreamIfindex) {
        for (Integer index : new Integer[] {upstreamIfindex, downstreamIfindex}) {
            if (mDeviceMapSet.contains(index)) continue;
            if (mBpfCoordinatorShim.addDevMap(index)) mDeviceMapSet.add(index);
        }
    }

    private void forwardingPairAdd(@NonNull String intIface, @NonNull String extIface) {
        if (!mForwardingPairs.containsKey(extIface)) {
            mForwardingPairs.put(extIface, new HashSet<String>());
        }
        mForwardingPairs.get(extIface).add(intIface);
    }

    private void forwardingPairRemove(@NonNull String intIface, @NonNull String extIface) {
        HashSet<String> downstreams = mForwardingPairs.get(extIface);
        if (downstreams == null) return;
        if (!downstreams.remove(intIface)) return;

        if (downstreams.isEmpty()) {
            mForwardingPairs.remove(extIface);
        }
    }

    private boolean forwardingPairExists(@NonNull String intIface, @NonNull String extIface) {
        if (!mForwardingPairs.containsKey(extIface)) return false;

        return mForwardingPairs.get(extIface).contains(intIface);
    }

    private boolean isAnyForwardingPairOnUpstream(@NonNull String extIface) {
        return mForwardingPairs.containsKey(extIface);
    }

    @NonNull
    private NetworkStats buildNetworkStats(@NonNull StatsType type, int ifIndex,
            @NonNull final ForwardedStats diff) {
        NetworkStats stats = new NetworkStats(0L, 0);
        final String iface = mInterfaceNames.get(ifIndex);
        if (iface == null) {
            // TODO: Use Log.wtf once the coordinator owns full control of tether stats from netd.
            // For now, netd may add the empty stats for the upstream which is not monitored by
            // the coordinator. Silently ignore it.
            return stats;
        }
        final int uid = (type == StatsType.STATS_PER_UID) ? UID_TETHERING : UID_ALL;
        // Note that the argument 'metered', 'roaming' and 'defaultNetwork' are not recorded for
        // network stats snapshot. See NetworkStatsRecorder#recordSnapshotLocked.
        return stats.addEntry(new Entry(iface, uid, SET_DEFAULT, TAG_NONE, METERED_NO,
                ROAMING_NO, DEFAULT_NETWORK_NO, diff.rxBytes, diff.rxPackets,
                diff.txBytes, diff.txPackets, 0L /* operations */));
    }

    private void updateAlertQuota(long newQuota) {
        if (newQuota < QUOTA_UNLIMITED) {
            throw new IllegalArgumentException("invalid quota value " + newQuota);
        }
        if (mRemainingAlertQuota == newQuota) return;

        mRemainingAlertQuota = newQuota;
        if (mRemainingAlertQuota == 0) {
            mLog.i("onAlertReached");
            if (mStatsProvider != null) mStatsProvider.notifyAlertReached();
        }
    }

    private void updateQuotaAndStatsFromSnapshot(
            @NonNull final SparseArray<TetherStatsValue> tetherStatsList) {
        long usedAlertQuota = 0;
        for (int i = 0; i < tetherStatsList.size(); i++) {
            final Integer ifIndex = tetherStatsList.keyAt(i);
            final TetherStatsValue tetherStats = tetherStatsList.valueAt(i);
            final ForwardedStats curr = new ForwardedStats(tetherStats);
            final ForwardedStats base = mStats.get(ifIndex);
            final ForwardedStats diff = (base != null) ? curr.subtract(base) : curr;
            usedAlertQuota += diff.rxBytes + diff.txBytes;

            // Update the local cache for counting tether stats delta.
            mStats.put(ifIndex, curr);

            // Update the accumulated tether stats delta to the stats provider for the service
            // querying.
            if (mStatsProvider != null) {
                try {
                    mStatsProvider.accumulateDiff(
                            buildNetworkStats(StatsType.STATS_PER_IFACE, ifIndex, diff),
                            buildNetworkStats(StatsType.STATS_PER_UID, ifIndex, diff));
                } catch (ArrayIndexOutOfBoundsException e) {
                    Log.wtf(TAG, "Fail to update the accumulated stats delta for interface index "
                            + ifIndex + " : ", e);
                }
            }
        }

        if (mRemainingAlertQuota > 0 && usedAlertQuota > 0) {
            // Trim to zero if overshoot.
            final long newQuota = Math.max(mRemainingAlertQuota - usedAlertQuota, 0);
            updateAlertQuota(newQuota);
        }

        // TODO: Count the used limit quota for notifying data limit reached.
    }

    private void updateForwardedStats() {
        final SparseArray<TetherStatsValue> tetherStatsList =
                mBpfCoordinatorShim.tetherOffloadGetStats();

        if (tetherStatsList == null) {
            mLog.e("Problem fetching tethering stats");
            return;
        }

        updateQuotaAndStatsFromSnapshot(tetherStatsList);
    }

    @VisibleForTesting
    int getPollingInterval() {
        // The valid range of interval is DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS..max_long.
        // Ignore the config value is less than the minimum polling interval. Note that the
        // minimum interval definition is invoked as OffloadController#isPollingStatsNeeded does.
        // TODO: Perhaps define a minimum polling interval constant.
        final TetheringConfiguration config = mDeps.getTetherConfig();
        final int configInterval = (config != null) ? config.getOffloadPollInterval() : 0;
        return Math.max(DEFAULT_TETHER_OFFLOAD_POLL_INTERVAL_MS, configInterval);
    }

    @Nullable
    private Inet4Address parseIPv4Address(byte[] addrBytes) {
        try {
            final InetAddress ia = Inet4Address.getByAddress(addrBytes);
            if (ia instanceof Inet4Address) return (Inet4Address) ia;
        } catch (UnknownHostException | IllegalArgumentException e) {
            mLog.e("Failed to parse IPv4 address: " + e);
        }
        return null;
    }

    // Update CTA_TUPLE_ORIG timeout for a given conntrack entry. Note that there will also be
    // coming a conntrack event to notify updated timeout.
    private void updateConntrackTimeout(byte proto, Inet4Address src4, short srcPort,
            Inet4Address dst4, short dstPort) {
        if (src4 == null || dst4 == null) return;

        // TODO: consider acquiring the timeout setting from nf_conntrack_* variables.
        // - proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established
        // - proc/sys/net/netfilter/nf_conntrack_udp_timeout_stream
        // See kernel document nf_conntrack-sysctl.txt.
        final int timeoutSec = (proto == OsConstants.IPPROTO_TCP)
                ? NF_CONNTRACK_TCP_TIMEOUT_ESTABLISHED
                : NF_CONNTRACK_UDP_TIMEOUT_STREAM;
        final byte[] msg = ConntrackMessage.newIPv4TimeoutUpdateRequest(
                proto, src4, (int) srcPort, dst4, (int) dstPort, timeoutSec);
        try {
            NetlinkSocket.sendOneShotKernelMessage(OsConstants.NETLINK_NETFILTER, msg);
        } catch (ErrnoException e) {
            mLog.e("Error updating conntrack entry ("
                    + "proto: " + proto + ", "
                    + "src4: " + src4 + ", "
                    + "srcPort: " + Short.toUnsignedInt(srcPort) + ", "
                    + "dst4: " + dst4 + ", "
                    + "dstPort: " + Short.toUnsignedInt(dstPort) + "), "
                    + "msg: " + NetlinkConstants.hexify(msg) + ", "
                    + "e: " + e);
        }
    }

    private void maybeRefreshConntrackTimeout() {
        final long now = mDeps.elapsedRealtimeNanos();

        // Reverse the source and destination {address, port} from downstream value because
        // #updateConntrackTimeout refresh the timeout of netlink attribute CTA_TUPLE_ORIG
        // which is opposite direction for downstream map value.
        mBpfCoordinatorShim.tetherOffloadRuleForEach(DOWNSTREAM, (k, v) -> {
            if ((now - v.lastUsed) / 1_000_000 < POLLING_CONNTRACK_TIMEOUT_MS) {
                updateConntrackTimeout((byte) k.l4proto,
                        ipv4MappedAddressBytesToIpv4Address(v.dst46), (short) v.dstPort,
                        ipv4MappedAddressBytesToIpv4Address(v.src46), (short) v.srcPort);
            }
        });

        // TODO: Consider ignoring TCP traffic on upstream and monitor on downstream only
        // because TCP is a bidirectional traffic. Probably don't need to extend timeout by
        // both directions for TCP.
        mBpfCoordinatorShim.tetherOffloadRuleForEach(UPSTREAM, (k, v) -> {
            if ((now - v.lastUsed) / 1_000_000 < POLLING_CONNTRACK_TIMEOUT_MS) {
                updateConntrackTimeout((byte) k.l4proto, parseIPv4Address(k.src4),
                        (short) k.srcPort, parseIPv4Address(k.dst4), (short) k.dstPort);
            }
        });
    }

    private void maybeSchedulePollingStats() {
        if (!mPollingStarted) return;

        if (mHandler.hasCallbacks(mScheduledPollingStats)) {
            mHandler.removeCallbacks(mScheduledPollingStats);
        }

        mHandler.postDelayed(mScheduledPollingStats, getPollingInterval());
    }

    private void maybeSchedulePollingConntrackTimeout() {
        if (!mPollingStarted) return;

        if (mHandler.hasCallbacks(mScheduledPollingConntrackTimeout)) {
            mHandler.removeCallbacks(mScheduledPollingConntrackTimeout);
        }

        mHandler.postDelayed(mScheduledPollingConntrackTimeout, POLLING_CONNTRACK_TIMEOUT_MS);
    }

    // Return forwarding rule map. This is used for testing only.
    // Note that this can be only called on handler thread.
    @NonNull
    @VisibleForTesting
    final HashMap<IpServer, LinkedHashMap<Inet6Address, Ipv6ForwardingRule>>
            getForwardingRulesForTesting() {
        return mIpv6ForwardingRules;
    }

    // Return upstream interface name map. This is used for testing only.
    // Note that this can be only called on handler thread.
    @NonNull
    @VisibleForTesting
    final SparseArray<String> getInterfaceNamesForTesting() {
        return mInterfaceNames;
    }

    // Return BPF conntrack event consumer. This is used for testing only.
    // Note that this can be only called on handler thread.
    @NonNull
    @VisibleForTesting
    final BpfConntrackEventConsumer getBpfConntrackEventConsumerForTesting() {
        return mBpfConntrackEventConsumer;
    }

    private static native String[] getBpfCounterNames();
}
