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

#define LOG_TAG "MagSensor"
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
#include "MagSensor.h"

#define MAG_CTRL_NAME    "FreescaleMagnetometer"
#define MAG_DATA_NAME    "eCompass"
#define MAG_SYSFS_PATH   "/sys/class/input"
#define MAG_SYSFS_DELAY  "poll"
#define MAG_SYSFS_ENABLE "enable"
#define MAG_EVENT_X     ABS_X
#define MAG_EVENT_Y     ABS_Y
#define MAG_EVENT_Z     ABS_Z
#define ORN_EVENT_YAW     ABS_RX
#define ORN_EVENT_PITCH     ABS_RY
#define ORN_EVENT_ROLL         ABS_RZ
#define ORN_EVENT_STATUS     ABS_WHEEL

#define MAG_DATA_CONVERSION(value) (float)((float)(((int)value)/(20.0f)))
#define ORN_DATA_CONVERSION(value) (float)((float)(((int)value)/(100.0f)))

MagSensor::MagSensor()
        : SensorBase(MAG_CTRL_NAME, MAG_DATA_NAME),
        mPendingMask(0),
        mInputReader(16)
{

        memset(&mPendingEvent[0], 0, sensors *sizeof(sensors_event_t));
    memset(mClassPath, '\0', sizeof(mClassPath));

    mEnabled[mag] = 0;
    mDelay[mag] = 0;
        mPendingEvent[mag].version = sizeof(sensors_event_t);
        mPendingEvent[mag].sensor = ID_M;
        mPendingEvent[mag].type = SENSOR_TYPE_MAGNETIC_FIELD;
        mPendingEvent[mag].magnetic.status = SENSOR_STATUS_ACCURACY_LOW;
        mPendingEvent[mag].version = sizeof(sensors_event_t);

    mEnabled[orn] = 0;
    mDelay[orn] = 0;
        mPendingEvent[orn].sensor  = ID_O;
        mPendingEvent[orn].type = SENSOR_TYPE_ORIENTATION;
        mPendingEvent[orn].orientation.status = SENSOR_STATUS_ACCURACY_LOW;
    mPendingEvent[orn].version = sizeof(sensors_event_t);


    if(sensor_get_class_path(mClassPath))
    {
        ALOGE("Can`t find the mag sensor!");
    }
}

MagSensor::~MagSensor()
{
}

int MagSensor::setEnable(int32_t handle, int en)
{
    int err = 0;
    int what = mag;

        switch(handle){
        case ID_M : what = mag; break;
        case ID_O : what = orn; break;
        }

        if(en)
                mEnabled[what]++;
    else
        mEnabled[what]--;

    if(mEnabled[what] < 0)
        mEnabled[what] = 0;

    if(mEnabled[mag] > 0 || mEnabled[orn] > 0)
        err = enable_sensor();
    else
        err = disable_sensor();

    if (!err) {
                update_delay(what);
        }
#ifdef DEBUG_SENSOR
    ALOGD("MagSensor mEnabled %d, OrientaionSensor mEnabled %d\n", mEnabled[mag], mEnabled[orn]);
#endif
        return err;
}

int MagSensor::setDelay(int32_t handle, int64_t ns)
{
        if (ns < 0)
                return -EINVAL;

#ifdef DEBUG_SENSOR
        ALOGD("%s: ns = %lld", __func__, ns);
#endif

    int what = mag;
        switch(handle){
        case ID_M : what = mag; break;
        case ID_O : what = orn; break;
        }

        mDelay[what] = ns;

        return update_delay(what);
}

int MagSensor::update_delay(int sensor_type)
{
        if (mEnabled[sensor_type]) {
                return set_delay(mDelay[sensor_type]);
        }
        else
        return 0;
}

int MagSensor::readEvents(sensors_event_t* data, int count)
{
    int i;
        if (count < 1)
                return -EINVAL;

        if(data_fd < 0)
                return 0;

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

            for(i = 0 ; i< sensors && mPendingMask && count ;i++){
                    if(mPendingMask & (1 << i)){
                        mPendingMask &= ~(1 << i);
                        mPendingEvent[i].timestamp = time;

                        if (mEnabled[i]) {
                                *data++ = mPendingEvent[i];
                                count--;
                                numEventReceived++;
                        }
                  }
                    }

                if (!mPendingMask) {
                        mInputReader.next();
                }

                } else {
                        mInputReader.next();
                }
        }

        return numEventReceived;
}

void MagSensor::processEvent(int code, int value)
{

        switch (code) {
                case MAG_EVENT_X :
                        mPendingMask |= 1 << mag;
                        mPendingEvent[mag].magnetic.x = MAG_DATA_CONVERSION(value);
                        break;
                case MAG_EVENT_Y:
                        mPendingMask |= 1 << mag;
                        mPendingEvent[mag].magnetic.y = MAG_DATA_CONVERSION(value);
                        break;
                case MAG_EVENT_Z:
                        mPendingMask |=  1 << mag;
                        mPendingEvent[mag].magnetic.z = MAG_DATA_CONVERSION(value);
                        break;
        case ORN_EVENT_YAW :
                        mPendingMask |=  1 << orn;
                        mPendingEvent[orn].orientation.azimuth    = ORN_DATA_CONVERSION(value);
                        break;
                case ORN_EVENT_PITCH :
                        mPendingMask |=  1 << orn;
                        mPendingEvent[orn].orientation.pitch = ORN_DATA_CONVERSION(value);
                        break;
                case ORN_EVENT_ROLL :
                        mPendingMask |=  1 << orn;
                        mPendingEvent[orn].orientation.roll    = ORN_DATA_CONVERSION(value);
                        break;
        case ORN_EVENT_STATUS:
            mPendingMask |=  ((1 << mag) |(1 << orn));
            mPendingEvent[mag].magnetic.status = value;
            mPendingEvent[orn].orientation.status = value;
            break;
        }
}

int MagSensor::writeEnable(int isEnable) {
        char attr[PATH_MAX] = {'\0'};

    if(mClassPath[0] == '\0')
        return -1;

    strcpy(attr, mClassPath);
    strcat(attr,"/");
    strcat(attr,MAG_SYSFS_ENABLE);

    int fd = open(attr, O_RDWR);
    if (0 > fd) {
        ALOGE("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
        return -errno;
    }

    char buf[2];

    if (isEnable) {
        buf[0] = '1';
    } else {
        buf[0] = '0';
    }
    buf[1] = '\0';

    int err = 0;
    err = write(fd, buf, sizeof(buf));

    if (0 > err) {
        err = -errno;
        ALOGE("Could not write SysFs attribute \"%s\" (%s).", attr, strerror(errno));
    } else {
        err = 0;
    }

    close(fd);

    return err;
}

int MagSensor::writeDelay(int64_t ns) {
    char attr[PATH_MAX] = {'\0'};

    if(mClassPath[0] == '\0')
        return -1;

    strcpy(attr, mClassPath);
    strcat(attr,"/");
    strcat(attr,MAG_SYSFS_DELAY);

    int fd = open(attr, O_RDWR);
    if (0 > fd) {
        ALOGE("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
        return -errno;
    }
    if (ns > 10240000000LL) {
        ns = 10240000000LL; /* maximum delay in nano second. */
    }
    if (ns < 312500LL) {
        ns = 312500LL; /* minimum delay in nano second. */
    }

        char buf[80];
        sprintf(buf, "%lld", ns/1000/1000);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;

}

int MagSensor::enable_sensor() {
    return writeEnable(1);
}

int MagSensor::disable_sensor() {
    return writeEnable(0);
}

int MagSensor::set_delay(int64_t ns) {
    return writeDelay(ns);
}

int MagSensor::getEnable(int32_t handle) {
    int what = mag;

    if(handle == ID_M)
        what = mag;

    else if(handle == ID_O)
        what = orn;

    return mEnabled[what];
}

int MagSensor::sensor_get_class_path(char *class_path)
{
    char dirname[] = MAG_SYSFS_PATH;
    char buf[256];
    int res;
    DIR *dir;
    struct dirent *de;
    int fd = -1;
    int found = 0;

    dir = opendir(dirname);
    if (dir == NULL)
        return -1;

    while((de = readdir(dir))) {
        if (strncmp(de->d_name, "input", strlen("input")) != 0) {
            continue;
            }

        sprintf(class_path, "%s/%s", dirname, de->d_name);
        snprintf(buf, sizeof(buf), "%s/name", class_path);

        fd = open(buf, O_RDONLY);
        if (fd < 0) {
            continue;
        }
        if ((res = read(fd, buf, sizeof(buf))) < 0) {
            close(fd);
            continue;
        }
        buf[res - 1] = '\0';
        if (strcmp(buf, MAG_CTRL_NAME) == 0) {
            found = 1;
            close(fd);
            break;
        }

        close(fd);
        fd = -1;
    }
    closedir(dir);
    if (found) {
        return 0;
    }else {
        *class_path = '\0';
        return -1;
    }
}

/*****************************************************************************/

