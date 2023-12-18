
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

#ifndef UEVENT_THREAD_H
#define UEVENT_THREAD_H

#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace sunxi {

// singleton vsync event listener for all display
class VsyncThread {
public:
    static VsyncThread* getInstance() {
        static VsyncThread _vsyncThreadInstance;
        return &_vsyncThreadInstance;
    }

   ~VsyncThread() = default;

    class EventHandler {
    public:
        virtual ~EventHandler() = default;
        virtual void onVsync(int disp, int64_t timestamp) = 0;
        virtual int getVsyncPeriodInNs(void) = 0;
    };

    void addEventHandler(int id, EventHandler *handler);
    void removeEventHandler(int id);

private:
    VsyncThread();
    void threadLoop();
    void handleVsyncEvent(int disp, int64_t timestamp);

    std::map<int, EventHandler*> mEventHandlers;
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::thread mThread;
};

} // namespace sunxi

#endif
