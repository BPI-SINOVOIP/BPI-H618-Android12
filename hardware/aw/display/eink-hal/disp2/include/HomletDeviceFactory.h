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

#ifndef HOMLET_DEVICE_FACTORY_H
#define HOMLET_DEVICE_FACTORY_H

#include <map>
#include <mutex>

#include "DeviceManager.h"
#include "Disp2Device.h"
#include "IVendorInterface.h"

namespace sunxi {

class HomletDeviceFactory: public IDeviceFactory, public IVendorInterface {
public:
    HomletDeviceFactory();
   ~HomletDeviceFactory();

    void scanConnectedDevice() override;
    void registerDeviceChangedListener(DeviceChangedListener* listener) override;
    std::shared_ptr<DeviceBase> getPrimaryDevice() override;

    int updateConnectedDevices(std::vector<DeviceInfo *>& devices) override;
    int blankDevice(int display, bool enabled) override;
    int setDeviceOutputInfo(int display, int xres, int yres, int refreshRate) override;

private:
    std::shared_ptr<DeviceBase> findDevice(int display);
    void sortByDisplayId(std::vector<DeviceInfo *>& devices);
    void notifyDeviceChanged();

private:
    // hardware id to device mapping
    std::map<int, std::shared_ptr<Disp2Device>> mHardwareDevices;
    std::vector<ConnectedInfo> mConnectedDisplays;
    DeviceChangedListener* mListener;

    mutable std::mutex mDeviceMutex;
    mutable std::condition_variable mCondition;
};

} // namespace sunxi

#endif
