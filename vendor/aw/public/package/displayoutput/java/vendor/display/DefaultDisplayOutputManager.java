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


public class DefaultDisplayOutputManager extends DisplayOutputManager {
    private static final String TAG = "DefaultDisplayOutputManager";
    private DisplayOutputManagerImpl mImpl = null;

    public DefaultDisplayOutputManager() {
        mImpl = new DisplayOutputManagerImpl();
        mImpl.setTAG(TAG);
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
        return mImpl.setDisplayOutputMode(display,mode);
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

    /* interface for 3D mode setting*/
    @Override
    public int getDisplay3DMode(int display) {
        return mImpl.getDisplay3DMode(display);
    }

    @Override
    public int setDisplay3DMode(int display, int mode) {
        return mImpl.setDisplay3DMode(display, mode);
    }

    @Override
    public int setDisplay3DMode(int display, int mode, int videoCropHeight) {
        return mImpl.setDisplay3DMode(display, mode, videoCropHeight);
    }

    /* interface for screen margin/offset setting */
    @Override
    public int[] getDisplayMargin(int display) {
        return mImpl.getDisplayMargin(display);
    }

    @Override
    public int setDisplayMargin(int display, int margin_x, int margin_y) {
        return mImpl.setDisplayMargin(display, margin_x, margin_y);
    }

    /* interface for display enhance effect*/
    @Override
    public int getDisplayBright(int display) {
        return mImpl.getDisplayBright(display);
    }

    @Override
    public int setDisplayBright(int display, int value) {
        return mImpl.setDisplayBright(display, value);
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
    public boolean getBlackWhiteMode(int display) {
        return mImpl.getBlackWhiteMode(display);
    }

    @Override
    public int setBlackWhiteMode(int display, boolean on) {
        return mImpl.setBlackWhiteMode(display, on);
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
    public boolean getReadingMode(int display){
        return mImpl.getReadingMode(display);
    }

    @Override
    public int setReadingMode(int display, boolean value) {
        return mImpl.setReadingMode(display, value);
    }

    @Override
    public int getColorTemperature(int display) {
        return mImpl.getColorTemperature(display);
    }

    @Override
    public int setColorTemperature(int display, int value) {
        return mImpl.setColorTemperature(display, value);
    }
}
