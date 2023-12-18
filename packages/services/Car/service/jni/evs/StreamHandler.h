/*
 * Copyright (C) 2021 The Android Open Source Project
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
#ifndef ANDROID_CARSERVICE_STREAMHANDLER_H
#define ANDROID_CARSERVICE_STREAMHANDLER_H

#include "EvsServiceCallback.h"

#include <android/hardware/automotive/evs/1.1/IEvsCamera.h>
#include <android/hardware/automotive/evs/1.1/IEvsCameraStream.h>
#include <android/hardware/automotive/evs/1.1/IEvsDisplay.h>
#include <ui/GraphicBuffer.h>

#include <list>

namespace android {
namespace automotive {
namespace evs {

/*
 * StreamHandler:
 * This class can be used to receive camera imagery from an IEvsCamera implementation.  It will
 * hold onto the most recent image buffer, returning older ones.
 * Note that the video frames are delivered on a background thread, while the control interface
 * is actuated from the applications foreground thread.
 */
class StreamHandler : public android::hardware::automotive::evs::V1_1::IEvsCameraStream {
public:
    StreamHandler(android::sp<android::hardware::automotive::evs::V1_1::IEvsCamera>& pCamera,
                  EvsServiceCallback* callback, int maxNumFramesInFlight);
    virtual ~StreamHandler();
    void shutdown();
    bool startStream();
    bool asyncStopStream();
    void blockingStopStream();
    bool isRunning();
    void doneWithFrame(const android::hardware::automotive::evs::V1_1::BufferDesc& buffer);

private:
    // Implementation for ::android::hardware::automotive::evs::V1_0::IEvsCameraStream
    android::hardware::Return<void> deliverFrame(
            const android::hardware::automotive::evs::V1_0::BufferDesc& buffer) override;

    // Implementation for ::android::hardware::automotive::evs::V1_1::IEvsCameraStream
    android::hardware::Return<void> deliverFrame_1_1(
            const android::hardware::hidl_vec<android::hardware::automotive::evs::V1_1::BufferDesc>&
                    buffer) override;
    android::hardware::Return<void> notify(
            const android::hardware::automotive::evs::V1_1::EvsEventDesc& event) override;

    // Values initialized as startup
    android::sp<android::hardware::automotive::evs::V1_1::IEvsCamera> mEvsCamera;

    // Since we get frames delivered to us asnchronously via the ICarCameraStream interface,
    // we need to protect all member variables that may be modified while we're streaming
    // (ie: those below)
    std::mutex mLock;
    std::condition_variable mCondition;
    bool mRunning = false;

    // Callbacks to forward EVS events and frames
    EvsServiceCallback* mCallback;

    std::list<android::hardware::automotive::evs::V1_1::BufferDesc> mReceivedBuffers
            GUARDED_BY(mLock);
    int mMaxNumFramesInFlight;
};

}  // namespace evs
}  // namespace automotive
}  // namespace android

#endif  // ANDROID_CARSERVICE_STREAMHANDLER_H
