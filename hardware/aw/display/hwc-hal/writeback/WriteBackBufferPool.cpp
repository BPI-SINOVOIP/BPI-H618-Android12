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

#include <memory>
#include <mutex>
#include <thread>

#include <utils/RefBase.h>

#include "Debug.h"
#include "uniquefd.h"
#include "syncfence.h"
#include "IonBuffer.h"
#include "WriteBackBufferPool.h"
#include "WriteBackDef.h"

namespace sunxi {

    WriteBackBuffer::WriteBackBuffer(uint32_t size, uint64_t usage) {
        std::unique_ptr<IonBuffer> buf(new IonBuffer(size, usage));
        private_handle_t* handle = (private_handle_t *)malloc(sizeof(private_handle_t));
        std::shared_ptr<private_handle_t> buffer(handle, buffer_deleter);
        mHandle = std::shared_ptr<private_handle_t>(buffer);
        if (buf->isValid() && mHandle != nullptr) {
            mIonBuffer = std::move(buf);
            mHandle->share_fd = mIonBuffer->getPrivateBuffer()->handle;
        } else {
            DLOGE(" allocate failed");
        }
        mLayer = std::make_shared<Layer>();
        mLayer->setLayerBuffer((buffer_handle_t)mHandle.get(), -1);
    }

    WriteBackBuffer::~WriteBackBuffer()
    {
        DLOGD("used lyr=%d, hdl=%d", mLayer.use_count(), mHandle.use_count());
        mLayer.reset();
        mHandle->share_fd = -1;
        mHandle.reset();
        mIonBuffer.release();
    }

    std::shared_ptr<Layer> WriteBackBuffer::getLayer()
    {
        return mLayer;
    }

    unsigned int WriteBackBuffer::getToken()
    {
        return mToken;
    }

    void WriteBackBuffer::setToken(unsigned int token)
    {
        mToken = token;
    }

    void WriteBackBuffer::setState(State s)
    {
        mState = s;
    }

    State WriteBackBuffer::getState()
    {
        return mState;
    }

    void WriteBackBuffer::setFence(int fence)
    {
        mFence = fence;
    }

    int WriteBackBuffer::getFence()
    {
        return mFence;
    }

    void WriteBackBuffer::setSyncCnt(unsigned int sync)
    {
        mSyncCnt = sync;
    }

    unsigned int WriteBackBuffer::getSyncCnt()
    {
        return mSyncCnt;
    }

    struct AllocateTask {
        WriteBackBufferPool* mBufferPool;
        AllocateTask (WriteBackBufferPool* pool) {
            mBufferPool = pool;
        };
    };

    WriteBackBufferPool::WriteBackBufferPool(int count, uint32_t size, uint64_t usage,
                                             bool allocateImmediately = true)
        : mBuffers(), mAllocated(false), mBufferCount(count), mSize(size),
        mUsage(usage), mSyncAllocateMode(allocateImmediately)
    {
        if (!allocateImmediately) {
            mThread = std::thread(&WriteBackBufferPool::threadLoop, this);
            pthread_setname_np(mThread.native_handle(), "WbBufferThread");
            mThread.detach();
        }
        allocateBuffer();
    }

    WriteBackBufferPool::~WriteBackBufferPool()
    {
        mBuffers.clear();
    }

    void WriteBackBufferPool::allocateBuffer()
    {
        if (mSyncAllocateMode) {
            std::lock_guard<std::mutex> lock(mMutex);
            mBuffers.clear();
            for (int i = 0; i < mBufferCount;) {
                std::shared_ptr<WriteBackBuffer> buf
                    = std::make_shared<WriteBackBuffer>(mSize, mUsage);
                if (buf != nullptr) {
                    mBuffers.push_back(buf);
                    i++;
                } else {
                    DLOGE("allocate failed");
                }
                if (i > 10 * mBufferCount) {
                    DLOGE("allocate failed too much");
                    break;
                }
            }
            DLOGD("mBufferCount=%d, size=%d", mBufferCount, mBuffers.size());
            for (int i = 0; i < mBufferCount; i++) {
                if (mBuffers[i] == nullptr) {
                    DLOGE("bad buffer!");
                    continue;
                }
                mBuffers[i]->setToken(i);
                mBuffers[i]->setFence(-1);
                mBuffers[i]->setState(State::FREE);
            }
            mAllocated = true;
        } else {
            std::unique_ptr<AllocateTask> task(new AllocateTask(this));
            postAsyncAllocateTask(std::move(task));
        }
    }

    void WriteBackBufferPool::reallocate(uint32_t size)
    {
        DTRACE_FUNC();
        {
            // buffer geometry change, reallocate every buffer.
            std::lock_guard<std::mutex> lock(mMutex);
            mBuffers.clear();
            mAllocated = false;
        }
        allocateBuffer();
    }

    std::shared_ptr<WriteBackBuffer> WriteBackBufferPool::getBufByLyr(std::shared_ptr<Layer> lyr)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated) {
            DLOGE("buffers not ready");
            return nullptr;
        }
        for (int i = 0; i < mBuffers.size(); i++) {
            if (mBuffers[i]->getLayer() == lyr) {
                return mBuffers[i];
            }
        }
        return nullptr;
    }

    std::shared_ptr<WriteBackBuffer> WriteBackBufferPool::getLastRelBuffer(int* count)
    {
        int last = -1;
        int freeCnt = 0;
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated) {
            *count = 0;
            DLOGE("buffers not ready");
            return nullptr;
        }
        for (int i = 0; i < mBuffers.size(); i++) {
            if (mBuffers[i]->getState() == State::RELEASED) {
                freeCnt++;
                if (last < 0) {
                    last = i;
                } else if (mBuffers[last]->getToken()
                           > mBuffers[i]->getToken()) {
                    last = i;
                }
            }
            DLOGD("fen=%d,toke=%d,state=%d", mBuffers[i]->getFence(),
                  mBuffers[i]->getToken(), mBuffers[i]->getState());
        }
        /*if (freeCnt > MINCACHE) {
            int first = -1, sec = -1;
            for (int i = 0; i < mBuffers.size(); i++) {
                if (mBuffers[i]->getState() == State::FREE
                    && mBuffers[i]->getFence() > 0
                    && (mBuffers[last]->getToken()
                    < mBuffers[i]->getToken())) {
                    if (first < 0) {
                        first = i;
                    } else if (mBuffers[first]->getToken()
                               > mBuffers[i]->getToken()) {
                        first = i;
                    }
                }
            }
            for (int i = 0; i < mBuffers.size(); i++) {
                if (mBuffers[i]->getState() == State::FREE
                    && mBuffers[i]->getFence() > 0
                    && (mBuffers[first]->getToken()
                    < mBuffers[i]->getToken())) {
                    if (sec < 0) {
                        sec = i;
                    } else if (mBuffers[sec]->getToken()
                               > mBuffers[i]->getToken()) {
                        sec = i;
                    }
                }
            }
            close(mBuffers[last]->getFence());
            first = dup(mBuffers[first]->getFence());
            mBuffers[last]->setFence(first);
        }*/
        if (last < 0) {
            DLOGD("no free buffer");
            return nullptr;
        }
        *count = freeCnt;
        return mBuffers[last];
    }

    std::shared_ptr<WriteBackBuffer> WriteBackBufferPool::acquireBuffer(int deep)
    {
        int lastQueued = -1;
        int queuedCnt = 0;
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated) {
            DLOGE("buffers not ready");
            return nullptr;
        }

        if (deep > 0 && deep > mBuffers.size()) {
            DLOGW("buffers.size() = %d, deep=%d", mBuffers.size(), deep);
        }

        /*find a queued buffer*/
        for (int i = 0; i < mBuffers.size(); i++) {
            if (mBuffers[i]->getState() == State::QUEUED) {
                queuedCnt++;
                if (lastQueued < 0) {
                    lastQueued = i;
                } else if (mBuffers[i]->getToken()
                           < mBuffers[lastQueued]->getToken()) {
                    lastQueued = i;
                }
            }
        }

        if (lastQueued < 0 || (deep > 0 && queuedCnt <= deep)) {
            return nullptr;
        }

        /*let fence shit forward two*/
        if (deep > 0 && queuedCnt > deep) {
            int first = -1, sec = -1;
            for (int i = 0; i < mBuffers.size(); i++) {
                if (mBuffers[i]->getState() == State::QUEUED
                    && (mBuffers[i]->getToken()
                        > mBuffers[lastQueued]->getToken())) {
                    if (first < 0) {
                        first = i;
                    } else if (mBuffers[i]->getToken()
                               < mBuffers[first]->getToken()) {
                        first = i;
                    }
                }
            }
            deep--;
            while (deep-- > 0) {
                for (int i = 0; i < mBuffers.size(); i++) {
                    if (mBuffers[i]->getState() == State::QUEUED
                        && (mBuffers[i]->getToken()
                            > mBuffers[first]->getToken())) {
                        if (sec < 0) {
                            sec = i;
                        } else if (mBuffers[i]->getToken()
                                   < mBuffers[sec]->getToken()) {
                            sec = i;
                        }
                    }
                }
                first = sec;
                sec = -1;
            }
            if (first >= 0) {
                sec = dup(mBuffers[first]->getLayer()->acquireFence());
                mBuffers[lastQueued]->getLayer()->setAcquireFence(sec);
            }
        }
        mBuffers[lastQueued]->setState(State::ACQUIRED);
        return mBuffers[lastQueued];

    }

    int WriteBackBufferPool::releaseBuffer(std::shared_ptr<WriteBackBuffer> buf,
            bool clearFence)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated || buf == nullptr) {
            DLOGE("bad para");
            return -1;
        }

        if (clearFence) {
            if (buf->getFence() > 0)
                close(buf->getFence());
            buf->setFence(-1);
            buf->setState(State::FREE);
        } else {
            buf->setState(State::RELEASED);
        }
        return 0;
    }

    int WriteBackBufferPool::setFence(std::shared_ptr<WriteBackBuffer> buf, int fence)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated || buf == nullptr) {
            return -1;
        }

        buf->setFence(fence);
        return 0;
    }

    std::shared_ptr<WriteBackBuffer> WriteBackBufferPool::dequeueBuffer()
    {
        int lastFree = -1;
        unsigned int firstToken = 0;
        int queuedCnt = 0;
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated) {
            DLOGE("buffers not ready");
            return nullptr;
        }

        /*find a free buffer*/
        for (int i = 0; i < mBuffers.size(); i++) {
            if (mBuffers[i]->getToken() > firstToken)
                firstToken = mBuffers[i]->getToken();
            if (mBuffers[i]->getState() == State::FREE) {
                if (lastFree < 0) {
                    lastFree = i;
                } else if (mBuffers[i]->getToken()
                           < mBuffers[lastFree]->getToken()) {
                    lastFree = i;
                }
            }
            if (mBuffers[i]->getState() == State::QUEUED) {
                queuedCnt++;
            }
        }

        /*a queued buf is ok too when no free buffer*/
        if (lastFree < 0) {
            int recycle = -1;
            for (int i = 0; i < mBuffers.size(); i++) {
                if (mBuffers[i]->getState() == State::QUEUED) {
                    if (lastFree < 0) {
                        lastFree = i;
                    } else if (mBuffers[i]->getToken()
                               < mBuffers[lastFree]->getToken()) {
                        lastFree = i;
                    }
                }
            }
            if (lastFree >= 0) {
                if (mBuffers[lastFree]->getFence() > 0)
                    close(mBuffers[lastFree]->getFence());
                mBuffers[lastFree]->setFence(-1);
            }
            for (int i = 0; i < mBuffers.size(); i++) {
                if (mBuffers[i]->getState() == State::QUEUED
                    && (mBuffers[i]->getToken() >
                    mBuffers[lastFree]->getToken())) {
                    if (recycle < 0) {
                        recycle  = i;
                    } else if (mBuffers[i]->getToken()
                               < mBuffers[recycle]->getToken()) {
                        recycle = i;
                    }
                }
            }
            if (recycle >= 0) {
                if (mBuffers[recycle]->getFence() > 0)
                    close(mBuffers[recycle]->getFence());
                mBuffers[recycle]->setFence(-1);
            }
        } else if (lastFree > 0 && queuedCnt > mBuffers.size() - 2) {
            int recycle = -1;
            for (int i = 0; i < mBuffers.size(); i++) {
                if (mBuffers[i]->getState() == State::QUEUED) {
                    if (recycle < 0) {
                        recycle  = i;
                    } else if (mBuffers[i]->getToken()
                               < mBuffers[recycle]->getToken()) {
                        recycle = i;
                    }
                }
            }
            if (recycle >= 0) {
                if (mBuffers[recycle]->getFence() > 0)
                    close(mBuffers[recycle]->getFence());
                mBuffers[recycle]->setFence(-1);
            }
        }
        if (lastFree < 0) {
            DLOGE("no free buffer");
            return nullptr;
        }

        /*set buffer token for loop*/
        mBuffers[lastFree]->setToken(firstToken + 1);
        mBuffers[lastFree]->setState(State::DEQUEUED);

        /*pretect token from overflowing*/
        if (firstToken > (UINT_MAX - 10 * mBuffers.size())) {
            for (int i = 0; i < mBuffers.size(); i++) {
                unsigned int token = mBuffers[i]->getToken();
                if (token  < (UINT_MAX - 20 * mBuffers.size())) {
                    DLOGD("bad token=%d", token);
                } else {
                    token = token - (UINT_MAX - 20 * mBuffers.size());
                    mBuffers[i]->setToken(token);
                    DLOGD("reset token to=%d", token);
                }
            }
        }

        return mBuffers[lastFree];
    }

    int WriteBackBufferPool::queueBuffer(std::shared_ptr<WriteBackBuffer> buf, int fence)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAllocated || buf == nullptr) {
            DLOGE("nullptr buft");
            return -1;
        }
        if (fence > 0) {
            buf->getLayer()->setAcquireFence(fence);
        }
        buf->setState(State::QUEUED);
        return 0;
    }

    void WriteBackBufferPool::setName(const char* name)
    {
        mName = std::string(name);
    }

    uint32_t WriteBackBufferPool::bufferSize() const
    {
        return mSize;
    }

    uint64_t WriteBackBufferPool::bufferUsage() const
    {
        return mUsage;
    }

    void WriteBackBufferPool::onBufferAllocated()
    {
        for (int i = 0; i < mBufferCount; i++) {
            DTRACE_FUNC();
            std::lock_guard<std::mutex> lock(mMutex);
            mBuffers.push_back(std::make_shared<WriteBackBuffer>(mSize, mUsage));
            mBuffers[i]->setToken(i);
            mBuffers[i]->setFence(-1);
            mBuffers[i]->setState(State::FREE);
        }
        mAllocated = true;
    }

    void WriteBackBufferPool::postAsyncAllocateTask(std::unique_ptr<AllocateTask> task)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mAllocateTasks.push_back(std::move(task));
        mCondition.notify_all();
    }

    void WriteBackBufferPool::threadLoop()
    {
        DLOGI("BufferAllocateThread start.");
        while (true) {
            std::vector<std::unique_ptr<AllocateTask>> pendingTasks;
            { // scope of the mMutex
                std::unique_lock<std::mutex> lock(mMutex);
                if (mAllocateTasks.empty()) {
                    mCondition.wait(lock);
                }
                pendingTasks.swap(mAllocateTasks);
            }

            for (int i = 0; i < pendingTasks.size(); i++) {
                auto& task = pendingTasks[i];
                if (task != nullptr) {
                    task->mBufferPool->onBufferAllocated();
                }
            }
            pendingTasks.clear();
        }
    }

} // namespace sunxi


