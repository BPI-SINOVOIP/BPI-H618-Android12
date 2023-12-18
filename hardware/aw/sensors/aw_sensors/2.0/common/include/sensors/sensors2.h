/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_SENSORS_INTERFACE_2_H
#define ANDROID_SENSORS_INTERFACE_2_H

#include <android/hardware/sensors/2.0/ISensors.h>

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <cutils/native_handle.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

//namespace android {
//namespace hardware {
//namespace sensors {
//namespace V2_0 {
//namespace implementation {
using android::hardware::sensors::V2_0::ISensorsCallback;
__BEGIN_DECLS
using namespace ::android::hardware::sensors::V1_0;
using ::android::sp;
using ::android::hardware::EventFlag;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptor;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::sensors::V1_0::Event;

/*****************************************************************************/
#define SENSORS2_HEADER_VERSION    2
#define SENSORS_DEVICE_API_VERSION_2_0  HARDWARE_DEVICE_API_VERSION_2(2, 0, SENSORS2_HEADER_VERSION)
#define SENSORS2_HARDWARE_MODULE_ID     "sensors2"

struct sensors_module_2_t {
    struct hw_module_t common;

    /**
     * Enumerate all available sensors. The list is returned in "list".
     * return number of sensors in the list
     */
    int (*get_sensors_list)(struct sensors_module_2_t* module,
            struct sensor_t const** list);

    /**
     *  Place the module in a specific mode. The following modes are defined
     *
     *  0 - Normal operation. Default state of the module.
     *  1 - Loopback mode. Data is injected for the supported
     *      sensors by the sensor service in this mode.
     * return 0 on success
     *         -EINVAL if requested mode is not supported
     *         -EPERM if operation is not allowed
     */
    int (*set_operation_mode)(uint32_t mode);
    /**
     * Initialize the Sensors HAL's Fast Message Queues (FMQ) and callback.
     *
     * The Fast Message Queues (FMQ) that are used to send data between the
     * framework and the HAL. The callback is used by the HAL to notify the
     * framework of asynchronous events, such as a dynamic sensor connection.
     *
     * The Event FMQ is used to transport sensor events from the HAL to the
     * framework. The Event FMQ is created using the eventQueueDescriptor.
     * Data may only be written to the Event FMQ. Data must not be read from
     * the Event FMQ since the framework is the only reader. Upon receiving
     * sensor events, the HAL writes the sensor events to the Event FMQ.
     *
     * Once the HAL is finished writing sensor events to the Event FMQ, the HAL
     * must notify the framework that sensor events are available to be read and
     * processed. This is accomplished by either:
     *     1) Calling the Event FMQ’s EventFlag::wake() function with
              EventQueueFlagBits::READ_AND_PROCESS
     *     2) Setting the write notification in the Event FMQ’s writeBlocking()
     *        function to EventQueueFlagBits::READ_AND_PROCESS.
     *
     * If the Event FMQ’s writeBlocking() function is used, the read
     * notification must be set to EventQueueFlagBits::EVENTS_READ in order to
     * be notified and unblocked when the framework has successfully read events
     * from the Event FMQ.
     *
     * The Wake Lock FMQ is used by the framework to notify the HAL when it is
     * safe to release its wake_lock. When the framework receives WAKE_UP events
     * from the Event FMQ and the framework has acquired a wake_lock, the
     * framework must write the number of WAKE_UP events processed to the Wake
     * Lock FMQ. When the HAL reads the data from the Wake Lock FMQ, the HAL
     * decrements its current count of unprocessed WAKE_UP events and releases
     * its wake_lock if the current count of unprocessed WAKE_UP events is
     * zero.
     *
     * The framework must use the WakeLockQueueFlagBits::DATA_WRITTEN value to
     * notify the HAL that data has been written to the Wake Lock FMQ and must
     * be read by HAL.
     *
     * The ISensorsCallback is used by the HAL to notify the framework of
     * asynchronous events, such as a dynamic sensor connection.
     *
     * The name of any wake_lock acquired by the Sensors HAL for WAKE_UP events
     * must begin with "SensorsHAL_WAKEUP".
     *
     * If WAKE_LOCK_TIMEOUT_SECONDS has elapsed since the most recent WAKE_UP
     * event was written to the Event FMQ without receiving a message on the
     * Wake Lock FMQ, then any held wake_lock for WAKE_UP events must be
     * released.
     *
     * If either the Event FMQ or the Wake Lock FMQ is already initialized when
     * initialize is invoked, then both existing FMQs must be discarded and the
	 *
     * new descriptors must be used to create new FMQs within the HAL. The
     * number of outstanding WAKE_UP events should also be reset to zero, and
     * any outstanding wake_locks held as a result of WAKE_UP events should be
     * released.
     *
     * All active sensor requests and direct channels must be closed and
     * properly cleaned up when initialize is called in order to ensure that the
     * HAL and framework's state is consistent (e.g. after a runtime restart).
     *
     * initialize must be thread safe and prevent concurrent calls
     * to initialize from simultaneously modifying state.
     *
     * @param eventQueueDescriptor Fast Message Queue descriptor that is used to
     *     create the Event FMQ which is where sensor events are written. The
     *     descriptor is obtained from the framework's FMQ that is used to read
     *     sensor events.
     * @param wakeLockDescriptor Fast Message Queue descriptor that is used to
     *     create the Wake Lock FMQ which is where wake_lock events are read
     *     from. The descriptor is obtained from the framework's FMQ that is
     *     used to write wake_lock events.
     * @param sensorsCallback sensors callback that receives asynchronous data
     *     from the Sensors HAL.
     * @return result OK on success; BAD_VALUE if descriptor is invalid (such
     *     as null)
     */
    int (*initialize)(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<ISensorsCallback>& sensorsCallback);

};

struct sensors_poll_device2_t {
    struct hw_device_t common;
    int (*activate)(struct sensors_poll_device2_t *dev,
            int sensor_handle, int enabled);
    int (*setDelay)(struct sensors_poll_device2_t *dev,
            int sensor_handle, int64_t sampling_period_ns);
};

/*
 * struct sensors_poll_device_2 is used in HAL versions >= SENSORS_DEVICE_API_VERSION_1_0
 */
typedef struct sensors_poll_device_2 {
    union {
        /* sensors_poll_device_2 is compatible with sensors_poll_device_t,
         * and can be down-cast to it
         */
        struct sensors_poll_device2_t v0;

        struct {
            struct hw_device_t common;

            /* Activate/de-activate one sensor.
             *
             * sensor_handle is the handle of the sensor to change.
             * enabled set to 1 to enable, or 0 to disable the sensor.
             *
             * Before sensor activation, existing sensor events that have not
             * been picked up by poll() should be abandoned so that application
             * upon new activation request will not get stale events.
             * (events that are generated during latter activation or during
             * data injection mode after sensor deactivation)
             *
             * Return 0 on success, negative errno code otherwise.
             */
            int (*activate)(struct sensors_poll_device2_t *dev,
                    int sensor_handle, int enabled);

            /**
             * Set the events's period in nanoseconds for a given sensor.
             * If sampling_period_ns > max_delay it will be truncated to
             * max_delay and if sampling_period_ns < min_delay it will be
             * replaced by min_delay.
             */
            int (*setDelay)(struct sensors_poll_device2_t *dev,
                    int sensor_handle, int64_t sampling_period_ns);
        };
    };


    /*
     * Sets a sensor’s parameters, including sampling frequency and maximum
     * report latency. This function can be called while the sensor is
     * activated, in which case it must not cause any sensor measurements to
     * be lost: transitioning from one sampling rate to the other cannot cause
     * lost events, nor can transitioning from a high maximum report latency to
     * a low maximum report latency.
     * See the Batching sensor results page for details:
     * http://source.android.com/devices/sensors/batching.html
     */
    int (*batch)(struct sensors_poll_device_2* dev,
            int sensor_handle, int flags, int64_t sampling_period_ns,
            int64_t max_report_latency_ns);

    /*
     * Flush adds a META_DATA_FLUSH_COMPLETE event (sensors_event_meta_data_t)
     * to the end of the "batch mode" FIFO for the specified sensor and flushes
     * the FIFO.
     * If the FIFO is empty or if the sensor doesn't support batching (FIFO size zero),
     * it should return SUCCESS along with a trivial META_DATA_FLUSH_COMPLETE event added to the
     * event stream. This applies to all sensors other than one-shot sensors.
     * If the sensor is a one-shot sensor, flush must return -EINVAL and not generate
     * any flush complete metadata.
     * If the sensor is not active at the time flush() is called, flush() should return
     * -EINVAL.
     */
    int (*flush)(struct sensors_poll_device_2* dev, int sensor_handle);

    /*
     * Inject a single sensor sample to be to this device.
     * data points to the sensor event to be injected
     * return 0 on success
     *         -EPERM if operation is not allowed
     *         -EINVAL if sensor event cannot be injected
     */
    int (*inject_sensor_data)(struct sensors_poll_device_2 *dev, const Event& event);

    /*
     * Register/unregister direct report channel.
     *
     * A HAL declares support for direct report by setting non-NULL values for both
     * register_direct_channel and config_direct_report.
     *
     * This function has two operation modes:
     *
     * Register: mem != NULL, register a channel using supplied shared memory information. By the
     * time this function returns, sensors must finish initializing shared memory content
     * (format dependent, see SENSOR_DIRECT_FMT_*).
     *      Parameters:
     *          mem             points to a valid struct sensors_direct_mem_t.
     *          channel_handle  is ignored.
     *      Return value:
     *          A handle of channel (>0, <INT32_MAX) when success, which later can be referred in
     *          unregister or config_direct_report call, or error code (<0) when failed
     * Unregister: mem == NULL, unregister a previously registered channel.
     *      Parameters:
     *          mem             set to NULL
     *          channel_handle  contains handle of channel to be unregistered
     *      Return value:
     *          0, even if the channel_handle is invalid, in which case it will be a no-op.
     */
    int (*register_direct_channel)(struct sensors_poll_device_2 *dev,
            const struct sensors_direct_mem_t* mem, int channel_handle);

    /*
     * Configure direct sensor event report in direct channel.
     *
     * Start, modify rate or stop direct report of a sensor in a certain direct channel. A special
     * case is setting sensor handle -1 to stop means to stop all active sensor report on the
     * channel specified.
     *
     * A HAL declares support for direct report by setting non-NULL values for both
     * register_direct_channel and config_direct_report.
     *
     * Parameters:
     *      sensor_handle   sensor to be configured. The sensor has to support direct report
     *                      mode by setting flags of sensor_t. Also, direct report mode is only
     *                      defined for continuous reporting mode sensors.
     *      channel_handle  channel handle to be configured.
     *      config          direct report parameters, see sensor_direct_cfg_t.
     * Return value:
     *      - when sensor is started or sensor rate level is changed: return positive identifier of
     *        sensor in specified channel if successful, otherwise return negative error code.
     *      - when sensor is stopped: return 0 for success or negative error code for failure.
     */
    int (*config_direct_report)(struct sensors_poll_device_2 *dev,
            int sensor_handle, int channel_handle, const struct sensors_direct_cfg_t *config);

    /*
     * Reserved for future use, must be zero.
     */
    void (*reserved_procs[5])(void);

} sensors_poll_device_2_t;

/** convenience API for opening and closing a device */

static inline int sensors_open(const struct hw_module_t* module,
        struct sensors_poll_device_2** device) {
    return module->methods->open(module,
            SENSORS_HARDWARE_POLL, TO_HW_DEVICE_T_OPEN(device));
}

static inline int sensors_close(struct sensors_poll_device_2* device) {
    return device->common.close(&device->common);
}

static inline int sensors_open_2(const struct hw_module_t* module,
        sensors_poll_device_2** device) {
    return module->methods->open(module,
            SENSORS_HARDWARE_POLL, TO_HW_DEVICE_T_OPEN(device));
}

static inline int sensors_close_2(sensors_poll_device_2* device) {
    return device->common.close(&device->common);
}

__END_DECLS
//}
//}
//}
//}
//}
#endif  // ANDROID_SENSORS_INTERFACE_2_H
