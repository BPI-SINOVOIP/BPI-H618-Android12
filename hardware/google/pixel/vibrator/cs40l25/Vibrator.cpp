/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "Vibrator.h"

#include <hardware/hardware.h>
#include <hardware/vibrator.h>
#include <log/log.h>
#include <stdio.h>
#include <utils/Trace.h>

#include <cinttypes>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#define PROC_SND_PCM "/proc/asound/pcm"
#define HAPTIC_PCM_DEVICE_SYMBOL "haptic nohost playback"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

static constexpr uint32_t BASE_CONTINUOUS_EFFECT_OFFSET = 32768;

static constexpr uint32_t WAVEFORM_EFFECT_0_20_LEVEL = 0;
static constexpr uint32_t WAVEFORM_EFFECT_1_00_LEVEL = 4;
static constexpr uint32_t WAVEFORM_EFFECT_LEVEL_MINIMUM = 4;

static constexpr uint32_t WAVEFORM_DOUBLE_CLICK_SILENCE_MS = 100;

static constexpr uint32_t WAVEFORM_LONG_VIBRATION_EFFECT_INDEX = 0;
static constexpr uint32_t WAVEFORM_LONG_VIBRATION_THRESHOLD_MS = 50;
static constexpr uint32_t WAVEFORM_SHORT_VIBRATION_EFFECT_INDEX = 3 + BASE_CONTINUOUS_EFFECT_OFFSET;

static constexpr uint32_t WAVEFORM_CLICK_INDEX = 2;
static constexpr uint32_t WAVEFORM_THUD_INDEX = 4;
static constexpr uint32_t WAVEFORM_SPIN_INDEX = 5;
static constexpr uint32_t WAVEFORM_QUICK_RISE_INDEX = 6;
static constexpr uint32_t WAVEFORM_SLOW_RISE_INDEX = 7;
static constexpr uint32_t WAVEFORM_QUICK_FALL_INDEX = 8;
static constexpr uint32_t WAVEFORM_LIGHT_TICK_INDEX = 9;
static constexpr uint32_t WAVEFORM_LOW_TICK_INDEX = 10;

static constexpr uint32_t WAVEFORM_UNSAVED_TRIGGER_QUEUE_INDEX = 65529;
static constexpr uint32_t WAVEFORM_TRIGGER_QUEUE_INDEX = 65534;

static constexpr uint32_t VOLTAGE_GLOBAL_SCALE_LEVEL = 5;
static constexpr uint8_t VOLTAGE_SCALE_MAX = 100;

static constexpr int8_t MAX_COLD_START_LATENCY_MS = 6;  // I2C Transaction + DSP Return-From-Standby
static constexpr int8_t MAX_PAUSE_TIMING_ERROR_MS = 1;  // ALERT Irq Handling
static constexpr uint32_t MAX_TIME_MS = UINT32_MAX;

static constexpr float AMP_ATTENUATE_STEP_SIZE = 0.125f;
static constexpr float EFFECT_FREQUENCY_KHZ = 48.0f;

static constexpr auto ASYNC_COMPLETION_TIMEOUT = std::chrono::milliseconds(100);

static constexpr int32_t COMPOSE_DELAY_MAX_MS = 10000;
static constexpr int32_t COMPOSE_SIZE_MAX = 127;
static constexpr int32_t COMPOSE_PWLE_SIZE_MAX_DEFAULT = 127;


// Measured resonant frequency, f0_measured, is represented by Q10.14 fixed
// point format on cs40l2x devices. The expression to calculate f0 is:
//   f0 = f0_measured / 2^Q14_BIT_SHIFT
// See the LRA Calibration Support documentation for more details.
static constexpr int32_t Q14_BIT_SHIFT = 14;

// Measured Q factor, q_measured, is represented by Q8.16 fixed
// point format on cs40l2x devices. The expression to calculate q is:
//   q = q_measured / 2^Q16_BIT_SHIFT
// See the LRA Calibration Support documentation for more details.
static constexpr int32_t Q16_BIT_SHIFT = 16;

static constexpr int32_t COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS = 16383;
static constexpr float PWLE_LEVEL_MIN = 0.0;
static constexpr float PWLE_LEVEL_MAX = 1.0;
static constexpr float CS40L2X_PWLE_LEVEL_MAX = 0.999511;
static constexpr float PWLE_FREQUENCY_RESOLUTION_HZ = 0.25;
static constexpr float PWLE_FREQUENCY_MIN_HZ = 0.25;
static constexpr float PWLE_FREQUENCY_MAX_HZ = 1023.75;
static constexpr float PWLE_BW_MAP_SIZE =
    1 + ((PWLE_FREQUENCY_MAX_HZ - PWLE_FREQUENCY_MIN_HZ) / PWLE_FREQUENCY_RESOLUTION_HZ);

static struct pcm_config haptic_nohost_config = {
    .channels = 1,
    .rate = 48000,
    .period_size = 80,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

static uint8_t amplitudeToScale(float amplitude, float maximum) {
    return std::round((-20 * std::log10(amplitude / static_cast<float>(maximum))) /
                      (AMP_ATTENUATE_STEP_SIZE));
}

// Discrete points of frequency:max_level pairs as recommended by the document
// [R4O6] Max. Allowable Chirp Levels (go/r4o6-max-chirp-levels) around resonant frequency
static std::map<float, float> discretePwleMaxLevels = {{120.0, 0.4},  {130.0, 0.31}, {140.0, 0.14},
                                                       {145.0, 0.09}, {150.0, 0.15}, {160.0, 0.35},
                                                       {170.0, 0.4}};

// Initialize all limits to 0.4 according to the document [R4O6] Max. Allowable Chirp Levels
// (go/r4o6-max-chirp-levels)
std::vector<float> pwleMaxLevelLimitMap(PWLE_BW_MAP_SIZE, 0.4);

void Vibrator::createPwleMaxLevelLimitMap() {
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        std::map<float, float>::iterator itr0, itr1;

        itr0 = discretePwleMaxLevels.begin();
        itr1 = std::next(itr0, 1);

        while (itr1 != discretePwleMaxLevels.end()) {
            float x0 = itr0->first;
            float y0 = itr0->second;
            float x1 = itr1->first;
            float y1 = itr1->second;
            float pwleMaxLevelLimitMapIdx =
                (itr0->first - PWLE_FREQUENCY_MIN_HZ) / PWLE_FREQUENCY_RESOLUTION_HZ;

            for (float xp = x0; xp < (x1 + PWLE_FREQUENCY_RESOLUTION_HZ);
                 xp += PWLE_FREQUENCY_RESOLUTION_HZ) {
                float yp = y0 + ((y1 - y0) / (x1 - x0)) * (xp - x0);

                pwleMaxLevelLimitMap[pwleMaxLevelLimitMapIdx++] = yp;
            }

            itr0++;
            itr1++;
        }
    }
}

enum class AlwaysOnId : uint32_t {
    GPIO_RISE,
    GPIO_FALL,
};

Vibrator::Vibrator(std::unique_ptr<HwApi> hwapi, std::unique_ptr<HwCal> hwcal)
    : mHwApi(std::move(hwapi)), mHwCal(std::move(hwcal)), mAsyncHandle(std::async([] {})) {
    int32_t longFreqencyShift;
    uint32_t calVer;
    uint32_t caldata;
    uint32_t effectCount;

    if (!mHwApi->setState(true)) {
        ALOGE("Failed to set state (%d): %s", errno, strerror(errno));
    }

    if (mHwCal->getF0(&caldata)) {
        mHwApi->setF0(caldata);
    }
    if (mHwCal->getRedc(&caldata)) {
        mHwApi->setRedc(caldata);
    }
    if (mHwCal->getQ(&caldata)) {
        mHwApi->setQ(caldata);
    }

    mHwCal->getLongFrequencyShift(&longFreqencyShift);
    if (longFreqencyShift > 0) {
        mF0Offset = longFreqencyShift * std::pow(2, 14);
    } else if (longFreqencyShift < 0) {
        mF0Offset = std::pow(2, 24) - std::abs(longFreqencyShift) * std::pow(2, 14);
    } else {
        mF0Offset = 0;
    }

    mHwCal->getVersion(&calVer);
    if (calVer == 1) {
        std::array<uint32_t, 6> volLevels;
        mHwCal->getVolLevels(&volLevels);
        /*
         * Given voltage levels for two intensities, assuming a linear function,
         * solve for 'f(0)' in 'v = f(i) = a + b * i' (i.e 'v0 - (v1 - v0) / ((i1 - i0) / i0)').
         */
        mClickEffectVol[0] = std::max(std::lround(volLevels[WAVEFORM_EFFECT_0_20_LEVEL] -
                                             (volLevels[WAVEFORM_EFFECT_1_00_LEVEL] -
                                              volLevels[WAVEFORM_EFFECT_0_20_LEVEL]) /
                                                     4.0f),
                                 static_cast<long>(WAVEFORM_EFFECT_LEVEL_MINIMUM));
        mClickEffectVol[1] = volLevels[WAVEFORM_EFFECT_1_00_LEVEL];
        mTickEffectVol = mClickEffectVol;
        mLongEffectVol[0] = 0;
        mLongEffectVol[1] = volLevels[VOLTAGE_GLOBAL_SCALE_LEVEL];
    } else {
        mHwCal->getTickVolLevels(&mTickEffectVol);
        mHwCal->getClickVolLevels(&mClickEffectVol);
        mHwCal->getLongVolLevels(&mLongEffectVol);
    }

    mHwApi->getEffectCount(&effectCount);
    mEffectDurations.resize(effectCount);
    for (size_t effectIndex = 0; effectIndex < effectCount; effectIndex++) {
        mHwApi->setEffectIndex(effectIndex);
        uint32_t effectDuration;
        if (mHwApi->getEffectDuration(&effectDuration)) {
            mEffectDurations[effectIndex] = std::ceil(effectDuration / EFFECT_FREQUENCY_KHZ);
        }
    }

    mHwApi->setClabEnable(true);

    if (!(getPwleCompositionSizeMax(&compositionSizeMax).isOk())) {
        ALOGE("Failed to get pwle composition size max, using default size: %d",
              COMPOSE_PWLE_SIZE_MAX_DEFAULT);
        compositionSizeMax = COMPOSE_PWLE_SIZE_MAX_DEFAULT;
    }

    createPwleMaxLevelLimitMap();
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t *_aidl_return) {
    ATRACE_NAME("Vibrator::getCapabilities");
    int32_t ret = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK |
                  IVibrator::CAP_COMPOSE_EFFECTS | IVibrator::CAP_ALWAYS_ON_CONTROL |
                  IVibrator::CAP_GET_RESONANT_FREQUENCY | IVibrator::CAP_GET_Q_FACTOR;
    if (mHwApi->hasEffectScale()) {
        ret |= IVibrator::CAP_AMPLITUDE_CONTROL;
    }
    if (mHwApi->hasAspEnable() || hasHapticAlsaDevice()) {
        ret |= IVibrator::CAP_EXTERNAL_CONTROL;
    }
    if (mHwApi->hasPwle()) {
        ret |= IVibrator::CAP_FREQUENCY_CONTROL | IVibrator::CAP_COMPOSE_PWLE_EFFECTS;
    }
    *_aidl_return = ret;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    ATRACE_NAME("Vibrator::off");
    setGlobalAmplitude(false);
    mHwApi->setF0Offset(0);
    if (!mHwApi->setActivate(0)) {
        ALOGE("Failed to turn vibrator off (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback> &callback) {
    ATRACE_NAME("Vibrator::on");
    const uint32_t index = timeoutMs < WAVEFORM_LONG_VIBRATION_THRESHOLD_MS
                                   ? WAVEFORM_SHORT_VIBRATION_EFFECT_INDEX
                                   : WAVEFORM_LONG_VIBRATION_EFFECT_INDEX;
    if (MAX_COLD_START_LATENCY_MS <= UINT32_MAX - timeoutMs) {
        timeoutMs += MAX_COLD_START_LATENCY_MS;
    }
    setGlobalAmplitude(true);
    mHwApi->setF0Offset(mF0Offset);
    return on(timeoutMs, index, callback);
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback> &callback,
                                     int32_t *_aidl_return) {
    ATRACE_NAME("Vibrator::perform");
    return performEffect(effect, strength, callback, _aidl_return);
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect> *_aidl_return) {
    *_aidl_return = {Effect::TEXTURE_TICK, Effect::TICK, Effect::CLICK, Effect::HEAVY_CLICK,
                     Effect::DOUBLE_CLICK};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    ATRACE_NAME("Vibrator::setAmplitude");
    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (!isUnderExternalControl()) {
        return setEffectAmplitude(amplitude, 1.0);
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    ATRACE_NAME("Vibrator::setExternalControl");
    setGlobalAmplitude(enabled);

    if (mHasHapticAlsaDevice) {
        if (!enableHapticPcmAmp(&mHapticPcm, enabled, mCard, mDevice)) {
            ALOGE("Failed to %s haptic pcm device: %d", (enabled ? "enable" : "disable"), mDevice);
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
    }
    if (mHwApi->hasAspEnable()) {
        if (!mHwApi->setAspEnable(enabled)) {
            ALOGE("Failed to set external control (%d): %s", errno, strerror(errno));
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
    }

    mIsUnderExternalControl = enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t *maxDelayMs) {
    ATRACE_NAME("Vibrator::getCompositionDelayMax");
    *maxDelayMs = COMPOSE_DELAY_MAX_MS;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t *maxSize) {
    ATRACE_NAME("Vibrator::getCompositionSizeMax");
    *maxSize = COMPOSE_SIZE_MAX;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive> *supported) {
    *supported = {
            CompositePrimitive::NOOP,       CompositePrimitive::CLICK,
            CompositePrimitive::THUD,       CompositePrimitive::SPIN,
            CompositePrimitive::QUICK_RISE, CompositePrimitive::SLOW_RISE,
            CompositePrimitive::QUICK_FALL, CompositePrimitive::LIGHT_TICK,
            CompositePrimitive::LOW_TICK,
    };
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
                                                  int32_t *durationMs) {
    ndk::ScopedAStatus status;
    uint32_t effectIndex;

    if (primitive != CompositePrimitive::NOOP) {
        status = getPrimitiveDetails(primitive, &effectIndex);
        if (!status.isOk()) {
            return status;
        }

        *durationMs = mEffectDurations[effectIndex];
    } else {
        *durationMs = 0;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect> &composite,
                                     const std::shared_ptr<IVibratorCallback> &callback) {
    ATRACE_NAME("Vibrator::compose");
    std::ostringstream effectBuilder;
    std::string effectQueue;

    if (composite.size() > COMPOSE_SIZE_MAX) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    for (auto &e : composite) {
        if (e.scale < 0.0f || e.scale > 1.0f) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        if (e.delayMs) {
            if (e.delayMs > COMPOSE_DELAY_MAX_MS) {
                return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
            }
            effectBuilder << e.delayMs << ",";
        }
        if (e.primitive != CompositePrimitive::NOOP) {
            ndk::ScopedAStatus status;
            uint32_t effectIndex;

            status = getPrimitiveDetails(e.primitive, &effectIndex);
            if (!status.isOk()) {
                return status;
            }

            effectBuilder << effectIndex << "." << intensityToVolLevel(e.scale, effectIndex) << ",";
        }
    }

    if (effectBuilder.tellp() == 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    effectBuilder << 0;

    effectQueue = effectBuilder.str();

    return performEffect(0 /*ignored*/, 0 /*ignored*/, &effectQueue, callback);
}

ndk::ScopedAStatus Vibrator::on(uint32_t timeoutMs, uint32_t effectIndex,
                                const std::shared_ptr<IVibratorCallback> &callback) {
    if (mAsyncHandle.wait_for(ASYNC_COMPLETION_TIMEOUT) != std::future_status::ready) {
        ALOGE("Previous vibration pending.");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    mHwApi->setEffectIndex(effectIndex);
    mHwApi->setDuration(timeoutMs);
    mHwApi->setActivate(1);

    mAsyncHandle = std::async(&Vibrator::waitForComplete, this, callback);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setEffectAmplitude(float amplitude, float maximum) {
    int32_t scale = amplitudeToScale(amplitude, maximum);

    if (!mHwApi->setEffectScale(scale)) {
        ALOGE("Failed to set effect amplitude (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setGlobalAmplitude(bool set) {
    uint8_t amplitude = set ? mLongEffectVol[1] : VOLTAGE_SCALE_MAX;
    int32_t scale = amplitudeToScale(amplitude, VOLTAGE_SCALE_MAX);

    if (!mHwApi->setGlobalScale(scale)) {
        ALOGE("Failed to set global amplitude (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect> *_aidl_return) {
    *_aidl_return = {Effect::TEXTURE_TICK, Effect::TICK, Effect::CLICK, Effect::HEAVY_CLICK};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) {
    ndk::ScopedAStatus status;
    uint32_t effectIndex;
    uint32_t timeMs;
    uint32_t volLevel;
    uint32_t scale;

    status = getSimpleDetails(effect, strength, &effectIndex, &timeMs, &volLevel);
    if (!status.isOk()) {
        return status;
    }

    scale = amplitudeToScale(volLevel, VOLTAGE_SCALE_MAX);

    switch (static_cast<AlwaysOnId>(id)) {
        case AlwaysOnId::GPIO_RISE:
            mHwApi->setGpioRiseIndex(effectIndex);
            mHwApi->setGpioRiseScale(scale);
            return ndk::ScopedAStatus::ok();
        case AlwaysOnId::GPIO_FALL:
            mHwApi->setGpioFallIndex(effectIndex);
            mHwApi->setGpioFallScale(scale);
            return ndk::ScopedAStatus::ok();
    }

    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id) {
    switch (static_cast<AlwaysOnId>(id)) {
        case AlwaysOnId::GPIO_RISE:
            mHwApi->setGpioRiseIndex(0);
            return ndk::ScopedAStatus::ok();
        case AlwaysOnId::GPIO_FALL:
            mHwApi->setGpioFallIndex(0);
            return ndk::ScopedAStatus::ok();
    }

    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float *resonantFreqHz) {
    uint32_t caldata;
    if (!mHwCal->getF0(&caldata)) {
        ALOGE("Failed to get resonant frequency (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *resonantFreqHz = static_cast<float>(caldata) / (1 << Q14_BIT_SHIFT);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getQFactor(float *qFactor) {
    uint32_t caldata;
    if (!mHwCal->getQ(&caldata)) {
        ALOGE("Failed to get q factor (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *qFactor = static_cast<float>(caldata) / (1 << Q16_BIT_SHIFT);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float *freqResolutionHz) {
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        *freqResolutionHz = PWLE_FREQUENCY_RESOLUTION_HZ;
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float *freqMinimumHz) {
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        *freqMinimumHz = PWLE_FREQUENCY_MIN_HZ;
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float> *_aidl_return) {
    // TODO(b/170919640): complete implementation
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        std::vector<float> bandwidthAmplitudeMap(PWLE_BW_MAP_SIZE, 1.0);
        *_aidl_return = bandwidthAmplitudeMap;
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t *durationMs) {
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        *durationMs = COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS;
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t *maxSize) {
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        uint32_t segments;
        if (!mHwApi->getAvailablePwleSegments(&segments)) {
            ALOGE("Failed to get availablePwleSegments (%d): %s", errno, strerror(errno));
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        *maxSize = segments;
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking> *supported) {
    int32_t capabilities;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        *supported = {
            Braking::NONE,
            Braking::CLAB,
        };
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::setPwle(const std::string &pwleQueue) {
    if (!mHwApi->setPwle(pwleQueue)) {
        ALOGE("Failed to write \"%s\" to pwle (%d): %s", pwleQueue.c_str(), errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

static void resetPreviousEndAmplitudeEndFrequency(float &prevEndAmplitude,
                                                  float &prevEndFrequency) {
    const float reset = -1.0;
    prevEndAmplitude = reset;
    prevEndFrequency = reset;
}

static void incrementIndex(int &index) {
    index += 1;
}

static void constructActiveDefaults(std::ostringstream &pwleBuilder, const int &segmentIdx) {
    pwleBuilder << ",C" << segmentIdx << ":1";
    pwleBuilder << ",B" << segmentIdx << ":0";
    pwleBuilder << ",AR" << segmentIdx << ":0";
    pwleBuilder << ",V" << segmentIdx << ":0";
}

static void constructActiveSegment(std::ostringstream &pwleBuilder, const int &segmentIdx,
                                   int duration, float amplitude, float frequency) {
    pwleBuilder << ",T" << segmentIdx << ":" << duration;
    pwleBuilder << ",L" << segmentIdx << ":" << amplitude;
    pwleBuilder << ",F" << segmentIdx << ":" << frequency;
    constructActiveDefaults(pwleBuilder, segmentIdx);
}

static void constructBrakingSegment(std::ostringstream &pwleBuilder, const int &segmentIdx,
                                    int duration, Braking brakingType) {
    pwleBuilder << ",T" << segmentIdx << ":" << duration;
    pwleBuilder << ",L" << segmentIdx << ":" << 0;
    pwleBuilder << ",F" << segmentIdx << ":" << PWLE_FREQUENCY_MIN_HZ;
    pwleBuilder << ",C" << segmentIdx << ":0";
    pwleBuilder << ",B" << segmentIdx << ":"
                << static_cast<std::underlying_type<Braking>::type>(brakingType);
    pwleBuilder << ",AR" << segmentIdx << ":0";
    pwleBuilder << ",V" << segmentIdx << ":0";
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle> &composite,
                                         const std::shared_ptr<IVibratorCallback> &callback) {
    ATRACE_NAME("Vibrator::composePwle");
    std::ostringstream pwleBuilder;
    std::string pwleQueue;

    if (composite.size() <= 0 || composite.size() > compositionSizeMax) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    float prevEndAmplitude;
    float prevEndFrequency;
    resetPreviousEndAmplitudeEndFrequency(prevEndAmplitude, prevEndFrequency);

    int segmentIdx = 0;
    uint32_t totalDuration = 0;

    pwleBuilder << "S:0,WF:4,RP:0,WT:0";

    for (auto &e : composite) {
        switch (e.getTag()) {
            case PrimitivePwle::active: {
                auto active = e.get<PrimitivePwle::active>();
                if (active.duration < 0 ||
                    active.duration > COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
                if (active.startAmplitude < PWLE_LEVEL_MIN ||
                    active.startAmplitude > PWLE_LEVEL_MAX ||
                    active.endAmplitude < PWLE_LEVEL_MIN || active.endAmplitude > PWLE_LEVEL_MAX) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
                if (active.startAmplitude > CS40L2X_PWLE_LEVEL_MAX) {
                    active.startAmplitude = CS40L2X_PWLE_LEVEL_MAX;
                }
                if (active.endAmplitude > CS40L2X_PWLE_LEVEL_MAX) {
                    active.endAmplitude = CS40L2X_PWLE_LEVEL_MAX;
                }

                if (active.startFrequency < PWLE_FREQUENCY_MIN_HZ ||
                    active.startFrequency > PWLE_FREQUENCY_MAX_HZ ||
                    active.endFrequency < PWLE_FREQUENCY_MIN_HZ ||
                    active.endFrequency > PWLE_FREQUENCY_MAX_HZ) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }

                // clip to the hard limit on input level from pwleMaxLevelLimitMap
                float maxLevelLimit =
                    pwleMaxLevelLimitMap[active.startFrequency / PWLE_FREQUENCY_RESOLUTION_HZ - 1];
                if (active.startAmplitude > maxLevelLimit) {
                    active.startAmplitude = maxLevelLimit;
                }
                maxLevelLimit =
                    pwleMaxLevelLimitMap[active.endFrequency / PWLE_FREQUENCY_RESOLUTION_HZ - 1];
                if (active.endAmplitude > maxLevelLimit) {
                    active.endAmplitude = maxLevelLimit;
                }

                if (!((active.startAmplitude == prevEndAmplitude) &&
                      (active.startFrequency == prevEndFrequency))) {
                    constructActiveSegment(pwleBuilder, segmentIdx, 0, active.startAmplitude,
                                           active.startFrequency);
                    incrementIndex(segmentIdx);
                }

                constructActiveSegment(pwleBuilder, segmentIdx, active.duration,
                                       active.endAmplitude, active.endFrequency);
                incrementIndex(segmentIdx);

                prevEndAmplitude = active.endAmplitude;
                prevEndFrequency = active.endFrequency;
                totalDuration += active.duration;
                break;
            }
            case PrimitivePwle::braking: {
                auto braking = e.get<PrimitivePwle::braking>();
                if (braking.braking > Braking::CLAB) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
                if (braking.duration > COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }

                constructBrakingSegment(pwleBuilder, segmentIdx, 0, braking.braking);
                incrementIndex(segmentIdx);

                constructBrakingSegment(pwleBuilder, segmentIdx, braking.duration, braking.braking);
                incrementIndex(segmentIdx);

                resetPreviousEndAmplitudeEndFrequency(prevEndAmplitude, prevEndFrequency);
                totalDuration += braking.duration;
                break;
            }
        }
    }

    pwleQueue = pwleBuilder.str();
    ALOGD("composePwle queue: (%s)", pwleQueue.c_str());

    ndk::ScopedAStatus status = setPwle(pwleQueue);
    if (!status.isOk()) {
        ALOGE("Failed to write pwle queue");
        return status;
    }

    setEffectAmplitude(VOLTAGE_SCALE_MAX, VOLTAGE_SCALE_MAX);
    mHwApi->setEffectIndex(WAVEFORM_UNSAVED_TRIGGER_QUEUE_INDEX);

    totalDuration += MAX_COLD_START_LATENCY_MS;
    mHwApi->setDuration(MAX_TIME_MS);

    mHwApi->setActivate(1);

    mAsyncHandle = std::async(&Vibrator::waitForComplete, this, callback);

    return ndk::ScopedAStatus::ok();
}

bool Vibrator::isUnderExternalControl() {
    return mIsUnderExternalControl;
}

binder_status_t Vibrator::dump(int fd, const char **args, uint32_t numArgs) {
    if (fd < 0) {
        ALOGE("Called debug() with invalid fd.");
        return STATUS_OK;
    }

    (void)args;
    (void)numArgs;

    dprintf(fd, "AIDL:\n");

    dprintf(fd, "  F0 Offset: %" PRIu32 "\n", mF0Offset);

    dprintf(fd, "  Voltage Levels:\n");
    dprintf(fd, "    Tick Effect Min: %" PRIu32 " Max: %" PRIu32 "\n",
            mTickEffectVol[0], mTickEffectVol[1]);
    dprintf(fd, "    Click Effect Min: %" PRIu32 " Max: %" PRIu32 "\n",
            mClickEffectVol[0], mClickEffectVol[1]);
    dprintf(fd, "    Long Effect Min: %" PRIu32 " Max: %" PRIu32 "\n",
            mLongEffectVol[0], mLongEffectVol[1]);

    dprintf(fd, "  Effect Durations:");
    for (auto d : mEffectDurations) {
        dprintf(fd, " %" PRIu32, d);
    }
    dprintf(fd, "\n");

    dprintf(fd, "\n");

    mHwApi->debug(fd);

    dprintf(fd, "\n");

    mHwCal->debug(fd);

    fsync(fd);
    return STATUS_OK;
}

ndk::ScopedAStatus Vibrator::getSimpleDetails(Effect effect, EffectStrength strength,
                                              uint32_t *outEffectIndex, uint32_t *outTimeMs,
                                              uint32_t *outVolLevel) {
    uint32_t effectIndex;
    uint32_t timeMs;
    float intensity;
    uint32_t volLevel;

    switch (strength) {
        case EffectStrength::LIGHT:
            intensity = 0.5f;
            break;
        case EffectStrength::MEDIUM:
            intensity = 0.7f;
            break;
        case EffectStrength::STRONG:
            intensity = 1.0f;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    switch (effect) {
        case Effect::TEXTURE_TICK:
            effectIndex = WAVEFORM_LIGHT_TICK_INDEX;
            intensity *= 0.5f;
            break;
        case Effect::TICK:
            effectIndex = WAVEFORM_CLICK_INDEX;
            intensity *= 0.5f;
            break;
        case Effect::CLICK:
            effectIndex = WAVEFORM_CLICK_INDEX;
            intensity *= 0.7f;
            break;
        case Effect::HEAVY_CLICK:
            effectIndex = WAVEFORM_CLICK_INDEX;
            intensity *= 1.0f;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    volLevel = intensityToVolLevel(intensity, effectIndex);
    timeMs = mEffectDurations[effectIndex] + MAX_COLD_START_LATENCY_MS;

    *outEffectIndex = effectIndex;
    *outTimeMs = timeMs;
    *outVolLevel = volLevel;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompoundDetails(Effect effect, EffectStrength strength,
                                                uint32_t *outTimeMs, uint32_t * /*outVolLevel*/,
                                                std::string *outEffectQueue) {
    ndk::ScopedAStatus status;
    uint32_t timeMs;
    std::ostringstream effectBuilder;
    uint32_t thisEffectIndex;
    uint32_t thisTimeMs;
    uint32_t thisVolLevel;

    switch (effect) {
        case Effect::DOUBLE_CLICK:
            timeMs = 0;

            status = getSimpleDetails(Effect::CLICK, strength, &thisEffectIndex, &thisTimeMs,
                                      &thisVolLevel);
            if (!status.isOk()) {
                return status;
            }
            effectBuilder << thisEffectIndex << "." << thisVolLevel;
            timeMs += thisTimeMs;

            effectBuilder << ",";

            effectBuilder << WAVEFORM_DOUBLE_CLICK_SILENCE_MS;
            timeMs += WAVEFORM_DOUBLE_CLICK_SILENCE_MS + MAX_PAUSE_TIMING_ERROR_MS;

            effectBuilder << ",";

            status = getSimpleDetails(Effect::HEAVY_CLICK, strength, &thisEffectIndex, &thisTimeMs,
                                      &thisVolLevel);
            if (!status.isOk()) {
                return status;
            }
            effectBuilder << thisEffectIndex << "." << thisVolLevel;
            timeMs += thisTimeMs;

            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    *outTimeMs = timeMs;
    *outEffectQueue = effectBuilder.str();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPrimitiveDetails(CompositePrimitive primitive,
                                                 uint32_t *outEffectIndex) {
    uint32_t effectIndex;

    switch (primitive) {
        case CompositePrimitive::NOOP:
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        case CompositePrimitive::CLICK:
            effectIndex = WAVEFORM_CLICK_INDEX;
            break;
        case CompositePrimitive::THUD:
            effectIndex = WAVEFORM_THUD_INDEX;
            break;
        case CompositePrimitive::SPIN:
            effectIndex = WAVEFORM_SPIN_INDEX;
            break;
        case CompositePrimitive::QUICK_RISE:
            effectIndex = WAVEFORM_QUICK_RISE_INDEX;
            break;
        case CompositePrimitive::SLOW_RISE:
            effectIndex = WAVEFORM_SLOW_RISE_INDEX;
            break;
        case CompositePrimitive::QUICK_FALL:
            effectIndex = WAVEFORM_QUICK_FALL_INDEX;
            break;
        case CompositePrimitive::LIGHT_TICK:
            effectIndex = WAVEFORM_LIGHT_TICK_INDEX;
            break;
        case CompositePrimitive::LOW_TICK:
            effectIndex = WAVEFORM_LOW_TICK_INDEX;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    *outEffectIndex = effectIndex;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setEffectQueue(const std::string &effectQueue) {
    if (!mHwApi->setEffectQueue(effectQueue)) {
        ALOGE("Failed to write \"%s\" to effect queue (%d): %s", effectQueue.c_str(), errno,
              strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::performEffect(Effect effect, EffectStrength strength,
                                           const std::shared_ptr<IVibratorCallback> &callback,
                                           int32_t *outTimeMs) {
    ndk::ScopedAStatus status;
    uint32_t effectIndex;
    uint32_t timeMs = 0;
    uint32_t volLevel;
    std::string effectQueue;

    switch (effect) {
        case Effect::TEXTURE_TICK:
            // fall-through
        case Effect::TICK:
            // fall-through
        case Effect::CLICK:
            // fall-through
        case Effect::HEAVY_CLICK:
            status = getSimpleDetails(effect, strength, &effectIndex, &timeMs, &volLevel);
            break;
        case Effect::DOUBLE_CLICK:
            status = getCompoundDetails(effect, strength, &timeMs, &volLevel, &effectQueue);
            break;
        default:
            status = ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
            break;
    }
    if (!status.isOk()) {
        goto exit;
    }

    status = performEffect(effectIndex, volLevel, &effectQueue, callback);

exit:

    *outTimeMs = timeMs;
    return status;
}

ndk::ScopedAStatus Vibrator::performEffect(uint32_t effectIndex, uint32_t volLevel,
                                           const std::string *effectQueue,
                                           const std::shared_ptr<IVibratorCallback> &callback) {
    if (effectQueue && !effectQueue->empty()) {
        ndk::ScopedAStatus status = setEffectQueue(*effectQueue);
        if (!status.isOk()) {
            return status;
        }
        setEffectAmplitude(VOLTAGE_SCALE_MAX, VOLTAGE_SCALE_MAX);
        effectIndex = WAVEFORM_TRIGGER_QUEUE_INDEX;
    } else {
        setEffectAmplitude(volLevel, VOLTAGE_SCALE_MAX);
    }

    return on(MAX_TIME_MS, effectIndex, callback);
}

void Vibrator::waitForComplete(std::shared_ptr<IVibratorCallback> &&callback) {
    mHwApi->pollVibeState(false);
    mHwApi->setActivate(false);

    if (callback) {
        auto ret = callback->onComplete();
        if (!ret.isOk()) {
            ALOGE("Failed completion callback: %d", ret.getExceptionCode());
        }
    }
}

uint32_t Vibrator::intensityToVolLevel(float intensity, uint32_t effectIndex) {

    uint32_t volLevel;
    auto calc = [](float intst, std::array<uint32_t, 2> v) -> uint32_t {
                return std::lround(intst * (v[1] - v[0])) + v[0]; };

    switch (effectIndex) {
        case WAVEFORM_LIGHT_TICK_INDEX:
            volLevel = calc(intensity, mTickEffectVol);
            break;
        case WAVEFORM_QUICK_RISE_INDEX:
            // fall-through
        case WAVEFORM_QUICK_FALL_INDEX:
            volLevel = calc(intensity, mLongEffectVol);
            break;
        case WAVEFORM_CLICK_INDEX:
            // fall-through
        case WAVEFORM_THUD_INDEX:
            // fall-through
        case WAVEFORM_SPIN_INDEX:
            // fall-through
        case WAVEFORM_SLOW_RISE_INDEX:
            // fall-through
        default:
            volLevel = calc(intensity, mClickEffectVol);
            break;
    }

    return volLevel;
}

bool Vibrator::findHapticAlsaDevice(int *card, int *device) {
    std::string line;
    std::ifstream myfile(PROC_SND_PCM);
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            if (line.find(HAPTIC_PCM_DEVICE_SYMBOL) != std::string::npos) {
                std::stringstream ss(line);
                std::string currentToken;
                std::getline(ss, currentToken, ':');
                sscanf(currentToken.c_str(), "%d-%d", card, device);
                return true;
            }
        }
        myfile.close();
    } else {
        ALOGE("Failed to read file: %s", PROC_SND_PCM);
    }
    return false;
}

bool Vibrator::hasHapticAlsaDevice() {
    // We need to call findHapticAlsaDevice once only. Calling in the
    // constructor is too early in the boot process and the pcm file contents
    // are empty. Hence we make the call here once only right before we need to.
    static bool configHapticAlsaDeviceDone = false;
    if (!configHapticAlsaDeviceDone) {
        if (findHapticAlsaDevice(&mCard, &mDevice)) {
            mHasHapticAlsaDevice = true;
            configHapticAlsaDeviceDone = true;
        } else {
            ALOGE("Haptic ALSA device not supported");
        }
    }
    return mHasHapticAlsaDevice;
}

bool Vibrator::enableHapticPcmAmp(struct pcm **haptic_pcm, bool enable, int card, int device) {
    int ret = 0;

    if (enable) {
        *haptic_pcm = pcm_open(card, device, PCM_OUT, &haptic_nohost_config);
        if (!pcm_is_ready(*haptic_pcm)) {
            ALOGE("cannot open pcm_out driver: %s", pcm_get_error(*haptic_pcm));
            goto fail;
        }

        ret = pcm_prepare(*haptic_pcm);
        if (ret < 0) {
            ALOGE("cannot prepare haptic_pcm: %s", pcm_get_error(*haptic_pcm));
            goto fail;
        }

        ret = pcm_start(*haptic_pcm);
        if (ret < 0) {
            ALOGE("cannot start haptic_pcm: %s", pcm_get_error(*haptic_pcm));
            goto fail;
        }

        return true;
    } else {
        if (*haptic_pcm) {
            pcm_close(*haptic_pcm);
            *haptic_pcm = NULL;
        }
        return true;
    }

fail:
    pcm_close(*haptic_pcm);
    *haptic_pcm = NULL;
    return false;
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
