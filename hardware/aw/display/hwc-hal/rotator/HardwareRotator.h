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

#ifndef SUNXI_HARDWARE_ROTATOR_H
#define SUNXI_HARDWARE_ROTATOR_H

#include <memory>
#include <queue>
#include <thread>
#include <hardware/hwcomposer_defs.h>
#include "uniquefd.h"
#include "IonBuffer.h"
#include "RotatorDevice.h"

namespace sunxi {

class Layer;
class HardwareRotator;

typedef struct OutputBuffer {
    const PrivateBuffer_t* buffer;
    uniquefd acquireFence;
    int width;
    int height;
    int align;
    int format;
} OutputBuffer_t;


using RotateFailedCallback = std::function<void()>;

// Rotate task
class RotateTask {
public:
    RotateTask(const std::shared_ptr<Layer>& layer,
            std::unique_ptr<OutputBuffer_t> outBuf, hwc_transform_t transform);

    int getRotatedBuffer(const OutputBuffer_t** buffer, int* acquireFence);
    const PrivateBuffer_t* outputBuffer() const { return mOutputBuffer->buffer; };
    void setReleaseFence(int fd) { mReleaseFence = uniquefd(fd); }
    int releaseFence() const { return mReleaseFence.get(); }
    int taskId() const { return mTaskId; }
    void setRotateFailedCallback(RotateFailedCallback cb) { mFailedCallback = cb; }

    friend class HardwareRotator;
private:
    static std::atomic<int> mNextTaskId;

    const int mTaskId;
    const std::shared_ptr<Layer> mSourceLayer;
    std::unique_ptr<OutputBuffer_t> mOutputBuffer;
    hwc_transform_t mTransform;
    RotateFailedCallback mFailedCallback;

    // Setup the rotate layer metadata on the validate/present thread,
    // so that we can make everything correctly before mSourceLayer/mOutputBuffer has been deleted.
    std::unique_ptr<RotatorDevice::Frame> mFrameData;

    // mOutputBuffer can't be accessed until the acquireFence has been signal
    uniquefd mAcquireFence;
    uniquefd mReleaseFence;

    unsigned int mSyncFencePt = 0;
};

class HardwareRotator {
public:
    static HardwareRotator& getInstance() {
        static HardwareRotator instance_;
        return instance_;
    }
    // Rotate capability detect, Check if the layer supports rotation
    bool capable(const std::shared_ptr<Layer>& layer);
    void postRotateTask(const std::shared_ptr<RotateTask>& task);
    void getAlignedWidthAndHeight(const std::shared_ptr<Layer>& layer,
            int* width, int* height, int* align);
    uint32_t computeBufferSize(int width, int height, int align, int pixelformat) const;
    int getPendingTaskCount() const;

private:
    HardwareRotator();
   ~HardwareRotator();

    void initSyncTimeline();
    void onTaskFinished(const std::shared_ptr<RotateTask>& task);
    void rotateLoop();
    void dumpRotatedBuffer(const RotateTask* task);

    // If there are more than kPendingTaskThrottle tasks in the queue,
    // consider using GPU composition.
    const int kPendingTaskThrottle = 16;
    mutable std::mutex mTaskLock;
    std::queue<std::shared_ptr<RotateTask>> mPendingTasks;
    std::unique_ptr<RotatorDevice> mHardwareRotator;

    bool mThreadStop;
    std::condition_variable mCondition;
    std::thread mRotateThread;

    uniquefd mSyncTimelineHandle;
    unsigned int mTimelineValue;
    unsigned int mSignaledPtValue;
};

} // namespace sunxi
#endif
