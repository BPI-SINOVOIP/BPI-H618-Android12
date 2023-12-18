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

package com.android.car.evs;

import static android.car.evs.CarEvsManager.ERROR_BUSY;
import static android.car.evs.CarEvsManager.ERROR_NONE;
import static android.car.evs.CarEvsManager.ERROR_UNAVAILABLE;
import static android.car.evs.CarEvsManager.SERVICE_STATE_ACTIVE;
import static android.car.evs.CarEvsManager.SERVICE_STATE_INACTIVE;
import static android.car.evs.CarEvsManager.SERVICE_STATE_REQUESTED;
import static android.car.evs.CarEvsManager.SERVICE_STATE_UNAVAILABLE;
import static android.car.evs.CarEvsManager.STREAM_EVENT_STREAM_STOPPED;

import static com.android.car.CarLog.TAG_EVS;
import static com.android.internal.util.function.pooled.PooledLambda.obtainMessage;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.car.Car;
import android.car.evs.CarEvsBufferDescriptor;
import android.car.evs.CarEvsManager;
import android.car.evs.CarEvsManager.CarEvsError;
import android.car.evs.CarEvsManager.CarEvsServiceState;
import android.car.evs.CarEvsManager.CarEvsServiceType;
import android.car.evs.CarEvsManager.CarEvsStreamEvent;
import android.car.evs.CarEvsStatus;
import android.car.evs.ICarEvsStatusListener;
import android.car.evs.ICarEvsStreamCallback;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarPropertyEventListener;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.hardware.HardwareBuffer;
import android.hardware.automotive.vehicle.V2_0.VehicleArea;
import android.hardware.automotive.vehicle.V2_0.VehicleGear;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.UserHandle;
import android.util.ArraySet;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;

import com.android.car.CarPropertyService;
import com.android.car.CarServiceBase;
import com.android.car.ICarImpl;
import com.android.car.R;
import com.android.car.hal.EvsHalService;
import com.android.internal.annotations.GuardedBy;

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;

/**
 * A service that listens to the Extended View System across a HAL boundary and exposes the data to
 * system clients in Android via {@link android.car.evs.CarEvsManager}.
 *
 * Because of Fast Message Queue usages, android.hardware.automotive.evs@1.1 interfaces does not
 * support Java backend and, therefore, actual API calls are done in native methods.
 *
 *
 * CarEvsService consists of four states:
 *
 * UNAVAILABLE: CarEvsService is not connected to the Extended View System service.  In this
 * state, any service request will be declined.
 *
 * INACTIVE: CarEvsService has a valid, live connection the Extended View System service and
 * ready for any service requests.
 *
 * REQUESTED: CarEvsService received a service requeste from a privileged client and requested
 * the System UI to launch the camera viewing activity.
 *
 * ACTIVE: CarEvsService is actively streaming a video to the client.
 *
 * See CarEvsService.StateMachine class for more details.
 */
public final class CarEvsService extends android.car.evs.ICarEvsService.Stub
        implements CarServiceBase, EvsHalService.EvsHalEventListener {

    private static final boolean DBG = Log.isLoggable(TAG_EVS, Log.DEBUG);

    // Integer value to indicate no buffer with a given id exists
    private static final int BUFFER_NOT_EXIST = -1;

    // Timeout for a stream-stopped confirmation
    private static final int STREAM_STOPPED_WAIT_TIMEOUT_MS = 500;

    // Timeout for a request to start a video stream with a valid token
    private static final int STREAM_START_REQUEST_TIMEOUT_MS = 3000;

    // Interval for connecting to the EVS HAL service trial
    private static final long EVS_HAL_SERVICE_BIND_RETRY_INTERVAL_MS = 1000;

    // Code to identify a message to request an activity again
    private static final int MSG_CHECK_ACTIVITY_REQUEST_TIMEOUT = 0;

    // Service request priorities
    private static final int REQUEST_PRIORITY_HIGH = 0;
    private static final int REQUEST_PRIORITY_NORMAL = 1;
    private static final int REQUEST_PRIORITY_LOW = 2;

    private static final class EvsHalEvent {
        private long mTimestamp;
        private int mServiceType;
        private boolean mOn;

        public EvsHalEvent(long timestamp, @CarEvsServiceType int type, boolean on) {
            mTimestamp = timestamp;
            mServiceType = type;
            mOn = on;
        }

        public long getTimestamp() {
            return mTimestamp;
        }

        public @CarEvsServiceType int getServiceType() {
            return mServiceType;
        }

        public boolean isRequestingToStartActivity() {
            return mOn;
        }

        public String toString() {
            return "ServiceType = " + mServiceType + ", mOn = " + mOn +
                    ", Timestamp = " + mTimestamp;
        }
    }

    private static final String COMMAND_TO_USE_DEFAULT_CAMERA = "default";

    private final Context mContext;
    private final EvsHalService mEvsHalService;
    private final CarPropertyService mPropertyService;
    private final Object mLock = new Object();

    private final ComponentName mEvsCameraActivity;

    // This handler is to monitor the client sends a video stream request within a given time
    // after a state transition to the REQUESTED state.
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    // Bookkeeps received frame buffers
    private final ArraySet mBufferRecords = new ArraySet();

    private final class StatusListenerList extends RemoteCallbackList<ICarEvsStatusListener> {
        private final WeakReference<CarEvsService> mService;

        StatusListenerList(CarEvsService evsService) {
            mService = new WeakReference<>(evsService);
        }

        /** Handle callback death */
        @Override
        public void onCallbackDied(ICarEvsStatusListener listener) {
            Slog.w(TAG_EVS, "StatusListener has died: " + listener.asBinder());

            CarEvsService svc = mService.get();
            if (svc != null) {
                svc.handleClientDisconnected(listener);
            }
        }
    }

    @GuardedBy("mLock")
    private final StatusListenerList mStatusListeners = new StatusListenerList(this);

    private final IBinder.DeathRecipient mStreamCallbackDeathRecipient =
            new IBinder.DeathRecipient() {
        @Override
        public void binderDied() {
            Slog.w(TAG_EVS, "StreamCallback has died");
            synchronized (mLock) {
                if (requestActivityIfNecessaryLocked()) {
                    Slog.i(TAG_EVS, "Requested to launch the activity.");
                }
            }
        }
    };

    /**
     * {@link CarPropertyEvent} listener registered with {@link CarPropertyService} to listen to
     * {@link VehicleProperty.GEAR_SELECTION} change notifications.
     */
    private final ICarPropertyEventListener mGearSelectionPropertyListener =
            new ICarPropertyEventListener.Stub() {
                @Override
                public void onEvent(List<CarPropertyEvent> events) throws RemoteException {
                    synchronized (mLock) {
                        // Handle only the latest event
                        Slog.e(TAG_EVS, "Handling GearSelection");
                        handlePropertyEventLocked(events.get(events.size() - 1));
                    }
                }
            };

    // CarEvsService state machine implementation to handle all state transitions.
    private final class StateMachine {
        // Current state
        @GuardedBy("mLock")
        private int mState = SERVICE_STATE_UNAVAILABLE;

        // Current service type
        @GuardedBy("mLock")
        private int mServiceType = CarEvsManager.SERVICE_TYPE_REARVIEW;

        // Priority of a last service request
        @GuardedBy("mLock")
        private int mLastRequestPriority = REQUEST_PRIORITY_LOW;

        public @CarEvsError int execute(int priority, int destination) {
            return execute(priority, destination, mServiceType, null, null);
        }

        public @CarEvsError int execute(int priority, int destination, int service) {
            return execute(priority, destination, service, null, null);
        }

        public @CarEvsError int execute(int priority, int destination,
                ICarEvsStreamCallback callback) {
            return execute(priority, destination, mServiceType, null, callback);
        }

        public @CarEvsError int execute(int priority, int destination, int service, IBinder token,
                ICarEvsStreamCallback callback) {

            int result = ERROR_NONE;
            synchronized (mLock) {
                // TODO(b/188970686): Reduce this lock duration.
                if (mState == destination && destination != SERVICE_STATE_REQUESTED) {
                    // Nothing to do
                    return ERROR_NONE;
                }

                int previousState = mState;
                Slog.d(TAG_EVS, "Transition requested: " + toString(previousState) +
                        " -> " + toString(destination));

                switch (destination) {
                    case SERVICE_STATE_UNAVAILABLE:
                        result = handleTransitionToUnavailableLocked();
                        break;

                    case SERVICE_STATE_INACTIVE:
                        result = handleTransitionToInactiveLocked(priority, service, callback);
                        break;

                    case SERVICE_STATE_REQUESTED:
                        result = handleTransitionToRequestedLocked(priority, service);
                        break;

                    case SERVICE_STATE_ACTIVE:
                        result = handleTransitionToActiveLocked(priority, service, token, callback);
                        break;

                    default:
                        throw new IllegalStateException(
                                "CarEvsService is in the unknown state, " + previousState);
                }
            }

            if (result == ERROR_NONE) {
                // Broadcasts current state
                broadcastStateTransition(mServiceType, mState);
            }

            return result;
        }

        public @CarEvsServiceState int getState() {
            synchronized (mLock) {
                return mState;
            }
        }

        public @CarEvsServiceType int getServiceType() {
            synchronized (mLock) {
                return mServiceType;
            }
        }

        public CarEvsStatus getStateAndServiceType() {
            synchronized (mLock) {
                return new CarEvsStatus(mServiceType, mState);
            }
        }

        @GuardedBy("mLock")
        public boolean checkCurrentStateRequiresActivityLocked() {
            return mState == SERVICE_STATE_ACTIVE || mState == SERVICE_STATE_REQUESTED;
        }


        @GuardedBy("mLock")
        private @CarEvsError int handleTransitionToUnavailableLocked() {
            // This transition happens only when CarEvsService loses the active connection to the
            // Extended View System service.
            switch (mState) {
                case SERVICE_STATE_UNAVAILABLE:
                    // Nothing to do
                    break;

                default:
                    // Stops any active video stream
                    stopService();
                    break;
            }

            mState = SERVICE_STATE_UNAVAILABLE;
            return ERROR_NONE;
        }

        @GuardedBy("mLock")
        private @CarEvsError int handleTransitionToInactiveLocked(int priority, int service,
                ICarEvsStreamCallback callback) {

            switch (mState) {
                case SERVICE_STATE_UNAVAILABLE:
                    if (callback != null) {
                        // We get a request to stop a video stream after losing a native EVS
                        // service.  Simply unregister a callback and return.
                        unlinkToDeathStreamCallbackLocked();
                        mStreamCallback = null;
                        return ERROR_NONE;
                    } else {
                        // Requested to connect to the Extended View System service
                        if (!nativeConnectToHalServiceIfNecessary(mNativeEvsServiceObj)) {
                            return ERROR_UNAVAILABLE;
                        }
                    }
                    break;

                case SERVICE_STATE_INACTIVE:
                    // Nothing to do
                    break;

                case SERVICE_STATE_REQUESTED:
                    // Requested to cancel a pending service request
                    if (mServiceType != service || mLastRequestPriority > priority) {
                        return ERROR_BUSY;
                    }

                    // Reset a timer for this new request
                    mHandler.removeMessages(MSG_CHECK_ACTIVITY_REQUEST_TIMEOUT);
                    break;

                case SERVICE_STATE_ACTIVE:
                    // Requested to stop a current video stream
                    if (mServiceType != service || mLastRequestPriority > priority) {
                        return ERROR_BUSY;
                    }

                    if (callback != null) {
                        stopVideoStreamAndUnregisterCallback(callback);
                    } else {
                        stopService();
                    }
                    break;

                default:
                    throw new IllegalStateException("CarEvsService is in the unknown state.");
            }

            mState = SERVICE_STATE_INACTIVE;
            setSessionToken(null);
            return ERROR_NONE;
        }

        @GuardedBy("mLock")
        private @CarEvsError int handleTransitionToRequestedLocked(int priority, int service) {
            switch (mState) {
                case SERVICE_STATE_UNAVAILABLE:
                    // Attempts to connect to the native EVS service and transits to the
                    // REQUESTED state if it succeeds.
                    if (!nativeConnectToHalServiceIfNecessary(mNativeEvsServiceObj)) {
                        return ERROR_UNAVAILABLE;
                    }
                    break;

                case SERVICE_STATE_INACTIVE:
                    // Nothing to do
                    break;

                case SERVICE_STATE_REQUESTED:
                    if (priority > mLastRequestPriority) {
                        // A current service request has a lower priority than a previous
                        // service request.
                        Slog.e(TAG_EVS,
                                "CarEvsService is busy with a higher priority client.");
                        return ERROR_BUSY;
                    }

                    // Reset a timer for this new request
                    mHandler.removeMessages(MSG_CHECK_ACTIVITY_REQUEST_TIMEOUT);
                    break;

                case SERVICE_STATE_ACTIVE:
                    if (priority > mLastRequestPriority) {
                        // We decline a request because CarEvsService is busy with a higher priority
                        // client.
                        return ERROR_BUSY;
                    } else if (priority == mLastRequestPriority) {
                        // We do not need to transit to the REQUESTED state because CarEvsService
                        // was transited to the ACTIVE state by a request that has the same priority
                        // with current request.
                        return ERROR_NONE;
                    } else {
                        // Stop stream on all lower priority clients.
                        processStreamEvent(STREAM_EVENT_STREAM_STOPPED);
                    }
                    break;

                default:
                    throw new IllegalStateException("CarEvsService is in the unknown state.");
            }

            // Arms the timer
            mHandler.sendMessageDelayed(obtainMessage(CarEvsService::handleActivityRequestTimeout,
                    CarEvsService.this).setWhat(MSG_CHECK_ACTIVITY_REQUEST_TIMEOUT),
                    STREAM_START_REQUEST_TIMEOUT_MS);

            mState = SERVICE_STATE_REQUESTED;
            mServiceType = service;
            mLastRequestPriority = priority;

            if (mEvsCameraActivity != null) {
                Intent evsIntent = new Intent(Intent.ACTION_MAIN)
                        .setComponent(mEvsCameraActivity)
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT)
                        .addFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK)
                        .addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
                if (priority == REQUEST_PRIORITY_HIGH) {
                    mSessionToken = new Binder();
                    evsIntent.putExtra(CarEvsManager.EXTRA_SESSION_TOKEN, mSessionToken);
                }
                mContext.startActivity(evsIntent);
            }
            return ERROR_NONE;
        }

        @GuardedBy("mLock")
        private @CarEvsError int handleTransitionToActiveLocked(int priority, int service,
                IBinder token, ICarEvsStreamCallback callback) {

            @CarEvsError int result = ERROR_NONE;
            switch (mState) {
                case SERVICE_STATE_UNAVAILABLE:
                    // We do not have a valid connection to the Extended View System service.
                    return ERROR_UNAVAILABLE;

                case SERVICE_STATE_INACTIVE:
                    // CarEvsService receives a low priority request to start a video stream.
                    result = startServiceAndVideoStream(service, callback);
                    if (result != ERROR_NONE) {
                        return result;
                    }
                    break;

                case SERVICE_STATE_REQUESTED:
                    // CarEvsService is reserved for higher priority clients.
                    if (priority == REQUEST_PRIORITY_HIGH && !isSessionToken(token)) {
                        // Declines a request with an expired token.
                        return ERROR_BUSY;
                    }

                    result = startServiceAndVideoStream(service, callback);
                    if (result != ERROR_NONE) {
                        return result;
                    }
                    break;

                case SERVICE_STATE_ACTIVE:
                    // CarEvsManager will transfer an active video stream to a new client with a
                    // higher or equal priority.
                    if (priority > mLastRequestPriority) {
                        Slog.i(TAG_EVS, "Declines a service request with a lower priority.");
                        break;
                    }

                    if (mStreamCallback != null) {
                        mHandler.sendMessage(obtainMessage(
                                CarEvsService::notifyStreamStopped, mStreamCallback));
                    }

                    mStreamCallback = callback;
                    break;

                default:
                    throw new IllegalStateException("CarEvsService is in the unknown state.");
            }

            mState = SERVICE_STATE_ACTIVE;
            mServiceType = service;
            mLastRequestPriority = priority;
            return ERROR_NONE;
        }

        private String toString(@CarEvsServiceState int state) {
            switch (state) {
                case SERVICE_STATE_UNAVAILABLE:
                    return "UNAVAILABLE";
                case SERVICE_STATE_INACTIVE:
                    return "INACTIVE";
                case SERVICE_STATE_REQUESTED:
                    return "REQUESTED";
                case SERVICE_STATE_ACTIVE:
                    return "ACTIVE";
                default:
                    return "UNKNOWN";
            }
        }

        public String toString() {
            return toString(mState);
        }
    }

    private final StateMachine mStateEngine = new StateMachine();

    @GuardedBy("mLock")
    private ICarEvsStreamCallback mStreamCallback = null;

    // The latest session token issued to the privileged clients
    @GuardedBy("mLock")
    private IBinder mSessionToken = null;

    // This boolean flag is true if CarEvsService uses GEAR_SELECTION VHAL property instead of
    // EVS_SERVICE_REQUEST.
    private boolean mUseGearSelection = true;

    // When this is set, CarEvsService will attempt to open a camera device the user sets.
    private boolean mUseCameraIdOverride = false;

    // This is a device name to be used when mUseCameraIdOverride is true.
    private String mCameraIdOverride;

    private void setSessionToken(IBinder token) {
        synchronized (mLock) {
            mSessionToken = token;
        }
    }

    private boolean isSessionToken(IBinder token) {
        synchronized (mLock) {
            return token != null && token == mSessionToken;
        }
    }

    // Synchronization object for a stream-stopped confirmation
    private CountDownLatch mStreamStoppedEvent = new CountDownLatch(0);

    // The last event EvsHalService reported.  This will be set to null when a related service
    // request is handled.
    @GuardedBy("mLock")
    private EvsHalEvent mLastEvsHalEvent = new EvsHalEvent(SystemClock.elapsedRealtimeNanos(),
            CarEvsManager.SERVICE_TYPE_REARVIEW, /* on = */false);

    // Stops a current video stream and unregisters a callback
    private void stopVideoStreamAndUnregisterCallback(ICarEvsStreamCallback callback) {
        synchronized (mLock) {
            if (callback.asBinder() != mStreamCallback.asBinder()) {
                Slog.i(TAG_EVS, "Declines a request to stop a video not from a current client.");
                return;
            }

            unlinkToDeathStreamCallbackLocked();
            mStreamCallback = null;
            Slog.i(TAG_EVS, "Last stream client has been disconnected.");
            nativeRequestToStopVideoStream(mNativeEvsServiceObj);
        }
    }

    // Starts a service and its video stream
    @GuardedBy("mLock")
    private @CarEvsError int startServiceAndVideoStream(
            @CarEvsServiceType int service, ICarEvsStreamCallback callback) {
        if (!startService(service)) {
            return ERROR_UNAVAILABLE;
        }

        mStreamCallback = callback;
        linkToDeathStreamCallbackLocked();

        if (!nativeRequestToStartVideoStream(mNativeEvsServiceObj)) {
            Slog.e(TAG_EVS, "Failed to start a video stream");
            mStreamCallback = null;
            return ERROR_UNAVAILABLE;
        }

        return ERROR_NONE;
    }

    @GuardedBy("mLock")
    private boolean requestActivityIfNecessaryLocked() {
        if (!mStateEngine.checkCurrentStateRequiresActivityLocked() || mLastEvsHalEvent == null ||
                !mLastEvsHalEvent.isRequestingToStartActivity()) {
            return false;
        }

        mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_REQUESTED,
                mLastEvsHalEvent.getServiceType());
        return true;
    }

    // Waits for a video stream request from the System UI with a valid token.
    private void handleActivityRequestTimeout() {
        synchronized (mLock) {
            // No client has responded to a state transition to the REQUESTED
            // state before the timer expires.  CarEvsService sends a
            // notification again if it's still needed.
            if (requestActivityIfNecessaryLocked()) {
                Slog.w(TAG_EVS, "Timer expired.  Request to launch the activity again.");
                return;
            } else if (mStateEngine.getState() == SERVICE_STATE_REQUESTED) {
                // If the service is no longer required by other services, we transit to
                // the INACTIVE state.
                mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_INACTIVE);
            }
        }
    }

    @GuardedBy("mLock")
    private void linkToDeathStreamCallbackLocked() {
        IBinder binder;
        if (mStreamCallback == null) {
            return;
        }

        binder = mStreamCallback.asBinder();
        if (binder == null) {
            Slog.w(TAG_EVS, "Linking to a binder death recipient skipped");
            return;
        }

        try {
            binder.linkToDeath(mStreamCallbackDeathRecipient, 0);
        } catch (RemoteException e) {
            Slog.w(TAG_EVS, "Failed to link a binder death recipient: " + e);
        }
    }

    @GuardedBy("mLock")
    private void unlinkToDeathStreamCallbackLocked() {
        IBinder binder;
        if (mStreamCallback == null) {
            return;
        }

        binder = mStreamCallback.asBinder();
        if (binder == null) {
            return;
        }

        binder.unlinkToDeath(mStreamCallbackDeathRecipient, 0);
    }

    /** Creates an Extended View System service instance given a {@link Context}. */
    public CarEvsService(Context context, EvsHalService halService,
            CarPropertyService propertyService) {
        mContext = context;
        mPropertyService = propertyService;
        mEvsHalService = halService;

        String activityName = mContext.getResources().getString(R.string.config_evsCameraActivity);
        if (!activityName.isEmpty()) {
            mEvsCameraActivity = ComponentName.unflattenFromString(activityName);
        } else {
            mEvsCameraActivity = null;
        }
        if (DBG) Slog.d(TAG_EVS, "evsCameraActivity=" + mEvsCameraActivity);
    }

    /** Implements EvsHalService.EvsHalEventListener to monitor VHAL properties. */
    @Override
    public void onEvent(@CarEvsServiceType int type, boolean on) {
        if (DBG) {
            Slog.d(TAG_EVS,
                    "Received an event from EVS HAL: type = " + type + ", on = " + on);
        }

        synchronized (mLock) {
            int targetState = on ? SERVICE_STATE_REQUESTED : SERVICE_STATE_INACTIVE;
            if (mStateEngine.execute(REQUEST_PRIORITY_HIGH, targetState, type) !=
                    ERROR_NONE) {
                Slog.e(TAG_EVS, "Failed to execute a service request.");
            }

            // Stores the last event
            mLastEvsHalEvent = new EvsHalEvent(SystemClock.elapsedRealtimeNanos(), type, on);
        }
    }

    @Override
    public void init() {
        if (DBG) {
            Slog.d(TAG_EVS, "Initializing the service");
        }

        if (mEvsHalService.isEvsServiceRequestSupported()) {
            try {
                mEvsHalService.setListener(this);
                if (DBG) {
                    Slog.d(TAG_EVS, "CarEvsService listens to EVS_SERVICE_REQUEST property.");
                }
                mUseGearSelection = false;
            } catch (IllegalStateException e) {
                Slog.w(TAG_EVS,
                        "Failed to set a EvsHalService listener.  Try to use GEAR_SELECTION.");
            }
        }

        if (mUseGearSelection) {
            if (DBG) {
                Slog.d(TAG_EVS, "CarEvsService listens to GEAR_SELECTION property.");
            }

            if (mPropertyService == null ||
                mPropertyService.getProperty(VehicleProperty.GEAR_SELECTION,
                        VehicleArea.GLOBAL) == null) {
                Slog.e(TAG_EVS,
                        "CarEvsService is disabled because GEAR_SELECTION is unavailable.");
                mUseGearSelection = false;
                return;
            }

            mPropertyService.registerListener(VehicleProperty.GEAR_SELECTION, /*rate=*/0,
                    mGearSelectionPropertyListener);
        }

        // Creates a handle to use the native Extended View System service
        mNativeEvsServiceObj = CarEvsService.nativeCreateServiceHandle();

        // Attempts to transit to the INACTIVE state
        if (mNativeEvsServiceObj == 0 ||
            mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_INACTIVE) != ERROR_NONE) {
            Slog.e(TAG_EVS, "Failed to create a service handle or transit to the INACTIVE state,");
            CarEvsService.nativeDestroyServiceHandle(mNativeEvsServiceObj);
            mNativeEvsServiceObj = 0;

            if (mUseGearSelection && mPropertyService != null) {
                if (DBG) {
                    Slog.d(TAG_EVS, "Unregister a property listener on init() failure.");
                }
                mPropertyService.unregisterListener(VehicleProperty.GEAR_SELECTION,
                        mGearSelectionPropertyListener);
            }
        }
    }

    @Override
    public void release() {
        if (DBG) {
            Slog.d(TAG_EVS, "Finalizing the service");
        }

        synchronized (mLock) {
            if (mUseGearSelection && mPropertyService != null) {
                if (DBG) {
                    Slog.d(TAG_EVS, "Unregister a property listener in release()");
                }
                mPropertyService.unregisterListener(VehicleProperty.GEAR_SELECTION,
                        mGearSelectionPropertyListener);
            }
            mStatusListeners.kill();
        }

        CarEvsService.nativeDestroyServiceHandle(mNativeEvsServiceObj);
        mNativeEvsServiceObj = 0;
    }

    @Override
    public void dump(IndentingPrintWriter writer) {
        writer.println("*CarEvsService*");
        writer.printf("Current state = %s\n", mStateEngine);
        writer.printf("%s to HAL service\n",
                mNativeEvsServiceObj == 0 ? "Not connected" : "Connected");

        ICarEvsStreamCallback cb;
        synchronized (mLock) {
            cb = mStreamCallback;
        }
        writer.printf("Active stream client = %s\n", cb == null? "null" : cb.asBinder());
        writer.printf("%d service listeners subscribed.\n",
                mStatusListeners.getRegisteredCallbackCount());
        writer.printf("Last HAL event = %s\n", mLastEvsHalEvent);
        writer.printf("Current session token = %s\n", mSessionToken);
    }

    /**
     * Registers a {@link ICarEvsStatusListener} to listen requests to control the camera
     * previewing activity.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_MONITOR_CAR_EVS_STATUS} permissions to
     * access.
     *
     * @param listener {@link ICarEvsStatusListener} listener to register.
     */
    @Override
    public void registerStatusListener(@NonNull ICarEvsStatusListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_MONITOR_CAR_EVS_STATUS);
        Objects.requireNonNull(listener);

        if (DBG) {
            Slog.d(TAG_EVS, "Registering a new service listener");
        }
        mStatusListeners.register(listener);
    }

    /**
     * Unregister the given {@link ICarEvsStatusListener} listener from receiving events.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_MONITOR_CAR_EVS_STATUS} permissions to
     * access.
     *
     * @param listener {@link ICarEvsStatusListener} listener to unregister.
     */
    @Override
    public void unregisterStatusListener(@NonNull ICarEvsStatusListener listener) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_MONITOR_CAR_EVS_STATUS);
        Objects.requireNonNull(listener);

        mStatusListeners.unregister(listener);
    }

    /**
     * Requests the system to start an activity to show the preview from a given EVS service type.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_REQUEST_CAR_EVS_ACTIVITY} permissions to
     * access.
     *
     * @param type {@link android.car.evs.CarEvsManager#CarEvsServiceType}
     * @return {@link android.car.evs.CarEvsManager#CarEvsError}
     */
    @Override
    public @CarEvsError int startActivity(int type) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_REQUEST_CAR_EVS_ACTIVITY);

        return mStateEngine.execute(REQUEST_PRIORITY_NORMAL, SERVICE_STATE_REQUESTED, type);
    }

    /**
     * Requests to stop a current previewing activity launched via {@link #startActivity}.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_REQUEST_CAR_EVS_ACTIVITY} permissions to
     * access.
     */
    @Override
    public void stopActivity() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_REQUEST_CAR_EVS_ACTIVITY);

        mStateEngine.execute(REQUEST_PRIORITY_NORMAL, SERVICE_STATE_INACTIVE);
    }

    /**
     * Starts a video stream.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_USE_CAR_EVS_CAMERA} permissions to access.
     *
     * @param type {@link android.car.evs.CarEvsManager#CarEvsServiceType}
     * @param token IBinder object as a session token.  If this is not null, CarEvsService handles a
     *              coming client as a privileged client.
     * @param callback {@link ICarEvsStreamCallback} listener to register.
     * @return {@link android.car.evs.CarEvsManager.CarEvsError}
     */
    @Override
    public @CarEvsError int startVideoStream(@CarEvsServiceType int type, @Nullable IBinder token,
            @NonNull ICarEvsStreamCallback callback) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_EVS_CAMERA);
        Objects.requireNonNull(callback);

        if (isSessionToken(token)) {
            mHandler.removeMessages(MSG_CHECK_ACTIVITY_REQUEST_TIMEOUT);
        }

        final int priority = token != null ? REQUEST_PRIORITY_HIGH : REQUEST_PRIORITY_LOW;
        return mStateEngine.execute(priority, SERVICE_STATE_ACTIVE, type, token, callback);
    }

    /**
     * Requests to stop a video stream from the current service.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_USE_CAR_EVS_CAMERA} permissions to access.
     *
     * @param callback {@link ICarEvsStreamCallback} listener to unregister.
     */
    @Override
    public void stopVideoStream(@NonNull ICarEvsStreamCallback callback) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_EVS_CAMERA);
        Objects.requireNonNull(callback);

        synchronized (mLock) {
            if (mStreamCallback == null || callback.asBinder() != mStreamCallback.asBinder()) {
                Slog.i(TAG_EVS, "Ignores a video stream request not from current stream client.");
                return;
            }
        }

        if (mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_INACTIVE, callback) !=
                ERROR_NONE) {
            Slog.w(TAG_EVS, "Failed to stop a video stream");

            // We want to return if a video stop request fails.
            return;
        }
    }

    /**
     * Returns an used buffer to EVS service.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_USE_CAR_EVS_CAMERA} permissions to access.
     *
     * @param bufferId An unique 32-bit integer identifier of the buffer to return.
     * @throws IllegalArgumentException if a passed buffer has an unregistered identifier.
     */
    @Override
    public void returnFrameBuffer(int bufferId) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_EVS_CAMERA);

        boolean returnThisBuffer = false;
        synchronized (mLock) {
            if (!mBufferRecords.contains(bufferId)) {
                Slog.w(TAG_EVS,
                        "Ignores a request to return a buffer with unknown id = " + bufferId);
                return;
            }

            mBufferRecords.remove(bufferId);
        }

        // This may throw a NullPointerException if the native EVS service handle is invalid.
        nativeDoneWithFrame(mNativeEvsServiceObj, bufferId);
    }

    /**
     * Returns a current status of CarEvsService.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_MONITOR_CAR_EVS_STATUS} permissions to
     * access.
     *
     * @return {@link android.car.evs.CarEvsStatus}
     */
    @Override
    @Nullable
    public CarEvsStatus getCurrentStatus() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_MONITOR_CAR_EVS_STATUS);

        return mStateEngine.getStateAndServiceType();
    }

    // TODO(b/157082995): Replaces below method with what PackageManager provides.
    @Nullable
    private String getSystemUiPackageName() {
        try {
            ComponentName componentName = ComponentName.unflattenFromString(mContext.getResources()
                    .getString(com.android.internal.R.string.config_systemUIServiceComponent));
            return componentName.getPackageName();
        } catch (RuntimeException e) {
            throw new IllegalStateException("error while getting system UI package name.");
        }
    }

    /**
     * Returns a session token to be used to request the services.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_CONTROL_CAR_EVS_ACTIVITY} permission to access.
     *
     * @return IBinder object as a session token.
     * @throws IllegalStateException if we fail to find System UI package.
     */
    @Override
    public IBinder generateSessionToken() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_CONTROL_CAR_EVS_ACTIVITY);

        String systemUiPackageName = getSystemUiPackageName();
        IBinder token = new Binder();
        try {
            int systemUiUid = mContext.getPackageManager().getPackageUidAsUser(
                    systemUiPackageName, UserHandle.USER_SYSTEM);
            int callerUid = Binder.getCallingUid();
            if (systemUiUid == callerUid) {
                setSessionToken(token);
            } else {
                throw new SecurityException("SystemUI only can generate SessionToken.");
            }
        } catch (NameNotFoundException err) {
            throw new IllegalStateException(systemUiPackageName + " package not found.");
        } finally {
            return token;
        }
    }

    private void handleClientDisconnected(ICarEvsStatusListener listener) {
        synchronized (mLock) {
            mStatusListeners.unregister(listener);
            if (mStatusListeners.getRegisteredCallbackCount() == 0) {
                Slog.d(TAG_EVS, "Last status listener has been disconnected.");
            }
        }
    }

    /**
     * Returns whether or not a given service type is supported.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_MONITOR_CAR_EVS_STATUS} permissions to
     * access.
     */
    @Override
    public boolean isSupported(@CarEvsServiceType int type) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_MONITOR_CAR_EVS_STATUS);

        switch (type) {
            case CarEvsManager.SERVICE_TYPE_REARVIEW:
                // We store a handle to the native EVS service in mNativeEvsServiceObj variable.
                return mNativeEvsServiceObj != 0;

            case CarEvsManager.SERVICE_TYPE_SURROUNDVIEW:
                // TODO(b/179029031): Implements necessary logic when Surround View service is
                // integrated.
                return false;

            default:
                throw new IllegalArgumentException("Unknown service type = " + type);
        }
    }

    /**
     * Sets a camera device for the rearview.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_USE_CAR_EVS_CAMERA} permissions to access.
     *
     * @param id A string identifier of a target camera device.
     * @return This method return a false if this runs in a release build; otherwise, this returns
     *         true.
     */
    public boolean setRearviewCameraIdFromCommand(@NonNull String id) {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_USE_CAR_EVS_CAMERA);
        Objects.requireNonNull(id);

        if (!Build.IS_DEBUGGABLE) {
            // This method is not allowed in the release build.
            return false;
        }

        if (id.equalsIgnoreCase(COMMAND_TO_USE_DEFAULT_CAMERA)) {
            mUseCameraIdOverride = false;
            Slog.i(TAG_EVS, "CarEvsService is set to use the default device for the rearview.");
        } else {
            mCameraIdOverride = id;
            mUseCameraIdOverride = true;
            Slog.i(TAG_EVS, "CarEvsService is set to use " + id + " for the rearview.");
        }

        return true;
    }

    /**
     * Gets an identifier of a current camera device for the rearview.
     *
     * <p>Requires {@link android.car.Car.PERMISSION_MONITOR_CAR_EVS_STATUS} permissions to
     * access.
     *
     * @return A string identifier of current rearview camera device.
     */
    @NonNull
    public String getRearviewCameraIdFromCommand() {
        ICarImpl.assertPermission(mContext, Car.PERMISSION_MONITOR_CAR_EVS_STATUS);
        if (mUseCameraIdOverride) {
            return mCameraIdOverride;
        } else {
            return mContext.getString(R.string.config_evsRearviewCameraId);
        }
    }

    /** Handles client disconnections; may request to stop a video stream. */
    private void handleClientDisconnected(ICarEvsStreamCallback callback) {
        // If the last stream client is disconnected before it stops a video stream, request to stop
        // current video stream.
        mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_INACTIVE, callback);
    }

    /** Notifies the service status gets changed */
    private void broadcastStateTransition(int type, int state) {
        int idx = mStatusListeners.beginBroadcast();
        while (idx-- > 0) {
            ICarEvsStatusListener listener = mStatusListeners.getBroadcastItem(idx);
            try {
                listener.onStatusChanged(new CarEvsStatus(type, state));
            } catch (RemoteException e) {
                // Likely the binder death incident
                Slog.e(TAG_EVS, Log.getStackTraceString(e));
            }
        }
        mStatusListeners.finishBroadcast();
    }

    /** Starts a requested service */
    private boolean startService(@CarEvsServiceType int type) {
        if (type == CarEvsManager.SERVICE_TYPE_SURROUNDVIEW) {
            // TODO(b/179029031): Removes below when Surround View service is integrated.
            Slog.e(TAG_EVS, "Surround view is not supported yet.");
            return false;
        }

        if (!nativeConnectToHalServiceIfNecessary(mNativeEvsServiceObj)) {
            Slog.e(TAG_EVS, "Failed to connect to EVS service");
            return false;
        }

        String cameraId;
        if (mUseCameraIdOverride) {
            cameraId = mCameraIdOverride;
        } else {
            cameraId = mContext.getString(R.string.config_evsRearviewCameraId);
        }

        if (!nativeOpenCamera(mNativeEvsServiceObj, cameraId)) {
            Slog.e(TAG_EVS, "Failed to open a target camera device");
            return false;
        }

        return true;
    }

    /** Stops a current service */
    private void stopService() {
        try {
            nativeRequestToStopVideoStream(mNativeEvsServiceObj);
        } catch (RuntimeException e) {
            Slog.w(TAG_EVS, Log.getStackTraceString(e));
        } finally {
            // Unregister all stream callbacks.
            synchronized (mLock) {
                mStreamCallback = null;
            }

            // We simply drop all buffer records; the native method will return all pending buffers
            // to the native Extended System View service if it is alive.
            synchronized (mBufferRecords) {
                mBufferRecords.clear();
            }

            // Cancel a pending message to check a request timeout
            mHandler.removeMessages(MSG_CHECK_ACTIVITY_REQUEST_TIMEOUT);
        }
    }

    @GuardedBy("mLock")
    private void handlePropertyEventLocked(CarPropertyEvent event) {
        if (event.getEventType() != CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE) {
            // CarEvsService is interested only in the property change event.
            return;
        }

        CarPropertyValue value = event.getCarPropertyValue();
        if (value.getPropertyId() != VehicleProperty.GEAR_SELECTION) {
            // CarEvsService is interested only in the GEAR_SELECTION property.
            return;
        }

        long timestamp = value.getTimestamp();
        if (timestamp <= mLastEvsHalEvent.getTimestamp()) {
            if (DBG) {
                Slog.d(TAG_EVS,
                        "Ignoring GEAR_SELECTION change happened past, timestamp = " + timestamp +
                        ", last event was at " + mLastEvsHalEvent.getTimestamp());
            }
            return;
        }

        // TODO(b/179029031): CarEvsService may need to process VehicleGear.GEAR_PARK when
        // Surround View service is integrated.
        int gear = (Integer) value.getValue();
        if (gear == VehicleGear.GEAR_REVERSE) {
            // Request to start the rearview activity when the gear is shifted into the reverse
            // position.
            if (mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_REQUESTED,
                    CarEvsManager.SERVICE_TYPE_REARVIEW) != ERROR_NONE) {
                Slog.w(TAG_EVS, "Failed to request the rearview activity.");
            }
        } else {
            // Request to stop the rearview activity when the gear is shifted from the reverse
            // position to other positions.
            if (mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_INACTIVE,
                    CarEvsManager.SERVICE_TYPE_REARVIEW) != ERROR_NONE) {
                Slog.d(TAG_EVS, "Failed to stop the rearview activity.");
            }
        }

        mLastEvsHalEvent = new EvsHalEvent(timestamp, CarEvsManager.SERVICE_TYPE_REARVIEW,
                gear == VehicleGear.GEAR_REVERSE);
    }

    /** Processes a streaming event and propagates it to registered clients */
    private void processStreamEvent(@CarEvsStreamEvent int event) {
        synchronized (mLock) {
            if (mStreamCallback == null) {
                return;
            }

            try {
                mStreamCallback.onStreamEvent(event);
            } catch (RemoteException e) {
                // Likely the binder death incident
                Slog.e(TAG_EVS, Log.getStackTraceString(e));
            }
        }
    }

    /** Processes a streaming event and propagates it to registered clients */
    private void processNewFrame(int id, @NonNull HardwareBuffer buffer) {
        Objects.requireNonNull(buffer);

        synchronized (mLock) {
            mBufferRecords.add(id);
            if (mStreamCallback == null) {
                return;
            }

            try {
                mStreamCallback.onNewFrame(new CarEvsBufferDescriptor(id, buffer));
            } catch (RemoteException e) {
                // Likely the binder death incident
                Slog.e(TAG_EVS, Log.getStackTraceString(e));
            }
        }
    }

    /** EVS stream event handler called after a native handler */
    private void postNativeEventHandler(int eventType) {
        processStreamEvent(
                CarEvsServiceUtils.convertToStreamEvent(eventType));
    }

    /** EVS frame handler called after a native handler */
    private void postNativeFrameHandler(int id, HardwareBuffer buffer) {
        processNewFrame(id, buffer);
    }

    /** EVS service death handler called after a native handler */
    private void postNativeDeathHandler() {
        // We have lost the Extended View System service.
        mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_UNAVAILABLE);
        connectToHalServiceIfNecessary(EVS_HAL_SERVICE_BIND_RETRY_INTERVAL_MS);
    }

    /** Try to connect to the EVS HAL service until it succeeds at a given interval */
    private void connectToHalServiceIfNecessary(long intervalInMillis) {
        Slog.d(TAG_EVS, "Trying to connect to the EVS HAL service.");
        if (mStateEngine.execute(REQUEST_PRIORITY_HIGH, SERVICE_STATE_INACTIVE) != ERROR_NONE) {
            // Try to restore a connection again after a given amount of time
            mHandler.sendMessageDelayed(obtainMessage(
                    CarEvsService::connectToHalServiceIfNecessary, this, intervalInMillis),
                    intervalInMillis);
        }
    }

    /** Notify the client of a video stream loss */
    private static void notifyStreamStopped(@NonNull ICarEvsStreamCallback callback) {
        Objects.requireNonNull(callback);

        try {
            callback.onStreamEvent(CarEvsManager.STREAM_EVENT_STREAM_STOPPED);
        } catch (RemoteException e) {
            // Likely the binder death incident
            Slog.w(TAG_EVS, Log.getStackTraceString(e));
        }
    }

    /**
     * Because of its dependency on FMQ type, android.hardware.automotive.evs@1.1 interface does
     * not support Java backend.  Therefore, all hwbinder transactions happen in native methods
     * declared below.
     */

    static {
        System.loadLibrary("carservicejni");
    }

    /** Stores a service handle initialized in native methods */
    private long mNativeEvsServiceObj = 0;

    /** Attempts to connect to the HAL service if it has not done yet */
    private native boolean nativeConnectToHalServiceIfNecessary(long handle);

    /** Attempts to disconnect from the HAL service */
    private native void nativeDisconnectFromHalService(long handle);

    /** Attempts to open a target camera device */
    private native boolean nativeOpenCamera(long handle, String cameraId);

    /** Requests to close a target camera device */
    private native void nativeCloseCamera(long handle);

    /** Requests to start a video stream */
    private native boolean nativeRequestToStartVideoStream(long handle);

    /** Requests to stop a video stream */
    private native void nativeRequestToStopVideoStream(long handle);

    /** Request to return an used buffer */
    private native void nativeDoneWithFrame(long handle, int bufferId);

    /** Creates a EVS service handle */
    private static native long nativeCreateServiceHandle();

    /** Destroys a EVS service handle */
    private static native void nativeDestroyServiceHandle(long handle);
}
