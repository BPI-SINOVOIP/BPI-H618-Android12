/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SUNXI_DRM_BUFFER_H
#define SUNXI_DRM_BUFFER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Debug.h"
#include "DrmBuffer.h"
#include "sunxi_drm_heap.h"
#include "uniquefd.h"

namespace sunxi {

// Wrapper class to maintain drm heap deivce fd
class DrmHeap {
public:
    static DrmHeap& getInstance() {
        static DrmHeap _instance;
        return _instance;
    }

    static bool isValid() {
        return DrmHeap::getInstance().drmHeapHandle() > 0;
    }

    static int allocate(uint32_t size, uint32_t flags) {
        return DrmHeap::getInstance().drmHeapAllocate(size, flags);
    }

private:
    int drmHeapAllocate(uint32_t size, uint32_t flags) {
        struct dma_heap_allocation_data data = {
            .len = size,
            .fd = 0,
            .fd_flags = O_RDWR | O_CLOEXEC,
            .heap_flags = flags,
        };
        int ret;

        if (mDrmHeapHandle.get() < 0)
            return -EINVAL;

        ret = ioctl(mDrmHeapHandle.get(), DMA_HEAP_IOC_ALLOC, &data);
        if (ret < 0) {
            DLOGE("sunxi drm heap allocate failed, ret=%d", ret);
            return ret;
        }
        return (int)data.fd;
    }

    DrmHeap() {
        int fd = open("/dev/sunxi_drm_heap", O_RDWR);
        if (fd < 0) {
            DLOGE("sunxi_drm_heap open failed, %s", strerror(errno));
        }
        mDrmHeapHandle = uniquefd(fd);
    }

    int drmHeapHandle() {
        return mDrmHeapHandle.get();
    }

   ~DrmHeap() = default;
    DrmHeap(const DrmHeap&) = delete;
    DrmHeap& operator=(const DrmHeap&) = delete;
    sunxi::uniquefd mDrmHeapHandle;
};

}; // namespace sunxi


#endif
