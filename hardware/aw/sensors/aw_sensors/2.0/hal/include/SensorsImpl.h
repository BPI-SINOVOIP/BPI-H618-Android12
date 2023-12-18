#ifndef ANDROID_SENSORSIMPL_H
#define ANDROID_SENSORSIMPL_H

#include <thread>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>
#include <poll.h>
#include <hardware/hardware.h>
#include <fmq/MessageQueue.h>
#include <hardware_legacy/power.h>
#include <hidl/MQDescriptor.h>
#include <android/hardware/sensors/2.0/ISensors.h>
#include "SensorEventCallback.h"
#include "SensorBase.h"

#define SENSORS2_HEADER_VERSION    2
#define SENSORS_DEVICE_API_VERSION_2_0  HARDWARE_DEVICE_API_VERSION_2(2, 0, SENSORS2_HEADER_VERSION)

class SensorsImpl {
private:
    static const int VERSION = SENSORS_DEVICE_API_VERSION_2_0;
    int mWakeWritePipeFd;
    int mWakeReadPipeFd;
    int mEpollFd;
    int mINotifyFd;
    int mInputWd;
    SensorBase* mSensors[10];
    std::thread mRunThread;

    int accel;
    int mag;
    int gyro;
    int light;
    int proximity;
    int temperature;
    int press;
    int accel_uncal;
	bool mStopNotifyThread;

    ~SensorsImpl();
    void createSensorDevices();
    bool checkHasWakeUpSensor();
    int handleToDriver(int handle) const;
    int setDelay(int handle, int64_t ns);
    bool setupNotify(void);
    static void startThread(SensorsImpl * impl);
    int setCallback(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback);

public:
    SensorsImpl();
    void startINotifyThread(void);
    int getHalVersion() {return VERSION;}
    int initialize(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback);
    int set_operation_mode(uint32_t mode);
    int get_sensors_list(struct sensor_t const **list);
    int activate(int32_t sensor_handle, bool enable);
    int batch(int32_t sensor_handle, int64_t sampling_period_ns,
        int64_t max_report_latency_ns);
    int flush(int32_t sensor_handle);
    int inject_sensor_data(const Event& event);
    int register_direct_channel(sensors_direct_mem_t* m, int channel_handle);
    int config_direct_report(int sensor_handle, int channel_handle, const struct sensors_direct_cfg_t* config);
    bool isSupportDirectChannel() {return false;}
};


#endif
