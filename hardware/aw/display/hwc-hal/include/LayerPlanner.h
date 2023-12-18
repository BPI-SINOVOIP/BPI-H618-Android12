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

#ifndef ANDROI_HWC_LAYER_PLANNER_H_
#define ANDROI_HWC_LAYER_PLANNER_H_

#include <mutex>
#include <memory>
#include <unordered_map>

#include "Display.h"

namespace sunxi {

const int SLOT_PER_CHANNEL = 4;
const int INVALID_CHANNEL = -1;
const int INVALID_SLOT = -1;

enum StrategyType {
    /*
     * All layers are GPU composition,
     * for debugging or other purposes
     */
    eGPUOnly        = 0x01,

    /*
     * Make the layer use hardware composite first,
     * and if the hardware can't support it, take the GPU composite.
     */
    eHardwareFirst  = 0x02,
};

class ScreenTransform;

struct PlanConstraint {
    // Hybrid Channel: support YUV and RGB input format
    //    RGB Channel: only support RGB input format
    int MaxHybridChannel;
    int MaxRGBChannel;

    // Hdr HybridChannel: support hdr10 hlg for video channel
    // Hdr RGBChannel: only support hdr10 hlg for RGB format
    int MaxHdrHybridChannel;
    int MaxHdrRGBChannel;
    bool usingHdr;
    int totalChannelCount;

    // Some old chipsets's DE does not support alpha blending on yuv channnel,
    // We should set HybridChannelWithAlpha to false, So that the strategy
    // can handle this special case.
    bool HybridChannelWithAlpha;

    // Max bandwidth of current frame, in Bytes.
    int MaxBandwidth;

    // Indicate whether it is currently 3D output
    // Only for HDMI device
    bool Output3D;

    // indicate in hdr or sdr mode
    int dataspace;

    // Transform from framebuffer space to screen spac,
    // We need this matrix to calculate the final display window.
    const ScreenTransform* ScreenTransform;

    class IHWLayerDetector {
    public:
        // If the hardware supports this layer, ERROR_NONE will be return,
        // Otherwise it returns the corresponding error code.
        virtual int capable(const std::shared_ptr<Layer>& layer) = 0;
        virtual ~IHWLayerDetector() = default;
    };
    std::shared_ptr<IHWLayerDetector> Detector;
};

struct ChannelConfig {
    // Hardware index of this channel
    size_t Index;

    // Declare if is video channel
    bool VideoChannel;
    float ScaleX, ScaleY;

    struct Slot {
        // Declare if the slot is in use
        bool Enable;
        bool DimLayer;
        size_t Z;
        std::shared_ptr<Layer> InputLayer;
    } Slots[SLOT_PER_CHANNEL];

    // Reset hardware channel as disbale status
    inline ChannelConfig() {
        Index = 0;
        for (size_t i = 0; i < SLOT_PER_CHANNEL; ++i) {
            Slots[i].Enable = false;
            Slots[i].DimLayer = false;
            Slots[i].Z = 0;
            Slots[i].InputLayer.reset();
        }
    }
};

// This structure is used to keep Track of the reason for being assigned
struct TrackNode {
    int FallbackReason;
    const std::shared_ptr<ChannelConfig> Channel;
    int Slot;

    TrackNode(
            int reason,
            const std::shared_ptr<ChannelConfig>& channel = nullptr,
            int slot = INVALID_SLOT)
        : FallbackReason(reason), Channel(channel), Slot(slot) { }
};

struct PlanningOutput {
    std::vector<std::shared_ptr<ChannelConfig>> Configs;
    std::unordered_map<uint64_t, TrackNode> Tracks;
};

class StrategyBase {
public:
    StrategyBase(const char* name)
        : mName(name) { }
    virtual ~StrategyBase() { }

    // constraint: restrictions related to hardware platforms
    // context   : all relevant information of current composition
    // out       : hardware channgel config
    virtual int perform(const PlanConstraint& constraint,
            CompositionContext* context,
            PlanningOutput *output) = 0;
    virtual void release() = 0;

    const char *name() const { return mName.c_str(); }

private:
    std::string mName;
};

class LayerPlanner {
public:
    LayerPlanner();

    void setStrategy(StrategyType type);
    void advanceFrame(const PlanConstraint& constraint,
            CompositionContext* context, PlanningOutput* output);
    void postCommit();

private:
    void debugPrint(CompositionContext* context, PlanningOutput* output) const;

    std::shared_ptr<StrategyBase> mActiveStrategy;
    std::unordered_map<int, std::shared_ptr<StrategyBase>> mStrategyTable;
    std::mutex mMutex;
};

} // namespace sunxi

#endif

