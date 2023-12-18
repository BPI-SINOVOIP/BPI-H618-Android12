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


#include <hardware/hwcomposer2.h>
#include "DeviceConnection.h"

namespace sunxi {

DeviceConnection::DeviceConnection()
    : mDeviceHub(nullptr) { }

DeviceConnection::DeviceConnection(const DeviceConnection& rhs)
    : mDeviceHub(rhs.mDeviceHub) { }

std::shared_ptr<DeviceBase> DeviceConnection::operator->()
{
    return mDeviceHub->getDevice();
}

bool DeviceConnection::populateDevice()
{
    return mDeviceHub->populateDevice();
}

void DeviceConnection::setDeviceHub(std::shared_ptr<DeviceHub>& hub)
{
    mDeviceHub = hub;
}

void DeviceConnection::setDisplayEventCallback(DisplayEventCallback *callback)
{
    mDeviceHub->setDisplayEventCallback(callback);
}

void DeviceConnection::dump(std::string& out)
{
    mDeviceHub->dump(out);
}

//----------------------------------------------------------------------------//

DeviceHub::DeviceHub(int32_t id, std::shared_ptr<DeviceBase>& dev)
  : mDisplayId(id),
    mDevice(dev),
    mWaitForPopulate(false) { }

bool DeviceHub::populateDevice()
{
    std::lock_guard<std::mutex> lock(mPopulateMutex);
    if (!mWaitForPopulate)
        return false;

    std::unique_lock<std::shared_mutex> dev_lock(mDeviceMutex);
    // the previous device will be replaced by a new coming device,
    // turn off it's vsync.
    mDevice->setVsyncEnabled(false);
    mDevice->setEventListener(
            std::weak_ptr<EventListener>());

    // replace with a new device
    mDevice = mCurrentDevice;
    mDevice->setPowerMode(HWC2_POWER_MODE_ON);
    mDevice->setEventListener(
            std::weak_ptr<EventListener>(shared_from_this()));
    mDevice->setVsyncEnabled(mDisplayId == HWC_DISPLAY_PRIMARY ? true : false);

    mWaitForPopulate = false;
    mCondition.notify_all();
    return true;
}

void DeviceHub::onDeviceChanged(std::shared_ptr<DeviceBase>& dev)
{
    std::unique_lock<std::mutex> lock(mPopulateMutex);
    mWaitForPopulate = true;
    mCurrentDevice = dev;
    mEventCallback->requestRefresh();
    mCondition.wait(lock, [this]{ return mWaitForPopulate == false; });
    mCurrentDevice.reset();
}

std::shared_ptr<DeviceBase> DeviceHub::getDevice()
{
    std::shared_lock<std::shared_mutex> lock(mDeviceMutex);
    return mDevice;
}

void DeviceHub::setDisplayEventCallback(DisplayEventCallback *callback)
{
    mEventCallback = callback;
    mDevice->setEventListener(
            std::weak_ptr<EventListener>(shared_from_this()));
}

void DeviceHub::onVsync(int64_t timestamp)
{
    if (mEventCallback)
        mEventCallback->onVsync(timestamp);
}

void DeviceHub::onRefresh()
{
    if (mEventCallback)
        mEventCallback->requestRefresh();
}

void DeviceHub::dump(std::string& out)
{
    std::shared_lock<std::shared_mutex> lock(mDeviceMutex);
    mDevice->dump(out);
}

}
