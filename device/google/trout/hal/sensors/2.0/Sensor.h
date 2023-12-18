/*
 * Copyright (C) 2020 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H
#define ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H

#include <android/hardware/sensors/1.0/types.h>
#include <poll.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "SensorThread.h"
#include "iio_utils.h"
#include "sensor_hal_configuration_V1_0.h"

#define NUM_OF_CHANNEL_SUPPORTED 4
// Subtract the timestamp channel to get the number of data channels
#define NUM_OF_DATA_CHANNELS NUM_OF_CHANNEL_SUPPORTED - 1

using ::android::hardware::sensors::V1_0::AdditionalInfo;
using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::sensor::hal::configuration::V1_0::Configuration;

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace subhal {
namespace implementation {

static constexpr unsigned int frequency_to_us(unsigned int x) {
    return (1E6 / x);
}

static constexpr unsigned int ns_to_frequency(unsigned int x) {
    return (1E9 / x);
}

// SCMI IIO driver gives sensor power in microwatts. Sensor HAL expects
// it in mA. Conversion process needs the input voltage for the IMU.
// Based on commonly used IMUs, 3.6V is picked as the default.
constexpr auto SENSOR_VOLTAGE_DEFAULT = 3.6f;

class ISensorsEventCallback {
  public:
    virtual ~ISensorsEventCallback() = default;
    virtual void postEvents(const std::vector<Event>& events, bool wakeup) = 0;
};

// Virtual Base Class for Sensor
class SensorBase {
  public:
    SensorBase(int32_t sensorHandle, ISensorsEventCallback* callback, SensorType type);
    virtual ~SensorBase();
    const SensorInfo& getSensorInfo() const;
    virtual void batch(int32_t samplingPeriodNs) = 0;
    virtual void activate(bool enable) = 0;
    virtual Result flush();
    void setOperationMode(OperationMode mode);
    bool supportsDataInjection() const;
    Result injectEvent(const Event& event);

    bool isEnabled() const;
    OperationMode getOperationMode() const;

  protected:
    friend class SensorThread;

    virtual void pollSensor() = 0;
    bool isWakeUpSensor();

    bool mIsEnabled;
    int64_t mSamplingPeriodNs;
    SensorInfo mSensorInfo;
    ISensorsEventCallback* mCallback;
    OperationMode mMode;
    SensorThread mSensorThread;
};

class SensorThread;

// HWSensorBase represents the actual physical sensor provided as the IIO device
class HWSensorBase : public SensorBase {
  public:
    static HWSensorBase* buildSensor(int32_t sensorHandle, ISensorsEventCallback* callback,
                                     const struct iio_device_data& iio_data,
                                     const std::optional<std::vector<Configuration>>& config);
    ~HWSensorBase();
    void batch(int32_t samplingPeriodNs);
    void activate(bool enable);
    Result flush();
    struct iio_device_data mIioData;

  protected:
    void pollSensor() override;

  private:
    void idleLoop();
    void pollForEvents();

    static constexpr uint8_t LOCATION_X_IDX = 3;
    static constexpr uint8_t LOCATION_Y_IDX = 7;
    static constexpr uint8_t LOCATION_Z_IDX = 11;
    static constexpr uint8_t ROTATION_X_IDX = 0;
    static constexpr uint8_t ROTATION_Y_IDX = 1;
    static constexpr uint8_t ROTATION_Z_IDX = 2;

    ssize_t mScanSize;
    struct pollfd mPollFdIio;
    std::vector<uint8_t> mSensorRawData;
    int64_t mXMap, mYMap, mZMap;
    bool mXNegate, mYNegate, mZNegate;
    std::vector<AdditionalInfo> mAdditionalInfoFrames;

    HWSensorBase(int32_t sensorHandle, ISensorsEventCallback* callback,
                 const struct iio_device_data& iio_data,
                 const std::optional<std::vector<Configuration>>& config);

    ssize_t calculateScanSize();
    void run();
    void setOrientation(std::optional<std::vector<Configuration>> config);
    void processScanData(uint8_t* data, Event* evt);
    void setAxisDefaultValues();
    status_t setAdditionalInfoFrames(const std::optional<std::vector<Configuration>>& config);
    void sendAdditionalInfoReport();
    status_t getSensorPlacement(AdditionalInfo* sensorPlacement,
                                const std::optional<std::vector<Configuration>>& config);
};
}  // namespace implementation
}  // namespace subhal
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
#endif  // ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H
