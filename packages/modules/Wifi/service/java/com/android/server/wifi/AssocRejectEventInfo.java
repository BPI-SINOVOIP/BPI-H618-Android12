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

package com.android.server.wifi;

import android.annotation.NonNull;
import android.hardware.wifi.supplicant.V1_4.ISupplicantStaIfaceCallback.AssociationRejectionData;

import com.android.server.wifi.util.NativeUtil;

import java.util.Objects;

/**
 * Stores assoc reject information passed from WifiMonitor.
 */
public class AssocRejectEventInfo {
    @NonNull public final String ssid;
    @NonNull public final String bssid;
    public final int statusCode;
    public final boolean timedOut;
    public final MboOceController.OceRssiBasedAssocRejectAttr oceRssiBasedAssocRejectInfo;
    public final MboOceController.MboAssocDisallowedAttr mboAssocDisallowedInfo;

    public AssocRejectEventInfo(@NonNull String ssid, @NonNull String bssid, int statusCode,
            boolean timedOut) {
        this.ssid = Objects.requireNonNull(ssid);
        this.bssid = Objects.requireNonNull(bssid);
        this.statusCode = statusCode;
        this.timedOut = timedOut;
        this.oceRssiBasedAssocRejectInfo = null;
        this.mboAssocDisallowedInfo = null;
    }

    public AssocRejectEventInfo(AssociationRejectionData assocRejectData) {
        String ssid = NativeUtil.encodeSsid(assocRejectData.ssid);
        String bssid = NativeUtil.macAddressFromByteArray(assocRejectData.bssid);
        this.ssid = Objects.requireNonNull(ssid);
        this.bssid = Objects.requireNonNull(bssid);
        this.statusCode = assocRejectData.statusCode;
        this.timedOut = assocRejectData.timedOut;
        if (assocRejectData.isMboAssocDisallowedReasonCodePresent) {
            this.mboAssocDisallowedInfo = new MboOceController.MboAssocDisallowedAttr(
                    assocRejectData.mboAssocDisallowedReason);
        } else {
            this.mboAssocDisallowedInfo = null;
        }
        if (assocRejectData.isOceRssiBasedAssocRejectAttrPresent) {
            this.oceRssiBasedAssocRejectInfo =
                    new MboOceController.OceRssiBasedAssocRejectAttr(
                            assocRejectData.oceRssiBasedAssocRejectData.deltaRssi,
                            assocRejectData.oceRssiBasedAssocRejectData.retryDelayS);
        } else {
            this.oceRssiBasedAssocRejectInfo = null;
        }
    }

    @Override
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append(" ssid: ").append(ssid);
        sb.append(" bssid: ").append(bssid);
        sb.append(" statusCode: ").append(statusCode);
        sb.append(" timedOut: ").append(timedOut);
        sb.append(" oceRssiBasedAssocRejectInfo: ").append(oceRssiBasedAssocRejectInfo);
        sb.append(" mboAssocDisallowedInfo: ").append(mboAssocDisallowedInfo);

        return sb.toString();
    }
}
