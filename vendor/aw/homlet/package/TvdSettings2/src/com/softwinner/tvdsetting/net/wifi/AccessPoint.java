/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.softwinner.tvdsetting.net.wifi;

import com.softwinner.tvdsetting.R;

import android.content.Context;
import android.net.NetworkInfo.DetailedState;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;
import android.os.Handler;
import android.os.Parcelable;
import android.os.Parcel;
import android.widget.TextView;
import android.app.Activity;

class AccessPoint implements Parcelable, Comparable<AccessPoint> {
    static final String TAG = "Settings.AccessPoint";

    private static final String KEY_DETAILEDSTATE = "key_detailedstate";
    private static final String KEY_WIFIINFO = "key_wifiinfo";
    private static final String KEY_SCANRESULT = "key_scanresult";
    private static final String KEY_CONFIG = "key_config";

    private static final int[] STATE_SECURED = {
        R.attr.state_encrypted
    };
    private static final int[] STATE_NONE = {};

    /** These values are matched in string arrays -- changes must be kept in sync */
    static final int SECURITY_NONE = 0;
    static final int SECURITY_WEP = 1;
    static final int SECURITY_PSK = 2;
    static final int SECURITY_EAP = 3;

    enum PskType {
        UNKNOWN,
        WPA,
        WPA2,
        WPA_WPA2
    }

    String ssid;
    String bssid;
    int security;
    int networkId;
    boolean wpsAvailable = false;

    PskType pskType = PskType.UNKNOWN;

    private WifiConfiguration mConfig;
    /* package */ScanResult mScanResult;

    private int mRssi;
    private WifiInfo mInfo;
    private DetailedState mState;
    private Context mContext;

    private View mView;
    private Handler mHandler = new Handler();
    public void setView(View view){
        mView = view;
    }

    static int getSecurity(WifiConfiguration config) {
        if (config.allowedKeyManagement.get(KeyMgmt.WPA_PSK)) {
            return SECURITY_PSK;
        }
        if (config.allowedKeyManagement.get(KeyMgmt.WPA_EAP) ||
                config.allowedKeyManagement.get(KeyMgmt.IEEE8021X)) {
            return SECURITY_EAP;
        }
        return (config.wepKeys[0] != null) ? SECURITY_WEP : SECURITY_NONE;
    }

    private static int getSecurity(ScanResult result) {
        if (result.capabilities.contains("WEP")) {
            return SECURITY_WEP;
        } else if (result.capabilities.contains("PSK")) {
            return SECURITY_PSK;
        } else if (result.capabilities.contains("EAP")) {
            return SECURITY_EAP;
        }
        return SECURITY_NONE;
    }

    public String getSecurityString(boolean concise) {
        switch(security) {
            case SECURITY_EAP:
                return concise ? mContext.getString(R.string.wifi_security_short_eap) :
                    mContext.getString(R.string.wifi_security_eap);
            case SECURITY_PSK:
                switch (pskType) {
                    case WPA:
                        return concise ? mContext.getString(R.string.wifi_security_short_wpa) :
                            mContext.getString(R.string.wifi_security_wpa);
                    case WPA2:
                        return concise ? mContext.getString(R.string.wifi_security_short_wpa2) :
                            mContext.getString(R.string.wifi_security_wpa2);
                    case WPA_WPA2:
                        return concise ? mContext.getString(R.string.wifi_security_short_wpa_wpa2) :
                            mContext.getString(R.string.wifi_security_wpa_wpa2);
                    case UNKNOWN:
                    default:
                        return concise ? mContext.getString(R.string.wifi_security_short_psk_generic)
                                : mContext.getString(R.string.wifi_security_psk_generic);
                }
            case SECURITY_WEP:
                return concise ? mContext.getString(R.string.wifi_security_short_wep) :
                    mContext.getString(R.string.wifi_security_wep);
            case SECURITY_NONE:
            default:
                return concise ? "" : mContext.getString(R.string.wifi_security_none);
        }
    }

    private static PskType getPskType(ScanResult result) {
        boolean wpa = result.capabilities.contains("WPA-PSK");
        boolean wpa2 = result.capabilities.contains("WPA2-PSK");
        if (wpa2 && wpa) {
            return PskType.WPA_WPA2;
        } else if (wpa2) {
            return PskType.WPA2;
        } else if (wpa) {
            return PskType.WPA;
        } else {
            Log.w(TAG, "Received abnormal flag string: " + result.capabilities);
            return PskType.UNKNOWN;
        }
    }

    AccessPoint() {
    }

    AccessPoint(Context context, WifiConfiguration config) {
        mContext = context;
        loadConfig(config);
    }

    AccessPoint(Context context, ScanResult result) {
        mContext = context;
        loadResult(result);
    }

    AccessPoint(Context context, Bundle savedState) {
        mContext = context;
        mConfig = savedState.getParcelable(KEY_CONFIG);
        if (mConfig != null) {
            loadConfig(mConfig);
        }
        mScanResult = (ScanResult) savedState.getParcelable(KEY_SCANRESULT);
        if (mScanResult != null) {
            loadResult(mScanResult);
        }
        mInfo = (WifiInfo) savedState.getParcelable(KEY_WIFIINFO);
        if (savedState.containsKey(KEY_DETAILEDSTATE)) {
            mState = DetailedState.valueOf(savedState.getString(KEY_DETAILEDSTATE));
        }
        update(mInfo, mState);
    }

    public void saveWifiState(Bundle savedState) {
        savedState.putParcelable(KEY_CONFIG, mConfig);
        savedState.putParcelable(KEY_SCANRESULT, mScanResult);
        savedState.putParcelable(KEY_WIFIINFO, mInfo);
        if (mState != null) {
            savedState.putString(KEY_DETAILEDSTATE, mState.toString());
        }
    }

    private void loadConfig(WifiConfiguration config) {
        ssid = (config.SSID == null ? "" : removeDoubleQuotes(config.SSID));
        bssid = config.BSSID;
        security = getSecurity(config);
        networkId = config.networkId;
        mRssi = Integer.MAX_VALUE;
        mConfig = config;
    }

    private void loadResult(ScanResult result) {
        ssid = result.SSID;
        bssid = result.BSSID;
        security = getSecurity(result);
        wpsAvailable = security != SECURITY_EAP && result.capabilities.contains("WPS");
        if (security == SECURITY_PSK)
            pskType = getPskType(result);
        networkId = -1;
        mRssi = result.level;
        mScanResult = result;
    }

    public void bindView() {
        if(mView == null) {
            return;
        }
        ImageView signal = (ImageView) mView.findViewById(R.id.netitemicon);
        if (mRssi == Integer.MAX_VALUE) {
            signal.setImageDrawable(null);
        } else {
            signal.setImageLevel(getLevel());
        }
        refresh();
    }

    @Override
    public int compareTo(AccessPoint accessPoint) {

        AccessPoint other = accessPoint;
        // Active one goes first.
        if (mInfo != null && other.mInfo == null) return -1;
        if (mInfo == null && other.mInfo != null) return 1;

        // Reachable one goes before unreachable one.
        if (mRssi != Integer.MAX_VALUE && other.mRssi == Integer.MAX_VALUE) return -1;
        if (mRssi == Integer.MAX_VALUE && other.mRssi != Integer.MAX_VALUE) return 1;

        // Configured one goes before unconfigured one.
        if (networkId != WifiConfiguration.INVALID_NETWORK_ID
                && other.networkId == WifiConfiguration.INVALID_NETWORK_ID) return -1;
        if (networkId == WifiConfiguration.INVALID_NETWORK_ID
                && other.networkId != WifiConfiguration.INVALID_NETWORK_ID) return 1;

        // Sort by signal strength.
        int difference = WifiManager.compareSignalLevel(other.mRssi, mRssi);
        if (difference != 0) {
            return difference;
        }
        // Sort by ssid.
        return ssid.compareToIgnoreCase(other.ssid);
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof AccessPoint)) return false;
        return (this.compareTo((AccessPoint) other) == 0);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(ssid);
        dest.writeInt(security);
        dest.writeInt(networkId);
        dest.writeInt(mRssi);
        dest.writeParcelable(mInfo, flags);
        dest.writeParcelable(mConfig, flags);
    }

    public static final Creator<AccessPoint> CREATOR =
        new Creator<AccessPoint>() {
            public AccessPoint createFromParcel(Parcel in) {
                AccessPoint mAp = new AccessPoint();

                String ssid = in.readString();
                if (ssid != null) {
                    mAp.ssid = ssid;
                }
                int security = in.readInt();
                mAp.security = security;
                int networkId = in.readInt();
                mAp.networkId = networkId;
                int mRssi = in.readInt();
                mAp.mRssi = mRssi;
                WifiInfo mInfo = (WifiInfo)in.readParcelable(null);
                mAp.mInfo = mInfo;
                WifiConfiguration mConfig = (WifiConfiguration)in.readParcelable(null);
                mAp.mConfig = mConfig;
                return mAp;
            }

            public AccessPoint[] newArray(int size) {
                return new AccessPoint[size];
            }
        };

    @Override
    public int hashCode() {
        int result = 0;
        if (mInfo != null) result += 13 * mInfo.hashCode();
        result += 19 * mRssi;
        result += 23 * networkId;
        result += 29 * ssid.hashCode();
        return result;
    }

    boolean update(ScanResult result) {
        if (ssid.equals(result.SSID) && security == getSecurity(result)) {
            if (WifiManager.compareSignalLevel(result.level, mRssi) > 0) {
                int oldLevel = getLevel();
                mRssi = result.level;
                if (getLevel() != oldLevel) {
//                    notifyChanged();
                }
            }
            // This flag only comes from scans, is not easily saved in config
            if (security == SECURITY_PSK) {
                pskType = getPskType(result);
            }
            refresh();
            return true;
        }
        return false;
    }

    void update(WifiInfo info, DetailedState state) {
        boolean reorder = false;
        if (info != null && networkId != WifiConfiguration.INVALID_NETWORK_ID
                && networkId == info.getNetworkId()) {
            reorder = (mInfo == null);
            mRssi = info.getRssi();
            mInfo = info;
            mState = state;
            refresh();
        } else if (mInfo != null) {
            reorder = true;
            mInfo = null;
            mState = null;
            refresh();
        }
        if (reorder) {
//            notifyHierarchyChanged();
        }
    }

    int getLevel() {
        if (mRssi == Integer.MAX_VALUE) {
            return -1;
        }
        return WifiManager.calculateSignalLevel(mRssi, 4);
    }

    WifiConfiguration getConfig() {
        return mConfig;
    }

    WifiInfo getInfo() {
        return mInfo;
    }

    DetailedState getState() {
        return mState;
    }

    static String removeDoubleQuotes(String string) {
        int length = string.length();
        if ((length > 1) && (string.charAt(0) == '"')
                && (string.charAt(length - 1) == '"')) {
            return string.substring(1, length - 1);
        }
        return string;
    }

    public static String convertToQuotedString(String string) {
        return "\"" + string + "\"";
    }

    /** Updates the title and summary; may indirectly call notifyChanged()  */
    private void refresh() {

        Context context = mContext;
        if (mConfig != null && mConfig.status == WifiConfiguration.Status.DISABLED) {
            switch (mConfig.disableReason) {
                case WifiConfiguration.DISABLED_AUTH_FAILURE:
                    setSummary(context.getString(R.string.wifi_disabled_password_failure));
                    break;
                case WifiConfiguration.DISABLED_DHCP_FAILURE:
                case WifiConfiguration.DISABLED_DNS_FAILURE:
                    setSummary(context.getString(R.string.wifi_disabled_network_failure));
                    break;
                case WifiConfiguration.DISABLED_UNKNOWN_REASON:
                    setSummary(context.getString(R.string.wifi_disabled_generic));
            }
        } else if (mRssi == Integer.MAX_VALUE) { // Wifi out of range
            setSummary(context.getString(R.string.wifi_not_in_range));
        } else if (mState != null) { // This is the active connection
        	setSummary(Summary.get(context, mState));
        } else { // In range, not disabled.
            StringBuilder summary = new StringBuilder();
            if (mConfig != null) { // Is saved network
                summary.append(context.getString(R.string.wifi_remembered));
            }
            setSummary(summary.toString());
        }
    }

    private void setSummary(String str) {
        if(mView != null) {
            TextView text = (TextView)(mView.findViewById(R.id.wifistat));
            text.setText(str);
        }
    }
    /**
     * Generate and save a default wifiConfiguration with common values.
     * Can only be called for unsecured networks.
     * @hide
     */
    protected void generateOpenNetworkConfig() {
        if (security != SECURITY_NONE)
            throw new IllegalStateException();
        if (mConfig != null)
            return;
        mConfig = new WifiConfiguration();
        mConfig.SSID = AccessPoint.convertToQuotedString(ssid);
        mConfig.allowedKeyManagement.set(KeyMgmt.NONE);
    }
}
