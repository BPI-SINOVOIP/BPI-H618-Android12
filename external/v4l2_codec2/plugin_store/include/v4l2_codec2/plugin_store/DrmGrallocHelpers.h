// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_V4L2_CODEC2_PLUGIN_STORE_STORE_DRM_GRALLOC_HELPERS_H
#define ANDROID_V4L2_CODEC2_PLUGIN_STORE_STORE_DRM_GRALLOC_HELPERS_H

#include <stdint.h>

#include <optional>

namespace android {

std::optional<int> openRenderFd();
std::optional<uint32_t> getDrmHandle(int renderFd, int primeFd);
void closeDrmHandle(int renderFd, uint32_t handle);

}  // namespace android
#endif  // ANDROID_V4L2_CODEC2_PLUGIN_STORE_STORE_DRM_GRALLOC_HELPERS_H
