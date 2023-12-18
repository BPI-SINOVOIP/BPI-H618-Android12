/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.net.ip;

import static android.net.dhcp.DhcpClient.EXPIRED_LEASE;
import static android.net.dhcp.DhcpPacket.DHCP_BOOTREQUEST;
import static android.net.dhcp.DhcpPacket.DHCP_CLIENT;
import static android.net.dhcp.DhcpPacket.DHCP_IPV6_ONLY_PREFERRED;
import static android.net.dhcp.DhcpPacket.DHCP_MAGIC_COOKIE;
import static android.net.dhcp.DhcpPacket.DHCP_SERVER;
import static android.net.dhcp.DhcpPacket.ENCAP_L2;
import static android.net.dhcp.DhcpPacket.INADDR_BROADCAST;
import static android.net.dhcp.DhcpPacket.INFINITE_LEASE;
import static android.net.dhcp.DhcpPacket.MIN_V6ONLY_WAIT_MS;
import static android.net.dhcp.DhcpResultsParcelableUtil.fromStableParcelable;
import static android.net.ipmemorystore.Status.SUCCESS;
import static android.system.OsConstants.ETH_P_IPV6;
import static android.system.OsConstants.IFA_F_TEMPORARY;
import static android.system.OsConstants.IPPROTO_ICMPV6;

import static com.android.net.module.util.Inet4AddressUtils.getBroadcastAddress;
import static com.android.net.module.util.Inet4AddressUtils.getPrefixMaskAsInet4Address;
import static com.android.net.module.util.NetworkStackConstants.ARP_REPLY;
import static com.android.net.module.util.NetworkStackConstants.ARP_REQUEST;
import static com.android.net.module.util.NetworkStackConstants.ETHER_ADDR_LEN;
import static com.android.net.module.util.NetworkStackConstants.ETHER_BROADCAST;
import static com.android.net.module.util.NetworkStackConstants.ETHER_HEADER_LEN;
import static com.android.net.module.util.NetworkStackConstants.ETHER_TYPE_OFFSET;
import static com.android.net.module.util.NetworkStackConstants.ICMPV6_NEIGHBOR_ADVERTISEMENT;
import static com.android.net.module.util.NetworkStackConstants.ICMPV6_ROUTER_SOLICITATION;
import static com.android.net.module.util.NetworkStackConstants.IPV4_ADDR_ANY;
import static com.android.net.module.util.NetworkStackConstants.IPV6_ADDR_ALL_NODES_MULTICAST;
import static com.android.net.module.util.NetworkStackConstants.IPV6_ADDR_ALL_ROUTERS_MULTICAST;
import static com.android.net.module.util.NetworkStackConstants.IPV6_HEADER_LEN;
import static com.android.net.module.util.NetworkStackConstants.IPV6_PROTOCOL_OFFSET;
import static com.android.net.module.util.NetworkStackConstants.PIO_FLAG_AUTONOMOUS;
import static com.android.net.module.util.NetworkStackConstants.PIO_FLAG_ON_LINK;

import static junit.framework.Assert.fail;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.contains;
import static org.mockito.ArgumentMatchers.longThat;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.argThat;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.AlarmManager;
import android.app.AlarmManager.OnAlarmListener;
import android.app.Instrumentation;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.DhcpResults;
import android.net.DhcpResultsParcelable;
import android.net.INetd;
import android.net.InetAddresses;
import android.net.InterfaceConfigurationParcel;
import android.net.IpPrefix;
import android.net.Layer2InformationParcelable;
import android.net.Layer2PacketParcelable;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.MacAddress;
import android.net.NetworkStackIpMemoryStore;
import android.net.TestNetworkInterface;
import android.net.TestNetworkManager;
import android.net.Uri;
import android.net.dhcp.DhcpClient;
import android.net.dhcp.DhcpDeclinePacket;
import android.net.dhcp.DhcpDiscoverPacket;
import android.net.dhcp.DhcpPacket;
import android.net.dhcp.DhcpPacket.ParseException;
import android.net.dhcp.DhcpRequestPacket;
import android.net.ipmemorystore.NetworkAttributes;
import android.net.ipmemorystore.OnNetworkAttributesRetrievedListener;
import android.net.ipmemorystore.Status;
import android.net.netlink.StructNdOptPref64;
import android.net.networkstack.TestNetworkStackServiceClient;
import android.net.networkstack.aidl.dhcp.DhcpOption;
import android.net.shared.Layer2Information;
import android.net.shared.ProvisioningConfiguration;
import android.net.shared.ProvisioningConfiguration.ScanResultInfo;
import android.net.util.InterfaceParams;
import android.net.util.NetworkStackUtils;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.stats.connectivity.NetworkQuirkEvent;
import android.system.ErrnoException;
import android.system.Os;

import androidx.annotation.NonNull;
import androidx.test.InstrumentationRegistry;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.internal.util.HexDump;
import com.android.internal.util.StateMachine;
import com.android.net.module.util.ArrayTrackRecord;
import com.android.net.module.util.Ipv6Utils;
import com.android.net.module.util.structs.PrefixInformationOption;
import com.android.net.module.util.structs.RdnssOption;
import com.android.networkstack.apishim.CaptivePortalDataShimImpl;
import com.android.networkstack.apishim.ConstantsShim;
import com.android.networkstack.apishim.common.ShimUtils;
import com.android.networkstack.arp.ArpPacket;
import com.android.networkstack.metrics.IpProvisioningMetrics;
import com.android.networkstack.metrics.NetworkQuirkMetrics;
import com.android.networkstack.packets.NeighborAdvertisement;
import com.android.server.NetworkObserver;
import com.android.server.NetworkObserverRegistry;
import com.android.server.NetworkStackService.NetworkStackServiceManager;
import com.android.server.connectivity.ipmemorystore.IpMemoryStoreService;
import com.android.testutils.DevSdkIgnoreRule;
import com.android.testutils.DevSdkIgnoreRule.IgnoreAfter;
import com.android.testutils.DevSdkIgnoreRule.IgnoreUpTo;
import com.android.testutils.HandlerUtils;
import com.android.testutils.TapPacketReader;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileReader;
import java.io.IOException;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Method;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;
import java.util.Random;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Predicate;

import kotlin.Lazy;
import kotlin.LazyKt;

/**
 * Base class for IpClient tests.
 *
 * Tests in this class can either be run with signature permissions, or with root access.
 */
@RunWith(AndroidJUnit4.class)
@SmallTest
public abstract class IpClientIntegrationTestCommon {
    private static final int DATA_BUFFER_LEN = 4096;
    private static final int PACKET_TIMEOUT_MS = 5_000;
    private static final String TEST_CLUSTER = "some cluster";
    private static final int TEST_LEASE_DURATION_S = 3_600; // 1 hour
    private static final int TEST_IPV6_ONLY_WAIT_S = 1_800; // 30 min
    private static final int TEST_LOWER_IPV6_ONLY_WAIT_S = (int) (MIN_V6ONLY_WAIT_MS / 1000 - 1);
    private static final int TEST_ZERO_IPV6_ONLY_WAIT_S = 0;
    private static final long TEST_MAX_IPV6_ONLY_WAIT_S = 0xffffffffL;
    protected static final String TEST_L2KEY = "some l2key";

    // TODO: move to NetlinkConstants, NetworkStackConstants, or OsConstants.
    private static final int IFA_F_STABLE_PRIVACY = 0x800;

    protected static final long TEST_TIMEOUT_MS = 2_000L;

    @Rule
    public final DevSdkIgnoreRule mIgnoreRule = new DevSdkIgnoreRule();
    @Rule
    public final TestName mTestNameRule = new TestName();

    /**
     * Indicates that a test requires signature permissions to run.
     *
     * Such tests can only be run on devices that use known signing keys, so this annotation must be
     * avoided as much as possible. Consider whether the test can be written to use shell and root
     * shell permissions, and run against the NetworkStack AIDL interface (IIpClient) instead.
     */
    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.METHOD})
    private @interface SignatureRequiredTest {
        String reason();
    }

    /**** BEGIN signature required test members ****/
    // Do not use unless the test *really* cannot be written to exercise IIpClient without mocks.
    // Tests using the below members must be annotated with @SignatureRequiredTest (otherwise the
    // members will be null), and can only be run on devices that use known signing keys.
    // The members could technically be moved to the IpClientIntegrationTest subclass together with
    // the tests requiring signature permissions, but this would make it harder to follow tests in
    // multiple classes, and harder to migrate tests between signature required and not required.

    @Mock private Context mContext;
    @Mock private ConnectivityManager mCm;
    @Mock private Resources mResources;
    @Mock private AlarmManager mAlarm;
    @Mock private ContentResolver mContentResolver;
    @Mock private NetworkStackServiceManager mNetworkStackServiceManager;
    @Mock private IpMemoryStoreService mIpMemoryStoreService;
    @Mock private PowerManager.WakeLock mTimeoutWakeLock;
    @Mock protected NetworkStackIpMemoryStore mIpMemoryStore;
    @Mock private NetworkQuirkMetrics.Dependencies mNetworkQuirkMetricsDeps;

    @Spy private INetd mNetd;
    private NetworkObserverRegistry mNetworkObserverRegistry;

    protected IpClient mIpc;
    protected Dependencies mDependencies;

    /***** END signature required test members *****/

    private IIpClientCallbacks mCb;
    private IIpClient mIIpClient;
    private String mIfaceName;
    private HandlerThread mPacketReaderThread;
    private Handler mHandler;
    private TapPacketReader mPacketReader;
    private FileDescriptor mTapFd;
    private byte[] mClientMac;

    private boolean mIsSignatureRequiredTest;

    // ReadHeads for various packet streams. Cannot be initialized in @Before because ReadHead is
    // single-thread-only, and AndroidJUnitRunner runs @Before and @Test on different threads.
    // While it looks like these are created only once per test, they are actually created once per
    // test method because JUnit recreates a fresh test class instance before every test method.
    private Lazy<ArrayTrackRecord<byte[]>.ReadHead> mDhcpPacketReadHead =
            LazyKt.lazy(() -> mPacketReader.getReceivedPackets().newReadHead());
    private Lazy<ArrayTrackRecord<byte[]>.ReadHead> mArpPacketReadHead =
            LazyKt.lazy(() -> mPacketReader.getReceivedPackets().newReadHead());

    // Ethernet header
    private static final int ETH_HEADER_LEN = 14;

    // IP header
    private static final int IPV4_HEADER_LEN = 20;
    private static final int IPV4_SRC_ADDR_OFFSET = ETH_HEADER_LEN + 12;
    private static final int IPV4_DST_ADDR_OFFSET = IPV4_SRC_ADDR_OFFSET + 4;

    // UDP header
    private static final int UDP_HEADER_LEN = 8;
    private static final int UDP_HEADER_OFFSET = ETH_HEADER_LEN + IPV4_HEADER_LEN;
    private static final int UDP_SRC_PORT_OFFSET = UDP_HEADER_OFFSET + 0;

    // DHCP header
    private static final int DHCP_HEADER_OFFSET = ETH_HEADER_LEN + IPV4_HEADER_LEN
            + UDP_HEADER_LEN;
    private static final int DHCP_MESSAGE_OP_CODE_OFFSET = DHCP_HEADER_OFFSET + 0;
    private static final int DHCP_TRANSACTION_ID_OFFSET = DHCP_HEADER_OFFSET + 4;
    private static final int DHCP_OPTION_MAGIC_COOKIE_OFFSET = DHCP_HEADER_OFFSET + 236;

    private static final Inet4Address SERVER_ADDR =
            (Inet4Address) InetAddresses.parseNumericAddress("192.168.1.100");
    private static final Inet4Address CLIENT_ADDR =
            (Inet4Address) InetAddresses.parseNumericAddress("192.168.1.2");
    private static final Inet4Address CLIENT_ADDR_NEW =
            (Inet4Address) InetAddresses.parseNumericAddress("192.168.1.3");
    private static final Inet4Address INADDR_ANY =
            (Inet4Address) InetAddresses.parseNumericAddress("0.0.0.0");
    private static final int PREFIX_LENGTH = 24;
    private static final Inet4Address NETMASK = getPrefixMaskAsInet4Address(PREFIX_LENGTH);
    private static final Inet4Address BROADCAST_ADDR = getBroadcastAddress(
            SERVER_ADDR, PREFIX_LENGTH);
    private static final String HOSTNAME = "testhostname";
    private static final int TEST_DEFAULT_MTU = 1500;
    private static final int TEST_MIN_MTU = 1280;
    private static final byte[] SERVER_MAC = new byte[] { 0x00, 0x1A, 0x11, 0x22, 0x33, 0x44 };
    private static final String TEST_HOST_NAME = "AOSP on Crosshatch";
    private static final String TEST_HOST_NAME_TRANSLITERATION = "AOSP-on-Crosshatch";
    private static final String TEST_CAPTIVE_PORTAL_URL = "https://example.com/capportapi";
    private static final byte[] TEST_HOTSPOT_OUI = new byte[] {
            (byte) 0x00, (byte) 0x17, (byte) 0xF2
    };
    private static final byte TEST_VENDOR_SPECIFIC_TYPE = 0x06;

    private static final String TEST_DEFAULT_SSID = "test_ssid";
    private static final String TEST_DEFAULT_BSSID = "00:11:22:33:44:55";
    private static final String TEST_DHCP_ROAM_SSID = "0001docomo";
    private static final String TEST_DHCP_ROAM_BSSID = "00:4e:35:17:98:55";
    private static final String TEST_DHCP_ROAM_L2KEY = "roaming_l2key";
    private static final String TEST_DHCP_ROAM_CLUSTER = "roaming_cluster";
    private static final byte[] TEST_AP_OUI = new byte[] { 0x00, 0x1A, 0x11 };
    private static final byte[] TEST_OEM_OUI = new byte[] {(byte) 0x00, (byte) 0x17, (byte) 0xc3};
    private static final String TEST_OEM_VENDOR_ID = "vendor-class-identifier";
    private static final byte[] TEST_OEM_USER_CLASS_INFO = new byte[] {
            // Instance of User Class: [0]
            (byte) 0x03, /* UC_Len_0 */ (byte) 0x11, (byte) 0x22, (byte) 0x33,
            // Instance of User Class: [1]
            (byte) 0x03, /* UC_Len_1 */ (byte) 0x44, (byte) 0x55, (byte) 0x66,
    };

    protected class Dependencies extends IpClient.Dependencies {
        // Can't use SparseIntArray, it doesn't have an easy way to know if a key is not present.
        private HashMap<String, Integer> mIntConfigProperties = new HashMap<>();
        private DhcpClient mDhcpClient;
        private boolean mIsHostnameConfigurationEnabled;
        private String mHostname;
        private boolean mIsInterfaceRecovered;

        public void setHostnameConfiguration(final boolean enable, final String hostname) {
            mIsHostnameConfigurationEnabled = enable;
            mHostname = hostname;
        }

        // Enable this flag to simulate the interface has been added back after removing
        // on the provisioning start. However, the actual tap interface has been removed,
        // interface parameters query will get null when attempting to restore Interface
        // MTU. Create a new InterfaceParams instance and return instead just for interface
        // toggling test case.
        public void simulateInterfaceRecover() {
            mIsInterfaceRecovered = true;
        }

        @Override
        public InterfaceParams getInterfaceParams(String ifname) {
            return mIsInterfaceRecovered
                    ? new InterfaceParams(ifname, 1 /* index */,
                            MacAddress.fromString("00:11:22:33:44:55"))
                    : super.getInterfaceParams(ifname);
        }

        @Override
        public INetd getNetd(Context context) {
            return mNetd;
        }

        @Override
        public NetworkStackIpMemoryStore getIpMemoryStore(Context context,
                NetworkStackServiceManager nssManager) {
            return mIpMemoryStore;
        }

        @Override
        public DhcpClient makeDhcpClient(Context context, StateMachine controller,
                InterfaceParams ifParams, DhcpClient.Dependencies deps) {
            mDhcpClient = DhcpClient.makeDhcpClient(context, controller, ifParams, deps);
            return mDhcpClient;
        }

        @Override
        public boolean isFeatureEnabled(final Context context, final String name,
                final boolean defaultEnabled) {
            return IpClientIntegrationTestCommon.this.isFeatureEnabled(name, defaultEnabled);
        }

        @Override
        public DhcpClient.Dependencies getDhcpClientDependencies(
                NetworkStackIpMemoryStore ipMemoryStore, IpProvisioningMetrics metrics) {
            return new DhcpClient.Dependencies(ipMemoryStore, metrics) {
                @Override
                public boolean isFeatureEnabled(final Context context, final String name,
                        final boolean defaultEnabled) {
                    return Dependencies.this.isFeatureEnabled(context, name, defaultEnabled);
                }

                @Override
                public int getIntDeviceConfig(final String name, int minimumValue,
                        int maximumValue, int defaultValue) {
                    return getDeviceConfigPropertyInt(name, 0 /* default value */);
                }

                @Override
                public PowerManager.WakeLock getWakeLock(final PowerManager powerManager) {
                    return mTimeoutWakeLock;
                }

                @Override
                public boolean getSendHostnameOption(final Context context) {
                    return mIsHostnameConfigurationEnabled;
                }

                @Override
                public String getDeviceName(final Context context) {
                    return mIsHostnameConfigurationEnabled ? mHostname : null;
                }
            };
        }

        @Override
        public int getDeviceConfigPropertyInt(String name, int defaultValue) {
            Integer value = mIntConfigProperties.get(name);
            if (value == null) {
                throw new IllegalStateException("Non-mocked device config property " + name);
            }
            return value;
        }

        public void setDeviceConfigProperty(String name, int value) {
            mIntConfigProperties.put(name, value);
        }

        @Override
        public NetworkQuirkMetrics getNetworkQuirkMetrics() {
            return new NetworkQuirkMetrics(mNetworkQuirkMetricsDeps);
        }
    }

    @NonNull
    protected abstract IIpClient makeIIpClient(
            @NonNull String ifaceName, @NonNull IIpClientCallbacks cb);

    protected abstract void setFeatureEnabled(String name, boolean enabled);

    protected abstract boolean isFeatureEnabled(String name, boolean defaultEnabled);

    protected abstract boolean useNetworkStackSignature();

    protected abstract NetworkAttributes getStoredNetworkAttributes(String l2Key, long timeout);

    protected abstract void assertIpMemoryNeverStoreNetworkAttributes(String l2Key, long timeout);

    protected final boolean testSkipped() {
        // TODO: split out a test suite for root tests, and fail hard instead of skipping the test
        // if it is run on devices where TestNetworkStackServiceClient is not supported
        return !useNetworkStackSignature()
                && (mIsSignatureRequiredTest || !TestNetworkStackServiceClient.isSupported());
    }

    protected void setDhcpFeatures(final boolean isDhcpLeaseCacheEnabled,
            final boolean isRapidCommitEnabled, final boolean isDhcpIpConflictDetectEnabled,
            final boolean isIPv6OnlyPreferredEnabled) {
        setFeatureEnabled(NetworkStackUtils.DHCP_INIT_REBOOT_VERSION, isDhcpLeaseCacheEnabled);
        setFeatureEnabled(NetworkStackUtils.DHCP_RAPID_COMMIT_VERSION, isRapidCommitEnabled);
        setFeatureEnabled(NetworkStackUtils.DHCP_IP_CONFLICT_DETECT_VERSION,
                isDhcpIpConflictDetectEnabled);
        setFeatureEnabled(NetworkStackUtils.DHCP_IPV6_ONLY_PREFERRED_VERSION,
                isIPv6OnlyPreferredEnabled);
    }

    @Before
    public void setUp() throws Exception {
        final Method testMethod = IpClientIntegrationTestCommon.class.getMethod(
                mTestNameRule.getMethodName());
        mIsSignatureRequiredTest = testMethod.getAnnotation(SignatureRequiredTest.class) != null;
        assumeFalse(testSkipped());

        setUpTapInterface();
        mCb = mock(IIpClientCallbacks.class);

        if (useNetworkStackSignature()) {
            setUpMocks();
            setUpIpClient();
        }

        mIIpClient = makeIIpClient(mIfaceName, mCb);
    }

    protected void setUpMocks() throws Exception {
        MockitoAnnotations.initMocks(this);

        mDependencies = new Dependencies();
        when(mContext.getSystemService(eq(Context.ALARM_SERVICE))).thenReturn(mAlarm);
        when(mContext.getSystemService(eq(ConnectivityManager.class))).thenReturn(mCm);
        when(mContext.getResources()).thenReturn(mResources);
        when(mContext.getContentResolver()).thenReturn(mContentResolver);
        when(mNetworkStackServiceManager.getIpMemoryStoreService())
                .thenReturn(mIpMemoryStoreService);

        mDependencies.setDeviceConfigProperty(IpClient.CONFIG_MIN_RDNSS_LIFETIME, 67);
        mDependencies.setDeviceConfigProperty(DhcpClient.DHCP_RESTART_CONFIG_DELAY, 10);
        mDependencies.setDeviceConfigProperty(DhcpClient.ARP_FIRST_PROBE_DELAY_MS, 10);
        mDependencies.setDeviceConfigProperty(DhcpClient.ARP_PROBE_MIN_MS, 10);
        mDependencies.setDeviceConfigProperty(DhcpClient.ARP_PROBE_MAX_MS, 20);
        mDependencies.setDeviceConfigProperty(DhcpClient.ARP_FIRST_ANNOUNCE_DELAY_MS, 10);
        mDependencies.setDeviceConfigProperty(DhcpClient.ARP_ANNOUNCE_INTERVAL_MS, 10);
    }

    private void awaitIpClientShutdown() throws Exception {
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onQuit();
    }

    @After
    public void tearDown() throws Exception {
        if (testSkipped()) return;
        if (mPacketReader != null) {
            mHandler.post(() -> mPacketReader.stop()); // Also closes the socket
            mTapFd = null;
        }
        if (mPacketReaderThread != null) {
            mPacketReaderThread.quitSafely();
        }
        mIIpClient.shutdown();
        awaitIpClientShutdown();
    }

    private void setUpTapInterface() throws Exception {
        final Instrumentation inst = InstrumentationRegistry.getInstrumentation();
        // Adopt the shell permission identity to create a test TAP interface.
        inst.getUiAutomation().adoptShellPermissionIdentity();

        final TestNetworkInterface iface;
        try {
            final TestNetworkManager tnm = (TestNetworkManager)
                    inst.getContext().getSystemService(Context.TEST_NETWORK_SERVICE);
            iface = tnm.createTapInterface();
        } finally {
            // Drop the identity in order to regain the network stack permissions, which the shell
            // does not have.
            inst.getUiAutomation().dropShellPermissionIdentity();
        }
        mIfaceName = iface.getInterfaceName();
        mClientMac = getIfaceMacAddr(mIfaceName).toByteArray();
        mPacketReaderThread = new HandlerThread(
                IpClientIntegrationTestCommon.class.getSimpleName());
        mPacketReaderThread.start();
        mHandler = mPacketReaderThread.getThreadHandler();

        // Detach the FileDescriptor from the ParcelFileDescriptor.
        // Otherwise, the garbage collector might call the ParcelFileDescriptor's finalizer, which
        // closes the FileDescriptor and destroys our tap interface. An alternative would be to
        // make the ParcelFileDescriptor or the TestNetworkInterface a class member so they never
        // go out of scope.
        mTapFd = new FileDescriptor();
        mTapFd.setInt$(iface.getFileDescriptor().detachFd());
        mPacketReader = new TapPacketReader(mHandler, mTapFd, DATA_BUFFER_LEN);
        mHandler.post(() -> mPacketReader.start());
    }

    private MacAddress getIfaceMacAddr(String ifaceName) throws IOException {
        // InterfaceParams.getByName requires CAP_NET_ADMIN: read the mac address with the shell
        final String strMacAddr = getOneLineCommandOutput(
                "su root cat /sys/class/net/" + ifaceName + "/address");
        return MacAddress.fromString(strMacAddr);
    }

    private String getOneLineCommandOutput(String cmd) throws IOException {
        try (ParcelFileDescriptor fd = InstrumentationRegistry.getInstrumentation()
                .getUiAutomation().executeShellCommand(cmd);
             BufferedReader reader = new BufferedReader(new FileReader(fd.getFileDescriptor()))) {
            return reader.readLine();
        }
    }

    private IpClient makeIpClient() throws Exception {
        IpClient ipc = new IpClient(mContext, mIfaceName, mCb, mNetworkObserverRegistry,
                mNetworkStackServiceManager, mDependencies);
        // Wait for IpClient to enter its initial state. Otherwise, additional setup steps or tests
        // that mock IpClient's dependencies might interact with those mocks while IpClient is
        // starting. This would cause UnfinishedStubbingExceptions as mocks cannot be interacted
        // with while they are being stubbed.
        HandlerUtils.waitForIdle(ipc.getHandler(), TEST_TIMEOUT_MS);
        return ipc;
    }

    private void setUpIpClient() throws Exception {
        final Instrumentation inst = InstrumentationRegistry.getInstrumentation();
        final IBinder netdIBinder =
                (IBinder) inst.getContext().getSystemService(Context.NETD_SERVICE);
        mNetd = spy(INetd.Stub.asInterface(netdIBinder));
        when(mContext.getSystemService(eq(Context.NETD_SERVICE))).thenReturn(netdIBinder);
        assertNotNull(mNetd);

        mNetworkObserverRegistry = new NetworkObserverRegistry();
        mNetworkObserverRegistry.register(mNetd);
        mIpc = makeIpClient();

        // Tell the IpMemoryStore immediately to answer any question about network attributes with a
        // null response. Otherwise, the DHCP client will wait for two seconds before starting,
        // while its query to the IpMemoryStore times out.
        // This does not affect any test that makes the mock memory store return results, because
        // unlike when(), it is documented that doAnswer() can be called more than once, to change
        // the behaviour of a mock in the middle of a test.
        doAnswer(invocation -> {
            final String l2Key = invocation.getArgument(0);
            ((OnNetworkAttributesRetrievedListener) invocation.getArgument(1))
                    .onNetworkAttributesRetrieved(new Status(SUCCESS), l2Key, null);
            return null;
        }).when(mIpMemoryStore).retrieveNetworkAttributes(any(), any());

        disableIpv6ProvisioningDelays();
    }

    private <T> T verifyWithTimeout(InOrder inOrder, T t) {
        if (inOrder != null) {
            return inOrder.verify(t, timeout(TEST_TIMEOUT_MS));
        } else {
            return verify(t, timeout(TEST_TIMEOUT_MS));
        }
    }

    private void expectAlarmCancelled(InOrder inOrder, OnAlarmListener listener) {
        inOrder.verify(mAlarm, timeout(TEST_TIMEOUT_MS)).cancel(eq(listener));
    }

    private OnAlarmListener expectAlarmSet(InOrder inOrder, String tagMatch, long afterSeconds,
            Handler handler) {
        // Allow +/- 3 seconds to prevent flaky tests.
        final long when = SystemClock.elapsedRealtime() + afterSeconds * 1000;
        final long min = when - 3 * 1000;
        final long max = when + 3 * 1000;
        ArgumentCaptor<OnAlarmListener> captor = ArgumentCaptor.forClass(OnAlarmListener.class);
        verifyWithTimeout(inOrder, mAlarm).setExact(
                anyInt(), longThat(x -> x >= min && x <= max),
                contains(tagMatch), captor.capture(), eq(handler));
        return captor.getValue();
    }

    private OnAlarmListener expectAlarmSet(InOrder inOrder, String tagMatch, int afterSeconds) {
        return expectAlarmSet(inOrder, tagMatch, (long) afterSeconds, mIpc.getHandler());
    }

    private boolean packetContainsExpectedField(final byte[] packet, final int offset,
            final byte[] expected) {
        if (packet.length < offset + expected.length) return false;
        for (int i = 0; i < expected.length; ++i) {
            if (packet[offset + i] != expected[i]) return false;
        }
        return true;
    }

    private boolean isDhcpPacket(final byte[] packet) {
        final ByteBuffer buffer = ByteBuffer.wrap(packet);

        // check the packet length
        if (packet.length < DHCP_HEADER_OFFSET) return false;

        // check the source port and dest port in UDP header
        buffer.position(UDP_SRC_PORT_OFFSET);
        final short udpSrcPort = buffer.getShort();
        final short udpDstPort = buffer.getShort();
        if (udpSrcPort != DHCP_CLIENT || udpDstPort != DHCP_SERVER) return false;

        // check DHCP message type
        buffer.position(DHCP_MESSAGE_OP_CODE_OFFSET);
        final byte dhcpOpCode = buffer.get();
        if (dhcpOpCode != DHCP_BOOTREQUEST) return false;

        // check DHCP magic cookie
        buffer.position(DHCP_OPTION_MAGIC_COOKIE_OFFSET);
        final int dhcpMagicCookie = buffer.getInt();
        if (dhcpMagicCookie != DHCP_MAGIC_COOKIE) return false;

        return true;
    }

    private ArpPacket parseArpPacketOrNull(final byte[] packet) {
        try {
            return ArpPacket.parseArpPacket(packet, packet.length);
        } catch (ArpPacket.ParseException e) {
            return null;
        }
    }

    private NeighborAdvertisement parseNeighborAdvertisementOrNull(final byte[] packet) {
        try {
            return NeighborAdvertisement.parse(packet, packet.length);
        } catch (NeighborAdvertisement.ParseException e) {
            return null;
        }
    }

    private static ByteBuffer buildDhcpOfferPacket(final DhcpPacket packet,
            final Inet4Address clientAddress, final Integer leaseTimeSec, final short mtu,
            final String captivePortalUrl, final Integer ipv6OnlyWaitTime) {
        return DhcpPacket.buildOfferPacket(DhcpPacket.ENCAP_L2, packet.getTransactionId(),
                false /* broadcast */, SERVER_ADDR, INADDR_ANY /* relayIp */,
                clientAddress /* yourIp */, packet.getClientMac(), leaseTimeSec,
                NETMASK /* netMask */, BROADCAST_ADDR /* bcAddr */,
                Collections.singletonList(SERVER_ADDR) /* gateways */,
                Collections.singletonList(SERVER_ADDR) /* dnsServers */,
                SERVER_ADDR /* dhcpServerIdentifier */, null /* domainName */, HOSTNAME,
                false /* metered */, mtu, captivePortalUrl, ipv6OnlyWaitTime);
    }

    private static ByteBuffer buildDhcpOfferPacket(final DhcpPacket packet,
            final Inet4Address clientAddress, final Integer leaseTimeSec, final short mtu,
            final String captivePortalUrl) {
        return buildDhcpOfferPacket(packet, clientAddress, leaseTimeSec, mtu, captivePortalUrl,
                null /* ipv6OnlyWaitTime */);
    }

    private static ByteBuffer buildDhcpAckPacket(final DhcpPacket packet,
            final Inet4Address clientAddress, final Integer leaseTimeSec, final short mtu,
            final boolean rapidCommit, final String captivePortalApiUrl,
            final Integer ipv6OnlyWaitTime) {
        return DhcpPacket.buildAckPacket(DhcpPacket.ENCAP_L2, packet.getTransactionId(),
                false /* broadcast */, SERVER_ADDR, INADDR_ANY /* relayIp */,
                clientAddress /* yourIp */, CLIENT_ADDR /* requestIp */, packet.getClientMac(),
                leaseTimeSec, NETMASK /* netMask */, BROADCAST_ADDR /* bcAddr */,
                Collections.singletonList(SERVER_ADDR) /* gateways */,
                Collections.singletonList(SERVER_ADDR) /* dnsServers */,
                SERVER_ADDR /* dhcpServerIdentifier */, null /* domainName */, HOSTNAME,
                false /* metered */, mtu, rapidCommit, captivePortalApiUrl, ipv6OnlyWaitTime);
    }

    private static ByteBuffer buildDhcpAckPacket(final DhcpPacket packet,
            final Inet4Address clientAddress, final Integer leaseTimeSec, final short mtu,
            final boolean rapidCommit, final String captivePortalApiUrl) {
        return buildDhcpAckPacket(packet, clientAddress, leaseTimeSec, mtu, rapidCommit,
                captivePortalApiUrl, null /* ipv6OnlyWaitTime */);
    }

    private static ByteBuffer buildDhcpNakPacket(final DhcpPacket packet) {
        return DhcpPacket.buildNakPacket(DhcpPacket.ENCAP_L2, packet.getTransactionId(),
            SERVER_ADDR /* serverIp */, INADDR_ANY /* relayIp */, packet.getClientMac(),
            false /* broadcast */, "duplicated request IP address");
    }

    private void sendArpReply(final byte[] clientMac) throws IOException {
        final ByteBuffer packet = ArpPacket.buildArpPacket(clientMac /* dst */,
                SERVER_MAC /* src */, INADDR_ANY.getAddress() /* target IP */,
                clientMac /* target HW address */, CLIENT_ADDR.getAddress() /* sender IP */,
                (short) ARP_REPLY);
        mPacketReader.sendResponse(packet);
    }

    private void sendArpProbe() throws IOException {
        final ByteBuffer packet = ArpPacket.buildArpPacket(DhcpPacket.ETHER_BROADCAST /* dst */,
                SERVER_MAC /* src */, CLIENT_ADDR.getAddress() /* target IP */,
                new byte[ETHER_ADDR_LEN] /* target HW address */,
                INADDR_ANY.getAddress() /* sender IP */, (short) ARP_REQUEST);
        mPacketReader.sendResponse(packet);
    }

    private void startIpClientProvisioning(final ProvisioningConfiguration cfg) throws Exception {
        mIIpClient.startProvisioning(cfg.toStableParcelable());
    }

    private void startIpClientProvisioning(final boolean isDhcpLeaseCacheEnabled,
            final boolean shouldReplyRapidCommitAck, final boolean isPreconnectionEnabled,
            final boolean isDhcpIpConflictDetectEnabled, final boolean isIPv6OnlyPreferredEnabled,
            final String displayName, final ScanResultInfo scanResultInfo,
            final Layer2Information layer2Info) throws Exception {
        ProvisioningConfiguration.Builder prov = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withLayer2Information(layer2Info == null
                        ? new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                              MacAddress.fromString(TEST_DEFAULT_BSSID))
                        : layer2Info)
                .withoutIPv6();
        if (isPreconnectionEnabled) prov.withPreconnection();
        if (displayName != null) prov.withDisplayName(displayName);
        if (scanResultInfo != null) prov.withScanResultInfo(scanResultInfo);

        setDhcpFeatures(isDhcpLeaseCacheEnabled, shouldReplyRapidCommitAck,
                isDhcpIpConflictDetectEnabled, isIPv6OnlyPreferredEnabled);

        startIpClientProvisioning(prov.build());
        if (!isPreconnectionEnabled) {
            verify(mCb, timeout(TEST_TIMEOUT_MS)).setFallbackMulticastFilter(false);
        }
        verify(mCb, never()).onProvisioningFailure(any());
    }

    private void startIpClientProvisioning(final boolean isDhcpLeaseCacheEnabled,
            final boolean isDhcpRapidCommitEnabled, final boolean isPreconnectionEnabled,
            final boolean isDhcpIpConflictDetectEnabled, final boolean isIPv6OnlyPreferredEnabled)
            throws Exception {
        startIpClientProvisioning(isDhcpLeaseCacheEnabled, isDhcpRapidCommitEnabled,
                isPreconnectionEnabled, isDhcpIpConflictDetectEnabled, isIPv6OnlyPreferredEnabled,
                null /* displayName */, null /* ScanResultInfo */, null /* layer2Info */);
    }

    private void assertIpMemoryStoreNetworkAttributes(final Integer leaseTimeSec,
            final long startTime, final int mtu) {
        final NetworkAttributes na = getStoredNetworkAttributes(TEST_L2KEY, TEST_TIMEOUT_MS);
        assertNotNull(na);
        assertEquals(CLIENT_ADDR, na.assignedV4Address);
        if (leaseTimeSec == null || leaseTimeSec.intValue() == DhcpPacket.INFINITE_LEASE) {
            assertEquals(Long.MAX_VALUE, na.assignedV4AddressExpiry.longValue());
        } else {
            // check the lease expiry's scope
            final long upperBound = startTime + 7_200_000; // start timestamp + 2h
            final long lowerBound = startTime + 3_600_000; // start timestamp + 1h
            final long expiry = na.assignedV4AddressExpiry;
            assertTrue(upperBound > expiry);
            assertTrue(lowerBound < expiry);
        }
        assertEquals(Collections.singletonList(SERVER_ADDR), na.dnsAddresses);
        assertEquals(new Integer(mtu), na.mtu);
    }

    private void assertIpMemoryNeverStoreNetworkAttributes() {
        assertIpMemoryNeverStoreNetworkAttributes(TEST_L2KEY, TEST_TIMEOUT_MS);
    }

    private void assertHostname(final boolean isHostnameConfigurationEnabled,
            final String hostname, final String hostnameAfterTransliteration,
            final List<DhcpPacket> packetList) throws Exception {
        for (DhcpPacket packet : packetList) {
            if (!isHostnameConfigurationEnabled || hostname == null) {
                assertNoHostname(packet.getHostname());
            } else {
                assertEquals(packet.getHostname(), hostnameAfterTransliteration);
            }
        }
    }

    private void assertNoHostname(String hostname) {
        if (ShimUtils.isAtLeastR()) {
            assertNull(hostname);
        } else {
            // Until Q, if no hostname is set, the device falls back to the hostname set via
            // system property, to avoid breaking Q devices already launched with that setup.
            assertEquals(SystemProperties.get("net.hostname"), hostname);
        }
    }

    // Helper method to complete DHCP 2-way or 4-way handshake
    private List<DhcpPacket> performDhcpHandshake(final boolean isSuccessLease,
            final Integer leaseTimeSec, final boolean isDhcpLeaseCacheEnabled,
            final boolean shouldReplyRapidCommitAck, final int mtu,
            final boolean isDhcpIpConflictDetectEnabled,
            final boolean isIPv6OnlyPreferredEnabled,
            final String captivePortalApiUrl, final String displayName,
            final ScanResultInfo scanResultInfo, final Layer2Information layer2Info)
            throws Exception {
        startIpClientProvisioning(isDhcpLeaseCacheEnabled, shouldReplyRapidCommitAck,
                false /* isPreconnectionEnabled */, isDhcpIpConflictDetectEnabled,
                isIPv6OnlyPreferredEnabled, displayName, scanResultInfo, layer2Info);
        return handleDhcpPackets(isSuccessLease, leaseTimeSec, shouldReplyRapidCommitAck, mtu,
                captivePortalApiUrl);
    }

    private List<DhcpPacket> handleDhcpPackets(final boolean isSuccessLease,
            final Integer leaseTimeSec, final boolean shouldReplyRapidCommitAck, final int mtu,
            final String captivePortalApiUrl) throws Exception {
        final List<DhcpPacket> packetList = new ArrayList<>();
        DhcpPacket packet;
        while ((packet = getNextDhcpPacket()) != null) {
            packetList.add(packet);
            if (packet instanceof DhcpDiscoverPacket) {
                if (shouldReplyRapidCommitAck) {
                    mPacketReader.sendResponse(buildDhcpAckPacket(packet, CLIENT_ADDR, leaseTimeSec,
                              (short) mtu, true /* rapidCommit */, captivePortalApiUrl));
                } else {
                    mPacketReader.sendResponse(buildDhcpOfferPacket(packet, CLIENT_ADDR,
                            leaseTimeSec, (short) mtu, captivePortalApiUrl));
                }
            } else if (packet instanceof DhcpRequestPacket) {
                final ByteBuffer byteBuffer = isSuccessLease
                        ? buildDhcpAckPacket(packet, CLIENT_ADDR, leaseTimeSec, (short) mtu,
                                false /* rapidCommit */, captivePortalApiUrl)
                        : buildDhcpNakPacket(packet);
                mPacketReader.sendResponse(byteBuffer);
            } else {
                fail("invalid DHCP packet");
            }

            // wait for reply to DHCPOFFER packet if disabling rapid commit option
            if (shouldReplyRapidCommitAck || !(packet instanceof DhcpDiscoverPacket)) {
                return packetList;
            }
        }
        fail("No DHCPREQUEST received on interface");
        return packetList;
    }

    private List<DhcpPacket> performDhcpHandshake(final boolean isSuccessLease,
            final Integer leaseTimeSec, final boolean isDhcpLeaseCacheEnabled,
            final boolean isDhcpRapidCommitEnabled, final int mtu,
            final boolean isDhcpIpConflictDetectEnabled) throws Exception {
        return performDhcpHandshake(isSuccessLease, leaseTimeSec, isDhcpLeaseCacheEnabled,
                isDhcpRapidCommitEnabled, mtu, isDhcpIpConflictDetectEnabled,
                false /* isIPv6OnlyPreferredEnabled */,
                null /* captivePortalApiUrl */, null /* displayName */, null /* scanResultInfo */,
                null /* layer2Info */);
    }

    private List<DhcpPacket> performDhcpHandshake() throws Exception {
        return performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                false /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
    }

    private DhcpPacket getNextDhcpPacket(final long timeout) throws Exception {
        byte[] packet;
        while ((packet = mDhcpPacketReadHead.getValue()
                .poll(timeout, this::isDhcpPacket)) != null) {
            final DhcpPacket dhcpPacket = DhcpPacket.decodeFullPacket(packet, packet.length,
                    ENCAP_L2);
            if (dhcpPacket != null) return dhcpPacket;
        }
        return null;
    }

    private DhcpPacket getNextDhcpPacket() throws Exception {
        final DhcpPacket packet = getNextDhcpPacket(PACKET_TIMEOUT_MS);
        assertNotNull("No expected DHCP packet received on interface within timeout", packet);
        return packet;
    }

    private DhcpPacket getReplyFromDhcpLease(final NetworkAttributes na, boolean timeout)
            throws Exception {
        doAnswer(invocation -> {
            if (timeout) return null;
            ((OnNetworkAttributesRetrievedListener) invocation.getArgument(1))
                    .onNetworkAttributesRetrieved(new Status(SUCCESS), TEST_L2KEY, na);
            return null;
        }).when(mIpMemoryStore).retrieveNetworkAttributes(eq(TEST_L2KEY), any());
        startIpClientProvisioning(true /* isDhcpLeaseCacheEnabled */,
                false /* shouldReplyRapidCommitAck */, false /* isPreconnectionEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        return getNextDhcpPacket();
    }

    private void removeTapInterface(final FileDescriptor fd) {
        try {
            Os.close(fd);
        } catch (ErrnoException e) {
            fail("Fail to close file descriptor: " + e);
        }
    }

    private void verifyAfterIpClientShutdown() throws RemoteException {
        final LinkProperties emptyLp = new LinkProperties();
        emptyLp.setInterfaceName(mIfaceName);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onLinkPropertiesChange(emptyLp);
    }

    // Verify IPv4-only provisioning success. No need to verify IPv4 provisioning when below cases
    // happen:
    // 1. if there's a failure lease, onProvisioningSuccess() won't be called;
    // 2. if duplicated IPv4 address detection is enabled, verify TIMEOUT will affect ARP packets
    //    capture running in other test cases.
    // 3. if IPv6 is enabled, e.g. withoutIPv6() isn't called when starting provisioning.
    private void verifyIPv4OnlyProvisioningSuccess(final Collection<InetAddress> addresses)
            throws Exception {
        final ArgumentCaptor<LinkProperties> captor = ArgumentCaptor.forClass(LinkProperties.class);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onProvisioningSuccess(captor.capture());
        LinkProperties lp = captor.getValue();
        assertNotNull(lp);
        assertNotEquals(0, lp.getDnsServers().size());
        assertEquals(addresses.size(), lp.getAddresses().size());
        assertTrue(lp.getAddresses().containsAll(addresses));
    }

    private void doRestoreInitialMtuTest(final boolean shouldChangeMtu,
            final boolean shouldRemoveTapInterface) throws Exception {
        final long currentTime = System.currentTimeMillis();
        int mtu = TEST_DEFAULT_MTU;

        if (shouldChangeMtu) mtu = TEST_MIN_MTU;
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                mtu, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, mtu);

        if (shouldChangeMtu) {
            // Pretend that ConnectivityService set the MTU.
            mNetd.interfaceSetMtu(mIfaceName, mtu);
            assertEquals(NetworkInterface.getByName(mIfaceName).getMTU(), mtu);
        }

        // Sometimes, IpClient receives an update with an empty LinkProperties during startup,
        // when the link-local address is deleted after interface bringup. Reset expectations
        // here to ensure that verifyAfterIpClientShutdown does not fail because it sees two
        // empty LinkProperties changes instead of one.
        reset(mCb);

        if (shouldRemoveTapInterface) removeTapInterface(mTapFd);
        try {
            mIpc.shutdown();
            awaitIpClientShutdown();
            if (shouldRemoveTapInterface) {
                verify(mNetd, never()).interfaceSetMtu(mIfaceName, TEST_DEFAULT_MTU);
            } else {
                // Verify that MTU indeed has been restored or not.
                verify(mNetd, times(shouldChangeMtu ? 1 : 0))
                        .interfaceSetMtu(mIfaceName, TEST_DEFAULT_MTU);
            }
            verifyAfterIpClientShutdown();
        } catch (Exception e) {
            fail("Exception should not have been thrown after shutdown: " + e);
        }
    }

    private DhcpPacket assertDiscoverPacketOnPreconnectionStart() throws Exception {
        final ArgumentCaptor<List<Layer2PacketParcelable>> l2PacketList =
                ArgumentCaptor.forClass(List.class);

        verify(mCb, timeout(TEST_TIMEOUT_MS)).onPreconnectionStart(l2PacketList.capture());
        final byte[] payload = l2PacketList.getValue().get(0).payload;
        DhcpPacket packet = DhcpPacket.decodeFullPacket(payload, payload.length, ENCAP_L2);
        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertArrayEquals(INADDR_BROADCAST.getAddress(),
                Arrays.copyOfRange(payload, IPV4_DST_ADDR_OFFSET, IPV4_DST_ADDR_OFFSET + 4));
        return packet;
    }

    private void doIpClientProvisioningWithPreconnectionTest(
            final boolean shouldReplyRapidCommitAck, final boolean shouldAbortPreconnection,
            final boolean shouldFirePreconnectionTimeout,
            final boolean timeoutBeforePreconnectionComplete) throws Exception {
        final long currentTime = System.currentTimeMillis();
        final ArgumentCaptor<InterfaceConfigurationParcel> ifConfig =
                ArgumentCaptor.forClass(InterfaceConfigurationParcel.class);

        startIpClientProvisioning(true /* isDhcpLeaseCacheEnabled */,
                shouldReplyRapidCommitAck, true /* isDhcpPreConnectionEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        DhcpPacket packet = assertDiscoverPacketOnPreconnectionStart();
        final int preconnDiscoverTransId = packet.getTransactionId();

        if (shouldAbortPreconnection) {
            if (shouldFirePreconnectionTimeout && timeoutBeforePreconnectionComplete) {
                mDependencies.mDhcpClient.sendMessage(DhcpClient.CMD_TIMEOUT);
            }

            mIpc.notifyPreconnectionComplete(false /* abort */);
            HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);

            if (shouldFirePreconnectionTimeout && !timeoutBeforePreconnectionComplete) {
                mDependencies.mDhcpClient.sendMessage(DhcpClient.CMD_TIMEOUT);
            }

            // Either way should get DhcpClient go back to INIT state, and broadcast
            // DISCOVER with new transaction ID.
            packet = getNextDhcpPacket();
            assertTrue(packet instanceof DhcpDiscoverPacket);
            assertTrue(packet.getTransactionId() != preconnDiscoverTransId);
        } else if (shouldFirePreconnectionTimeout && timeoutBeforePreconnectionComplete) {
            // If timeout fires before success preconnection, DhcpClient will go back to INIT state,
            // and broadcast DISCOVER with new transaction ID.
            mDependencies.mDhcpClient.sendMessage(DhcpClient.CMD_TIMEOUT);
            packet = getNextDhcpPacket();
            assertTrue(packet instanceof DhcpDiscoverPacket);
            assertTrue(packet.getTransactionId() != preconnDiscoverTransId);
            // any old response would be ignored due to mismatched transaction ID.
        }

        final short mtu = (short) TEST_DEFAULT_MTU;
        if (!shouldReplyRapidCommitAck) {
            mPacketReader.sendResponse(buildDhcpOfferPacket(packet, CLIENT_ADDR,
                    TEST_LEASE_DURATION_S, mtu, null /* captivePortalUrl */));
            packet = getNextDhcpPacket();
            assertTrue(packet instanceof DhcpRequestPacket);
        }
        mPacketReader.sendResponse(buildDhcpAckPacket(packet, CLIENT_ADDR, TEST_LEASE_DURATION_S,
                mtu, shouldReplyRapidCommitAck, null /* captivePortalUrl */));

        if (!shouldAbortPreconnection) {
            mIpc.notifyPreconnectionComplete(true /* success */);
            HandlerUtils.waitForIdle(mDependencies.mDhcpClient.getHandler(), TEST_TIMEOUT_MS);

            // If timeout fires after successful preconnection, right now DhcpClient will have
            // already entered BOUND state, the delayed CMD_TIMEOUT command would be ignored. So
            // this case should be very rare, because the timeout alarm is cancelled when state
            // machine exits from Preconnecting state.
            if (shouldFirePreconnectionTimeout && !timeoutBeforePreconnectionComplete) {
                mDependencies.mDhcpClient.sendMessage(DhcpClient.CMD_TIMEOUT);
            }
        }
        verify(mCb, timeout(TEST_TIMEOUT_MS)).setFallbackMulticastFilter(false);

        final LinkAddress ipAddress = new LinkAddress(CLIENT_ADDR, PREFIX_LENGTH);
        verify(mNetd, timeout(TEST_TIMEOUT_MS).times(1)).interfaceSetCfg(ifConfig.capture());
        assertEquals(ifConfig.getValue().ifName, mIfaceName);
        assertEquals(ifConfig.getValue().ipv4Addr, ipAddress.getAddress().getHostAddress());
        assertEquals(ifConfig.getValue().prefixLength, PREFIX_LENGTH);
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    private ArpPacket getNextArpPacket(final long timeout) throws Exception {
        byte[] packet;
        while ((packet = mArpPacketReadHead.getValue().poll(timeout, p -> true)) != null) {
            final ArpPacket arpPacket = parseArpPacketOrNull(packet);
            if (arpPacket != null) return arpPacket;
        }
        return null;
    }

    private ArpPacket getNextArpPacket() throws Exception {
        final ArpPacket packet = getNextArpPacket(PACKET_TIMEOUT_MS);
        assertNotNull("No expected ARP packet received on interface within timeout", packet);
        return packet;
    }

    private void assertArpPacket(final ArpPacket packet) {
        assertEquals(packet.opCode, ARP_REQUEST);
        assertEquals(packet.targetIp, CLIENT_ADDR);
        assertTrue(Arrays.equals(packet.senderHwAddress.toByteArray(), mClientMac));
    }

    private void assertArpProbe(final ArpPacket packet) {
        assertArpPacket(packet);
        assertEquals(packet.senderIp, INADDR_ANY);
    }

    private void assertArpAnnounce(final ArpPacket packet) {
        assertArpPacket(packet);
        assertEquals(packet.senderIp, CLIENT_ADDR);
    }

    private void assertGratuitousARP(final ArpPacket packet) {
        assertEquals(packet.opCode, ARP_REPLY);
        assertEquals(packet.senderIp, CLIENT_ADDR);
        assertEquals(packet.targetIp, CLIENT_ADDR);
        assertTrue(Arrays.equals(packet.senderHwAddress.toByteArray(), mClientMac));
        assertTrue(Arrays.equals(packet.targetHwAddress.toByteArray(), ETHER_BROADCAST));
    }

    private void doIpAddressConflictDetectionTest(final boolean causeIpAddressConflict,
            final boolean shouldReplyRapidCommitAck, final boolean isDhcpIpConflictDetectEnabled,
            final boolean shouldResponseArpReply) throws Exception {
        final long currentTime = System.currentTimeMillis();

        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, shouldReplyRapidCommitAck,
                TEST_DEFAULT_MTU, isDhcpIpConflictDetectEnabled);

        // If we receive an ARP packet here, it's guaranteed to be from IP conflict detection,
        // because at this time the test interface does not have an IP address and therefore
        // won't send ARP for anything.
        if (causeIpAddressConflict) {
            final ArpPacket arpProbe = getNextArpPacket();
            assertArpProbe(arpProbe);

            if (shouldResponseArpReply) {
                sendArpReply(mClientMac);
            } else {
                sendArpProbe();
            }
            final DhcpPacket packet = getNextDhcpPacket();
            assertTrue(packet instanceof DhcpDeclinePacket);
            assertEquals(packet.mServerIdentifier, SERVER_ADDR);
            assertEquals(packet.mRequestedIp, CLIENT_ADDR);

            verify(mCb, never()).onProvisioningFailure(any());
            assertIpMemoryNeverStoreNetworkAttributes();
        } else if (isDhcpIpConflictDetectEnabled) {
            int arpPacketCount = 0;
            final List<ArpPacket> packetList = new ArrayList<ArpPacket>();
            // Total sent ARP packets should be 5 (3 ARP Probes + 2 ARP Announcements)
            ArpPacket packet;
            while ((packet = getNextArpPacket(TEST_TIMEOUT_MS)) != null) {
                packetList.add(packet);
            }
            assertEquals(5, packetList.size());
            assertArpProbe(packetList.get(0));
            assertArpAnnounce(packetList.get(3));
        } else {
            verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
            assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime,
                    TEST_DEFAULT_MTU);
        }
    }

    @Test @SignatureRequiredTest(reason = "InterfaceParams.getByName requires CAP_NET_ADMIN")
    public void testInterfaceParams() throws Exception {
        InterfaceParams params = InterfaceParams.getByName(mIfaceName);
        assertNotNull(params);
        assertEquals(mIfaceName, params.name);
        assertTrue(params.index > 0);
        assertNotNull(params.macAddr);
        assertTrue(params.hasMacAddress);

        //  Check interface "lo".
        params = InterfaceParams.getByName("lo");
        assertNotNull(params);
        assertEquals("lo", params.name);
        assertTrue(params.index > 0);
        assertNotNull(params.macAddr);
        assertFalse(params.hasMacAddress);
    }

    @Test
    public void testDhcpInit() throws Exception {
        startIpClientProvisioning(false /* isDhcpLeaseCacheEnabled */,
                false /* shouldReplyRapidCommitAck */, false /* isPreconnectionEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        final DhcpPacket packet = getNextDhcpPacket();
        assertTrue(packet instanceof DhcpDiscoverPacket);
    }

    @Test
    public void testHandleSuccessDhcpLease() throws Exception {
        final long currentTime = System.currentTimeMillis();
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    @Test
    public void testHandleFailureDhcpLease() throws Exception {
        performDhcpHandshake(false /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);

        verify(mCb, never()).onProvisioningSuccess(any());
        assertIpMemoryNeverStoreNetworkAttributes();
    }

    @Test
    public void testHandleInfiniteLease() throws Exception {
        final long currentTime = System.currentTimeMillis();
        performDhcpHandshake(true /* isSuccessLease */, INFINITE_LEASE,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(INFINITE_LEASE, currentTime, TEST_DEFAULT_MTU);
    }

    @Test
    public void testHandleNoLease() throws Exception {
        final long currentTime = System.currentTimeMillis();
        performDhcpHandshake(true /* isSuccessLease */, null /* no lease time */,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(null, currentTime, TEST_DEFAULT_MTU);
    }

    @Test @IgnoreAfter(Build.VERSION_CODES.Q) // INIT-REBOOT is enabled on R.
    public void testHandleDisableInitRebootState() throws Exception {
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                false /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryNeverStoreNetworkAttributes();
    }

    @Test
    public void testHandleRapidCommitOption() throws Exception {
        final long currentTime = System.currentTimeMillis();
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, true /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientStartWithCachedInfiniteLease() throws Exception {
        final DhcpPacket packet = getReplyFromDhcpLease(
                new NetworkAttributes.Builder()
                    .setAssignedV4Address(CLIENT_ADDR)
                    .setAssignedV4AddressExpiry(Long.MAX_VALUE) // lease is always valid
                    .setMtu(new Integer(TEST_DEFAULT_MTU))
                    .setCluster(TEST_CLUSTER)
                    .setDnsAddresses(Collections.singletonList(SERVER_ADDR))
                    .build(), false /* timeout */);
        assertTrue(packet instanceof DhcpRequestPacket);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientStartWithCachedExpiredLease() throws Exception {
        final DhcpPacket packet = getReplyFromDhcpLease(
                 new NetworkAttributes.Builder()
                    .setAssignedV4Address(CLIENT_ADDR)
                    .setAssignedV4AddressExpiry(EXPIRED_LEASE)
                    .setMtu(new Integer(TEST_DEFAULT_MTU))
                    .setCluster(TEST_CLUSTER)
                    .setDnsAddresses(Collections.singletonList(SERVER_ADDR))
                    .build(), false /* timeout */);
        assertTrue(packet instanceof DhcpDiscoverPacket);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientStartWithNullRetrieveNetworkAttributes() throws Exception {
        final DhcpPacket packet = getReplyFromDhcpLease(null /* na */, false /* timeout */);
        assertTrue(packet instanceof DhcpDiscoverPacket);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientStartWithTimeoutRetrieveNetworkAttributes() throws Exception {
        final DhcpPacket packet = getReplyFromDhcpLease(
                new NetworkAttributes.Builder()
                    .setAssignedV4Address(CLIENT_ADDR)
                    .setAssignedV4AddressExpiry(System.currentTimeMillis() + 3_600_000)
                    .setMtu(new Integer(TEST_DEFAULT_MTU))
                    .setCluster(TEST_CLUSTER)
                    .setDnsAddresses(Collections.singletonList(SERVER_ADDR))
                    .build(), true /* timeout */);
        assertTrue(packet instanceof DhcpDiscoverPacket);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientStartWithCachedLeaseWithoutIPAddress() throws Exception {
        final DhcpPacket packet = getReplyFromDhcpLease(
                new NetworkAttributes.Builder()
                    .setMtu(new Integer(TEST_DEFAULT_MTU))
                    .setCluster(TEST_CLUSTER)
                    .setDnsAddresses(Collections.singletonList(SERVER_ADDR))
                    .build(), false /* timeout */);
        assertTrue(packet instanceof DhcpDiscoverPacket);
    }

    @Test
    public void testDhcpClientRapidCommitEnabled() throws Exception {
        startIpClientProvisioning(false /* isDhcpLeaseCacheEnabled */,
                true /* shouldReplyRapidCommitAck */, false /* isPreconnectionEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        final DhcpPacket packet = getNextDhcpPacket();
        assertTrue(packet instanceof DhcpDiscoverPacket);
    }

    @Test @IgnoreUpTo(Build.VERSION_CODES.Q)
    public void testDhcpServerInLinkProperties() throws Exception {
        assumeTrue(ConstantsShim.VERSION > Build.VERSION_CODES.Q);

        performDhcpHandshake();
        ArgumentCaptor<LinkProperties> captor = ArgumentCaptor.forClass(LinkProperties.class);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onProvisioningSuccess(captor.capture());
        assertEquals(SERVER_ADDR, captor.getValue().getDhcpServerAddress());
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRestoreInitialInterfaceMtu() throws Exception {
        doRestoreInitialMtuTest(true /* shouldChangeMtu */, false /* shouldRemoveTapInterface */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRestoreInitialInterfaceMtu_WithoutMtuChange() throws Exception {
        doRestoreInitialMtuTest(false /* shouldChangeMtu */, false /* shouldRemoveTapInterface */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRestoreInitialInterfaceMtu_WithException() throws Exception {
        doThrow(new RemoteException("NetdNativeService::interfaceSetMtu")).when(mNetd)
                .interfaceSetMtu(mIfaceName, TEST_DEFAULT_MTU);

        doRestoreInitialMtuTest(true /* shouldChangeMtu */, false /* shouldRemoveTapInterface */);
        assertEquals(NetworkInterface.getByName(mIfaceName).getMTU(), TEST_MIN_MTU);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRestoreInitialInterfaceMtu_NotFoundInterfaceWhenStopping() throws Exception {
        doRestoreInitialMtuTest(true /* shouldChangeMtu */, true /* shouldRemoveTapInterface */);
    }

    @Test
    public void testRestoreInitialInterfaceMtu_NotFoundInterfaceWhenStartingProvisioning()
            throws Exception {
        removeTapInterface(mTapFd);
        ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withoutIPv6()
                .build();

        startIpClientProvisioning(config);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onProvisioningFailure(any());
        verify(mCb, never()).setNeighborDiscoveryOffload(true);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRestoreInitialInterfaceMtu_stopIpClientAndRestart() throws Exception {
        long currentTime = System.currentTimeMillis();

        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_MIN_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_MIN_MTU);

        // Pretend that ConnectivityService set the MTU.
        mNetd.interfaceSetMtu(mIfaceName, TEST_MIN_MTU);
        assertEquals(NetworkInterface.getByName(mIfaceName).getMTU(), TEST_MIN_MTU);

        reset(mCb);
        reset(mIpMemoryStore);

        // Stop IpClient and then restart provisioning immediately.
        mIpc.stop();
        currentTime = System.currentTimeMillis();
        // Intend to set mtu option to 0, then verify that won't influence interface mtu restore.
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                0 /* mtu */, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, 0 /* mtu */);
        assertEquals(NetworkInterface.getByName(mIfaceName).getMTU(), TEST_DEFAULT_MTU);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRestoreInitialInterfaceMtu_removeInterfaceAndAddback() throws Exception {
        doAnswer(invocation -> {
            final LinkProperties lp = invocation.getArgument(0);
            assertEquals(lp.getInterfaceName(), mIfaceName);
            assertEquals(0, lp.getLinkAddresses().size());
            assertEquals(0, lp.getDnsServers().size());

            mDependencies.simulateInterfaceRecover();
            return null;
        }).when(mCb).onProvisioningFailure(any());

        final ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withoutIPv6()
                .build();

        // Intend to remove the tap interface and force IpClient throw provisioning failure
        // due to that interface is not found.
        removeTapInterface(mTapFd);
        assertNull(InterfaceParams.getByName(mIfaceName));

        startIpClientProvisioning(config);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onProvisioningFailure(any());

        // Make sure everything queued by this test was processed (e.g. transition to StoppingState
        // from ClearingIpAddressState) and tearDown will check if IpClient exits normally or crash.
        HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);
    }

    private boolean isRouterSolicitation(final byte[] packetBytes) {
        ByteBuffer packet = ByteBuffer.wrap(packetBytes);
        return packet.getShort(ETHER_TYPE_OFFSET) == (short) ETH_P_IPV6
                && packet.get(ETHER_HEADER_LEN + IPV6_PROTOCOL_OFFSET) == (byte) IPPROTO_ICMPV6
                && packet.get(ETHER_HEADER_LEN + IPV6_HEADER_LEN)
                        == (byte) ICMPV6_ROUTER_SOLICITATION;
    }

    private boolean isNeighborAdvertisement(final byte[] packetBytes) {
        ByteBuffer packet = ByteBuffer.wrap(packetBytes);
        return packet.getShort(ETHER_TYPE_OFFSET) == (short) ETH_P_IPV6
                && packet.get(ETHER_HEADER_LEN + IPV6_PROTOCOL_OFFSET) == (byte) IPPROTO_ICMPV6
                && packet.get(ETHER_HEADER_LEN + IPV6_HEADER_LEN)
                        == (byte) ICMPV6_NEIGHBOR_ADVERTISEMENT;
    }

    private NeighborAdvertisement getNextNeighborAdvertisement() throws ParseException {
        final byte[] packet = mPacketReader.popPacket(PACKET_TIMEOUT_MS,
                this::isNeighborAdvertisement);
        if (packet == null) return null;

        final NeighborAdvertisement na = parseNeighborAdvertisementOrNull(packet);
        assertNotNull("Invalid neighbour advertisement received", na);
        return na;
    }

    private void waitForRouterSolicitation() throws ParseException {
        assertNotNull("No router solicitation received on interface within timeout",
                mPacketReader.popPacket(PACKET_TIMEOUT_MS, this::isRouterSolicitation));
    }

    private void sendRouterAdvertisement(boolean waitForRs, short lifetime) throws Exception {
        final String dnsServer = "2001:4860:4860::64";
        final ByteBuffer pio = buildPioOption(3600, 1800, "2001:db8:1::/64");
        ByteBuffer rdnss = buildRdnssOption(3600, dnsServer);
        ByteBuffer ra = buildRaPacket(lifetime, pio, rdnss);

        if (waitForRs) {
            waitForRouterSolicitation();
        }

        mPacketReader.sendResponse(ra);
    }

    private void sendBasicRouterAdvertisement(boolean waitForRs) throws Exception {
        sendRouterAdvertisement(waitForRs, (short) 1800);
    }

    private void sendRouterAdvertisementWithZeroLifetime() throws Exception {
        sendRouterAdvertisement(false /* waitForRs */, (short) 0);
    }

    // TODO: move this and the following method to a common location and use them in ApfTest.
    private static ByteBuffer buildPioOption(int valid, int preferred, String prefixString)
            throws Exception {
        return PrefixInformationOption.build(new IpPrefix(prefixString),
                (byte) (PIO_FLAG_ON_LINK | PIO_FLAG_AUTONOMOUS), valid, preferred);
    }

    private static ByteBuffer buildRdnssOption(int lifetime, String... servers) throws Exception {
        return RdnssOption.build(lifetime, servers);
    }

    private static ByteBuffer buildRaPacket(short lifetime, ByteBuffer... options)
            throws Exception {
        final MacAddress dstMac = MacAddress.fromString("33:33:00:00:00:01");
        final MacAddress srcMac = MacAddress.fromString("01:02:03:04:05:06");
        final Inet6Address routerLinkLocal =
                (Inet6Address) InetAddresses.parseNumericAddress("fe80::1");

        return Ipv6Utils.buildRaPacket(srcMac, dstMac, routerLinkLocal,
                IPV6_ADDR_ALL_NODES_MULTICAST, (byte) 0 /* M=0, O=0 */, lifetime,
                0 /* Reachable time, unspecified */, 100 /* Retrans time 100ms */,
                options);
    }

    private static ByteBuffer buildRaPacket(ByteBuffer... options) throws Exception {
        return buildRaPacket((short) 1800, options);
    }

    private void disableIpv6ProvisioningDelays() throws Exception {
        // Speed up the test by disabling DAD and removing router_solicitation_delay.
        // We don't need to restore the default value because the interface is removed in tearDown.
        // TODO: speed up further by not waiting for RS but keying off first IPv6 packet.
        mNetd.setProcSysNet(INetd.IPV6, INetd.CONF, mIfaceName, "router_solicitation_delay", "0");
        mNetd.setProcSysNet(INetd.IPV6, INetd.CONF, mIfaceName, "dad_transmits", "0");
    }

    private void assertHasAddressThat(String msg, LinkProperties lp,
            Predicate<LinkAddress> condition) {
        for (LinkAddress addr : lp.getLinkAddresses()) {
            if (condition.test(addr)) {
                return;
            }
        }
        fail(msg + " not found in: " + lp);
    }

    private boolean hasFlag(LinkAddress addr, int flag) {
        return (addr.getFlags() & flag) == flag;
    }

    private boolean isPrivacyAddress(LinkAddress addr) {
        return addr.isGlobalPreferred() && hasFlag(addr, IFA_F_TEMPORARY);
    }

    private boolean isStablePrivacyAddress(LinkAddress addr) {
        // TODO: move away from getting address updates from netd and make this work on Q as well.
        final int flag = ShimUtils.isAtLeastR() ? IFA_F_STABLE_PRIVACY : 0;
        return addr.isGlobalPreferred() && hasFlag(addr, flag);
    }

    private LinkProperties doIpv6OnlyProvisioning() throws Exception {
        final InOrder inOrder = inOrder(mCb);
        final String dnsServer = "2001:4860:4860::64";
        final ByteBuffer pio = buildPioOption(3600, 1800, "2001:db8:1::/64");
        final ByteBuffer rdnss = buildRdnssOption(3600, dnsServer);
        final ByteBuffer ra = buildRaPacket(pio, rdnss);

        return doIpv6OnlyProvisioning(inOrder, ra);
    }

    private LinkProperties doIpv6OnlyProvisioning(InOrder inOrder, ByteBuffer ra) throws Exception {
        waitForRouterSolicitation();
        mPacketReader.sendResponse(ra);

        // The lambda below needs to write a LinkProperties to a local variable, but lambdas cannot
        // write to non-final local variables. So declare a final variable to write to.
        final AtomicReference<LinkProperties> lpRef = new AtomicReference<>();

        ArgumentCaptor<LinkProperties> captor = ArgumentCaptor.forClass(LinkProperties.class);
        verifyWithTimeout(inOrder, mCb).onProvisioningSuccess(captor.capture());
        lpRef.set(captor.getValue());

        // Sometimes provisioning completes as soon as the link-local and the stable address appear,
        // before the privacy address appears. If so, wait here for the LinkProperties update that
        // contains all three address. Otherwise, future calls to verify() might get confused.
        if (captor.getValue().getLinkAddresses().size() == 2) {
            verifyWithTimeout(inOrder, mCb).onLinkPropertiesChange(argThat(lp -> {
                lpRef.set(lp);
                return lp.getLinkAddresses().size() == 3;
            }));
        }

        LinkProperties lp = lpRef.get();
        assertEquals("Should have 3 IPv6 addresses after provisioning: " + lp,
                3, lp.getLinkAddresses().size());
        assertHasAddressThat("link-local address", lp, x -> x.getAddress().isLinkLocalAddress());
        assertHasAddressThat("privacy address", lp, this::isPrivacyAddress);
        assertHasAddressThat("stable privacy address", lp, this::isStablePrivacyAddress);

        return lp;
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRaRdnss() throws Exception {
        ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withoutIPv4()
                .build();
        startIpClientProvisioning(config);

        InOrder inOrder = inOrder(mCb);
        ArgumentCaptor<LinkProperties> captor = ArgumentCaptor.forClass(LinkProperties.class);

        final String dnsServer = "2001:4860:4860::64";
        final String lowlifeDnsServer = "2001:4860:4860::6464";

        final ByteBuffer pio = buildPioOption(600, 300, "2001:db8:1::/64");
        ByteBuffer rdnss1 = buildRdnssOption(60, lowlifeDnsServer);
        ByteBuffer rdnss2 = buildRdnssOption(600, dnsServer);
        ByteBuffer ra = buildRaPacket(pio, rdnss1, rdnss2);

        LinkProperties lp = doIpv6OnlyProvisioning(inOrder, ra);

        // Expect that DNS servers with lifetimes below CONFIG_MIN_RDNSS_LIFETIME are not accepted.
        assertNotNull(lp);
        assertEquals(1, lp.getDnsServers().size());
        assertTrue(lp.getDnsServers().contains(InetAddress.getByName(dnsServer)));

        // If the RDNSS lifetime is above the minimum, the DNS server is accepted.
        rdnss1 = buildRdnssOption(68, lowlifeDnsServer);
        ra = buildRaPacket(pio, rdnss1, rdnss2);
        mPacketReader.sendResponse(ra);
        inOrder.verify(mCb, timeout(TEST_TIMEOUT_MS)).onLinkPropertiesChange(captor.capture());
        lp = captor.getValue();
        assertNotNull(lp);
        assertEquals(2, lp.getDnsServers().size());
        assertTrue(lp.getDnsServers().contains(InetAddress.getByName(dnsServer)));
        assertTrue(lp.getDnsServers().contains(InetAddress.getByName(lowlifeDnsServer)));

        // Expect that setting RDNSS lifetime of 0 causes loss of provisioning.
        rdnss1 = buildRdnssOption(0, dnsServer);
        rdnss2 = buildRdnssOption(0, lowlifeDnsServer);
        ra = buildRaPacket(pio, rdnss1, rdnss2);
        mPacketReader.sendResponse(ra);

        inOrder.verify(mCb, timeout(TEST_TIMEOUT_MS)).onProvisioningFailure(captor.capture());
        lp = captor.getValue();
        assertNotNull(lp);
        assertEquals(0, lp.getDnsServers().size());
        reset(mCb);
    }

    private void expectNat64PrefixUpdate(InOrder inOrder, IpPrefix expected) throws Exception {
        inOrder.verify(mCb, timeout(TEST_TIMEOUT_MS)).onLinkPropertiesChange(
                argThat(lp -> Objects.equals(expected, lp.getNat64Prefix())));

    }

    private void expectNoNat64PrefixUpdate(InOrder inOrder, IpPrefix unchanged) throws Exception {
        inOrder.verify(mCb, timeout(TEST_TIMEOUT_MS).times(0)).onLinkPropertiesChange(argThat(
                lp -> !Objects.equals(unchanged, lp.getNat64Prefix())));

    }

    @Test @IgnoreUpTo(Build.VERSION_CODES.Q)
    @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testPref64Option() throws Exception {
        assumeTrue(ConstantsShim.VERSION > Build.VERSION_CODES.Q);

        ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withoutIPv4()
                .build();
        startIpClientProvisioning(config);

        final String dnsServer = "2001:4860:4860::64";
        final IpPrefix prefix = new IpPrefix("64:ff9b::/96");
        final IpPrefix otherPrefix = new IpPrefix("2001:db8:64::/96");

        final ByteBuffer pio = buildPioOption(600, 300, "2001:db8:1::/64");
        ByteBuffer rdnss = buildRdnssOption(600, dnsServer);
        ByteBuffer pref64 = new StructNdOptPref64(prefix, 600).toByteBuffer();
        ByteBuffer ra = buildRaPacket(pio, rdnss, pref64);

        // The NAT64 prefix might be detected before or after provisioning success.
        // Don't test order between these two events.
        LinkProperties lp = doIpv6OnlyProvisioning(null /*inOrder*/, ra);
        expectAlarmSet(null /*inOrder*/, "PREF64", 600);

        // From now on expect events in order.
        InOrder inOrder = inOrder(mCb, mAlarm);
        if (lp.getNat64Prefix() != null) {
            assertEquals(prefix, lp.getNat64Prefix());
        } else {
            expectNat64PrefixUpdate(inOrder, prefix);
        }

        // Increase the lifetime and expect the prefix not to change.
        pref64 = new StructNdOptPref64(prefix, 1800).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64);
        mPacketReader.sendResponse(ra);
        OnAlarmListener pref64Alarm = expectAlarmSet(inOrder, "PREF64", 1800);
        expectNoNat64PrefixUpdate(inOrder, prefix);
        reset(mCb, mAlarm);

        // Reduce the lifetime and expect to reschedule expiry.
        pref64 = new StructNdOptPref64(prefix, 1500).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64);
        mPacketReader.sendResponse(ra);
        pref64Alarm = expectAlarmSet(inOrder, "PREF64", 1496);
        expectNoNat64PrefixUpdate(inOrder, prefix);
        reset(mCb, mAlarm);

        // Withdraw the prefix and expect it to be set to null.
        pref64 = new StructNdOptPref64(prefix, 0).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64);
        mPacketReader.sendResponse(ra);
        expectAlarmCancelled(inOrder, pref64Alarm);
        expectNat64PrefixUpdate(inOrder, null);
        reset(mCb, mAlarm);

        // Re-announce the prefix.
        pref64 = new StructNdOptPref64(prefix, 600).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64);
        mPacketReader.sendResponse(ra);
        expectAlarmSet(inOrder, "PREF64", 600);
        expectNat64PrefixUpdate(inOrder, prefix);
        reset(mCb, mAlarm);

        // Announce two prefixes. Don't expect any update because if there is already a NAT64
        // prefix, any new prefix is ignored.
        ByteBuffer otherPref64 = new StructNdOptPref64(otherPrefix, 1200).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64, otherPref64);
        mPacketReader.sendResponse(ra);
        expectAlarmSet(inOrder, "PREF64", 600);
        expectNoNat64PrefixUpdate(inOrder, prefix);
        reset(mCb, mAlarm);

        // Withdraw the old prefix and continue to announce the new one. Expect a prefix change.
        pref64 = new StructNdOptPref64(prefix, 0).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64, otherPref64);
        mPacketReader.sendResponse(ra);
        expectAlarmCancelled(inOrder, pref64Alarm);
        // Need a different OnAlarmListener local variable because posting it to the handler in the
        // lambda below requires it to be final.
        final OnAlarmListener lastAlarm = expectAlarmSet(inOrder, "PREF64", 1200);
        expectNat64PrefixUpdate(inOrder, otherPrefix);
        reset(mCb, mAlarm);

        // Simulate prefix expiry.
        mIpc.getHandler().post(() -> lastAlarm.onAlarm());
        expectAlarmCancelled(inOrder, pref64Alarm);
        expectNat64PrefixUpdate(inOrder, null);

        // Announce a non-/96 prefix and expect it to be ignored.
        IpPrefix invalidPrefix = new IpPrefix("64:ff9b::/64");
        pref64 = new StructNdOptPref64(invalidPrefix, 1200).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64);
        mPacketReader.sendResponse(ra);
        expectNoNat64PrefixUpdate(inOrder, invalidPrefix);

        // Re-announce the prefix.
        pref64 = new StructNdOptPref64(prefix, 600).toByteBuffer();
        ra = buildRaPacket(pio, rdnss, pref64);
        mPacketReader.sendResponse(ra);
        final OnAlarmListener clearAlarm = expectAlarmSet(inOrder, "PREF64", 600);
        expectNat64PrefixUpdate(inOrder, prefix);
        reset(mCb, mAlarm);

        // Check that the alarm is cancelled when IpClient is stopped.
        mIpc.stop();
        HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);
        expectAlarmCancelled(inOrder, clearAlarm);
        expectNat64PrefixUpdate(inOrder, null);

        // Check that even if the alarm was already in the message queue while it was cancelled, it
        // is safely ignored.
        mIpc.getHandler().post(() -> clearAlarm.onAlarm());
        HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);
    }

    private void addIpAddressAndWaitForIt(final String iface) throws Exception {
        final CountDownLatch latch = new CountDownLatch(1);

        final String addr1 = "192.0.2.99";
        final String addr2 = "192.0.2.3";
        final int prefixLength = 26;

        // Add two IPv4 addresses to the specified interface, and proceed when the NetworkObserver
        // has seen the second one. This ensures that every other NetworkObserver registered with
        // mNetworkObserverRegistry - in particular, IpClient's - has seen the addition of the first
        // address.
        final LinkAddress trigger = new LinkAddress(addr2 + "/" + prefixLength);
        NetworkObserver observer = new NetworkObserver() {
            @Override
            public void onInterfaceAddressUpdated(LinkAddress address, String ifName) {
                if (ifName.equals(iface) && address.isSameAddressAs(trigger)) {
                    latch.countDown();
                }
            }
        };

        mNetworkObserverRegistry.registerObserverForNonblockingCallback(observer);
        try {
            mNetd.interfaceAddAddress(iface, addr1, prefixLength);
            mNetd.interfaceAddAddress(iface, addr2, prefixLength);
            assertTrue("Trigger IP address " + addr2 + " not seen after " + TEST_TIMEOUT_MS + "ms",
                    latch.await(TEST_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        } finally {
            mNetworkObserverRegistry.unregisterObserver(observer);
        }

        // Wait for IpClient to process the addition of the address.
        HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);
    }

    private void doIPv4OnlyProvisioningAndExitWithLeftAddress() throws Exception {
        final long currentTime = System.currentTimeMillis();
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* shouldReplyRapidCommitAck */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);

        // Stop IpClient and expect a final LinkProperties callback with an empty LP.
        mIIpClient.stop();
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onLinkPropertiesChange(argThat(
                x -> x.getAddresses().size() == 0
                        && x.getRoutes().size() == 0
                        && x.getDnsServers().size() == 0));
        reset(mCb);

        // Pretend that something else (e.g., Tethering) used the interface and left an IP address
        // configured on it. When IpClient starts, it must clear this address before proceeding.
        // The address must be noticed before startProvisioning is called, or IpClient will
        // immediately declare provisioning success due to the presence of an IPv4 address.
        // The address must be IPv4 because IpClient clears IPv6 addresses on startup.
        //
        // TODO: once IpClient gets IP addresses directly from netlink instead of from netd, it
        // may be sufficient to call waitForIdle to see if IpClient has seen the address.
        addIpAddressAndWaitForIt(mIfaceName);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testIpClientClearingIpAddressState() throws Exception {
        doIPv4OnlyProvisioningAndExitWithLeftAddress();

        ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .build();
        startIpClientProvisioning(config);

        sendBasicRouterAdvertisement(true /*waitForRs*/);

        // Check that the IPv4 addresses configured earlier are not in LinkProperties...
        ArgumentCaptor<LinkProperties> captor = ArgumentCaptor.forClass(LinkProperties.class);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onProvisioningSuccess(captor.capture());
        assertFalse(captor.getValue().hasIpv4Address());

        // ... or configured on the interface.
        InterfaceConfigurationParcel cfg = mNetd.interfaceGetCfg(mIfaceName);
        assertEquals("0.0.0.0", cfg.ipv4Addr);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testIpClientClearingIpAddressState_enablePreconnection() throws Exception {
        doIPv4OnlyProvisioningAndExitWithLeftAddress();

        // Enter ClearingIpAddressesState to clear the remaining IPv4 addresses and transition to
        // PreconnectionState instead of RunningState.
        startIpClientProvisioning(false /* isDhcpLeaseCacheEnabled */,
                false /* shouldReplyRapidCommitAck */, true /* isDhcpPreConnectionEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        assertDiscoverPacketOnPreconnectionStart();

        // Force to enter RunningState.
        mIpc.notifyPreconnectionComplete(false /* abort */);
        HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_success() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(true /* shouldReplyRapidCommitAck */,
                false /* shouldAbortPreconnection */, false /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_SuccessWithoutRapidCommit() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(false /* shouldReplyRapidCommitAck */,
                false /* shouldAbortPreconnection */, false /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_Abort() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(true /* shouldReplyRapidCommitAck */,
                true /* shouldAbortPreconnection */, false /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_AbortWithoutRapiCommit() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(false /* shouldReplyRapidCommitAck */,
                true /* shouldAbortPreconnection */, false /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutBeforeAbort() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(true /* shouldReplyRapidCommitAck */,
                true /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                true /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutBeforeAbortWithoutRapidCommit()
            throws Exception {
        doIpClientProvisioningWithPreconnectionTest(false /* shouldReplyRapidCommitAck */,
                true /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                true /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutafterAbort() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(true /* shouldReplyRapidCommitAck */,
                true /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutAfterAbortWithoutRapidCommit() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(false /* shouldReplyRapidCommitAck */,
                true /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutBeforeSuccess() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(true /* shouldReplyRapidCommitAck */,
                false /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                true /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutBeforeSuccessWithoutRapidCommit()
            throws Exception {
        doIpClientProvisioningWithPreconnectionTest(false /* shouldReplyRapidCommitAck */,
                false /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                true /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutAfterSuccess() throws Exception {
        doIpClientProvisioningWithPreconnectionTest(true /* shouldReplyRapidCommitAck */,
                false /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_TimeoutAfterSuccessWithoutRapidCommit()
            throws Exception {
        doIpClientProvisioningWithPreconnectionTest(false /* shouldReplyRapidCommitAck */,
                false /* shouldAbortPreconnection */, true /* shouldFirePreconnectionTimeout */,
                false /* timeoutBeforePreconnectionComplete */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpClientPreconnection_WithoutLayer2InfoWhenStartingProv() throws Exception {
        // For FILS connection, current bssid (also l2key and cluster) is still null when
        // starting provisioning since the L2 link hasn't been established yet. Ensure that
        // IpClient won't crash even if initializing an Layer2Info class with null members.
        ProvisioningConfiguration.Builder prov = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withoutIPv6()
                .withPreconnection()
                .withLayer2Information(new Layer2Information(null /* l2key */, null /* cluster */,
                        null /* bssid */));

        startIpClientProvisioning(prov.build());
        assertDiscoverPacketOnPreconnectionStart();
        verify(mCb).setNeighborDiscoveryOffload(true);

        // Force IpClient transition to RunningState from PreconnectionState.
        mIIpClient.notifyPreconnectionComplete(false /* success */);
        HandlerUtils.waitForIdle(mDependencies.mDhcpClient.getHandler(), TEST_TIMEOUT_MS);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).setFallbackMulticastFilter(false);
    }

    @Test
    public void testDhcpDecline_conflictByArpReply() throws Exception {
        doIpAddressConflictDetectionTest(true /* causeIpAddressConflict */,
                false /* shouldReplyRapidCommitAck */, true /* isDhcpIpConflictDetectEnabled */,
                true /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_conflictByArpProbe() throws Exception {
        doIpAddressConflictDetectionTest(true /* causeIpAddressConflict */,
                false /* shouldReplyRapidCommitAck */, true /* isDhcpIpConflictDetectEnabled */,
                false /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_EnableFlagWithoutIpConflict() throws Exception {
        doIpAddressConflictDetectionTest(false /* causeIpAddressConflict */,
                false /* shouldReplyRapidCommitAck */, true /* isDhcpIpConflictDetectEnabled */,
                false /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_WithoutIpConflict() throws Exception {
        doIpAddressConflictDetectionTest(false /* causeIpAddressConflict */,
                false /* shouldReplyRapidCommitAck */, false /* isDhcpIpConflictDetectEnabled */,
                false /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_WithRapidCommitWithoutIpConflict() throws Exception {
        doIpAddressConflictDetectionTest(false /* causeIpAddressConflict */,
                true /* shouldReplyRapidCommitAck */, false /* isDhcpIpConflictDetectEnabled */,
                false /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_WithRapidCommitConflictByArpReply() throws Exception {
        doIpAddressConflictDetectionTest(true /* causeIpAddressConflict */,
                true /* shouldReplyRapidCommitAck */, true /* isDhcpIpConflictDetectEnabled */,
                true /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_WithRapidCommitConflictByArpProbe() throws Exception {
        doIpAddressConflictDetectionTest(true /* causeIpAddressConflict */,
                true /* shouldReplyRapidCommitAck */, true /* isDhcpIpConflictDetectEnabled */,
                false /* shouldResponseArpReply */);
    }

    @Test
    public void testDhcpDecline_EnableFlagWithRapidCommitWithoutIpConflict() throws Exception {
        doIpAddressConflictDetectionTest(false /* causeIpAddressConflict */,
                true /* shouldReplyRapidCommitAck */, true /* isDhcpIpConflictDetectEnabled */,
                false /* shouldResponseArpReply */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testHostname_enableConfig() throws Exception {
        mDependencies.setHostnameConfiguration(true /* isHostnameConfigurationEnabled */,
                TEST_HOST_NAME);

        final long currentTime = System.currentTimeMillis();
        final List<DhcpPacket> sentPackets = performDhcpHandshake(true /* isSuccessLease */,
                TEST_LEASE_DURATION_S, true /* isDhcpLeaseCacheEnabled */,
                false /* isDhcpRapidCommitEnabled */, TEST_DEFAULT_MTU,
                false /* isDhcpIpConflictDetectEnabled */);

        assertEquals(2, sentPackets.size());
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertHostname(true, TEST_HOST_NAME, TEST_HOST_NAME_TRANSLITERATION, sentPackets);
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testHostname_disableConfig() throws Exception {
        mDependencies.setHostnameConfiguration(false /* isHostnameConfigurationEnabled */,
                TEST_HOST_NAME);

        final long currentTime = System.currentTimeMillis();
        final List<DhcpPacket> sentPackets = performDhcpHandshake(true /* isSuccessLease */,
                TEST_LEASE_DURATION_S, true /* isDhcpLeaseCacheEnabled */,
                false /* isDhcpRapidCommitEnabled */, TEST_DEFAULT_MTU,
                false /* isDhcpIpConflictDetectEnabled */);

        assertEquals(2, sentPackets.size());
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertHostname(false, TEST_HOST_NAME, TEST_HOST_NAME_TRANSLITERATION, sentPackets);
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testHostname_enableConfigWithNullHostname() throws Exception {
        mDependencies.setHostnameConfiguration(true /* isHostnameConfigurationEnabled */,
                null /* hostname */);

        final long currentTime = System.currentTimeMillis();
        final List<DhcpPacket> sentPackets = performDhcpHandshake(true /* isSuccessLease */,
                TEST_LEASE_DURATION_S, true /* isDhcpLeaseCacheEnabled */,
                false /* isDhcpRapidCommitEnabled */, TEST_DEFAULT_MTU,
                false /* isDhcpIpConflictDetectEnabled */);

        assertEquals(2, sentPackets.size());
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertHostname(true, null /* hostname */, null /* hostnameAfterTransliteration */,
                sentPackets);
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    private void runDhcpClientCaptivePortalApiTest(boolean featureEnabled,
            boolean serverSendsOption) throws Exception {
        startIpClientProvisioning(false /* isDhcpLeaseCacheEnabled */,
                false /* shouldReplyRapidCommitAck */, false /* isPreConnectionEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        final DhcpPacket discover = getNextDhcpPacket();
        assertTrue(discover instanceof DhcpDiscoverPacket);
        assertEquals(featureEnabled, discover.hasRequestedParam(DhcpPacket.DHCP_CAPTIVE_PORTAL));

        // Send Offer and handle Request -> Ack
        final String serverSentUrl = serverSendsOption ? TEST_CAPTIVE_PORTAL_URL : null;
        mPacketReader.sendResponse(buildDhcpOfferPacket(discover, CLIENT_ADDR,
                TEST_LEASE_DURATION_S, (short) TEST_DEFAULT_MTU, serverSentUrl));
        final int testMtu = 1345;
        handleDhcpPackets(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                false /* shouldReplyRapidCommitAck */, testMtu, serverSentUrl);

        final Uri expectedUrl = featureEnabled && serverSendsOption
                ? Uri.parse(TEST_CAPTIVE_PORTAL_URL) : null;
        // LinkProperties will be updated multiple times. Wait for it to contain DHCP-obtained info,
        // such as MTU.
        final ArgumentCaptor<LinkProperties> captor = ArgumentCaptor.forClass(LinkProperties.class);
        verify(mCb, timeout(TEST_TIMEOUT_MS).atLeastOnce()).onLinkPropertiesChange(
                argThat(lp -> lp.getMtu() == testMtu));

        // Ensure that the URL was set as expected in the callbacks.
        // Can't verify the URL up to Q as there is no such attribute in LinkProperties.
        if (!ShimUtils.isAtLeastR()) return;
        verify(mCb, atLeastOnce()).onLinkPropertiesChange(captor.capture());
        assertTrue(captor.getAllValues().stream().anyMatch(
                lp -> Objects.equals(expectedUrl, lp.getCaptivePortalApiUrl())));
    }

    @Test
    public void testDhcpClientCaptivePortalApiEnabled() throws Exception {
        // Only run the test on platforms / builds where the API is enabled
        assumeTrue(CaptivePortalDataShimImpl.isSupported());
        runDhcpClientCaptivePortalApiTest(true /* featureEnabled */, true /* serverSendsOption */);
    }

    @Test
    public void testDhcpClientCaptivePortalApiEnabled_NoUrl() throws Exception {
        // Only run the test on platforms / builds where the API is enabled
        assumeTrue(CaptivePortalDataShimImpl.isSupported());
        runDhcpClientCaptivePortalApiTest(true /* featureEnabled */, false /* serverSendsOption */);
    }

    @Test
    public void testDhcpClientCaptivePortalApiDisabled() throws Exception {
        // Only run the test on platforms / builds where the API is disabled
        assumeFalse(CaptivePortalDataShimImpl.isSupported());
        runDhcpClientCaptivePortalApiTest(false /* featureEnabled */, true /* serverSendsOption */);
    }

    private ScanResultInfo makeScanResultInfo(final int id, final String ssid,
            final String bssid, final byte[] oui, final byte type, final byte[] data) {
        final ByteBuffer payload = ByteBuffer.allocate(4 + data.length);
        payload.put(oui);
        payload.put(type);
        payload.put(data);
        payload.flip();
        final ScanResultInfo.InformationElement ie =
                new ScanResultInfo.InformationElement(id /* IE id */, payload);
        return new ScanResultInfo(ssid, bssid, Collections.singletonList(ie));
    }

    private ScanResultInfo makeScanResultInfo(final int id, final byte[] oui, final byte type) {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        return makeScanResultInfo(id, TEST_DEFAULT_SSID, TEST_DEFAULT_BSSID, oui, type, data);
    }

    private ScanResultInfo makeScanResultInfo(final String ssid, final String bssid) {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        return makeScanResultInfo(0xdd, ssid, bssid, TEST_AP_OUI, (byte) 0x06, data);
    }

    private void doUpstreamHotspotDetectionTest(final int id, final String displayName,
            final String ssid, final byte[] oui, final byte type, final byte[] data,
            final boolean expectMetered) throws Exception {
        final ScanResultInfo info = makeScanResultInfo(id, ssid, TEST_DEFAULT_BSSID, oui, type,
                data);
        final long currentTime = System.currentTimeMillis();
        final List<DhcpPacket> sentPackets = performDhcpHandshake(true /* isSuccessLease */,
                TEST_LEASE_DURATION_S, true /* isDhcpLeaseCacheEnabled */,
                false /* isDhcpRapidCommitEnabled */, TEST_DEFAULT_MTU,
                false /* isDhcpIpConflictDetectEnabled */,
                false /* isIPv6OnlyPreferredEnabled */,
                null /* captivePortalApiUrl */, displayName, info /* scanResultInfo */,
                null /* layer2Info */);
        assertEquals(2, sentPackets.size());
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));

        ArgumentCaptor<DhcpResultsParcelable> captor =
                ArgumentCaptor.forClass(DhcpResultsParcelable.class);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).onNewDhcpResults(captor.capture());
        DhcpResults lease = fromStableParcelable(captor.getValue());
        assertNotNull(lease);
        assertEquals(lease.getIpAddress().getAddress(), CLIENT_ADDR);
        assertEquals(lease.getGateway(), SERVER_ADDR);
        assertEquals(1, lease.getDnsServers().size());
        assertTrue(lease.getDnsServers().contains(SERVER_ADDR));
        assertEquals(lease.getServerAddress(), SERVER_ADDR);
        assertEquals(lease.getMtu(), TEST_DEFAULT_MTU);

        if (expectMetered) {
            assertEquals(lease.vendorInfo, DhcpPacket.VENDOR_INFO_ANDROID_METERED);
        } else {
            assertNull(lease.vendorInfo);
        }

        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    @Test
    public void testUpstreamHotspotDetection() throws Exception {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        doUpstreamHotspotDetectionTest(0xdd, "\"ssid\"", "ssid",
                new byte[] { (byte) 0x00, (byte) 0x17, (byte) 0xF2 }, (byte) 0x06, data,
                true /* expectMetered */);
    }

    @Test
    public void testUpstreamHotspotDetection_incorrectIeId() throws Exception {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        doUpstreamHotspotDetectionTest(0xdc, "\"ssid\"", "ssid",
                new byte[] { (byte) 0x00, (byte) 0x17, (byte) 0xF2 }, (byte) 0x06, data,
                false /* expectMetered */);
    }

    @Test
    public void testUpstreamHotspotDetection_incorrectOUI() throws Exception {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        doUpstreamHotspotDetectionTest(0xdd, "\"ssid\"", "ssid",
                new byte[] { (byte) 0x00, (byte) 0x1A, (byte) 0x11 }, (byte) 0x06, data,
                false /* expectMetered */);
    }

    @Test
    public void testUpstreamHotspotDetection_incorrectSsid() throws Exception {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        doUpstreamHotspotDetectionTest(0xdd, "\"another ssid\"", "ssid",
                new byte[] { (byte) 0x00, (byte) 0x17, (byte) 0xF2 }, (byte) 0x06, data,
                false /* expectMetered */);
    }

    @Test
    public void testUpstreamHotspotDetection_incorrectType() throws Exception {
        byte[] data = new byte[10];
        new Random().nextBytes(data);
        doUpstreamHotspotDetectionTest(0xdd, "\"ssid\"", "ssid",
                new byte[] { (byte) 0x00, (byte) 0x17, (byte) 0xF2 }, (byte) 0x0a, data,
                false /* expectMetered */);
    }

    @Test
    public void testUpstreamHotspotDetection_zeroLengthData() throws Exception {
        byte[] data = new byte[0];
        doUpstreamHotspotDetectionTest(0xdd, "\"ssid\"", "ssid",
                new byte[] { (byte) 0x00, (byte) 0x17, (byte) 0xF2 }, (byte) 0x06, data,
                true /* expectMetered */);
    }

    private void forceLayer2Roaming() throws Exception {
        final Layer2InformationParcelable roamingInfo = new Layer2InformationParcelable();
        roamingInfo.bssid = MacAddress.fromString(TEST_DHCP_ROAM_BSSID);
        roamingInfo.l2Key = TEST_DHCP_ROAM_L2KEY;
        roamingInfo.cluster = TEST_DHCP_ROAM_CLUSTER;
        mIIpClient.updateLayer2Information(roamingInfo);
    }

    private void doDhcpRoamingTest(final boolean hasMismatchedIpAddress, final String displayName,
            final MacAddress bssid, final boolean expectRoaming) throws Exception {
        long currentTime = System.currentTimeMillis();
        final Layer2Information layer2Info = new Layer2Information(TEST_L2KEY, TEST_CLUSTER, bssid);

        doAnswer(invocation -> {
            // we don't rely on the Init-Reboot state to renew previous cached IP lease.
            // Just return null and force state machine enter INIT state.
            final String l2Key = invocation.getArgument(0);
            ((OnNetworkAttributesRetrievedListener) invocation.getArgument(1))
                    .onNetworkAttributesRetrieved(new Status(SUCCESS), l2Key, null);
            return null;
        }).when(mIpMemoryStore).retrieveNetworkAttributes(eq(TEST_L2KEY), any());

        mDependencies.setHostnameConfiguration(true /* isHostnameConfigurationEnabled */,
                null /* hostname */);
        performDhcpHandshake(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* isDhcpLeaseCacheEnabled */, false /* isDhcpRapidCommitEnabled */,
                TEST_DEFAULT_MTU, false /* isDhcpIpConflictDetectEnabled */,
                false /* isIPv6OnlyPreferredEnabled */,
                null /* captivePortalApiUrl */, displayName, null /* scanResultInfo */,
                layer2Info);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);

        // simulate the roaming by updating bssid.
        forceLayer2Roaming();

        currentTime = System.currentTimeMillis();
        reset(mIpMemoryStore);
        reset(mCb);
        if (!expectRoaming) {
            assertIpMemoryNeverStoreNetworkAttributes();
            return;
        }
        // check DHCPREQUEST broadcast sent to renew IP address.
        DhcpPacket packet;
        packet = getNextDhcpPacket();
        assertTrue(packet instanceof DhcpRequestPacket);
        assertEquals(packet.mClientIp, CLIENT_ADDR);    // client IP
        assertNull(packet.mRequestedIp);                // requested IP option
        assertNull(packet.mServerIdentifier);           // server ID

        mPacketReader.sendResponse(buildDhcpAckPacket(packet,
                hasMismatchedIpAddress ? CLIENT_ADDR_NEW : CLIENT_ADDR, TEST_LEASE_DURATION_S,
                (short) TEST_DEFAULT_MTU, false /* rapidcommit */, null /* captivePortalUrl */));
        HandlerUtils.waitForIdle(mIpc.getHandler(), TEST_TIMEOUT_MS);
        if (hasMismatchedIpAddress) {
            // notifyFailure
            ArgumentCaptor<DhcpResultsParcelable> captor =
                    ArgumentCaptor.forClass(DhcpResultsParcelable.class);
            verify(mCb, timeout(TEST_TIMEOUT_MS)).onNewDhcpResults(captor.capture());
            DhcpResults lease = fromStableParcelable(captor.getValue());
            assertNull(lease);

            // roll back to INIT state.
            packet = getNextDhcpPacket();
            assertTrue(packet instanceof DhcpDiscoverPacket);
        } else {
            assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime,
                    TEST_DEFAULT_MTU);
        }
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpRoaming() throws Exception {
        doDhcpRoamingTest(false /* hasMismatchedIpAddress */, "\"0001docomo\"" /* display name */,
                MacAddress.fromString(TEST_DEFAULT_BSSID), true /* expectRoaming */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpRoaming_invalidBssid() throws Exception {
        doDhcpRoamingTest(false /* hasMismatchedIpAddress */, "\"0001docomo\"" /* display name */,
                MacAddress.fromString(TEST_DHCP_ROAM_BSSID), false /* expectRoaming */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpRoaming_nullBssid() throws Exception {
        doDhcpRoamingTest(false /* hasMismatchedIpAddress */, "\"0001docomo\"" /* display name */,
                null /* BSSID */, false /* expectRoaming */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpRoaming_invalidDisplayName() throws Exception {
        doDhcpRoamingTest(false /* hasMismatchedIpAddress */, "\"test-ssid\"" /* display name */,
                MacAddress.fromString(TEST_DEFAULT_BSSID), false /* expectRoaming */);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDhcpRoaming_mismatchedLeasedIpAddress() throws Exception {
        doDhcpRoamingTest(true /* hasMismatchedIpAddress */, "\"0001docomo\"" /* display name */,
                MacAddress.fromString(TEST_DEFAULT_BSSID), true /* expectRoaming */);
    }

    private void performDualStackProvisioning() throws Exception {
        final InOrder inOrder = inOrder(mCb);
        final CompletableFuture<LinkProperties> lpFuture = new CompletableFuture<>();
        final String dnsServer = "2001:4860:4860::64";
        final ByteBuffer pio = buildPioOption(3600, 1800, "2001:db8:1::/64");
        final ByteBuffer rdnss = buildRdnssOption(3600, dnsServer);
        final ByteBuffer ra = buildRaPacket(pio, rdnss);

        doIpv6OnlyProvisioning(inOrder, ra);

        // Start IPv4 provisioning and wait until entire provisioning completes.
        handleDhcpPackets(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* shouldReplyRapidCommitAck */, TEST_DEFAULT_MTU, null /* serverSentUrl */);
        verify(mCb, timeout(TEST_TIMEOUT_MS).atLeastOnce()).onLinkPropertiesChange(argThat(x -> {
            if (!x.isIpv4Provisioned() || !x.isIpv6Provisioned()) return false;
            lpFuture.complete(x);
            return true;
        }));

        final LinkProperties lp = lpFuture.get(TEST_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        assertNotNull(lp);
        assertTrue(lp.getDnsServers().contains(InetAddress.getByName(dnsServer)));
        assertTrue(lp.getDnsServers().contains(SERVER_ADDR));

        reset(mCb);
    }

    private void doDualStackProvisioning(boolean shouldDisableAcceptRa) throws Exception {
        when(mCm.shouldAvoidBadWifi()).thenReturn(true);

        final ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .build();

        setFeatureEnabled(NetworkStackUtils.IPCLIENT_DISABLE_ACCEPT_RA_VERSION,
                shouldDisableAcceptRa);
        // Enable rapid commit to accelerate DHCP handshake to shorten test duration,
        // not strictly necessary.
        setDhcpFeatures(false /* isDhcpLeaseCacheEnabled */, true /* isRapidCommitEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        mIpc.startProvisioning(config);

        performDualStackProvisioning();
    }

    @Test @SignatureRequiredTest(reason = "signature perms are required due to mocked callabck")
    public void testIgnoreIpv6ProvisioningLoss_disableIPv6Stack() throws Exception {
        doDualStackProvisioning(false /* shouldDisableAcceptRa */);

        final CompletableFuture<LinkProperties> lpFuture = new CompletableFuture<>();

        // Send RA with 0-lifetime and wait until all IPv6-related default route and DNS servers
        // have been removed, then verify if there is IPv4-only info left in the LinkProperties.
        sendRouterAdvertisementWithZeroLifetime();
        verify(mCb, timeout(TEST_TIMEOUT_MS).atLeastOnce()).onLinkPropertiesChange(
                argThat(x -> {
                    final boolean isOnlyIPv4Provisioned = (x.getLinkAddresses().size() == 1
                            && x.getDnsServers().size() == 1
                            && x.getAddresses().get(0) instanceof Inet4Address
                            && x.getDnsServers().get(0) instanceof Inet4Address);

                    if (!isOnlyIPv4Provisioned) return false;
                    lpFuture.complete(x);
                    return true;
                }));
        final LinkProperties lp = lpFuture.get(TEST_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        assertNotNull(lp);
        assertEquals(lp.getAddresses().get(0), CLIENT_ADDR);
        assertEquals(lp.getDnsServers().get(0), SERVER_ADDR);

        final ArgumentCaptor<Integer> quirkEvent = ArgumentCaptor.forClass(Integer.class);
        verify(mNetworkQuirkMetricsDeps, timeout(TEST_TIMEOUT_MS)).writeStats(quirkEvent.capture());
        assertEquals((long) quirkEvent.getValue(),
                (long) NetworkQuirkEvent.QE_IPV6_PROVISIONING_ROUTER_LOST.ordinal());
    }

    @Test @SignatureRequiredTest(reason = "signature perms are required due to mocked callabck")
    public void testIgnoreIpv6ProvisioningLoss_disableAcceptRa() throws Exception {
        doDualStackProvisioning(true /* shouldDisableAcceptRa */);

        final CompletableFuture<LinkProperties> lpFuture = new CompletableFuture<>();

        // Send RA with 0-lifetime and wait until all global IPv6 addresses, IPv6-related default
        // route and DNS servers have been removed, then verify if there is IPv4-only, IPv6 link
        // local address and route to fe80::/64 info left in the LinkProperties.
        sendRouterAdvertisementWithZeroLifetime();
        verify(mCb, timeout(TEST_TIMEOUT_MS).atLeastOnce()).onLinkPropertiesChange(
                argThat(x -> {
                    // Only IPv4 provisioned and IPv6 link-local address
                    final boolean isIPv6LinkLocalAndIPv4OnlyProvisioned =
                            (x.getLinkAddresses().size() == 2
                                    && x.getDnsServers().size() == 1
                                    && x.getAddresses().get(0) instanceof Inet4Address
                                    && x.getDnsServers().get(0) instanceof Inet4Address);

                    if (!isIPv6LinkLocalAndIPv4OnlyProvisioned) return false;
                    lpFuture.complete(x);
                    return true;
                }));
        final LinkProperties lp = lpFuture.get(TEST_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        assertNotNull(lp);
        assertEquals(lp.getAddresses().get(0), CLIENT_ADDR);
        assertEquals(lp.getDnsServers().get(0), SERVER_ADDR);
        assertTrue(lp.getAddresses().get(1).isLinkLocalAddress());

        reset(mCb);

        // Send an RA to verify that global IPv6 addresses won't be configured on the interface.
        sendBasicRouterAdvertisement(false /* waitForRs */);
        verify(mCb, timeout(TEST_TIMEOUT_MS).times(0)).onLinkPropertiesChange(any());
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDualStackProvisioning() throws Exception {
        doDualStackProvisioning(false /* shouldDisableAcceptRa */);

        verify(mCb, never()).onProvisioningFailure(any());
    }

    private DhcpPacket verifyDhcpPacketRequestsIPv6OnlyPreferredOption(
            Class<? extends DhcpPacket> packetType) throws Exception {
        final DhcpPacket packet = getNextDhcpPacket();
        assertTrue(packetType.isInstance(packet));
        assertTrue(packet.hasRequestedParam(DHCP_IPV6_ONLY_PREFERRED));
        return packet;
    }

    private void doIPv6OnlyPreferredOptionTest(final Integer ipv6OnlyWaitTime,
            final Inet4Address clientAddress) throws Exception {
        final ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .build();
        setDhcpFeatures(false /* isDhcpLeaseCacheEnabled */, false /* isRapidCommitEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, true /* isIPv6OnlyPreferredEnabled */);
        startIpClientProvisioning(config);

        final DhcpPacket packet =
                verifyDhcpPacketRequestsIPv6OnlyPreferredOption(DhcpDiscoverPacket.class);

        // Respond DHCPOFFER with IPv6-Only preferred option and offered address.
        mPacketReader.sendResponse(buildDhcpOfferPacket(packet, clientAddress,
                TEST_LEASE_DURATION_S, (short) TEST_DEFAULT_MTU, null /* captivePortalUrl */,
                ipv6OnlyWaitTime));
    }

    private void doDiscoverIPv6OnlyPreferredOptionTest(final int optionSecs,
            final long expectedWaitSecs) throws Exception {
        doIPv6OnlyPreferredOptionTest(optionSecs, CLIENT_ADDR);
        final OnAlarmListener alarm = expectAlarmSet(null /* inOrder */, "TIMEOUT",
                expectedWaitSecs, mDependencies.mDhcpClient.getHandler());
        mDependencies.mDhcpClient.getHandler().post(() -> alarm.onAlarm());
        // Implicitly check that the client never sent a DHCPREQUEST to request the offered address.
        verifyDhcpPacketRequestsIPv6OnlyPreferredOption(DhcpDiscoverPacket.class);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption() throws Exception {
        doDiscoverIPv6OnlyPreferredOptionTest(TEST_IPV6_ONLY_WAIT_S, TEST_IPV6_ONLY_WAIT_S);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption_LowerIPv6OnlyWait() throws Exception {
        doDiscoverIPv6OnlyPreferredOptionTest(TEST_LOWER_IPV6_ONLY_WAIT_S,
                TEST_LOWER_IPV6_ONLY_WAIT_S);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption_ZeroIPv6OnlyWait() throws Exception {
        doDiscoverIPv6OnlyPreferredOptionTest(TEST_ZERO_IPV6_ONLY_WAIT_S,
                TEST_LOWER_IPV6_ONLY_WAIT_S);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption_MaxIPv6OnlyWait() throws Exception {
        doDiscoverIPv6OnlyPreferredOptionTest((int) TEST_MAX_IPV6_ONLY_WAIT_S, 0xffffffffL);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption_ZeroIPv6OnlyWaitWithOfferedAnyAddress()
            throws Exception {
        doIPv6OnlyPreferredOptionTest(TEST_ZERO_IPV6_ONLY_WAIT_S, IPV4_ADDR_ANY);

        final OnAlarmListener alarm = expectAlarmSet(null /* inOrder */, "TIMEOUT", 300,
                mDependencies.mDhcpClient.getHandler());
        mDependencies.mDhcpClient.getHandler().post(() -> alarm.onAlarm());

        verifyDhcpPacketRequestsIPv6OnlyPreferredOption(DhcpDiscoverPacket.class);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption_enabledPreconnection() throws Exception {
        final ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withPreconnection()
                .build();

        setDhcpFeatures(false /* isDhcpLeaseCacheEnabled */, true /* isRapidCommitEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, true /* isIPv6OnlyPreferredEnabled */);
        startIpClientProvisioning(config);

        final DhcpPacket packet = assertDiscoverPacketOnPreconnectionStart();
        verify(mCb).setNeighborDiscoveryOffload(true);

        // Force IpClient transition to RunningState from PreconnectionState.
        mIpc.notifyPreconnectionComplete(true /* success */);
        HandlerUtils.waitForIdle(mDependencies.mDhcpClient.getHandler(), TEST_TIMEOUT_MS);
        verify(mCb, timeout(TEST_TIMEOUT_MS)).setFallbackMulticastFilter(false);

        // DHCP server SHOULD NOT honor the Rapid-Commit option if the response would
        // contain the IPv6-only Preferred option to the client, instead respond with
        // a DHCPOFFER.
        mPacketReader.sendResponse(buildDhcpOfferPacket(packet, CLIENT_ADDR, TEST_LEASE_DURATION_S,
                (short) TEST_DEFAULT_MTU, null /* captivePortalUrl */, TEST_IPV6_ONLY_WAIT_S));

        final OnAlarmListener alarm = expectAlarmSet(null /* inOrder */, "TIMEOUT", 1800,
                mDependencies.mDhcpClient.getHandler());
        mDependencies.mDhcpClient.getHandler().post(() -> alarm.onAlarm());

        verifyDhcpPacketRequestsIPv6OnlyPreferredOption(DhcpDiscoverPacket.class);
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testDiscoverIPv6OnlyPreferredOption_NoIPv6OnlyPreferredOption() throws Exception {
        doIPv6OnlyPreferredOptionTest(null /* ipv6OnlyWaitTime */, CLIENT_ADDR);

        // The IPv6-only Preferred option SHOULD be included in the Parameter Request List option
        // in DHCPREQUEST messages after receiving a DHCPOFFER without this option.
        verifyDhcpPacketRequestsIPv6OnlyPreferredOption(DhcpRequestPacket.class);
    }

    private void startFromInitRebootStateWithIPv6OnlyPreferredOption(final Integer ipv6OnlyWaitTime,
            final long expectedWaitSecs) throws Exception {
        doAnswer(invocation -> {
            ((OnNetworkAttributesRetrievedListener) invocation.getArgument(1))
                    .onNetworkAttributesRetrieved(new Status(SUCCESS), TEST_L2KEY,
                            new NetworkAttributes.Builder()
                                .setAssignedV4Address(CLIENT_ADDR)
                                .setAssignedV4AddressExpiry(Long.MAX_VALUE) // lease is always valid
                                .setMtu(new Integer(TEST_DEFAULT_MTU))
                                .setCluster(TEST_CLUSTER)
                                .setDnsAddresses(Collections.singletonList(SERVER_ADDR))
                                .build());
            return null;
        }).when(mIpMemoryStore).retrieveNetworkAttributes(eq(TEST_L2KEY), any());

        final ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withLayer2Information(new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                          MacAddress.fromString(TEST_DEFAULT_BSSID)))
                .build();

        setDhcpFeatures(true /* isDhcpLeaseCacheEnabled */, false /* isRapidCommitEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, true /* isIPv6OnlyPreferredEnabled */);
        startIpClientProvisioning(config);

        final DhcpPacket packet =
                verifyDhcpPacketRequestsIPv6OnlyPreferredOption(DhcpRequestPacket.class);

        // Respond DHCPACK with IPv6-Only preferred option.
        mPacketReader.sendResponse(buildDhcpAckPacket(packet, CLIENT_ADDR,
                TEST_LEASE_DURATION_S, (short) TEST_DEFAULT_MTU, false /* rapidcommit */,
                null /* captivePortalUrl */, ipv6OnlyWaitTime));

        if (ipv6OnlyWaitTime != null) {
            expectAlarmSet(null /* inOrder */, "TIMEOUT", expectedWaitSecs,
                    mDependencies.mDhcpClient.getHandler());
        }
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRequestIPv6OnlyPreferredOption() throws Exception {
        startFromInitRebootStateWithIPv6OnlyPreferredOption(TEST_IPV6_ONLY_WAIT_S,
                TEST_IPV6_ONLY_WAIT_S);

        // Client transits to IPv6OnlyPreferredState from INIT-REBOOT state when receiving valid
        // IPv6-Only preferred option(default value) in the DHCPACK packet.
        assertIpMemoryNeverStoreNetworkAttributes();
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRequestIPv6OnlyPreferredOption_LowerIPv6OnlyWait() throws Exception {
        startFromInitRebootStateWithIPv6OnlyPreferredOption(TEST_LOWER_IPV6_ONLY_WAIT_S,
                TEST_LOWER_IPV6_ONLY_WAIT_S);

        // Client transits to IPv6OnlyPreferredState from INIT-REBOOT state when receiving valid
        // IPv6-Only preferred option(less than MIN_V6ONLY_WAIT_MS) in the DHCPACK packet.
        assertIpMemoryNeverStoreNetworkAttributes();
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRequestIPv6OnlyPreferredOption_ZeroIPv6OnlyWait() throws Exception {
        startFromInitRebootStateWithIPv6OnlyPreferredOption(TEST_ZERO_IPV6_ONLY_WAIT_S,
                TEST_LOWER_IPV6_ONLY_WAIT_S);

        // Client transits to IPv6OnlyPreferredState from INIT-REBOOT state when receiving valid
        // IPv6-Only preferred option(0) in the DHCPACK packet.
        assertIpMemoryNeverStoreNetworkAttributes();
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRequestIPv6OnlyPreferredOption_MaxIPv6OnlyWait() throws Exception {
        startFromInitRebootStateWithIPv6OnlyPreferredOption((int) TEST_MAX_IPV6_ONLY_WAIT_S,
                0xffffffffL);

        // Client transits to IPv6OnlyPreferredState from INIT-REBOOT state when receiving valid
        // IPv6-Only preferred option(MAX_UNSIGNED_INTEGER: 0xFFFFFFFF) in the DHCPACK packet.
        assertIpMemoryNeverStoreNetworkAttributes();
    }

    @Test @SignatureRequiredTest(reason = "TODO: evaluate whether signature perms are required")
    public void testRequestIPv6OnlyPreferredOption_NoIPv6OnlyPreferredOption() throws Exception {
        final long currentTime = System.currentTimeMillis();
        startFromInitRebootStateWithIPv6OnlyPreferredOption(null /* ipv6OnlyWaitTime */,
                0 /* expectedWaitSecs */);

        // Client processes DHCPACK packet normally and transits to the ConfiguringInterfaceState
        // due to the null V6ONLY_WAIT.
        assertIpMemoryStoreNetworkAttributes(TEST_LEASE_DURATION_S, currentTime, TEST_DEFAULT_MTU);
    }

    private static int getNumOpenFds() {
        return new File("/proc/" + Os.getpid() + "/fd").listFiles().length;
    }

    private void shutdownAndRecreateIpClient() throws Exception {
        mIpc.shutdown();
        awaitIpClientShutdown();
        mIpc = makeIpClient();
    }

    @Test @SignatureRequiredTest(reason = "Only counts FDs from the current process. TODO: fix")
    public void testNoFdLeaks() throws Exception {
        // Shut down and restart IpClient once to ensure that any fds that are opened the first
        // time it runs do not cause the test to fail.
        doDualStackProvisioning(false /* shouldDisableAcceptRa */);
        shutdownAndRecreateIpClient();

        // Unfortunately we cannot use a large number of iterations as it would make the test run
        // too slowly. On crosshatch-eng each iteration takes ~250ms.
        final int iterations = 10;
        final int before = getNumOpenFds();
        for (int i = 0; i < iterations; i++) {
            doDualStackProvisioning(false /* shouldDisableAcceptRa */);
            shutdownAndRecreateIpClient();
            // The last time this loop runs, mIpc will be shut down in tearDown.
        }
        final int after = getNumOpenFds();

        // Check that the number of open fds is the same as before.
        // If this exact match becomes flaky, we could add some tolerance here (e.g., allow 2-3
        // extra fds), since it's likely that any leak would at least leak one FD per loop.
        assertEquals("Fd leak after " + iterations + " iterations: ", before, after);
    }

    // TODO: delete when DhcpOption is @JavaOnlyImmutable.
    private static DhcpOption makeDhcpOption(final byte type, final byte[] value) {
        final DhcpOption opt = new DhcpOption();
        opt.type = type;
        opt.value = value;
        return opt;
    }

    private static final List<DhcpOption> TEST_OEM_DHCP_OPTIONS = Arrays.asList(
            // DHCP_USER_CLASS
            makeDhcpOption((byte) 77, TEST_OEM_USER_CLASS_INFO),
            // DHCP_VENDOR_CLASS_ID
            makeDhcpOption((byte) 60, TEST_OEM_VENDOR_ID.getBytes())
    );

    private DhcpPacket doCustomizedDhcpOptionsTest(final List<DhcpOption> options,
             final ScanResultInfo info) throws Exception {
        ProvisioningConfiguration.Builder prov = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withLayer2Information(new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                        MacAddress.fromString(TEST_DEFAULT_BSSID)))
                .withScanResultInfo(info)
                .withDhcpOptions(options)
                .withoutIPv6();

        setDhcpFeatures(false /* isDhcpLeaseCacheEnabled */, false /* isRapidCommitEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);

        startIpClientProvisioning(prov.build());
        verify(mCb, timeout(TEST_TIMEOUT_MS)).setFallbackMulticastFilter(false);
        verify(mCb, never()).onProvisioningFailure(any());

        return getNextDhcpPacket();
    }

    @Test
    public void testCustomizedDhcpOptions() throws Exception {
        final ScanResultInfo info = makeScanResultInfo(0xdd /* vendor-specificIE */, TEST_OEM_OUI,
                (byte) 0x17 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(TEST_OEM_DHCP_OPTIONS, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, TEST_OEM_VENDOR_ID);
        assertArrayEquals(packet.mUserClass, TEST_OEM_USER_CLASS_INFO);
    }

    @Test
    public void testCustomizedDhcpOptions_nullDhcpOptions() throws Exception {
        final ScanResultInfo info = makeScanResultInfo(0xdd /* vendor-specificIE */, TEST_OEM_OUI,
                (byte) 0x17 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(null /* options */, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, new String("android-dhcp-" + Build.VERSION.RELEASE));
        assertNull(packet.mUserClass);
    }

    @Test
    public void testCustomizedDhcpOptions_nullScanResultInfo() throws Exception {
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(TEST_OEM_DHCP_OPTIONS,
                null /* scanResultInfo */);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, new String("android-dhcp-" + Build.VERSION.RELEASE));
        assertNull(packet.mUserClass);
    }

    @Test
    public void testCustomizedDhcpOptions_disallowedOui() throws Exception {
        final ScanResultInfo info = makeScanResultInfo(0xdd /* vendor-specificIE */,
                new byte[]{ 0x00, 0x11, 0x22} /* oui */, (byte) 0x17 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(TEST_OEM_DHCP_OPTIONS, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, new String("android-dhcp-" + Build.VERSION.RELEASE));
        assertNull(packet.mUserClass);
    }

    @Test
    public void testCustomizedDhcpOptions_invalidIeId() throws Exception {
        final ScanResultInfo info = makeScanResultInfo(0xde /* vendor-specificIE */, TEST_OEM_OUI,
                (byte) 0x17 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(TEST_OEM_DHCP_OPTIONS, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, new String("android-dhcp-" + Build.VERSION.RELEASE));
        assertNull(packet.mUserClass);
    }

    @Test
    public void testCustomizedDhcpOptions_invalidVendorSpecificType() throws Exception {
        final ScanResultInfo info = makeScanResultInfo(0xdd /* vendor-specificIE */, TEST_OEM_OUI,
                (byte) 0x10 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(TEST_OEM_DHCP_OPTIONS, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, new String("android-dhcp-" + Build.VERSION.RELEASE));
        assertNull(packet.mUserClass);
    }

    @Test
    public void testCustomizedDhcpOptions_disallowedOption() throws Exception {
        final List<DhcpOption> options = Arrays.asList(
                makeDhcpOption((byte) 60, TEST_OEM_VENDOR_ID.getBytes()),
                makeDhcpOption((byte) 77, TEST_OEM_USER_CLASS_INFO),
                // Option 26: MTU
                makeDhcpOption((byte) 26, HexDump.toByteArray(TEST_DEFAULT_MTU)));
        final ScanResultInfo info = makeScanResultInfo(0xdd /* vendor-specificIE */, TEST_OEM_OUI,
                (byte) 0x17 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(options, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, TEST_OEM_VENDOR_ID);
        assertArrayEquals(packet.mUserClass, TEST_OEM_USER_CLASS_INFO);
        assertNull(packet.mMtu);
    }

    @Test
    public void testCustomizedDhcpOptions_disallowedParamRequestOption() throws Exception {
        final List<DhcpOption> options = Arrays.asList(
                makeDhcpOption((byte) 60, TEST_OEM_VENDOR_ID.getBytes()),
                makeDhcpOption((byte) 77, TEST_OEM_USER_CLASS_INFO),
                // NTP_SERVER
                makeDhcpOption((byte) 42, null));
        final ScanResultInfo info = makeScanResultInfo(0xdd /* vendor-specificIE */, TEST_OEM_OUI,
                (byte) 0x17 /* vendor-specific IE type */);
        final DhcpPacket packet = doCustomizedDhcpOptionsTest(options, info);

        assertTrue(packet instanceof DhcpDiscoverPacket);
        assertEquals(packet.mVendorId, TEST_OEM_VENDOR_ID);
        assertArrayEquals(packet.mUserClass, TEST_OEM_USER_CLASS_INFO);
        assertFalse(packet.hasRequestedParam((byte) 42 /* NTP_SERVER */));
    }

    private void assertGratuitousNa(final NeighborAdvertisement na) throws Exception {
        final MacAddress etherMulticast =
                NetworkStackUtils.ipv6MulticastToEthernetMulticast(IPV6_ADDR_ALL_ROUTERS_MULTICAST);
        final LinkAddress target = new LinkAddress(na.naHdr.target, 64);

        assertEquals(etherMulticast, na.ethHdr.dstMac);
        assertEquals(ETH_P_IPV6, na.ethHdr.etherType);
        assertEquals(IPPROTO_ICMPV6, na.ipv6Hdr.nextHeader);
        assertEquals(0xff, na.ipv6Hdr.hopLimit);
        assertTrue(na.ipv6Hdr.srcIp.isLinkLocalAddress());
        assertEquals(IPV6_ADDR_ALL_ROUTERS_MULTICAST, na.ipv6Hdr.dstIp);
        assertEquals(ICMPV6_NEIGHBOR_ADVERTISEMENT, na.icmpv6Hdr.type);
        assertEquals(0, na.icmpv6Hdr.code);
        assertEquals(0, na.naHdr.flags);
        assertTrue(target.isGlobalPreferred());
    }

    @Test
    public void testGratuitousNaForNewGlobalUnicastAddresses() throws Exception {
        final ProvisioningConfiguration config = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withoutIPv4()
                .build();

        setFeatureEnabled(NetworkStackUtils.IPCLIENT_GRATUITOUS_NA_VERSION,
                true /* isGratuitousNaEnabled */);
        assertTrue(isFeatureEnabled(NetworkStackUtils.IPCLIENT_GRATUITOUS_NA_VERSION, false));
        startIpClientProvisioning(config);

        doIpv6OnlyProvisioning();

        final List<NeighborAdvertisement> naList = new ArrayList<>();
        NeighborAdvertisement packet;
        while ((packet = getNextNeighborAdvertisement()) != null) {
            assertGratuitousNa(packet);
            naList.add(packet);
        }
        assertEquals(2, naList.size()); // privacy address and stable privacy address
    }

    private void startGratuitousArpAndNaAfterRoamingTest(boolean isGratuitousArpNaRoamingEnabled,
            boolean hasIpv4, boolean hasIpv6) throws Exception {
        final Layer2Information layer2Info = new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                MacAddress.fromString(TEST_DEFAULT_BSSID));
        final ScanResultInfo scanResultInfo =
                makeScanResultInfo(TEST_DEFAULT_SSID, TEST_DEFAULT_BSSID);
        final ProvisioningConfiguration.Builder prov = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withLayer2Information(layer2Info)
                .withScanResultInfo(scanResultInfo)
                .withDisplayName("ssid");
        if (!hasIpv4) prov.withoutIPv4();
        if (!hasIpv6) prov.withoutIPv6();

        // Enable rapid commit to accelerate DHCP handshake to shorten test duration,
        // not strictly necessary.
        setDhcpFeatures(false /* isDhcpLeaseCacheEnabled */, true /* isRapidCommitEnabled */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        if (isGratuitousArpNaRoamingEnabled) {
            setFeatureEnabled(NetworkStackUtils.IPCLIENT_GARP_NA_ROAMING_VERSION, true);
            assertTrue(isFeatureEnabled(NetworkStackUtils.IPCLIENT_GARP_NA_ROAMING_VERSION, false));
        }
        startIpClientProvisioning(prov.build());
    }

    private void waitForGratuitousArpAndNaPacket(final List<ArpPacket> arpList,
            final List<NeighborAdvertisement> naList) throws Exception {
        NeighborAdvertisement na;
        ArpPacket garp;
        do {
            na = getNextNeighborAdvertisement();
            if (na != null) {
                assertGratuitousNa(na);
                naList.add(na);
            }
            garp = getNextArpPacket(TEST_TIMEOUT_MS);
            if (garp != null) {
                assertGratuitousARP(garp);
                arpList.add(garp);
            }
        } while (na != null || garp != null);
    }

    @Test
    public void testGratuitousArpAndNaAfterRoaming() throws Exception {
        startGratuitousArpAndNaAfterRoamingTest(true /* isGratuitousArpNaRoamingEnabled */,
                true /* hasIpv4 */, true /* hasIpv6 */);
        performDualStackProvisioning();
        forceLayer2Roaming();

        final List<ArpPacket> arpList = new ArrayList<>();
        final List<NeighborAdvertisement> naList = new ArrayList<>();
        waitForGratuitousArpAndNaPacket(arpList, naList);
        assertEquals(2, naList.size()); // privacy address and stable privacy address
        assertEquals(1, arpList.size()); // IPv4 address
    }

    @Test
    public void testGratuitousArpAndNaAfterRoaming_disableExpFlag() throws Exception {
        startGratuitousArpAndNaAfterRoamingTest(false /* isGratuitousArpNaRoamingEnabled */,
                true /* hasIpv6 */, true /* hasIpv6 */);
        performDualStackProvisioning();
        forceLayer2Roaming();

        final List<ArpPacket> arpList = new ArrayList<>();
        final List<NeighborAdvertisement> naList = new ArrayList<>();
        waitForGratuitousArpAndNaPacket(arpList, naList);
        assertEquals(0, naList.size());
        assertEquals(0, arpList.size());
    }

    @Test
    public void testGratuitousArpAndNaAfterRoaming_IPv6OnlyNetwork() throws Exception {
        startGratuitousArpAndNaAfterRoamingTest(true /* isGratuitousArpNaRoamingEnabled */,
                false /* hasIpv4 */, true /* hasIpv6 */);
        doIpv6OnlyProvisioning();
        forceLayer2Roaming();

        final List<ArpPacket> arpList = new ArrayList<>();
        final List<NeighborAdvertisement> naList = new ArrayList<>();
        waitForGratuitousArpAndNaPacket(arpList, naList);
        assertEquals(2, naList.size());
        assertEquals(0, arpList.size());
    }

    @Test
    public void testGratuitousArpAndNaAfterRoaming_IPv4OnlyNetwork() throws Exception {
        startGratuitousArpAndNaAfterRoamingTest(true /* isGratuitousArpNaRoamingEnabled */,
                true /* hasIpv4 */, false /* hasIpv6 */);

        // Start IPv4 provisioning and wait until entire provisioning completes.
        handleDhcpPackets(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* shouldReplyRapidCommitAck */, TEST_DEFAULT_MTU, null /* serverSentUrl */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        forceLayer2Roaming();

        final List<ArpPacket> arpList = new ArrayList<>();
        final List<NeighborAdvertisement> naList = new ArrayList<>();
        waitForGratuitousArpAndNaPacket(arpList, naList);
        assertEquals(0, naList.size());
        assertEquals(1, arpList.size());
    }

    private void doInitialBssidSetupTest(final Layer2Information layer2Info,
            final ScanResultInfo scanResultInfo) throws Exception {
        ProvisioningConfiguration.Builder prov = new ProvisioningConfiguration.Builder()
                .withoutIpReachabilityMonitor()
                .withLayer2Information(layer2Info)
                .withScanResultInfo(scanResultInfo)
                .withDisplayName("\"0001docomo\"")
                .withoutIPv6();

        setDhcpFeatures(false /* isDhcpLeaseCacheEnabled */, true /* shouldReplyRapidCommitAck */,
                false /* isDhcpIpConflictDetectEnabled */, false /* isIPv6OnlyPreferredEnabled */);
        startIpClientProvisioning(prov.build());

        handleDhcpPackets(true /* isSuccessLease */, TEST_LEASE_DURATION_S,
                true /* shouldReplyRapidCommitAck */, TEST_DEFAULT_MTU, null /* serverSentUrl */);
        verifyIPv4OnlyProvisioningSuccess(Collections.singletonList(CLIENT_ADDR));
        forceLayer2Roaming();
    }

    @Test @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromLayer2Info() throws Exception {
        final Layer2Information layer2Info = new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                MacAddress.fromString(TEST_DEFAULT_BSSID));

        doInitialBssidSetupTest(layer2Info, null /* scanResultInfo */);

        // Initial BSSID comes from layer2Info, it's different with target roaming bssid,
        // then verify that DHCPREQUEST packet is sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket();
        assertTrue(packet instanceof DhcpRequestPacket);
    }

    @Test @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromLayer2Info_NullBssid() throws Exception {
        final Layer2Information layer2Info = new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                null /* bssid */);
        final ScanResultInfo scanResultInfo =
                makeScanResultInfo(TEST_DEFAULT_SSID, TEST_DHCP_ROAM_BSSID);

        doInitialBssidSetupTest(layer2Info, scanResultInfo);

        // Initial BSSID comes from layer2Info, it's null, no DHCPREQUEST packet
        // will be sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket(TEST_TIMEOUT_MS);
        assertNull(packet);
    }

    @Test @IgnoreUpTo(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromLayer2Info_SameRoamingBssid() throws Exception {
        final Layer2Information layer2Info = new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                MacAddress.fromString(TEST_DHCP_ROAM_BSSID));

        doInitialBssidSetupTest(layer2Info, null /* scanResultInfo */);

        // Initial BSSID comes from layer2Info, it's same with target roaming bssid,
        // no DHCPREQUEST packet will be sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket(TEST_TIMEOUT_MS);
        assertNull(packet);
    }

    @Test @IgnoreAfter(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromScanResultInfo() throws Exception {
        final ScanResultInfo scanResultInfo =
                makeScanResultInfo(TEST_DEFAULT_SSID, TEST_DEFAULT_BSSID);

        doInitialBssidSetupTest(null /* layer2Info */, scanResultInfo);

        // Initial BSSID comes from ScanResultInfo, it's different with target roaming bssid,
        // then verify that DHCPREQUEST packet is sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket();
        assertTrue(packet instanceof DhcpRequestPacket);
    }

    @Test @IgnoreAfter(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromScanResultInfo_SameRoamingBssid() throws Exception {
        final ScanResultInfo scanResultInfo =
                makeScanResultInfo(TEST_DEFAULT_SSID, TEST_DHCP_ROAM_BSSID);

        doInitialBssidSetupTest(null /* layer2Info */, scanResultInfo);

        // Initial BSSID comes from ScanResultInfo, it's same with target roaming bssid,
        // no DHCPREQUEST packet will be sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket(TEST_TIMEOUT_MS);
        assertNull(packet);
    }

    @Test @IgnoreAfter(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromScanResultInfo_BrokenInitialBssid() throws Exception {
        final ScanResultInfo scanResultInfo =
                makeScanResultInfo(TEST_DEFAULT_SSID, "00:11:22:33:44:");

        doInitialBssidSetupTest(null /* layer2Info */, scanResultInfo);

        // Initial BSSID comes from ScanResultInfo, it's broken MAC address format and fallback
        // to null layer2Info, no DHCPREQUEST packet will be sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket(TEST_TIMEOUT_MS);
        assertNull(packet);
    }

    @Test @IgnoreAfter(Build.VERSION_CODES.R)
    public void testSetInitialBssidFromScanResultInfo_BrokenInitialBssidFallback()
            throws Exception {
        final Layer2Information layer2Info = new Layer2Information(TEST_L2KEY, TEST_CLUSTER,
                MacAddress.fromString(TEST_DEFAULT_BSSID));
        final ScanResultInfo scanResultInfo =
                makeScanResultInfo(TEST_DEFAULT_SSID, "00:11:22:33:44:");

        doInitialBssidSetupTest(layer2Info, scanResultInfo);

        // Initial BSSID comes from ScanResultInfo, it's broken MAC address format and fallback
        // to check layer2Info, then verify DHCPREQUEST packet will be sent after roaming.
        final DhcpPacket packet = getNextDhcpPacket();
        assertTrue(packet instanceof DhcpRequestPacket);
    }
}
