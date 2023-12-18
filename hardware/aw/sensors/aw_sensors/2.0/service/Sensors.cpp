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

#include "Sensors.h"
#include <sensors/convert.h>

#include <android-base/logging.h>

#include <sys/stat.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

using namespace ::android::hardware::sensors::V1_0::implementation;
/*
 * If a multi-hal configuration file exists in the proper location,
 * return true indicating we need to use multi-hal functionality.
 */
/*
static bool UseMultiHal() {
    return false;
}
*/
static Result ResultFromStatus(status_t err) {
    switch (err) {
        case OK:
            return Result::OK;
        case PERMISSION_DENIED:
            return Result::PERMISSION_DENIED;
        case NO_MEMORY:
            return Result::NO_MEMORY;
        case BAD_VALUE:
            return Result::BAD_VALUE;
        default:
            return Result::INVALID_OPERATION;
    }
}

Sensors::Sensors()
    : mInitCheck(NO_INIT) {
    mSensorsImpl = new SensorsImpl();

    // Require all the old HAL APIs to be present except for injection, which
    // is considered optional.
    CHECK_GE(getHalDeviceVersion(), SENSORS_DEVICE_API_VERSION_2_0);

    mInitCheck = OK;
}

status_t Sensors::initCheck() const {
    return mInitCheck;
}

Return<Result> Sensors::initialize(
        const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback) {
        return ResultFromStatus(mSensorsImpl->initialize(eventQueueDescriptor, wakeLockDescriptor, sensorsCallback));
}

Return<void> Sensors::getSensorsList(getSensorsList_cb _hidl_cb) {
    sensor_t const *list;
    size_t count = mSensorsImpl->get_sensors_list(&list);

    hidl_vec<SensorInfo> out;
    out.resize(count);

    for (size_t i = 0; i < count; ++i) {
        const sensor_t *src = &list[i];
        SensorInfo *dst = &out[i];

        convertFromSensor(*src, dst);
    }

    _hidl_cb(out);

    return Void();
}

int Sensors::getHalDeviceVersion() const {
    if (mSensorsImpl != nullptr)
        return mSensorsImpl->getHalVersion();
    return -1;
}

Return<Result> Sensors::setOperationMode(OperationMode mode) {
    return ResultFromStatus((status_t)(mSensorsImpl->set_operation_mode((uint32_t)mode)));
}

Return<Result> Sensors::activate(
        int32_t sensor_handle, bool enabled) {
    return ResultFromStatus(
            mSensorsImpl->activate(sensor_handle, enabled));
}

Return<Result> Sensors::batch(
        int32_t sensor_handle,
        int64_t sampling_period_ns,
        int64_t max_report_latency_ns) {
    return ResultFromStatus(
            mSensorsImpl->batch(sensor_handle,
                sampling_period_ns,
                max_report_latency_ns));
}

Return<Result> Sensors::flush(int32_t sensor_handle) {
    return ResultFromStatus(mSensorsImpl->flush(sensor_handle));
}

Return<Result> Sensors::injectSensorData(const Event& event) {
    return ResultFromStatus(
            mSensorsImpl->inject_sensor_data(event));
}

Return<void> Sensors::registerDirectChannel(
        const SharedMemInfo& mem, registerDirectChannel_cb _hidl_cb) {
    if (!mSensorsImpl->isSupportDirectChannel()) {
        // HAL does not support
        _hidl_cb(Result::INVALID_OPERATION, -1);
        return Void();
    }

    sensors_direct_mem_t m;
    if (!convertFromSharedMemInfo(mem, &m)) {
      _hidl_cb(Result::BAD_VALUE, -1);
      return Void();
    }

    int err = mSensorsImpl->register_direct_channel(&m, -1);

    if (err < 0) {
        _hidl_cb(ResultFromStatus(err), -1);
    } else {
        int32_t channelHandle = static_cast<int32_t>(err);
        _hidl_cb(Result::OK, channelHandle);
    }
    return Void();
}

Return<Result> Sensors::unregisterDirectChannel(int32_t channelHandle) {
    if (!mSensorsImpl->isSupportDirectChannel()) {
        // HAL does not support
        return Result::INVALID_OPERATION;
    }

    mSensorsImpl->register_direct_channel(nullptr, channelHandle);

    return Result::OK;
}

Return<void> Sensors::configDirectReport(
        int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
        configDirectReport_cb _hidl_cb) {
    if (!mSensorsImpl->isSupportDirectChannel()) {
        // HAL does not support
        _hidl_cb(Result::INVALID_OPERATION, -1);
        return Void();
    }

    sensors_direct_cfg_t cfg = {
        .rate_level = convertFromRateLevel(rate)
    };
    if (cfg.rate_level < 0) {
        _hidl_cb(Result::BAD_VALUE, -1);
        return Void();
    }

    int err = mSensorsImpl->config_direct_report(sensorHandle, channelHandle, &cfg);

    if (rate == RateLevel::STOP) {
        _hidl_cb(ResultFromStatus(err), -1);
    } else {
        _hidl_cb(err > 0 ? Result::OK : ResultFromStatus(err), err);
    }
    return Void();
}

// static
void Sensors::convertFromSensorEvents(
        size_t count,
        const sensors_event_t *srcArray,
        hidl_vec<Event> *dstVec) {
    for (size_t i = 0; i < count; ++i) {
        const sensors_event_t &src = srcArray[i];
        Event *dst = &(*dstVec)[i];

        convertFromSensorEvent(src, dst);
    }
}

ISensors *HIDL_FETCH_ISensors(const char * /* hal */) {
    Sensors *sensors = new Sensors;
    if (sensors->initCheck() != OK) {
        delete sensors;
        sensors = nullptr;

        return nullptr;
    }

    return sensors;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
