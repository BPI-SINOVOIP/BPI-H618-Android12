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

#define LOG_TAG "hwc-buffer-info-getter"

#include "BufferInfoGetter.h"

#if PLATFORM_SDK_VERSION >= 30
#include "BufferInfoMapperMetadata.h"
#endif

#include <cutils/properties.h>
#include <gralloc_handle.h>
#include <hardware/gralloc.h>
#include <log/log.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace android {

BufferInfoGetter *BufferInfoGetter::GetInstance() {
  static std::unique_ptr<BufferInfoGetter> inst;
  if (inst == nullptr) {
#if PLATFORM_SDK_VERSION >= 30
    inst.reset(BufferInfoMapperMetadata::CreateInstance());
    if (inst == nullptr) {
      ALOGW(
          "Generic buffer getter is not available. Falling back to legacy...");
#endif
      inst.reset(LegacyBufferInfoGetter::CreateInstance());
#if PLATFORM_SDK_VERSION >= 30
    }
#endif
  }

  return inst.get();
}

bool BufferInfoGetter::IsHandleUsable(buffer_handle_t handle) {
  hwc_drm_bo_t bo;
  memset(&bo, 0, sizeof(hwc_drm_bo_t));

  if (ConvertBoInfo(handle, &bo) != 0)
    return false;

  if (bo.prime_fds[0] == 0)
    return false;

  return true;
}

int LegacyBufferInfoGetter::Init() {
  int ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
                          (const hw_module_t **)&gralloc_);
  if (ret) {
    ALOGE("Failed to open gralloc module");
    return ret;
  }

  ALOGI("Using %s gralloc module: %s\n", gralloc_->common.name,
        gralloc_->common.author);

  return 0;
}

uint32_t LegacyBufferInfoGetter::ConvertHalFormatToDrm(uint32_t hal_format) {
  switch (hal_format) {
    case HAL_PIXEL_FORMAT_RGB_888:
      return DRM_FORMAT_BGR888;
    case HAL_PIXEL_FORMAT_BGRA_8888:
      return DRM_FORMAT_ARGB8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
      return DRM_FORMAT_XBGR8888;
    case HAL_PIXEL_FORMAT_RGBA_8888:
      return DRM_FORMAT_ABGR8888;
    case HAL_PIXEL_FORMAT_RGB_565:
      return DRM_FORMAT_BGR565;
    case HAL_PIXEL_FORMAT_YV12:
      return DRM_FORMAT_YVU420;
    default:
      ALOGE("Cannot convert hal format to drm format %u", hal_format);
      return DRM_FORMAT_INVALID;
  }
}

bool BufferInfoGetter::IsDrmFormatRgb(uint32_t drm_format) {
  switch (drm_format) {
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_XBGR8888:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_BGR888:
    case DRM_FORMAT_BGR565:
      return true;
    default:
      return false;
  }
}

__attribute__((weak)) LegacyBufferInfoGetter *
LegacyBufferInfoGetter::CreateInstance() {
  ALOGE("No legacy buffer info getters available");
  return nullptr;
}

}  // namespace android
