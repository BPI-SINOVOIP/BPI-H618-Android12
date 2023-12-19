/*
 * Copyright (C) 2020 The Android Open Source Project
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

#if PLATFORM_SDK_VERSION >= 30

#define LOG_TAG "hwc-bufferinfo-mappermetadata"

#include "BufferInfoMapperMetadata.h"

#include <drm/drm_fourcc.h>
#include <inttypes.h>
#include <log/log.h>
#include <ui/GraphicBufferMapper.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

using android::hardware::graphics::common::V1_1::BufferUsage;

namespace android {

BufferInfoGetter *BufferInfoMapperMetadata::CreateInstance() {
  if (GraphicBufferMapper::getInstance().getMapperVersion() <
      GraphicBufferMapper::GRALLOC_4)
    return nullptr;

  return new BufferInfoMapperMetadata();
}

/* The implementation below makes assumptions on the order and number of file
 * descriptors that Gralloc places in the native_handle_t and as such it very
 * likely needs to be adapted to match the particular Gralloc implementation
 * used in the system. For this reason it is been declared as a weak symbol,
 * so that it can be overridden.
 */
int __attribute__((weak))
BufferInfoMapperMetadata::GetFds(buffer_handle_t handle, hwc_drm_bo_t *bo) {
  int num_fds = handle->numFds;

  if (num_fds >= 1 && num_fds <= 2) {
    if (IsDrmFormatRgb(bo->format)) {
      bo->prime_fds[0] = handle->data[0];
    } else {
      bo->prime_fds[0] = bo->prime_fds[1] = bo->prime_fds[2] = handle->data[0];
    }
    if (bo->prime_fds[0] <= 0) {
      ALOGE("Encountered invalid fd %d", bo->prime_fds[0]);
      return android::BAD_VALUE;
    }

  } else if (num_fds >= 3) {
    bo->prime_fds[0] = handle->data[0];
    bo->prime_fds[1] = handle->data[1];
    bo->prime_fds[2] = handle->data[2];
    for (int i = 0; i < 3; i++) {
      if (bo->prime_fds[i] <= 0) {
        ALOGE("Encountered invalid fd %d", bo->prime_fds[i]);
        return android::BAD_VALUE;
      }
    }
  }
  return 0;
}

int BufferInfoMapperMetadata::ConvertBoInfo(buffer_handle_t handle,
                                            hwc_drm_bo_t *bo) {
  GraphicBufferMapper &mapper = GraphicBufferMapper::getInstance();
  if (!handle)
    return -EINVAL;

  uint64_t usage = 0;
  int err = mapper.getUsage(handle, &usage);
  if (err) {
    ALOGE("Failed to get usage err=%d", err);
    return err;
  }
  bo->usage = static_cast<uint32_t>(usage);

  ui::PixelFormat hal_format;
  err = mapper.getPixelFormatRequested(handle, &hal_format);
  if (err) {
    ALOGE("Failed to get HAL Pixel Format err=%d", err);
    return err;
  }
  bo->hal_format = static_cast<uint32_t>(hal_format);

  err = mapper.getPixelFormatFourCC(handle, &bo->format);
  if (err) {
    ALOGE("Failed to get FourCC format err=%d", err);
    return err;
  }

  err = mapper.getPixelFormatModifier(handle, &bo->modifiers[0]);
  if (err) {
    ALOGE("Failed to get DRM Modifier err=%d", err);
    return err;
  }
  bo->with_modifiers = true;

  uint64_t width = 0;
  err = mapper.getWidth(handle, &width);
  if (err) {
    ALOGE("Failed to get Width err=%d", err);
    return err;
  }
  bo->width = static_cast<uint32_t>(width);

  uint64_t height = 0;
  err = mapper.getHeight(handle, &height);
  if (err) {
    ALOGE("Failed to get Height err=%d", err);
    return err;
  }
  bo->height = static_cast<uint32_t>(height);

  std::vector<ui::PlaneLayout> layouts;
  err = mapper.getPlaneLayouts(handle, &layouts);
  if (err) {
    ALOGE("Failed to get Plane Layouts err=%d", err);
    return err;
  }

  for (uint32_t i = 0; i < layouts.size(); i++) {
    bo->modifiers[i] = bo->modifiers[0];
    bo->pitches[i] = layouts[i].strideInBytes;
    bo->offsets[i] = layouts[i].offsetInBytes;
  }

  return GetFds(handle, bo);
}

}  // namespace android

#endif
