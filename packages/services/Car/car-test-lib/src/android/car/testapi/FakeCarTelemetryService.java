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

package android.car.testapi;

import static android.car.telemetry.CarTelemetryManager.ERROR_NEWER_MANIFEST_EXISTS;
import static android.car.telemetry.CarTelemetryManager.ERROR_NONE;
import static android.car.telemetry.CarTelemetryManager.ERROR_SAME_MANIFEST_EXISTS;

import android.car.telemetry.CarTelemetryManager.AddManifestError;
import android.car.telemetry.ICarTelemetryService;
import android.car.telemetry.ICarTelemetryServiceListener;
import android.car.telemetry.ManifestKey;
import android.os.RemoteException;

import java.util.HashMap;
import java.util.Map;

/**
 * A fake implementation of {@link ICarTelemetryService.Stub} to facilitate the use of
 * {@link android.car.telemetry.CarTelemetryManager} in external unit tests.
 *
 * @hide
 */
public class FakeCarTelemetryService extends ICarTelemetryService.Stub implements
        CarTelemetryController {

    private byte[] mErrorBytes;
    private ICarTelemetryServiceListener mListener;

    private final Map<String, Integer> mNameVersionMap = new HashMap<>();
    private final Map<ManifestKey, byte[]> mManifestMap = new HashMap<>();
    private final Map<ManifestKey, byte[]> mScriptResultMap = new HashMap<>();

    @Override
    public void setListener(ICarTelemetryServiceListener listener) {
        mListener = listener;
    }

    @Override
    public void clearListener() {
        mListener = null;
    }

    @Override
    public @AddManifestError int addManifest(ManifestKey key, byte[] manifest) {
        if (mNameVersionMap.getOrDefault(key.getName(), 0) > key.getVersion()) {
            return ERROR_NEWER_MANIFEST_EXISTS;
        } else if (mNameVersionMap.getOrDefault(key.getName(), 0) == key.getVersion()) {
            return ERROR_SAME_MANIFEST_EXISTS;
        }
        mNameVersionMap.put(key.getName(), key.getVersion());
        mManifestMap.put(key, manifest);
        return ERROR_NONE;
    }

    @Override
    public boolean removeManifest(ManifestKey key) {
        if (!mManifestMap.containsKey(key)) {
            return false;
        }
        mNameVersionMap.remove(key.getName());
        mManifestMap.remove(key);
        return true;
    }

    @Override
    public void removeAllManifests() {
        mNameVersionMap.clear();
        mManifestMap.clear();
    }

    @Override
    public void sendFinishedReports(ManifestKey key) throws RemoteException {
        if (!mScriptResultMap.containsKey(key)) {
            return;
        }
        mListener.onResult(key, mScriptResultMap.get(key));
        mScriptResultMap.remove(key);
    }

    @Override
    public void sendAllFinishedReports() throws RemoteException {
        for (Map.Entry<ManifestKey, byte[]> entry : mScriptResultMap.entrySet()) {
            mListener.onResult(entry.getKey(), entry.getValue());
        }
        mScriptResultMap.clear();
    }

    @Override
    public void sendScriptExecutionErrors() throws RemoteException {
        mListener.onError(mErrorBytes);
    }

    /**************************** CarTelemetryController impl ********************************/
    @Override
    public boolean isListenerSet() {
        return mListener != null;
    }

    @Override
    public int getValidManifestsCount() {
        return mManifestMap.size();
    }

    @Override
    public void addDataForKey(ManifestKey key, byte[] data) {
        mScriptResultMap.put(key, data);
    }

    @Override
    public void setErrorData(byte[] error) {
        mErrorBytes = error;
    }
}
