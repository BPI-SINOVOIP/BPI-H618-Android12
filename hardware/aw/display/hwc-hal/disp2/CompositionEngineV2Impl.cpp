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

#include <cutils/properties.h>

#include "BandwidthMeter.h"
#include "CompositionEngineV2Impl.h"
#include "Debug.h"
#include "disputils.h"
#include "helper.h"
#include "LayerPlanner.h"
#include "errorcode.h"
#include "draminfo.h"
#include "WriteBackManager.h"
#include "Rect.h"

namespace sunxi {

int CompositionEngineV2Impl::mSyncTimelineActive[CONFIG_MAX_DISPLAY_ENGINE] = {0};
// static
std::shared_ptr<CompositionEngineV2Impl> CompositionEngineV2Impl::create(int id)
{
    if (id < 0 || id >= CONFIG_MAX_DISPLAY_ENGINE) {
        DLOGE("Display Engine id(%d) not support");
        return nullptr;
    }
    std::shared_ptr<CompositionEngineV2Impl> engine(new CompositionEngineV2Impl(id));
    if (engine->setup() != 0) {
        DLOGE("Display Engine init error");
        return nullptr;
    }
    return engine;
}

CompositionEngineV2Impl::CompositionEngineV2Impl(int id)
  : mHardwareIndex(id),
    mAfbcBufferSupported(false),
    mIommuEnabled(true)
{ }

CompositionEngineV2Impl::~CompositionEngineV2Impl()
{
    if (mSyncTimelineActive[mHardwareIndex] > 0)
        mSyncTimelineActive[mHardwareIndex]--;
    else
       DLOGI("~CompositionEngineV2Impl get incorrect mSyncTimelineActive for de%d", mHardwareIndex);
    if (mSyncTimelineActive[mHardwareIndex] == 0) {
        int error = destroySyncTimeline(mHardwareIndex);
        DLOGI("timeline%d destroy",mHardwareIndex);
        if (error != 0)
            DLOGE("destroySyncTimeline return error = %d", error);
    }
    DLOGI("CompositionEngine(%d) destroy use cnt=%d", mHardwareIndex,
          mSyncTimelineActive[mHardwareIndex]);
}

int CompositionEngineV2Impl::setup()
{
    int error = 0;
    if (mSyncTimelineActive[mHardwareIndex] == 0)
        error = createSyncTimeline(mHardwareIndex);
    DLOGI("timeline%d create",mHardwareIndex);
    if (error != 0) {
        DLOGE("createSyncTimeline return error = %d", error);
        return error;
    }
    mSyncTimelineActive[mHardwareIndex]++;

    const auto& info = HardwareConfigs[mHardwareIndex];
    mAttribute.featureMask = info.featureMask;
    mAttribute.maxHybridChannelCount = info.videoChannelCount;
    mAttribute.maxRGBChannelCount = info.uiChannelCount;
    mAttribute.maxHdrHybridCount = info.videoHdrCount;
    mAttribute.maxHdrRGBCount = info.uiHdrCount;
    mAttribute.maxBandwidth = calculateBandwidthLimitation();
    mFrequency = ::getDeFrequency();
    mAfbcBufferSupported = mAttribute.featureMask & eAfbcBufferSupported;

    DLOGI("Bandwidth limitation: %d Bytes per frame", mAttribute.maxBandwidth);
    return 0;
}

const CompositionEngine::Attribute& CompositionEngineV2Impl::getAttribute() const
{
    return mAttribute;
}

void CompositionEngineV2Impl::rearrangeChannel(
        std::vector<std::shared_ptr<ChannelConfig>>& configs) const
{
    std::vector<std::shared_ptr<ChannelConfig>> vchannels;
    for (auto iter = configs.begin(); iter != configs.end();) {
        if ((*iter)->VideoChannel == true) {
            vchannels.push_back(*iter);
            iter = configs.erase(iter);
        } else {
            ++iter;
        }
    }
    configs.insert(configs.begin(), vchannels.begin(), vchannels.end());

    size_t hardwareIndex = 0;
    std::for_each(configs.begin(), configs.end(),
            [&hardwareIndex](const auto& config) {
            config->Index = hardwareIndex;
            hardwareIndex++;
            });
}

int CompositionEngineV2Impl::createSyncpt(syncinfo *info)
{
    if (info == nullptr) {
        DLOGE("invalid argument, info is nullptr");
        return -1;
    }
    return ::createSyncpt(mHardwareIndex, info);
}

int CompositionEngineV2Impl::submitLayer(
        unsigned int syncnum, disp_layer_config2* configs, int configCount)
{
    DTRACE_FUNC();
    if (configs == nullptr) {
        DLOGE("invalid argument, configs is nullptr");
        return -1;
    }
#ifdef WRITE_BACK_MODE
    std::shared_ptr<WriteBackManager> manager = WriteBackManager::getWbInstance();
    if (manager != nullptr && manager->isSupportWb()) {
        if (manager->onFrameCommit(mHardwareIndex, syncnum,
                configs, configCount) == 0)
        return 0;
    }
#endif
    return ::submitLayer(mHardwareIndex, syncnum, configs, configCount);
}

int CompositionEngineV2Impl::capable(
        const ScreenTransform& transform, const std::shared_ptr<Layer>& layer)
{
    if (!layer || !layer->bufferHandle()) {
        DLOGW("nullptr layer or nullptr buffer");
        return eNullBufferHandle;
    }

    // buffer fromat filter
    if (!compositionEngineV2FormatFilter(getLayerPixelFormat(layer))) {
        return eNotSupportFormat;
    }

    // scaler check
    hwc_transform_t buf_transform = static_cast<hwc_transform_t>(layer->transform());
    hwc_rect_t sourceCrop = layer->sourceCrop();
    if (buf_transform & HAL_TRANSFORM_ROT_90) {
        // input source crop
        std::swap(sourceCrop.left,  sourceCrop.top);
        std::swap(sourceCrop.right, sourceCrop.bottom);
    }
    if (!hardwareScalerCheck(getDeFrequency(), transform,
                isYuvFormat(layer), sourceCrop, layer->displayFrame())) {
        return eScaleError;
    }

    if (isAfbcBuffer(layer->bufferHandle())) {
        if (mAfbcBufferSupported) {
            bool supported = afbcBufferScaleCheck(mFrequency,
                    transform, layer->sourceCrop(), layer->displayFrame());
            if (!supported) {
                return eScaleError;
            }
        } else {
            return eAfbcBuffer;
        }
    }

    if (!mIommuEnabled
            && !isPhysicalContinuousBuffer(layer->bufferHandle())) {
        return eNonPhysicalContinuousMemory;
    }

    /* DO NOT assign a small buffer(widht or height < MINIMUM_HARDWARE_LAYER_SIZE) into hardware channel ! */
    ::Rect crop{sourceCrop};
    if (crop.width() < MINIMUM_HARDWARE_LAYER_SIZE || crop.height() < MINIMUM_HARDWARE_LAYER_SIZE) {
        return eScaleError;
    }

    return 0;
}

static int computeDisplayEngineAvailableBandwidth(float factor)
{
#define ADEQUATE_BANDWIDTH_MAX (4200) // max adequate system bandwidth 4.1 GByte (792MHz 32bit DRAM)
#define ADEQUATE_BANDWIDTH_MIN (1024) // min adequate system bandwidth 1.0 GByte

    draminfo* info = draminfo::getInstance();
    int totalDramBandwidth = info->effectiveBandwidth();

    if (totalDramBandwidth < ADEQUATE_BANDWIDTH_MAX
            && totalDramBandwidth > ADEQUATE_BANDWIDTH_MIN) {
        int maxBandwidth = (int)floor(totalDramBandwidth * factor / 60);
        return maxBandwidth * 1000 * 1000;
    }
    return 0;
}

#ifdef CONFIG_RECONFIG_BANDWIDTH_FOR_FULL_HD
static bool isFullHDFrameBuffer(int width, int height) {
    return (width * height) >= (1920 * 1080);
}
#endif

void CompositionEngineV2Impl::reconfigBandwidthLimitation(
        const DeviceBase::Config& config)
{
    bool legacyPolicy = checkNeedLimitPolicy();
    if (!legacyPolicy) {
        // when maxBandwidth == 0, it means no bandwidth limit.
        mAttribute.maxBandwidth = BandwidthMeter::BANDWIDTH_NO_LIMIT;
        DLOGI("disable bandwidth limit!");
    }

#ifdef CONFIG_RECONFIG_BANDWIDTH_FOR_FULL_HD
    if (isFullHDFrameBuffer(config.width, config.height)) {
        // for ui resolution equal or exceed 1080p,
        // Limit the factor to 0.4 or 0.6 !
        int maxBandwidth = computeDisplayEngineAvailableBandwidth(legacyPolicy ? 0.40f : 0.60f);
        if (maxBandwidth > 0) {
            mAttribute.maxBandwidth = maxBandwidth;
        } else {
            mAttribute.maxBandwidth =
                BandwidthConfigs_FullHD[CONFIG_MAX_BANDWIDTH_LEVE - 1].maxAvaliableBandwidth;
        }
        DLOGI("reconfigBandwidthLimitation: %d Bytes per frame",
                mAttribute.maxBandwidth);
    }
#endif
}

int CompositionEngineV2Impl::calculateBandwidthLimitation() const
{
    // calculate bandwidth limitation according dram info,
    // if failed, fallback into predefined value.
    int maxBandwidth = computeDisplayEngineAvailableBandwidth(0.45);
    if (maxBandwidth > 0) {
        return maxBandwidth;
    }

    // Choose a suitable bandwidth config
    draminfo* info = draminfo::getInstance();
    int dramFreq = info->frequency();
    if (dramFreq <= 0) {
        DLOGE("getDramFrequency return error(%d), fallback to default bandwidth config", dramFreq);
        maxBandwidth = BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE - 1].maxAvaliableBandwidth;
    } else {
        for (int i = 0; i < CONFIG_MAX_BANDWIDTH_LEVE; i++) {
            if (dramFreq >= BandwidthConfigs[i].dramFrequency) {
                maxBandwidth = BandwidthConfigs[i].maxAvaliableBandwidth;
                break;
            }
        }
        maxBandwidth = maxBandwidth ?
            maxBandwidth: BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE - 1].maxAvaliableBandwidth;
    }
    return maxBandwidth;
}

} // namespace sunxi
