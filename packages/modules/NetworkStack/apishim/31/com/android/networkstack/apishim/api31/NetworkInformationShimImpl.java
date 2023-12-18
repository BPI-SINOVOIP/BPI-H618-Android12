/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.networkstack.apishim.api31;

import android.net.LinkProperties;
import android.net.NetworkCapabilities;
import android.os.Build;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.annotation.VisibleForTesting;

import com.android.networkstack.apishim.common.CaptivePortalDataShim;
import com.android.networkstack.apishim.common.NetworkInformationShim;
import com.android.networkstack.apishim.common.ShimUtils;

/**
 * Compatibility implementation of {@link NetworkInformationShim}.
 */
public class NetworkInformationShimImpl
        extends com.android.networkstack.apishim.api30.NetworkInformationShimImpl {
    protected NetworkInformationShimImpl() {}

    /**
     * Indicates whether the shim can use APIs above the R SDK.
     */
    @VisibleForTesting
    public static boolean useApiAboveR() {
        return ShimUtils.isReleaseOrDevelopmentApiAbove(Build.VERSION_CODES.R);
    }

    /**
     * Get a new instance of {@link NetworkInformationShim}.
     */
    public static NetworkInformationShim newInstance() {
        if (!useApiAboveR()) {
            return com.android.networkstack.apishim.api30.NetworkInformationShimImpl.newInstance();
        }
        return new NetworkInformationShimImpl();
    }

    @Nullable
    @Override
    public CaptivePortalDataShim getCaptivePortalData(@Nullable LinkProperties lp) {
        if (lp == null || lp.getCaptivePortalData() == null) return null;
        return new CaptivePortalDataShimImpl(lp.getCaptivePortalData());
    }

    @RequiresApi(Build.VERSION_CODES.S)
    @Nullable
    @Override
    public String getCapabilityCarrierName(int capability) {
        return NetworkCapabilities.getCapabilityCarrierName(capability);
    }
}
