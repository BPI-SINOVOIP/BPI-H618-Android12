/*
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TelemetryServer.h"

#include "CarTelemetryImpl.h"
#include "RingBuffer.h"

#include <aidl/android/automotive/telemetry/internal/CarDataInternal.h>
#include <android-base/logging.h>

#include <inttypes.h>  // for PRIu64 and friends

#include <memory>

namespace android {
namespace automotive {
namespace telemetry {

namespace {

using ::aidl::android::automotive::telemetry::internal::CarDataInternal;
using ::aidl::android::automotive::telemetry::internal::ICarDataListener;
using ::aidl::android::frameworks::automotive::telemetry::CarData;
using ::android::base::Error;
using ::android::base::Result;

enum {
    MSG_PUSH_CAR_DATA_TO_LISTENER = 1,
};

// If ICarDataListener cannot accept data, the next push should be delayed little bit to allow
// the listener to recover.
constexpr const std::chrono::seconds kPushCarDataFailureDelaySeconds = 1s;
}  // namespace

TelemetryServer::TelemetryServer(LooperWrapper* looper,
                                 const std::chrono::nanoseconds& pushCarDataDelayNs,
                                 const int maxBufferSize) :
      mLooper(looper),
      mPushCarDataDelayNs(pushCarDataDelayNs),
      mRingBuffer(maxBufferSize),
      mMessageHandler(new MessageHandlerImpl(this)) {}

Result<void> TelemetryServer::setListener(const std::shared_ptr<ICarDataListener>& listener) {
    const std::scoped_lock<std::mutex> lock(mMutex);
    if (mCarDataListener != nullptr) {
        return Error(::EX_ILLEGAL_STATE) << "ICarDataListener is already set";
    }
    mCarDataListener = listener;
    mLooper->sendMessageDelayed(mPushCarDataDelayNs.count(), mMessageHandler,
                                MSG_PUSH_CAR_DATA_TO_LISTENER);
    return {};
}

void TelemetryServer::clearListener() {
    const std::scoped_lock<std::mutex> lock(mMutex);
    if (mCarDataListener == nullptr) {
        return;
    }
    mCarDataListener = nullptr;
    mLooper->removeMessages(mMessageHandler, MSG_PUSH_CAR_DATA_TO_LISTENER);
}

std::shared_ptr<ICarDataListener> TelemetryServer::getListener() {
    const std::scoped_lock<std::mutex> lock(mMutex);
    return mCarDataListener;
}

void TelemetryServer::dump(int fd) {
    const std::scoped_lock<std::mutex> lock(mMutex);
    dprintf(fd, "  TelemetryServer:\n");
    mRingBuffer.dump(fd);
}

// TODO(b/174608802): Add 10kb size check for the `dataList`, see the AIDL for the limits
void TelemetryServer::writeCarData(const std::vector<CarData>& dataList, uid_t publisherUid) {
    const std::scoped_lock<std::mutex> lock(mMutex);
    bool bufferWasEmptyBefore = mRingBuffer.size() == 0;
    for (auto&& data : dataList) {
        mRingBuffer.push({.mId = data.id,
                          .mContent = std::move(data.content),
                          .mPublisherUid = publisherUid});
    }
    // If the mRingBuffer was not empty, the message is already scheduled. It prevents scheduling
    // too many unnecessary idendical messages in the looper.
    if (mCarDataListener != nullptr && bufferWasEmptyBefore && mRingBuffer.size() > 0) {
        mLooper->sendMessageDelayed(mPushCarDataDelayNs.count(), mMessageHandler,
                                    MSG_PUSH_CAR_DATA_TO_LISTENER);
    }
}

// Runs on the main thread.
void TelemetryServer::pushCarDataToListeners() {
    std::shared_ptr<ICarDataListener> listener;
    std::vector<CarDataInternal> pendingCarDataInternals;
    {
        const std::scoped_lock<std::mutex> lock(mMutex);
        // Remove extra messages.
        mLooper->removeMessages(mMessageHandler, MSG_PUSH_CAR_DATA_TO_LISTENER);
        if (mCarDataListener == nullptr || mRingBuffer.size() == 0) {
            return;
        }
        listener = mCarDataListener;
        // Push elements to pendingCarDataInternals in reverse order so we can send data
        // from the back of the pendingCarDataInternals vector.
        while (mRingBuffer.size() > 0) {
            auto carData = std::move(mRingBuffer.popBack());
            CarDataInternal data;
            data.id = carData.mId;
            data.content = std::move(carData.mContent);
            pendingCarDataInternals.push_back(data);
        }
    }

    // Now the mutex is unlocked, we can do the heavy work.

    // TODO(b/186477983): send data in batch to improve performance, but careful sending too
    //                    many data at once, as it could clog the Binder - it has <1MB limit.
    while (!pendingCarDataInternals.empty()) {
        auto status = listener->onCarDataReceived({pendingCarDataInternals.back()});
        if (!status.isOk()) {
            LOG(WARNING) << "Failed to push CarDataInternal, will try again: "
                         << status.getMessage();
            sleep(kPushCarDataFailureDelaySeconds.count());
        } else {
            pendingCarDataInternals.pop_back();
        }
    }
}

TelemetryServer::MessageHandlerImpl::MessageHandlerImpl(TelemetryServer* server) :
      mTelemetryServer(server) {}

void TelemetryServer::MessageHandlerImpl::handleMessage(const Message& message) {
    switch (message.what) {
        case MSG_PUSH_CAR_DATA_TO_LISTENER:
            mTelemetryServer->pushCarDataToListeners();
            break;
        default:
            LOG(WARNING) << "Unknown message: " << message.what;
    }
}

}  // namespace telemetry
}  // namespace automotive
}  // namespace android
