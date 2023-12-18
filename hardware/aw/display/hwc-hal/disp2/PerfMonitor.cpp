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

#include <android-base/stringprintf.h>
#include "Debug.h"
#include "PerfMonitor.h"

namespace sunxi {

PerfMonitor::PerfMonitor()
    : mLastElapsedTime(0), mWorstElapsedTime(0),
      mNextSampleSlot(0), mPrevSampleSlot(0)
{
    memset(&mWorstSample, 0, sizeof(SampleEntry));
    memset(&mSampleDatas, 0, sizeof(SampleEntry) * kNumSamples);
}

void PerfMonitor::addSample(int type)
{
    mSampleDatas[mNextSampleSlot][type] = android::uptimeMillis();
    if (type == kSampleTagCommitEnd) {
        SampleEntry& sample = mSampleDatas[mNextSampleSlot];
        mLastElapsedTime = sample[kSampleTagCommitEnd] - sample[kSampleTagCommitBegin];
        if (mLastElapsedTime >= mWorstElapsedTime) {
            mWorstElapsedTime = mLastElapsedTime;
            memcpy(&mWorstSample, &mSampleDatas[mNextSampleSlot], sizeof(SampleEntry));
        }

        DLOGD_IF(kTagPerf, "PerfMonitor total[D]: %4lld acquire: %4lld submit: %4lld release: %4lld",
                sample[kSampleTagCommitEnd] - sample[kSampleTagCommitBegin],
                sample[kSampleTagHardwareSubmit] - sample[kSampleTagWaitAcquireFence],
                sample[kSampleTagWaitReleaseFence] - sample[kSampleTagHardwareSubmit],
                sample[kSampleTagCommitEnd] - sample[kSampleTagWaitReleaseFence]);

        mPrevSampleSlot = mNextSampleSlot++;
        if (mNextSampleSlot >= kNumSamples) mNextSampleSlot = 0;
    }
}

int PerfMonitor::checkPerformance() const
{
    if (mLastElapsedTime > ELAPSED_TIME_THROTTLE) {
        const SampleEntry& sample = mSampleDatas[mPrevSampleSlot];
        DLOGD("PerfMonitor total[w]: %4lld acquire: %4lld submit: %4lld release: %4lld",
                sample[kSampleTagCommitEnd] - sample[kSampleTagCommitBegin],
                sample[kSampleTagHardwareSubmit] - sample[kSampleTagWaitAcquireFence],
                sample[kSampleTagWaitReleaseFence] - sample[kSampleTagHardwareSubmit],
                sample[kSampleTagCommitEnd] - sample[kSampleTagWaitReleaseFence]);
        return -1;
    }
    return 0;
}

void PerfMonitor::dump(std::string& out) const
{
    if(CC_UNLIKELY(Debug::get().enable(kTagPerf))) {
        for (int i = 0; i < kNumSamples; i++) {
            int index = (mNextSampleSlot + i) % kNumSamples;
            const SampleEntry& sample = mSampleDatas[index];
            out += StringPrintf("[%02d] %" PRId64 ", %" PRId64 ", %" PRId64 ", %" PRId64 ", %" PRId64 "\n",
                    i,
                    sample[kSampleTagCommitBegin],
                    sample[kSampleTagWaitAcquireFence],
                    sample[kSampleTagHardwareSubmit],
                    sample[kSampleTagWaitReleaseFence],
                    sample[kSampleTagCommitEnd]);
        }
    }
}

}
