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
#include "BufferPool.h"
#include "uniquefd.h"
#include "syncfence.h"
#include "IonBuffer.h"

namespace sunxi {

class BufferPoolImpl;

struct AllocateTask {
    std::shared_ptr<BufferPoolImpl> mBufferPool;
};

class BufferAllocateThread {
public:
    BufferAllocateThread();
   ~BufferAllocateThread() = default;
    void postAsyncAllocateTask(std::unique_ptr<AllocateTask> task);

private:
    void threadLoop();
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::thread mThread;
    std::vector<std::unique_ptr<AllocateTask>> mAllocateTasks;
};

//---------------------------------------------------------------------------------//

class BufferPoolImpl: public std::enable_shared_from_this<BufferPoolImpl> {
public:
    BufferPoolImpl()
        : mAllocated(false), mSlots(), mBufferCount(1), mSize(0),
          mUsage(0), mGenerationNumber(0), mAcquireBufferCount(0) { }
   ~BufferPoolImpl() { mSlots.clear(); }

    void allocateBuffer(int count, uint32_t size,
            uint64_t usage, bool allocateImmediately) {
        mBufferCount = count;
        mSize = size;
        mUsage = usage;
        mSyncAllocateMode = allocateImmediately;

        if (allocateImmediately) {
            // sync mode
            std::lock_guard<std::mutex> lock(mMutex);
            mSlots.resize(count);
            for (int i = 0; i < count; i++) {
                std::unique_ptr<IonBuffer> buf(new IonBuffer(mSize, usage));
                if (buf->isValid()) {
                    mSlots[i].mGraphicBuffer = std::move(buf);
                    mSlots[i].mBusy = false;
                }
            }
            mAllocated = true;
        } else {
            // async mode: allocate buffer on BufferAllocateThread
            static BufferAllocateThread __allocateThread;
            std::unique_ptr<AllocateTask> task(new AllocateTask());
            task->mBufferPool = shared_from_this();
            __allocateThread.postAsyncAllocateTask(std::move(task));
        }
    }

    void reallocate(uint32_t size) {
        DTRACE_FUNC();
        {
            // buffer geometry change, reallocate every buffer.
            std::lock_guard<std::mutex> lock(mMutex);
            mAllocated = false;
            for (int i = 0; i < mSlots.size(); i++) {
                if (mSlots[i].mBusy) {
                    // move the busy buffer to pending list,
                    // we will delete it when the slot has been released by caller.
                    mPendingReleaseBuffers.push_back(std::move(mSlots[i].mGraphicBuffer));
                }
            }
            mSlots.clear();
        }
        allocateBuffer(mBufferCount, size, mUsage, mSyncAllocateMode);
    }

	int getFreeBufNo()
	{
        std::lock_guard<std::mutex> lock(mMutex);
		int freeBufNo = 0;

        if (!mAllocated) {
            // async allocate mode, buffer not allocate yet!
            return -1;
        }

        for (int i = 0; i < mSlots.size(); i++) {
            if (mSlots[i].mGraphicBuffer != nullptr && !mSlots[i].mBusy) {
				freeBufNo++;
            }
        }

		return freeBufNo;

	}

    int acquireBuffer(const PrivateBuffer_t** buf, int* acquireFence) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mAllocated) {
            // async allocate mode, buffer not allocate yet!
            return -1;
        }

        int found = -1;
        uint64_t generation = UINT64_MAX;
        for (int i = 0; i < mSlots.size(); i++) {
            if (mSlots[i].mGraphicBuffer != nullptr && !mSlots[i].mBusy) {
                if (mSlots[i].mGenerationNumber < generation) {
                    generation = mSlots[i].mGenerationNumber;
                    found = i;
                }
            }
        }
        if (found != -1) {
            DTRACE_SCOPED((mName + "#" + std::to_string(found)).c_str());
            BufferSlot& slot = mSlots[found];
            slot.mBusy = true;
            slot.mGenerationNumber = ++mGenerationNumber;
            mAcquireBufferCount++;
            *buf = slot.mGraphicBuffer->getPrivateBuffer();
            *acquireFence = slot.mReleaseFence.dup();
            return 0;
        }
        return -1;
    }

    void release(const PrivateBuffer_t* buf, int releaseFence) {
        std::lock_guard<std::mutex> lock(mMutex);
        bool released = false;
        uniquefd fence(releaseFence);
        for (int i = 0; i < mSlots.size(); i++) {
            if (mSlots[i].mGraphicBuffer->getPrivateBuffer() == buf) {
                DTRACE_SCOPED((mName + "#" + std::to_string(i)).c_str());
                mSlots[i].mBusy = false;
                mSlots[i].mReleaseFence = std::move(fence);
                released = true;
                mAcquireBufferCount--;
                break;
            }
        }
        if (!released) {
            const auto iter = mPendingReleaseBuffers.begin();
            while (iter != mPendingReleaseBuffers.end()) {
                if ((*iter)->getPrivateBuffer() == buf) {
                    mPendingReleaseBuffers.erase(iter);
                    mAcquireBufferCount--;
                    break;
                }
            }
        }
    }

    void setName(const char* name) {
        mName = std::string(name);
    }

    int acquireBufferCount() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mAcquireBufferCount;
    }

    uint32_t bufferSize() const {
        return mSize;
    }

    uint64_t bufferUsage() const {
        return mUsage;
    }

    friend class BufferAllocateThread;
private:
    void onBufferAllocated() {
        for (int i = 0; i < mBufferCount; i++) {
            DTRACE_FUNC();
            std::unique_ptr<IonBuffer> buf(new IonBuffer(mSize, mUsage));
            if (!buf->isValid()) {
                // allocate failed!
                continue;
            }
            BufferSlot slot;
            slot.mGraphicBuffer = std::move(buf);
            slot.mBusy = false;
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mSlots.push_back(std::move(slot));
            }
        }
        mAllocated = true;
    }

    struct BufferSlot {
        std::unique_ptr<IonBuffer> mGraphicBuffer;
        uniquefd mReleaseFence;
        bool mBusy;
        uint64_t mGenerationNumber;
        BufferSlot(): mGraphicBuffer(nullptr), mBusy(false), mGenerationNumber(0) { }
        BufferSlot(BufferSlot&& other) {
            mGenerationNumber = other.mGenerationNumber;
            mBusy = other.mBusy;
            mReleaseFence = std::move(other.mReleaseFence);
            mGraphicBuffer = std::move(other.mGraphicBuffer);
            other.mGraphicBuffer = nullptr;
        }
    };

    std::string mName;
    mutable std::mutex mMutex;
    bool mAllocated;
    std::vector<BufferSlot> mSlots;
    std::vector<std::unique_ptr<IonBuffer>> mPendingReleaseBuffers;

    int mBufferCount;
    uint32_t mSize;
    uint64_t mUsage;
    bool mSyncAllocateMode = true;
    uint64_t mGenerationNumber;
    int mAcquireBufferCount;
};

//---------------------------------------------------------------------------------//

BufferAllocateThread::BufferAllocateThread()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mThread = std::thread(&BufferAllocateThread::threadLoop, this);
    pthread_setname_np(mThread.native_handle(), "BufferAllocateThread");
    mThread.detach();
}

void BufferAllocateThread::postAsyncAllocateTask(std::unique_ptr<AllocateTask> task)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mAllocateTasks.push_back(std::move(task));
    mCondition.notify_all();
}

void BufferAllocateThread::threadLoop()
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


//---------------------------------------------------------------------------------//

BufferPool::BufferPool(
        int count, uint32_t size, uint64_t usage, bool allocateImmediately)
{
    DTRACE_FUNC();
    mBufferPoolImpl = std::make_shared<BufferPoolImpl>();
    mBufferPoolImpl->allocateBuffer(count, size, usage, allocateImmediately);
}

BufferPool::~BufferPool()
{
    DTRACE_FUNC();
    mBufferPoolImpl.reset();
}

void BufferPool::reallocate(uint32_t size)
{
    mBufferPoolImpl->reallocate(size);
}

int BufferPool::getFreeBufNo()
{
    return mBufferPoolImpl->getFreeBufNo();
}

int BufferPool::acquireBuffer(const PrivateBuffer_t** buf, int* acquireFence)
{
    return mBufferPoolImpl->acquireBuffer(buf, acquireFence);
}

void BufferPool::releaseBuffer(const PrivateBuffer_t* buf, int releaseFence)
{
    mBufferPoolImpl->release(buf, releaseFence);
}

void BufferPool::setName(const char* name)
{
    mBufferPoolImpl->setName(name);
}

int BufferPool::acquireBufferCount() const {
    return mBufferPoolImpl->acquireBufferCount();
}

uint32_t BufferPool::bufferSize() const {
    return mBufferPoolImpl->bufferSize();
}

uint64_t BufferPool::bufferUsage() const {
    return mBufferPoolImpl->bufferUsage();
}

} // namespace sunxi


