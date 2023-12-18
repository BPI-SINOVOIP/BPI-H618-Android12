/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_SENSORS_V2_0_AW_SENSORS_H_

#define HARDWARE_INTERFACES_SENSORS_V2_0_AW_SENSORS_H_

#include <android-base/macros.h>
#include <android/hardware/sensors/2.0/ISensors.h>
#include <mutex>
#include <fmq/MessageQueue.h>
#include <hardware_legacy/power.h>
#include <hidl/MQDescriptor.h>
#include "SensorsImpl.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

using namespace ::android::hardware::sensors::V1_0;

struct Sensors : public ::android::hardware::sensors::V2_0::ISensors {
    Sensors();

    status_t initCheck() const;

    Return<void> getSensorsList(getSensorsList_cb _hidl_cb) override;

    Return<Result> initialize(
            const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<ISensorsCallback>& sensorsCallback) override;

    Return<Result> setOperationMode(OperationMode mode) override;

    Return<Result> activate(
            int32_t sensor_handle, bool enabled) override;

    Return<Result> batch(
            int32_t sensor_handle,
            int64_t sampling_period_ns,
            int64_t max_report_latency_ns) override;

    Return<Result> flush(int32_t sensor_handle) override;

    Return<Result> injectSensorData(const Event& event) override;

    Return<void> registerDirectChannel(
            const SharedMemInfo& mem, registerDirectChannel_cb _hidl_cb) override;

    Return<Result> unregisterDirectChannel(int32_t channelHandle) override;

    Return<void> configDirectReport(
            int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
            configDirectReport_cb _hidl_cb) override;

private:
    static constexpr int32_t kPollMaxBufferSize = 128;
    status_t mInitCheck;
    SensorsImpl* mSensorsImpl;
    std::mutex mPollLock;

    int getHalDeviceVersion() const;

    static void convertFromSensorEvents(
            size_t count, const sensors_event_t *src, hidl_vec<Event> *dst);

    DISALLOW_COPY_AND_ASSIGN(Sensors);
};

extern "C" ISensors *HIDL_FETCH_ISensors(const char *name);

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_SENSORS_V2_0_AW_SENSORS_H_
