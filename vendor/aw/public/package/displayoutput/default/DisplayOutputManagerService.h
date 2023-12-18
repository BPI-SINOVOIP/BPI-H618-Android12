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

#pragma once

#include <aidl/vendor/display/output/BnDisplayOutputManager.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>

namespace aidl {
namespace vendor {
namespace display {
namespace output {

using ::android::sp;
using ::vendor::display::config::V1_0::IDisplayConfig;

class DisplayOutputManagerService : public BnDisplayOutputManager {
  ::ndk::ScopedAStatus debug(bool in_on) override;
  ::ndk::ScopedAStatus getDisplayOutput(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayOutput(int32_t in_display, int32_t in_format, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayOutputType(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayOutputMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayOutputMode(int32_t in_display, int32_t in_mode, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayOutputPixelFormat(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayOutputPixelFormat(int32_t in_display, int32_t in_format, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayOutputCurDataspaceMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayOutputDataspaceMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayOutputDataspaceMode(int32_t in_display, int32_t in_mode, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus isSupportHdmiMode(int32_t in_display, int32_t in_mode, bool* _aidl_return) override;
  ::ndk::ScopedAStatus getSupportModes(int32_t in_display, int32_t in_type, std::vector<int32_t>* _aidl_return) override;
  ::ndk::ScopedAStatus getHdmiFullscreen(int32_t in_display, bool* _aidl_return) override;
  ::ndk::ScopedAStatus setHdmiFullscreen(int32_t in_display, bool in_full, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplaySupport3DMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplay3DMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplay3DMode(int32_t in_display, int32_t in_mode, int32_t in_videoCropHeight, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplay3DLayerOffset(int32_t in_display, int32_t in_offset, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayMargin(int32_t in_display, std::vector<int32_t>* _aidl_return) override;
  ::ndk::ScopedAStatus getDisplayOffset(int32_t in_display, std::vector<int32_t>* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayMargin(int32_t in_display, int32_t in_margin_x, int32_t in_margin_y, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setDisplayOffset(int32_t in_display, int32_t in_offset_x, int32_t in_offset_y, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getEnhanceComponent(int32_t in_display, int32_t in_type, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setEnhanceComponent(int32_t in_display, int32_t in_type, int32_t in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBlackWhiteMode(int32_t in_display, bool* _aidl_return) override;
  ::ndk::ScopedAStatus setBlackWhiteMode(int32_t in_display, bool in_on, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getSmartBacklight(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setSmartBacklight(int32_t in_display, int32_t in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getReadingMode(int32_t in_display, bool* _aidl_return) override;
  ::ndk::ScopedAStatus setReadingMode(int32_t in_display, bool in_on, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getColorTemperature(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setColorTemperature(int32_t in_display, int32_t in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus supportedSNRSetting(int32_t in_display, bool* _aidl_return) override;
  ::ndk::ScopedAStatus getSNRFeatureMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getSNRStrength(int32_t in_display, std::vector<int32_t>* _aidl_return) override;
  ::ndk::ScopedAStatus setSNRConfig(int32_t in_display, int32_t in_mode, int32_t in_ystrength, int32_t in_ustrength, int32_t in_vstrength, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getHdmiNativeMode(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getHdmiUserSetting(int32_t in_display, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus configHdcp(bool enable, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getConnectedHdcpLevel(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getAuthorizedStatus(int32_t* _aidl_return) override;

public:
    DisplayOutputManagerService();
    ~DisplayOutputManagerService();


private:
    int mState;
    sp<IDisplayConfig> mDisplayConfig;
    bool DEBUG_ON = false;
    int mPlatform = 0;   //0:default 1:homlet

    //for default platform
    bool mEnhanceSave = true;   //for temp
    int32_t mSmartBacklightMode = 0;
    bool mReadingMode = false;
    bool mBlackWhiteMode = false;
    int32_t mColorTemperature   = 25;
    int32_t mCurrentHdmiMode    = 0;
    bool mHdmiFullscreen    = false;
    int32_t mCurrent3DMode  = 0;
    int32_t mMarginWidth    = 100;
    int32_t mMarginHeight   = 100;
    int __setColorTemperature(int32_t in_display);
    void initDefaultPlatform();

    //for homlet platform
    int _getType(int32_t display);
    int _getMode(int32_t display);
    int _setMode(int display, int fmt);
};
}  // namespace output
}  // namespace display
}  // namespace vendor
}  // namespace aidl
