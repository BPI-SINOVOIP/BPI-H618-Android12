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

#ifndef ANDROID_HWC_DISPLAY_H
#define ANDROID_HWC_DISPLAY_H

#include <utils/Thread.h>

#include <optional>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Common.h"
#include "Composer.h"
#include "FencedBuffer.h"
#include "Layer.h"

namespace android {

class Composer;
class Device;

class Display {
 public:
  Display(Device& device, Composer* composer, hwc2_display_t id);
  ~Display();

  Display(const Display& display) = delete;
  Display& operator=(const Display& display) = delete;

  Display(Display&& display) = delete;
  Display& operator=(Display&& display) = delete;

  HWC2::Error init(uint32_t width, uint32_t height, uint32_t dpiX,
                   uint32_t dpiY, uint32_t refreshRateHz,
                   const std::optional<std::vector<uint8_t>>& edid = std::nullopt);

  HWC2::Error updateParameters(uint32_t width, uint32_t height, uint32_t dpiX,
                               uint32_t dpiY, uint32_t refreshRateHz,
                               const std::optional<std::vector<uint8_t>>& edid
                                   = std::nullopt);

  hwc2_display_t getId() const { return mId; }

  Layer* getLayer(hwc2_layer_t layerHandle);

  FencedBuffer& getClientTarget() { return mClientTarget; }
  buffer_handle_t waitAndGetClientTargetBuffer();

  const std::vector<Layer*>& getOrderedLayers() { return mOrderedLayers; }

  HWC2::Error acceptChanges();
  HWC2::Error createLayer(hwc2_layer_t* outLayerId);
  HWC2::Error destroyLayer(hwc2_layer_t layerId);
  HWC2::Error getActiveConfig(hwc2_config_t* outConfigId);
  HWC2::Error getDisplayAttribute(hwc2_config_t configId, int32_t attribute,
                                  int32_t* outValue);
  HWC2::Error getDisplayAttributeEnum(hwc2_config_t configId,
                                      HWC2::Attribute attribute,
                                      int32_t* outValue);
  HWC2::Error getChangedCompositionTypes(uint32_t* outNumElements,
                                         hwc2_layer_t* outLayers,
                                         int32_t* outTypes);
  HWC2::Error getColorModes(uint32_t* outNumModes, int32_t* outModes);
  HWC2::Error getConfigs(uint32_t* outNumConfigs, hwc2_config_t* outConfigIds);
  HWC2::Error getDozeSupport(int32_t* outSupport);
  HWC2::Error getHdrCapabilities(uint32_t* outNumTypes, int32_t* outTypes,
                                 float* outMaxLuminance,
                                 float* outMaxAverageLuminance,
                                 float* outMinLuminance);
  HWC2::Error getName(uint32_t* outSize, char* outName);
  HWC2::Error addReleaseFenceLocked(int32_t fence);
  HWC2::Error addReleaseLayerLocked(hwc2_layer_t layerId);
  HWC2::Error getReleaseFences(uint32_t* outNumElements,
                               hwc2_layer_t* outLayers, int32_t* outFences);
  HWC2::Error clearReleaseFencesAndIdsLocked();
  HWC2::Error getRequests(int32_t* outDisplayRequests, uint32_t* outNumElements,
                          hwc2_layer_t* outLayers, int32_t* outLayerRequests);
  HWC2::Error getType(int32_t* outType);
  HWC2::Error present(int32_t* outRetireFence);
  HWC2::Error setActiveConfig(hwc2_config_t configId);
  HWC2::Error setClientTarget(buffer_handle_t target, int32_t acquireFence,
                              int32_t dataspace, hwc_region_t damage);
  HWC2::Error setColorMode(int32_t mode);
  HWC2::Error setColorTransform(const float* matrix, int32_t hint);
  bool hasColorTransform() const { return mSetColorTransform; }
  HWC2::Error setOutputBuffer(buffer_handle_t buffer, int32_t releaseFence);
  HWC2::Error setPowerMode(int32_t mode);
  HWC2::Error setVsyncEnabled(int32_t enabled);
  HWC2::Error setVsyncPeriod(uint32_t period);
  HWC2::Error validate(uint32_t* outNumTypes, uint32_t* outNumRequests);
  HWC2::Error updateLayerZ(hwc2_layer_t layerId, uint32_t z);
  HWC2::Error getClientTargetSupport(uint32_t width, uint32_t height,
                                     int32_t format, int32_t dataspace);
  HWC2::Error getDisplayIdentificationData(uint8_t* outPort,
                                           uint32_t* outDataSize,
                                           uint8_t* outData);
  HWC2::Error getDisplayCapabilities(uint32_t* outNumCapabilities,
                                     uint32_t* outCapabilities);
  HWC2::Error getDisplayBrightnessSupport(bool* out_support);
  HWC2::Error setDisplayBrightness(float brightness);
  void lock() { mStateMutex.lock(); }
  void unlock() { mStateMutex.unlock(); }

 private:
  class Config {
   public:
    Config(hwc2_config_t configId) : mId(configId) {}

    Config(const Config& display) = default;
    Config& operator=(const Config& display) = default;

    Config(Config&& display) = default;
    Config& operator=(Config&& display) = default;

    hwc2_config_t getId() const { return mId; }
    void setId(hwc2_config_t id) { mId = id; }

    int32_t getAttribute(HWC2::Attribute attribute) const;
    void setAttribute(HWC2::Attribute attribute, int32_t value);

    std::string toString() const;

   private:
    hwc2_config_t mId;
    std::unordered_map<HWC2::Attribute, int32_t> mAttributes;
  };

  // Stores changes requested from the device upon calling prepare().
  // Handles change request to:
  //   - Layer composition type.
  //   - Layer hints.
  class Changes {
   public:
    uint32_t getNumTypes() const {
      return static_cast<uint32_t>(mTypeChanges.size());
    }

    uint32_t getNumLayerRequests() const {
      return static_cast<uint32_t>(mLayerRequests.size());
    }

    const std::unordered_map<hwc2_layer_t, HWC2::Composition>& getTypeChanges()
        const {
      return mTypeChanges;
    }

    const std::unordered_map<hwc2_layer_t, HWC2::LayerRequest>&
    getLayerRequests() const {
      return mLayerRequests;
    }

    void addTypeChange(hwc2_layer_t layerId, HWC2::Composition type) {
      mTypeChanges.insert({layerId, type});
    }

    void clearTypeChanges() { mTypeChanges.clear(); }

    void addLayerRequest(hwc2_layer_t layerId, HWC2::LayerRequest request) {
      mLayerRequests.insert({layerId, request});
    }

   private:
    std::unordered_map<hwc2_layer_t, HWC2::Composition> mTypeChanges;
    std::unordered_map<hwc2_layer_t, HWC2::LayerRequest> mLayerRequests;
  };

  // Generate sw vsync signal
  class VsyncThread : public Thread {
   public:
    VsyncThread(Display& display) : mDisplay(display) {}
    virtual ~VsyncThread() {}

    VsyncThread(const VsyncThread&) = default;
    VsyncThread& operator=(const VsyncThread&) = default;

    VsyncThread(VsyncThread&&) = default;
    VsyncThread& operator=(VsyncThread&&) = default;

   private:
    Display& mDisplay;
    bool threadLoop() final;
  };

 private:
  // The state of this display should only be modified from
  // SurfaceFlinger's main loop, with the exception of when dump is
  // called. To prevent a bad state from crashing us during a dump
  // call, all public calls into Display must acquire this mutex.
  mutable std::recursive_mutex mStateMutex;

  Device& mDevice;
  Composer* mComposer = nullptr;
  const hwc2_display_t mId;
  std::string mName;
  HWC2::DisplayType mType = HWC2::DisplayType::Physical;
  HWC2::PowerMode mPowerMode = HWC2::PowerMode::Off;
  HWC2::Vsync mVsyncEnabled = HWC2::Vsync::Invalid;
  uint32_t mVsyncPeriod;
  sp<VsyncThread> mVsyncThread;
  FencedBuffer mClientTarget;
  // Will only be non-null after the Display has been validated and
  // before it has been presented
  std::unique_ptr<Changes> mChanges;

  std::unordered_map<hwc2_layer_t, std::unique_ptr<Layer>> mLayers;
  // Ordered layers available after validate().
  std::vector<Layer*> mOrderedLayers;

  std::vector<hwc2_display_t> mReleaseLayerIds;
  std::vector<int32_t> mReleaseFences;
  std::optional<hwc2_config_t> mActiveConfigId;
  std::unordered_map<hwc2_config_t, Config> mConfigs;
  std::set<android_color_mode_t> mColorModes;
  android_color_mode_t mActiveColorMode;
  bool mSetColorTransform = false;
  std::optional<std::vector<uint8_t>> mEdid;
};

}  // namespace android

#endif
