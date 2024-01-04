/*
 * Copyright (C) 2020 The Android Open Source Project
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

package vendor.display;

import android.os.Binder;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.util.Log;

import vendor.display.output.IDisplayOutputManager;
import vendor.display.DisplayOutputManager.SinkInfo;

import static vendor.display.DisplayOutputManager.DISPLAY_TVFORMAT_1080P_24HZ_3D_FP;
import static vendor.display.DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI;
import static vendor.display.DisplayOutputManager.ENHANCE_MODE;
import static vendor.display.DisplayOutputManager.ENHANCE_BRIGHT;
import static vendor.display.DisplayOutputManager.ENHANCE_CONTRAST;
import static vendor.display.DisplayOutputManager.ENHANCE_DENOISE;
import static vendor.display.DisplayOutputManager.ENHANCE_DETAIL;
import static vendor.display.DisplayOutputManager.ENHANCE_EDGE;
import static vendor.display.DisplayOutputManager.ENHANCE_SATURATION;

public class DisplayOutputManagerImpl {
    private String TAG = "DisplayOutputManagerImpl";
    private IDisplayOutputManager mService = null;
    private static boolean DEBUG_ON = false;

    public void setTAG(String tag) {
        TAG = tag;
    }

    public DisplayOutputManagerImpl() {
        if (mService != null){
            return;
        }

        IBinder binder = Binder.allowBlocking(ServiceManager.waitForDeclaredService(
                    "vendor.display.output.IDisplayOutputManager/default"));
        if (binder != null) {
            mService = IDisplayOutputManager.Stub.asInterface(binder);
            Log.d(TAG, "get IDisplayOutputManager service success");
        } else {
            Log.d(TAG, "get IDisplayOutputManager service failed");
        }
    }

    private void debug(String msg) {
        if (DEBUG_ON)
            Log.d(TAG, msg);
    }

    /*@hide*/
    public static final int DISPLAY_OUTPUT_TYPE_MASK = 0xff00;
    /*@hide*/
    public static final int DISPLAY_OUTPUT_MODE_MASK = 0xff;

    interface IntCmd {
        int run() throws RemoteException;
    }

    interface BooleanCmd {
        boolean run() throws RemoteException;
    }

    private int runIntCmd(IntCmd cmd, int display, String method, String param) {
        int retval = -1;
        if (mService != null) {
            try {
                retval = cmd.run();
            } catch (RemoteException e) {
                throw e.rethrowFromSystemServer();
            }
        }
        debug("Display." + display + " " + method + ": param (" + param + ") value(" + retval + ")");
        return retval;
    }

    private boolean runBooleanCmd(BooleanCmd cmd, int display, String method, String param) {
        boolean retval = false;
        if (mService != null) {
            try {
                retval = cmd.run();
            } catch (RemoteException e) {
                throw e.rethrowFromSystemServer();
            }
        }
        debug("Display." + display + " " + method + ": param (" + param + ") value(" + retval + ")");
        return retval;
    }

    public int getDisplayOutput(int display) {
        return runIntCmd(() -> { return mService.getDisplayOutput(display);}, display, "getDisplayOutput", null);
    }

    public int setDisplayOutput(int display, int format) {
        return runIntCmd(() -> { return mService.setDisplayOutput(display, format);}, display, "setDisplayOutput", "format = " + format);
    }

    public int getDisplayOutputType(int display) {
        return runIntCmd(() -> { return mService.getDisplayOutputType(display);}, display, "getDisplayOutputType", null);
    }

    public int getDisplayOutputMode(int display){
        return runIntCmd(() -> { return mService.getDisplayOutputMode(display);}, display, "getDisplayOutputMode", null);
    }

    public int setDisplayOutputMode(int display, int mode){
        return runIntCmd(() -> { return mService.setDisplayOutputMode(display, mode);}, display, "setDisplayOutputMode", "mode = " + mode);
    }

    public int getDisplayOutputPixelFormat(int display) {
        return runIntCmd(() -> { return mService.getDisplayOutputPixelFormat(display);}, display, "getDisplayOutputPixelFormat", null);
    }

    public int setDisplayOutputPixelFormat(int display, int format) {
        return runIntCmd(() -> { return mService.setDisplayOutputPixelFormat(display, format);}, display, "setDisplayOutputPixelFormat", "format = " + format);
    }

    public int getDisplayOutputCurDataspaceMode(int display) {
        return runIntCmd(() -> { return mService.getDisplayOutputCurDataspaceMode(display);}, display, "getDisplayOutputCurDataspaceMode", null);
    }

    public int getDisplayOutputDataspaceMode(int display) {
        return runIntCmd(() -> { return mService.getDisplayOutputDataspaceMode(display);}, display, "getDisplayOutputDataspaceMode", null);
    }

    public int setDisplayOutputDataspaceMode(int display, int mode) {
        return runIntCmd(() -> { return mService.setDisplayOutputDataspaceMode(display, mode);}, display, "setDisplayOutputDataspaceMode", "mode = " + mode);
    }

    public boolean isSupportHdmiMode(int display, int mode) {
        return runBooleanCmd(() -> { return mService.isSupportHdmiMode(display, mode);}, display, "isSupportHdmiMode", "mode = " + mode);
    }

    public int[] getSupportModes(int display, int type) {
        int[] retval = null;
        try {
          retval = mService.getSupportModes(display,type);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

     /* interface for 3D mode setting */
    public int getDisplaySupport3DMode(int display) {
        return runIntCmd(() -> { return mService.getDisplaySupport3DMode(display);}, display, "getDisplaySupport3DMode", null);
    }

    public int setDisplay3DMode(int display, int mode){
        return runIntCmd(() -> { return mService.setDisplay3DMode(display, mode, 0);}, display, "setDisplay3DMode", "mode = " + mode);
    }

    public int setDisplay3DMode(int display, int mode, int videoCropHeight) {
        return runIntCmd(() -> { return mService.setDisplay3DMode(display, mode, videoCropHeight);}, display, "setDisplay3DMode",
                        "mode = " + mode + ", videoCropHeight = " + videoCropHeight);
    }

    public int setDisplay3DLayerOffset(int display, int offset) {
        return runIntCmd(() -> { return mService.setDisplay3DLayerOffset(display, offset);}, display, "setDisplay3DLayerOffset", "offset = " + offset);
    }

    public boolean isCurrent3Doutput(int display) {
        if (getDisplayOutputMode(display) == DISPLAY_TVFORMAT_1080P_24HZ_3D_FP)
            return true;
        else
            return false;
    }

     /* interface for screen margin/offset setting */
    public int[] getDisplayMargin(int display) {
        int[] retval = null;
        try {
            retval = mService.getDisplayMargin(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int[] getDisplayOffset(int display) {
        int[] retval = null;
        try {
          retval = mService.getDisplayOffset(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayMargin(int display, int margin_x, int margin_y) {
        return runIntCmd(() -> { return mService.setDisplayMargin(display, margin_x, margin_y);}, display, "setDisplayMargin",
                                "margin_x = " + margin_x + ", margin_y = " + margin_y);
    }

    public boolean getHdmiFullscreen(int display) {
        return runBooleanCmd(() -> { return mService.getHdmiFullscreen(display);}, display, "getHdmiFullscreen", null);
    }

    public int setHdmiFullscreen(int display, boolean full) {
        return runIntCmd(() -> { return mService.setHdmiFullscreen(display, full);}, display, "setHdmiFullscreen", "full = " + full);
    }

    public int getDisplay3DMode(int display) {
        return runIntCmd(() -> { return mService.getDisplay3DMode(display);}, display, "getDisplay3DMode", null);
    }

    public int setDisplayOffset(int display, int offset_x, int offset_y){
        return runIntCmd(() -> { return mService.setDisplayOffset(display, offset_x, offset_y);}, display, "setDisplayOffset",
                                "offset_x = " + offset_x + ", offset_y = " + offset_y);
    }

     /* interface for display enhance effect */
    public int getDisplayEdge(int display) {
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_EDGE);}, display, "getDisplayEdge", null);
    }

    public int setDisplayEdge(int display, int value) {
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_EDGE, value);}, display, "setDisplayEdge", "edge = " + value);
    }

    public int getDisplayDetail(int display) {
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_DETAIL);}, display, "getDisplayDetail", null);
    }

    public int setDisplayDetail(int display, int value) {
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_DETAIL, value);}, display, "setDisplayDetail", "detail = " + value);
    }

    public int getDisplayBright(int display) {
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_BRIGHT);}, display, "getDisplayBright", null);
    }

    public int setDisplayBright(int display, int value){
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_BRIGHT, value);}, display, "setDisplayBright", "bright = " + value);
    }

    public int getDisplayDenoise(int display){
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_DENOISE);}, display, "getDisplayDenoise", null);
    }

    public int setDisplayDenoise(int display, int value){
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_DENOISE, value);}, display, "setDisplayDenoise", "denoise = " + value);
    }

    public boolean getBlackWhiteMode(int display) {
        return runBooleanCmd(() -> { return mService.getBlackWhiteMode(display);}, display, "getBlackWhiteMode", null);
    }

    public int setBlackWhiteMode(int display, boolean on) {
        return runIntCmd(() -> { return mService.setBlackWhiteMode(display, on);}, display, "setBlackWhiteMode", "on = " + on);
    }

    public int getDisplaySaturation(int display) {
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_SATURATION);}, display, "getDisplaySaturation", null);
    }

    public int setDisplaySaturation(int display, int value) {
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_SATURATION, value);}, display, "setDisplaySaturation", "value = " + value);
    }

    public int getDisplayContrast(int display) {
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_CONTRAST);}, display, "getDisplayContrast", null);
    }

    public int setDisplayContrast(int display, int value) {
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_CONTRAST, value);}, display, "setDisplayContrast", "value = " + value);
    }

    public int getDisplayEnhanceMode(int display) {
        return runIntCmd(() -> { return mService.getEnhanceComponent(display, ENHANCE_MODE);}, display, "getDisplayEnhanceMode", null);
    }

    public int setDisplayEnhanceMode(int display, int value){
        return runIntCmd(() -> { return mService.setEnhanceComponent(display, ENHANCE_MODE, value);}, display, "setDisplayEnhanceMode", "mode = " + value);
    }

    public boolean getReadingMode(int display){
        return runBooleanCmd(() -> { return mService.getReadingMode(display);}, display, "getReadingMode", null);
    }

    public int setReadingMode(int display, boolean value){
        return runIntCmd(() -> { return mService.setReadingMode(display, value);}, display, "setReadingMode", "value = " + value);
    }

    public int getSmartBacklight(int display){
        return runIntCmd(() -> { return mService.getSmartBacklight(display);}, display, "getSmartBacklight", null);
    }

    public int setSmartBacklight(int display, int value){
        return runIntCmd(() -> { return mService.setSmartBacklight(display, value);}, display, "setSmartBacklight", "value = " + value);
    }

    public int getColorTemperature(int display){
        return runIntCmd(() -> { return mService.getColorTemperature(display);}, display, "getColorTemperature", null);
    }

    public int setColorTemperature(int display, int value){
        return runIntCmd(() -> { return mService.setColorTemperature(display, value);}, display, "setColorTemperature", "value = " + value);
    }

    static public int getDisplayModeFromFormat(int format) {
        return format & DISPLAY_OUTPUT_MODE_MASK;
    }

    static public int getDisplayTypeFromFormat(int format) {
        return (format & DISPLAY_OUTPUT_TYPE_MASK) >> 8;
    }

    public int makeDisplayFormat(int type, int mode) {
        return ((type << 8) & DISPLAY_OUTPUT_TYPE_MASK) | (mode & DISPLAY_OUTPUT_MODE_MASK);
    }

    public boolean supportedSNRSetting(int display) {
        return runBooleanCmd(() -> { return mService.supportedSNRSetting(display);}, display, "supportedSNRSetting", null);
    }

    public int getSNRFeatureMode(int display) {
        return runIntCmd(() -> { return mService.getSNRFeatureMode(display);}, display, "getSNRFeatureMode", null);
    }

    public int[] getSNRStrength(int display) {
        int[] retval = null;
        try {
            retval = mService.getSNRStrength(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setSNRConfig(int display, int mode, int ystrength, int ustrength, int vstrength) {
        return runIntCmd(() -> { return mService.setSNRConfig(display, mode, ystrength, ustrength, vstrength);}, display,
                                "setSNRConfig", "mode = " + mode + ", ystrength = " + ystrength + ", ustrength = " + ustrength
                                + ", vstrength = " + vstrength);
    }

    public int getHdmiUserSetting(int display) {
        return runIntCmd(() -> { return mService.getHdmiUserSetting(display);}, display, "getHdmiUserSetting", null);
    }

    public int getHdmiNativeMode(int display){
        return runIntCmd(() -> { return mService.getHdmiNativeMode(display);}, display, "getHdmiNativeMode", null);
    }

    public SinkInfo getSinkInfo(int display) {
        SinkInfo info = new SinkInfo();
        int type = getDisplayOutputType(display);

        if (type == DISPLAY_OUTPUT_TYPE_HDMI) {
            info.Type = DISPLAY_OUTPUT_TYPE_HDMI;
            info.SupportedModes = getSupportModes(display, info.Type);
            info.CurrentMode = getDisplayOutputMode(display);
            info.UserSetting = getHdmiUserSetting(display);

            int nativeMode = getHdmiNativeMode(display);
            info.IsNative = (nativeMode == info.CurrentMode);
        }
        return info;
    }
}
