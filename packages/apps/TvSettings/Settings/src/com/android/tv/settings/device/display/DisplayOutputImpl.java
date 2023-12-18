package com.android.tv.settings.device.display;

import android.util.Log;
import androidx.preference.ListPreference;

import java.util.Map;
import java.util.HashMap;

import vendor.display.DisplayOutputManager;

public class DisplayOutputImpl {
    private static final String TAG = "DisplayOutputImpl";

    public static final int TYPE_NONE   = 0;
    public static final int TYPE_LCD    = 1;
    public static final int TYPE_CVBS   = 2;
    public static final int TYPE_HDMI   = 4;
    public static final int TYPE_VGA    = 8;

    private static final HashMap<Integer, String> mDispFormatNameMap = createDispFormatMap();
    private static HashMap<Integer, String> createDispFormatMap() {
        HashMap<Integer, String> map = new HashMap<Integer, String>();

        map.put(255,                                                    " AUTO"              );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_480I,             " 480i"              );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_576I,             " 576i"              );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_480P,             " 480p"              );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_576P,             " 576p"              );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_720P_50HZ,        " 720p 50Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_720P_60HZ,        " 720p 60Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080I_50HZ,       "1080i 50Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080I_60HZ,       "1080i 60Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080P_24HZ,       "1080p 24Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080P_50HZ,       "1080p 50Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080P_60HZ,       "1080p 60Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080P_24HZ_3D_FP, "1080p 24Hz (3D FP)" );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080P_25HZ,       "1080p 25Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_1080P_30HZ,       "1080p 30Hz"         );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_PAL,              "PAL"                );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_PAL_SVIDEO,       "NTSC SVIDEO"        );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_NTSC,             "NTSC"               );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_NTSC_SVIDEO,      "NTSC SVIDEO"        );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_PAL_M,            "PAL M"              );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_PAL_M_SVIDEO,     "PAL M SVIDEO"       );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_PAL_NC,           "PAL NC"             );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_PAL_NC_SVIDEO,    "PAL NC SVIDEO"      );
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_30HZ,  "4K 30Hz (3840x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_25HZ,  "4K 25Hz (3840x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_24HZ,  "4K 24Hz (3840x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_24HZ,  "4K 24Hz (4096x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_25HZ,  "4K 25Hz (4096x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_30HZ,  "4K 30Hz (4096x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_60HZ,  "4K 60Hz (3840x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_60HZ,  "4K 60Hz (4096x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_50HZ,  "4K 50Hz (3840x2160)");
        map.put(DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_50HZ,  "4K 50Hz (4096x2160)");

        return map;
    }

    private DisplayOutputManager mDisplayManager;
    private int mDisplay;

    public DisplayOutputImpl() {
        mDisplayManager = DisplayOutputManager.getInstance();
    }

    public void updateSupportModes(ListPreference lp, int type) {
        Log.d(TAG, "updateSupportModes  type = " + type);
        mDisplay = -1;
        for (int i = 0; i < 2; i++) {
            if (mDisplayManager.getDisplayOutputType(i) == type) {
                mDisplay = i;
                break;
            }
        }

        int[] values = mDisplayManager.getSupportModes(mDisplay, 0);
        final CharSequence[] entries;
        final CharSequence[] entryValues;

        if(type == TYPE_HDMI) {
            entries = new CharSequence[values.length + 1];
            entryValues = new CharSequence[values.length + 1];
            DisplayOutputManager.SinkInfo info = mDisplayManager.getSinkInfo(mDisplay);
            Log.d(TAG,"updateSupportModes = " + info.CurrentMode + " , " + info.UserSetting + " , " + info.IsNative);
            if(info.IsNative == true && info.UserSetting == 255)
                entries[0] = "AUTO(" + mDispFormatNameMap.get(info.CurrentMode) + ")";
            else
                entries[0] = "AUTO";

            entryValues[0] = "255";

            for (int i = 0; i < values.length; i++) {
                entries[i + 1] = mDispFormatNameMap.get(values[i]);
                entryValues[i + 1] = String.valueOf(values[i]);
            }
        } else {
            entries = new CharSequence[values.length];
            entryValues = new CharSequence[values.length];
            for (int i = 0; i < values.length; i++) {
                entries[i] = mDispFormatNameMap.get(values[i]);
                entryValues[i] = String.valueOf(values[i]);
            }
        }
        lp.setEntries(entries);
        lp.setEntryValues(entryValues);
    }

    public void updateDisplayDevice(int type) {
        Log.d(TAG,"updateDisplayDevice  type = " + type);
        mDisplay = -1;
        for (int i = 0; i < 2; i++) {
            if (mDisplayManager.getDisplayOutputType(i) == type) {
                mDisplay = i;
                break;
            }
        }
    }

    public int getDisplayOutputMode() {
        return mDisplayManager.getDisplayOutputMode(mDisplay);
    }

    public int getDisplayOutputModeHdmi() {
        DisplayOutputManager.SinkInfo info = mDisplayManager.getSinkInfo(mDisplay);
        Log.d(TAG,"getDisplayOutputMode = " + info.CurrentMode + " , " + info.UserSetting + " , " + info.IsNative);
        if(info.IsNative == true && info.UserSetting == 255)
            return info.UserSetting;
        else
            return info.CurrentMode;
    }
    public int getCvbsOutputMode() {
        return mDisplayManager.getDisplayOutputMode(mDisplay);
    }
    public int setDisplayOutputMode(int mode) {
        Log.d(TAG,"setDisplayOutputMode = " + mode);
        return mDisplayManager.setDisplayOutputMode(mDisplay, mode);
    }

    public void setDisplayPercent(int value) {
        mDisplayManager.setDisplayMargin(mDisplay, value,value);
    }

    public void setDisplayMarginHorizontalPercent(int value) {
        mDisplayManager.setDisplayMargin(mDisplay, value, getDisplayVerticalPercent());
    }

    public void setDisplayMarginVerticalPercent(int value) {
        mDisplayManager.setDisplayMargin(mDisplay, getDisplayHorizontalPercent(), value);
    }

    public int getDisplayPercent() {
        return mDisplayManager.getDisplayMargin(mDisplay)[0];
    }

    public int getDisplayHorizontalPercent() {
        return mDisplayManager.getDisplayMargin(mDisplay)[0];
    }

    public int getDisplayVerticalPercent() {
        return mDisplayManager.getDisplayMargin(mDisplay)[2];
    }
}
