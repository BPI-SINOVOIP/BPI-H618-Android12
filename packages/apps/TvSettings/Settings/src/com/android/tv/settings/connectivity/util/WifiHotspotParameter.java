/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.util;

import androidx.lifecycle.ViewModelProviders;
import android.net.ProxyInfo;
import android.os.ServiceManager;
import androidx.fragment.app.FragmentActivity;
import android.text.TextUtils;
import android.content.Context;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.setup.UserChoiceInfo;
import com.android.tv.settings.connectivity.HotspotFragment;

import android.util.Log;

/**
 * Class that helps process wifi hotspot setup.
 */
public class WifiHotspotParameter {
    private static final int SSID_ASCII_MIN_LENGTH = 1;
    private static final int SSID_ASCII_MAX_LENGTH = 32;
    private static final int PASSWORD_MIN_LENGTH = 8;
    private static final int PASSWORD_MAX_LENGTH = 63;
    public  static String hotspotname = "";
    public  static String hotspotpassword = "";
    private static int securityvalue = -1;
    private static int apbandvalue = -1;
    private static final String TAG = "WifiHotspotParameter";

    public static int setHotspotName(FragmentActivity activity) {
        UserChoiceInfo mUserChoiceInfo = ViewModelProviders
                .of(activity)
                .get(UserChoiceInfo.class);
        if (!isTextValid(mUserChoiceInfo.get(UserChoiceInfo.HOTSPOTNAME))) {
            return -1;
        }
        hotspotname = mUserChoiceInfo.get(UserChoiceInfo.HOTSPOTNAME);
        HotspotFragment.setHotspotNameSummary(hotspotname);
        return 0;
    }

    public static String getNewHotspotName() {
        return hotspotname;
    }

    public static int setHotspotPassword(FragmentActivity activity) {
        UserChoiceInfo mUserChoiceInfo = ViewModelProviders
                .of(activity)
                .get(UserChoiceInfo.class);
        if (!isHotspotPasswordValid(mUserChoiceInfo.get(UserChoiceInfo.HOTSPOTPASSWORD))) {
            return -1;
        }
        hotspotpassword = mUserChoiceInfo.get(UserChoiceInfo.HOTSPOTPASSWORD);
        HotspotFragment.setHotspotPasswordSummary(hotspotpassword);
        return 0;
    }

    public static String getNewHotspotPassword() {
        return hotspotpassword;
    }

    public static void setSecurityType(int typevalue) {
        securityvalue = typevalue;
        HotspotFragment.setHotspotSecuritySummary(typevalue);
    }

    public static int getSecurityType() {
        return securityvalue;
    }

    public static void setApbnadType(int typevalue) {
        apbandvalue = typevalue;
        HotspotFragment.setHotspotApbandSummary(typevalue);
    }

    public static int getApbandType() {
        return apbandvalue;
    }

    public static boolean isSSIDTooLong(String ssid) {
        if (TextUtils.isEmpty(ssid)) {
            return false;
        }
        return ssid.length() > SSID_ASCII_MAX_LENGTH;
    }

    public static boolean isSSIDTooShort(String ssid) {
        if (TextUtils.isEmpty(ssid)) {
            return true;
        }
        return ssid.length() < SSID_ASCII_MIN_LENGTH;
    }

    public static boolean isTextValid(String value) {
        return !isSSIDTooLong(value) && !isSSIDTooShort(value);
    }

    public static boolean isHotspotPasswordValid(String password) {
        if (TextUtils.isEmpty(password)) {
            return false;
        }
        final int length = password.length();
        return length >= PASSWORD_MIN_LENGTH && length <= PASSWORD_MAX_LENGTH;
    }
}
