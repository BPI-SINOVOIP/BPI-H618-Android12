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

#ifndef ANDROID_HWC_LAYER_H
#define ANDROID_HWC_LAYER_H

#include <vector>

#include "Common.h"
#include "FencedBuffer.h"

namespace android {

class Layer {
 public:
  explicit Layer();

  Layer(const Layer&) = delete;
  Layer& operator=(const Layer&) = delete;

  Layer(Layer&&) = default;
  Layer& operator=(Layer&&) = default;

  HWC2::Error setBuffer(buffer_handle_t buffer, int32_t acquireFence);
  FencedBuffer& getBuffer() { return mBuffer; }

  buffer_handle_t waitAndGetBuffer();

  hwc2_layer_t getId() const { return mId; }

  HWC2::Error setCursorPosition(int32_t x, int32_t y);

  HWC2::Error setSurfaceDamage(hwc_region_t damage);

  HWC2::Error setBlendMode(int32_t mode);
  HWC2::BlendMode getBlendMode() const;

  HWC2::Error setColor(hwc_color_t color);
  hwc_color_t getColor() const;

  HWC2::Error setCompositionTypeEnum(HWC2::Composition type);
  HWC2::Error setCompositionType(int32_t type);
  HWC2::Composition getCompositionType() const;

  HWC2::Error setDataspace(int32_t dataspace);

  HWC2::Error setDisplayFrame(hwc_rect_t frame);
  hwc_rect_t getDisplayFrame() const;

  HWC2::Error setPlaneAlpha(float alpha);
  float getPlaneAlpha() const;

  HWC2::Error setSidebandStream(const native_handle_t* stream);

  HWC2::Error setSourceCrop(hwc_frect_t crop);
  hwc_frect_t getSourceCrop() const;
  hwc_rect_t getSourceCropInt() const;

  HWC2::Error setTransform(int32_t transform);
  hwc_transform_t getTransform() const;

  HWC2::Error setVisibleRegion(hwc_region_t visible);
  std::size_t getNumVisibleRegions() const;

  HWC2::Error setZ(uint32_t z);
  uint32_t getZ() const;

 private:
  const hwc2_layer_t mId;
  FencedBuffer mBuffer;
  std::vector<hwc_rect_t> mSurfaceDamage;
  HWC2::BlendMode mBlendMode = HWC2::BlendMode::None;
  hwc_color_t mColor = {0, 0, 0, 0};
  HWC2::Composition mCompositionType = HWC2::Composition::Invalid;
  hwc_rect_t mDisplayFrame = {0, 0, -1, -1};
  float mPlaneAlpha = 0.0f;
  const native_handle_t* mSidebandStream = nullptr;
  hwc_frect_t mSourceCrop = {0.0f, 0.0f, -1.0f, -1.0f};
  HWC2::Transform mTransform = HWC2::Transform::None;
  std::vector<hwc_rect_t> mVisibleRegion;
  uint32_t mZ = 0;
};

}  // namespace android

#endif