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

#ifndef CPP_WATCHDOG_SERVER_TESTS_MOCKBINDER_H_
#define CPP_WATCHDOG_SERVER_TESTS_MOCKBINDER_H_

#include <binder/IBinder.h>
#include <binder/Status.h>
#include <gmock/gmock.h>
#include <utils/StrongPointer.h>

namespace android {
namespace automotive {
namespace watchdog {

class MockBinder : public android::BBinder {
public:
    MockBinder() {
        EXPECT_CALL(*this, linkToDeath(::testing::_, nullptr, 0))
                .WillRepeatedly(::testing::Return(OK));
        EXPECT_CALL(*this, unlinkToDeath(::testing::_, nullptr, 0, nullptr))
                .WillRepeatedly(::testing::Return(OK));
    }
    MOCK_METHOD(android::status_t, linkToDeath,
                (const android::sp<android::IBinder::DeathRecipient>&, void*, uint32_t),
                (override));
    MOCK_METHOD(android::status_t, unlinkToDeath,
                (const android::wp<android::IBinder::DeathRecipient>&, void*, uint32_t,
                 android::wp<android::IBinder::DeathRecipient>*),
                (override));
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_TESTS_MOCKBINDER_H_
