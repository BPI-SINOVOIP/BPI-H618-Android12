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
import vendor.display.config.V1_0.IDisplayConfig;
import vendor.display.config.V1_0.EnhanceItem;

public class DefaultDisplayOutputManager extends DisplayOutputManager{
    private static final String TAG = "DefaultDisplayOutputManager";
    private IDisplayOutputManager mService = null;
    private static boolean DEBUG_ON = false;

    public DefaultDisplayOutputManager(){
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

    public int getDisplayOutputMode(int display){
        int retval = 0;
        try {
          retval = mService.getDisplayOutputMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " getDisplayOutputMode: " +
                " value(" + retval + ")");
        return retval;
    }

    public int setDisplayOutputMode(int display, int mode){
        int retval = 0;
        try {
          retval = mService.setDisplayOutputMode(display,mode);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " setDisplayOutputMode: " +
                " value(" + retval + ")");
        return retval;
    }

    /* just for defalut platform*/
    public boolean getHdmiFullscreen(int display) {
        boolean retval = false;
        try {
          retval = mService.getHdmiFullscreen(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " getHdmiFullscreen: " +
                " value(" + retval + ")");
        return retval;
    }
    public int setHdmiFullscreen(int display, boolean full) {
        int retval = 0;
        try {
          retval = mService.setHdmiFullscreen(display, full);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " setHdmiFullscreen: " +
                " value(" + full + ")");
        return retval;
    }


    /* interface for 3D mode setting*/
    public int getDisplay3DMode(int display){
        int retval = 0;
        try {
          retval = mService.getDisplay3DMode(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " getDisplay3DMode: " +
                " value(" + retval + ")");
        return retval;
    }

    public int setDisplay3DMode(int display, int mode){
        int retval = 0;
        try {
          retval = mService.setDisplay3DMode(display, mode, 0);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " setDisplay3DMode: " +
                " value(" + mode + ")");
        return retval;
    }

    public int setDisplay3DMode(int display, int mode, int videoCropHeight){
        int retval = 0;
        try {
          retval = mService.setDisplay3DMode(display, mode, videoCropHeight);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " setDisplay3DMode: " +
                " value(" + mode + "-" + videoCropHeight + ")");
        return retval;
    }

     /* interface for screen margin/offset setting */
    public int[] getDisplayMargin(int display){
        int[] retval = null;
        try {
            retval = mService.getDisplayMargin(display);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
        debug("Display." + display + " getDisplayMargin: " +
                " value(" + retval==null? "0":retval[0]+
                "-" + retval==null?"0":retval[1] + ")");
        return retval;
    }

    public int setDisplayMargin(int display, int margin_x, int margin_y){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            mService.setDisplayMargin(display, margin_x, margin_y);
        } catch (RemoteException e) {
            Log.e(TAG, "setDisplayMargin:", e);
        }

        debug("Display." + display + " setDisplayMargin: " +
                " value(" + margin_x+"-"+margin_y + ") return: " + ret);
        return ret;
    }

    /* interface for display enhance effect*/
    public int getDisplayBright(int display){
        int value = 0;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }
        try {
            value = mService.getEnhanceComponent(display, ENHANCE_BRIGHT);
        } catch (RemoteException e) {
            Log.e(TAG, "getEnhanceComponent:", e);
        }

        debug("Display." + display + " Enhance: " +
                EnhanceItem.toString(ENHANCE_BRIGHT) + " value: " + value);
        return value;
    }

    public int setDisplayBright(int display, int value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            ret = mService.setEnhanceComponent(display, ENHANCE_BRIGHT, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setEnhanceComponent:", e);
        }

        debug("Display." + display + " setEnhanceComponent: " +
                EnhanceItem.toString(ENHANCE_BRIGHT) + " value(" + value + ") return: " + ret);
        return ret;
    }

    public int getDisplaySaturation(int display){
        int value = 0;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }
        try {
            value = mService.getEnhanceComponent(display, ENHANCE_SATURATION);
        } catch (RemoteException e) {
            Log.e(TAG, "getEnhanceComponent:", e);
        }

        debug("Display." + display + " Enhance: " +
                EnhanceItem.toString(ENHANCE_SATURATION) + " value: " + value);
        return value;
    }

    public int setDisplaySaturation(int display, int value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            ret = mService.setEnhanceComponent(display, ENHANCE_SATURATION, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setEnhanceComponent:", e);
        }

        debug("Display." + display + " setEnhanceComponent: " +
                EnhanceItem.toString(ENHANCE_SATURATION) + " value(" + value + ") return: " + ret);
        return ret;
    }

    public int getDisplayContrast(int display){
        int value = 0;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }
        try {
            value = mService.getEnhanceComponent(display, ENHANCE_CONTRAST);
        } catch (RemoteException e) {
            Log.e(TAG, "getEnhanceComponent:", e);
        }

        debug("Display." + display + " Enhance: " +
                EnhanceItem.toString(ENHANCE_CONTRAST) + " value: " + value);
        return value;
    }

    public int setDisplayContrast(int display, int value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            ret = mService.setEnhanceComponent(display, ENHANCE_CONTRAST, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setEnhanceComponent:", e);
        }

        debug("Display." + display + " setEnhanceComponent: " +
                EnhanceItem.toString(ENHANCE_CONTRAST) + " value(" + value + ") return: " + ret);
        return ret;
    }

    public int getDisplayEnhanceMode(int display){
        int value = 0;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }
        try {
            value = mService.getEnhanceComponent(display, ENHANCE_MODE);
        } catch (RemoteException e) {
            Log.e(TAG, "getEnhanceComponent:", e);
        }

        debug("Display." + display + " Enhance: " +
                EnhanceItem.toString(ENHANCE_MODE) + " value: " + value);
        return value;
    }

    public int setDisplayEnhanceMode(int display, int value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            ret = mService.setEnhanceComponent(display, ENHANCE_MODE, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setEnhanceComponent:", e);
        }

        debug("Display." + display + " setEnhanceComponent: " +
                EnhanceItem.toString(ENHANCE_MODE) + " value(" + value + ") return: " + ret);
        return ret;
    }

    public boolean getBlackWhiteMode(int display) {
        boolean value = false;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }
        try {
            value = mService.getBlackWhiteMode(display);
        } catch (RemoteException e) {
            Log.e(TAG, "getBlackWhiteMode:", e);
        }

        debug("Display." + display + " getBlackWhiteMode: value: " + value);
        return value;
    }

    public int setBlackWhiteMode(int display, boolean on) {
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
                return ERROR;
        }
        try {
            ret = mService.setBlackWhiteMode(display, on);
        } catch (RemoteException e) {
            Log.e(TAG, "setBlackWhiteMode:", e);
        }

        debug("Display." + display + " setBlackWhiteMode: value(" + on + ") return: " + ret);
        return ret;
    }

    public int getSmartBacklight(int display){
        int value = 0;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }
        try {
            value = mService.getSmartBacklight(display);
        } catch (RemoteException e) {
            Log.e(TAG, "getEnhanceComponent:", e);
        }

        debug("Display." + display + " getSmartBacklight: " +
                " value: " + value);
        return value;
    }

    public int setSmartBacklight(int display, int value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            ret = mService.setSmartBacklight(display, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setSmartBacklight:", e);
        }

        debug("Display." + display + " setSmartBacklight: " +
                " value(" + value + ") return: " + ret);
        return ret;
    }

    public boolean getReadingMode(int display){
        boolean value = false;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }

        try {
            value = mService.getReadingMode(display);
        } catch (RemoteException e) {
            Log.e(TAG, "getReadingMode:", e);
        }
        debug("Display." + display + " getReadingMode: " +
                " value: " + value);
        return value;
    }

    public int setReadingMode(int display, boolean value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }

        try {
            ret = mService.setReadingMode(display, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setReadingMode:", e);
        }

        debug("Display." + display + " setReadingMode: " +
                " value(" + value + ") return: " + ret);
        return ret;
    }

    public int getColorTemperature(int display){
        int value = 0;

        if (mService == null) {
            Log.d(TAG, "mService is null");
            return value;
        }

        try {
            value = mService.getColorTemperature(display);
        } catch (RemoteException e) {
            Log.e(TAG, "getColorTemperature:", e);
        }

        debug("Display." + display + " getColorTemperature: " +
                " value: " + value);
        return value;
    }

    public int setColorTemperature(int display, int value){
        int ret = ERROR;
        if (mService == null) {
            Log.d(TAG, "mService is null");
            return ERROR;
        }
        try {
            ret = mService.setColorTemperature(display, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setColorTemperature:", e);
        }

        debug("Display." + display + " setColorTemperature: " +
                " value(" + value + ") return: " + ret);
        return ret;
    }
}
