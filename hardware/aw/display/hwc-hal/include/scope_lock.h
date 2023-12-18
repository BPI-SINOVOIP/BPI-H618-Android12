
#include <mutex>
#include <condition_variable>

#include "Debug.h"

#ifndef __SCOPE_LOCK_H__
#define __SCOPE_LOCK_H__

class ScopeLock {
public:
    ScopeLock()
      : mLocking(false) {}
   ~ScopeLock() {}

    void acquire() { mLocking = true; }
    void release() { mLocking = false; mCondition.notify_all(); }
    void wait(std::unique_lock<std::mutex>& lock) {
        std::chrono::milliseconds timeout(64);
        if (mLocking) {
            if (mCondition.wait_for(lock, timeout) == std::cv_status::timeout) {
                DLOGW("validate but not present?");
            }
        }
    }

private:
    std::atomic<bool> mLocking;
    std::condition_variable mCondition;
};

#endif
