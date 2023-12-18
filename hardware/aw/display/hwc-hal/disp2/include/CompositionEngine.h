/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef _SUNXI_COMPOSITION_ENGINE_H_
#define _SUNXI_COMPOSITION_ENGINE_H_

#include <memory>
#include "Device.h"

struct syncinfo;
struct disp_layer_config2;

namespace sunxi {

class Layer;
class ScreenTransform;
struct ChannelConfig;

class CompositionEngine {
public:
    enum {
        eHybridChannelWithAlpha = 1,
        eAfbcBufferSupported    = 2,
    };

    struct Attribute {
        int maxHybridChannelCount;
        int maxRGBChannelCount;
        // Max bandwidth limitations in byte unit
        int maxBandwidth;
        int featureMask;
        int maxHdrHybridCount;
        int maxHdrRGBCount;
    };

    // Get attribute of this engine.
    virtual const Attribute& getAttribute() const = 0;

    // If this supports this layer, ERROR_NONE will be return,
    // Otherwise returns the corresponding error code.
    virtual int capable(const ScreenTransform& transform, const std::shared_ptr<Layer>& layer) = 0;

    // The PlanningOutput::Configs is sort by zorder,
    // We need to rearrange to match the hardware order.
    virtual void rearrangeChannel(
            std::vector<std::shared_ptr<ChannelConfig>>& configs) const = 0;

    virtual int createSyncpt(syncinfo *info) = 0;
    virtual int submitLayer(unsigned int syncnum, disp_layer_config2* configs, int configCount) = 0;
    virtual void reconfigBandwidthLimitation(const DeviceBase::Config& config) = 0;

    virtual ~CompositionEngine() = default;
};

} // namespace sunxi

#endif
