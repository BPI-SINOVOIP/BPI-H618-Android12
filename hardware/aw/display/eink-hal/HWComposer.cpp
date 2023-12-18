/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <errno.h>
#include <functional>

#include "Debug.h"
#include "Display.h"
#include "Layer.h"
#include "HWComposer.h"
#include "DeviceManager.h"
#include "DevicePowerManager.h"

namespace sunxi {

int HWComposer::open(const struct hw_module_t *module,
                     const char *name, struct hw_device_t **device) {
    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        DLOGE("Invalid module name: %s", name);
        return -EINVAL;
    }

    std::unique_ptr<HWComposer> hwc(new HWComposer());
    if (!hwc) {
        DLOGE("Failed to allocate HWComposer");
        return -ENOMEM;
    }

    hwc->common.module = const_cast<hw_module_t *>(module);
    *device = &hwc->common;
    hwc.release();
    DLOGI("Loading: sunxi hwcomposer module");
    return 0;
}

extern std::unique_ptr<IDeviceFactory> createDeviceFactory();

int HWComposer::initializeDevices()
{
    {
        // make sure all displays are close
        std::lock_guard<std::mutex> lock(mDisplayMutex);
        mDisplays.clear();
        mValidDisplayHandles.clear();
    }

    HotplugCallback cb = [this](int32_t display, bool connected) {
        this->onHotplugReceived(display, connected);
    };

    mDeviceManager = std::make_shared<DeviceManager>();
    mDeviceManager->registerHotplugCallback(cb);
    mDeviceManager->startEventThread();

    mDevicePool = createDeviceFactory();
    mDevicePool->registerDeviceChangedListener(mDeviceManager.get());
    mDevicePool->scanConnectedDevice();

    mPowerManager = createDevicePowerManager();
    return 0;
}

void HWComposer::terminateDevices()
{
    mDevicePool = nullptr;
    mDeviceManager->stopEventThread();
    mDeviceManager = nullptr;

    // force driver into idle state
    mPowerManager->setRuntimeStateIdle();
    mPowerManager = nullptr;

    // blank all displays
    for (auto& entry : mDisplays) {
        std::shared_ptr<Display> display = entry.second;
        display->setPowerMode(HWC2_POWER_MODE_OFF);
    }

    // DO NOT clear mDisplays!
    // SurfaceFlinger will perform a final presentDisplay afterwards.
    // mDisplays.clear();
}

HWC2::Error HWComposer::setPowerMode(hwc2_display_t id, int32_t mode)
{
    HWC2::Error err = HWC2::Error::Unsupported;
    std::shared_ptr<Display> display = getDisplay(id);

    if (display) {
        err = display->setPowerMode(mode);

        // only response for primary display power request
        if (err == HWC2::Error::None && id == HWC_DISPLAY_PRIMARY)
            mPowerManager->setRuntimeState(mode);
    } else {
        err = HWC2::Error::BadDisplay;
    }
    return err;
}

void HWComposer::onHotplugReceived(int32_t handle, bool connected)
{
    hwc2_display_t id = static_cast<hwc2_display_t>(handle);
    DLOGI("display(%lld) connect state[%d]", id, connected);

    if (connected) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);
        DLOGI("display size: %d display.0 %d display.1 %d display.%lld %d",
                mDisplays.size(), mDisplays.count(0), mDisplays.count(1),
                id, mDisplays.count(id));
        if (mDisplays.count(id) == 1) {
            DLOGW("display(%lld) is already connected", id);
            return;
        }
        std::shared_ptr<Display> display = std::make_shared<Display>(id, mDeviceManager);
        display->setEventHandler(this);
        mDisplays.emplace(id, std::move(display));
        mValidDisplayHandles.insert(handle);
    } else {
        std::lock_guard<std::mutex> lock(mDisplayMutex);
        if (!mDisplays.count(id)) {
            DLOGW("display(%lld) is already disconnect", id);
            return;
        }
        mDisplays.erase(id);
    }

    {
        std::lock_guard<std::mutex> lock(mCallbackMutex);
        if (!mCallbacks.count(HWC2::Callback::Hotplug)) {
            int state = connected ? HWC2_CONNECTION_CONNECTED : HWC2_CONNECTION_DISCONNECTED;
            auto event = std::make_pair(id, state);
            mPendingHotplugs.push_back(event);
            mCondition.notify_all();
            return;
        }
    }

    const std::unique_ptr<CallbackInfo>& info = mCallbacks[HWC2::Callback::Hotplug];
    auto hotplug = reinterpret_cast<HWC2_PFN_HOTPLUG>(info->pointer);
    hotplug(info->data, id,
            connected ? HWC2_CONNECTION_CONNECTED : HWC2_CONNECTION_DISCONNECTED);
}

void HWComposer::waitForHotplugIssued(std::unique_lock<std::mutex>& lock)
{
    DLOGI("Wait for display connect");
    mCondition.wait(lock);
}

void HWComposer::onVSyncReceived(hwc2_display_t id, int64_t timestamp)
{
    if (mCallbacks.count(HWC2::Callback::Vsync) == 0)
        return;
    const std::unique_ptr<CallbackInfo>& info = mCallbacks[HWC2::Callback::Vsync];
    auto vsync = reinterpret_cast<HWC2_PFN_VSYNC>(info->pointer);
    vsync(info->data, id, timestamp);
}

void HWComposer::onRefreshReceived(hwc2_display_t id)
{
    if (mCallbacks.count(HWC2::Callback::Refresh) == 0)
        return;
    const std::unique_ptr<CallbackInfo>& info = mCallbacks[HWC2::Callback::Refresh];
    auto refresh = reinterpret_cast<HWC2_PFN_REFRESH>(info->pointer);
    refresh(info->data, id);
}

hwc2_function_pointer_t HWComposer::getFunctionHook(
        struct hwc2_device * /*device*/, int32_t descriptor)
{
    auto function = static_cast<HWC2::FunctionDescriptor>(descriptor);
    switch (function) {
        // Device functions
      case HWC2::FunctionDescriptor::CreateVirtualDisplay:
          return asFunctionPointer<HWC2_PFN_CREATE_VIRTUAL_DISPLAY>(
                  DeviceHook<int32_t, decltype(&HWComposer::createVirtualDisplay),
                  &HWComposer::createVirtualDisplay,
                  uint32_t, uint32_t, int32_t*, hwc2_display_t*>);
      case HWC2::FunctionDescriptor::DestroyVirtualDisplay:
          return asFunctionPointer<HWC2_PFN_DESTROY_VIRTUAL_DISPLAY>(
                  DeviceHook<int32_t, decltype(&HWComposer::destroyVirtualDisplay),
                  &HWComposer::destroyVirtualDisplay, hwc2_display_t>);
      case HWC2::FunctionDescriptor::Dump:
          return asFunctionPointer<HWC2_PFN_DUMP>(
                  DeviceHook<void, decltype(&HWComposer::dump),
                  &HWComposer::dump, uint32_t*, char*>);
      case HWC2::FunctionDescriptor::GetMaxVirtualDisplayCount:
          return asFunctionPointer<HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT>(
                  DeviceHook<uint32_t, decltype(&HWComposer::getMaxVirtualDisplayCount),
                  &HWComposer::getMaxVirtualDisplayCount>);
      case HWC2::FunctionDescriptor::RegisterCallback:
          return asFunctionPointer<HWC2_PFN_REGISTER_CALLBACK>(
                  DeviceHook<int32_t, decltype(&HWComposer::registerCallback),
                  &HWComposer::registerCallback,
                  int32_t, hwc2_callback_data_t, hwc2_function_pointer_t>);

      // Display functions
      case HWC2::FunctionDescriptor::AcceptDisplayChanges:
          return asFunctionPointer<HWC2_PFN_ACCEPT_DISPLAY_CHANGES>(
                  DisplayHook<decltype(&Display::acceptDisplayChanges),
                  &Display::acceptDisplayChanges>);
      case HWC2::FunctionDescriptor::CreateLayer:
          return asFunctionPointer<HWC2_PFN_CREATE_LAYER>(
                  DisplayHook<decltype(&Display::createLayer),
                  &Display::createLayer, hwc2_layer_t*>);
      case HWC2::FunctionDescriptor::DestroyLayer:
          return asFunctionPointer<HWC2_PFN_DESTROY_LAYER>(
                  DisplayHook<decltype(&Display::destroyLayer),
                  &Display::destroyLayer, hwc2_layer_t>);
      case HWC2::FunctionDescriptor::GetActiveConfig:
          return asFunctionPointer<HWC2_PFN_GET_ACTIVE_CONFIG>(
                  DisplayHook<decltype(&Display::getActiveConfig),
                  &Display::getActiveConfig, hwc2_config_t*>);
      case HWC2::FunctionDescriptor::GetChangedCompositionTypes:
          return asFunctionPointer<HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES>(
                  DisplayHook<decltype(&Display::getChangedCompositionTypes),
                  &Display::getChangedCompositionTypes,
                  uint32_t *, hwc2_layer_t *, int32_t *>);
      case HWC2::FunctionDescriptor::GetClientTargetSupport:
          return asFunctionPointer<HWC2_PFN_GET_CLIENT_TARGET_SUPPORT>(
                  DisplayHook<decltype(&Display::getClientTargetSupport),
                  &Display::getClientTargetSupport,
                  uint32_t, uint32_t, int32_t, int32_t>);
      case HWC2::FunctionDescriptor::GetColorModes:
          return asFunctionPointer<HWC2_PFN_GET_COLOR_MODES>(
                  DisplayHook<decltype(&Display::getColorModes),
                  &Display::getColorModes, uint32_t *, int32_t *>);
      case HWC2::FunctionDescriptor::GetDisplayAttribute:
          return asFunctionPointer<HWC2_PFN_GET_DISPLAY_ATTRIBUTE>(
                  DisplayHook<decltype(&Display::getDisplayAttribute),
                  &Display::getDisplayAttribute,
                  hwc2_config_t, int32_t, int32_t *>);
      case HWC2::FunctionDescriptor::GetDisplayConfigs:
          return asFunctionPointer<HWC2_PFN_GET_DISPLAY_CONFIGS>(
                  DisplayHook<decltype(&Display::getDisplayConfigs),
                  &Display::getDisplayConfigs, uint32_t*, hwc2_config_t*>);
      case HWC2::FunctionDescriptor::GetDisplayName:
          return asFunctionPointer<HWC2_PFN_GET_DISPLAY_NAME>(
                  DisplayHook<decltype(&Display::getDisplayName),
                  &Display::getDisplayName, uint32_t *, char *>);
      case HWC2::FunctionDescriptor::GetDisplayRequests:
          return asFunctionPointer<HWC2_PFN_GET_DISPLAY_REQUESTS>(
                  DisplayHook<decltype(&Display::getDisplayRequests),
                  &Display::getDisplayRequests,
                  int32_t*, uint32_t*, hwc2_layer_t*, int32_t*>);
      case HWC2::FunctionDescriptor::GetDisplayType:
          return asFunctionPointer<HWC2_PFN_GET_DISPLAY_TYPE>(
                  DisplayHook<decltype(&Display::getDisplayType),
                  &Display::getDisplayType, int32_t *>);
      case HWC2::FunctionDescriptor::GetDozeSupport:
          return asFunctionPointer<HWC2_PFN_GET_DOZE_SUPPORT>(
                  DisplayHook<decltype(&Display::getDozeSupport),
                  &Display::getDozeSupport, int32_t *>);
      case HWC2::FunctionDescriptor::GetHdrCapabilities:
          return asFunctionPointer<HWC2_PFN_GET_HDR_CAPABILITIES>(
                  DisplayHook<decltype(&Display::getHdrCapabilities),
                  &Display::getHdrCapabilities,
                  uint32_t*, int32_t*, float*, float*, float*>);
      case HWC2::FunctionDescriptor::GetReleaseFences:
          return asFunctionPointer<HWC2_PFN_GET_RELEASE_FENCES>(
                  DisplayHook<decltype(&Display::getReleaseFences),
                  &Display::getReleaseFences,
                  uint32_t*, hwc2_layer_t*, int32_t*>);
      case HWC2::FunctionDescriptor::PresentDisplay:
          return asFunctionPointer<HWC2_PFN_PRESENT_DISPLAY>(
                  DisplayHook<decltype(&Display::presentDisplay),
                  &Display::presentDisplay, int32_t*>);
      case HWC2::FunctionDescriptor::SetActiveConfig:
          return asFunctionPointer<HWC2_PFN_SET_ACTIVE_CONFIG>(
                  DisplayHook<decltype(&Display::setActiveConfig),
                  &Display::setActiveConfig, hwc2_config_t>);
      case HWC2::FunctionDescriptor::SetClientTarget:
          return asFunctionPointer<HWC2_PFN_SET_CLIENT_TARGET>(
                  DisplayHook<decltype(&Display::setClientTarget),
                  &Display::setClientTarget,
                  buffer_handle_t, int32_t, int32_t, hwc_region_t>);
      case HWC2::FunctionDescriptor::SetColorMode:
          return asFunctionPointer<HWC2_PFN_SET_COLOR_MODE>(
                  DisplayHook<decltype(&Display::setColorMode),
                  &Display::setColorMode, int32_t>);
      case HWC2::FunctionDescriptor::SetColorTransform:
          return asFunctionPointer<HWC2_PFN_SET_COLOR_TRANSFORM>(
                  DisplayHook<decltype(&Display::setColorTransform),
                  &Display::setColorTransform, const float*, int32_t>);
      case HWC2::FunctionDescriptor::SetOutputBuffer:
          return asFunctionPointer<HWC2_PFN_SET_OUTPUT_BUFFER>(
                  DisplayHook<decltype(&Display::setOutputBuffer),
                  &Display::setOutputBuffer, buffer_handle_t, int32_t>);
      case HWC2::FunctionDescriptor::SetPowerMode:
          return asFunctionPointer<HWC2_PFN_SET_POWER_MODE>(
                  DeviceHook<int32_t, decltype(&HWComposer::setPowerMode),
                  &HWComposer::setPowerMode, hwc2_display_t, int32_t>);
      case HWC2::FunctionDescriptor::SetVsyncEnabled:
          return asFunctionPointer<HWC2_PFN_SET_VSYNC_ENABLED>(
                  DisplayHook<decltype(&Display::setVsyncEnabled),
                  &Display::setVsyncEnabled, int32_t>);
      case HWC2::FunctionDescriptor::ValidateDisplay:
          return asFunctionPointer<HWC2_PFN_VALIDATE_DISPLAY>(
                  DisplayHook<decltype(&Display::validateDisplay),
                  &Display::validateDisplay, uint32_t*, uint32_t*>);
      case HWC2::FunctionDescriptor::GetRenderIntents:
          return asFunctionPointer<HWC2_PFN_GET_RENDER_INTENTS>(
                  DisplayHook<decltype(&Display::getRenderIntents),
                  &Display::getRenderIntents, int32_t, uint32_t*, int32_t*>);
      case HWC2::FunctionDescriptor::SetColorModeWithRenderIntent:
          return asFunctionPointer<HWC2_PFN_SET_COLOR_MODE_WITH_RENDER_INTENT>(
                  DisplayHook<decltype(&Display::setColorModeWithRenderIntent),
                  &Display::setColorModeWithRenderIntent, int32_t, int32_t>);

          // Layer functions
      case HWC2::FunctionDescriptor::SetCursorPosition:
          return asFunctionPointer<HWC2_PFN_SET_CURSOR_POSITION>(
                  LayerHook<decltype(&Layer::setCursorPosition),
                  &Layer::setCursorPosition, int32_t, int32_t>);
      case HWC2::FunctionDescriptor::SetLayerBlendMode:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_BLEND_MODE>(
                  LayerHook<decltype(&Layer::setLayerBlendMode),
                  &Layer::setLayerBlendMode, int32_t>);
      case HWC2::FunctionDescriptor::SetLayerBuffer:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_BUFFER>(
                  LayerHook<decltype(&Layer::setLayerBuffer),
                  &Layer::setLayerBuffer, buffer_handle_t, int32_t>);
      case HWC2::FunctionDescriptor::SetLayerColor:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_COLOR>(
                  LayerHook<decltype(&Layer::setLayerColor),
                  &Layer::setLayerColor, hwc_color_t>);
      case HWC2::FunctionDescriptor::SetLayerCompositionType:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_COMPOSITION_TYPE>(
                  LayerHook<decltype(&Layer::setLayerCompositionType),
                  &Layer::setLayerCompositionType, int32_t>);
      case HWC2::FunctionDescriptor::SetLayerDataspace:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_DATASPACE>(
                  LayerHook<decltype(&Layer::setLayerDataspace),
                  &Layer::setLayerDataspace, int32_t>);
      case HWC2::FunctionDescriptor::SetLayerDisplayFrame:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_DISPLAY_FRAME>(
                  LayerHook<decltype(&Layer::setLayerDisplayFrame),
                  &Layer::setLayerDisplayFrame, hwc_rect_t>);
      case HWC2::FunctionDescriptor::SetLayerPlaneAlpha:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_PLANE_ALPHA>(
                  LayerHook<decltype(&Layer::setLayerPlaneAlpha),
                  &Layer::setLayerPlaneAlpha, float>);
      case HWC2::FunctionDescriptor::SetLayerSidebandStream:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_SIDEBAND_STREAM>(
                  LayerHook<decltype(&Layer::setLayerSidebandStream),
                  &Layer::setLayerSidebandStream,
                  const native_handle_t*>);
      case HWC2::FunctionDescriptor::SetLayerSourceCrop:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_SOURCE_CROP>(
                  LayerHook<decltype(&Layer::setLayerSourceCrop),
                  &Layer::setLayerSourceCrop, hwc_frect_t>);
      case HWC2::FunctionDescriptor::SetLayerSurfaceDamage:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_SURFACE_DAMAGE>(
                  LayerHook<decltype(&Layer::setLayerSurfaceDamage),
                  &Layer::setLayerSurfaceDamage, hwc_region_t>);
      case HWC2::FunctionDescriptor::SetLayerTransform:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_TRANSFORM>(
                  LayerHook<decltype(&Layer::setLayerTransform),
                  &Layer::setLayerTransform, int32_t>);
      case HWC2::FunctionDescriptor::SetLayerVisibleRegion:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_VISIBLE_REGION>(
                  LayerHook<decltype(&Layer::setLayerVisibleRegion),
                  &Layer::setLayerVisibleRegion, hwc_region_t>);
      case HWC2::FunctionDescriptor::SetLayerZOrder:
          return asFunctionPointer<HWC2_PFN_SET_LAYER_Z_ORDER>(
                  LayerHook<decltype(&Layer::setLayerZOrder),
                  &Layer::setLayerZOrder, uint32_t>);
      case HWC2::FunctionDescriptor::Invalid:
      default:
          return nullptr;
    }
}

HWC2::Error HWComposer::createVirtualDisplay(
    uint32_t /*width*/, uint32_t /*height*/, int32_t * /*format*/,
    hwc2_display_t * /*display*/)
{
    // TODO: Implement virtual display
    DLOGW("Unsupported functions: %s", __func__);
    return HWC2::Error::Unsupported;
}

std::shared_ptr<Display> HWComposer::getDisplay(hwc2_display_t id)
{
    std::lock_guard<std::mutex> lock(mDisplayMutex);
    if (mDisplays.count(id)) {
        return mDisplays[id];
    }
    return nullptr;
}

HWC2::Error HWComposer::destroyVirtualDisplay(hwc2_display_t /*display*/)
{
    // TODO: Implement virtual display
    DLOGW("Unsupported functions: %s", __func__);
    return HWC2::Error::Unsupported;
}

void HWComposer::dump(uint32_t *size, char *buffer)
{
    if (buffer == nullptr) {
        *size = 8192;
        return;
    }

    std::string s("\nSUNXI HWC:\n");
    for (int id = HWC_DISPLAY_PRIMARY; id <= HWC_DISPLAY_EXTERNAL; id++) {
        if (mDisplays.count(id)) {
            std::string result;
            mDisplays[id]->dump(result);
            s.append(result);
        }
    }
    s.append("\n");

    *size = s.size() > 8192 ? 8192 : s.size();
    s.copy(buffer, *size);
}

uint32_t HWComposer::getMaxVirtualDisplayCount()
{
    // TODO: Implement virtual display
    return 0;
}

HWC2::Error HWComposer::registerCallback(int32_t descriptor,
        hwc2_callback_data_t data, hwc2_function_pointer_t function)
{
    HWC2::Callback callback = static_cast<HWC2::Callback>(descriptor);

    std::unique_lock<std::mutex> lock(mCallbackMutex);
    if (!function) {
        if (callback == HWC2::Callback::Hotplug) {
            // SurfaceFlinger will register nullptr callback to hwcomposer module
            // at the end of its life cyle, So do terminate here!
            terminateDevices();
            DLOGI("Devices terminate");
        }
        mCallbacks.erase(callback);
        return HWC2::Error::None;
    }

    if (callback == HWC2::Callback::Hotplug) {
        // After hotplug callback installed, SurfaceFlinger is ready to
        // handle primary display, So initialize devices at this point.
        int error = initializeDevices();
        if (error != 0) {
            DLOGE("Failed to initialize HWComposer: %s", strerror(error));
        }
        // Wait for display connected, so that we can perform
        // primary display's hotplug event at surfaceflinger's main thread.
        waitForHotplugIssued(lock);
        auto hotplug = reinterpret_cast<HWC2_PFN_HOTPLUG>(function);
        for (auto& item : mPendingHotplugs) {
            hotplug(data, item.first, item.second);
        }
        mPendingHotplugs.clear();
    }
    mCallbacks.emplace(callback, std::make_unique<CallbackInfo>(data, function));
    return HWC2::Error::None;
}

void HWComposer::getCapabilitiesHook(
        hwc2_device_t *dev, uint32_t *count, int32_t *capabilities)
{
	//TODO:eink capabilities
    *count = 0;
}

int HWComposer::closeHook(hw_device_t *dev)
{
    DLOGI("HWComposer module close");
    return 0;
}

HWComposer::HWComposer()
{
    common.tag      = HARDWARE_DEVICE_TAG;
    common.version  = HWC_DEVICE_API_VERSION_2_0;
    common.close    = closeHook;
    getCapabilities = getCapabilitiesHook;
    getFunction     = getFunctionHook;
}

HWComposer::~HWComposer() = default;

} // namespace sunxi

static struct hw_module_methods_t hwc2_module_methods = {
    .open = sunxi::HWComposer::open,
};

hw_module_t HAL_MODULE_INFO_SYM = {
    .tag      = HARDWARE_MODULE_TAG,
    .module_api_version = HARDWARE_MODULE_API_VERSION(2, 0),
    .id       = HWC_HARDWARE_MODULE_ID,
    .name     = "sunxi hwcomposer module",
    .author   = "sunxi",
    .methods  = &hwc2_module_methods,
    .dso      = nullptr,
    .reserved = {0},
};

