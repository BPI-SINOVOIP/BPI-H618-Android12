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

#define LOG_TAG "hwc-bufferinfo-mali-mediatek"

#include "BufferInfoMaliMediatek.h"

#include <hardware/gralloc.h>
#include <log/log.h>
#include <stdatomic.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cinttypes>

#include "gralloc_priv.h"

namespace android {

LEGACY_BUFFER_INFO_GETTER(BufferInfoMaliMediatek);

int BufferInfoMaliMediatek::ConvertBoInfo(buffer_handle_t handle,
                                          hwc_drm_bo_t *bo) {
  private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(
      handle);
  if (!hnd)
    return -EINVAL;

  uint32_t fmt = ConvertHalFormatToDrm(hnd->req_format);
  if (fmt == DRM_FORMAT_INVALID)
    return -EINVAL;

  bo->width = hnd->width;
  bo->height = hnd->height;
  bo->hal_format = hnd->req_format;
  bo->format = fmt;
  bo->usage = hnd->consumer_usage | hnd->producer_usage;
  bo->prime_fds[0] = hnd->share_fd;
  bo->pitches[0] = hnd->byte_stride;
  bo->offsets[0] = 0;

  return 0;
}

}  // namespace android
