/*
 * Copyright (C) 2012 Freescale Semiconductor Inc.
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
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include "TempSensor.h"

#define TEMP_DATA_NAME    tempSensorInfo.sensorName

TempSensor::TempSensor()
        : SensorBase(NULL, TEMP_DATA_NAME),
        mEnabled(0),
        mInputReader(4),
    mPendingMask(0)
{
        memset(&mPendingEvent, 0, sizeof(mPendingEvent));

        mPendingEvent.sensor  = ID_T;
        mPendingEvent.type    = SENSOR_TYPE_TEMPERATURE;
    mPendingEvent.version = sizeof(sensors_event_t);
}

TempSensor::~TempSensor()
{
        if (mEnabled) {
                setEnable(ID_T, 0);
        }
}

int TempSensor::setEnable(int32_t handle, int en)
{
        //ALOGD("enable:  handle:  %ld, en: %d", handle, en);
        char buf[2];
        int err = -1;


    if(tempSensorInfo.classPath[0] == ICHAR)
        return -1;

        int flags = en ? 1 : 0;

        if (flags != mEnabled) {
            int bytes = sprintf(buf, "%d", flags);
            err = set_sysfs_input_attr(tempSensorInfo.classPath,"enable",buf,bytes);
            mEnabled = flags;
    }

    return 0;
}

int TempSensor::setDelay(int32_t handle, int64_t ns)
{
       // ALOGD("delay:  handle:  %ld, ns: %lld", handle, ns);
        if(tempSensorInfo.classPath[0] == ICHAR)
        return -1;

    if (ns > 10240000000LL) {
        ns = 10240000000LL; /* maximum delay in nano second. */
    }
    if (ns < 312500LL) {
        ns = 312500LL; /* minimum delay in nano second. */
    }

        char buf[80];
        int bytes = sprintf(buf, "%lld", ns/1000 / 1000);


        int err = set_sysfs_input_attr(tempSensorInfo.classPath,"delay",buf,bytes);


        return 0;
}


int TempSensor::readEvents(sensors_event_t* data, int count)
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
                        mInputReader.next();
                } else if (type == EV_SYN) {
                        int64_t time = timevalToNano(event->time);

            if (mPendingMask) {
                mPendingMask = 0;
                mPendingEvent.timestamp = time;

                if (mEnabled) {
                    *data++ = mPendingEvent;
                    count--;
                    numEventReceived++;
                }
            }

                        if (!mPendingMask) {
                                mInputReader.next();
                        }

                } else {
                        ALOGE("AccelSensor: unknown event (type=%d, code=%d)",
                                type, event->code);
                        mInputReader.next();
                }
        }

        return numEventReceived;
}

void TempSensor::processEvent(int code, int value)
{

        mPendingMask = 1 ;
        mPendingEvent.temperature = value;
      //  ALOGD("Sensor data:  value:  %f", mPendingEvent.temperature);

}






/*****************************************************************************/

