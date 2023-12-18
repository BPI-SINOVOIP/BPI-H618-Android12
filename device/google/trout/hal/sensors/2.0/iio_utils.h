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
#ifndef ANDROID_SENSORS_IIO_UTILS_H
#define ANDROID_SENSORS_IIO_UTILS_H

#include <android/hardware/sensors/1.0/types.h>
#include <dirent.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <functional>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace subhal {
namespace implementation {

using ::android::hardware::sensors::V1_0::SensorType;

extern const char* DEFAULT_IIO_DIR;

static constexpr auto DEFAULT_IIO_BUFFER_LEN = 2;
static constexpr auto DISABLE_CHANNEL = 0;
static constexpr auto ENABLE_CHANNEL = 1;

struct sensors_supported_hal {
    std::string name;
    SensorType type;
};

struct iio_info_channel {
    std::string name;
    uint8_t index;
    uint8_t storage_bytes;
    uint8_t bits_used;
    uint8_t shift;
    bool big_endian;
    bool sign;
};

struct iio_device_data {
    std::string name;
    std::string sysfspath;
    float resolution;
    float scale;
    SensorType type;
    std::vector<iio_info_channel> channelInfo;
    std::vector<double> sampling_freq_avl;
    uint8_t iio_dev_num;
    int64_t max_range;
};

using DeviceFilterFunction = std::function<bool(iio_device_data*)>;

int load_iio_devices(std::string iio_dir, std::vector<iio_device_data>* iio_data,
                     DeviceFilterFunction filter);
int scan_elements(const std::string& device_dir, struct iio_device_data* iio_data);
int enable_sensor(const std::string& name, const bool flag);
int set_sampling_frequency(const std::string& name, const double frequency);
}  // namespace implementation
}  // namespace subhal
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
#endif
