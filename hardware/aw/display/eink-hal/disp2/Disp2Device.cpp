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

#include <android-base/stringprintf.h>
#include "Compositor.h"
#include "CompositionEngineV2Impl.h"
#include "disputils.h"
#include "Disp2Device.h"
#include "Debug.h"
#include "FenceMonitor.h"

using android::base::StringPrintf;

namespace sunxi {

Disp2Device::OutputState::OutputState()
{
    xres = yres = 0;
    refreshRate = 0;
    output3d = 0;
}

Disp2Device::Disp2Device(std::string name,
        std::shared_ptr<CompositionEngineV2Impl>& engine, const Config& config)
  : mDeviceName(name),
    mPowerMode(HWC2_POWER_MODE_OFF),
    mDefaultConfig(config),
    mHardwareEngine(engine),
    mDisplayEventListener(),
    mVsyncEnabled(false)
{
    mCompositor = std::make_unique<Compositor>();
    mCompositor->setCompositionEngine(std::static_pointer_cast<CompositionEngine>(mHardwareEngine));
    mCompositor->setTraceNameSuffix(mDeviceName.c_str());
    mCompositor->setRefreshCallback([this](){ this->requestRefreshLocked(); });
    VsyncThread::getInstance()->addEventHandler(mHardwareEngine->hardwareId(), this);
}

Disp2Device::~Disp2Device()
{
    VsyncThread::getInstance()->removeEventHandler(mHardwareEngine->hardwareId());
}

const char* Disp2Device::name()
{
    return mDeviceName.c_str();
}

const DeviceBase::Config& Disp2Device::getDefaultConfig()
{
    return mDefaultConfig;
}

int Disp2Device::getVsyncPeriodInNs()
{
    std::lock_guard<std::mutex> lock(mMutex);
    return 1000 * 1000 * 1000 / mCurrentState.refreshRate;
}

int Disp2Device::getRefreshRate()
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mCurrentState.refreshRate;
}

void Disp2Device::latchOutputState()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mActivedState = mCurrentState;
    mCompositor->setScreenSize(mActivedState.xres, mActivedState.yres);
}

void Disp2Device::prepare(CompositionContext* ctx)
{
    // latch output state config and sync to compositor
    latchOutputState();
    mCompositor->setFramebufferSize(ctx->Viewport.width(), ctx->Viewport.height());
    mCompositor->prepare(ctx);
}

void Disp2Device::commit(CompositionContext* ctx)
{
	std::lock_guard<std::mutex> lock(mMutex);
    mCompositor->commit(ctx);

    if (CC_UNLIKELY(atrace_is_tag_enabled(ATRACE_TAG_GRAPHICS))) {
        FenceDebugger::queueFence(mHardwareEngine->hardwareId(), ctx->RetireFence.dup());
    }
}

int32_t Disp2Device::setPowerMode(int32_t mode)
{
    std::lock_guard<std::mutex> lock(mMutex);
    hwc2_power_mode_t pm = static_cast<hwc2_power_mode_t>(mode);

    switch (pm) {
        case HWC2_POWER_MODE_OFF:
            mCompositor->blank(true);
            setVsyncEnabledInternal(false);
            break;
        case HWC2_POWER_MODE_ON:
            mCompositor->blank(false);
            break;
        case HWC2_POWER_MODE_DOZE:
        case HWC2_POWER_MODE_DOZE_SUSPEND:
            return HWC2_ERROR_UNSUPPORTED;
    };

    mPowerMode = pm;
    DLOGD("device(%s) power mode: %s", mDeviceName.c_str(), HWC2::to_string(pm).c_str());
    return HWC2_ERROR_NONE;
}

void Disp2Device::setEventListener(std::weak_ptr<EventListener> listener)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mDisplayEventListener = listener;
}

void Disp2Device::setVsyncEnabled(bool enabled)
{
    if (enabled && mPowerMode != HWC2_POWER_MODE_ON) {
        DLOGW("enable vsync while poweroff!");
        return;
    }
    setVsyncEnabledInternal(enabled);
    DLOGD_IF(kTagVSync, "%s vsync enable %d", mDeviceName.c_str(), enabled);
}

void Disp2Device::skipFrameEnable(bool enabled)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mCompositor->skipFrameEnable(enabled);
}

void Disp2Device::setColorTransform(const float *matrix, int32_t hint)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mCompositor->setColorTransform(matrix, hint);
}

void Disp2Device::setVsyncEnabledInternal(bool enabled)
{
    mVsyncEnabled.store(enabled);
    ::vsyncCtrl(mHardwareEngine->hardwareId(), enabled ? 1: 0);
}

void Disp2Device::setOutput(int xres, int yres, int refreshrate)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mCurrentState.xres = xres;
    mCurrentState.yres = yres;
    mCurrentState.refreshRate = refreshrate;
    requestRefreshLocked();
}

void Disp2Device::setActive(bool active)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mCompositor->blank(!active);
}

int Disp2Device::freezeOuput(bool freeze)
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mCompositor->freeze(freeze);
}

void Disp2Device::setRotatorManager(const std::shared_ptr<RotatorManager>& rotator)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mCompositor->setRotatorManager(rotator);
}

void Disp2Device::onVsync(int disp, int64_t timestamp)
{
    if (disp != mHardwareEngine->hardwareId()) {
        return;
    }

    DLOGI_IF(kTagVSync, "%s(hardware id %d) timestamp %lld",
            mDeviceName.c_str(), disp, timestamp);

    EventListener *handler = nullptr;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        handler = mDisplayEventListener.lock().get();
    }
    if (mVsyncEnabled.load() && handler) {
        DLOGI_IF(kTagVSync, "%s(hardware id %d) timestamp %lld",
                mDeviceName.c_str(), disp, timestamp);
        handler->onVsync(timestamp);
    }
}

void Disp2Device::requestRefreshLocked()
{
    EventListener *handler = mDisplayEventListener.lock().get();
    if (handler) {
        handler->onRefresh();
    }
}

void Disp2Device::dump(std::string& out)
{
    std::string str;
    mCompositor->dump(str);
    out += StringPrintf("\tDevice name : %s\n", mDeviceName.c_str());
    out += StringPrintf("\tPower mode  : %d\n", mPowerMode);
    out += StringPrintf("\tVsync state : %s\n", mVsyncEnabled.load() ? "enable" : "disable");
    out += StringPrintf("\tOutput state: %dx%d %d Hz\n",
            mCurrentState.xres, mCurrentState.yres, mCurrentState.refreshRate);
    out += std::string("   Compositor:\n") + str;
}

int Disp2Device::setEinkMode(int mode)
{
	return mCompositor->setEinkMode(mode);
}

int Disp2Device::setEinkBufferFd(int fd)
{
	return mCompositor->setEinkBufferFd(fd);
}

int Disp2Device::updateEinkRegion(int left, int top, int right, int bottom)
{
	return mCompositor->updateEinkRegion(left, top, right, bottom);
}

int Disp2Device::setEinkUpdateArea(int left, int top, int right, int bottom)
{
	return mCompositor->setEinkUpdateArea(left, top, right, bottom);
}

int Disp2Device::forceEinkRefresh(bool rightNow)
{
    return mCompositor->forceEinkRefresh(rightNow);
}
} // namespace sunxi
