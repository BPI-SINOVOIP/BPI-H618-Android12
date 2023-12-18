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

package com.android.tv.settings.service;

public final class ServiceUtil {
    public static final int STATE_NETWORK_MAIN = 0;
    public static final int STATE_WIFI_DETAILS = 1;

    public static final String INFO_INTENT = "intent";
    public static final String INFO_NEXT_STATE = "next_state";
    public static final String INFO_WIFI_SIGNAL_LEVEL = "wifi_signal_level";
    public static final String INFO_COLLAPSE = "collapse";

    private ServiceUtil() {}

    static byte getChecked(boolean checked) {
        return checked ? (byte) 2 : (byte) 1;
    }

    static byte getVisible(boolean visible) {
        return visible ? (byte) 2 : (byte) 1;
    }

    static boolean isChecked(PreferenceParcelable pref) {
        return pref.getChecked() == 2 ? true : false;
    }

    static boolean isVisible(PreferenceParcelable pref) {
        return pref.getVisible() == 2 ? true : false;
    }
}
