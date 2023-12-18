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

#ifndef SUNXI_WRITEBACK_BUFFER_POOL_H_
#define SUNXI_WRITEBACK_BUFFER_POOL_H_

#include <stdint.h>
#include <sys/types.h>
#include <vector>
#include <nativebase/nativebase.h>
#include <private_handle.h>
#include "IonBuffer.h"
#include "Layer.h"

namespace sunxi {

    typedef enum {
        FREE = 0,
        DEQUEUED,
        QUEUED,
        ACQUIRED,
        RELEASED,
    } State;

    struct AllocateTask;

    class WriteBackBuffer {
    public:
        WriteBackBuffer(uint32_t size, uint64_t usage);
        ~WriteBackBuffer();

        static void buffer_deleter (private_handle_t* ptr) {
    DLOGW("destroy %p", ptr);
            free(ptr);
        };

        std::shared_ptr<Layer> getLayer();
        unsigned int getToken();
        void setToken(unsigned int token);
        void setFence(int fence);
        int getFence();
        void setSyncCnt(unsigned int sync);
        unsigned int getSyncCnt();
        void setState(State s);
        State getState();

    private:
        std::shared_ptr<Layer> mLayer;
        std::shared_ptr<private_handle_t> mHandle;
        std::unique_ptr<IonBuffer> mIonBuffer;
        unsigned int mToken;
        int mFence;
        State mState;
        unsigned int mSyncCnt;
    };

    class WriteBackBufferPool : public std::enable_shared_from_this<WriteBackBufferPool> {
    public:
        WriteBackBufferPool(int count, uint32_t size, uint64_t usage,
                            bool allocateImmediately);
        ~WriteBackBufferPool();
        void allocateBuffer();
        std::shared_ptr<WriteBackBuffer> acquireBuffer(int deep);
        int releaseBuffer(std::shared_ptr<WriteBackBuffer> buf, bool cf = true);
        std::shared_ptr<WriteBackBuffer> dequeueBuffer();
        int queueBuffer(std::shared_ptr<WriteBackBuffer> buf, int fence = -1);
        std::shared_ptr<WriteBackBuffer> getBufByLyr(std::shared_ptr<Layer> lyr);
        std::shared_ptr<WriteBackBuffer> getLastRelBuffer(int* count);
        int setFence(std::shared_ptr<WriteBackBuffer> buf, int fence);
        void reallocate(uint32_t size);
        void setName(const char* name);
        int acquireBufferCount() const;
        uint32_t bufferSize() const;
        uint64_t bufferUsage() const;
        void postAsyncAllocateTask(std::unique_ptr<AllocateTask> task);

    private:
        void onBufferAllocated();
        std::vector<std::shared_ptr<WriteBackBuffer>> mBuffers;
        bool mAllocated;
        int mBufferCount;
        std::mutex mMutex;
        uint32_t mSize;
        uint64_t mUsage;
        bool mSyncAllocateMode;
        void threadLoop();
        std::string mName;
        std::condition_variable mCondition;
        std::thread mThread;
        std::vector<std::unique_ptr<AllocateTask>> mAllocateTasks;
    };

} // namespace sunxi
#endif
