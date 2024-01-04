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
    private DisplayOutputManagerImpl mImpl = null;

    public HomletDisplayOutputManager() {
        mImpl = new DisplayOutputManagerImpl();
        mImpl.setTAG(TAG);
    }

    @Override
    public int getDisplayOutput(int display) {
        return mImpl.getDisplayOutput(display);
    }

    @Override
    public int setDisplayOutput(int display, int format) {
        return mImpl.setDisplayOutput(display, format);
    }

    @Override
    public int getDisplayOutputType(int display) {
        return mImpl.getDisplayOutputType(display);
    }

    @Override
    public int getDisplayOutputMode(int display) {
        return mImpl.getDisplayOutputMode(display);
    }

    @Override
    public int setDisplayOutputMode(int display, int mode) {
        return mImpl.setDisplayOutputMode(display, mode);
    }

    @Override
    public int getDisplayOutputPixelFormat(int display) {
        return mImpl.getDisplayOutputPixelFormat(display);
    }

    @Override
    public int setDisplayOutputPixelFormat(int display, int format) {
        return mImpl.setDisplayOutputPixelFormat(display, format);
    }

    @Override
    public int getDisplayOutputCurDataspaceMode(int display) {
        return mImpl.getDisplayOutputCurDataspaceMode(display);
    }

    @Override
    public int getDisplayOutputDataspaceMode(int display) {
        return mImpl.getDisplayOutputDataspaceMode(display);
    }

    @Override
    public int setDisplayOutputDataspaceMode(int display, int mode) {
        return mImpl.setDisplayOutputDataspaceMode(display, mode);
    }

    @Override
    public boolean isSupportHdmiMode(int display, int mode) {
        return mImpl.isSupportHdmiMode(display, mode);
    }

    @Override
    public int[] getSupportModes(int display, int type) {
        return mImpl.getSupportModes(display, type);
    }

    /* interface for 3D mode setting */
    @Override
    public int getDisplaySupport3DMode(int display) {
        return mImpl.getDisplaySupport3DMode(display);
    }

    @Override
    public int setDisplay3DMode(int display, int mode) {
        return mImpl.setDisplay3DMode(display, mode);
    }

    @Override
    public int setDisplay3DMode(int display, int mode, int videoCropHeight) {
        return mImpl.setDisplay3DMode(display, mode, videoCropHeight);
    }

    @Override
    public int setDisplay3DLayerOffset(int display, int offset) {
        return mImpl.setDisplay3DLayerOffset(display, offset);
    }

    @Override
    public boolean isCurrent3Doutput(int display) {
        return mImpl.isCurrent3Doutput(display);
    }

    /* interface for screen margin/offset setting */
    @Override
    public int[] getDisplayMargin(int display) {
        return mImpl.getDisplayMargin(display);
    }

    @Override
    public int[] getDisplayOffset(int display) {
        return mImpl.getDisplayOffset(display);
    }

    @Override
    public int setDisplayMargin(int display, int margin_x, int margin_y) {
        return mImpl.setDisplayMargin(display, margin_x, margin_y);
    }

    /* just for defalut platform*/
    @Override
    public boolean getHdmiFullscreen(int display) {
        return mImpl.getHdmiFullscreen(display);
    }
    @Override
    public int setHdmiFullscreen(int display, boolean full) {
        return mImpl.setHdmiFullscreen(display, full);
    }

    @Override
    public int setDisplayOffset(int display, int offset_x, int offset_y) {
        return mImpl.setDisplayOffset(display, offset_x, offset_y);
    }

    /* interface for display enhance effect */
    @Override
    public int getDisplayEdge(int display) {
        return mImpl.getDisplayEdge(display);
    }

    @Override
    public int setDisplayEdge(int display, int value) {
        return mImpl.setDisplayEdge(display, value);
    }

    @Override
    public int getDisplayDetail(int display) {
        return mImpl.getDisplayDetail(display);
    }

    @Override
    public int setDisplayDetail(int display, int value) {
        return mImpl.setDisplayDetail(display, value);
    }

    @Override
    public int getDisplayBright(int display) {
        return mImpl.getDisplayBright(display);
    }

    @Override
    public int setDisplayBright(int display, int value) {
        return mImpl.setDisplayBright(display, value);
    }

    @Override
    public int getDisplayDenoise(int display) {
        return mImpl.getDisplayDenoise(display);
    }

    @Override
    public int setDisplayDenoise(int display, int value) {
        return mImpl.setDisplayDenoise(display, value);
    }

    @Override
    public boolean getBlackWhiteMode(int display) {
        return mImpl.getBlackWhiteMode(display);
    }

    @Override
    public int setBlackWhiteMode(int display, boolean on) {
        return mImpl.setBlackWhiteMode(display, on);
    }

    @Override
    public int getDisplaySaturation(int display) {
        return mImpl.getDisplaySaturation(display);
    }

    @Override
    public int setDisplaySaturation(int display, int value) {
        return mImpl.setDisplaySaturation(display, value);
    }

    @Override
    public int getDisplayContrast(int display) {
        return mImpl.getDisplayContrast(display);
    }

    @Override
    public int setDisplayContrast(int display, int value) {
        return mImpl.setDisplayContrast(display, value);
    }

    @Override
    public int getDisplayEnhanceMode(int display) {
        return mImpl.getDisplayEnhanceMode(display);
    }

    @Override
    public int setDisplayEnhanceMode(int display, int value) {
        return mImpl.setDisplayEnhanceMode(display, value);
    }

    @Override
    public boolean getReadingMode(int display) {
        return mImpl.getReadingMode(display);
    }

    @Override
    public int setReadingMode(int display, boolean value) {
        return mImpl.setReadingMode(display, value);
    }

    @Override
    public int getSmartBacklight(int display) {
        return mImpl.getSmartBacklight(display);
    }

    @Override
    public int setSmartBacklight(int display, int value) {
        return mImpl.setSmartBacklight(display, value);
    }

    @Override
    public int getColorTemperature(int display) {
        return mImpl.getColorTemperature(display);
    }

    @Override
    public int setColorTemperature(int display, int value) {
        return mImpl.setColorTemperature(display, value);
    }

    static public int getDisplayModeFromFormat(int format) {
        return DisplayOutputManagerImpl.getDisplayModeFromFormat(format);
    }

    static public int getDisplayTypeFromFormat(int format) {
        return DisplayOutputManagerImpl.getDisplayTypeFromFormat(format);
    }

    @Override
    public int makeDisplayFormat(int type, int mode) {
        return mImpl.makeDisplayFormat(type, mode);
    }

    @Override
    public boolean supportedSNRSetting(int display) {
        return mImpl.supportedSNRSetting(display);
    }

    @Override
    public int getSNRFeatureMode(int display) {
        return mImpl.getSNRFeatureMode(display);
    }

    @Override
    public int[] getSNRStrength(int display) {
        return mImpl.getSNRStrength(display);
    }

    @Override
    public int setSNRConfig(int display, int mode, int ystrength, int ustrength, int vstrength) {
        return mImpl.setSNRConfig(display, mode, ystrength, ustrength, vstrength);
    }

    @Override
    public int getHdmiUserSetting(int display) {
        return mImpl.getHdmiUserSetting(display);
    }

    @Override
    public int getHdmiNativeMode(int display) {
        return mImpl.getHdmiNativeMode(display);
    }

    @Override
    public SinkInfo getSinkInfo(int display) {
        return mImpl.getSinkInfo(display);
    }
}
