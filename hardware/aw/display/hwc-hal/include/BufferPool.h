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

#ifndef SUNXI_BUFFER_POOL_H_
#define SUNXI_BUFFER_POOL_H_

#include <stdint.h>
#include <sys/types.h>
#include <vector>

#include "IonBuffer.h"

namespace sunxi {

class BufferPoolImpl;

class BufferPool {
public:
    BufferPool(int count, uint32_t size, uint64_t usage, bool allocateImmediately = true);
   ~BufferPool();
    int acquireBuffer(const PrivateBuffer_t** buf, int* acquireFence);
    void releaseBuffer(const PrivateBuffer_t* buf, int releaseFence);
    void reallocate(uint32_t size);
    void setName(const char* name);
    int acquireBufferCount() const;
    uint32_t bufferSize() const;
    uint64_t bufferUsage() const;

private:
    std::shared_ptr<BufferPoolImpl> mBufferPoolImpl;
};

} // namespace sunxi
#endif
