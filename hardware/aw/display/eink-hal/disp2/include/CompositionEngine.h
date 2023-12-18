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
#include "sunxi_eink.h"

struct syncinfo;
struct disp_layer_config2;

namespace sunxi {

class Layer;
class ScreenTransform;
struct ChannelConfig;

typedef enum: uint32_t {
	HWC_EINK_DISABLE					= 0xffffffff,
	HWC_EINK_MASK					= 	0xffff0000,
	HWC_EINK_SURFACEVIEW			=	0x10000,
	HWC_EINK_FORCE_REFRESH			=	0x20000,
	HWC_EINK_HANDWRITTEN			=	0x40000,
	HWC_EINK_RELATIVE_COORDINATES	=	0x80000,
	HWC_EINK_HANDWRITTEN_BG			=	0x100000,
} hwc2_eink_layer_type_t;

/*
 * Eink support refresh mode
 */
typedef enum: uint32_t {
    HWC2_EINK_INIT_MODE     = 0x01,
    HWC2_EINK_DU_MODE       = 0x02,
    HWC2_EINK_GC16_MODE     = 0x04,
    HWC2_EINK_GC4_MODE      = 0x08,
    HWC2_EINK_A2_MODE       = 0x10,
    HWC2_EINK_GL16_MODE     = 0x20,
    HWC2_EINK_GLR16_MODE    = 0x40,
    HWC2_EINK_GLD16_MODE    = 0x80,
    HWC2_EINK_GU16_MODE     = 0x84,
	HWC2_EINK_CLEAR_MODE = 0x88,
	HWC2_EINK_GC4L_MODE = 0x8c,
	HWC2_EINK_GCC16_MODE = 0xa0,
	/* use self upd win not de*/
	HWC2_EINK_RECT_MODE  = 0x400,
} hwc2_eink_refresh_mode_t;

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
    virtual int submitLayer(unsigned int syncnum, disp_layer_config2* configs,
		    int configCount, std::shared_ptr<struct eink_img>& cur_img, bool ForceRefresh, struct upd_win *handwrite_win);
    virtual void reconfigBandwidthLimitation(const DeviceBase::Config& config) = 0;

    virtual ~CompositionEngine() = default;
};

} // namespace sunxi

#endif
