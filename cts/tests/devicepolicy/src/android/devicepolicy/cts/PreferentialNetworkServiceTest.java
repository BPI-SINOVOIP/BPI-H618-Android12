/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.devicepolicy.cts;

import static android.Manifest.permission.ACCESS_NETWORK_STATE;
import static android.Manifest.permission.NETWORK_SETTINGS;
import static android.net.NetworkCapabilities.NET_CAPABILITY_ENTERPRISE;
import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED;
import static android.net.NetworkCapabilities.NET_CAPABILITY_NOT_VCN_MANAGED;

import android.app.Instrumentation;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkProvider;
import android.net.NetworkScore;
import android.os.HandlerExecutor;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Range;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.EnsureHasPermission;
import com.android.bedstead.harrier.annotations.Postsubmit;
import com.android.bedstead.harrier.annotations.enterprise.PositivePolicyTest;
import com.android.bedstead.harrier.policies.PreferentialNetworkService;
import com.android.bedstead.nene.TestApis;
import com.android.testutils.TestableNetworkCallback;
import com.android.testutils.TestableNetworkOfferCallback;

import org.junit.After;
import org.junit.Before;
import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Objects;
import java.util.Set;

// TODO(b/190797743): Move this test to to net test folder.
@RunWith(BedsteadJUnit4.class)
public class PreferentialNetworkServiceTest {
    @ClassRule @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    private final long DEFAULT_TIMEOUT_MS = 30_000L;
    private final long NO_CALLBACK_TIMEOUT_MS = 100L;
    private final String TAG = PreferentialNetworkServiceTest.class.getSimpleName();

    private static final TestApis sTestApis = new TestApis();
    private static final Context sContext = sTestApis.context().instrumentedContext();
    private static final ConnectivityManager sCm =
            sContext.getSystemService(ConnectivityManager.class);
    private final HandlerThread mHandlerThread = new HandlerThread(TAG + " handler thread");
    private final NetworkCapabilities mEnterpriseNcFilter = new NetworkCapabilities.Builder()
            .addCapability(NET_CAPABILITY_INTERNET)
            .addCapability(NET_CAPABILITY_NOT_VCN_MANAGED)
            .addCapability(NET_CAPABILITY_ENTERPRISE)
            .removeCapability(NET_CAPABILITY_NOT_RESTRICTED)
            // Only accept requests from this uid, otherwise the provider who uses this
            // filter might see all requests of the same user if the feature is enabled.
            .setUids(Set.of(new Range(Process.myUid(), Process.myUid())))
            .build();

    @Before
    public void setUp() throws Exception {
        mHandlerThread.start();
    }

    @After
    public void tearDown() throws Exception {
        mHandlerThread.quitSafely();
    }

    /**
     * Enable PreferentialNetworkService, verify the provider that provides enterprise slice can
     * see the enterprise slice requests.
     */
    @Test
    @Postsubmit(reason = "New test")
    @EnsureHasPermission({ACCESS_NETWORK_STATE, NETWORK_SETTINGS})
    @PositivePolicyTest(policy = PreferentialNetworkService.class)
    public void setPreferentialNetworkServiceEnabled_enableService_issueRequest() {
        // Expect a regular default network.
        final Network defaultNetwork = Objects.requireNonNull(sCm.getActiveNetwork(),
                "Default network is required to perform the test.");
        final TestableNetworkCallback defaultCallback = new TestableNetworkCallback();
        sCm.registerDefaultNetworkCallback(defaultCallback);
        defaultCallback.expectAvailableCallbacks(defaultNetwork, /* suspended= */ false,
                /* validated= */ true, /* blocked= */ false, DEFAULT_TIMEOUT_MS);

        // Register a pair of provider and offer that can provides enterprise slice, verify it
        // received nothing since the feature is not enabled.
        final NetworkProvider provider =
                new NetworkProvider(sContext, mHandlerThread.getLooper(), TAG);
        sCm.registerNetworkProvider(provider);
        final TestableNetworkOfferCallback offerCallback = registerEnterpriseNetworkOffer(provider);

        try {
            // Enable PreferentialNetworkService, verify the provider sees the enterprise
            // slice request.
            // But the network callback received nothing since it should automatically fallback to
            // default request if there is no enterprise slice.
            sDeviceState.dpc().devicePolicyManager().setPreferentialNetworkServiceEnabled(true);
            offerCallback.expectOnNetworkNeeded(mEnterpriseNcFilter);
            defaultCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);
        } finally {
            sCm.unregisterNetworkCallback(defaultCallback);
            sDeviceState.dpc().devicePolicyManager().setPreferentialNetworkServiceEnabled(false);
            sCm.unregisterNetworkProvider(provider);
        }
    }

    /**
     * Disable PreferentialNetworkService, verify the provider that provides enterprise slice cannot
     * see the enterprise slice requests.
     */
    @Test
    @Postsubmit(reason = "New test")
    @EnsureHasPermission({ACCESS_NETWORK_STATE, NETWORK_SETTINGS})
    @PositivePolicyTest(policy = PreferentialNetworkService.class)
    public void setPreferentialNetworkServiceEnabled_disableService_noIssueRequest() {
        // Expect a regular default network.
        final Network defaultNetwork = Objects.requireNonNull(sCm.getActiveNetwork(),
                "Default network is required to perform the test.");
        final TestableNetworkCallback defaultCallback = new TestableNetworkCallback();
        sCm.registerDefaultNetworkCallback(defaultCallback);
        defaultCallback.expectAvailableCallbacks(defaultNetwork, /* suspended= */ false,
                /* validated= */ true, /* blocked= */ false, DEFAULT_TIMEOUT_MS);

        // Register a pair of provider and offer that can provides enterprise slice, verify it
        // received nothing since the feature is not enabled.
        final NetworkProvider provider =
                new NetworkProvider(sContext, mHandlerThread.getLooper(), TAG);
        sCm.registerNetworkProvider(provider);
        final TestableNetworkOfferCallback offerCallback = registerEnterpriseNetworkOffer(provider);

        try {
            // Disable PreferentialNetworkService, verify the provider cannot see the enterprise
            // slice request. And the network callback received nothing since there is no any
            // change.
            sDeviceState.dpc().devicePolicyManager().setPreferentialNetworkServiceEnabled(false);
            offerCallback.assertNoCallback();  // Still unneeded.
            defaultCallback.assertNoCallback(NO_CALLBACK_TIMEOUT_MS);
        } finally {
            sCm.unregisterNetworkCallback(defaultCallback);
            sDeviceState.dpc().devicePolicyManager().setPreferentialNetworkServiceEnabled(false);
            sCm.unregisterNetworkProvider(provider);
        }
    }

    private TestableNetworkOfferCallback registerEnterpriseNetworkOffer(
            NetworkProvider provider) {
        final TestableNetworkOfferCallback offerCallback =
                new TestableNetworkOfferCallback(DEFAULT_TIMEOUT_MS, NO_CALLBACK_TIMEOUT_MS);
        provider.registerNetworkOffer(new NetworkScore.Builder().build(), mEnterpriseNcFilter,
                new HandlerExecutor(mHandlerThread.getThreadHandler()), offerCallback);
        offerCallback.assertNoCallback();
        return offerCallback;
    }
}
