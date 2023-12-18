/*
 * Copyright 2021 The Android Open Source Project
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

#include "Device.h"

#include <android-base/properties.h>

#include "GuestComposer.h"
#include "HostComposer.h"

namespace android {
namespace {

template <typename PFN, typename T>
static hwc2_function_pointer_t asFP(T function) {
  static_assert(std::is_same<PFN, T>::value, "Incompatible function pointer");
  return reinterpret_cast<hwc2_function_pointer_t>(function);
}

static int CloseHook(hw_device_t* dev) {
  Device* device = Device::fromDevice(dev);
  delete device;
  return 0;
}

bool IsCuttlefish() {
  return android::base::GetProperty("ro.hardware.vulkan", "") == "pastel";
}

}  // namespace

Device::Device() {
  DEBUG_LOG("%s", __FUNCTION__);

  common.tag = HARDWARE_DEVICE_TAG;
  common.version = HWC_DEVICE_API_VERSION_2_0;
  common.close = CloseHook;
  hwc2_device_t::getCapabilities = getCapabilitiesHook;
  hwc2_device_t::getFunction = getFunctionHook;
}

HWC2::Error Device::init() {
  DEBUG_LOG("%s", __FUNCTION__);

  if (IsCuttlefish()) {
    mComposer = std::make_unique<GuestComposer>();
  } else {
    mComposer = std::make_unique<HostComposer>();
  }

  HWC2::Error error = mComposer->init(
      [this](bool connected, uint32_t id, uint32_t width, uint32_t height,
             uint32_t dpiX, uint32_t dpiY, uint32_t refreshRate) {
        handleHotplug(connected, id, width, height, dpiX, dpiY, refreshRate);
      });

  if (error != HWC2::Error::None) {
    ALOGE("%s failed to initialize Composer", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  return HWC2::Error::None;
}

Device::~Device() {
  DEBUG_LOG("%s", __FUNCTION__);

  HWC2::Error error = HWC2::Error::None;

  error = destroyDisplays();
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to destroy displays", __FUNCTION__);
  }
}

HWC2::Error Device::createDisplays() {
  DEBUG_LOG("%s", __FUNCTION__);

  std::unique_lock<std::mutex> lock(mStateMutex);

  if (!mComposer) {
    ALOGE("%s composer not initialized!", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  auto addDisplayLockedFn = [this](std::unique_ptr<Display> display) {
    auto displayId = display->getId();
    DEBUG_LOG("%s: adding display:%" PRIu64, __FUNCTION__, displayId);
    mDisplays.emplace(displayId, std::move(display));
    return HWC2::Error::None;
  };

  HWC2::Error error = mComposer->createDisplays(this, addDisplayLockedFn);
  if (error != HWC2::Error::None) {
    ALOGE("%s composer failed to create displays", __FUNCTION__);
    return error;
  }

  return HWC2::Error::None;
}

HWC2::Error Device::createDisplay(uint32_t displayId, uint32_t width,
                                  uint32_t height, uint32_t dpiX, uint32_t dpiY,
                                  uint32_t refreshRate) {
  if (!mComposer) {
    ALOGE("%s composer not initialized!", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  auto addDisplayLockedFn = [this](std::unique_ptr<Display> display) {
    auto displayId = display->getId();
    DEBUG_LOG("%s: adding display:%" PRIu64, __FUNCTION__, displayId);
    mDisplays.emplace(displayId, std::move(display));
    return HWC2::Error::None;
  };

  HWC2::Error error =
      mComposer->createDisplay(this, displayId, width, height, dpiX, dpiY,
                               refreshRate, addDisplayLockedFn);
  if (error != HWC2::Error::None) {
    ALOGE("%s composer failed to create displays", __FUNCTION__);
    return error;
  }

  return HWC2::Error::None;
}

HWC2::Error Device::destroyDisplays() {
  DEBUG_LOG("%s", __FUNCTION__);

  std::unique_lock<std::mutex> lock(mStateMutex);

  if (!mComposer) {
    ALOGE("%s composer not initialized!", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  for (auto& [displayId, displayPtr] : mDisplays) {
    HWC2::Error error = mComposer->onDisplayDestroy(displayPtr.get());
    if (error != HWC2::Error::None) {
      ALOGE("%s composer failed to destroy displays", __FUNCTION__);
      return error;
    }

    displayPtr.reset();
  }

  mDisplays.clear();

  return HWC2::Error::None;
}

void Device::getCapabilities(uint32_t* outCount, int32_t* outCapabilities) {
  DEBUG_LOG("%s", __FUNCTION__);

  if (outCapabilities == nullptr) {
    *outCount = mCapabilities.size();
    return;
  }

  auto capabilityIter = mCapabilities.cbegin();
  for (size_t i = 0; i < *outCount; ++i) {
    if (capabilityIter == mCapabilities.cend()) {
      return;
    }
    outCapabilities[i] = static_cast<int32_t>(*capabilityIter);
    ++capabilityIter;
  }
}

/*static*/
void Device::getCapabilitiesHook(hwc2_device_t* dev, uint32_t* outCount,
                                 int32_t* outCapabilities) {
  DEBUG_LOG("%s", __FUNCTION__);

  Device* device = Device::fromDevice(dev);
  device->getCapabilities(outCount, outCapabilities);
}

hwc2_function_pointer_t Device::getFunction(int32_t desc) {
  const auto func = static_cast<HWC2::FunctionDescriptor>(desc);
  const auto funcString = to_string(func);
  DEBUG_LOG("%s(%s)", __FUNCTION__, funcString.c_str());

  switch (func) {
    // Device functions.
    case HWC2::FunctionDescriptor::CreateVirtualDisplay:
      return asFP<HWC2_PFN_CREATE_VIRTUAL_DISPLAY>(
          DeviceHook<int32_t, decltype(&Device::createVirtualDisplay),
                     &Device::createVirtualDisplay, uint32_t, uint32_t,
                     int32_t*, hwc2_display_t*>);
    case HWC2::FunctionDescriptor::DestroyVirtualDisplay:
      return asFP<HWC2_PFN_DESTROY_VIRTUAL_DISPLAY>(
          DeviceHook<int32_t, decltype(&Device::destroyVirtualDisplay),
                     &Device::destroyVirtualDisplay, hwc2_display_t>);
    case HWC2::FunctionDescriptor::Dump:
      return asFP<HWC2_PFN_DUMP>(DeviceHook<void, decltype(&Device::dump),
                                            &Device::dump, uint32_t*, char*>);
    case HWC2::FunctionDescriptor::GetMaxVirtualDisplayCount:
      return asFP<HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT>(
          DeviceHook<uint32_t, decltype(&Device::getMaxVirtualDisplayCount),
                     &Device::getMaxVirtualDisplayCount>);
    case HWC2::FunctionDescriptor::RegisterCallback:
      return asFP<HWC2_PFN_REGISTER_CALLBACK>(
          DeviceHook<int32_t, decltype(&Device::registerCallback),
                     &Device::registerCallback, int32_t, hwc2_callback_data_t,
                     hwc2_function_pointer_t>);

    // Display functions
    case HWC2::FunctionDescriptor::AcceptDisplayChanges:
      return asFP<HWC2_PFN_ACCEPT_DISPLAY_CHANGES>(
          displayHook<decltype(&Display::acceptChanges),
                      &Display::acceptChanges>);
    case HWC2::FunctionDescriptor::CreateLayer:
      return asFP<HWC2_PFN_CREATE_LAYER>(
          displayHook<decltype(&Display::createLayer), &Display::createLayer,
                      hwc2_layer_t*>);
    case HWC2::FunctionDescriptor::DestroyLayer:
      return asFP<HWC2_PFN_DESTROY_LAYER>(
          displayHook<decltype(&Display::destroyLayer), &Display::destroyLayer,
                      hwc2_layer_t>);
    case HWC2::FunctionDescriptor::GetActiveConfig:
      return asFP<HWC2_PFN_GET_ACTIVE_CONFIG>(
          displayHook<decltype(&Display::getActiveConfig),
                      &Display::getActiveConfig, hwc2_config_t*>);
    case HWC2::FunctionDescriptor::GetChangedCompositionTypes:
      return asFP<HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES>(
          displayHook<decltype(&Display::getChangedCompositionTypes),
                      &Display::getChangedCompositionTypes, uint32_t*,
                      hwc2_layer_t*, int32_t*>);
    case HWC2::FunctionDescriptor::GetColorModes:
      return asFP<HWC2_PFN_GET_COLOR_MODES>(
          displayHook<decltype(&Display::getColorModes),
                      &Display::getColorModes, uint32_t*, int32_t*>);
    case HWC2::FunctionDescriptor::GetDisplayAttribute:
      return asFP<HWC2_PFN_GET_DISPLAY_ATTRIBUTE>(
          displayHook<decltype(&Display::getDisplayAttribute),
                      &Display::getDisplayAttribute, hwc2_config_t, int32_t,
                      int32_t*>);
    case HWC2::FunctionDescriptor::GetDisplayConfigs:
      return asFP<HWC2_PFN_GET_DISPLAY_CONFIGS>(
          displayHook<decltype(&Display::getConfigs), &Display::getConfigs,
                      uint32_t*, hwc2_config_t*>);
    case HWC2::FunctionDescriptor::GetDisplayName:
      return asFP<HWC2_PFN_GET_DISPLAY_NAME>(
          displayHook<decltype(&Display::getName), &Display::getName, uint32_t*,
                      char*>);
    case HWC2::FunctionDescriptor::GetDisplayRequests:
      return asFP<HWC2_PFN_GET_DISPLAY_REQUESTS>(
          displayHook<decltype(&Display::getRequests), &Display::getRequests,
                      int32_t*, uint32_t*, hwc2_layer_t*, int32_t*>);
    case HWC2::FunctionDescriptor::GetDisplayType:
      return asFP<HWC2_PFN_GET_DISPLAY_TYPE>(
          displayHook<decltype(&Display::getType), &Display::getType,
                      int32_t*>);
    case HWC2::FunctionDescriptor::GetDozeSupport:
      return asFP<HWC2_PFN_GET_DOZE_SUPPORT>(
          displayHook<decltype(&Display::getDozeSupport),
                      &Display::getDozeSupport, int32_t*>);
    case HWC2::FunctionDescriptor::GetHdrCapabilities:
      return asFP<HWC2_PFN_GET_HDR_CAPABILITIES>(
          displayHook<decltype(&Display::getHdrCapabilities),
                      &Display::getHdrCapabilities, uint32_t*, int32_t*, float*,
                      float*, float*>);
    case HWC2::FunctionDescriptor::GetReleaseFences:
      return asFP<HWC2_PFN_GET_RELEASE_FENCES>(
          displayHook<decltype(&Display::getReleaseFences),
                      &Display::getReleaseFences, uint32_t*, hwc2_layer_t*,
                      int32_t*>);
    case HWC2::FunctionDescriptor::PresentDisplay:
      return asFP<HWC2_PFN_PRESENT_DISPLAY>(
          displayHook<decltype(&Display::present), &Display::present,
                      int32_t*>);
    case HWC2::FunctionDescriptor::SetActiveConfig:
      return asFP<HWC2_PFN_SET_ACTIVE_CONFIG>(
          displayHook<decltype(&Display::setActiveConfig),
                      &Display::setActiveConfig, hwc2_config_t>);
    case HWC2::FunctionDescriptor::SetClientTarget:
      return asFP<HWC2_PFN_SET_CLIENT_TARGET>(
          displayHook<decltype(&Display::setClientTarget),
                      &Display::setClientTarget, buffer_handle_t, int32_t,
                      int32_t, hwc_region_t>);
    case HWC2::FunctionDescriptor::SetColorMode:
      return asFP<HWC2_PFN_SET_COLOR_MODE>(
          displayHook<decltype(&Display::setColorMode), &Display::setColorMode,
                      int32_t>);
    case HWC2::FunctionDescriptor::SetColorTransform:
      return asFP<HWC2_PFN_SET_COLOR_TRANSFORM>(
          displayHook<decltype(&Display::setColorTransform),
                      &Display::setColorTransform, const float*, int32_t>);
    case HWC2::FunctionDescriptor::SetOutputBuffer:
      return asFP<HWC2_PFN_SET_OUTPUT_BUFFER>(
          displayHook<decltype(&Display::setOutputBuffer),
                      &Display::setOutputBuffer, buffer_handle_t, int32_t>);
    case HWC2::FunctionDescriptor::SetPowerMode:
      return asFP<HWC2_PFN_SET_POWER_MODE>(
          displayHook<decltype(&Display::setPowerMode), &Display::setPowerMode,
                      int32_t>);
    case HWC2::FunctionDescriptor::SetVsyncEnabled:
      return asFP<HWC2_PFN_SET_VSYNC_ENABLED>(
          displayHook<decltype(&Display::setVsyncEnabled),
                      &Display::setVsyncEnabled, int32_t>);
    case HWC2::FunctionDescriptor::ValidateDisplay:
      return asFP<HWC2_PFN_VALIDATE_DISPLAY>(
          displayHook<decltype(&Display::validate), &Display::validate,
                      uint32_t*, uint32_t*>);
    case HWC2::FunctionDescriptor::GetClientTargetSupport:
      return asFP<HWC2_PFN_GET_CLIENT_TARGET_SUPPORT>(
          displayHook<decltype(&Display::getClientTargetSupport),
                      &Display::getClientTargetSupport, uint32_t, uint32_t,
                      int32_t, int32_t>);
    case HWC2::FunctionDescriptor::GetDisplayIdentificationData:
      return asFP<HWC2_PFN_GET_DISPLAY_IDENTIFICATION_DATA>(
          displayHook<decltype(&Display::getDisplayIdentificationData),
                      &Display::getDisplayIdentificationData, uint8_t*,
                      uint32_t*, uint8_t*>);
    case HWC2::FunctionDescriptor::GetDisplayCapabilities:
      return asFP<HWC2_PFN_GET_DISPLAY_CAPABILITIES>(
          displayHook<decltype(&Display::getDisplayCapabilities),
                      &Display::getDisplayCapabilities, uint32_t*, uint32_t*>);
    case HWC2::FunctionDescriptor::GetDisplayBrightnessSupport:
      return asFP<HWC2_PFN_GET_DISPLAY_BRIGHTNESS_SUPPORT>(
          displayHook<decltype(&Display::getDisplayBrightnessSupport),
                      &Display::getDisplayBrightnessSupport, bool*>);
    case HWC2::FunctionDescriptor::SetDisplayBrightness:
      return asFP<HWC2_PFN_SET_DISPLAY_BRIGHTNESS>(
          displayHook<decltype(&Display::setDisplayBrightness),
                      &Display::setDisplayBrightness, float>);

    // Layer functions
    case HWC2::FunctionDescriptor::SetCursorPosition:
      return asFP<HWC2_PFN_SET_CURSOR_POSITION>(
          layerHook<decltype(&Layer::setCursorPosition),
                    &Layer::setCursorPosition, int32_t, int32_t>);
    case HWC2::FunctionDescriptor::SetLayerBuffer:
      return asFP<HWC2_PFN_SET_LAYER_BUFFER>(
          layerHook<decltype(&Layer::setBuffer), &Layer::setBuffer,
                    buffer_handle_t, int32_t>);
    case HWC2::FunctionDescriptor::SetLayerSurfaceDamage:
      return asFP<HWC2_PFN_SET_LAYER_SURFACE_DAMAGE>(
          layerHook<decltype(&Layer::setSurfaceDamage),
                    &Layer::setSurfaceDamage, hwc_region_t>);
    case HWC2::FunctionDescriptor::SetLayerBlendMode:
      return asFP<HWC2_PFN_SET_LAYER_BLEND_MODE>(
          layerHook<decltype(&Layer::setBlendMode), &Layer::setBlendMode,
                    int32_t>);
    case HWC2::FunctionDescriptor::SetLayerColor:
      return asFP<HWC2_PFN_SET_LAYER_COLOR>(
          layerHook<decltype(&Layer::setColor), &Layer::setColor, hwc_color_t>);
    case HWC2::FunctionDescriptor::SetLayerCompositionType:
      return asFP<HWC2_PFN_SET_LAYER_COMPOSITION_TYPE>(
          layerHook<decltype(&Layer::setCompositionType),
                    &Layer::setCompositionType, int32_t>);
    case HWC2::FunctionDescriptor::SetLayerDataspace:
      return asFP<HWC2_PFN_SET_LAYER_DATASPACE>(
          layerHook<decltype(&Layer::setDataspace), &Layer::setDataspace,
                    int32_t>);
    case HWC2::FunctionDescriptor::SetLayerDisplayFrame:
      return asFP<HWC2_PFN_SET_LAYER_DISPLAY_FRAME>(
          layerHook<decltype(&Layer::setDisplayFrame), &Layer::setDisplayFrame,
                    hwc_rect_t>);
    case HWC2::FunctionDescriptor::SetLayerPlaneAlpha:
      return asFP<HWC2_PFN_SET_LAYER_PLANE_ALPHA>(
          layerHook<decltype(&Layer::setPlaneAlpha), &Layer::setPlaneAlpha,
                    float>);
    case HWC2::FunctionDescriptor::SetLayerSidebandStream:
      return asFP<HWC2_PFN_SET_LAYER_SIDEBAND_STREAM>(
          layerHook<decltype(&Layer::setSidebandStream),
                    &Layer::setSidebandStream, const native_handle_t*>);
    case HWC2::FunctionDescriptor::SetLayerSourceCrop:
      return asFP<HWC2_PFN_SET_LAYER_SOURCE_CROP>(
          layerHook<decltype(&Layer::setSourceCrop), &Layer::setSourceCrop,
                    hwc_frect_t>);
    case HWC2::FunctionDescriptor::SetLayerTransform:
      return asFP<HWC2_PFN_SET_LAYER_TRANSFORM>(
          layerHook<decltype(&Layer::setTransform), &Layer::setTransform,
                    int32_t>);
    case HWC2::FunctionDescriptor::SetLayerVisibleRegion:
      return asFP<HWC2_PFN_SET_LAYER_VISIBLE_REGION>(
          layerHook<decltype(&Layer::setVisibleRegion),
                    &Layer::setVisibleRegion, hwc_region_t>);
    case HWC2::FunctionDescriptor::SetLayerZOrder:
      return asFP<HWC2_PFN_SET_LAYER_Z_ORDER>(
          displayHook<decltype(&Display::updateLayerZ), &Display::updateLayerZ,
                      hwc2_layer_t, uint32_t>);

    default:
      ALOGE("GetFunction: Unknown function descriptor: %d",
            static_cast<int32_t>(desc));
      return nullptr;
  }
}

/*static*/
hwc2_function_pointer_t Device::getFunctionHook(hwc2_device_t* dev,
                                                int32_t desc) {
  Device* device = Device::fromDevice(dev);
  return device->getFunction(desc);
}

// Device functions

HWC2::Error Device::createVirtualDisplay(uint32_t /*width*/,
                                         uint32_t /*height*/,
                                         int32_t* /*format*/,
                                         hwc2_display_t* /*outDisplay*/) {
  DEBUG_LOG("%s", __FUNCTION__);
  // TODO: VirtualDisplay support
  return HWC2::Error::None;
}

HWC2::Error Device::destroyVirtualDisplay(hwc2_display_t /*displayId*/) {
  DEBUG_LOG("%s", __FUNCTION__);
  // TODO: VirtualDisplay support
  return HWC2::Error::None;
}

void Device::dump(uint32_t* /*outSize*/, char* /*outBuffer*/) {
  DEBUG_LOG("%s", __FUNCTION__);
  // TODO:
  return;
}

uint32_t Device::getMaxVirtualDisplayCount() {
  DEBUG_LOG("%s", __FUNCTION__);
  // TODO: VirtualDisplay support
  return 0;
}

static bool IsHandledCallback(HWC2::Callback descriptor) {
  switch (descriptor) {
    case HWC2::Callback::Hotplug: {
      return true;
    }
    case HWC2::Callback::Refresh: {
      return true;
    }
    case HWC2::Callback::Vsync: {
      return true;
    }
    case HWC2::Callback::Vsync_2_4: {
      return false;
    }
    case HWC2::Callback::VsyncPeriodTimingChanged: {
      return false;
    }
    case HWC2::Callback::Invalid: {
      return false;
    }
    case HWC2::Callback::SeamlessPossible: {
      return false;
    }
  }
  return false;
}

HWC2::Error Device::registerCallback(int32_t desc,
                                     hwc2_callback_data_t callbackData,
                                     hwc2_function_pointer_t pointer) {
  const auto callbackType = static_cast<HWC2::Callback>(desc);
  const auto callbackTypeString = to_string(callbackType);
  DEBUG_LOG("%s callback %s", __FUNCTION__, callbackTypeString.c_str());

  if (!IsHandledCallback(callbackType)) {
    ALOGE("%s unhandled callback: %s", __FUNCTION__,
          callbackTypeString.c_str());
    return HWC2::Error::BadParameter;
  }

  std::unique_lock<std::mutex> lock(mStateMutex);

  if (pointer != nullptr) {
    mCallbacks[callbackType] = {callbackData, pointer};
  } else {
    mCallbacks.erase(callbackType);
    return HWC2::Error::None;
  }

  if (callbackType == HWC2::Callback::Hotplug) {
    // Callback without the state lock held
    lock.unlock();
    auto hotplug = reinterpret_cast<HWC2_PFN_HOTPLUG>(pointer);
    auto hotplugConnect = static_cast<int32_t>(HWC2::Connection::Connected);
    for (const auto& [displayId, display] : mDisplays) {
      ALOGI("%s hotplug connecting display:%" PRIu64, __FUNCTION__, displayId);
      hotplug(callbackData, displayId, hotplugConnect);
    }
  }

  return HWC2::Error::None;
}

bool Device::handleHotplug(bool connected, uint32_t id, uint32_t width,
                           uint32_t height, uint32_t dpiX, uint32_t dpiY,
                           uint32_t refreshRate) {
  std::unique_lock<std::mutex> lock(mStateMutex);
  if (mCallbacks[HWC2::Callback::Hotplug].pointer == nullptr) {
    return false;
  }
  auto hotplug = reinterpret_cast<HWC2_PFN_HOTPLUG>(
      mCallbacks[HWC2::Callback::Hotplug].pointer);
  auto hotplugConnect = static_cast<int32_t>(HWC2::Connection::Connected);
  auto hotplugDisconnect = static_cast<int32_t>(HWC2::Connection::Disconnected);
  Display* display = getDisplay(id);
  if (display) {
    // if existed, disconnect first
    ALOGD("callback hotplugDisconnect display %" PRIu32, id);
    hotplug(mCallbacks[HWC2::Callback::Hotplug].data, id, hotplugDisconnect);
    display->lock();
    mComposer->onDisplayDestroy(display);
    display->unlock();
  }
  if (connected) {
    createDisplay(id, width, height, dpiX, dpiY, refreshRate);
    ALOGD("callback hotplugConnect display %" PRIu32 " width %" PRIu32
          " height %" PRIu32 " dpiX %" PRIu32 " dpiY %" PRIu32
          "fps %" PRIu32, id, width, height, dpiX, dpiY, refreshRate);
    hotplug(mCallbacks[HWC2::Callback::Hotplug].data, id, hotplugConnect);
  };

  return true;
}

Display* Device::getDisplay(hwc2_display_t id) {
  auto display = mDisplays.find(id);
  if (display == mDisplays.end()) {
    ALOGW("Failed to get display for id=%d", (uint32_t)id);
    return nullptr;
  }
  return display->second.get();
}

static int OpenDevice(const struct hw_module_t* module, const char* name,
                      struct hw_device_t** dev) {
  DEBUG_LOG("%s ", __FUNCTION__);

  if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
    ALOGE("Invalid module name- %s", name);
    return -EINVAL;
  }

  std::unique_ptr<Device> device = std::make_unique<Device>();
  HWC2::Error error = device->init();
  if (error != HWC2::Error::None) {
    ALOGE("%s: failed to initialize device", __FUNCTION__);
    return -EINVAL;
  }

  error = device->createDisplays();
  if (error != HWC2::Error::None) {
    ALOGE("%s: failed to initialize device displays.", __FUNCTION__);
    return -EINVAL;
  }

  device->common.module = const_cast<hw_module_t*>(module);
  *dev = &device.release()->common;
  return 0;
}

}  // namespace android

static struct hw_module_methods_t hwc2_module_methods = {
    .open = android::OpenDevice,
};

hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 2,
    .version_minor = 3,
    .id = HWC_HARDWARE_MODULE_ID,
    .name = "goldfish HWC2 module",
    .author = "The Android Open Source Project",
    .methods = &hwc2_module_methods,
    .dso = NULL,
    .reserved = {0},
};
