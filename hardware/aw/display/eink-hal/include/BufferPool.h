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
#include <mutex>

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
	int getFreeBufNo();

private:
    std::shared_ptr<BufferPoolImpl> mBufferPoolImpl;
};

class BufferPoolCache {
public:
    std::unique_ptr<BufferPool> get(uint32_t size, uint64_t usage) {
        std::lock_guard<std::mutex> lock(mMutex);
        int found = -1;
        uint32_t tmpSize = UINT32_MAX;
        for (int i = 0; i < mCachedBufferPool.size(); i++) {
            uint32_t bufSize = mCachedBufferPool[i]->bufferSize();
            uint64_t bufUsage = mCachedBufferPool[i]->bufferUsage();
            if (bufUsage == usage && bufSize >= size && bufSize < tmpSize) {
                found = i;
                tmpSize = bufSize;
            }
        }
        if (found != -1) {
            std::unique_ptr<BufferPool> pool = std::move(mCachedBufferPool[found]);
            mCachedBufferPool.erase(mCachedBufferPool.begin() + found);
            return pool;
        }
        return nullptr;
    }

    void put(std::unique_ptr<BufferPool> pool) {
        std::lock_guard<std::mutex> lock(mMutex);
        mCachedBufferPool.push_back(std::move(pool));
        if (mCachedBufferPool.size() > mCacheDepth) {
            // we always push the newest one at the back,
            // so the oldest pool should be the first item of the vector.
            auto oldest = mCachedBufferPool.begin();
            mCachedBufferPool.erase(oldest);
        }
    }

private:
    const int mCacheDepth = 2;
    std::mutex mMutex;
    std::vector<std::unique_ptr<BufferPool>> mCachedBufferPool;
};

} // namespace sunxi
#endif
