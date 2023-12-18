package com.softwinner.update;

import android.text.TextUtils;

import com.bigbigcloud.devicehive.BaseDeviceApplication;

/**
 * Created by zengshuchuan on 2017
 */

public class UpdateApplication extends BaseDeviceApplication {
    @Override
    public void onCreate() {
        super.onCreate();
        String sysLicense = PropertiesHelper.get(this, "ro.sys.ota.license");
        if (TextUtils.isEmpty(sysLicense)) {
            sysLicense = PropertiesHelper.get(this, "ro.vendor.sys.ota.license");
        }
        if (!TextUtils.isEmpty(sysLicense)) {
            license = sysLicense;
        }
    }
}
