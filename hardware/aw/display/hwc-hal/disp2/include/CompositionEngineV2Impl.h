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

#ifndef SUNXI_COMPOSITION_ENGINE_H
#define SUNXI_COMPOSITION_ENGINE_H

#include "CompositionEngine.h"
#include "CompositionEngineV2Impl_defs.h"

namespace sunxi {

class CompositionEngineV2Impl : public CompositionEngine {
public:
    static std::shared_ptr<CompositionEngineV2Impl> create(int id);

    virtual ~CompositionEngineV2Impl();
    int hardwareId() const { return mHardwareIndex; }

    // CompositionEngine api
    const Attribute& getAttribute() const override;
    int capable(const ScreenTransform& transform, const std::shared_ptr<Layer>& layer) override;
    void rearrangeChannel(std::vector<std::shared_ptr<ChannelConfig>>& configs) const override;
    int createSyncpt(syncinfo *info) override;
    int submitLayer(unsigned int syncnum, disp_layer_config2* configs, int configCount) override;
    void reconfigBandwidthLimitation(const DeviceBase::Config& config) override;

private:
    explicit CompositionEngineV2Impl(int id);
    int setup();
    int calculateBandwidthLimitation() const;

private:
    const int mHardwareIndex;
    static int mSyncTimelineActive[CONFIG_MAX_DISPLAY_ENGINE];
    Attribute mAttribute;
    int mFrequency;
    bool mAfbcBufferSupported;
    bool mIommuEnabled;
};

} // namespace sunxi

#endif

