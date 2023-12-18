/*
 * Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include <aidl/android/automotive/watchdog/BnCarWatchdog.h>
#include <aidl/android/automotive/watchdog/BnCarWatchdogClient.h>
#include <utils/Looper.h>
#include <utils/Mutex.h>

namespace android::hardware::automotive::utils {

class BaseWatchdogClient : public aidl::android::automotive::watchdog::BnCarWatchdogClient {
  public:
    ndk::ScopedAStatus checkIfAlive(
            int32_t sessionId, aidl::android::automotive::watchdog::TimeoutLength timeout) override;
    ndk::ScopedAStatus prepareProcessTermination() override;

    bool initialize();

    virtual ~BaseWatchdogClient() = default;

  protected:
    explicit BaseWatchdogClient(const ::android::sp<::android::Looper>& handlerLooper);
    virtual bool isClientHealthy() const = 0;

  private:
    class MessageHandlerImpl : public ::android::MessageHandler {
      public:
        explicit MessageHandlerImpl(BaseWatchdogClient* client);
        void handleMessage(const ::android::Message& message) override;

      private:
        BaseWatchdogClient* mClient;
    };

  private:
    void respondToWatchdog();

  private:
    ::android::sp<::android::Looper> mHandlerLooper;
    ::android::sp<MessageHandlerImpl> mMessageHandler;
    std::shared_ptr<aidl::android::automotive::watchdog::ICarWatchdog> mWatchdogServer;
    std::shared_ptr<aidl::android::automotive::watchdog::ICarWatchdogClient> mTestClient;
    ::android::Mutex mMutex;
    int mCurrentSessionId GUARDED_BY(mMutex);
};

}  // namespace android::hardware::automotive::utils
