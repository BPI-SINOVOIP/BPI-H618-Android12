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

#ifndef SUNXI_SNR_MANAGER_H
#define SUNXI_SNR_MANAGER_H

#include <map>
#include <vendor/display/config/1.0/IDisplayConfig.h>

namespace sunxi {

class IHWCPrivateService;

using ::vendor::display::config::V1_0::SNRInfo;
using ::vendor::display::config::V1_0::SNRFeatureMode;

class SNRManager {
public:
    SNRManager(IHWCPrivateService& composer);
    bool platformSupportSNR(int display);
    int getSNRInfo(int display, SNRInfo& info);
    int setSNRInfo(int display, const SNRInfo& info);

private:
    IHWCPrivateService& mHWComposer;

    // Level to snr info maps,
    // support SNR_LEVEL1 ~ SNR_LEVEL3 currently
    std::map<uint32_t, SNRInfo> mPreDefineSNRInfo;

    // user define snr info
    SNRInfo mCustomSNRInfo;
};

} // namespace sunxi
#endif
