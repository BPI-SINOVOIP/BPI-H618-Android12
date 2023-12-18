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

package com.android.car.watchdog;

import static android.car.watchdog.CarWatchdogManager.TIMEOUT_CRITICAL;
import static android.car.watchdog.CarWatchdogManager.TIMEOUT_MODERATE;
import static android.car.watchdog.CarWatchdogManager.TIMEOUT_NORMAL;

import static com.android.internal.util.function.pooled.PooledLambda.obtainMessage;

import android.annotation.NonNull;
import android.annotation.UserIdInt;
import android.automotive.watchdog.internal.ICarWatchdogServiceForSystem;
import android.car.watchdog.ICarWatchdogServiceCallback;
import android.car.watchdoglib.CarWatchdogDaemonHelper;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.UserHandle;
import android.util.IndentingPrintWriter;
import android.util.SparseArray;
import android.util.SparseBooleanArray;

import com.android.internal.annotations.GuardedBy;
import com.android.server.utils.Slogf;

import java.util.ArrayList;

/**
 * Handles clients' health status checking and reporting the statuses to the watchdog daemon.
 */
public final class WatchdogProcessHandler {
    private static final int[] ALL_TIMEOUTS =
            { TIMEOUT_CRITICAL, TIMEOUT_MODERATE, TIMEOUT_NORMAL };

    private final ICarWatchdogServiceForSystem mWatchdogServiceForSystem;
    private final CarWatchdogDaemonHelper mCarWatchdogDaemonHelper;
    private final Handler mMainHandler = new Handler(Looper.getMainLooper());
    private final Object mLock = new Object();
    /*
     * Keeps the list of car watchdog client according to timeout:
     * key => timeout, value => ClientInfo list.
     * The value of SparseArray is guarded by mLock.
     */
    @GuardedBy("mLock")
    private final SparseArray<ArrayList<ClientInfo>> mClientMap = new SparseArray<>();
    /*
     * Keeps the map of car watchdog client being checked by CarWatchdogService according to
     * timeout: key => timeout, value => ClientInfo map.
     * The value is also a map: key => session id, value => ClientInfo.
     */
    @GuardedBy("mLock")
    private final SparseArray<SparseArray<ClientInfo>> mPingedClientMap = new SparseArray<>();
    /*
     * Keeps whether client health checking is being performed according to timeout:
     * key => timeout, value => boolean (whether client health checking is being performed).
     * The value of SparseArray is guarded by mLock.
     */
    @GuardedBy("mLock")
    private final SparseArray<Boolean> mClientCheckInProgress = new SparseArray<>();
    @GuardedBy("mLock")
    private final ArrayList<ClientInfo> mClientsNotResponding = new ArrayList<>();
    @GuardedBy("mMainHandler")
    private int mLastSessionId;
    @GuardedBy("mMainHandler")
    private final SparseBooleanArray mStoppedUser = new SparseBooleanArray();

    public WatchdogProcessHandler(ICarWatchdogServiceForSystem serviceImpl,
            CarWatchdogDaemonHelper daemonHelper) {
        mWatchdogServiceForSystem = serviceImpl;
        mCarWatchdogDaemonHelper = daemonHelper;
    }

    /** Initializes the handler. */
    public void init() {
        for (int timeout : ALL_TIMEOUTS) {
            mClientMap.put(timeout, new ArrayList<ClientInfo>());
            mPingedClientMap.put(timeout, new SparseArray<ClientInfo>());
            mClientCheckInProgress.put(timeout, false);
        }
        if (CarWatchdogService.DEBUG) {
            Slogf.d(CarWatchdogService.TAG, "WatchdogProcessHandler is initialized");
        }
    }

    /** Dumps its state. */
    public void dump(IndentingPrintWriter writer) {
        synchronized (mLock) {
            writer.println("Registered clients");
            writer.increaseIndent();
            int count = 1;
            for (int timeout : ALL_TIMEOUTS) {
                ArrayList<ClientInfo> clients = mClientMap.get(timeout);
                String timeoutStr = timeoutToString(timeout);
                for (ClientInfo clientInfo : clients) {
                    writer.printf("client #%d: timeout = %s, pid = %d\n", count++, timeoutStr,
                            clientInfo.pid);
                }
            }
            writer.printf("Stopped users: ");
            int size = mStoppedUser.size();
            if (size > 0) {
                writer.printf("%d", mStoppedUser.keyAt(0));
                for (int i = 1; i < size; i++) {
                    writer.printf(", %d", mStoppedUser.keyAt(i));
                }
                writer.println();
            } else {
                writer.println("none");
            }
        }
    }

    /** Registers the client callback */
    public void registerClient(ICarWatchdogServiceCallback client, int timeout) {
        synchronized (mLock) {
            ArrayList<ClientInfo> clients = mClientMap.get(timeout);
            if (clients == null) {
                Slogf.w(CarWatchdogService.TAG, "Cannot register the client: invalid timeout");
                return;
            }
            IBinder binder = client.asBinder();
            for (int i = 0; i < clients.size(); i++) {
                ClientInfo clientInfo = clients.get(i);
                if (binder == clientInfo.client.asBinder()) {
                    Slogf.w(CarWatchdogService.TAG,
                            "Cannot register the client: the client(pid: %d) has been already "
                            + "registered", clientInfo.pid);
                    return;
                }
            }
            int pid = Binder.getCallingPid();
            int userId = UserHandle.getUserId(Binder.getCallingUid());
            ClientInfo clientInfo = new ClientInfo(client, pid, userId, timeout);
            try {
                clientInfo.linkToDeath();
            } catch (RemoteException e) {
                Slogf.w(CarWatchdogService.TAG,
                        "Cannot register the client: linkToDeath to the client failed");
                return;
            }
            clients.add(clientInfo);
            if (CarWatchdogService.DEBUG) {
                Slogf.d(CarWatchdogService.TAG, "Client(pid: %d) is registered", pid);
            }
        }
    }

    /** Unregisters the previously registered client callback */
    public void unregisterClient(ICarWatchdogServiceCallback client) {
        synchronized (mLock) {
            IBinder binder = client.asBinder();
            for (int timeout : ALL_TIMEOUTS) {
                ArrayList<ClientInfo> clients = mClientMap.get(timeout);
                for (int i = 0; i < clients.size(); i++) {
                    ClientInfo clientInfo = clients.get(i);
                    if (binder != clientInfo.client.asBinder()) {
                        continue;
                    }
                    clientInfo.unlinkToDeath();
                    clients.remove(i);
                    if (CarWatchdogService.DEBUG) {
                        Slogf.d(CarWatchdogService.TAG, "Client(pid: %d) is unregistered",
                                clientInfo.pid);
                    }
                    return;
                }
            }
        }
        Slogf.w(CarWatchdogService.TAG,
                "Cannot unregister the client: the client has not been registered before");
        return;
    }

    /** Tells the handler that the client is alive. */
    public void tellClientAlive(ICarWatchdogServiceCallback client, int sessionId) {
        synchronized (mLock) {
            for (int timeout : ALL_TIMEOUTS) {
                if (!mClientCheckInProgress.get(timeout)) {
                    continue;
                }
                SparseArray<ClientInfo> pingedClients = mPingedClientMap.get(timeout);
                ClientInfo clientInfo = pingedClients.get(sessionId);
                if (clientInfo != null && clientInfo.client.asBinder() == client.asBinder()) {
                    pingedClients.remove(sessionId);
                    return;
                }
            }
        }
    }

    /** Updates the user stopped state */
    public void updateUserState(@UserIdInt int userId, boolean isStopped) {
        synchronized (mLock) {
            if (isStopped) {
                mStoppedUser.put(userId, true);
            } else {
                mStoppedUser.delete(userId);
            }
        }
    }

    /** Posts health check message */
    public void postHealthCheckMessage(int sessionId) {
        mMainHandler.sendMessage(obtainMessage(
                WatchdogProcessHandler::doHealthCheck, this, sessionId));
    }

    /** Returns the registered and alive client count. */
    public int getClientCount(int timeout) {
        synchronized (mLock) {
            ArrayList<ClientInfo> clients = mClientMap.get(timeout);
            return clients != null ? clients.size() : 0;
        }
    }

    /** Resets pinged clients before health checking */
    public void prepareHealthCheck() {
        synchronized (mLock) {
            for (int timeout : ALL_TIMEOUTS) {
                SparseArray<ClientInfo> pingedClients = mPingedClientMap.get(timeout);
                pingedClients.clear();
            }
        }
    }

    private void onClientDeath(ICarWatchdogServiceCallback client, int timeout) {
        synchronized (mLock) {
            removeClientLocked(client.asBinder(), timeout);
        }
    }

    private void doHealthCheck(int sessionId) {
        // For critical clients, the response status are checked just before reporting to car
        // watchdog daemon. For moderate and normal clients, the status are checked after allowed
        // delay per timeout.
        analyzeClientResponse(TIMEOUT_CRITICAL);
        reportHealthCheckResult(sessionId);
        sendPingToClients(TIMEOUT_CRITICAL);
        sendPingToClientsAndCheck(TIMEOUT_MODERATE);
        sendPingToClientsAndCheck(TIMEOUT_NORMAL);
    }

    private void analyzeClientResponse(int timeout) {
        // Clients which are not responding are stored in mClientsNotResponding, and will be dumped
        // and killed at the next response of CarWatchdogService to car watchdog daemon.
        SparseArray<ClientInfo> pingedClients = mPingedClientMap.get(timeout);
        synchronized (mLock) {
            for (int i = 0; i < pingedClients.size(); i++) {
                ClientInfo clientInfo = pingedClients.valueAt(i);
                if (mStoppedUser.get(clientInfo.userId)) {
                    continue;
                }
                mClientsNotResponding.add(clientInfo);
                removeClientLocked(clientInfo.client.asBinder(), timeout);
            }
            mClientCheckInProgress.setValueAt(timeout, false);
        }
    }

    private void sendPingToClients(int timeout) {
        SparseArray<ClientInfo> pingedClients = mPingedClientMap.get(timeout);
        ArrayList<ClientInfo> clientsToCheck;
        synchronized (mLock) {
            pingedClients.clear();
            clientsToCheck = new ArrayList<>(mClientMap.get(timeout));
            for (int i = 0; i < clientsToCheck.size(); i++) {
                ClientInfo clientInfo = clientsToCheck.get(i);
                if (mStoppedUser.get(clientInfo.userId)) {
                    continue;
                }
                int sessionId = getNewSessionId();
                clientInfo.sessionId = sessionId;
                pingedClients.put(sessionId, clientInfo);
            }
            mClientCheckInProgress.setValueAt(timeout, true);
        }
        for (int i = 0; i < clientsToCheck.size(); i++) {
            ClientInfo clientInfo = clientsToCheck.get(i);
            try {
                clientInfo.client.onCheckHealthStatus(clientInfo.sessionId, timeout);
            } catch (RemoteException e) {
                Slogf.w(CarWatchdogService.TAG,
                        "Sending a ping message to client(pid: %d) failed: %s", clientInfo.pid, e);
                synchronized (mLock) {
                    pingedClients.remove(clientInfo.sessionId);
                }
            }
        }
    }

    private void sendPingToClientsAndCheck(int timeout) {
        synchronized (mLock) {
            if (mClientCheckInProgress.get(timeout)) {
                return;
            }
        }
        sendPingToClients(timeout);
        mMainHandler.sendMessageDelayed(obtainMessage(WatchdogProcessHandler::analyzeClientResponse,
                this, timeout), timeoutToDurationMs(timeout));
    }

    private int getNewSessionId() {
        if (++mLastSessionId <= 0) {
            mLastSessionId = 1;
        }
        return mLastSessionId;
    }

    private void removeClientLocked(IBinder clientBinder, int timeout) {
        ArrayList<ClientInfo> clients = mClientMap.get(timeout);
        for (int i = 0; i < clients.size(); i++) {
            ClientInfo clientInfo = clients.get(i);
            if (clientBinder == clientInfo.client.asBinder()) {
                clients.remove(i);
                return;
            }
        }
    }

    private void reportHealthCheckResult(int sessionId) {
        int[] clientsNotResponding;
        ArrayList<ClientInfo> clientsToNotify;
        synchronized (mLock) {
            clientsNotResponding = toIntArray(mClientsNotResponding);
            clientsToNotify = new ArrayList<>(mClientsNotResponding);
            mClientsNotResponding.clear();
        }
        for (int i = 0; i < clientsToNotify.size(); i++) {
            ClientInfo clientInfo = clientsToNotify.get(i);
            try {
                clientInfo.client.onPrepareProcessTermination();
            } catch (RemoteException e) {
                Slogf.w(CarWatchdogService.TAG,
                        "Notifying onPrepareProcessTermination to client(pid: %d) failed: %s",
                        clientInfo.pid, e);
            }
        }

        try {
            mCarWatchdogDaemonHelper.tellCarWatchdogServiceAlive(
                    mWatchdogServiceForSystem, clientsNotResponding, sessionId);
        } catch (RemoteException | RuntimeException e) {
            Slogf.w(CarWatchdogService.TAG,
                    "Cannot respond to car watchdog daemon (sessionId=%d): %s", sessionId, e);
        }
    }

    @NonNull
    private int[] toIntArray(@NonNull ArrayList<ClientInfo> list) {
        int size = list.size();
        int[] intArray = new int[size];
        for (int i = 0; i < size; i++) {
            intArray[i] = list.get(i).pid;
        }
        return intArray;
    }

    private String timeoutToString(int timeout) {
        switch (timeout) {
            case TIMEOUT_CRITICAL:
                return "critical";
            case TIMEOUT_MODERATE:
                return "moderate";
            case TIMEOUT_NORMAL:
                return "normal";
            default:
                Slogf.w(CarWatchdogService.TAG, "Unknown timeout value");
                return "unknown";
        }
    }

    private long timeoutToDurationMs(int timeout) {
        switch (timeout) {
            case TIMEOUT_CRITICAL:
                return 3000L;
            case TIMEOUT_MODERATE:
                return 5000L;
            case TIMEOUT_NORMAL:
                return 10000L;
            default:
                Slogf.w(CarWatchdogService.TAG, "Unknown timeout value");
                return 10000L;
        }
    }

    private final class ClientInfo implements IBinder.DeathRecipient {
        public final ICarWatchdogServiceCallback client;
        public final int pid;
        @UserIdInt public final int userId;
        public final int timeout;
        public volatile int sessionId;

        ClientInfo(ICarWatchdogServiceCallback client, int pid, @UserIdInt int userId,
                int timeout) {
            this.client = client;
            this.pid = pid;
            this.userId = userId;
            this.timeout = timeout;
        }

        @Override
        public void binderDied() {
            Slogf.w(CarWatchdogService.TAG, "Client(pid: %d) died", pid);
            onClientDeath(client, timeout);
        }

        private void linkToDeath() throws RemoteException {
            client.asBinder().linkToDeath(this, 0);
        }

        private void unlinkToDeath() {
            client.asBinder().unlinkToDeath(this, 0);
        }
    }
}
