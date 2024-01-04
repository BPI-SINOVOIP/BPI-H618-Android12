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

import android.os.SystemProperties;
import android.util.Log;
import vendor.display.config.V1_0.EnhanceItem;

public abstract class DisplayOutputManager {
    private static final String TAG = "DisplayOutputManager";

    public static final int SUCCESS =  0;
    public static final int ERROR   = -1;

    public static final int MODE_DISABLE    = 0;
    public static final int MODE_ENABLE     = 1;
    public static final int MODE_DEMO       = 2;

    public static final int ENHANCE_MODE       = EnhanceItem.ENHANCE_MODE;
    public static final int ENHANCE_BRIGHT     = EnhanceItem.ENHANCE_BRIGHT;
    public static final int ENHANCE_CONTRAST   = EnhanceItem.ENHANCE_CONTRAST;
    public static final int ENHANCE_DENOISE    = EnhanceItem.ENHANCE_DENOISE;
    public static final int ENHANCE_DETAIL     = EnhanceItem.ENHANCE_DETAIL;
    public static final int ENHANCE_EDGE       = EnhanceItem.ENHANCE_EDGE;
    public static final int ENHANCE_SATURATION = EnhanceItem.ENHANCE_SATURATION;

    private static DisplayOutputManager instance;
    public static DisplayOutputManager getInstance(){
        if(instance == null) {
            String platform = SystemProperties.get("ro.build.characteristics", "");
            Log.d(TAG, "ro.build.characteristics="+platform);
			//bpi, both tablet and homlet use homelet display
            if (platform.equals("homlet") || platform.equals("tablet")) {
                instance = new HomletDisplayOutputManager();
            } else {
                instance = new DefaultDisplayOutputManager();
            }
        }
        return instance;
    }

    public static final int DISPLAY_2D_ORIGINAL                  = 0;
    public static final int DISPLAY_2D_LEFT                      = 1;
    public static final int DISPLAY_2D_TOP                       = 2;
    public static final int DISPLAY_3D_LEFT_RIGHT_HDMI           = 3;
    public static final int DISPLAY_3D_TOP_BOTTOM_HDMI           = 4;
    public static final int DISPLAY_2D_DUAL_STREAM               = 5;
    public static final int DISPLAY_3D_DUAL_STREAM               = 6;

    public static final int DISPLAY_OUTPUT_TYPE_NONE             = 0;
    public static final int DISPLAY_OUTPUT_TYPE_LCD              = 1;
    public static final int DISPLAY_OUTPUT_TYPE_TV               = 2;
    public static final int DISPLAY_OUTPUT_TYPE_HDMI             = 4;
    public static final int DISPLAY_OUTPUT_TYPE_VGA              = 8;

    public static final int DISPLAY_TVFORMAT_480I                = 0;
    public static final int DISPLAY_TVFORMAT_576I                = 1;
    public static final int DISPLAY_TVFORMAT_480P                = 2;
    public static final int DISPLAY_TVFORMAT_576P                = 3;
    public static final int DISPLAY_TVFORMAT_720P_50HZ           = 4;
    public static final int DISPLAY_TVFORMAT_720P_60HZ           = 5;
    public static final int DISPLAY_TVFORMAT_1080I_50HZ          = 6;
    public static final int DISPLAY_TVFORMAT_1080I_60HZ          = 7;
    public static final int DISPLAY_TVFORMAT_1080P_24HZ          = 8;
    public static final int DISPLAY_TVFORMAT_1080P_25HZ          = 0x1a;
    public static final int DISPLAY_TVFORMAT_1080P_30HZ          = 0x1b;
    public static final int DISPLAY_TVFORMAT_1080P_50HZ          = 9;
    public static final int DISPLAY_TVFORMAT_1080P_60HZ          = 0xa;
    public static final int DISPLAY_TVFORMAT_3840_2160P_30HZ     = 0x1c;
    public static final int DISPLAY_TVFORMAT_3840_2160P_25HZ     = 0x1d;
    public static final int DISPLAY_TVFORMAT_3840_2160P_24HZ     = 0x1e;
    public static final int DISPLAY_TVFORMAT_4096_2160P_24HZ     = 0x1f;
    public static final int DISPLAY_TVFORMAT_4096_2160P_25HZ     = 0x20;
    public static final int DISPLAY_TVFORMAT_4096_2160P_30HZ     = 0x21;
    public static final int DISPLAY_TVFORMAT_3840_2160P_60HZ     = 0x22;
    public static final int DISPLAY_TVFORMAT_4096_2160P_60HZ     = 0x23;
    public static final int DISPLAY_TVFORMAT_3840_2160P_50HZ     = 0x24;
    public static final int DISPLAY_TVFORMAT_4096_2160P_50HZ     = 0x25;

    public static final int DISPLAY_TVFORMAT_PAL                 = 0xb;
    public static final int DISPLAY_TVFORMAT_PAL_SVIDEO          = 0xc;
    public static final int DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO     = 0xd;
    public static final int DISPLAY_TVFORMAT_NTSC                = 0xe;
    public static final int DISPLAY_TVFORMAT_NTSC_SVIDEO         = 0xf;
    public static final int DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO    = 0x10;
    public static final int DISPLAY_TVFORMAT_PAL_M               = 0x11;
    public static final int DISPLAY_TVFORMAT_PAL_M_SVIDEO        = 0x12;
    public static final int DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO   = 0x13;
    public static final int DISPLAY_TVFORMAT_PAL_NC              = 0x14;
    public static final int DISPLAY_TVFORMAT_PAL_NC_SVIDEO       = 0x15;
    public static final int DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO  = 0x16;
    public static final int DISPLAY_TVFORMAT_1080P_24HZ_3D_FP    = 0x17;

    public static final int DISPLAY_VGA_FORMAT_640x480P_60HZ     = 0x0;
    public static final int DISPLAY_VGA_FORMAT_800x600P_60HZ     = 0x1;
    public static final int DISPLAY_VGA_FORMAT_1024x768P_60HZ    = 0x2;
    public static final int DISPLAY_VGA_FORMAT_1280x768P_60HZ    = 0x3;
    public static final int DISPLAY_VGA_FORMAT_1280x800P_60HZ    = 0x4;
    public static final int DISPLAY_VGA_FORMAT_1366x768P_60HZ    = 0x5;
    public static final int DISPLAY_VGA_FORMAT_1440x900P_60HZ    = 0x6;
    public static final int DISPLAY_VGA_FORMAT_1920x1080P_60HZ   = 0x7;
    public static final int DISPLAY_VGA_FORMAT_1920x1200P_60HZ   = 0x8;

    public static final int SNR_FEATURE_MODE_DISABLE = 0;
    public static final int SNR_FEATURE_MODE_LEVEL1  = 1;
    public static final int SNR_FEATURE_MODE_LEVEL2  = 2;
    public static final int SNR_FEATURE_MODE_LEVEL3  = 3;
    public static final int SNR_FEATURE_MODE_CUSTOM  = 4;
    public static final int SNR_FEATURE_MODE_DEMO    = 5;

    public int getDisplayOutput(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayOutput(int display, int format){
        throw new UnsupportedOperationException();
    }

    public int getDisplayOutputType(int display){
        throw new UnsupportedOperationException();
    }

    public int getDisplayOutputMode(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayOutputMode(int display, int mode){
        throw new UnsupportedOperationException();
    }

    public int getDisplayOutputPixelFormat(int display) {
        throw new UnsupportedOperationException();
    }

    public int setDisplayOutputPixelFormat(int display, int format) {
        throw new UnsupportedOperationException();
    }

    public int getDisplayOutputCurDataspaceMode(int display) {
        throw new UnsupportedOperationException();
    }

    public int getDisplayOutputDataspaceMode(int display) {
        throw new UnsupportedOperationException();
    }

    public int setDisplayOutputDataspaceMode(int display, int mode) {
        throw new UnsupportedOperationException();
    }

    public boolean isSupportHdmiMode(int display, int mode){
        throw new UnsupportedOperationException();
    }

    public int[] getSupportModes(int display, int type){
        throw new UnsupportedOperationException();
    }

    /* just for defalut platform*/
    public boolean getHdmiFullscreen(int display) {
        throw new UnsupportedOperationException();
    }
    public int setHdmiFullscreen(int display, boolean full) {
        throw new UnsupportedOperationException();
    }

    /* interface for 3D mode setting*/
    public int getDisplaySupport3DMode(int display){
         throw new UnsupportedOperationException();
    }

    public int getDisplay3DMode(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplay3DMode(int display, int mode){
        throw new UnsupportedOperationException();
    }

    public int setDisplay3DMode(int display, int mode, int videoCropHeight){
        throw new UnsupportedOperationException();
    }

    public int setDisplay3DLayerOffset(int display, int offset){
        throw new UnsupportedOperationException();
    }

    public boolean isCurrent3Doutput(int display) {
        throw new UnsupportedOperationException();
    }

    /* interface for screen margin/offset setting */
    public int[] getDisplayMargin(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayMargin(int display, int margin_x, int margin_y){
        throw new UnsupportedOperationException();
    }

    public int[] getDisplayOffset(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayOffset(int display, int offset_x, int offset_y){
        throw new UnsupportedOperationException();
    }

     /* interface for display enhance effect */
    public int getDisplayEdge(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayEdge(int display, int value){
        throw new UnsupportedOperationException();
    }

    public int getDisplayDetail(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayDetail(int display, int value){
        throw new UnsupportedOperationException();
    }

    public int getDisplayBright(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayBright(int display, int value){
        throw new UnsupportedOperationException();
    }

    public int getDisplayDenoise(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayDenoise(int display, int value){
        throw new UnsupportedOperationException();
    }

    public int getDisplaySaturation(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplaySaturation(int display, int value){
        throw new UnsupportedOperationException();
    }

    public int getDisplayContrast(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayContrast(int display, int value){
        throw new UnsupportedOperationException();
    }

    public int getDisplayEnhanceMode(int display){
        throw new UnsupportedOperationException();
    }

    public int setDisplayEnhanceMode(int display, int mode){
        throw new UnsupportedOperationException();
    }

    public boolean getBlackWhiteMode(int display) {
        throw new UnsupportedOperationException();
    }

    public int setBlackWhiteMode(int display, boolean on) {
        throw new UnsupportedOperationException();
    }

    public int getSmartBacklight(int display) {
        throw new UnsupportedOperationException();
    }

    public int setSmartBacklight(int display, int mode) {
        throw new UnsupportedOperationException();
    }

    public boolean getReadingMode(int display) {
        throw new UnsupportedOperationException();
    }

    public int setReadingMode(int display, boolean on) {
        throw new UnsupportedOperationException();
    }

    public int getColorTemperature(int display){
        throw new UnsupportedOperationException();
    }

    public int setColorTemperature(int display, int value){
        throw new UnsupportedOperationException();
    }

    static public int getDisplayModeFromFormat(int format) {
        throw new UnsupportedOperationException();
    }

    static public int getDisplayTypeFromFormat(int format) {
        throw new UnsupportedOperationException();
    }

    public int makeDisplayFormat(int type, int mode) {
        throw new UnsupportedOperationException();
    }

    public boolean supportedSNRSetting(int display) {
        throw new UnsupportedOperationException();
    }

    public int getSNRFeatureMode(int display) {
        throw new UnsupportedOperationException();
    }

    public int[] getSNRStrength(int display) {
        throw new UnsupportedOperationException();
    }

    public int setSNRConfig(int display, int mode, int ystrength, int ustrength, int vstrength) {
        throw new UnsupportedOperationException();
    }

    // exten for hdmi interface
    public final class SinkInfo {
        public int Type;
        public int[] SupportedModes;
        public int CurrentMode;
        public int UserSetting;
        // true when current output mode is equal to native mode
        public boolean IsNative;
    }

    public int getHdmiUserSetting(int display){
        throw new UnsupportedOperationException();
    }

    public int getHdmiNativeMode(int display){
        throw new UnsupportedOperationException();
    }

    public SinkInfo getSinkInfo(int display) {
        throw new UnsupportedOperationException();
    }

    /* hdcp for homlet platform */
    public int configHdcp(boolean enable) {
        throw new UnsupportedOperationException();
    }
    public int getConnectedHdcpLevel() {
        throw new UnsupportedOperationException();
    }
    public int getAuthorizedStatus() {
        throw new UnsupportedOperationException();
    }
}

