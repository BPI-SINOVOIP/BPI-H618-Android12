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

package com.android.networkstack.apishim.api29;

import android.content.Context;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.NetworkRequest;
import android.os.Handler;

import androidx.annotation.NonNull;

import com.android.networkstack.apishim.common.ConnectivityManagerShim;
import com.android.networkstack.apishim.common.UnsupportedApiLevelException;

/**
 * Implementation of {@link ConnectivityManagerShim} for API 29.
 */
public class ConnectivityManagerShimImpl implements ConnectivityManagerShim {
    protected ConnectivityManagerShimImpl(Context context) {}

    /**
     * Get a new instance of {@link ConnectivityManagerShim}.
     */
    public static ConnectivityManagerShim newInstance(Context context) {
        return new ConnectivityManagerShimImpl(context);
    }
    /**
     * See android.net.ConnectivityManager#requestBackgroundNetwork
     * @throws UnsupportedApiLevelException if API is not available in this API level.
     */
    @Override
    public void requestBackgroundNetwork(@NonNull NetworkRequest request,
            @NonNull NetworkCallback networkCallback, @NonNull Handler handler)
            throws UnsupportedApiLevelException {
        // Not supported for API 29.
        throw new UnsupportedApiLevelException("Not supported in API 29.");
    }

    /**
     * See android.net.ConnectivityManager#registerSystemDefaultNetworkCallback
     * @throws UnsupportedApiLevelException if API is not available in this API level.
     */
    @Override
    public void registerSystemDefaultNetworkCallback(@NonNull NetworkCallback networkCallback,
            @NonNull Handler handler) throws UnsupportedApiLevelException {
        // Not supported for API 29.
        throw new UnsupportedApiLevelException("Not supported in API 29.");
    }
}
