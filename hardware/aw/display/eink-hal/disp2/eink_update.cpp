/*
 * disp2/eink_update/eink_update.cpp
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
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
#include <cutils/properties.h>
#include "Debug.h"
#include "disputils.h"
#include "helper.h"
#include "errorcode.h"
#include "sunxi_eink.h"
#include "private_handle.h"
#include "eink_update.h"
#include "syncfence.h"
#include "disputils.h"
#include "CompositionEngineV2Impl.h"
#include <sys/resource.h>
#include <sync/sync.h>
#include <android-base/stringprintf.h>
#include <processgroup/sched_policy.h>

namespace sunxi {

eink_update::~eink_update()
{
    mRunning.store(false);
    mCondition.notify_all();
    mEinkUpdateThread.join();
	mFenceSignalThread.join();
}

eink_update::eink_update(CompositionEngineV2Impl* engine)
{
    if (!engine) {
        DLOGE("CompositionEngine is null");
        return;
	}
	mDisplayEngine = engine;
	//start einkupdate thread
    mRunning.store(true);
    mEinkUpdateThread = std::thread(&eink_update::EinkUpdate, this);
    pthread_setname_np(mEinkUpdateThread.native_handle(), "EinkUpdate");
    mFenceSignalThread = std::thread(&eink_update::FenceSignalThread, this);
    pthread_setname_np(mFenceSignalThread.native_handle(), "FenceSignal");

    int fd = fence_timeline_create();
    if (fd < 0) {
        DLOGE("sw sync timeline create failed: %s", strerror(errno));
        return;
    }
    mTimelineValue = 0;
    mSignaledPtValue = 0;
    mSyncTimelineHandle = sunxi::uniquefd(fd);
}

void eink_update::FenceSignalThread()
{
    DLOGI("eink fence signal thread start.");
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY+3);
	int ret = -1;

    while (true) {

		struct buf_slot slot;
		int i = 0;
		if (mRunning.load() == false) {
			break;
		}

		ret = ::GetFreeBufSlot(&slot);
		if (!ret) {
			for (i = 0; i < slot.count; ++i) {
				FenceSignal(slot.upd_order[i]);
			}
		}
	}
}

void eink_update::EinkUpdate()
{
    DLOGI("eink update thread start.");
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY+3);
    while (true) {
		std::unique_ptr<eink_update::OutputBuffer_t> Buffer;
		{//scope of mLock
			std::unique_lock<std::mutex> lock(mBufferLock);
			if (mPendingOutBuffer.empty()) {
				if (mRunning.load() == false) {
					break;
				}
				mCondition.wait(lock);
			}
			if (mPendingOutBuffer.empty())
				continue;
			Buffer = std::move(mPendingOutBuffer.front());
			mPendingOutBuffer.pop();
		}

		struct eink_upd_cfg config;
		config.order = Buffer->img_no;
		config.img = Buffer->cur_img;
		config.force_fresh = 0;
		::eink_update_image(&config);
		mDisplayEngine->putWbBuffer(
									Buffer->WbOutBuf,
									-1);

    }
}

unsigned int eink_update::GetPipeCount()
{
	std::unique_lock<std::mutex> lock(mLock);
	return mTimelineValue - mSignaledPtValue;
}

int eink_update::FenceCreate(syncinfo *info)
{
	std::unique_lock<std::mutex> lock(mLock);
	int ret = -1;

	info->count = ++mTimelineValue;
	info->fd = fence_create(mSyncTimelineHandle.get(), "eink_update.fence",
							mTimelineValue);
	if (info->fd < 0)
		DLOGE("fence create failed: %s", strerror(errno));
	else
		ret = 0;

	return ret;
}

int eink_update::FenceSignal(unsigned int img_no)
{
	std::unique_lock<std::mutex> lock(mLock);
	//if (mSignaledPtValue != img_no) {
		//DLOGW("fence timeline corruption, mTimelineValue=%d mSignaledPtValue=%d img_no=%d",
			  //mTimelineValue, mSignaledPtValue, img_no);
	//}
	++mSignaledPtValue;
	return fence_timeline_inc(mSyncTimelineHandle.get(), 1);
}

int eink_update::queueBuffer(std::unique_ptr<struct OutputBuffer_t> buffer)
{
	std::unique_lock<std::mutex> lock(mBufferLock);
	int ret = -1;

	if (buffer.get() == nullptr) {
		DLOGE("NUll pointer!\n");
		goto OUT;
	}

	mPendingOutBuffer.push(std::move(buffer));
    mCondition.notify_all();
	ret = 0;

OUT:
	return ret;

}

}
