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

#ifndef ANDROID_SENSOR_BASE_H
#define ANDROID_SENSOR_BASE_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <thread>
#include <sensors/convert.h>
#include "InputEventReader.h"
#include "SensorEventCallback.h"

#define SENSORS_MAX  20

//namespace android {
//namespace hardware {
//namespace sensors {
//namespace V2_0 {
//namespace implementation {

using android::hardware::EventFlag;
using android::hardware::sensors::V1_0::OperationMode;
/*****************************************************************************/
class SensorBase {
protected:
        const char* dev_name;
        const char* data_name;
        int         dev_fd;
        int         data_fd;
        sensor_t*   mSensorInfo;
        std::thread mRunThread;
        std::mutex mRunMutex;
        std::atomic_bool mStopThread;
        std::condition_variable mWaitCV;
        SensorEventCallback *mCallback;
        bool mIsEnabled;
        bool mShouldCheckSelf;
        OperationMode mMode;
        int mHandle;

        void run();
        bool isAvailable();
        void checkSelf();
        void openDataFd();
        static void startThread(SensorBase *sensor);
        int openInput(const char* inputName);
        int set_sysfs_input_attr(char *class_path,const char *attr, char *value, int len);
        static int64_t getTimestamp();


        static int64_t timevalToNano(timeval const& t) {
            return t.tv_sec*1000000000LL + t.tv_usec*1000;
        }

        int open_device();
        int close_device();

public:
        SensorBase(const char* dev_name,const char* data_name);
        void setSensorInfo(sensor_t* info) {mSensorInfo = info;}
        sensor_t* getSensorInfo() {return mSensorInfo;}
        void setHandle(int handle) {mHandle = handle;}
        bool isWakeUpSensors();
        void setCheckSelf();

        void setCallback(SensorEventCallback* sCallback);
        void set_operation_mode(OperationMode mode);
        int injectEvent(const Event& event);
        int flush(int sensor_handle);
        virtual ~SensorBase();
        virtual bool hasPendingEvents() const;
        virtual int getFd() const;
        virtual int setDelay(int32_t handle, int64_t ns);
        virtual int setEnable(int32_t handle, int enabled);
        virtual int getEnable(int32_t handle);
        virtual int readEvents(sensors_event_t* data, int count);
        virtual void processEvent(int code, int value) = 0;
};


//}   //namespace implementation
//}   //namespace V2_0
//}   //namespace sensors
//}   //namespace hardware
//}   //namespace android

/*****************************************************************************/

#endif  // ANDROID_SENSOR_BASE_H
