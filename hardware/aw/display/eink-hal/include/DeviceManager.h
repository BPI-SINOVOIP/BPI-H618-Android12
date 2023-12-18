/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef SUNXI_HWC_DEVICE_MANAGER_H_
#define SUNXI_HWC_DEVICE_MANAGER_H_

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <queue>
#include <unordered_map>
#include <vector>

#include "Device.h"
#include "DeviceConnection.h"

namespace sunxi {

class IDeviceFactory {
public:
    struct ConnectedInfo {
        int32_t displayId;
        int32_t connected;
        std::shared_ptr<DeviceBase> device;
    };
    class DeviceChangedListener {
    public:
        virtual void onDeviceChanged(std::vector<ConnectedInfo>& cinfo) = 0;
        virtual ~DeviceChangedListener() { }
    };

    virtual void scanConnectedDevice() = 0;
    virtual void registerDeviceChangedListener(DeviceChangedListener* listener) = 0;
    virtual std::shared_ptr<DeviceBase> getPrimaryDevice() = 0;
    virtual ~IDeviceFactory() = default;
};

using ConnectedInfo = IDeviceFactory::ConnectedInfo;
using HotplugCallback = std::function<void(int32_t, bool)>;

class DeviceManager: public IDeviceFactory::DeviceChangedListener {
public:

    DeviceManager();
   ~DeviceManager();

    void onDeviceChanged(std::vector<ConnectedInfo>& cinfo) override;
    std::shared_ptr<DeviceBase> getDeviceByDisplayId(int32_t id);
    DeviceConnection createDeviceConnection(int32_t id);
    void disconnect(int32_t id);

    void startEventThread();
    void stopEventThread();
    void registerHotplugCallback(HotplugCallback cb);

    class EventQueue;

private:
    void deviceChangedEventProcess(std::vector<ConnectedInfo>& cinfo);
    void populateDeviceHub(std::vector<ConnectedInfo>& cinfo);
    void threadLoop();

    std::unique_ptr<EventQueue> mEventQueue;

    std::atomic<bool> mRunning;
    std::thread mEventThread;
    HotplugCallback mHotplugCallback;

    // Connected logical displays
    std::mutex mDisplayLock;
    std::unordered_map<int32_t, std::shared_ptr<DeviceBase>> mLogicalDisplays;
    std::unordered_map<int32_t, std::shared_ptr<DeviceHub>> mDeviceHubs;
};

} // namespace sunxi

#endif // SUNXI_HWC_DEVICE_MANAGER_H_

