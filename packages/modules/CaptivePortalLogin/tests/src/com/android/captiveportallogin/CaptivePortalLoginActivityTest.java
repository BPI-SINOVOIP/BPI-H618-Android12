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

package com.android.captiveportallogin;

import static android.Manifest.permission.MANAGE_TEST_NETWORKS;
import static android.app.Activity.RESULT_OK;
import static android.content.Intent.ACTION_CREATE_DOCUMENT;
import static android.net.ConnectivityManager.ACTION_CAPTIVE_PORTAL_SIGN_IN;
import static android.net.ConnectivityManager.EXTRA_CAPTIVE_PORTAL;
import static android.net.ConnectivityManager.EXTRA_CAPTIVE_PORTAL_URL;
import static android.net.ConnectivityManager.EXTRA_CAPTIVE_PORTAL_USER_AGENT;
import static android.net.ConnectivityManager.EXTRA_NETWORK;
import static android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED;
import static android.provider.DeviceConfig.NAMESPACE_CONNECTIVITY;

import static androidx.test.espresso.intent.Intents.intending;
import static androidx.test.espresso.intent.matcher.IntentMatchers.hasAction;
import static androidx.test.espresso.intent.matcher.IntentMatchers.isInternal;
import static androidx.test.espresso.web.sugar.Web.onWebView;
import static androidx.test.espresso.web.webdriver.DriverAtoms.findElement;
import static androidx.test.espresso.web.webdriver.DriverAtoms.webClick;
import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;
import static com.android.testutils.TestNetworkTrackerKt.initTestNetwork;

import static junit.framework.Assert.assertEquals;

import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.app.Instrumentation.ActivityResult;
import android.app.KeyguardManager;
import android.app.UiAutomation;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.CaptivePortal;
import android.net.CaptivePortalData;
import android.net.ConnectivityManager;
import android.net.InetAddresses;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Parcel;
import android.os.Parcelable;
import android.provider.DeviceConfig;

import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.intent.Intents;
import androidx.test.espresso.intent.rule.IntentsTestRule;
import androidx.test.espresso.web.webdriver.Locator;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.filters.SmallTest;

import com.android.testutils.TestNetworkTracker;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;
import org.mockito.quality.Strictness;

import java.io.IOException;
import java.lang.reflect.Method;
import java.net.ServerSocket;
import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;

import fi.iki.elonen.NanoHTTPD;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class CaptivePortalLoginActivityTest {
    private static final String TEST_URL = "http://android.test.com";
    private static final int TEST_NETID = 1234;
    private static final String TEST_NC_SSID = "Test NetworkCapabilities SSID";
    private static final String TEST_WIFIINFO_SSID = "Test Other SSID";
    private static final String TEST_URL_QUERY = "testquery";
    private static final long TEST_TIMEOUT_MS = 10_000L;
    private static final LinkAddress TEST_LINKADDR = new LinkAddress(
            InetAddresses.parseNumericAddress("2001:db8::8"), 64);
    private static final String TEST_USERAGENT = "Test/42.0 Unit-test";
    private static final String TEST_FRIENDLY_NAME = "Network friendly name";
    private InstrumentedCaptivePortalLoginActivity mActivity;
    private MockitoSession mSession;
    private Network mNetwork = new Network(TEST_NETID);
    private TestNetworkTracker mTestNetworkTracker;

    private static ConnectivityManager sConnectivityManager;
    private static WifiManager sMockWifiManager;
    private static DevicePolicyManager sMockDevicePolicyManager;

    public static class InstrumentedCaptivePortalLoginActivity extends CaptivePortalLoginActivity {
        private final ConditionVariable mDestroyedCv = new ConditionVariable(false);
        private final CompletableFuture<Intent> mForegroundServiceStart = new CompletableFuture<>();
        @Override
        public Object getSystemService(String name) {
            switch (name) {
                case Context.CONNECTIVITY_SERVICE:
                    return sConnectivityManager;
                case Context.DEVICE_POLICY_SERVICE:
                    return sMockDevicePolicyManager;
                case Context.WIFI_SERVICE:
                    return sMockWifiManager;
                default:
                    return super.getSystemService(name);
            }
        }

        @Override
        public ComponentName startForegroundService(Intent service) {
            assertTrue("Multiple foreground services were started during the test",
                    mForegroundServiceStart.complete(service));
            // Do not actually start the service
            return service.getComponent();
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            mDestroyedCv.open();
        }

        void waitForDestroy(long timeoutMs) {
            assertTrue("Activity not destroyed within timeout", mDestroyedCv.block(timeoutMs));
        }
    }

    /** Class to replace CaptivePortal to prevent mock object is updated and replaced by parcel. */
    public static class MockCaptivePortal extends CaptivePortal {
        int mDismissTimes;
        int mIgnoreTimes;
        int mUseTimes;

        private MockCaptivePortal() {
            this(0, 0, 0);
        }
        private MockCaptivePortal(int dismissTimes, int ignoreTimes, int useTimes) {
            super(null);
            mDismissTimes = dismissTimes;
            mIgnoreTimes = ignoreTimes;
            mUseTimes = useTimes;
        }
        @Override
        public void reportCaptivePortalDismissed() {
            mDismissTimes++;
        }

        @Override
        public void ignoreNetwork() {
            mIgnoreTimes++;
        }

        @Override
        public void useNetwork() {
            mUseTimes++;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            out.writeInt(mDismissTimes);
            out.writeInt(mIgnoreTimes);
            out.writeInt(mUseTimes);
        }

        public static final Parcelable.Creator<MockCaptivePortal> CREATOR =
                new Parcelable.Creator<MockCaptivePortal>() {
                @Override
                public MockCaptivePortal createFromParcel(Parcel in) {
                    return new MockCaptivePortal(in.readInt(), in.readInt(), in.readInt());
                }

                @Override
                public MockCaptivePortal[] newArray(int size) {
                    return new MockCaptivePortal[size];
                }
        };
    }

    @Rule
    public final IntentsTestRule mActivityRule =
            new IntentsTestRule<>(InstrumentedCaptivePortalLoginActivity.class,
                    false /* initialTouchMode */, false  /* launchActivity */);

    @Before
    public void setUp() throws Exception {
        final Context context = getInstrumentation().getContext();
        sConnectivityManager = spy(context.getSystemService(ConnectivityManager.class));
        sMockWifiManager = mock(WifiManager.class);
        sMockDevicePolicyManager = mock(DevicePolicyManager.class);
        MockitoAnnotations.initMocks(this);
        mSession = mockitoSession()
                .spyStatic(DeviceConfig.class)
                .strictness(Strictness.WARN)
                .startMocking();
        setDismissPortalInValidatedNetwork(true);
        // Use a real (but test) network for the application. The application will pass this
        // network to ConnectivityManager#bindProcessToNetwork, so it needs to be a real, existing
        // network on the device but otherwise has no functional use at all. The http server set up
        // by this test will run on the loopback interface and will not use this test network.
        final UiAutomation automation = getInstrumentation().getUiAutomation();
        automation.adoptShellPermissionIdentity(MANAGE_TEST_NETWORKS);
        try {
            mTestNetworkTracker = initTestNetwork(
                    getInstrumentation().getContext(), TEST_LINKADDR, TEST_TIMEOUT_MS);
        } finally {
            automation.dropShellPermissionIdentity();
        }
        mNetwork = mTestNetworkTracker.getNetwork();

        final WifiInfo testInfo = makeWifiInfo();
        doReturn(testInfo).when(sMockWifiManager).getConnectionInfo();
    }

    private static WifiInfo makeWifiInfo() throws Exception {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return new WifiInfo.Builder()
                    .setSsid(TEST_WIFIINFO_SSID.getBytes(StandardCharsets.US_ASCII))
                    .build();
        }

        // WifiInfo did not have a builder before R. Use non-public APIs on Q to set SSID.
        final WifiInfo info = WifiInfo.class.getConstructor().newInstance();
        final Class<?> wifiSsidClass = Class.forName("android.net.wifi.WifiSsid");
        final Object wifiSsid = wifiSsidClass.getMethod("createFromAsciiEncoded",
                String.class).invoke(null, TEST_WIFIINFO_SSID);
        WifiInfo.class.getMethod("setSSID", wifiSsidClass).invoke(info, wifiSsid);
        return info;
    }

    @After
    public void tearDown() throws Exception {
        mActivityRule.finishActivity();
        if (mActivity != null) mActivity.waitForDestroy(TEST_TIMEOUT_MS);
        getInstrumentation().getContext().getSystemService(ConnectivityManager.class)
                .bindProcessToNetwork(null);
        if (mTestNetworkTracker != null) mTestNetworkTracker.teardown();
        // finish mocking after the activity has terminated to avoid races on teardown.
        mSession.finishMocking();
    }

    private void initActivity(String url) {
        // onCreate will be triggered in launchActivity(). Handle mock objects after
        // launchActivity() if any new mock objects. Activity launching flow will be
        //  1. launchActivity()
        //  2. onCreate()
        //  3. end of launchActivity()
        mActivity = (InstrumentedCaptivePortalLoginActivity) mActivityRule.launchActivity(
            new Intent(ACTION_CAPTIVE_PORTAL_SIGN_IN)
                .putExtra(EXTRA_CAPTIVE_PORTAL_URL, url)
                .putExtra(EXTRA_NETWORK, mNetwork)
                .putExtra(EXTRA_CAPTIVE_PORTAL_USER_AGENT, TEST_USERAGENT)
                .putExtra(EXTRA_CAPTIVE_PORTAL, new MockCaptivePortal())
        );
        // Verify activity created successfully.
        assertNotNull(mActivity);
        getInstrumentation().getContext().getSystemService(KeyguardManager.class)
                .requestDismissKeyguard(mActivity, null);
        // Dismiss dialogs or notification shade, so that the test can interact with the activity.
        mActivity.sendBroadcast(new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));
        getInstrumentation().waitForIdleSync();
    }

    @Test
    public void testonCreateWithNullCaptivePortal() throws Exception {
        mActivity = (InstrumentedCaptivePortalLoginActivity) mActivityRule.launchActivity(
                new Intent(ACTION_CAPTIVE_PORTAL_SIGN_IN)
                    .putExtra(EXTRA_CAPTIVE_PORTAL_URL, TEST_URL)
                    .putExtra(EXTRA_NETWORK, mNetwork)
                    .putExtra(EXTRA_CAPTIVE_PORTAL_USER_AGENT, TEST_USERAGENT)
                    .putExtra(EXTRA_CAPTIVE_PORTAL, (Bundle) null));
        // Verify that activity is still created but waiting for closing.
        assertNotNull(mActivity);
    }

    private MockCaptivePortal getCaptivePortal() {
        return (MockCaptivePortal) mActivity.mCaptivePortal;
    }

    private void configNonVpnNetwork() {
        final Network[] networks = new Network[] {new Network(mNetwork)};
        doReturn(networks).when(sConnectivityManager).getAllNetworks();
        final NetworkCapabilities nonVpnCapabilities;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // SSID and NetworkCapabilities builder was added in R
            nonVpnCapabilities = new NetworkCapabilities.Builder()
                    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                    .setSsid(TEST_NC_SSID)
                    .build();
        } else {
            nonVpnCapabilities = new NetworkCapabilities()
                    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        }
        doReturn(nonVpnCapabilities).when(sConnectivityManager).getNetworkCapabilities(
                mNetwork);
    }

    private void configVpnNetwork() {
        final Network network1 = new Network(TEST_NETID + 1);
        final Network network2 = new Network(TEST_NETID + 2);
        final Network[] networks = new Network[] {network1, network2};
        doReturn(networks).when(sConnectivityManager).getAllNetworks();
        final NetworkCapabilities underlyingCapabilities = new NetworkCapabilities()
                .addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        final NetworkCapabilities vpnCapabilities = new NetworkCapabilities(underlyingCapabilities)
                .addTransportType(NetworkCapabilities.TRANSPORT_VPN);
        doReturn(underlyingCapabilities).when(sConnectivityManager).getNetworkCapabilities(
                network1);
        doReturn(vpnCapabilities).when(sConnectivityManager).getNetworkCapabilities(network2);
    }

    @Test
    public void testHasVpnNetwork() throws Exception {
        initActivity(TEST_URL);
        // Test non-vpn case.
        configNonVpnNetwork();
        assertFalse(mActivity.hasVpnNetwork());
        // Test vpn case.
        configVpnNetwork();
        assertTrue(mActivity.hasVpnNetwork());
    }

    @Test
    public void testIsAlwaysOnVpnEnabled() throws Exception {
        initActivity(TEST_URL);
        doReturn(false).when(sMockDevicePolicyManager).isAlwaysOnVpnLockdownEnabled(any());
        assertFalse(mActivity.isAlwaysOnVpnEnabled());
        doReturn(true).when(sMockDevicePolicyManager).isAlwaysOnVpnLockdownEnabled(any());
        assertTrue(mActivity.isAlwaysOnVpnEnabled());
    }

    private void runVpnMsgOrLinkToBrowser(boolean useVpnMatcher) {
        initActivity(TEST_URL);
        // Test non-vpn case.
        configNonVpnNetwork();
        doReturn(false).when(sMockDevicePolicyManager).isAlwaysOnVpnLockdownEnabled(any());
        final String linkMatcher = ".*<a[^>]+href.*";
        assertTrue(mActivity.getWebViewClient().getVpnMsgOrLinkToBrowser().matches(linkMatcher));

        // Test has vpn case.
        configVpnNetwork();
        final String vpnMatcher = ".*<div.*vpnwarning.*";
        assertTrue(mActivity.getWebViewClient().getVpnMsgOrLinkToBrowser().matches(vpnMatcher));

        // Test always-on vpn case.
        configNonVpnNetwork();
        doReturn(true).when(sMockDevicePolicyManager).isAlwaysOnVpnLockdownEnabled(any());
        assertTrue(mActivity.getWebViewClient().getVpnMsgOrLinkToBrowser().matches(
                (useVpnMatcher ? vpnMatcher : linkMatcher)));
    }

    @Test @SdkSuppress(maxSdkVersion = Build.VERSION_CODES.Q)
    public void testVpnMsgOrLinkToBrowser_BeforeR() throws Exception {
        // Before Android R, CaptivePortalLogin cannot call isAlwaysOnVpnLockdownEnabled() due to
        // permission denied. So CaptivePortalLogin doesn't know the status of VPN always-on, and it
        // simply provides a link for user to open the browser as usual.
        runVpnMsgOrLinkToBrowser(false /* useVpnMatcher */);
    }

    @Test @SdkSuppress(minSdkVersion = Build.VERSION_CODES.R)
    public void testVpnMsgOrLinkToBrowser() throws Exception {
        // After Android R(including), DevicePolicyManager allows the caller who has the
        // PERMISSION_MAINLINE_NETWORK_STACK can call the isAlwaysOnVpnLockdownEnabled() to get the
        // status of VPN always-on. So the CaptivePortalLogin could know the status of VPN always-on
        // and show the related warning message to the user.
        runVpnMsgOrLinkToBrowser(true /* useVpnMatcher */);
    }

    private void notifyCapabilitiesChanged(final NetworkCapabilities nc) {
        mActivity.handleCapabilitiesChanged(mNetwork, nc);
        getInstrumentation().waitForIdleSync();
    }

    private void verifyDismissed() {
        final MockCaptivePortal cp = getCaptivePortal();
        assertEquals(cp.mDismissTimes, 1);
        assertEquals(cp.mIgnoreTimes, 0);
        assertEquals(cp.mUseTimes, 0);
    }

    private void notifyValidatedChangedAndDismissed(final NetworkCapabilities nc) {
        notifyCapabilitiesChanged(nc);
        verifyDismissed();
    }

    private void verifyNotDone() {
        final MockCaptivePortal cp = getCaptivePortal();
        assertEquals(cp.mDismissTimes, 0);
        assertEquals(cp.mIgnoreTimes, 0);
        assertEquals(cp.mUseTimes, 0);
    }

    private void notifyValidatedChangedNotDone(final NetworkCapabilities nc) {
        notifyCapabilitiesChanged(nc);
        verifyNotDone();
    }

    private void verifyUseAsIs() {
        final MockCaptivePortal cp = getCaptivePortal();
        assertEquals(cp.mDismissTimes, 0);
        assertEquals(cp.mIgnoreTimes, 0);
        assertEquals(cp.mUseTimes, 1);
    }

    private void setDismissPortalInValidatedNetwork(final boolean enable) {
        // Feature is enabled if the package version greater than configuration. Instead of reading
        // the package version, use Long.MAX_VALUE to replace disable configuration and 1 for
        // enabling.
        doReturn(enable ? 1 : Long.MAX_VALUE).when(() -> DeviceConfig.getLong(
                NAMESPACE_CONNECTIVITY,
                CaptivePortalLoginActivity.DISMISS_PORTAL_IN_VALIDATED_NETWORK, 0 /* default */));
    }

    @Test
    public void testNetworkCapabilitiesUpdate() throws Exception {
        initActivity(TEST_URL);
        // NetworkCapabilities updates w/o NET_CAPABILITY_VALIDATED.
        final NetworkCapabilities nc = new NetworkCapabilities();
        notifyValidatedChangedNotDone(nc);

        // NetworkCapabilities updates w/ NET_CAPABILITY_VALIDATED.
        nc.setCapability(NET_CAPABILITY_VALIDATED, true);
        notifyValidatedChangedAndDismissed(nc);
    }

    @Test
    public void testNetworkCapabilitiesUpdateWithFlag() throws Exception {
        initActivity(TEST_URL);
        final NetworkCapabilities nc = new NetworkCapabilities();
        nc.setCapability(NET_CAPABILITY_VALIDATED, true);
        // Disable flag. Auto-dismiss should not happen.
        setDismissPortalInValidatedNetwork(false);
        notifyValidatedChangedNotDone(nc);

        // Enable flag. Auto-dismissed.
        setDismissPortalInValidatedNetwork(true);
        notifyValidatedChangedAndDismissed(nc);
    }

    private HttpServer runCustomSchemeTest(String linkUri) throws Exception {
        final HttpServer server = new HttpServer();
        server.setResponseBody(TEST_URL_QUERY,
                "<a id='tst_link' href='" + linkUri + "'>Test link</a>");

        server.start();
        ActivityScenario.launch(RequestDismissKeyguardActivity.class);
        initActivity(server.makeUrl(TEST_URL_QUERY));
        // Mock all external intents
        intending(not(isInternal())).respondWith(new ActivityResult(RESULT_OK, null));

        onWebView().withElement(findElement(Locator.ID, "tst_link")).perform(webClick());
        getInstrumentation().waitForIdleSync();
        return server;
    }

    @Test
    public void testTelScheme() throws Exception {
        final String telUri = "tel:0123456789";
        final HttpServer server = runCustomSchemeTest(telUri);

        final Intent sentIntent = Intents.getIntents().get(0);
        assertEquals(Intent.ACTION_DIAL, sentIntent.getAction());
        assertEquals(Uri.parse(telUri), sentIntent.getData());

        server.stop();
    }

    @Test
    public void testSmsScheme() throws Exception {
        final String telUri = "sms:0123456789";
        final HttpServer server = runCustomSchemeTest(telUri);

        final Intent sentIntent = Intents.getIntents().get(0);
        assertEquals(Intent.ACTION_SENDTO, sentIntent.getAction());
        assertEquals(Uri.parse(telUri), sentIntent.getData());

        server.stop();
    }

    @Test
    public void testUnsupportedScheme() throws Exception {
        final HttpServer server = runCustomSchemeTest("mailto:test@example.com");
        assertEquals(0, Intents.getIntents().size());

        onWebView().withElement(findElement(Locator.ID, "continue_link"))
                .perform(webClick());

        // The intent is sent in onDestroy(); there is no way to wait for that event, so poll
        // until the intent is found.
        assertTrue(isEventually(() -> Intents.getIntents().size() == 1, TEST_TIMEOUT_MS));
        verifyUseAsIs();
        final Intent sentIntent = Intents.getIntents().get(0);
        assertEquals(Intent.ACTION_VIEW, sentIntent.getAction());
        assertEquals(Uri.parse(server.makeUrl(TEST_URL_QUERY)), sentIntent.getData());

        server.stop();
    }

    @Test
    public void testDownload() throws Exception {
        // Setup the server with a single link on the portal page, leading to a download
        final HttpServer server = new HttpServer();
        final String linkIdDownload = "download";
        final String downloadQuery = "dl";
        final String filename = "testfile.png";
        final String mimetype = "image/png";
        server.setResponseBody(TEST_URL_QUERY,
                "<a id='" + linkIdDownload + "' href='?" + downloadQuery + "'>Download</a>");
        server.setResponse(downloadQuery, "This is a test file", mimetype, Collections.singletonMap(
                "Content-Disposition", "attachment; filename=\"" + filename + "\""));
        server.start();

        ActivityScenario.launch(RequestDismissKeyguardActivity.class);
        initActivity(server.makeUrl(TEST_URL_QUERY));

        // Create a mock file to be returned when mocking the file chooser
        final Context ctx = mActivity.getApplicationContext();
        final Intent mockFileResponse = new Intent();
        final Uri mockFile = Uri.parse("content://mockdata");
        mockFileResponse.setData(mockFile);

        // Mock file chooser and DownloadService intents
        intending(hasAction(ACTION_CREATE_DOCUMENT)).respondWith(
                new ActivityResult(RESULT_OK, mockFileResponse));
        // No intent fired yet
        assertEquals(0, Intents.getIntents().size());

        onWebView().withElement(findElement(Locator.ID, linkIdDownload))
                .perform(webClick());

        // The create file intent should be fired when the download starts
        assertTrue("Create file intent not received within timeout",
                isEventually(() -> Intents.getIntents().size() == 1, TEST_TIMEOUT_MS));

        final Intent fileIntent = Intents.getIntents().get(0);
        assertEquals(ACTION_CREATE_DOCUMENT, fileIntent.getAction());
        assertEquals(mimetype, fileIntent.getType());
        assertEquals(filename, fileIntent.getStringExtra(Intent.EXTRA_TITLE));

        // The download intent should be fired after the create file result is received
        final Intent dlIntent = mActivity.mForegroundServiceStart.get(
                TEST_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        assertEquals(DownloadService.class.getName(), dlIntent.getComponent().getClassName());
        assertEquals(mNetwork, dlIntent.getParcelableExtra(DownloadService.ARG_NETWORK));
        assertEquals(TEST_USERAGENT, dlIntent.getStringExtra(DownloadService.ARG_USERAGENT));
        final String expectedUrl = server.makeUrl(downloadQuery);
        assertEquals(expectedUrl, dlIntent.getStringExtra(DownloadService.ARG_URL));
        assertEquals(filename, dlIntent.getStringExtra(DownloadService.ARG_DISPLAY_NAME));
        assertEquals(mockFile, dlIntent.getParcelableExtra(DownloadService.ARG_OUTFILE));

        server.stop();
    }

    @Test
    public void testVenueFriendlyNameTitle() throws Exception {
        assumeTrue(isAtLeastS());
        final LinkProperties linkProperties = new LinkProperties();
        CaptivePortalData.Builder captivePortalDataBuilder = new CaptivePortalData.Builder();
        // TODO: Use reflection for setVenueFriendlyName until shims are available
        final Class captivePortalDataBuilderClass = captivePortalDataBuilder.getClass();
        final Method setVenueFriendlyNameMethod;

        setVenueFriendlyNameMethod = captivePortalDataBuilderClass.getDeclaredMethod(
                "setVenueFriendlyName", CharSequence.class);

        captivePortalDataBuilder = (CaptivePortalData.Builder)
                setVenueFriendlyNameMethod.invoke(captivePortalDataBuilder, TEST_FRIENDLY_NAME);

        final CaptivePortalData captivePortalData = captivePortalDataBuilder.build();
        linkProperties.setCaptivePortalData(captivePortalData);

        when(sConnectivityManager.getLinkProperties(mNetwork)).thenReturn(linkProperties);
        configNonVpnNetwork();
        initActivity("https://tc.example.com/");

        // Verify that the correct venue friendly name is used
        assertEquals(getInstrumentation().getContext().getString(R.string.action_bar_title,
                TEST_FRIENDLY_NAME), mActivity.getActionBar().getTitle());
    }

    @Test @SdkSuppress(maxSdkVersion = Build.VERSION_CODES.Q)
    public void testWifiSsid_Q() throws Exception {
        configNonVpnNetwork();
        initActivity("https://portal.example.com/");
        assertEquals(mActivity.getActionBar().getTitle(),
                getInstrumentation().getContext().getString(R.string.action_bar_title,
                        TEST_WIFIINFO_SSID));
    }

    @Test @SdkSuppress(minSdkVersion = Build.VERSION_CODES.R)
    public void testWifiSsid() throws Exception {
        configNonVpnNetwork();
        initActivity("https://portal.example.com/");
        assertEquals(mActivity.getActionBar().getTitle(),
                getInstrumentation().getContext().getString(R.string.action_bar_title,
                        TEST_NC_SSID));
    }

    /**
     * Check whether the device release or development API level is strictly higher than the passed
     * in level.
     *
     * @return True if the device supports an SDK that has or will have a higher version number,
     *         even if still in development.
     */
    private static boolean isReleaseOrDevelopmentApiAbove(int apiLevel) {
        // In-development API after n may have SDK_INT == n and CODENAME != REL
        // Stable API n has SDK_INT == n and CODENAME == REL.
        final int devApiLevel = Build.VERSION.SDK_INT
                + ("REL".equals(Build.VERSION.CODENAME) ? 0 : 1);
        return devApiLevel > apiLevel;
    }

    /**
     * Check whether the device supports in-development or final S networking APIs.
     */
    private static boolean isAtLeastS() {
        return isReleaseOrDevelopmentApiAbove(Build.VERSION_CODES.R);
    }

    private static boolean isEventually(BooleanSupplier condition, long timeout)
            throws InterruptedException {
        final long start = System.currentTimeMillis();
        do {
            if (condition.getAsBoolean()) return true;
            Thread.sleep(10);
        } while ((System.currentTimeMillis() - start) < timeout);

        return false;
    }

    private static class HttpServer extends NanoHTTPD {
        private final ServerSocket mSocket;
        // Responses per URL query
        private final HashMap<String, MockResponse> mResponses = new HashMap<>();

        private static final class MockResponse {
            private final String mBody;
            private final String mMimetype;
            private final Map<String, String> mHeaders;

            MockResponse(String body, String mimetype, Map<String, String> headers) {
                this.mBody = body;
                this.mMimetype = mimetype;
                this.mHeaders = Collections.unmodifiableMap(new HashMap<>(headers));
            }
        }

        HttpServer() throws IOException {
            this(new ServerSocket());
        }

        private HttpServer(ServerSocket socket) {
            // 0 as port for picking a port automatically
            super("localhost", 0);
            mSocket = socket;
        }

        @Override
        public ServerSocketFactory getServerSocketFactory() {
            return () -> mSocket;
        }

        private String makeUrl(String query) {
            return new Uri.Builder()
                    .scheme("http")
                    .encodedAuthority("localhost:" + mSocket.getLocalPort())
                    // Explicitly specify an empty path to match the format of URLs returned by
                    // WebView (for example in onDownloadStart)
                    .path("/")
                    .query(query)
                    .build()
                    .toString();
        }

        private void setResponseBody(String query, String body) {
            setResponse(query, body, NanoHTTPD.MIME_HTML, Collections.emptyMap());
        }

        private void setResponse(String query, String body, String mimetype,
                Map<String, String> headers) {
            mResponses.put(query, new MockResponse(body, mimetype, headers));
        }

        @Override
        public Response serve(IHTTPSession session) {
            final MockResponse mockResponse = mResponses.get(session.getQueryParameterString());
            if (mockResponse == null) {
                // Default response is a 404
                return super.serve(session);
            }

            final Response response = newFixedLengthResponse(Response.Status.OK,
                    mockResponse.mMimetype,
                    "<!doctype html>"
                    + "<html>"
                    + "<head><title>Test portal</title></head>"
                    + "<body>" + mockResponse.mBody + "</body>"
                    + "</html>");
            mockResponse.mHeaders.forEach(response::addHeader);
            return response;
        }
    }
}
