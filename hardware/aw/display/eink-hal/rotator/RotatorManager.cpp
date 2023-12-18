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

#include "BufferPool.h"
#include "Debug.h"
#include "HardwareRotator.h"
#include "Layer.h"
#include "RotatorManager.h"
#include "private_handle.h"

#include <android-base/stringprintf.h>
using android::base::StringPrintf;

namespace sunxi {


class RotatorManager::LayerDestroyedListener: public Layer::DestroyedListener {
public:
    explicit LayerDestroyedListener(RotatorManager* m) : mRotatedManager(m) { }
    void onLayerDestroyed(hwc2_layer_t id) override {
        if (mRotatedManager)
            mRotatedManager->removeRotateSession(id);
    }
private:
    RotatorManager* const mRotatedManager;
};

RotatorManager::RotatorManager()
    : mLayerDestroyedListener(new LayerDestroyedListener(this)),
      mRotateSession(),
      mBufferPoolCache(new BufferPoolCache())
{

}

RotatorManager::~RotatorManager()
{
    mRotateSession.clear();
}

int RotatorManager::tryUsingHardwareRotator(const std::shared_ptr<Layer>& layer)
{
    if (layer == nullptr || layer->bufferHandle() == nullptr) {
        return -1;
    }

    HardwareRotator& hwRotator = HardwareRotator::getInstance();
    if (!hwRotator.capable(layer)) {
        // can not handle by hardware rotator.
        return -1;
    }

    std::lock_guard<std::mutex> lock(mSessionLock);
    hwc2_layer_t id = layer->id();
    RotateSession* session = getSessionById(id);
    if (session == nullptr) {
        DTRACE_INT("add-rotate-session", (int32_t)id);
        // create a session for the new incoming layer
        session = new RotateSession();
        session->setBufferPoolCache(mBufferPoolCache.get());
        std::unique_ptr<RotateSession> uptr(session);
        mRotateSession.emplace(id, std::move(uptr));
        /*
         * register layer destroy listener,
         * so that we can remove the corresponding session after layer destroyed.
         */
        layer->registerLayerDestroyListener(mLayerDestroyedListener);
    }

    return session->createAsyncRotateTask(layer);
}

int RotatorManager::getRotatedBuffer(
        const std::shared_ptr<Layer>& layer, const OutputBuffer_t** buffer, int *acquireFence)
{
    std::lock_guard<std::mutex> lock(mSessionLock);
    RotateSession* session = getSessionById(layer->id());
    if (session == nullptr) {
        return -1;
    }
    return session->getRotatedBuffer(buffer, acquireFence);
}

void RotatorManager::postRotateTask()
{
    std::lock_guard<std::mutex> lock(mSessionLock);
    for (auto& item : mRotateSession) {
        std::unique_ptr<RotateSession>& session = item.second;
        if (session->active()) {
            session->postRotateTask();
        }
    }
}

void RotatorManager::postCommit(int releaseFenceFd)
{
    std::lock_guard<std::mutex> lock(mSessionLock);
    uniquefd fence(releaseFenceFd);
    for (auto& item : mRotateSession) {
        std::unique_ptr<RotateSession>& session = item.second;
        if (session->active()) {
            session->putRotatedBuffer(fence.dup());
        }
    }
}

void RotatorManager::removeRotateSession(hwc2_layer_t id)
{
    std::lock_guard<std::mutex> lock(mSessionLock);
    DTRACE_INT("remove-rotate-session", (int32_t)id);
    mRotateSession.erase(id);
}

RotateSession* RotatorManager::getSessionById(hwc2_layer_t id) {
    auto iter = mRotateSession.find(id);
    return iter == mRotateSession.end() ? nullptr : iter->second.get();
}

void RotatorManager::dump(std::string& out) {
    std::lock_guard<std::mutex> lock(mSessionLock);
    int sessionCount = mRotateSession.size();
    int pendingTaskCount = HardwareRotator::getInstance().getPendingTaskCount();
    out += StringPrintf("toatal RotateSession: %d, total pending task: %d\n", sessionCount, pendingTaskCount);
}

//---------------------------------------------------------------------------------//

RotateSession::RotateSession()
    : mBufferPool(nullptr), mRotateTask(nullptr), mBufferPoolCache(nullptr),
      mAllocatedBufferWidth(0), mAllocatedBufferHeight(0), mAllocatedBufferPixelFormat(0),
      mRotateFailedCount(0)
{

}

RotateSession::~RotateSession()
{
    if (mBufferPoolCache && mBufferPool) {
        mBufferPoolCache->put(std::move(mBufferPool));
    }
    mBufferPool.reset();
    mRotateTask.reset();
}

int RotateSession::createAsyncRotateTask(const std::shared_ptr<Layer>& layer)
{
    if (mRotateFailedCount >= 3) {
        DLOGE_IF(kTagRotate, "RotateTask failed more than 3 times, skip hardware rotate");
        return -1;
    }

    if (mRotateTask) {
        // when layer assign process has been retry, the mRotateTask has create on
        // the first assignment.
        DLOGD_IF(kTagRotate, "RotateTask has beed setup before");
        return 0;
    }

    const private_handle_t* handle = from(layer->bufferHandle());
    int w = handle->width;
    int h = handle->height;
    int align;
    // the output buffer should be aligned!
    HardwareRotator::getInstance().getAlignedWidthAndHeight(layer, &w, &h, &align);
    DLOGD_IF(kTagRotate, "rotate buffer size: %dx%d align %d %d %d stride %d align size: %dx%d",
            handle->width, handle->height,
            handle->aw_byte_align[0],
            handle->aw_byte_align[1],
            handle->aw_byte_align[2],
            handle->stride, w, h);

    hwc_transform_t transform = static_cast<hwc_transform_t>(layer->transform());
    if (transform & HAL_TRANSFORM_ROT_90) {
        std::swap(w, h);
    }

    DLOGD_IF(kTagRotate, "rotate buffer input layer size: %dx%d align size: %dx%d",
            handle->width, handle->height, w, h);

    if (mBufferPool == nullptr) {
        char name[32];
        sprintf(name, "BufferPool.%" PRIu64, layer->id());
        DTRACE_SCOPED(name);
        uint64_t usage = GRALLOC_USAGE_HW_2D;
        uint32_t size = HardwareRotator::getInstance().computeBufferSize(w, h, align, handle->format);

        // if source buffer is secure, the output buffer must be secure too !
        if (handle->usage & GRALLOC_USAGE_PROTECTED) {
            usage |= GRALLOC_USAGE_PROTECTED;
        }

        // try get BufferPool from cache first.
        if (mBufferPoolCache) {
            mBufferPool = mBufferPoolCache->get(size, usage);
            DLOGD_IF(kTagRotate, "get cached BufferPool:%p size:%d request size:%d",
                    mBufferPool.get(),
                    mBufferPool ? mBufferPool->bufferSize() : 0, size);
        }
        if (!mBufferPool) {
            mBufferPool = std::make_unique<BufferPool>(BUFFER_POOL_DEPTH, size, usage, false);
        }
        mBufferPool->setName(name);
        mAllocatedBufferWidth = w;
        mAllocatedBufferHeight = h;
        mAllocatedBufferPixelFormat = handle->format;
    } else {
        if (mAllocatedBufferWidth != w
                || mAllocatedBufferHeight != h
                || mAllocatedBufferPixelFormat != handle->format) {
            DLOGD_IF(kTagRotate, "reallocate prev size:%dx%d format:%08x --> current size:%dx%d fromat:%08x",
                    mAllocatedBufferWidth, mAllocatedBufferHeight, mAllocatedBufferPixelFormat,
                    w, h, handle->format);
            uint32_t size = HardwareRotator::getInstance().computeBufferSize(
                    w, h, align, handle->format);

            // only reallocate when the request size is bigger the current buffer size.
            if (size > mBufferPool->bufferSize()) {
                std::unique_ptr<BufferPool> cachedBufferPool = nullptr;

                // if source buffer is secure, the output buffer must be secure too !
                uint64_t usage = GRALLOC_USAGE_HW_2D;
                if (handle->usage & GRALLOC_USAGE_PROTECTED) {
                    usage |= GRALLOC_USAGE_PROTECTED;
                }
                if (mBufferPoolCache) {
                    cachedBufferPool = mBufferPoolCache->get(size, usage);
                    DLOGD_IF(kTagRotate, "reallocate get BufferPool:%p size:%d request size:%d",
                            cachedBufferPool.get(),
                            cachedBufferPool.get() ? cachedBufferPool->bufferSize() : 0, size);
                }
                if (cachedBufferPool) {
                    DLOGD_IF(kTagRotate, "replace mBufferPool, current acquireBufferCount: %d",
                            mBufferPool->acquireBufferCount());
                    mBufferPoolCache->put(std::move(mBufferPool));
                    mBufferPool = std::move(cachedBufferPool);
                } else {
                    mBufferPool->reallocate(size);
                }
            }

            mAllocatedBufferWidth = w;
            mAllocatedBufferHeight = h;
            mAllocatedBufferPixelFormat = handle->format;
        }
    }

    std::unique_ptr<OutputBuffer_t> outbuf(new OutputBuffer_t());
    outbuf->width  = w;
    outbuf->height = h;
    outbuf->align  = align;
    outbuf->format = handle->format;
    outbuf->buffer = nullptr;

    int acquireFence;
    int ret = mBufferPool->acquireBuffer(&(outbuf->buffer), &acquireFence);
    if (ret == 0 && outbuf->buffer) {
        outbuf->acquireFence = uniquefd(acquireFence);
        mRotateTask = std::make_shared<RotateTask>(layer, std::move(outbuf), transform);

        // TODO: use weak_ptr for RotateFailedCallback,
        //       to protect from calling back into a destructed RotateSession object
        RotateFailedCallback cb = [this]() {
            this->onRotateFailed();
        };
        mRotateTask->setRotateFailedCallback(cb);
        return 0;
    }
    return -1;
}

void RotateSession::postRotateTask()
{
    if (mRotateTask == nullptr) {
        DLOGW("no active rotate task");
        return;
    }
    HardwareRotator& hwRotator = HardwareRotator::getInstance();
    hwRotator.postRotateTask(mRotateTask);
}

int RotateSession::getRotatedBuffer(const OutputBuffer_t** buffer, int* acquireFence)
{
    if (mRotateTask == nullptr) {
        DLOGW("no active rotate task");
        *buffer = nullptr;
        *acquireFence = -1;
        return -1;
    }
    int ret = mRotateTask->getRotatedBuffer(buffer, acquireFence);
    return ret;
}

int RotateSession::putRotatedBuffer(int releaseFence)
{
    const PrivateBuffer_t* buf = mRotateTask->outputBuffer();
    mBufferPool->releaseBuffer(buf, releaseFence);
    mRotateTask.reset();
    return 0;
}

} // namespace sunxi
