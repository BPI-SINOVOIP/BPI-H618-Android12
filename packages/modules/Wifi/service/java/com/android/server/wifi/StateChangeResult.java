/*
 * Copyright (C) 2011 The Android Open Source Project
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
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiSsid;

import java.util.Objects;

/**
 * Stores supplicant state change information passed from WifiMonitor to
 * a state machine. ClientModeImpl, SupplicantStateTracker and WpsStateMachine
 * are example state machines that handle it.
 */
public class StateChangeResult {
    StateChangeResult(int networkId, @NonNull WifiSsid wifiSsid, @NonNull String bssid,
            SupplicantState state) {
        this.state = state;
        this.wifiSsid = Objects.requireNonNull(wifiSsid);
        this.bssid = Objects.requireNonNull(bssid);
        this.networkId = networkId;
    }

    public final int networkId;
    @NonNull public final WifiSsid wifiSsid;
    @NonNull public final String bssid;
    public final SupplicantState state;

    @Override
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append(" ssid: ").append(wifiSsid);
        sb.append(" bssid: ").append(bssid);
        sb.append(" nid: ").append(networkId);
        sb.append(" state: ").append(state);
        return sb.toString();
    }
}
