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

#ifndef ANDROID_HWC_GUESTCOMPOSER_H
#define ANDROID_HWC_GUESTCOMPOSER_H

#include "Common.h"
#include "Composer.h"
#include "DrmPresenter.h"
#include "Gralloc.h"
#include "Layer.h"

namespace android {

class GuestComposer : public Composer {
 public:
  GuestComposer() = default;

  GuestComposer(const GuestComposer&) = delete;
  GuestComposer& operator=(const GuestComposer&) = delete;

  GuestComposer(GuestComposer&&) = delete;
  GuestComposer& operator=(GuestComposer&&) = delete;

  HWC2::Error init(const HotplugCallback& cb) override;

  HWC2::Error createDisplays(
      Device* device,
      const AddDisplayToDeviceFunction& addDisplayToDeviceFn) override;

  HWC2::Error createDisplay(
      Device* device, uint32_t displayId, uint32_t width, uint32_t height,
      uint32_t dpiX, uint32_t dpiY, uint32_t refreshRateHz,
      const AddDisplayToDeviceFunction& addDisplayToDeviceFn) override;

  HWC2::Error onDisplayDestroy(Display*) override;

  HWC2::Error onDisplayClientTargetSet(Display*) override {
    return HWC2::Error::None;
  }

  // Determines if this composer can compose the given layers on the given
  // display and requests changes for layers that can't not be composed.
  HWC2::Error validateDisplay(
      Display* display, std::unordered_map<hwc2_layer_t, HWC2::Composition>*
                            outLayerCompositionChanges) override;

  // Performs the actual composition of layers and presents the composed result
  // to the display.
  HWC2::Error presentDisplay(Display* display,
                             int32_t* outPresentFence) override;

 private:
  struct DisplayConfig {
    int width;
    int height;
    int dpiX;
    int dpiY;
    int refreshRateHz;
  };

  HWC2::Error getDisplayConfigsFromDeviceConfig(
      std::vector<DisplayConfig>* configs);

  HWC2::Error getDisplayConfigsFromSystemProp(
      std::vector<DisplayConfig>* configs);

  // Returns true if the given layer's buffer has supported format.
  bool canComposeLayer(Layer* layer);

  // Composes the given layer into the given destination buffer.
  HWC2::Error composeLayerInto(Layer* layer, std::uint8_t* dstBuffer,
                               std::uint32_t dstBufferWidth,
                               std::uint32_t dstBufferHeight,
                               std::uint32_t dstBufferStrideBytes,
                               std::uint32_t dstBufferBytesPerPixel);

  struct GuestComposerDisplayInfo {
    // Additional per display buffer for the composition result.
    buffer_handle_t compositionResultBuffer = nullptr;

    std::unique_ptr<DrmBuffer> compositionResultDrmBuffer;
  };

  std::unordered_map<hwc2_display_t, GuestComposerDisplayInfo> mDisplayInfos;

  Gralloc mGralloc;

  DrmPresenter mDrmPresenter;

  // Cuttlefish on QEMU does not have a display. Disable presenting to avoid
  // spamming logcat with DRM commit failures.
  bool mPresentDisabled = false;

  uint8_t* getRotatingScratchBuffer(std::size_t neededSize,
                                    std::uint32_t order);
  uint8_t* getSpecialScratchBuffer(std::size_t neededSize);

  std::vector<uint8_t> mScratchBuffer;
  std::vector<uint8_t> mSpecialScratchBuffer;
};

}  // namespace android

#endif
