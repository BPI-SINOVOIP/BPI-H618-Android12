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

#include "BackendRCarDu.h"

#include "BackendManager.h"
#include "bufferinfo/BufferInfoGetter.h"
#include "drm_fourcc.h"

namespace android {

bool BackendRCarDu::IsClientLayer(DrmHwcTwo::HwcDisplay *display,
                                  DrmHwcTwo::HwcLayer *layer) {
  hwc_drm_bo_t bo;

  int ret = BufferInfoGetter::GetInstance()->ConvertBoInfo(layer->buffer(),
                                                           &bo);
  if (ret)
    return true;

  if (bo.format == DRM_FORMAT_ABGR8888)
    return true;

  if (layer->RequireScalingOrPhasing())
    return true;

  return Backend::IsClientLayer(display, layer);
}

REGISTER_BACKEND("rcar-du", BackendRCarDu);

}  // namespace android