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

#ifndef SUNXI_TABLET_DEVICE_FACTORY_H
#define SUNXI_TABLET_DEVICE_FACTORY_H

#include "DeviceManager.h"
#include "Disp2Device.h"
#include "vendorservice/default/IVendorInterface.h"

namespace sunxi {

class TabletDeviceFactory: public IDeviceFactory, public IVendorInterface {
public:
    TabletDeviceFactory();
   ~TabletDeviceFactory();

    void scanConnectedDevice() override;
    void registerDeviceChangedListener(DeviceChangedListener* listener) override;
    std::shared_ptr<DeviceBase> getPrimaryDevice() override;
    int freezeOuput(int display, bool enabled) override;
    int32_t getHwIdx(int32_t logicId);

private:
    enum {
        eTopologySingleScreen = 0,
        eTopologyDualScreen   = 1,
        eTopologyMaxNumber    = 2,
    };

    std::vector<DeviceInfo> getOutputTopology() const;
    int bringupDevices();
    std::shared_ptr<Disp2Device> createDisplayDevice(int hardwareid);

    DeviceBase::Config makeLcdDefaultConfig() const;
    void fixDisplaySizebyMemory(DeviceBase::Config &config);

    const static int MaxDisplayCount = 2;
    std::vector<ConnectedInfo> mConnectedDisplays;
    DeviceChangedListener* mListener;
    std::mutex mDeviceMutex;
};

} // namespace sunxi
#endif
