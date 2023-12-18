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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <stdio.h>
#include <android/log.h>
#include <inttypes.h>

#include "ProximitySensor.h"

#define PROSENSOR_DATA_NAME    proSensorInfo.sensorName

/*****************************************************************************/

ProximitySensor::ProximitySensor()
    : SensorBase(NULL, PROSENSOR_DATA_NAME),
    mEnabled(0),
    mInputReader(4),
    mPendingMask(0)
{

    memset(&mPendingEvent, 0, sizeof(mPendingEvent));

    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_PX;
    mPendingEvent.type = SENSOR_TYPE_PROXIMITY;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

#ifdef DEBUG_SENSOR
    ALOGD("%s:data_fd:%d\n", __func__,data_fd);
#endif

#if 0
    if (data_fd > 0) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
        enable(0, 1);
    }
#endif

}

ProximitySensor::~ProximitySensor() {
    if (mEnabled) {
        setEnable(0, 0);
    }
}

int ProximitySensor::setDelay(int32_t handle, int64_t ns)
{
    if(proSensorInfo.classPath[0] == ICHAR)
        return -1;

    if (ns > 10240000000LL) {
        ns = 10240000000LL; /* maximum delay in nano second. */
    }
    if (ns < 312500LL) {
        ns = 312500LL; /* minimum delay in nano second. */
    }

    char buf[80];
    int bytes = sprintf(buf, "%" SCNi64, ns/1000 / 1000);

    int err = set_sysfs_input_attr(proSensorInfo.classPath,"ps_poll_delay",buf,bytes);

    return 0;
}

int ProximitySensor::setEnable(int32_t, int en) {
    char buf[2];
    int err = -1;

    if (proSensorInfo.classPath[0] == ICHAR)
        return -1;

    int flags = en ? 1 : 0;

    if (flags != mEnabled) {
        int bytes = sprintf(buf, "%d", flags);
        err = set_sysfs_input_attr(proSensorInfo.classPath,"enable",buf,bytes);
        mEnabled = flags;
    }

    return 0;
}


int ProximitySensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;

        if ((type == EV_ABS) || (type == EV_REL) || (type == EV_KEY)) {
                processEvent(event->code, event->value);
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            if (mPendingMask) {
                mPendingMask = 0;
                mPendingEvent.timestamp = time;

                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            } else
                ALOGE("ProximitySensor sensor error no data but sync event\n");

        } else {
            ALOGE("ProximitySensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
    }

    return numEventReceived;
}

void ProximitySensor::processEvent(int code, int value) {
    if (code == EVENT_TYPE_PROXIMITY) {
        mPendingMask = 1;
        mPendingEvent.distance = indexToValue(value);
#ifdef DEBUG_SENSOR
        ALOGD("distance value: %f\n", mPendingEvent.distance);
#endif
    }
}

float ProximitySensor::indexToValue(size_t index) const
{
    return index * PROXIMITY_THRESHOLD_GP2A;
}

int ProximitySensor::getEnable(int32_t handle) {
    return (handle == ID_PX) ? mEnabled : 0;
}
