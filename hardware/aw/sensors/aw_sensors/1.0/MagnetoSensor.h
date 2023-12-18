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

#ifndef ANDROID_MAGNETOMETER_SENSOR_H
#define ANDROID_MAGNETOMETER_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include <cutils/native_handle.h>
//#include <cutils/sockets.h>
#include <hardware/sensors.h>
#include <linux/input.h>
//#include <pthread.h>

#include "sensors.h"
#include "SensorBase.h"
#include "AccelSensor.h"
#include "InputEventReader.h"
#include "MEMSAlgLib_eCompass.h"

/*****************************************************************************/

struct input_event;
class AccSensor;

class MagnetoSensor : public SensorBase {
private:
    //int mEnabled;
    int mEnCount;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvent;

    int setInitialState();

    AccelSensor*  mAccSensor;
    int mPendingMask;


public:
            MagnetoSensor(AccelSensor* as);
    virtual ~MagnetoSensor();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int read_sensor_calibration_data(int *mx, int *my, int *mz);
    virtual int write_sensor_calibration_data(int mx, int my, int mz);
    void processEvent(int code, int value);
};

/*****************************************************************************/

#endif  // ANDROID_MAGNETOMETER_SENSOR_H
