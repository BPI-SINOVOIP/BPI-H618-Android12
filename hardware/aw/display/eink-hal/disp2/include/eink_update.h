/*
 * eink_update.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _EINK_UPDATE_H
#define _EINK_UPDATE_H

#include <memory>
#include <map>
#include <thread>
#include <hardware/hwcomposer_defs.h>
#include "uniquefd.h"
#include <mutex>
#include "IonBuffer.h"
#include <queue>

namespace sunxi {

class CompositionEngineV2Impl;


class eink_update {
public:
	eink_update(CompositionEngineV2Impl* engine);
	~eink_update();
	struct OutputBuffer_t {
		struct eink_img cur_img;
		uniquefd acqfence;
		uniquefd releaseFence;
		const PrivateBuffer_t* WbOutBuf;
		unsigned int img_no;
	};
	int queueBuffer(std::unique_ptr<struct OutputBuffer_t> buffer);
	int FenceCreate(syncinfo *info);
	unsigned int GetPipeCount();


private:
    void EinkUpdate();
    void FenceSignalThread();
	int FenceSignal(unsigned int img_no);

private:
	std::queue<std::unique_ptr<eink_update::OutputBuffer_t>> mPendingOutBuffer;
	CompositionEngineV2Impl* mDisplayEngine;
	std::thread mEinkUpdateThread;
	std::thread mFenceSignalThread;
    uniquefd mSyncTimelineHandle;
    unsigned int mTimelineValue;
    unsigned int mSignaledPtValue;
    std::atomic<bool> mRunning; //status of thread;
    mutable std::condition_variable mCondition;
    mutable std::mutex mLock;
    mutable std::mutex mBufferLock;
};

}

#endif /*End of file*/
