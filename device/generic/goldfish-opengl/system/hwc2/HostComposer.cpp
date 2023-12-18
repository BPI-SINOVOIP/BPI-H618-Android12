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

#include "HostComposer.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <drm/virtgpu_drm.h>
#include <poll.h>
#include <sync/sync.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include "../egl/goldfish_sync.h"
#include "Device.h"
#include "Display.h"

namespace android {
namespace {

static int getVsyncHzFromProperty() {
  static constexpr const auto kVsyncProp = "ro.boot.qemu.vsync";

  const auto vsyncProp = android::base::GetProperty(kVsyncProp, "");
  DEBUG_LOG("%s: prop value is: %s", __FUNCTION__, vsyncProp.c_str());

  uint64_t vsyncPeriod;
  if (!android::base::ParseUint(vsyncProp, &vsyncPeriod)) {
    ALOGE("%s: failed to parse vsync period '%s', returning default 60",
          __FUNCTION__, vsyncProp.c_str());
    return 60;
  }

  return static_cast<int>(vsyncPeriod);
}

static bool isMinigbmFromProperty() {
  static constexpr const auto kGrallocProp = "ro.hardware.gralloc";

  const auto grallocProp = android::base::GetProperty(kGrallocProp, "");
  DEBUG_LOG("%s: prop value is: %s", __FUNCTION__, grallocProp.c_str());

  if (grallocProp == "minigbm") {
    ALOGD("%s: Using minigbm, in minigbm mode.\n", __FUNCTION__);
    return true;
  } else {
    ALOGD("%s: Is not using minigbm, in goldfish mode.\n", __FUNCTION__);
    return false;
  }
}

#define DEFINE_AND_VALIDATE_HOST_CONNECTION                                   \
  HostConnection* hostCon = createOrGetHostConnection();                      \
  if (!hostCon) {                                                             \
    ALOGE("%s: Failed to get host connection\n", __FUNCTION__);               \
    return HWC2::Error::NoResources;                                          \
  }                                                                           \
  ExtendedRCEncoderContext* rcEnc = hostCon->rcEncoder();                     \
  if (!rcEnc) {                                                               \
    ALOGE("%s: Failed to get renderControl encoder context\n", __FUNCTION__); \
    return HWC2::Error::NoResources;                                          \
  }

static std::unique_ptr<HostConnection> sHostCon;

static HostConnection* createOrGetHostConnection() {
  if (!sHostCon) {
    sHostCon = HostConnection::createUnique();
  }
  return sHostCon.get();
}

typedef struct compose_layer {
  uint32_t cbHandle;
  hwc2_composition_t composeMode;
  hwc_rect_t displayFrame;
  hwc_frect_t crop;
  int32_t blendMode;
  float alpha;
  hwc_color_t color;
  hwc_transform_t transform;
} ComposeLayer;

typedef struct compose_device {
  uint32_t version;
  uint32_t targetHandle;
  uint32_t numLayers;
  struct compose_layer layer[0];
} ComposeDevice;

typedef struct compose_device_v2 {
  uint32_t version;
  uint32_t displayId;
  uint32_t targetHandle;
  uint32_t numLayers;
  struct compose_layer layer[0];
} ComposeDevice_v2;

class ComposeMsg {
 public:
  ComposeMsg(uint32_t layerCnt = 0)
      : mData(sizeof(ComposeDevice) + layerCnt * sizeof(ComposeLayer)) {
    mComposeDevice = reinterpret_cast<ComposeDevice*>(mData.data());
    mLayerCnt = layerCnt;
  }

  ComposeDevice* get() { return mComposeDevice; }

  uint32_t getLayerCnt() { return mLayerCnt; }

 private:
  std::vector<uint8_t> mData;
  uint32_t mLayerCnt;
  ComposeDevice* mComposeDevice;
};

class ComposeMsg_v2 {
 public:
  ComposeMsg_v2(uint32_t layerCnt = 0)
      : mData(sizeof(ComposeDevice_v2) + layerCnt * sizeof(ComposeLayer)) {
    mComposeDevice = reinterpret_cast<ComposeDevice_v2*>(mData.data());
    mLayerCnt = layerCnt;
  }

  ComposeDevice_v2* get() { return mComposeDevice; }

  uint32_t getLayerCnt() { return mLayerCnt; }

 private:
  std::vector<uint8_t> mData;
  uint32_t mLayerCnt;
  ComposeDevice_v2* mComposeDevice;
};

const native_handle_t* AllocateDisplayColorBuffer(int width, int height) {
  const uint32_t layerCount = 1;
  const uint64_t graphicBufferId = 0;  // not used
  buffer_handle_t h;
  uint32_t stride;

  if (GraphicBufferAllocator::get().allocate(
          width, height, PIXEL_FORMAT_RGBA_8888, layerCount,
          (GraphicBuffer::USAGE_HW_COMPOSER | GraphicBuffer::USAGE_HW_RENDER),
          &h, &stride, graphicBufferId, "EmuHWC2") == OK) {
    return static_cast<const native_handle_t*>(h);
  } else {
    return nullptr;
  }
}

void FreeDisplayColorBuffer(const native_handle_t* h) {
  GraphicBufferAllocator::get().free(h);
}

}  // namespace

HWC2::Error HostComposer::init(const HotplugCallback& cb) {
  mIsMinigbm = isMinigbmFromProperty();
  if (mIsMinigbm) {
    if (!mDrmPresenter.init(cb)) {
      ALOGE("%s: failed to initialize DrmPresenter", __FUNCTION__);
      return HWC2::Error::NoResources;
    }
  } else {
    mSyncDeviceFd = goldfish_sync_open();
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::createDisplays(
    Device* device, const AddDisplayToDeviceFunction& addDisplayToDeviceFn) {
  HWC2::Error error = HWC2::Error::None;

  error = createPrimaryDisplay(device, addDisplayToDeviceFn);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to create primary display", __FUNCTION__);
    return error;
  }

  error = createSecondaryDisplays(device, addDisplayToDeviceFn);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to create secondary displays", __FUNCTION__);
    return error;
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::createPrimaryDisplay(
    Device* device, const AddDisplayToDeviceFunction& addDisplayToDeviceFn) {
  HWC2::Error error = HWC2::Error::None;

  DEFINE_AND_VALIDATE_HOST_CONNECTION
  hostCon->lock();
  int width = rcEnc->rcGetFBParam(rcEnc, FB_WIDTH);
  int height = rcEnc->rcGetFBParam(rcEnc, FB_HEIGHT);
  int dpiX = rcEnc->rcGetFBParam(rcEnc, FB_XDPI);
  int dpiY = rcEnc->rcGetFBParam(rcEnc, FB_YDPI);
  hostCon->unlock();

  int refreshRateHz = getVsyncHzFromProperty();

  auto display = std::make_unique<Display>(*device, this, 0);
  if (display == nullptr) {
    ALOGE("%s failed to allocate display", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  auto displayId = display->getId();

  error = display->init(width, height, dpiX, dpiY, refreshRateHz);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to initialize display:%" PRIu64, __FUNCTION__, displayId);
    return error;
  }

  error = createHostComposerDisplayInfo(display.get(), /*hostDisplayId=*/0);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to initialize host info for display:%" PRIu64,
          __FUNCTION__, displayId);
    return error;
  }

  error = addDisplayToDeviceFn(std::move(display));
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to add display:%" PRIu64, __FUNCTION__, displayId);
    return error;
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::createDisplay(
    Device* device, uint32_t displayId, uint32_t width, uint32_t height,
    uint32_t dpiX, uint32_t dpiY, uint32_t refreshRateHz,
    const AddDisplayToDeviceFunction& addDisplayToDeviceFn) {
  HWC2::Error error;
  Display* display = device->getDisplay(displayId);
  if (display) {
    ALOGD("%s display %d already existed, then update", __func__, displayId);
  }

  DEFINE_AND_VALIDATE_HOST_CONNECTION
  hostCon->lock();
  if (rcEnc->rcCreateDisplayById(rcEnc, displayId)) {
    ALOGE("%s host failed to create display %" PRIu32, __func__, displayId);
    hostCon->unlock();
    return HWC2::Error::NoResources;
  }
  if (rcEnc->rcSetDisplayPoseDpi(rcEnc, displayId, -1, -1, width, height, dpiX/1000)) {
    ALOGE("%s host failed to set display %" PRIu32, __func__, displayId);
    hostCon->unlock();
    return HWC2::Error::NoResources;
  }
  hostCon->unlock();

  std::optional<std::vector<uint8_t>> edid;
  if (mIsMinigbm) {
    edid = mDrmPresenter.getEdid(displayId);
  }
  if (!display) {
    auto newDisplay = std::make_unique<Display>(*device, this, displayId);
    if (newDisplay == nullptr) {
      ALOGE("%s failed to allocate display", __FUNCTION__);
      return HWC2::Error::NoResources;
    }


    error = newDisplay->init(width, height, dpiX, dpiY, refreshRateHz, edid);
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to initialize display:%" PRIu32, __FUNCTION__,
            displayId);
      return error;
    }

    error =
        createHostComposerDisplayInfo(newDisplay.get(), displayId);
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to initialize host info for display:%" PRIu32,
            __FUNCTION__, displayId);
      return error;
    }

    error = addDisplayToDeviceFn(std::move(newDisplay));
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to add display:%" PRIu32, __FUNCTION__, displayId);
      return error;
    }
  } else {
    display->lock();
    // update display parameters
    error = display->updateParameters(width, height, dpiX, dpiY,
                                      refreshRateHz, edid);
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to update display:%" PRIu32, __FUNCTION__, displayId);
      display->unlock();
      return error;
    }

    error = createHostComposerDisplayInfo(display, displayId);
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to initialize host info for display:%" PRIu32,
            __FUNCTION__, displayId);
      display->unlock();
      return error;
    }
    display->unlock();
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::createSecondaryDisplays(
    Device* device, const AddDisplayToDeviceFunction& addDisplayToDeviceFn) {
  HWC2::Error error = HWC2::Error::None;

  static constexpr const char kExternalDisplayProp[] =
      "hwservicemanager.external.displays";

  const auto propString = android::base::GetProperty(kExternalDisplayProp, "");
  DEBUG_LOG("%s: prop value is: %s", __FUNCTION__, propString.c_str());

  if (propString.empty()) {
    return HWC2::Error::None;
  }

  const std::vector<std::string> propStringParts =
      android::base::Split(propString, ",");
  if (propStringParts.size() % 5 != 0) {
    ALOGE("%s: Invalid syntax for system prop %s which is %s", __FUNCTION__,
          kExternalDisplayProp, propString.c_str());
    return HWC2::Error::BadParameter;
  }

  std::vector<int> propIntParts;
  for (const std::string& propStringPart : propStringParts) {
    uint64_t propUintPart;
    if (!android::base::ParseUint(propStringPart, &propUintPart)) {
      ALOGE("%s: Invalid syntax for system prop %s which is %s", __FUNCTION__,
            kExternalDisplayProp, propString.c_str());
      return HWC2::Error::BadParameter;
    }
    propIntParts.push_back(static_cast<int>(propUintPart));
  }

  static constexpr const uint32_t kHostDisplayIdStart = 6;

  uint32_t secondaryDisplayIndex = 1;
  while (!propIntParts.empty()) {
    int width = propIntParts[1];
    int height = propIntParts[2];
    int dpiX = propIntParts[3];
    int dpiY = propIntParts[3];
    int refreshRateHz = 160;

    propIntParts.erase(propIntParts.begin(), propIntParts.begin() + 5);

    uint32_t expectedHostDisplayId =
        kHostDisplayIdStart + secondaryDisplayIndex - 1;
    uint32_t actualHostDisplayId = 0;

    DEFINE_AND_VALIDATE_HOST_CONNECTION
    hostCon->lock();
    rcEnc->rcDestroyDisplay(rcEnc, expectedHostDisplayId);
    rcEnc->rcCreateDisplay(rcEnc, &actualHostDisplayId);
    rcEnc->rcSetDisplayPose(rcEnc, actualHostDisplayId, -1, -1, width, height);
    hostCon->unlock();

    if (actualHostDisplayId != expectedHostDisplayId) {
      ALOGE(
          "Something wrong with host displayId allocation, expected %d "
          "but received %d",
          expectedHostDisplayId, actualHostDisplayId);
    }

    auto display =
        std::make_unique<Display>(*device, this, secondaryDisplayIndex++);
    if (display == nullptr) {
      ALOGE("%s failed to allocate display", __FUNCTION__);
      return HWC2::Error::NoResources;
    }

    auto displayId = display->getId();

    error = display->init(width, height, dpiX, dpiY, refreshRateHz);
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to initialize display:%" PRIu64, __FUNCTION__,
            displayId);
      return error;
    }

    error = createHostComposerDisplayInfo(display.get(), actualHostDisplayId);
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to initialize host info for display:%" PRIu64,
            __FUNCTION__, displayId);
      return error;
    }

    error = addDisplayToDeviceFn(std::move(display));
    if (error != HWC2::Error::None) {
      ALOGE("%s failed to add display:%" PRIu64, __FUNCTION__, displayId);
      return error;
    }
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::createHostComposerDisplayInfo(
    Display* display, uint32_t hostDisplayId) {
  HWC2::Error error = HWC2::Error::None;

  hwc2_display_t displayId = display->getId();
  hwc2_config_t displayConfigId;
  int32_t displayWidth;
  int32_t displayHeight;

  error = display->getActiveConfig(&displayConfigId);
  if (error != HWC2::Error::None) {
    ALOGE("%s: display:%" PRIu64 " has no active config", __FUNCTION__,
          displayId);
    return error;
  }

  error = display->getDisplayAttributeEnum(
      displayConfigId, HWC2::Attribute::Width, &displayWidth);
  if (error != HWC2::Error::None) {
    ALOGE("%s: display:%" PRIu64 " failed to get width", __FUNCTION__,
          displayId);
    return error;
  }

  error = display->getDisplayAttributeEnum(
      displayConfigId, HWC2::Attribute::Height, &displayHeight);
  if (error != HWC2::Error::None) {
    ALOGE("%s: display:%" PRIu64 " failed to get height", __FUNCTION__,
          displayId);
    return error;
  }

  auto it = mDisplayInfos.find(displayId);
  if (it != mDisplayInfos.end()) {
    ALOGE("%s: display:%" PRIu64 " already created?", __FUNCTION__, displayId);
  }

  HostComposerDisplayInfo& displayInfo = mDisplayInfos[displayId];

  displayInfo.hostDisplayId = hostDisplayId;

  displayInfo.compositionResultBuffer =
      AllocateDisplayColorBuffer(displayWidth, displayHeight);
  if (displayInfo.compositionResultBuffer == nullptr) {
    ALOGE("%s: display:%" PRIu64 " failed to create target buffer",
          __FUNCTION__, displayId);
    return HWC2::Error::NoResources;
  }

  if (mIsMinigbm) {
    displayInfo.compositionResultDrmBuffer.reset(
        new DrmBuffer(displayInfo.compositionResultBuffer, mDrmPresenter));

    uint32_t vsyncPeriod = 1000 * 1000 * 1000 / mDrmPresenter.refreshRate();
    error = display->setVsyncPeriod(vsyncPeriod);
    if (error != HWC2::Error::None) {
      ALOGE("%s: display:%" PRIu64 " failed to set vsync height", __FUNCTION__,
            displayId);
      return error;
    }
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::onDisplayDestroy(Display* display) {
  hwc2_display_t displayId = display->getId();

  auto it = mDisplayInfos.find(displayId);
  if (it == mDisplayInfos.end()) {
    ALOGE("%s: display:%" PRIu64 " missing display buffers?", __FUNCTION__,
          displayId);
    return HWC2::Error::BadDisplay;
  }

  HostComposerDisplayInfo& displayInfo = mDisplayInfos[displayId];

  if (displayId != 0) {
    DEFINE_AND_VALIDATE_HOST_CONNECTION
    hostCon->lock();
    rcEnc->rcDestroyDisplay(rcEnc, displayInfo.hostDisplayId);
    hostCon->unlock();
  }

  FreeDisplayColorBuffer(displayInfo.compositionResultBuffer);

  mDisplayInfos.erase(it);

  return HWC2::Error::None;
}

HWC2::Error HostComposer::onDisplayClientTargetSet(Display* display) {
  hwc2_display_t displayId = display->getId();

  auto it = mDisplayInfos.find(displayId);
  if (it == mDisplayInfos.end()) {
    ALOGE("%s: display:%" PRIu64 " missing display buffers?", __FUNCTION__,
          displayId);
    return HWC2::Error::BadDisplay;
  }

  HostComposerDisplayInfo& displayInfo = mDisplayInfos[displayId];

  if (mIsMinigbm) {
    FencedBuffer& clientTargetFencedBuffer = display->getClientTarget();

    displayInfo.clientTargetDrmBuffer.reset(
        new DrmBuffer(clientTargetFencedBuffer.getBuffer(), mDrmPresenter));
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::validateDisplay(
    Display* display, std::unordered_map<hwc2_layer_t, HWC2::Composition>*
                          layerCompositionChanges) {
  DEFINE_AND_VALIDATE_HOST_CONNECTION
  hostCon->lock();
  bool hostCompositionV1 = rcEnc->hasHostCompositionV1();
  bool hostCompositionV2 = rcEnc->hasHostCompositionV2();
  hostCon->unlock();

  const std::vector<Layer*> layers = display->getOrderedLayers();

  if (hostCompositionV1 || hostCompositionV2) {
    // Support Device and SolidColor, otherwise, fallback all layers to Client.
    bool fallBack = false;
    // TODO: use local var compositiontype, avoid call getCompositionType() many
    // times
    for (auto& layer : layers) {
      if (layer->getCompositionType() == HWC2::Composition::Invalid) {
        // Log error for unused layers, layer leak?
        ALOGE("%s layer %u CompositionType(%d) not set", __FUNCTION__,
              (uint32_t)layer->getId(), layer->getCompositionType());
        continue;
      }
      if (layer->getCompositionType() == HWC2::Composition::Client ||
          layer->getCompositionType() == HWC2::Composition::Cursor ||
          layer->getCompositionType() == HWC2::Composition::Sideband) {
        ALOGW("%s: layer %u CompositionType %d, fallback", __FUNCTION__,
              (uint32_t)layer->getId(), layer->getCompositionType());
        fallBack = true;
        break;
      }
    }

    if (display->hasColorTransform()) {
      fallBack = true;
    }

    if (fallBack) {
      for (auto& layer : layers) {
        if (layer->getCompositionType() == HWC2::Composition::Invalid) {
          continue;
        }
        if (layer->getCompositionType() != HWC2::Composition::Client) {
          (*layerCompositionChanges)[layer->getId()] =
              HWC2::Composition::Client;
        }
      }
    }
  } else {
    for (auto& layer : layers) {
      if (layer->getCompositionType() != HWC2::Composition::Client) {
        (*layerCompositionChanges)[layer->getId()] = HWC2::Composition::Client;
      }
    }
  }

  return HWC2::Error::None;
}

HWC2::Error HostComposer::presentDisplay(Display* display,
                                         int32_t* outRetireFence) {
  auto it = mDisplayInfos.find(display->getId());
  if (it == mDisplayInfos.end()) {
    ALOGE("%s: failed to find display buffers for display:%" PRIu64,
          __FUNCTION__, display->getId());
    return HWC2::Error::BadDisplay;
  }

  HostComposerDisplayInfo& displayInfo = it->second;

  DEFINE_AND_VALIDATE_HOST_CONNECTION
  hostCon->lock();
  bool hostCompositionV1 = rcEnc->hasHostCompositionV1();
  bool hostCompositionV2 = rcEnc->hasHostCompositionV2();
  hostCon->unlock();

  // Ff we supports v2, then discard v1
  if (hostCompositionV2) {
    hostCompositionV1 = false;
  }

  const std::vector<Layer*> layers = display->getOrderedLayers();
  if (hostCompositionV2 || hostCompositionV1) {
    uint32_t numLayer = 0;
    for (auto layer : layers) {
      if (layer->getCompositionType() == HWC2::Composition::Device ||
          layer->getCompositionType() == HWC2::Composition::SolidColor) {
        numLayer++;
      }
    }

    DEBUG_LOG("%s: presenting display:%" PRIu64 " with %d layers", __FUNCTION__,
              display->getId(), static_cast<int>(layers.size()));

    display->clearReleaseFencesAndIdsLocked();

    if (numLayer == 0) {
      ALOGW(
          "%s display has no layers to compose, flushing client target buffer.",
          __FUNCTION__);

      FencedBuffer& displayClientTarget = display->getClientTarget();
      if (displayClientTarget.getBuffer() != nullptr) {
        if (mIsMinigbm) {
          int retireFence;
          displayInfo.clientTargetDrmBuffer->flushToDisplay(display->getId(),
                                                            &retireFence);
          *outRetireFence = dup(retireFence);
          close(retireFence);
        } else {
          post(hostCon, rcEnc, displayClientTarget.getBuffer());
          *outRetireFence = displayClientTarget.getFence();
        }
      }
      return HWC2::Error::None;
    }

    std::unique_ptr<ComposeMsg> composeMsg;
    std::unique_ptr<ComposeMsg_v2> composeMsgV2;

    if (hostCompositionV1) {
      composeMsg.reset(new ComposeMsg(numLayer));
    } else {
      composeMsgV2.reset(new ComposeMsg_v2(numLayer));
    }

    // Handle the composition
    ComposeDevice* p;
    ComposeDevice_v2* p2;
    ComposeLayer* l;

    if (hostCompositionV1) {
      p = composeMsg->get();
      l = p->layer;
    } else {
      p2 = composeMsgV2->get();
      l = p2->layer;
    }

    int releaseLayersCount = 0;
    for (auto layer : layers) {
      // TODO: use local var composisitonType to store getCompositionType()
      if (layer->getCompositionType() != HWC2::Composition::Device &&
          layer->getCompositionType() != HWC2::Composition::SolidColor) {
        ALOGE("%s: Unsupported composition types %d layer %u", __FUNCTION__,
              layer->getCompositionType(), (uint32_t)layer->getId());
        continue;
      }
      // send layer composition command to host
      if (layer->getCompositionType() == HWC2::Composition::Device) {
        display->addReleaseLayerLocked(layer->getId());
        releaseLayersCount++;

        int fence = layer->getBuffer().getFence();
        if (fence != -1) {
          int err = sync_wait(fence, 3000);
          if (err < 0 && errno == ETIME) {
            ALOGE("%s waited on fence %d for 3000 ms", __FUNCTION__, fence);
          }
          close(fence);
        } else {
          ALOGV("%s: acquire fence not set for layer %u", __FUNCTION__,
                (uint32_t)layer->getId());
        }
        const native_handle_t* cb = layer->getBuffer().getBuffer();
        if (cb != nullptr) {
          l->cbHandle = hostCon->grallocHelper()->getHostHandle(cb);
        } else {
          ALOGE("%s null buffer for layer %d", __FUNCTION__,
                (uint32_t)layer->getId());
        }
      } else {
        // solidcolor has no buffer
        l->cbHandle = 0;
      }
      l->composeMode = (hwc2_composition_t)layer->getCompositionType();
      l->displayFrame = layer->getDisplayFrame();
      l->crop = layer->getSourceCrop();
      l->blendMode = static_cast<int32_t>(layer->getBlendMode());
      l->alpha = layer->getPlaneAlpha();
      l->color = layer->getColor();
      l->transform = layer->getTransform();
      ALOGV(
          "   cb %d blendmode %d alpha %f %d %d %d %d z %d"
          " composeMode %d, transform %d",
          l->cbHandle, l->blendMode, l->alpha, l->displayFrame.left,
          l->displayFrame.top, l->displayFrame.right, l->displayFrame.bottom,
          layer->getZ(), l->composeMode, l->transform);
      l++;
    }
    if (hostCompositionV1) {
      p->version = 1;
      p->targetHandle = hostCon->grallocHelper()->getHostHandle(
          displayInfo.compositionResultBuffer);
      p->numLayers = numLayer;
    } else {
      p2->version = 2;
      p2->displayId = displayInfo.hostDisplayId;
      p2->targetHandle = hostCon->grallocHelper()->getHostHandle(
          displayInfo.compositionResultBuffer);
      p2->numLayers = numLayer;
    }

    hostCon->lock();
    if (rcEnc->hasAsyncFrameCommands()) {
      if (mIsMinigbm) {
        if (hostCompositionV1) {
          rcEnc->rcComposeAsyncWithoutPost(
              rcEnc, sizeof(ComposeDevice) + numLayer * sizeof(ComposeLayer),
              (void*)p);
        } else {
          rcEnc->rcComposeAsyncWithoutPost(
              rcEnc, sizeof(ComposeDevice_v2) + numLayer * sizeof(ComposeLayer),
              (void*)p2);
        }
      } else {
        if (hostCompositionV1) {
          rcEnc->rcComposeAsync(
              rcEnc, sizeof(ComposeDevice) + numLayer * sizeof(ComposeLayer),
              (void*)p);
        } else {
          rcEnc->rcComposeAsync(
              rcEnc, sizeof(ComposeDevice_v2) + numLayer * sizeof(ComposeLayer),
              (void*)p2);
        }
      }
    } else {
      if (mIsMinigbm) {
        if (hostCompositionV1) {
          rcEnc->rcComposeWithoutPost(
              rcEnc, sizeof(ComposeDevice) + numLayer * sizeof(ComposeLayer),
              (void*)p);
        } else {
          rcEnc->rcComposeWithoutPost(
              rcEnc, sizeof(ComposeDevice_v2) + numLayer * sizeof(ComposeLayer),
              (void*)p2);
        }
      } else {
        if (hostCompositionV1) {
          rcEnc->rcCompose(
              rcEnc, sizeof(ComposeDevice) + numLayer * sizeof(ComposeLayer),
              (void*)p);
        } else {
          rcEnc->rcCompose(
              rcEnc, sizeof(ComposeDevice_v2) + numLayer * sizeof(ComposeLayer),
              (void*)p2);
        }
      }
    }

    hostCon->unlock();

    // Send a retire fence and use it as the release fence for all layers,
    // since media expects it
    EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_ANDROID,
                        EGL_NO_NATIVE_FENCE_FD_ANDROID};

    uint64_t sync_handle, thread_handle;
    int retire_fd;

    hostCon->lock();
    rcEnc->rcCreateSyncKHR(rcEnc, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs,
                           2 * sizeof(EGLint), true /* destroy when signaled */,
                           &sync_handle, &thread_handle);
    hostCon->unlock();

    if (mIsMinigbm) {
      displayInfo.compositionResultDrmBuffer->flushToDisplay(display->getId(),
                                                             &retire_fd);
    } else {
      goldfish_sync_queue_work(mSyncDeviceFd, sync_handle, thread_handle,
                               &retire_fd);
    }

    for (size_t i = 0; i < releaseLayersCount; ++i) {
      display->addReleaseFenceLocked(dup(retire_fd));
    }

    *outRetireFence = dup(retire_fd);
    close(retire_fd);
    hostCon->lock();
    if (rcEnc->hasAsyncFrameCommands()) {
      rcEnc->rcDestroySyncKHRAsync(rcEnc, sync_handle);
    } else {
      rcEnc->rcDestroySyncKHR(rcEnc, sync_handle);
    }
    hostCon->unlock();

  } else {
    // we set all layers Composition::Client, so do nothing.
    if (mIsMinigbm) {
      int retireFence;
      displayInfo.clientTargetDrmBuffer->flushToDisplay(display->getId(),
                                                        &retireFence);
      *outRetireFence = dup(retireFence);
      close(retireFence);
    } else {
      FencedBuffer& displayClientTarget = display->getClientTarget();
      post(hostCon, rcEnc, displayClientTarget.getBuffer());
      *outRetireFence = displayClientTarget.getFence();
    }
    ALOGV("%s fallback to post, returns outRetireFence %d", __FUNCTION__,
          *outRetireFence);
  }

  return HWC2::Error::None;
}

void HostComposer::post(HostConnection* hostCon,
                        ExtendedRCEncoderContext* rcEnc, buffer_handle_t h) {
  assert(cb && "native_handle_t::from(h) failed");

  hostCon->lock();
  rcEnc->rcFBPost(rcEnc, hostCon->grallocHelper()->getHostHandle(h));
  hostCon->flush();
  hostCon->unlock();
}

}  // namespace android
