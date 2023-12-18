/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.telecom.cts;

import static org.junit.Assert.assertTrue;

import android.content.Intent;
import android.os.IBinder;
import android.telecom.Conference;
import android.telecom.Connection;
import android.telecom.ConnectionRequest;
import android.telecom.ConnectionService;
import android.telecom.PhoneAccountHandle;
import android.telecom.RemoteConference;
import android.telecom.RemoteConnection;
import android.util.Log;

import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.CountDownLatch;

/**
 * This is the official ConnectionService for Telecom's CTS App. Since telecom requires that a
 * CS be registered in the AndroidManifest.xml file, we have to have a single implementation
 * of a CS and this is it. To test specific CS behavior, tests will implement their own CS and
 * tell CtsConnectionService to forward any method invocations to that test's implementation.
 * This is set up using {@link #setUp} and should be cleaned up before the end of the test using
 * {@link #tearDown}.
 *
 * sConnectionService: Contains the connection service object provided by the current test in
 *                     progress. We use this object to forward any communication received from the
 *                     Telecom framework to the test connection service.
 * sTelecomConnectionService: Contains the connection service object registered to the Telecom
 *                            framework. We use this object to forward any communication from the
 *                            test connection service to the Telecom framework. After Telecom
 *                            binds to CtsConnectionService, this is set to be the instance of
 *                            CtsConnectionService created by the framework after Telecom binds.
 */
public class CtsConnectionService extends ConnectionService {
    private static String LOG_TAG = "CtsConnectionService";
    // This is the connection service implemented by the test
    private static ConnectionService sConnectionService;
    // This is the connection service registered with Telecom
    private static ConnectionService sTelecomConnectionService;
    private static boolean sIsBound = false;
    private static CountDownLatch sServiceUnBoundLatch = new CountDownLatch(1);

    @Override
    public void onBindClient(Intent intent) {
        sTelecomConnectionService = this;
        Log.i("TelecomCTS", "CS bound");
        sIsBound = true;
    }

    private static Object sLock = new Object();

    public static void setUp(ConnectionService connectionService) throws Exception {
        synchronized(sLock) {
            if (sConnectionService != null) {
                throw new Exception("Mock ConnectionService exists.  Failed to call setUp().");
            }
            sConnectionService = connectionService;
        }
    }

    public static void tearDown() {
        synchronized(sLock) {
            sConnectionService = null;
        }
    }

    @Override
    public Connection onCreateOutgoingConnection(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized(sLock) {
            if (sConnectionService != null) {
                return sConnectionService.onCreateOutgoingConnection(
                        connectionManagerPhoneAccount, request);
            } else {
                Log.e(LOG_TAG,
                        "Tried to create outgoing connection when sConnectionService null!");
                return null;
            }
        }
    }

    @Override
    public Connection onCreateIncomingConnection(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized(sLock) {
            if (sConnectionService != null) {
                return sConnectionService.onCreateIncomingConnection(
                        connectionManagerPhoneAccount, request);
            } else {
                Log.e(LOG_TAG,
                        "Tried to create incoming connection when sConnectionService null!");
                return null;
            }
        }
    }

    @Override
    public void onCreateIncomingConnectionFailed(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        if (sConnectionService != null) {
            sConnectionService.onCreateIncomingConnectionFailed(connectionManagerPhoneAccount,
                    request);
        } else {
            Log.e(LOG_TAG,
                    "onCreateIncomingConnectionFailed called when sConnectionService null!");
        }
    }

    @Override
    public Conference onCreateOutgoingConference(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized (sLock) {
            if (sConnectionService != null) {
                return sConnectionService.onCreateOutgoingConference(connectionManagerPhoneAccount,
                        request);
            } else {
                Log.e(LOG_TAG,
                        "onCreateOutgoingConference called when sConnectionService null!");
                return null;
            }
        }
    }

    @Override
    public void onCreateOutgoingConferenceFailed(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized (sLock) {
            if (sConnectionService != null) {
                sConnectionService.onCreateOutgoingConferenceFailed(connectionManagerPhoneAccount,
                        request);
            } else {
                Log.e(LOG_TAG,
                        "onCreateOutgoingConferenceFailed called when sConnectionService null!");
            }
        }
    }

    @Override
    public Conference onCreateIncomingConference(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized (sLock) {
            if (sConnectionService != null) {
                return sConnectionService.onCreateIncomingConference(connectionManagerPhoneAccount,
                        request);
            } else {
                Log.e(LOG_TAG,
                        "onCreateIncomingConference called when sConnectionService null!");
                return null;
            }
        }
    }

    @Override
    public void onCreateIncomingConferenceFailed(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized (sLock) {
            if (sConnectionService != null) {
                sConnectionService.onCreateIncomingConferenceFailed(connectionManagerPhoneAccount,
                        request);
            } else {
                Log.e(LOG_TAG,
                        "onCreateIncomingConferenceFailed called when sConnectionService null!");
            }
        }
    }

    @Override
    public void onConference(Connection connection1, Connection connection2) {
        synchronized(sLock) {
            if (sConnectionService != null) {
                sConnectionService.onConference(connection1, connection2);
            } else {
                Log.e(LOG_TAG,
                        "onConference called when sConnectionService null!");
            }
        }
    }

    @Override
    public void onRemoteExistingConnectionAdded(RemoteConnection connection) {
        synchronized(sLock) {
            if (sConnectionService != null) {
                sConnectionService.onRemoteExistingConnectionAdded(connection);
            } else {
                Log.e(LOG_TAG,
                        "onRemoteExistingConnectionAdded called when sConnectionService null!");
            }
        }
    }

    public static void addConferenceToTelecom(Conference conference) {
        synchronized(sLock) {
            if (sTelecomConnectionService != null) {
                sTelecomConnectionService.addConference(conference);
            } else {
                Log.e(LOG_TAG, "addConferenceToTelecom called when"
                        + " sTelecomConnectionService null!");
            }
        }
    }

    public static void addExistingConnectionToTelecom(
            PhoneAccountHandle phoneAccountHandle, Connection connection) {
        synchronized(sLock) {
            if (sTelecomConnectionService != null) {
                sTelecomConnectionService.addExistingConnection(phoneAccountHandle, connection);
            } else {
                Log.e(LOG_TAG, "addExistingConnectionToTelecom called when"
                        + " sTelecomConnectionService null!");
            }
        }
    }

    public static Collection<Connection> getAllConnectionsFromTelecom() {
        synchronized(sLock) {
            if (sTelecomConnectionService == null) {
                return Collections.EMPTY_LIST;
            }
            return sTelecomConnectionService.getAllConnections();
        }
    }

    public static RemoteConnection createRemoteOutgoingConnectionToTelecom(
            PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized(sLock) {
            if (sTelecomConnectionService != null) {
                return sTelecomConnectionService.createRemoteOutgoingConnection(
                        connectionManagerPhoneAccount, request);
            } else {
                Log.e(LOG_TAG, "createRemoteOutgoingConnectionToTelecom called when"
                        + " sTelecomConnectionService null!");
                return null;
            }
        }
    }

    public static RemoteConnection createRemoteIncomingConnectionToTelecom(
            PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized(sLock) {
            if (sTelecomConnectionService != null) {
                return sTelecomConnectionService.createRemoteIncomingConnection(
                        connectionManagerPhoneAccount, request);
            } else {
                Log.e(LOG_TAG, "createRemoteIncomingConnectionToTelecom called when"
                        + " sTelecomConnectionService null!");
                return null;
            }
        }
    }

    public static RemoteConference createRemoteIncomingConferenceToTelecom(
            PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized (sLock) {
            if (sTelecomConnectionService != null) {
                return sTelecomConnectionService.createRemoteIncomingConference(
                        connectionManagerPhoneAccount, request);
            } else {
                Log.e(LOG_TAG, "createRemoteIncomingConferenceToTelecom called when"
                        + " sTelecomConnectionService null!");
                return null;
            }
        }
    }


    public static RemoteConference createRemoteOutgoingConferenceToTelecom(
            PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        synchronized (sLock) {
            if (sTelecomConnectionService != null) {
                return sTelecomConnectionService.createRemoteOutgoingConference(
                        connectionManagerPhoneAccount, request);
            } else {
                Log.e(LOG_TAG, "createRemoteOutgoingConferenceToTelecom called when"
                        + " sTelecomConnectionService null!");
                return null;
            }
        }
    }

    @Override
    public void onRemoteConferenceAdded(RemoteConference conference) {
        synchronized(sLock) {
            if (sConnectionService != null) {
                sConnectionService.onRemoteConferenceAdded(conference);
            } else {
                Log.e(LOG_TAG,
                        "onRemoteConferenceAdded called when sConnectionService null!");
            }
        }
    }

    @Override
    public void onConnectionServiceFocusGained() {
        synchronized (sLock) {
            if (sConnectionService != null) {
                sConnectionService.onConnectionServiceFocusGained();
            } else {
                Log.e(LOG_TAG,
                        "onConnectionServiceFocusGained called when sConnectionService null!");
            }
        }
    }

    @Override
    public void onConnectionServiceFocusLost() {
        synchronized (sLock) {
            if (sConnectionService != null) {
                sConnectionService.onConnectionServiceFocusLost();
            } else {
                Log.e(LOG_TAG,
                        "onConnectionServiceFocusLost called when sConnectionService null!");
            }
        }
    }

    @Override
    public boolean onUnbind(Intent intent) {
        synchronized (sLock) {
            Log.i(LOG_TAG, "Service has been unbound");
            sIsBound = false;
            sServiceUnBoundLatch.countDown();
            sConnectionService = null;
            sTelecomConnectionService = null;
            return super.onUnbind(intent);
        }
    }

    public static boolean isServiceRegisteredToTelecom() {
        return sTelecomConnectionService != null;
    }

    public static boolean isBound() {
        return sIsBound;
    }

    public static boolean waitForUnBinding() {
        return TestUtils.waitForLatchCountDown(sServiceUnBoundLatch);
    }
}
