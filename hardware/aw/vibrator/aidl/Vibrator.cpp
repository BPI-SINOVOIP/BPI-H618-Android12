/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <log/log.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stddef.h>
#include <linux/input.h>
#include "vibrator-impl/Vibrator.h"

#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

#define TIMEOUT_STR_LEN 20
#define VIBRATOR_FILE_LEN 64

struct vibrator_device_t {
    int (*vibrator_off)();
    int (*vibrator_on)(int32_t timeoutMs);
};

static bool vibra_flag = false;
static vibrator_device_t vibra_device;
static const char THE_DEVICE[] = "/sys/class/timed_output/vibrator/enable";

static bool device_exists(const char *file) {
    int fd;

    fd = TEMP_FAILURE_RETRY(open(file, O_RDWR));
    if (fd < 0) {
        ALOGD("open %s failed, errno=%d(%s)\n", file, errno, strerror(errno));
        return false;
    }

    close(fd);
    return true;
}

static bool vibra_exists() { return device_exists(THE_DEVICE); }

static int write_value(const char *file, const char *value) {
    int to_write, written, ret, fd;

    fd = TEMP_FAILURE_RETRY(open(file, O_WRONLY));
    if (fd < 0) {
        return -errno;
    }

    to_write = strlen(value) + 1;
    written = TEMP_FAILURE_RETRY(write(fd, value, to_write));
    if (written == -1) {
        ALOGE("write value is error, please check!");
        vibra_flag = false;
        ret = -errno;
    } else if (written != to_write) {
    /* even though EAGAIN is an errno value that could be set
       by write() in some cases, none of them apply here.  So, this return
       value can be clearly identified when debugging and suggests the
       caller that it may try to call vibrator_on() again */
        ret = -EAGAIN;
    } else {
        ret = 0;
    }

    errno = 0;
    close(fd);

    return ret;
}

static int sendit(int32_t timeoutMs) {
    char value[TIMEOUT_STR_LEN]; /* large enough for millions of years */

    snprintf(value, sizeof(value), "%u", timeoutMs);
    return write_value(THE_DEVICE, value);
}

static int vibra_on(int32_t timeoutMs) {
    /* constant on, up to maximum allowed time */
    return sendit(timeoutMs);
}

static int vibra_off() { return sendit(0); }

static bool get_vibrator_devices() {
    if (vibra_exists()) {
        vibra_device.vibrator_off = vibra_off;
        vibra_device.vibrator_on = vibra_on;
        return true;
	}
	return false;
}

ndk::ScopedAStatus Vibrator::off() {
    int ret = 0;

    if (!vibra_flag) {
        if (get_vibrator_devices()) {
            vibra_flag = true;
        }
    }

    if (!vibra_flag) {
        return ndk::ScopedAStatus::ok();
    }

    ret = vibra_device.vibrator_off();
    if (ret) ALOGE("%s fail:%d", __func__, ret);

    ALOGD("vibra_off");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(
    int32_t timeoutMs, const std::shared_ptr<IVibratorCallback> &callback) {
    int ret = 0;

    if (callback != nullptr) {
        ALOGD("Vibrator callback is not nullptr");
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    if (!vibra_flag) {
        if (get_vibrator_devices()) {
            vibra_flag = true;
        }
    }

    if (!vibra_flag) {
        return ndk::ScopedAStatus::ok();
    }

    ret = vibra_device.vibrator_on(timeoutMs);
    if (ret) ALOGE("%s fail:%d", __func__, ret);

    ALOGD("vibra_on %d", timeoutMs);

    return ndk::ScopedAStatus::ok();
}


ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    ALOGD("Vibra reporting capabilities");
    *_aidl_return = 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    ndk::ScopedAStatus status;
    uint32_t timeMS = 0;

    ALOGD("Vibra::perform");

    if (callback != nullptr) {
        ALOGD("Vibrator callback is not nullptr");
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    switch (effect) {
        case Effect::CLICK:
            timeMS = 45;
            break;
        case Effect::DOUBLE_CLICK:
            timeMS = 65;
            break;
        case Effect::TICK:
        case Effect::TEXTURE_TICK:
            timeMS = 30;
            break;
        case Effect::HEAVY_CLICK:
            timeMS = 55;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    if (strength != EffectStrength::LIGHT && strength != EffectStrength::MEDIUM &&
          strength != EffectStrength::STRONG) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    status = on(timeMS, callback);
    if (!status.isOk()) {
        return status;
    }

    *_aidl_return = timeMS;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    *_aidl_return = {Effect::TEXTURE_TICK, Effect::TICK, Effect::CLICK,
                   Effect::HEAVY_CLICK, Effect::DOUBLE_CLICK};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    LOG(INFO) << "Vibrator set amplitude: " << amplitude;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    LOG(INFO) << "Vibrator set external control: " << enabled;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
	return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
                                                  int32_t* durationMs) {
    if (primitive != CompositePrimitive::NOOP) {
        *durationMs = 100;
    } else {
        *durationMs = 0;
    }
	return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
                                     const std::shared_ptr<IVibratorCallback>& callback) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id) {
    LOG(INFO) << "Disabling always-on ID " << id;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float *resonantFreqHz) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getQFactor(float *qFactor) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float *freqResolutionHz) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float *freqMinimumHz) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float> *_aidl_return) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t *durationMs) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t *maxSize) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking> *supported) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle> &composite,
                                         const std::shared_ptr<IVibratorCallback> &callback) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
