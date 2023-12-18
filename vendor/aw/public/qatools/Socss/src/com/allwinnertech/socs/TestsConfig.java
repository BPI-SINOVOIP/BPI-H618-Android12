package com.allwinnertech.socs;

import android.content.Intent;
import android.content.ComponentName;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

public class TestsConfig {
    private static final String TAG = "TestsConfig";
    public String tfcard_video;
    public String usbhost_video;
    public boolean enable_ethernet_test;
    public boolean enable_cpuid_test;
    public boolean hide_config;
    private boolean auto_test;
    public boolean enable_camera;
    public boolean enable_bluetooth;
    public boolean enable_wifi;
    public boolean enable_gps;
    public boolean enable_gpu;
    public boolean enable_automap;

    public static ArrayList<Intent> TESTCASES = new ArrayList<>();

    public static int gTestPosition;

    public TestsConfig() {
        tfcard_video = "";
        usbhost_video = "";
        enable_ethernet_test = true;
        enable_cpuid_test = true;
        hide_config = false;
        auto_test = true;
        enable_camera = true;
        enable_bluetooth = true;
        enable_wifi = true;
        enable_gps = false;
        enable_gpu = true;
        enable_automap = true;
    }

    public void DumpAllValue(){
        Log.d(TAG,"enable_ethernet_test = " + enable_ethernet_test );
        Log.d(TAG,"enable_cpuid_test = " + enable_cpuid_test);
        Log.d(TAG,"enable_camera = " + enable_camera);
        Log.d(TAG,"enable_bluetooth = " + enable_bluetooth);
        Log.d(TAG,"enable_wifi = " + enable_wifi);
        Log.d(TAG,"enable_gps = " + enable_gps);
        Log.d(TAG,"enable_gpu = " + enable_gpu);
        Log.d(TAG,"enable_automap = " + enable_automap);
    }
    private void addTest(String pkg, String cls, int countDown) {
        Intent intent = new Intent();
        ComponentName cn = new ComponentName(pkg, cls);
        intent.setComponent(cn);
        // countDown para is that test auto finish
        intent.putExtra("countDown", countDown);
        TESTCASES.add(intent);
    }

    public void InitTestCases() {
        if (TESTCASES.size() > 0)
            return;
        TESTCASES.clear();
        gTestPosition = 0;
        addTest("com.allwinnertech.socs", "com.allwinnertech.socs.VideoTestActivity", -1);
        if (enable_camera) {
            Log.d(TAG, "enable camera, add Camera Test");
            addTest("com.allwinnertech.socs", "com.allwinnertech.socs.CameraTestActivity", -1);
        }
        if (enable_automap) {
            Log.d(TAG, "enable automap, add automap Test");
            addTest("com.autonavi.amapauto", "com.autonavi.auto.MainMapActivity", 15);
        }
        if (enable_bluetooth)
            addTest("com.allwinnertech.socs", "com.allwinnertech.socs.BluetoothTest", -1);
        if (enable_wifi)
            addTest("com.allwinnertech.socs", "com.allwinnertech.socs.WifiTest", -1);
        if (enable_gps)
            addTest("com.allwinnertech.socs", "com.allwinnertech.socs.GPSTest", -1);
        if (enable_gpu)
            addTest("com.allwinnertech.socs", "com.allwinnertech.socs.gpu.KubeActivity", -1);
        // add more befor here
        addTest("com.allwinnertech.socs", "com.allwinnertech.socs.MainTestActivity", -1);  // should be at last
    }

    public String getTfcardVideoPath() {
        return tfcard_video;
    }

    public void setTfcardVideoPath(String path) {
         tfcard_video = path;
    }

    public String getUsbhostVideoPath() {
        return usbhost_video;
    }

    public void setUsbhostVideoPath(String path) {
        usbhost_video = path;
    }

    public boolean getEnableEthTest() {
        return enable_ethernet_test;
    }

    public void setEnableEthTest(boolean enable) {
        enable_ethernet_test = enable;
    }

    public boolean getEnableCpuidTest() {
        return enable_cpuid_test;
    }

    public void setEnableCpuidTest(boolean enable) {
        enable_cpuid_test = enable;
    }

    public boolean getHideConfig() {
        return hide_config;
    }

    public void setHideConfig(boolean hide) {
        hide_config = hide;
    }

    public boolean getAutoTest() {
        return auto_test;
    }

    public void setAutoTest(boolean auto) {
        auto_test = auto;
    }

    public void setEnableCameraTest(boolean enable) {
        enable_camera = enable;
    }

    public boolean getEnableCameraTest() {
        return enable_camera;
    }

    public void setEnableBluetoothTest(boolean enable) {
        enable_bluetooth = enable;
    }

    public boolean getEnableBluetoothTest() {
        return enable_bluetooth;
    }

    public void setEnableWifiTest(boolean enable) {
        enable_wifi = enable;
    }

    public boolean getEnableWifiTest() {
        return enable_wifi;
    }

    public void setEnableAutoMapTest(boolean enable) {
        enable_automap = enable;
    }

    public boolean getEnableAutoMapTest() {
        return enable_automap;
    }

    public void saveAllConfigToFile(String configpath) {

        File configFile = new File(configpath);
        JSONObject configObject = new JSONObject();
        try {
            configObject.put("tfcard_video_path", this.tfcard_video);
            configObject.put("usbhost_video_path", this.usbhost_video);
            configObject.put("enable_ethernet_test", this.enable_ethernet_test);
            configObject.put("enable_cpuid_test", this.enable_cpuid_test);
            configObject.put("hide_config", this.hide_config);
            configObject.put("auto_test", this.auto_test);
            configObject.put("enable_camera", this.enable_camera);
            configObject.put("enable_bluetooth", this.enable_bluetooth);
            configObject.put("enable_wifi", this.enable_wifi);
            configObject.put("enable_gps", this.enable_gps);
            configObject.put("enable_gpu", this.enable_gpu);
            configObject.put("enable_automap", this.enable_automap);
            Log.d(TAG,configObject.toString());
            FileWriter fw = new FileWriter(configFile);
            fw.write(configObject.toString());
            fw.flush();
            fw.close();
        } catch (JSONException | IOException e) {
            e.printStackTrace();
        }
    }

    public boolean checkConfigFileExist(String configpath) {
        File configFilePath = new File(configpath);
        return configFilePath.exists();
    }

    public void loadAllConfigFromFile(String configpath) {
        File configFile = new File(configpath);
        try {
            FileReader fr  = new FileReader(configFile);
            BufferedReader br = new BufferedReader(fr);
            JSONObject configObject = new JSONObject(br.readLine());
            Log.d(TAG,configObject.toString());
            this.tfcard_video = configObject.getString("tfcard_video_path");
            this.usbhost_video = configObject.getString("usbhost_video_path");
            this.enable_ethernet_test = configObject.getBoolean("enable_ethernet_test");
            this.enable_cpuid_test = configObject.getBoolean("enable_cpuid_test");
            this.hide_config = configObject.getBoolean("hide_config");
            this.auto_test = configObject.getBoolean("auto_test");
            this.enable_camera = configObject.getBoolean("enable_camera");
            this.enable_bluetooth = configObject.getBoolean("enable_bluetooth");
            this.enable_wifi = configObject.getBoolean("enable_wifi");
            this.enable_gps = configObject.getBoolean("enable_gps");
            this.enable_gpu = configObject.getBoolean("enable_gpu");
            this.enable_automap = configObject.getBoolean("enable_automap");
        } catch (JSONException | IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
}
