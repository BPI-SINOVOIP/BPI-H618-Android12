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

package com.android.eventlib;

import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.content.Context.BIND_AUTO_CREATE;

import static com.android.eventlib.QueryService.EARLIEST_LOG_TIME_KEY;
import static com.android.eventlib.QueryService.EVENT_KEY;
import static com.android.eventlib.QueryService.QUERIER_KEY;
import static com.android.eventlib.QueryService.TIMEOUT_KEY;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.packages.Package;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.bedstead.nene.users.User;

import java.time.Duration;
import java.time.Instant;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Implementation of {@link EventQuerier} used to query a single other process.
 */
public class
    RemoteEventQuerier<E extends Event, F extends EventLogsQuery> implements EventQuerier<E> {

    private static final int CONNECTION_TIMEOUT_SECONDS = 30;
    private static final String LOG_TAG = "RemoteEventQuerier";
    private static final TestApis sTestApis = new TestApis();
    private static final Context sContext = sTestApis.context().instrumentedContext();

    private final String mPackageName;
    private final EventLogsQuery<E, F> mEventLogsQuery;
    // Each client gets a random ID
    private final long id = UUID.randomUUID().getMostSignificantBits();

    public RemoteEventQuerier(String packageName, EventLogsQuery<E, F> eventLogsQuery) {
        mPackageName = packageName;
        mEventLogsQuery = eventLogsQuery;
    }

    private final ServiceConnection connection =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName className, IBinder service) {
                    mQuery.set(IQueryService.Stub.asInterface(service));
                    mConnectionCountdown.countDown();
                }

                @Override
                public void onServiceDisconnected(ComponentName className) {
                    mQuery.set(null);
                    Log.i(LOG_TAG, "Service disconnected from " + className);
                }
            };

    @Override
    public E get(Instant earliestLogTime) {
        ensureInitialised();
        Bundle data = createRequestBundle();
        try {
            Bundle resultMessage = mQuery.get().get(id, data);
            E e = (E) resultMessage.getSerializable(EVENT_KEY);
            while (e != null && !mEventLogsQuery.filterAll(e)) {
                resultMessage = mQuery.get().getNext(id, data);
                e = (E) resultMessage.getSerializable(EVENT_KEY);
            }
            return e;
        } catch (RemoteException e) {
            throw new IllegalStateException("Error making cross-process call", e);
        }
    }

    @Override
    public E next(Instant earliestLogTime) {
        ensureInitialised();
        Bundle data = createRequestBundle();
        try {
            Bundle resultMessage = mQuery.get().next(id, data);
            E e = (E) resultMessage.getSerializable(EVENT_KEY);
            while (e != null && !mEventLogsQuery.filterAll(e)) {
                resultMessage = mQuery.get().next(id, data);
                e = (E) resultMessage.getSerializable(EVENT_KEY);
            }
            return e;
        } catch (RemoteException e) {
            throw new IllegalStateException("Error making cross-process call", e);
        }
    }

    @Override
    public E poll(Instant earliestLogTime, Duration timeout) {
        ensureInitialised();
        Instant endTime = Instant.now().plus(timeout);
        Bundle data = createRequestBundle();
        Duration remainingTimeout = Duration.between(Instant.now(), endTime);
        data.putSerializable(TIMEOUT_KEY, remainingTimeout);
        try {
            Bundle resultMessage = mQuery.get().poll(id, data);
            E e = (E) resultMessage.getSerializable(EVENT_KEY);
            while (e != null && !mEventLogsQuery.filterAll(e)) {
                remainingTimeout = Duration.between(Instant.now(), endTime);
                data.putSerializable(TIMEOUT_KEY, remainingTimeout);
                resultMessage = mQuery.get().poll(id, data);
                e = (E) resultMessage.getSerializable(EVENT_KEY);
            }
            return e;
        } catch (RemoteException e) {
            throw new IllegalStateException("Error making cross-process call", e);
        }
    }

    private Bundle createRequestBundle() {
        Bundle data = new Bundle();
        data.putSerializable(EARLIEST_LOG_TIME_KEY, EventLogs.sEarliestLogTime);
        return data;
    }

    private AtomicReference<IQueryService> mQuery = new AtomicReference<>();
    private CountDownLatch mConnectionCountdown;

    private static final int MAX_INITIALISATION_ATTEMPTS = 300;
    private static final long INITIALISATION_ATTEMPT_DELAY_MS = 100;

    private void ensureInitialised() {
        // We have retries for binding because there are a number of reasons binding could fail in
        // unpredictable ways
        int attempts = 0;
        while (attempts++ < MAX_INITIALISATION_ATTEMPTS) {
            try {
                ensureInitialisedOrThrow();
                return;
            } catch (Exception | Error e) {
                // Ignore, we will retry
            }
            try {
                Thread.sleep(INITIALISATION_ATTEMPT_DELAY_MS);
            } catch (InterruptedException e) {
                throw new IllegalStateException("Interrupted while initialising", e);
            }
        }

        ensureInitialisedOrThrow();
    }

    private void ensureInitialisedOrThrow() {
        if (mQuery.get() != null) {
            return;
        }

        blockingConnectOrFail();
        Bundle data = new Bundle();
        data.putSerializable(QUERIER_KEY, mEventLogsQuery);

        try {
            mQuery.get().init(id, data);
        } catch (RemoteException e) {
            mQuery.set(null);
            throw new IllegalStateException("Error making cross-process call", e);
        }
    }

    private void blockingConnectOrFail() {
        mConnectionCountdown = new CountDownLatch(1);
        Intent intent = new Intent();
        intent.setPackage(mPackageName);
        intent.setClassName(mPackageName, "com.android.eventlib.QueryService");

        AtomicBoolean didBind = new AtomicBoolean(false);
        if (mEventLogsQuery.getUserHandle() != null
                && mEventLogsQuery.getUserHandle().getIdentifier()
                != sTestApis.users().instrumented().id()) {
            try (PermissionContext p =
                         sTestApis.permissions().withPermission(INTERACT_ACROSS_USERS_FULL)) {
                didBind.set(sContext.bindServiceAsUser(
                        intent, connection, /* flags= */ BIND_AUTO_CREATE,
                        mEventLogsQuery.getUserHandle()));
            }
        } else {
            didBind.set(sContext.bindService(intent, connection, /* flags= */ BIND_AUTO_CREATE));
        }

        if (didBind.get()) {
            try {
                mConnectionCountdown.await(CONNECTION_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                throw new IllegalStateException("Interrupted while binding to service", e);
            }
        } else {
            User user = (mEventLogsQuery.getUserHandle() == null) ? sTestApis.users().instrumented().resolve() : sTestApis.users().find(mEventLogsQuery.getUserHandle()).resolve();
            if (user == null) {
                throw new AssertionError("Tried to bind to user " + mEventLogsQuery.getUserHandle() + " but does not exist");
            }
            if (user.state() != User.UserState.RUNNING_UNLOCKED) {
                throw new AssertionError("Tried to bind to user " + user + " but they are not RUNNING_UNLOCKED");
            }
            Package pkg = sTestApis.packages().find(mPackageName).resolve();
            if (pkg == null) {
                throw new AssertionError("Tried to bind to package " + mPackageName + " but it is not installed on any user.");
            }
            if (!pkg.installedOnUsers().contains(user)) {
                throw new AssertionError("Tried to bind to package " + mPackageName + " but it is not installed on target user " + user);
            }

            throw new IllegalStateException("Tried to bind but call returned false (intent is "
                    + intent + ", user is  " + mEventLogsQuery.getUserHandle() + ")");
        }

        if (mQuery.get() == null) {
            throw new IllegalStateException("Tried to bind but failed");
        }
    }
}
