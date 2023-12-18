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

#include <mutex>
#include "CompositionEngine.h"
#include "eink_update.h"
#include "IonBuffer.h"
#include "BufferPool.h"

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
    int submitLayer(unsigned int syncnum, disp_layer_config2 *configs,
                    int configCount, std::shared_ptr<struct eink_img> &cur_img,
                    bool ForceRefresh, struct upd_win *handwrite_win) override;
    void reconfigBandwidthLimitation(const DeviceBase::Config& config) override;
	int putWbBuffer(const PrivateBuffer_t* buf, int releaseFence);
	int acquireBuffer(const std::shared_ptr<Layer>& layer);

private:
    explicit CompositionEngineV2Impl(int id);
    int setup();
    int calculateBandwidthLimitation() const;

private:
	//same as CommitThread::mMaxQueuedFrame
    const int EINK_PIPE_DEPTH = 32;
    std::unique_ptr<BufferPool> mBufferPool;
    std::unique_ptr<BufferPoolCache> mBufferPoolCache;
	uint64_t mBufferPoolUsage;
	std::unique_ptr<eink_update::OutputBuffer_t> last_buffer;
	std::shared_ptr<struct eink_img> last_img;
	uniquefd last_buffer_fence;
	bool firstframe;
	int img_no;
    std::mutex mImgLock;
	//to next module
    std::unique_ptr<eink_update> mEinkUpdate;

    const int mHardwareIndex;
    bool mSyncTimelineActive;
    Attribute mAttribute;
    int mFrequency;
    bool mAfbcBufferSupported;
    bool mIommuEnabled;
};

} // namespace sunxi

#endif
