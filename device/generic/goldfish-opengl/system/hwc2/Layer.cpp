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

#include "Layer.h"

#include <sync/sync.h>

#include <atomic>

namespace android {
namespace {

std::atomic<hwc2_layer_t> sNextId{1};

}  // namespace

Layer::Layer() : mId(sNextId++) {}

HWC2::Error Layer::setBuffer(buffer_handle_t buffer, int32_t fence) {
  DEBUG_LOG("%s: layer:%" PRIu64 " buffer:%p fence:%" PRIu32, __FUNCTION__, mId,
            buffer, fence);
  mBuffer.setBuffer(buffer);
  mBuffer.setFence(fence);
  return HWC2::Error::None;
}

buffer_handle_t Layer::waitAndGetBuffer() {
  DEBUG_LOG("%s layer:%" PRIu64, __FUNCTION__, mId);

  int fence = mBuffer.getFence();
  if (fence != -1) {
    int err = sync_wait(fence, 3000);
    if (err < 0 && errno == ETIME) {
      ALOGE("%s waited on fence %" PRId32 " for 3000 ms", __FUNCTION__, fence);
    }
    close(fence);
  }

  return mBuffer.getBuffer();
}

HWC2::Error Layer::setCursorPosition(int32_t /*x*/, int32_t /*y*/) {
  DEBUG_LOG("%s layer:%" PRIu64, __FUNCTION__, mId);

  if (mCompositionType != HWC2::Composition::Cursor) {
    ALOGE("%s: CompositionType not Cursor type", __FUNCTION__);
    return HWC2::Error::BadLayer;
  }

  return HWC2::Error::None;
}

HWC2::Error Layer::setSurfaceDamage(hwc_region_t /*damage*/) {
  DEBUG_LOG("%s layer:%" PRIu64, __FUNCTION__, mId);

  return HWC2::Error::None;
}

HWC2::Error Layer::setBlendMode(int32_t m) {
  const auto blendMode = static_cast<HWC2::BlendMode>(m);
  const auto blendModeString = to_string(blendMode);
  DEBUG_LOG("%s layer:%" PRIu64 " blend mode:%s", __FUNCTION__, mId,
            blendModeString.c_str());

  mBlendMode = blendMode;
  return HWC2::Error::None;
}

HWC2::BlendMode Layer::getBlendMode() const {
  const auto blendMode = mBlendMode;
  const auto blendModeString = to_string(blendMode);
  DEBUG_LOG("%s layer:%" PRIu64 " blend mode:%s", __FUNCTION__, mId,
            blendModeString.c_str());

  return blendMode;
}

HWC2::Error Layer::setColor(hwc_color_t color) {
  DEBUG_LOG("%s layer:%" PRIu64 " color-r:%d color-g:%d color-b:%d color-a:%d)",
            __FUNCTION__, mId, color.r, color.g, color.b, color.a);

  mColor = color;
  return HWC2::Error::None;
}

hwc_color_t Layer::getColor() const {
  auto color = mColor;
  DEBUG_LOG("%s layer:%" PRIu64 " color-r:%d color-g:%d color-b:%d color-a:%d)",
            __FUNCTION__, mId, color.r, color.g, color.b, color.a);

  return color;
}

HWC2::Error Layer::setCompositionTypeEnum(HWC2::Composition compositionType) {
  const auto compositionTypeString = to_string(compositionType);
  DEBUG_LOG("%s layer:%" PRIu64 " composition type:%s", __FUNCTION__, mId,
            compositionTypeString.c_str());

  mCompositionType = compositionType;
  return HWC2::Error::None;
}

HWC2::Error Layer::setCompositionType(int32_t type) {
  const auto compositionType = static_cast<HWC2::Composition>(type);
  return setCompositionTypeEnum(compositionType);
}

HWC2::Composition Layer::getCompositionType() const {
  const auto compositionType = mCompositionType;
  const auto compositionTypeString = to_string(compositionType);
  DEBUG_LOG("%s layer:%" PRIu64 " composition type:%s", __FUNCTION__, mId,
            compositionTypeString.c_str());

  return compositionType;
}

HWC2::Error Layer::setDataspace(int32_t) {
  DEBUG_LOG("%s layer:%" PRIu64, __FUNCTION__, mId);

  return HWC2::Error::None;
}

HWC2::Error Layer::setDisplayFrame(hwc_rect_t frame) {
  DEBUG_LOG("%s layer:%" PRIu64
            " display frame rect-left:%d rect-top:%d rect-right:%d rect-bot:%d",
            __FUNCTION__, mId, frame.left, frame.top, frame.right,
            frame.bottom);

  mDisplayFrame = frame;
  return HWC2::Error::None;
}

hwc_rect_t Layer::getDisplayFrame() const {
  auto frame = mDisplayFrame;
  DEBUG_LOG("%s layer:%" PRIu64
            " display frame rect-left:%d rect-top:%d rect-right:%d rect-bot:%d",
            __FUNCTION__, mId, frame.left, frame.top, frame.right,
            frame.bottom);

  return frame;
}

HWC2::Error Layer::setPlaneAlpha(float alpha) {
  DEBUG_LOG("%s layer:%" PRIu64 "alpha:%f", __FUNCTION__, mId, alpha);

  mPlaneAlpha = alpha;
  return HWC2::Error::None;
}

float Layer::getPlaneAlpha() const {
  auto alpha = mPlaneAlpha;
  DEBUG_LOG("%s layer:%" PRIu64 "alpha:%f", __FUNCTION__, mId, alpha);

  return alpha;
}

HWC2::Error Layer::setSidebandStream(const native_handle_t* stream) {
  DEBUG_LOG("%s layer:%" PRIu64, __FUNCTION__, mId);

  mSidebandStream = stream;
  return HWC2::Error::None;
}

HWC2::Error Layer::setSourceCrop(hwc_frect_t crop) {
  DEBUG_LOG("%s layer:%" PRIu64
            "crop rect-left:%f rect-top:%f rect-right:%f rect-bot:%f",
            __FUNCTION__, mId, crop.left, crop.top, crop.right, crop.bottom);

  mSourceCrop = crop;
  return HWC2::Error::None;
}

hwc_frect_t Layer::getSourceCrop() const {
  hwc_frect_t crop = mSourceCrop;
  DEBUG_LOG("%s layer:%" PRIu64
            "crop rect-left:%f rect-top:%f rect-right:%f rect-bot:%f",
            __FUNCTION__, mId, crop.left, crop.top, crop.right, crop.bottom);

  return crop;
}

hwc_rect_t Layer::getSourceCropInt() const {
  hwc_rect_t crop = {};
  crop.left = static_cast<int>(mSourceCrop.left);
  crop.top = static_cast<int>(mSourceCrop.top);
  crop.right = static_cast<int>(mSourceCrop.right);
  crop.bottom = static_cast<int>(mSourceCrop.bottom);
  DEBUG_LOG("%s layer:%" PRIu64
            "crop rect-left:%d rect-top:%d rect-right:%d rect-bot:%d",
            __FUNCTION__, mId, crop.left, crop.top, crop.right, crop.bottom);

  return crop;
}

HWC2::Error Layer::setTransform(int32_t transform) {
  const auto transformType = static_cast<HWC2::Transform>(transform);
  const auto transformTypeString = to_string(transformType);
  DEBUG_LOG("%s layer:%" PRIu64 " transform:%s", __FUNCTION__, mId,
            transformTypeString.c_str());

  mTransform = transformType;
  return HWC2::Error::None;
}

hwc_transform_t Layer::getTransform() const {
  const auto transform = mTransform;
  const auto transformString = to_string(transform);
  DEBUG_LOG("%s layer:%" PRIu64 " transform:%s", __FUNCTION__, mId,
            transformString.c_str());

  return static_cast<hwc_transform_t>(transform);
}

HWC2::Error Layer::setVisibleRegion(hwc_region_t visible) {
  DEBUG_LOG("%s layer:%" PRIu64, __FUNCTION__, mId);

  mVisibleRegion.resize(visible.numRects);
  std::copy_n(visible.rects, visible.numRects, mVisibleRegion.data());
  return HWC2::Error::None;
}

std::size_t Layer::getNumVisibleRegions() const {
  std::size_t num = mVisibleRegion.size();
  DEBUG_LOG("%s layer:%" PRIu64 " number of visible regions: %zu", __FUNCTION__,
            mId, num);

  return num;
}

HWC2::Error Layer::setZ(uint32_t z) {
  DEBUG_LOG("%s layer:%" PRIu64 " z:%d", __FUNCTION__, mId, z);

  mZ = z;
  return HWC2::Error::None;
}

uint32_t Layer::getZ() const {
  uint32_t z = mZ;
  DEBUG_LOG("%s layer:%" PRIu64 " z:%d", __FUNCTION__, mId, z);

  return z;
}

}  // namespace android