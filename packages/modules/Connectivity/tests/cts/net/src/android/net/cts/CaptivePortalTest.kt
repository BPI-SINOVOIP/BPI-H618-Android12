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

package android.net.cts

import android.Manifest.permission.CONNECTIVITY_INTERNAL
import android.Manifest.permission.NETWORK_SETTINGS
import android.Manifest.permission.READ_DEVICE_CONFIG
import android.content.pm.PackageManager.FEATURE_TELEPHONY
import android.content.pm.PackageManager.FEATURE_WATCH
import android.content.pm.PackageManager.FEATURE_WIFI
import android.net.ConnectivityManager
import android.net.ConnectivityManager.NetworkCallback
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkCapabilities.NET_CAPABILITY_CAPTIVE_PORTAL
import android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET
import android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED
import android.net.NetworkCapabilities.TRANSPORT_CELLULAR
import android.net.NetworkCapabilities.TRANSPORT_WIFI
import android.net.NetworkRequest
import android.net.Uri
import android.net.cts.NetworkValidationTestUtil.clearValidationTestUrlsDeviceConfig
import android.net.cts.NetworkValidationTestUtil.setHttpUrlDeviceConfig
import android.net.cts.NetworkValidationTestUtil.setHttpsUrlDeviceConfig
import android.net.cts.NetworkValidationTestUtil.setUrlExpirationDeviceConfig
import android.net.cts.util.CtsNetUtils
import android.net.util.NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTPS_URL
import android.net.util.NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTP_URL
import android.net.wifi.WifiManager
import android.os.Build
import android.platform.test.annotations.AppModeFull
import android.provider.DeviceConfig
import android.provider.DeviceConfig.NAMESPACE_CONNECTIVITY
import android.text.TextUtils
import androidx.test.platform.app.InstrumentationRegistry.getInstrumentation
import androidx.test.runner.AndroidJUnit4
import com.android.testutils.RecorderCallback
import com.android.testutils.TestHttpServer
import com.android.testutils.TestHttpServer.Request
import com.android.testutils.TestableNetworkCallback
import com.android.testutils.isDevSdkInRange
import com.android.testutils.runAsShell
import fi.iki.elonen.NanoHTTPD.Response.Status
import junit.framework.AssertionFailedError
import org.junit.After
import org.junit.Assume.assumeTrue
import org.junit.Assume.assumeFalse
import org.junit.Before
import org.junit.runner.RunWith
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
import kotlin.test.Test
import kotlin.test.assertNotEquals
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

private const val TEST_HTTPS_URL_PATH = "/https_path"
private const val TEST_HTTP_URL_PATH = "/http_path"
private const val TEST_PORTAL_URL_PATH = "/portal_path"

private const val LOCALHOST_HOSTNAME = "localhost"

// Re-connecting to the AP, obtaining an IP address, revalidating can take a long time
private const val WIFI_CONNECT_TIMEOUT_MS = 120_000L
private const val TEST_TIMEOUT_MS = 10_000L

private fun <T> CompletableFuture<T>.assertGet(timeoutMs: Long, message: String): T {
    try {
        return get(timeoutMs, TimeUnit.MILLISECONDS)
    } catch (e: TimeoutException) {
        throw AssertionFailedError(message)
    }
}

@AppModeFull(reason = "WRITE_DEVICE_CONFIG permission can't be granted to instant apps")
@RunWith(AndroidJUnit4::class)
class CaptivePortalTest {
    private val context: android.content.Context by lazy { getInstrumentation().context }
    private val wm by lazy { context.getSystemService(WifiManager::class.java) }
    private val cm by lazy { context.getSystemService(ConnectivityManager::class.java) }
    private val pm by lazy { context.packageManager }
    private val utils by lazy { CtsNetUtils(context) }

    private val server = TestHttpServer("localhost")

    @Before
    fun setUp() {
        runAsShell(READ_DEVICE_CONFIG) {
            // Verify that the test URLs are not normally set on the device, but do not fail if the
            // test URLs are set to what this test uses (URLs on localhost), in case the test was
            // interrupted manually and rerun.
            assertEmptyOrLocalhostUrl(TEST_CAPTIVE_PORTAL_HTTPS_URL)
            assertEmptyOrLocalhostUrl(TEST_CAPTIVE_PORTAL_HTTP_URL)
        }
        clearValidationTestUrlsDeviceConfig()
        server.start()
    }

    @After
    fun tearDown() {
        clearValidationTestUrlsDeviceConfig()
        if (pm.hasSystemFeature(FEATURE_WIFI)) {
            reconnectWifi()
        }
        server.stop()
    }

    private fun assertEmptyOrLocalhostUrl(urlKey: String) {
        val url = DeviceConfig.getProperty(NAMESPACE_CONNECTIVITY, urlKey)
        assertTrue(TextUtils.isEmpty(url) || LOCALHOST_HOSTNAME == Uri.parse(url).host,
                "$urlKey must not be set in production scenarios (current value: $url)")
    }

    @Test
    fun testCaptivePortalIsNotDefaultNetwork() {
        assumeTrue(pm.hasSystemFeature(FEATURE_TELEPHONY))
        assumeTrue(pm.hasSystemFeature(FEATURE_WIFI))
        assumeFalse(pm.hasSystemFeature(FEATURE_WATCH))
        utils.ensureWifiConnected()
        val cellNetwork = utils.connectToCell()

        // Verify cell network is validated
        val cellReq = NetworkRequest.Builder()
                .addTransportType(TRANSPORT_CELLULAR)
                .addCapability(NET_CAPABILITY_INTERNET)
                .build()
        val cellCb = TestableNetworkCallback(timeoutMs = TEST_TIMEOUT_MS)
        cm.registerNetworkCallback(cellReq, cellCb)
        val cb = cellCb.eventuallyExpectOrNull<RecorderCallback.CallbackEntry.CapabilitiesChanged> {
            it.network == cellNetwork && it.caps.hasCapability(NET_CAPABILITY_VALIDATED)
        }
        assertNotNull(cb, "Mobile network $cellNetwork has no access to the internet. " +
                "Check the mobile data connection.")

        // Have network validation use a local server that serves a HTTPS error / HTTP redirect
        server.addResponse(Request(TEST_PORTAL_URL_PATH), Status.OK,
                content = "Test captive portal content")
        server.addResponse(Request(TEST_HTTPS_URL_PATH), Status.INTERNAL_ERROR)
        val headers = mapOf("Location" to makeUrl(TEST_PORTAL_URL_PATH))
        server.addResponse(Request(TEST_HTTP_URL_PATH), Status.REDIRECT, headers)
        setHttpsUrlDeviceConfig(makeUrl(TEST_HTTPS_URL_PATH))
        setHttpUrlDeviceConfig(makeUrl(TEST_HTTP_URL_PATH))
        // URL expiration needs to be in the next 10 minutes
        assertTrue(WIFI_CONNECT_TIMEOUT_MS < TimeUnit.MINUTES.toMillis(10))
        setUrlExpirationDeviceConfig(System.currentTimeMillis() + WIFI_CONNECT_TIMEOUT_MS)

        // Wait for a captive portal to be detected on the network
        val wifiNetworkFuture = CompletableFuture<Network>()
        val wifiCb = object : NetworkCallback() {
            override fun onCapabilitiesChanged(
                network: Network,
                nc: NetworkCapabilities
            ) {
                if (nc.hasCapability(NET_CAPABILITY_CAPTIVE_PORTAL)) {
                    wifiNetworkFuture.complete(network)
                }
            }
        }
        cm.requestNetwork(NetworkRequest.Builder().addTransportType(TRANSPORT_WIFI).build(), wifiCb)

        try {
            reconnectWifi()
            val network = wifiNetworkFuture.assertGet(WIFI_CONNECT_TIMEOUT_MS,
                    "Captive portal not detected after ${WIFI_CONNECT_TIMEOUT_MS}ms")

            val wifiDefaultMessage = "Wifi should not be the default network when a captive " +
                    "portal was detected and another network (mobile data) can provide internet " +
                    "access."
            assertNotEquals(network, cm.activeNetwork, wifiDefaultMessage)

            val startPortalAppPermission =
                    if (isDevSdkInRange(0, Build.VERSION_CODES.Q)) CONNECTIVITY_INTERNAL
                    else NETWORK_SETTINGS
            runAsShell(startPortalAppPermission) { cm.startCaptivePortalApp(network) }

            // Expect the portal content to be fetched at some point after detecting the portal.
            // Some implementations may fetch the URL before startCaptivePortalApp is called.
            assertNotNull(server.requestsRecord.poll(TEST_TIMEOUT_MS, pos = 0) {
                it.path == TEST_PORTAL_URL_PATH
            }, "The captive portal login page was still not fetched ${TEST_TIMEOUT_MS}ms " +
                    "after startCaptivePortalApp.")

            assertNotEquals(network, cm.activeNetwork, wifiDefaultMessage)
        } finally {
            cm.unregisterNetworkCallback(wifiCb)
            server.stop()
            // disconnectFromCell should be called after connectToCell
            utils.disconnectFromCell()
        }
    }

    /**
     * Create a URL string that, when fetched, will hit the test server with the given URL [path].
     */
    private fun makeUrl(path: String) = "http://localhost:${server.listeningPort}" + path

    private fun reconnectWifi() {
        utils.ensureWifiDisconnected(null /* wifiNetworkToCheck */)
        utils.ensureWifiConnected()
    }
}