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

#ifndef SUNXI_SNRINFO_H
#define SUNXI_SNRINFO_H

#include <cinttypes>
#include <memory>
#include <mutex>

struct disp_snr_info;

namespace sunxi {

class SNRApi {
public:
    // snr feature style.
    static const int SNR_STYLE_NORMAL = 0;
    static const int SNR_STYLE_DEMO   = 1;

    struct SNRMetadata {
        int display;
        int enabled;
        int style;
        int strength[3];
    };

    virtual int config(const SNRMetadata& metadata) = 0;
    virtual ~SNRApi() = default;
};

class SNRSetting : public SNRApi {
public:
    int config(const SNRMetadata& metadata) override;
    // convert SNR strength to disp_snr_info for display
    void convertSnrInfoForDisplay(struct disp_snr_info* snr);
    static SNRSetting* getInstance();
    ~SNRSetting();

private:
    SNRSetting();
    static std::mutex sMutex;
    static std::unique_ptr<SNRSetting> sInstance;

    std::mutex mMetadataLock;
    SNRApi::SNRMetadata mConfigMetadata;
};

} // namespace sunxi
#endif
