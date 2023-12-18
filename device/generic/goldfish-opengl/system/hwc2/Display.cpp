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

#include "Display.h"

#include <sync/sync.h>

#include <atomic>
#include <numeric>

#include "Device.h"

namespace android {
namespace {

std::atomic<hwc2_config_t> sNextConfigId{0};

bool IsValidColorMode(android_color_mode_t mode) {
  switch (mode) {
    case HAL_COLOR_MODE_NATIVE:                         // Fall-through
    case HAL_COLOR_MODE_STANDARD_BT601_625:             // Fall-through
    case HAL_COLOR_MODE_STANDARD_BT601_625_UNADJUSTED:  // Fall-through
    case HAL_COLOR_MODE_STANDARD_BT601_525:             // Fall-through
    case HAL_COLOR_MODE_STANDARD_BT601_525_UNADJUSTED:  // Fall-through
    case HAL_COLOR_MODE_STANDARD_BT709:                 // Fall-through
    case HAL_COLOR_MODE_DCI_P3:                         // Fall-through
    case HAL_COLOR_MODE_SRGB:                           // Fall-through
    case HAL_COLOR_MODE_ADOBE_RGB:                      // Fall-through
    case HAL_COLOR_MODE_DISPLAY_P3:
      return true;
    default:
      return false;
  }
}

bool isValidPowerMode(HWC2::PowerMode mode) {
  switch (mode) {
    case HWC2::PowerMode::Off:          // Fall-through
    case HWC2::PowerMode::DozeSuspend:  // Fall-through
    case HWC2::PowerMode::Doze:         // Fall-through
    case HWC2::PowerMode::On:
      return true;
    default:
      return false;
  }
}

}  // namespace

Display::Display(Device& device, Composer* composer, hwc2_display_t id)
    : mDevice(device),
      mComposer(composer),
      mId(id),
      mVsyncThread(new VsyncThread(*this)) {}

Display::~Display() {}

HWC2::Error Display::init(uint32_t width, uint32_t height, uint32_t dpiX,
                          uint32_t dpiY, uint32_t refreshRateHz,
                          const std::optional<std::vector<uint8_t>>& edid) {
  ALOGD("%s initializing display:%" PRIu64
        " width:%d height:%d dpiX:%d dpiY:%d refreshRateHz:%d",
        __FUNCTION__, mId, width, height, dpiX, dpiY, refreshRateHz);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  mVsyncPeriod = 1000 * 1000 * 1000 / refreshRateHz;
  mVsyncThread->run("", ANDROID_PRIORITY_URGENT_DISPLAY);

  hwc2_config_t configId = sNextConfigId++;

  Config config(configId);
  config.setAttribute(HWC2::Attribute::VsyncPeriod, mVsyncPeriod);
  config.setAttribute(HWC2::Attribute::Width, width);
  config.setAttribute(HWC2::Attribute::Height, height);
  config.setAttribute(HWC2::Attribute::DpiX, dpiX * 1000);
  config.setAttribute(HWC2::Attribute::DpiY, dpiY * 1000);
  mConfigs.emplace(configId, config);

  mActiveConfigId = configId;
  mActiveColorMode = HAL_COLOR_MODE_NATIVE;
  mColorModes.emplace((android_color_mode_t)HAL_COLOR_MODE_NATIVE);
  mEdid = edid;

  return HWC2::Error::None;
}

HWC2::Error Display::updateParameters(
    uint32_t width, uint32_t height, uint32_t dpiX, uint32_t dpiY,
    uint32_t refreshRateHz, const std::optional<std::vector<uint8_t>>& edid) {
  DEBUG_LOG("%s updating display:%" PRIu64
            " width:%d height:%d dpiX:%d dpiY:%d refreshRateHz:%d",
            __FUNCTION__, mId, width, height, dpiX, dpiY, refreshRateHz);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  mVsyncPeriod = 1000 * 1000 * 1000 / refreshRateHz;

  auto it = mConfigs.find(*mActiveConfigId);
  if (it == mConfigs.end()) {
    ALOGE("%s: failed to find config %" PRIu32, __func__, *mActiveConfigId);
    return HWC2::Error::NoResources;
  }
  it->second.setAttribute(HWC2::Attribute::VsyncPeriod, mVsyncPeriod);
  it->second.setAttribute(HWC2::Attribute::Width, width);
  it->second.setAttribute(HWC2::Attribute::Height, height);
  it->second.setAttribute(HWC2::Attribute::DpiX, dpiX * 1000);
  it->second.setAttribute(HWC2::Attribute::DpiY, dpiY * 1000);

  mEdid = edid;

  return HWC2::Error::None;
}

Layer* Display::getLayer(hwc2_layer_t layerId) {
  auto it = mLayers.find(layerId);
  if (it == mLayers.end()) {
    ALOGE("%s Unknown layer:%" PRIu64, __FUNCTION__, layerId);
    return nullptr;
  }

  return it->second.get();
}

buffer_handle_t Display::waitAndGetClientTargetBuffer() {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  int fence = mClientTarget.getFence();
  if (fence != -1) {
    int err = sync_wait(fence, 3000);
    if (err < 0 && errno == ETIME) {
      ALOGE("%s waited on fence %" PRId32 " for 3000 ms", __FUNCTION__, fence);
    }
    close(fence);
  }

  return mClientTarget.getBuffer();
}

HWC2::Error Display::acceptChanges() {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!mChanges) {
    ALOGE("%s: display %" PRIu64 " failed, not validated", __FUNCTION__, mId);
    return HWC2::Error::NotValidated;
  }

  for (auto& [layerId, layerCompositionType] : mChanges->getTypeChanges()) {
    auto* layer = getLayer(layerId);
    if (layer == nullptr) {
      ALOGE("%s: display:%" PRIu64 " layer:%" PRIu64
            " dropped before AcceptChanges?",
            __FUNCTION__, mId, layerId);
      continue;
    }

    layer->setCompositionTypeEnum(layerCompositionType);
  }
  mChanges->clearTypeChanges();

  return HWC2::Error::None;
}

HWC2::Error Display::createLayer(hwc2_layer_t* outLayerId) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  auto layer = std::make_unique<Layer>();
  auto layerId = layer->getId();
  DEBUG_LOG("%s created layer:%" PRIu64, __FUNCTION__, layerId);

  *outLayerId = layerId;

  mLayers.emplace(layerId, std::move(layer));

  return HWC2::Error::None;
}

HWC2::Error Display::destroyLayer(hwc2_layer_t layerId) {
  DEBUG_LOG("%s destroy layer:%" PRIu64, __FUNCTION__, layerId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  auto it = mLayers.find(layerId);
  if (it == mLayers.end()) {
    ALOGE("%s display:%" PRIu64 " has no such layer:%." PRIu64, __FUNCTION__,
          mId, layerId);
    return HWC2::Error::BadLayer;
  }

  mOrderedLayers.erase(std::remove_if(mOrderedLayers.begin(),  //
                                      mOrderedLayers.end(),    //
                                      [layerId](Layer* layer) {
                                        return layer->getId() == layerId;
                                      }),
                       mOrderedLayers.end());

  mLayers.erase(it);

  DEBUG_LOG("%s destroyed layer:%" PRIu64, __FUNCTION__, layerId);
  return HWC2::Error::None;
}

HWC2::Error Display::getActiveConfig(hwc2_config_t* outConfig) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!mActiveConfigId) {
    ALOGW("%s: display:%" PRIu64 " has no active config.", __FUNCTION__, mId);
    return HWC2::Error::BadConfig;
  }

  *outConfig = *mActiveConfigId;
  return HWC2::Error::None;
}

HWC2::Error Display::getDisplayAttributeEnum(hwc2_config_t configId,
                                             HWC2::Attribute attribute,
                                             int32_t* outValue) {
  auto attributeString = to_string(attribute);
  DEBUG_LOG("%s: display:%" PRIu64 " attribute:%s", __FUNCTION__, mId,
            attributeString.c_str());

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  auto it = mConfigs.find(configId);
  if (it == mConfigs.end()) {
    ALOGW("%s: display:%" PRIu64 "bad config:%" PRIu32, __FUNCTION__, mId,
          configId);
    return HWC2::Error::BadConfig;
  }

  const Config& config = it->second;
  *outValue = config.getAttribute(attribute);
  DEBUG_LOG("%s: display:%" PRIu64 " attribute:%s value is %" PRIi32,
            __FUNCTION__, mId, attributeString.c_str(), *outValue);
  return HWC2::Error::None;
}

HWC2::Error Display::getDisplayAttribute(hwc2_config_t configId,
                                         int32_t attribute, int32_t* outValue) {
  return getDisplayAttributeEnum(
      configId, static_cast<HWC2::Attribute>(attribute), outValue);
}

HWC2::Error Display::getChangedCompositionTypes(uint32_t* outNumElements,
                                                hwc2_layer_t* outLayers,
                                                int32_t* outTypes) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!mChanges) {
    ALOGE("%s: for display:%" PRIu64 " failed, display not validated",
          __FUNCTION__, mId);
    return HWC2::Error::NotValidated;
  }

  if ((outLayers == nullptr) || (outTypes == nullptr)) {
    *outNumElements = mChanges->getTypeChanges().size();
    return HWC2::Error::None;
  }

  uint32_t numWritten = 0;
  for (const auto& element : mChanges->getTypeChanges()) {
    if (numWritten == *outNumElements) {
      break;
    }

    auto layerId = element.first;
    const auto layerCompositionType = element.second;
    const auto layerCompositionTypeString = to_string(layerCompositionType);
    DEBUG_LOG("%s: display:%" PRIu64 " layer:%" PRIu64 " changed to %s",
              __FUNCTION__, mId, layerId, layerCompositionTypeString.c_str());

    outLayers[numWritten] = layerId;
    outTypes[numWritten] = static_cast<int32_t>(layerCompositionType);
    ++numWritten;
  }
  *outNumElements = numWritten;
  return HWC2::Error::None;
}

HWC2::Error Display::getColorModes(uint32_t* outNumModes, int32_t* outModes) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!outModes) {
    *outNumModes = mColorModes.size();
    return HWC2::Error::None;
  }

  // we only support HAL_COLOR_MODE_NATIVE so far
  uint32_t numModes = std::min<uint32_t>(
      *outNumModes, static_cast<uint32_t>(mColorModes.size()));
  std::copy_n(mColorModes.cbegin(), numModes, outModes);
  *outNumModes = numModes;
  return HWC2::Error::None;
}

HWC2::Error Display::getConfigs(uint32_t* outNumConfigs,
                                hwc2_config_t* outConfigs) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!outConfigs) {
    *outNumConfigs = mConfigs.size();
    return HWC2::Error::None;
  }

  uint32_t numWritten = 0;
  for (const auto& [configId, config] : mConfigs) {
    if (numWritten == *outNumConfigs) {
      break;
    }
    outConfigs[numWritten] = configId;
    ++numWritten;
  }

  *outNumConfigs = numWritten;
  return HWC2::Error::None;
}

HWC2::Error Display::getDozeSupport(int32_t* outSupport) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  // We don't support so far
  *outSupport = 0;
  return HWC2::Error::None;
}

HWC2::Error Display::getHdrCapabilities(uint32_t* outNumTypes,
                                        int32_t* /*outTypes*/,
                                        float* /*outMaxLuminance*/,
                                        float* /*outMaxAverageLuminance*/,
                                        float* /*outMinLuminance*/) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  // We don't support so far
  *outNumTypes = 0;
  return HWC2::Error::None;
}

HWC2::Error Display::getName(uint32_t* outSize, char* outName) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!outName) {
    *outSize = mName.size();
    return HWC2::Error::None;
  }
  auto numCopied = mName.copy(outName, *outSize);
  *outSize = numCopied;
  return HWC2::Error::None;
}

HWC2::Error Display::addReleaseFenceLocked(int32_t fence) {
  DEBUG_LOG("%s: display:%" PRIu64 " fence:%d", __FUNCTION__, mId, fence);

  mReleaseFences.push_back(fence);
  return HWC2::Error::None;
}

HWC2::Error Display::addReleaseLayerLocked(hwc2_layer_t layerId) {
  DEBUG_LOG("%s: display:%" PRIu64 " layer:%" PRIu64, __FUNCTION__, mId,
            layerId);

  mReleaseLayerIds.push_back(layerId);
  return HWC2::Error::None;
}

HWC2::Error Display::getReleaseFences(uint32_t* outNumElements,
                                      hwc2_layer_t* outLayers,
                                      int32_t* outFences) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  *outNumElements = mReleaseLayerIds.size();

  if (*outNumElements && outLayers) {
    DEBUG_LOG("%s export release layers", __FUNCTION__);
    memcpy(outLayers, mReleaseLayerIds.data(),
           sizeof(hwc2_layer_t) * (*outNumElements));
  }

  if (*outNumElements && outFences) {
    DEBUG_LOG("%s export release fences", __FUNCTION__);
    memcpy(outFences, mReleaseFences.data(),
           sizeof(int32_t) * (*outNumElements));
  }

  return HWC2::Error::None;
}

HWC2::Error Display::clearReleaseFencesAndIdsLocked() {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  mReleaseLayerIds.clear();
  mReleaseFences.clear();

  return HWC2::Error::None;
}

HWC2::Error Display::getRequests(int32_t* outDisplayRequests,
                                 uint32_t* outNumElements,
                                 hwc2_layer_t* outLayers,
                                 int32_t* outLayerRequests) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!mChanges) {
    return HWC2::Error::NotValidated;
  }

  if (outLayers == nullptr || outLayerRequests == nullptr) {
    *outNumElements = mChanges->getNumLayerRequests();
    return HWC2::Error::None;
  }

  // TODO
  //  Display requests (HWC2::DisplayRequest) are not supported so far:
  *outDisplayRequests = 0;

  uint32_t numWritten = 0;
  for (const auto& request : mChanges->getLayerRequests()) {
    if (numWritten == *outNumElements) {
      break;
    }
    outLayers[numWritten] = request.first;
    outLayerRequests[numWritten] = static_cast<int32_t>(request.second);
    ++numWritten;
  }

  return HWC2::Error::None;
}

HWC2::Error Display::getType(int32_t* outType) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  *outType = (int32_t)mType;
  return HWC2::Error::None;
}

HWC2::Error Display::present(int32_t* outRetireFence) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  *outRetireFence = -1;

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!mChanges || (mChanges->getNumTypes() > 0)) {
    ALOGE("%s: display:%" PRIu64 " failed, not validated", __FUNCTION__, mId);
    return HWC2::Error::NotValidated;
  }
  mChanges.reset();

  if (mComposer == nullptr) {
    ALOGE("%s: display:%" PRIu64 " missing composer", __FUNCTION__, mId);
    return HWC2::Error::NoResources;
  }

  HWC2::Error error = mComposer->presentDisplay(this, outRetireFence);
  if (error != HWC2::Error::None) {
    ALOGE("%s: display:%" PRIu64 " failed to present", __FUNCTION__, mId);
    return error;
  }

  DEBUG_LOG("%s: display:%" PRIu64 " present done!", __FUNCTION__, mId);
  return HWC2::Error::None;
}

HWC2::Error Display::setActiveConfig(hwc2_config_t configId) {
  DEBUG_LOG("%s: display:%" PRIu64 " setting active config to %" PRIu32,
            __FUNCTION__, mId, configId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (mConfigs.find(configId) == mConfigs.end()) {
    ALOGE("%s: display:%" PRIu64 " bad config:%" PRIu32, __FUNCTION__, mId,
          configId);
    return HWC2::Error::BadConfig;
  }

  mActiveConfigId = configId;
  return HWC2::Error::None;
}

HWC2::Error Display::setClientTarget(buffer_handle_t target,
                                     int32_t acquireFence,
                                     int32_t /*dataspace*/,
                                     hwc_region_t /*damage*/) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);
  mClientTarget.setBuffer(target);
  mClientTarget.setFence(acquireFence);
  mComposer->onDisplayClientTargetSet(this);
  return HWC2::Error::None;
}

HWC2::Error Display::setColorMode(int32_t intMode) {
  DEBUG_LOG("%s: display:%" PRIu64 " setting color mode to %" PRId32,
            __FUNCTION__, mId, intMode);

  auto mode = static_cast<android_color_mode_t>(intMode);
  if (!IsValidColorMode(mode)) {
    ALOGE("%s: display:%" PRIu64 " invalid color mode %" PRId32, __FUNCTION__,
          mId, intMode);
    return HWC2::Error::BadParameter;
  }

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (mColorModes.count(mode) == 0) {
    ALOGE("%s: display %" PRIu64 " mode %d not found", __FUNCTION__, mId,
          intMode);
    return HWC2::Error::Unsupported;
  }
  mActiveColorMode = mode;
  return HWC2::Error::None;
}

HWC2::Error Display::setColorTransform(const float* /*matrix*/, int32_t hint) {
  DEBUG_LOG("%s: display:%" PRIu64 " setting hint to %d", __FUNCTION__, mId,
            hint);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);
  // we force client composition if this is set
  if (hint == 0) {
    mSetColorTransform = false;
  } else {
    mSetColorTransform = true;
  }
  return HWC2::Error::None;
}

HWC2::Error Display::setOutputBuffer(buffer_handle_t /*buffer*/,
                                     int32_t /*releaseFence*/) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);
  // TODO: for virtual display
  return HWC2::Error::None;
}

HWC2::Error Display::setPowerMode(int32_t intMode) {
  auto mode = static_cast<HWC2::PowerMode>(intMode);
  auto modeString = to_string(mode);
  DEBUG_LOG("%s: display:%" PRIu64 " setting power mode to %s", __FUNCTION__,
            mId, modeString.c_str());

  if (!isValidPowerMode(mode)) {
    return HWC2::Error::BadParameter;
  }

  if (mode == HWC2::PowerMode::Doze || mode == HWC2::PowerMode::DozeSuspend) {
    ALOGE("%s display %" PRIu64 " power mode %s not supported", __FUNCTION__,
          mId, modeString.c_str());
    return HWC2::Error::Unsupported;
  }

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  mPowerMode = mode;
  return HWC2::Error::None;
}

HWC2::Error Display::setVsyncEnabled(int32_t intEnable) {
  auto enable = static_cast<HWC2::Vsync>(intEnable);
  auto enableString = to_string(enable);
  DEBUG_LOG("%s: display:%" PRIu64 " setting vsync to %s", __FUNCTION__, mId,
            enableString.c_str());

  if (enable == HWC2::Vsync::Invalid) {
    return HWC2::Error::BadParameter;
  }

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);
  DEBUG_LOG("%s: display:%" PRIu64 " setting vsync locked to %s", __FUNCTION__,
            mId, enableString.c_str());

  mVsyncEnabled = enable;
  return HWC2::Error::None;
}

HWC2::Error Display::setVsyncPeriod(uint32_t period) {
  DEBUG_LOG("%s: display:%" PRIu64 " setting vsync period to %d", __FUNCTION__,
            mId, period);

  mVsyncPeriod = period;
  return HWC2::Error::None;
}

HWC2::Error Display::validate(uint32_t* outNumTypes, uint32_t* outNumRequests) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  mOrderedLayers.clear();
  mOrderedLayers.reserve(mLayers.size());
  for (auto& [_, layerPtr] : mLayers) {
    mOrderedLayers.push_back(layerPtr.get());
  }

  std::sort(mOrderedLayers.begin(), mOrderedLayers.end(),
            [](const Layer* layerA, const Layer* layerB) {
              const auto zA = layerA->getZ();
              const auto zB = layerB->getZ();
              if (zA != zB) {
                return zA < zB;
              }
              return layerA->getId() < layerB->getId();
            });

  if (!mChanges) {
    mChanges.reset(new Changes);
  } else {
    ALOGE("Validate was called more than once!");
  }

  if (mComposer == nullptr) {
    ALOGE("%s: display:%" PRIu64 " missing composer", __FUNCTION__, mId);
    return HWC2::Error::NoResources;
  }

  std::unordered_map<hwc2_layer_t, HWC2::Composition> changes;

  HWC2::Error error = mComposer->validateDisplay(this, &changes);
  if (error != HWC2::Error::None) {
    ALOGE("%s: display:%" PRIu64 " failed to validate", __FUNCTION__, mId);
    return error;
  }

  for (const auto& [layerId, changedCompositionType] : changes) {
    mChanges->addTypeChange(layerId, changedCompositionType);
  }

  *outNumTypes = mChanges->getNumTypes();
  *outNumRequests = mChanges->getNumLayerRequests();
  return *outNumTypes > 0 ? HWC2::Error::HasChanges : HWC2::Error::None;
}

HWC2::Error Display::updateLayerZ(hwc2_layer_t layerId, uint32_t z) {
  DEBUG_LOG("%s: display:%" PRIu64 " update layer:%" PRIu64 " z:%d",
            __FUNCTION__, mId, layerId, z);

  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  const auto layerIt = mLayers.find(layerId);
  if (layerIt == mLayers.end()) {
    ALOGE("%s failed to find layer %" PRIu64, __FUNCTION__, layerId);
    return HWC2::Error::BadLayer;
  }

  auto& layer = layerIt->second;
  layer->setZ(z);
  return HWC2::Error::None;
}

HWC2::Error Display::getClientTargetSupport(uint32_t width, uint32_t height,
                                            int32_t format, int32_t dataspace) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);
  std::unique_lock<std::recursive_mutex> lock(mStateMutex);

  if (!mActiveConfigId) {
    return HWC2::Error::Unsupported;
  }

  const auto it = mConfigs.find(*mActiveConfigId);
  if (it == mConfigs.end()) {
    ALOGE("%s failed to find active config:%" PRIu32, __FUNCTION__,
          *mActiveConfigId);
    return HWC2::Error::Unsupported;
  }

  const Config& activeConfig = it->second;
  const uint32_t activeConfigWidth =
      static_cast<uint32_t>(activeConfig.getAttribute(HWC2::Attribute::Width));
  const uint32_t activeConfigHeight =
      static_cast<uint32_t>(activeConfig.getAttribute(HWC2::Attribute::Height));
  if (width == activeConfigWidth && height == activeConfigHeight &&
      format == HAL_PIXEL_FORMAT_RGBA_8888 &&
      dataspace == HAL_DATASPACE_UNKNOWN) {
    return HWC2::Error::None;
  }

  return HWC2::Error::None;
}

// thess EDIDs are carefully generated according to the EDID spec version 1.3,
// more info can be found from the following file:
//   frameworks/native/services/surfaceflinger/DisplayHardware/DisplayIdentification.cpp
// approved pnp ids can be found here: https://uefi.org/pnp_id_list
// pnp id: GGL, name: EMU_display_0, last byte is checksum
// display id is local:8141603649153536
static const uint8_t sEDID0[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1c, 0xec, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x1b, 0x10, 0x01, 0x03, 0x80, 0x50, 0x2d, 0x78,
    0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38,
    0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x45, 0x4d, 0x55, 0x5f, 0x64, 0x69, 0x73,
    0x70, 0x6c, 0x61, 0x79, 0x5f, 0x30, 0x00, 0x4b};

// pnp id: GGL, name: EMU_display_1
// display id is local:8140900251843329
static const uint8_t sEDID1[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1c, 0xec, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x1b, 0x10, 0x01, 0x03, 0x80, 0x50, 0x2d, 0x78,
    0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38,
    0x2d, 0x40, 0x58, 0x2c, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x45, 0x4d, 0x55, 0x5f, 0x64, 0x69, 0x73,
    0x70, 0x6c, 0x61, 0x79, 0x5f, 0x31, 0x00, 0x3b};

// pnp id: GGL, name: EMU_display_2
// display id is local:8140940453066754
static const uint8_t sEDID2[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1c, 0xec, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x1b, 0x10, 0x01, 0x03, 0x80, 0x50, 0x2d, 0x78,
    0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38,
    0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x45, 0x4d, 0x55, 0x5f, 0x64, 0x69, 0x73,
    0x70, 0x6c, 0x61, 0x79, 0x5f, 0x32, 0x00, 0x49};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

HWC2::Error Display::getDisplayIdentificationData(uint8_t* outPort,
                                                  uint32_t* outDataSize,
                                                  uint8_t* outData) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  if (outPort == nullptr || outDataSize == nullptr) {
    return HWC2::Error::BadParameter;
  }

  if (mEdid) {
    if (outData) {
      *outDataSize = std::min<uint32_t>(*outDataSize, (*mEdid).size());
      memcpy(outData, (*mEdid).data(), *outDataSize);
    } else {
      *outDataSize = (*mEdid).size();
    }
    *outPort = mId;
    return HWC2::Error::None;
  }

  // fallback to legacy EDID implementation
  uint32_t len = std::min(*outDataSize, (uint32_t)ARRAY_SIZE(sEDID0));
  if (outData != nullptr && len < (uint32_t)ARRAY_SIZE(sEDID0)) {
    ALOGW("%s: display:%" PRIu64 " small buffer size: %u is specified",
          __FUNCTION__, mId, len);
  }
  *outDataSize = ARRAY_SIZE(sEDID0);
  switch (mId) {
    case 0:
      *outPort = 0;
      if (outData) memcpy(outData, sEDID0, len);
      break;

    case 1:
      *outPort = 1;
      if (outData) memcpy(outData, sEDID1, len);
      break;

    case 2:
      *outPort = 2;
      if (outData) memcpy(outData, sEDID2, len);
      break;

    default:
      *outPort = (uint8_t)mId;
      if (outData) {
        memcpy(outData, sEDID2, len);
        uint32_t size = ARRAY_SIZE(sEDID0);
        // change the name to EMU_display_<mID>
        // note the 3rd char from back is the number, _0, _1, _2, etc.
        if (len >= size - 2) outData[size - 3] = '0' + (uint8_t)mId;
        if (len >= size) {
          // update the last byte, which is checksum byte
          uint8_t checksum = -(uint8_t)std::accumulate(
              outData, outData + size - 1, static_cast<uint8_t>(0));
          outData[size - 1] = checksum;
        }
      }
      break;
  }

  return HWC2::Error::None;
}

HWC2::Error Display::getDisplayCapabilities(uint32_t* outNumCapabilities,
                                            uint32_t* outCapabilities) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);
  if (outNumCapabilities == nullptr) {
    return HWC2::Error::None;
  }

  bool brightness_support = false;
  bool doze_support = false;

  uint32_t count = 1 + (doze_support ? 1 : 0) + (brightness_support ? 1 : 0);
  int index = 0;
  if (outCapabilities != nullptr && (*outNumCapabilities >= count)) {
    outCapabilities[index++] =
        HWC2_DISPLAY_CAPABILITY_SKIP_CLIENT_COLOR_TRANSFORM;
    if (doze_support) {
      outCapabilities[index++] = HWC2_DISPLAY_CAPABILITY_DOZE;
    }
    if (brightness_support) {
      outCapabilities[index++] = HWC2_DISPLAY_CAPABILITY_BRIGHTNESS;
    }
  }

  *outNumCapabilities = count;
  return HWC2::Error::None;
}

HWC2::Error Display::getDisplayBrightnessSupport(bool* out_support) {
  DEBUG_LOG("%s: display:%" PRIu64, __FUNCTION__, mId);

  *out_support = false;
  return HWC2::Error::None;
}

HWC2::Error Display::setDisplayBrightness(float brightness) {
  DEBUG_LOG("%s: display:%" PRIu64 " brightness %f", __FUNCTION__, mId,
            brightness);

  ALOGW("TODO: setDisplayBrightness() is not implemented yet: brightness=%f",
        brightness);
  return HWC2::Error::Unsupported;
}

void Display::Config::setAttribute(HWC2::Attribute attribute, int32_t value) {
  mAttributes[attribute] = value;
}

int32_t Display::Config::getAttribute(HWC2::Attribute attribute) const {
  if (mAttributes.count(attribute) == 0) {
    return -1;
  }
  return mAttributes.at(attribute);
}

std::string Display::Config::toString() const {
  std::string output;

  auto widthIt = mAttributes.find(HWC2::Attribute::Width);
  if (widthIt != mAttributes.end()) {
    output += " w:" + std::to_string(widthIt->second);
  }

  auto heightIt = mAttributes.find(HWC2::Attribute::Height);
  if (heightIt != mAttributes.end()) {
    output += " h:" + std::to_string(heightIt->second);
  }

  auto vsyncIt = mAttributes.find(HWC2::Attribute::VsyncPeriod);
  if (vsyncIt != mAttributes.end()) {
    output += " vsync:" + std::to_string(1e9 / vsyncIt->second);
  }

  auto dpiXIt = mAttributes.find(HWC2::Attribute::DpiX);
  if (dpiXIt != mAttributes.end()) {
    output += " dpi-x:" + std::to_string(dpiXIt->second / 1000.0f);
  }

  auto dpiYIt = mAttributes.find(HWC2::Attribute::DpiY);
  if (dpiYIt != mAttributes.end()) {
    output += " dpi-y:" + std::to_string(dpiYIt->second / 1000.0f);
  }

  return output;
}

// VsyncThread function
bool Display::VsyncThread::threadLoop() {
  struct timespec rt;
  if (clock_gettime(CLOCK_MONOTONIC, &rt) == -1) {
    ALOGE("%s: error in vsync thread clock_gettime: %s", __FUNCTION__,
          strerror(errno));
    return true;
  }
  const int logInterval = 60;
  int64_t lastLogged = rt.tv_sec;
  int sent = 0;
  int lastSent = 0;
  bool vsyncEnabled = false;

  struct timespec wait_time;
  wait_time.tv_sec = 0;
  wait_time.tv_nsec = mDisplay.mVsyncPeriod;
  const int64_t kOneRefreshNs = mDisplay.mVsyncPeriod;
  const int64_t kOneSecondNs = 1000ULL * 1000ULL * 1000ULL;
  int64_t lastTimeNs = -1;
  int64_t phasedWaitNs = 0;
  int64_t currentNs = 0;

  while (true) {
    clock_gettime(CLOCK_MONOTONIC, &rt);
    currentNs = rt.tv_nsec + rt.tv_sec * kOneSecondNs;

    if (lastTimeNs < 0) {
      phasedWaitNs = currentNs + kOneRefreshNs;
    } else {
      phasedWaitNs =
          kOneRefreshNs * ((currentNs - lastTimeNs) / kOneRefreshNs + 1) +
          lastTimeNs;
    }

    wait_time.tv_sec = phasedWaitNs / kOneSecondNs;
    wait_time.tv_nsec = phasedWaitNs - wait_time.tv_sec * kOneSecondNs;

    int ret;
    do {
      ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wait_time, NULL);
    } while (ret == -1 && errno == EINTR);

    lastTimeNs = phasedWaitNs;

    std::unique_lock<std::recursive_mutex> lock(mDisplay.mStateMutex);
    vsyncEnabled = (mDisplay.mVsyncEnabled == HWC2::Vsync::Enable);
    lock.unlock();

    if (!vsyncEnabled) {
      continue;
    }

    lock.lock();
    const auto& callbackInfo =
        mDisplay.mDevice.mCallbacks[HWC2::Callback::Vsync];
    auto vsync = reinterpret_cast<HWC2_PFN_VSYNC>(callbackInfo.pointer);
    lock.unlock();

    if (vsync) {
      DEBUG_LOG("%s: display:%" PRIu64 " calling vsync", __FUNCTION__,
                mDisplay.mId);
      vsync(callbackInfo.data, mDisplay.mId, lastTimeNs);
    }

    int64_t lastSentInterval = rt.tv_sec - lastLogged;
    if (lastSentInterval >= logInterval) {
      DEBUG_LOG("sent %d syncs in %" PRId64 "s", sent - lastSent,
                lastSentInterval);
      lastLogged = rt.tv_sec;
      lastSent = sent;
    }
    ++sent;
  }
  return false;
}

}  // namespace android
