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

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Json { class Value; }

namespace sunxi {

class DeviceConfig {
public:
    struct DeviceHardwareInfo {
        int DisplayId;
        int InterfaceType;
        int DisplayEnginePortId;
        int OverrideFramebufferSize;
        int FramebufferWidth;
        int FramebufferHeight;
        int DpiX;
        int DpiY;
        int HotplugSupported;
        int AfbcSupported;
        int HardwareRotateSupported;
        int BandwidthLimited;
        int DefaultOutputMode;
    };

    DeviceConfig();
    int init();
    int getDeviceNumber() const;
    std::unique_ptr<DeviceHardwareInfo> getDeviceHardwareInfo(int displayEnginePortId);
    const std::vector<DeviceHardwareInfo>& getDeviceList() const {
        return mHardwareInfoList;
    }
    void dump(std::string& out) const;

private:
    std::string getDeviceConfigFilePath();
    void parseDeviceConfigFile();
    bool parseOneConfig(Json::Value& obj, DeviceHardwareInfo& info);

private:
    static DeviceHardwareInfo sDefaultConfig;
    mutable std::mutex mMutex;
    std::string mConfigFilePath;
    int mDisplayEngineVersion;
    std::vector<DeviceHardwareInfo> mHardwareInfoList;
};

} // namespace sunxi
