/**
 * Copyright (c) 2020, The Android Open Source Project
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

#ifndef CPP_WATCHDOG_SERVER_TESTS_MOCKWATCHDOGPROCESSSERVICE_H_
#define CPP_WATCHDOG_SERVER_TESTS_MOCKWATCHDOGPROCESSSERVICE_H_

#include "WatchdogProcessService.h"
#include "WatchdogServiceHelper.h"

#include <android-base/result.h>
#include <android/automotive/watchdog/ICarWatchdogClient.h>
#include <android/automotive/watchdog/internal/ICarWatchdogMonitor.h>
#include <android/automotive/watchdog/internal/ICarWatchdogServiceForSystem.h>
#include <android/automotive/watchdog/internal/PowerCycle.h>
#include <android/automotive/watchdog/internal/UserState.h>
#include <binder/Status.h>
#include <gmock/gmock.h>
#include <utils/String16.h>
#include <utils/Vector.h>

#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

class MockWatchdogProcessService : public WatchdogProcessService {
public:
    MockWatchdogProcessService() : WatchdogProcessService(nullptr) {}
    MOCK_METHOD(android::base::Result<void>, dump, (int fd, const Vector<android::String16>& args),
                (override));
    MOCK_METHOD(android::base::Result<void>, registerWatchdogServiceHelper,
                (const android::sp<IWatchdogServiceHelper>& helper), (override));

    MOCK_METHOD(android::binder::Status, registerClient,
                (const sp<ICarWatchdogClient>& client, TimeoutLength timeout), (override));
    MOCK_METHOD(android::binder::Status, unregisterClient, (const sp<ICarWatchdogClient>& client),
                (override));
    MOCK_METHOD(android::binder::Status, registerCarWatchdogService,
                (const android::sp<IBinder>& binder), (override));
    MOCK_METHOD(void, unregisterCarWatchdogService, (const android::sp<IBinder>& binder),
                (override));
    MOCK_METHOD(android::binder::Status, registerMonitor,
                (const sp<android::automotive::watchdog::internal::ICarWatchdogMonitor>& monitor),
                (override));
    MOCK_METHOD(android::binder::Status, unregisterMonitor,
                (const sp<android::automotive::watchdog::internal::ICarWatchdogMonitor>& monitor),
                (override));
    MOCK_METHOD(android::binder::Status, tellClientAlive,
                (const sp<ICarWatchdogClient>& client, int32_t sessionId), (override));
    MOCK_METHOD(android::binder::Status, tellCarWatchdogServiceAlive,
                (const android::sp<
                         android::automotive::watchdog::internal::ICarWatchdogServiceForSystem>&
                         service,
                 const std::vector<int32_t>& clientsNotResponding, int32_t sessionId),
                (override));
    MOCK_METHOD(android::binder::Status, tellDumpFinished,
                (const android::sp<android::automotive::watchdog::internal::ICarWatchdogMonitor>&
                         monitor,
                 int32_t pid),
                (override));
    MOCK_METHOD(void, setEnabled, (bool), (override));
    MOCK_METHOD(android::binder::Status, notifyUserStateChange,
                (userid_t userId, android::automotive::watchdog::internal::UserState state),
                (override));
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_TESTS_MOCKWATCHDOGPROCESSSERVICE_H_
