/*
 * Copyright (C) 2008 The Android Open Source Project
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
#define LOG_TAG "SensorBase"
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <stdlib.h>
#include <android/log.h>

#include <linux/input.h>

#include "SensorBase.h"

using namespace android::hardware::sensors::V1_0::implementation;
SensorBase::SensorBase(const char* dev_name, const char* data_name)
    : dev_name(dev_name), data_name(data_name), mMode(OperationMode::NORMAL),
    dev_fd(-1),
    data_fd(-1),
    mStopThread(false),
    mShouldCheckSelf(false),
    mIsEnabled(false)
{
    openDataFd();
    mRunThread = std::thread(startThread, this);
}

SensorBase::~SensorBase() {
    std::unique_lock<std::mutex> lock(mRunMutex);
    if (data_fd >= 0) {
        close(data_fd);
    }

    if (dev_fd >= 0) {
        close(dev_fd);
    }
    mStopThread = true;
    lock.release();
    mRunThread.join();
}

int SensorBase::open_device() {
    if (dev_fd < 0 && dev_name) {
        dev_fd = open(dev_name, O_RDONLY);
        ALOGE_IF(dev_fd<0, "Couldn't open %s (%s)", dev_name, strerror(errno));
    }
    return 0;
}

void SensorBase::openDataFd() {
    if (data_name) {
        int clockid = CLOCK_BOOTTIME;
        data_fd = openInput(data_name);
        if (ioctl(data_fd, EVIOCSCLOCKID, &clockid))
            ALOGE("set EVIOCSCLOCKID failed ! \n");
    }
}

void SensorBase::setCheckSelf() {
    mShouldCheckSelf = true;
    mWaitCV.notify_all();
}

bool SensorBase::isAvailable() {
    return data_fd > 0;
}

void SensorBase::checkSelf() {
    if (data_fd < 0) {
        openDataFd();
    } else {
        char name[80];

        if (ioctl(data_fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
            name[0] = '\0';
        }

        if (strcmp(name, data_name)) {
            close(data_fd);
            data_fd = -1;
        }
    }
    mShouldCheckSelf = false;
}

void SensorBase::run() {
    std::unique_lock<std::mutex> runLock(mRunMutex);
    while (!mStopThread) {
        if (!mIsEnabled || mMode == OperationMode::DATA_INJECTION || !isAvailable()) {
            mWaitCV.wait(runLock, [&] {
                return ((mIsEnabled && mMode == OperationMode::NORMAL
                        && mShouldCheckSelf) || mStopThread);
            });
        } else {
            std::vector<Event> events;
            sensors_event_t sensor_event;
            Event event;
            int count = readEvents(&sensor_event, 1);
            if (count == 1) {
                convertFromSensorEvent(sensor_event, &event);
                events.push_back(event);
                mCallback->postEvents(events, isWakeUpSensors());
            }
        }
        if (mShouldCheckSelf) {
            checkSelf();
        }
    }
}

int SensorBase::injectEvent(const Event& event) {
    std::vector<Event> events;
    events.push_back(event);
    mCallback->postEvents(events, isWakeUpSensors());
    return 0;
}

int SensorBase::flush(int sensor_handle) {
    if (!mIsEnabled || 0 == getEnable(sensor_handle)) {
        return android::BAD_VALUE;
    }
    if (mSensorInfo->flags & SENSOR_FLAG_ONE_SHOT_MODE) {
        return android::BAD_VALUE;
    }

    // Note: If a sensor supports batching, write all of the currently batched events for the sensor
    // to the Event FMQ prior to writing the flush complete event.
    using SensorType = ::android::hardware::sensors::V1_0::SensorType;
    using MetaDataEventType = ::android::hardware::sensors::V1_0::MetaDataEventType;
    Event ev;
    ev.sensorHandle = sensor_handle;
    ev.sensorType = SensorType::META_DATA;
    ev.u.meta.what = MetaDataEventType::META_DATA_FLUSH_COMPLETE;
    std::vector<Event> evs{ev};
    mCallback->postEvents(evs, isWakeUpSensors());
 
    return 0;
}
void SensorBase::startThread(SensorBase *sensor) {
    sensor->run();
}

int SensorBase::close_device() {
    if (dev_fd >= 0) {
        close(dev_fd);
        dev_fd = -1;
    }

    return 0;
}
int SensorBase::getFd() const
{
    if (!data_name) {
        return dev_fd;
    }

    return data_fd;
}
int SensorBase::setEnable(int32_t handle, int enabled) {
    return 0;
}
int SensorBase::getEnable(int32_t handle) {
    return 0;
}

int SensorBase::setDelay(int32_t handle, int64_t ns) {
    return 0;
}

bool SensorBase::hasPendingEvents() const {
    return false;
}
void  processEvent(int code, int value) {

}

int64_t SensorBase::getTimestamp() {
    struct timespec t;

    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);

    return int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}
int SensorBase::set_sysfs_input_attr(char *class_path,const char *attr, char *value, int len)
{
    char path[256];
    int fd;

    if (class_path == NULL || *class_path == '\0'
        || attr == NULL || value == NULL || len < 1) {
        return -EINVAL;
    }

    snprintf(path, sizeof(path), "%s/%s", class_path, attr);
    path[sizeof(path) - 1] = '\0';

    fd = open(path, O_WRONLY);

    if (fd < 0) {
        ALOGD("Could not open (write-only) SysFs attribute \"%s\" (%s).", path, strerror(errno));
        close(fd);
        return -errno;
    }

    if (write(fd, value, len) < 0) {
        ALOGD("Could not write SysFs attribute \"%s\" (%s).", path, strerror(errno));
        close(fd);
        return -errno;
    }

    close(fd);

    return 0;
}

int SensorBase::openInput(const char* inputName) {
    int fd = -1;
    int input_id = -1;
    const char *dirname = "/dev/input";
    const char *inputsysfs = "/sys/class/input";
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if (dir == NULL)
        return -1;

    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

        strcpy(filename, de->d_name);
        fd = open(devname, O_RDONLY);

        if (fd>=0) {
            char name[80];

            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                name[0] = '\0';
            }

            if (!strcmp(name, inputName)) {
                break;
            } else {
                close(fd);
                fd = -1;
            }
        }
    }

    closedir(dir);
    ALOGE_IF(fd<0, "couldn't find '%s' input device", inputName);

    return fd;
}
int SensorBase::readEvents(sensors_event_t* data, int count) {
      return 0;
}

void SensorBase::setCallback(SensorEventCallback* sCallback) {
    mIsEnabled = true;
    mCallback = sCallback;
    mWaitCV.notify_all();
}

void SensorBase::set_operation_mode(OperationMode mode) {
    mMode = mode;
}


bool SensorBase::isWakeUpSensors() {
    return (mSensorInfo->flags & SENSOR_FLAG_WAKE_UP);
}
