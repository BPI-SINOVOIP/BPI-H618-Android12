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

#ifndef SUNXI_GENERAL_DEVICE_FACTORY_H
#define SUNXI_GENERAL_DEVICE_FACTORY_H

#include "DeviceManager.h"
#include "Disp2Device.h"
#include "vendorservice/default/IVendorInterface.h"
#include "configs/DeviceConfig.h"

namespace sunxi {

class GeneralDeviceFactory: public IDeviceFactory, public IVendorInterface {
public:
    GeneralDeviceFactory();
   ~GeneralDeviceFactory();

    void scanConnectedDevice() override;
    void registerDeviceChangedListener(DeviceChangedListener* listener) override;
    std::shared_ptr<DeviceBase> getPrimaryDevice() override;
    int freezeOuput(int display, bool enabled) override;
    int setDeviceOutputInfo(int display, int xres, int yres, int refreshRate) override;
    int32_t getHwIdx(int32_t logicId)

private:
    std::shared_ptr<DeviceBase> findDevice(int display);
    int bringupDevices();
    std::shared_ptr<Disp2Device> createDisplayDevice(int hardwareid);
    DeviceBase::Config makeLcdDefaultConfig() const;
    DeviceBase::Config makeConfigFromHardwareInfo(const DeviceConfig::DeviceHardwareInfo& info);

    const static int MaxDisplayCount = 2;
    std::vector<ConnectedInfo> mConnectedDisplays;
    DeviceChangedListener* mListener;
    std::mutex mDeviceMutex;

    std::unique_ptr<DeviceConfig> mDeviceConfigInfo;
    std::vector<DeviceConfig::DeviceHardwareInfo> mHardwareDevices;
};

} // namespace sunxi
#endif
