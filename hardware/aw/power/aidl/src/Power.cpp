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

#include "Power.h"
#include "PowerNode.h"
#include "ModeExt.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#define  LOG_TAG "AW_PowerHAL"
#include <utils/Log.h>
#include <cutils/properties.h>

#define NSIBANDWIDTH "/dev/nsi"
#define DISABLE_BANDWIDTH_LIMIT  0x102
#define ENABLE_BANDWIDTH_LIMIT  0x101


namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace aw {

Power::Power() : mPowerEnable(true),
                 mBootCompleted(false),
                 mCurPowerMode((int)Mode::INTERACTIVE),
                 mPrePowerMode((int)Mode::INTERACTIVE) {
    ALOGI("PowerHAL initiated");
}

bool Power::getBootCompleted() {
    return mBootCompleted;
}

void Power::setBootCompleted(bool bootCompleted) {
    mBootCompleted = bootCompleted;
}

int Power::getCurPowerMode() {
    return mCurPowerMode;
}
void Power::setCurPowerMode(int mode) {
    if (mode != mCurPowerMode) {
        setPrePowerMode(mCurPowerMode);
        mCurPowerMode = mode;
    }
}

int Power::getPrePowerMode() {
    return mPrePowerMode;
}

void Power::setPrePowerMode(int mode) {
    mPrePowerMode = mode;
}

static void sysfs_write(const char *node, const char *value)
{
    char buf[64];
    int len;
    int fd = open(node, O_WRONLY);
    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("error opening %s: %s\n", node, buf);
        return;
    }
    len = write(fd, value, strlen(value));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("error writing to %s: %s\n", node, buf);
    }
    close(fd);
}

bool Power::checkPowerEnable() {
   char propval[PROPERTY_VALUE_MAX] = {'\0'};
   property_get("persist.vendor.power.disable", propval,"0");
   if(!strcmp(propval,"1")) {
     ALOGI("Power disabled");
     mPowerEnable = false;
   }
    return mPowerEnable;
}

static bool nsi_bandwidth_limit = true;
static void set_nsi_bandwidth_limit(bool enable) {

    int fd = open(NSIBANDWIDTH, O_WRONLY);
    if (fd < 0) {
        ALOGW("Could not open nsi-bw: %s", strerror(errno));
        return;
    }

    if(ioctl(fd,enable? ENABLE_BANDWIDTH_LIMIT:DISABLE_BANDWIDTH_LIMIT,1)) {
        ALOGE("ioctl %s limit %d eroor %s",NSIBANDWIDTH,enable, strerror(errno));
    } else {
        nsi_bandwidth_limit = enable;
    }

    close(fd);
}

static void set_mq_deadline_low_latency() {
    sysfs_write(IO_SCHEDULER,"mq-deadline");
    sysfs_write(MQ_READ_EXPIRE,"100");
    sysfs_write(MQ_WRITE_EXPIRE,"1000");
    sysfs_write(MQ_WRITES_STARVED,"2");
    sysfs_write(MQ_FIFO_BATCH,"12");
}

static void set_mq_deadline_read_priority_latency() {
    sysfs_write(IO_SCHEDULER,"mq-deadline");
    sysfs_write(MQ_READ_EXPIRE,"250");
    sysfs_write(MQ_WRITES_STARVED,"4");
    sysfs_write(MQ_FIFO_BATCH,"32");
}

static void set_mq_deadline_high_batch() {

    sysfs_write(IO_SCHEDULER,"mq-deadline");
    sysfs_write(MQ_READ_EXPIRE,"750");
    sysfs_write(MQ_WRITES_STARVED,"1");
    sysfs_write(MQ_FIFO_BATCH,"64");
}

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {

    ALOGI("Power setMode: %s to: %d\n", toString(type).c_str(), enabled);
    if(!mPowerEnable) {
        ALOGI("Power disabled");
        return ndk::ScopedAStatus::ok();
    }

    int setMode = (int)type;
    //Before system boot completed, we won't take any action. That's for we have to let system run
    //at BENCHMARK Mode, which is been set in kernel, at boot stage. Only if system completed boot,
    //then Power Hal will handle setMode and switch to other modes.
    if (!getBootCompleted() && (setMode != (int)Mode::SUSTAINED_PERFORMANCE)) {
        ALOGI("not completed yet, ignore mode request other");
        return ndk::ScopedAStatus::ok();
    }

    if((setMode != (int)Mode::DISPLAY_INACTIVE)
       && (setMode != (int)Mode::SUSTAINED_PERFORMANCE)
       && (setMode != (int)Mode::INTERACTIVE)) {//except TP-operation、bootcomplete-false,special
        //If the request is to disable a power mode, we must currently be in this mode, otherwise ignore the request.
        if (!enabled && (mCurPowerMode != setMode)) {
            ALOGI("invalid mode disable request. current mode: %d, requested disable mode: %d", mCurPowerMode,setMode);
            return ndk::ScopedAStatus::ok();
        } else if(!enabled) { //disable request,set to normal mode .
            setMode = (int)Mode::INTERACTIVE;
        }
    }




    bool hitMode = true;
    switch (setMode) {
         case (int)Mode::INTERACTIVE:
            sysfs_write(THERMAL_MODE,THERMAL_ENABLE);
            sysfs_write(CPU0GOV, SCHEDUTIL_GOVERNOR);
            set_mq_deadline_low_latency();
            if(!nsi_bandwidth_limit) {
                set_nsi_bandwidth_limit(1);
            }

            ALOGI("cur-mode:SCHEDUTIL");
            break;
        case (int)Mode::LAUNCH:
            //For launch mode, enable thermal and force cpu to work at max frequency.
            set_mq_deadline_read_priority_latency();

            sysfs_write(THERMAL_MODE,THERMAL_ENABLE);
            sysfs_write(CPU0GOV, PERFORMANCE_GOVERNOR);
            ALOGI("cur-mode:LAUNCH");
            break;
        case (int)Mode::SUSTAINED_PERFORMANCE:
            if(enabled) {
                //For benchmark mode, disable thermal and force cpu to work at max frequency.
                set_mq_deadline_high_batch();
                sysfs_write(THERMAL_MODE,THERMAL_DISABLE);
                sysfs_write(CPU0GOV, PERFORMANCE_GOVERNOR);

                set_nsi_bandwidth_limit(0);

                ALOGI("cur-mode:PERFORMANCE");
            } else {

                if (!getBootCompleted()) {
                     setBootCompleted(true);
                     ALOGI("BootCompleted");
                     if(!checkPowerEnable()) {
                     return ndk::ScopedAStatus::ok();
                    }
                }
                sysfs_write(CPU0GOV, SCHEDUTIL_GOVERNOR);
                set_mq_deadline_low_latency();
                ALOGI("cur-mode:SCHEDUTIL-Home");

            }
            break;

        case (int)Mode::DISPLAY_INACTIVE:
            if(enabled) {
                sysfs_write(TP_SUSPEND_CONTROL,"1");// tp suspend enable-screen off
                //set_mq_deadline_high_batch();
            } else {
                sysfs_write(TP_SUSPEND_CONTROL,"0");// tp suspend disable-screen on
                //set_mq_deadline_low_latency();
            }
            ALOGI("cur-mode:DISPLAY_INACTIVE set %d",enabled);
            break;

        case (int)Mode::DEVICE_IDLE:
            [[fallthrough]];
        case (int)Mode::VR:
            [[fallthrough]];
        case (int)Mode::DOUBLE_TAP_TO_WAKE:
            [[fallthrough]];
        case (int)Mode::FIXED_PERFORMANCE:
            [[fallthrough]];
        case (int)Mode::EXPENSIVE_RENDERING:
            [[fallthrough]];
        case (int)Mode::LOW_POWER:
            [[fallthrough]];
        case (int)Mode::AUDIO_STREAMING_LOW_LATENCY:
            [[fallthrough]];
        case (int)Mode::CAMERA_STREAMING_SECURE:
            [[fallthrough]];
        case (int)Mode::CAMERA_STREAMING_LOW:
            [[fallthrough]];
        case (int)Mode::CAMERA_STREAMING_MID:
            [[fallthrough]];
        case (int)Mode::CAMERA_STREAMING_HIGH:
            hitMode = false;
            break;
        default:
            hitMode = false;
            ALOGE(" invalid mode at setMode: %d\n",type);
            break;
    }

    if(hitMode) {
        setCurPowerMode(setMode);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    switch ((int)type) {
        case (int)Mode::INTERACTIVE:
            [[fallthrough]];
        case (int)Mode::SUSTAINED_PERFORMANCE:
            [[fallthrough]];
        case (int)Mode::DEVICE_IDLE:
            [[fallthrough]];
        case (int)Mode::DISPLAY_INACTIVE:
            [[fallthrough]];
        case (int)Mode::LAUNCH:
        case (int)Mode::FIXED_PERFORMANCE:
            *_aidl_return = true;
            break;
        default:
            *_aidl_return = false;
    }
    if(!*_aidl_return) {
        ALOGI("check: Mode %d not Supported ,do nothing",type);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost type, int durationMs) {
    ALOGV("setBoost: %d, duration: %d\n", type, durationMs);
    return ndk::ScopedAStatus::ok();
}

//Currently we don't support setBoost.
ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool* _aidl_return) {
    ALOGI("isBoostSupported: %d\n", type);
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

binder_status_t Power::dump(int fd, const char **, unsigned int) {
    std::string buf(::android::base::StringPrintf(
            "mBootCompleted: %d\n"
            "mCurPowerMode: %d\n"
            "mPrePowerMode: %d\n",
            mBootCompleted, mCurPowerMode,mPrePowerMode));
    if (!::android::base::WriteStringToFd(buf, fd)) {
        PLOG(ERROR) << "Failed to dump state to fd";
    }
    fsync(fd);
    return STATUS_OK;
}

}  // namespace aw
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
