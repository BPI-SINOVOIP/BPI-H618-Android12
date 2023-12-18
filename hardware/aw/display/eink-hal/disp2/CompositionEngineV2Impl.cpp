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
#include "CompositionEngineV2Impl.h"
#include "CompositionEngineV2Impl_defs.h"
#include "Debug.h"
#include "disputils.h"
#include "helper.h"
#include "LayerPlanner.h"
#include "errorcode.h"
#include "draminfo.h"
#include "sunxi_eink.h"
#include "syncfence.h"
#include "private_handle.h"
#include <errno.h>

namespace sunxi {

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
	:mBufferPool(nullptr),
	mBufferPoolCache(new BufferPoolCache()),
	mHardwareIndex(id),
	mSyncTimelineActive(false),
	mAfbcBufferSupported(false),
	mIommuEnabled(true)
{
	last_img = std::make_shared<struct eink_img>();
	img_no = 0;
	mEinkUpdate = std::make_unique<eink_update>(this);
	firstframe = true;
}

CompositionEngineV2Impl::~CompositionEngineV2Impl()
{
    if (mSyncTimelineActive) {
        int error = destroySyncTimeline();
        if (error != 0)
            DLOGE("destroySyncTimeline return error = %d", error);
    }
    DLOGI("CompositionEngine(%d) destroy", mHardwareIndex);
    mBufferPool.reset();
}

int CompositionEngineV2Impl::setup()
{
    mSyncTimelineActive = true;

    const auto& info = HardwareConfigs[mHardwareIndex];
    mAttribute.featureMask = info.featureMask;
    mAttribute.maxHybridChannelCount = info.videoChannelCount;
    mAttribute.maxRGBChannelCount = info.uiChannelCount;
    mAttribute.maxBandwidth = calculateBandwidthLimitation();
    mFrequency = ::getDeFrequency();

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
    std::lock_guard<std::mutex> lock(mImgLock);
	int ret = -1;

    if (info == nullptr) {
        DLOGE("invalid argument, info is nullptr");
        return -1;
    }
	ret = ::createSyncpt(info);
    return ret;
}


int CompositionEngineV2Impl::submitLayer(unsigned int syncnum,
					 disp_layer_config2* configs,
					 int configCount,
					 std::shared_ptr<struct eink_img> &cur_img, bool ForceRefresh, struct upd_win *handwrite_win)
{
	int ret = -1;
    std::lock_guard<std::mutex> lock(mImgLock);
	int acquireFence = -1, pipe_cnt = 16;
    syncinfo syncpt;
	const PrivateBuffer_t *outbuf = nullptr;
	uniquefd fence;
	bool is_handwrite_layer = false, is_handwrite_mode = false;

    DTRACE_FUNC();
    if (configs == nullptr) {
		is_handwrite_layer = true;
    }

	if (mBufferPool.get() == nullptr)
		acquireBuffer(nullptr);

	if (mBufferPool.get() == nullptr) {
        DLOGE("mBufferPool is nullptr");
		goto OUT;
	}
	last_buffer = std::make_unique<eink_update::OutputBuffer_t>();
	last_buffer->WbOutBuf = nullptr;

	if (is_handwrite_layer == false) {
		ret = mBufferPool->acquireBuffer(&(last_buffer->WbOutBuf), &acquireFence);
		if (ret) {
			DLOGE("Acquire wb out buffer fail!");
			::FenceSignal(syncnum);
			goto OUT;
		}
		cur_img->fd = last_buffer->WbOutBuf->handle;
	} else
		pipe_cnt = 31;

	if (mEinkUpdate->GetPipeCount() >= pipe_cnt) {
		ret = fence_wait(last_buffer_fence.get(), 1000);
		if (ret != Signaled)
			DLOGI("Wait WbOutBufFence timeout!ret:%d fd:%d", ret, last_buffer_fence.get());
	}

	if (firstframe == true) {
		ret = mBufferPool->acquireBuffer(&(outbuf), &acquireFence);
		if (ret) {
			DLOGE("Allocate first last_img fail!\n");
			goto OUT;
		}

		last_img->fd = outbuf->handle;
		std::memset(outbuf->virt_addr, 0xff, outbuf->size);
		last_img->out_fmt = EINK_Y8;
	}

	if (is_handwrite_layer == false) {
		ret = ::submitLayer(syncnum, configs, configCount, cur_img.get(), last_img.get());
		if (ret) {
			DLOGE("submitLayer fail cur:%d last:%d\n", cur_img->fd, last_img->fd);
			mBufferPool->releaseBuffer(last_buffer->WbOutBuf, -1);
			::FenceSignal(syncnum);
			last_buffer_fence = uniquefd(-1);
			goto OUT;
		}
		::FenceSignal(syncnum);


		last_img = cur_img;
	}

	if (cur_img->upd_mode & HWC_EINK_HANDWRITTEN) {
		is_handwrite_mode = true;
	}

	cur_img->upd_mode = (enum upd_mode)(cur_img->upd_mode & ~HWC_EINK_MASK);
	cur_img->upd_mode = (cur_img->upd_mode) ? (enum upd_mode)cur_img->upd_mode : EINK_GU16_MODE;
	if (ForceRefresh == true) {
		cur_img->upd_mode = EINK_GC16_MODE;
		if (is_handwrite_layer == true) {
			cur_img->upd_win = *handwrite_win;
		} else {
			cur_img->upd_win.left = 0;
			cur_img->upd_win.top = 0;
			cur_img->upd_win.right = cur_img->size.width - 1;
			cur_img->upd_win.bottom = cur_img->size.height - 1;
		}
		cur_img->upd_all_en = 1;
	}

	if (firstframe == true) {
		firstframe = false;
		mBufferPool->releaseBuffer(outbuf, -1);
	}


	memcpy(&last_buffer->cur_img, cur_img.get(), sizeof(struct eink_img));
	last_buffer->img_no = img_no;


	if (is_handwrite_layer == false && is_handwrite_mode == true && ForceRefresh == false) {
		if (isUpdWinOverLap(&last_buffer->cur_img.upd_win, handwrite_win) == true) {
			ret = removeOverLapWin(&last_buffer->cur_img.upd_win, handwrite_win);
		}

	}

	if (isUpdWinZero(&last_buffer->cur_img.upd_win) == true) {
		mBufferPool->releaseBuffer(last_buffer->WbOutBuf, -1);
		last_buffer_fence = uniquefd(-1);
		goto OUT;
	}

	ret =  mEinkUpdate->FenceCreate(&syncpt);
	if (ret)
		DLOGI("Fence create fail");
    fence = uniquefd(syncpt.fd);
	last_buffer_fence = uniquefd(fence.dup());

	ret = mEinkUpdate->queueBuffer(std::move(last_buffer));
	++img_no;

OUT:
	return ret;
}


int CompositionEngineV2Impl::putWbBuffer(const PrivateBuffer_t* buf, int releaseFence)
{
    std::lock_guard<std::mutex> lock(mImgLock);
    mBufferPool->releaseBuffer(buf, releaseFence);

	return 0;
}

int CompositionEngineV2Impl::acquireBuffer(const std::shared_ptr<Layer>& layer)
{
	uint64_t usage = GRALLOC_USAGE_HW_COMPOSER;

    if (layer && layer->bufferHandle()) {
		const private_handle_t* handle = from(layer->bufferHandle());

		// if source buffer is secure, the output buffer must be secure too !
		if (handle->usage & GRALLOC_USAGE_PROTECTED) {
			usage |= GRALLOC_USAGE_PROTECTED;
		}
    }

	int width = 0, height = 0;
	::getDisplayOutputSize(0, &width, &height);
	uint32_t size = width * height;

	if (!mBufferPool) {
        if (mBufferPoolCache) {
            mBufferPool = mBufferPoolCache->get(size, usage);
            DLOGD_IF(kTagRotate, "get cached BufferPool:%p size:%d request size:%d",
                    mBufferPool.get(),
                    mBufferPool ? mBufferPool->bufferSize() : 0, size);
        }
		if (!mBufferPool) {
			mBufferPool = std::make_unique<BufferPool>(EINK_PIPE_DEPTH, size, usage, true);
		}
		mBufferPool->setName("mDisplayEngine");
	} else if((mBufferPoolUsage != usage) || (mBufferPool->bufferSize() != size)) {
		if (mBufferPoolCache) {
			mBufferPoolCache->put(std::move(mBufferPool));
			mBufferPool = mBufferPoolCache->get(size, usage);
			DLOGD_IF(kTagRotate, "get cached BufferPool:%p size:%d request size:%d",
					 mBufferPool.get(),
					 mBufferPool ? mBufferPool->bufferSize() : 0, size);
		}
		if (!mBufferPool) {
			mBufferPool = std::make_unique<BufferPool>(EINK_PIPE_DEPTH, size, usage, false);
		}
		mBufferPool->setName("mDisplayEngine");
	}

	mBufferPoolUsage = usage;
	return 0;
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


	acquireBuffer(layer);

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
#ifdef CONFIG_RECONFIG_BANDWIDTH_FOR_FULL_HD
    if (isFullHDFrameBuffer(config.width, config.height)) {
        // for ui resolution equal or exceed 1080p,
        // Limit the factor to 0.4!
        int maxBandwidth = computeDisplayEngineAvailableBandwidth(0.40);
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
