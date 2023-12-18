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
#include "PressSensor.h"

#define PRESS_DATA_NAME    "mpl3115"
#define PRESS_SYSFS_PATH   "/sys/class/input"
#define PRESS_SYSFS_DELAY  "poll"
#define PRESS_SYSFS_ENABLE "enable"
#define PRESS_EVENT         ABS_PRESSURE
#define TEMPERATURE_EVENT     ABS_MISC
#define PRESS_DATA_CONVERSION(value) (float)((float)(((int)value)/(4.0f*100)))
#define TEMPERATURE_DATA_CONVERSION(value) (float)((float)(((int)value)/(16.0f)))

PressSensor::PressSensor()
        : SensorBase(NULL, PRESS_DATA_NAME),
        mPendingMask(0),
        mInputReader(4)
{
        memset(&mPendingEvent[0], 0, sensors *sizeof(sensors_event_t));
    memset(mClassPath, '\0', sizeof(mClassPath));

    mEnabled[press] = 0;
    mDelay[press] = 0;
        mPendingEvent[press].version = sizeof(sensors_event_t);
        mPendingEvent[press].sensor  = ID_P;
        mPendingEvent[press].type    = SENSOR_TYPE_PRESSURE;
        mPendingEvent[press].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;
        mPendingEvent[press].version = sizeof(sensors_event_t);

    mEnabled[temperature] = 0;
    mDelay[temperature] = 0;
        mPendingEvent[temperature].sensor  = ID_T;
        mPendingEvent[temperature].type    = SENSOR_TYPE_TEMPERATURE;
        mPendingEvent[temperature].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;
    mPendingEvent[temperature].version = sizeof(sensors_event_t);

    if(sensor_get_class_path(mClassPath))
    {
        ALOGE("Can`t find the press sensor!");
    }
}

PressSensor::~PressSensor()
{
}

int PressSensor::setEnable(int32_t handle, int en)
{
    int err = 0;
    int what = press;
        switch(handle){
        case ID_P : what = press; break;
        case ID_T : what = temperature; break;
        }
        if(en)
        mEnabled[what]++;
    else
        mEnabled[what]--;

    if(mEnabled[what] < 0)
        mEnabled[what] = 0;
    if(mEnabled[press] > 0 || mEnabled[temperature] > 0)
        err = enable_sensor();
    else
        err = disable_sensor();
    if (!err) {
            update_delay(what);
        }
    ALOGD("PressSensor mEnabled %d, Temperature mEnabled %d\n",mEnabled[press],mEnabled[temperature]);
        return err;
}

int PressSensor::setDelay(int32_t handle, int64_t ns)
{
        if (ns < 0)
                return -EINVAL;
    int what = press;

        switch(handle){
        case ID_P : what = press; break;
        case ID_T : what = temperature; break;
        }
        mDelay[what] = ns;
        return update_delay(what);
}

int PressSensor::update_delay(int sensor_type)
{
        if (mEnabled[sensor_type]) {
                return set_delay(mDelay[sensor_type]);
        }
        else
        return 0;
}

int PressSensor::readEvents(sensors_event_t* data, int count)
{
    int i;
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

void PressSensor::processEvent(int code, int value)
{

        switch (code) {
                case PRESS_EVENT:
                        mPendingMask |= 1 << press;
                        mPendingEvent[press].pressure = PRESS_DATA_CONVERSION(value);
                        break;
                case TEMPERATURE_EVENT:
                        mPendingMask |= 1 << temperature;
                        mPendingEvent[temperature].temperature = TEMPERATURE_DATA_CONVERSION(value);
            break;
        }
}

int PressSensor::writeEnable(int isEnable) {
    char attr[PATH_MAX] = {'\0'};
    if(mClassPath[0] == '\0')
        return -1;

    strcpy(attr, mClassPath);
    strcat(attr,"/");
    strcat(attr,PRESS_SYSFS_ENABLE);

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

int PressSensor::writeDelay(int64_t ns) {
    char attr[PATH_MAX] = {'\0'};
    if(mClassPath[0] == '\0')
        return -1;

    strcpy(attr, mClassPath);
    strcat(attr,"/");
    strcat(attr,PRESS_SYSFS_DELAY);

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

int PressSensor::enable_sensor() {
    return writeEnable(1);
}

int PressSensor::disable_sensor() {
    return writeEnable(0);
}

int PressSensor::set_delay(int64_t ns) {
    return writeDelay(ns);
}

int PressSensor::getEnable(int32_t handle) {
    int what = press;
    if(handle == ID_P)
        what = press;
    else if(handle == ID_T)
        what = temperature;
    return mEnabled[what];
}

int PressSensor::sensor_get_class_path(char *class_path)
{
    char dirname[] = PRESS_SYSFS_PATH;
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
        if (strcmp(buf, PRESS_DATA_NAME) == 0) {
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

