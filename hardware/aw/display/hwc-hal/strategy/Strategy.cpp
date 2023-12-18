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

#include <array>
#include <math.h>

#include "Rect.h"
#include "ScreenTransform.h"
#include "Strategy.h"
#include "Debug.h"
#include "helper.h"

namespace sunxi {

std::shared_ptr<StrategyBase> createStrategy(StrategyType type) {
    switch (type) {
    case eGPUOnly:
        return std::make_shared<StrategyGPUOnly>();
    case eHardwareFirst:
        return std::make_shared<StrategyHardwareFirst>();
    }
}

//------------------------------------------------------------------------------------//

int StrategyGPUOnly::perform(const PlanConstraint& constraint,
        CompositionContext* context, PlanningOutput* out)
{
    // Make all input layers as GPU composition
    for (auto& layer : context->InputLayers) {
        layer->setValidatedType(HWC2::Composition::Client);
        out->Tracks.emplace(layer->id(), TrackNode(eForceGpuComposition));
    }

    if (context->FramebufferTarget && context->InputLayers.size()) {
        context->FramebufferTarget->setValidatedType(HWC2::Composition::Device);
        std::shared_ptr<ChannelConfig> config = std::make_shared<ChannelConfig>();
        config->Index = 0;
        config->VideoChannel = false;
        config->ScaleX = 1.0f;
        config->ScaleY = 1.0f;

        config->Slots[0].Enable = true;
        config->Slots[0].DimLayer = false;
        config->Slots[0].Z = 0;
        config->Slots[0].InputLayer = context->FramebufferTarget;

        out->Configs.clear();
        out->Configs.push_back(config);

        out->Tracks.emplace(
                context->FramebufferTarget->id(),
                TrackNode(eClientTarget, config, 0));
    }

    // Emplace empty configs to PlanningOutput,
    // so that we can close the unused hardware channel.
    int emptyChannelCount = constraint.totalChannelCount - out->Configs.size();
    while (emptyChannelCount) {
        auto config = std::make_shared<ChannelConfig>();
        out->Configs.push_back(config);
        emptyChannelCount--;
    }

    return 0;
}

void StrategyGPUOnly::release() { }

//------------------------------------------------------------------------------------//

struct ZoomFactor {
    float x = 0.0f;
    float y = 0.0f;
};

// DE Hardware channel abstract
class StrategyHardwareFirst::HWChannel {
public:
    HWChannel()
        : mHardwareIndex(0),
          mType(RGB_CHANNEL),
          mAsVideoChannel(false),
          mAsFrameBufferTarget(false),
          mPixelFormat(-1),
          mBlending(HWC2::BlendMode::Invalid),
          mIsAFBC(false),
          mPlaneAlpha(1.0f),
          mNextFreeSlot(0),
          mDisplayFrameList() { }

    void insert(std::shared_ptr<Layer> layer, const ZoomFactor& zoom, int expectSlots);
    bool match(const std::shared_ptr<Layer>& layer,
            const ZoomFactor& zoom, size_t slot_expend) const;
    bool intersect(const std::shared_ptr<Layer>& layer) const;
    void updateDisplayFrame(const std::shared_ptr<Layer>& layer);
    void setChannelType(HWChannelType type) { mType = type; }

    friend class StrategyHardwareFirst;

private:
    size_t freeSlots() const { return (SLOT_PER_CHANNEL - mNextFreeSlot); }
    void setupChannelConfig(std::shared_ptr<ChannelConfig>& config) const;

    const float EPSINON = 0.001;

    size_t mHardwareIndex;
    HWChannelType mType;

    bool mAsVideoChannel;
    bool mAsFrameBufferTarget;

    // Pixel format of the input layer on first slot
    int mPixelFormat;  /* android_pixel_format_t  */

    HWC2::BlendMode mBlending;
    bool mIsAFBC;

    float mPlaneAlpha;
    ZoomFactor mZoomFactor;

    // Each hardware channel has 4 slots to serve input layer,
    // Those input layer must have the same plane alpha and scale factor,
    // And do not support alpha blending but overlay.
    struct Slot {
        std::shared_ptr<Layer> InputLayer;
        size_t Z;
        bool DimLayer;
        bool DepthView;
    };

    size_t mNextFreeSlot;
    std::array<Slot, SLOT_PER_CHANNEL> mSlots;

    std::vector<hwc_rect_t> mDisplayFrameList;
};

// The zoom factor is the target window size to the source window size
static ZoomFactor computeLayerZoomFactor(
        const std::shared_ptr<Layer>& layer, const ScreenTransform* transform)
{
    ZoomFactor zoom;
    if (!layer || isSoildLayer(layer)) {
        zoom.x = 1.0f;
        zoom.y = 1.0f;
        return zoom;
    }

    Rect dst = transform->transform(Rect(layer->displayFrame()));

    Rect src(layer->sourceCrop());
    int src_width  = src.width();
    int src_height = src.height();
    int32_t rotate = static_cast<int32_t>(layer->transform());
    if (rotate & HAL_TRANSFORM_ROT_90) {
        std::swap(src_width, src_height);
    }

    int dst_width = dst.width();
    if (src_width == 0 || src_width == 1)
        zoom.x = 1.0f;
    else
        zoom.x = ((float)dst_width) / ((float)src_width);

    int dst_height = dst.height();
    if (src_height == 0 || src_height == 1)
        zoom.y = 1.0f;
    else
        zoom.y = ((float)dst_height) / ((float)src_height);

    return zoom;
}

void StrategyHardwareFirst::HWChannel::insert(
        std::shared_ptr<Layer> layer, const ZoomFactor& zoom, int expectSlots)
{
    // Init the channel attributes on first ref
    if (mNextFreeSlot == 0) {
        mPlaneAlpha = layer->alpha();
        mPixelFormat = getLayerPixelFormat(layer);
        mZoomFactor = zoom;
        mBlending = layer->blending();
        mIsAFBC = isAfbcBuffer(layer->bufferHandle());
    }

    Slot& slot = mSlots[mNextFreeSlot];
    slot.InputLayer = layer;
    slot.Z = 0;
    slot.DimLayer = isSoildLayer(layer);
    slot.DepthView = false;

    mDisplayFrameList.push_back(layer->displayFrame());
    mNextFreeSlot += expectSlots;
}

bool StrategyHardwareFirst::HWChannel::match(
        const std::shared_ptr<Layer>& layer, const ZoomFactor& zoom, size_t slot_expend) const
{
    if (freeSlots() < slot_expend)
        return false;

    /*only support one afbc layer*/
    if (mIsAFBC)
        return false;

    /*alpha mode must the same*/
    if (mBlending != layer->blending())
        return false;

    // The alpha of the layer in the same channel needs to be the same
    if (fabs(layer->alpha() - mPlaneAlpha) > EPSINON)
        return false;

    /*the format must the same in the same channel, no matter video or ui*/
    if (mPixelFormat != getLayerPixelFormat(layer))
        return false;

    /*
    if (isYuvFormat(layer)) {
        if (!mAsVideoChannel ||
                mPixelFormat != getLayerPixelFormat(layer)) {
            return false;
        } else {
            if (mPixelFormat != getLayerPixelFormat(layer))
                return false;
        }
    }*/

    if (fabs(zoom.x - mZoomFactor.x) > EPSINON
            || fabs(zoom.y - mZoomFactor.y) > EPSINON) {
        return false;
    }

    /* DO NOT share a small buffer with other layers on a hardware channel ! */
    Rect crop{layer->sourceCrop()};
    if ((mZoomFactor.x != 1.0f || mZoomFactor.y != 1.0f) &&
        (crop.width() < 16 || crop.height() < 16)) {
            return false;
    }

    return true;
}

static inline bool rectangle_intersect(const hwc_rect_t& a, const hwc_rect_t& b)
{
    int x0, y0, x1, y1;
    int distancex, distancey;
    int sumw, sumh;

    x0 = (a.right  + a.left) / 2;
    y0 = (a.bottom + a.top ) / 2;
    x1 = (b.right  + b.left) / 2;
    y1 = (b.bottom + b.top ) / 2;

    distancex = std::abs(x0 - x1);
    distancey = std::abs(y0 - y1);

    sumw = ((a.right  - a.left) + (b.right  - b.left)) / 2;
    sumh = ((a.bottom - a.top ) + (b.bottom - b.top )) / 2;

    return (distancex <= sumw && distancey <= sumh);
}

bool StrategyHardwareFirst::HWChannel::intersect(
        const std::shared_ptr<Layer>& layer) const
{
    return std::any_of(
            mDisplayFrameList.begin(),
            mDisplayFrameList.end(),
            [&](const auto& rect) {
                return rectangle_intersect(rect, layer->displayFrame());
            });
}

void StrategyHardwareFirst::HWChannel::updateDisplayFrame(
        const std::shared_ptr<Layer>& layer)
{
    mDisplayFrameList.push_back(layer->displayFrame());
}

void StrategyHardwareFirst::HWChannel::setupChannelConfig(
        std::shared_ptr<ChannelConfig>& config) const
{
    config->Index = mHardwareIndex;
    config->VideoChannel = mAsVideoChannel;
    config->ScaleX = mZoomFactor.x;
    config->ScaleY = mZoomFactor.y;

    for (int i = 0; i < mSlots.size(); ++i) {
        if (mSlots[i].InputLayer) {
            config->Slots[i].Enable = true;
            config->Slots[i].DimLayer = mSlots[i].DimLayer;
            config->Slots[i].Z = mSlots[i].Z;
            config->Slots[i].InputLayer = mSlots[i].InputLayer;
        }
    }
}

//------------------------------------------------------------------------------------//

StrategyHardwareFirst::StrategyHardwareFirst()
    : StrategyBase("HardwareFirst"),
      mTotalInputYuvLayerCnt(0),
      mReservedYUVChannelCnt(0),
      mReservedYUVChannel(),
      mFreeChannel(),
      mFreeRgbOverlayChannel(),
      mAssignedChannel(),
      mClientTarget(),
      mChannelExhausted(false),
      mNeedClientTarget(false),
      mGpuCompositonLayerCnt(0),
      mClientCompositionLayers() { }

StrategyHardwareFirst::~StrategyHardwareFirst()
{

}

int StrategyHardwareFirst::perform(const PlanConstraint& constraint,
        CompositionContext* context, PlanningOutput* out)
{
    DLOGD_IF(kTagStrategy, "Assign start >>>>");

    // Calculate the total number of yuv layers
    mTotalInputYuvLayerCnt = 0;
    std::for_each(context->InputLayers.begin(), context->InputLayers.end(),
        [this](const std::shared_ptr<Layer>& layer) {
            if (isYuvFormat(layer) || isViChannelExclusiveRgbFormat(layer))
                this->mTotalInputYuvLayerCnt += 1;
        }
    );
    // reserved as much yuv channel as possible
    mReservedYUVChannelCnt = mTotalInputYuvLayerCnt < constraint.MaxHybridChannel ?
        mTotalInputYuvLayerCnt : constraint.MaxHybridChannel;

    DLOGD_IF(kTagStrategy,
            "Init assign, mTotalInputYuvLayerCnt: %d mReservedYUVChannelCnt: %d",
            mTotalInputYuvLayerCnt, mReservedYUVChannelCnt);

    // No need to use GPU composition by default
    mNeedClientTarget = false;

    while (true) {
        initAssignment(constraint, context);

        AssignRetCode error = doAssignment(constraint, context);

        if (error == ASSIGN_SUCCESS) {
            if (mChannelExhausted && !mReservedYUVChannel.empty()) {
                // The UI channel has been runs out of, but there still remaining YUV channel,
                // reduce the reserved yuv channel count, so that we can use it for rgb layer.
                mReservedYUVChannelCnt -= mReservedYUVChannel.size();
                if (mReservedYUVChannelCnt < 0)
                    mReservedYUVChannelCnt = 0;

                // We has a new hardware channel,
                // reset and perform another assignment.
                mChannelExhausted = false;
                mNeedClientTarget = false;
                DLOGD_IF(kTagStrategy, "Reduce YUV channel and try again");
                continue;
            }

            // Wow, Assign finish!
            break;
        } else if (error == ASSIGN_FAILED_NO_CLIENT_TARGET_CHANNEL) {
            // If the mNeedClientTarget is already true, it means we have in dead loop!
            // So, we just let is crash!
            assert(mNeedClientTarget == false);

            mNeedClientTarget = true;
            DLOGD_IF(kTagStrategy, "Reserved Client target and try again");
            continue;
        } else {
            DLOGE("Failed with unknow error");
            return -1;
        }
    }

    if (mClientTarget) {
        DLOGD_IF(kTagStrategy,
                "Assign client target, GPU composition layer count: %d",
                mGpuCompositonLayerCnt);
        ZoomFactor zoom = computeLayerZoomFactor(context->FramebufferTarget,
                constraint.ScreenTransform);
        mClientTarget->insert(context->FramebufferTarget, zoom, 1);
    }
    acceptAssignResult(context, out);
    DLOGD_IF(kTagStrategy, "Assign finish <<<<");
    return 0;
}

void StrategyHardwareFirst::release()
{
    // We have finished this frame, clean all reference of CompositionContext
    mAssignedChannel.clear();
    mClientCompositionLayers.clear();
}

StrategyHardwareFirst::HWChannelType StrategyHardwareFirst::pickChannelType(
        const std::shared_ptr<Layer>& layer)
{
    if (isYuvFormat(layer) || isViChannelExclusiveRgbFormat(layer))
        return YUV_CHANNEL;
    else if (isBlendingLayer(layer))
        return RGB_CHANNEL;
    else
        return RGB_WITHOUT_ALPHA_CHANNEL;
}

int StrategyHardwareFirst::initAssignment(
        const PlanConstraint& constraint, CompositionContext *context)
{
    // Reset all layer's validated type as **Invalid**
    for (const std::shared_ptr<Layer>& layer : context->InputLayers) {
        layer->setValidatedType(HWC2::Composition::Invalid);
    }

    mBWMeter.reset();
    mBWMeter.setBandwidthlimitation(constraint.MaxBandwidth);

    mClientTarget = nullptr;
    mGpuCompositonLayerCnt = 0;
    mChannelExhausted = false;

    mClientCompositionLayers.clear();
    mAssignedChannel.clear();
    mReservedYUVChannel.clear();
    mFreeChannel.clear();
    mFreeRgbOverlayChannel.clear();

    int totalChannel = constraint.MaxHybridChannel + constraint.MaxRGBChannel;
    while (totalChannel > 0) {
        auto chptr = std::make_unique<HWChannel>();
        mFreeChannel.push_back(std::move(chptr));
        --totalChannel;
    }

    for (int reserved = mReservedYUVChannelCnt; reserved > 0; reserved--) {
       std::unique_ptr<HWChannel>& channel = moveOneChannel(
               mReservedYUVChannel, mFreeChannel);
       channel->setChannelType(YUV_CHANNEL);
    }

    if (!constraint.HybridChannelWithAlpha) {
        int overlayChannelCount = constraint.MaxHybridChannel - mReservedYUVChannelCnt;
        for (; overlayChannelCount > 0; --overlayChannelCount) {
            std::unique_ptr<HWChannel>& channel = moveOneChannel(
                    mFreeRgbOverlayChannel, mFreeChannel);
            channel->setChannelType(RGB_WITHOUT_ALPHA_CHANNEL);
        }
    }

    if (mNeedClientTarget) {
        if (!mFreeChannel.empty()) {
            mClientTarget = std::move(mFreeChannel.back());
            mClientTarget->mAsFrameBufferTarget = true;
            mFreeChannel.pop_back();
            DLOGD_IF(kTagStrategy, "Reserved Client target on start");

            mBWMeter.add(computeLayerMemoryFootprint(context->FramebufferTarget));
        } else {
            DLOGE("Cannot allocate client target channel");
            return -EINVAL;
        }
    }
    return 0;
}

StrategyHardwareFirst::AssignRetCode StrategyHardwareFirst::doAssignment(
        const PlanConstraint& constraint, CompositionContext* context)
{
    // Not any InputLayers, just return success.
    if (context->InputLayers.empty()) {
        return ASSIGN_SUCCESS;
    }

    for (const std::shared_ptr<Layer>& layer : context->InputLayers) {
        int error = 0;
        int fallbackReason = 0;
        bool gpuComposition = false;

        // SurfaceFlinger has mark this layer as GPU composition,
        // set to GPU composition
        if (isSkipLayer(layer)) {
            fallbackReason = eSkipBySurfaceFlinger;
            gpuComposition = true;
            goto __assign_as_gpu_composition;
        }

        // If it intersects with the previous GPU composite layer,
        // set to GPU composition
        if (mGpuCompositonLayerCnt && mClientTarget->intersect(layer)) {
            fallbackReason = eCoverByClientTarget;
            gpuComposition = true;
            goto __assign_as_gpu_composition;
        }

        // Everything seems ok !
        // Try to assign to hardware composition
        error = dispatchToDevice(constraint, layer);
        if (error == 0) {
            // Success make it as hardware composition
            continue;
        } else {
            fallbackReason = error;
            gpuComposition = true;
        }

__assign_as_gpu_composition:
        // If uhe bandwidth is not enough for current layer, and the Client target
        // has not been assigned yet (It means that we had not enough memory bandwidth for client target).
        // So we need to roll back and reserve the Client target first.
        if (fallbackReason == eMemoryLimit && mClientTarget == nullptr) {
            DLOGW_IF(kTagStrategy, "No available bandwidth for client target");
            return ASSIGN_FAILED_NO_CLIENT_TARGET_CHANNEL;
        }

        if (gpuComposition) {
            if (mClientTarget == nullptr) {
                // check if we have enough memory bandwidth for the client target layer.
                size_t memoryFootprint = computeLayerMemoryFootprint(context->FramebufferTarget);
                if (mBWMeter.avaliable() < memoryFootprint) {
                    DLOGI_IF(kTagStrategy, "No enought memory bandwidth for client target");
                    return ASSIGN_FAILED_NO_CLIENT_TARGET_CHANNEL;
                }

                mClientTarget = allocateHWChannel(RGB_CHANNEL);
                if (mClientTarget == nullptr) {
                    DLOGI_IF(kTagStrategy, "No free channel for client target");
                    return ASSIGN_FAILED_NO_CLIENT_TARGET_CHANNEL;
                }
                mClientTarget->mAsFrameBufferTarget = true;
                mBWMeter.add(memoryFootprint);
            }
            dispatchToClientTarget(layer, fallbackReason);
            mGpuCompositonLayerCnt++;
        }
    }

    return ASSIGN_SUCCESS;
}

void StrategyHardwareFirst::acceptAssignResult(
        CompositionContext* context, PlanningOutput* out)
{
    // merge all channels together
    if (mClientTarget) {
        mAssignedChannel.push_back(std::move(mClientTarget));
    }

    mergeChannels(mAssignedChannel, mReservedYUVChannel);
    mergeChannels(mAssignedChannel, mFreeRgbOverlayChannel);
    mergeChannels(mAssignedChannel, mFreeChannel);

    int zorder = 0;
    int hardwareIndex = 0;
    for (auto& channel : mAssignedChannel) {
        channel->mHardwareIndex = hardwareIndex++;
        std::shared_ptr<ChannelConfig> config = std::make_shared<ChannelConfig>();
        out->Configs.push_back(config);

        for (int i = 0; i < channel->mSlots.size(); ++i) {
            HWChannel::Slot& slot = channel->mSlots[i];
            if (slot.InputLayer != nullptr) {
                slot.Z = zorder++;
                slot.InputLayer->setValidatedType(
                        isSoildLayer(slot.InputLayer) ? HWC2::Composition::SolidColor : HWC2::Composition::Device);

                // record this layer
                out->Tracks.emplace(
                        slot.InputLayer->id(),
                        TrackNode(eHardwareLayer, config, i));
            }
        }
        channel->setupChannelConfig(config);
    }

    // Client composition layers
    for (auto& p : mClientCompositionLayers) {
        std::shared_ptr<Layer>& layer = p.first;
        layer->setValidatedType(HWC2::Composition::Client);
        out->Tracks.emplace(layer->id(), TrackNode(p.second));
    }

    if (mGpuCompositonLayerCnt == 0 && context->FramebufferTarget) {
        // No any Gpu composition layer,
        // make the FrameBufferTarget as not used
        out->Tracks.emplace(context->FramebufferTarget->id(), TrackNode(eClientTarget));
    }
}

int StrategyHardwareFirst::dispatchToDevice(
        const PlanConstraint& constraint, std::shared_ptr<Layer> layer)
{
    bool soildLayer = isSoildLayer(layer);

    if (!soildLayer && layer->bufferHandle() == 0) {
        DLOGD_IF(kTagStrategy, "Null BufferHandle");
        return eNullBufferHandle;
    }

    // Check memory bandwidth limit
    size_t memoryFootprint = computeLayerMemoryFootprint(layer);
    if (mBWMeter.avaliable() < memoryFootprint) {
        return eMemoryLimit;
    }

    // Check whether can handle by hardware
    if (!soildLayer && constraint.Detector != nullptr) {
        int error = constraint.Detector->capable(layer);
        if (error != 0) {
            DLOGD_IF(kTagStrategy, "Hardware not capable: %d", error);
            return error;
        }
    }

    // Find the lowest channel which has not intersect with this layer.
    size_t lowestNotIntersectPos = SIZE_MAX;
    for (int index = mAssignedChannel.size() - 1; index >= 0; --index) {
        const std::unique_ptr<HWChannel>& channel = mAssignedChannel[index];
        if (channel->intersect(layer))
            break;
        lowestNotIntersectPos = index;
    }

    // Find the lowest match channle
    int expectSlot = constraint.Output3D ? 2 : 1;
    ZoomFactor zoom = computeLayerZoomFactor(layer, constraint.ScreenTransform);
    size_t matchIndex = lowestNotIntersectPos;
    while (matchIndex < mAssignedChannel.size()) {
        const std::unique_ptr<HWChannel>& channel = mAssignedChannel[matchIndex];
        if (channel->match(layer, zoom, expectSlot)) {
            break;
        }
        ++matchIndex;
    }

    //  No match channel, alloc a new channel.
    if (matchIndex == SIZE_MAX || matchIndex == mAssignedChannel.size()) {
        HWChannelType type = pickChannelType(layer);
        std::unique_ptr<HWChannel> channel = allocateHWChannel(type);
        if (channel == nullptr) {
            DLOGD_IF(kTagStrategy, "No free channel(type=%d)", type);

            // We need this flag to optimize the assign process.
            // eg. if mChannelExhausted is true but there still free video channle leave,
            //     we should reduce the mReservedYUVChannelCnt;
            mChannelExhausted = true;

            return eNoFreeChannel;
        }

        mAssignedChannel.push_back(std::move(channel));
        matchIndex = mAssignedChannel.size() - 1;
    }

    // wow, We had find an available channel !!!
    const std::unique_ptr<HWChannel>& channel = mAssignedChannel[matchIndex];
    channel->insert(layer, zoom, expectSlot);
    mBWMeter.add(memoryFootprint);
    return 0;
}

int StrategyHardwareFirst::dispatchToClientTarget(
        std::shared_ptr<Layer> layer, int reason)
{
    assert(mClientTarget != nullptr);

    // Just store the displayFrame, So that we can use this
    // to identify the region covered by client target.
    mClientTarget->updateDisplayFrame(layer);
    mClientCompositionLayers.emplace_back(std::make_pair(layer, reason));
    return 0;
}

std::unique_ptr<StrategyHardwareFirst::HWChannel>
        StrategyHardwareFirst::allocateHWChannel(HWChannelType t)
{
    std::unique_ptr<HWChannel> alloc;
    switch (t) {
    case YUV_CHANNEL:
        if (mReservedYUVChannel.empty()) {
            alloc = nullptr;
        } else {
            alloc = std::move(mReservedYUVChannel.back());
            alloc->mAsVideoChannel = true;
            mReservedYUVChannel.pop_back();
        }
        break;
    case RGB_WITHOUT_ALPHA_CHANNEL:
        if (!mFreeRgbOverlayChannel.empty()) {
            alloc = std::move(mFreeRgbOverlayChannel.back());
            mFreeRgbOverlayChannel.pop_back();
            break;
        }
        // If there is not free RGB overlay channel, just try to
        // request channel from mFreeChannel!
        // So we intend to fallback to RGB_CHANNEL here!
        [[fallthrough]];
    case RGB_CHANNEL:
        if (mFreeChannel.empty()) {
            alloc = nullptr;
        } else {
            alloc = std::move(mFreeChannel.back());
            mFreeChannel.pop_back();
        }
        break;
    }
    return alloc;
}

void StrategyHardwareFirst::mergeChannels(std::vector<std::unique_ptr<HWChannel>>& to,
        std::vector<std::unique_ptr<HWChannel>>& from)
{
    while (!from.empty()) {
        to.push_back(std::move(from.back()));
        from.pop_back();
    }
}

std::unique_ptr<StrategyHardwareFirst::HWChannel>& StrategyHardwareFirst::moveOneChannel(
        std::vector<std::unique_ptr<HWChannel>>& to,
        std::vector<std::unique_ptr<HWChannel>>& from)
{
    to.push_back(std::move(from.back()));
    from.pop_back();
    return to.back();
}

} // namespace sunxi
