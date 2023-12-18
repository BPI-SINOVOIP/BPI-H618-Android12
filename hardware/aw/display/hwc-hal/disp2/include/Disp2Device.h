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

#ifndef SUNXI_DISP2_DEVICE_H
#define SUNXI_DISP2_DEVICE_H

#include <string>
#include <memory>
#include <hardware/hwcomposer2.h>

#include "Device.h"
#include "VsyncThread.h"

namespace sunxi {

class Compositor;
class CompositionEngineV2Impl;
class RotatorManager;

class Disp2Device: public DeviceBase, VsyncThread::EventHandler {
public:
    Disp2Device(
            std::string name,
            std::shared_ptr<CompositionEngineV2Impl>& engine,
            const Config& config);
   ~Disp2Device();

    // DeviceBase impl
    const char* name() override;
    const Config& getDefaultConfig() override;
    int getRefreshRate() override;
    void prepare(CompositionContext* ctx) override;
    void commit (CompositionContext* ctx) override;
    int32_t setPowerMode(int32_t mode) override;
    void setEventListener(std::weak_ptr<EventListener> listener) override;
    void setVsyncEnabled(bool enabled) override;
    void skipFrameEnable(bool enabled) override;
    void setColorTransform(const float *matrix, int32_t hint);
    void dump(std::string& out) override;

    // VsyncThread::EventListener impl
    void onVsync(int disp, int64_t timestamp) override;

    void setOutput(int xres, int yres, int refreshrate);
    void setActive(bool active);
    void setRotatorManager(const std::shared_ptr<RotatorManager>& rotator);
    void setForceClientCompositionOnFreezed(bool enable);
    int freezeOuput(bool freeze);

    // add for homlet platform
    void setMargin(int l, int r, int t, int b);
    void setDataSpace(int dataspace);
    void setDisplaydCb(void* cb);
    void set3DMode(int mode);

private:
    void latchOutputState();
    void setVsyncEnabledInternal(bool enabled);
    void requestRefreshLocked();

private:
    std::string mDeviceName;
    std::mutex mMutex;
    // mPowerMode from SurfaceFlinger,
    // video image will be displayed only when mPowerMode is
    // HWC2_POWER_MODE_ON and mActivedState.active is true.
    hwc2_power_mode_t mPowerMode;
    Config mDefaultConfig;

    std::shared_ptr<CompositionEngineV2Impl> mHardwareEngine;
    std::unique_ptr<Compositor> mCompositor;
    std::weak_ptr<EventListener> mDisplayEventListener;
    std::atomic<bool> mVsyncEnabled;

    struct OutputState {
        int xres;
        int yres;
        int refreshRate;
        // screen margin in percentage,
        // [l,r,t,b]
        int margin[4];
        int dataspace;
        bool output3d;
        int mode3d;
        OutputState();
    };
    void* mDisplaydCb;
    OutputState mCurrentState;
    OutputState mActivedState;
    OutputState mBackUpState;//back up raw state for restore state after 3d exited
};

} // namespace sunxi


#endif
