package com.softwinner.update;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.text.TextUtils;

import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.DeviceClass;

/**
 * Created by zengshuchuan on 2017.
 */

public class UpdateDevice extends Device {
    private static UpdateDevice mInstance;

    /**
     * Construct device data with given parameters.
     *
     * @param deviceId        Device unique identifier.
     * @param name
     * @param mac
     * @param vendor
     * @param firmwareversion
     * @param romType
     * @param deviceClass
     */
    public UpdateDevice(String deviceId, String name, String mac, String vendor, String firmwareversion, String romType, DeviceClass deviceClass) {
        super(deviceId, name, mac, vendor, firmwareversion, romType, deviceClass);
    }

    public static UpdateDevice getInstance(Context context) {
        if (mInstance == null) {
            WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            String temp_mac = wifiInfo.getMacAddress();
            String mac = temp_mac.replace(":", "");
//            String mac = readlineFile("/sys/class/net/wlan0/address").replace(":", "");
            String id = Build.SERIAL;
            String name = Build.PRODUCT.replaceAll("[^0-9a-zA-Z]", "");
            String vendor = Build.BRAND;
            String model = Build.PRODUCT.replaceAll("[^0-9a-zA-Z]", "");
            String firmwareVersion = PropertiesHelper.get(context, "ro.build.version.ota");
            if (TextUtils.isEmpty(firmwareVersion)) PropertiesHelper.get(context, "ro.vendor.build.version.ota");
            String romType = "STABLE"; // DEV or UT or STABLE
            DeviceClass deviceClass = new DeviceClass(model);
            mInstance = new UpdateDevice(id, name, mac, vendor, firmwareVersion, romType, deviceClass);
        }
        return mInstance;
    }

//    private static String readlineFile(String path) {
//        String line = null;
//        try {
//            BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(path)));
//            line = br.readLine();
//            br.close();
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
//        return line;
//    }
}
