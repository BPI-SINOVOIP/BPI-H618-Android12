package com.softwinner.dragonbox.entity;

import com.softwinner.dragonbox.utils.WifiUtil;

import android.net.wifi.ScanResult;

public class WifiConnInfo implements Comparable<WifiConnInfo> {
    public boolean isConfig = false;
    public String ssid;
    public int RSSI;
    public int mMaxRSSI = -65;
    public String mWifiPWD;
    public int security;
    public int connectionType;

    public void updateWifiConnInfo(ScanResult result) {
        ssid = result.SSID;
        RSSI = result.level;
        security = WifiUtil.getSecurity(result);
    }

    @Override
    public int compareTo(WifiConnInfo another) {
        if (isConfig && !another.isConfig) {
            return -1;
        } else if (!isConfig && another.isConfig) {
            return 1;
        } else {
            return RSSI < another.RSSI ? 1 : -1;
        }
    }
}
