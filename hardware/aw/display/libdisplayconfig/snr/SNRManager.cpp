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
#include "SNRManager.h"
#include "snr.h"
#include "IHWCPrivateService.h"

namespace sunxi {

const SNRInfo defalutSNRLevels[3] = {
    {SNRFeatureMode::SNR_LEVEL1, 1, 1, 1},
    {SNRFeatureMode::SNR_LEVEL2, 3, 3, 3},
    {SNRFeatureMode::SNR_LEVEL3, 5, 5, 5},
};

SNRManager::SNRManager(IHWCPrivateService& composer)
    : mHWComposer(composer), mCustomSNRInfo()
{
    mPreDefineSNRInfo.emplace(static_cast<uint32_t>(SNRFeatureMode::SNR_LEVEL1), defalutSNRLevels[0]);
    mPreDefineSNRInfo.emplace(static_cast<uint32_t>(SNRFeatureMode::SNR_LEVEL2), defalutSNRLevels[1]);
    mPreDefineSNRInfo.emplace(static_cast<uint32_t>(SNRFeatureMode::SNR_LEVEL3), defalutSNRLevels[2]);
}

bool SNRManager::platformSupportSNR(int /*display*/)
{
    SNRApi* snrapi = mHWComposer.getSNRInterface();
    if (!snrapi) {
        ALOGE("platform not support snr feature");
        return false;
    }
    return true;
}

int SNRManager::getSNRInfo(int /*display*/, SNRInfo& info)
{
    if (mCustomSNRInfo.mode == SNRFeatureMode::SNR_LEVEL1
            || mCustomSNRInfo.mode == SNRFeatureMode::SNR_LEVEL2
            || mCustomSNRInfo.mode == SNRFeatureMode::SNR_LEVEL3) {
        uint32_t index = static_cast<uint32_t>(mCustomSNRInfo.mode);
        if (mPreDefineSNRInfo.count(index) == 0) {
            ALOGE("no pre define snr feature mode: %s",  toString(mCustomSNRInfo.mode).c_str());
            return -1;
        }
        info = mPreDefineSNRInfo[index];
    } else if (mCustomSNRInfo.mode == SNRFeatureMode::SNR_CUSTOM) {
        info = mCustomSNRInfo;
    } else if (mCustomSNRInfo.mode == SNRFeatureMode::SNR_DISABLE) {
        info.mode = SNRFeatureMode::SNR_DISABLE;
        info.y = 0;
        info.u = 0;
        info.v = 0;
    } else {
        ALOGE("unknow snr feature mode: %s",  toString(mCustomSNRInfo.mode).c_str());
        return -1;
    }
    ALOGV("getSNRInfo: %s %d %d %d", toString(info.mode).c_str(), info.y, info.u, info.v);
    return 0;
}

int SNRManager::setSNRInfo(int display, const SNRInfo& info)
{
    SNRApi* snrapi = mHWComposer.getSNRInterface();
    if (!snrapi) {
        ALOGE("no SNRApi found");
        return -1;
    }

    if (info.mode == SNRFeatureMode::SNR_LEVEL1
            || info.mode == SNRFeatureMode::SNR_LEVEL2
            || info.mode == SNRFeatureMode::SNR_LEVEL3) {
        uint32_t index = static_cast<uint32_t>(info.mode);
        if (mPreDefineSNRInfo.count(index) == 0) {
            ALOGE("no pre define snr feature mode: %s",  toString(info.mode).c_str());
            return -1;
        }
        mCustomSNRInfo = mPreDefineSNRInfo[index];
    } else {
        mCustomSNRInfo = info;
    }

    ALOGV("setSNRInfo: %s %d %d %d",
            toString(mCustomSNRInfo.mode).c_str(), mCustomSNRInfo.y, mCustomSNRInfo.u, mCustomSNRInfo.v);

    SNRApi::SNRMetadata metadata;
    metadata.display = display;
    metadata.enabled = mCustomSNRInfo.mode != SNRFeatureMode::SNR_DISABLE;
    metadata.style = (mCustomSNRInfo.mode == SNRFeatureMode::SNR_DEMO)
        ? SNRApi::SNR_STYLE_DEMO : SNRApi::SNR_STYLE_NORMAL;
    metadata.strength[0] = mCustomSNRInfo.y;
    metadata.strength[1] = mCustomSNRInfo.u;
    metadata.strength[2] = mCustomSNRInfo.v;
    return snrapi->config(metadata);
}

} // namespace sunxi
