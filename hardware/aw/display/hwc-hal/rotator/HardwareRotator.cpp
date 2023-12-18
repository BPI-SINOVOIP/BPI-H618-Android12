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

#include <sys/resource.h>

#include "Debug.h"
#include "HardwareRotator.h"
#include "RotatorDevice.h"
#include "Layer.h"
#include "syncfence.h"
#include "utils.h"

namespace sunxi {

std::atomic<int> RotateTask::mNextTaskId {0};

RotateTask::RotateTask(const std::shared_ptr<Layer>& layer,
        std::unique_ptr<OutputBuffer_t> outBuf, hwc_transform_t transform)
    : mTaskId(mNextTaskId++),
      mSourceLayer(layer),
      mOutputBuffer(std::move(outBuf)),
      mTransform(transform),
      mFailedCallback(nullptr)
{ }

int RotateTask::getRotatedBuffer(const OutputBuffer_t** buffer, int* acquireFence)
{
    *buffer = mOutputBuffer.get();
    *acquireFence = mAcquireFence.release();
    return 0;
}

HardwareRotator::HardwareRotator()
  : mPendingTasks(),
    mHardwareRotator(nullptr),
    mThreadStop(false)
{
    mHardwareRotator = createHardwareRotator();
    if (mHardwareRotator) {
        initSyncTimeline();
        mRotateThread = std::thread(&HardwareRotator::rotateLoop, this);
        pthread_setname_np(mRotateThread.native_handle(), "rotate-thread");
    }
}

HardwareRotator::~HardwareRotator()
{
    mThreadStop = true;
    mRotateThread.join();
    mHardwareRotator.reset();
}

bool HardwareRotator::capable(const std::shared_ptr<Layer>& layer)
{
    {
        std::lock_guard<std::mutex> lock(mTaskLock);
        int pendingTasksCount = mPendingTasks.size();
        if (pendingTasksCount >= kPendingTaskThrottle) {
            DLOGW("pendingTasks count = %d, Something went wrong ?", pendingTasksCount);
            return false;
        }
    }

    if (mHardwareRotator)
        return mHardwareRotator->capableHook(layer);
    return false;
}

void HardwareRotator::getAlignedWidthAndHeight(
        const std::shared_ptr<Layer>& layer, int* width, int* height, int* align)
{
    if (mHardwareRotator)
        mHardwareRotator->getAlignedWidthAndHeight(layer, width, height, align);
}

uint32_t HardwareRotator::computeBufferSize(int width, int height, int align, int pixelformat) const
{
    if (mHardwareRotator)
        return mHardwareRotator->computeBufferSize(width, height, align, pixelformat);
    return 0;
}

void HardwareRotator::postRotateTask(const std::shared_ptr<RotateTask>& task)
{
    std::lock_guard<std::mutex> lock(mTaskLock);

    task->mSyncFencePt = (++mTimelineValue);
    int fencefd = fence_create(mSyncTimelineHandle.get(), "rotator.fence", task->mSyncFencePt);
    if (fencefd < 0) {
        DLOGE("fence create failed: %s", strerror(errno));
        fencefd = -1;
    }
    task->mAcquireFence = uniquefd(fencefd);
    task->mFrameData = mHardwareRotator->createFrame(
            task->mSourceLayer,
            task->mOutputBuffer.get());

    mPendingTasks.push(task);
    mCondition.notify_all();
}

void HardwareRotator::onTaskFinished(const std::shared_ptr<RotateTask>& task)
{
    std::lock_guard<std::mutex> lock(mTaskLock);
    ++mSignaledPtValue;
    if (mSignaledPtValue != task->mSyncFencePt) {
        DLOGW("fence timeline corruption, mTimelineValue=%d mSignaledPtValue=%d mSyncFencePt=%d",
                mTimelineValue, mSignaledPtValue, task->mSyncFencePt);
    }
    fence_timeline_inc(mSyncTimelineHandle.get(), 1);
}

void HardwareRotator::rotateLoop()
{
    DLOGI("HardwareRotator thread start.");
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY+2);

    while (true) {
        std::queue<std::shared_ptr<RotateTask>> pendingTasks;
        { // scope of mTaskLock
            std::unique_lock<std::mutex> lock(mTaskLock);
            if (mPendingTasks.empty()) {
                mCondition.wait(lock);
            }
            pendingTasks.swap(mPendingTasks);
        }

        while (!pendingTasks.empty()) {
            const int defaultFenceTimeOutMs = 3000;
            const std::shared_ptr<RotateTask>& task = pendingTasks.front();
            {
                DTRACE_SCOPED("wait-src-fence");
                uniquefd acquirefence(dup(task->mSourceLayer->acquireFence()));
                sync_wait(acquirefence.get(), defaultFenceTimeOutMs);
            }
            {
                DTRACE_SCOPED("wait-dst-fence");
                sync_wait(task->mOutputBuffer->acquireFence.get(), defaultFenceTimeOutMs);
            }

            int err = mHardwareRotator->rotateOneFrame(task->mFrameData.get());
            if (err != 0 && task->mFailedCallback) {
                task->mFailedCallback();
            }
            onTaskFinished(task);
            dumpRotatedBuffer(task.get());
            pendingTasks.pop();
        }
    }
}

void HardwareRotator::dumpRotatedBuffer(const RotateTask* task)
{
    if (CC_LIKELY(!DumpBufferRequest::get().requestDump(DumpBufferRequest::eHardwareRotateBuffer)))
        return;

    char path[128] = {0};
    sprintf(path, "/data/dump_%s_%dx%d_%u.dat",
            getHalPixelFormatString(task->mOutputBuffer->format),
            task->mOutputBuffer->width,
            task->mOutputBuffer->height,
            mTimelineValue);
    dumpBuffer(task->mOutputBuffer->buffer->handle,
            task->mOutputBuffer->width,
            task->mOutputBuffer->height,
            bitsPerPixel(task->mOutputBuffer->format),
            path);
}

void HardwareRotator::initSyncTimeline()
{
    int fd = fence_timeline_create();
    if (fd < 0) {
        DLOGE("sw sync timeline create failed: %s", strerror(errno));
        return;
    }
    mTimelineValue = 0;
    mSignaledPtValue = 0;
    mSyncTimelineHandle = uniquefd(fd);
}

int HardwareRotator::getPendingTaskCount() const
{
    std::unique_lock<std::mutex> lock(mTaskLock);
    return mPendingTasks.size();
}

} // namespace sunxi

