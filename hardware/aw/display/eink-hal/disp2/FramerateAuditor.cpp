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

#include "Debug.h"
#include "FramerateAuditor.h"

namespace sunxi {

FramerateAuditor::FramerateAuditor()
  : mFrameTimeoutCount(0),
    mOutputFreezed(false),
    mCurrentRefreshRate(0.0f),
    mLastTimeoutRefreshRate(0.0f),
    mEventHandler(nullptr)
{ }

void FramerateAuditor::onFramePresent()
{
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    computeFrameRate(now);
    mFrameTimestamp.push(now);
    if (mFrameTimestamp.size() > MAX_PRESENT_TIMESTAMP_SAMPLES) {
        mFrameTimestamp.pop();
    }

    if (isOutputFreezed()) {
        float inc = mCurrentRefreshRate - mLastTimeoutRefreshRate;
        if (inc > FREEZED_STATE_THRESHOLD * 0.5) {
            updateFreezedState(false);
        }
    }
    mFrameTimeoutCount = 0;
    DLOGI_IF(kTagFrameRate, "freezed %d, refresh rate %.2f", mOutputFreezed, mCurrentRefreshRate);
}

void FramerateAuditor::computeFrameRate(nsecs_t now, bool timeout)
{
    if (mFrameTimestamp.size() > 0) {
        int frameCount = mFrameTimestamp.size();
        if (timeout)
            frameCount--;
        nsecs_t t0 = mFrameTimestamp.front();
        nsecs_t duration = now - t0;
        mCurrentRefreshRate = ((float)(s2ns(1) * frameCount)) / ((float)duration);
    }
}

void FramerateAuditor::onFrameTimeout()
{
    computeFrameRate(systemTime(SYSTEM_TIME_MONOTONIC), true);

    mFrameTimeoutCount++;
    if (mFrameTimeoutCount >= FREEZED_STATE_THRESHOLD
            && mFrameTimestamp.size() >= MAX_PRESENT_TIMESTAMP_SAMPLES) {
        updateFreezedState(true);
        mLastTimeoutRefreshRate = mCurrentRefreshRate;
    }
    DLOGI_IF(kTagFrameRate, "freezed %d, refresh rate %.2f", mOutputFreezed, mCurrentRefreshRate);
}

int FramerateAuditor::getFrameTimeoutDuration() const
{
    return FRAME_TIMEOUT_DURATION_IN_MS;
}

float FramerateAuditor::getRefreshRate() const
{
    return mCurrentRefreshRate;
}

bool FramerateAuditor::isOutputFreezed() const
{
    return mOutputFreezed;
}

void FramerateAuditor::setOutputFreezedHandler(OutputFreezedHandler* handler)
{
    mEventHandler = handler;
}

void FramerateAuditor::updateFreezedState(bool freezed)
{
    static bool sFreezedState = false;
    mOutputFreezed = freezed;
    if (mOutputFreezed != sFreezedState) {
        sFreezedState = mOutputFreezed;
        if (mEventHandler)
            mEventHandler->onOutputFreezed(sFreezedState);
        DLOGI_IF(kTagFrameRate, "screen output freezed [%d]", sFreezedState);
    }
}

};
