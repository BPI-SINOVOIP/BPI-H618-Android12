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

#include <utils/Log.h>
#include "snr.h"
#include "hardware/sunxi_display2.h"

namespace sunxi {

std::mutex SNRSetting::sMutex;
std::unique_ptr<SNRSetting> SNRSetting::sInstance {nullptr};

SNRSetting* SNRSetting::getInstance()
{
    std::lock_guard<std::mutex> lock(sMutex);
    if (sInstance == nullptr) {
        sInstance.reset(new SNRSetting());
    }
    return sInstance.get();
}

SNRSetting::SNRSetting()
    : mConfigMetadata() { }

SNRSetting::~SNRSetting() = default;

int SNRSetting::config(const SNRMetadata& metadata)
{
    std::lock_guard<std::mutex> lock(mMetadataLock);
    mConfigMetadata = metadata;
    return 0;
}

void SNRSetting::convertSnrInfoForDisplay(struct disp_snr_info* snr)
{
    if (snr) {
        std::lock_guard<std::mutex> lock(mMetadataLock);
        snr->en = mConfigMetadata.enabled != 0;
        snr->demo_en = mConfigMetadata.style == SNR_STYLE_DEMO;
        snr->y_strength = mConfigMetadata.strength[0];
        snr->u_strength = mConfigMetadata.strength[1];
        snr->v_strength = mConfigMetadata.strength[2];
        ALOGV("convertSnrInfoForDisplay: enable=%d style=%d %d %d %d",
                mConfigMetadata.enabled,
                mConfigMetadata.style,
                mConfigMetadata.strength[0],
                mConfigMetadata.strength[1],
                mConfigMetadata.strength[2]);
    }
}

} // namespace sunxi

