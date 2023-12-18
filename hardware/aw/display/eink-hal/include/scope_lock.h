
#include <mutex>
#include <condition_variable>

#ifndef __SCOPE_LOCK_H__
#define __SCOPE_LOCK_H__

class ScopeLock {
public:
    ScopeLock()
      : mLocking(false) {}
   ~ScopeLock() {}

    void acquire() { mLocking = true; }
    void release() { mLocking = false; mCondition.notify_all(); }
    void wait(std::unique_lock<std::mutex>& lock) { if (mLocking) mCondition.wait(lock); }

private:
    std::atomic<bool> mLocking;
    std::condition_variable mCondition;
};

#endif
