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

package android.car.evs;

import android.annotation.CallbackExecutor;
import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.RequiresPermission;
import android.annotation.SuppressLint;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.annotation.RequiredFeature;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.util.Slog;

import com.android.internal.annotations.GuardedBy;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.util.Objects;
import java.util.concurrent.Executor;

/**
 * Provides an application interface for interativing with the Extended View System service.
 *
 * @hide
 */
@RequiredFeature(Car.CAR_EVS_SERVICE)
@SystemApi
public final class CarEvsManager extends CarManagerBase {
    public static final String EXTRA_SESSION_TOKEN = "android.car.evs.extra.SESSION_TOKEN";

    private static final String TAG = CarEvsManager.class.getSimpleName();
    private static final boolean DBG = Log.isLoggable(TAG, Log.DEBUG);

    private final ICarEvsService mService;
    private final Object mStreamLock = new Object();

    @GuardedBy("mStreamLock")
    private CarEvsStreamCallback mStreamCallback;

    @GuardedBy("mStreamLock")
    private Executor mStreamCallbackExecutor;

    private final CarEvsStreamListenerToService mStreamListenerToService =
            new CarEvsStreamListenerToService(this);

    private final Object mStatusLock = new Object();

    @GuardedBy("mStatusLock")
    private CarEvsStatusListener mStatusListener;

    @GuardedBy("mStatusLock")
    private Executor mStatusListenerExecutor;

    private final CarEvsStatusListenerToService mStatusListenerToService =
            new CarEvsStatusListenerToService(this);

    /**
     * Service type to represent the rearview camera service.
     */
    public static final int SERVICE_TYPE_REARVIEW = 0;

    /**
     * Service type to represent the surround view service.
     */
    public static final int SERVICE_TYPE_SURROUNDVIEW = 1;

    /** @hide */
    @IntDef (prefix = {"SERVICE_TYPE_"}, value = {
            SERVICE_TYPE_REARVIEW,
            SERVICE_TYPE_SURROUNDVIEW,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface CarEvsServiceType {}

    /**
     * State that a corresponding service type is not available.
     */
    public static final int SERVICE_STATE_UNAVAILABLE = 0;

    /**
     * State that a corresponding service type is inactive; it's available but not used
     * by any clients.
     */
    public static final int SERVICE_STATE_INACTIVE = 1;

    /**
     * State that CarEvsManager received a service request from the client.
     */
    public static final int SERVICE_STATE_REQUESTED = 2;

    /**
     * State that a corresponding service type is actively being used.
     */
    public static final int SERVICE_STATE_ACTIVE = 3;

    /** @hide */
    @IntDef (prefix = {"SERVICE_STATE_"}, value = {
            SERVICE_STATE_UNAVAILABLE,
            SERVICE_STATE_INACTIVE,
            SERVICE_STATE_REQUESTED,
            SERVICE_STATE_ACTIVE
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface CarEvsServiceState {}

    /**
     * This is a default EVS stream event type.
     */
    public static final int STREAM_EVENT_NONE = 0;

    /**
     * EVS stream event to notify a video stream has been started.
     */
    public static final int STREAM_EVENT_STREAM_STARTED = 1;

    /**
     * EVS stream event to notify a video stream has been stopped.
     */
    public static final int STREAM_EVENT_STREAM_STOPPED = 2;

    /**
     * EVS stream event to notify that a video stream is dropped.
     */
    public static final int STREAM_EVENT_FRAME_DROPPED = 3;

    /**
     * EVS stream event occurs when a timer for a new frame's arrival is expired.
     */
    public static final int STREAM_EVENT_TIMEOUT = 4;

    /**
     * EVS stream event occurs when a camera parameter is changed.
     */
    public static final int STREAM_EVENT_PARAMETER_CHANGED = 5;

    /**
     * EVS stream event to notify the primary owner has been changed.
     */
    public static final int STREAM_EVENT_PRIMARY_OWNER_CHANGED = 6;

    /**
     * Other EVS stream errors
     */
    public static final int STREAM_EVENT_OTHER_ERRORS = 7;

    /** @hide */
    @IntDef(prefix = {"STREAM_EVENT_"}, value = {
        STREAM_EVENT_NONE,
        STREAM_EVENT_STREAM_STARTED,
        STREAM_EVENT_STREAM_STOPPED,
        STREAM_EVENT_FRAME_DROPPED,
        STREAM_EVENT_TIMEOUT,
        STREAM_EVENT_PARAMETER_CHANGED,
        STREAM_EVENT_PRIMARY_OWNER_CHANGED,
        STREAM_EVENT_OTHER_ERRORS
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface CarEvsStreamEvent {}

    /**
     * Status to tell that a request is successfully processed.
     */
    public static final int ERROR_NONE = 0;

    /**
     * Status to tell a requested service is not available.
     */
    public static final int ERROR_UNAVAILABLE = -1;

    /**
     * Status to tell CarEvsService is busy to serve the privileged client.
     */
    public static final int ERROR_BUSY = -2;

    /** @hide */
    @IntDef(prefix = {"ERROR_"}, value = {
        ERROR_NONE,
        ERROR_UNAVAILABLE,
        ERROR_BUSY
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface CarEvsError {}

    /**
     * Gets an instance of CarEvsManager
     *
     * CarEvsManager manages {@link com.android.car.evs.CarEvsService} and provides APIs that the
     * clients can use the Extended View System service.
     *
     * This must not be obtained directly by clients, use {@link Car#getCarManager(String)} instead.
     *
     * @hide
     */
    public CarEvsManager(Car car, IBinder service) {
        super(car);

        // Gets CarEvsService
        mService = ICarEvsService.Stub.asInterface(service);
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        synchronized (mStatusLock) {
            mStatusListener = null;
            mStatusListenerExecutor = null;
        }

        synchronized (mStreamLock) {
            mStreamCallback = null;
            mStreamCallbackExecutor = null;
        }
    }

    /**
     * Application registers {@link #CarEvsStatusListener} object to receive requests to control
     * the activity and monitor the status of the EVS service.
     */
    public interface CarEvsStatusListener {
        /**
         * Called when the status of EVS service is changed.
         *
         * @param type A type of EVS service; e.g. the rearview.
         * @param state Updated service state; e.g. the service is started.
         */
        void onStatusChanged(@NonNull CarEvsStatus status);
    }

    /**
     * Class implementing the listener interface {@link com.android.car.ICarEvsStatusListener}
     * to listen status updates across the binder interface.
     */
    private static class CarEvsStatusListenerToService extends ICarEvsStatusListener.Stub {
        private final WeakReference<CarEvsManager> mManager;

        CarEvsStatusListenerToService(CarEvsManager manager) {
            mManager = new WeakReference<>(manager);
        }

        @Override
        public void onStatusChanged(@NonNull CarEvsStatus status) {
            Objects.requireNonNull(status);

            CarEvsManager mgr = mManager.get();
            if (mgr != null) {
                mgr.handleServiceStatusChanged(status);
            }
        }
    }

    /**
     * Gets the {@link #CarEvsStatus} from the service listener {@link
     * #CarEvsStatusListenerToService} and forwards it to the client.
     *
     * @param status {@link android.car.evs.CarEvsStatus}
     */
    private void handleServiceStatusChanged(CarEvsStatus status) {
        if (DBG) {
            Slog.d(TAG, "Service state changed: service = " + status.getServiceType() +
                    ", state = " + status.getState());
        }

        final CarEvsStatusListener listener;
        final Executor executor;
        synchronized (mStatusLock) {
            listener = mStatusListener;
            executor = mStatusListenerExecutor;
        }

        if (listener != null) {
            executor.execute(() -> listener.onStatusChanged(status));
        } else if (DBG) {
            Slog.w(TAG, "No client seems active; a received event is ignored.");
        }
    }

    /**
     * Sets {@link #CarEvsStatusListener} object to receive requests to control the activity
     * view and EVS data.
     *
     * @param executor {@link java.util.concurrent.Executor} to execute callbacks.
     * @param listener {@link #CarEvsStatusListener} to register.
     * @throws IllegalStateException if this method is called while a registered status listener
     *         exists.
     */
    @RequiresPermission(Car.PERMISSION_MONITOR_CAR_EVS_STATUS)
    public void setStatusListener(@NonNull @CallbackExecutor Executor executor,
            @NonNull CarEvsStatusListener listener) {
        if (DBG) {
            Slog.d(TAG, "Registering a service monitoring listener.");
        }

        Objects.requireNonNull(listener);
        Objects.requireNonNull(executor);

        if (mStatusListener != null) {
            throw new IllegalStateException("A status listener is already registered.");
        }

        synchronized (mStatusLock) {
            mStatusListener = listener;
            mStatusListenerExecutor = executor;
        }

        try {
            mService.registerStatusListener(mStatusListenerToService);
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        }
    }

    /**
     * Stops getting callbacks to control the camera viewing activity by clearing
     * {@link #CarEvsStatusListener} object.
     */
    @RequiresPermission(Car.PERMISSION_MONITOR_CAR_EVS_STATUS)
    public void clearStatusListener() {
        if (DBG) {
            Slog.d(TAG, "Unregistering a service monitoring callback.");
        }

        synchronized (mStatusLock) {
            mStatusListener = null;
        }

        try{
            mService.unregisterStatusListener(mStatusListenerToService);
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        }
    }

    /**
     * Application registers {@link #CarEvsStreamCallback} object to listen to EVS services' status
     * changes.
     *
     * CarEvsManager supports two client types; one is a System UI type client and another is a
     * normal Android activity type client.  The former client type has a priority over
     * the latter type client and CarEvsManager allows only a single client of each type to
     * subscribe.
     */
    // TODO(b/174572385): Removes below lint suppression
    @SuppressLint("CallbackInterface")
    public interface CarEvsStreamCallback {
        /**
         * Called when any EVS stream events occur.
         *
         * @param event {@link #CarEvsStreamEvent}; e.g. a stream started
         */
        default void onStreamEvent(@CarEvsStreamEvent int event) {}

        /**
         * Called when new frame arrives.
         *
         * @param buffer {@link android.car.evs.CarEvsBufferDescriptor} contains a EVS frame
         */
        default void onNewFrame(@NonNull CarEvsBufferDescriptor buffer) {}
    }

    /**
     * Class implementing the listener interface and gets callbacks from the
     * {@link com.android.car.ICarEvsStreamCallback} across the binder interface.
     */
    private static class CarEvsStreamListenerToService extends ICarEvsStreamCallback.Stub {
        private final WeakReference<CarEvsManager> mManager;

        CarEvsStreamListenerToService(CarEvsManager manager) {
            mManager = new WeakReference<>(manager);
        }

        @Override
        public void onStreamEvent(@CarEvsStreamEvent int event) {
            CarEvsManager manager = mManager.get();
            if (manager != null) {
                manager.handleStreamEvent(event);
            }
        }

        @Override
        public void onNewFrame(CarEvsBufferDescriptor buffer) {
            CarEvsManager manager = mManager.get();
            if (manager != null) {
                manager.handleNewFrame(buffer);
            }
        }
    }

    /**
     * Gets the {@link #CarEvsStreamEvent} from the service listener
     * {@link #CarEvsStreamListenerToService} and dispatches it to an executor provided
     * to the manager.
     *
     * @param event {@link #CarEvsStreamEvent} from the service this manager subscribes to.
     */
    private void handleStreamEvent(@CarEvsStreamEvent int event) {
        if (DBG) {
            Slog.d(TAG, "Received: " + event);
        }

        final CarEvsStreamCallback callback;
        final Executor executor;
        synchronized (mStreamLock) {
            callback = mStreamCallback;
            executor = mStreamCallbackExecutor;
        }

        if (callback != null) {
            executor.execute(() -> callback.onStreamEvent(event));
        } else if (DBG) {
            Slog.w(TAG, "No client seems active; a current stream event is ignored.");
        }
    }

    /**
     * Gets the {@link android.car.evs.CarEvsBufferDescriptor} from the service listener
     * {@link #CarEvsStreamListenerToService} and dispatches it to an executor provided
     * to the manager.
     *
     * @param buffer {@link android.car.evs.CarEvsBufferDescriptor}
     */
    private void handleNewFrame(@NonNull CarEvsBufferDescriptor buffer) {
        Objects.requireNonNull(buffer);
        if (DBG) {
            Slog.d(TAG, "Received a buffer: " + buffer);
        }

        final CarEvsStreamCallback callback;
        final Executor executor;
        synchronized (mStreamLock) {
            callback = mStreamCallback;
            executor = mStreamCallbackExecutor;
        }

        if (callback != null) {
            executor.execute(() -> callback.onNewFrame(buffer));
        } else {
            if (DBG) {
                Slog.w(TAG, "A buffer is being returned back to the service " +
                        "because no active clients exist.");
            }
            returnFrameBuffer(buffer);
        }
    }

    /**
     * Returns a consumed {@link android.car.evs.CarEvsBufferDescriptor}.
     *
     * @param buffer {@link android.car.evs.CarEvsBufferDescriptor} to be returned to
     * the EVS service.
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_EVS_CAMERA)
    public void returnFrameBuffer(@NonNull CarEvsBufferDescriptor buffer) {
        Objects.requireNonNull(buffer);
        try {
            mService.returnFrameBuffer(buffer.getId());
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        }
    }

    /**
     * Requests the system to start an activity for {@link #CarEvsServiceType}.
     *
     * @param type A type of EVS service to start.
     * @return {@link #CarEvsError} to tell the result of the request.
     *         {@link #ERROR_UNAVAILABLE} will be returned if the CarEvsService is not connected to
     *         the native EVS service or the binder transaction fails.
     *         {@link #ERROR_BUSY} will be returned if the CarEvsService is in the
     *         {@link #SERVICE_STATE_REQUESTED} for a different service type.
     *         If the same service type is running, this will return {@link #ERROR_NONE}.
     *         {@link #ERROR_NONE} will be returned for all other cases.
     */
    @RequiresPermission(Car.PERMISSION_REQUEST_CAR_EVS_ACTIVITY)
    public @CarEvsError int startActivity(@CarEvsServiceType int type) {
        try {
            return mService.startActivity(type);
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        }

        return ERROR_UNAVAILABLE;
    }

    /**
     * Requests the system to stop a current activity launched via {@link #startActivity}.
     */
    @RequiresPermission(Car.PERMISSION_REQUEST_CAR_EVS_ACTIVITY)
    public void stopActivity() {
        try {
            mService.stopActivity();
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        }
    }

    /**
     * Requests to start a video stream from {@link #CarEvsServiceType}.
     *
     * @param type A type of EVS service.
     * @param token A session token that is issued to privileged clients.  SystemUI must obtain this
     *        token obtain this via {@link #generateSessionToken} and pass it to the activity, to
     *        prioritize its service requests.
     *        TODO(b/179517136): Defines an Intent extra
     * @param callback {@link #CarEvsStreamCallback} to listen to the stream.
     * @param executor {@link java.util.concurrent.Executor} to run a callback.
     * @return {@link #CarEvsError} to tell the result of the request.
     *         {@link #ERROR_UNAVAILABLE} will be returned if the CarEvsService is not connected to
     *         the native EVS service or the binder transaction fails.
     *         {@link #ERROR_BUSY} will be returned if the CarEvsService is handling a service
     *         request with a valid session token.
     *         {@link #ERROR_NONE} for all other cases.
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_EVS_CAMERA)
    public @CarEvsError int startVideoStream(
            @CarEvsServiceType int type,
            @Nullable IBinder token,
            @NonNull @CallbackExecutor Executor executor,
            @NonNull CarEvsStreamCallback callback) {
        if (DBG) {
            Slog.d(TAG, "Received a request to start a video stream: " + type);
        }

        Objects.requireNonNull(executor);
        Objects.requireNonNull(callback);

        synchronized (mStreamLock) {
            mStreamCallback = callback;
            mStreamCallbackExecutor = executor;
        }

        int status = ERROR_UNAVAILABLE;
        try {
            // Requests the service to start a video stream
            status = mService.startVideoStream(type, token, mStreamListenerToService);
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        } finally {
            return status;
        }
    }

    /**
     * Requests to stop a current {@link #CarEvsServiceType}.
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_EVS_CAMERA)
    public void stopVideoStream() {
        synchronized (mStreamLock) {
            if (mStreamCallback == null) {
                Slog.e(TAG, "The service has not started yet.");
                return;
            }

            // We're not interested in frames and events anymore.  The client can safely assume
            // the service is stopped properly.
            mStreamCallback = null;
            mStreamCallbackExecutor = null;
        }

        try {
            mService.stopVideoStream(mStreamListenerToService);
        } catch (RemoteException err) {
            handleRemoteExceptionFromCarService(err);
        }
    }

    /**
     * Queries the current status of CarEvsService
     *
     * @return {@link android.car.evs.CarEvsStatus} that describes current status of
     * CarEvsService.
     */
    @RequiresPermission(Car.PERMISSION_MONITOR_CAR_EVS_STATUS)
    @NonNull
    public CarEvsStatus getCurrentStatus() {
        try {
            return mService.getCurrentStatus();
        } catch (RemoteException err) {
            Slog.e(TAG, "Failed to read a status of the service.");
            return new CarEvsStatus(SERVICE_TYPE_REARVIEW, SERVICE_STATE_UNAVAILABLE);
        }
    }

    /**
     * Generates a service session token.
     *
     * @return {@link IBinder} object as a service session token.
     */
    @RequiresPermission(Car.PERMISSION_CONTROL_CAR_EVS_ACTIVITY)
    @NonNull
    public IBinder generateSessionToken() {
        IBinder token = null;
        try {
            token =  mService.generateSessionToken();
            if (token == null) {
                token = new Binder();
            }
        } catch (RemoteException err) {
            Slog.e(TAG, "Failed to generate a session token.");
            token = new Binder();
        } finally {
            return token;
        }

    }

    /**
     * Returns whether or not a given service type is supported.
     *
     * @param type {@link CarEvsServiceType} to query
     * @return true if a given service type is available on the system.
     */
    @RequiresPermission(Car.PERMISSION_MONITOR_CAR_EVS_STATUS)
    public boolean isSupported(@CarEvsServiceType int type) {
        try {
            return mService.isSupported(type);
        } catch (RemoteException err) {
            Slog.e(TAG, "Failed to query a service availability");
            return false;
        }
    }
}
