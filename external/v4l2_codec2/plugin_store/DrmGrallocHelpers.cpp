// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "DrmGrallocHelper"

#include <v4l2_codec2/plugin_store/DrmGrallocHelpers.h>

#include <fcntl.h>
#include <string.h>

#include <drm/drm.h>
#include <log/log.h>

namespace android {

std::optional<int> openRenderFd() {
    const char kVirglName[] = "virtio_gpu";

    for (uint32_t i = 128; i < 192; i++) {
        char devName[32];
        snprintf(devName, sizeof(devName), "/dev/dri/renderD%d", i);

        int fd = open(devName, O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }

        char name[32];
        struct drm_version v;
        memset(&v, 0, sizeof(v));
        v.name = name;
        v.name_len = sizeof(name);

        if (ioctl(fd, static_cast<int>(DRM_IOCTL_VERSION), &v)) {
            close(fd);
            continue;
        }
        if (v.name_len != sizeof(kVirglName) - 1 || memcmp(name, kVirglName, v.name_len)) {
            close(fd);
            continue;
        }
        return fd;
    }
    return std::nullopt;
}

std::optional<uint32_t> getDrmHandle(int renderFd, int primeFd) {
    ALOGV("%s(renderFd=%d, primeFd=%u)", __func__, renderFd, primeFd);

    struct drm_prime_handle prime;
    memset(&prime, 0, sizeof(prime));
    prime.fd = primeFd;

    if (ioctl(renderFd, static_cast<int>(DRM_IOCTL_PRIME_FD_TO_HANDLE), &prime)) {
        ALOGE("Can't translate prime fd %d to handle", prime.fd);
        return std::nullopt;
    }
    return prime.handle;
}

void closeDrmHandle(int renderFd, uint32_t handle) {
    ALOGV("%s(renderFd=%d, handle=%u)", __func__, renderFd, handle);

    struct drm_gem_close gem;
    memset(&gem, 0, sizeof(gem));
    gem.handle = handle;

    ioctl(renderFd, DRM_IOCTL_GEM_CLOSE, &gem);
}

}  // namespace android
