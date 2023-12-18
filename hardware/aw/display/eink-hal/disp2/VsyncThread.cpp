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

#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>
#include<chrono>
#include <time.h>
#include <errno.h>


#include "Debug.h"
#include "uniquefd.h"
#include "VsyncThread.h"

namespace sunxi {

VsyncThread::VsyncThread()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mThread = std::thread(&VsyncThread::threadLoop, this);
    pthread_setname_np(mThread.native_handle(), "VsyncThread");
    mThread.detach();
}

void VsyncThread::addEventHandler(int id, EventHandler *handler)
{
    DLOGI_IF(kTagVSync, "add event handler(%p)", handler);
    std::unique_lock<std::mutex> lock(mMutex);
    mEventHandlers.insert_or_assign(id, handler);
    mCondition.notify_all();
}

void VsyncThread::removeEventHandler(int id)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mEventHandlers.count(id)) {
        DLOGI_IF(kTagVSync, "remove event handler(%p)", mEventHandlers[id]);
        mEventHandlers.erase(id);
    }
}

void VsyncThread::handleVsyncEvent(int disp, int64_t timestamp)
{
    DLOGI_IF(kTagVSync, "display(%d) timestamp %lld", disp, timestamp);
    std::unique_lock<std::mutex> lock(mMutex);
	EventHandler *handler = mEventHandlers[disp];
	handler->onVsync(disp, timestamp);
}

void VsyncThread::threadLoop()
{
    DLOGI("VsyncThread start");


    while (1) {
        { // scope of the mMutex
            std::unique_lock<std::mutex> lock(mMutex);
            if (mEventHandlers.empty()) {
                mCondition.wait(lock);
            }
        }

		struct timespec now = {0, 0};

		std::this_thread::sleep_for(std::chrono::nanoseconds(mEventHandlers[0]->getVsyncPeriodInNs()));
		if(clock_gettime(CLOCK_MONOTONIC, &now))
			DLOGI("clock_gettime fail:%d", errno);
		else
			handleVsyncEvent(0, now.tv_nsec);
    }
}



} // namespace suxni

