/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.net.cts;

import static android.Manifest.permission.CONNECTIVITY_INTERNAL;
import static android.Manifest.permission.CONNECTIVITY_USE_RESTRICTED_NETWORKS;
import static android.Manifest.permission.NETWORK_SETTINGS;
import static android.Manifest.permission.READ_DEVICE_CONFIG;
import static android.content.pm.PackageManager.FEATURE_BLUETOOTH;
import static android.content.pm.PackageManager.FEATURE_ETHERNET;
import static android.content.pm.PackageManager.FEATURE_TELEPHONY;
import static android.content.pm.PackageManager.FEATURE_USB_HOST;
import static android.content.pm.PackageManager.FEATURE_WATCH;
import static android.content.pm.PackageManager.FEATURE_WIFI;
import static android.content.pm.PackageManager.FEATURE_WIFI_DIRECT;
import static android.content.pm.PackageManager.GET_PERMISSIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.net.ConnectivityManager.EXTRA_NETWORK;
import static android.net.ConnectivityManager.EXTRA_NETWORK_REQUEST;
import static android.net.ConnectivityManager.PROFILE_NETWORK_PREFERENCE_ENTERPRISE;
import static android.net.ConnectivityManager.TYPE_BLUETOOTH;
import static android.net.ConnectivityManager.TYPE_ETHERNET;
import static android.net.ConnectivityManager.TYPE_MOBILE_CBS;
import static android.net.ConnectivityManager.TYPE_MOBILE_DUN;
import static android.net.ConnectivityManager.TYPE_MOBILE_EMERGENCY;
import static android.net.ConnectivityManager.TYPE_MOBILE_FOTA;
import static android.net.ConnectivityManager.TYPE_MOBILE_HIPRI;
import static android.net.ConnectivityManager.TYPE_MOBILE_IA;
import static android.net.ConnectivityManager.TYPE_MOBILE_IMS;
import static android.net.ConnectivityManager.TYPE_MOBILE_MMS;
import static android.net.ConnectivityManager.TYPE_MOBILE_SUPL;
import static android.net.ConnectivityManager.TYPE_PROXY;
import static android.net.ConnectivityManager.TYPE_VPN;
import static android.net.ConnectivityManager.TYPE_WIFI_P2P;
import static android.net.NetworkCapabilities.NET_CAPABILITY_FOREGROUND;
import static android.net.NetworkCapabilities.NET_CAPABILITY_IMS;
import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.NetworkCapabilities.NET_CAPABILITY_NOT_METERED;
import static android.net.NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED;
import static android.net.NetworkCapabilities.NET_CAPABILITY_PARTIAL_CONNECTIVITY;
import static android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED;
import static android.net.NetworkCapabilities.TRANSPORT_TEST;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;
import static android.net.cts.util.CtsNetUtils.ConnectivityActionReceiver;
import static android.net.cts.util.CtsNetUtils.HTTP_PORT;
import static android.net.cts.util.CtsNetUtils.NETWORK_CALLBACK_ACTION;
import static android.net.cts.util.CtsNetUtils.TEST_HOST;
import static android.net.cts.util.CtsNetUtils.TestNetworkCallback;
import static android.net.cts.util.CtsTetheringUtils.TestTetheringEventCallback;
import static android.net.util.NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTPS_URL;
import static android.net.util.NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTP_URL;
import static android.os.MessageQueue.OnFileDescriptorEventListener.EVENT_INPUT;
import static android.provider.Settings.Global.NETWORK_METERED_MULTIPATH_PREFERENCE;
import static android.system.OsConstants.AF_INET;
import static android.system.OsConstants.AF_INET6;
import static android.system.OsConstants.AF_UNSPEC;

import static com.android.compatibility.common.util.SystemUtil.callWithShellPermissionIdentity;
import static com.android.compatibility.common.util.SystemUtil.runShellCommand;
import static com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity;
import static com.android.modules.utils.build.SdkLevel.isAtLeastS;
import static com.android.networkstack.apishim.ConstantsShim.BLOCKED_REASON_LOCKDOWN_VPN;
import static com.android.networkstack.apishim.ConstantsShim.BLOCKED_REASON_NONE;
import static com.android.testutils.MiscAsserts.assertThrows;
import static com.android.testutils.TestNetworkTrackerKt.initTestNetwork;
import static com.android.testutils.TestPermissionUtil.runAsShell;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.annotation.NonNull;
import android.app.Instrumentation;
import android.app.PendingIntent;
import android.app.UiAutomation;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.ConnectivitySettingsManager;
import android.net.InetAddresses;
import android.net.IpSecManager;
import android.net.IpSecManager.UdpEncapsulationSocket;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkAgent;
import android.net.NetworkAgentConfig;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.NetworkInfo.State;
import android.net.NetworkProvider;
import android.net.NetworkRequest;
import android.net.NetworkScore;
import android.net.NetworkSpecifier;
import android.net.NetworkStateSnapshot;
import android.net.NetworkUtils;
import android.net.OemNetworkPreferences;
import android.net.ProxyInfo;
import android.net.SocketKeepalive;
import android.net.TelephonyNetworkSpecifier;
import android.net.TestNetworkInterface;
import android.net.TestNetworkManager;
import android.net.Uri;
import android.net.cts.util.CtsNetUtils;
import android.net.cts.util.CtsTetheringUtils;
import android.net.util.KeepaliveUtils;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.MessageQueue;
import android.os.Process;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.VintfRuntimeInfo;
import android.platform.test.annotations.AppModeFull;
import android.provider.DeviceConfig;
import android.provider.Settings;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.util.Pair;
import android.util.Range;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.internal.util.ArrayUtils;
import com.android.modules.utils.build.SdkLevel;
import com.android.networkstack.apishim.ConnectivityManagerShimImpl;
import com.android.networkstack.apishim.ConstantsShim;
import com.android.networkstack.apishim.NetworkInformationShimImpl;
import com.android.networkstack.apishim.common.ConnectivityManagerShim;
import com.android.testutils.CompatUtil;
import com.android.testutils.DevSdkIgnoreRule;
import com.android.testutils.DevSdkIgnoreRule.IgnoreUpTo;
import com.android.testutils.DevSdkIgnoreRuleKt;
import com.android.testutils.RecorderCallback.CallbackEntry;
import com.android.testutils.SkipPresubmit;
import com.android.testutils.TestHttpServer;
import com.android.testutils.TestNetworkTracker;
import com.android.testutils.TestableNetworkCallback;

import junit.framework.AssertionFailedError;

import libcore.io.Streams;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.Socket;
import java.net.URL;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Supplier;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import fi.iki.elonen.NanoHTTPD.Method;
import fi.iki.elonen.NanoHTTPD.Response.IStatus;
import fi.iki.elonen.NanoHTTPD.Response.Status;

@RunWith(AndroidJUnit4.class)
public class ConnectivityManagerTest {
    @Rule
    public final DevSdkIgnoreRule ignoreRule = new DevSdkIgnoreRule();

    private static final String TAG = ConnectivityManagerTest.class.getSimpleName();

    public static final int TYPE_MOBILE = ConnectivityManager.TYPE_MOBILE;
    public static final int TYPE_WIFI = ConnectivityManager.TYPE_WIFI;

    private static final int HOST_ADDRESS = 0x7f000001;// represent ip 127.0.0.1
    private static final int KEEPALIVE_CALLBACK_TIMEOUT_MS = 2000;
    private static final int INTERVAL_KEEPALIVE_RETRY_MS = 500;
    private static final int MAX_KEEPALIVE_RETRY_COUNT = 3;
    private static final int MIN_KEEPALIVE_INTERVAL = 10;

    private static final int NETWORK_CALLBACK_TIMEOUT_MS = 30_000;
    private static final int NO_CALLBACK_TIMEOUT_MS = 100;
    private static final int NUM_TRIES_MULTIPATH_PREF_CHECK = 20;
    private static final long INTERVAL_MULTIPATH_PREF_CHECK_MS = 500;
    // device could have only one interface: data, wifi.
    private static final int MIN_NUM_NETWORK_TYPES = 1;

    // Airplane Mode BroadcastReceiver Timeout
    private static final long AIRPLANE_MODE_CHANGE_TIMEOUT_MS = 10_000L;

    // Timeout for applying uids allowed on restricted networks
    private static final long APPLYING_UIDS_ALLOWED_ON_RESTRICTED_NETWORKS_TIMEOUT_MS = 3_000L;

    // Minimum supported keepalive counts for wifi and cellular.
    public static final int MIN_SUPPORTED_CELLULAR_KEEPALIVE_COUNT = 1;
    public static final int MIN_SUPPORTED_WIFI_KEEPALIVE_COUNT = 3;

    private static final String NETWORK_METERED_MULTIPATH_PREFERENCE_RES_NAME =
            "config_networkMeteredMultipathPreference";
    private static final String KEEPALIVE_ALLOWED_UNPRIVILEGED_RES_NAME =
            "config_allowedUnprivilegedKeepalivePerUid";
    private static final String KEEPALIVE_RESERVED_PER_SLOT_RES_NAME =
            "config_reservedPrivilegedKeepaliveSlots";

    private static final LinkAddress TEST_LINKADDR = new LinkAddress(
            InetAddresses.parseNumericAddress("2001:db8::8"), 64);

    private static final int AIRPLANE_MODE_OFF = 0;
    private static final int AIRPLANE_MODE_ON = 1;

    private static final String TEST_HTTPS_URL_PATH = "/https_path";
    private static final String TEST_HTTP_URL_PATH = "/http_path";
    private static final String LOCALHOST_HOSTNAME = "localhost";
    // Re-connecting to the AP, obtaining an IP address, revalidating can take a long time
    private static final long WIFI_CONNECT_TIMEOUT_MS = 60_000L;

    private Context mContext;
    private Instrumentation mInstrumentation;
    private ConnectivityManager mCm;
    private ConnectivityManagerShim mCmShim;
    private WifiManager mWifiManager;
    private PackageManager mPackageManager;
    private final ArraySet<Integer> mNetworkTypes = new ArraySet<>();
    private UiAutomation mUiAutomation;
    private CtsNetUtils mCtsNetUtils;

    // Used for cleanup purposes.
    private final List<Range<Integer>> mVpnRequiredUidRanges = new ArrayList<>();

    private final TestHttpServer mHttpServer = new TestHttpServer(LOCALHOST_HOSTNAME);

    @Before
    public void setUp() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mContext = mInstrumentation.getContext();
        mCm = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        mCmShim = ConnectivityManagerShimImpl.newInstance(mContext);
        mWifiManager = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
        mPackageManager = mContext.getPackageManager();
        mCtsNetUtils = new CtsNetUtils(mContext);

        if (DevSdkIgnoreRuleKt.isDevSdkInRange(null /* minExclusive */,
                Build.VERSION_CODES.R /* maxInclusive */)) {
            addLegacySupportedNetworkTypes();
        } else {
            addSupportedNetworkTypes();
        }

        mUiAutomation = mInstrumentation.getUiAutomation();

        assertNotNull("CTS requires a working Internet connection", mCm.getActiveNetwork());
    }

    private void addLegacySupportedNetworkTypes() {
        // Network type support as expected for android R-
        // Get com.android.internal.R.array.networkAttributes
        int resId = mContext.getResources().getIdentifier("networkAttributes", "array", "android");
        String[] naStrings = mContext.getResources().getStringArray(resId);
        boolean wifiOnly = SystemProperties.getBoolean("ro.radio.noril", false);
        for (String naString : naStrings) {
            try {
                final String[] splitConfig = naString.split(",");
                // Format was name,type,radio,priority,restoreTime,dependencyMet
                final int type = Integer.parseInt(splitConfig[1]);
                if (wifiOnly && ConnectivityManager.isNetworkTypeMobile(type)) {
                    continue;
                }
                mNetworkTypes.add(type);
            } catch (Exception e) {}
        }
    }

    private void addSupportedNetworkTypes() {
        final PackageManager pm = mContext.getPackageManager();
        if (pm.hasSystemFeature(FEATURE_WIFI)) {
            mNetworkTypes.add(TYPE_WIFI);
        }
        if (pm.hasSystemFeature(FEATURE_WIFI_DIRECT)) {
            mNetworkTypes.add(TYPE_WIFI_P2P);
        }
        if (mContext.getSystemService(TelephonyManager.class).isDataCapable()) {
            mNetworkTypes.add(TYPE_MOBILE);
            mNetworkTypes.add(TYPE_MOBILE_MMS);
            mNetworkTypes.add(TYPE_MOBILE_SUPL);
            mNetworkTypes.add(TYPE_MOBILE_DUN);
            mNetworkTypes.add(TYPE_MOBILE_HIPRI);
            mNetworkTypes.add(TYPE_MOBILE_FOTA);
            mNetworkTypes.add(TYPE_MOBILE_IMS);
            mNetworkTypes.add(TYPE_MOBILE_CBS);
            mNetworkTypes.add(TYPE_MOBILE_IA);
            mNetworkTypes.add(TYPE_MOBILE_EMERGENCY);
        }
        if (pm.hasSystemFeature(FEATURE_BLUETOOTH)) {
            mNetworkTypes.add(TYPE_BLUETOOTH);
        }
        if (pm.hasSystemFeature(FEATURE_WATCH)) {
            mNetworkTypes.add(TYPE_PROXY);
        }
        if (mContext.getSystemService(Context.ETHERNET_SERVICE) != null) {
            mNetworkTypes.add(TYPE_ETHERNET);
        }
        mNetworkTypes.add(TYPE_VPN);
    }

    @After
    public void tearDown() throws Exception {
        // Release any NetworkRequests filed to connect mobile data.
        if (mCtsNetUtils.cellConnectAttempted()) {
            mCtsNetUtils.disconnectFromCell();
        }

        if (TestUtils.shouldTestSApis()) {
            runWithShellPermissionIdentity(
                    () -> mCmShim.setRequireVpnForUids(false, mVpnRequiredUidRanges),
                    NETWORK_SETTINGS);
        }

        // All tests in this class require a working Internet connection as they start. Make
        // sure there is still one as they end that's ready to use for the next test to use.
        final TestNetworkCallback callback = new TestNetworkCallback();
        mCm.registerDefaultNetworkCallback(callback);
        try {
            assertNotNull("Couldn't restore Internet connectivity", callback.waitForAvailable());
        } finally {
            mCm.unregisterNetworkCallback(callback);
        }
    }

    @Test
    public void testIsNetworkTypeValid() {
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_WIFI));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_MMS));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_SUPL));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_DUN));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_HIPRI));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_WIMAX));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_BLUETOOTH));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_DUMMY));
        assertTrue(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_ETHERNET));
        assertTrue(mCm.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_FOTA));
        assertTrue(mCm.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_IMS));
        assertTrue(mCm.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_CBS));
        assertTrue(mCm.isNetworkTypeValid(ConnectivityManager.TYPE_WIFI_P2P));
        assertTrue(mCm.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE_IA));
        assertFalse(mCm.isNetworkTypeValid(-1));
        assertTrue(mCm.isNetworkTypeValid(0));
        assertTrue(mCm.isNetworkTypeValid(ConnectivityManager.MAX_NETWORK_TYPE));
        assertFalse(ConnectivityManager.isNetworkTypeValid(ConnectivityManager.MAX_NETWORK_TYPE+1));

        NetworkInfo[] ni = mCm.getAllNetworkInfo();

        for (NetworkInfo n: ni) {
            assertTrue(ConnectivityManager.isNetworkTypeValid(n.getType()));
        }

    }

    @Test
    public void testSetNetworkPreference() {
        // getNetworkPreference() and setNetworkPreference() are both deprecated so they do
        // not preform any action.  Verify they are at least still callable.
        mCm.setNetworkPreference(mCm.getNetworkPreference());
    }

    @Test
    public void testGetActiveNetworkInfo() {
        NetworkInfo ni = mCm.getActiveNetworkInfo();

        assertNotNull("You must have an active network connection to complete CTS", ni);
        assertTrue(ConnectivityManager.isNetworkTypeValid(ni.getType()));
        assertTrue(ni.getState() == State.CONNECTED);
    }

    @Test
    public void testGetActiveNetwork() {
        Network network = mCm.getActiveNetwork();
        assertNotNull("You must have an active network connection to complete CTS", network);

        NetworkInfo ni = mCm.getNetworkInfo(network);
        assertNotNull("Network returned from getActiveNetwork was invalid", ni);

        // Similar to testGetActiveNetworkInfo above.
        assertTrue(ConnectivityManager.isNetworkTypeValid(ni.getType()));
        assertTrue(ni.getState() == State.CONNECTED);
    }

    @Test
    public void testGetNetworkInfo() {
        for (int type = -1; type <= ConnectivityManager.MAX_NETWORK_TYPE+1; type++) {
            if (shouldBeSupported(type)) {
                NetworkInfo ni = mCm.getNetworkInfo(type);
                assertTrue("Info shouldn't be null for " + type, ni != null);
                State state = ni.getState();
                assertTrue("Bad state for " + type, State.UNKNOWN.ordinal() >= state.ordinal()
                           && state.ordinal() >= State.CONNECTING.ordinal());
                DetailedState ds = ni.getDetailedState();
                assertTrue("Bad detailed state for " + type,
                           DetailedState.FAILED.ordinal() >= ds.ordinal()
                           && ds.ordinal() >= DetailedState.IDLE.ordinal());
            } else {
                assertNull("Info should be null for " + type, mCm.getNetworkInfo(type));
            }
        }
    }

    @Test
    public void testGetAllNetworkInfo() {
        NetworkInfo[] ni = mCm.getAllNetworkInfo();
        assertTrue(ni.length >= MIN_NUM_NETWORK_TYPES);
        for (int type = 0; type <= ConnectivityManager.MAX_NETWORK_TYPE; type++) {
            int desiredFoundCount = (shouldBeSupported(type) ? 1 : 0);
            int foundCount = 0;
            for (NetworkInfo i : ni) {
                if (i.getType() == type) foundCount++;
            }
            if (foundCount != desiredFoundCount) {
                Log.e(TAG, "failure in testGetAllNetworkInfo.  Dump of returned NetworkInfos:");
                for (NetworkInfo networkInfo : ni) Log.e(TAG, "  " + networkInfo);
            }
            assertTrue("Unexpected foundCount of " + foundCount + " for type " + type,
                    foundCount == desiredFoundCount);
        }
    }

    private String getSubscriberIdForCellNetwork(Network cellNetwork) {
        final NetworkCapabilities cellCaps = mCm.getNetworkCapabilities(cellNetwork);
        final NetworkSpecifier specifier = cellCaps.getNetworkSpecifier();
        assertTrue(specifier instanceof TelephonyNetworkSpecifier);
        // Get subscription from Telephony network specifier.
        final int subId = ((TelephonyNetworkSpecifier) specifier).getSubscriptionId();
        assertNotEquals(SubscriptionManager.INVALID_SUBSCRIPTION_ID, subId);

        // Get subscriber Id from telephony manager.
        final TelephonyManager tm = mContext.getSystemService(TelephonyManager.class);
        return runWithShellPermissionIdentity(() -> tm.getSubscriberId(subId),
                android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE);
    }

    @AppModeFull(reason = "Cannot request network in instant app mode")
    @DevSdkIgnoreRule.IgnoreUpTo(Build.VERSION_CODES.R)
    @Test
    public void testGetAllNetworkStateSnapshots()
            throws InterruptedException {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_TELEPHONY));
        // Make sure cell is active to retrieve IMSI for verification in later step.
        final Network cellNetwork = mCtsNetUtils.connectToCell();
        final String subscriberId = getSubscriberIdForCellNetwork(cellNetwork);
        assertFalse(TextUtils.isEmpty(subscriberId));

        // Verify the API cannot be called without proper permission.
        assertThrows(SecurityException.class, () -> mCm.getAllNetworkStateSnapshots());

        // Get all networks, verify the result of getAllNetworkStateSnapshots matches the result
        // got from other APIs.
        final Network[] networks = mCm.getAllNetworks();
        assertGreaterOrEqual(networks.length, 1);
        final List<NetworkStateSnapshot> snapshots = runWithShellPermissionIdentity(
                () -> mCm.getAllNetworkStateSnapshots(), NETWORK_SETTINGS);
        assertEquals(networks.length, snapshots.size());
        for (final Network network : networks) {
            // Can't use a lambda because it will cause the test to crash on R with
            // NoClassDefFoundError.
            NetworkStateSnapshot snapshot = null;
            for (NetworkStateSnapshot item : snapshots) {
                if (item.getNetwork().equals(network)) {
                    snapshot = item;
                    break;
                }
            }
            assertNotNull(snapshot);
            final NetworkCapabilities caps =
                    Objects.requireNonNull(mCm.getNetworkCapabilities(network));
            // Redact specifier of the capabilities of the snapshot before comparing since
            // the result returned from getNetworkCapabilities always get redacted.
            final NetworkSpecifier snapshotCapSpecifier =
                    snapshot.getNetworkCapabilities().getNetworkSpecifier();
            final NetworkSpecifier redactedSnapshotCapSpecifier =
                    snapshotCapSpecifier == null ? null : snapshotCapSpecifier.redact();
            assertEquals("", caps.describeImmutableDifferences(
                    snapshot.getNetworkCapabilities()
                            .setNetworkSpecifier(redactedSnapshotCapSpecifier)));
            assertEquals(mCm.getLinkProperties(network), snapshot.getLinkProperties());
            assertEquals(mCm.getNetworkInfo(network).getType(), snapshot.getLegacyType());

            if (network.equals(cellNetwork)) {
                assertEquals(subscriberId, snapshot.getSubscriberId());
            }
        }
    }

    /**
     * Tests that connections can be opened on WiFi and cellphone networks,
     * and that they are made from different IP addresses.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    @SkipPresubmit(reason = "Virtual devices use a single internet connection for all networks")
    public void testOpenConnection() throws Exception {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_TELEPHONY));

        Network wifiNetwork = mCtsNetUtils.connectToWifi();
        Network cellNetwork = mCtsNetUtils.connectToCell();
        // This server returns the requestor's IP address as the response body.
        URL url = new URL("http://google-ipv6test.appspot.com/ip.js?fmt=text");
        String wifiAddressString = httpGet(wifiNetwork, url);
        String cellAddressString = httpGet(cellNetwork, url);

        assertFalse(String.format("Same address '%s' on two different networks (%s, %s)",
                wifiAddressString, wifiNetwork, cellNetwork),
                wifiAddressString.equals(cellAddressString));

        // Verify that the IP addresses that the requests appeared to come from are actually on the
        // respective networks.
        assertOnNetwork(wifiAddressString, wifiNetwork);
        assertOnNetwork(cellAddressString, cellNetwork);

        assertFalse("Unexpectedly equal: " + wifiNetwork, wifiNetwork.equals(cellNetwork));
    }

    /**
     * Performs a HTTP GET to the specified URL on the specified Network, and returns
     * the response body decoded as UTF-8.
     */
    private static String httpGet(Network network, URL httpUrl) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) network.openConnection(httpUrl);
        try {
            InputStream inputStream = connection.getInputStream();
            return Streams.readFully(new InputStreamReader(inputStream, StandardCharsets.UTF_8));
        } finally {
            connection.disconnect();
        }
    }

    private void assertOnNetwork(String adressString, Network network) throws UnknownHostException {
        InetAddress address = InetAddress.getByName(adressString);
        LinkProperties linkProperties = mCm.getLinkProperties(network);
        // To make sure that the request went out on the right network, check that
        // the IP address seen by the server is assigned to the expected network.
        // We can only do this for IPv6 addresses, because in IPv4 we will likely
        // have a private IPv4 address, and that won't match what the server sees.
        if (address instanceof Inet6Address) {
            assertContains(linkProperties.getAddresses(), address);
        }
    }

    private static<T> void assertContains(Collection<T> collection, T element) {
        assertTrue(element + " not found in " + collection, collection.contains(element));
    }

    private void assertStartUsingNetworkFeatureUnsupported(int networkType, String feature) {
        try {
            mCm.startUsingNetworkFeature(networkType, feature);
            fail("startUsingNetworkFeature is no longer supported in the current API version");
        } catch (UnsupportedOperationException expected) {}
    }

    private void assertStopUsingNetworkFeatureUnsupported(int networkType, String feature) {
        try {
            mCm.startUsingNetworkFeature(networkType, feature);
            fail("stopUsingNetworkFeature is no longer supported in the current API version");
        } catch (UnsupportedOperationException expected) {}
    }

    private void assertRequestRouteToHostUnsupported(int networkType, int hostAddress) {
        try {
            mCm.requestRouteToHost(networkType, hostAddress);
            fail("requestRouteToHost is no longer supported in the current API version");
        } catch (UnsupportedOperationException expected) {}
    }

    @Test
    public void testStartUsingNetworkFeature() {

        final String invalidateFeature = "invalidateFeature";
        final String mmsFeature = "enableMMS";

        assertStartUsingNetworkFeatureUnsupported(TYPE_MOBILE, invalidateFeature);
        assertStopUsingNetworkFeatureUnsupported(TYPE_MOBILE, invalidateFeature);
        assertStartUsingNetworkFeatureUnsupported(TYPE_WIFI, mmsFeature);
    }

    private boolean shouldEthernetBeSupported() {
        // Instant mode apps aren't allowed to query the Ethernet service due to selinux policies.
        // When in instant mode, don't fail if the Ethernet service is available. Instead, rely on
        // the fact that Ethernet should be supported if the device has a hardware Ethernet port, or
        // if the device can be a USB host and thus can use USB Ethernet adapters.
        //
        // Note that this test this will still fail in instant mode if a device supports Ethernet
        // via other hardware means. We are not currently aware of any such device.
        return (mContext.getSystemService(Context.ETHERNET_SERVICE) != null) ||
            mPackageManager.hasSystemFeature(FEATURE_ETHERNET) ||
            mPackageManager.hasSystemFeature(FEATURE_USB_HOST);
    }

    private boolean shouldBeSupported(int networkType) {
        return mNetworkTypes.contains(networkType)
            || (networkType == ConnectivityManager.TYPE_VPN)
            || (networkType == ConnectivityManager.TYPE_ETHERNET && shouldEthernetBeSupported());
    }

    @Test
    public void testIsNetworkSupported() {
        for (int type = -1; type <= ConnectivityManager.MAX_NETWORK_TYPE; type++) {
            boolean supported = mCm.isNetworkSupported(type);
            if (shouldBeSupported(type)) {
                assertTrue("Network type " + type + " should be supported", supported);
            } else {
                assertFalse("Network type " + type + " should not be supported", supported);
            }
        }
    }

    @Test
    public void testRequestRouteToHost() {
        for (int type = -1 ; type <= ConnectivityManager.MAX_NETWORK_TYPE; type++) {
            assertRequestRouteToHostUnsupported(type, HOST_ADDRESS);
        }
    }

    @Test
    public void testTest() {
        mCm.getBackgroundDataSetting();
    }

    private NetworkRequest makeDefaultRequest() {
        // Make a request that is similar to the way framework tracks the system
        // default network.
        return new NetworkRequest.Builder()
                .clearCapabilities()
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_TRUSTED)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build();
    }

    private NetworkRequest makeWifiNetworkRequest() {
        return new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                .addCapability(NET_CAPABILITY_INTERNET)
                .build();
    }

    private NetworkRequest makeCellNetworkRequest() {
        return new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR)
                .addCapability(NET_CAPABILITY_INTERNET)
                .build();
    }

    @AppModeFull(reason = "WRITE_SECURE_SETTINGS permission can't be granted to instant apps")
    @Test @IgnoreUpTo(Build.VERSION_CODES.Q)
    public void testIsPrivateDnsBroken() throws InterruptedException {
        final String invalidPrivateDnsServer = "invalidhostname.example.com";
        final String goodPrivateDnsServer = "dns.google";
        mCtsNetUtils.storePrivateDnsSetting();
        final TestableNetworkCallback cb = new TestableNetworkCallback();
        mCm.registerNetworkCallback(makeWifiNetworkRequest(), cb);
        try {
            // Verifying the good private DNS sever
            mCtsNetUtils.setPrivateDnsStrictMode(goodPrivateDnsServer);
            final Network networkForPrivateDns =  mCtsNetUtils.ensureWifiConnected();
            cb.eventuallyExpect(CallbackEntry.NETWORK_CAPS_UPDATED, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> (!((CallbackEntry.CapabilitiesChanged) entry).getCaps()
                    .isPrivateDnsBroken()) && networkForPrivateDns.equals(entry.getNetwork()));

            // Verifying the broken private DNS sever
            mCtsNetUtils.setPrivateDnsStrictMode(invalidPrivateDnsServer);
            cb.eventuallyExpect(CallbackEntry.NETWORK_CAPS_UPDATED, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> (((CallbackEntry.CapabilitiesChanged) entry).getCaps()
                    .isPrivateDnsBroken()) && networkForPrivateDns.equals(entry.getNetwork()));
        } finally {
            mCtsNetUtils.restorePrivateDnsSetting();
            // Toggle wifi to make sure it is re-validated
            reconnectWifi();
        }
    }

    /**
     * Exercises both registerNetworkCallback and unregisterNetworkCallback. This checks to
     * see if we get a callback for the TRANSPORT_WIFI transport type being available.
     *
     * <p>In order to test that a NetworkCallback occurs, we need some change in the network
     * state (either a transport or capability is now available). The most straightforward is
     * WiFi. We could add a version that uses the telephony data connection but it's not clear
     * that it would increase test coverage by much (how many devices have 3G radio but not Wifi?).
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testRegisterNetworkCallback() {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        // We will register for a WIFI network being available or lost.
        final TestNetworkCallback callback = new TestNetworkCallback();
        mCm.registerNetworkCallback(makeWifiNetworkRequest(), callback);

        final TestNetworkCallback defaultTrackingCallback = new TestNetworkCallback();
        mCm.registerDefaultNetworkCallback(defaultTrackingCallback);

        final TestNetworkCallback systemDefaultCallback = new TestNetworkCallback();
        final TestNetworkCallback perUidCallback = new TestNetworkCallback();
        final TestNetworkCallback bestMatchingCallback = new TestNetworkCallback();
        final Handler h = new Handler(Looper.getMainLooper());
        if (TestUtils.shouldTestSApis()) {
            runWithShellPermissionIdentity(() -> {
                mCmShim.registerSystemDefaultNetworkCallback(systemDefaultCallback, h);
                mCmShim.registerDefaultNetworkCallbackForUid(Process.myUid(), perUidCallback, h);
            }, NETWORK_SETTINGS);
            mCm.registerBestMatchingNetworkCallback(makeDefaultRequest(), bestMatchingCallback, h);
        }

        Network wifiNetwork = null;

        try {
            mCtsNetUtils.ensureWifiConnected();

            // Now we should expect to get a network callback about availability of the wifi
            // network even if it was already connected as a state-based action when the callback
            // is registered.
            wifiNetwork = callback.waitForAvailable();
            assertNotNull("Did not receive onAvailable for TRANSPORT_WIFI request",
                    wifiNetwork);

            final Network defaultNetwork = defaultTrackingCallback.waitForAvailable();
            assertNotNull("Did not receive onAvailable on default network callback",
                    defaultNetwork);

            if (TestUtils.shouldTestSApis()) {
                assertNotNull("Did not receive onAvailable on system default network callback",
                        systemDefaultCallback.waitForAvailable());
                final Network perUidNetwork = perUidCallback.waitForAvailable();
                assertNotNull("Did not receive onAvailable on per-UID default network callback",
                        perUidNetwork);
                assertEquals(defaultNetwork, perUidNetwork);
                final Network bestMatchingNetwork = bestMatchingCallback.waitForAvailable();
                assertNotNull("Did not receive onAvailable on best matching network callback",
                        bestMatchingNetwork);
                assertEquals(defaultNetwork, bestMatchingNetwork);
            }

        } catch (InterruptedException e) {
            fail("Broadcast receiver or NetworkCallback wait was interrupted.");
        } finally {
            mCm.unregisterNetworkCallback(callback);
            mCm.unregisterNetworkCallback(defaultTrackingCallback);
            if (TestUtils.shouldTestSApis()) {
                mCm.unregisterNetworkCallback(systemDefaultCallback);
                mCm.unregisterNetworkCallback(perUidCallback);
                mCm.unregisterNetworkCallback(bestMatchingCallback);
            }
        }
    }

    /**
     * Tests both registerNetworkCallback and unregisterNetworkCallback similarly to
     * {@link #testRegisterNetworkCallback} except that a {@code PendingIntent} is used instead
     * of a {@code NetworkCallback}.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testRegisterNetworkCallback_withPendingIntent() {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        // Create a ConnectivityActionReceiver that has an IntentFilter for our locally defined
        // action, NETWORK_CALLBACK_ACTION.
        final IntentFilter filter = new IntentFilter();
        filter.addAction(NETWORK_CALLBACK_ACTION);

        final ConnectivityActionReceiver receiver = new ConnectivityActionReceiver(
                mCm, ConnectivityManager.TYPE_WIFI, NetworkInfo.State.CONNECTED);
        mContext.registerReceiver(receiver, filter);

        // Create a broadcast PendingIntent for NETWORK_CALLBACK_ACTION.
        final Intent intent = new Intent(NETWORK_CALLBACK_ACTION)
                .setPackage(mContext.getPackageName());
        // While ConnectivityService would put extra info such as network or request id before
        // broadcasting the inner intent. The MUTABLE flag needs to be added accordingly.
        // TODO: replace with PendingIntent.FLAG_MUTABLE when this code compiles against S+ or
        //  shims.
        final int pendingIntentFlagMutable = 1 << 25;
        final PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0 /*requestCode*/,
                intent, PendingIntent.FLAG_CANCEL_CURRENT | pendingIntentFlagMutable);

        // We will register for a WIFI network being available or lost.
        mCm.registerNetworkCallback(makeWifiNetworkRequest(), pendingIntent);

        try {
            mCtsNetUtils.ensureWifiConnected();

            // Now we expect to get the Intent delivered notifying of the availability of the wifi
            // network even if it was already connected as a state-based action when the callback
            // is registered.
            assertTrue("Did not receive expected Intent " + intent + " for TRANSPORT_WIFI",
                    receiver.waitForState());
        } catch (InterruptedException e) {
            fail("Broadcast receiver or NetworkCallback wait was interrupted.");
        } finally {
            mCm.unregisterNetworkCallback(pendingIntent);
            pendingIntent.cancel();
            mContext.unregisterReceiver(receiver);
        }
    }

    private void runIdenticalPendingIntentsRequestTest(boolean useListen) throws Exception {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        // Disconnect before registering callbacks, reconnect later to fire them
        mCtsNetUtils.ensureWifiDisconnected(null);

        final NetworkRequest firstRequest = makeWifiNetworkRequest();
        final NetworkRequest secondRequest = new NetworkRequest(firstRequest);
        // Will match wifi or test, since transports are ORed; but there should only be wifi
        secondRequest.networkCapabilities.addTransportType(TRANSPORT_TEST);

        PendingIntent firstIntent = null;
        PendingIntent secondIntent = null;
        BroadcastReceiver receiver = null;

        // Avoid receiving broadcasts from other runs by appending a timestamp
        final String broadcastAction = NETWORK_CALLBACK_ACTION + System.currentTimeMillis();
        try {
            // TODO: replace with PendingIntent.FLAG_MUTABLE when this code compiles against S+
            // Intent is mutable to receive EXTRA_NETWORK_REQUEST from ConnectivityService
            final int pendingIntentFlagMutable = 1 << 25;
            final String extraBoolKey = "extra_bool";
            firstIntent = PendingIntent.getBroadcast(mContext,
                    0 /* requestCode */,
                    new Intent(broadcastAction).putExtra(extraBoolKey, false),
                    PendingIntent.FLAG_UPDATE_CURRENT | pendingIntentFlagMutable);

            if (useListen) {
                mCm.registerNetworkCallback(firstRequest, firstIntent);
            } else {
                mCm.requestNetwork(firstRequest, firstIntent);
            }

            // Second intent equals the first as per filterEquals (extras don't count), so first
            // intent will be updated with the new extras
            secondIntent = PendingIntent.getBroadcast(mContext,
                    0 /* requestCode */,
                    new Intent(broadcastAction).putExtra(extraBoolKey, true),
                    PendingIntent.FLAG_UPDATE_CURRENT | pendingIntentFlagMutable);

            // Because secondIntent.intentFilterEquals the first, the request should be replaced
            if (useListen) {
                mCm.registerNetworkCallback(secondRequest, secondIntent);
            } else {
                mCm.requestNetwork(secondRequest, secondIntent);
            }

            final IntentFilter filter = new IntentFilter();
            filter.addAction(broadcastAction);

            final CompletableFuture<Network> networkFuture = new CompletableFuture<>();
            final AtomicInteger receivedCount = new AtomicInteger(0);
            receiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    final NetworkRequest request = intent.getParcelableExtra(EXTRA_NETWORK_REQUEST);
                    assertPendingIntentRequestMatches(request, secondRequest, useListen);
                    receivedCount.incrementAndGet();
                    networkFuture.complete(intent.getParcelableExtra(EXTRA_NETWORK));
                }
            };
            mContext.registerReceiver(receiver, filter);

            final Network wifiNetwork = mCtsNetUtils.ensureWifiConnected();
            try {
                assertEquals(wifiNetwork, networkFuture.get(
                        NETWORK_CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS));
            } catch (TimeoutException e) {
                throw new AssertionError("PendingIntent not received for " + secondRequest, e);
            }

            // Sleep for a small amount of time to try to check that only one callback is ever
            // received (so the first callback was really unregistered). This does not guarantee
            // that the test will fail if it runs very slowly, but it should at least be very
            // noticeably flaky.
            Thread.sleep(NO_CALLBACK_TIMEOUT_MS);

            // For R- frameworks, listens will receive duplicated callbacks. See b/189868426.
            if (isAtLeastS() || !useListen) {
                assertEquals("PendingIntent should only be received once", 1, receivedCount.get());
            }
        } finally {
            if (firstIntent != null) mCm.unregisterNetworkCallback(firstIntent);
            if (secondIntent != null) mCm.unregisterNetworkCallback(secondIntent);
            if (receiver != null) mContext.unregisterReceiver(receiver);
            mCtsNetUtils.ensureWifiConnected();
        }
    }

    private void assertPendingIntentRequestMatches(NetworkRequest broadcasted, NetworkRequest filed,
            boolean useListen) {
        assertArrayEquals(filed.networkCapabilities.getCapabilities(),
                broadcasted.networkCapabilities.getCapabilities());
        // For R- frameworks, listens will receive duplicated callbacks. See b/189868426.
        if (!isAtLeastS() && useListen) return;
        assertArrayEquals(filed.networkCapabilities.getTransportTypes(),
                broadcasted.networkCapabilities.getTransportTypes());
    }

    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testRegisterNetworkRequest_identicalPendingIntents() throws Exception {
        runIdenticalPendingIntentsRequestTest(false /* useListen */);
    }

    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testRegisterNetworkCallback_identicalPendingIntents() throws Exception {
        runIdenticalPendingIntentsRequestTest(true /* useListen */);
    }

    /**
     * Exercises the requestNetwork with NetworkCallback API. This checks to
     * see if we get a callback for an INTERNET request.
     */
    @AppModeFull(reason = "CHANGE_NETWORK_STATE permission can't be granted to instant apps")
    @Test
    public void testRequestNetworkCallback() {
        final TestNetworkCallback callback = new TestNetworkCallback();
        mCm.requestNetwork(new NetworkRequest.Builder()
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build(), callback);

        try {
            // Wait to get callback for availability of internet
            Network internetNetwork = callback.waitForAvailable();
            assertNotNull("Did not receive NetworkCallback#onAvailable for INTERNET",
                    internetNetwork);
        } catch (InterruptedException e) {
            fail("NetworkCallback wait was interrupted.");
        } finally {
            mCm.unregisterNetworkCallback(callback);
        }
    }

    /**
     * Exercises the requestNetwork with NetworkCallback API with timeout - expected to
     * fail. Use WIFI and switch Wi-Fi off.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testRequestNetworkCallback_onUnavailable() {
        final boolean previousWifiEnabledState = mWifiManager.isWifiEnabled();
        if (previousWifiEnabledState) {
            mCtsNetUtils.ensureWifiDisconnected(null);
        }

        final TestNetworkCallback callback = new TestNetworkCallback();
        mCm.requestNetwork(new NetworkRequest.Builder()
                .addTransportType(TRANSPORT_WIFI)
                .build(), callback, 100);

        try {
            // Wait to get callback for unavailability of requested network
            assertTrue("Did not receive NetworkCallback#onUnavailable",
                    callback.waitForUnavailable());
        } catch (InterruptedException e) {
            fail("NetworkCallback wait was interrupted.");
        } finally {
            mCm.unregisterNetworkCallback(callback);
            if (previousWifiEnabledState) {
                mCtsNetUtils.connectToWifi();
            }
        }
    }

    private InetAddress getFirstV4Address(Network network) {
        LinkProperties linkProperties = mCm.getLinkProperties(network);
        for (InetAddress address : linkProperties.getAddresses()) {
            if (address instanceof Inet4Address) {
                return address;
            }
        }
        return null;
    }

    /**
     * Checks that enabling/disabling wifi causes CONNECTIVITY_ACTION broadcasts.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testToggleWifiConnectivityAction() {
        // toggleWifi calls connectToWifi and disconnectFromWifi, which both wait for
        // CONNECTIVITY_ACTION broadcasts.
        mCtsNetUtils.toggleWifi();
    }

    /** Verify restricted networks cannot be requested. */
    @AppModeFull(reason = "CHANGE_NETWORK_STATE permission can't be granted to instant apps")
    @Test
    public void testRestrictedNetworks() {
        // Verify we can request unrestricted networks:
        NetworkRequest request = new NetworkRequest.Builder()
                .addCapability(NET_CAPABILITY_INTERNET).build();
        NetworkCallback callback = new NetworkCallback();
        mCm.requestNetwork(request, callback);
        mCm.unregisterNetworkCallback(callback);
        // Verify we cannot request restricted networks:
        request = new NetworkRequest.Builder().addCapability(NET_CAPABILITY_IMS).build();
        callback = new NetworkCallback();
        try {
            mCm.requestNetwork(request, callback);
            fail("No exception thrown when restricted network requested.");
        } catch (SecurityException expected) {}
    }

    // Returns "true", "false" or "none"
    private String getWifiMeteredStatus(String ssid) throws Exception {
        // Interestingly giving the SSID as an argument to list wifi-networks
        // only works iff the network in question has the "false" policy.
        // Also unfortunately runShellCommand does not pass the command to the interpreter
        // so it's not possible to | grep the ssid.
        final String command = "cmd netpolicy list wifi-networks";
        final String policyString = runShellCommand(mInstrumentation, command);

        final Matcher m = Pattern.compile("^" + ssid + ";(true|false|none)$",
                Pattern.MULTILINE | Pattern.UNIX_LINES).matcher(policyString);
        if (!m.find()) {
            fail("Unexpected format from cmd netpolicy, policyString = " + policyString);
        }
        return m.group(1);
    }

    // metered should be "true", "false" or "none"
    private void setWifiMeteredStatus(String ssid, String metered) throws Exception {
        final String setCommand = "cmd netpolicy set metered-network " + ssid + " " + metered;
        runShellCommand(mInstrumentation, setCommand);
        assertEquals(getWifiMeteredStatus(ssid), metered);
    }

    private String unquoteSSID(String ssid) {
        // SSID is returned surrounded by quotes if it can be decoded as UTF-8.
        // Otherwise it's guaranteed not to start with a quote.
        if (ssid.charAt(0) == '"') {
            return ssid.substring(1, ssid.length() - 1);
        } else {
            return ssid;
        }
    }

    private void waitForActiveNetworkMetered(final int targetTransportType,
            final boolean requestedMeteredness, final boolean useSystemDefault)
            throws Exception {
        final CountDownLatch latch = new CountDownLatch(1);
        final NetworkCallback networkCallback = new NetworkCallback() {
            @Override
            public void onCapabilitiesChanged(Network network, NetworkCapabilities nc) {
                if (!nc.hasTransport(targetTransportType)) return;

                final boolean metered = !nc.hasCapability(NET_CAPABILITY_NOT_METERED);
                if (metered == requestedMeteredness) {
                    latch.countDown();
                }
            }
        };

        try {
            // Registering a callback here guarantees onCapabilitiesChanged is called immediately
            // with the current setting. Therefore, if the setting has already been changed,
            // this method will return right away, and if not, it'll wait for the setting to change.
            if (useSystemDefault) {
                runWithShellPermissionIdentity(() ->
                                mCmShim.registerSystemDefaultNetworkCallback(networkCallback,
                                        new Handler(Looper.getMainLooper())),
                        NETWORK_SETTINGS);
            } else {
                mCm.registerDefaultNetworkCallback(networkCallback);
            }

            // Changing meteredness on wifi involves reconnecting, which can take several seconds
            // (involves re-associating, DHCP...).
            if (!latch.await(NETWORK_CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
                fail("Timed out waiting for active network metered status to change to "
                        + requestedMeteredness + " ; network = " + mCm.getActiveNetwork());
            }
        } finally {
            mCm.unregisterNetworkCallback(networkCallback);
        }
    }

    private void setWifiMeteredStatusAndWait(String ssid, boolean isMetered) throws Exception {
        setWifiMeteredStatus(ssid, Boolean.toString(isMetered) /* metered */);
        waitForActiveNetworkMetered(TRANSPORT_WIFI,
                isMetered /* requestedMeteredness */,
                true /* useSystemDefault */);
    }

    private void assertMultipathPreferenceIsEventually(Network network, int oldValue,
            int expectedValue) {
        // Quick check : if oldValue == expectedValue, there is no way to guarantee the test
        // is not flaky.
        assertNotSame(oldValue, expectedValue);

        for (int i = 0; i < NUM_TRIES_MULTIPATH_PREF_CHECK; ++i) {
            final int actualValue = mCm.getMultipathPreference(network);
            if (actualValue == expectedValue) {
                return;
            }
            if (actualValue != oldValue) {
                fail("Multipath preference is neither previous (" + oldValue
                        + ") nor expected (" + expectedValue + ")");
            }
            SystemClock.sleep(INTERVAL_MULTIPATH_PREF_CHECK_MS);
        }
        fail("Timed out waiting for multipath preference to change. expected = "
                + expectedValue + " ; actual = " + mCm.getMultipathPreference(network));
    }

    private int getCurrentMeteredMultipathPreference(ContentResolver resolver) {
        final String rawMeteredPref = Settings.Global.getString(resolver,
                NETWORK_METERED_MULTIPATH_PREFERENCE);
        return TextUtils.isEmpty(rawMeteredPref)
            ? getIntResourceForName(NETWORK_METERED_MULTIPATH_PREFERENCE_RES_NAME)
            : Integer.parseInt(rawMeteredPref);
    }

    private int findNextPrefValue(ContentResolver resolver) {
        // A bit of a nuclear hammer, but race conditions in CTS are bad. To be able to
        // detect a correct setting value without race conditions, the next pref must
        // be a valid value (range 0..3) that is different from the old setting of the
        // metered preference and from the unmetered preference.
        final int meteredPref = getCurrentMeteredMultipathPreference(resolver);
        final int unmeteredPref = ConnectivityManager.MULTIPATH_PREFERENCE_UNMETERED;
        if (0 != meteredPref && 0 != unmeteredPref) return 0;
        if (1 != meteredPref && 1 != unmeteredPref) return 1;
        return 2;
    }

    /**
     * Verify that getMultipathPreference does return appropriate values
     * for metered and unmetered networks.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testGetMultipathPreference() throws Exception {
        final ContentResolver resolver = mContext.getContentResolver();
        mCtsNetUtils.ensureWifiConnected();
        final String ssid = unquoteSSID(mWifiManager.getConnectionInfo().getSSID());
        final String oldMeteredSetting = getWifiMeteredStatus(ssid);
        final String oldMeteredMultipathPreference = Settings.Global.getString(
                resolver, NETWORK_METERED_MULTIPATH_PREFERENCE);
        try {
            final int initialMeteredPreference = getCurrentMeteredMultipathPreference(resolver);
            int newMeteredPreference = findNextPrefValue(resolver);
            Settings.Global.putString(resolver, NETWORK_METERED_MULTIPATH_PREFERENCE,
                    Integer.toString(newMeteredPreference));
            // Wifi meterness changes from unmetered to metered will disconnect and reconnect since
            // R.
            setWifiMeteredStatusAndWait(ssid, true);
            final Network network = mCtsNetUtils.ensureWifiConnected();
            assertEquals(ssid, unquoteSSID(mWifiManager.getConnectionInfo().getSSID()));
            assertEquals(mCm.getNetworkCapabilities(network).hasCapability(
                    NET_CAPABILITY_NOT_METERED), false);
            assertMultipathPreferenceIsEventually(network, initialMeteredPreference,
                    newMeteredPreference);

            final int oldMeteredPreference = newMeteredPreference;
            newMeteredPreference = findNextPrefValue(resolver);
            Settings.Global.putString(resolver, NETWORK_METERED_MULTIPATH_PREFERENCE,
                    Integer.toString(newMeteredPreference));
            assertEquals(mCm.getNetworkCapabilities(network).hasCapability(
                    NET_CAPABILITY_NOT_METERED), false);
            assertMultipathPreferenceIsEventually(network,
                    oldMeteredPreference, newMeteredPreference);

            // No disconnect from unmetered to metered.
            setWifiMeteredStatusAndWait(ssid, false);
            assertEquals(mCm.getNetworkCapabilities(network).hasCapability(
                    NET_CAPABILITY_NOT_METERED), true);
            assertMultipathPreferenceIsEventually(network, newMeteredPreference,
                    ConnectivityManager.MULTIPATH_PREFERENCE_UNMETERED);
        } finally {
            Settings.Global.putString(resolver, NETWORK_METERED_MULTIPATH_PREFERENCE,
                    oldMeteredMultipathPreference);
            setWifiMeteredStatus(ssid, oldMeteredSetting);
        }
    }

    // TODO: move the following socket keep alive test to dedicated test class.
    /**
     * Callback used in tcp keepalive offload that allows caller to wait callback fires.
     */
    private static class TestSocketKeepaliveCallback extends SocketKeepalive.Callback {
        public enum CallbackType { ON_STARTED, ON_STOPPED, ON_ERROR };

        public static class CallbackValue {
            public final CallbackType callbackType;
            public final int error;

            private CallbackValue(final CallbackType type, final int error) {
                this.callbackType = type;
                this.error = error;
            }

            public static class OnStartedCallback extends CallbackValue {
                OnStartedCallback() { super(CallbackType.ON_STARTED, 0); }
            }

            public static class OnStoppedCallback extends CallbackValue {
                OnStoppedCallback() { super(CallbackType.ON_STOPPED, 0); }
            }

            public static class OnErrorCallback extends CallbackValue {
                OnErrorCallback(final int error) { super(CallbackType.ON_ERROR, error); }
            }

            @Override
            public boolean equals(Object o) {
                return o.getClass() == this.getClass()
                        && this.callbackType == ((CallbackValue) o).callbackType
                        && this.error == ((CallbackValue) o).error;
            }

            @Override
            public String toString() {
                return String.format("%s(%s, %d)", getClass().getSimpleName(), callbackType, error);
            }
        }

        private final LinkedBlockingQueue<CallbackValue> mCallbacks = new LinkedBlockingQueue<>();

        @Override
        public void onStarted() {
            mCallbacks.add(new CallbackValue.OnStartedCallback());
        }

        @Override
        public void onStopped() {
            mCallbacks.add(new CallbackValue.OnStoppedCallback());
        }

        @Override
        public void onError(final int error) {
            mCallbacks.add(new CallbackValue.OnErrorCallback(error));
        }

        public CallbackValue pollCallback() {
            try {
                return mCallbacks.poll(KEEPALIVE_CALLBACK_TIMEOUT_MS,
                        TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                fail("Callback not seen after " + KEEPALIVE_CALLBACK_TIMEOUT_MS + " ms");
            }
            return null;
        }
        private void expectCallback(CallbackValue expectedCallback) {
            final CallbackValue actualCallback = pollCallback();
            assertEquals(expectedCallback, actualCallback);
        }

        public void expectStarted() {
            expectCallback(new CallbackValue.OnStartedCallback());
        }

        public void expectStopped() {
            expectCallback(new CallbackValue.OnStoppedCallback());
        }

        public void expectError(int error) {
            expectCallback(new CallbackValue.OnErrorCallback(error));
        }
    }

    private InetAddress getAddrByName(final String hostname, final int family) throws Exception {
        final InetAddress[] allAddrs = InetAddress.getAllByName(hostname);
        for (InetAddress addr : allAddrs) {
            if (family == AF_INET && addr instanceof Inet4Address) return addr;

            if (family == AF_INET6 && addr instanceof Inet6Address) return addr;

            if (family == AF_UNSPEC) return addr;
        }
        return null;
    }

    private Socket getConnectedSocket(final Network network, final String host, final int port,
            final int family) throws Exception {
        final Socket s = network.getSocketFactory().createSocket();
        try {
            final InetAddress addr = getAddrByName(host, family);
            if (addr == null) fail("Fail to get destination address for " + family);

            final InetSocketAddress sockAddr = new InetSocketAddress(addr, port);
            s.connect(sockAddr);
        } catch (Exception e) {
            s.close();
            throw e;
        }
        return s;
    }

    private int getSupportedKeepalivesForNet(@NonNull Network network) throws Exception {
        final NetworkCapabilities nc = mCm.getNetworkCapabilities(network);

        // Get number of supported concurrent keepalives for testing network.
        final int[] keepalivesPerTransport = KeepaliveUtils.getSupportedKeepalives(mContext);
        return KeepaliveUtils.getSupportedKeepalivesForNetworkCapabilities(
                keepalivesPerTransport, nc);
    }

    private static boolean isTcpKeepaliveSupportedByKernel() {
        final String kVersionString = VintfRuntimeInfo.getKernelVersion();
        return compareMajorMinorVersion(kVersionString, "4.8") >= 0;
    }

    private static Pair<Integer, Integer> getVersionFromString(String version) {
        // Only gets major and minor number of the version string.
        final Pattern versionPattern = Pattern.compile("^(\\d+)(\\.(\\d+))?.*");
        final Matcher m = versionPattern.matcher(version);
        if (m.matches()) {
            final int major = Integer.parseInt(m.group(1));
            final int minor = TextUtils.isEmpty(m.group(3)) ? 0 : Integer.parseInt(m.group(3));
            return new Pair<>(major, minor);
        } else {
            return new Pair<>(0, 0);
        }
    }

    // TODO: Move to util class.
    private static int compareMajorMinorVersion(final String s1, final String s2) {
        final Pair<Integer, Integer> v1 = getVersionFromString(s1);
        final Pair<Integer, Integer> v2 = getVersionFromString(s2);

        if (v1.first == v2.first) {
            return Integer.compare(v1.second, v2.second);
        } else {
            return Integer.compare(v1.first, v2.first);
        }
    }

    /**
     * Verifies that version string compare logic returns expected result for various cases.
     * Note that only major and minor number are compared.
     */
    @Test
    public void testMajorMinorVersionCompare() {
        assertEquals(0, compareMajorMinorVersion("4.8.1", "4.8"));
        assertEquals(1, compareMajorMinorVersion("4.9", "4.8.1"));
        assertEquals(1, compareMajorMinorVersion("5.0", "4.8"));
        assertEquals(1, compareMajorMinorVersion("5", "4.8"));
        assertEquals(0, compareMajorMinorVersion("5", "5.0"));
        assertEquals(1, compareMajorMinorVersion("5-beta1", "4.8"));
        assertEquals(0, compareMajorMinorVersion("4.8.0.0", "4.8"));
        assertEquals(0, compareMajorMinorVersion("4.8-RC1", "4.8"));
        assertEquals(0, compareMajorMinorVersion("4.8", "4.8"));
        assertEquals(-1, compareMajorMinorVersion("3.10", "4.8.0"));
        assertEquals(-1, compareMajorMinorVersion("4.7.10.10", "4.8"));
    }

    /**
     * Verifies that the keepalive API cannot create any keepalive when the maximum number of
     * keepalives is set to 0.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testKeepaliveWifiUnsupported() throws Exception {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final Network network = mCtsNetUtils.ensureWifiConnected();
        if (getSupportedKeepalivesForNet(network) != 0) return;
        final InetAddress srcAddr = getFirstV4Address(network);
        assumeTrue("This test requires native IPv4", srcAddr != null);

        runWithShellPermissionIdentity(() -> {
            assertEquals(0, createConcurrentSocketKeepalives(network, srcAddr, 1, 0));
            assertEquals(0, createConcurrentSocketKeepalives(network, srcAddr, 0, 1));
        });
    }

    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    @SkipPresubmit(reason = "Keepalive is not supported on virtual hardware")
    public void testCreateTcpKeepalive() throws Exception {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final Network network = mCtsNetUtils.ensureWifiConnected();
        if (getSupportedKeepalivesForNet(network) == 0) return;
        final InetAddress srcAddr = getFirstV4Address(network);
        assumeTrue("This test requires native IPv4", srcAddr != null);

        // If kernel < 4.8 then it doesn't support TCP keepalive, but it might still support
        // NAT-T keepalive. If keepalive limits from resource overlay is not zero, TCP keepalive
        // needs to be supported except if the kernel doesn't support it.
        if (!isTcpKeepaliveSupportedByKernel()) {
            // Verify that the callback result is expected.
            runWithShellPermissionIdentity(() -> {
                assertEquals(0, createConcurrentSocketKeepalives(network, srcAddr, 0, 1));
            });
            Log.i(TAG, "testCreateTcpKeepalive is skipped for kernel "
                    + VintfRuntimeInfo.getKernelVersion());
            return;
        }

        final byte[] requestBytes = CtsNetUtils.HTTP_REQUEST.getBytes("UTF-8");
        // So far only ipv4 tcp keepalive offload is supported.
        // TODO: add test case for ipv6 tcp keepalive offload when it is supported.
        try (Socket s = getConnectedSocket(network, TEST_HOST, HTTP_PORT, AF_INET)) {

            // Should able to start keep alive offload when socket is idle.
            final Executor executor = mContext.getMainExecutor();
            final TestSocketKeepaliveCallback callback = new TestSocketKeepaliveCallback();

            mUiAutomation.adoptShellPermissionIdentity();
            try (SocketKeepalive sk = mCm.createSocketKeepalive(network, s, executor, callback)) {
                sk.start(MIN_KEEPALIVE_INTERVAL);
                callback.expectStarted();

                // App should not able to write during keepalive offload.
                final OutputStream out = s.getOutputStream();
                try {
                    out.write(requestBytes);
                    fail("Should not able to write");
                } catch (IOException e) { }
                // App should not able to read during keepalive offload.
                final InputStream in = s.getInputStream();
                byte[] responseBytes = new byte[4096];
                try {
                    in.read(responseBytes);
                    fail("Should not able to read");
                } catch (IOException e) { }

                // Stop.
                sk.stop();
                callback.expectStopped();
            } finally {
                mUiAutomation.dropShellPermissionIdentity();
            }

            // Ensure socket is still connected.
            assertTrue(s.isConnected());
            assertFalse(s.isClosed());

            // Let socket be not idle.
            try {
                final OutputStream out = s.getOutputStream();
                out.write(requestBytes);
            } catch (IOException e) {
                fail("Failed to write data " + e);
            }
            // Make sure response data arrives.
            final MessageQueue fdHandlerQueue = Looper.getMainLooper().getQueue();
            final FileDescriptor fd = s.getFileDescriptor$();
            final CountDownLatch mOnReceiveLatch = new CountDownLatch(1);
            fdHandlerQueue.addOnFileDescriptorEventListener(fd, EVENT_INPUT, (readyFd, events) -> {
                mOnReceiveLatch.countDown();
                return 0; // Unregister listener.
            });
            if (!mOnReceiveLatch.await(2, TimeUnit.SECONDS)) {
                fdHandlerQueue.removeOnFileDescriptorEventListener(fd);
                fail("Timeout: no response data");
            }

            // Should get ERROR_SOCKET_NOT_IDLE because there is still data in the receive queue
            // that has not been read.
            mUiAutomation.adoptShellPermissionIdentity();
            try (SocketKeepalive sk = mCm.createSocketKeepalive(network, s, executor, callback)) {
                sk.start(MIN_KEEPALIVE_INTERVAL);
                callback.expectError(SocketKeepalive.ERROR_SOCKET_NOT_IDLE);
            } finally {
                mUiAutomation.dropShellPermissionIdentity();
            }
        }
    }

    private ArrayList<SocketKeepalive> createConcurrentKeepalivesOfType(
            int requestCount, @NonNull TestSocketKeepaliveCallback callback,
            Supplier<SocketKeepalive> kaFactory) {
        final ArrayList<SocketKeepalive> kalist = new ArrayList<>();

        int remainingRetries = MAX_KEEPALIVE_RETRY_COUNT;

        // Test concurrent keepalives with the given supplier.
        while (kalist.size() < requestCount) {
            final SocketKeepalive ka = kaFactory.get();
            ka.start(MIN_KEEPALIVE_INTERVAL);
            TestSocketKeepaliveCallback.CallbackValue cv = callback.pollCallback();
            assertNotNull(cv);
            if (cv.callbackType == TestSocketKeepaliveCallback.CallbackType.ON_ERROR) {
                if (kalist.size() == 0 && cv.error == SocketKeepalive.ERROR_UNSUPPORTED) {
                    // Unsupported.
                    break;
                } else if (cv.error == SocketKeepalive.ERROR_INSUFFICIENT_RESOURCES) {
                    // Limit reached or temporary unavailable due to stopped slot is not yet
                    // released.
                    if (remainingRetries > 0) {
                        SystemClock.sleep(INTERVAL_KEEPALIVE_RETRY_MS);
                        remainingRetries--;
                        continue;
                    }
                    break;
                }
            }
            if (cv.callbackType == TestSocketKeepaliveCallback.CallbackType.ON_STARTED) {
                kalist.add(ka);
            } else {
                fail("Unexpected error when creating " + (kalist.size() + 1) + " "
                        + ka.getClass().getSimpleName() + ": " + cv);
            }
        }

        return kalist;
    }

    private @NonNull ArrayList<SocketKeepalive> createConcurrentNattSocketKeepalives(
            @NonNull Network network, @NonNull InetAddress srcAddr, int requestCount,
            @NonNull TestSocketKeepaliveCallback callback)  throws Exception {

        final Executor executor = mContext.getMainExecutor();

        // Initialize a real NaT-T socket.
        final IpSecManager mIpSec = (IpSecManager) mContext.getSystemService(Context.IPSEC_SERVICE);
        final UdpEncapsulationSocket nattSocket = mIpSec.openUdpEncapsulationSocket();
        final InetAddress dstAddr = getAddrByName(TEST_HOST, AF_INET);
        assertNotNull(srcAddr);
        assertNotNull(dstAddr);

        // Test concurrent Nat-T keepalives.
        final ArrayList<SocketKeepalive> result = createConcurrentKeepalivesOfType(requestCount,
                callback, () -> mCm.createSocketKeepalive(network, nattSocket,
                        srcAddr, dstAddr, executor, callback));

        nattSocket.close();
        return result;
    }

    private @NonNull ArrayList<SocketKeepalive> createConcurrentTcpSocketKeepalives(
            @NonNull Network network, int requestCount,
            @NonNull TestSocketKeepaliveCallback callback) {
        final Executor executor = mContext.getMainExecutor();

        // Create concurrent TCP keepalives.
        return createConcurrentKeepalivesOfType(requestCount, callback, () -> {
            // Assert that TCP connections can be established. The file descriptor of tcp
            // sockets will be duplicated and kept valid in service side if the keepalives are
            // successfully started.
            try (Socket tcpSocket = getConnectedSocket(network, TEST_HOST, HTTP_PORT,
                    AF_INET)) {
                return mCm.createSocketKeepalive(network, tcpSocket, executor, callback);
            } catch (Exception e) {
                fail("Unexpected error when creating TCP socket: " + e);
            }
            return null;
        });
    }

    /**
     * Creates concurrent keepalives until the specified counts of each type of keepalives are
     * reached or the expected error callbacks are received for each type of keepalives.
     *
     * @return the total number of keepalives created.
     */
    private int createConcurrentSocketKeepalives(
            @NonNull Network network, @NonNull InetAddress srcAddr, int nattCount, int tcpCount)
            throws Exception {
        final ArrayList<SocketKeepalive> kalist = new ArrayList<>();
        final TestSocketKeepaliveCallback callback = new TestSocketKeepaliveCallback();

        kalist.addAll(createConcurrentNattSocketKeepalives(network, srcAddr, nattCount, callback));
        kalist.addAll(createConcurrentTcpSocketKeepalives(network, tcpCount, callback));

        final int ret = kalist.size();

        // Clean up.
        for (final SocketKeepalive ka : kalist) {
            ka.stop();
            callback.expectStopped();
        }
        kalist.clear();

        return ret;
    }

    /**
     * Verifies that the concurrent keepalive slots meet the minimum requirement, and don't
     * get leaked after iterations.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    @SkipPresubmit(reason = "Keepalive is not supported on virtual hardware")
    public void testSocketKeepaliveLimitWifi() throws Exception {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final Network network = mCtsNetUtils.ensureWifiConnected();
        final int supported = getSupportedKeepalivesForNet(network);
        if (supported == 0) {
            return;
        }
        final InetAddress srcAddr = getFirstV4Address(network);
        assumeTrue("This test requires native IPv4", srcAddr != null);

        runWithShellPermissionIdentity(() -> {
            // Verifies that the supported keepalive slots meet MIN_SUPPORTED_KEEPALIVE_COUNT.
            assertGreaterOrEqual(supported, MIN_SUPPORTED_WIFI_KEEPALIVE_COUNT);

            // Verifies that Nat-T keepalives can be established.
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr,
                    supported + 1, 0));
            // Verifies that keepalives don't get leaked in second round.
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr, supported,
                    0));
        });

        // If kernel < 4.8 then it doesn't support TCP keepalive, but it might still support
        // NAT-T keepalive. Test below cases only if TCP keepalive is supported by kernel.
        if (!isTcpKeepaliveSupportedByKernel()) return;

        runWithShellPermissionIdentity(() -> {
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr, 0,
                    supported + 1));

            // Verifies that different types can be established at the same time.
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr,
                    supported / 2, supported - supported / 2));

            // Verifies that keepalives don't get leaked in second round.
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr, 0,
                    supported));
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr,
                    supported / 2, supported - supported / 2));
        });
    }

    /**
     * Verifies that the concurrent keepalive slots meet the minimum telephony requirement, and
     * don't get leaked after iterations.
     */
    @AppModeFull(reason = "Cannot request network in instant app mode")
    @Test
    @SkipPresubmit(reason = "Keepalive is not supported on virtual hardware")
    public void testSocketKeepaliveLimitTelephony() throws Exception {
        if (!mPackageManager.hasSystemFeature(FEATURE_TELEPHONY)) {
            Log.i(TAG, "testSocketKeepaliveLimitTelephony cannot execute unless device"
                    + " supports telephony");
            return;
        }

        final int firstSdk = SdkLevel.isAtLeastS()
                ? Build.VERSION.DEVICE_INITIAL_SDK_INT
                // FIRST_SDK_INT was a @TestApi field renamed to DEVICE_INITIAL_SDK_INT in S
                : Build.VERSION.class.getField("FIRST_SDK_INT").getInt(null);
        if (firstSdk < Build.VERSION_CODES.Q) {
            Log.i(TAG, "testSocketKeepaliveLimitTelephony: skip test for devices launching"
                    + " before Q: " + firstSdk);
            return;
        }

        final Network network = mCtsNetUtils.connectToCell();
        final int supported = getSupportedKeepalivesForNet(network);
        final InetAddress srcAddr = getFirstV4Address(network);
        assumeTrue("This test requires native IPv4", srcAddr != null);

        runWithShellPermissionIdentity(() -> {
            // Verifies that the supported keepalive slots meet minimum requirement.
            assertGreaterOrEqual(supported, MIN_SUPPORTED_CELLULAR_KEEPALIVE_COUNT);
            // Verifies that Nat-T keepalives can be established.
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr,
                    supported + 1, 0));
            // Verifies that keepalives don't get leaked in second round.
            assertEquals(supported, createConcurrentSocketKeepalives(network, srcAddr, supported,
                    0));
        });
    }

    private int getIntResourceForName(@NonNull String resName) {
        final Resources r = mContext.getResources();
        final int resId = r.getIdentifier(resName, "integer", "android");
        return r.getInteger(resId);
    }

    /**
     * Verifies that the keepalive slots are limited as customized for unprivileged requests.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    @SkipPresubmit(reason = "Keepalive is not supported on virtual hardware")
    public void testSocketKeepaliveUnprivileged() throws Exception {
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final Network network = mCtsNetUtils.ensureWifiConnected();
        final int supported = getSupportedKeepalivesForNet(network);
        if (supported == 0) {
            return;
        }
        final InetAddress srcAddr = getFirstV4Address(network);
        assumeTrue("This test requires native IPv4", srcAddr != null);

        // Resource ID might be shifted on devices that compiled with different symbols.
        // Thus, resolve ID at runtime is needed.
        final int allowedUnprivilegedPerUid =
                getIntResourceForName(KEEPALIVE_ALLOWED_UNPRIVILEGED_RES_NAME);
        final int reservedPrivilegedSlots =
                getIntResourceForName(KEEPALIVE_RESERVED_PER_SLOT_RES_NAME);
        // Verifies that unprivileged request per uid cannot exceed the limit customized in the
        // resource. Currently, unprivileged keepalive slots are limited to Nat-T only, this test
        // does not apply to TCP.
        assertGreaterOrEqual(supported, reservedPrivilegedSlots);
        assertGreaterOrEqual(supported, allowedUnprivilegedPerUid);
        final int expectedUnprivileged =
                Math.min(allowedUnprivilegedPerUid, supported - reservedPrivilegedSlots);
        assertEquals(expectedUnprivileged,
                createConcurrentSocketKeepalives(network, srcAddr, supported + 1, 0));
    }

    private static void assertGreaterOrEqual(long greater, long lesser) {
        assertTrue("" + greater + " expected to be greater than or equal to " + lesser,
                greater >= lesser);
    }

    /**
     * Verifies that apps are not allowed to access restricted networks even if they declare the
     * CONNECTIVITY_USE_RESTRICTED_NETWORKS permission in their manifests.
     * See. b/144679405.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testRestrictedNetworkPermission() throws Exception {
        // Ensure that CONNECTIVITY_USE_RESTRICTED_NETWORKS isn't granted to this package.
        final PackageInfo app = mPackageManager.getPackageInfo(mContext.getPackageName(),
                GET_PERMISSIONS);
        final int index = ArrayUtils.indexOf(
                app.requestedPermissions, CONNECTIVITY_USE_RESTRICTED_NETWORKS);
        assertTrue(index >= 0);
        assertTrue(app.requestedPermissionsFlags[index] != PERMISSION_GRANTED);

        // Ensure that NetworkUtils.queryUserAccess always returns false since this package should
        // not have netd system permission to call this function.
        final Network wifiNetwork = mCtsNetUtils.ensureWifiConnected();
        assertFalse(NetworkUtils.queryUserAccess(Binder.getCallingUid(), wifiNetwork.netId));

        // Ensure that this package cannot bind to any restricted network that's currently
        // connected.
        Network[] networks = mCm.getAllNetworks();
        for (Network network : networks) {
            NetworkCapabilities nc = mCm.getNetworkCapabilities(network);
            if (nc != null && !nc.hasCapability(NET_CAPABILITY_NOT_RESTRICTED)) {
                try {
                    network.bindSocket(new Socket());
                    fail("Bind to restricted network " + network + " unexpectedly succeeded");
                } catch (IOException expected) {}
            }
        }
    }

    /**
     * Verifies that apps are allowed to call setAirplaneMode if they declare
     * NETWORK_AIRPLANE_MODE permission in their manifests.
     * See b/145164696.
     */
    @AppModeFull(reason = "NETWORK_AIRPLANE_MODE permission can't be granted to instant apps")
    @Test
    public void testSetAirplaneMode() throws Exception{
        final boolean supportWifi = mPackageManager.hasSystemFeature(FEATURE_WIFI);
        final boolean supportTelephony = mPackageManager.hasSystemFeature(FEATURE_TELEPHONY);
        // store the current state of airplane mode
        final boolean isAirplaneModeEnabled = isAirplaneModeEnabled();
        final TestableNetworkCallback wifiCb = new TestableNetworkCallback();
        final TestableNetworkCallback telephonyCb = new TestableNetworkCallback();
        // disable airplane mode to reach a known state
        runShellCommand("cmd connectivity airplane-mode disable");
        // Verify that networks are available as expected if wifi or cell is supported. Continue the
        // test if none of them are supported since test should still able to verify the permission
        // mechanism.
        if (supportWifi) requestAndWaitForAvailable(makeWifiNetworkRequest(), wifiCb);
        if (supportTelephony) requestAndWaitForAvailable(makeCellNetworkRequest(), telephonyCb);

        try {
            // Verify we cannot set Airplane Mode without correct permission:
            try {
                setAndVerifyAirplaneMode(true);
                fail("SecurityException should have been thrown when setAirplaneMode was called"
                        + "without holding permission NETWORK_AIRPLANE_MODE.");
            } catch (SecurityException expected) {}

            // disable airplane mode again to reach a known state
            runShellCommand("cmd connectivity airplane-mode disable");

            // adopt shell permission which holds NETWORK_AIRPLANE_MODE
            mUiAutomation.adoptShellPermissionIdentity();

            // Verify we can enable Airplane Mode with correct permission:
            try {
                setAndVerifyAirplaneMode(true);
            } catch (SecurityException e) {
                fail("SecurityException should not have been thrown when setAirplaneMode(true) was"
                        + "called whilst holding the NETWORK_AIRPLANE_MODE permission.");
            }
            // Verify that the enabling airplane mode takes effect as expected to prevent flakiness
            // caused by fast airplane mode switches. Ensure network lost before turning off
            // airplane mode.
            if (supportWifi) waitForLost(wifiCb);
            if (supportTelephony) waitForLost(telephonyCb);

            // Verify we can disable Airplane Mode with correct permission:
            try {
                setAndVerifyAirplaneMode(false);
            } catch (SecurityException e) {
                fail("SecurityException should not have been thrown when setAirplaneMode(false) was"
                        + "called whilst holding the NETWORK_AIRPLANE_MODE permission.");
            }
            // Verify that turning airplane mode off takes effect as expected.
            if (supportWifi) waitForAvailable(wifiCb);
            if (supportTelephony) waitForAvailable(telephonyCb);
        } finally {
            if (supportWifi) mCm.unregisterNetworkCallback(wifiCb);
            if (supportTelephony) mCm.unregisterNetworkCallback(telephonyCb);
            // Restore the previous state of airplane mode and permissions:
            runShellCommand("cmd connectivity airplane-mode "
                    + (isAirplaneModeEnabled ? "enable" : "disable"));
            mUiAutomation.dropShellPermissionIdentity();
        }
    }

    private void requestAndWaitForAvailable(@NonNull final NetworkRequest request,
            @NonNull final TestableNetworkCallback cb) {
        mCm.registerNetworkCallback(request, cb);
        waitForAvailable(cb);
    }

    private void waitForAvailable(@NonNull final TestableNetworkCallback cb) {
        cb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                c -> c instanceof CallbackEntry.Available);
    }

    private void waitForAvailable(
            @NonNull final TestableNetworkCallback cb, final int expectedTransport) {
        cb.eventuallyExpect(
                CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                entry -> {
                    final NetworkCapabilities nc = mCm.getNetworkCapabilities(entry.getNetwork());
                    return nc.hasTransport(expectedTransport);
                }
        );
    }

    private void waitForAvailable(
            @NonNull final TestableNetworkCallback cb, @NonNull final Network expectedNetwork) {
        cb.expectAvailableCallbacks(expectedNetwork, false /* suspended */,
                true /* validated */,
                false /* blocked */, NETWORK_CALLBACK_TIMEOUT_MS);
    }

    private void waitForLost(@NonNull final TestableNetworkCallback cb) {
        cb.eventuallyExpect(CallbackEntry.LOST, NETWORK_CALLBACK_TIMEOUT_MS,
                c -> c instanceof CallbackEntry.Lost);
    }

    private void setAndVerifyAirplaneMode(Boolean expectedResult)
            throws Exception {
        final CompletableFuture<Boolean> actualResult = new CompletableFuture();
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                // The defaultValue of getExtraBoolean should be the opposite of what is
                // expected, thus ensuring a test failure if the extra is absent.
                actualResult.complete(intent.getBooleanExtra("state", !expectedResult));
            }
        };
        try {
            mContext.registerReceiver(receiver,
                    new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED));
            mCm.setAirplaneMode(expectedResult);
            final String msg = "Setting Airplane Mode failed,";
            assertEquals(msg, expectedResult, actualResult.get(AIRPLANE_MODE_CHANGE_TIMEOUT_MS,
                    TimeUnit.MILLISECONDS));
        } finally {
            mContext.unregisterReceiver(receiver);
        }
    }

    private static boolean isAirplaneModeEnabled() {
        return runShellCommand("cmd connectivity airplane-mode")
                .trim().equals("enabled");
    }

    @Test
    public void testGetCaptivePortalServerUrl() {
        final String permission = Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q
                ? CONNECTIVITY_INTERNAL
                : NETWORK_SETTINGS;
        final String url = runAsShell(permission, mCm::getCaptivePortalServerUrl);
        assertNotNull("getCaptivePortalServerUrl must not be null", url);
        try {
            final URL parsedUrl = new URL(url);
            // As per the javadoc, the URL must be HTTP
            assertEquals("Invalid captive portal URL protocol", "http", parsedUrl.getProtocol());
        } catch (MalformedURLException e) {
            throw new AssertionFailedError("Captive portal server URL is invalid: " + e);
        }
    }

    /**
     * Verifies that apps are forbidden from getting ssid information from
     * {@Code NetworkCapabilities} if they do not hold NETWORK_SETTINGS permission.
     * See b/161370134.
     */
    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testSsidInNetworkCapabilities() throws Exception {
        assumeTrue("testSsidInNetworkCapabilities cannot execute unless device supports WiFi",
                mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final Network network = mCtsNetUtils.ensureWifiConnected();
        final String ssid = unquoteSSID(mWifiManager.getConnectionInfo().getSSID());
        assertNotNull("Ssid getting from WifiManager is null", ssid);
        // This package should have no NETWORK_SETTINGS permission. Verify that no ssid is contained
        // in the NetworkCapabilities.
        verifySsidFromQueriedNetworkCapabilities(network, ssid, false /* hasSsid */);
        verifySsidFromCallbackNetworkCapabilities(ssid, false /* hasSsid */);
        // Adopt shell permission to allow to get ssid information.
        runWithShellPermissionIdentity(() -> {
            verifySsidFromQueriedNetworkCapabilities(network, ssid, true /* hasSsid */);
            verifySsidFromCallbackNetworkCapabilities(ssid, true /* hasSsid */);
        });
    }

    private void verifySsidFromQueriedNetworkCapabilities(@NonNull Network network,
            @NonNull String ssid, boolean hasSsid) throws Exception {
        // Verify if ssid is contained in NetworkCapabilities queried from ConnectivityManager.
        final NetworkCapabilities nc = mCm.getNetworkCapabilities(network);
        assertNotNull("NetworkCapabilities of the network is null", nc);
        assertEquals(hasSsid, Pattern.compile(ssid).matcher(nc.toString()).find());
    }

    private void verifySsidFromCallbackNetworkCapabilities(@NonNull String ssid, boolean hasSsid)
            throws Exception {
        final CompletableFuture<NetworkCapabilities> foundNc = new CompletableFuture();
        final NetworkCallback callback = new NetworkCallback() {
            @Override
            public void onCapabilitiesChanged(Network network, NetworkCapabilities nc) {
                foundNc.complete(nc);
            }
        };
        try {
            mCm.registerNetworkCallback(makeWifiNetworkRequest(), callback);
            // Registering a callback here guarantees onCapabilitiesChanged is called immediately
            // because WiFi network should be connected.
            final NetworkCapabilities nc =
                    foundNc.get(NETWORK_CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            // Verify if ssid is contained in the NetworkCapabilities received from callback.
            assertNotNull("NetworkCapabilities of the network is null", nc);
            assertEquals(hasSsid, Pattern.compile(ssid).matcher(nc.toString()).find());
        } finally {
            mCm.unregisterNetworkCallback(callback);
        }
    }

    /**
     * Verify background request can only be requested when acquiring
     * {@link android.Manifest.permission.NETWORK_SETTINGS}.
     */
    @AppModeFull(reason = "Instant apps cannot create test networks")
    @Test
    public void testRequestBackgroundNetwork() {
        // Cannot use @IgnoreUpTo(Build.VERSION_CODES.R) because this test also requires API 31
        // shims, and @IgnoreUpTo does not check that.
        assumeTrue(TestUtils.shouldTestSApis());

        // Create a tun interface. Use the returned interface name as the specifier to create
        // a test network request.
        final TestNetworkManager tnm = runWithShellPermissionIdentity(() ->
                mContext.getSystemService(TestNetworkManager.class),
                android.Manifest.permission.MANAGE_TEST_NETWORKS);
        final TestNetworkInterface testNetworkInterface = runWithShellPermissionIdentity(() ->
                    tnm.createTunInterface(new LinkAddress[]{TEST_LINKADDR}),
                android.Manifest.permission.MANAGE_TEST_NETWORKS,
                android.Manifest.permission.NETWORK_SETTINGS);
        assertNotNull(testNetworkInterface);

        final NetworkRequest testRequest = new NetworkRequest.Builder()
                .addTransportType(TRANSPORT_TEST)
                // Test networks do not have NOT_VPN or TRUSTED capabilities by default
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_TRUSTED)
                .setNetworkSpecifier(CompatUtil.makeTestNetworkSpecifier(
                        testNetworkInterface.getInterfaceName()))
                .build();

        // Verify background network cannot be requested without NETWORK_SETTINGS permission.
        final TestableNetworkCallback callback = new TestableNetworkCallback();
        final Handler handler = new Handler(Looper.getMainLooper());
        assertThrows(SecurityException.class,
                () -> mCmShim.requestBackgroundNetwork(testRequest, callback, handler));

        Network testNetwork = null;
        try {
            // Request background test network via Shell identity which has NETWORK_SETTINGS
            // permission granted.
            runWithShellPermissionIdentity(
                    () -> mCmShim.requestBackgroundNetwork(testRequest, callback, handler),
                    new String[] { android.Manifest.permission.NETWORK_SETTINGS });

            // Register the test network agent which has no foreground request associated to it.
            // And verify it can satisfy the background network request just fired.
            final Binder binder = new Binder();
            runWithShellPermissionIdentity(() ->
                    tnm.setupTestNetwork(testNetworkInterface.getInterfaceName(), binder),
                    new String[] { android.Manifest.permission.MANAGE_TEST_NETWORKS,
                            android.Manifest.permission.NETWORK_SETTINGS });
            waitForAvailable(callback);
            testNetwork = callback.getLastAvailableNetwork();
            assertNotNull(testNetwork);

            // The test network that has just connected is a foreground network,
            // non-listen requests will get available callback before it can be put into
            // background if no foreground request can be satisfied. Thus, wait for a short
            // period is needed to let foreground capability go away.
            callback.eventuallyExpect(CallbackEntry.NETWORK_CAPS_UPDATED,
                    NETWORK_CALLBACK_TIMEOUT_MS,
                    c -> c instanceof CallbackEntry.CapabilitiesChanged
                            && !((CallbackEntry.CapabilitiesChanged) c).getCaps()
                            .hasCapability(NET_CAPABILITY_FOREGROUND));
            final NetworkCapabilities nc = mCm.getNetworkCapabilities(testNetwork);
            assertFalse("expected background network, but got " + nc,
                    nc.hasCapability(NET_CAPABILITY_FOREGROUND));
        } finally {
            final Network n = testNetwork;
            runWithShellPermissionIdentity(() -> {
                if (null != n) {
                    tnm.teardownTestNetwork(n);
                    callback.eventuallyExpect(CallbackEntry.LOST,
                            NETWORK_CALLBACK_TIMEOUT_MS,
                            lost -> n.equals(lost.getNetwork()));
                }
                testNetworkInterface.getFileDescriptor().close();
            }, new String[] { android.Manifest.permission.MANAGE_TEST_NETWORKS });
            mCm.unregisterNetworkCallback(callback);
        }
    }

    private class DetailedBlockedStatusCallback extends TestableNetworkCallback {
        public void expectAvailableCallbacks(Network network) {
            super.expectAvailableCallbacks(network, false /* suspended */, true /* validated */,
                    BLOCKED_REASON_NONE, NETWORK_CALLBACK_TIMEOUT_MS);
        }
        public void expectBlockedStatusCallback(Network network, int blockedStatus) {
            super.expectBlockedStatusCallback(blockedStatus, network, NETWORK_CALLBACK_TIMEOUT_MS);
        }
        public void onBlockedStatusChanged(Network network, int blockedReasons) {
            getHistory().add(new CallbackEntry.BlockedStatusInt(network, blockedReasons));
        }
    }

    private void setRequireVpnForUids(boolean requireVpn, Collection<Range<Integer>> ranges)
            throws Exception {
        mCmShim.setRequireVpnForUids(requireVpn, ranges);
        for (Range<Integer> range : ranges) {
            if (requireVpn) {
                mVpnRequiredUidRanges.add(range);
            } else {
                assertTrue(mVpnRequiredUidRanges.remove(range));
            }
        }
    }

    private void doTestBlockedStatusCallback() throws Exception {
        final DetailedBlockedStatusCallback myUidCallback = new DetailedBlockedStatusCallback();
        final DetailedBlockedStatusCallback otherUidCallback = new DetailedBlockedStatusCallback();

        final int myUid = Process.myUid();
        final int otherUid = UserHandle.getUid(5, Process.FIRST_APPLICATION_UID);
        final Handler handler = new Handler(Looper.getMainLooper());
        mCm.registerDefaultNetworkCallback(myUidCallback, handler);
        mCmShim.registerDefaultNetworkCallbackForUid(otherUid, otherUidCallback, handler);

        final Network defaultNetwork = mCm.getActiveNetwork();
        final List<DetailedBlockedStatusCallback> allCallbacks =
                List.of(myUidCallback, otherUidCallback);
        for (DetailedBlockedStatusCallback callback : allCallbacks) {
            callback.expectAvailableCallbacks(defaultNetwork);
        }

        final Range<Integer> myUidRange = new Range<>(myUid, myUid);
        final Range<Integer> otherUidRange = new Range<>(otherUid, otherUid);

        setRequireVpnForUids(true, List.of(myUidRange));
        myUidCallback.expectBlockedStatusCallback(defaultNetwork, BLOCKED_REASON_LOCKDOWN_VPN);
        otherUidCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);

        setRequireVpnForUids(true, List.of(myUidRange, otherUidRange));
        myUidCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);
        otherUidCallback.expectBlockedStatusCallback(defaultNetwork, BLOCKED_REASON_LOCKDOWN_VPN);

        // setRequireVpnForUids does no deduplication or refcounting. Removing myUidRange does not
        // unblock myUid because it was added to the blocked ranges twice.
        setRequireVpnForUids(false, List.of(myUidRange));
        myUidCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);
        otherUidCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);

        setRequireVpnForUids(false, List.of(myUidRange, otherUidRange));
        myUidCallback.expectBlockedStatusCallback(defaultNetwork, BLOCKED_REASON_NONE);
        otherUidCallback.expectBlockedStatusCallback(defaultNetwork, BLOCKED_REASON_NONE);

        myUidCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);
        otherUidCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);
    }

    @Test
    public void testBlockedStatusCallback() {
        // Cannot use @IgnoreUpTo(Build.VERSION_CODES.R) because this test also requires API 31
        // shims, and @IgnoreUpTo does not check that.
        assumeTrue(TestUtils.shouldTestSApis());
        runWithShellPermissionIdentity(() -> doTestBlockedStatusCallback(), NETWORK_SETTINGS);
    }

    private void doTestLegacyLockdownEnabled() throws Exception {
        NetworkInfo info = mCm.getActiveNetworkInfo();
        assertNotNull(info);
        assertEquals(DetailedState.CONNECTED, info.getDetailedState());

        try {
            mCmShim.setLegacyLockdownVpnEnabled(true);

            // setLegacyLockdownVpnEnabled is asynchronous and only takes effect when the
            // ConnectivityService handler thread processes it. Ensure it has taken effect by doing
            // something that blocks until the handler thread is idle.
            final TestableNetworkCallback callback = new TestableNetworkCallback();
            mCm.registerDefaultNetworkCallback(callback);
            waitForAvailable(callback);
            mCm.unregisterNetworkCallback(callback);

            // Test one of the effects of setLegacyLockdownVpnEnabled: the fact that any NetworkInfo
            // in state CONNECTED is degraded to CONNECTING if the legacy VPN is not connected.
            info = mCm.getActiveNetworkInfo();
            assertNotNull(info);
            assertEquals(DetailedState.CONNECTING, info.getDetailedState());
        } finally {
            mCmShim.setLegacyLockdownVpnEnabled(false);
        }
    }

    @Test
    public void testLegacyLockdownEnabled() {
        // Cannot use @IgnoreUpTo(Build.VERSION_CODES.R) because this test also requires API 31
        // shims, and @IgnoreUpTo does not check that.
        assumeTrue(TestUtils.shouldTestSApis());
        runWithShellPermissionIdentity(() -> doTestLegacyLockdownEnabled(), NETWORK_SETTINGS);
    }

    @Test
    public void testGetCapabilityCarrierName() {
        assumeTrue(TestUtils.shouldTestSApis());
        assertEquals("ENTERPRISE", NetworkInformationShimImpl.newInstance()
                .getCapabilityCarrierName(ConstantsShim.NET_CAPABILITY_ENTERPRISE));
        assertNull(NetworkInformationShimImpl.newInstance()
                .getCapabilityCarrierName(ConstantsShim.NET_CAPABILITY_NOT_VCN_MANAGED));
    }

    @Test
    public void testSetGlobalProxy() {
        assumeTrue(TestUtils.shouldTestSApis());
        // Behavior is verified in gts. Verify exception thrown w/o permission.
        assertThrows(SecurityException.class, () -> mCm.setGlobalProxy(
                ProxyInfo.buildDirectProxy("example.com" /* host */, 8080 /* port */)));
    }

    @Test
    public void testFactoryResetWithoutPermission() {
        assumeTrue(TestUtils.shouldTestSApis());
        assertThrows(SecurityException.class, () -> mCm.factoryReset());
    }

    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testFactoryReset() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());

        // Store current settings.
        final int curAvoidBadWifi =
                ConnectivitySettingsManager.getNetworkAvoidBadWifi(mContext);
        final int curPrivateDnsMode = ConnectivitySettingsManager.getPrivateDnsMode(mContext);

        TestTetheringEventCallback tetherEventCallback = null;
        final CtsTetheringUtils tetherUtils = new CtsTetheringUtils(mContext);
        try {
            tetherEventCallback = tetherUtils.registerTetheringEventCallback();
            // Adopt for NETWORK_SETTINGS permission.
            mUiAutomation.adoptShellPermissionIdentity();
            // start tethering
            tetherEventCallback.assumeWifiTetheringSupported(mContext);
            tetherUtils.startWifiTethering(tetherEventCallback);
            // Update setting to verify the behavior.
            mCm.setAirplaneMode(true);
            ConnectivitySettingsManager.setPrivateDnsMode(mContext,
                    ConnectivitySettingsManager.PRIVATE_DNS_MODE_OFF);
            ConnectivitySettingsManager.setNetworkAvoidBadWifi(mContext,
                    ConnectivitySettingsManager.NETWORK_AVOID_BAD_WIFI_IGNORE);
            assertEquals(AIRPLANE_MODE_ON, Settings.Global.getInt(
                    mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON));
            // Verify factoryReset
            mCm.factoryReset();
            verifySettings(AIRPLANE_MODE_OFF,
                    ConnectivitySettingsManager.PRIVATE_DNS_MODE_OPPORTUNISTIC,
                    ConnectivitySettingsManager.NETWORK_AVOID_BAD_WIFI_PROMPT);

            tetherEventCallback.expectNoTetheringActive();
        } finally {
            // Restore settings.
            mCm.setAirplaneMode(false);
            ConnectivitySettingsManager.setNetworkAvoidBadWifi(mContext, curAvoidBadWifi);
            ConnectivitySettingsManager.setPrivateDnsMode(mContext, curPrivateDnsMode);
            if (tetherEventCallback != null) {
                tetherUtils.unregisterTetheringEventCallback(tetherEventCallback);
            }
            tetherUtils.stopAllTethering();
            mUiAutomation.dropShellPermissionIdentity();
        }
    }

    /**
     * Verify that {@link ConnectivityManager#setProfileNetworkPreference} cannot be called
     * without required NETWORK_STACK permissions.
     */
    @Test
    public void testSetProfileNetworkPreference_NoPermission() {
        // Cannot use @IgnoreUpTo(Build.VERSION_CODES.R) because this test also requires API 31
        // shims, and @IgnoreUpTo does not check that.
        assumeTrue(TestUtils.shouldTestSApis());
        assertThrows(SecurityException.class, () -> mCm.setProfileNetworkPreference(
                UserHandle.of(0), PROFILE_NETWORK_PREFERENCE_ENTERPRISE, null /* executor */,
                null /* listener */));
    }

    @Test
    public void testSystemReady() {
        assumeTrue(TestUtils.shouldTestSApis());
        assertThrows(SecurityException.class, () -> mCm.systemReady());
    }

    @Test
    public void testGetIpSecNetIdRange() {
        assumeTrue(TestUtils.shouldTestSApis());
        // The lower refers to ConnectivityManager.TUN_INTF_NETID_START.
        final long lower = 64512;
        // The upper refers to ConnectivityManager.TUN_INTF_NETID_START
        // + ConnectivityManager.TUN_INTF_NETID_RANGE - 1
        final long upper = 65535;
        assertEquals(lower, (long) ConnectivityManager.getIpSecNetIdRange().getLower());
        assertEquals(upper, (long) ConnectivityManager.getIpSecNetIdRange().getUpper());
    }

    private void verifySettings(int expectedAirplaneMode, int expectedPrivateDnsMode,
            int expectedAvoidBadWifi) throws Exception {
        assertEquals(expectedAirplaneMode, Settings.Global.getInt(
                mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON));
        assertEquals(expectedPrivateDnsMode,
                ConnectivitySettingsManager.getPrivateDnsMode(mContext));
        assertEquals(expectedAvoidBadWifi,
                ConnectivitySettingsManager.getNetworkAvoidBadWifi(mContext));
    }

    /**
     * Verify that per-app OEM network preference functions as expected for network preference TEST.
     * For specified apps, validate networks are prioritized in order: unmetered, TEST transport,
     * default network.
     */
    @AppModeFull(reason = "Instant apps cannot create test networks")
    @Test
    public void testSetOemNetworkPreferenceForTestPref() throws Exception {
        // Cannot use @IgnoreUpTo(Build.VERSION_CODES.R) because this test also requires API 31
        // shims, and @IgnoreUpTo does not check that.
        assumeTrue(TestUtils.shouldTestSApis());
        assumeTrue(mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final TestNetworkTracker tnt = callWithShellPermissionIdentity(
                () -> initTestNetwork(mContext, TEST_LINKADDR, NETWORK_CALLBACK_TIMEOUT_MS));
        final TestableNetworkCallback defaultCallback = new TestableNetworkCallback();
        final TestableNetworkCallback systemDefaultCallback = new TestableNetworkCallback();

        final Network wifiNetwork = mCtsNetUtils.ensureWifiConnected();
        final NetworkCapabilities wifiNetworkCapabilities = callWithShellPermissionIdentity(
                () -> mCm.getNetworkCapabilities(wifiNetwork));
        final String ssid = unquoteSSID(wifiNetworkCapabilities.getSsid());
        final boolean oldMeteredValue = wifiNetworkCapabilities.isMetered();

        try {
            // This network will be used for unmetered.
            setWifiMeteredStatusAndWait(ssid, false /* isMetered */);

            setOemNetworkPreferenceForMyPackage(OemNetworkPreferences.OEM_NETWORK_PREFERENCE_TEST);
            registerTestOemNetworkPreferenceCallbacks(defaultCallback, systemDefaultCallback);

            // Validate that an unmetered network is used over other networks.
            waitForAvailable(defaultCallback, wifiNetwork);
            waitForAvailable(systemDefaultCallback, wifiNetwork);

            // Validate when setting unmetered to metered, unmetered is lost and replaced by the
            // network with the TEST transport.
            setWifiMeteredStatusAndWait(ssid, true /* isMetered */);
            defaultCallback.expectCallback(CallbackEntry.LOST, wifiNetwork,
                    NETWORK_CALLBACK_TIMEOUT_MS);
            waitForAvailable(defaultCallback, tnt.getNetwork());
            // Depending on if this device has cellular connectivity or not, multiple available
            // callbacks may be received. Eventually, metered Wi-Fi should be the final available
            // callback in any case therefore confirm its receipt before continuing to assure the
            // system is in the expected state.
            waitForAvailable(systemDefaultCallback, TRANSPORT_WIFI);
        } finally {
            // Validate that removing the test network will fallback to the default network.
            runWithShellPermissionIdentity(tnt::teardown);
            defaultCallback.expectCallback(CallbackEntry.LOST, tnt.getNetwork(),
                    NETWORK_CALLBACK_TIMEOUT_MS);
            waitForAvailable(defaultCallback);

            setWifiMeteredStatusAndWait(ssid, oldMeteredValue);

            // Cleanup any prior test state from setOemNetworkPreference
            clearOemNetworkPreference();
            unregisterTestOemNetworkPreferenceCallbacks(defaultCallback, systemDefaultCallback);
        }
    }

    /**
     * Verify that per-app OEM network preference functions as expected for network pref TEST_ONLY.
     * For specified apps, validate that only TEST transport type networks are used.
     */
    @AppModeFull(reason = "Instant apps cannot create test networks")
    @Test
    public void testSetOemNetworkPreferenceForTestOnlyPref() throws Exception {
        // Cannot use @IgnoreUpTo(Build.VERSION_CODES.R) because this test also requires API 31
        // shims, and @IgnoreUpTo does not check that.
        assumeTrue(TestUtils.shouldTestSApis());

        final TestNetworkTracker tnt = callWithShellPermissionIdentity(
                () -> initTestNetwork(mContext, TEST_LINKADDR, NETWORK_CALLBACK_TIMEOUT_MS));
        final TestableNetworkCallback defaultCallback = new TestableNetworkCallback();
        final TestableNetworkCallback systemDefaultCallback = new TestableNetworkCallback();

        final Network wifiNetwork = mCtsNetUtils.ensureWifiConnected();

        try {
            setOemNetworkPreferenceForMyPackage(
                    OemNetworkPreferences.OEM_NETWORK_PREFERENCE_TEST_ONLY);
            registerTestOemNetworkPreferenceCallbacks(defaultCallback, systemDefaultCallback);
            waitForAvailable(defaultCallback, tnt.getNetwork());
            waitForAvailable(systemDefaultCallback, wifiNetwork);
        } finally {
            runWithShellPermissionIdentity(tnt::teardown);
            defaultCallback.expectCallback(CallbackEntry.LOST, tnt.getNetwork(),
                    NETWORK_CALLBACK_TIMEOUT_MS);

            // This network preference should only ever use the test network therefore available
            // should not trigger when the test network goes down (e.g. switch to cellular).
            defaultCallback.assertNoCallback();
            // The system default should still be connected to Wi-fi
            assertEquals(wifiNetwork, systemDefaultCallback.getLastAvailableNetwork());

            // Cleanup any prior test state from setOemNetworkPreference
            clearOemNetworkPreference();

            // The default (non-test) network should be available as the network pref was cleared.
            waitForAvailable(defaultCallback);
            unregisterTestOemNetworkPreferenceCallbacks(defaultCallback, systemDefaultCallback);
        }
    }

    private void unregisterTestOemNetworkPreferenceCallbacks(
            @NonNull final TestableNetworkCallback defaultCallback,
            @NonNull final TestableNetworkCallback systemDefaultCallback) {
        mCm.unregisterNetworkCallback(defaultCallback);
        mCm.unregisterNetworkCallback(systemDefaultCallback);
    }

    private void registerTestOemNetworkPreferenceCallbacks(
            @NonNull final TestableNetworkCallback defaultCallback,
            @NonNull final TestableNetworkCallback systemDefaultCallback) {
        mCm.registerDefaultNetworkCallback(defaultCallback);
        runWithShellPermissionIdentity(() ->
                mCmShim.registerSystemDefaultNetworkCallback(systemDefaultCallback,
                        new Handler(Looper.getMainLooper())), NETWORK_SETTINGS);
    }

    private static final class OnCompleteListenerCallback {
        final CompletableFuture<Object> mDone = new CompletableFuture<>();

        public void onComplete() {
            mDone.complete(new Object());
        }

        void expectOnComplete() throws Exception {
            try {
                mDone.get(NETWORK_CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            } catch (TimeoutException e) {
                fail("Expected onComplete() not received after "
                        + NETWORK_CALLBACK_TIMEOUT_MS + " ms");
            }
        }
    }

    private void setOemNetworkPreferenceForMyPackage(final int networkPref) throws Exception {
        final OemNetworkPreferences pref = new OemNetworkPreferences.Builder()
                .addNetworkPreference(mContext.getPackageName(), networkPref)
                .build();
        final OnCompleteListenerCallback oemPrefListener = new OnCompleteListenerCallback();
        mUiAutomation.adoptShellPermissionIdentity();
        try {
            mCm.setOemNetworkPreference(
                    pref, mContext.getMainExecutor(), oemPrefListener::onComplete);
        } finally {
            mUiAutomation.dropShellPermissionIdentity();
        }
        oemPrefListener.expectOnComplete();
    }

    /**
     * This will clear the OEM network preference on the device. As there is currently no way of
     * getting the existing preference, if this is executed while an existing preference is in
     * place, that preference will need to be reapplied after executing this test.
     * @throws Exception
     */
    private void clearOemNetworkPreference() throws Exception {
        final OemNetworkPreferences clearPref = new OemNetworkPreferences.Builder().build();
        final OnCompleteListenerCallback oemPrefListener = new OnCompleteListenerCallback();
        mUiAutomation.adoptShellPermissionIdentity();
        try {
            mCm.setOemNetworkPreference(
                    clearPref, mContext.getMainExecutor(), oemPrefListener::onComplete);
        } finally {
            mUiAutomation.dropShellPermissionIdentity();
        }
        oemPrefListener.expectOnComplete();
    }

    @Test
    public void testSetAcceptPartialConnectivity_NoPermission_GetException() {
        assumeTrue(TestUtils.shouldTestSApis());
        assertThrows(SecurityException.class, () -> mCm.setAcceptPartialConnectivity(
                mCm.getActiveNetwork(), false /* accept */ , false /* always */));
    }

    @AppModeFull(reason = "WRITE_DEVICE_CONFIG permission can't be granted to instant apps")
    @Test
    public void testAcceptPartialConnectivity_validatedNetwork() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());
        assumeTrue("testAcceptPartialConnectivity_validatedNetwork cannot execute"
                        + " unless device supports WiFi",
                mPackageManager.hasSystemFeature(FEATURE_WIFI));

        try {
            // Wait for partial connectivity to be detected on the network
            final Network network = preparePartialConnectivity();

            runAsShell(NETWORK_SETTINGS, () -> {
                // The always bit is verified in NetworkAgentTest
                mCm.setAcceptPartialConnectivity(network, true /* accept */, false /* always */);
            });

            // Accept partial connectivity network should result in a validated network
            expectNetworkHasCapability(network, NET_CAPABILITY_VALIDATED, WIFI_CONNECT_TIMEOUT_MS);
        } finally {
            resetValidationConfig();
            // Reconnect wifi to reset the wifi status
            reconnectWifi();
        }
    }

    @AppModeFull(reason = "WRITE_DEVICE_CONFIG permission can't be granted to instant apps")
    @Test
    public void testRejectPartialConnectivity_TearDownNetwork() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());
        assumeTrue("testAcceptPartialConnectivity_validatedNetwork cannot execute"
                        + " unless device supports WiFi",
                mPackageManager.hasSystemFeature(FEATURE_WIFI));

        final TestNetworkCallback cb = new TestNetworkCallback();
        try {
            // Wait for partial connectivity to be detected on the network
            final Network network = preparePartialConnectivity();

            mCm.requestNetwork(makeWifiNetworkRequest(), cb);
            runAsShell(NETWORK_SETTINGS, () -> {
                // The always bit is verified in NetworkAgentTest
                mCm.setAcceptPartialConnectivity(network, false /* accept */, false /* always */);
            });
            // Reject partial connectivity network should cause the network being torn down
            assertEquals(network, cb.waitForLost());
        } finally {
            mCm.unregisterNetworkCallback(cb);
            resetValidationConfig();
            // Wifi will not automatically reconnect to the network. ensureWifiDisconnected cannot
            // apply here. Thus, turn off wifi first and restart to restore.
            runShellCommand("svc wifi disable");
            mCtsNetUtils.ensureWifiConnected();
        }
    }

    @Test
    public void testSetAcceptUnvalidated_NoPermission_GetException() {
        assumeTrue(TestUtils.shouldTestSApis());
        assertThrows(SecurityException.class, () -> mCm.setAcceptUnvalidated(
                mCm.getActiveNetwork(), false /* accept */ , false /* always */));
    }

    @AppModeFull(reason = "WRITE_DEVICE_CONFIG permission can't be granted to instant apps")
    @Test
    public void testRejectUnvalidated_TearDownNetwork() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());
        final boolean canRunTest = mPackageManager.hasSystemFeature(FEATURE_WIFI)
                && mPackageManager.hasSystemFeature(FEATURE_TELEPHONY);
        assumeTrue("testAcceptPartialConnectivity_validatedNetwork cannot execute"
                        + " unless device supports WiFi and telephony", canRunTest);

        final TestableNetworkCallback wifiCb = new TestableNetworkCallback();
        try {
            // Ensure at least one default network candidate connected.
            mCtsNetUtils.connectToCell();

            final Network wifiNetwork = prepareUnvalidatedNetwork();
            // Default network should not be wifi ,but checking that wifi is not the default doesn't
            // guarantee that it won't become the default in the future.
            assertNotEquals(wifiNetwork, mCm.getActiveNetwork());

            mCm.registerNetworkCallback(makeWifiNetworkRequest(), wifiCb);
            runAsShell(NETWORK_SETTINGS, () -> {
                mCm.setAcceptUnvalidated(wifiNetwork, false /* accept */, false /* always */);
            });
            waitForLost(wifiCb);
        } finally {
            mCm.unregisterNetworkCallback(wifiCb);
            resetValidationConfig();
            /// Wifi will not automatically reconnect to the network. ensureWifiDisconnected cannot
            // apply here. Thus, turn off wifi first and restart to restore.
            runShellCommand("svc wifi disable");
            mCtsNetUtils.ensureWifiConnected();
        }
    }

    @AppModeFull(reason = "WRITE_DEVICE_CONFIG permission can't be granted to instant apps")
    @Test
    public void testSetAvoidUnvalidated() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());
        // TODO: Allow in debuggable ROM only. To be replaced by FabricatedOverlay
        assumeTrue(Build.isDebuggable());
        final boolean canRunTest = mPackageManager.hasSystemFeature(FEATURE_WIFI)
                && mPackageManager.hasSystemFeature(FEATURE_TELEPHONY);
        assumeTrue("testSetAvoidUnvalidated cannot execute"
                + " unless device supports WiFi and telephony", canRunTest);

        final TestableNetworkCallback wifiCb = new TestableNetworkCallback();
        final TestableNetworkCallback defaultCb = new TestableNetworkCallback();
        final int previousAvoidBadWifi =
                ConnectivitySettingsManager.getNetworkAvoidBadWifi(mContext);

        allowBadWifi();

        final Network cellNetwork = mCtsNetUtils.connectToCell();
        final Network wifiNetwork = prepareValidatedNetwork();

        mCm.registerDefaultNetworkCallback(defaultCb);
        mCm.registerNetworkCallback(makeWifiNetworkRequest(), wifiCb);

        try {
            // Verify wifi is the default network.
            defaultCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> wifiNetwork.equals(entry.getNetwork()));
            wifiCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> wifiNetwork.equals(entry.getNetwork()));
            assertTrue(mCm.getNetworkCapabilities(wifiNetwork).hasCapability(
                    NET_CAPABILITY_VALIDATED));

            // Configure response code for unvalidated network
            configTestServer(Status.INTERNAL_ERROR, Status.INTERNAL_ERROR);
            mCm.reportNetworkConnectivity(wifiNetwork, false);
            // Default network should stay on unvalidated wifi because avoid bad wifi is disabled.
            defaultCb.eventuallyExpect(CallbackEntry.NETWORK_CAPS_UPDATED,
                    NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> !((CallbackEntry.CapabilitiesChanged) entry).getCaps()
                            .hasCapability(NET_CAPABILITY_VALIDATED));
            wifiCb.eventuallyExpect(CallbackEntry.NETWORK_CAPS_UPDATED,
                    NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> !((CallbackEntry.CapabilitiesChanged) entry).getCaps()
                            .hasCapability(NET_CAPABILITY_VALIDATED));

            runAsShell(NETWORK_SETTINGS, () -> {
                mCm.setAvoidUnvalidated(wifiNetwork);
            });
            // Default network should be updated to validated cellular network.
            defaultCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> cellNetwork.equals(entry.getNetwork()));
            // No update on wifi callback.
            wifiCb.assertNoCallback();
        } finally {
            mCm.unregisterNetworkCallback(wifiCb);
            mCm.unregisterNetworkCallback(defaultCb);
            resetAvoidBadWifi(previousAvoidBadWifi);
            resetValidationConfig();
            // Reconnect wifi to reset the wifi status
            reconnectWifi();
        }
    }

    private void resetAvoidBadWifi(int settingValue) {
        setTestAllowBadWifiResource(0 /* timeMs */);
        ConnectivitySettingsManager.setNetworkAvoidBadWifi(mContext, settingValue);
    }

    private void allowBadWifi() {
        setTestAllowBadWifiResource(
                System.currentTimeMillis() + WIFI_CONNECT_TIMEOUT_MS /* timeMs */);
        ConnectivitySettingsManager.setNetworkAvoidBadWifi(mContext,
                ConnectivitySettingsManager.NETWORK_AVOID_BAD_WIFI_IGNORE);
    }

    private void setTestAllowBadWifiResource(long timeMs) {
        runAsShell(NETWORK_SETTINGS, () -> {
            mCm.setTestAllowBadWifiUntil(timeMs);
        });
    }

    private Network expectNetworkHasCapability(Network network, int expectedNetCap, long timeout)
            throws Exception {
        final CompletableFuture<Network> future = new CompletableFuture();
        final NetworkCallback cb = new NetworkCallback() {
            @Override
            public void onCapabilitiesChanged(Network n, NetworkCapabilities nc) {
                if (n.equals(network) && nc.hasCapability(expectedNetCap)) {
                    future.complete(network);
                }
            }
        };

        try {
            mCm.registerNetworkCallback(new NetworkRequest.Builder().build(), cb);
            return future.get(timeout, TimeUnit.MILLISECONDS);
        } finally {
            mCm.unregisterNetworkCallback(cb);
        }
    }

    private void resetValidationConfig() {
        NetworkValidationTestUtil.clearValidationTestUrlsDeviceConfig();
        mHttpServer.stop();
    }

    private void prepareHttpServer() throws Exception {
        runAsShell(READ_DEVICE_CONFIG, () -> {
            // Verify that the test URLs are not normally set on the device, but do not fail if the
            // test URLs are set to what this test uses (URLs on localhost), in case the test was
            // interrupted manually and rerun.
            assertEmptyOrLocalhostUrl(TEST_CAPTIVE_PORTAL_HTTPS_URL);
            assertEmptyOrLocalhostUrl(TEST_CAPTIVE_PORTAL_HTTP_URL);
        });

        NetworkValidationTestUtil.clearValidationTestUrlsDeviceConfig();

        mHttpServer.start();
    }

    private Network reconnectWifi() {
        mCtsNetUtils.ensureWifiDisconnected(null /* wifiNetworkToCheck */);
        return mCtsNetUtils.ensureWifiConnected();
    }

    private Network prepareValidatedNetwork() throws Exception {
        prepareHttpServer();
        configTestServer(Status.NO_CONTENT, Status.NO_CONTENT);
        // Disconnect wifi first then start wifi network with configuration.
        final Network wifiNetwork = reconnectWifi();

        return expectNetworkHasCapability(wifiNetwork, NET_CAPABILITY_VALIDATED,
                WIFI_CONNECT_TIMEOUT_MS);
    }

    private Network preparePartialConnectivity() throws Exception {
        prepareHttpServer();
        // Configure response code for partial connectivity
        configTestServer(Status.INTERNAL_ERROR  /* httpsStatusCode */,
                Status.NO_CONTENT  /* httpStatusCode */);
        // Disconnect wifi first then start wifi network with configuration.
        mCtsNetUtils.ensureWifiDisconnected(null /* wifiNetworkToCheck */);
        final Network network = mCtsNetUtils.ensureWifiConnected();

        return expectNetworkHasCapability(network, NET_CAPABILITY_PARTIAL_CONNECTIVITY,
                WIFI_CONNECT_TIMEOUT_MS);
    }

    private Network prepareUnvalidatedNetwork() throws Exception {
        prepareHttpServer();
        // Configure response code for unvalidated network
        configTestServer(Status.INTERNAL_ERROR /* httpsStatusCode */,
                Status.INTERNAL_ERROR /* httpStatusCode */);

        // Disconnect wifi first then start wifi network with configuration.
        mCtsNetUtils.ensureWifiDisconnected(null /* wifiNetworkToCheck */);
        final Network wifiNetwork = mCtsNetUtils.ensureWifiConnected();
        return expectNetworkHasCapability(wifiNetwork, NET_CAPABILITY_INTERNET,
                WIFI_CONNECT_TIMEOUT_MS);
    }

    private String makeUrl(String path) {
        return "http://localhost:" + mHttpServer.getListeningPort() + path;
    }

    private void assertEmptyOrLocalhostUrl(String urlKey) {
        final String url = DeviceConfig.getProperty(DeviceConfig.NAMESPACE_CONNECTIVITY, urlKey);
        assertTrue(urlKey + " must not be set in production scenarios, current value= " + url,
                TextUtils.isEmpty(url) || LOCALHOST_HOSTNAME.equals(Uri.parse(url).getHost()));
    }

    private void configTestServer(IStatus httpsStatusCode, IStatus httpStatusCode) {
        mHttpServer.addResponse(new TestHttpServer.Request(
                TEST_HTTPS_URL_PATH, Method.GET, "" /* queryParameters */),
                httpsStatusCode, null /* locationHeader */, "" /* content */);
        mHttpServer.addResponse(new TestHttpServer.Request(
                TEST_HTTP_URL_PATH, Method.GET, "" /* queryParameters */),
                httpStatusCode, null /* locationHeader */, "" /* content */);
        NetworkValidationTestUtil.setHttpsUrlDeviceConfig(makeUrl(TEST_HTTPS_URL_PATH));
        NetworkValidationTestUtil.setHttpUrlDeviceConfig(makeUrl(TEST_HTTP_URL_PATH));
        NetworkValidationTestUtil.setUrlExpirationDeviceConfig(
                System.currentTimeMillis() + WIFI_CONNECT_TIMEOUT_MS);
    }

    @AppModeFull(reason = "Cannot get WifiManager in instant app mode")
    @Test
    public void testMobileDataPreferredUids() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());
        final boolean canRunTest = mPackageManager.hasSystemFeature(FEATURE_WIFI)
                && mPackageManager.hasSystemFeature(FEATURE_TELEPHONY);
        assumeTrue("testMobileDataPreferredUidsWithCallback cannot execute"
                + " unless device supports both WiFi and telephony", canRunTest);

        final int uid = mPackageManager.getPackageUid(mContext.getPackageName(), 0 /* flag */);
        final Set<Integer> mobileDataPreferredUids =
                ConnectivitySettingsManager.getMobileDataPreferredUids(mContext);
        // CtsNetTestCases uid should not list in MOBILE_DATA_PREFERRED_UIDS setting because it just
        // installs to device. In case the uid is existed in setting mistakenly, try to remove the
        // uid and set correct uids to setting.
        mobileDataPreferredUids.remove(uid);
        ConnectivitySettingsManager.setMobileDataPreferredUids(mContext, mobileDataPreferredUids);

        // For testing mobile data preferred uids feature, it needs both wifi and cell network.
        final Network wifiNetwork = mCtsNetUtils.ensureWifiConnected();
        final Network cellNetwork = mCtsNetUtils.connectToCell();
        final TestableNetworkCallback defaultTrackingCb = new TestableNetworkCallback();
        final TestableNetworkCallback systemDefaultCb = new TestableNetworkCallback();
        final Handler h = new Handler(Looper.getMainLooper());
        runWithShellPermissionIdentity(() -> mCm.registerSystemDefaultNetworkCallback(
                systemDefaultCb, h), NETWORK_SETTINGS);
        mCm.registerDefaultNetworkCallback(defaultTrackingCb);

        try {
            // CtsNetTestCases uid is not listed in MOBILE_DATA_PREFERRED_UIDS setting, so the
            // per-app default network should be same as system default network.
            waitForAvailable(systemDefaultCb, wifiNetwork);
            defaultTrackingCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> wifiNetwork.equals(entry.getNetwork()));
            // Active network for CtsNetTestCases uid should be wifi now.
            assertEquals(wifiNetwork, mCm.getActiveNetwork());

            // Add CtsNetTestCases uid to MOBILE_DATA_PREFERRED_UIDS setting, then available per-app
            // default network callback should be received with cell network.
            final Set<Integer> newMobileDataPreferredUids = new ArraySet<>(mobileDataPreferredUids);
            newMobileDataPreferredUids.add(uid);
            ConnectivitySettingsManager.setMobileDataPreferredUids(
                    mContext, newMobileDataPreferredUids);
            defaultTrackingCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> cellNetwork.equals(entry.getNetwork()));
            // System default network doesn't change.
            systemDefaultCb.assertNoCallback();
            // Active network for CtsNetTestCases uid should change to cell, too.
            assertEquals(cellNetwork, mCm.getActiveNetwork());

            // Remove CtsNetTestCases uid from MOBILE_DATA_PREFERRED_UIDS setting, then available
            // per-app default network callback should be received again with system default network
            newMobileDataPreferredUids.remove(uid);
            ConnectivitySettingsManager.setMobileDataPreferredUids(
                    mContext, newMobileDataPreferredUids);
            defaultTrackingCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> wifiNetwork.equals(entry.getNetwork()));
            // System default network still doesn't change.
            systemDefaultCb.assertNoCallback();
            // Active network for CtsNetTestCases uid should change back to wifi.
            assertEquals(wifiNetwork, mCm.getActiveNetwork());
        } finally {
            mCm.unregisterNetworkCallback(systemDefaultCb);
            mCm.unregisterNetworkCallback(defaultTrackingCb);

            // Restore setting.
            ConnectivitySettingsManager.setMobileDataPreferredUids(
                    mContext, mobileDataPreferredUids);
        }
    }

    /** Wait for assigned time. */
    private void waitForMs(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            fail("Thread was interrupted");
        }
    }

    private void assertBindSocketToNetworkSuccess(final Network network) throws Exception {
        final CompletableFuture<Boolean> future = new CompletableFuture<>();
        final ExecutorService executor = Executors.newSingleThreadExecutor();
        try {
            executor.execute(() -> {
                for (int i = 0; i < 30; i++) {
                    waitForMs(100);

                    try (Socket socket = new Socket()) {
                        network.bindSocket(socket);
                        future.complete(true);
                        return;
                    } catch (IOException e) { }
                }
            });
            assertTrue(future.get(APPLYING_UIDS_ALLOWED_ON_RESTRICTED_NETWORKS_TIMEOUT_MS,
                    TimeUnit.MILLISECONDS));
        } finally {
            executor.shutdown();
        }
    }

    @AppModeFull(reason = "WRITE_SECURE_SETTINGS permission can't be granted to instant apps")
    @Test
    public void testUidsAllowedOnRestrictedNetworks() throws Exception {
        assumeTrue(TestUtils.shouldTestSApis());

        // TODO (b/175199465): figure out a reasonable permission check for
        //  setUidsAllowedOnRestrictedNetworks that allows tests but not system-external callers.
        assumeTrue(Build.isDebuggable());

        final int uid = mPackageManager.getPackageUid(mContext.getPackageName(), 0 /* flag */);
        final Set<Integer> originalUidsAllowedOnRestrictedNetworks =
                ConnectivitySettingsManager.getUidsAllowedOnRestrictedNetworks(mContext);
        // CtsNetTestCases uid should not list in UIDS_ALLOWED_ON_RESTRICTED_NETWORKS setting
        // because it has been just installed to device. In case the uid is existed in setting
        // mistakenly, try to remove the uid and set correct uids to setting.
        originalUidsAllowedOnRestrictedNetworks.remove(uid);
        runWithShellPermissionIdentity(() ->
                ConnectivitySettingsManager.setUidsAllowedOnRestrictedNetworks(
                        mContext, originalUidsAllowedOnRestrictedNetworks), NETWORK_SETTINGS);

        final Handler h = new Handler(Looper.getMainLooper());
        final TestableNetworkCallback testNetworkCb = new TestableNetworkCallback();
        mCm.registerBestMatchingNetworkCallback(new NetworkRequest.Builder().clearCapabilities()
                .addTransportType(NetworkCapabilities.TRANSPORT_TEST).build(), testNetworkCb, h);

        // Create test network agent with restricted network.
        final NetworkCapabilities nc = new NetworkCapabilities.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_TEST)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_SUSPENDED)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_ROAMING)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VCN_MANAGED)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_TRUSTED)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED)
                .build();
        final NetworkScore score = new NetworkScore.Builder()
                .setExiting(false)
                .setTransportPrimary(false)
                .setKeepConnectedReason(NetworkScore.KEEP_CONNECTED_FOR_HANDOVER)
                .build();
        final NetworkAgent agent = new NetworkAgent(mContext, Looper.getMainLooper(),
                TAG, nc, new LinkProperties(), score, new NetworkAgentConfig.Builder().build(),
                new NetworkProvider(mContext, Looper.getMainLooper(), TAG)) {};
        runWithShellPermissionIdentity(() -> agent.register(),
                android.Manifest.permission.MANAGE_TEST_NETWORKS);
        agent.markConnected();

        final Network network = agent.getNetwork();

        try (Socket socket = new Socket()) {
            testNetworkCb.eventuallyExpect(CallbackEntry.AVAILABLE, NETWORK_CALLBACK_TIMEOUT_MS,
                    entry -> network.equals(entry.getNetwork()));
            // Verify that the network is restricted.
            final NetworkCapabilities testNetworkNc = mCm.getNetworkCapabilities(network);
            assertNotNull(testNetworkNc);
            assertFalse(testNetworkNc.hasCapability(
                    NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED));
            // CtsNetTestCases package doesn't hold CONNECTIVITY_USE_RESTRICTED_NETWORKS, so it
            // does not allow to bind socket to restricted network.
            assertThrows(IOException.class, () -> network.bindSocket(socket));

            // Add CtsNetTestCases uid to UIDS_ALLOWED_ON_RESTRICTED_NETWORKS setting, then it can
            // bind socket to restricted network normally.
            final Set<Integer> newUidsAllowedOnRestrictedNetworks =
                    new ArraySet<>(originalUidsAllowedOnRestrictedNetworks);
            newUidsAllowedOnRestrictedNetworks.add(uid);
            runWithShellPermissionIdentity(() ->
                    ConnectivitySettingsManager.setUidsAllowedOnRestrictedNetworks(
                            mContext, newUidsAllowedOnRestrictedNetworks), NETWORK_SETTINGS);
            // Wait a while for sending allowed uids on the restricted network to netd.
            // TODD: Have a significant signal to know the uids has been send to netd.
            assertBindSocketToNetworkSuccess(network);
        } finally {
            mCm.unregisterNetworkCallback(testNetworkCb);
            agent.unregister();

            // Restore setting.
            runWithShellPermissionIdentity(() ->
                    ConnectivitySettingsManager.setUidsAllowedOnRestrictedNetworks(
                            mContext, originalUidsAllowedOnRestrictedNetworks), NETWORK_SETTINGS);
        }
    }
}
