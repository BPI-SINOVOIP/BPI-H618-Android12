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

#include <chrono>
#include "gnss_measurement.h"

namespace goldfish {
using ::android::hardware::hidl_vec;
using GnssClockFlags10 = ahg10::IGnssMeasurementCallback::GnssClockFlags;
using GnssMeasurementT20 = ahg20::IGnssMeasurementCallback::GnssMeasurement;

namespace {
void initGnssData(GnssData20& data,
                  int64_t elapsedRealtimeNs,
                  int64_t timeNs,
                  int64_t fullBiasNs,
                  double biasUncertaintyNs,
                  size_t nMeasurements) {
    data.elapsedRealtime.flags = ahg20::ElapsedRealtimeFlags::HAS_TIMESTAMP_NS | 0;
    data.elapsedRealtime.timestampNs = static_cast<uint64_t>(elapsedRealtimeNs);
    data.clock.gnssClockFlags = GnssClockFlags10::HAS_FULL_BIAS | 0;
    data.clock.timeNs = timeNs;
    data.clock.fullBiasNs = fullBiasNs;
    data.clock.biasUncertaintyNs = biasUncertaintyNs;
    data.measurements.resize(nMeasurements);
}

GnssMeasurementT20 makeGnssMeasurementT20(int svid,
                                          int state,
                                          int64_t receivedSvTimeInNs,
                                          int64_t receivedSvTimeUncertaintyInNs,
                                          double cN0DbHz,
                                          double pseudorangeRateMps,
                                          double pseudorangeRateUncertaintyMps,
                                          int accumulatedDeltaRangeState,
                                          double accumulatedDeltaRangeM,
                                          double accumulatedDeltaRangeUncertaintyM,
                                          int multipathIndicator,
                                          int constellation) {
    using GnssMeasurementFlags10 = ahg10::IGnssMeasurementCallback::GnssMeasurementFlags;
    using GnssMultipathIndicator10 = ahg10::IGnssMeasurementCallback::GnssMultipathIndicator;
    using GnssAccumulatedDeltaRangeState11 = ahg11::IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState;
    using GnssMeasurementState20 = ahg20::IGnssMeasurementCallback::GnssMeasurementState;

    GnssMeasurementT20 m20;
    auto& m11 = m20.v1_1;
    auto& m10 = m11.v1_0;

    m10.flags = GnssMeasurementFlags10::HAS_CARRIER_FREQUENCY | 0;
    m10.svid = svid;
    m10.receivedSvTimeInNs = receivedSvTimeInNs;
    m10.receivedSvTimeUncertaintyInNs = receivedSvTimeUncertaintyInNs;
    m10.cN0DbHz = cN0DbHz;
    m10.pseudorangeRateMps = pseudorangeRateMps;
    m10.pseudorangeRateUncertaintyMps = pseudorangeRateUncertaintyMps;
    m10.accumulatedDeltaRangeM = accumulatedDeltaRangeM;
    m10.accumulatedDeltaRangeUncertaintyM = accumulatedDeltaRangeUncertaintyM;
    m10.carrierFrequencyHz = 1.59975e+09;
    m10.multipathIndicator = static_cast<GnssMultipathIndicator10>(multipathIndicator);
    m11.accumulatedDeltaRangeState =
        GnssAccumulatedDeltaRangeState11::ADR_STATE_UNKNOWN | accumulatedDeltaRangeState;
    m20.codeType = "UNKNOWN";
    m20.state = GnssMeasurementState20::STATE_UNKNOWN | state;
    m20.constellation = static_cast<ahg20::GnssConstellationType>(constellation);

    return m20;
}
}  // namespace

GnssMeasurement20::GnssMeasurement20() {
    m_gnssData.resize(1);

    initGnssData(m_gnssData[0], 139287, 116834000000, -1189181444165780000, 5.26068202130163, 7);
    m_gnssData[0].measurements[0] = makeGnssMeasurementT20(22,  47, 3927349114,      29, 29.9917297363281,  245.509362821673,  0.148940800975766, 1,  6620.74237064615,  0.00271145859733223, 0, 1);
    m_gnssData[0].measurements[1] = makeGnssMeasurementT20(23,  47, 3920005435,      14, 36.063377380371,  -731.947951627658, 0.0769754027959242, 1,  -23229.096048105,  0.00142954161856323, 0, 1);
    m_gnssData[0].measurements[2] = makeGnssMeasurementT20(25,  47, 3923720994,      56, 24.5171585083007, -329.789995021822,  0.277918601850871, 1, -15511.1976492851,  0.00509250536561012, 0, 1);
    m_gnssData[0].measurements[3] = makeGnssMeasurementT20(31,  47, 3925772934,      11, 37.9193840026855,  -380.23772244582, 0.0602980729893803, 1, -11325.9094456612,  0.00115450704470276, 0, 1);
    m_gnssData[0].measurements[4] = makeGnssMeasurementT20(32,  47, 3919018415,      21, 32.8980560302734,  581.800347848025,  0.109060249597082, 1,  15707.8963147985,  0.00205808319151401, 0, 1);
    m_gnssData[0].measurements[5] = makeGnssMeasurementT20(10, 227, 69142929947304, 127, 23.432445526123,    259.17838762857,   0.31591691295607, 4,  8152.78081298147, 3.40282346638528E+38, 0, 3);
    m_gnssData[0].measurements[6] = makeGnssMeasurementT20(2,  227, 69142935176327,  41, 33.180908203125,  -53.8773853795901,  0.104984458760586, 1, -1708.08166640048,  0.00196184404194355, 0, 3);
}

GnssMeasurement20::~GnssMeasurement20() {
    if (m_isRunning) {
        stopLocked();
    }
}

Return<GnssMeasurementStatus10>
GnssMeasurement20::setCallback_2_0(const sp<ahg20::IGnssMeasurementCallback>& callback,
                                   bool enableFullTracking) {
    (void)enableFullTracking;
    if (callback == nullptr) {
        return GnssMeasurementStatus10::ERROR_GENERIC;
    }

    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_isRunning) {
        stopLocked();
    }

    m_callback = callback;
    startLocked();

    return GnssMeasurementStatus10::SUCCESS;
}

Return<void> GnssMeasurement20::close() {
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_isRunning) {
        stopLocked();
    }

    m_callback = nullptr;
    return {};
}

void GnssMeasurement20::startLocked() {
    m_thread = std::thread([this](){
        while (m_isRunning) {
            update();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    m_isRunning = true;
}

void GnssMeasurement20::stopLocked() {
    m_isRunning = false;
    m_thread.join();
}

void GnssMeasurement20::update() {
    std::unique_lock<std::mutex> lock(m_mtx);

    m_callback->gnssMeasurementCb_2_0(m_gnssData[m_gnssDataIndex]);
    m_gnssDataIndex = (m_gnssDataIndex + 1) % m_gnssData.size();
}

/// old and deprecated /////////////////////////////////////////////////////////
Return<GnssMeasurementStatus10> GnssMeasurement20::setCallback_1_1(const sp<ahg11::IGnssMeasurementCallback>&, bool) {
    return GnssMeasurementStatus10::ERROR_GENERIC;
}

Return<GnssMeasurementStatus10> GnssMeasurement20::setCallback(const sp<ahg10::IGnssMeasurementCallback>&) {
    return GnssMeasurementStatus10::ERROR_GENERIC;
}

}  // namespace goldfish
