#ifndef ANDROID_HARDWARE_SENSORS_V2_0_SENSOREVENTCALLBACK_H
#define ANDROID_HARDWARE_SENSORS_V2_0_SENSOREVENTCALLBACK_H

#include <thread>
#include <fmq/MessageQueue.h>
#include <hardware_legacy/power.h>
#include <hidl/MQDescriptor.h>
#include <android/hardware/sensors/2.0/ISensors.h>
#include <android/hardware/sensors/2.0/types.h>

//namespace android {
//namespace hardware {
//namespace sensors {
//namespace V2_0 {
//namespace implementation {
using android::sp;
using android::hardware::MessageQueue;
using Event = android::hardware::sensors::V1_0::Event;
using EventMessageQueue = MessageQueue<Event, android::hardware::kSynchronizedReadWrite>;
using WakeLockMessageQueue = MessageQueue<uint32_t, android::hardware::kSynchronizedReadWrite>;
using android::hardware::sensors::V2_0::ISensorsCallback;
using android::hardware::Return;
using android::hardware::EventFlag;
using android::hardware::sensors::V2_0::EventQueueFlagBits;
using android::hardware::sensors::V2_0::WakeLockQueueFlagBits;

class SensorEventCallback {
public:
    SensorEventCallback() {};
    SensorEventCallback(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback);
    ~SensorEventCallback(){};
    void postEvents(const std::vector<Event>& events, bool wakeup);
    bool isVaild();
    void setHasWakeLock(bool hasWake);
private:
    /* The Event FMQ where sensor events are written */
    std::unique_ptr<EventMessageQueue> mEventQueue;

    std::mutex mWriteLock;

    std::mutex mWakeLockLock;

    uint32_t mOutstandingWakeUpEvents;

    int64_t mAutoReleaseWakeLockTime;

    bool mHasWakeLock;

    /* The Wake Lock FMQ that is read to determine when the framework has handled WAKE_UP events */
    std::unique_ptr<WakeLockMessageQueue> mWakeLockQueue;

    EventFlag* mEventQueueFlag;

    /* Callback for asynchronous events, such as dynamic sensor connections. */
    sp<ISensorsCallback> mCallback;

    std::thread mWakeLockThread;

    std::atomic_bool mReadWakeLockQueueRun;

    void deleteEventFlag();

    void updateWakeLock(int32_t eventsWritten, int32_t eventsHandled);

    void readWakeLockFMQ();

    static void startReadWakeLockThread(SensorEventCallback* callback);
};

//}  //namespace implementation
//}  //namespace V2_0
//}  //namespace sensors
//}  //namespace hardware
//}  //namespace android

#endif   //ANDROID_HARDWARE_SENSORS_V2_0_SENSOREVENTCALLBACK_H
