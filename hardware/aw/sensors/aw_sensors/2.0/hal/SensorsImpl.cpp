/*
 * Copyright (C) 2011 Freescale Semiconductor Inc.
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

#define LOG_TAG "SensorsImpl"
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include "SensorsImpl.h"

#include "AccelSensor.h"
#include "MagSensor.h"
#include "PressSensor.h"
#include "GyroSensor.h"
#include "LightSensor.h"
#include "ProximitySensor.h"
#include "TempSensor.h"

#include <cutils/properties.h>

#define MODULE_VERSION "V2.0.1"

using android::hardware::sensors::V1_0::OperationMode;
int SensorsImpl::get_sensors_list(struct sensor_t const** list) {
    int a = 0;
    *list = sSensorList;
    a = ARRAY_SIZE(sSensorList);
    ALOGD("sensors__get_sensors_list sNumber:%d, a:%d\n", sNumber, a);
    return sNumber;
}

bool SensorsImpl::checkHasWakeUpSensor() {
    int num = ARRAY_SIZE(sSensorList);
    for (int i = 0; i < num; i++) {
        if (sSensorList[i].flags & SENSOR_FLAG_WAKE_UP)
            return true;
    }
    return false;
}

int SensorsImpl::initialize(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback)
{
    int result = 0;
    int a = ARRAY_SIZE(sSensorList);
    int i = 0;
    for (i = 0; i < a; i++) {
        activate(sSensorList[i].handle /* handle */, 0 /* disable */);
    }
    result = setCallback(eventQueueDescriptor, wakeLockDescriptor, sensorsCallback);

    return result;
}

bool SensorsImpl::setupNotify(void) {
    int wakeFds[2];
    int result = 0;
    struct epoll_event eventItem = {};

    result = pipe(wakeFds);
    if (result != 0) {
        ALOGE("startINotifyThread create pipe failed");
        return false;
    }
    mWakeReadPipeFd = wakeFds[0];
    mWakeWritePipeFd = wakeFds[1];
    mINotifyFd = inotify_init();
    if (mINotifyFd < 0) {
        ALOGE("startINotifyThread inotify_init failed");
        return false;
    }
    mInputWd = inotify_add_watch(mINotifyFd, "/dev/input", IN_DELETE | IN_CREATE);
    if (mInputWd < 0) {
        ALOGE("startINotifyThread inotify_add_watch failed");
        return false;
    }
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
    eventItem.events = EPOLLIN;
    eventItem.data.fd = mINotifyFd;
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mINotifyFd, &eventItem);
    if (result != 0) {
        ALOGE("startINotifyThread EPOLL add mINotifyFd failed");
        return false;
    }
    result = fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    result = fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    eventItem.data.fd = mWakeReadPipeFd;
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeReadPipeFd, &eventItem);
    if (result != 0) {
        ALOGE("startINotifyThread EPOLL add mWakeReadPipeFd failed");
        return false;
    }
    return true;
}

void SensorsImpl::startINotifyThread(void) {
    struct epoll_event pendingEventItems[2];
    int i, j;
    int num;
    struct inotify_event* event;
    char event_buf[512];
    int event_size;
    int event_pos = 0;

    while (!mStopNotifyThread) {
        num = epoll_wait(mEpollFd, pendingEventItems, 2, -1);
        for (i = 0; i < 2; i++) {
            if (pendingEventItems[i].data.fd == mINotifyFd) {
                if (pendingEventItems[i].events & EPOLLIN) {
                    int res = read(mINotifyFd, event_buf, sizeof(event_buf));
                    if (res < (int)sizeof(*event)) {
                        if (errno == EINTR) {
                            ALOGE("einter inotify event");
                            break;
                        }
                        ALOGE("could not get inotify event");
                        return;
                    }
                    event_pos = 0;
                    while (res >= (int)sizeof(*event)) {
                        event = (struct inotify_event*)(event_buf + event_pos);
                        if (event->len) {
                            if (event->wd == mInputWd && event->mask & IN_CREATE) {
                                for (j = 0; j < sNumber; j++) {
                                    mSensors[j]->setCheckSelf();
                                }
                            }
                        }
                        event_size = sizeof(*event) + event->len;
                        res -= event_size;
                        event_pos += event_size;
                    }
                }
            }
        }
    }
}

void SensorsImpl::startThread(SensorsImpl *impl) {
    impl->startINotifyThread();
}

int SensorsImpl::set_operation_mode(uint32_t mode) {
    bool hasInject = false;
    for (int i = 0; i < sNumber; i++) {
        if (mSensors[i]->getSensorInfo()->flags & SENSOR_FLAG_DATA_INJECTION) {
            mSensors[i]->set_operation_mode((OperationMode)mode);
            hasInject = true;
        }
    }
    if (!hasInject)
        return android::BAD_VALUE;
    return 0;
}

void SensorsImpl::createSensorDevices()
{
    int first = -1;
    accel = mag = gyro = light = proximity = temperature = press = accel_uncal = -1;

    if((seStatus[ID_A].isUsed == true) && (seStatus[ID_A].isFound == true)) {
        first = first + 1;
        accel = first;
        mSensors[first] = new AccelSensor();
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_A]]);
        mSensors[first]->setHandle(ID_A);
        if((seStatus[ID_AU].isUsed == true) && (seStatus[ID_AU].isFound == true)) {
            first = first + 1;
            accel_uncal = first;
            mSensors[first] = mSensors[accel];
        }
    }

    if((seStatus[ID_M].isUsed == true) && (seStatus[ID_M].isFound == true)) {
        first = first + 1;
        mag = first;

#if defined (MAG_SENSOR_FXOS8700)
        mSensors[first] = new MagSensor();
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_M]]);
        mSensors[first]->setHandle(ID_M);
#else
/*
        mSensors[first] = new MagnetoSensor((AccelSensor*)mSensors[accel]);
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_M]]);
        mSensors[first]->setHandle(ID_M);
*/
#endif
    }

/*
    if((seStatus[ID_GY].isUsed == true) && (seStatus[ID_GY].isFound == true)) {
        first = first + 1;
        gyro = first;
        mSensors[first] = new GyroSensor();
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_GY]]);
        mSensors[first]->setHandle(ID_GY);
    }
*/
    if((seStatus[ID_L].isUsed == true) && (seStatus[ID_L].isFound == true)) {
        first = first + 1;
        light = first;
        mSensors[first] = new LightSensor();
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_L]]);
        mSensors[first]->setHandle(ID_L);
    }

    if((seStatus[ID_PX].isUsed == true) && (seStatus[ID_PX].isFound == true)) {
        first = first + 1;
        proximity = first;
        mSensors[first] = new ProximitySensor();
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_PX]]);
        mSensors[first]->setHandle(ID_PX);
    }

    if((seStatus[ID_T].isUsed == true) && (seStatus[ID_T].isFound == true)) {
        first = first + 1;
        temperature = first;
        mSensors[first] = new TempSensor();
        mSensors[first]->setSensorInfo(&sSensorList[seSensorIndex[ID_T]]);
        mSensors[first]->setHandle(ID_T);
    }
    if (setupNotify()) {
        mStopNotifyThread = false;
        mRunThread = std::thread(startThread, this);
    }
    for (int i = 0; i < sNumber; i++) {
        mSensors[i]->setCheckSelf();
    }
}

SensorsImpl::~SensorsImpl() {
    for (int i=0 ; i < sNumber ; i++) {
        delete mSensors[i];
    }
    mStopNotifyThread = true;
    write(mWakeWritePipeFd, "w", 2);
    mRunThread.join();
    close(mINotifyFd);
    close(mEpollFd);
    close(mWakeWritePipeFd);
    close(mWakeReadPipeFd);
}

int SensorsImpl::activate(int32_t handle, bool enabled) {

    int index = handleToDriver(handle);
    if (index < 0) return android::BAD_VALUE;
    int err = 0 ;
    // if handle == orientaion or magnetic ,please enable ACCELERATE Sensor
    if(handle == ID_O || handle ==  ID_M){
        err =  mSensors[accel]->setEnable(handle, enabled);
        if(err)
            return err;
    }
    err |=  mSensors[index]->setEnable(handle, enabled);

    return err;
}

int SensorsImpl::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    int err = -1;
    if (index < 0)
        return android::BAD_VALUE;

    if(handle == ID_O || handle ==  ID_M){
        mSensors[accel]->setDelay(handle, ns);
    }

    err = mSensors[index]->setDelay(handle, ns);

    return err;
}

int SensorsImpl::setCallback(const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback)
{
    SensorEventCallback *sCallback = new SensorEventCallback(eventQueueDescriptor, wakeLockDescriptor, sensorsCallback);
    if (!sCallback->isVaild())
        return -EINVAL;
    sCallback->setHasWakeLock(checkHasWakeUpSensor());
    for (int i = 0; i < sNumber; i++) {
        mSensors[i]->setCallback(sCallback);
    }
    return 0;
}

int SensorsImpl::inject_sensor_data(const Event& event) {
    int index = handleToDriver(event.sensorHandle);
    if (index < 0) return index;
    int err = 0 ;
    err = mSensors[index]->injectEvent(event);
    return err;
}

int SensorsImpl::handleToDriver(int handle) const {
    switch (handle) {
        case ID_A:
            return accel;
        case ID_M:
        case ID_O:
            return mag;
        case ID_GY:
            return gyro;
        case ID_L:
            return light;
        case ID_PX:
            return proximity;
        case ID_T:
            return temperature;
        case ID_P:
            return press;
        case ID_AU:
            return accel_uncal;
    }
    return -EINVAL;
}

int SensorsImpl::batch(int handle, int64_t sampling_period_ns,
         int64_t max_report_latency_ns) {
    int index = handleToDriver(handle);
    if (index < 0) return android::BAD_VALUE;
    return setDelay(handle,sampling_period_ns);
}

int SensorsImpl::flush(int sensor_handle) {

    ALOGW("AW flush handle %d\n",sensor_handle);
    int index = handleToDriver(sensor_handle);
    if (index < 0) return android::BAD_VALUE;
    return mSensors[index]->flush(sensor_handle);
}

int SensorsImpl::register_direct_channel(sensors_direct_mem_t* m, int channel_handle) {
    return 0;
}

int SensorsImpl::config_direct_report(int sensor_handle, int channel_handle, const struct sensors_direct_cfg_t* config) {
    return 0;
}

SensorsImpl::SensorsImpl()
{
    property_set("sys.sensors", "1");/*Modify the  enable and delay interface  group */
    ALOGD("aw sensor hal start! current module version is : %s\n", MODULE_VERSION);
    insmodDevice();
    sensorsDetect();/*detect device,filling sensor_t structure */
    createSensorDevices();
}
