#include "SensorEventCallback.h"

using ::android::hardware::sensors::V2_0::SensorTimeout;

constexpr const char* kWakeLockName = "SensorsHAL_WAKEUP";


SensorEventCallback::SensorEventCallback(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
    const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
    const sp<ISensorsCallback>& sensorsCallback) : mReadWakeLockQueueRun(false),
    mOutstandingWakeUpEvents(0), mHasWakeLock(false), mEventQueueFlag(nullptr) {

    mEventQueue = std::make_unique<EventMessageQueue>(eventQueueDescriptor, true /* resetPointers */);;
    mWakeLockQueue = std::make_unique<WakeLockMessageQueue>(wakeLockDescriptor, true /* resetPointers */);;
    mCallback = sensorsCallback;
    //deleteEventFlag();
    EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag);
}

void SensorEventCallback::deleteEventFlag() {
    android::status_t status = EventFlag::deleteEventFlag(&mEventQueueFlag);
    if (status != android::OK)
        ALOGE("Failed to  delete event flag: %d", status);
}

bool SensorEventCallback::isVaild() {
    if (!mCallback || !mEventQueue || !mWakeLockQueue || !mEventQueueFlag)
        return false;
    return true;
}

void SensorEventCallback::setHasWakeLock(bool hasWake) {
    if (hasWake && !mReadWakeLockQueueRun.load()) {
        mReadWakeLockQueueRun = true;
        mWakeLockThread = std::thread(startReadWakeLockThread, this);
    } else if (!hasWake) {
        mReadWakeLockQueueRun = false;
        //mWakeLockThread.join();
    }
}

void SensorEventCallback::postEvents(const std::vector<Event>& events, bool wakeup) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mEventQueue->write(events.data(), events.size())) {
        mEventQueueFlag->wake(static_cast<uint32_t>(EventQueueFlagBits::READ_AND_PROCESS));
        if (wakeup) {
            updateWakeLock(events.size(), 0);
        }
    }
}

void SensorEventCallback::updateWakeLock(int32_t eventsWritten, int32_t eventsHandled) {
    std::lock_guard<std::mutex> lock(mWakeLockLock);
    int32_t newVal = mOutstandingWakeUpEvents + eventsWritten - eventsHandled;
    if (newVal < 0) {
        mOutstandingWakeUpEvents = 0;
    } else {
        mOutstandingWakeUpEvents = newVal;
    }

    if (eventsWritten > 0) {
        // Update the time at which the last WAKE_UP event was sent
        mAutoReleaseWakeLockTime = ::android::uptimeMillis() + 
            static_cast<uint32_t>(SensorTimeout::WAKE_LOCK_SECONDS) * 1000;
    }

    if (!mHasWakeLock && mOutstandingWakeUpEvents > 0 &&
        acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLockName) == 0) {
        mHasWakeLock = true;
    } else if (mHasWakeLock) {
       // Check if the wake lock should be released automatically if
       // SensorTimeout::WAKE_LOCK_SECONDS has elapsed since the last WAKE_UP event was written to
       // the Wake Lock FMQ.
       if (::android::uptimeMillis() > mAutoReleaseWakeLockTime) {
           ALOGD("No events read from wake lock FMQ for %d seconds, auto releasing wake lock",
                 SensorTimeout::WAKE_LOCK_SECONDS);
           mOutstandingWakeUpEvents = 0;
       }

       if (mOutstandingWakeUpEvents == 0 && release_wake_lock(kWakeLockName) == 0) {
           mHasWakeLock = false;
       }
    }
}

void SensorEventCallback::readWakeLockFMQ() {
    while (mReadWakeLockQueueRun.load()) {
        constexpr int64_t kReadTimeoutNs = 500 * 1000 * 1000;  // 500 ms
        uint32_t eventsHandled = 0;

        // Read events from the Wake Lock FMQ. Timeout after a reasonable amount of time to ensure
        // that any held wake lock is able to be released if it is held for too long.
        mWakeLockQueue->readBlocking(&eventsHandled, 1 /* count */, 0 /* readNotification */,
                                      static_cast<uint32_t>(WakeLockQueueFlagBits::DATA_WRITTEN),
                                      kReadTimeoutNs);
       updateWakeLock(0 /* eventsWritten */, eventsHandled);
    }
}

void SensorEventCallback::startReadWakeLockThread(SensorEventCallback* callback) {
    callback->readWakeLockFMQ();
}
