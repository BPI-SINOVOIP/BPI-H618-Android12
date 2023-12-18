/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner;

import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import java.lang.String;

/**
 * Class that provides access to some of the gpio management functions.
 *
 */
@Deprecated
public class AWDisplay {
    public static final String TAG = "awdisplay";
    public static final int DISP0 = 0;
    public static final int DISP1 = 1;
    public static final int DISPLAY_CMD_SET_3DMODE = 0x01;    //取值范围 0,1,2,3,4,5,6
    public static final int DISPLAY_CMD_GET_3DMODE = 0x02;
    public static final int DISPLAY_CMD_SET_BACKLIGHT = 0x03; //取值范围 0,1,2
    public static final int DISPLAY_CMD_GET_BACKLIGHT = 0x04;
    public static final int DISPLAY_CMD_SET_ENHANCE = 0x05;   //取值范围0,1,2,3
    public static final int DISPLAY_CMD_GET_ENHANCE = 0x06;
    public static final int DISPLAY_CMD_SET_READING_MODE = 0x08;
    public static final int DISPLAY_CMD_GET_READING_STATE = 0x09;
    public static final int DISPLAY_CMD_GET_READING_STRENGTH = 0x0a;
    public static final int DISPLAY_CMD_SET_COLOR_TEMPERATURE = 0x0b;
    public static final int DISPLAY_CMD_GET_COLOR_TEMPERATURE = 0x0c;

    /*Status Masks for Enhance/smbl/reading mode*/
    public static final int DISPLAY_FUNC_OFF_MASK = 0x00;
    public static final int DISPLAY_FUNC_ON_MASK = 0x01;
    public static final int DISPLAY_FUNC_DEMO_ON_MASK = 0x03;

    public static int sVersion = 0;
    public static boolean sReady = false;
    public static int sSmartBacklight; // 智能背光
    public static int sColorEnhance; // 丽色系统
    public static int sReadingModeState;
    public static int sReadingModeStrength;
    public static int sColorTemperature;

    private static IDisplayService mBinder;

    static {
        init();
    }

    // can't instantiate this class
    private AWDisplay() {
    }

    private static void init() {
        mBinder = IDisplayService.Stub.asInterface(ServiceManager
                .getService("aw_display"));
        if (mBinder != null) {
            try {
                sVersion = mBinder.getInterfaceVersion();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            sReady = true;
            if (sVersion < 1) {
                sSmartBacklight = dispCtrl(DISP0, DISPLAY_CMD_GET_BACKLIGHT, 0, 0);
                sColorEnhance = dispCtrl(DISP0, DISPLAY_CMD_GET_ENHANCE, 0, 0);
                sReadingModeState = dispCtrl(DISP0, DISPLAY_CMD_GET_READING_STATE, 0, 0);
                sReadingModeStrength = dispCtrl(DISP0, DISPLAY_CMD_GET_READING_STRENGTH, 0, 0);
                sColorTemperature = dispCtrl(DISP0, DISPLAY_CMD_GET_COLOR_TEMPERATURE, 0, 0);
            }
        } else {
            Log.w(TAG, "Cannot connect to aw_display");
        }
        Log.i(TAG, "AWDisplay remoteVersion: " + sVersion + " clientVersion: " + IDisplayService.VERSION);
    }

    public static boolean ready() {
        return sReady;
    }

    public static int dispCtrl(int disp, int cmd0, int cmd1, int data) {
        int ret = -1;
        if (mBinder != null) {
            try {
                ret = mBinder.displayCtrl(disp, cmd0, cmd1, data);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        } else {
            Log.w(TAG, "Cannot dispCtrl");
        }
        return ret;
    }

    public static int getVersion() {
        return sVersion;
    }

    public static int getHdmiMode() {
        if (sVersion < 1) {
            Log.e(TAG, "getHdmiMode not support below version 1");
            return -1;
        }
        try {
            return mBinder.getHdmiMode();
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
            return -1;
        }
    }

    public static boolean setHdmiMode(int mode) {
        if (sVersion < 1) {
            Log.e(TAG, "setHdmiMode not support below version 1");
            return false;
        }
        try {
            return mBinder.setHdmiMode(mode);
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
            return false;
        } catch (android.os.ServiceSpecificException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
            return false;
        }
    }

    public static boolean getSmartBacklight() {
        if (sVersion >= 1) {
            try {
                return mBinder.getSmartBacklight();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return false;
        }
        return (sSmartBacklight & DISPLAY_FUNC_ON_MASK) == DISPLAY_FUNC_ON_MASK;
    }

    public static void setSmartBacklight(boolean on) {
        if (sVersion >= 1) {
            try {
                mBinder.setSmartBacklight(on);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return;
        }
        if (on) sSmartBacklight |= DISPLAY_FUNC_ON_MASK;
        else sSmartBacklight &= DISPLAY_FUNC_OFF_MASK;
        dispCtrl(DISP0, DISPLAY_CMD_SET_BACKLIGHT, sSmartBacklight, 0);
    }

    public static boolean getSmartBacklightDemo() {
        if (sVersion >= 1) {
            try {
                return mBinder.getSmartBacklightDemo();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return false;
        }
        return (sSmartBacklight & DISPLAY_FUNC_DEMO_ON_MASK) == DISPLAY_FUNC_DEMO_ON_MASK;
    }

    public static void setSmartBacklightDemo(boolean on) {
        if (sVersion >= 1) {
            try {
                mBinder.setSmartBacklightDemo(on);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return;
        }
        if (on) sSmartBacklight |= DISPLAY_FUNC_DEMO_ON_MASK;
        else sSmartBacklight &= DISPLAY_FUNC_ON_MASK;
        dispCtrl(DISP0, DISPLAY_CMD_SET_BACKLIGHT, sSmartBacklight, 0);
    }

    public static boolean getColorEnhance() {
        if (sVersion >= 1) {
            try {
                return mBinder.getColorEnhance();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return false;
        }
        return (sColorEnhance & DISPLAY_FUNC_ON_MASK) == DISPLAY_FUNC_ON_MASK;
    }

    public static void setColorEnhance(boolean on) {
        if (sVersion >= 1) {
            try {
                mBinder.setColorEnhance(on);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return;
        }
        if (on)  sColorEnhance |= DISPLAY_FUNC_ON_MASK;
        else sColorEnhance &= DISPLAY_FUNC_OFF_MASK;
        dispCtrl(DISP0, DISPLAY_CMD_SET_ENHANCE, sColorEnhance, 0);
    }

    public static boolean getColorEnhanceDemo() {
        if (sVersion >= 1) {
            try {
                return mBinder.getColorEnhanceDemo();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return false;
        }
        return (sColorEnhance & DISPLAY_FUNC_DEMO_ON_MASK) == DISPLAY_FUNC_DEMO_ON_MASK;
    }

    public static void setColorEnhanceDemo(boolean on) {
        if (sVersion >= 1) {
            try {
                mBinder.setColorEnhanceDemo(on);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return;
        }
        if (on) sColorEnhance |= DISPLAY_FUNC_DEMO_ON_MASK;
        else sColorEnhance &= DISPLAY_FUNC_ON_MASK;
        dispCtrl(DISP0, DISPLAY_CMD_SET_ENHANCE, sColorEnhance, 0);
    }

    public static boolean getReadingMode() {
        if (sVersion >= 1) {
            try {
                return mBinder.getReadingMode();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return false;
        }
        return false;
    }

    public static void setReadingMode(boolean on) {
        if (sVersion >= 1) {
            try {
                mBinder.setReadingMode(on);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return;
        }
    }

    public static int getColorTemperature() {
        if (sVersion >= 1) {
            try {
                return mBinder.getColorTemperature();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return -1;
        }
        return dispCtrl(DISP0, DISPLAY_CMD_GET_COLOR_TEMPERATURE, 0, 0);
    }

    public static void setColorTemperature(int value) {
        if (sVersion >= 1) {
            try {
                mBinder.setColorTemperature(value);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return;
        }
        dispCtrl(DISP0, DISPLAY_CMD_SET_COLOR_TEMPERATURE, value, 0);
    }

    public static int getHSLBright() {
        if (sVersion >= 1) {
            try {
                return mBinder.getHSLBright();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        }
        return -1;
    }

    public static void setHSLBright(int value) {
        if (sVersion >= 1) {
            try {
                mBinder.setHSLBright(value);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        }
    }

    public static int getHSLContrast() {
        if (sVersion >= 1) {
            try {
                return mBinder.getHSLContrast();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        }
        return -1;
    }

    public static void setHSLContrast(int value) {
        if (sVersion >= 1) {
            try {
                mBinder.setHSLContrast(value);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        }
    }

    public static int getHSLSaturation() {
        if (sVersion >= 1) {
            try {
                return mBinder.getHSLSaturation();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        }
        return -1;
    }

    public static void setHSLSaturation(int value) {
        if (sVersion >= 1) {
            try {
                mBinder.setHSLSaturation(value);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
        }
    }

    public static int get3DMode() {
        if (sVersion >= 1) {
            try {
                return mBinder.get3DMode();
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return -1;
        }
        return dispCtrl(DISP1, DISPLAY_CMD_GET_3DMODE, 0, 0);
    }

    public static boolean set3DMode(int mode) {
        if (sVersion >= 1) {
            try {
                return mBinder.set3DMode(mode);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot connect to aw_display", e);
            }
            return false;
        }
        return dispCtrl(DISP1, DISPLAY_CMD_SET_3DMODE, mode, 0) == 0;
    }

    public static int getMarginWidth() {
        if (sVersion < 1) {
            Log.e(TAG, "getMarginWidth not support below version 1");
            return -1;
        }
        try {
            return mBinder.getMarginWidth();
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
            return -1;
        }
    }

    public static void setMarginWidth(int scale) {
        if (sVersion < 1) {
            Log.e(TAG, "setMarginWidth not support below version 1");
            return;
        }
        try {
            mBinder.setMarginWidth(scale);
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
        }
    }

    public static int getMarginHeight() {
        if (sVersion < 1) {
            Log.e(TAG, "getMarginHeight not support below version 1");
            return -1;
        }
        try {
            return mBinder.getMarginHeight();
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
            return -1;
        }
    }

    public static void setMarginHeight(int scale) {
        if (sVersion < 1) {
            Log.e(TAG, "setMarginHeight not support below version 1");
            return;
        }
        try {
            mBinder.setMarginHeight(scale);
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
        }
    }

    public static boolean getHdmiFullscreen() {
        if (sVersion < 1) {
            Log.e(TAG, "getHdmiFullscreen not support below version 1");
            return false;
        }
        try {
            return mBinder.getHdmiFullscreen();
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
            return false;
        }
    }

    public static void setHdmiFullscreen(boolean full) {
        if (sVersion < 1) {
            Log.e(TAG, "setHdmiFullscreen not support below version 1");
            return;
        }
        try {
            mBinder.setHdmiFullscreen(full);
        } catch (RemoteException e) {
            Log.w(TAG, "Cannot connect to aw_display", e);
        }
    }
}

