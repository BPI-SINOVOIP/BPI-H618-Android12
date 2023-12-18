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

package android.car.telemetry;

import android.annotation.CallbackExecutor;
import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.RequiresPermission;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.annotation.RequiredFeature;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Slog;

import com.android.internal.annotations.GuardedBy;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.util.concurrent.Executor;

/**
 * Provides an application interface for interacting with the Car Telemetry Service.
 *
 * @hide
 */
@RequiredFeature(Car.CAR_TELEMETRY_SERVICE)
public final class CarTelemetryManager extends CarManagerBase {

    private static final boolean DEBUG = false;
    private static final String TAG = CarTelemetryManager.class.getSimpleName();
    private static final int MANIFEST_MAX_SIZE_BYTES = 10 * 1024; // 10 kb

    private final CarTelemetryServiceListener mCarTelemetryServiceListener =
            new CarTelemetryServiceListener(this);
    private final ICarTelemetryService mService;
    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private CarTelemetryResultsListener mResultsListener;
    @GuardedBy("mLock")
    private Executor mExecutor;

    /**
     * Status to indicate that manifest was added successfully.
     */
    public static final int ERROR_NONE = 0;

    /**
     * Status to indicate that add manifest failed because the same manifest based on the
     * ManifestKey already exists.
     */
    public static final int ERROR_SAME_MANIFEST_EXISTS = 1;

    /**
     * Status to indicate that add manifest failed because a newer version of the manifest exists.
     */
    public static final int ERROR_NEWER_MANIFEST_EXISTS = 2;

    /**
     * Status to indicate that add manifest failed because CarTelemetryService is unable to parse
     * the given byte array into a Manifest.
     */
    public static final int ERROR_PARSE_MANIFEST_FAILED = 3;

    /**
     * Status to indicate that add manifest failed because of failure to verify the signature of
     * the manifest.
     */
    public static final int ERROR_SIGNATURE_VERIFICATION_FAILED = 4;

    /**
     * Status to indicate that add manifest failed because of a general error in cars.
     */
    public static final int ERROR_UNKNOWN = 5;

    /** @hide */
    @IntDef(prefix = {"ERROR_"}, value = {
            ERROR_NONE,
            ERROR_SAME_MANIFEST_EXISTS,
            ERROR_NEWER_MANIFEST_EXISTS,
            ERROR_PARSE_MANIFEST_FAILED,
            ERROR_SIGNATURE_VERIFICATION_FAILED,
            ERROR_UNKNOWN
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface AddManifestError {}

    /**
     * Application registers {@link CarTelemetryResultsListener} object to receive data from
     * {@link com.android.car.telemetry.CarTelemetryService}.
     *
     * @hide
     */
    public interface CarTelemetryResultsListener {
        /**
         * Called by {@link com.android.car.telemetry.CarTelemetryService} to send script result to
         * the client.
         * TODO(b/184964661): Publish the documentation for the format of the results.
         *
         * @param key the {@link ManifestKey} that the result is associated with.
         * @param result the serialized car telemetry result.
         */
        void onResult(@NonNull ManifestKey key, @NonNull byte[] result);

        /**
         * Called by {@link com.android.car.telemetry.CarTelemetryService} to send error message to
         * the client.
         *
         * @param error the serialized car telemetry error.
         */
        void onError(@NonNull byte[] error);
    }

    /**
     * Class implementing the listener interface
     * {@link com.android.car.ICarTelemetryServiceListener} to receive telemetry results.
     */
    private static final class CarTelemetryServiceListener
            extends ICarTelemetryServiceListener.Stub {
        private WeakReference<CarTelemetryManager> mManager;

        private CarTelemetryServiceListener(CarTelemetryManager manager) {
            mManager = new WeakReference<>(manager);
        }

        @Override
        public void onResult(@NonNull ManifestKey key, @NonNull byte[] result) {
            CarTelemetryManager manager = mManager.get();
            if (manager == null) {
                return;
            }
            manager.onResult(key, result);
        }

        @Override
        public void onError(@NonNull byte[] error) {
            CarTelemetryManager manager = mManager.get();
            if (manager == null) {
                return;
            }
            manager.onError(error);
        }
    }

    private void onResult(ManifestKey key, byte[] result) {
        long token = Binder.clearCallingIdentity();
        synchronized (mLock) {
            mExecutor.execute(() -> mResultsListener.onResult(key, result));
        }
        Binder.restoreCallingIdentity(token);
    }

    private void onError(byte[] error) {
        long token = Binder.clearCallingIdentity();
        synchronized (mLock) {
            mExecutor.execute(() -> mResultsListener.onError(error));
        }
        Binder.restoreCallingIdentity(token);
    }

    /**
     * Gets an instance of CarTelemetryManager.
     *
     * CarTelemetryManager manages {@link com.android.car.telemetry.CarTelemetryService} and
     * provides APIs so the client can use the car telemetry service.
     *
     * There is only one client to this manager, which is OEM's cloud application. It uses the
     * APIs to send config to and receive data from CarTelemetryService.
     *
     * @hide
     */
    public CarTelemetryManager(Car car, IBinder service) {
        super(car);
        mService = ICarTelemetryService.Stub.asInterface(service);
        if (DEBUG) {
            Slog.d(TAG, "starting car telemetry manager");
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        synchronized (mLock) {
            mResultsListener = null;
            mExecutor = null;
        }
    }

    /**
     * Registers a listener with {@link com.android.car.telemetry.CarTelemetryService} for client
     * to receive script execution results.
     *
     * @param listener to received data from {@link com.android.car.telemetry.CarTelemetryService}.
     * @throws IllegalStateException if the listener is already set.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public void setListener(@NonNull @CallbackExecutor Executor executor,
            @NonNull CarTelemetryResultsListener listener) {
        synchronized (mLock) {
            if (mResultsListener != null) {
                throw new IllegalStateException(
                        "Attempting to set a listener that is already set.");
            }
            mExecutor = executor;
            mResultsListener = listener;
        }
        try {
            mService.setListener(mCarTelemetryServiceListener);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Unregisters the listener from {@link com.android.car.telemetry.CarTelemetryService}.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public void clearListener() {
        synchronized (mLock) {
            mResultsListener = null;
            mExecutor = null;
        }
        try {
            mService.clearListener();
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Called by client to send telemetry manifest. The size of the manifest cannot exceed a
     * predefined size. Otherwise an exception is thrown.
     * The {@link ManifestKey} is used to uniquely identify a manifest. If a manifest of the same
     * name already exists in {@link com.android.car.telemetry.CarTelemetryService}, then the
     * version will be compared. If the version is strictly higher, the existing manifest will be
     * replaced by the new one. All cache and intermediate results will be cleared if replaced.
     * TODO(b/185420981): Update javadoc after CarTelemetryService has concrete implementation.
     *
     * @param key      the unique key to identify the manifest.
     * @param manifest the serialized bytes of a Manifest object.
     * @return {@link #AddManifestError} to tell the result of the request.
     * @throws IllegalArgumentException if the manifest size exceeds limit.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public @AddManifestError int addManifest(@NonNull ManifestKey key, @NonNull byte[] manifest) {
        if (manifest.length > MANIFEST_MAX_SIZE_BYTES) {
            throw new IllegalArgumentException("Manifest size exceeds limit.");
        }
        try {
            return mService.addManifest(key, manifest);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
        return ERROR_UNKNOWN;
    }

    /**
     * Removes a manifest from {@link com.android.car.telemetry.CarTelemetryService}. If the
     * manifest does not exist, nothing will be removed but the status will be indicated in the
     * return value.
     *
     * @param key the unique key to identify the manifest. Name and version must be exact.
     * @return true for success, false otherwise.
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public boolean removeManifest(@NonNull ManifestKey key) {
        try {
            return mService.removeManifest(key);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
        return false;
    }

    /**
     * Removes all manifests from {@link com.android.car.telemetry.CarTelemetryService}.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public void removeAllManifests() {
        try {
            mService.removeAllManifests();
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * An asynchronous API for the client to get script execution results of a specific manifest
     * from the {@link com.android.car.telemetry.CarTelemetryService} through the listener.
     * This call is destructive. The returned results will be deleted from CarTelemetryService.
     *
     * @param key the unique key to identify the manifest.
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public void sendFinishedReports(@NonNull ManifestKey key) {
        try {
            mService.sendFinishedReports(key);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * An asynchronous API for the client to get all script execution results
     * from the {@link com.android.car.telemetry.CarTelemetryService} through the listener.
     * This call is destructive. The returned results will be deleted from CarTelemetryService.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public void sendAllFinishedReports() {
        try {
            mService.sendAllFinishedReports();
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * An asynchronous API for the client to get all script execution errors
     * from the {@link com.android.car.telemetry.CarTelemetryService} through the listener.
     * This call is destructive. The returned results will be deleted from CarTelemetryService.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE)
    public void sendScriptExecutionErrors() {
        try {
            mService.sendScriptExecutionErrors();
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }
}
