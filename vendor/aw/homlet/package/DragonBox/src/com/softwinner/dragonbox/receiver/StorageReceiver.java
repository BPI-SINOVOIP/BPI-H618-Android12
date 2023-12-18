package com.softwinner.dragonbox.receiver;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.Bundle;
import android.os.SystemProperties;
import java.io.IOException;
import android.os.storage.StorageVolume;
import com.softwinner.dragonbox.DragonBoxApplication;
import com.softwinner.dragonbox.config.ConfigManager;

public class StorageReceiver extends BroadcastReceiver {
    private static final String TAG = "DragonBox-StorageReceiver";

    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.w(TAG,"Receiver broadcast,its action is "+action);
        //Test
        //DragonBoxApplication.getReadPrivateJNI().nativeSetParameter("btpair", "start");

        String assyResult = DragonBoxApplication.getReadPrivateJNI().nativeGetParameter("assy");
        String btpair = DragonBoxApplication.getReadPrivateJNI().nativeGetParameter("btpair");
        if(assyResult!=null&&assyResult.equals("pass")) {
            if(btpair==null||!btpair.equals("start")){
                Log.w(TAG,"assy = pass, btpair = " + btpair + " ,donot start Dragon Tools!!");
                return;
            } else {
                ConfigManager.startBTPairActivity(context);
                return;
            }
        }
        if(action!=null&&action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
            Bundle bundle = intent.getExtras();
            StorageVolume storageVolume = (StorageVolume)bundle.get(StorageVolume.EXTRA_STORAGE_VOLUME);
            Log.d(TAG, "storageVolume = "+storageVolume.getPath()+", isEmulated = "+storageVolume.isEmulated());
            if(storageVolume.isEmulated())
                return;
        }
        if((!action.equals("android.intent.action.BOOT_COMPLETED"))&&(!SystemProperties.get("sys.boot_completed").equals("1"))) {
            Log.e(TAG, "Android does not boot completed,donot start Dragonbox!");
            return;
        }
        if(action.equals("android.intent.action.BOOT_COMPLETED")) {
            if(btpair!=null && btpair.equals("start")){

                return;
            }
            boolean startDragonAging = SystemProperties.getBoolean("persist.sys.dragonaging",false);
            if(startDragonAging) {
                ConfigManager.startConfigAPK(context, ConfigManager.CONFIG_DRAGON_AGING, false);
                return;
            }
        }
        if (!ConfigManager.startConfigAPK(context, ConfigManager.CONFIG_DRAGON_BOX, false)) {
            if (!ConfigManager.startConfigAPK(context, ConfigManager.CONFIG_DRAGON_SN, false)) {
                if (!ConfigManager.startConfigAPK(context, ConfigManager.CONFIG_DRAGON_AGING, false)) {
                    Log.d(TAG,"config file not exist");
                }
            }
        }
        //set usb device mode
        if(ConfigManager.isConfigFileExist(context,ConfigManager.CONFIG_USB_DEVICE)){
            SystemProperties.set("persist.sys.usb.config","mtp,adb");
            SystemProperties.set("persist.sys.usb0device","1");
            SystemProperties.set("persist.vendor.usb0device","1");
            return;
        }
        if (ConfigManager.isConfigFileExist(context, "/DragonBox/TimeCamera")) {
            Intent it = new Intent();
            it.setComponent(new ComponentName("com.ysten.app.camera", "com.ysten.app.camera.WelcomeActivity"));
            it.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(it);
        }
        if(ConfigManager.isConfigFileExist(context, "/DragonBox/switch_usb")) {
            Log.d(TAG,"switch the USB0 to device mode");
            SystemProperties.set("sys.service.adbd.enable",  "1");
            try {
                Runtime runtime = Runtime.getRuntime();
                String[] cmd = {"qw", "-c", "cat /sys/devices/soc.0/usbc0.5/usb_device"};
                Process proc = runtime.exec(cmd);
                proc.waitFor();
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        } else {
            Log.d(TAG,"DragonBox Receiver Exit!!!");
        }
    }
}
