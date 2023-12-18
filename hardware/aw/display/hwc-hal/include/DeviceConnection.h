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

#ifndef SUNXI_HWC_DEVICE_CONNECTION_H_
#define SUNXI_HWC_DEVICE_CONNECTION_H_

#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <condition_variable>
#include <hardware/hwcomposer2.h>
#include "Device.h"

namespace sunxi {

class DeviceHub;

class DisplayEventCallback {
public:
    virtual void onVsync(int64_t timestamp) = 0;
    virtual void requestRefresh() = 0;
    virtual ~DisplayEventCallback() = default;
};

class DeviceConnection {
public:
    DeviceConnection();
    DeviceConnection(const DeviceConnection& rhs);
   ~DeviceConnection() = default;

   std::shared_ptr<DeviceBase> operator->();

    // populate underlying device mapping and output config,
    // return true when underlying device property has changed,
    // otherwise return false.
    bool populateDevice();
    void setDeviceHub(std::shared_ptr<DeviceHub>& hub);
    void setDisplayEventCallback(DisplayEventCallback *callback);
    void dump(std::string& out);

private:
   std::shared_ptr<DeviceHub> mDeviceHub;
};

// A wrapper class to DeviceBase,
// so that we can handle device switching inside this object.
class DeviceHub: public DeviceBase::EventListener,
                 public std::enable_shared_from_this<DeviceHub>
{
public:
    DeviceHub(int32_t id, std::shared_ptr<DeviceBase>& dev);
   ~DeviceHub() = default;

    // EventListener impl, call from vsync thread and EventThread
    void onVsync(int64_t timestamp) override;
    void onRefresh() override;
    void onDeviceChanged(std::shared_ptr<DeviceBase>& dev);

    bool populateDevice();
    std::shared_ptr<DeviceBase> getDevice();
    void setDisplayEventCallback(DisplayEventCallback *callback);

    void dump(std::string& out);

private:
    bool primaryDisplay() const {
        return mDisplayId == HWC_DISPLAY_PRIMARY;
    }

    const int32_t mDisplayId;
    std::shared_ptr<DeviceBase> mDevice;
    DisplayEventCallback* mEventCallback;

    // shared_mutex to protect mDevice,
    // So that we can access to device simultaneously.
    mutable std::shared_mutex mDeviceMutex;

    mutable std::mutex mPopulateMutex;
    mutable std::condition_variable mCondition;
    bool mWaitForPopulate;
    std::shared_ptr<DeviceBase> mCurrentDevice;
};

} // namespace sunxi

#endif

