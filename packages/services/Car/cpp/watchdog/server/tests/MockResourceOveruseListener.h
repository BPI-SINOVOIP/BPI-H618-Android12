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

#ifndef CPP_WATCHDOG_SERVER_TESTS_MOCKRESOURCEOVERUSELISTENER_H_
#define CPP_WATCHDOG_SERVER_TESTS_MOCKRESOURCEOVERUSELISTENER_H_

#include "MockBinder.h"

#include <android/automotive/watchdog/BnResourceOveruseListener.h>
#include <android/automotive/watchdog/ResourceOveruseStats.h>
#include <binder/Status.h>
#include <gmock/gmock.h>

namespace android {
namespace automotive {
namespace watchdog {

class MockResourceOveruseListener : public IResourceOveruseListenerDefault {
public:
    MockResourceOveruseListener() : mMockBinder(new MockBinder()) {
        ON_CALL(*this, onAsBinder()).WillByDefault(::testing::Return(mMockBinder.get()));
    }
    ~MockResourceOveruseListener() { mMockBinder.clear(); }

    MOCK_METHOD(android::IBinder*, onAsBinder, (), (override));
    MOCK_METHOD(android::binder::Status, onOveruse, (const ResourceOveruseStats&), (override));

    void injectLinkToDeathFailure() {
        EXPECT_CALL(*mMockBinder, linkToDeath(::testing::_, nullptr, 0))
                .WillOnce(::testing::Return(android::binder::Status::EX_ILLEGAL_STATE));
    }
    void injectUnlinkToDeathFailure() {
        EXPECT_CALL(*mMockBinder, unlinkToDeath(::testing::_, nullptr, 0, nullptr))
                .WillOnce(::testing::Return(android::binder::Status::EX_ILLEGAL_STATE));
    }

private:
    android::sp<MockBinder> mMockBinder;
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_TESTS_MOCKRESOURCEOVERUSELISTENER_H_
