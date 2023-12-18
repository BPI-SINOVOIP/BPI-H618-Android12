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

#include <errno.h>
#include <hardware/gralloc.h>

#include "Debug.h"
#include "IonBuffer.h"
#include "uniquefd.h"
#include <sys/mman.h>

namespace sunxi {

// Wrapper class to maintain ion deivce fd
class iondev {
public:
    static iondev& getInstance() {
        static iondev _instance;
        return _instance;
    }
    inline int getHandleFd() { return mIonDeviceHandle.get(); }
    static int fd() { return getInstance().getHandleFd(); }

private:
    iondev();
   ~iondev() = default;
    iondev(const iondev&) = delete;
    iondev& operator=(const iondev&) = delete;
    sunxi::uniquefd mIonDeviceHandle;
};

iondev::iondev() {
    int fd = ion_open();
    if (fd < 0) {
        DLOGE("ion_open() failed, %s", strerror(errno));
    }
    mIonDeviceHandle = uniquefd(fd);
}

IonBuffer::IonBuffer()
{
    handle = -1;
    size = 0;
    usage = 0;
	virt_addr = nullptr;
}

IonBuffer::IonBuffer(uint32_t inSize, uint64_t inUsage)
    : IonBuffer() {
    DTRACE_FUNC();
    initWithSize(inSize, inUsage);
}

IonBuffer::~IonBuffer()
{
    DTRACE_FUNC();
    if (handle != -1) {
        ::close(handle);
        handle = -1;
    }
    size = 0;
    usage = 0;
}

const PrivateBuffer_t* IonBuffer::getPrivateBuffer() const
{
    return static_cast<const PrivateBuffer_t*>(this);
}

#define USE_IOMMU
#define DEF_ION_BUF_ALIGN   (4096)

#ifndef ION_HEAP_SECURE_MASK
#define ION_HEAP_TYPE_SECURE     (ION_HEAP_TYPE_CUSTOM + 1)
#define ION_HEAP_SECURE_MASK     (1 << ION_HEAP_TYPE_SECURE)
#endif

int IonBuffer::initWithSize(uint32_t inSize, uint64_t inUsage)
{
    bool physicalContiguous = false;
    if (inUsage & GRALLOC_USAGE_HW_MASK) {
#ifndef USE_IOMMU
        // only require physical contiguous when iommu is disable.
        physicalContiguous = true;
#endif
    }

    int ret = -1;
    int buf = -1;
    if (inUsage & GRALLOC_USAGE_PROTECTED) {
        ret = ion_alloc_fd(iondev::fd(), inSize,
                DEF_ION_BUF_ALIGN, ION_HEAP_SECURE_MASK, ION_FLAG_CACHED, &buf);
        if (ret != 0) {
            DLOGE("ion alloacte secure buffer failed, mask=0x%08x, ret=%d",
                    ION_HEAP_SECURE_MASK, ret);
            return -1;
        }
        goto __allocated;
    }

    if (!physicalContiguous) {
        // if we failed with ION_HEAP_SYSTEM_MASK, fallback into
        // physical contiguous memory.
        ret = ion_alloc_fd(iondev::fd(), inSize,
                DEF_ION_BUF_ALIGN, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED, &buf);
    }

    if (buf == -1) {
        ret = ion_alloc_fd(iondev::fd(), inSize,
                DEF_ION_BUF_ALIGN, ION_HEAP_TYPE_DMA_MASK, ION_FLAG_CACHED, &buf);

        if (ret != 0) {
            ret = ion_alloc_fd(iondev::fd(), inSize,
                    DEF_ION_BUF_ALIGN, ION_HEAP_SYSTEM_CONTIG_MASK, ION_FLAG_CACHED, &buf);
        }
    }
    if (buf == -1) {
        DLOGE("ion alloacte faile, ret=%d", ret);
        return -1;
    }
	//mmap 

	virt_addr =
		mmap(NULL, inSize, PROT_READ | PROT_WRITE,
		     MAP_SHARED, buf, 0);

__allocated:
    handle = buf;
    size = inSize;
    usage = inUsage;
    return 0;
}

} // namespace sunxi

