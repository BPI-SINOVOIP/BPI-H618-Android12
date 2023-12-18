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

#ifndef ANDROID_HWC_HARDWARE_ABSTRACT_H_
#define ANDROID_HWC_HARDWARE_ABSTRACT_H_

#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "Device.h"

namespace sunxi {

class DeviceConnection;

using HotplugCallback = std::function<void(int32_t, bool)>;

struct DeviceMapping {
    int32_t LogicDisplayId;
    std::shared_ptr<DeviceBase> HardwareDevice;
};

class DeviceRouter {
public:
    static DeviceRouter* getInstance() {
        return mInstance;
    }

    std::shared_ptr<DeviceConnection> connect(int32_t id);
    void disconnect(int32_t id);
    // call on DeviceManager thread
    void onDeviceChanged(std::vector<DeviceMapping>& maps);

    // We don't want device exchanges during commits,
    // Use this api to inform the router not to switch devices.
    void switchEnable(int32_t id, bool enable);

    void registerHotplugCallback(HotplugCallback cb);
    void performHotplug(int32_t id, bool connected);

private:
    DeviceRouter()
        : mSwitchDisableMask(0),
          mConnections(), mHotplugCallback(nullptr) { }
   ~DeviceRouter() = default;
    DeviceRouter(const DeviceRouter&) = delete;
    DeviceRouter& operator=(const DeviceRouter&) = delete;

    std::shared_ptr<DeviceConnection> createDeviceConnection(int32_t id);

    // const DeviceRouter* DeviceRouter::mInstance = new DeviceRouter();
    static DeviceRouter* mInstance;

    mutable std::mutex mDeviceLock;
    mutable std::condition_variable mCondition;
    volatile int32_t mSwitchDisableMask;

    std::vector<DeviceMapping> mCurrentMapping;
    std::unordered_map< int32_t, std::shared_ptr<DeviceConnection> > mConnections;
    HotplugCallback mHotplugCallback;
};

// A wrapper class to DeviceBase,
// so that we can handle device switching inside this object.
class DeviceConnection: public DeviceBase::EventListener,
                        public std::enable_shared_from_this<DeviceConnection>
{
public:
    DeviceConnection(DeviceRouter& r, int32_t id, std::shared_ptr<DeviceBase>& dev);
    virtual ~DeviceConnection();

    // IDevice impl
    const DeviceBase::Config& getDefaultConfig();
    int32_t getRefreshRate();
    int32_t setPowerMode(int32_t mode);
    int32_t setVsyncEnabled(bool enabled);
    void validate(CompositionContext *ctx);
    void present(CompositionContext *ctx);

    // EventListener impl
    void onVsync(int64_t timestamp) override;
    void onDeviceChanged(std::shared_ptr<DeviceBase>& dev);

    using RefreshCallback = std::function<void()>;
    using VsyncCallback   = std::function<void(int64_t)>;
    void setRefreshCallback(RefreshCallback cb);
    void setVsyncCallback(VsyncCallback cb);

    void abandoned() { mAbandoned = true; }
    void setSwitchingEnable(bool enable);
    void dump(std::string& out);
    // just for test, do not use this api in hwcomposer module!!!
    uint64_t getDeviceUniqueId() const {
        return (uint64_t)mDevice.get();
    }

private:
    // update EventListener to underlying DeviceImpl
    void updateEventListenerLocked();

private:
    const int32_t mDisplayId;
    bool mAbandoned;
    DeviceRouter& mDeviceRouter;

    // Cache vsync state here, so that we can maintain it when switching device.
    bool mVsyncEnabled;
    VsyncCallback mVsyncCallback;
    RefreshCallback mRefreshCallback;

    // shared_mutex to protect mDevice, So that we can access to device simultaneously.
    mutable std::shared_mutex mSharedMutex;
    std::shared_ptr<DeviceBase> mDevice;
};

} // namespace sunxi

#endif

