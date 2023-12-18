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

#include "GuestComposer.h"

#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <device_config_shared.h>
#include <drm_fourcc.h>
#include <libyuv.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include "Device.h"
#include "Display.h"
#include "Drm.h"
#include "Layer.h"

namespace android {
namespace {

uint64_t AlignToPower2(uint64_t val, uint8_t align_log) {
  uint64_t align = 1ULL << align_log;
  return ((val + (align - 1)) / align) * align;
}

bool LayerNeedsScaling(const Layer& layer) {
  hwc_rect_t crop = layer.getSourceCropInt();
  hwc_rect_t frame = layer.getDisplayFrame();

  int fromW = crop.right - crop.left;
  int fromH = crop.bottom - crop.top;
  int toW = frame.right - frame.left;
  int toH = frame.bottom - frame.top;

  bool not_rot_scale = fromW != toW || fromH != toH;
  bool rot_scale = fromW != toH || fromH != toW;

  bool needs_rot = layer.getTransform() & HAL_TRANSFORM_ROT_90;

  return needs_rot ? rot_scale : not_rot_scale;
}

bool LayerNeedsBlending(const Layer& layer) {
  return layer.getBlendMode() != HWC2::BlendMode::None;
}

bool LayerNeedsAttenuation(const Layer& layer) {
  return layer.getBlendMode() == HWC2::BlendMode::Coverage;
}

struct BufferSpec;
typedef int (*ConverterFunction)(const BufferSpec& src, const BufferSpec& dst,
                                 bool v_flip);
int DoCopy(const BufferSpec& src, const BufferSpec& dst, bool vFlip);
int ConvertFromRGB565(const BufferSpec& src, const BufferSpec& dst, bool vFlip);
int ConvertFromYV12(const BufferSpec& src, const BufferSpec& dst, bool vFlip);

ConverterFunction GetConverterForDrmFormat(uint32_t drmFormat) {
  switch (drmFormat) {
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_XBGR8888:
      return &DoCopy;
    case DRM_FORMAT_RGB565:
      return &ConvertFromRGB565;
    case DRM_FORMAT_YVU420:
      return &ConvertFromYV12;
  }
  ALOGW("Unsupported drm format: %d(%s), returning null converter", drmFormat,
        GetDrmFormatString(drmFormat));
  return nullptr;
}

bool IsDrmFormatSupported(uint32_t drmFormat) {
  return GetConverterForDrmFormat(drmFormat) != nullptr;
}

// Libyuv's convert functions only allow the combination of any rotation
// (multiple of 90 degrees) and a vertical flip, but not horizontal flips.
// Surfaceflinger's transformations are expressed in terms of a vertical flip,
// a horizontal flip and/or a single 90 degrees clockwise rotation (see
// NATIVE_WINDOW_TRANSFORM_HINT documentation on system/window.h for more
// insight). The following code allows to turn a horizontal flip into a 180
// degrees rotation and a vertical flip.
libyuv::RotationMode GetRotationFromTransform(uint32_t transform) {
  uint32_t rotation =
      (transform & HAL_TRANSFORM_ROT_90) ? 1 : 0;          // 1 * ROT90 bit
  rotation += (transform & HAL_TRANSFORM_FLIP_H) ? 2 : 0;  // 2 * VFLIP bit
  return static_cast<libyuv::RotationMode>(90 * rotation);
}

bool GetVFlipFromTransform(uint32_t transform) {
  // vertical flip xor horizontal flip
  return ((transform & HAL_TRANSFORM_FLIP_V) >> 1) ^
         (transform & HAL_TRANSFORM_FLIP_H);
}

struct BufferSpec {
  uint8_t* buffer;
  std::optional<android_ycbcr> buffer_ycbcr;
  int width;
  int height;
  int cropX;
  int cropY;
  int cropWidth;
  int cropHeight;
  uint32_t drmFormat;
  int strideBytes;
  int sampleBytes;

  BufferSpec(uint8_t* buffer, std::optional<android_ycbcr> buffer_ycbcr,
             int width, int height, int cropX, int cropY, int cropWidth,
             int cropHeight, uint32_t drmFormat, int strideBytes,
             int sampleBytes)
      : buffer(buffer),
        buffer_ycbcr(buffer_ycbcr),
        width(width),
        height(height),
        cropX(cropX),
        cropY(cropY),
        cropWidth(cropWidth),
        cropHeight(cropHeight),
        drmFormat(drmFormat),
        strideBytes(strideBytes),
        sampleBytes(sampleBytes) {}

  BufferSpec(uint8_t* buffer, int width, int height, int strideBytes)
      : BufferSpec(buffer,
                   /*buffer_ycbcr=*/std::nullopt, width, height,
                   /*cropX=*/0,
                   /*cropY=*/0,
                   /*cropWidth=*/width,
                   /*cropHeight=*/height,
                   /*drmFormat=*/DRM_FORMAT_ABGR8888, strideBytes,
                   /*sampleBytes=*/4) {}
};

int ConvertFromRGB565(const BufferSpec& src, const BufferSpec& dst,
                      bool vFlip) {
  // Point to the upper left corner of the crop rectangle
  uint8_t* srcBuffer =
      src.buffer + src.cropY * src.strideBytes + src.cropX * src.sampleBytes;
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;

  int width = src.cropWidth;
  int height = src.cropHeight;
  if (vFlip) {
    height = -height;
  }

  return libyuv::RGB565ToARGB(srcBuffer, src.strideBytes,  //
                              dstBuffer, dst.strideBytes,  //
                              width, height);
}

int ConvertFromYV12(const BufferSpec& src, const BufferSpec& dst, bool vFlip) {
  // The following calculation of plane offsets and alignments are based on
  // swiftshader's Sampler::setTextureLevel() implementation
  // (Renderer/Sampler.cpp:225)

  auto& srcBufferYCbCrOpt = src.buffer_ycbcr;
  if (!srcBufferYCbCrOpt) {
    ALOGE("%s called on non ycbcr buffer", __FUNCTION__);
    return -1;
  }
  auto& srcBufferYCbCr = *srcBufferYCbCrOpt;

  // The libyuv::I420ToARGB() function is for tri-planar.
  if (srcBufferYCbCr.chroma_step != 1) {
    ALOGE("%s called with bad chroma step", __FUNCTION__);
    return -1;
  }

  uint8_t* srcY = reinterpret_cast<uint8_t*>(srcBufferYCbCr.y);
  int strideY = srcBufferYCbCr.ystride;
  uint8_t* srcU = reinterpret_cast<uint8_t*>(srcBufferYCbCr.cb);
  int strideU = srcBufferYCbCr.cstride;
  uint8_t* srcV = reinterpret_cast<uint8_t*>(srcBufferYCbCr.cr);
  int strideV = srcBufferYCbCr.cstride;

  // Adjust for crop
  srcY += src.cropY * strideY + src.cropX;
  srcV += (src.cropY / 2) * strideV + (src.cropX / 2);
  srcU += (src.cropY / 2) * strideU + (src.cropX / 2);
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;

  int width = dst.cropWidth;
  int height = dst.cropHeight;

  if (vFlip) {
    height = -height;
  }

  // YV12 is the same as I420, with the U and V planes swapped
  return libyuv::I420ToARGB(srcY, strideY, srcV, strideV, srcU, strideU,
                            dstBuffer, dst.strideBytes, width, height);
}

int DoConversion(const BufferSpec& src, const BufferSpec& dst, bool v_flip) {
  ConverterFunction func = GetConverterForDrmFormat(src.drmFormat);
  if (!func) {
    // GetConverterForDrmFormat should've logged the issue for us.
    return -1;
  }
  return func(src, dst, v_flip);
}

int DoCopy(const BufferSpec& src, const BufferSpec& dst, bool v_flip) {
  // Point to the upper left corner of the crop rectangle
  uint8_t* srcBuffer =
      src.buffer + src.cropY * src.strideBytes + src.cropX * src.sampleBytes;
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;
  int width = src.cropWidth;
  int height = src.cropHeight;

  if (v_flip) {
    height = -height;
  }

  // HAL formats are named based on the order of the pixel components on the
  // byte stream, while libyuv formats are named based on the order of those
  // pixel components in an integer written from left to right. So
  // libyuv::FOURCC_ARGB is equivalent to HAL_PIXEL_FORMAT_BGRA_8888.
  auto ret = libyuv::ARGBCopy(srcBuffer, src.strideBytes, dstBuffer,
                              dst.strideBytes, width, height);
  return ret;
}

int DoRotation(const BufferSpec& src, const BufferSpec& dst,
               libyuv::RotationMode rotation, bool v_flip) {
  // Point to the upper left corner of the crop rectangles
  uint8_t* srcBuffer =
      src.buffer + src.cropY * src.strideBytes + src.cropX * src.sampleBytes;
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;
  int width = src.cropWidth;
  int height = src.cropHeight;

  if (v_flip) {
    height = -height;
  }

  return libyuv::ARGBRotate(srcBuffer, src.strideBytes, dstBuffer,
                            dst.strideBytes, width, height, rotation);
}

int DoScaling(const BufferSpec& src, const BufferSpec& dst, bool v_flip) {
  // Point to the upper left corner of the crop rectangles
  uint8_t* srcBuffer =
      src.buffer + src.cropY * src.strideBytes + src.cropX * src.sampleBytes;
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;
  int srcWidth = src.cropWidth;
  int srcHeight = src.cropHeight;
  int dstWidth = dst.cropWidth;
  int dstHeight = dst.cropHeight;

  if (v_flip) {
    srcHeight = -srcHeight;
  }

  return libyuv::ARGBScale(srcBuffer, src.strideBytes, srcWidth, srcHeight,
                           dstBuffer, dst.strideBytes, dstWidth, dstHeight,
                           libyuv::kFilterBilinear);
}

int DoAttenuation(const BufferSpec& src, const BufferSpec& dst, bool v_flip) {
  // Point to the upper left corner of the crop rectangles
  uint8_t* srcBuffer =
      src.buffer + src.cropY * src.strideBytes + src.cropX * src.sampleBytes;
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;
  int width = dst.cropWidth;
  int height = dst.cropHeight;

  if (v_flip) {
    height = -height;
  }

  return libyuv::ARGBAttenuate(srcBuffer, src.strideBytes, dstBuffer,
                               dst.strideBytes, width, height);
}

int DoBlending(const BufferSpec& src, const BufferSpec& dst, bool v_flip) {
  // Point to the upper left corner of the crop rectangles
  uint8_t* srcBuffer =
      src.buffer + src.cropY * src.strideBytes + src.cropX * src.sampleBytes;
  uint8_t* dstBuffer =
      dst.buffer + dst.cropY * dst.strideBytes + dst.cropX * dst.sampleBytes;
  int width = dst.cropWidth;
  int height = dst.cropHeight;

  if (v_flip) {
    height = -height;
  }

  // libyuv's ARGB format is hwcomposer's BGRA format, since blending only cares
  // for the position of alpha in the pixel and not the position of the colors
  // this function is perfectly usable.
  return libyuv::ARGBBlend(srcBuffer, src.strideBytes, dstBuffer,
                           dst.strideBytes, dstBuffer, dst.strideBytes, width,
                           height);
}

std::optional<BufferSpec> GetBufferSpec(GrallocBuffer& buffer,
                                        GrallocBufferView& bufferView,
                                        const hwc_rect_t& bufferCrop) {
  auto bufferFormatOpt = buffer.GetDrmFormat();
  if (!bufferFormatOpt) {
    ALOGE("Failed to get gralloc buffer format.");
    return std::nullopt;
  }
  uint32_t bufferFormat = *bufferFormatOpt;

  auto bufferWidthOpt = buffer.GetWidth();
  if (!bufferWidthOpt) {
    ALOGE("Failed to get gralloc buffer width.");
    return std::nullopt;
  }
  uint32_t bufferWidth = *bufferWidthOpt;

  auto bufferHeightOpt = buffer.GetHeight();
  if (!bufferHeightOpt) {
    ALOGE("Failed to get gralloc buffer height.");
    return std::nullopt;
  }
  uint32_t bufferHeight = *bufferHeightOpt;

  uint8_t* bufferData = nullptr;
  uint32_t bufferStrideBytes = 0;
  std::optional<android_ycbcr> bufferYCbCrData;

  if (bufferFormat == DRM_FORMAT_NV12 || bufferFormat == DRM_FORMAT_NV21 ||
      bufferFormat == DRM_FORMAT_YVU420) {
    bufferYCbCrData = bufferView.GetYCbCr();
    if (!bufferYCbCrData) {
      ALOGE("%s failed to get raw ycbcr from view.", __FUNCTION__);
      return std::nullopt;
    }
  } else {
    auto bufferDataOpt = bufferView.Get();
    if (!bufferDataOpt) {
      ALOGE("%s failed to lock gralloc buffer.", __FUNCTION__);
      return std::nullopt;
    }
    bufferData = reinterpret_cast<uint8_t*>(*bufferDataOpt);

    auto bufferStrideBytesOpt = buffer.GetMonoPlanarStrideBytes();
    if (!bufferStrideBytesOpt) {
      ALOGE("%s failed to get plane stride.", __FUNCTION__);
      return std::nullopt;
    }
    bufferStrideBytes = *bufferStrideBytesOpt;
  }

  return BufferSpec(bufferData, bufferYCbCrData, bufferWidth, bufferHeight,
                    bufferCrop.left, bufferCrop.top,
                    bufferCrop.right - bufferCrop.left,
                    bufferCrop.bottom - bufferCrop.top, bufferFormat,
                    bufferStrideBytes, GetDrmFormatBytesPerPixel(bufferFormat));
}

}  // namespace

HWC2::Error GuestComposer::init(const HotplugCallback& cb) {
  DEBUG_LOG("%s", __FUNCTION__);

  if (!mDrmPresenter.init(cb)) {
    ALOGE("%s: failed to initialize DrmPresenter", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::createDisplays(
    Device* device, const AddDisplayToDeviceFunction& addDisplayToDeviceFn) {
  DEBUG_LOG("%s", __FUNCTION__);

  HWC2::Error error = HWC2::Error::None;

  std::vector<DisplayConfig> displayConfigs;

  error = getDisplayConfigsFromDeviceConfig(&displayConfigs);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to get display configs from device config", __FUNCTION__);
    return error;
  }

  error = getDisplayConfigsFromSystemProp(&displayConfigs);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to get display configs from system prop", __FUNCTION__);
    return error;
  }
  uint32_t id = 0;
  for (const auto& displayConfig : displayConfigs) {
    error = createDisplay(device, id, displayConfig.width, displayConfig.height,
                          displayConfig.dpiX, displayConfig.dpiY,
                          displayConfig.refreshRateHz, addDisplayToDeviceFn);
    if (error != HWC2::Error::None) {
      ALOGE("%s: failed to create display %d", __FUNCTION__, id);
      return error;
    }

    ++id;
  }

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::createDisplay(
    Device* device, uint32_t id, uint32_t width, uint32_t height, uint32_t dpiX,
    uint32_t dpiY, uint32_t refreshRateHz,
    const AddDisplayToDeviceFunction& addDisplayToDeviceFn) {
  auto display = std::make_unique<Display>(*device, this, id);
  if (display == nullptr) {
    ALOGE("%s failed to allocate display", __FUNCTION__);
    return HWC2::Error::NoResources;
  }

  auto displayId = display->getId();

  HWC2::Error error = display->init(width, height, dpiX, dpiY, refreshRateHz);
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to initialize display:%" PRIu64, __FUNCTION__, displayId);
    return error;
  }

  auto it = mDisplayInfos.find(displayId);
  if (it != mDisplayInfos.end()) {
    ALOGE("%s: display:%" PRIu64 " already created?", __FUNCTION__, displayId);
  }

  GuestComposerDisplayInfo& displayInfo = mDisplayInfos[displayId];

  uint32_t bufferStride;
  buffer_handle_t bufferHandle;

  auto status = GraphicBufferAllocator::get().allocate(
      width,                   //
      height,                  //
      PIXEL_FORMAT_RGBA_8888,  //
      /*layerCount=*/1,        //
      GraphicBuffer::USAGE_HW_COMPOSER | GraphicBuffer::USAGE_SW_READ_OFTEN |
          GraphicBuffer::USAGE_SW_WRITE_OFTEN,  //
      &bufferHandle,                            //
      &bufferStride,                            //
      "RanchuHwc");
  if (status != OK) {
    ALOGE("%s failed to allocate composition buffer for display:%" PRIu64,
          __FUNCTION__, displayId);
    return HWC2::Error::NoResources;
  }

  displayInfo.compositionResultBuffer = bufferHandle;

  displayInfo.compositionResultDrmBuffer = std::make_unique<DrmBuffer>(
      displayInfo.compositionResultBuffer, mDrmPresenter);

  if (displayId == 0) {
    int flushSyncFd = -1;

    HWC2::Error flushError =
        displayInfo.compositionResultDrmBuffer->flushToDisplay(displayId,
                                                               &flushSyncFd);
    if (flushError != HWC2::Error::None) {
      ALOGW(
          "%s: Initial display flush failed. HWComposer assuming that we are "
          "running in QEMU without a display and disabling presenting.",
          __FUNCTION__);
      mPresentDisabled = true;
    } else {
      close(flushSyncFd);
    }
  }

  error = addDisplayToDeviceFn(std::move(display));
  if (error != HWC2::Error::None) {
    ALOGE("%s failed to add display:%" PRIu64, __FUNCTION__, displayId);
    return error;
  }

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::onDisplayDestroy(Display* display) {
  auto displayId = display->getId();

  auto it = mDisplayInfos.find(displayId);
  if (it == mDisplayInfos.end()) {
    ALOGE("%s: display:%" PRIu64 " missing display buffers?", __FUNCTION__,
          displayId);
    return HWC2::Error::BadDisplay;
  }

  GuestComposerDisplayInfo& displayInfo = mDisplayInfos[displayId];

  GraphicBufferAllocator::get().free(displayInfo.compositionResultBuffer);

  mDisplayInfos.erase(it);

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::getDisplayConfigsFromDeviceConfig(
    std::vector<GuestComposer::DisplayConfig>* configs) {
  DEBUG_LOG("%s", __FUNCTION__);

  const auto deviceConfig = cuttlefish::GetDeviceConfig();
  for (const auto& deviceDisplayConfig : deviceConfig.display_config()) {
    DisplayConfig displayConfig = {
        .width = deviceDisplayConfig.width(),
        .height = deviceDisplayConfig.height(),
        .dpiX = deviceDisplayConfig.dpi(),
        .dpiY = deviceDisplayConfig.dpi(),
        .refreshRateHz = deviceDisplayConfig.refresh_rate_hz(),
    };

    configs->push_back(displayConfig);
  }

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::getDisplayConfigsFromSystemProp(
    std::vector<GuestComposer::DisplayConfig>* configs) {
  DEBUG_LOG("%s", __FUNCTION__);

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
    int propIntPart;
    if (!android::base::ParseInt(propStringPart, &propIntPart)) {
      ALOGE("%s: Invalid syntax for system prop %s which is %s", __FUNCTION__,
            kExternalDisplayProp, propString.c_str());
      return HWC2::Error::BadParameter;
    }
    propIntParts.push_back(propIntPart);
  }

  while (!propIntParts.empty()) {
    DisplayConfig display_config = {
        .width = propIntParts[1],
        .height = propIntParts[2],
        .dpiX = propIntParts[3],
        .dpiY = propIntParts[3],
        .refreshRateHz = 160,
    };

    configs->push_back(display_config);

    propIntParts.erase(propIntParts.begin(), propIntParts.begin() + 5);
  }

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::validateDisplay(
    Display* display, std::unordered_map<hwc2_layer_t, HWC2::Composition>*
                          outLayerCompositionChanges) {
  const auto displayId = display->getId();
  DEBUG_LOG("%s display:%" PRIu64, __FUNCTION__, displayId);

  const std::vector<Layer*>& layers = display->getOrderedLayers();

  bool fallbackToClientComposition = false;
  for (Layer* layer : layers) {
    const auto layerId = layer->getId();
    const auto layerCompositionType = layer->getCompositionType();
    const auto layerCompositionTypeString = to_string(layerCompositionType);

    if (layerCompositionType == HWC2::Composition::Invalid) {
      ALOGE("%s display:%" PRIu64 " layer:%" PRIu64 " has Invalid composition",
            __FUNCTION__, displayId, layerId);
      continue;
    }

    if (layerCompositionType == HWC2::Composition::Client ||
        layerCompositionType == HWC2::Composition::Cursor ||
        layerCompositionType == HWC2::Composition::Sideband ||
        layerCompositionType == HWC2::Composition::SolidColor) {
      ALOGW("%s: display:%" PRIu64 " layer:%" PRIu64
            " has composition type %s, falling back to client composition",
            __FUNCTION__, displayId, layerId,
            layerCompositionTypeString.c_str());
      fallbackToClientComposition = true;
      break;
    }

    if (!canComposeLayer(layer)) {
      ALOGW("%s: display:%" PRIu64 " layer:%" PRIu64
            " composition not supported, falling back to client composition",
            __FUNCTION__, displayId, layerId);
      fallbackToClientComposition = true;
      break;
    }
  }

  if (display->hasColorTransform()) {
    fallbackToClientComposition = true;
  }

  if (fallbackToClientComposition) {
    for (Layer* layer : layers) {
      const auto layerId = layer->getId();
      const auto layerCompositionType = layer->getCompositionType();

      if (layerCompositionType == HWC2::Composition::Invalid) {
        continue;
      }
      if (layerCompositionType != HWC2::Composition::Client) {
        DEBUG_LOG("%s display:%" PRIu64 " layer:%" PRIu64
                  "composition updated to Client",
                  __FUNCTION__, displayId, layerId);
        (*outLayerCompositionChanges)[layerId] = HWC2::Composition::Client;
      }
    }
  }

  // We can not draw below a Client (SurfaceFlinger) composed layer. Change all
  // layers below a Client composed layer to also be Client composed.
  if (layers.size() > 1) {
    for (std::size_t layerIndex = layers.size() - 1; layerIndex > 0;
         layerIndex--) {
      auto layer = layers[layerIndex];
      auto layerCompositionType = layer->getCompositionType();

      if (layerCompositionType == HWC2::Composition::Client) {
        for (std::size_t lowerLayerIndex = 0; lowerLayerIndex < layerIndex;
             lowerLayerIndex++) {
          auto lowerLayer = layers[lowerLayerIndex];
          auto lowerLayerId = lowerLayer->getId();
          auto lowerLayerCompositionType = lowerLayer->getCompositionType();

          if (lowerLayerCompositionType != HWC2::Composition::Client) {
            DEBUG_LOG("%s: display:%" PRIu64 " changing layer:%" PRIu64
                      " to Client because"
                      "hwcomposer can not draw below the Client composed "
                      "layer:%" PRIu64,
                      __FUNCTION__, displayId, lowerLayerId, layer->getId());

            (*outLayerCompositionChanges)[lowerLayerId] =
                HWC2::Composition::Client;
          }
        }
      }
    }
  }

  return HWC2::Error::None;
}

HWC2::Error GuestComposer::presentDisplay(Display* display,
                                          int32_t* outRetireFence) {
  const auto displayId = display->getId();
  DEBUG_LOG("%s display:%" PRIu64, __FUNCTION__, displayId);

  if (displayId != 0) {
    // TODO(b/171305898): remove after multi-display fully supported.
    return HWC2::Error::None;
  }

  if (mPresentDisabled) {
    return HWC2::Error::None;
  }

  auto it = mDisplayInfos.find(displayId);
  if (it == mDisplayInfos.end()) {
    ALOGE("%s: display:%" PRIu64 " not found", __FUNCTION__, displayId);
    return HWC2::Error::NoResources;
  }

  GuestComposerDisplayInfo& displayInfo = it->second;

  if (displayInfo.compositionResultBuffer == nullptr) {
    ALOGE("%s: display:%" PRIu64 " missing composition result buffer",
          __FUNCTION__, displayId);
    return HWC2::Error::NoResources;
  }

  std::optional<GrallocBuffer> compositionResultBufferOpt =
      mGralloc.Import(displayInfo.compositionResultBuffer);
  if (!compositionResultBufferOpt) {
    ALOGE("%s: display:%" PRIu64 " failed to import buffer", __FUNCTION__,
          displayId);
    return HWC2::Error::NoResources;
  }

  std::optional<uint32_t> compositionResultBufferWidthOpt =
      compositionResultBufferOpt->GetWidth();
  if (!compositionResultBufferWidthOpt) {
    ALOGE("%s: display:%" PRIu64 " failed to query buffer width", __FUNCTION__,
          displayId);
    return HWC2::Error::NoResources;
  }

  std::optional<uint32_t> compositionResultBufferHeightOpt =
      compositionResultBufferOpt->GetHeight();
  if (!compositionResultBufferHeightOpt) {
    ALOGE("%s: display:%" PRIu64 " failed to query buffer height", __FUNCTION__,
          displayId);
    return HWC2::Error::NoResources;
  }

  std::optional<uint32_t> compositionResultBufferStrideOpt =
      compositionResultBufferOpt->GetMonoPlanarStrideBytes();
  if (!compositionResultBufferStrideOpt) {
    ALOGE("%s: display:%" PRIu64 " failed to query buffer stride", __FUNCTION__,
          displayId);
    return HWC2::Error::NoResources;
  }

  std::optional<GrallocBufferView> compositionResultBufferViewOpt =
      compositionResultBufferOpt->Lock();
  if (!compositionResultBufferViewOpt) {
    ALOGE("%s: display:%" PRIu64 " failed to get buffer view", __FUNCTION__,
          displayId);
    return HWC2::Error::NoResources;
  }

  const std::optional<void*> compositionResultBufferDataOpt =
      compositionResultBufferViewOpt->Get();
  if (!compositionResultBufferDataOpt) {
    ALOGE("%s: display:%" PRIu64 " failed to get buffer data", __FUNCTION__,
          displayId);
    return HWC2::Error::NoResources;
  }

  uint32_t compositionResultBufferWidth = *compositionResultBufferWidthOpt;
  uint32_t compositionResultBufferHeight = *compositionResultBufferHeightOpt;
  uint32_t compositionResultBufferStride = *compositionResultBufferStrideOpt;
  uint8_t* compositionResultBufferData =
      reinterpret_cast<uint8_t*>(*compositionResultBufferDataOpt);

  const std::vector<Layer*>& layers = display->getOrderedLayers();

  const bool noOpComposition = layers.empty();
  const bool allLayersClientComposed = std::all_of(
      layers.begin(),  //
      layers.end(),    //
      [](const Layer* layer) {
        return layer->getCompositionType() == HWC2::Composition::Client;
      });

  if (noOpComposition) {
    ALOGW("%s: display:%" PRIu64 " empty composition", __FUNCTION__, displayId);
  } else if (allLayersClientComposed) {
    auto clientTargetBufferOpt =
        mGralloc.Import(display->waitAndGetClientTargetBuffer());
    if (!clientTargetBufferOpt) {
      ALOGE("%s: failed to import client target buffer.", __FUNCTION__);
      return HWC2::Error::NoResources;
    }
    GrallocBuffer& clientTargetBuffer = *clientTargetBufferOpt;

    auto clientTargetBufferViewOpt = clientTargetBuffer.Lock();
    if (!clientTargetBufferViewOpt) {
      ALOGE("%s: failed to lock client target buffer.", __FUNCTION__);
      return HWC2::Error::NoResources;
    }
    GrallocBufferView& clientTargetBufferView = *clientTargetBufferViewOpt;

    auto clientTargetPlaneLayoutsOpt = clientTargetBuffer.GetPlaneLayouts();
    if (!clientTargetPlaneLayoutsOpt) {
      ALOGE("Failed to get client target buffer plane layouts.");
      return HWC2::Error::NoResources;
    }
    auto& clientTargetPlaneLayouts = *clientTargetPlaneLayoutsOpt;

    if (clientTargetPlaneLayouts.size() != 1) {
      ALOGE("Unexpected number of plane layouts for client target buffer.");
      return HWC2::Error::NoResources;
    }

    std::size_t clientTargetPlaneSize =
        clientTargetPlaneLayouts[0].totalSizeInBytes;

    auto clientTargetDataOpt = clientTargetBufferView.Get();
    if (!clientTargetDataOpt) {
      ALOGE("%s failed to lock gralloc buffer.", __FUNCTION__);
      return HWC2::Error::NoResources;
    }
    auto* clientTargetData = reinterpret_cast<uint8_t*>(*clientTargetDataOpt);

    std::memcpy(compositionResultBufferData, clientTargetData,
                clientTargetPlaneSize);
  } else {
    for (Layer* layer : layers) {
      const auto layerId = layer->getId();
      const auto layerCompositionType = layer->getCompositionType();
      if (layerCompositionType != HWC2::Composition::Device) {
        continue;
      }

      HWC2::Error error = composeLayerInto(layer,                          //
                                           compositionResultBufferData,    //
                                           compositionResultBufferWidth,   //
                                           compositionResultBufferHeight,  //
                                           compositionResultBufferStride,  //
                                           4);
      if (error != HWC2::Error::None) {
        ALOGE("%s: display:%" PRIu64 " failed to compose layer:%" PRIu64,
              __FUNCTION__, displayId, layerId);
        return error;
      }
    }
  }

  DEBUG_LOG("%s display:%" PRIu64 " flushing drm buffer", __FUNCTION__,
            displayId);

  HWC2::Error error = displayInfo.compositionResultDrmBuffer->flushToDisplay(
      static_cast<int>(displayId), outRetireFence);
  if (error != HWC2::Error::None) {
    ALOGE("%s: display:%" PRIu64 " failed to flush drm buffer" PRIu64,
          __FUNCTION__, displayId);
  }
  return error;
}

bool GuestComposer::canComposeLayer(Layer* layer) {
  buffer_handle_t bufferHandle = layer->getBuffer().getBuffer();
  if (bufferHandle == nullptr) {
    ALOGW("%s received a layer with a null handle", __FUNCTION__);
    return false;
  }

  auto bufferOpt = mGralloc.Import(bufferHandle);
  if (!bufferOpt) {
    ALOGE("Failed to import layer buffer.");
    return false;
  }
  GrallocBuffer& buffer = *bufferOpt;

  auto bufferFormatOpt = buffer.GetDrmFormat();
  if (!bufferFormatOpt) {
    ALOGE("Failed to get layer buffer format.");
    return false;
  }
  uint32_t bufferFormat = *bufferFormatOpt;

  if (!IsDrmFormatSupported(bufferFormat)) {
    return false;
  }

  return true;
}

HWC2::Error GuestComposer::composeLayerInto(
    Layer* srcLayer, std::uint8_t* dstBuffer, std::uint32_t dstBufferWidth,
    std::uint32_t dstBufferHeight, std::uint32_t dstBufferStrideBytes,
    std::uint32_t dstBufferBytesPerPixel) {
  libyuv::RotationMode rotation =
      GetRotationFromTransform(srcLayer->getTransform());

  auto srcBufferOpt = mGralloc.Import(srcLayer->waitAndGetBuffer());
  if (!srcBufferOpt) {
    ALOGE("%s: failed to import layer buffer.", __FUNCTION__);
    return HWC2::Error::NoResources;
  }
  GrallocBuffer& srcBuffer = *srcBufferOpt;

  auto srcBufferViewOpt = srcBuffer.Lock();
  if (!srcBufferViewOpt) {
    ALOGE("%s: failed to lock import layer buffer.", __FUNCTION__);
    return HWC2::Error::NoResources;
  }
  GrallocBufferView& srcBufferView = *srcBufferViewOpt;

  hwc_rect_t srcLayerCrop = srcLayer->getSourceCropInt();
  hwc_rect_t srcLayerDisplayFrame = srcLayer->getDisplayFrame();

  auto srcLayerSpecOpt = GetBufferSpec(srcBuffer, srcBufferView, srcLayerCrop);
  if (!srcLayerSpecOpt) {
    return HWC2::Error::NoResources;
  }
  BufferSpec srcLayerSpec = *srcLayerSpecOpt;

  // TODO(jemoreira): Remove the hardcoded fomat.
  bool needsConversion = srcLayerSpec.drmFormat != DRM_FORMAT_XBGR8888;
  bool needsScaling = LayerNeedsScaling(*srcLayer);
  bool needsRotation = rotation != libyuv::kRotate0;
  bool needsTranspose = needsRotation && rotation != libyuv::kRotate180;
  bool needsVFlip = GetVFlipFromTransform(srcLayer->getTransform());
  bool needsAttenuation = LayerNeedsAttenuation(*srcLayer);
  bool needsBlending = LayerNeedsBlending(*srcLayer);
  bool needsCopy = !(needsConversion || needsScaling || needsRotation ||
                     needsVFlip || needsAttenuation || needsBlending);

  BufferSpec dstLayerSpec(
      dstBuffer,
      /*buffer_ycbcr=*/std::nullopt, dstBufferWidth, dstBufferHeight,
      srcLayerDisplayFrame.left, srcLayerDisplayFrame.top,
      srcLayerDisplayFrame.right - srcLayerDisplayFrame.left,
      srcLayerDisplayFrame.bottom - srcLayerDisplayFrame.top,
      DRM_FORMAT_XBGR8888, dstBufferStrideBytes, dstBufferBytesPerPixel);

  // Add the destination layer to the bottom of the buffer stack
  std::vector<BufferSpec> dstBufferStack(1, dstLayerSpec);

  // If more than operation is to be performed, a temporary buffer is needed for
  // each additional operation

  // N operations need N destination buffers, the destination layer (the
  // framebuffer) is one of them, so only N-1 temporary buffers are needed.
  // Vertical flip is not taken into account because it can be done together
  // with any other operation.
  int neededScratchBuffers = (needsConversion ? 1 : 0) +
                             (needsScaling ? 1 : 0) + (needsRotation ? 1 : 0) +
                             (needsAttenuation ? 1 : 0) +
                             (needsBlending ? 1 : 0) + (needsCopy ? 1 : 0) - 1;

  int mScratchBufferWidth =
      srcLayerDisplayFrame.right - srcLayerDisplayFrame.left;
  int mScratchBufferHeight =
      srcLayerDisplayFrame.bottom - srcLayerDisplayFrame.top;
  int mScratchBufferStrideBytes =
      AlignToPower2(mScratchBufferWidth * dstBufferBytesPerPixel, 4);
  int mScratchBufferSizeBytes =
      mScratchBufferHeight * mScratchBufferStrideBytes;

  for (int i = 0; i < neededScratchBuffers; i++) {
    BufferSpec mScratchBufferspec(
        getRotatingScratchBuffer(mScratchBufferSizeBytes, i),
        mScratchBufferWidth, mScratchBufferHeight, mScratchBufferStrideBytes);
    dstBufferStack.push_back(mScratchBufferspec);
  }

  // Conversion and scaling should always be the first operations, so that every
  // other operation works on equally sized frames (guaranteed to fit in the
  // scratch buffers).

  // TODO(jemoreira): We are converting to ARGB as the first step under the
  // assumption that scaling ARGB is faster than scaling I420 (the most common).
  // This should be confirmed with testing.
  if (needsConversion) {
    BufferSpec& dstBufferSpec = dstBufferStack.back();
    if (needsScaling || needsTranspose) {
      // If a rotation or a scaling operation are needed the dimensions at the
      // top of the buffer stack are wrong (wrong sizes for scaling, swapped
      // width and height for 90 and 270 rotations).
      // Make width and height match the crop sizes on the source
      int srcWidth = srcLayerSpec.cropWidth;
      int srcHeight = srcLayerSpec.cropHeight;
      int dst_stride_bytes =
          AlignToPower2(srcWidth * dstBufferBytesPerPixel, 4);
      size_t needed_size = dst_stride_bytes * srcHeight;
      dstBufferSpec.width = srcWidth;
      dstBufferSpec.height = srcHeight;
      // Adjust the stride accordingly
      dstBufferSpec.strideBytes = dst_stride_bytes;
      // Crop sizes also need to be adjusted
      dstBufferSpec.cropWidth = srcWidth;
      dstBufferSpec.cropHeight = srcHeight;
      // cropX and y are fine at 0, format is already set to match destination

      // In case of a scale, the source frame may be bigger than the default tmp
      // buffer size
      dstBufferSpec.buffer = getSpecialScratchBuffer(needed_size);
    }

    int retval = DoConversion(srcLayerSpec, dstBufferSpec, needsVFlip);
    if (retval) {
      ALOGE("Got error code %d from DoConversion function", retval);
    }
    needsVFlip = false;
    srcLayerSpec = dstBufferSpec;
    dstBufferStack.pop_back();
  }

  if (needsScaling) {
    BufferSpec& dstBufferSpec = dstBufferStack.back();
    if (needsTranspose) {
      // If a rotation is needed, the temporary buffer has the correct size but
      // needs to be transposed and have its stride updated accordingly. The
      // crop sizes also needs to be transposed, but not the x and y since they
      // are both zero in a temporary buffer (and it is a temporary buffer
      // because a rotation will be performed next).
      std::swap(dstBufferSpec.width, dstBufferSpec.height);
      std::swap(dstBufferSpec.cropWidth, dstBufferSpec.cropHeight);
      // TODO (jemoreira): Aligment (To align here may cause the needed size to
      // be bigger than the buffer, so care should be taken)
      dstBufferSpec.strideBytes = dstBufferSpec.width * dstBufferBytesPerPixel;
    }
    int retval = DoScaling(srcLayerSpec, dstBufferSpec, needsVFlip);
    needsVFlip = false;
    if (retval) {
      ALOGE("Got error code %d from DoScaling function", retval);
    }
    srcLayerSpec = dstBufferSpec;
    dstBufferStack.pop_back();
  }

  if (needsRotation) {
    int retval =
        DoRotation(srcLayerSpec, dstBufferStack.back(), rotation, needsVFlip);
    needsVFlip = false;
    if (retval) {
      ALOGE("Got error code %d from DoTransform function", retval);
    }
    srcLayerSpec = dstBufferStack.back();
    dstBufferStack.pop_back();
  }

  if (needsAttenuation) {
    int retval = DoAttenuation(srcLayerSpec, dstBufferStack.back(), needsVFlip);
    needsVFlip = false;
    if (retval) {
      ALOGE("Got error code %d from DoBlending function", retval);
    }
    srcLayerSpec = dstBufferStack.back();
    dstBufferStack.pop_back();
  }

  if (needsCopy) {
    int retval = DoCopy(srcLayerSpec, dstBufferStack.back(), needsVFlip);
    needsVFlip = false;
    if (retval) {
      ALOGE("Got error code %d from DoBlending function", retval);
    }
    srcLayerSpec = dstBufferStack.back();
    dstBufferStack.pop_back();
  }

  // Blending (if needed) should always be the last operation, so that it reads
  // and writes in the destination layer and not some temporary buffer.
  if (needsBlending) {
    int retval = DoBlending(srcLayerSpec, dstBufferStack.back(), needsVFlip);
    needsVFlip = false;
    if (retval) {
      ALOGE("Got error code %d from DoBlending function", retval);
    }
    // Don't need to assign destination to source in the last one
    dstBufferStack.pop_back();
  }

  return HWC2::Error::None;
}

uint8_t* GuestComposer::getRotatingScratchBuffer(std::size_t neededSize,
                                                 std::uint32_t order) {
  static constexpr const int kNumScratchBufferPieces = 2;

  std::size_t totalNeededSize = neededSize * kNumScratchBufferPieces;
  if (mScratchBuffer.size() < totalNeededSize) {
    mScratchBuffer.resize(totalNeededSize);
  }

  std::size_t bufferIndex = order % kNumScratchBufferPieces;
  std::size_t bufferOffset = bufferIndex * neededSize;
  return &mScratchBuffer[bufferOffset];
}

uint8_t* GuestComposer::getSpecialScratchBuffer(size_t neededSize) {
  if (mSpecialScratchBuffer.size() < neededSize) {
    mSpecialScratchBuffer.resize(neededSize);
  }

  return &mSpecialScratchBuffer[0];
}

}  // namespace android
