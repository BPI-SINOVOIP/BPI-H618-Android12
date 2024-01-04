package com.softwinner.dragonbox.platform;

import java.io.FileReader;
import java.io.IOException;

import android.content.Context;
import android.hardware.display.DisplayManager;
import android.os.SystemProperties;
import vendor.display.DisplayOutputManager;
import android.util.Log;
import android.os.Build;
import java.io.File;
import com.softwinner.SystemMix;

public class DisplayManagerPlatform implements IDisplayManagerPlatform {
    private static final String TAG = "DragonBox-DisplayManagerPlatform";
    private static final String HDMI_PLUG_PATH= "/sys/class/hdmi/hdmi/attr/hpd_mask";
    private Context mContext;
    private DisplayOutputManager mDisplayManager;
    private static int mHDMIDisplayOutput = 0;//record HDMI resolution

    public DisplayManagerPlatform(Context context) {
        mContext = context;
        mDisplayManager = DisplayOutputManager.getInstance();
        if(mHDMIDisplayOutput == 0){
            mHDMIDisplayOutput = mDisplayManager.getDisplayOutput(DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI);
            Log.d(TAG,"mHDMIDisplayOutput = "+mHDMIDisplayOutput);
        }
    }

    /**
     * not support cvbs
     *
     * @return
     */
    @Override
    public boolean getTvHotPlugStatus() {
        boolean plugged = false;
        if (Build.VERSION.SDK_INT < 31) {
            final String filePath = "/sys/class/switch/cvbs/state";
            plugged = readStateInPoint(filePath,0);
        } else {
            final String rootPath = "/sys/class/extcon";
            File root = new File(rootPath);
            if (root.exists() && root.isDirectory()) {
                File[] filesList = root.listFiles();
                if (filesList != null) {
                    int size = filesList.length;
                    Log.d(TAG,"size:"+size);
                    for (File file : filesList) {
                        String path = rootPath + File.separator + file.getName() + File.separator + "state";
                        if (readStateInPoint(path,0)) {
                            plugged = true;
                            Log.d(TAG, "read cvbs point success");
                            break;
                        }
                    }
                }
            }
        }
        return plugged;
    }

    @Override
    public boolean getHdmiHotPlugStatus() {
        boolean plugged = false;
        if (Build.VERSION.SDK_INT < 31) {
            final String filePath = "/sys/class/switch/hdmi/state";
            plugged = readStateInPoint(filePath,1);
        } else {
            final String rootPath = "/sys/class/extcon";
            File root = new File(rootPath);
            if (root.exists() && root.isDirectory()) {
                File[] filesList = root.listFiles();
                if (filesList != null) {
                    int size = filesList.length;
                    Log.d(TAG,"size:"+size);
                    for (File file : filesList) {
                        String path = rootPath + File.separator + file.getName() + File.separator + "state";
                        if (readStateInPoint(path,1)) {
                            plugged = true;
                            Log.d(TAG, "read hdmi point success");
                            break;
                        }
                    }
                }
            }
        }
        return plugged;
    }

    // type: 0 means cvbs. 1 means hdmi
    private boolean readStateInPoint(String filePath,int type) {
        Log.d(TAG,"readStateInPoint filePath : " + filePath + " type:"+type);
        boolean res = false;
        FileReader reader = null;
        try {
            reader = new FileReader(filePath);
            char[] buf = new char[15];
            int n = reader.read(buf);
            if (n > 1) {
                String context = new String(buf, 0, n - 1);
                Log.d(TAG,"readStateInPoint context:"+context);
                switch (type) {
                    case 0:
                        res = context.contains("CVBS") && context.contains("1");
                        break;
                    case 1:
                        res = context.contains("HDMI") && context.contains("1");
                        break;
                    default:
                        Log.e(TAG,"no type for read");
                        break;
                }
            }
        } catch (IOException ex) {
            Log.e(TAG, "Couldn't read state from " + filePath + ": " + ex);
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ex) {
                }
            }
        }
        return res;
    }

    @Override
    public void changeToHDMI() {
        //changeOutputChannel("404");
        try {
            //int format = Integer.parseInt(value, 16);
            //int dispformat = mDisplayManager.getDisplayModeFromFormat(format);
            //Log.d(TAG, "dispformat = " + dispformat + "format = " + format);
            //int mCurType = mDisplayManager
            //		.getDisplayOutputType(android.view.Display.TYPE_BUILT_IN);
            //int hdmiFormat = Integer.parseInt(SystemProperties.get("persist.sys.disp_dev0"), 16);

            //mDisplayManager.setDisplayOutput(android.view.Display.TYPE_BUILT_IN, mHDMIDisplayOutput);

            //for m20  单显，通过插拔hdmi在hdmi和cvbs之间切换。
            SystemMix.writeFile(HDMI_PLUG_PATH,"0x11");
        } catch (NumberFormatException e) {
            Log.w(TAG, "Invalid display output format!");
        }
    }

    @Override
    public void changeToCVBS() {
        //changeOutputChannel("20b");

        //for m20  单显，通过插拔hdmi在hdmi和cvbs之间切换。
        SystemMix.writeFile(HDMI_PLUG_PATH,"0x10");
    }

    private void changeOutputChannel(String value) {
        try {
            int format = Integer.parseInt(value, 16);
            int dispformat = mDisplayManager.getDisplayModeFromFormat(format);
            Log.d(TAG, "dispformat = " + dispformat + "format = " + format);
            mDisplayManager.setDisplayOutput(
                    android.view.Display.TYPE_INTERNAL, format);
        } catch (NumberFormatException e) {
            Log.w(TAG, "Invalid display output format!");
        }
    }

}
