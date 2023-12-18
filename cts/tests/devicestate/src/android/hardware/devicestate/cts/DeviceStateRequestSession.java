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

package android.hardware.devicestate.cts;

import static android.hardware.devicestate.cts.DeviceStateUtils.runWithControlDeviceStatePermission;

import android.hardware.devicestate.DeviceStateManager;
import android.hardware.devicestate.DeviceStateRequest;
import androidx.annotation.NonNull;

/**
 * An implementation of {@link java.lang.AutoCloseable} that submits a request to override the
 * device state using the provided {@link DeviceStateRequest} and automatically cancels the request
 * on a call to {@link #close()}.
 */
public final class DeviceStateRequestSession implements AutoCloseable {
    @NonNull
    private final DeviceStateManager mDeviceStateManager;
    @NonNull
    private final DeviceStateRequest mRequest;
    @NonNull
    private DeviceStateRequest.Callback mCallback;

    public DeviceStateRequestSession(@NonNull DeviceStateManager manager,
            @NonNull DeviceStateRequest request, @NonNull DeviceStateRequest.Callback callback) {
        mDeviceStateManager = manager;
        mRequest = request;
        mCallback = callback;

        submitRequest(request);
    }

    private void submitRequest(@NonNull DeviceStateRequest request) {
        try {
            runWithControlDeviceStatePermission(() ->
                    mDeviceStateManager.requestState(mRequest, Runnable::run, mCallback));
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    @Override
    public void close() {
        try {
            runWithControlDeviceStatePermission(() ->
                    mDeviceStateManager.cancelRequest(mRequest));
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }
}
