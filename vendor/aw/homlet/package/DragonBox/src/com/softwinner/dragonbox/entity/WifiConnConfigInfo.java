package com.softwinner.dragonbox.entity;

public class WifiConnConfigInfo {

    private String ssid;
    private int mMaxRSSI = -65;
    private String mWifiPWD;

    public String getSsid() {
        return ssid;
    }
    public void setSsid(String ssid) {
        this.ssid = ssid;
    }
    public int getMaxRSSI() {
        return mMaxRSSI;
    }
    public void setMaxRSSI(int maxRSSI) {
        this.mMaxRSSI = maxRSSI;
    }
    public String getWifiPWD() {
        return mWifiPWD;
    }
    public void setWifiPWD(String wifiPWD) {
        this.mWifiPWD = wifiPWD;
    }



}
