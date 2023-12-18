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

package vendor.display.output;
@VintfStability
interface IDisplayOutputManager {
    void debug(boolean on);

    /* interface for output mode/type setting */
    int getDisplayOutput(int display);
    int setDisplayOutput(int display, int format);

    int getDisplayOutputType(int display);
    int getDisplayOutputMode(int display);
    int setDisplayOutputMode(int display, int mode);

    /* interface for output pixelFormat/dataspace setting */
    int getDisplayOutputPixelFormat(int display);
    int setDisplayOutputPixelFormat(int display, int format);
    int getDisplayOutputCurDataspaceMode(int display);
    int getDisplayOutputDataspaceMode(int display);
    int setDisplayOutputDataspaceMode(int display, int mode);

    boolean isSupportHdmiMode(int display, int mode);
    int[] getSupportModes(int display, int type);

    /* interface just for default platfom*/
    boolean getHdmiFullscreen(int display);
    int setHdmiFullscreen(int display, boolean full);

    /* interface for 3D mode setting */
    int getDisplaySupport3DMode(int display);
    int getDisplay3DMode(int display);
    int setDisplay3DMode(int display, int mode, int videoCropHeight);
    int setDisplay3DLayerOffset(int display, int offset);

    /* interface for screen margin/offset setting */
    int[] getDisplayMargin(int display);
    int[] getDisplayOffset(int display);
    int setDisplayMargin(int display, int margin_x, int margin_y);
    int setDisplayOffset(int display, int offset_x, int offset_y);

    /* interface for display enhance effect */
    int getEnhanceComponent(int display, int type);
    int setEnhanceComponent(int display, int type, int value);

    int getSmartBacklight(int display);
    int setSmartBacklight(int display, int mode);
    boolean getReadingMode(int display);
    int setReadingMode(int display, boolean on);
    int getColorTemperature(int display);
    int setColorTemperature(int display, int value);

    /* interface for snr feature */
    boolean supportedSNRSetting(int display);
    int getSNRFeatureMode(int display);
    int[] getSNRStrength(int display);
    int setSNRConfig(int display, int mode, int ystrength, int ustrength, int vstrength);

    /* extern for hdmi interface */
    int getHdmiNativeMode(int display);
    int getHdmiUserSetting(int display);

    /* hdcp for homlet platform */
    int configHdcp(boolean enable);
    int getConnectedHdcpLevel();
    int getAuthorizedStatus();

    boolean getBlackWhiteMode(int display);
    int setBlackWhiteMode(int display, boolean on);
}
