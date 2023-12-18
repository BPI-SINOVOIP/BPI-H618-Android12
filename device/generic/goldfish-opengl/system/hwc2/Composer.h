/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_HWC_COMPOSER_H
#define ANDROID_HWC_COMPOSER_H

#include <functional>
#include <unordered_map>
#include <vector>

#include "Common.h"

namespace android {
class Device;
class Display;

class Composer {
 public:
  virtual ~Composer() {}

  using HotplugCallback = std::function<void(
      bool /*connected*/, uint32_t /*id*/, uint32_t /*width*/,
      uint32_t /*height*/, uint32_t /*dpiX*/, uint32_t /*dpiY*/,
      uint32_t /*refreshRate*/)>;

  virtual HWC2::Error init(const HotplugCallback& cb) = 0;

  using AddDisplayToDeviceFunction =
      std::function<HWC2::Error(std::unique_ptr<Display>)>;

  // Queries Cuttlefish/Goldfish/System configurations and creates displays
  // for the given Device.
  virtual HWC2::Error createDisplays(
      Device* device,
      const AddDisplayToDeviceFunction& addDisplayToDeviceFn) = 0;

  virtual HWC2::Error createDisplay(
      Device* device, uint32_t displayId, uint32_t width, uint32_t height,
      uint32_t dpiX, uint32_t dpiY, uint32_t refreshRateHz,
      const AddDisplayToDeviceFunction& addDisplayToDeviceFn) = 0;

  virtual HWC2::Error onDisplayDestroy(Display* display) = 0;

  virtual HWC2::Error onDisplayClientTargetSet(Display* display) = 0;

  // Determines if this composer can compose the given layers and requests
  // changes for layers that can't not be composed.
  virtual HWC2::Error validateDisplay(
      Display* display, std::unordered_map<hwc2_layer_t, HWC2::Composition>*
                            outLayerCompositionChanges) = 0;

  // Performs the actual composition of layers and presents the composed result
  // to the display.
  virtual HWC2::Error presentDisplay(Display* display,
                                     int32_t* outPresentFence) = 0;
};

}  // namespace android

#endif
