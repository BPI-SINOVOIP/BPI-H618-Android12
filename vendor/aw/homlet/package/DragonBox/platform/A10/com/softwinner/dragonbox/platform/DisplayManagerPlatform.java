package com.softwinner.dragonbox.platform;

import android.content.Context;
import android.view.DisplayManager;

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
        return mDisplayManager.getTvHotPlugStatus() == 2;
    }

    @Override
    public boolean getHdmiHotPlugStatus() {

        return mDisplayManager.getHdmiHotPlugStatus() == 1;
    }

    @Override
    public void changeToHDMI() {
        if (mDisplayManager.getDisplayOutputType(0) != DisplayManager.DISPLAY_OUTPUT_TYPE_HDMI) {
            mDisplayManager.setDisplayOutputType(0,
                    DisplayManager.DISPLAY_OUTPUT_TYPE_HDMI,
                    DisplayManager.DISPLAY_TVFORMAT_720P_50HZ);
            mDisplayManager.setDisplayParameter(0,
                    DisplayManager.DISPLAY_OUTPUT_TYPE_HDMI,
                    DisplayManager.DISPLAY_TVFORMAT_720P_50HZ);
            mDisplayManager
                    .setDisplayMode(DisplayManager.DISPLAY_MODE_SINGLE_FB_GPU);
        }
    }

    @Override
    public void changeToCVBS() {
        if (mDisplayManager.getDisplayOutputType(0) != DisplayManager.DISPLAY_OUTPUT_TYPE_TV
                || mDisplayManager.getDisplayOutputFormat(0) != DisplayManager.DISPLAY_TVFORMAT_PAL) {
            mDisplayManager.setDisplayOutputType(0,
                    DisplayManager.DISPLAY_OUTPUT_TYPE_TV,
                    DisplayManager.DISPLAY_TVFORMAT_PAL);
            mDisplayManager.setDisplayParameter(0,
                    DisplayManager.DISPLAY_OUTPUT_TYPE_TV,
                    DisplayManager.DISPLAY_TVFORMAT_PAL);
            mDisplayManager
                    .setDisplayMode(DisplayManager.DISPLAY_MODE_SINGLE_FB_GPU);
        }
    }
}
