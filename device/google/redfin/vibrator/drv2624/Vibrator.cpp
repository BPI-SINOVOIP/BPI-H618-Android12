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
#include "utils.h"

#include <android/looper.h>
#include <android/sensor.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/vibrator.h>
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/Trace.h>

#include <cinttypes>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

using ::android::NO_ERROR;
using ::android::UNEXPECTED_NULL;

static constexpr int8_t MAX_RTP_INPUT = 127;
static constexpr int8_t MIN_RTP_INPUT = 0;

static constexpr char RTP_MODE[] = "rtp";
static constexpr char WAVEFORM_MODE[] = "waveform";

// Use effect #1 in the waveform library for CLICK effect
static constexpr char WAVEFORM_CLICK_EFFECT_SEQ[] = "1 0";

// Use effect #2 in the waveform library for TICK effect
static constexpr char WAVEFORM_TICK_EFFECT_SEQ[] = "2 0";

// Use effect #3 in the waveform library for DOUBLE_CLICK effect
static constexpr char WAVEFORM_DOUBLE_CLICK_EFFECT_SEQ[] = "3 0";

// Use effect #4 in the waveform library for HEAVY_CLICK effect
static constexpr char WAVEFORM_HEAVY_CLICK_EFFECT_SEQ[] = "4 0";

// UT team design those target G values
static std::array<float, 5> EFFECT_TARGET_G = {0.275, 0.55, 0.6, 0.9, 1.12};
static std::array<float, 3> STEADY_TARGET_G = {2.15, 1.145, 1.3};

struct SensorContext {
    ASensorEventQueue *queue;
};
static std::vector<float> sXAxleData;
static std::vector<float> sYAxleData;
static uint64_t sEndTime = 0;
static struct timespec sGetTime;

#define MAX_VOLTAGE 3.2
#define FLOAT_EPS 1e-7
#define SENSOR_DATA_NUM 20
// Set sensing period to 2s
#define SENSING_PERIOD 2000000000
#define VIBRATION_MOTION_TIME_THRESHOLD 100
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int GSensorCallback(__attribute__((unused)) int fd, __attribute__((unused)) int events,
                    void *data) {
    ASensorEvent event;
    int event_count = 0;
    SensorContext *context = reinterpret_cast<SensorContext *>(data);
    event_count = ASensorEventQueue_getEvents(context->queue, &event, 1);
    sXAxleData.push_back(event.data[0]);
    sYAxleData.push_back(event.data[1]);
    return 1;
}
// TODO: b/152305970
int32_t PollGSensor() {
    int err = NO_ERROR, counter = 0;
    ASensorManager *sensorManager = nullptr;
    ASensorRef GSensor;
    ALooper *looper;
    struct SensorContext context = {nullptr};

    // Get proximity sensor events from the NDK
    sensorManager = ASensorManager_getInstanceForPackage("");
    if (!sensorManager) {
        ALOGI("Chase %s: Sensor manager is NULL.\n", __FUNCTION__);
        err = UNEXPECTED_NULL;
        return 0;
    }
    GSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GRAVITY);
    if (GSensor == nullptr) {
        ALOGE("%s:Chase Unable to get g sensor\n", __func__);
    } else {
        looper = ALooper_forThread();
        if (looper == nullptr) {
            looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        }
        context.queue =
            ASensorManager_createEventQueue(sensorManager, looper, 0, GSensorCallback, &context);

        err = ASensorEventQueue_registerSensor(context.queue, GSensor, 0, 0);
        if (err != NO_ERROR) {
            ALOGE("Chase %s: Error %d registering G sensor with event queue.\n", __FUNCTION__, err);
            return 0;
        }
        if (err < 0) {
            ALOGE("%s:Chase Unable to register for G sensor events\n", __func__);
        } else {
            for (counter = 0; counter < SENSOR_DATA_NUM; counter++) {
                ALooper_pollOnce(5, nullptr, nullptr, nullptr);
            }
        }
    }
    if (sensorManager != nullptr && context.queue != nullptr) {
        ASensorEventQueue_disableSensor(context.queue, GSensor);
        ASensorManager_destroyEventQueue(sensorManager, context.queue);
    }

    return 0;
}

// Temperature protection upper bound 10°C and lower bound 5°C
static constexpr int32_t TEMP_UPPER_BOUND = 10000;
static constexpr int32_t TEMP_LOWER_BOUND = 5000;
// Steady vibration's voltage in lower bound guarantee
static uint32_t STEADY_VOLTAGE_LOWER_BOUND = 90;  // 1.8 Vpeak

static std::uint32_t freqPeriodFormula(std::uint32_t in) {
    return 1000000000 / (24615 * in);
}

static std::uint32_t convertLevelsToOdClamp(float voltageLevel, uint32_t lraPeriod) {
    float odClamp;

    odClamp = voltageLevel /
              ((21.32 / 1000.0) *
               sqrt(1.0 - (static_cast<float>(freqPeriodFormula(lraPeriod)) * 8.0 / 10000.0)));

    return round(odClamp);
}

static float targetGToVlevelsUnderLinearEquation(std::array<float, 4> inputCoeffs, float targetG) {
    // Implement linear equation to get voltage levels, f(x) = ax + b
    // 0 to 3.2 is our valid output
    float outPutVal = 0.0f;
    outPutVal = (targetG - inputCoeffs[1]) / inputCoeffs[0];
    if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
      return outPutVal;
    } else {
      return 0.0f;
    }
}

static float targetGToVlevelsUnderCubicEquation(std::array<float, 4> inputCoeffs, float targetG) {
    // Implement cubic equation to get voltage levels, f(x) = ax^3 + bx^2 + cx + d
    // 0 to 3.2 is our valid output
    float AA = 0.0f, BB = 0.0f, CC = 0.0f, Delta = 0.0f;
    float Y1 = 0.0f, Y2 = 0.0f, K = 0.0f, T = 0.0f, sita = 0.0f;
    float outPutVal = 0.0f;
    float oneHalf = 1.0 / 2.0, oneThird = 1.0 / 3.0;
    float cosSita = 0.0f, sinSitaSqrt3 = 0.0f, sqrtA = 0.0f;

    AA = inputCoeffs[1] * inputCoeffs[1] - 3.0 * inputCoeffs[0] * inputCoeffs[2];
    BB = inputCoeffs[1] * inputCoeffs[2] - 9.0 * inputCoeffs[0] * (inputCoeffs[3] - targetG);
    CC = inputCoeffs[2] * inputCoeffs[2] - 3.0 * inputCoeffs[1] * (inputCoeffs[3] - targetG);

    Delta = BB * BB - 4.0 * AA * CC;

    // There are four discriminants in Shengjin formula.
    // https://zh.wikipedia.org/wiki/%E4%B8%89%E6%AC%A1%E6%96%B9%E7%A8%8B#%E7%9B%9B%E9%87%91%E5%85%AC%E5%BC%8F%E6%B3%95
    if ((fabs(AA) <= FLOAT_EPS) && (fabs(BB) <= FLOAT_EPS)) {
        // Case 1: A = B = 0
        outPutVal = -inputCoeffs[1] / (3 * inputCoeffs[0]);
        if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
          return outPutVal;
        }
        return 0.0f;
    } else if (Delta > FLOAT_EPS) {
        // Case 2: Delta > 0
        Y1 = AA * inputCoeffs[1] + 3.0 * inputCoeffs[0] * (-BB + pow(Delta, oneHalf)) / 2.0;
        Y2 = AA * inputCoeffs[1] + 3.0 * inputCoeffs[0] * (-BB - pow(Delta, oneHalf)) / 2.0;

        if ((Y1 < -FLOAT_EPS) && (Y2 > FLOAT_EPS)) {
            return (-inputCoeffs[1] + pow(-Y1, oneThird) - pow(Y2, oneThird)) /
                   (3.0 * inputCoeffs[0]);
        } else if ((Y1 > FLOAT_EPS) && (Y2 < -FLOAT_EPS)) {
            return (-inputCoeffs[1] - pow(Y1, oneThird) + pow(-Y2, oneThird)) /
                   (3.0 * inputCoeffs[0]);
        } else if ((Y1 < -FLOAT_EPS) && (Y2 < -FLOAT_EPS)) {
            return (-inputCoeffs[1] + pow(-Y1, oneThird) + pow(-Y2, oneThird)) /
                   (3.0 * inputCoeffs[0]);
        } else {
            return (-inputCoeffs[1] - pow(Y1, oneThird) - pow(Y2, oneThird)) /
                   (3.0 * inputCoeffs[0]);
        }
        return 0.0f;
    } else if (Delta < -FLOAT_EPS) {
        // Case 3: Delta < 0
        T = (2 * AA * inputCoeffs[1] - 3 * inputCoeffs[0] * BB) / (2 * AA * sqrt(AA));
        sita = acos(T);
        cosSita = cos(sita / 3);
        sinSitaSqrt3 = sqrt(3.0) * sin(sita / 3);
        sqrtA = sqrt(AA);

        outPutVal = (-inputCoeffs[1] - 2 * sqrtA * cosSita) / (3 * inputCoeffs[0]);
        if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
          return outPutVal;
        }
        outPutVal = (-inputCoeffs[1] + sqrtA * (cosSita + sinSitaSqrt3)) / (3 * inputCoeffs[0]);
        if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
          return outPutVal;
        }
        outPutVal = (-inputCoeffs[1] + sqrtA * (cosSita - sinSitaSqrt3)) / (3 * inputCoeffs[0]);
        if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
          return outPutVal;
        }
        return 0.0f;
    } else if (Delta <= FLOAT_EPS) {
        // Case 4: Delta = 0
        K = BB / AA;
        outPutVal = (-inputCoeffs[1] / inputCoeffs[0] + K);
        if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
          return outPutVal;
        }
        outPutVal = (-K / 2);
        if ((outPutVal > FLOAT_EPS) && (outPutVal <= MAX_VOLTAGE)) {
          return outPutVal;
        }
        return 0.0f;
    } else {
        // Exception handling
        return 0.0f;
    }
}

static float vLevelsToTargetGUnderCubicEquation(
    std::array<float, 4> inputCoeffs, float vLevel) {
  float inputVoltage = 0.0f;
  inputVoltage = vLevel * MAX_VOLTAGE;
  return inputCoeffs[0] * pow(inputVoltage, 3) +
         inputCoeffs[1] * pow(inputVoltage, 2) + inputCoeffs[2] * inputVoltage +
         inputCoeffs[3];
}

static bool motionAwareness() {
    float avgX = 0.0, avgY = 0.0;
    uint64_t current_time = 0;
    clock_gettime(CLOCK_MONOTONIC, &sGetTime);
    current_time = ((uint64_t)sGetTime.tv_sec * 1000 * 1000 * 1000) + sGetTime.tv_nsec;

    if ((current_time - sEndTime) > SENSING_PERIOD) {
        sXAxleData.clear();
        sYAxleData.clear();
        PollGSensor();
        clock_gettime(CLOCK_MONOTONIC, &sGetTime);
        sEndTime = ((uint64_t)sGetTime.tv_sec * 1000 * 1000 * 1000) + sGetTime.tv_nsec;
    }

    avgX = std::accumulate(sXAxleData.begin(), sXAxleData.end(), 0.0) / sXAxleData.size();
    avgY = std::accumulate(sYAxleData.begin(), sYAxleData.end(), 0.0) / sYAxleData.size();

    if ((avgX > -1.3) && (avgX < 1.3) && (avgY > -0.8) && (avgY < 0.8)) {
        return false;
    } else {
        return true;
    }
}

using utils::toUnderlying;

Vibrator::Vibrator(std::unique_ptr<HwApi> hwapi, std::unique_ptr<HwCal> hwcal)
    : mHwApi(std::move(hwapi)), mHwCal(std::move(hwcal)) {
    std::string autocal;
    uint32_t lraPeriod = 0, lpTrigSupport = 0;
    std::array<float, 4> effectCoeffs = {0.0f};
    std::array<float, 4> steadyCoeffs = {0.0f};

    if (!mHwApi->setState(true)) {
        ALOGE("Failed to set state (%d): %s", errno, strerror(errno));
    }

    if (mHwCal->getAutocal(&autocal)) {
        mHwApi->setAutocal(autocal);
    }
    mHwCal->getLraPeriod(&lraPeriod);

    mHwCal->getCloseLoopThreshold(&mCloseLoopThreshold);
    mHwCal->getDynamicConfig(&mDynamicConfig);

    if (mDynamicConfig) {
        uint8_t i = 0;
        bool hasEffectCoeffs = false, hasSteadyCoeffs = false,
             hasExternalEffectG = false, hasExternalSteadyG = false;
        std::array<float, 5> externalEffectTargetG = {0.0f};
        std::array<float, 3> externalSteadyTargetG = {0.0f};
        float tempVolLevel = 0.0f, tempAmpMax = 0.0f;
        uint32_t longFreqencyShift = 0, shortVoltageMax = 0, longVoltageMax = 0,
                 shape = 0;
        std::string devHwVersion;

        mHwCal->getLongFrequencyShift(&longFreqencyShift);
        mHwCal->getShortVoltageMax(&shortVoltageMax);
        mHwCal->getLongVoltageMax(&longVoltageMax);

        // TODO: This is a workaround for b/157610908
        mHwCal->getDevHwVer(&devHwVersion);
        if ((devHwVersion.find("EVT") != std::string::npos) ||
            (devHwVersion.find("PROTO") != std::string::npos)) {
          EFFECT_TARGET_G = {0.15, 0.27, 0.35, 0.54, 0.65};
          STEADY_TARGET_G = {1.2, 1.145, 0.4};
          ALOGW("Device HW version: %s, this is an EVT device",
                devHwVersion.c_str());
        } else {
          ALOGW("Device HW version: %s, no need to change the target G values",
                devHwVersion.c_str());
        }

        hasEffectCoeffs = mHwCal->getEffectCoeffs(&effectCoeffs);
        hasExternalEffectG = mHwCal->getEffectTargetG(&externalEffectTargetG);
        for (i = 0; i < 5; i++) {
            if (hasEffectCoeffs) {
              if (hasExternalEffectG) {
                EFFECT_TARGET_G[i] = externalEffectTargetG[i];
              }
              // Use linear approach to get the target voltage levels
              if ((effectCoeffs[2] == 0) && (effectCoeffs[3] == 0)) {
                tempVolLevel = targetGToVlevelsUnderLinearEquation(
                    effectCoeffs, EFFECT_TARGET_G[i]);
                mEffectTargetOdClamp[i] =
                    convertLevelsToOdClamp(tempVolLevel, lraPeriod);
              } else {
                // Use cubic approach to get the target voltage levels
                tempVolLevel = targetGToVlevelsUnderCubicEquation(
                    effectCoeffs, EFFECT_TARGET_G[i]);
                mEffectTargetOdClamp[i] =
                    convertLevelsToOdClamp(tempVolLevel, lraPeriod);
              }
            } else {
                mEffectTargetOdClamp[i] = shortVoltageMax;
            }
        }
        // Add a boundary protection for level 5 only, since
        // some devices might not be able to reach the maximum target G
        if ((mEffectTargetOdClamp[4] <= 0) || (mEffectTargetOdClamp[4] > shortVoltageMax)) {
            mEffectTargetOdClamp[4] = shortVoltageMax;
        }

        mHwCal->getEffectShape(&shape);
        mEffectConfig.reset(new VibrationConfig({
            .shape = (shape == UINT32_MAX) ? WaveShape::SINE : static_cast<WaveShape>(shape),
            .odClamp = &mEffectTargetOdClamp[0],
            .olLraPeriod = lraPeriod,
        }));

        hasSteadyCoeffs = mHwCal->getSteadyCoeffs(&steadyCoeffs);
        hasExternalSteadyG = mHwCal->getSteadyTargetG(&externalSteadyTargetG);
        if (hasSteadyCoeffs) {
            for (i = 0; i < 3; i++) {
              if (hasExternalSteadyG) {
                STEADY_TARGET_G[i] = externalSteadyTargetG[i];
              }
              // Use cubic approach to get the steady target voltage levels
              // For steady level 3 voltage which is used for non-motion
              // voltage, we use interpolation method to calculate the voltage
              // via 20% of MAX voltage, 60% of MAX voltage and steady level 3
              // target G
              if (i == 2) {
                tempVolLevel =
                    ((STEADY_TARGET_G[2] -
                      vLevelsToTargetGUnderCubicEquation(steadyCoeffs, 0.2)) *
                     0.4 * MAX_VOLTAGE) /
                        (vLevelsToTargetGUnderCubicEquation(steadyCoeffs, 0.6) -
                         vLevelsToTargetGUnderCubicEquation(steadyCoeffs,
                                                            0.2)) +
                    0.2 * MAX_VOLTAGE;
              } else {
                tempVolLevel = targetGToVlevelsUnderCubicEquation(
                    steadyCoeffs, STEADY_TARGET_G[i]);
              }
              mSteadyTargetOdClamp[i] =
                  convertLevelsToOdClamp(tempVolLevel, lraPeriod);
              if ((mSteadyTargetOdClamp[i] <= 0) ||
                  (mSteadyTargetOdClamp[i] > longVoltageMax)) {
                mSteadyTargetOdClamp[i] = longVoltageMax;
              }
            }
        } else {
          if (hasExternalSteadyG) {
            STEADY_TARGET_G[0] = externalSteadyTargetG[0];
            STEADY_TARGET_G[2] = externalSteadyTargetG[2];
          }
          mSteadyTargetOdClamp[0] =
              mHwCal->getSteadyAmpMax(&tempAmpMax)
                  ? round((STEADY_TARGET_G[0] / tempAmpMax) * longVoltageMax)
                  : longVoltageMax;
            mSteadyTargetOdClamp[2] =
                mHwCal->getSteadyAmpMax(&tempAmpMax)
                    ? round((STEADY_TARGET_G[2] / tempAmpMax) * longVoltageMax)
                    : longVoltageMax;
        }
        mHwCal->getSteadyShape(&shape);
        mSteadyConfig.reset(new VibrationConfig({
            .shape = (shape == UINT32_MAX) ? WaveShape::SQUARE : static_cast<WaveShape>(shape),
            .odClamp = &mSteadyTargetOdClamp[0],
            .olLraPeriod = lraPeriod,
        }));
        mSteadyOlLraPeriod = lraPeriod;
        // 1. Change long lra period to frequency
        // 2. Get frequency': subtract the frequency shift from the frequency
        // 3. Get final long lra period after put the frequency' to formula
        mSteadyOlLraPeriodShift =
            freqPeriodFormula(freqPeriodFormula(lraPeriod) - longFreqencyShift);
    } else {
        mHwApi->setOlLraPeriod(lraPeriod);
    }

    mHwCal->getClickDuration(&mClickDuration);
    mHwCal->getTickDuration(&mTickDuration);
    mHwCal->getDoubleClickDuration(&mDoubleClickDuration);
    mHwCal->getHeavyClickDuration(&mHeavyClickDuration);

    // This enables effect #1 from the waveform library to be triggered by SLPI
    // while the AP is in suspend mode
    // For default setting, we will enable this feature if that project did not
    // set the lptrigger config
    mHwCal->getTriggerEffectSupport(&lpTrigSupport);
    if (!mHwApi->setLpTriggerEffect(lpTrigSupport)) {
        ALOGW("Failed to set LP trigger mode (%d): %s", errno, strerror(errno));
    }
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t *_aidl_return) {
    ATRACE_NAME("Vibrator::getCapabilities");
    int32_t ret = 0;
    if (mHwApi->hasRtpInput()) {
        ret |= IVibrator::CAP_AMPLITUDE_CONTROL;
    }
    ret |= IVibrator::CAP_GET_RESONANT_FREQUENCY;
    *_aidl_return = ret;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(uint32_t timeoutMs, const char mode[],
                                const std::unique_ptr<VibrationConfig> &config,
                                const int8_t volOffset) {
    LoopControl loopMode = LoopControl::OPEN;

    // Open-loop mode is used for short click for over-drive
    // Close-loop mode is used for long notification for stability
    if (mode == RTP_MODE && timeoutMs > mCloseLoopThreshold) {
        loopMode = LoopControl::CLOSE;
    }

    mHwApi->setCtrlLoop(toUnderlying(loopMode));
    if (!mHwApi->setDuration(timeoutMs)) {
        ALOGE("Failed to set duration (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    mHwApi->setMode(mode);
    if (config != nullptr) {
        mHwApi->setLraWaveShape(toUnderlying(config->shape));
        mHwApi->setOdClamp(config->odClamp[volOffset]);
        mHwApi->setOlLraPeriod(config->olLraPeriod);
    }

    if (!mHwApi->setActivate(1)) {
        ALOGE("Failed to activate (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback> &callback) {
    ATRACE_NAME("Vibrator::on");

    if (callback) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    if (mDynamicConfig) {
        int temperature = 0;
        mHwApi->getPATemp(&temperature);
        if (temperature > TEMP_UPPER_BOUND) {
            mSteadyConfig->odClamp = &mSteadyTargetOdClamp[0];
            mSteadyConfig->olLraPeriod = mSteadyOlLraPeriod;
            // TODO: b/162346934 This a compromise way to bypass the motion
            // awareness delay
            if ((timeoutMs > VIBRATION_MOTION_TIME_THRESHOLD) && (!motionAwareness())) {
                return on(timeoutMs, RTP_MODE, mSteadyConfig, 2);
            }
        } else if (temperature < TEMP_LOWER_BOUND) {
            mSteadyConfig->odClamp = &STEADY_VOLTAGE_LOWER_BOUND;
            mSteadyConfig->olLraPeriod = mSteadyOlLraPeriodShift;
        }
    }

    return on(timeoutMs, RTP_MODE, mSteadyConfig, 0);
}

ndk::ScopedAStatus Vibrator::off() {
    ATRACE_NAME("Vibrator::off");
    if (!mHwApi->setActivate(0)) {
        ALOGE("Failed to turn vibrator off (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    ATRACE_NAME("Vibrator::setAmplitude");
    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int32_t rtp_input = std::round(amplitude * (MAX_RTP_INPUT - MIN_RTP_INPUT) + MIN_RTP_INPUT);

    if (!mHwApi->setRtpInput(rtp_input)) {
        ALOGE("Failed to set amplitude (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    ATRACE_NAME("Vibrator::setExternalControl");
    ALOGE("Not support in DRV2624 solution, %d", enabled);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

binder_status_t Vibrator::dump(int fd, const char **args, uint32_t numArgs) {
    if (fd < 0) {
        ALOGE("Called debug() with invalid fd.");
        return STATUS_OK;
    }

    (void)args;
    (void)numArgs;

    dprintf(fd, "AIDL:\n");

    dprintf(fd, "  Close Loop Thresh: %" PRIu32 "\n", mCloseLoopThreshold);
    if (mSteadyConfig) {
        dprintf(fd, "  Steady Shape: %" PRIu32 "\n", mSteadyConfig->shape);
        dprintf(fd, "  Steady OD Clamp: %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                mSteadyConfig->odClamp[0], mSteadyConfig->odClamp[1], mSteadyConfig->odClamp[2]);
        dprintf(fd, "  Steady target G: %f %f %f\n", STEADY_TARGET_G[0],
                STEADY_TARGET_G[1], STEADY_TARGET_G[2]);
        dprintf(fd, "  Steady OL LRA Period: %" PRIu32 "\n", mSteadyConfig->olLraPeriod);
    }
    if (mEffectConfig) {
        dprintf(fd, "  Effect Shape: %" PRIu32 "\n", mEffectConfig->shape);
        dprintf(fd,
                "  Effect OD Clamp: %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                mEffectConfig->odClamp[0], mEffectConfig->odClamp[1], mEffectConfig->odClamp[2],
                mEffectConfig->odClamp[3], mEffectConfig->odClamp[4]);
        dprintf(fd, "  Effect target G: %f %f %f %f %f\n", EFFECT_TARGET_G[0],
                EFFECT_TARGET_G[1], EFFECT_TARGET_G[2], EFFECT_TARGET_G[3],
                EFFECT_TARGET_G[4]);
        dprintf(fd, "  Effect OL LRA Period: %" PRIu32 "\n", mEffectConfig->olLraPeriod);
    }
    dprintf(fd, "  Click Duration: %" PRIu32 "\n", mClickDuration);
    dprintf(fd, "  Tick Duration: %" PRIu32 "\n", mTickDuration);
    dprintf(fd, "  Double Click Duration: %" PRIu32 "\n", mDoubleClickDuration);
    dprintf(fd, "  Heavy Click Duration: %" PRIu32 "\n", mHeavyClickDuration);

    dprintf(fd, "\n");

    mHwApi->debug(fd);

    dprintf(fd, "\n");

    mHwCal->debug(fd);

    fsync(fd);
    return STATUS_OK;
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect> *_aidl_return) {
    *_aidl_return = {Effect::TEXTURE_TICK, Effect::TICK, Effect::CLICK, Effect::HEAVY_CLICK,
                     Effect::DOUBLE_CLICK};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback> &callback,
                                     int32_t *_aidl_return) {
    ATRACE_NAME("Vibrator::perform");
    ndk::ScopedAStatus status;

    if (callback) {
        status = ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else {
        status = performEffect(effect, strength, _aidl_return);
    }

    return status;
}

ndk::ScopedAStatus Vibrator::performEffect(Effect effect, EffectStrength strength,
                                           int32_t *outTimeMs) {
    ndk::ScopedAStatus status;
    uint32_t timeMS;
    int8_t volOffset;

    switch (strength) {
        case EffectStrength::LIGHT:
            volOffset = 0;
            break;
        case EffectStrength::MEDIUM:
            volOffset = 1;
            break;
        case EffectStrength::STRONG:
            volOffset = 1;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
            break;
    }

    switch (effect) {
        case Effect::TEXTURE_TICK:
            mHwApi->setSequencer(WAVEFORM_TICK_EFFECT_SEQ);
            timeMS = mTickDuration;
            volOffset = TEXTURE_TICK;
            break;
        case Effect::CLICK:
            mHwApi->setSequencer(WAVEFORM_CLICK_EFFECT_SEQ);
            timeMS = mClickDuration;
            volOffset += CLICK;
            break;
        case Effect::DOUBLE_CLICK:
            mHwApi->setSequencer(WAVEFORM_DOUBLE_CLICK_EFFECT_SEQ);
            timeMS = mDoubleClickDuration;
            volOffset += CLICK;
            break;
        case Effect::TICK:
            mHwApi->setSequencer(WAVEFORM_TICK_EFFECT_SEQ);
            timeMS = mTickDuration;
            volOffset += TICK;
            break;
        case Effect::HEAVY_CLICK:
            mHwApi->setSequencer(WAVEFORM_HEAVY_CLICK_EFFECT_SEQ);
            timeMS = mHeavyClickDuration;
            volOffset += HEAVY_CLICK;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    status = on(timeMS, WAVEFORM_MODE, mEffectConfig, volOffset);
    if (!status.isOk()) {
        return status;
    }

    *outTimeMs = timeMS;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect> * /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t /*id*/, Effect /*effect*/,
                                            EffectStrength /*strength*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t /*id*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t * /*maxDelayMs*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t * /*maxSize*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive> * /*supported*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive /*primitive*/,
                                                  int32_t * /*durationMs*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect> & /*composite*/,
                                     const std::shared_ptr<IVibratorCallback> & /*callback*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

static float freqPeriodFormulaFloat(std::uint32_t in) {
    return static_cast<float>(1000000000) / static_cast<float>(24615 * in);
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float *resonantFreqHz) {
    uint32_t lraPeriod;
    if(!mHwCal->getLraPeriod(&lraPeriod)) {
        ALOGE("Failed to get resonant frequency (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *resonantFreqHz = freqPeriodFormulaFloat(lraPeriod);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getQFactor(float * /*qFactor*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float * /*freqResolutionHz*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float * /*freqMinimumHz*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float> * /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t * /*durationMs*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t * /*maxSize*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking> * /*supported*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle> & /*composite*/,
                                         const std::shared_ptr<IVibratorCallback> & /*callback*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
