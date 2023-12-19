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

#ifndef ANDROID_PLATFORM_DRM_GENERIC_H_
#define ANDROID_PLATFORM_DRM_GENERIC_H_

#include <drm/drm_fourcc.h>
#include <hardware/gralloc.h>

#include <map>

#include "drm/DrmDevice.h"
#include "drmhwcgralloc.h"

#ifndef DRM_FORMAT_INVALID
#define DRM_FORMAT_INVALID 0
#endif

namespace android {

class Importer {
 public:
  virtual ~Importer() {
  }

  // Imports the buffer referred to by handle into bo.
  //
  // Note: This can be called from a different thread than ReleaseBuffer. The
  //       implementation is responsible for ensuring thread safety.
  virtual int ImportBuffer(hwc_drm_bo_t *bo) = 0;

  // Releases the buffer object (ie: does the inverse of ImportBuffer)
  //
  // Note: This can be called from a different thread than ImportBuffer. The
  //       implementation is responsible for ensuring thread safety.
  virtual int ReleaseBuffer(hwc_drm_bo_t *bo) = 0;
};

class DrmGenericImporter : public Importer {
 public:
  DrmGenericImporter(DrmDevice *drm);
  ~DrmGenericImporter() override;

  int ImportBuffer(hwc_drm_bo_t *bo) override;
  int ReleaseBuffer(hwc_drm_bo_t *bo) override;
  int ImportHandle(uint32_t gem_handle);
  int ReleaseHandle(uint32_t gem_handle);

 protected:
  DrmDevice *drm_;

 private:
  int CloseHandle(uint32_t gem_handle);
  std::map<uint32_t, int> gem_refcount_;
  bool has_modifier_support_;
};

}  // namespace android

#endif
