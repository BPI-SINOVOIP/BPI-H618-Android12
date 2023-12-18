/**
 * Copyright (c) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner;

/**
 * This must be kept manually in sync with system/security/keystore until AIDL
 * can generate both Java and C++ bindings.
 *
 * @hide
 */
interface IDisplayService {
  int displayCtrl(int disp, int cmd0, int cmd1, int data);
  int getVersion();
  int getHdmiMode();
  boolean setHdmiMode(int mode);
  boolean getSmartBacklight();
  void setSmartBacklight(boolean on);
  boolean getSmartBacklightDemo();
  void setSmartBacklightDemo(boolean on);
  boolean getColorEnhance();
  void setColorEnhance(boolean on);
  boolean getColorEnhanceDemo();
  void setColorEnhanceDemo(boolean on);
  boolean getReadingMode();
  void setReadingMode(boolean on);
  int getColorTemperature();
  void setColorTemperature(int value);
  int getHSLBright();
  void setHSLBright(int value);
  int getHSLContrast();
  void setHSLContrast(int value);
  int getHSLSaturation();
  void setHSLSaturation(int value);
  int get3DMode();
  boolean set3DMode(int mode);
  int getMarginWidth();
  void setMarginWidth(int scale);
  int getMarginHeight();
  void setMarginHeight(int scale);
  boolean getHdmiFullscreen();
  void setHdmiFullscreen(boolean full);
}
