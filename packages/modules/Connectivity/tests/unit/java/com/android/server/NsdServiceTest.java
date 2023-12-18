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

package com.android.server;

import static libcore.junit.util.compat.CoreCompatChangeRule.DisableCompatChanges;
import static libcore.junit.util.compat.CoreCompatChangeRule.EnableCompatChanges;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.compat.testing.PlatformCompatChangeRule;
import android.content.ContentResolver;
import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;

import androidx.test.filters.SmallTest;

import com.android.server.NsdService.DaemonConnection;
import com.android.server.NsdService.DaemonConnectionSupplier;
import com.android.server.NsdService.NativeCallbackReceiver;
import com.android.testutils.DevSdkIgnoreRule;
import com.android.testutils.DevSdkIgnoreRunner;
import com.android.testutils.HandlerUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

// TODOs:
//  - test client can send requests and receive replies
//  - test NSD_ON ENABLE/DISABLED listening
@RunWith(DevSdkIgnoreRunner.class)
@SmallTest
@DevSdkIgnoreRule.IgnoreUpTo(Build.VERSION_CODES.R)
public class NsdServiceTest {

    static final int PROTOCOL = NsdManager.PROTOCOL_DNS_SD;
    private static final long CLEANUP_DELAY_MS = 500;
    private static final long TIMEOUT_MS = 500;

    @Rule
    public TestRule compatChangeRule = new PlatformCompatChangeRule();
    @Mock Context mContext;
    @Mock ContentResolver mResolver;
    @Mock NsdService.NsdSettings mSettings;
    NativeCallbackReceiver mDaemonCallback;
    @Spy DaemonConnection mDaemon = new DaemonConnection(mDaemonCallback);
    HandlerThread mThread;
    TestHandler mHandler;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mThread = new HandlerThread("mock-service-handler");
        mThread.start();
        doReturn(true).when(mDaemon).execute(any());
        mHandler = new TestHandler(mThread.getLooper());
        when(mContext.getContentResolver()).thenReturn(mResolver);
    }

    @After
    public void tearDown() throws Exception {
        if (mThread != null) {
            mThread.quit();
            mThread = null;
        }
    }

    @Test
    @DisableCompatChanges(NsdManager.RUN_NATIVE_NSD_ONLY_IF_LEGACY_APPS)
    public void testPreSClients() {
        when(mSettings.isEnabled()).thenReturn(true);
        NsdService service = makeService();

        // Pre S client connected, the daemon should be started.
        NsdManager client1 = connectClient(service);
        waitForIdle();
        verify(mDaemon, times(1)).maybeStart();
        verifyDaemonCommands("start-service");

        NsdManager client2 = connectClient(service);
        waitForIdle();
        verify(mDaemon, times(1)).maybeStart();

        client1.disconnect();
        // Still 1 client remains, daemon shouldn't be stopped.
        waitForIdle();
        verify(mDaemon, never()).maybeStop();

        client2.disconnect();
        // All clients are disconnected, the daemon should be stopped.
        verifyDelayMaybeStopDaemon(CLEANUP_DELAY_MS);
        verifyDaemonCommands("stop-service");
    }

    @Test
    @EnableCompatChanges(NsdManager.RUN_NATIVE_NSD_ONLY_IF_LEGACY_APPS)
    public void testNoDaemonStartedWhenClientsConnect() {
        when(mSettings.isEnabled()).thenReturn(true);

        NsdService service = makeService();

        // Creating an NsdManager will not cause any cmds executed, which means
        // no daemon is started.
        NsdManager client1 = connectClient(service);
        waitForIdle();
        verify(mDaemon, never()).execute(any());

        // Creating another NsdManager will not cause any cmds executed.
        NsdManager client2 = connectClient(service);
        waitForIdle();
        verify(mDaemon, never()).execute(any());

        // If there is no active request, try to clean up the daemon
        // every time the client disconnects.
        client1.disconnect();
        verifyDelayMaybeStopDaemon(CLEANUP_DELAY_MS);
        reset(mDaemon);
        client2.disconnect();
        verifyDelayMaybeStopDaemon(CLEANUP_DELAY_MS);

        client1.disconnect();
        client2.disconnect();
    }

    @Test
    @EnableCompatChanges(NsdManager.RUN_NATIVE_NSD_ONLY_IF_LEGACY_APPS)
    public void testClientRequestsAreGCedAtDisconnection() {
        when(mSettings.isEnabled()).thenReturn(true);

        NsdService service = makeService();
        NsdManager client = connectClient(service);

        waitForIdle();
        verify(mDaemon, never()).maybeStart();
        verify(mDaemon, never()).execute(any());

        NsdServiceInfo request = new NsdServiceInfo("a_name", "a_type");
        request.setPort(2201);

        // Client registration request
        NsdManager.RegistrationListener listener1 = mock(NsdManager.RegistrationListener.class);
        client.registerService(request, PROTOCOL, listener1);
        waitForIdle();
        verify(mDaemon, times(1)).maybeStart();
        verifyDaemonCommands("start-service", "register 2 a_name a_type 2201");

        // Client discovery request
        NsdManager.DiscoveryListener listener2 = mock(NsdManager.DiscoveryListener.class);
        client.discoverServices("a_type", PROTOCOL, listener2);
        waitForIdle();
        verify(mDaemon, times(1)).maybeStart();
        verifyDaemonCommand("discover 3 a_type");

        // Client resolve request
        NsdManager.ResolveListener listener3 = mock(NsdManager.ResolveListener.class);
        client.resolveService(request, listener3);
        waitForIdle();
        verify(mDaemon, times(1)).maybeStart();
        verifyDaemonCommand("resolve 4 a_name a_type local.");

        // Client disconnects, stop the daemon after CLEANUP_DELAY_MS.
        client.disconnect();
        verifyDelayMaybeStopDaemon(CLEANUP_DELAY_MS);
        // checks that request are cleaned
        verifyDaemonCommands("stop-register 2", "stop-discover 3",
                "stop-resolve 4", "stop-service");

        client.disconnect();
    }

    @Test
    @EnableCompatChanges(NsdManager.RUN_NATIVE_NSD_ONLY_IF_LEGACY_APPS)
    public void testCleanupDelayNoRequestActive() {
        when(mSettings.isEnabled()).thenReturn(true);

        NsdService service = makeService();
        NsdManager client = connectClient(service);

        NsdServiceInfo request = new NsdServiceInfo("a_name", "a_type");
        request.setPort(2201);
        NsdManager.RegistrationListener listener1 = mock(NsdManager.RegistrationListener.class);
        client.registerService(request, PROTOCOL, listener1);
        waitForIdle();
        verify(mDaemon, times(1)).maybeStart();
        verifyDaemonCommands("start-service", "register 2 a_name a_type 2201");

        client.unregisterService(listener1);
        verifyDaemonCommand("stop-register 2");

        verifyDelayMaybeStopDaemon(CLEANUP_DELAY_MS);
        verifyDaemonCommand("stop-service");
        reset(mDaemon);
        client.disconnect();
        // Client disconnects, after CLEANUP_DELAY_MS, maybeStop the daemon.
        verifyDelayMaybeStopDaemon(CLEANUP_DELAY_MS);
    }

    private void waitForIdle() {
        HandlerUtils.waitForIdle(mHandler, TIMEOUT_MS);
    }

    NsdService makeService() {
        DaemonConnectionSupplier supplier = (callback) -> {
            mDaemonCallback = callback;
            return mDaemon;
        };
        NsdService service = new NsdService(mContext, mSettings,
                mHandler, supplier, CLEANUP_DELAY_MS);
        verify(mDaemon, never()).execute(any(String.class));
        return service;
    }

    NsdManager connectClient(NsdService service) {
        return new NsdManager(mContext, service);
    }

    void verifyDelayMaybeStopDaemon(long cleanupDelayMs) {
        waitForIdle();
        // Stop daemon shouldn't be called immediately.
        verify(mDaemon, never()).maybeStop();
        // Clean up the daemon after CLEANUP_DELAY_MS.
        verify(mDaemon, timeout(cleanupDelayMs + TIMEOUT_MS)).maybeStop();
    }

    void verifyDaemonCommands(String... wants) {
        verifyDaemonCommand(String.join(" ", wants), wants.length);
    }

    void verifyDaemonCommand(String want) {
        verifyDaemonCommand(want, 1);
    }

    void verifyDaemonCommand(String want, int n) {
        waitForIdle();
        final ArgumentCaptor<Object> argumentsCaptor = ArgumentCaptor.forClass(Object.class);
        verify(mDaemon, times(n)).execute(argumentsCaptor.capture());
        String got = "";
        for (Object o : argumentsCaptor.getAllValues()) {
            got += o + " ";
        }
        assertEquals(want, got.trim());
        // rearm deamon for next command verification
        reset(mDaemon);
        doReturn(true).when(mDaemon).execute(any());
    }

    public static class TestHandler extends Handler {
        public Message lastMessage;

        TestHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            lastMessage = obtainMessage();
            lastMessage.copyFrom(msg);
        }
    }
}
