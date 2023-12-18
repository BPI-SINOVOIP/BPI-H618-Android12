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

#include "CarPowerPolicyServer.h"

#include <gmock/gmock.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

namespace android {
namespace frameworks {
namespace automotive {
namespace powerpolicy {

using binder::Status;
using ::testing::_;
using ::testing::Return;

namespace {

class MockBinder : public BBinder {
public:
    MOCK_METHOD(status_t, linkToDeath,
                (const sp<DeathRecipient>& recipient, void* cookie, uint32_t flags), (override));
    MOCK_METHOD(status_t, unlinkToDeath,
                (const wp<DeathRecipient>& recipient, void* cookie, uint32_t flags,
                 wp<DeathRecipient>* outRecipient),
                (override));
};

class MockPowerPolicyChangeCallback : public ICarPowerPolicyChangeCallbackDefault {
public:
    MockPowerPolicyChangeCallback() { mBinder = new MockBinder(); }

    MOCK_METHOD(IBinder*, onAsBinder, (), (override));

    void expectLinkToDeathStatus(status_t linkToDeathResult) {
        EXPECT_CALL(*mBinder, linkToDeath(_, nullptr, 0)).WillRepeatedly(Return(linkToDeathResult));
        EXPECT_CALL(*mBinder, unlinkToDeath(_, nullptr, 0, nullptr)).WillRepeatedly(Return(OK));
        EXPECT_CALL(*this, onAsBinder()).WillRepeatedly(Return(mBinder.get()));
    }

private:
    sp<MockBinder> mBinder;
};

}  // namespace

namespace internal {

class CarPowerPolicyServerPeer : public RefBase {
public:
    CarPowerPolicyServerPeer() { server = new CarPowerPolicyServer(); }

    Status getCurrentPowerPolicy(CarPowerPolicy* aidlReturn) {
        return server->getCurrentPowerPolicy(aidlReturn);
    }
    Status registerPowerPolicyChangeCallback(const sp<ICarPowerPolicyChangeCallback>& callback,
                                             const CarPowerPolicyFilter& filter) {
        return server->registerPowerPolicyChangeCallback(callback, filter);
    }
    Status unregisterPowerPolicyChangeCallback(const sp<ICarPowerPolicyChangeCallback>& callback) {
        return server->unregisterPowerPolicyChangeCallback(callback);
    }

    sp<CarPowerPolicyServer> server;
};

}  // namespace internal

class CarPowerPolicyServerTest : public ::testing::Test {};

TEST_F(CarPowerPolicyServerTest, TestRegisterCallback) {
    sp<internal::CarPowerPolicyServerPeer> server = new internal::CarPowerPolicyServerPeer();
    sp<MockPowerPolicyChangeCallback> callbackOne = new MockPowerPolicyChangeCallback();
    callbackOne->expectLinkToDeathStatus(OK);

    CarPowerPolicyFilter filter;
    Status status = server->registerPowerPolicyChangeCallback(callbackOne, filter);
    ASSERT_TRUE(status.isOk()) << status;
    status = server->registerPowerPolicyChangeCallback(callbackOne, filter);
    ASSERT_FALSE(status.isOk()) << "Duplicated registration is not allowed";
    filter.components = {PowerComponent::BLUETOOTH, PowerComponent::AUDIO};
    status = server->registerPowerPolicyChangeCallback(callbackOne, filter);
    ASSERT_FALSE(status.isOk()) << "Duplicated registration is not allowed";

    sp<MockPowerPolicyChangeCallback> callbackTwo = new MockPowerPolicyChangeCallback();
    callbackTwo->expectLinkToDeathStatus(OK);

    status = server->registerPowerPolicyChangeCallback(callbackTwo, filter);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(CarPowerPolicyServerTest, TestRegisterCallback_BinderDied) {
    sp<internal::CarPowerPolicyServerPeer> server = new internal::CarPowerPolicyServerPeer();
    sp<MockPowerPolicyChangeCallback> callback = new MockPowerPolicyChangeCallback();
    callback->expectLinkToDeathStatus(DEAD_OBJECT);
    CarPowerPolicyFilter filter;

    ASSERT_FALSE(server->registerPowerPolicyChangeCallback(callback, filter).isOk())
            << "When linkToDeath fails, registerPowerPolicyChangeCallback should return an error";
}

TEST_F(CarPowerPolicyServerTest, TestUnregisterCallback) {
    sp<internal::CarPowerPolicyServerPeer> server = new internal::CarPowerPolicyServerPeer();
    sp<MockPowerPolicyChangeCallback> callback = new MockPowerPolicyChangeCallback();
    callback->expectLinkToDeathStatus(OK);
    CarPowerPolicyFilter filter;

    server->registerPowerPolicyChangeCallback(callback, filter);
    Status status = server->unregisterPowerPolicyChangeCallback(callback);
    ASSERT_TRUE(status.isOk()) << status;
    ASSERT_FALSE(server->unregisterPowerPolicyChangeCallback(callback).isOk())
            << "Unregistering an unregistered powerpolicy change callback should return an error";
}

TEST_F(CarPowerPolicyServerTest, TestGetCurrentPowerPolicy) {
    sp<internal::CarPowerPolicyServerPeer> server = new internal::CarPowerPolicyServerPeer();
    CarPowerPolicy currentPolicy;

    Status status = server->getCurrentPowerPolicy(&currentPolicy);
    ASSERT_FALSE(status.isOk()) << "The current policy at creation should be null";
    // TODO(b/168545262): Add more test cases after VHAL integration is complete.
}

}  // namespace powerpolicy
}  // namespace automotive
}  // namespace frameworks
}  // namespace android
