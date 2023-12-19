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

#define LOG_TAG "hwc-bufferinfo-mali-hisi"

#include "BufferInfoMaliHisi.h"

#include <log/log.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cinttypes>

#include "gralloc_priv.h"

#define MALI_ALIGN(value, base) (((value) + ((base)-1)) & ~((base)-1))

namespace android {

LEGACY_BUFFER_INFO_GETTER(BufferInfoMaliHisi);

#if defined(MALI_GRALLOC_INTFMT_AFBC_BASIC) && \
    defined(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16)
uint64_t BufferInfoMaliHisi::ConvertGrallocFormatToDrmModifiers(uint64_t flags,
                                                                bool is_rgb) {
  uint64_t features = 0UL;

  if (flags & MALI_GRALLOC_INTFMT_AFBC_BASIC)
    features |= AFBC_FORMAT_MOD_BLOCK_SIZE_16x16;

  if (flags & MALI_GRALLOC_INTFMT_AFBC_SPLITBLK)
    features |= (AFBC_FORMAT_MOD_SPLIT | AFBC_FORMAT_MOD_SPARSE);

  if (flags & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK)
    features |= AFBC_FORMAT_MOD_BLOCK_SIZE_32x8;

  if (flags & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS)
    features |= AFBC_FORMAT_MOD_TILED;

  if (features) {
    if (is_rgb)
      features |= AFBC_FORMAT_MOD_YTR;

    return DRM_FORMAT_MOD_ARM_AFBC(features);
  }

  return 0;
}
#else
uint64_t BufferInfoMaliHisi::ConvertGrallocFormatToDrmModifiers(
    uint64_t /* flags */, bool /* is_rgb */) {
  return 0;
}
#endif

int BufferInfoMaliHisi::ConvertBoInfo(buffer_handle_t handle,
                                      hwc_drm_bo_t *bo) {
  bool is_rgb;

  private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(
      handle);
  if (!hnd)
    return -EINVAL;

  if (!(hnd->usage & GRALLOC_USAGE_HW_FB))
    return -EINVAL;

  uint32_t fmt = ConvertHalFormatToDrm(hnd->req_format);
  if (fmt == DRM_FORMAT_INVALID)
    return -EINVAL;

  is_rgb = IsDrmFormatRgb(fmt);
  bo->modifiers[0] = ConvertGrallocFormatToDrmModifiers(hnd->internal_format,
                                                        is_rgb);

  bo->width = hnd->width;
  bo->height = hnd->height;
  bo->hal_format = hnd->req_format;
  bo->format = fmt;
  bo->usage = hnd->usage;
  bo->pitches[0] = hnd->byte_stride;
  bo->prime_fds[0] = hnd->share_fd;
  bo->offsets[0] = 0;

  switch (fmt) {
    case DRM_FORMAT_YVU420: {
      int align = 128;
      if (hnd->usage &
          (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
        align = 16;
      int adjusted_height = MALI_ALIGN(hnd->height, 2);
      int y_size = adjusted_height * hnd->byte_stride;
      int vu_stride = MALI_ALIGN(hnd->byte_stride / 2, align);
      int v_size = vu_stride * (adjusted_height / 2);

      /* V plane*/
      bo->prime_fds[1] = hnd->share_fd;
      bo->pitches[1] = vu_stride;
      bo->offsets[1] = y_size;
      /* U plane */
      bo->prime_fds[2] = hnd->share_fd;
      bo->pitches[2] = vu_stride;
      bo->offsets[2] = y_size + v_size;
      break;
    }
    default:
      break;
  }

  bo->with_modifiers = true;

  return 0;
}

}  // namespace android
