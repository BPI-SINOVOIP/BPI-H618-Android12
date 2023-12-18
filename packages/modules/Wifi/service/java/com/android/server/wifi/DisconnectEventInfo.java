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

import java.util.Objects;

/**
 * Stores disconnect information passed from WifiMonitor.
 */
public class DisconnectEventInfo {
    @NonNull public final String ssid;
    @NonNull public final String bssid;
    public final int reasonCode;
    public final boolean locallyGenerated;

    public DisconnectEventInfo(@NonNull String ssid, @NonNull String bssid, int reasonCode,
            boolean locallyGenerated) {
        this.ssid = Objects.requireNonNull(ssid);
        this.bssid = Objects.requireNonNull(bssid);
        this.reasonCode = reasonCode;
        this.locallyGenerated = locallyGenerated;
    }

    @Override
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append(" ssid: ").append(ssid);
        sb.append(" bssid: ").append(bssid);
        sb.append(" reasonCode: ").append(reasonCode);
        sb.append(" locallyGenerated: ").append(locallyGenerated);
        return sb.toString();
    }
}
