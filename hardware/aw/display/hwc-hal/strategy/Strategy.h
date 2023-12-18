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

#ifndef ANDROID_HWC_STRATEGY_H_
#define ANDROID_HWC_STRATEGY_H_

#include "BandwidthMeter.h"
#include "errorcode.h"
#include "LayerPlanner.h"

namespace sunxi {

// All input layer will mark as GPU composition
// For debugging purposes only
class StrategyGPUOnly : public StrategyBase {
public:
    StrategyGPUOnly()
        : StrategyBase("GPUOnly") { }

    int perform(const PlanConstraint& constraint,
            CompositionContext* context, PlanningOutput* out);
    void release();
};

class StrategyHardwareFirst : public StrategyBase {
public:
    StrategyHardwareFirst();
   ~StrategyHardwareFirst();

    int perform(const PlanConstraint& constraint,
            CompositionContext* context, PlanningOutput* out);
    void release();

private:
    // DE2+ Hardware channel abstract
    class HWChannel;
    typedef std::vector<std::unique_ptr<HWChannel>> ChannelList;

    // Helper functions for layer assignment
    enum HWChannelType {
        YUV_CHANNEL,
        RGB_CHANNEL,
        // RGB channel without alpha blending support.
        // There is not alpha support on video channel of some version DE2,
        // We mark those channel as RGB_WITHOUT_ALPHA_CHANNEL when they use for RGB layer.
        RGB_WITHOUT_ALPHA_CHANNEL,
    };

    enum AssignRetCode {
       ASSIGN_SUCCESS,
       ASSIGN_FAILED_NO_CLIENT_TARGET_CHANNEL,
       ASSIGN_FAILED_UNKOWN,
    };

    HWChannelType pickChannelType(const std::shared_ptr<Layer>& layer);
    int initAssignment(const PlanConstraint& constraint, CompositionContext* context);
    AssignRetCode doAssignment(const PlanConstraint& constraint,
            CompositionContext* context);
    void acceptAssignResult(CompositionContext* context, PlanningOutput* out);

    // helper functions
    std::unique_ptr<HWChannel> allocateHWChannel(HWChannelType t);
    int dispatchToClientTarget(std::shared_ptr<Layer> layer, int reason);
    int dispatchToDevice(const PlanConstraint& constraint, std::shared_ptr<Layer> layer);
    inline void mergeChannels(ChannelList& to, ChannelList& from);
    inline std::unique_ptr<HWChannel>& moveOneChannel(ChannelList& to,ChannelList& from);

private:
    // Total count of the unassigned yuv layer
    int mTotalInputYuvLayerCnt;
    int mReservedYUVChannelCnt;

    // Channel reserved for the YUV layer,
    // So that We can guarantee that the YUV layer has priority
    // in hardware composition.
    ChannelList mReservedYUVChannel;

    // Free Channel for RGB input layer
    ChannelList mFreeChannel;
    // Free channel that without alpha blending support
    ChannelList mFreeRgbOverlayChannel;

    // Every channel assigned a layer will be moved to here
    ChannelList mAssignedChannel;

    // Point to the Client target, if needed
    std::unique_ptr<HWChannel> mClientTarget;

    // This variable will set to true when the hardware channel is exhausted
    // and GPU composition is required.
    bool mChannelExhausted;
    bool mNeedClientTarget;

    int mGpuCompositonLayerCnt;

    // keep Track of the reason for being assigned as Client composition
    std::vector<std::pair<std::shared_ptr<Layer>, int>> mClientCompositionLayers;

    BandwidthMeter mBWMeter;
};

std::shared_ptr<StrategyBase> createStrategy(StrategyType type);

} // namespace sunxi

#endif

