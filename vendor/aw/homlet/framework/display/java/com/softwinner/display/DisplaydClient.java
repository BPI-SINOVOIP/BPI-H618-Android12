
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

package com.softwinner.display;

import android.os.HwBinder;
import android.os.RemoteException;
import android.util.Log;
import android.util.Slog;
import android.content.Context;
import android.hidl.manager.V1_0.IServiceManager;
import android.hidl.manager.V1_0.IServiceNotification;

import java.lang.StringBuilder;
import java.util.ArrayList;
import java.util.NoSuchElementException;

import vendor.display.config.V1_0.IDisplayConfig;
import vendor.display.config.V1_0.DisplayPortType;
import vendor.display.config.V1_0.PixelFormat;
import vendor.display.config.V1_0.AspectRatio;
import vendor.display.config.V1_0.Dataspace;
import vendor.display.config.V1_0.LayerMode;
import vendor.display.config.V1_0.EnhanceItem;
import vendor.display.config.V1_0.ScreenMargin;
import vendor.display.config.V1_0.SNRInfo;
import vendor.display.config.V1_0.SNRFeatureMode;
import vendor.display.config.V1_0.HdcpLevel;
import vendor.display.config.V1_0.HdcpAuthorizedStatus;

public class DisplaydClient {
    public static final int SUCCESS =  0;
    public static final int ERROR   = -1;

    public static final int IFACE_TYPE_NONE    = DisplayPortType.DISPLAY_PORT_NONE;
    public static final int IFACE_TYPE_LCD     = DisplayPortType.DISPLAY_PORT_LCD;
    public static final int IFACE_TYPE_CVBS    = DisplayPortType.DISPLAY_PORT_CVBS;
    public static final int IFACE_TYPE_HDMI    = DisplayPortType.DISPLAY_PORT_HDMI;
    public static final int IFACE_TYPE_VGA     = DisplayPortType.DISPLAY_PORT_VGA;

    public static final int ENHANCE_MODE       = EnhanceItem.ENHANCE_MODE;
    public static final int ENHANCE_BRIGHT     = EnhanceItem.ENHANCE_BRIGHT;
    public static final int ENHANCE_CONTRAST   = EnhanceItem.ENHANCE_CONTRAST;
    public static final int ENHANCE_DENOISE    = EnhanceItem.ENHANCE_DENOISE;
    public static final int ENHANCE_DETAIL     = EnhanceItem.ENHANCE_DETAIL;
    public static final int ENHANCE_EDGE       = EnhanceItem.ENHANCE_EDGE;
    public static final int ENHANCE_SATURATION = EnhanceItem.ENHANCE_SATURATION;

    public static final int SNR_DISABLE = SNRFeatureMode.SNR_DISABLE;
    public static final int SNR_LEVEL1  = SNRFeatureMode.SNR_LEVEL1;
    public static final int SNR_LEVEL2  = SNRFeatureMode.SNR_LEVEL2;
    public static final int SNR_LEVEL3  = SNRFeatureMode.SNR_LEVEL3;
    public static final int SNR_CUSTOM  = SNRFeatureMode.SNR_CUSTOM;
    public static final int SNR_DEMO    = SNRFeatureMode.SNR_DEMO;

    public static final int HDCP_LEVEL_UNKNOWN   = HdcpLevel.HDCP_UNKNOWN;
    public static final int HDCP_LEVEL_NONE      = HdcpLevel.HDCP_NONE;
    public static final int HDCP_LEVEL_V1        = HdcpLevel.HDCP_V1;
    public static final int HDCP_LEVEL_V2        = HdcpLevel.HDCP_V2;
    public static final int HDCP_LEVEL_V2_1      = HdcpLevel.HDCP_V2_1;
    public static final int HDCP_LEVEL_V2_2      = HdcpLevel.HDCP_V2_2;
    public static final int HDCP_LEVEL_NO_OUTPUT = HdcpLevel.HDCP_NO_OUTPUT;

    public static final int HDCP_STATE_ERROR         = HdcpAuthorizedStatus.ERROR;
    public static final int HDCP_STATE_UN_AUTHORIZED = HdcpAuthorizedStatus.UN_AUTHORIZED;
    public static final int HDCP_STATE_AUTHORIZED    = HdcpAuthorizedStatus.AUTHORIZED;

    private static boolean DEBUG_ON = true;
    private static final String TAG = "DisplaydClient";
    private static final int DISPLAYD_HAL_DEATH_COOKIE = 1000;

    // The system context.
    private final Context mContext;

    // Proxy object for the Displayd hal daemon.
    private IDisplayConfig mProxy = null;
    private final Object mLock = new Object();

    public DisplaydClient(Context context) {
        mContext = context;
        try {
            ServiceNotification serviceNotification = new ServiceNotification();
            boolean ret = IServiceManager.getService()
                    .registerForNotifications(
                            "vendor.display.config@1.0::IDisplayConfig",
                            "", serviceNotification);
            if (!ret) {
                Slog.e(TAG, "Failed to register service start notification");
            }
        } catch (RemoteException e) {
            Slog.e(TAG, "Failed to register service start notification:", e);
            return;
        }
        connectToProxy();
    }

    final class DeathRecipient implements HwBinder.DeathRecipient {
        DeathRecipient() {
            Slog.e(TAG, "new DeathRecipient for Displayd");
        }

        @Override
        public void serviceDied(long cookie) {
            if (cookie == DISPLAYD_HAL_DEATH_COOKIE) {
                Slog.e(TAG, "Displayd hal service died cookie: " + cookie);
                synchronized (mLock) {
                    mProxy = null;
                }
            }
        }
    }

    final class ServiceNotification extends IServiceNotification.Stub {
        @Override
        public void onRegistration(String fqName, String name, boolean preexisting) {
            Slog.e(TAG, "Displayd hal service started " + fqName + " " + name);
            connectToProxy();
        }
    }

    private void connectToProxy() {
        synchronized (mLock) {
            if (mProxy != null) {
                return;
            }

            try {
                mProxy = IDisplayConfig.getService();
                mProxy.linkToDeath(new DeathRecipient(), DISPLAYD_HAL_DEATH_COOKIE);
            } catch (NoSuchElementException e) {
                Slog.e(TAG, "connectToProxy: Displayd hal service not found."
                        + " Did the service fail to start?", e);
            } catch (RemoteException e) {
                Slog.e(TAG, "connectToProxy: Displayd hal service not responding", e);
            }
        }
    }

    private static void debug(String msg) {
        if (DEBUG_ON)
            Slog.d(TAG, msg);
    }

    public int getType(int display) {
        int type = DisplayPortType.DISPLAY_PORT_NONE;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return type;
            }
            try {
                type = mProxy.getDisplayPortType(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getType:", e);
            }
        }
        return type;
    }

    private class ActiveModeCallback implements IDisplayConfig.getActiveModeCallback {
        public int mError = 0;
        public int mActiveMode = 0;
        public void onValues(int error, int mode) {
            mError = error;
            mActiveMode = mode;
        }
    }

    public int getMode(int display) {
        ActiveModeCallback cb = new ActiveModeCallback();
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return -1;
            }
            try {
                mProxy.getActiveMode(display, cb);
            } catch (RemoteException e) {
                Slog.e(TAG, "getMode:", e);
            }
        }
        debug("Display." + display + " mode: " + cb.mActiveMode);
        if (cb.mError == 0)
            return cb.mActiveMode;
        return -1;
    }

    public int setMode(int display, int fmt) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setActiveMode(display, fmt);
            } catch (RemoteException e) {
                Slog.e(TAG, "setMode:", e);
            }
        }
        debug("Display." + display + " setMode: " + fmt + " return: " + ret);
        return ret;
    }

    public boolean isSupportedMode(int display, int fmt) {
        ArrayList<Integer> modes = getSupportedModes(display);
        boolean supported = modes.contains(fmt);
        debug("Display." + display + " support: " + fmt + " return " + supported);
        return supported;
    }

    public ArrayList<Integer> getSupportedModes(int display) {
        ArrayList<Integer> modes = null;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return modes;
            }
            try {
                modes = mProxy.getSupportedModes(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getSupportedModes:", e);
            }
        }
        return modes;
    }

    public boolean isSupported3D(int display) {
        boolean ret = false;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return false;
            }
            try {
                ret = mProxy.supported3D(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "isSupported3D:", e);
            }
        }
        debug("Display." + display + " support 3D: " + ret);
        return ret;
    }

    public int get3DLayerMode(int display) {
        int lm = LayerMode.LAYER_2D_ORIGINAL;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return lm;
            }
            try {
                lm = mProxy.get3DLayerMode(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "get3DLayerMode:", e);
            }
        }
        debug("Display." + display + " layer mode: " + LayerMode.toString(lm));
        return lm;
    }

    public int set3DLayerMode(int display, int mode) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.set3DLayerMode(display, mode);
            } catch (RemoteException e) {
                Slog.e(TAG, "set3DLayerMode:", e);
            }
        }
        debug("Display." + display + " set3DLayerMode: " +
                LayerMode.toString(mode) + " return: " + ret);
        return ret;
    }

    public int getAspectRatio(int display) {
        int ratio = AspectRatio.ASPECT_RATIO_AUTO;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ratio;
            }
            try {
                ratio = mProxy.getAspectRatio(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getAspectRatio:", e);
            }
        }
        debug("Display." + display + " AspectRatio: " + AspectRatio.toString(ratio));
        return ratio;
    }

    public int setAspectRatio(int display, int ratio) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setAspectRatio(display, ratio);
            } catch (RemoteException e) {
                Slog.e(TAG, "setAspectRatio:", e);
            }
        }
        debug("Display." + display + " setAspectRatio: " +
                AspectRatio.toString(ratio) + " return: " + ret);
        return ret;
    }

    public ScreenMargin getMargin(int display) {
        ScreenMargin margin = null;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return margin;
            }
            try {
                margin = mProxy.getScreenMargin(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getMargin:", e);
            }
        }
        debug("Display." + display + " margin: " + margin.toString());
        return margin;
    }

    public int setMargin(int display, ScreenMargin margin) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setScreenMargin(display, margin);
            } catch (RemoteException e) {
                Slog.e(TAG, "setMargin:", e);
            }
        }
        debug("Display." + display + " setMargin: " +
                margin.toString() + " return: " + ret);
        return ret;
    }

    public ArrayList<Integer> getSupportedPixelFormats(int display) {
        ArrayList<Integer> fmts = null;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return fmts;
            }
            try {
                fmts = mProxy.getSupportedPixelFormats(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getSupportedPixelFormats:", e);
            }
        }
        return fmts;
    }

    public int getPixelFormat(int display) {
        int fmt = PixelFormat.PIXEL_FORMAT_AUTO;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return fmt;
            }
            try {
                fmt = mProxy.getPixelFormat(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getPixelFormat:", e);
            }
        }
        debug("Display." + display + " PixelFormat: " + PixelFormat.toString(fmt));
        return fmt;
    }

    public int setPixelFormat(int display, int fmt) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setPixelFormat(display, fmt);
            } catch (RemoteException e) {
                Slog.e(TAG, "setPixelFormat:", e);
            }
        }
        debug("Display." + display + " setPixelFormat: " +
                PixelFormat.toString(fmt) + " return: " + ret);
        return ret;
    }

    public int getCurrentDataspace(int display) {
        int mode = Dataspace.DATASPACE_MODE_AUTO;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return mode;
            }
            try {
                mode = mProxy.getCurrentDataspace(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getCurrentDataspace:", e);
            }
        }
        debug("Display." + display + " current Dataspace: " +
                Dataspace.toString(mode));
        return mode;
    }

    public int getDataspaceMode(int display) {
        int mode = Dataspace.DATASPACE_MODE_AUTO;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return mode;
            }
            try {
                mode = mProxy.getDataspaceMode(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getDataspaceMode:", e);
            }
        }
        debug("Display." + display + " DataspaceMode: " + Dataspace.toString(mode));
        return mode;
    }

    public int setDataspaceMode(int display, int mode) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setDataspaceMode(display, mode);
            } catch (RemoteException e) {
                Slog.e(TAG, "setDataspaceMode:", e);
            }
        }
        debug("Display." + display + " setDataspaceMode: " +
                Dataspace.toString(mode) + " return: " + ret);
        return ret;
    }

    public int getEnhanceComponent(int display, int item) {
        int value = 0;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return value;
            }
            try {
                value = mProxy.getEnhanceComponent(display, item);
            } catch (RemoteException e) {
                Slog.e(TAG, "getEnhanceComponent:", e);
            }
        }
        debug("Display." + display + " Enhance: " +
                EnhanceItem.toString(item) + " value: " + value);
        return value;
    }

    public int setEnhanceComponent(int display, int item, int value) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setEnhanceComponent(display, item, value);
            } catch (RemoteException e) {
                Slog.e(TAG, "setEnhanceComponent:", e);
            }
        }
        debug("Display." + display + " setEnhanceComponent: " +
                EnhanceItem.toString(item) + " value(" + value + ") return: " + ret);
        return ret;
    }

    public boolean supportedSNRSetting(int display) {
        boolean ret = false;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return false;
            }
            try {
                ret = mProxy.supportedSNRSetting(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "supportedSNRSetting:", e);
            }
        }
        debug("Display." + display + " support SNR: " + ret);
        return ret;
    }

    private class SNRInfoCallback implements IDisplayConfig.getSNRInfoCallback {
        public int mError = 0;
        public SNRInfo mInfo;
        public void onValues(int error, SNRInfo info) {
            mError = error;
            mInfo = info;
        }
    }

    public SNRInfo getSNRInfo(int display) {
        SNRInfoCallback cb = new SNRInfoCallback();
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return cb.mInfo;
            }
            try {
                mProxy.getSNRInfo(display, cb);
            } catch (RemoteException e) {
                Slog.e(TAG, "getSNRInfo:", e);
            }
        }
        debug("Display." + display + " getSNRInfo: " + cb.mInfo.toString());
        return cb.mInfo;
    }

    public int setSNRInfo(int display, SNRInfo info) {
        int ret = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ERROR;
            }
            try {
                ret = mProxy.setSNRInfo(display, info);
            } catch (RemoteException e) {
                Slog.e(TAG, "setSNRInfo:", e);
            }
        }
        debug("Display." + display + " setSNRInfo: " + info.toString() + " return: " + ret);
        return ret;
    }

    public int getHdmiNativeMode(int display) {
        int mode = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return mode;
            }
            try {
                mode = mProxy.getHDMINativeMode(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getHdmiNativeMode:", e);
            }
        }
        debug("Display." + display + " native mode: " + mode);
        return mode;
    }

    public int getHdmiUserSetting(int display) {
        int setting = ERROR;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return setting;
            }
            try {
                setting = mProxy.getHdmiUserSetting(display);
            } catch (RemoteException e) {
                Slog.e(TAG, "getHdmiUserSetting:", e);
            }
        }
        debug("Display." + display + " user setting: " + setting);
        return setting;
    }

    public int configHdcp(boolean enable) {
        int ret = 0;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ret;
            }
            try {
                ret = mProxy.configHdcp(enable);
            } catch (RemoteException e) {
                Slog.e(TAG, "configHdcp:", e);
            }
        }
        debug("Display configHdcp return: " + ret);
        return ret;
    }

    public int getConnectedHdcpLevel() {
        int ret = 0;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ret;
            }
            try {
                ret = mProxy.getConnectedHdcpLevel();
            } catch (RemoteException e) {
                Slog.e(TAG, "getConnectedHdcpLevel:", e);
            }
        }
        debug("Display getConnectedHdcpLevel return: " + HdcpLevel.toString(ret));
        return ret;
    }

    public int getAuthorizedStatus() {
        int ret = 0;
        synchronized (mLock) {
            if (mProxy == null) {
                Slog.d(TAG, "DisplaydClient: mProxy is null");
                return ret;
            }
            try {
                ret = mProxy.getAuthorizedStatus();
            } catch (RemoteException e) {
                Slog.e(TAG, "getAuthorizedStatus:", e);
            }
        }
        debug("Display getAuthorizedStatus return: " + HdcpAuthorizedStatus.toString(ret));
        return ret;
    }

    public void dump() {
        int id = 0;
        StringBuilder builder = new StringBuilder();
        builder.append("\n");
        builder.append("Primary Display info:\n");
        builder.append("\tType     : " + DisplayPortType.toString(getType(id))  + "\n");
        builder.append("\tMode     : " + String.valueOf(getMode(id)) + "\n");
        builder.append("\tMargin   : " + getMargin(id).toString() + "\n");
        builder.append("\tSampling : " + PixelFormat.toString(getPixelFormat(id)) + "\n");
        builder.append("\tDataspace: " + Dataspace.toString(getDataspaceMode(id)) + "\n");

        ArrayList<Integer> fmts = getSupportedModes(id);
        builder.append("\tSupported Modes(" + fmts.size() + "): ");
        for (int i = 0; i < fmts.size(); i++) {
            builder.append(String.valueOf(fmts.get(i)));
            builder.append(" ");
        }
        builder.append("\n");

        ArrayList<Integer> pfmts = getSupportedPixelFormats(id);
        builder.append("\tSupported PixelFormats(" + pfmts.size() + "): ");
        for (int i = 0; i < pfmts.size(); i++) {
            builder.append(PixelFormat.toString(pfmts.get(i)));
            builder.append(" ");
        }
        builder.append("\n");

        Slog.e(TAG, builder.toString());
    }
}

