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

#define LOG_TAG "Sensors"
#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"

#include "AccelSensor.h"
#if defined (MAG_SENSOR_FXOS8700)
#include "MagSensor.h"
#else
#include "MagnetoSensor.h"
#endif
#include "PressSensor.h"
#include "GyroSensor.h"
#include "LightSensor.h"
#include "ProximitySensor.h"
#include "TempSensor.h"

#include <cutils/properties.h>


static int open_sensors(const struct hw_module_t* module, const char* id,
        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    int a = 0;
    *list = sSensorList;
    a = ARRAY_SIZE(sSensorList);
    ALOGD("sensors__get_sensors_list sNumber:%d, a:%d\n", sNumber, a);
    return sNumber;
}

static struct hw_module_methods_t sensors_module_methods = {
open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
common: {
tag: HARDWARE_MODULE_TAG,
     version_major: 1,
     version_minor: 1,
     id: SENSORS_HARDWARE_MODULE_ID,
     name: "Sensor module",
     author: "Freescale Semiconductor Inc.",
     methods: &sensors_module_methods,
     dso : NULL,
     reserved : {},
        },
get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_1 device; // must be first

    sensors_poll_context_t();
    ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
	void setFlushPending(int handle, bool pending);
	int getFlushPending(int handle);

    private:
    int accel;
    int mag;
    int gyro;
    int light;
    int proximity;
    int temperature ;
    int press;
	int flushPending[10];

    static const size_t wake = 10;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[11];
    int mWritePipeFd;
    SensorBase* mSensors[10];

    int handleToDriver(int handle) const {
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
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
    :gyro(-1),
    press(-1)
{
    int first = -1;
	for (int i = 0; i < 10; i++) {
		flushPending[i] = 0;
	}

    if((seStatus[ID_A].isUsed == true) && (seStatus[ID_A].isFound == true)) {
        first = first + 1;
        accel = first;
        mSensors[first] = new AccelSensor();
        mPollFds[first].fd = mSensors[accel]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
    }

    if((seStatus[ID_M].isUsed == true) && (seStatus[ID_M].isFound == true)) {
        first = first + 1;
        mag = first;

#if defined (MAG_SENSOR_FXOS8700)
        mSensors[first] = new MagSensor();
        mPollFds[first].fd = mSensors[mag]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
#else
/*
        mSensors[first] = new MagnetoSensor((AccelSensor*)mSensors[accel]);
        mPollFds[first].fd = mSensors[mag]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
*/
#endif
    }

/*
    if((seStatus[ID_GY].isUsed == true) && (seStatus[ID_GY].isFound == true)) {
        first = first + 1;
        gyro = first;
        mSensors[first] = new GyroSensor();
        mPollFds[first].fd = mSensors[gyro]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
    }
*/
    if((seStatus[ID_L].isUsed == true) && (seStatus[ID_L].isFound == true)) {
        first = first + 1;
        light = first;
        mSensors[first] = new LightSensor();
        mPollFds[first].fd = mSensors[light]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
    }

    if((seStatus[ID_PX].isUsed == true) && (seStatus[ID_PX].isFound == true)) {
        first = first + 1;
        proximity = first;
        mSensors[first] = new ProximitySensor();
        mPollFds[first].fd = mSensors[proximity]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
    }

    if((seStatus[ID_T].isUsed == true) && (seStatus[ID_T].isFound == true)) {
        first = first + 1;
        temperature = first;
        mSensors[first] = new TempSensor();
        mPollFds[first].fd = mSensors[temperature]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
    }

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i < 10 ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err = 0 ;
	// if handle == orientaion or magnetic ,please enable ACCELERATE Sensor
    if(handle == ID_O || handle ==  ID_M){
        err =  mSensors[accel]->setEnable(handle, enabled);
        if(err)
            return err;
    }
    err |=  mSensors[index]->setEnable(handle, enabled);

    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    int err = -1;
    if (index < 0)
        return index;

    if(handle == ID_O || handle ==  ID_M){
        mSensors[accel]->setDelay(handle, ns);
    }

    err = mSensors[index]->setDelay(handle, ns);

    return err;
}
void sensors_poll_context_t::setFlushPending(int handle, bool pending)
{
	if (handle < 10) {
		if (pending)
			flushPending[handle]++;
		else
			flushPending[handle]--;
	}
}

int sensors_poll_context_t::getFlushPending(int handle)
{
	if (handle < 10)
		return flushPending[handle];
	else return 0;
}


int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;
    int noInCnt = 0;
        // check flush pending
    for (int i = 0; i < 10; i++) {
		if(getFlushPending(i)) {
			data[0].version = META_DATA_VERSION;
			data[0].timestamp = 0;
			data[0].type = SENSOR_TYPE_META_DATA;
			data[0].sensor = i;
			data[0].meta_data.what = META_DATA_FLUSH_COMPLETE;
			data[0].meta_data.sensor = i;
			setFlushPending(i,false);
			return 1;
		}
	}

	n = poll(mPollFds, sNumber, -1);
	if (n<0) {
				ALOGE(" AW sensor poll() failed (%s)", strerror(errno));
				return 0;
	}
	if (mPollFds[wake].revents & POLLIN) {
			char msg;
			int result = read(mPollFds[wake].fd, &msg, 1);
			ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
			ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
			mPollFds[wake].revents = 0;
	}
	// read sensor event
	for (int i = 0 ; (count >= 0) && (i < sNumber) ; i++) {
		SensorBase* const sensor(mSensors[i]);
#ifdef DEBUG_SENSOR
            ALOGE("AW count:%d, mPollFds[%d].revents:%d, hasPendingEvents():%d,sNumber:%d\n",
                    count, i, mPollFds[i].revents, sensor->hasPendingEvents(),sNumber);
#endif
		if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
			int nb = sensor->readEvents(data, count);
			if (nb < count) {
				mPollFds[i].revents = 0;
			}
			if (nb <= 0)
				ALOGE("AW sensor: %d  error readEvents return %d\n", i, nb);
			count -= nb;
			nbEvents += nb;
			data += nb;
		} else
			noInCnt++;
		if (noInCnt == sNumber)
			ALOGE("AW sensor error no data pollin\n");
	}
	return nbEvents;
}

/*****************************************************************************/

static int poll__batch(struct sensors_poll_device_1* dev,
		  int handle, int flags, int64_t sampling_period_ns,
		  int64_t max_report_latency_ns)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	return ctx->setDelay(handle,sampling_period_ns);
}


static int poll__close(struct hw_device_t *dev)
{
        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;

        if (ctx) {
                delete ctx;
        }
        return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {

        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
        return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {

        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
        return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {

        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
        return ctx->pollEvents(data, count);
}


static int poll__flush(struct sensors_poll_device_1 *dev,
		int handle) {

		ALOGW("AW poll__flush handle %d\n",handle);
		sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
		ctx->setFlushPending(handle,true);
		return 0;
}


/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
        //insmodDevice();/*Automatic detection, loading device drivers */
        property_set("sys.sensors", "1");/*Modify the  enable and delay interface  group */
        sensorsDetect();/*detect device,filling sensor_t structure */

        sensors_poll_context_t *dev = new sensors_poll_context_t();
        memset(&dev->device, 0, sizeof(sensors_poll_device_t));

        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_3;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = poll__close;
        dev->device.activate        = poll__activate;
        dev->device.setDelay        = poll__setDelay;
        dev->device.poll            = poll__poll;
		dev->device.batch			= poll__batch;
		dev->device.flush			= poll__flush;
		dev->device.register_direct_channel = NULL;

        *device = &dev->device.common;

        return 0;
}
