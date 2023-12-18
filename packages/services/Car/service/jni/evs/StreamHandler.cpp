/*
 * Copyright 2021 The Android Open Source Project
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

#include "StreamHandler.h"

#include <android-base/chrono_utils.h>
#include <android-base/logging.h>
#include <cutils/native_handle.h>

#include <cstring>

using namespace ::android::hardware::automotive::evs::V1_1;

using ::android::sp;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::automotive::evs::V1_0::EvsResult;
using ::android::hardware::automotive::evs::V1_1::IEvsDisplay;

using EvsDisplayState = ::android::hardware::automotive::evs::V1_0::DisplayState;
using BufferDesc_1_0 = ::android::hardware::automotive::evs::V1_0::BufferDesc;
using BufferDesc_1_1 = ::android::hardware::automotive::evs::V1_1::BufferDesc;

namespace android {
namespace automotive {
namespace evs {

StreamHandler::StreamHandler(sp<IEvsCamera>& camera, EvsServiceCallback* callback,
                             int maxNumFramesInFlight) :
      mEvsCamera(camera), mCallback(callback), mMaxNumFramesInFlight(maxNumFramesInFlight) {
    if (camera == nullptr) {
        LOG(ERROR) << "IEvsCamera is invalid.";
    } else {
        // We rely on the camera having at least two buffers available since we'll hold one and
        // expect the camera to be able to capture a new image in the background.
        auto status = camera->setMaxFramesInFlight(maxNumFramesInFlight);
        if (!status.isOk()) {
            LOG(WARNING) << "Failed to adjust the maximum number of frames in flight.";
        }
    }
}

/*
 * Shuts down a stream handler
 */
StreamHandler::~StreamHandler() {
    shutdown();
}

/*
 * Stops an active stream and releases the camera device in use
 */
void StreamHandler::shutdown() {
    // Make sure we're not still streaming
    blockingStopStream();

    // At this point, the receiver thread is no longer running, so we can safely drop
    // our remote object references so they can be freed
    mEvsCamera = nullptr;
}

/*
 * Requests EVS to start a video stream
 */
bool StreamHandler::startStream() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mRunning) {
        auto result = mEvsCamera->startVideoStream(this);
        if (!result.isOk() or result != EvsResult::OK) {
            LOG(ERROR) << "StreamHandler failed to start a video stream.";
            return false;
        }

        // Marks ourselves as running
        mRunning = true;
    }

    return true;
}

/*
 * Requests to stop a video stream
 */
bool StreamHandler::asyncStopStream() {
    bool success = true;

    // This will result in STREAM_STOPPED event; the client may want to wait
    // this event to confirm the closure.
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mReceivedBuffers.begin();
        while (it != mReceivedBuffers.end()) {
            // Packages a returned buffer and sends it back to the camera
            hidl_vec<BufferDesc_1_1> frames;
            frames.resize(1);
            frames[0] = *it;
            auto status = mEvsCamera->doneWithFrame_1_1(frames);
            if (!status.isOk()) {
                LOG(WARNING) << "Failed to return a frame to EVS service; "
                             << "this may leak the memory.";
                success = false;
            }

            it = mReceivedBuffers.erase(it);
        }
    }

    auto status = mEvsCamera->stopVideoStream();
    if (!status.isOk()) {
        LOG(WARNING) << "stopVideoStream() failed but ignored.";
        success = false;
    }

    return success;
}

/*
 * Requests to stop a video stream and waits for a confirmation
 */
void StreamHandler::blockingStopStream() {
    if (!asyncStopStream()) {
        // EVS service may die so no stream-stop event occurs.
        std::lock_guard<std::mutex> lock(mLock);
        mRunning = false;
        return;
    }

    // Waits until the stream has actually stopped
    std::unique_lock<std::mutex> lock(mLock);
    while (mRunning) {
        if (!mCondition.wait_for(lock, 1s, [this]() { return !mRunning; })) {
            LOG(WARNING) << "STREAM_STOPPED event timer expired.  EVS service may die.";
            break;
        }
    }
}

bool StreamHandler::isRunning() {
    std::lock_guard<std::mutex> lock(mLock);
    return mRunning;
}

void StreamHandler::doneWithFrame(const BufferDesc_1_1& buffer) {
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mReceivedBuffers.begin();
        while (it != mReceivedBuffers.end()) {
            if (it->bufferId == buffer.bufferId) {
                mReceivedBuffers.erase(it);
                break;
            }
        }
    }

    // Packages a returned buffer and sends it back to the camera
    hidl_vec<BufferDesc_1_1> frames;
    frames.resize(1);
    frames[0] = buffer;
    auto status = mEvsCamera->doneWithFrame_1_1(frames);
    if (!status.isOk()) {
        LOG(ERROR) << "Failed to return a frame to EVS service; this may leak the memory.";
    }
}

Return<void> StreamHandler::deliverFrame(const BufferDesc_1_0& buffer) {
    LOG(WARNING) << "Ignores a frame delivered from v1.0 EVS service.";
    auto status = mEvsCamera->doneWithFrame(buffer);
    if (!status.isOk()) {
        LOG(ERROR) << "Failed to return a frame to EVS service; this may leak the memory.";
    }
    return {};
}

Return<void> StreamHandler::deliverFrame_1_1(const hidl_vec<BufferDesc_1_1>& buffers) {
    LOG(DEBUG) << "Received frames from the camera, bufferId = " << buffers[0].bufferId;

    // Takes the lock to protect our frameDesc slots and running state variable
    BufferDesc_1_1 frameDesc = buffers[0];
    if (frameDesc.buffer.nativeHandle.getNativeHandle() == nullptr) {
        // Signals that the last frameDesc has been received and the stream is stopped
        LOG(WARNING) << "Invalid null frameDesc (id: 0x" << std::hex << frameDesc.bufferId
                     << ") is ignored";

        return {};
    }

    size_t numBuffersInUse;
    {
        std::lock_guard<std::mutex> lock(mLock);
        numBuffersInUse = mReceivedBuffers.size();
    }

    if (numBuffersInUse > mMaxNumFramesInFlight) {
        // We're holding more than what allowed; returns this buffer
        // immediately.
        doneWithFrame(frameDesc);
    } else {
        {
            std::lock_guard<std::mutex> lock(mLock);
            // Records a new frameDesc and forwards to clients
            mReceivedBuffers.emplace_back(frameDesc);
            LOG(DEBUG) << "Got buffer " << frameDesc.bufferId
                       << ", total = " << mReceivedBuffers.size();

            // Notify anybody who cares that things have changed
            mCondition.notify_all();
        }

        // Forwards a new frame
        mCallback->onNewFrame(frameDesc);
    }

    return {};
}

Return<void> StreamHandler::notify(const EvsEventDesc& event) {
    switch (event.aType) {
        case EvsEventType::STREAM_STOPPED: {
            {
                std::lock_guard<std::mutex> lock(mLock);
                // Signal that the last frame has been received and the stream is stopped
                mRunning = false;
            }
            LOG(DEBUG) << "Received a STREAM_STOPPED event";
            break;
        }
        case EvsEventType::PARAMETER_CHANGED:
            LOG(DEBUG) << "Camera parameter 0x" << std::hex << event.payload[0] << " is set to 0x"
                       << std::hex << event.payload[1];
            break;
        // Below events are ignored in reference implementation.
        case EvsEventType::STREAM_STARTED:
            [[fallthrough]];
        case EvsEventType::FRAME_DROPPED:
            [[fallthrough]];
        case EvsEventType::TIMEOUT:
            LOG(INFO) << "Event 0x" << std::hex << static_cast<int32_t>(event.aType)
                      << " is received but ignored";
            break;
        default:
            LOG(ERROR) << "Unknown event id 0x" << std::hex << static_cast<int32_t>(event.aType);
            break;
    }

    mCallback->onNewEvent(event);

    return {};
}

}  // namespace evs
}  // namespace automotive
}  // namespace android
