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

#ifndef CPP_WATCHDOG_SERVER_TESTS_MOCKWATCHDOGSERVICEHELPER_H_
#define CPP_WATCHDOG_SERVER_TESTS_MOCKWATCHDOGSERVICEHELPER_H_

#include "WatchdogProcessService.h"
#include "WatchdogServiceHelper.h"

#include <android-base/result.h>
#include <android/automotive/watchdog/TimeoutLength.h>
#include <android/automotive/watchdog/internal/ICarWatchdogServiceForSystem.h>
#include <android/automotive/watchdog/internal/PackageIoOveruseStats.h>
#include <binder/Status.h>
#include <gmock/gmock.h>
#include <utils/StrongPointer.h>

namespace android {
namespace automotive {
namespace watchdog {

class MockWatchdogServiceHelper : public IWatchdogServiceHelper {
public:
    MockWatchdogServiceHelper() {}
    ~MockWatchdogServiceHelper() {}

    MOCK_METHOD(android::base::Result<void>, init, (const android::sp<WatchdogProcessService>&),
                (override));
    MOCK_METHOD(android::binder::Status, registerService,
                (const sp<android::automotive::watchdog::internal::ICarWatchdogServiceForSystem>&),
                (override));
    MOCK_METHOD(android::binder::Status, unregisterService,
                (const sp<android::automotive::watchdog::internal::ICarWatchdogServiceForSystem>&),
                (override));
    MOCK_METHOD(void, binderDied, (const android::wp<android::IBinder>&), (override));

    MOCK_METHOD(android::binder::Status, checkIfAlive,
                (const android::wp<android::IBinder>&, int32_t, TimeoutLength), (const, override));
    MOCK_METHOD(android::binder::Status, prepareProcessTermination,
                (const android::wp<android::IBinder>&), (override));
    MOCK_METHOD(android::binder::Status, getPackageInfosForUids,
                (const std::vector<int32_t>&, const std::vector<std::string>&,
                 std::vector<android::automotive::watchdog::internal::PackageInfo>*),
                (override));
    MOCK_METHOD(
            android::binder::Status, latestIoOveruseStats,
            (const std::vector<android::automotive::watchdog::internal::PackageIoOveruseStats>&),
            (override));
    MOCK_METHOD(android::binder::Status, resetResourceOveruseStats,
                (const std::vector<std::string>&), (override));
    MOCK_METHOD(void, terminate, (), (override));
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_TESTS_MOCKWATCHDOGSERVICEHELPER_H_
