/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/



package android.os;

/** @hide */
interface IDisplayOutputManager {
 
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
 
     /* interface for 3D mode setting */
     int getDisplaySupport3DMode(int display);
     int setDisplay3DMode(int display, int mode, int videoCropHeight);
     int setDisplay3DLayerOffset(int display, int offset);
 
     /* interface for screen margin/offset setting */
     int[] getDisplayMargin(int display);
     int[] getDisplayOffset(int display);
     int setDisplayMargin(int display, int margin_x, int margin_y);
     int setDisplayOffset(int display, int offset_x, int offset_y);
 
     /* interface for display enhance effect */
     int getDisplayEdge(int display);
     int setDisplayEdge(int display, int edge);
     int getDisplayDetail(int display);
     int setDisplayDetail(int display, int detail);
     int getDisplayBright(int display);
     int setDisplayBright(int display, int bright);
     int getDisplayDenoise(int display);
     int setDisplayDenoise(int display, int denoise);
     int getDisplaySaturation(int display);
     int setDisplaySaturation(int display, int saturation);
     int getDisplayContrast(int display);
     int setDisplayContrast(int display, int contrast);
     int getDisplayEnhanceMode(int display);
     int setDisplayEnhanceMode(int display, int mode);
 }
