

package com.android.settings.display;

import android.util.Log;
import android.util.Slog;
import android.content.Context;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import java.util.Map;
import java.util.HashMap;
import java.util.ArrayList;

import com.softwinner.display.DisplaydClient;
import softwinner.homlet.displayd.V1_0.DispFormat;

public class DisplayOutputImpl {
    public static int TYPE_HDMI = DisplaydClient.IFACE_TYPE_HDMI;
    public static int TYPE_CVBS = DisplaydClient.IFACE_TYPE_CVBS;

    private static final String TAG = "DisplaySetting";
    private static final HashMap<Integer, String> mDispFormatNameMap = createDispFormatMap();

    private DisplaydClient mDispdClient = null;

    private final Object mLock = new Object();

    class IfaceContext {
        public static final int IDEL = 0;
        public static final int WAITING_CONFIRM = 1;

        public int type;
        public int displayIndex;
        public int targetMode;
        public int modeBeforeSwitching;
        public int opState;

        public ArrayList<Integer> supportedModes;
    };

    /* type to interface map */
    private HashMap<Integer, IfaceContext> mIfaces = new HashMap<Integer, IfaceContext>();
    /* Preference to interface map */
    private HashMap<Preference, IfaceContext> mPreferenceItems = new HashMap<Preference, IfaceContext>();

    public DisplayOutputImpl() {
        mDispdClient = new DisplaydClient(null);
        updateSupportedModeList();
    }

    public void addPreference(Preference p, int type) {
        synchronized (mLock) {
            if (mPreferenceItems.containsKey(p)) {
                mPreferenceItems.remove(p);
                Slog.d(TAG, "Remove preference(" +
                            p.getKey() + ") for type(" + mPreferenceItems.get(p) + ")");
            }
            mPreferenceItems.put(p, mIfaces.get(type));
            Slog.d(TAG, "Add Preference for type(" + type + "): " + p.getKey());

            final IfaceContext c = mIfaces.get(type);
            final int size = c.supportedModes.size();
            final CharSequence[] entries = new CharSequence[size];
            final CharSequence[] entryValues = new CharSequence[size];

            for (int i = 0; i < size; i++) {
                entries[i] = mDispFormatNameMap.get(c.supportedModes.get(i));
                entryValues[i] = c.supportedModes.get(i).toString();
            }
            ListPreference lp = (ListPreference)p;
            lp.setEntries(entries);
            lp.setEntryValues(entryValues);
        }
    }

    private void updateSupportedModeList() {
        synchronized (mLock) {
            for (int i = 0; i <= 1; i++) {
                int type = mDispdClient.getType(i);
                if (type != DisplaydClient.IFACE_TYPE_HDMI
                        && type != DisplaydClient.IFACE_TYPE_CVBS) {
                    Slog.d(TAG, "Not support iface type(" + type + ")");
                    return;
                }

                IfaceContext iface = mIfaces.get(type);
                if (iface == null) {
                    iface = new IfaceContext();
                    mIfaces.put(type, iface);
                }

                iface.type = type;
                iface.displayIndex = i;
                iface.targetMode = -1;
                iface.opState = IfaceContext.IDEL;
                iface.modeBeforeSwitching = mDispdClient.getMode(i);
                iface.supportedModes = mDispdClient.getSupportedModes(i);
            }
        }
    }

    int onModeSwitch(Preference preference, Object value) {
        synchronized (mLock) {
            if (!mPreferenceItems.containsKey(preference)) {
                Slog.e(TAG, "Should not got here!");
                return 0;
            }

            IfaceContext iface = mPreferenceItems.get(preference);
            iface.targetMode = Integer.parseInt((String)value);
            iface.modeBeforeSwitching = mDispdClient.getMode(iface.displayIndex);

            Slog.d(TAG, "Display." + iface.displayIndex
                    + "Mode switch: " + mDispFormatNameMap.get(iface.modeBeforeSwitching)
                    + " ---> " + mDispFormatNameMap.get(iface.targetMode));

            if (iface.targetMode == iface.modeBeforeSwitching)
                return 0;
            if (mDispdClient.setMode(iface.displayIndex, iface.targetMode) == 0) {
                iface.opState = IfaceContext.WAITING_CONFIRM;
            }
        }
        return 0;
    }

    public String getCurrentValue(Preference preference) {
        synchronized (mLock) {
            if (!mPreferenceItems.containsKey(preference)) {
                Slog.e(TAG, "Not such preference: " + preference.getKey());
                return null;
            }
            IfaceContext iface = mPreferenceItems.get(preference);
            Integer mode = mDispdClient.getMode(iface.displayIndex);
            return mode.toString();
        }
    }

    int confirmSwitch() {
        for (Map.Entry<Integer, IfaceContext> entry : mIfaces.entrySet()) {
            IfaceContext iface = entry.getValue();
            if (iface.opState == IfaceContext.WAITING_CONFIRM) {
                iface.opState = IfaceContext.IDEL;
            }
        }
        return 0;
    }

    int cancelSwitch() {
        for (Map.Entry<Integer, IfaceContext> entry : mIfaces.entrySet()) {
            IfaceContext iface = entry.getValue();
            if (iface.opState == IfaceContext.WAITING_CONFIRM) {
                mDispdClient.setMode(iface.displayIndex, iface.modeBeforeSwitching);
                iface.opState = IfaceContext.IDEL;
            }
        }
        return 0;
    }

    private static HashMap<Integer, String> createDispFormatMap() {
        HashMap<Integer, String> map = new HashMap<Integer, String>();

        map.put(DispFormat.DISP_FMT_480I,             " 480i"              );
        map.put(DispFormat.DISP_FMT_576I,             " 576i"              );
        map.put(DispFormat.DISP_FMT_480P,             " 480p"              );
        map.put(DispFormat.DISP_FMT_576P,             " 576p"              );
        map.put(DispFormat.DISP_FMT_720P_50HZ,        " 720p 50Hz"         );
        map.put(DispFormat.DISP_FMT_720P_60HZ,        " 720p 60Hz"         );
        map.put(DispFormat.DISP_FMT_1080I_50HZ,       "1080i 50Hz"         );
        map.put(DispFormat.DISP_FMT_1080I_60HZ,       "1080i 60Hz"         );
        map.put(DispFormat.DISP_FMT_1080P_24HZ,       "1080p 24Hz"         );
        map.put(DispFormat.DISP_FMT_1080P_50HZ,       "1080p 50Hz"         );
        map.put(DispFormat.DISP_FMT_1080P_60HZ,       "1080p 60Hz"         );
        map.put(DispFormat.DISP_FMT_1080P_24HZ_3D_FP, "1080p 24Hz (3D FP)" );
        map.put(DispFormat.DISP_FMT_720P_50HZ_3D_FP,  " 720p 50Hz (3D FP)" );
        map.put(DispFormat.DISP_FMT_720P_60HZ_3D_FP,  " 720p 60Hz (3D FP)" );
        map.put(DispFormat.DISP_FMT_1080P_25HZ,       "1080p 25Hz"         );
        map.put(DispFormat.DISP_FMT_1080P_30HZ,       "1080p 30Hz"         );
        map.put(DispFormat.DISP_FMT_PAL,              "PAL"                );
        map.put(DispFormat.DISP_FMT_PAL_SVIDEO,       "NTSC SVIDEO"        );
        map.put(DispFormat.DISP_FMT_NTSC,             "NTSC"               );
        map.put(DispFormat.DISP_FMT_NTSC_SVIDEO,      "NTSC SVIDEO"        );
        map.put(DispFormat.DISP_FMT_PAL_M,            "PAL M"              );
        map.put(DispFormat.DISP_FMT_PAL_M_SVIDEO,     "PAL M SVIDEO"       );
        map.put(DispFormat.DISP_FMT_PAL_NC,           "PAL NC"             );
        map.put(DispFormat.DISP_FMT_PAL_NC_SVIDEO,    "PAL NC SVIDEO"      );
        map.put(DispFormat.DISP_FMT_3840_2160P_30HZ,  "4K 30Hz (3840x2160)");
        map.put(DispFormat.DISP_FMT_3840_2160P_25HZ,  "4K 25Hz (3840x2160)");
        map.put(DispFormat.DISP_FMT_3840_2160P_24HZ,  "4K 24Hz (3840x2160)");
        map.put(DispFormat.DISP_FMT_4096_2160P_24HZ,  "4K 24Hz (4096x2160)");
        map.put(DispFormat.DISP_FMT_4096_2160P_25HZ,  "4K 25Hz (4096x2160)");
        map.put(DispFormat.DISP_FMT_4096_2160P_30HZ,  "4K 30Hz (4096x2160)");
        map.put(DispFormat.DISP_FMT_3840_2160P_60HZ,  "4K 60Hz (3840x2160)");
        map.put(DispFormat.DISP_FMT_4096_2160P_60HZ,  "4K 60Hz (4096x2160)");
        map.put(DispFormat.DISP_FMT_3840_2160P_50HZ,  "4K 50Hz (3840x2160)");
        map.put(DispFormat.DISP_FMT_4096_2160P_50HZ,  "4K 50Hz (4096x2160)");

        return map;
    }
}
