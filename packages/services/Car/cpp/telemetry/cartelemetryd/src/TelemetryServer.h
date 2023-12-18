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

#ifndef CPP_TELEMETRY_CARTELEMETRYD_SRC_TELEMETRYSERVER_H_
#define CPP_TELEMETRY_CARTELEMETRYD_SRC_TELEMETRYSERVER_H_

#include "LooperWrapper.h"
#include "RingBuffer.h"

#include <aidl/android/automotive/telemetry/internal/ICarDataListener.h>
#include <aidl/android/frameworks/automotive/telemetry/CarData.h>
#include <android-base/chrono_utils.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>
#include <gtest/gtest_prod.h>
#include <utils/Looper.h>

#include <memory>

namespace android {
namespace automotive {
namespace telemetry {

// This class contains the main logic of cartelemetryd native service.
//
//   [writer clients] -> ICarTelemetry  -----------.
//   [reader client] --> ICarTelemetryInternal -----`-> TelemetryServer
//
// TelemetryServer starts pushing CarData to ICarDataListener when there is a data available and
// the listener is set and alive. It uses `mLooper` for periodically pushing the data.
//
// This class is thread-safe.
class TelemetryServer {
public:
    explicit TelemetryServer(LooperWrapper* looper,
                             const std::chrono::nanoseconds& pushCarDataDelayNs, int maxBufferSize);

    // Dumps the current state for dumpsys.
    // Expected to be called from a binder thread pool.
    void dump(int fd);

    // Writes incoming CarData to the RingBuffer.
    // Expected to be called from a binder thread pool.
    void writeCarData(
            const std::vector<aidl::android::frameworks::automotive::telemetry::CarData>& dataList,
            uid_t publisherUid);

    // Sets the listener. If the listener already set, it returns an error.
    // Expected to be called from a binder thread pool.
    android::base::Result<void> setListener(
            const std::shared_ptr<aidl::android::automotive::telemetry::internal::ICarDataListener>&
                    listener);

    // Clears the listener and returns it.
    // Expected to be called from a binder thread pool.
    void clearListener();

    // Expected to be called from a binder thread pool.
    std::shared_ptr<aidl::android::automotive::telemetry::internal::ICarDataListener> getListener();

private:
    class MessageHandlerImpl : public MessageHandler {
    public:
        explicit MessageHandlerImpl(TelemetryServer* server);

        void handleMessage(const Message& message) override;

    private:
        TelemetryServer* mTelemetryServer;  // not owned
    };

private:
    // Periodically called by mLooper if there is a "push car data" messages.
    void pushCarDataToListeners();

    LooperWrapper* mLooper;  // not owned
    const std::chrono::nanoseconds mPushCarDataDelayNs;

    // A single mutex for all the sensitive operations. Threads must not lock it for long time,
    // as clients will be writing CarData to the ring buffer under this mutex.
    std::mutex mMutex;
    RingBuffer mRingBuffer GUARDED_BY(mMutex);
    std::shared_ptr<aidl::android::automotive::telemetry::internal::ICarDataListener>
            mCarDataListener GUARDED_BY(mMutex);
    android::sp<MessageHandlerImpl> mMessageHandler;  // Handler for mLooper.

    // Friends are simplest way of testing if `pushCarDataToListeners()` can handle edge cases.
    friend class TelemetryServerTest;
    FRIEND_TEST(TelemetryServerTest, NoListenerButMultiplePushes);
    FRIEND_TEST(TelemetryServerTest, NoDataButMultiplePushes);
};

}  // namespace telemetry
}  // namespace automotive
}  // namespace android

#endif  // CPP_TELEMETRY_CARTELEMETRYD_SRC_TELEMETRYSERVER_H_
