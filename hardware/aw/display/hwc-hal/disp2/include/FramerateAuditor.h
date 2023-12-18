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

#ifndef FRAME_RATE_AUDITOR_H
#define FRAME_RATE_AUDITOR_H

#include <queue>
#include <stddef.h>
#include <utils/Timers.h>

namespace sunxi {

class FramerateAuditor {
public:
    FramerateAuditor();
   ~FramerateAuditor() = default;

    // Called on when frame starting to present on screen.
    void onFramePresent();
    void onFrameTimeout();
    // Frame timeout duration in milliseconds.
    int getFrameTimeoutDuration() const;
    float getRefreshRate() const;
    bool isOutputFreezed() const;

    bool isNeedGPUComposiLastFrame() const;

    class OutputFreezedHandler {
    public:
        virtual ~OutputFreezedHandler() = default;
        virtual void onOutputFreezed(bool freezed) = 0;
    };
    void setOutputFreezedHandler(OutputFreezedHandler* handler);

private:
    enum { MAX_PRESENT_TIMESTAMP_SAMPLES = 8 };
    enum { FRAME_TIMEOUT_DURATION_IN_MS  = 200 };
    enum { FREEZED_STATE_THRESHOLD = 4 };

    float computeFrameRate(nsecs_t now, bool timeout=false);
    void updateFreezedState(bool freezed);

    std::queue<nsecs_t> mFrameTimestamp;
    int mFrameTimeoutCount;
    // When frame timeout count reach the FREEZED_STATE_THRESHOLD,
    // set mOutputFreezed as true.
    bool mOutputFreezed;
    bool mCurrentFreezedState;
    bool mNeedGPUComposiLastFrame; //need gpu-composi save power when commit-timeout to freezed

    float mCurrentRefreshRate;

    OutputFreezedHandler* mEventHandler;
};

} // namespace sunxi

#endif
