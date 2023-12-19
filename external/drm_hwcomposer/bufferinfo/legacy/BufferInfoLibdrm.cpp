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

#define LOG_TAG "hwc-bufferinfo-libdrm"

#include "BufferInfoLibdrm.h"

#include <cutils/properties.h>
#include <gralloc_handle.h>
#include <hardware/gralloc.h>
#include <log/log.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace android {

LEGACY_BUFFER_INFO_GETTER(BufferInfoLibdrm);

enum chroma_order {
  YCbCr,
  YCrCb,
};

struct droid_yuv_format {
  /* Lookup keys */
  int native;                     /* HAL_PIXEL_FORMAT_ */
  enum chroma_order chroma_order; /* chroma order is {Cb, Cr} or {Cr, Cb} */
  int chroma_step; /* Distance in bytes between subsequent chroma pixels. */

  /* Result */
  int fourcc; /* DRM_FORMAT_ */
};

/* The following table is used to look up a DRI image FourCC based
 * on native format and information contained in android_ycbcr struct. */
static const struct droid_yuv_format droid_yuv_formats[] = {
    /* Native format, YCrCb, Chroma step, DRI image FourCC */
    {HAL_PIXEL_FORMAT_YCbCr_420_888, YCbCr, 2, DRM_FORMAT_NV12},
    {HAL_PIXEL_FORMAT_YCbCr_420_888, YCbCr, 1, DRM_FORMAT_YUV420},
    {HAL_PIXEL_FORMAT_YCbCr_420_888, YCrCb, 1, DRM_FORMAT_YVU420},
    {HAL_PIXEL_FORMAT_YV12, YCrCb, 1, DRM_FORMAT_YVU420},
    /* HACK: See droid_create_image_from_prime_fds() and
     * https://issuetracker.google.com/32077885. */
    {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCbCr, 2, DRM_FORMAT_NV12},
    {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCbCr, 1, DRM_FORMAT_YUV420},
    {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCrCb, 1, DRM_FORMAT_YVU420},
    {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCrCb, 1, DRM_FORMAT_AYUV},
    {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCrCb, 1, DRM_FORMAT_XYUV8888},
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int get_fourcc_yuv(int native, enum chroma_order chroma_order,
                          int chroma_step) {
  for (int i = 0; i < ARRAY_SIZE(droid_yuv_formats); ++i)
    if (droid_yuv_formats[i].native == native &&
        droid_yuv_formats[i].chroma_order == chroma_order &&
        droid_yuv_formats[i].chroma_step == chroma_step)
      return droid_yuv_formats[i].fourcc;

  return -1;
}

static bool is_yuv(int native) {
  for (int i = 0; i < ARRAY_SIZE(droid_yuv_formats); ++i)
    if (droid_yuv_formats[i].native == native)
      return true;

  return false;
}

bool BufferInfoLibdrm::GetYuvPlaneInfo(int num_fds, buffer_handle_t handle,
                                       hwc_drm_bo_t *bo) {
  struct android_ycbcr ycbcr;
  enum chroma_order chroma_order;
  int ret;

  if (!gralloc_->lock_ycbcr) {
    static std::once_flag once;
    std::call_once(once,
                   []() { ALOGW("Gralloc does not support lock_ycbcr()"); });
    return false;
  }

  memset(&ycbcr, 0, sizeof(ycbcr));
  ret = gralloc_->lock_ycbcr(gralloc_, handle, 0, 0, 0, 0, 0, &ycbcr);
  if (ret) {
    ALOGW("gralloc->lock_ycbcr failed: %d", ret);
    return false;
  }
  gralloc_->unlock(gralloc_, handle);

  /* When lock_ycbcr's usage argument contains no SW_READ/WRITE flags
   * it will return the .y/.cb/.cr pointers based on a NULL pointer,
   * so they can be interpreted as offsets. */
  bo->offsets[0] = (size_t)ycbcr.y;
  /* We assume here that all the planes are located in one DMA-buf. */
  if ((size_t)ycbcr.cr < (size_t)ycbcr.cb) {
    chroma_order = YCrCb;
    bo->offsets[1] = (size_t)ycbcr.cr;
    bo->offsets[2] = (size_t)ycbcr.cb;
  } else {
    chroma_order = YCbCr;
    bo->offsets[1] = (size_t)ycbcr.cb;
    bo->offsets[2] = (size_t)ycbcr.cr;
  }

  /* .ystride is the line length (in bytes) of the Y plane,
   * .cstride is the line length (in bytes) of any of the remaining
   * Cb/Cr/CbCr planes, assumed to be the same for Cb and Cr for fully
   * planar formats. */
  bo->pitches[0] = ycbcr.ystride;
  bo->pitches[1] = bo->pitches[2] = ycbcr.cstride;

  /* .chroma_step is the byte distance between the same chroma channel
   * values of subsequent pixels, assumed to be the same for Cb and Cr. */
  bo->format = get_fourcc_yuv(bo->hal_format, chroma_order, ycbcr.chroma_step);
  if (bo->format == -1) {
    ALOGW(
        "unsupported YUV format, native = %x, chroma_order = %s, chroma_step = "
        "%d",
        bo->hal_format, chroma_order == YCbCr ? "YCbCr" : "YCrCb",
        (int)ycbcr.chroma_step);
    return false;
  }

  /*
   * Since this is EGL_NATIVE_BUFFER_ANDROID don't assume that
   * the single-fd case cannot happen.  So handle eithe single
   * fd or fd-per-plane case:
   */
  if (num_fds == 1) {
    bo->prime_fds[2] = bo->prime_fds[1] = bo->prime_fds[0];
  } else {
    int expected_planes = (ycbcr.chroma_step == 2) ? 2 : 3;
    if (num_fds != expected_planes)
      return false;
  }

  return true;
}

int BufferInfoLibdrm::ConvertBoInfo(buffer_handle_t handle, hwc_drm_bo_t *bo) {
  gralloc_handle_t *gr_handle = gralloc_handle(handle);
  if (!gr_handle)
    return -EINVAL;

  bo->width = gr_handle->width;
  bo->height = gr_handle->height;
  bo->hal_format = gr_handle->format;

#if GRALLOC_HANDLE_VERSION < 4
  static std::once_flag once;
  std::call_once(once, []() {
    ALOGE(
        "libdrm < v2.4.97 has broken gralloc_handle structure. Please update.");
  });
#endif
#if GRALLOC_HANDLE_VERSION == 4
  bo->modifiers[0] = gr_handle->modifier;
  bo->with_modifiers = gr_handle->modifier != DRM_FORMAT_MOD_NONE &&
                       gr_handle->modifier != DRM_FORMAT_MOD_INVALID;
#endif

  bo->usage = gr_handle->usage;
  bo->prime_fds[0] = gr_handle->prime_fd;

  if (is_yuv(gr_handle->format)) {
    if (!GetYuvPlaneInfo(handle->numFds, handle, bo))
      return -EINVAL;
  } else {
    bo->pitches[0] = gr_handle->stride;
    bo->offsets[0] = 0;
    bo->format = ConvertHalFormatToDrm(gr_handle->format);
    if (bo->format == DRM_FORMAT_INVALID)
      return -EINVAL;
  }

  return 0;
}

}  // namespace android
