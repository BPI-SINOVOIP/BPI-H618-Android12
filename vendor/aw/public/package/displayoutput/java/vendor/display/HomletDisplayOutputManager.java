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

public class HomletDisplayOutputManager extends DisplayOutputManager{
    private static final String TAG = "HomletDisplayOutputManager";
    private IDisplayOutputManager mService = null;
    private static boolean DEBUG_ON = false;

    public HomletDisplayOutputManager(){
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
    private static void debug(String msg) {
        if (DEBUG_ON)
            Log.d(TAG, msg);
    }

    /*@hide*/
    public static final int DISPLAY_OUTPUT_TYPE_MASK = 0xff00;
    /*@hide*/
    public static final int DISPLAY_OUTPUT_MODE_MASK = 0xff;

    public int getDisplayOutput(int display){
        int retval = 0;
        try {
          retval = mService.getDisplayOutput(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayOutput(int display, int format){
    int retval = 0;
        try {
          retval = mService.setDisplayOutput(display,format);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayOutputType(int display){
        int retval = 0;
        try {
          retval = mService.getDisplayOutputType(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayOutputMode(int display){
        int retval = 0;
        try {
          retval = mService.getDisplayOutputMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayOutputMode(int display, int mode){
        int retval = 0;
        try {
          retval = mService.setDisplayOutputMode(display,mode);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayOutputPixelFormat(int display) {
        int retval = 0;
        try {
          retval = mService.getDisplayOutputPixelFormat(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayOutputPixelFormat(int display, int format) {
        int retval = 0;
        try {
          retval = mService.setDisplayOutputPixelFormat(display, format);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayOutputCurDataspaceMode(int display) {
        int retval = 0;
        try {
          retval = mService.getDisplayOutputCurDataspaceMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayOutputDataspaceMode(int display) {
        int retval = 0;
        try {
          retval = mService.getDisplayOutputDataspaceMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayOutputDataspaceMode(int display, int mode) {
        int retval = 0;
        try {
          retval = mService.setDisplayOutputDataspaceMode(display, mode);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public boolean isSupportHdmiMode(int display, int mode){
        boolean retval = false;
        try {
          retval = mService.isSupportHdmiMode(display,mode);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int[] getSupportModes(int display, int type){
        int[] retval = null;
        try {
          retval = mService.getSupportModes(display,type);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

     /* interface for 3D mode setting */
    public int getDisplaySupport3DMode(int display){
        int retval = 0;
        try {
          retval = mService.getDisplaySupport3DMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplay3DMode(int display, int mode){
        int retval = 0;
        try {
          retval = mService.setDisplay3DMode(display, mode, 0);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplay3DMode(int display, int mode, int videoCropHeight){
        int retval = 0;
        try {
          retval = mService.setDisplay3DMode(display, mode, videoCropHeight);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplay3DLayerOffset(int display, int offset){
        int retval = 0;
        try {
          retval = mService.setDisplay3DLayerOffset(display,offset);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public boolean isCurrent3Doutput(int display) {
        if (getDisplayOutputMode(display) == DISPLAY_TVFORMAT_1080P_24HZ_3D_FP)
            return true;
        else
            return false;
    }

     /* interface for screen margin/offset setting */
    public int[] getDisplayMargin(int display){
        int[] retval = null;
        try {
            retval = mService.getDisplayMargin(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int[] getDisplayOffset(int display){
        int[] retval = null;
        try {
          retval = mService.getDisplayOffset(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayMargin(int display, int margin_x, int margin_y){
        int retval = 0;
        try {
          retval = mService.setDisplayMargin(display,margin_x,margin_y);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayOffset(int display, int offset_x, int offset_y){
        int retval = 0;
        try {
          retval = mService.setDisplayOffset(display,offset_x,offset_y);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

     /* interface for display enhance effect */
    public int getDisplayEdge(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_EDGE);
          //retval = mService.getDisplayEdge(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayEdge(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_EDGE, value);
          //retval = mService.setDisplayEdge(display,value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayDetail(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_DETAIL);
          //retval = mService.getDisplayDetail(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayDetail(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_DETAIL, value);
          //retval = mService.setDisplayDetail(display,value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayBright(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_BRIGHT);
          //retval = mService.getDisplayBright(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayBright(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_BRIGHT, value);
          //retval = mService.setDisplayBright(display,value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayDenoise(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_DENOISE);
          //retval = mService.getDisplayDenoise(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayDenoise(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_DENOISE, value);
          //retval = mService.setDisplayDenoise(display,value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplaySaturation(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_SATURATION);
          //retval = mService.getDisplaySaturation(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplaySaturation(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_SATURATION, value);
          //retval = mService.setDisplaySaturation(display,value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayContrast(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_CONTRAST);
          //retval = mService.getDisplayContrast(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayContrast(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_CONTRAST, value);
          //retval = mService.setDisplayContrast(display,value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getDisplayEnhanceMode(int display){
        int retval = 0;
        try {
            retval = mService.getEnhanceComponent(display, ENHANCE_MODE);
          //retval = mService.getDisplayEnhanceMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int setDisplayEnhanceMode(int display, int value){
        int retval = 0;
        try {
            retval = mService.setEnhanceComponent(display, ENHANCE_MODE, value);
          //retval = mService.setDisplayEnhanceMode(display, value);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
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
        boolean retval = false;
        try {
            retval = mService.supportedSNRSetting(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getSNRFeatureMode(int display) {
        int mode = SNR_FEATURE_MODE_DISABLE;
        try {
            mode = mService.getSNRFeatureMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return mode;
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
        int retval = 0;
        try {
            retval = mService.setSNRConfig(display, mode, ystrength, ustrength, vstrength);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getHdmiUserSetting(int display){
        int retval = 0;
        try {
            retval = mService.getHdmiUserSetting(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getHdmiNativeMode(int display){
        int retval = 0;
        try {
            retval = mService.getHdmiNativeMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
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

    /* hdcp for homlet platform */
    public int configHdcp(boolean enable) {
        int retval = 0;
        try {
          retval = mService.configHdcp(enable);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getConnectedHdcpLevel() {
        int retval = 0;
        try {
            retval = mService.getConnectedHdcpLevel();
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }

    public int getAuthorizedStatus() {
        int retval = 0;
        try {
            retval = mService.getAuthorizedStatus();
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        return retval;
    }
}
