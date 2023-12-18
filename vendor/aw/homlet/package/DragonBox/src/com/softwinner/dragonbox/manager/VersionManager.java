package com.softwinner.dragonbox.manager;

import android.content.Context;
import android.os.Build;

import com.softwinner.dragonbox.entity.VersionInfo;
import com.softwinner.dragonbox.entity.VersionInfoMore;
import com.softwinner.dragonbox.utils.SystemUtil;
import com.softwinner.dragonbox.utils.WifiUtil;

public class VersionManager {
    private Context mContext;
    private VersionInfo mConfVersionInfo;
    private VersionInfoMore mSysVersionInfo;

    public VersionManager(Context context) {
        mContext = context;
        mSysVersionInfo = new VersionInfoMore(context);
        mSysVersionInfo.setWifiMac(WifiUtil.getWIFIMac(mContext));
        mSysVersionInfo.setEth0Mac(SystemUtil.getEth0MAC());
    }



    public void setConfVersionInfo(VersionInfo confVersionInfo) {
        mConfVersionInfo = confVersionInfo;
    }

    public boolean isVersionEquals() {
        return mSysVersionInfo.equals(mConfVersionInfo);
    }

    public VersionInfoMore getSysVersionInfo() {
        return mSysVersionInfo;
    }

    public VersionInfo getConfVersionInfo() {
        return mConfVersionInfo;
    }
}
