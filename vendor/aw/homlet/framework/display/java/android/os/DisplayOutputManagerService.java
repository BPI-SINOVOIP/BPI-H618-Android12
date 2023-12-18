/*
 * Copyright (C) 2011 The Android Open Source Project
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

package android.os;

import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.util.Slog;
import android.util.Log;
import java.util.ArrayList;
import android.widget.Toast;

import com.softwinner.IDisplayOutputManager;
import vendor.display.config.V1_0.ScreenMargin;
import vendor.display.config.V1_0.SNRInfo;
import vendor.display.config.V1_0.SNRFeatureMode;
import com.softwinner.display.DisplaydClient;

/**
 * @hide
 */
public class DisplayOutputManagerService extends IDisplayOutputManager.Stub {
    private static final String TAG = "DisplayOutputManagerService";
    private static final boolean DBG = false;
    private DisplaydClient mClient;

    public DisplayOutputManagerService(){
        mClient = new DisplaydClient(null);
        Slog.d(TAG, "DisplayOutputManagerService init");
    }

    public int getInterfaceVersion() {
        return 1;
    }

    /* interface for output mode/type setting */
    public int getDisplayOutput(int display){
        int type = mClient.getType(display);
        int mode = mClient.getMode(display);
        return ((type & 0xff) << 8) | (mode & 0xff);
    }

    public int setDisplayOutput(int display, int format){
        int type = (format >> 8) & 0xff;
        int mode = format & 0xff;
        if (mClient.getType(display) != type) {
            Slog.e(TAG, "setDisplayOutput: device type error");
            return -1;
        }
        return mClient.setMode(display, mode);
    }

    public int getDisplayOutputType(int display){
        return mClient.getType(display);
    }

    public int getDisplayOutputMode(int display){
        return mClient.getMode(display);
    }

    public int setDisplayOutputMode(int display, int mode){
        return mClient.setMode(display, mode);
    }

    public int getDisplayOutputPixelFormat(int display) {
        return mClient.getPixelFormat(display);
    }

    public int setDisplayOutputPixelFormat(int display, int format) {
        return mClient.setPixelFormat(display, format);
    }

    public int getDisplayOutputCurDataspaceMode(int display) {
        // TODO:
        return 0;
    }

    public int getDisplayOutputDataspaceMode(int display) {
        return mClient.getDataspaceMode(display);
    }

    public int setDisplayOutputDataspaceMode(int display, int mode) {
        return mClient.setDataspaceMode(display, mode);
    }

    public boolean isSupportHdmiMode(int display, int mode){
        boolean supported = mClient.isSupportedMode(display, mode);
        return supported;
    }

    public int[] getSupportModes(int display, int type){
        int[] out = null;
        ArrayList<Integer> modes = mClient.getSupportedModes(display);
        if (modes != null) {
            out = new int[modes.size()];
            for (int i = 0; i < modes.size(); i++) {
                out[i] = modes.get(i).intValue();
            }
        }
        return out;
    }

     /* interface for 3D mode setting */
    public int getDisplaySupport3DMode(int display){
        return mClient.isSupported3D(display) ? 1 : 0;
    }

    public int setDisplay3DMode(int display, int mode, int videoCropHeight) {
        return mClient.set3DLayerMode(display, mode);
    }

    public int setDisplay3DLayerOffset(int display, int offset){
        // TODO:
        return 0;
    }

     /* interface for screen margin/offset setting */
    public int[] getDisplayMargin(int display){
        int[] out = null;
        ScreenMargin margin = mClient.getMargin(display);
        if (margin != null) {
            out = new int[4];
            out[0] = margin.left;
            out[1] = margin.right;
            out[2] = margin.top;
            out[3] = margin.bottom;
        }
        return out;
    }

    public int[] getDisplayOffset(int display){
        // TODO:
        return null;
    }

    public int setDisplayMargin(int display, int margin_x, int margin_y){
        ScreenMargin margin = new ScreenMargin();
        margin.left = margin.right  = margin_x;
        margin.top  = margin.bottom = margin_y;
        return mClient.setMargin(display, margin);
    }

    public int setDisplayOffset(int display, int offset_x, int offset_y){
        // TODO:
        return 0;
    }

     /* interface for display enhance effect */
    public int getDisplayEdge(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_EDGE);
    }

    public int setDisplayEdge(int display, int edge){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_EDGE, edge);
    }

    public int getDisplayDetail(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_DETAIL);
    }

    public int setDisplayDetail(int display, int detail){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_DETAIL, detail);
    }

    public int getDisplayBright(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_BRIGHT);
    }

    public int setDisplayBright(int display, int bright){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_BRIGHT, bright);
    }

    public int getDisplayDenoise(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_DENOISE);
    }

    public int setDisplayDenoise(int display, int denoise){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_DENOISE, denoise);
    }

    public int getDisplaySaturation(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_SATURATION);
    }

    public int setDisplaySaturation(int display, int saturation){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_SATURATION, saturation);
    }

    public int getDisplayContrast(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_CONTRAST);
    }

    public int setDisplayContrast(int display, int contrast){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_CONTRAST, contrast);
    }

    public int getDisplayEnhanceMode(int display){
        return mClient.getEnhanceComponent(display,
                    DisplaydClient.ENHANCE_MODE);
    }

    public int setDisplayEnhanceMode(int display, int mode){
        return mClient.setEnhanceComponent(display,
                    DisplaydClient.ENHANCE_MODE, mode);
    }

    public boolean supportedSNRSetting(int display) {
        return mClient.supportedSNRSetting(display);
    }

    public int getSNRFeatureMode(int display) {
        SNRInfo info = mClient.getSNRInfo(display);
        if (info != null)
            return info.mode;
        return SNRFeatureMode.SNR_DISABLE;
    }

    public int[] getSNRStrength(int display) {
        int[] out = null;
        SNRInfo info = mClient.getSNRInfo(display);
        if (info != null) {
            out = new int[3];
            out[0] = info.y;
            out[1] = info.u;
            out[2] = info.v;
        }
        return out;
    }

    public int setSNRConfig(int display, int mode, int ystrength, int ustrength, int vstrength) {
        SNRInfo info = new SNRInfo();
        info.mode = mode;
        info.y = ystrength;
        info.u = ustrength;
        info.v = vstrength;
        return mClient.setSNRInfo(display, info);
    }

    /* extern for hdmi interface */
    public int getHdmiNativeMode(int display){
        return mClient.getHdmiNativeMode(display);
    }
    public int getHdmiUserSetting(int display){
        return mClient.getHdmiUserSetting(display);
    }

    /* hdcp for homlet platform */
    public int configHdcp(boolean enable) {
        return mClient.configHdcp(enable);
    }
    public int getConnectedHdcpLevel() {
        return mClient.getConnectedHdcpLevel();
    }
    public int getAuthorizedStatus() {
        return mClient.getAuthorizedStatus();
    }
}
