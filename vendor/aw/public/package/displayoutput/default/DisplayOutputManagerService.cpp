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

#include "DisplayOutputManagerService.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <log/log.h>

namespace aidl {
namespace vendor {
namespace display {
namespace output {

using ::vendor::display::config::V1_0::DisplayPortType;
using ::vendor::display::config::V1_0::PixelFormat;
//using ::vendor::display::config::V1_0::AspectRatio;
using ::vendor::display::config::V1_0::Dataspace;
using ::vendor::display::config::V1_0::LayerMode;
using ::vendor::display::config::V1_0::EnhanceItem;
using ::vendor::display::config::V1_0::ScreenMargin;
using ::vendor::display::config::V1_0::SNRInfo;
using ::vendor::display::config::V1_0::SNRFeatureMode;
using ::vendor::display::config::V1_0::HdcpAuthorizedStatus;
using ::vendor::display::config::V1_0::HdcpLevel;

using ::android::hardware::Return;

using namespace android;

#define MODULE_VERSION "V2.2.0"

#define LOGD(x, ...) {   \
    if (DEBUG_ON) {     \
        ALOGD(x, ##__VA_ARGS__);  \
    }                   \
}                       \

enum HIDL_set_cmd {
    HIDL_HDMI_MODE_CMD,
    HIDL_ENHANCE_MODE_CMD,
    HIDL_SML_MODE_CMD,
    HIDL_COLOR_TEMPERATURE_CMD,
    HIDL_SET3D_MODE,
    HIDL_SETMARGIN,
    HIDL_SETVIDEORATIO,
};

enum RESULT {
    ERROR = -1,
    SUCCESS = 0,
};

enum PLATFORM {
    DEFAULT = 0,
    HOMLET  = 1,
};

static const int maxEnhanceCount = 7;
static const int COLOR_TEMPERATURE_ATTR = 8;
static const char* property_list_[maxEnhanceCount] = {
    "persist.vendor.display.enhance_mode",
    "persist.vendor.display.enhance_bright",
    "persist.vendor.display.enhance_contrast",
    "persist.vendor.display.enhance_denoise",
    "persist.vendor.display.enhance_detail",
    "persist.vendor.display.enhance_edge",
    "persist.vendor.display.enhance_saturation",
};
static const char* kBlackWhiteModeProperty =    "persist.vendor.display.blackwhite_mode";
static const char* kSmartBacklightProperty =    "persist.vendor.display.smart_backlight";
static const char* kReadingModeProperty =       "persist.vendor.display.reading_mode";
static const char* kColorTemperatureProperty =  "persist.vendor.display.color_temperature";

::ndk::ScopedAStatus DisplayOutputManagerService::debug(bool in_on) {
    LOG(INFO) << "debug : " << in_on;
    DEBUG_ON = in_on;
    ALOGD("debug:%s", in_on ? "open" : "close");
    return ndk::ScopedAStatus::ok();
}

int DisplayOutputManagerService::_getType(int32_t display){
    DisplayPortType type = DisplayPortType::DISPLAY_PORT_NONE;
    if (mDisplayConfig != nullptr) {
        type = mDisplayConfig->getDisplayPortType(display);
    }
    LOGD("_getType type=%s", toString(type).c_str());
    return (int)type;
}

int DisplayOutputManagerService::_getMode(int32_t display){
    int error = -1;
    int activeMode;
    std::function<void(int32_t, uint32_t)> cb = [&](int32_t err, uint32_t mode) {
        error = err;
        activeMode = mode;
        LOGD("_getMode err=%d, mode=%d", err, mode);
    };

    if (mDisplayConfig != nullptr) {
        Return<void> ret = mDisplayConfig->getActiveMode(display, cb);
    }
    if (error == 0)
        return activeMode;
    return -1;

}

int DisplayOutputManagerService::_setMode(int display, int fmt) {
    int ret = ERROR;
    if (mDisplayConfig != nullptr) {
        ret = mDisplayConfig->setActiveMode(display, fmt);
    }
    LOGD("_setMode display=%d mode=%d ret=%d", display, fmt, ret);
    return ret;
}

::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOutput(
    int32_t in_display, int32_t* _aidl_return) {
    int type = _getType(in_display);
    int mode = _getMode(in_display);
    *_aidl_return = ((type & 0xff) << 8) | (mode & 0xff);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setDisplayOutput(
    int32_t in_display, int32_t in_format, int32_t* _aidl_return) {
    int ret = ERROR;
    int type = (in_format >> 8) & 0xff;
    int mode = in_format & 0xff;
    if (_getType(in_display) != type) {
        ALOGE("setDisplayOutput: device type error");
    } else {
        ret = _setMode(in_display, mode);
    }
    *_aidl_return = ret;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOutputType(
    int32_t in_display, int32_t* _aidl_return) {
    *_aidl_return = _getType(in_display);
    return ndk::ScopedAStatus::ok();

}
::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOutputMode(
    int32_t in_display, int32_t* _aidl_return) {
    int value = 0;
    if (mPlatform == DEFAULT) {
        value = mCurrentHdmiMode;
    } else {
        value = _getMode(in_display);
    }
    *_aidl_return = value;
    LOGD("getDisplayOutputMode display=%d value=%d", in_display, value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setDisplayOutputMode(
    int32_t in_display, int32_t in_mode, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mPlatform == DEFAULT) {
        mCurrentHdmiMode = in_mode;
        ret = mDisplayConfig->setDisplayArgs(in_display, HIDL_HDMI_MODE_CMD, in_mode, 0);
    } else {
        ret = _setMode(in_display, in_mode);
    }
    *_aidl_return = ret;
    LOGD("setDisplayOutputMode display=%d value=%d ret=%d", in_display, in_mode, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOutputPixelFormat(
    int32_t in_display, int32_t* _aidl_return) {
    PixelFormat fmt = PixelFormat::PIXEL_FORMAT_AUTO;
    if (mDisplayConfig != nullptr) {
        fmt = mDisplayConfig->getPixelFormat(in_display);
    }
    *_aidl_return = (int)fmt;
    LOGD("getDisplayOutputPixelFormat type=%s", toString(fmt).c_str());
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setDisplayOutputPixelFormat(
    int32_t in_display, int32_t in_value, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mDisplayConfig != nullptr) {
        ret = mDisplayConfig->setPixelFormat(in_display, (PixelFormat)in_value);
    }
    *_aidl_return = ret;
    LOGD("setDisplayEdge display=%d value=%d ret=%d", in_display, in_value, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOutputCurDataspaceMode(
    int32_t in_display, int32_t* _aidl_return) {
    // TODO:
    *_aidl_return = 0;
    LOGD("getDisplayOutputCurDataspaceMode not support display=%d", in_display);
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOutputDataspaceMode(
    int32_t in_display, int32_t* _aidl_return) {
    Dataspace value = Dataspace::DATASPACE_MODE_AUTO;
    if (mDisplayConfig != nullptr) {
        value = mDisplayConfig->getDataspaceMode(in_display);
    }
    *_aidl_return = (int)value;
    LOGD("getDisplayOutputDataspaceMode type=%s", toString(value).c_str())
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus DisplayOutputManagerService::setDisplayOutputDataspaceMode(
    int32_t in_display, int32_t in_value, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mDisplayConfig != nullptr) {
        ret = mDisplayConfig->setDataspaceMode(in_display, (Dataspace)in_value);
    }
    *_aidl_return = ret;
    LOGD("setDisplayOutputDataspaceMode display=%d value=%d ret=%d", in_display, in_value, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::isSupportHdmiMode(
    int32_t in_display, int32_t in_mode, bool* _aidl_return) {
    bool ret = false;
    std::vector<int32_t> hdmiModes;
    if (mDisplayConfig != nullptr) {
        mDisplayConfig->getSupportedModes(in_display, [&](const auto &list){
            size_t count = list.size();
            LOGD("getSupportedModes size=%d", (int)count);
            for (size_t i=0 ; i < count; i++) {
                hdmiModes.push_back(list[i]);
            }
        });
    }
    if (0 < std::count(hdmiModes.begin(), hdmiModes.end(), in_mode)) {
        ret = true;
    }
    *_aidl_return = ret;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getSupportModes(
    int32_t in_display, int32_t in_type, std::vector<int32_t>* _aidl_return) {
    if (mDisplayConfig != nullptr) {
        mDisplayConfig->getSupportedModes(in_display, [&](const auto &list){
            size_t count = list.size();
            LOGD("getSupportedModes size=%d", (int)count);
            for (size_t i=0 ; i < count; i++) {
                (*_aidl_return).push_back(list[i]);
            }
        });
    }
    LOGD("getSupportModes display=%d type=%d", in_display, in_type);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getHdmiFullscreen(
    int32_t in_display, bool* _aidl_return) {
    bool value = false;
    if (mPlatform == DEFAULT) {
        value = mHdmiFullscreen;
    }
    *_aidl_return = value;
    LOGD("getHdmiFullscreen display=%d value=%d", in_display, value?1:0);
    return ndk::ScopedAStatus::ok();

}
::ndk::ScopedAStatus DisplayOutputManagerService::setHdmiFullscreen(
    int32_t in_display, bool in_full, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mPlatform == DEFAULT) {
        mHdmiFullscreen = in_full;
        ret = mDisplayConfig->setDisplayArgs(in_display, HIDL_SETVIDEORATIO, 0, in_full ? 2 : 0);
    }
    *_aidl_return = ret;
    LOGD("setHdmiFullscreen display=%d value=%d ret=%d", in_display, in_full?1:0, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getDisplaySupport3DMode(
    int32_t in_display, int32_t* _aidl_return) {
    int ret = 0;
    bool support = false;
    if (mDisplayConfig != nullptr) {
        support = mDisplayConfig->supported3D(in_display);
        ret = support ? 1 : 0;
    }
    *_aidl_return = ret;
    LOGD("getDisplaySupport3DMode support=%d", ret);
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus DisplayOutputManagerService::getDisplay3DMode(
    int32_t in_display, int32_t* _aidl_return) {
    LayerMode value = LayerMode::LAYER_2D_ORIGINAL;
    if (mPlatform == DEFAULT) {
        value = (LayerMode)mCurrent3DMode;
    } else {
        value = mDisplayConfig->get3DLayerMode(in_display);
    }
    *_aidl_return = (int)value;
    LOGD("getDisplay3DMode display=%d value=%d", in_display, value);
    return ndk::ScopedAStatus::ok();

}

::ndk::ScopedAStatus DisplayOutputManagerService::setDisplay3DMode(
    int32_t in_display, int32_t in_mode, int32_t in_videoCropHeight, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mPlatform == DEFAULT) {
        mCurrent3DMode = in_mode;
        ret = mDisplayConfig->setDisplayArgs(in_display, HIDL_SET3D_MODE, in_mode, 0);
    } else {
        ret = mDisplayConfig->set3DLayerMode(in_display, (LayerMode)in_mode);
    }
    *_aidl_return = ret;
    LOGD("setDisplay3DMode display=%d value=%d,%d ret=%d",
        in_display, in_mode, in_videoCropHeight, ret);
    return ndk::ScopedAStatus::ok();

}
::ndk::ScopedAStatus DisplayOutputManagerService::setDisplay3DLayerOffset(
    int32_t in_display, int32_t in_offset, int32_t* _aidl_return) {
    // TODO:
    *_aidl_return = 0;
    LOGD("setDisplay3DLayerOffset not support display=%d value=%d", in_display, in_offset);
    return ndk::ScopedAStatus::ok();

}
::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayMargin(
    int32_t in_display, std::vector<int32_t>* _aidl_return) {
    if (mPlatform == DEFAULT) {
        (*_aidl_return).push_back(mMarginWidth);
        (*_aidl_return).push_back(mMarginHeight);
    } else {
        ScreenMargin margin;
        std::function<void(ScreenMargin)> cb = [&](ScreenMargin screen) {
            margin.left = screen.left;
            margin.right = screen.right;
            margin.top = screen.top;
            margin.bottom = screen.bottom;
            LOGD("getDisplayMargin:%d,%d,%d,%d",
                screen.right, screen.right, screen.top, screen.bottom);
        };
        Return<void> ret = mDisplayConfig->getScreenMargin(in_display, cb);
        (*_aidl_return).push_back(margin.left);
        (*_aidl_return).push_back(margin.right);
        (*_aidl_return).push_back(margin.top);
        (*_aidl_return).push_back(margin.bottom);
    }
    LOGD("getDisplayMargin display=%d value=%d,%d,%d,%d", in_display,
        (*_aidl_return)[0],  (*_aidl_return)[1], (*_aidl_return)[2], (*_aidl_return)[3]);
    return ndk::ScopedAStatus::ok();

}

::ndk::ScopedAStatus DisplayOutputManagerService::setDisplayMargin(
    int32_t in_display, int32_t in_margin_x, int32_t in_margin_y, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mPlatform == DEFAULT) {
        mMarginWidth = in_margin_x;
        mMarginHeight = in_margin_y;
        int data = in_margin_x << 16 | in_margin_y;
        ret = mDisplayConfig->setDisplayArgs(in_display, HIDL_SETMARGIN, 0, data);
    } else {
        ScreenMargin margin;;
        margin.left = margin.right  = in_margin_x;
        margin.top  = margin.bottom = in_margin_y;
        ret = mDisplayConfig->setScreenMargin(in_display, margin);
    }
    *_aidl_return = ret;
    LOGD("setDisplayMargin display=%d value=%d,%d ret=%d",
        in_display, in_margin_x, in_margin_y, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getDisplayOffset(
    int32_t in_display, std::vector<int32_t>* _aidl_return) {
    // TODO:
    LOGD("getDisplayOffset not support display=%d", in_display);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setDisplayOffset(
    int32_t in_display, int32_t in_offset_x, int32_t in_offset_y, int32_t* _aidl_return) {
    // TODO:
    *_aidl_return = 0;
    LOGD("setDisplayOffset not support display=%d value=%d,%d", in_display, in_offset_x, in_offset_y);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getEnhanceComponent(
    int32_t in_display, int32_t in_type, int32_t* _aidl_return) {
    int value = 0;
    if (mDisplayConfig != nullptr) {
        value = mDisplayConfig->getEnhanceComponent(in_display, (EnhanceItem)in_type);
    }
    *_aidl_return = value;
    LOGD("getEnhanceComponent display=%d type=%s value=%d",
        in_display, toString((EnhanceItem)in_type).c_str(), value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setEnhanceComponent(
    int32_t in_display, int32_t in_type, int32_t in_value, int32_t* _aidl_return){
    int ret = ERROR;
    if (mDisplayConfig != nullptr) {
        if ((EnhanceItem)in_type == EnhanceItem::ENHANCE_SATURATION
            && mBlackWhiteMode) {
            LOGD("setEnhanceComponent ENHANCE_SATURATION fail for in BlackWhite mode");
        } else {
            ret = mDisplayConfig->setEnhanceComponent(in_display, (EnhanceItem)in_type, in_value);

            if (!ret && mPlatform == DEFAULT) {
            std::stringstream ss;
            ss << in_value;
            base::SetProperty(property_list_[in_type], ss.str());
            }
        }
    }
    *_aidl_return = ret;
    LOGD("setEnhanceComponent display=%d type = %s value=%d ret=%d",
        in_display, toString((EnhanceItem)in_type).c_str(), in_value, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getBlackWhiteMode(
    int32_t in_display, bool* _aidl_return) {
    bool value = mBlackWhiteMode;
    *_aidl_return = value;
    LOGD("getBlackWhiteMode display=%d value=%d", in_display, value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setBlackWhiteMode(
    int32_t in_display, bool in_on, int32_t* _aidl_return) {
    int ret = ERROR;
    mBlackWhiteMode = in_on;
    if (mEnhanceSave) {
        std::stringstream ss;
        ss << in_on;
        base::SetProperty(kBlackWhiteModeProperty, ss.str());
    }
    int value = base::GetIntProperty(property_list_[(int)EnhanceItem::ENHANCE_SATURATION], 50);
    if (mBlackWhiteMode) {
        value = 0;
    }
    ret = mDisplayConfig->setEnhanceComponent(0, EnhanceItem::ENHANCE_SATURATION, value);
    *_aidl_return = ret;
    LOGD("setBlackWhiteMode display=%d value=%d ret=%d", in_display, (int)in_on, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getSmartBacklight(
    int32_t in_display, int32_t* _aidl_return) {
    int value = mSmartBacklightMode;
    *_aidl_return = value;
    LOGD("getSmartBacklight display=%d value=%d", in_display, value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setSmartBacklight(
    int32_t in_display, int32_t in_value, int32_t* _aidl_return) {
    int ret = ERROR;
    if (mDisplayConfig != nullptr) {
        ret = mDisplayConfig->setDisplayArgs(in_display, HIDL_SML_MODE_CMD, 0, in_value);

        if (!ret && mEnhanceSave) {
            mSmartBacklightMode = in_value;
            std::stringstream ss;
            ss << in_value;
            base::SetProperty(kSmartBacklightProperty, ss.str());
        }
    }
    *_aidl_return = ret;
    LOGD("setSmartBacklight display=%d value=%d ret=%d", in_display, in_value, ret);
    return ndk::ScopedAStatus::ok();
}

int DisplayOutputManagerService::__setColorTemperature(int32_t in_display) {
    int value = mColorTemperature;
    if (mReadingMode) {
        /* 70 ~ 200 */
        value = (value * 130 / 100) + 70;
    } else {
        /* 0 ~ 200 */
        value *= 2;
    }
    return mDisplayConfig->setDisplayArgs(in_display,
        HIDL_COLOR_TEMPERATURE_CMD, COLOR_TEMPERATURE_ATTR, value);
}

::ndk::ScopedAStatus DisplayOutputManagerService::getReadingMode(
    int32_t in_display, bool* _aidl_return) {
    bool value = mReadingMode;
    *_aidl_return = value;
    LOGD("getReadingMode display=%d value=%d", in_display, value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setReadingMode(
    int32_t in_display, bool in_on, int32_t* _aidl_return) {
    int ret = ERROR;
    mReadingMode = in_on;
    if (mEnhanceSave) {
        std::stringstream ss;
        ss << in_on;
        base::SetProperty(kReadingModeProperty, ss.str());
    }
    ret = __setColorTemperature(in_display);
    *_aidl_return = ret;
    LOGD("setReadingMode display=%d value=%d ret=%d", in_display, (int)in_on, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getColorTemperature(
    int32_t in_display, int32_t* _aidl_return) {
    int value = mColorTemperature;
    *_aidl_return = value;
    LOGD("getColorTemperature display=%d value=%d", in_display, value);
    return ndk::ScopedAStatus::ok();

}

::ndk::ScopedAStatus DisplayOutputManagerService::setColorTemperature(
    int32_t in_display, int32_t in_value, int32_t* _aidl_return) {
    int ret = ERROR;
    mColorTemperature = in_value;
    if (mEnhanceSave) {
        std::stringstream ss;
        ss << in_value;
        base::SetProperty(kColorTemperatureProperty, ss.str());
    }
    ret = __setColorTemperature(in_display);
    *_aidl_return = ret;
    LOGD("setColorTemperature display=%d value=%d ret=%d", in_display, (int)in_value, ret);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::supportedSNRSetting(
    int32_t in_display, bool* _aidl_return) {
    bool support = false;
    if (mDisplayConfig != nullptr) {
        support = mDisplayConfig->supportedSNRSetting(in_display);
    }
    *_aidl_return = (int)support;
    LOGD("supportedSNRSetting support=%s", support ? "true" : "false");
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getSNRFeatureMode(
    int32_t in_display, int32_t* _aidl_return) {
    int mode = (int)SNRFeatureMode::SNR_DISABLE;
    int error = -1;
    SNRInfo info;
    if (mDisplayConfig != nullptr) {
        mDisplayConfig->getSNRInfo(in_display, [&](int32_t ret, const SNRInfo& input){
            error = ret;
            info = input;
        });
    }

    if (error == 0) {
        mode = (int)info.mode;
    }
    *_aidl_return = mode;
    LOGD("getSNRFeatureMode display=%d value=%d error=%d", in_display, mode, error);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getSNRStrength(
    int32_t in_display, std::vector<int32_t>* _aidl_return) {
    int error = -1;
    SNRInfo info;
    if (mDisplayConfig != nullptr) {
        mDisplayConfig->getSNRInfo(in_display, [&](int32_t ret, const SNRInfo& input){
            error = ret;
            info = input;
        });
    }

    if (error == 0) {
        (*_aidl_return).push_back(info.y);
        (*_aidl_return).push_back(info.u);
        (*_aidl_return).push_back(info.v);
    }
    LOGD("getSNRStrength display=%d value=%d,%d,%d error=%d",
        in_display, info.y, info.u, info.v, error);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::setSNRConfig(
    int32_t in_display, int32_t in_mode, int32_t in_ystrength,
    int32_t in_ustrength, int32_t in_vstrength, int32_t* _aidl_return) {
    int ret = ERROR;
    SNRInfo info;
    info.mode = (SNRFeatureMode)in_mode;
    info.y = in_ystrength;
    info.u = in_ustrength;
    info.v = in_vstrength;
    if (mDisplayConfig != nullptr) {
        ret = mDisplayConfig->setSNRInfo(in_display, info);
    }
    *_aidl_return = ret;
    LOGD("setSNRConfig display=%d value=%d,%d,%d,%d ret=%d",
        in_display, in_mode, in_ystrength, in_ustrength, in_vstrength, ret);
    return ndk::ScopedAStatus::ok();
}


::ndk::ScopedAStatus DisplayOutputManagerService::getHdmiNativeMode(
    int32_t in_display, int32_t* _aidl_return) {
    int value = -1;
    if (mDisplayConfig != nullptr) {
        value = mDisplayConfig->getHDMINativeMode(in_display);
    }
    *_aidl_return = value;
    LOGD("getHdmiNativeMode value=%d", value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getHdmiUserSetting(
    int32_t in_display, int32_t* _aidl_return) {
    int value = -1;
    if (mDisplayConfig != nullptr) {
        value = mDisplayConfig->getHdmiUserSetting(in_display);
    }
    *_aidl_return = value;
    LOGD("getHdmiUserSetting value=%d", value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::configHdcp(
    bool enable, int32_t* _aidl_return) {
    int value = -1;
    if (mDisplayConfig != nullptr) {
        value = mDisplayConfig->configHdcp(enable);
    }
    *_aidl_return = value;
    LOGD("configHdcp ret=%d", value);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getConnectedHdcpLevel(
    int32_t* _aidl_return) {
    HdcpLevel value = HdcpLevel::HDCP_UNKNOWN;
    if (mDisplayConfig != nullptr) {
        value = mDisplayConfig->getConnectedHdcpLevel();
    }
    *_aidl_return = (int)value;
    LOGD("getConnectedHdcpLevel value=%s", ::vendor::display::config::V1_0::toString(value).c_str());
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DisplayOutputManagerService::getAuthorizedStatus(
    int32_t* _aidl_return) {
    HdcpAuthorizedStatus status = HdcpAuthorizedStatus::ERROR;
    if (mDisplayConfig != nullptr) {
        status = mDisplayConfig->getAuthorizedStatus();
    }
    *_aidl_return = (int)status;
    LOGD("getAuthorizedStatus value=%s", ::vendor::display::config::V1_0::toString(status).c_str());
    return ndk::ScopedAStatus::ok();
}

void DisplayOutputManagerService::initDefaultPlatform() {
    int value = -1;
    value  = base::GetIntProperty(property_list_[(int)EnhanceItem::ENHANCE_MODE], -1);
    if (value != -1) {
        mDisplayConfig->setEnhanceComponent(0, EnhanceItem::ENHANCE_MODE, value);
    }

    value  = base::GetIntProperty(property_list_[(int)EnhanceItem::ENHANCE_BRIGHT], 50);
    mDisplayConfig->setEnhanceComponent(0, EnhanceItem::ENHANCE_BRIGHT, value);
    value  = base::GetIntProperty(property_list_[(int)EnhanceItem::ENHANCE_CONTRAST], 50);
    mDisplayConfig->setEnhanceComponent(0, EnhanceItem::ENHANCE_CONTRAST, value);
    value  = base::GetIntProperty(property_list_[(int)EnhanceItem::ENHANCE_SATURATION], 50);
    mDisplayConfig->setEnhanceComponent(0, EnhanceItem::ENHANCE_SATURATION, value);

    value  = base::GetIntProperty(kColorTemperatureProperty, -1);
    if (value != -1) {
        mColorTemperature = value;
    }
    mReadingMode  = base::GetBoolProperty(kReadingModeProperty, false);
    __setColorTemperature(0);

    value  = base::GetIntProperty(kSmartBacklightProperty, -1);
    if (value != -1) {
        mSmartBacklightMode = value;
        mDisplayConfig->setDisplayArgs(0, HIDL_SML_MODE_CMD, 0, value);
    }
}
DisplayOutputManagerService::DisplayOutputManagerService() {
    LOG(INFO) << "DisplayOutputManagerService is started";
    LOG(INFO) << "DisplayOutputManagerService current version " << MODULE_VERSION;
    mState = 0;
    if (mDisplayConfig == nullptr) {
        mDisplayConfig = IDisplayConfig::getService();
    }
    if (mDisplayConfig == nullptr) {
        LOG(ERROR) << "failed to get hwcomposer service";
        return;
    }

    std::string platform = base::GetProperty("ro.build.characteristics", "");
	//bpi, both tablet and homlet use homlet display
    if (platform == "tablet" || platform == "homlet") {
        mPlatform = HOMLET;
        LOG(INFO) << "BPI: platform is homlet or tablet";
    } else {
        LOG(INFO) << "platform is default";
    }

    if (mPlatform == DEFAULT) {
        initDefaultPlatform();
    }
}

DisplayOutputManagerService::~DisplayOutputManagerService() {
    LOG(INFO) << "DisplayOutputManagerService exit";
}

}  // namespace output
}  // namespace display
}  // namespace vendor
}  // namespace aidl
