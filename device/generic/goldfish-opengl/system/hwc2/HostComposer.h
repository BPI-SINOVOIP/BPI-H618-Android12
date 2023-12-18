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

#ifndef ANDROID_HWC_HOSTCOMPOSER_H
#define ANDROID_HWC_HOSTCOMPOSER_H

#include "Common.h"
#include "Composer.h"
#include "DrmPresenter.h"
#include "HostConnection.h"

namespace android {

class HostComposer : public Composer {
 public:
  HostComposer() = default;

  HostComposer(const HostComposer&) = delete;
  HostComposer& operator=(const HostComposer&) = delete;

  HostComposer(HostComposer&&) = delete;
  HostComposer& operator=(HostComposer&&) = delete;

  HWC2::Error init(const HotplugCallback& cb) override;

  HWC2::Error createDisplays(
      Device* device,
      const AddDisplayToDeviceFunction& addDisplayToDeviceFn) override;

  HWC2::Error createDisplay(
      Device* device, uint32_t displayId, uint32_t width, uint32_t height,
      uint32_t dpiX, uint32_t dpiY, uint32_t refreshRateHz,
      const AddDisplayToDeviceFunction& addDisplayToDeviceFn) override;

  HWC2::Error onDisplayDestroy(Display* display) override;

  HWC2::Error onDisplayClientTargetSet(Display* display) override;

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
  HWC2::Error createPrimaryDisplay(
      Device* device, const AddDisplayToDeviceFunction& addDisplayToDeviceFn);

  HWC2::Error createSecondaryDisplays(
      Device* device, const AddDisplayToDeviceFunction& addDisplayToDeviceFn);

  HWC2::Error createHostComposerDisplayInfo(Display* display,
                                            uint32_t hostDisplayId);

  void post(HostConnection* hostCon, ExtendedRCEncoderContext* rcEnc,
            buffer_handle_t h);

  bool mIsMinigbm = false;

  int mSyncDeviceFd = -1;

  struct HostComposerDisplayInfo {
    uint32_t hostDisplayId = 0;

    // Additional per display buffer for the composition result.
    const native_handle_t* compositionResultBuffer = nullptr;

    // Drm info for the additional composition result buffer.
    std::unique_ptr<DrmBuffer> compositionResultDrmBuffer;

    // Drm info for the displays client target buffer.
    std::unique_ptr<DrmBuffer> clientTargetDrmBuffer;
  };

  std::unordered_map<hwc2_display_t, HostComposerDisplayInfo> mDisplayInfos;

  DrmPresenter mDrmPresenter;
};

}  // namespace android

#endif
