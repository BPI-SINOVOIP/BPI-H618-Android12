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

package com.android.bedstead.testapp;

import android.content.BroadcastReceiver;
import android.content.IntentFilter;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.packages.ProcessReference;
import com.android.bedstead.nene.users.UserReference;

import com.google.android.enterprise.connectedapps.ConnectionListener;
import com.google.android.enterprise.connectedapps.CrossProfileConnector;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import javax.annotation.Nullable;

/**
 * A reference to a specific instance of a {@link TestApp} on a given user.
 *
 * <p>The user may not exist, or the test app may not be installed on the user.
 */
public final class TestAppInstanceReference implements AutoCloseable, ConnectionListener {

    private static final TestApis sTestApis = new TestApis();

    private final TestApp mTestApp;
    private final UserReference mUser;
    private final CrossProfileConnector mConnector;
    private final Map<IntentFilter, Long> mRegisteredBroadcastReceivers = new HashMap<>();
    private boolean mKeepAliveManually = false;
    private final ProfileTestAppController mTestAppController;
    private final TestAppActivities mTestAppActivities;

    TestAppInstanceReference(TestApp testApp, UserReference user) {
        mTestApp = testApp;
        mUser = user;
        mConnector = CrossProfileConnector.builder(sTestApis.context().instrumentedContext())
                .setBinder(new TestAppBinder(this))
                .build();
        mConnector.registerConnectionListener(this);
        mTestAppController =
                ProfileTestAppController.create(mConnector);
        mTestAppActivities = TestAppActivities.create(this);
    }

    CrossProfileConnector connector() {
        return mConnector;
    }

    /**
     * Access activities on the test app.
     */
    public TestAppActivities activities() {
        return mTestAppActivities;
    }

    /**
     * The {@link TestApp} this instance refers to.
     */
    public TestApp testApp() {
        return mTestApp;
    }

    /**
     * The {@link UserReference} this instance refers to.
     */
    public UserReference user() {
        return mUser;
    }

    /**
     * Uninstall the {@link TestApp} from the user referenced by
     * this {@link TestAppInstanceReference}.
     */
    public void uninstall() {
        mTestApp.uninstall(mUser);
    }

    /**
     * Register a {@link BroadcastReceiver} for a given {@link IntentFilter}.
     *
     * <p>A new {@link BroadcastReceiver} instance will be created for each {@link IntentFilter}.
     *
     * <p>Note that {@link IntentFilter} does not override {@code equals} and one broadcast receiver
     * will be registered for each instance of {@link IntentFilter} regardless of the content of the
     * {@link IntentFilter}.
     *
     * <p>As registered receivers are only active while the application is open, calling this method
     * will have the same effect as calling {@link #keepAlive()}.
     */
    public void registerReceiver(IntentFilter intentFilter) {
        if (mRegisteredBroadcastReceivers.containsKey(intentFilter)) {
            return;
        }

        long receiverId = UUID.randomUUID().getMostSignificantBits();
        registerReceiver(intentFilter, receiverId);
        keepAlive(/* manualKeepAlive= */ false);
    }

    private void registerReceiver(IntentFilter intentFilter, long receiverId) {
        try {
            mConnector.connect();
            mTestAppController.other().registerReceiver(receiverId, intentFilter);
            mRegisteredBroadcastReceivers.put(intentFilter, receiverId);
        } catch (UnavailableProfileException e) {
            throw new IllegalStateException("Could not connect to test app", e);
        } finally {
            mConnector.stopManualConnectionManagement();
        }
    }

    /**
     * Unregister the receiver
     * @param intentFilter
     */
    public TestAppInstanceReference unregisterReceiver(IntentFilter intentFilter) {
        if (!mRegisteredBroadcastReceivers.containsKey(intentFilter)) {
            return this;
        }

        long receiverId = mRegisteredBroadcastReceivers.remove(intentFilter);

        try {
            mConnector.connect();
            mTestAppController.other().unregisterReceiver(receiverId);
            mRegisteredBroadcastReceivers.put(intentFilter, receiverId);
        } catch (UnavailableProfileException e) {
            throw new IllegalStateException("Could not connect to test app", e);
        } finally {
            mConnector.stopManualConnectionManagement();
        }

        if (mRegisteredBroadcastReceivers.isEmpty() && !mKeepAliveManually) {
            stopKeepAlive();
        }

        return this;
    }

    /**
     * Starts keeping the test app process alive.
     *
     * <p>This ensures that it will receive broadcasts using registered broadcast receivers.
     *
     * @see {@link #stopKeepAlive()}.
     */
    public TestAppInstanceReference keepAlive() {
        keepAlive(/* manualKeepAlive=*/ true);
        return this;
    }

    /**
     * Starts keep alive mode and marks it as manual so that it won't be automatically ended if
     * the last broadcast receiver is unregistered.
     */
    private void keepAlive(boolean manualKeepAlive) {
        mKeepAliveManually = manualKeepAlive;
        try {
            connector().connect();
        } catch (UnavailableProfileException e) {
            throw new IllegalStateException("Could not connect to test app. Is it installed?", e);
        }
    }

    /**
     * Stops keeping the target app alive.
     *
     * <p>This will not kill the app immediately. To do that see {@link #stop()}.
     */
    public TestAppInstanceReference stopKeepAlive() {
        mKeepAliveManually = false;
        connector().stopManualConnectionManagement();
        return this;
    }

    /**
     * Immediately force stops the app.
     *
     * <p>This will also stop keeping the target app alive (see {@link #stopKeepAlive()}.
     */
    public TestAppInstanceReference stop() {
        stopKeepAlive();

        ProcessReference process = mTestApp.reference().runningProcess(mUser);
        if (process != null) {
            try {
                process.kill();
            } catch (NeneException e) {
                throw new NeneException("Error killing process... process is " + process(), e);
            }
        }

        return this;
    }

    /**
     * Gets the {@link ProcessReference} of the app, if any.
     */
    @Nullable
    public ProcessReference process() {
        return mTestApp.reference().runningProcess(mUser);
    }

    @Override
    public void close() {
        stopKeepAlive();
        uninstall();
    }

    @Override
    public void connectionChanged() {
        if (mConnector.isConnected()) {
            // re-register broadcast receivers when re-connected
            for (Map.Entry<IntentFilter, Long> entry : mRegisteredBroadcastReceivers.entrySet()) {
                registerReceiver(entry.getKey(), entry.getValue());
            }
        }
    }
}
