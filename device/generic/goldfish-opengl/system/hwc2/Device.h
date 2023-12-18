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

#ifndef ANDROID_HWC_DEVICE_H
#define ANDROID_HWC_DEVICE_H

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common.h"
#include "Composer.h"
#include "Display.h"
#include "Layer.h"

namespace android {

class Composer;
class Display;

class Device : public hwc2_device_t {
 public:
  static inline Device* fromDevice(hw_device_t* device) {
    return reinterpret_cast<Device*>(device);
  }

  static inline Device* fromDevice(hwc2_device_t* device) {
    return static_cast<Device*>(device);
  }

  Device();
  ~Device();

  HWC2::Error init();

  HWC2::Error createDisplays();

  HWC2::Error createDisplay(uint32_t displayId, uint32_t width, uint32_t height,
                            uint32_t dpiX, uint32_t dpiY, uint32_t refreshRate);

  Display* getDisplay(hwc2_display_t displayId);

 private:
  HWC2::Error destroyDisplays();

  bool handleHotplug(bool connected, uint32_t id, uint32_t width,
                     uint32_t height, uint32_t dpiX, uint32_t dpiY,
                     uint32_t refreshRate);

  void getCapabilities(uint32_t* outCount, int32_t* outCapabilities);
  static void getCapabilitiesHook(hwc2_device_t* device, uint32_t* outCount,
                                  int32_t* outCapabilities);

  hwc2_function_pointer_t getFunction(int32_t descriptor);
  static hwc2_function_pointer_t getFunctionHook(hwc2_device_t* device,
                                                 int32_t descriptor);

  // Wrapper to call a specific function on a specific device.
  template <typename T, typename HookType, HookType func, typename... Args>
  static T DeviceHook(hwc2_device_t* dev, Args... args) {
    Device* device = Device::fromDevice(dev);
    return static_cast<T>(((*device).*func)(std::forward<Args>(args)...));
  }

  // Wrapper to call a specific function on a specific display.
  template <typename HookType, HookType func, typename... Args>
  static int32_t displayHook(hwc2_device_t* dev, hwc2_display_t displayId,
                             Args... args) {
    Device* device = Device::fromDevice(dev);

    Display* display = device->getDisplay(displayId);
    if (display == nullptr) {
      return static_cast<int32_t>(HWC2::Error::BadDisplay);
    }

    return static_cast<int32_t>((display->*func)(std::forward<Args>(args)...));
  }

  // Wrapper to call a specific function on a specific layer.
  template <typename HookType, HookType func, typename... Args>
  static int32_t layerHook(hwc2_device_t* dev, hwc2_display_t displayId,
                           hwc2_layer_t layerId, Args... args) {
    Device* device = Device::fromDevice(dev);

    Display* display = device->getDisplay(displayId);
    if (display == nullptr) {
      return static_cast<int32_t>(HWC2::Error::BadDisplay);
    }

    Layer* layer = display->getLayer(layerId);
    if (layer == nullptr) {
      return static_cast<int32_t>(HWC2::Error::BadLayer);
    }

    return static_cast<int32_t>((layer->*func)(std::forward<Args>(args)...));
  }

  // Device functions
  HWC2::Error createVirtualDisplay(uint32_t width, uint32_t height,
                                   int32_t* format, hwc2_display_t* outDisplay);

  HWC2::Error destroyVirtualDisplay(hwc2_display_t display);

  void dump(uint32_t* outSize, char* outBuffer);

  uint32_t getMaxVirtualDisplayCount();

  HWC2::Error registerCallback(int32_t descriptor,
                               hwc2_callback_data_t callbackData,
                               hwc2_function_pointer_t pointer);

  // These are potentially accessed from multiple threads, and are protected
  // by this mutex.
  std::mutex mStateMutex;

  std::unique_ptr<Composer> mComposer;

  std::unordered_set<HWC2::Capability> mCapabilities;

  // For sharing Vsync callback with each displays Vsync thread.
  friend class Display;

  struct CallbackInfo {
    hwc2_callback_data_t data;
    hwc2_function_pointer_t pointer;
  };
  std::unordered_map<HWC2::Callback, CallbackInfo> mCallbacks;

  std::map<hwc2_display_t, std::unique_ptr<Display>> mDisplays;
};

}  // namespace android
#endif
