package com.softwinner.dragonbox.entity;

import com.softwinner.dragonbox.utils.SystemUtil;

import android.content.Context;
import android.os.Build;
import android.os.SystemProperties;

public class VersionInfoMore extends VersionInfo{
    private String mWifiMac;
    private String mEth0Mac;
    private String mSN;

    public VersionInfoMore(Context context) {
        super(Build.VERSION.RELEASE, Build.MODEL, Build.DISPLAY, "",
                SystemUtil.getTotalMemory(context),SystemUtil.getFlashCapability());
    }

    public String getWifiMac() {
        return mWifiMac;
    }
    public void setWifiMac(String wifiMac) {
        mWifiMac = wifiMac;
    }
    public String getEth0Mac() {
        return mEth0Mac;
    }
    public void setEth0Mac(String eth0Mac) {
        mEth0Mac = eth0Mac;
    }

    public String getSN() {
        return mSN;
    }
    public void setSN(String sN) {
        mSN = sN;
    }



}
