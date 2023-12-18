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

#include "Debug.h"
#include "LayerPlanner.h"
#include "strategy/Strategy.h"
#include "utils.h"

namespace sunxi {

LayerPlanner::LayerPlanner()
    : mActiveStrategy(nullptr),
      mStrategyTable()
{
    mStrategyTable.emplace(eGPUOnly, createStrategy(eGPUOnly));
    mStrategyTable.emplace(eHardwareFirst, createStrategy(eHardwareFirst));
}

void LayerPlanner::setStrategy(StrategyType type)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mStrategyTable.count(type) == 0) {
        DLOGE("LayerPlanner: unknown strategy type (%d)", type);
        return;
    }
    if (mActiveStrategy != nullptr)
        mActiveStrategy->release();
    mActiveStrategy = mStrategyTable.at(type);
    DLOGI_IF(kTagStrategy, "LayerPlanner: switch to composition strategy: %s", mActiveStrategy->name());
}

void LayerPlanner::advanceFrame(const PlanConstraint& constraint,
        CompositionContext* context, PlanningOutput* output)
{
    DTRACE_FUNC();
    std::unique_lock<std::mutex> lock(mMutex);

    int error = mActiveStrategy->perform(constraint, context, output);
    if (error) {
        std::shared_ptr<StrategyBase> gpustrategy = mStrategyTable.at(eGPUOnly);
        gpustrategy->perform(constraint, context, output);
        DLOGI("LayerPlanner: %s advanceFrame error (%d), fallback into GPU only strategy",
                mActiveStrategy->name(), error);
    }

    debugPrint(context, output);
}

void LayerPlanner::postCommit()
{
    mActiveStrategy->release();
}

static inline void printLayer(const std::shared_ptr<Layer>& layer, PlanningOutput* output)
{
    if (layer == nullptr)
        return;

    if (output->Tracks.count(layer->id()) == 0) {
        DLOGE("Layer %d is not exist in track list", layer->id());
        return;
    }

    const TrackNode& track = output->Tracks.at(layer->id());
    const std::shared_ptr<ChannelConfig>& channel = track.Channel;
    const ChannelConfig::Slot* slot = (channel != nullptr && track.Slot != INVALID_SLOT)
                                      ? &(channel->Slots[track.Slot]) : nullptr;
    hwc_rect_t frame = layer->displayFrame();
    hwc_rect_t crop = layer->sourceCrop();
    hwc_color_t color = layer->solidColor();
    DLOGD("%11s|  %3d  | %2d | %2d |%4d|%5.3f|%13s|%9s|0x%08x|%11s|%18s|%5d,%5d,%5d,%5d|%5d,%5d,%5d,%5d|%4.2f %4.2f| %s",
            to_string(layer->compositionFromValidated()).c_str(),
            channel ? channel->Index : -1,
            track.Slot,
            slot ? slot->Z : -1,
            layer->zOrder(),
            layer->alpha(),
            to_string(layer->blending()).c_str(),
            to_string(layer->transform()).c_str(),
            layer->dataspace(),
            getHalPixelFormatString(getLayerPixelFormat(layer)),
            isSoildLayer(layer) ? toString(color).c_str() : toString(layer->bufferHandle()).c_str(),
            crop.left, crop.top, crop.right, crop.bottom,
            frame.left, frame.top, frame.right, frame.bottom,
            channel ? channel->ScaleX : 0.0f,
            channel ? channel->ScaleY : 0.0f,
            getErrorCodeString(track.FallbackReason));
}

void LayerPlanner::debugPrint(CompositionContext* context, PlanningOutput* output) const
{
    if (!DEBUG_ENABLE(kTagStrategy))
        return;
    DLOGD("Composition|Channel|Slot|Z-hw|Z-sf|Alpha| BlendingMode|Transform| Dataspace|PixelForamt|      Handle      |       sourceCrop      |      displayFrame     |  scale  |");
    DLOGD("-----------+-------+----+----+----+-----+-------------+---------+----------+-----------+------------------+-----------------------+-----------------------+---------+");

    for (const auto& layer : context->InputLayers) {
        printLayer(layer, output);
    }
    printLayer(context->FramebufferTarget, output);
}

} // namespace sunxi
