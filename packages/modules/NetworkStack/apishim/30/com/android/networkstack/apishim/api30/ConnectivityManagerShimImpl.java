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

package com.android.networkstack.apishim.api30;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Build;
import android.os.Handler;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import com.android.networkstack.apishim.common.ConnectivityManagerShim;
import com.android.networkstack.apishim.common.ShimUtils;
import com.android.networkstack.apishim.common.UnsupportedApiLevelException;

/**
 * Implementation of {@link ConnectivityManagerShim} for API 30.
 */
public class ConnectivityManagerShimImpl
        extends com.android.networkstack.apishim.api29.ConnectivityManagerShimImpl {
    protected final ConnectivityManager mCm;
    protected ConnectivityManagerShimImpl(Context context) {
        super(context);
        mCm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    /**
     * Get a new instance of {@link ConnectivityManagerShim}.
     */
    public static ConnectivityManagerShim newInstance(Context context) {
        if (!ShimUtils.isReleaseOrDevelopmentApiAbove(Build.VERSION_CODES.Q)) {
            return com.android.networkstack.apishim.api29.ConnectivityManagerShimImpl
                    .newInstance(context);
        }
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
        // Not supported for API 30.
        throw new UnsupportedApiLevelException("Not supported in API 30.");
    }

    /**
     * See android.net.ConnectivityManager#registerSystemDefaultNetworkCallback
     * @throws UnsupportedApiLevelException if API is not available in this API level.
     */
    @Override
    @RequiresApi(Build.VERSION_CODES.R)
    public void registerSystemDefaultNetworkCallback(@NonNull NetworkCallback networkCallback,
            @NonNull Handler handler) {
        // defaultNetworkRequest is not really a "request", just a way of tracking the system
        // default network. It's guaranteed not to actually bring up any networks because it
        // should be the same request as the ConnectivityService default request, and thus
        // shares fate with it.  In API <= R, registerSystemDefaultNetworkCallback is not
        // available, and registerDefaultNetworkCallback will not track the system default when
        // a VPN applies to the UID of this process.
        final NetworkRequest defaultNetworkRequest = new NetworkRequest.Builder()
                .clearCapabilities()
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_TRUSTED)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build();
        mCm.requestNetwork(defaultNetworkRequest, networkCallback, handler);
    }
}
