/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef FENCE_MONITOR_H
#define FENCE_MONITOR_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <string>

#include <log/log.h>
#include <utils/Trace.h>
#include <ui/Fence.h>

#include "uniquefd.h"

namespace sunxi {

using android::Fence;
using android::sp;

class FenceMonitor {
public:
explicit FenceMonitor(const char* name)
        : mName(name), mFencesQueued(0), mFencesSignaled(0) {
    std::thread thread(&FenceMonitor::loop, this);
    pthread_setname_np(thread.native_handle(), name);
    thread.detach();
}

void queueFence(int fd) {
    char message[64];
    std::lock_guard<std::mutex> lock(mMutex);

    sp<Fence> fence = new Fence(fd);
    if (fence->getSignalTime() != Fence::SIGNAL_TIME_PENDING) {
        snprintf(message, sizeof(message), "%s fence %u has signaled", mName.c_str(), mFencesQueued);
        ATRACE_NAME(message);
        // Need an increment on both to make the trace number correct.
        mFencesQueued++;
        mFencesSignaled++;
        return;
    }
    snprintf(message, sizeof(message), "Trace %s fence %u", mName.c_str(), mFencesQueued);
    ATRACE_NAME(message);

    mQueue.push_back(fence);
    mCondition.notify_one();
    mFencesQueued++;
    ATRACE_INT(mName.c_str(), int32_t(mQueue.size()));
}

private:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    void loop() {
        while (true) {
            threadLoop();
        }
    }
#pragma clang diagnostic pop

    void threadLoop() {
        sp<Fence> fence;
        uint32_t fenceNum;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            while (mQueue.empty()) {
                mCondition.wait(lock);
            }
            fence = mQueue[0];
            fenceNum = mFencesSignaled;
        }
        {
            char message[64];
            snprintf(message, sizeof(message), "waiting for %s %u", mName.c_str(), fenceNum);
            ATRACE_NAME(message);

            android::status_t result = fence->waitForever(message);
            if (result != android::OK) {
                ALOGE("Error waiting for fence: %d", result);
            }
        }
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mQueue.pop_front();
            mFencesSignaled++;
            ATRACE_INT(mName.c_str(), int32_t(mQueue.size()));
        }
    }

    std::string mName;
    uint32_t mFencesQueued;
    uint32_t mFencesSignaled;
    std::deque<sp<Fence>> mQueue;
    std::condition_variable mCondition;
    std::mutex mMutex;
};

// Singleton wrapper class to FenceMonitor
class FenceDebugger {
public:
    static void queueFence(int displayid, int fencefd) {
        static FenceDebugger _debugger;
        _debugger.addFence(displayid, fencefd);
    }

private:
    FenceDebugger() : mMonitors() { }
   ~FenceDebugger() = default;

    void addFence(int displayid, int fencefd) {
        if (mMonitors.count(displayid) == 0) {
            char name[32] = {0};
            sprintf(name, "display-release-%d", displayid);
            mMonitors.emplace(displayid, std::make_unique<FenceMonitor>(name));
        }
        std::unique_ptr<FenceMonitor>& m = mMonitors[displayid];
        m->queueFence(fencefd);
    }

    std::map<int, std::unique_ptr<FenceMonitor>> mMonitors;
};

} // namespace sunxi

#endif
