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
    mCurrentFreezedState(false),
    mNeedGPUComposiLastFrame(false),
    mCurrentRefreshRate(0.0f),
    mEventHandler(nullptr)
{ }


bool FramerateAuditor::isNeedGPUComposiLastFrame() const
{
            return mNeedGPUComposiLastFrame;
}

void FramerateAuditor::onFramePresent()
{
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);

    if (isOutputFreezed()) {
        if (mNeedGPUComposiLastFrame) {// already freezed;cur frame is last gpu-composi present
            mNeedGPUComposiLastFrame = false;
        } else {
            updateFreezedState(false);
        }
    }
    mFrameTimeoutCount = 0;
    DLOGI_IF(kTagFrameRate, "freezed %d, refresh rate %.2f", mOutputFreezed,computeFrameRate(now));

    mFrameTimestamp.push(now);
    if (mFrameTimestamp.size() > MAX_PRESENT_TIMESTAMP_SAMPLES) {
        mFrameTimestamp.pop();
    }

}

float FramerateAuditor::computeFrameRate(nsecs_t now, bool timeout)
{
    if (mFrameTimestamp.size() > 0) {
        int frameCount = mFrameTimestamp.size();
        if (timeout)
            frameCount--;
        nsecs_t t0 = mFrameTimestamp.front();
        nsecs_t duration = now - t0;
        mCurrentRefreshRate = ((float)(s2ns(1) * frameCount)) / ((float)duration);
        return mCurrentRefreshRate;
    } else {
        return 0;
    }
}

void FramerateAuditor::onFrameTimeout()
{
    if(!isOutputFreezed()) {
        mFrameTimeoutCount++;
        if (mFrameTimeoutCount >= FREEZED_STATE_THRESHOLD
                && mFrameTimestamp.size() >= MAX_PRESENT_TIMESTAMP_SAMPLES) {
                mNeedGPUComposiLastFrame = true;
                updateFreezedState(true);
            //        mLastTimeoutRefreshRate = mCurrentRefreshRate;
        }
    }
    DLOGI_IF(kTagFrameRate, "freezed %d, refresh rate %.2f", mOutputFreezed,computeFrameRate(systemTime(SYSTEM_TIME_MONOTONIC),true));
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
    mOutputFreezed = freezed;
    if (mOutputFreezed != mCurrentFreezedState) {
        mCurrentFreezedState = mOutputFreezed;
        if (mEventHandler)
            mEventHandler->onOutputFreezed(mCurrentFreezedState);
        DLOGI_IF(kTagFrameRate, "screen output freezed [%d]", mCurrentFreezedState);
    }
}

};
