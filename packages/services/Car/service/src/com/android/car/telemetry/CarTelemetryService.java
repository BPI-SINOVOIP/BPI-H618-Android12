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
package com.android.car.telemetry;

import static android.car.telemetry.CarTelemetryManager.ERROR_NEWER_MANIFEST_EXISTS;
import static android.car.telemetry.CarTelemetryManager.ERROR_NONE;
import static android.car.telemetry.CarTelemetryManager.ERROR_PARSE_MANIFEST_FAILED;
import static android.car.telemetry.CarTelemetryManager.ERROR_SAME_MANIFEST_EXISTS;

import android.annotation.NonNull;
import android.car.Car;
import android.car.telemetry.CarTelemetryManager.AddManifestError;
import android.car.telemetry.ICarTelemetryService;
import android.car.telemetry.ICarTelemetryServiceListener;
import android.car.telemetry.ManifestKey;
import android.content.Context;
import android.util.IndentingPrintWriter;
import android.util.Slog;

import com.android.car.CarServiceBase;
import com.android.internal.annotations.GuardedBy;

import com.google.protobuf.InvalidProtocolBufferException;

import java.util.HashMap;
import java.util.Map;

/**
 * CarTelemetryService manages OEM telemetry collection, processing and communication
 * with a data upload service.
 */
public class CarTelemetryService extends ICarTelemetryService.Stub implements CarServiceBase {

    // TODO(b/189340793): Rename Manifest to MetricsConfig

    private static final boolean DEBUG = false;
    private static final int DEFAULT_VERSION = 0;
    private static final String TAG = CarTelemetryService.class.getSimpleName();

    private final Context mContext;
    @GuardedBy("mLock")
    private final Map<String, Integer> mNameVersionMap = new HashMap<>();
    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private ICarTelemetryServiceListener mListener;

    public CarTelemetryService(Context context) {
        mContext = context;
    }

    @Override
    public void init() {
        // nothing to do
    }

    @Override
    public void release() {
        // nothing to do
    }

    @Override
    public void dump(IndentingPrintWriter writer) {
        writer.println("Car Telemetry service");
    }

    /**
     * Registers a listener with CarTelemetryService for the service to send data to cloud app.
     */
    @Override
    public void setListener(@NonNull ICarTelemetryServiceListener listener) {
        // TODO(b/184890506): verify that only a hardcoded app can set the listener
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        synchronized (mLock) {
            setListenerLocked(listener);
        }
    }

    /**
     * Clears the listener registered with CarTelemetryService.
     */
    @Override
    public void clearListener() {
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        synchronized (mLock) {
            clearListenerLocked();
        }
    }

    /**
     * Allows client to send telemetry manifests.
     *
     * @param key    the unique key to identify the manifest.
     * @param config the serialized bytes of a Manifest object.
     * @return {@link AddManifestError} the error code.
     */
    @Override
    public @AddManifestError int addManifest(@NonNull ManifestKey key, @NonNull byte[] config) {
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        synchronized (mLock) {
            return addManifestLocked(key, config);
        }
    }

    /**
     * Removes a manifest based on the key.
     */
    @Override
    public boolean removeManifest(@NonNull ManifestKey key) {
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        synchronized (mLock) {
            return removeManifestLocked(key);
        }
    }

    /**
     * Removes all manifests.
     */
    @Override
    public void removeAllManifests() {
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        synchronized (mLock) {
            removeAllManifestsLocked();
        }
    }

    /**
     * Sends script results associated with the given key using the
     * {@link ICarTelemetryServiceListener}.
     */
    @Override
    public void sendFinishedReports(@NonNull ManifestKey key) {
        // TODO(b/184087869): Implement
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        if (DEBUG) {
            Slog.d(TAG, "Flushing reports for a manifest");
        }
    }

    /**
     * Sends all script results associated using the {@link ICarTelemetryServiceListener}.
     */
    @Override
    public void sendAllFinishedReports() {
        // TODO(b/184087869): Implement
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        if (DEBUG) {
            Slog.d(TAG, "Flushing all reports");
        }
    }

    /**
     * Sends all errors using the {@link ICarTelemetryServiceListener}.
     */
    @Override
    public void sendScriptExecutionErrors() {
        // TODO(b/184087869): Implement
        mContext.enforceCallingOrSelfPermission(
                Car.PERMISSION_USE_CAR_TELEMETRY_SERVICE, "setListener");
        if (DEBUG) {
            Slog.d(TAG, "Flushing script execution errors");
        }
    }

    @GuardedBy("mLock")
    private void setListenerLocked(@NonNull ICarTelemetryServiceListener listener) {
        if (DEBUG) {
            Slog.d(TAG, "Setting the listener for car telemetry service");
        }
        mListener = listener;
    }

    @GuardedBy("mLock")
    private void clearListenerLocked() {
        if (DEBUG) {
            Slog.d(TAG, "Clearing listener");
        }
        mListener = null;
    }

    @GuardedBy("mLock")
    private @AddManifestError int addManifestLocked(ManifestKey key, byte[] configProto) {
        if (DEBUG) {
            Slog.d(TAG, "Adding MetricsConfig to car telemetry service");
        }
        int currentVersion = mNameVersionMap.getOrDefault(key.getName(), DEFAULT_VERSION);
        if (currentVersion > key.getVersion()) {
            return ERROR_NEWER_MANIFEST_EXISTS;
        } else if (currentVersion == key.getVersion()) {
            return ERROR_SAME_MANIFEST_EXISTS;
        }

        TelemetryProto.MetricsConfig metricsConfig;
        try {
            metricsConfig = TelemetryProto.MetricsConfig.parseFrom(configProto);
        } catch (InvalidProtocolBufferException e) {
            Slog.e(TAG, "Failed to parse MetricsConfig.", e);
            return ERROR_PARSE_MANIFEST_FAILED;
        }
        mNameVersionMap.put(key.getName(), key.getVersion());

        // TODO(b/186047142): Store the MetricsConfig to disk
        // TODO(b/186047142): Send metricsConfig to a script manager or a queue
        return ERROR_NONE;
    }

    @GuardedBy("mLock")
    private boolean removeManifestLocked(@NonNull ManifestKey key) {
        Integer version = mNameVersionMap.remove(key.getName());
        if (version == null) {
            return false;
        }
        // TODO(b/186047142): Delete manifest from disk and remove it from queue
        return true;
    }

    @GuardedBy("mLock")
    private void removeAllManifestsLocked() {
        if (DEBUG) {
            Slog.d(TAG, "Removing all manifest from car telemetry service");
        }
        mNameVersionMap.clear();
        // TODO(b/186047142): Delete all manifests from disk & queue
    }
}
