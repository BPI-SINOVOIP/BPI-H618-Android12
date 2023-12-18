package com.softwinner.dragonbox.platform;

import java.io.FileReader;
import java.io.IOException;

import android.content.Context;
import android.hardware.display.DisplayManager;
import android.util.Log;

public class DisplayManagerPlatform implements IDisplayManagerPlatform {
    private static final String TAG = "DisplayManagerPlatform";
    private Context mContext;
    private DisplayManager mDisplayManager;

    public DisplayManagerPlatform(Context context) {
        mContext = context;
        mDisplayManager = (DisplayManager) mContext
                .getSystemService(Context.DISPLAY_SERVICE);
    }

    /**
     * not support cvbs
     *
     * @return
     */
    @Override
    public boolean getTvHotPlugStatus() {
        boolean plugged = false;
        final String filename = "/sys/class/switch/cvbs/state";
        FileReader reader = null;
        try {
            reader = new FileReader(filename);
            char[] buf = new char[15];
            int n = reader.read(buf);
            if (n > 1) {
                plugged = 0 != Integer.parseInt(new String(buf, 0, n - 1));
            }
        } catch (IOException ex) {
            Log.e(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
        } catch (NumberFormatException ex) {
            Log.w(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ex) {
                }
            }
        }
        return plugged;
    }

    @Override
    public boolean getHdmiHotPlugStatus() {
        boolean plugged = false;
        final String filename = "/sys/class/switch/hdmi/state";
        FileReader reader = null;
        try {
            reader = new FileReader(filename);
            char[] buf = new char[15];
            int n = reader.read(buf);
            if (n > 1) {
                plugged = 0 != Integer.parseInt(new String(buf, 0, n - 1));
            }
        } catch (IOException ex) {
            Log.e(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
        } catch (NumberFormatException ex) {
            Log.w(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ex) {
                }
            }
        }
        return plugged;
    }

    @Override
    public void changeToHDMI() {
        changeOutputChannel("404");
    }

    @Override
    public void changeToCVBS() {
        changeOutputChannel("20b");
    }

    private void changeOutputChannel(String value) {
        try {
            int format = Integer.parseInt(value, 16);
            int dispformat = mDisplayManager.getDisplayModeFromFormat(format);
            Log.d(TAG, "dispformat = " + dispformat + "format = " + format);
            //int mCurType = mDisplayManager
            //		.getDisplayOutputType(android.view.Display.TYPE_BUILT_IN);
            mDisplayManager.setDisplayOutput(
                    android.view.Display.TYPE_BUILT_IN, format);
        } catch (NumberFormatException e) {
            Log.w(TAG, "Invalid display output format!");
        }
    }

}
