/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef _BANDWIDTH_METER_H_
#define _BANDWIDTH_METER_H_

#include <cstddef>
#include <cstdint>

class BandwidthMeter {
public:
    BandwidthMeter()
        : mMaximumBandwidth(BANDWIDTH_NO_LIMIT),
          mCurrentBandwidth(0) { }

    // Set the maximum allowable bandwidth, in bytes;
    // if limit==0, it means no bandwidth limit.
    void setBandwidthlimitation(std::size_t limit) {
        mMaximumBandwidth = limit;
    }

    void add(std::size_t bw) {
        mCurrentBandwidth += bw;
    }

    std::size_t avaliable() const {
        if (mMaximumBandwidth == BANDWIDTH_NO_LIMIT)
            return SIZE_MAX;
        else if (mMaximumBandwidth <= mCurrentBandwidth)
            return 0;
        else
            return mMaximumBandwidth - mCurrentBandwidth;
    }

    void reset() {
        mCurrentBandwidth = 0;
    }

private:
    const static std::size_t BANDWIDTH_NO_LIMIT = 0;
    std::size_t mMaximumBandwidth;
    std::size_t mCurrentBandwidth;
};

#endif
