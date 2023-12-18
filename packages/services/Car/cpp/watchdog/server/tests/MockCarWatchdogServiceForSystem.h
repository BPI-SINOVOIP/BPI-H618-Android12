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

#ifndef CPP_WATCHDOG_SERVER_TESTS_MOCKCARWATCHDOGSERVICEFORSYSTEM_H_
#define CPP_WATCHDOG_SERVER_TESTS_MOCKCARWATCHDOGSERVICEFORSYSTEM_H_

#include "MockBinder.h"

#include <android/automotive/watchdog/internal/ICarWatchdogServiceForSystem.h>
#include <android/automotive/watchdog/internal/TimeoutLength.h>
#include <binder/Status.h>
#include <gmock/gmock.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

namespace android {
namespace automotive {
namespace watchdog {

class MockCarWatchdogServiceForSystem :
      public android::automotive::watchdog::internal::ICarWatchdogServiceForSystemDefault {
public:
    MockCarWatchdogServiceForSystem() : mBinder(new MockBinder()) {
        ON_CALL(*this, onAsBinder()).WillByDefault(::testing::Return(mBinder.get()));
    }

    android::sp<MockBinder> getBinder() const { return mBinder; }

    MOCK_METHOD(android::IBinder*, onAsBinder, (), (override));
    MOCK_METHOD(android::binder::Status, checkIfAlive,
                (int32_t, android::automotive::watchdog::internal::TimeoutLength), (override));
    MOCK_METHOD(android::binder::Status, prepareProcessTermination, (), (override));
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

private:
    android::sp<MockBinder> mBinder;
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_TESTS_MOCKCARWATCHDOGSERVICEFORSYSTEM_H_
