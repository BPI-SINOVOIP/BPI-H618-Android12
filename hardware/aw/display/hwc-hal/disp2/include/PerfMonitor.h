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

#ifndef PERF_MONITOR_H
#define PERF_MONITOR_H

#include <string>
#include <utils/SystemClock.h>

using android::base::StringPrintf;

namespace sunxi {

enum {
    kSampleTagCommitBegin = 0,
    kSampleTagWaitAcquireFence,
    kSampleTagHardwareSubmit,
    kSampleTagWaitReleaseFence,
    kSampleTagCommitEnd,
    kSampleTagNumber,
};

class PerfMonitor {
public:
    PerfMonitor();
    void addSample(int type);
    int checkPerformance() const;
    void dump(std::string& out) const;

private:
    typedef int64_t SampleEntry[kSampleTagNumber];
    static const int32_t kNumSamples = 32;
    static const int64_t ELAPSED_TIME_THROTTLE = 64; /* millis */
    int64_t mLastElapsedTime;
    int64_t mWorstElapsedTime;
    SampleEntry mWorstSample;
    SampleEntry mSampleDatas[kNumSamples];
    int mNextSampleSlot;
    int mPrevSampleSlot;
};

} // namespace sunxi
#endif
