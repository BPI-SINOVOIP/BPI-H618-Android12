/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static android.Manifest.permission.NETWORK_SETTINGS;
import static android.content.pm.PackageManager.GET_ACTIVITIES;
import static android.hardware.usb.UsbManager.USB_CONFIGURED;
import static android.hardware.usb.UsbManager.USB_CONNECTED;
import static android.hardware.usb.UsbManager.USB_FUNCTION_NCM;
import static android.hardware.usb.UsbManager.USB_FUNCTION_RNDIS;
import static android.net.ConnectivityManager.ACTION_RESTRICT_BACKGROUND_CHANGED;
import static android.net.ConnectivityManager.RESTRICT_BACKGROUND_STATUS_DISABLED;
import static android.net.ConnectivityManager.RESTRICT_BACKGROUND_STATUS_ENABLED;
import static android.net.ConnectivityManager.TYPE_MOBILE_DUN;
import static android.net.ConnectivityManager.TYPE_WIFI;
import static android.net.NetworkCapabilities.NET_CAPABILITY_DUN;
import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.NetworkCapabilities.TRANSPORT_BLUETOOTH;
import static android.net.NetworkCapabilities.TRANSPORT_CELLULAR;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;
import static android.net.RouteInfo.RTN_UNICAST;
import static android.net.TetheringManager.ACTION_TETHER_STATE_CHANGED;
import static android.net.TetheringManager.CONNECTIVITY_SCOPE_GLOBAL;
import static android.net.TetheringManager.EXTRA_ACTIVE_LOCAL_ONLY;
import static android.net.TetheringManager.EXTRA_ACTIVE_TETHER;
import static android.net.TetheringManager.EXTRA_AVAILABLE_TETHER;
import static android.net.TetheringManager.TETHERING_BLUETOOTH;
import static android.net.TetheringManager.TETHERING_ETHERNET;
import static android.net.TetheringManager.TETHERING_NCM;
import static android.net.TetheringManager.TETHERING_USB;
import static android.net.TetheringManager.TETHERING_WIFI;
import static android.net.TetheringManager.TETHERING_WIFI_P2P;
import static android.net.TetheringManager.TETHER_ERROR_IFACE_CFG_ERROR;
import static android.net.TetheringManager.TETHER_ERROR_NO_ERROR;
import static android.net.TetheringManager.TETHER_ERROR_SERVICE_UNAVAIL;
import static android.net.TetheringManager.TETHER_ERROR_UNKNOWN_IFACE;
import static android.net.TetheringManager.TETHER_HARDWARE_OFFLOAD_FAILED;
import static android.net.TetheringManager.TETHER_HARDWARE_OFFLOAD_STARTED;
import static android.net.TetheringManager.TETHER_HARDWARE_OFFLOAD_STOPPED;
import static android.net.dhcp.IDhcpServer.STATUS_SUCCESS;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_INTERFACE_NAME;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_MODE;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_STATE;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_LOCAL_ONLY;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_TETHERED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLED;
import static android.system.OsConstants.RT_SCOPE_UNIVERSE;
import static android.telephony.SubscriptionManager.INVALID_SUBSCRIPTION_ID;

import static com.android.modules.utils.build.SdkLevel.isAtLeastS;
import static com.android.net.module.util.Inet4AddressUtils.inet4AddressToIntHTH;
import static com.android.net.module.util.Inet4AddressUtils.intToInet4AddressHTH;
import static com.android.networkstack.tethering.OffloadHardwareInterface.OFFLOAD_HAL_VERSION_1_0;
import static com.android.networkstack.tethering.OffloadHardwareInterface.OFFLOAD_HAL_VERSION_NONE;
import static com.android.networkstack.tethering.TestConnectivityManager.BROADCAST_FIRST;
import static com.android.networkstack.tethering.TestConnectivityManager.CALLBACKS_FIRST;
import static com.android.networkstack.tethering.Tethering.UserRestrictionActionListener;
import static com.android.networkstack.tethering.TetheringConfiguration.TETHER_FORCE_USB_FUNCTIONS;
import static com.android.networkstack.tethering.TetheringConfiguration.TETHER_USB_NCM_FUNCTION;
import static com.android.networkstack.tethering.TetheringConfiguration.TETHER_USB_RNDIS_FUNCTION;
import static com.android.networkstack.tethering.TetheringNotificationUpdater.DOWNSTREAM_NONE;
import static com.android.networkstack.tethering.UpstreamNetworkMonitor.EVENT_ON_CAPABILITIES;
import static com.android.testutils.TestPermissionUtil.runAsShell;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.app.usage.NetworkStatsManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothPan;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothProfile.ServiceListener;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.hardware.usb.UsbManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.EthernetManager;
import android.net.EthernetManager.TetheredInterfaceCallback;
import android.net.EthernetManager.TetheredInterfaceRequest;
import android.net.IConnectivityManager;
import android.net.IIntResultListener;
import android.net.INetd;
import android.net.ITetheringEventCallback;
import android.net.InetAddresses;
import android.net.InterfaceConfigurationParcel;
import android.net.IpPrefix;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.MacAddress;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.RouteInfo;
import android.net.TetherStatesParcel;
import android.net.TetheredClient;
import android.net.TetheredClient.AddressInfo;
import android.net.TetheringCallbackStartedParcel;
import android.net.TetheringConfigurationParcel;
import android.net.TetheringInterface;
import android.net.TetheringRequestParcel;
import android.net.dhcp.DhcpLeaseParcelable;
import android.net.dhcp.DhcpServerCallbacks;
import android.net.dhcp.DhcpServingParamsParcel;
import android.net.dhcp.IDhcpEventCallbacks;
import android.net.dhcp.IDhcpServer;
import android.net.ip.DadProxy;
import android.net.ip.IpNeighborMonitor;
import android.net.ip.IpServer;
import android.net.ip.RouterAdvertisementDaemon;
import android.net.util.InterfaceParams;
import android.net.util.NetworkConstants;
import android.net.util.SharedLog;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.WifiClient;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.SoftApCallback;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.PersistableBundle;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.test.TestLooper;
import android.provider.Settings;
import android.telephony.CarrierConfigManager;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.test.mock.MockContentResolver;

import androidx.annotation.NonNull;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.internal.util.ArrayUtils;
import com.android.internal.util.StateMachine;
import com.android.internal.util.test.BroadcastInterceptingContext;
import com.android.internal.util.test.FakeSettingsProvider;
import com.android.networkstack.tethering.TestConnectivityManager.TestNetworkAgent;
import com.android.testutils.MiscAsserts;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Vector;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class TetheringTest {
    private static final int IFINDEX_OFFSET = 100;

    private static final String TEST_MOBILE_IFNAME = "test_rmnet_data0";
    private static final String TEST_DUN_IFNAME = "test_dun0";
    private static final String TEST_XLAT_MOBILE_IFNAME = "v4-test_rmnet_data0";
    private static final String TEST_RNDIS_IFNAME = "test_rndis0";
    private static final String TEST_WIFI_IFNAME = "test_wlan0";
    private static final String TEST_WLAN_IFNAME = "test_wlan1";
    private static final String TEST_P2P_IFNAME = "test_p2p-p2p0-0";
    private static final String TEST_NCM_IFNAME = "test_ncm0";
    private static final String TEST_ETH_IFNAME = "test_eth0";
    private static final String TEST_BT_IFNAME = "test_pan0";
    private static final String TETHERING_NAME = "Tethering";
    private static final String[] PROVISIONING_APP_NAME = {"some", "app"};
    private static final String PROVISIONING_NO_UI_APP_NAME = "no_ui_app";
    private static final String TEST_RNDIS_REGEX = "test_rndis\\d";
    private static final String TEST_NCM_REGEX = "test_ncm\\d";
    private static final String TEST_WIFI_REGEX = "test_wlan\\d";
    private static final String TEST_P2P_REGEX = "test_p2p-p2p\\d-.*";
    private static final String TEST_BT_REGEX = "test_pan\\d";

    private static final int CELLULAR_NETID = 100;
    private static final int WIFI_NETID = 101;
    private static final int DUN_NETID = 102;

    private static final int TETHER_USB_RNDIS_NCM_FUNCTIONS = 2;

    private static final int DHCPSERVER_START_TIMEOUT_MS = 1000;

    @Mock private ApplicationInfo mApplicationInfo;
    @Mock private Context mContext;
    @Mock private NetworkStatsManager mStatsManager;
    @Mock private OffloadHardwareInterface mOffloadHardwareInterface;
    @Mock private OffloadHardwareInterface.ForwardedStats mForwardedStats;
    @Mock private Resources mResources;
    @Mock private TelephonyManager mTelephonyManager;
    @Mock private UsbManager mUsbManager;
    @Mock private WifiManager mWifiManager;
    @Mock private CarrierConfigManager mCarrierConfigManager;
    @Mock private IPv6TetheringCoordinator mIPv6TetheringCoordinator;
    @Mock private DadProxy mDadProxy;
    @Mock private RouterAdvertisementDaemon mRouterAdvertisementDaemon;
    @Mock private IpNeighborMonitor mIpNeighborMonitor;
    @Mock private IDhcpServer mDhcpServer;
    @Mock private INetd mNetd;
    @Mock private UserManager mUserManager;
    @Mock private EthernetManager mEm;
    @Mock private TetheringNotificationUpdater mNotificationUpdater;
    @Mock private BpfCoordinator mBpfCoordinator;
    @Mock private PackageManager mPackageManager;
    @Mock private BluetoothAdapter mBluetoothAdapter;
    @Mock private BluetoothPan mBluetoothPan;

    private final MockIpServerDependencies mIpServerDependencies =
            spy(new MockIpServerDependencies());
    private final MockTetheringDependencies mTetheringDependencies =
            new MockTetheringDependencies();

    // Like so many Android system APIs, these cannot be mocked because it is marked final.
    // We have to use the real versions.
    private final PersistableBundle mCarrierConfig = new PersistableBundle();
    private final TestLooper mLooper = new TestLooper();

    private Vector<Intent> mIntents;
    private BroadcastInterceptingContext mServiceContext;
    private MockContentResolver mContentResolver;
    private BroadcastReceiver mBroadcastReceiver;
    private Tethering mTethering;
    private PhoneStateListener mPhoneStateListener;
    private InterfaceConfigurationParcel mInterfaceConfiguration;
    private TetheringConfiguration mConfig;
    private EntitlementManager mEntitleMgr;
    private OffloadController mOffloadCtrl;
    private PrivateAddressCoordinator mPrivateAddressCoordinator;
    private SoftApCallback mSoftApCallback;
    private UpstreamNetworkMonitor mUpstreamNetworkMonitor;

    private TestConnectivityManager mCm;

    private class TestContext extends BroadcastInterceptingContext {
        TestContext(Context base) {
            super(base);
        }

        @Override
        public ApplicationInfo getApplicationInfo() {
            return mApplicationInfo;
        }

        @Override
        public ContentResolver getContentResolver() {
            return mContentResolver;
        }

        @Override
        public String getPackageName() {
            return "TetheringTest";
        }

        @Override
        public Resources getResources() {
            return mResources;
        }

        @Override
        public Object getSystemService(String name) {
            if (Context.WIFI_SERVICE.equals(name)) return mWifiManager;
            if (Context.USB_SERVICE.equals(name)) return mUsbManager;
            if (Context.TELEPHONY_SERVICE.equals(name)) return mTelephonyManager;
            if (Context.USER_SERVICE.equals(name)) return mUserManager;
            if (Context.NETWORK_STATS_SERVICE.equals(name)) return mStatsManager;
            if (Context.CONNECTIVITY_SERVICE.equals(name)) return mCm;
            if (Context.ETHERNET_SERVICE.equals(name)) return mEm;
            return super.getSystemService(name);
        }

        @Override
        public PackageManager getPackageManager() {
            return mPackageManager;
        }

        @Override
        public String getSystemServiceName(Class<?> serviceClass) {
            if (TelephonyManager.class.equals(serviceClass)) return Context.TELEPHONY_SERVICE;
            return super.getSystemServiceName(serviceClass);
        }
    }

    public class MockIpServerDependencies extends IpServer.Dependencies {
        @Override
        public DadProxy getDadProxy(
                Handler handler, InterfaceParams ifParams) {
            return mDadProxy;
        }

        @Override
        public RouterAdvertisementDaemon getRouterAdvertisementDaemon(
                InterfaceParams ifParams) {
            return mRouterAdvertisementDaemon;
        }

        @Override
        public InterfaceParams getInterfaceParams(String ifName) {
            assertTrue("Non-mocked interface " + ifName,
                    ifName.equals(TEST_RNDIS_IFNAME)
                            || ifName.equals(TEST_WLAN_IFNAME)
                            || ifName.equals(TEST_WIFI_IFNAME)
                            || ifName.equals(TEST_MOBILE_IFNAME)
                            || ifName.equals(TEST_DUN_IFNAME)
                            || ifName.equals(TEST_P2P_IFNAME)
                            || ifName.equals(TEST_NCM_IFNAME)
                            || ifName.equals(TEST_ETH_IFNAME)
                            || ifName.equals(TEST_BT_IFNAME));
            final String[] ifaces = new String[] {
                    TEST_RNDIS_IFNAME, TEST_WLAN_IFNAME, TEST_WIFI_IFNAME, TEST_MOBILE_IFNAME,
                    TEST_DUN_IFNAME, TEST_P2P_IFNAME, TEST_NCM_IFNAME, TEST_ETH_IFNAME};
            return new InterfaceParams(ifName, ArrayUtils.indexOf(ifaces, ifName) + IFINDEX_OFFSET,
                    MacAddress.ALL_ZEROS_ADDRESS);
        }

        @Override
        public void makeDhcpServer(String ifName, DhcpServingParamsParcel params,
                DhcpServerCallbacks cb) {
            new Thread(() -> {
                try {
                    cb.onDhcpServerCreated(STATUS_SUCCESS, mDhcpServer);
                } catch (RemoteException e) {
                    fail(e.getMessage());
                }
            }).run();
        }

        public IpNeighborMonitor getIpNeighborMonitor(Handler h, SharedLog l,
                IpNeighborMonitor.NeighborEventConsumer c) {
            return mIpNeighborMonitor;
        }
    }

    public class MockTetheringDependencies extends TetheringDependencies {
        StateMachine mUpstreamNetworkMonitorSM;
        ArrayList<IpServer> mIpv6CoordinatorNotifyList;

        @Override
        public BpfCoordinator getBpfCoordinator(
                BpfCoordinator.Dependencies deps) {
            return mBpfCoordinator;
        }

        @Override
        public OffloadHardwareInterface getOffloadHardwareInterface(Handler h, SharedLog log) {
            return mOffloadHardwareInterface;
        }

        @Override
        public OffloadController getOffloadController(Handler h, SharedLog log,
                OffloadController.Dependencies deps) {
            mOffloadCtrl = spy(super.getOffloadController(h, log, deps));
            // Return real object here instead of mock because
            // testReportFailCallbackIfOffloadNotSupported depend on real OffloadController object.
            return mOffloadCtrl;
        }

        @Override
        public UpstreamNetworkMonitor getUpstreamNetworkMonitor(Context ctx,
                StateMachine target, SharedLog log, int what) {
            // Use a real object instead of a mock so that some tests can use a real UNM and some
            // can use a mock.
            mUpstreamNetworkMonitorSM = target;
            mUpstreamNetworkMonitor = spy(super.getUpstreamNetworkMonitor(ctx, target, log, what));
            return mUpstreamNetworkMonitor;
        }

        @Override
        public IPv6TetheringCoordinator getIPv6TetheringCoordinator(
                ArrayList<IpServer> notifyList, SharedLog log) {
            mIpv6CoordinatorNotifyList = notifyList;
            return mIPv6TetheringCoordinator;
        }

        @Override
        public IpServer.Dependencies getIpServerDependencies() {
            return mIpServerDependencies;
        }

        @Override
        public EntitlementManager getEntitlementManager(Context ctx, Handler h, SharedLog log,
                Runnable callback) {
            mEntitleMgr = spy(super.getEntitlementManager(ctx, h, log, callback));
            return mEntitleMgr;
        }

        @Override
        public boolean isTetheringSupported() {
            return true;
        }

        @Override
        public TetheringConfiguration generateTetheringConfiguration(Context ctx, SharedLog log,
                int subId) {
            mConfig = spy(new FakeTetheringConfiguration(ctx, log, subId));
            return mConfig;
        }

        @Override
        public INetd getINetd(Context context) {
            return mNetd;
        }

        @Override
        public Looper getTetheringLooper() {
            return mLooper.getLooper();
        }

        @Override
        public Context getContext() {
            return mServiceContext;
        }

        @Override
        public BluetoothAdapter getBluetoothAdapter() {
            return mBluetoothAdapter;
        }

        @Override
        public TetheringNotificationUpdater getNotificationUpdater(Context ctx, Looper looper) {
            return mNotificationUpdater;
        }

        @Override
        public boolean isTetheringDenied() {
            return false;
        }


        @Override
        public PrivateAddressCoordinator getPrivateAddressCoordinator(Context ctx,
                TetheringConfiguration cfg) {
            mPrivateAddressCoordinator = super.getPrivateAddressCoordinator(ctx, cfg);
            return mPrivateAddressCoordinator;
        }
    }

    private static LinkProperties buildUpstreamLinkProperties(String interfaceName,
            boolean withIPv4, boolean withIPv6, boolean with464xlat) {
        final LinkProperties prop = new LinkProperties();
        prop.setInterfaceName(interfaceName);

        if (withIPv4) {
            prop.addLinkAddress(new LinkAddress("10.1.2.3/15"));
            prop.addRoute(new RouteInfo(new IpPrefix(Inet4Address.ANY, 0),
                    InetAddresses.parseNumericAddress("10.0.0.1"),
                    interfaceName, RTN_UNICAST));
        }

        if (withIPv6) {
            prop.addDnsServer(InetAddresses.parseNumericAddress("2001:db8::2"));
            prop.addLinkAddress(
                    new LinkAddress(InetAddresses.parseNumericAddress("2001:db8::"),
                            NetworkConstants.RFC7421_PREFIX_LENGTH));
            prop.addRoute(new RouteInfo(new IpPrefix(Inet6Address.ANY, 0),
                    InetAddresses.parseNumericAddress("2001:db8::1"),
                    interfaceName, RTN_UNICAST));
        }

        if (with464xlat) {
            final String clatInterface = "v4-" + interfaceName;
            final LinkProperties stackedLink = new LinkProperties();
            stackedLink.setInterfaceName(clatInterface);
            stackedLink.addRoute(new RouteInfo(new IpPrefix(Inet4Address.ANY, 0),
                    InetAddresses.parseNumericAddress("192.0.0.1"),
                    clatInterface, RTN_UNICAST));

            prop.addStackedLink(stackedLink);
        }

        return prop;
    }

    private static NetworkCapabilities buildUpstreamCapabilities(int transport, int... otherCaps) {
        // TODO: add NOT_VCN_MANAGED.
        final NetworkCapabilities nc = new NetworkCapabilities()
                .addTransportType(transport);
        for (int cap : otherCaps) {
            nc.addCapability(cap);
        }
        return nc;
    }

    private static UpstreamNetworkState buildMobileUpstreamState(boolean withIPv4,
            boolean withIPv6, boolean with464xlat) {
        return new UpstreamNetworkState(
                buildUpstreamLinkProperties(TEST_MOBILE_IFNAME, withIPv4, withIPv6, with464xlat),
                buildUpstreamCapabilities(TRANSPORT_CELLULAR, NET_CAPABILITY_INTERNET),
                new Network(CELLULAR_NETID));
    }

    private static UpstreamNetworkState buildMobileIPv4UpstreamState() {
        return buildMobileUpstreamState(true, false, false);
    }

    private static UpstreamNetworkState buildMobileIPv6UpstreamState() {
        return buildMobileUpstreamState(false, true, false);
    }

    private static UpstreamNetworkState buildMobileDualStackUpstreamState() {
        return buildMobileUpstreamState(true, true, false);
    }

    private static UpstreamNetworkState buildMobile464xlatUpstreamState() {
        return buildMobileUpstreamState(false, true, true);
    }

    private static UpstreamNetworkState buildWifiUpstreamState() {
        return new UpstreamNetworkState(
                buildUpstreamLinkProperties(TEST_WIFI_IFNAME, true /* IPv4 */, true /* IPv6 */,
                        false /* 464xlat */),
                buildUpstreamCapabilities(TRANSPORT_WIFI, NET_CAPABILITY_INTERNET),
                new Network(WIFI_NETID));
    }

    private static UpstreamNetworkState buildDunUpstreamState() {
        return new UpstreamNetworkState(
                buildUpstreamLinkProperties(TEST_DUN_IFNAME, true /* IPv4 */, true /* IPv6 */,
                        false /* 464xlat */),
                buildUpstreamCapabilities(TRANSPORT_CELLULAR, NET_CAPABILITY_DUN),
                new Network(DUN_NETID));
    }

    // See FakeSettingsProvider#clearSettingsProvider() that this also needs to be called before
    // use.
    @BeforeClass
    public static void setupOnce() {
        FakeSettingsProvider.clearSettingsProvider();
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        when(mResources.getStringArray(R.array.config_tether_dhcp_range))
                .thenReturn(new String[0]);
        when(mResources.getBoolean(R.bool.config_tether_enable_legacy_dhcp_server)).thenReturn(
                false);
        when(mNetd.interfaceGetList())
                .thenReturn(new String[] {
                        TEST_MOBILE_IFNAME, TEST_WLAN_IFNAME, TEST_RNDIS_IFNAME, TEST_P2P_IFNAME,
                        TEST_NCM_IFNAME, TEST_ETH_IFNAME, TEST_BT_IFNAME});
        when(mResources.getString(R.string.config_wifi_tether_enable)).thenReturn("");
        mInterfaceConfiguration = new InterfaceConfigurationParcel();
        mInterfaceConfiguration.flags = new String[0];
        when(mRouterAdvertisementDaemon.start())
                .thenReturn(true);
        initOffloadConfiguration(true /* offloadConfig */, OFFLOAD_HAL_VERSION_1_0,
                0 /* defaultDisabled */);
        when(mOffloadHardwareInterface.getForwardedStats(any())).thenReturn(mForwardedStats);

        mServiceContext = new TestContext(mContext);
        mServiceContext.setUseRegisteredHandlers(true);
        mContentResolver = new MockContentResolver(mServiceContext);
        mContentResolver.addProvider(Settings.AUTHORITY, new FakeSettingsProvider());
        setTetheringSupported(true /* supported */);
        mIntents = new Vector<>();
        mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                mIntents.addElement(intent);
            }
        };
        mServiceContext.registerReceiver(mBroadcastReceiver,
                new IntentFilter(ACTION_TETHER_STATE_CHANGED));

        mCm = spy(new TestConnectivityManager(mServiceContext, mock(IConnectivityManager.class)));

        mTethering = makeTethering();
        verify(mStatsManager, times(1)).registerNetworkStatsProvider(anyString(), any());
        verify(mNetd).registerUnsolicitedEventListener(any());
        verifyDefaultNetworkRequestFiled();

        final ArgumentCaptor<PhoneStateListener> phoneListenerCaptor =
                ArgumentCaptor.forClass(PhoneStateListener.class);
        verify(mTelephonyManager).listen(phoneListenerCaptor.capture(),
                eq(PhoneStateListener.LISTEN_ACTIVE_DATA_SUBSCRIPTION_ID_CHANGE));
        mPhoneStateListener = phoneListenerCaptor.getValue();

        final ArgumentCaptor<SoftApCallback> softApCallbackCaptor =
                ArgumentCaptor.forClass(SoftApCallback.class);
        verify(mWifiManager).registerSoftApCallback(any(), softApCallbackCaptor.capture());
        mSoftApCallback = softApCallbackCaptor.getValue();

        when(mPackageManager.hasSystemFeature(PackageManager.FEATURE_WIFI)).thenReturn(true);
        when(mPackageManager.hasSystemFeature(PackageManager.FEATURE_WIFI_DIRECT)).thenReturn(true);
    }

    private void setTetheringSupported(final boolean supported) {
        Settings.Global.putInt(mContentResolver, Settings.Global.TETHER_SUPPORTED,
                supported ? 1 : 0);
        when(mUserManager.hasUserRestriction(
                UserManager.DISALLOW_CONFIG_TETHERING)).thenReturn(!supported);
        when(mResources.getInteger(R.integer.config_tether_usb_functions)).thenReturn(
                TetheringConfiguration.TETHER_USB_RNDIS_FUNCTION);
        // Setup tetherable configuration.
        when(mResources.getStringArray(R.array.config_tether_usb_regexs))
                .thenReturn(new String[] {TEST_RNDIS_REGEX});
        when(mResources.getStringArray(R.array.config_tether_wifi_regexs))
                .thenReturn(new String[] {TEST_WIFI_REGEX});
        when(mResources.getStringArray(R.array.config_tether_wifi_p2p_regexs))
                .thenReturn(new String[] {TEST_P2P_REGEX});
        when(mResources.getStringArray(R.array.config_tether_bluetooth_regexs))
                .thenReturn(new String[] {TEST_BT_REGEX});
        when(mResources.getStringArray(R.array.config_tether_ncm_regexs))
                .thenReturn(new String[] {TEST_NCM_REGEX});
        when(mResources.getIntArray(R.array.config_tether_upstream_types)).thenReturn(
                new int[] {TYPE_WIFI, TYPE_MOBILE_DUN});
        when(mResources.getBoolean(R.bool.config_tether_upstream_automatic)).thenReturn(true);
    }

    private void initTetheringUpstream(UpstreamNetworkState upstreamState) {
        doReturn(upstreamState).when(mUpstreamNetworkMonitor).getCurrentPreferredUpstream();
        doReturn(upstreamState).when(mUpstreamNetworkMonitor).selectPreferredUpstreamType(any());
    }

    private Tethering makeTethering() {
        return new Tethering(mTetheringDependencies);
    }

    private TetheringRequestParcel createTetheringRequestParcel(final int type) {
        return createTetheringRequestParcel(type, null, null, false, CONNECTIVITY_SCOPE_GLOBAL);
    }

    private TetheringRequestParcel createTetheringRequestParcel(final int type,
            final LinkAddress serverAddr, final LinkAddress clientAddr, final boolean exempt,
            final int scope) {
        final TetheringRequestParcel request = new TetheringRequestParcel();
        request.tetheringType = type;
        request.localIPv4Address = serverAddr;
        request.staticClientAddress = clientAddr;
        request.exemptFromEntitlementCheck = exempt;
        request.showProvisioningUi = false;
        request.connectivityScope = scope;

        return request;
    }

    @After
    public void tearDown() {
        mServiceContext.unregisterReceiver(mBroadcastReceiver);
        FakeSettingsProvider.clearSettingsProvider();
    }

    private void sendWifiApStateChanged(int state) {
        final Intent intent = new Intent(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        intent.putExtra(EXTRA_WIFI_AP_STATE, state);
        mServiceContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
        mLooper.dispatchAll();
    }

    private void sendWifiApStateChanged(int state, String ifname, int ipmode) {
        final Intent intent = new Intent(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        intent.putExtra(EXTRA_WIFI_AP_STATE, state);
        intent.putExtra(EXTRA_WIFI_AP_INTERFACE_NAME, ifname);
        intent.putExtra(EXTRA_WIFI_AP_MODE, ipmode);
        mServiceContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
        mLooper.dispatchAll();
    }

    private static final String[] P2P_RECEIVER_PERMISSIONS_FOR_BROADCAST = {
            android.Manifest.permission.ACCESS_FINE_LOCATION,
            android.Manifest.permission.ACCESS_WIFI_STATE
    };

    private void sendWifiP2pConnectionChanged(
            boolean isGroupFormed, boolean isGroupOwner, String ifname) {
        WifiP2pGroup group = null;
        WifiP2pInfo p2pInfo = new WifiP2pInfo();
        p2pInfo.groupFormed = isGroupFormed;
        if (isGroupFormed) {
            p2pInfo.isGroupOwner = isGroupOwner;
            group = mock(WifiP2pGroup.class);
            when(group.isGroupOwner()).thenReturn(isGroupOwner);
            when(group.getInterface()).thenReturn(ifname);
        }

        final Intent intent = mock(Intent.class);
        when(intent.getAction()).thenReturn(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        when(intent.getParcelableExtra(WifiP2pManager.EXTRA_WIFI_P2P_INFO)).thenReturn(p2pInfo);
        when(intent.getParcelableExtra(WifiP2pManager.EXTRA_WIFI_P2P_GROUP)).thenReturn(group);

        mServiceContext.sendBroadcastAsUserMultiplePermissions(intent, UserHandle.ALL,
                P2P_RECEIVER_PERMISSIONS_FOR_BROADCAST);
        mLooper.dispatchAll();
    }

    // enableType:
    // No function enabled            = -1
    // TETHER_USB_RNDIS_FUNCTION      = 0
    // TETHER_USB_NCM_FUNCTIONS       = 1
    // TETHER_USB_RNDIS_NCM_FUNCTIONS = 2
    private boolean tetherUsbFunctionMatches(int function, int enabledType) {
        if (enabledType < 0) return false;

        if (enabledType == TETHER_USB_RNDIS_NCM_FUNCTIONS) return function < enabledType;

        return function == enabledType;
    }

    private void sendUsbBroadcast(boolean connected, boolean configured, int function) {
        final Intent intent = new Intent(UsbManager.ACTION_USB_STATE);
        intent.putExtra(USB_CONNECTED, connected);
        intent.putExtra(USB_CONFIGURED, configured);
        intent.putExtra(USB_FUNCTION_RNDIS,
                tetherUsbFunctionMatches(TETHER_USB_RNDIS_FUNCTION, function));
        intent.putExtra(USB_FUNCTION_NCM,
                tetherUsbFunctionMatches(TETHER_USB_NCM_FUNCTION, function));
        mServiceContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
        mLooper.dispatchAll();
    }

    private void sendConfigurationChanged() {
        final Intent intent = new Intent(Intent.ACTION_CONFIGURATION_CHANGED);
        mServiceContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
        mLooper.dispatchAll();
    }

    private void verifyDefaultNetworkRequestFiled() {
        if (isAtLeastS()) {
            verify(mCm, times(1)).registerSystemDefaultNetworkCallback(
                    any(NetworkCallback.class), any(Handler.class));
        } else {
            ArgumentCaptor<NetworkRequest> reqCaptor = ArgumentCaptor.forClass(
                    NetworkRequest.class);
            verify(mCm, times(1)).requestNetwork(reqCaptor.capture(),
                    any(NetworkCallback.class), any(Handler.class));
            assertTrue(TestConnectivityManager.looksLikeDefaultRequest(reqCaptor.getValue()));
        }

        // The default network request is only ever filed once.
        verifyNoMoreInteractions(mCm);
    }

    private void verifyInterfaceServingModeStarted(String ifname) throws Exception {
        verify(mNetd, times(1)).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, times(1)).tetherInterfaceAdd(ifname);
        verify(mNetd, times(1)).networkAddInterface(INetd.LOCAL_NET_ID, ifname);
        verify(mNetd, times(2)).networkAddRoute(eq(INetd.LOCAL_NET_ID), eq(ifname),
                anyString(), anyString());
    }

    private void verifyTetheringBroadcast(String ifname, String whichExtra) {
        // Verify that ifname is in the whichExtra array of the tether state changed broadcast.
        final Intent bcast = mIntents.get(0);
        assertEquals(ACTION_TETHER_STATE_CHANGED, bcast.getAction());
        final ArrayList<String> ifnames = bcast.getStringArrayListExtra(whichExtra);
        assertTrue(ifnames.contains(ifname));
        mIntents.remove(bcast);
    }

    public void failingLocalOnlyHotspotLegacyApBroadcast(
            boolean emulateInterfaceStatusChanged) throws Exception {
        // Emulate externally-visible WifiManager effects, causing the
        // per-interface state machine to start up, and telling us that
        // hotspot mode is to be started.
        if (emulateInterfaceStatusChanged) {
            mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        }
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED);

        // If, and only if, Tethering received an interface status changed then
        // it creates a IpServer and sends out a broadcast indicating that the
        // interface is "available".
        if (emulateInterfaceStatusChanged) {
            // There is 1 IpServer state change event: STATE_AVAILABLE
            verify(mNotificationUpdater, times(1)).onDownstreamChanged(DOWNSTREAM_NONE);
            verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_AVAILABLE_TETHER);
            verify(mWifiManager).updateInterfaceIpState(
                    TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        }
        verifyNoMoreInteractions(mNetd);
        verifyNoMoreInteractions(mWifiManager);
    }

    private void prepareNcmTethering() {
        // Emulate startTethering(TETHERING_NCM) called
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_NCM), null);
        mLooper.dispatchAll();
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_NCM);
    }

    private void prepareUsbTethering() {
        // Emulate pressing the USB tethering button in Settings UI.
        final TetheringRequestParcel request = createTetheringRequestParcel(TETHERING_USB);
        mTethering.startTethering(request, null);
        mLooper.dispatchAll();

        assertEquals(1, mTethering.getActiveTetheringRequests().size());
        assertEquals(request, mTethering.getActiveTetheringRequests().get(TETHERING_USB));

        if (mTethering.getTetheringConfiguration().isUsingNcm()) {
            verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_NCM);
            mTethering.interfaceStatusChanged(TEST_NCM_IFNAME, true);
        } else {
            verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_RNDIS);
            mTethering.interfaceStatusChanged(TEST_RNDIS_IFNAME, true);
        }

    }

    @Test
    public void testUsbConfiguredBroadcastStartsTethering() throws Exception {
        UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        initTetheringUpstream(upstreamState);
        prepareUsbTethering();

        // This should produce no activity of any kind.
        verifyNoMoreInteractions(mNetd);

        // Pretend we then receive USB configured broadcast.
        sendUsbBroadcast(true, true, TETHER_USB_RNDIS_FUNCTION);
        // Now we should see the start of tethering mechanics (in this case:
        // tetherMatchingInterfaces() which starts by fetching all interfaces).
        verify(mNetd, times(1)).interfaceGetList();

        // UpstreamNetworkMonitor should receive selected upstream
        verify(mUpstreamNetworkMonitor, times(1)).getCurrentPreferredUpstream();
        verify(mUpstreamNetworkMonitor, times(1)).setCurrentUpstream(upstreamState.network);
    }

    @Test
    public void failingLocalOnlyHotspotLegacyApBroadcastWithIfaceStatusChanged() throws Exception {
        failingLocalOnlyHotspotLegacyApBroadcast(true);
    }

    @Test
    public void failingLocalOnlyHotspotLegacyApBroadcastSansIfaceStatusChanged() throws Exception {
        failingLocalOnlyHotspotLegacyApBroadcast(false);
    }

    public void workingLocalOnlyHotspotEnrichedApBroadcast(
            boolean emulateInterfaceStatusChanged) throws Exception {
        // Emulate externally-visible WifiManager effects, causing the
        // per-interface state machine to start up, and telling us that
        // hotspot mode is to be started.
        if (emulateInterfaceStatusChanged) {
            mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        }
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED, TEST_WLAN_IFNAME, IFACE_IP_MODE_LOCAL_ONLY);

        verifyInterfaceServingModeStarted(TEST_WLAN_IFNAME);
        verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_AVAILABLE_TETHER);
        verify(mNetd, times(1)).ipfwdEnableForwarding(TETHERING_NAME);
        verify(mNetd, times(1)).tetherStartWithConfiguration(any());
        verifyNoMoreInteractions(mNetd);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_LOCAL_ONLY);
        verifyNoMoreInteractions(mWifiManager);
        verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_ACTIVE_LOCAL_ONLY);
        verify(mUpstreamNetworkMonitor, times(1)).startObserveAllNetworks();
        // There are 2 IpServer state change events: STATE_AVAILABLE -> STATE_LOCAL_ONLY
        verify(mNotificationUpdater, times(2)).onDownstreamChanged(DOWNSTREAM_NONE);

        // Emulate externally-visible WifiManager effects, when hotspot mode
        // is being torn down.
        sendWifiApStateChanged(WifiManager.WIFI_AP_STATE_DISABLED);
        mTethering.interfaceRemoved(TEST_WLAN_IFNAME);
        mLooper.dispatchAll();

        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
        verify(mNetd, times(1)).tetherInterfaceRemove(TEST_WLAN_IFNAME);
        verify(mNetd, times(1)).networkRemoveInterface(INetd.LOCAL_NET_ID, TEST_WLAN_IFNAME);
        // interfaceSetCfg() called once for enabling and twice disabling IPv4.
        verify(mNetd, times(3)).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, times(1)).tetherStop();
        verify(mNetd, times(1)).ipfwdDisableForwarding(TETHERING_NAME);
        verify(mWifiManager, times(3)).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        verifyNoMoreInteractions(mNetd);
        verifyNoMoreInteractions(mWifiManager);
        // Asking for the last error after the per-interface state machine
        // has been reaped yields an unknown interface error.
        assertEquals(TETHER_ERROR_UNKNOWN_IFACE, mTethering.getLastErrorForTest(TEST_WLAN_IFNAME));
    }

    /**
     * Send CMD_IPV6_TETHER_UPDATE to IpServers as would be done by IPv6TetheringCoordinator.
     */
    private void sendIPv6TetherUpdates(UpstreamNetworkState upstreamState) {
        // IPv6TetheringCoordinator must have been notified of downstream
        for (IpServer ipSrv : mTetheringDependencies.mIpv6CoordinatorNotifyList) {
            UpstreamNetworkState ipv6OnlyState = buildMobileUpstreamState(false, true, false);
            ipSrv.sendMessage(IpServer.CMD_IPV6_TETHER_UPDATE, 0, 0,
                    upstreamState.linkProperties.isIpv6Provisioned()
                            ? ipv6OnlyState.linkProperties
                            : null);
            break;
        }
        mLooper.dispatchAll();
    }

    private void runUsbTethering(UpstreamNetworkState upstreamState) {
        initTetheringUpstream(upstreamState);
        prepareUsbTethering();
        if (mTethering.getTetheringConfiguration().isUsingNcm()) {
            sendUsbBroadcast(true, true, TETHER_USB_NCM_FUNCTION);
            verify(mIPv6TetheringCoordinator).addActiveDownstream(
                    argThat(sm -> sm.linkProperties().getInterfaceName().equals(TEST_NCM_IFNAME)),
                    eq(IpServer.STATE_TETHERED));
        } else {
            sendUsbBroadcast(true, true, TETHER_USB_RNDIS_FUNCTION);
            verify(mIPv6TetheringCoordinator).addActiveDownstream(
                    argThat(sm -> sm.linkProperties().getInterfaceName().equals(TEST_RNDIS_IFNAME)),
                    eq(IpServer.STATE_TETHERED));
        }

    }

    private void assertSetIfaceToDadProxy(final int numOfCalls, final String ifaceName) {
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.R || "S".equals(Build.VERSION.CODENAME)
                    || "T".equals(Build.VERSION.CODENAME)) {
            verify(mDadProxy, times(numOfCalls)).setUpstreamIface(
                     argThat(ifaceParams  -> ifaceName.equals(ifaceParams.name)));
        }
    }

    @Test
    public void workingMobileUsbTethering_IPv4() throws Exception {
        UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        runUsbTethering(upstreamState);

        verify(mNetd, times(1)).tetherAddForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);

        sendIPv6TetherUpdates(upstreamState);
        assertSetIfaceToDadProxy(0 /* numOfCalls */, "" /* ifaceName */);
        verify(mRouterAdvertisementDaemon, never()).buildNewRa(any(), notNull());
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
    }

    @Test
    public void workingMobileUsbTethering_IPv4LegacyDhcp() {
        when(mResources.getBoolean(R.bool.config_tether_enable_legacy_dhcp_server)).thenReturn(
                true);
        sendConfigurationChanged();
        final UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        runUsbTethering(upstreamState);
        sendIPv6TetherUpdates(upstreamState);

        verify(mIpServerDependencies, never()).makeDhcpServer(any(), any(), any());
    }

    @Test
    public void workingMobileUsbTethering_IPv6() throws Exception {
        UpstreamNetworkState upstreamState = buildMobileIPv6UpstreamState();
        runUsbTethering(upstreamState);

        verify(mNetd, times(1)).tetherAddForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);

        sendIPv6TetherUpdates(upstreamState);
        // TODO: add interfaceParams to compare in verify.
        assertSetIfaceToDadProxy(1 /* numOfCalls */, TEST_MOBILE_IFNAME /* ifaceName */);
        verify(mRouterAdvertisementDaemon, times(1)).buildNewRa(any(), notNull());
        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
    }

    @Test
    public void workingMobileUsbTethering_DualStack() throws Exception {
        UpstreamNetworkState upstreamState = buildMobileDualStackUpstreamState();
        runUsbTethering(upstreamState);

        verify(mNetd, times(1)).tetherAddForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);
        verify(mRouterAdvertisementDaemon, times(1)).start();
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());

        sendIPv6TetherUpdates(upstreamState);
        assertSetIfaceToDadProxy(1 /* numOfCalls */, TEST_MOBILE_IFNAME /* ifaceName */);
        verify(mRouterAdvertisementDaemon, times(1)).buildNewRa(any(), notNull());
        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
    }

    @Test
    public void workingMobileUsbTethering_MultipleUpstreams() throws Exception {
        UpstreamNetworkState upstreamState = buildMobile464xlatUpstreamState();
        runUsbTethering(upstreamState);

        verify(mNetd, times(1)).tetherAddForward(TEST_RNDIS_IFNAME, TEST_XLAT_MOBILE_IFNAME);
        verify(mNetd, times(1)).tetherAddForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_RNDIS_IFNAME,
                TEST_XLAT_MOBILE_IFNAME);
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_RNDIS_IFNAME, TEST_MOBILE_IFNAME);

        sendIPv6TetherUpdates(upstreamState);
        assertSetIfaceToDadProxy(1 /* numOfCalls */, TEST_MOBILE_IFNAME /* ifaceName */);
        verify(mRouterAdvertisementDaemon, times(1)).buildNewRa(any(), notNull());
        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
    }

    @Test
    public void workingMobileUsbTethering_v6Then464xlat() throws Exception {
        when(mResources.getInteger(R.integer.config_tether_usb_functions)).thenReturn(
                TetheringConfiguration.TETHER_USB_NCM_FUNCTION);
        when(mResources.getStringArray(R.array.config_tether_usb_regexs))
                .thenReturn(new String[] {TEST_NCM_REGEX});
        sendConfigurationChanged();

        // Setup IPv6
        UpstreamNetworkState upstreamState = buildMobileIPv6UpstreamState();
        runUsbTethering(upstreamState);

        verify(mNetd, times(1)).tetherAddForward(TEST_NCM_IFNAME, TEST_MOBILE_IFNAME);
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_NCM_IFNAME, TEST_MOBILE_IFNAME);

        // Then 464xlat comes up
        upstreamState = buildMobile464xlatUpstreamState();
        initTetheringUpstream(upstreamState);

        // Upstream LinkProperties changed: UpstreamNetworkMonitor sends EVENT_ON_LINKPROPERTIES.
        mTetheringDependencies.mUpstreamNetworkMonitorSM.sendMessage(
                Tethering.TetherMainSM.EVENT_UPSTREAM_CALLBACK,
                UpstreamNetworkMonitor.EVENT_ON_LINKPROPERTIES,
                0,
                upstreamState);
        mLooper.dispatchAll();

        // Forwarding is added for 464xlat
        verify(mNetd, times(1)).tetherAddForward(TEST_NCM_IFNAME, TEST_XLAT_MOBILE_IFNAME);
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_NCM_IFNAME,
                TEST_XLAT_MOBILE_IFNAME);
        // Forwarding was not re-added for v6 (still times(1))
        verify(mNetd, times(1)).tetherAddForward(TEST_NCM_IFNAME, TEST_MOBILE_IFNAME);
        verify(mNetd, times(1)).ipfwdAddInterfaceForward(TEST_NCM_IFNAME, TEST_MOBILE_IFNAME);
        // DHCP not restarted on downstream (still times(1))
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
    }

    @Test
    public void configTetherUpstreamAutomaticIgnoresConfigTetherUpstreamTypes() throws Exception {
        when(mResources.getBoolean(R.bool.config_tether_upstream_automatic)).thenReturn(true);
        sendConfigurationChanged();

        // Setup IPv6
        final UpstreamNetworkState upstreamState = buildMobileIPv6UpstreamState();
        runUsbTethering(upstreamState);

        // UpstreamNetworkMonitor should choose upstream automatically
        // (in this specific case: choose the default network).
        verify(mUpstreamNetworkMonitor, times(1)).getCurrentPreferredUpstream();
        verify(mUpstreamNetworkMonitor, never()).selectPreferredUpstreamType(any());

        verify(mUpstreamNetworkMonitor, times(1)).setCurrentUpstream(upstreamState.network);
    }

    private void verifyDisableTryCellWhenTetheringStop(InOrder inOrder) {
        runStopUSBTethering();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
    }

    private void upstreamSelectionTestCommon(final boolean automatic, InOrder inOrder,
            TestNetworkAgent mobile, TestNetworkAgent wifi) throws Exception {
        // Enable automatic upstream selection.
        when(mResources.getBoolean(R.bool.config_tether_upstream_automatic)).thenReturn(automatic);
        sendConfigurationChanged();
        mLooper.dispatchAll();

        // Start USB tethering with no current upstream.
        prepareUsbTethering();
        sendUsbBroadcast(true, true, TETHER_USB_RNDIS_FUNCTION);
        inOrder.verify(mUpstreamNetworkMonitor).startObserveAllNetworks();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(true);

        // Pretend cellular connected and expect the upstream to be set.
        mobile.fakeConnect();
        mCm.makeDefaultNetwork(mobile, BROADCAST_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        // Switch upstream to wifi.
        wifi.fakeConnect();
        mCm.makeDefaultNetwork(wifi, BROADCAST_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(wifi.networkId);
    }

    @Test
    public void testAutomaticUpstreamSelection() throws Exception {
        TestNetworkAgent mobile = new TestNetworkAgent(mCm, buildMobileDualStackUpstreamState());
        TestNetworkAgent wifi = new TestNetworkAgent(mCm, buildWifiUpstreamState());
        InOrder inOrder = inOrder(mCm, mUpstreamNetworkMonitor);
        // Enable automatic upstream selection.
        upstreamSelectionTestCommon(true, inOrder, mobile, wifi);

        // This code has historically been racy, so test different orderings of CONNECTIVITY_ACTION
        // broadcasts and callbacks, and add mLooper.dispatchAll() calls between the two.
        final Runnable doDispatchAll = () -> mLooper.dispatchAll();

        // Switch upstreams a few times.
        mCm.makeDefaultNetwork(mobile, BROADCAST_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        mCm.makeDefaultNetwork(wifi, BROADCAST_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(wifi.networkId);

        mCm.makeDefaultNetwork(mobile, CALLBACKS_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        mCm.makeDefaultNetwork(wifi, CALLBACKS_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(wifi.networkId);

        mCm.makeDefaultNetwork(mobile, CALLBACKS_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        // Wifi disconnecting should not have any affect since it's not the current upstream.
        wifi.fakeDisconnect();
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor, never()).setCurrentUpstream(any());

        // Lose and regain upstream.
        assertTrue(mUpstreamNetworkMonitor.getCurrentPreferredUpstream().linkProperties
                .hasIPv4Address());
        mCm.makeDefaultNetwork(null, BROADCAST_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        mobile.fakeDisconnect();
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(true);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(null);

        mobile = new TestNetworkAgent(mCm, buildMobile464xlatUpstreamState());
        mobile.fakeConnect();
        mCm.makeDefaultNetwork(mobile, BROADCAST_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        // Check the IP addresses to ensure that the upstream is indeed not the same as the previous
        // mobile upstream, even though the netId is (unrealistically) the same.
        assertFalse(mUpstreamNetworkMonitor.getCurrentPreferredUpstream().linkProperties
                .hasIPv4Address());

        // Lose and regain upstream again.
        mCm.makeDefaultNetwork(null, CALLBACKS_FIRST, doDispatchAll);
        mobile.fakeDisconnect();
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(true);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(null);

        mobile = new TestNetworkAgent(mCm, buildMobileDualStackUpstreamState());
        mobile.fakeConnect();
        mCm.makeDefaultNetwork(mobile, CALLBACKS_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        assertTrue(mUpstreamNetworkMonitor.getCurrentPreferredUpstream().linkProperties
                .hasIPv4Address());

        // Check that the code does not crash if onLinkPropertiesChanged is received after onLost.
        mobile.fakeDisconnect();
        mobile.sendLinkProperties();
        mLooper.dispatchAll();

        verifyDisableTryCellWhenTetheringStop(inOrder);
    }

    @Test
    public void testLegacyUpstreamSelection() throws Exception {
        TestNetworkAgent mobile = new TestNetworkAgent(mCm, buildMobileDualStackUpstreamState());
        TestNetworkAgent wifi = new TestNetworkAgent(mCm, buildWifiUpstreamState());
        InOrder inOrder = inOrder(mCm, mUpstreamNetworkMonitor);
        // Enable legacy upstream selection.
        upstreamSelectionTestCommon(false, inOrder, mobile, wifi);

        // Wifi disconnecting and the default network switch to mobile, the upstream should also
        // switch to mobile.
        wifi.fakeDisconnect();
        mLooper.dispatchAll();
        mCm.makeDefaultNetwork(mobile, CALLBACKS_FIRST, null);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(mobile.networkId);

        wifi.fakeConnect();
        mLooper.dispatchAll();
        mCm.makeDefaultNetwork(wifi, CALLBACKS_FIRST, null);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(wifi.networkId);

        verifyDisableTryCellWhenTetheringStop(inOrder);
    }

    @Test
    public void testChooseDunUpstreamByAutomaticMode() throws Exception {
        // Enable automatic upstream selection.
        TestNetworkAgent mobile = new TestNetworkAgent(mCm, buildMobileDualStackUpstreamState());
        TestNetworkAgent wifi = new TestNetworkAgent(mCm, buildWifiUpstreamState());
        TestNetworkAgent dun = new TestNetworkAgent(mCm, buildDunUpstreamState());
        InOrder inOrder = inOrder(mCm, mUpstreamNetworkMonitor);
        chooseDunUpstreamTestCommon(true, inOrder, mobile, wifi, dun);

        // When default network switch to mobile and wifi is connected (may have low signal),
        // automatic mode would request dun again and choose it as upstream.
        mCm.makeDefaultNetwork(mobile, CALLBACKS_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(true);
        ArgumentCaptor<NetworkCallback> captor = ArgumentCaptor.forClass(NetworkCallback.class);
        inOrder.verify(mCm).requestNetwork(any(), eq(0), eq(TYPE_MOBILE_DUN), any(), any());
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(null);
        final Runnable doDispatchAll = () -> mLooper.dispatchAll();
        dun.fakeConnect(CALLBACKS_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(dun.networkId);

        // Lose and regain upstream again.
        dun.fakeDisconnect(CALLBACKS_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(true);
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(null);
        inOrder.verify(mCm, never()).unregisterNetworkCallback(any(NetworkCallback.class));
        dun.fakeConnect(CALLBACKS_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(dun.networkId);

        verifyDisableTryCellWhenTetheringStop(inOrder);
    }

    @Test
    public void testChooseDunUpstreamByLegacyMode() throws Exception {
        // Enable Legacy upstream selection.
        TestNetworkAgent mobile = new TestNetworkAgent(mCm, buildMobileDualStackUpstreamState());
        TestNetworkAgent wifi = new TestNetworkAgent(mCm, buildWifiUpstreamState());
        TestNetworkAgent dun = new TestNetworkAgent(mCm, buildDunUpstreamState());
        InOrder inOrder = inOrder(mCm, mUpstreamNetworkMonitor);
        chooseDunUpstreamTestCommon(false, inOrder, mobile, wifi, dun);

        // Legacy mode would keep use wifi as upstream (because it has higher priority in the
        // list).
        mCm.makeDefaultNetwork(mobile, CALLBACKS_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mUpstreamNetworkMonitor, never()).setCurrentUpstream(any());
        // BUG: when wifi disconnect, the dun request would not be filed again because wifi is
        // no longer be default network which do not have CONNECTIVIY_ACTION broadcast.
        wifi.fakeDisconnect();
        mLooper.dispatchAll();
        inOrder.verify(mCm, never()).requestNetwork(any(), eq(0), eq(TYPE_MOBILE_DUN), any(),
                any());

        // Change the legacy priority list that dun is higher than wifi.
        when(mResources.getIntArray(R.array.config_tether_upstream_types)).thenReturn(
                new int[] { TYPE_MOBILE_DUN, TYPE_WIFI });
        sendConfigurationChanged();
        mLooper.dispatchAll();

        // Make wifi as default network. Note: mobile also connected.
        wifi.fakeConnect();
        mLooper.dispatchAll();
        mCm.makeDefaultNetwork(wifi, CALLBACKS_FIRST);
        mLooper.dispatchAll();
        // BUG: dun has higher priority than wifi but tethering don't file dun request because
        // current upstream is wifi.
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mCm, never()).requestNetwork(any(), eq(0), eq(TYPE_MOBILE_DUN), any(),
                any());

        verifyDisableTryCellWhenTetheringStop(inOrder);
    }

    private void chooseDunUpstreamTestCommon(final boolean automatic, InOrder inOrder,
            TestNetworkAgent mobile, TestNetworkAgent wifi, TestNetworkAgent dun) throws Exception {
        when(mResources.getBoolean(R.bool.config_tether_upstream_automatic)).thenReturn(automatic);
        when(mTelephonyManager.isTetheringApnRequired()).thenReturn(true);
        sendConfigurationChanged();
        mLooper.dispatchAll();

        // Start USB tethering with no current upstream.
        prepareUsbTethering();
        sendUsbBroadcast(true, true, TETHER_USB_RNDIS_FUNCTION);
        inOrder.verify(mUpstreamNetworkMonitor).startObserveAllNetworks();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(true);
        ArgumentCaptor<NetworkCallback> captor = ArgumentCaptor.forClass(NetworkCallback.class);
        inOrder.verify(mCm).requestNetwork(any(), eq(0), eq(TYPE_MOBILE_DUN), any(),
                captor.capture());
        final NetworkCallback dunNetworkCallback1 = captor.getValue();

        // Pretend cellular connected and expect the upstream to be set.
        mobile.fakeConnect();
        mCm.makeDefaultNetwork(mobile, BROADCAST_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor, never()).setCurrentUpstream(mobile.networkId);

        // Pretend dun connected and expect choose dun as upstream.
        final Runnable doDispatchAll = () -> mLooper.dispatchAll();
        dun.fakeConnect(BROADCAST_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(dun.networkId);

        // When wifi connected, unregister dun request and choose wifi as upstream.
        wifi.fakeConnect();
        mCm.makeDefaultNetwork(wifi, CALLBACKS_FIRST);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor).setTryCell(false);
        inOrder.verify(mCm).unregisterNetworkCallback(eq(dunNetworkCallback1));
        inOrder.verify(mUpstreamNetworkMonitor).setCurrentUpstream(wifi.networkId);
        dun.fakeDisconnect(BROADCAST_FIRST, doDispatchAll);
        mLooper.dispatchAll();
        inOrder.verify(mUpstreamNetworkMonitor, never()).setCurrentUpstream(any());
    }

    private void runNcmTethering() {
        prepareNcmTethering();
        sendUsbBroadcast(true, true, TETHER_USB_NCM_FUNCTION);
    }

    @Test
    public void workingNcmTethering() throws Exception {
        runNcmTethering();

        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
    }

    @Test
    public void workingNcmTethering_LegacyDhcp() {
        when(mResources.getBoolean(R.bool.config_tether_enable_legacy_dhcp_server)).thenReturn(
                true);
        sendConfigurationChanged();
        runNcmTethering();

        verify(mIpServerDependencies, never()).makeDhcpServer(any(), any(), any());
    }

    @Test
    public void workingLocalOnlyHotspotEnrichedApBroadcastWithIfaceChanged() throws Exception {
        workingLocalOnlyHotspotEnrichedApBroadcast(true);
    }

    @Test
    public void workingLocalOnlyHotspotEnrichedApBroadcastSansIfaceChanged() throws Exception {
        workingLocalOnlyHotspotEnrichedApBroadcast(false);
    }

    // TODO: Test with and without interfaceStatusChanged().
    @Test
    public void failingWifiTetheringLegacyApBroadcast() throws Exception {
        when(mWifiManager.startTetheredHotspot(any(SoftApConfiguration.class))).thenReturn(true);

        // Emulate pressing the WiFi tethering button.
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_WIFI), null);
        mLooper.dispatchAll();
        verify(mWifiManager, times(1)).startTetheredHotspot(null);
        verifyNoMoreInteractions(mWifiManager);
        verifyNoMoreInteractions(mNetd);

        // Emulate externally-visible WifiManager effects, causing the
        // per-interface state machine to start up, and telling us that
        // tethering mode is to be started.
        mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED);

        // There is 1 IpServer state change event: STATE_AVAILABLE
        verify(mNotificationUpdater, times(1)).onDownstreamChanged(DOWNSTREAM_NONE);
        verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_AVAILABLE_TETHER);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        verifyNoMoreInteractions(mNetd);
        verifyNoMoreInteractions(mWifiManager);
    }

    // TODO: Test with and without interfaceStatusChanged().
    @Test
    public void workingWifiTetheringEnrichedApBroadcast() throws Exception {
        when(mWifiManager.startTetheredHotspot(any(SoftApConfiguration.class))).thenReturn(true);

        // Emulate pressing the WiFi tethering button.
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_WIFI), null);
        mLooper.dispatchAll();
        verify(mWifiManager, times(1)).startTetheredHotspot(null);
        verifyNoMoreInteractions(mWifiManager);
        verifyNoMoreInteractions(mNetd);

        // Emulate externally-visible WifiManager effects, causing the
        // per-interface state machine to start up, and telling us that
        // tethering mode is to be started.
        mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED, TEST_WLAN_IFNAME, IFACE_IP_MODE_TETHERED);

        verifyInterfaceServingModeStarted(TEST_WLAN_IFNAME);
        verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_AVAILABLE_TETHER);
        verify(mNetd, times(1)).ipfwdEnableForwarding(TETHERING_NAME);
        verify(mNetd, times(1)).tetherStartWithConfiguration(any());
        verify(mNetd, times(2)).networkAddRoute(eq(INetd.LOCAL_NET_ID), eq(TEST_WLAN_IFNAME),
                anyString(), anyString());
        verifyNoMoreInteractions(mNetd);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_TETHERED);
        verifyNoMoreInteractions(mWifiManager);
        verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_ACTIVE_TETHER);
        verify(mUpstreamNetworkMonitor, times(1)).startObserveAllNetworks();
        // In tethering mode, in the default configuration, an explicit request
        // for a mobile network is also made.
        verify(mUpstreamNetworkMonitor, times(1)).setTryCell(true);
        // There are 2 IpServer state change events: STATE_AVAILABLE -> STATE_TETHERED
        verify(mNotificationUpdater, times(1)).onDownstreamChanged(DOWNSTREAM_NONE);
        verify(mNotificationUpdater, times(1)).onDownstreamChanged(eq(1 << TETHERING_WIFI));

        /////
        // We do not currently emulate any upstream being found.
        //
        // This is why there are no calls to verify mNetd.tetherAddForward() or
        // mNetd.ipfwdAddInterfaceForward().
        /////

        // Emulate pressing the WiFi tethering button.
        mTethering.stopTethering(TETHERING_WIFI);
        mLooper.dispatchAll();
        verify(mWifiManager, times(1)).stopSoftAp();
        verifyNoMoreInteractions(mWifiManager);
        verifyNoMoreInteractions(mNetd);

        // Emulate externally-visible WifiManager effects, when tethering mode
        // is being torn down.
        sendWifiApStateChanged(WifiManager.WIFI_AP_STATE_DISABLED);
        mTethering.interfaceRemoved(TEST_WLAN_IFNAME);
        mLooper.dispatchAll();

        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
        verify(mNetd, times(1)).tetherInterfaceRemove(TEST_WLAN_IFNAME);
        verify(mNetd, times(1)).networkRemoveInterface(INetd.LOCAL_NET_ID, TEST_WLAN_IFNAME);
        // interfaceSetCfg() called once for enabling and twice for disabling IPv4.
        verify(mNetd, times(3)).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, times(1)).tetherStop();
        verify(mNetd, times(1)).ipfwdDisableForwarding(TETHERING_NAME);
        verify(mWifiManager, times(3)).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        verifyNoMoreInteractions(mNetd);
        verifyNoMoreInteractions(mWifiManager);
        // Asking for the last error after the per-interface state machine
        // has been reaped yields an unknown interface error.
        assertEquals(TETHER_ERROR_UNKNOWN_IFACE, mTethering.getLastErrorForTest(TEST_WLAN_IFNAME));
    }

    // TODO: Test with and without interfaceStatusChanged().
    @Test
    public void failureEnablingIpForwarding() throws Exception {
        when(mWifiManager.startTetheredHotspot(any(SoftApConfiguration.class))).thenReturn(true);
        doThrow(new RemoteException()).when(mNetd).ipfwdEnableForwarding(TETHERING_NAME);

        // Emulate pressing the WiFi tethering button.
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_WIFI), null);
        mLooper.dispatchAll();
        verify(mWifiManager, times(1)).startTetheredHotspot(null);
        verifyNoMoreInteractions(mWifiManager);
        verifyNoMoreInteractions(mNetd);

        // Emulate externally-visible WifiManager effects, causing the
        // per-interface state machine to start up, and telling us that
        // tethering mode is to be started.
        mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED, TEST_WLAN_IFNAME, IFACE_IP_MODE_TETHERED);

        // We verify get/set called three times here: twice for setup and once during
        // teardown because all events happen over the course of the single
        // dispatchAll() above. Note that once the IpServer IPv4 address config
        // code is refactored the two calls during shutdown will revert to one.
        verify(mNetd, times(3)).interfaceSetCfg(argThat(p -> TEST_WLAN_IFNAME.equals(p.ifName)));
        verify(mNetd, times(1)).tetherInterfaceAdd(TEST_WLAN_IFNAME);
        verify(mNetd, times(1)).networkAddInterface(INetd.LOCAL_NET_ID, TEST_WLAN_IFNAME);
        verify(mNetd, times(2)).networkAddRoute(eq(INetd.LOCAL_NET_ID), eq(TEST_WLAN_IFNAME),
                anyString(), anyString());
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_TETHERED);
        // There are 3 IpServer state change event:
        //         STATE_AVAILABLE -> STATE_TETHERED -> STATE_AVAILABLE.
        verify(mNotificationUpdater, times(2)).onDownstreamChanged(DOWNSTREAM_NONE);
        verify(mNotificationUpdater, times(1)).onDownstreamChanged(eq(1 << TETHERING_WIFI));
        verifyTetheringBroadcast(TEST_WLAN_IFNAME, EXTRA_AVAILABLE_TETHER);
        // This is called, but will throw.
        verify(mNetd, times(1)).ipfwdEnableForwarding(TETHERING_NAME);
        // This never gets called because of the exception thrown above.
        verify(mNetd, times(0)).tetherStartWithConfiguration(any());
        // When the main state machine transitions to an error state it tells
        // downstream interfaces, which causes us to tell Wi-Fi about the error
        // so it can take down AP mode.
        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
        verify(mNetd, times(1)).tetherInterfaceRemove(TEST_WLAN_IFNAME);
        verify(mNetd, times(1)).networkRemoveInterface(INetd.LOCAL_NET_ID, TEST_WLAN_IFNAME);
        verify(mWifiManager).updateInterfaceIpState(
                TEST_WLAN_IFNAME, WifiManager.IFACE_IP_MODE_CONFIGURATION_ERROR);

        verifyNoMoreInteractions(mWifiManager);
        verifyNoMoreInteractions(mNetd);
    }

    private UserRestrictionActionListener makeUserRestrictionActionListener(
            final Tethering tethering, final boolean currentDisallow, final boolean nextDisallow) {
        final Bundle newRestrictions = new Bundle();
        newRestrictions.putBoolean(UserManager.DISALLOW_CONFIG_TETHERING, nextDisallow);
        when(mUserManager.getUserRestrictions()).thenReturn(newRestrictions);

        final UserRestrictionActionListener ural =
                new UserRestrictionActionListener(mUserManager, tethering, mNotificationUpdater);
        ural.mDisallowTethering = currentDisallow;
        return ural;
    }

    private void runUserRestrictionsChange(
            boolean currentDisallow, boolean nextDisallow, boolean isTetheringActive,
            int expectedInteractionsWithShowNotification) throws  Exception {
        final Tethering mockTethering = mock(Tethering.class);
        when(mockTethering.isTetheringActive()).thenReturn(isTetheringActive);

        final UserRestrictionActionListener ural =
                makeUserRestrictionActionListener(mockTethering, currentDisallow, nextDisallow);
        ural.onUserRestrictionsChanged();

        verify(mNotificationUpdater, times(expectedInteractionsWithShowNotification))
                .notifyTetheringDisabledByRestriction();
        verify(mockTethering, times(expectedInteractionsWithShowNotification)).untetherAll();
    }

    @Test
    public void testDisallowTetheringWhenTetheringIsNotActive() throws Exception {
        final boolean isTetheringActive = false;
        final boolean currDisallow = false;
        final boolean nextDisallow = true;
        final int expectedInteractionsWithShowNotification = 0;

        runUserRestrictionsChange(currDisallow, nextDisallow, isTetheringActive,
                expectedInteractionsWithShowNotification);
    }

    @Test
    public void testDisallowTetheringWhenTetheringIsActive() throws Exception {
        final boolean isTetheringActive = true;
        final boolean currDisallow = false;
        final boolean nextDisallow = true;
        final int expectedInteractionsWithShowNotification = 1;

        runUserRestrictionsChange(currDisallow, nextDisallow, isTetheringActive,
                expectedInteractionsWithShowNotification);
    }

    @Test
    public void testAllowTetheringWhenTetheringIsNotActive() throws Exception {
        final boolean isTetheringActive = false;
        final boolean currDisallow = true;
        final boolean nextDisallow = false;
        final int expectedInteractionsWithShowNotification = 0;

        runUserRestrictionsChange(currDisallow, nextDisallow, isTetheringActive,
                expectedInteractionsWithShowNotification);
    }

    @Test
    public void testAllowTetheringWhenTetheringIsActive() throws Exception {
        final boolean isTetheringActive = true;
        final boolean currDisallow = true;
        final boolean nextDisallow = false;
        final int expectedInteractionsWithShowNotification = 0;

        runUserRestrictionsChange(currDisallow, nextDisallow, isTetheringActive,
                expectedInteractionsWithShowNotification);
    }

    @Test
    public void testDisallowTetheringUnchanged() throws Exception {
        final boolean isTetheringActive = true;
        final int expectedInteractionsWithShowNotification = 0;
        boolean currDisallow = true;
        boolean nextDisallow = true;

        runUserRestrictionsChange(currDisallow, nextDisallow, isTetheringActive,
                expectedInteractionsWithShowNotification);

        currDisallow = false;
        nextDisallow = false;

        runUserRestrictionsChange(currDisallow, nextDisallow, isTetheringActive,
                expectedInteractionsWithShowNotification);
    }

    @Test
    public void testUntetherUsbWhenRestrictionIsOn() {
        // Start usb tethering and check that usb interface is tethered.
        final UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        runUsbTethering(upstreamState);
        assertContains(Arrays.asList(mTethering.getTetheredIfaces()), TEST_RNDIS_IFNAME);
        assertTrue(mTethering.isTetheringActive());
        assertEquals(0, mTethering.getActiveTetheringRequests().size());

        final Tethering.UserRestrictionActionListener ural = makeUserRestrictionActionListener(
                mTethering, false /* currentDisallow */, true /* nextDisallow */);

        ural.onUserRestrictionsChanged();
        mLooper.dispatchAll();

        // Verify that restriction notification has showed to user.
        verify(mNotificationUpdater, times(1)).notifyTetheringDisabledByRestriction();
        // Verify that usb tethering has been disabled.
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_NONE);
    }

    private class TestTetheringEventCallback extends ITetheringEventCallback.Stub {
        private final ArrayList<Network> mActualUpstreams = new ArrayList<>();
        private final ArrayList<TetheringConfigurationParcel> mTetheringConfigs =
                new ArrayList<>();
        private final ArrayList<TetherStatesParcel> mTetherStates = new ArrayList<>();
        private final ArrayList<Integer> mOffloadStatus = new ArrayList<>();
        private final ArrayList<List<TetheredClient>> mTetheredClients = new ArrayList<>();

        // This function will remove the recorded callbacks, so it must be called once for
        // each callback. If this is called after multiple callback, the order matters.
        // onCallbackCreated counts as the first call to expectUpstreamChanged with
        // @see onCallbackCreated.
        public void expectUpstreamChanged(Network... networks) {
            if (networks == null) {
                assertNoUpstreamChangeCallback();
                return;
            }

            final ArrayList<Network> expectedUpstreams =
                    new ArrayList<Network>(Arrays.asList(networks));
            for (Network upstream : expectedUpstreams) {
                // throws OOB if no expectations
                assertEquals(mActualUpstreams.remove(0), upstream);
            }
            assertNoUpstreamChangeCallback();
        }

        // This function will remove the recorded callbacks, so it must be called once
        // for each callback. If this is called after multiple callback, the order matters.
        // onCallbackCreated counts as the first call to onConfigurationChanged with
        // @see onCallbackCreated.
        public void expectConfigurationChanged(TetheringConfigurationParcel... tetherConfigs) {
            final ArrayList<TetheringConfigurationParcel> expectedTetherConfig =
                    new ArrayList<TetheringConfigurationParcel>(Arrays.asList(tetherConfigs));
            for (TetheringConfigurationParcel config : expectedTetherConfig) {
                // throws OOB if no expectations
                final TetheringConfigurationParcel actualConfig = mTetheringConfigs.remove(0);
                assertTetherConfigParcelEqual(actualConfig, config);
            }
            assertNoConfigChangeCallback();
        }

        public void expectOffloadStatusChanged(final int expectedStatus) {
            assertOffloadStatusChangedCallback();
            assertEquals(mOffloadStatus.remove(0), new Integer(expectedStatus));
        }

        public TetherStatesParcel pollTetherStatesChanged() {
            assertStateChangeCallback();
            return mTetherStates.remove(0);
        }

        public void expectTetheredClientChanged(List<TetheredClient> leases) {
            assertFalse(mTetheredClients.isEmpty());
            final List<TetheredClient> result = mTetheredClients.remove(0);
            assertEquals(leases.size(), result.size());
            assertTrue(leases.containsAll(result));
        }

        @Override
        public void onUpstreamChanged(Network network) {
            mActualUpstreams.add(network);
        }

        @Override
        public void onConfigurationChanged(TetheringConfigurationParcel config) {
            mTetheringConfigs.add(config);
        }

        @Override
        public void onTetherStatesChanged(TetherStatesParcel states) {
            mTetherStates.add(states);
        }

        @Override
        public void onTetherClientsChanged(List<TetheredClient> clients) {
            mTetheredClients.add(clients);
        }

        @Override
        public void onOffloadStatusChanged(final int status) {
            mOffloadStatus.add(status);
        }

        @Override
        public void onCallbackStarted(TetheringCallbackStartedParcel parcel) {
            mActualUpstreams.add(parcel.upstreamNetwork);
            mTetheringConfigs.add(parcel.config);
            mTetherStates.add(parcel.states);
            mOffloadStatus.add(parcel.offloadStatus);
            mTetheredClients.add(parcel.tetheredClients);
        }

        @Override
        public void onCallbackStopped(int errorCode) { }

        public void assertNoUpstreamChangeCallback() {
            assertTrue(mActualUpstreams.isEmpty());
        }

        public void assertNoConfigChangeCallback() {
            assertTrue(mTetheringConfigs.isEmpty());
        }

        public void assertNoStateChangeCallback() {
            assertTrue(mTetherStates.isEmpty());
        }

        public void assertStateChangeCallback() {
            assertFalse(mTetherStates.isEmpty());
        }

        public void assertOffloadStatusChangedCallback() {
            assertFalse(mOffloadStatus.isEmpty());
        }

        public void assertNoCallback() {
            assertNoUpstreamChangeCallback();
            assertNoConfigChangeCallback();
            assertNoStateChangeCallback();
            assertTrue(mTetheredClients.isEmpty());
        }

        private void assertTetherConfigParcelEqual(@NonNull TetheringConfigurationParcel actual,
                @NonNull TetheringConfigurationParcel expect) {
            assertEquals(actual.subId, expect.subId);
            assertArrayEquals(actual.tetherableUsbRegexs, expect.tetherableUsbRegexs);
            assertArrayEquals(actual.tetherableWifiRegexs, expect.tetherableWifiRegexs);
            assertArrayEquals(actual.tetherableBluetoothRegexs, expect.tetherableBluetoothRegexs);
            assertEquals(actual.isDunRequired, expect.isDunRequired);
            assertEquals(actual.chooseUpstreamAutomatically, expect.chooseUpstreamAutomatically);
            assertArrayEquals(actual.preferredUpstreamIfaceTypes,
                    expect.preferredUpstreamIfaceTypes);
            assertArrayEquals(actual.legacyDhcpRanges, expect.legacyDhcpRanges);
            assertArrayEquals(actual.defaultIPv4DNS, expect.defaultIPv4DNS);
            assertEquals(actual.enableLegacyDhcpServer, expect.enableLegacyDhcpServer);
            assertArrayEquals(actual.provisioningApp, expect.provisioningApp);
            assertEquals(actual.provisioningAppNoUi, expect.provisioningAppNoUi);
            assertEquals(actual.provisioningCheckPeriod, expect.provisioningCheckPeriod);
        }
    }

    private void assertTetherStatesNotNullButEmpty(final TetherStatesParcel parcel) {
        assertFalse(parcel == null);
        assertEquals(0, parcel.availableList.length);
        assertEquals(0, parcel.tetheredList.length);
        assertEquals(0, parcel.localOnlyList.length);
        assertEquals(0, parcel.erroredIfaceList.length);
        assertEquals(0, parcel.lastErrorList.length);
        MiscAsserts.assertFieldCountEquals(5, TetherStatesParcel.class);
    }

    @Test
    public void testRegisterTetheringEventCallback() throws Exception {
        TestTetheringEventCallback callback = new TestTetheringEventCallback();
        TestTetheringEventCallback callback2 = new TestTetheringEventCallback();
        final TetheringInterface wifiIface = new TetheringInterface(
                TETHERING_WIFI, TEST_WLAN_IFNAME);

        // 1. Register one callback before running any tethering.
        mTethering.registerTetheringEventCallback(callback);
        mLooper.dispatchAll();
        callback.expectTetheredClientChanged(Collections.emptyList());
        callback.expectUpstreamChanged(new Network[] {null});
        callback.expectConfigurationChanged(
                mTethering.getTetheringConfiguration().toStableParcelable());
        TetherStatesParcel tetherState = callback.pollTetherStatesChanged();
        assertTetherStatesNotNullButEmpty(tetherState);
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STOPPED);
        // 2. Enable wifi tethering.
        UpstreamNetworkState upstreamState = buildMobileDualStackUpstreamState();
        initTetheringUpstream(upstreamState);
        when(mWifiManager.startTetheredHotspot(any(SoftApConfiguration.class))).thenReturn(true);
        mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        mLooper.dispatchAll();
        tetherState = callback.pollTetherStatesChanged();
        assertArrayEquals(tetherState.availableList, new TetheringInterface[] {wifiIface});

        mTethering.startTethering(createTetheringRequestParcel(TETHERING_WIFI), null);
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED, TEST_WLAN_IFNAME, IFACE_IP_MODE_TETHERED);
        tetherState = callback.pollTetherStatesChanged();
        assertArrayEquals(tetherState.tetheredList, new TetheringInterface[] {wifiIface});
        callback.expectUpstreamChanged(upstreamState.network);
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STARTED);

        // 3. Register second callback.
        mTethering.registerTetheringEventCallback(callback2);
        mLooper.dispatchAll();
        callback2.expectTetheredClientChanged(Collections.emptyList());
        callback2.expectUpstreamChanged(upstreamState.network);
        callback2.expectConfigurationChanged(
                mTethering.getTetheringConfiguration().toStableParcelable());
        tetherState = callback2.pollTetherStatesChanged();
        assertEquals(tetherState.tetheredList, new TetheringInterface[] {wifiIface});
        callback2.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STARTED);

        // 4. Unregister first callback and disable wifi tethering
        mTethering.unregisterTetheringEventCallback(callback);
        mLooper.dispatchAll();
        mTethering.stopTethering(TETHERING_WIFI);
        sendWifiApStateChanged(WifiManager.WIFI_AP_STATE_DISABLED);
        tetherState = callback2.pollTetherStatesChanged();
        assertArrayEquals(tetherState.availableList, new TetheringInterface[] {wifiIface});
        mLooper.dispatchAll();
        callback2.expectUpstreamChanged(new Network[] {null});
        callback2.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STOPPED);
        callback.assertNoCallback();
    }

    @Test
    public void testReportFailCallbackIfOffloadNotSupported() throws Exception {
        final UpstreamNetworkState upstreamState = buildMobileDualStackUpstreamState();
        TestTetheringEventCallback callback = new TestTetheringEventCallback();
        mTethering.registerTetheringEventCallback(callback);
        mLooper.dispatchAll();
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STOPPED);

        // 1. Offload fail if no OffloadConfig.
        initOffloadConfiguration(false /* offloadConfig */, OFFLOAD_HAL_VERSION_1_0,
                0 /* defaultDisabled */);
        runUsbTethering(upstreamState);
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_FAILED);
        runStopUSBTethering();
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STOPPED);
        reset(mUsbManager, mIPv6TetheringCoordinator);
        // 2. Offload fail if no OffloadControl.
        initOffloadConfiguration(true /* offloadConfig */, OFFLOAD_HAL_VERSION_NONE,
                0 /* defaultDisabled */);
        runUsbTethering(upstreamState);
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_FAILED);
        runStopUSBTethering();
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STOPPED);
        reset(mUsbManager, mIPv6TetheringCoordinator);
        // 3. Offload fail if disabled by settings.
        initOffloadConfiguration(true /* offloadConfig */, OFFLOAD_HAL_VERSION_1_0,
                1 /* defaultDisabled */);
        runUsbTethering(upstreamState);
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_FAILED);
        runStopUSBTethering();
        callback.expectOffloadStatusChanged(TETHER_HARDWARE_OFFLOAD_STOPPED);
    }

    private void runStopUSBTethering() {
        mTethering.stopTethering(TETHERING_USB);
        mLooper.dispatchAll();
        sendUsbBroadcast(true, true, -1 /* function */);
        mLooper.dispatchAll();
        verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_NONE);
    }

    private void initOffloadConfiguration(final boolean offloadConfig,
            @OffloadHardwareInterface.OffloadHalVersion final int offloadControlVersion,
            final int defaultDisabled) {
        when(mOffloadHardwareInterface.initOffloadConfig()).thenReturn(offloadConfig);
        when(mOffloadHardwareInterface.initOffloadControl(any())).thenReturn(offloadControlVersion);
        when(mOffloadHardwareInterface.getDefaultTetherOffloadDisabled()).thenReturn(
                defaultDisabled);
    }

    @Test
    public void testMultiSimAware() throws Exception {
        final TetheringConfiguration initailConfig = mTethering.getTetheringConfiguration();
        assertEquals(INVALID_SUBSCRIPTION_ID, initailConfig.activeDataSubId);

        final int fakeSubId = 1234;
        mPhoneStateListener.onActiveDataSubscriptionIdChanged(fakeSubId);
        final TetheringConfiguration newConfig = mTethering.getTetheringConfiguration();
        assertEquals(fakeSubId, newConfig.activeDataSubId);
        verify(mNotificationUpdater, times(1)).onActiveDataSubscriptionIdChanged(eq(fakeSubId));
    }

    @Test
    public void testNoDuplicatedEthernetRequest() throws Exception {
        final TetheredInterfaceRequest mockRequest = mock(TetheredInterfaceRequest.class);
        when(mEm.requestTetheredInterface(any(), any())).thenReturn(mockRequest);
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_ETHERNET), null);
        mLooper.dispatchAll();
        verify(mEm, times(1)).requestTetheredInterface(any(), any());
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_ETHERNET), null);
        mLooper.dispatchAll();
        verifyNoMoreInteractions(mEm);
        mTethering.stopTethering(TETHERING_ETHERNET);
        mLooper.dispatchAll();
        verify(mockRequest, times(1)).release();
        mTethering.stopTethering(TETHERING_ETHERNET);
        mLooper.dispatchAll();
        verifyNoMoreInteractions(mEm);
    }

    private void workingWifiP2pGroupOwner(
            boolean emulateInterfaceStatusChanged) throws Exception {
        if (emulateInterfaceStatusChanged) {
            mTethering.interfaceStatusChanged(TEST_P2P_IFNAME, true);
        }
        sendWifiP2pConnectionChanged(true, true, TEST_P2P_IFNAME);

        verifyInterfaceServingModeStarted(TEST_P2P_IFNAME);
        verifyTetheringBroadcast(TEST_P2P_IFNAME, EXTRA_AVAILABLE_TETHER);
        verify(mNetd, times(1)).ipfwdEnableForwarding(TETHERING_NAME);
        verify(mNetd, times(1)).tetherStartWithConfiguration(any());
        verifyNoMoreInteractions(mNetd);
        verifyTetheringBroadcast(TEST_P2P_IFNAME, EXTRA_ACTIVE_LOCAL_ONLY);
        verify(mUpstreamNetworkMonitor, times(1)).startObserveAllNetworks();
        // There are 2 IpServer state change events: STATE_AVAILABLE -> STATE_LOCAL_ONLY
        verify(mNotificationUpdater, times(2)).onDownstreamChanged(DOWNSTREAM_NONE);

        assertEquals(TETHER_ERROR_NO_ERROR, mTethering.getLastErrorForTest(TEST_P2P_IFNAME));

        // Emulate externally-visible WifiP2pManager effects, when wifi p2p group
        // is being removed.
        sendWifiP2pConnectionChanged(false, true, TEST_P2P_IFNAME);
        mTethering.interfaceRemoved(TEST_P2P_IFNAME);

        verify(mNetd, times(1)).tetherApplyDnsInterfaces();
        verify(mNetd, times(1)).tetherInterfaceRemove(TEST_P2P_IFNAME);
        verify(mNetd, times(1)).networkRemoveInterface(INetd.LOCAL_NET_ID, TEST_P2P_IFNAME);
        // interfaceSetCfg() called once for enabling and twice for disabling IPv4.
        verify(mNetd, times(3)).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, times(1)).tetherStop();
        verify(mNetd, times(1)).ipfwdDisableForwarding(TETHERING_NAME);
        verify(mUpstreamNetworkMonitor, never()).getCurrentPreferredUpstream();
        verify(mUpstreamNetworkMonitor, never()).selectPreferredUpstreamType(any());
        verifyNoMoreInteractions(mNetd);
        // Asking for the last error after the per-interface state machine
        // has been reaped yields an unknown interface error.
        assertEquals(TETHER_ERROR_UNKNOWN_IFACE, mTethering.getLastErrorForTest(TEST_P2P_IFNAME));
    }

    private void workingWifiP2pGroupClient(
            boolean emulateInterfaceStatusChanged) throws Exception {
        if (emulateInterfaceStatusChanged) {
            mTethering.interfaceStatusChanged(TEST_P2P_IFNAME, true);
        }
        sendWifiP2pConnectionChanged(true, false, TEST_P2P_IFNAME);

        verify(mNetd, never()).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, never()).tetherInterfaceAdd(TEST_P2P_IFNAME);
        verify(mNetd, never()).networkAddInterface(INetd.LOCAL_NET_ID, TEST_P2P_IFNAME);
        verify(mNetd, never()).ipfwdEnableForwarding(TETHERING_NAME);
        verify(mNetd, never()).tetherStartWithConfiguration(any());

        // Emulate externally-visible WifiP2pManager effects, when wifi p2p group
        // is being removed.
        sendWifiP2pConnectionChanged(false, false, TEST_P2P_IFNAME);
        mTethering.interfaceRemoved(TEST_P2P_IFNAME);

        verify(mNetd, never()).tetherApplyDnsInterfaces();
        verify(mNetd, never()).tetherInterfaceRemove(TEST_P2P_IFNAME);
        verify(mNetd, never()).networkRemoveInterface(INetd.LOCAL_NET_ID, TEST_P2P_IFNAME);
        verify(mNetd, never()).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, never()).tetherStop();
        verify(mNetd, never()).ipfwdDisableForwarding(TETHERING_NAME);
        verifyNoMoreInteractions(mNetd);
        // Asking for the last error after the per-interface state machine
        // has been reaped yields an unknown interface error.
        assertEquals(TETHER_ERROR_UNKNOWN_IFACE, mTethering.getLastErrorForTest(TEST_P2P_IFNAME));
    }

    @Test
    public void workingWifiP2pGroupOwnerWithIfaceChanged() throws Exception {
        workingWifiP2pGroupOwner(true);
    }

    @Test
    public void workingWifiP2pGroupOwnerSansIfaceChanged() throws Exception {
        workingWifiP2pGroupOwner(false);
    }

    private void workingWifiP2pGroupOwnerLegacyMode(
            boolean emulateInterfaceStatusChanged) throws Exception {
        // change to legacy mode and update tethering information by chaning SIM
        when(mResources.getStringArray(R.array.config_tether_wifi_p2p_regexs))
                .thenReturn(new String[]{});
        final int fakeSubId = 1234;
        mPhoneStateListener.onActiveDataSubscriptionIdChanged(fakeSubId);

        if (emulateInterfaceStatusChanged) {
            mTethering.interfaceStatusChanged(TEST_P2P_IFNAME, true);
        }
        sendWifiP2pConnectionChanged(true, true, TEST_P2P_IFNAME);

        verify(mNetd, never()).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd, never()).tetherInterfaceAdd(TEST_P2P_IFNAME);
        verify(mNetd, never()).networkAddInterface(INetd.LOCAL_NET_ID, TEST_P2P_IFNAME);
        verify(mNetd, never()).ipfwdEnableForwarding(TETHERING_NAME);
        verify(mNetd, never()).tetherStartWithConfiguration(any());
        assertEquals(TETHER_ERROR_UNKNOWN_IFACE, mTethering.getLastErrorForTest(TEST_P2P_IFNAME));
    }
    @Test
    public void workingWifiP2pGroupOwnerLegacyModeWithIfaceChanged() throws Exception {
        workingWifiP2pGroupOwnerLegacyMode(true);
    }

    @Test
    public void workingWifiP2pGroupOwnerLegacyModeSansIfaceChanged() throws Exception {
        workingWifiP2pGroupOwnerLegacyMode(false);
    }

    @Test
    public void workingWifiP2pGroupClientWithIfaceChanged() throws Exception {
        workingWifiP2pGroupClient(true);
    }

    @Test
    public void workingWifiP2pGroupClientSansIfaceChanged() throws Exception {
        workingWifiP2pGroupClient(false);
    }

    private void setDataSaverEnabled(boolean enabled) {
        final int status = enabled ? RESTRICT_BACKGROUND_STATUS_ENABLED
                : RESTRICT_BACKGROUND_STATUS_DISABLED;
        doReturn(status).when(mCm).getRestrictBackgroundStatus();

        final Intent intent = new Intent(ACTION_RESTRICT_BACKGROUND_CHANGED);
        mServiceContext.sendBroadcastAsUser(intent, UserHandle.ALL);
        mLooper.dispatchAll();
    }

    @Test
    public void testDataSaverChanged() {
        // Start Tethering.
        final UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        runUsbTethering(upstreamState);
        assertContains(Arrays.asList(mTethering.getTetheredIfaces()), TEST_RNDIS_IFNAME);
        // Data saver is ON.
        setDataSaverEnabled(true);
        // Verify that tethering should be disabled.
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        sendUsbBroadcast(true, true, -1 /* function */);
        mLooper.dispatchAll();
        assertEquals(mTethering.getTetheredIfaces(), new String[0]);
        reset(mUsbManager, mIPv6TetheringCoordinator);

        runUsbTethering(upstreamState);
        // Verify that user can start tethering again without turning OFF data saver.
        assertContains(Arrays.asList(mTethering.getTetheredIfaces()), TEST_RNDIS_IFNAME);

        // If data saver is keep ON with change event, tethering should not be OFF this time.
        setDataSaverEnabled(true);
        verify(mUsbManager, times(0)).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        assertContains(Arrays.asList(mTethering.getTetheredIfaces()), TEST_RNDIS_IFNAME);

        // If data saver is turned OFF, it should not change tethering.
        setDataSaverEnabled(false);
        verify(mUsbManager, times(0)).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        assertContains(Arrays.asList(mTethering.getTetheredIfaces()), TEST_RNDIS_IFNAME);
    }

    private static <T> void assertContains(Collection<T> collection, T element) {
        assertTrue(element + " not found in " + collection, collection.contains(element));
    }

    private class ResultListener extends IIntResultListener.Stub {
        private final int mExpectedResult;
        private boolean mHasResult = false;
        ResultListener(final int expectedResult) {
            mExpectedResult = expectedResult;
        }

        @Override
        public void onResult(final int resultCode) {
            mHasResult = true;
            if (resultCode != mExpectedResult) {
                fail("expected result: " + mExpectedResult + " but actual result: " + resultCode);
            }
        }

        public void assertHasResult() {
            if (!mHasResult) fail("No callback result");
        }
    }

    @Test
    public void testMultipleStartTethering() throws Exception {
        final LinkAddress serverLinkAddr = new LinkAddress("192.168.20.1/24");
        final LinkAddress clientLinkAddr = new LinkAddress("192.168.20.42/24");
        final String serverAddr = "192.168.20.1";
        final ResultListener firstResult = new ResultListener(TETHER_ERROR_NO_ERROR);
        final ResultListener secondResult = new ResultListener(TETHER_ERROR_NO_ERROR);
        final ResultListener thirdResult = new ResultListener(TETHER_ERROR_NO_ERROR);

        // Enable USB tethering and check that Tethering starts USB.
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_USB), firstResult);
        mLooper.dispatchAll();
        firstResult.assertHasResult();
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_RNDIS);
        verifyNoMoreInteractions(mUsbManager);

        // Enable USB tethering again with the same request and expect no change to USB.
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_USB), secondResult);
        mLooper.dispatchAll();
        secondResult.assertHasResult();
        verify(mUsbManager, never()).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        reset(mUsbManager);

        // Enable USB tethering with a different request and expect that USB is stopped and
        // started.
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_USB,
                  serverLinkAddr, clientLinkAddr, false, CONNECTIVITY_SCOPE_GLOBAL), thirdResult);
        mLooper.dispatchAll();
        thirdResult.assertHasResult();
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_RNDIS);

        // Expect that when USB comes up, the DHCP server is configured with the requested address.
        mTethering.interfaceStatusChanged(TEST_RNDIS_IFNAME, true);
        sendUsbBroadcast(true, true, TETHER_USB_RNDIS_FUNCTION);
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
        verify(mNetd).interfaceSetCfg(argThat(cfg -> serverAddr.equals(cfg.ipv4Addr)));
    }

    @Test
    public void testRequestStaticIp() throws Exception {
        when(mResources.getInteger(R.integer.config_tether_usb_functions)).thenReturn(
                TetheringConfiguration.TETHER_USB_NCM_FUNCTION);
        when(mResources.getStringArray(R.array.config_tether_usb_regexs))
                .thenReturn(new String[] {TEST_NCM_REGEX});
        sendConfigurationChanged();

        final LinkAddress serverLinkAddr = new LinkAddress("192.168.0.123/24");
        final LinkAddress clientLinkAddr = new LinkAddress("192.168.0.42/24");
        final String serverAddr = "192.168.0.123";
        final int clientAddrParceled = 0xc0a8002a;
        final ArgumentCaptor<DhcpServingParamsParcel> dhcpParamsCaptor =
                ArgumentCaptor.forClass(DhcpServingParamsParcel.class);
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_USB,
                  serverLinkAddr, clientLinkAddr, false, CONNECTIVITY_SCOPE_GLOBAL), null);
        mLooper.dispatchAll();
        verify(mUsbManager, times(1)).setCurrentFunctions(UsbManager.FUNCTION_NCM);
        mTethering.interfaceStatusChanged(TEST_NCM_IFNAME, true);
        sendUsbBroadcast(true, true, TETHER_USB_NCM_FUNCTION);
        verify(mNetd).interfaceSetCfg(argThat(cfg -> serverAddr.equals(cfg.ipv4Addr)));
        verify(mIpServerDependencies, times(1)).makeDhcpServer(any(), dhcpParamsCaptor.capture(),
                any());
        final DhcpServingParamsParcel params = dhcpParamsCaptor.getValue();
        assertEquals(serverAddr, intToInet4AddressHTH(params.serverAddr).getHostAddress());
        assertEquals(24, params.serverAddrPrefixLength);
        assertEquals(clientAddrParceled, params.singleClientAddr);
    }

    @Test
    public void testUpstreamNetworkChanged() {
        final Tethering.TetherMainSM stateMachine = (Tethering.TetherMainSM)
                mTetheringDependencies.mUpstreamNetworkMonitorSM;
        final UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        initTetheringUpstream(upstreamState);
        stateMachine.chooseUpstreamType(true);

        verify(mUpstreamNetworkMonitor, times(1)).setCurrentUpstream(eq(upstreamState.network));
        verify(mNotificationUpdater, times(1)).onUpstreamCapabilitiesChanged(any());
    }

    @Test
    public void testUpstreamCapabilitiesChanged() {
        final Tethering.TetherMainSM stateMachine = (Tethering.TetherMainSM)
                mTetheringDependencies.mUpstreamNetworkMonitorSM;
        final UpstreamNetworkState upstreamState = buildMobileIPv4UpstreamState();
        initTetheringUpstream(upstreamState);
        stateMachine.chooseUpstreamType(true);

        stateMachine.handleUpstreamNetworkMonitorCallback(EVENT_ON_CAPABILITIES, upstreamState);
        // Should have two onUpstreamCapabilitiesChanged().
        // One is called by reportUpstreamChanged(). One is called by EVENT_ON_CAPABILITIES.
        verify(mNotificationUpdater, times(2)).onUpstreamCapabilitiesChanged(any());
        reset(mNotificationUpdater);

        // Verify that onUpstreamCapabilitiesChanged won't be called if not current upstream network
        // capabilities changed.
        final UpstreamNetworkState upstreamState2 = new UpstreamNetworkState(
                upstreamState.linkProperties, upstreamState.networkCapabilities,
                new Network(WIFI_NETID));
        stateMachine.handleUpstreamNetworkMonitorCallback(EVENT_ON_CAPABILITIES, upstreamState2);
        verify(mNotificationUpdater, never()).onUpstreamCapabilitiesChanged(any());
    }

    @Test
    public void testDumpTetheringLog() throws Exception {
        final FileDescriptor mockFd = mock(FileDescriptor.class);
        final PrintWriter mockPw = mock(PrintWriter.class);
        runUsbTethering(null);
        mLooper.startAutoDispatch();
        mTethering.dump(mockFd, mockPw, new String[0]);
        verify(mConfig).dump(any());
        verify(mEntitleMgr).dump(any());
        verify(mOffloadCtrl).dump(any());
        mLooper.stopAutoDispatch();
    }

    @Test
    public void testExemptFromEntitlementCheck() throws Exception {
        setupForRequiredProvisioning();
        final TetheringRequestParcel wifiNotExemptRequest =
                createTetheringRequestParcel(TETHERING_WIFI, null, null, false,
                        CONNECTIVITY_SCOPE_GLOBAL);
        mTethering.startTethering(wifiNotExemptRequest, null);
        mLooper.dispatchAll();
        verify(mEntitleMgr).startProvisioningIfNeeded(TETHERING_WIFI, false);
        verify(mEntitleMgr, never()).setExemptedDownstreamType(TETHERING_WIFI);
        assertFalse(mEntitleMgr.isCellularUpstreamPermitted());
        mTethering.stopTethering(TETHERING_WIFI);
        mLooper.dispatchAll();
        verify(mEntitleMgr).stopProvisioningIfNeeded(TETHERING_WIFI);
        reset(mEntitleMgr);

        setupForRequiredProvisioning();
        final TetheringRequestParcel wifiExemptRequest =
                createTetheringRequestParcel(TETHERING_WIFI, null, null, true,
                        CONNECTIVITY_SCOPE_GLOBAL);
        mTethering.startTethering(wifiExemptRequest, null);
        mLooper.dispatchAll();
        verify(mEntitleMgr, never()).startProvisioningIfNeeded(TETHERING_WIFI, false);
        verify(mEntitleMgr).setExemptedDownstreamType(TETHERING_WIFI);
        assertTrue(mEntitleMgr.isCellularUpstreamPermitted());
        mTethering.stopTethering(TETHERING_WIFI);
        mLooper.dispatchAll();
        verify(mEntitleMgr).stopProvisioningIfNeeded(TETHERING_WIFI);
        reset(mEntitleMgr);

        // If one app enables tethering without provisioning check first, then another app enables
        // tethering of the same type but does not disable the provisioning check.
        setupForRequiredProvisioning();
        mTethering.startTethering(wifiExemptRequest, null);
        mLooper.dispatchAll();
        verify(mEntitleMgr, never()).startProvisioningIfNeeded(TETHERING_WIFI, false);
        verify(mEntitleMgr).setExemptedDownstreamType(TETHERING_WIFI);
        assertTrue(mEntitleMgr.isCellularUpstreamPermitted());
        reset(mEntitleMgr);
        setupForRequiredProvisioning();
        mTethering.startTethering(wifiNotExemptRequest, null);
        mLooper.dispatchAll();
        verify(mEntitleMgr).startProvisioningIfNeeded(TETHERING_WIFI, false);
        verify(mEntitleMgr, never()).setExemptedDownstreamType(TETHERING_WIFI);
        assertFalse(mEntitleMgr.isCellularUpstreamPermitted());
        mTethering.stopTethering(TETHERING_WIFI);
        mLooper.dispatchAll();
        verify(mEntitleMgr).stopProvisioningIfNeeded(TETHERING_WIFI);
        reset(mEntitleMgr);
    }

    private void setupForRequiredProvisioning() {
        // Produce some acceptable looking provision app setting if requested.
        when(mResources.getStringArray(R.array.config_mobile_hotspot_provision_app))
                .thenReturn(PROVISIONING_APP_NAME);
        when(mResources.getString(R.string.config_mobile_hotspot_provision_app_no_ui))
                .thenReturn(PROVISIONING_NO_UI_APP_NAME);
        // Act like the CarrierConfigManager is present and ready unless told otherwise.
        when(mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE))
                .thenReturn(mCarrierConfigManager);
        when(mCarrierConfigManager.getConfigForSubId(anyInt())).thenReturn(mCarrierConfig);
        mCarrierConfig.putBoolean(CarrierConfigManager.KEY_REQUIRE_ENTITLEMENT_CHECKS_BOOL, true);
        mCarrierConfig.putBoolean(CarrierConfigManager.KEY_CARRIER_CONFIG_APPLIED_BOOL, true);
        sendConfigurationChanged();
    }

    private static UpstreamNetworkState buildV4UpstreamState(final LinkAddress address,
            final Network network, final String iface, final int transportType) {
        final LinkProperties prop = new LinkProperties();
        prop.setInterfaceName(iface);

        prop.addLinkAddress(address);

        final NetworkCapabilities capabilities = new NetworkCapabilities()
                .addTransportType(transportType);
        return new UpstreamNetworkState(prop, capabilities, network);
    }

    private void updateV4Upstream(final LinkAddress ipv4Address, final Network network,
            final String iface, final int transportType) {
        final UpstreamNetworkState upstream = buildV4UpstreamState(ipv4Address, network, iface,
                transportType);
        mTetheringDependencies.mUpstreamNetworkMonitorSM.sendMessage(
                Tethering.TetherMainSM.EVENT_UPSTREAM_CALLBACK,
                UpstreamNetworkMonitor.EVENT_ON_LINKPROPERTIES,
                0,
                upstream);
        mLooper.dispatchAll();
    }

    @Test
    public void testHandleIpConflict() throws Exception {
        final Network wifiNetwork = new Network(200);
        final Network[] allNetworks = { wifiNetwork };
        doReturn(allNetworks).when(mCm).getAllNetworks();
        runUsbTethering(null);
        final ArgumentCaptor<InterfaceConfigurationParcel> ifaceConfigCaptor =
                ArgumentCaptor.forClass(InterfaceConfigurationParcel.class);
        verify(mNetd).interfaceSetCfg(ifaceConfigCaptor.capture());
        final String ipv4Address = ifaceConfigCaptor.getValue().ipv4Addr;
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
        reset(mUsbManager);

        // Cause a prefix conflict by assigning a /30 out of the downstream's /24 to the upstream.
        updateV4Upstream(new LinkAddress(InetAddresses.parseNumericAddress(ipv4Address), 30),
                wifiNetwork, TEST_WIFI_IFNAME, TRANSPORT_WIFI);
        // verify turn off usb tethering
        verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        sendUsbBroadcast(true, true, -1 /* function */);
        mLooper.dispatchAll();
        // verify restart usb tethering
        verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_RNDIS);
    }

    @Test
    public void testNoAddressAvailable() throws Exception {
        final Network wifiNetwork = new Network(200);
        final Network btNetwork = new Network(201);
        final Network mobileNetwork = new Network(202);
        final Network[] allNetworks = { wifiNetwork, btNetwork, mobileNetwork };
        doReturn(allNetworks).when(mCm).getAllNetworks();
        runUsbTethering(null);
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());
        reset(mUsbManager);
        final TetheredInterfaceRequest mockRequest = mock(TetheredInterfaceRequest.class);
        when(mEm.requestTetheredInterface(any(), any())).thenReturn(mockRequest);
        final ArgumentCaptor<TetheredInterfaceCallback> callbackCaptor =
                ArgumentCaptor.forClass(TetheredInterfaceCallback.class);
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_ETHERNET), null);
        mLooper.dispatchAll();
        verify(mEm).requestTetheredInterface(any(), callbackCaptor.capture());
        TetheredInterfaceCallback ethCallback = callbackCaptor.getValue();
        ethCallback.onAvailable(TEST_ETH_IFNAME);
        mLooper.dispatchAll();
        reset(mUsbManager, mEm);

        updateV4Upstream(new LinkAddress("192.168.0.100/16"), wifiNetwork, TEST_WIFI_IFNAME,
                TRANSPORT_WIFI);
        updateV4Upstream(new LinkAddress("172.16.0.0/12"), btNetwork, TEST_BT_IFNAME,
                TRANSPORT_BLUETOOTH);
        updateV4Upstream(new LinkAddress("10.0.0.0/8"), mobileNetwork, TEST_MOBILE_IFNAME,
                TRANSPORT_CELLULAR);

        mLooper.dispatchAll();
        // verify turn off usb tethering
        verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        // verify turn off ethernet tethering
        verify(mockRequest).release();
        sendUsbBroadcast(true, true, -1 /* function */);
        ethCallback.onUnavailable();
        mLooper.dispatchAll();
        // verify restart usb tethering
        verify(mUsbManager).setCurrentFunctions(UsbManager.FUNCTION_RNDIS);
        // verify restart ethernet tethering
        verify(mEm).requestTetheredInterface(any(), callbackCaptor.capture());
        ethCallback = callbackCaptor.getValue();
        ethCallback.onAvailable(TEST_ETH_IFNAME);

        reset(mUsbManager, mEm);
        when(mNetd.interfaceGetList())
                .thenReturn(new String[] {
                        TEST_MOBILE_IFNAME, TEST_WLAN_IFNAME, TEST_RNDIS_IFNAME, TEST_P2P_IFNAME,
                        TEST_NCM_IFNAME, TEST_ETH_IFNAME});

        mTethering.interfaceStatusChanged(TEST_RNDIS_IFNAME, true);
        sendUsbBroadcast(true, true, TETHER_USB_RNDIS_FUNCTION);
        assertContains(Arrays.asList(mTethering.getTetherableIfacesForTest()), TEST_RNDIS_IFNAME);
        assertContains(Arrays.asList(mTethering.getTetherableIfacesForTest()), TEST_ETH_IFNAME);
        assertEquals(TETHER_ERROR_IFACE_CFG_ERROR, mTethering.getLastErrorForTest(
                TEST_RNDIS_IFNAME));
        assertEquals(TETHER_ERROR_IFACE_CFG_ERROR, mTethering.getLastErrorForTest(TEST_ETH_IFNAME));
    }

    @Test
    public void testProvisioningNeededButUnavailable() throws Exception {
        assertTrue(mTethering.isTetheringSupported());
        verify(mPackageManager, never()).getPackageInfo(PROVISIONING_APP_NAME[0], GET_ACTIVITIES);

        setupForRequiredProvisioning();
        assertTrue(mTethering.isTetheringSupported());
        verify(mPackageManager).getPackageInfo(PROVISIONING_APP_NAME[0], GET_ACTIVITIES);
        reset(mPackageManager);

        doThrow(PackageManager.NameNotFoundException.class).when(mPackageManager).getPackageInfo(
                PROVISIONING_APP_NAME[0], GET_ACTIVITIES);
        setupForRequiredProvisioning();
        assertFalse(mTethering.isTetheringSupported());
        verify(mPackageManager).getPackageInfo(PROVISIONING_APP_NAME[0], GET_ACTIVITIES);
    }

    @Test
    public void testUpdateConnectedClients() throws Exception {
        TestTetheringEventCallback callback = new TestTetheringEventCallback();
        runAsShell(NETWORK_SETTINGS, () -> {
            mTethering.registerTetheringEventCallback(callback);
            mLooper.dispatchAll();
        });
        callback.expectTetheredClientChanged(Collections.emptyList());

        IDhcpEventCallbacks eventCallbacks;
        final ArgumentCaptor<IDhcpEventCallbacks> dhcpEventCbsCaptor =
                 ArgumentCaptor.forClass(IDhcpEventCallbacks.class);
        // Run local only tethering.
        mTethering.interfaceStatusChanged(TEST_P2P_IFNAME, true);
        sendWifiP2pConnectionChanged(true, true, TEST_P2P_IFNAME);
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS)).startWithCallbacks(
                any(), dhcpEventCbsCaptor.capture());
        eventCallbacks = dhcpEventCbsCaptor.getValue();
        // Update lease for local only tethering.
        final MacAddress testMac1 = MacAddress.fromString("11:11:11:11:11:11");
        final ArrayList<DhcpLeaseParcelable> p2pLeases = new ArrayList<>();
        p2pLeases.add(createDhcpLeaseParcelable("clientId1", testMac1, "192.168.50.24", 24,
                Long.MAX_VALUE, "test1"));
        notifyDhcpLeasesChanged(p2pLeases, eventCallbacks);
        final List<TetheredClient> clients = toTetheredClients(p2pLeases, TETHERING_WIFI_P2P);
        callback.expectTetheredClientChanged(clients);
        reset(mDhcpServer);

        // Run wifi tethering.
        mTethering.interfaceStatusChanged(TEST_WLAN_IFNAME, true);
        sendWifiApStateChanged(WIFI_AP_STATE_ENABLED, TEST_WLAN_IFNAME, IFACE_IP_MODE_TETHERED);
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS)).startWithCallbacks(
                any(), dhcpEventCbsCaptor.capture());
        eventCallbacks = dhcpEventCbsCaptor.getValue();
        // Update mac address from softAp callback before getting dhcp lease.
        final ArrayList<WifiClient> wifiClients = new ArrayList<>();
        final MacAddress testMac2 = MacAddress.fromString("22:22:22:22:22:22");
        final WifiClient testClient = mock(WifiClient.class);
        when(testClient.getMacAddress()).thenReturn(testMac2);
        wifiClients.add(testClient);
        mSoftApCallback.onConnectedClientsChanged(wifiClients);
        final TetheredClient noAddrClient = new TetheredClient(testMac2,
                Collections.emptyList() /* addresses */, TETHERING_WIFI);
        clients.add(noAddrClient);
        callback.expectTetheredClientChanged(clients);

        // Update dhcp lease for wifi tethering.
        clients.remove(noAddrClient);
        final ArrayList<DhcpLeaseParcelable> wifiLeases = new ArrayList<>();
        wifiLeases.add(createDhcpLeaseParcelable("clientId2", testMac2, "192.168.43.24", 24,
                Long.MAX_VALUE, "test2"));
        notifyDhcpLeasesChanged(wifiLeases, eventCallbacks);
        clients.addAll(toTetheredClients(wifiLeases, TETHERING_WIFI));
        callback.expectTetheredClientChanged(clients);

        // Test onStarted callback that register second callback when tethering is running.
        TestTetheringEventCallback callback2 = new TestTetheringEventCallback();
        runAsShell(NETWORK_SETTINGS, () -> {
            mTethering.registerTetheringEventCallback(callback2);
            mLooper.dispatchAll();
        });
        callback2.expectTetheredClientChanged(clients);
    }

    private void notifyDhcpLeasesChanged(List<DhcpLeaseParcelable> leaseParcelables,
            IDhcpEventCallbacks callback) throws Exception {
        callback.onLeasesChanged(leaseParcelables);
        mLooper.dispatchAll();
    }

    private List<TetheredClient> toTetheredClients(List<DhcpLeaseParcelable> leaseParcelables,
            int type) throws Exception {
        final ArrayList<TetheredClient> leases = new ArrayList<>();
        for (DhcpLeaseParcelable lease : leaseParcelables) {
            final LinkAddress address = new LinkAddress(
                    intToInet4AddressHTH(lease.netAddr), lease.prefixLength,
                    0 /* flags */, RT_SCOPE_UNIVERSE /* as per RFC6724#3.2 */,
                    lease.expTime /* deprecationTime */, lease.expTime /* expirationTime */);

            final MacAddress macAddress = MacAddress.fromBytes(lease.hwAddr);

            final AddressInfo addressInfo = new TetheredClient.AddressInfo(address, lease.hostname);
            leases.add(new TetheredClient(
                    macAddress,
                    Collections.singletonList(addressInfo),
                    type));
        }

        return leases;
    }

    private DhcpLeaseParcelable createDhcpLeaseParcelable(final String clientId,
            final MacAddress hwAddr, final String netAddr, final int prefixLength,
            final long expTime, final String hostname) throws Exception {
        final DhcpLeaseParcelable lease = new DhcpLeaseParcelable();
        lease.clientId = clientId.getBytes();
        lease.hwAddr = hwAddr.toByteArray();
        lease.netAddr = inet4AddressToIntHTH(
                (Inet4Address) InetAddresses.parseNumericAddress(netAddr));
        lease.prefixLength = prefixLength;
        lease.expTime = expTime;
        lease.hostname = hostname;

        return lease;
    }

    @Test
    public void testBluetoothTethering() throws Exception {
        final ResultListener result = new ResultListener(TETHER_ERROR_NO_ERROR);
        when(mBluetoothAdapter.isEnabled()).thenReturn(true);
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_BLUETOOTH), result);
        mLooper.dispatchAll();
        verifySetBluetoothTethering(true);
        result.assertHasResult();

        mTethering.interfaceAdded(TEST_BT_IFNAME);
        mLooper.dispatchAll();

        mTethering.interfaceStatusChanged(TEST_BT_IFNAME, false);
        mTethering.interfaceStatusChanged(TEST_BT_IFNAME, true);
        final ResultListener tetherResult = new ResultListener(TETHER_ERROR_NO_ERROR);
        mTethering.tether(TEST_BT_IFNAME, IpServer.STATE_TETHERED, tetherResult);
        mLooper.dispatchAll();
        tetherResult.assertHasResult();

        verify(mNetd).tetherInterfaceAdd(TEST_BT_IFNAME);
        verify(mNetd).networkAddInterface(INetd.LOCAL_NET_ID, TEST_BT_IFNAME);
        verify(mNetd, times(2)).networkAddRoute(eq(INetd.LOCAL_NET_ID), eq(TEST_BT_IFNAME),
                anyString(), anyString());
        verify(mNetd).ipfwdEnableForwarding(TETHERING_NAME);
        verify(mNetd).tetherStartWithConfiguration(any());
        verify(mNetd, times(2)).networkAddRoute(eq(INetd.LOCAL_NET_ID), eq(TEST_BT_IFNAME),
                anyString(), anyString());
        verifyNoMoreInteractions(mNetd);
        reset(mNetd);

        when(mBluetoothAdapter.isEnabled()).thenReturn(true);
        mTethering.stopTethering(TETHERING_BLUETOOTH);
        mLooper.dispatchAll();
        final ResultListener untetherResult = new ResultListener(TETHER_ERROR_NO_ERROR);
        mTethering.untether(TEST_BT_IFNAME, untetherResult);
        mLooper.dispatchAll();
        untetherResult.assertHasResult();
        verifySetBluetoothTethering(false);

        verify(mNetd).tetherApplyDnsInterfaces();
        verify(mNetd).tetherInterfaceRemove(TEST_BT_IFNAME);
        verify(mNetd).networkRemoveInterface(INetd.LOCAL_NET_ID, TEST_BT_IFNAME);
        verify(mNetd).interfaceSetCfg(any(InterfaceConfigurationParcel.class));
        verify(mNetd).tetherStop();
        verify(mNetd).ipfwdDisableForwarding(TETHERING_NAME);
        verifyNoMoreInteractions(mNetd);
    }

    private void verifySetBluetoothTethering(final boolean enable) {
        final ArgumentCaptor<ServiceListener> listenerCaptor =
                ArgumentCaptor.forClass(ServiceListener.class);
        verify(mBluetoothAdapter).isEnabled();
        verify(mBluetoothAdapter).getProfileProxy(eq(mServiceContext), listenerCaptor.capture(),
                eq(BluetoothProfile.PAN));
        final ServiceListener listener = listenerCaptor.getValue();
        when(mBluetoothPan.isTetheringOn()).thenReturn(enable);
        listener.onServiceConnected(BluetoothProfile.PAN, mBluetoothPan);
        verify(mBluetoothPan).setBluetoothTethering(enable);
        verify(mBluetoothPan).isTetheringOn();
        verify(mBluetoothAdapter).closeProfileProxy(eq(BluetoothProfile.PAN), eq(mBluetoothPan));
        verifyNoMoreInteractions(mBluetoothAdapter, mBluetoothPan);
        reset(mBluetoothAdapter, mBluetoothPan);
    }

    private void runDualStackUsbTethering(final String expectedIface) throws Exception {
        when(mNetd.interfaceGetList()).thenReturn(new String[] {expectedIface});
        when(mRouterAdvertisementDaemon.start())
                .thenReturn(true);
        final UpstreamNetworkState upstreamState = buildMobileDualStackUpstreamState();
        runUsbTethering(upstreamState);

        verify(mNetd).interfaceGetList();
        verify(mNetd).tetherAddForward(expectedIface, TEST_MOBILE_IFNAME);
        verify(mNetd).ipfwdAddInterfaceForward(expectedIface, TEST_MOBILE_IFNAME);

        verify(mRouterAdvertisementDaemon).start();
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS)).startWithCallbacks(
                any(), any());
        sendIPv6TetherUpdates(upstreamState);
        assertSetIfaceToDadProxy(1 /* numOfCalls */, TEST_MOBILE_IFNAME /* ifaceName */);
        verify(mRouterAdvertisementDaemon).buildNewRa(any(), notNull());
        verify(mNetd).tetherApplyDnsInterfaces();
    }

    private void forceUsbTetheringUse(final int function) {
        Settings.Global.putInt(mContentResolver, TETHER_FORCE_USB_FUNCTIONS, function);
        final ContentObserver observer = mTethering.getSettingsObserverForTest();
        observer.onChange(false /* selfChange */);
        mLooper.dispatchAll();
    }

    private void verifyUsbTetheringStopDueToSettingChange(final String iface) {
        verify(mUsbManager, times(2)).setCurrentFunctions(UsbManager.FUNCTION_NONE);
        mTethering.interfaceRemoved(iface);
        sendUsbBroadcast(true, true, -1 /* no functions enabled */);
        reset(mUsbManager, mNetd, mDhcpServer, mRouterAdvertisementDaemon,
                mIPv6TetheringCoordinator, mDadProxy);
    }

    @Test
    public void testUsbFunctionConfigurationChange() throws Exception {
        // Run TETHERING_NCM.
        runNcmTethering();
        verify(mDhcpServer, timeout(DHCPSERVER_START_TIMEOUT_MS).times(1)).startWithCallbacks(
                any(), any());

        // Change the USB tethering function to NCM. Because the USB tethering function was set to
        // RNDIS (the default), tethering is stopped.
        forceUsbTetheringUse(TETHER_USB_NCM_FUNCTION);
        verifyUsbTetheringStopDueToSettingChange(TEST_NCM_IFNAME);

        // If TETHERING_USB is forced to use ncm function, TETHERING_NCM would no longer be
        // available.
        final ResultListener ncmResult = new ResultListener(TETHER_ERROR_SERVICE_UNAVAIL);
        mTethering.startTethering(createTetheringRequestParcel(TETHERING_NCM), ncmResult);
        mLooper.dispatchAll();
        ncmResult.assertHasResult();

        // Run TETHERING_USB with ncm configuration.
        runDualStackUsbTethering(TEST_NCM_IFNAME);

        // Change configuration to rndis.
        forceUsbTetheringUse(TETHER_USB_RNDIS_FUNCTION);
        verifyUsbTetheringStopDueToSettingChange(TEST_NCM_IFNAME);

        // Run TETHERING_USB with rndis configuration.
        runDualStackUsbTethering(TEST_RNDIS_IFNAME);
        runStopUSBTethering();
    }
    // TODO: Test that a request for hotspot mode doesn't interfere with an
    // already operating tethering mode interface.
}
