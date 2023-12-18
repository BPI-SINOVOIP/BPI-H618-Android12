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

#include <hardware/hwcomposer.h>
#include "CompositionEngineV2Impl.h"
#include "Debug.h"
#include "disputils.h"
#include "hardware/sunxi_display2.h"
#include "HomletDeviceFactory.h"
#include "VendorServiceAdapter.h"

namespace sunxi {

HomletDeviceFactory::HomletDeviceFactory()
  : mConnectedDisplays(),
    mListener(nullptr)
{
    VendorServiceAdapter::getService()->setVendorInterface(this);
}

HomletDeviceFactory::~HomletDeviceFactory()
{
    VendorServiceAdapter::getService()->setVendorInterface(nullptr);
    mConnectedDisplays.clear();
    mHardwareDevices.clear();
    mListener = nullptr;
}

void HomletDeviceFactory::scanConnectedDevice()
{
    std::unique_lock<std::mutex> lock(mDeviceMutex);
    if (mConnectedDisplays.empty()) {
        DLOGI("wait for device connect");
        mCondition.wait(lock);
    }
    notifyDeviceChanged();
}

void HomletDeviceFactory::notifyDeviceChanged()
{
    if (!mListener) {
        DLOGE("DeviceChangedListener not register yet!");
        return;
    }
    mListener->onDeviceChanged(mConnectedDisplays);
}

std::shared_ptr<DeviceBase> HomletDeviceFactory::findDevice(int display)
{
    std::shared_ptr<DeviceBase> device = nullptr;
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == display) {
            device = item.device;
        }
    }
    return device;
}

void HomletDeviceFactory::registerDeviceChangedListener(DeviceChangedListener* listener)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    mListener = listener;
}

std::shared_ptr<DeviceBase> HomletDeviceFactory::getPrimaryDevice()
{
    std::unique_lock<std::mutex> lock(mDeviceMutex);
    if (mConnectedDisplays.empty()) {
        DLOGI("wait for device connect");
        mCondition.wait(lock);
    }
    return findDevice(HWC_DISPLAY_PRIMARY);
}

void HomletDeviceFactory::sortByDisplayId(std::vector<DeviceInfo *>& devices)
{
    std::sort(devices.begin(), devices.end(),
            [](DeviceInfo* a, DeviceInfo* b){ return a->logicalId < b->logicalId; });
}

int HomletDeviceFactory::updateConnectedDevices(std::vector<DeviceInfo *>& devices)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    const DeviceBase::Config config = {
        .name = std::string("default-config"),
        .configId = 0,
        .width = 1280,
        .height = 720,
        .dpix = 213,
        .dpiy = 213,
        .refreshRate = 60,
    };

    mConnectedDisplays.clear();
    sortByDisplayId(devices);

    for (const auto devinfo : devices) {
        if (devinfo == nullptr) {
            DLOGE("DeviceInfo should not be nullptr!");
            continue;
        }

        int hardwareIndex = devinfo->hardwareIndex;
        std::shared_ptr<Disp2Device> device = nullptr;
        if (devinfo->connected) {
            if (mHardwareDevices.count(hardwareIndex)) {
                device = mHardwareDevices[hardwareIndex];
                device->setOutput(devinfo->xres, devinfo->yres, devinfo->refreshRate);
            } else {
                std::string name = std::string(outputType2Name(devinfo->type)) + std::to_string(hardwareIndex);
                std::shared_ptr<CompositionEngineV2Impl> engine = CompositionEngineV2Impl::create(hardwareIndex);
                device = std::make_shared<Disp2Device>(name, engine, config);
                device->setOutput(devinfo->xres, devinfo->yres, devinfo->refreshRate);
                mHardwareDevices.emplace(hardwareIndex, device);
            }
        } else {
            if (mHardwareDevices.count(hardwareIndex)) {
                device = mHardwareDevices[hardwareIndex];
                mHardwareDevices.erase(hardwareIndex);
            }
        }
        ConnectedInfo c;
        c.displayId = devinfo->logicalId;
        c.connected = devinfo->connected;
        c.device = device;
        mConnectedDisplays.push_back(c);
        DLOGI("display[%d] connected=%d type=%d device=%p",
                devinfo->logicalId, devinfo->connected, devinfo->type, device.get());
    }

    mCondition.notify_all();
    notifyDeviceChanged();

    // we had finished, clear the disconnected devcie
    for (auto& display : mConnectedDisplays) {
        if (display.connected == 0) {
            display.device = nullptr;
        }
    }
    return 0;
}

int HomletDeviceFactory::blankDevice(int display, bool enabled)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->setActive(!enabled);
    }
    return 0;
}

int HomletDeviceFactory::setDeviceOutputInfo(
        int display, int xres, int yres, int refreshRate)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->setOutput(xres, yres, refreshRate);
    }
    return 0;
}

} // namespace sunxi
