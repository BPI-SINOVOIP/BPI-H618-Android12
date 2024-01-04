package com.softwinner.dragonbox.platform;

import android.content.Context;
import android.view.DisplayManagerAw;

public class DisplayManagerPlatform implements IDisplayManagerPlatform {
    private Context mContext;
    private DisplayManagerAw mDisplayManager;

    public DisplayManagerPlatform(Context context) {
        mContext = context;
        mDisplayManager = (DisplayManagerAw) mContext
                .getSystemService(Context.DISPLAY_SERVICE_AW);
    }

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
        if (mDisplayManager.getDisplayOutputType(0) != DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI) {
            mDisplayManager.setDisplayOutputType(0,
                    DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI,
                    DisplayManagerAw.DISPLAY_TVFORMAT_720P_50HZ);
            mDisplayManager.setDisplayParameter(0,
                    DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI,
                    DisplayManagerAw.DISPLAY_TVFORMAT_720P_50HZ);
            mDisplayManager
                    .setDisplayMode(DisplayManagerAw.DISPLAY_MODE_SINGLE_FB_GPU);
        }

    }

    @Override
    public void changeToCVBS() {
        if (mDisplayManager.getDisplayOutputType(0) != DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV
                || mDisplayManager.getDisplayOutputFormat(0) != DisplayManagerAw.DISPLAY_TVFORMAT_PAL) {
            mDisplayManager.setDisplayOutputType(0,
                    DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV,
                    DisplayManagerAw.DISPLAY_TVFORMAT_PAL);
            mDisplayManager.setDisplayParameter(0,
                    DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV,
                    DisplayManagerAw.DISPLAY_TVFORMAT_PAL);
            mDisplayManager
                    .setDisplayMode(DisplayManagerAw.DISPLAY_MODE_SINGLE_FB_GPU);
        }
    }

}
