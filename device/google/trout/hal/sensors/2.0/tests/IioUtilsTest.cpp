/*
 * Copyright 2020 The Android Open Source Project
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

#include <android-base/file.h>
#include <android/hardware/sensors/2.0/types.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include "iio_utils.h"

using ::android::hardware::sensors::V1_0::SensorType;
using android::hardware::sensors::V2_0::subhal::implementation::iio_device_data;
using android::hardware::sensors::V2_0::subhal::implementation::load_iio_devices;
using android::hardware::sensors::V2_0::subhal::implementation::sensors_supported_hal;

static bool sensorFilter(iio_device_data* dev) {
    static std::map<std::string, SensorType> KNOWN_SENSORS = {
            {"scmi.iio.accel", SensorType::ACCELEROMETER},
    };

    if (!dev) return false;

    const auto iter = KNOWN_SENSORS.find(dev->name);
    if (iter == KNOWN_SENSORS.end()) return false;

    dev->type = iter->second;
    return true;
}

TEST(IIoUtilsTest, ScanEmptyDirectory) {
    TemporaryDir td;
    std::vector<iio_device_data> iio_devices;
    const auto err = load_iio_devices(td.path, &iio_devices, sensorFilter);
    ASSERT_EQ(0, err);
    ASSERT_EQ(0, iio_devices.size());
}

static std::string concatPaths(const std::string& a, const std::string& b) {
    std::stringstream ss;
    ss << a << '/' << b;
    return ss.str();
}

template <typename T>
static bool writeFile(const std::string& path, const T& content, bool nl = true) {
    std::stringstream ss;
    ss << content;

    std::ofstream f;
    f.open(path);
    if (!f) return false;
    f << ss.str();
    if (nl) f << '\n';
    f.close();
    return true;
}

template <typename U>
static bool writeFile(const std::string& path, const std::vector<U>& content, bool nl = true) {
    std::stringstream ss;
    bool first = true;
    for (const auto& item : content) {
        if (!first) ss << ' ';
        ss << item;
        if (first) first = false;
    }

    return writeFile(path, ss.str(), nl);
}

static bool writeAccelDevice(const std::string& td_path, const iio_device_data& dev) {
    std::stringstream ss;
    ss << concatPaths(td_path, "iio:device") << std::to_string(dev.iio_dev_num);
    const std::string dev_path(ss.str());

    int err = mkdir(dev_path.c_str(), 0777);
    if (err != 0) return false;

    if (!writeFile(concatPaths(dev_path, "name"), dev.name)) return false;
    if (!writeFile(concatPaths(dev_path, "in_accel_x_scale"), dev.scale)) return false;
    if (!writeFile(concatPaths(dev_path, "in_accel_y_scale"), dev.scale)) return false;
    if (!writeFile(concatPaths(dev_path, "in_accel_z_scale"), dev.scale)) return false;
    if (!writeFile(concatPaths(dev_path, "in_accel_raw_available"),
                   "[-78381056.000000000 2392.000000000 78378664.000000000]"))
        return false;
    if (!writeFile(concatPaths(dev_path, "in_accel_sampling_frequency_available"),
                   dev.sampling_freq_avl))
        return false;

    return true;
}

// sets up a new iio:device<id> device with default parameters for an accelerometer
static iio_device_data createDefaultAccelerometerDevice(int id) {
    iio_device_data dev;
    dev.type = SensorType::ACCELEROMETER;
    dev.iio_dev_num = id;
    dev.name = "scmi.iio.accel";
    dev.sampling_freq_avl = {12.500000, 26.000364, 52.002080, 104.004160, 208.003993};
    dev.resolution = 2392;
    dev.scale = 0.000001000f;
    dev.max_range = 78378664;

    return dev;
}

TEST(IioUtilsTest, LoadValidSensor) {
    TemporaryDir td;
    const std::string td_path(td.path);
    const auto dev_model = createDefaultAccelerometerDevice(0);
    bool ok = writeAccelDevice(td_path, dev_model);
    ASSERT_TRUE(ok);

    std::vector<iio_device_data> iio_devices;
    const auto err = load_iio_devices(td_path, &iio_devices, sensorFilter);
    ASSERT_EQ(0, err);
    ASSERT_EQ(1, iio_devices.size());

    const auto& accel(iio_devices[0]);

    EXPECT_EQ(SensorType::ACCELEROMETER, accel.type);
    EXPECT_EQ("scmi.iio.accel", accel.name);
    EXPECT_EQ(0, accel.iio_dev_num);

    EXPECT_NEAR(dev_model.resolution, accel.resolution, 0.0002);
    EXPECT_NEAR(dev_model.scale, accel.scale, 0.0002);
    EXPECT_EQ(dev_model.max_range, accel.max_range);

    EXPECT_EQ(dev_model.sampling_freq_avl.size(), accel.sampling_freq_avl.size());
    for (size_t i = 0; i < dev_model.sampling_freq_avl.size(); ++i) {
        if (i >= accel.sampling_freq_avl.size()) break;
        EXPECT_NEAR(dev_model.sampling_freq_avl[i], accel.sampling_freq_avl[i], 0.0002);
    }
}
