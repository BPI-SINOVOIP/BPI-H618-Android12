/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android-base/stringprintf.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <utils/Log.h>
#include <json/json.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "DeviceConfig.h"
#include "hardware/sunxi_display2.h"

using android::base::StringPrintf;
using DeviceHardwareInfo = sunxi::DeviceConfig::DeviceHardwareInfo;

namespace sunxi {

DeviceConfig::DeviceHardwareInfo DeviceConfig::sDefaultConfig = {
    .DisplayId = 0,
    .InterfaceType = DISP_OUTPUT_TYPE_LCD,
    .DisplayEnginePortId = 0,
    .OverrideFramebufferSize = 0,
    .FramebufferWidth  = 0,
    .FramebufferHeight = 0,
    .HotplugSupported  = 0,
    .AfbcSupported     = 0,
    .BandwidthLimited  = 49152000,
    .DefaultOutputMode = 0,
};

DeviceConfig::DeviceConfig(): mConfigFilePath(), mDisplayEngineVersion(0), mHardwareInfoList() { }

int DeviceConfig::init()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mConfigFilePath = getDeviceConfigFilePath();
    mHardwareInfoList.clear();
    parseDeviceConfigFile();
    return 0;
}

int DeviceConfig::getDeviceNumber() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mHardwareInfoList.size();
}

std::unique_ptr<DeviceHardwareInfo> DeviceConfig::getDeviceHardwareInfo(int displayEnginePortId)
{
    std::lock_guard<std::mutex> lock(mMutex);
    for (const auto& info : mHardwareInfoList) {
        if (info.DisplayEnginePortId == displayEnginePortId) {
            DeviceHardwareInfo* out = new DeviceHardwareInfo();
            *out = info;
            return std::unique_ptr<DeviceHardwareInfo>(out);
        }
    }
    return nullptr;
}

struct InterfaceTypeMap {
    int TypeCode;
    const char* Name;
};

static InterfaceTypeMap sInterfaceTypeMap[] =
{
    { DISP_OUTPUT_TYPE_NONE, "NONE" },
    { DISP_OUTPUT_TYPE_LCD , "LCD"  },
    { DISP_OUTPUT_TYPE_TV  , "CVBS" },
    { DISP_OUTPUT_TYPE_HDMI, "HDMI" },
    { DISP_OUTPUT_TYPE_VGA , "VGA"  },
    { DISP_OUTPUT_TYPE_VDPO, "VDPO" },
    { DISP_OUTPUT_TYPE_EDP , "EDP"  },
};

#define arraySize(_arry) (sizeof(_arry) / sizeof(_arry[0]))

static const char* interfaceType2Name(int type)
{
    for (int i = 0; i < arraySize(sInterfaceTypeMap); i++) {
        if (sInterfaceTypeMap[i].TypeCode == type)
            return sInterfaceTypeMap[i].Name;
    }
    return "Unknow";
}

static int name2InterfaceType(const char* name)
{
    for (int i = 0; i < arraySize(sInterfaceTypeMap); i++) {
        if (!strcmp(sInterfaceTypeMap[i].Name, name))
            return sInterfaceTypeMap[i].TypeCode;
    }
    return -1;
}

static std::string sConfigDir = "/vendor/etc/dispconfigs/";
static inline void jsonParser(Json::CharReader::Factory const& read, Json::Value& value, std::string file);

std::string DeviceConfig::getDeviceConfigFilePath()
{
    // first get config file by vendor device name
    char deviceName[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.product.vendor.device", deviceName, nullptr);
    ALOGI("Device name: %s", deviceName);
    std::string jsonpath = sConfigDir + std::string(deviceName) + ".json";

    // if failed, try to get config file name from confg.json
    if (access(jsonpath.c_str(), F_OK) != 0) {
        Json::CharReaderBuilder read;
        Json::Value value;
        std::string config = sConfigDir + "config.json";
        jsonParser(read, value, config);
        std::string name = value["DeviceConfig"].asString();
        jsonpath = sConfigDir + name;
    }

    return jsonpath;
}

static inline void jsonParser(Json::CharReader::Factory const& read, Json::Value& value, std::string file)
{
    std::ifstream in(file);
    Json::String errs;

    if (!Json::parseFromStream(read, in, &value, &errs)) {
        if (!value.isObject()) {
            ALOGE("parse json '%s' return error", file.c_str());
        }
        ALOGE("Message: %s", errs.c_str());
    }
}

void DeviceConfig::parseDeviceConfigFile()
{
    ALOGD("Device config file: %s", mConfigFilePath.c_str());

    std::ifstream t(mConfigFilePath);
    std::string str((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());

    Json::CharReaderBuilder read;
    Json::Value value;
    jsonParser(read, value, mConfigFilePath);

    Json::Value version = value["DisplayEngineVersion"];
    if (version.isInt()) {
        mDisplayEngineVersion = version.asInt();
    }

    int totalDisplayDevice = 0;
    Json::Value displayCount = value["DisplayNumber"];
    if (displayCount.isInt()) {
        totalDisplayDevice = displayCount.asInt();
    }

    for (int i = 0; i < totalDisplayDevice; i++) {
        std::string name = std::string("Display") + std::to_string(i);
        ALOGD("Parse '%s' config", name.c_str());
        Json::Value config = value[name.c_str()];
        if (config.isObject()) {
            DeviceHardwareInfo info;
            if (parseOneConfig(config, info)) {
                info.DisplayId = i;
                mHardwareInfoList.push_back(info);
            }
        }
    }
}

#define PARSE_INT(_obj, _info, _tag, _def) \
{ \
    Json::Value item = _obj[#_tag]; \
    if (item.isInt()) { \
        _info._tag = item.asInt(); \
    } else { \
        _info._tag = _def; \
    } \
}

bool DeviceConfig::parseOneConfig(Json::Value& obj, DeviceConfig::DeviceHardwareInfo& info)
{
    Json::Value item = obj["InterfaceType"];
    if (item.isString()) {
        info.InterfaceType = name2InterfaceType(item.asString().c_str());
    }
    PARSE_INT(obj, info, DisplayEnginePortId, 0);
    PARSE_INT(obj, info, DisplayEnginePortId, 0);
    PARSE_INT(obj, info, OverrideFramebufferSize, 0);
    PARSE_INT(obj, info, FramebufferWidth,  0);
    PARSE_INT(obj, info, FramebufferHeight, 0);
    PARSE_INT(obj, info, HotplugSupported,  0);
    PARSE_INT(obj, info, AfbcSupported,     0);
    PARSE_INT(obj, info, BandwidthLimited,  0);
    PARSE_INT(obj, info, DefaultOutputMode, 0);
    PARSE_INT(obj, info, DpiX, 0);
    PARSE_INT(obj, info, DpiY, 0);
    PARSE_INT(obj, info, HardwareRotateSupported, 0);
    return true;
}

void DeviceConfig::dump(std::string& out) const
{
    std::lock_guard<std::mutex> lock(mMutex);
    out += StringPrintf("HAL device config file: %s\n", mConfigFilePath.c_str());
    out += StringPrintf("Displays (%zu entries)\n", mHardwareInfoList.size());
    for (const auto& info : mHardwareInfoList) {
        out += StringPrintf("+ Display{id=%d, framebuffer=%dx%d(override=%d), dpi=%dx%d}\n",
                info.DisplayId, info.FramebufferWidth,
                info.FramebufferHeight, info.OverrideFramebufferSize,
                info.DpiX, info.DpiY);
        out += StringPrintf("   DisplayEnginePortId=%d\n", info.DisplayEnginePortId);
        out += StringPrintf("   InterfaceType=%s\n", interfaceType2Name(info.InterfaceType));
        out += StringPrintf("   DefaultOutputMode=%d\n", info.DefaultOutputMode);
        out += StringPrintf("   HotplugSupported=%d\n", info.HotplugSupported);
        out += StringPrintf("   AfbcSupported=%d\n", info.AfbcSupported);
        out += StringPrintf("   HardwareRotateSupported=%d\n", info.HardwareRotateSupported);
        out += StringPrintf("   BandwidthLimited=%d Bytes per frame\n", info.BandwidthLimited);
    }
}

}
