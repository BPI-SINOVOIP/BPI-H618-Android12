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
#define LOG_TAG "GoogleIIOSensorSubHal"

#include "iio_utils.h"
#include <errno.h>
#include <limits.h>
#include <log/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

static const char* IIO_DEVICE_BASE = "iio:device";
static const char* IIO_SCAN_ELEMENTS_EN = "_en";
static const char* IIO_SFA_FILENAME = "sampling_frequency_available";
static const char* IIO_SCALE_FILENAME = "_scale";
static const char* IIO_SAMPLING_FREQUENCY = "_sampling_frequency";
static const char* IIO_BUFFER_ENABLE = "buffer/enable";
static const char* IIO_NAME_FILENAME = "name";
static const char* IIO_RANGE_AVAIL_FILENAME = "raw_available";

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace subhal {
namespace implementation {

const char* DEFAULT_IIO_DIR = "/sys/bus/iio/devices/";

using DirPtr = std::unique_ptr<DIR, decltype(&closedir)>;
using FilePtr = std::unique_ptr<FILE, decltype(&fclose)>;

static bool str_has_prefix(const char* s, const char* prefix) {
    if (!s || !prefix) return false;

    const auto len_s = strlen(s);
    const auto len_prefix = strlen(prefix);
    if (len_s < len_prefix) return false;
    return std::equal(s, s + len_prefix, prefix);
}

static bool str_has_suffix(const char* s, const char* suffix) {
    if (!s || !suffix) return false;

    const auto len_s = strlen(s);
    const auto len_suffix = strlen(suffix);
    if (len_s < len_suffix) return false;
    return std::equal(s + len_s - len_suffix, s + len_s, suffix);
}

static int sysfs_opendir(const std::string& name, DirPtr* dp) {
    if (dp == nullptr) {
        return -EINVAL;
    }

    /*
     * Check if path exists, if a component of path does not exist,
     * or path is an empty string return ENOENT
     * If path is not accessible return EACCES
     */
    struct stat sb;
    if (stat(name.c_str(), &sb) == -1) {
        return -errno;
    }

    /* Open sysfs directory */
    DIR* tmp = opendir(name.c_str());
    if (tmp == nullptr) return -errno;

    dp->reset(tmp);

    return 0;
}

// TODO(egranata): could this (and _read_ below), infer the fmt string directly
// from the type of value being passed in? that would be a safer alternative
template <typename T>
static int sysfs_write_val(const std::string& f, const std::string& fmt, const T value) {
    FilePtr fp = {fopen(f.c_str(), "r+"), fclose};
    if (nullptr == fp) return -errno;

    fprintf(fp.get(), fmt.c_str(), value);

    return 0;
}

static int sysfs_write_uint(const std::string& file, const unsigned int val) {
    return sysfs_write_val(file, "%u", val);
}

static int sysfs_write_double(const std::string& file, const double val) {
    return sysfs_write_val(file, "%f", val);
}

template <typename T>
static int sysfs_read_val(const std::string& f, const std::string& fmt, const T* value) {
    if (!value) return -EINVAL;

    FilePtr fp = {fopen(f.c_str(), "r"), fclose};
    if (nullptr == fp) return -errno;

    const int ret = fscanf(fp.get(), fmt.c_str(), value);
    return (ret == 1) ? 0 : -EINVAL;
}

static int sysfs_read_uint8(const std::string& file, uint8_t* val) {
    return sysfs_read_val(file, "%hhu\n", val);
}

static int sysfs_read_float(const std::string& file, float* val) {
    return sysfs_read_val(file, "%f\n", val);
}

static int sysfs_read_str(const std::string& file, std::string* str) {
    std::ifstream infile(file);
    if (!infile.is_open()) return -EINVAL;

    if (!std::getline(infile, *str))
        return -EINVAL;
    else
        return 0;
}

static int check_file(const std::string& filename) {
    struct stat info;
    return stat(filename.c_str(), &info);
}

int enable_sensor(const std::string& device_dir, const bool enable) {
    int err = check_file(device_dir);
    if (!err) {
        std::string enable_file = device_dir;
        enable_file += "/";
        enable_file += IIO_BUFFER_ENABLE;
        err = sysfs_write_uint(enable_file, enable);
    }

    return err;
}

static int get_sampling_frequency_available(const std::string& device_dir,
                                            std::vector<double>* sfa) {
    int ret = 0;
    char* rest;
    std::string line;
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;

    ret = sysfs_opendir(device_dir, &dp);
    if (ret) return ret;
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (str_has_suffix(ent->d_name, IIO_SFA_FILENAME)) {
            std::string filename = device_dir;
            filename += "/";
            filename += ent->d_name;
            ret = sysfs_read_str(filename, &line);
            if (ret < 0) return ret;
            char* pch = strtok_r(const_cast<char*>(line.c_str()), " ,", &rest);
            while (pch != nullptr) {
                sfa->push_back(atof(pch));
                pch = strtok_r(nullptr, " ,", &rest);
            }
        }
    }

    return ret < 0 ? ret : 0;
}

static int get_sensor_range(const std::string& device_dir, float* resolution, int64_t* max_range) {
    int ret = 0;
    char* rest;
    std::string line;
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;

    ret = sysfs_opendir(device_dir, &dp);
    if (ret) return ret;
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (str_has_suffix(ent->d_name, IIO_RANGE_AVAIL_FILENAME)) {
            std::string filename = device_dir;
            filename += "/";
            filename += ent->d_name;

            ret = sysfs_read_str(filename, &line);
            if (ret < 0) return ret;
            char* pch = strtok_r(const_cast<char*>(line.c_str()), " ", &rest);
            std::vector<std::string> range_avail;
            while (pch != nullptr) {
                range_avail.push_back(pch);
                pch = strtok_r(nullptr, " ", &rest);
            }
            *resolution = atof(range_avail[1].c_str());
            *max_range = atoll(range_avail[2].c_str());
        }
    }

    return ret < 0 ? ret : 0;
}

static int get_sensor_name(const std::string& device_dir, std::string* name) {
    const std::string filename = device_dir + "/" + IIO_NAME_FILENAME;

    return sysfs_read_str(filename, name);
}

int set_sampling_frequency(const std::string& device_dir, const double frequency) {
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;

    int ret = sysfs_opendir(device_dir, &dp);
    if (ret) return ret;
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (str_has_suffix(ent->d_name, IIO_SAMPLING_FREQUENCY)) {
            std::string filename = device_dir;
            filename += "/";
            filename += ent->d_name;
            ret = sysfs_write_double(filename, frequency);
        }
    }
    return ret;
}

static int get_sensor_scale(const std::string& device_dir, float* scale) {
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;
    int err;
    std::string filename;
    if (scale == nullptr) {
        return -EINVAL;
    }
    err = sysfs_opendir(device_dir, &dp);
    if (err) return err;
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (str_has_suffix(ent->d_name, IIO_SCALE_FILENAME)) {
            filename = device_dir;
            filename += "/";
            filename += ent->d_name;
            err = sysfs_read_float(filename, scale);
        }
    }
    return err;
}

int load_iio_devices(std::string iio_dir, std::vector<iio_device_data>* iio_data,
                     DeviceFilterFunction filter) {
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;
    int err;

    if (!iio_dir.empty() && iio_dir.back() != '/') iio_dir += '/';

    std::ifstream iio_file;
    const auto iio_base_len = strlen(IIO_DEVICE_BASE);
    err = sysfs_opendir(iio_dir, &dp);
    if (err) return err;
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (!str_has_prefix(ent->d_name, IIO_DEVICE_BASE)) continue;

        std::string path_device = iio_dir;
        path_device += ent->d_name;

        iio_device_data iio_dev_data;
        iio_dev_data.sysfspath.append(path_device, 0, iio_dir.size() + strlen(ent->d_name));
        err = get_sensor_name(iio_dev_data.sysfspath, &iio_dev_data.name);
        if (err) {
            ALOGE("get_sensor_name for %s returned error %d", path_device.c_str(), err);
            continue;
        }

        if (!filter(&iio_dev_data)) continue;

        ALOGI("found sensor %s at path %s", iio_dev_data.name.c_str(), path_device.c_str());
        err = get_sampling_frequency_available(iio_dev_data.sysfspath,
                                               &iio_dev_data.sampling_freq_avl);
        if (err) {
            ALOGE("get_sampling_frequency_available for %s returned error %d", path_device.c_str(),
                  err);
            continue;
        }

        std::sort(iio_dev_data.sampling_freq_avl.begin(), iio_dev_data.sampling_freq_avl.end());
        err = get_sensor_scale(iio_dev_data.sysfspath, &iio_dev_data.scale);
        if (err) {
            ALOGE("get_sensor_scale for %s returned error %d", path_device.c_str(), err);
            continue;
        }
        err = get_sensor_range(iio_dev_data.sysfspath, &iio_dev_data.resolution,
                               &iio_dev_data.max_range);
        if (err) {
            ALOGE("get_sensor_range for %s returned error %d", path_device.c_str(), err);
            continue;
        }

        sscanf(ent->d_name + iio_base_len, "%hhu", &iio_dev_data.iio_dev_num);

        iio_data->push_back(iio_dev_data);
    }
    return err;
}

static int get_scan_type(const std::string& device_dir, struct iio_info_channel* chanInfo) {
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;
    std::string scan_dir;
    std::string filename;
    std::string type_name;
    char signchar, endianchar;
    unsigned int storage_bits;

    if (chanInfo == nullptr) {
        return -EINVAL;
    }
    scan_dir = device_dir;
    scan_dir += "/scan_elements";
    const int err = sysfs_opendir(scan_dir, &dp);
    if (err) return err;
    type_name = chanInfo->name;
    type_name += "_type";
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (strcmp(ent->d_name, type_name.c_str()) == 0) {
            filename = scan_dir;
            filename += "/";
            filename += ent->d_name;
            FilePtr fp = {fopen(filename.c_str(), "r"), fclose};
            if (fp == nullptr) continue;
            const int ret = fscanf(fp.get(), "%ce:%c%hhu/%u>>%hhu", &endianchar, &signchar,
                                   &chanInfo->bits_used, &storage_bits, &chanInfo->shift);
            if (ret < 0) continue;
            chanInfo->big_endian = (endianchar == 'b');
            chanInfo->sign = (signchar == 's');
            chanInfo->storage_bytes = (storage_bits >> 3);
        }
    }
    return 0;
}

int scan_elements(const std::string& device_dir, struct iio_device_data* iio_data) {
    DirPtr dp(nullptr, closedir);
    const struct dirent* ent;
    std::string scan_dir;
    std::string filename;
    uint8_t temp;
    int ret;

    if (iio_data == nullptr) {
        return -EINVAL;
    }
    scan_dir = device_dir;
    scan_dir += "/scan_elements";
    ret = sysfs_opendir(scan_dir, &dp);
    if (ret) return ret;
    while (ent = readdir(dp.get()), ent != nullptr) {
        if (str_has_suffix(ent->d_name, IIO_SCAN_ELEMENTS_EN)) {
            filename = scan_dir;
            filename += "/";
            filename += ent->d_name;
            ret = sysfs_write_uint(filename, ENABLE_CHANNEL);
            if (ret == 0) {
                ret = sysfs_read_uint8(filename, &temp);
                if ((ret == 0) && (temp == 1)) {
                    iio_info_channel chan_info;
                    chan_info.name = strndup(ent->d_name,
                                             strlen(ent->d_name) - strlen(IIO_SCAN_ELEMENTS_EN));
                    filename = scan_dir;
                    filename += "/";
                    filename += chan_info.name;
                    filename += "_index";
                    ret = sysfs_read_uint8(filename, &chan_info.index);
                    if (ret) {
                        ALOGE("Getting index for channel %s for sensor %s returned error %d",
                              chan_info.name.c_str(), device_dir.c_str(), ret);
                        return ret;
                    }
                    ret = get_scan_type(device_dir, &chan_info);
                    if (ret) {
                        ALOGE("Getting scan type for channel %s sensor %s returned error %d",
                              chan_info.name.c_str(), device_dir.c_str(), ret);
                        return ret;
                    }
                    iio_data->channelInfo.push_back(chan_info);
                } else {
                    ALOGE("Not able to successfully enable channel %s for sensor %s error %d",
                          ent->d_name, device_dir.c_str(), ret);
                    return ret;
                }
            } else {
                ALOGE("Enabling scan channel %s for sensor %s returned error %d", ent->d_name,
                      device_dir.c_str(), ret);
                return ret;
            }
        }
    }
    return ret;
}
}  // namespace implementation
}  // namespace subhal
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
