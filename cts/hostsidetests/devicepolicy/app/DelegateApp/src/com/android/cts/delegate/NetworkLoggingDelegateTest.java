/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.cts.delegate;

import static android.app.admin.DeviceAdminReceiver.ACTION_NETWORK_LOGS_AVAILABLE;

import static com.android.cts.delegate.DelegateTestUtils.assertExpectException;

import static com.google.common.truth.Truth.assertThat;

import android.app.admin.DevicePolicyManager;
import android.content.IntentFilter;
import android.os.UserManager;
import android.util.Log;

import com.android.bedstead.dpmwrapper.TestAppHelper;
import com.android.cts.delegate.DelegateTestUtils.DelegatedLogsReceiver;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.CountDownLatch;

/**
 * Tests that a delegate app with DELEGATION_NETWORK_LOGGING is able to control and access
 * network logging.
 */
public final class NetworkLoggingDelegateTest extends BaseJUnit3TestCase {

    private static final String TAG = "NetworkLoggingDelegateTest";

    private static final String[] URL_LIST = {
            "example.edu",
            "ipv6.google.com",
            "google.co.jp",
            "google.fr",
            "google.com.br",
            "google.com.tr",
            "google.co.uk",
            "google.de"
    };

    // TODO(b/176993670): receiver needed to forward intents from device owner user to current user
    // on headless system user mode. Might be removed once tests are refactor to use proper IPC.
    private DelegatedLogsReceiver mReceiver;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        if (UserManager.isHeadlessSystemUserMode()) {
            mReceiver = new DelegatedLogsReceiver();
            TestAppHelper.registerTestCaseReceiver(mContext, mReceiver,
                    new IntentFilter(ACTION_NETWORK_LOGS_AVAILABLE));
        }

        DelegatedLogsReceiver.sBatchCountDown = new CountDownLatch(1);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();

        if (mReceiver != null) {
            TestAppHelper.unregisterTestCaseReceiver(mContext, mReceiver);
        }
    }


    public void testCanAccessApis() throws Throwable {
        assertThat(mDpm.getDelegatedScopes(null, mContext.getPackageName())).contains(
                DevicePolicyManager.DELEGATION_NETWORK_LOGGING);
        testNetworkLogging();
    }

    public void testCannotAccessApis()throws Exception {
        assertExpectException(SecurityException.class, null,
                () -> mDpm.isNetworkLoggingEnabled(null));

        assertExpectException(SecurityException.class, null,
                () -> mDpm.setNetworkLoggingEnabled(null, true));

        assertExpectException(SecurityException.class, null,
                () -> mDpm.retrieveNetworkLogs(null, 0));
    }

    public void testNetworkLogging() throws Throwable {
        mDpm.setNetworkLoggingEnabled(null, true);
        assertTrue(mDpm.isNetworkLoggingEnabled(null));

        try {
            for (final String url : URL_LIST) {
                connectToWebsite(url);
            }
            mDevice.executeShellCommand("dpm force-network-logs");

            DelegateTestUtils.DelegatedLogsReceiver.waitForBroadcast();
        } finally {
            mDpm.setNetworkLoggingEnabled(null, false);
            assertFalse(mDpm.isNetworkLoggingEnabled(null));
        }
    }

    private void connectToWebsite(String server) throws Exception {
        final URL url = new URL("http://" + server);
        HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
        try (AutoCloseable ac = () -> urlConnection.disconnect()){
            urlConnection.setConnectTimeout(2000);
            urlConnection.setReadTimeout(2000);
            urlConnection.getResponseCode();
        } catch (IOException e) {
            Log.w(TAG, "Failed to connect to " + server, e);
        }
    }
}
