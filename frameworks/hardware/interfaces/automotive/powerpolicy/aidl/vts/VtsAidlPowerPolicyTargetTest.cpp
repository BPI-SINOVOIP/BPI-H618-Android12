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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android/frameworks/automotive/powerpolicy/CarPowerPolicy.h>
#include <android/frameworks/automotive/powerpolicy/CarPowerPolicyFilter.h>
#include <android/frameworks/automotive/powerpolicy/ICarPowerPolicyChangeCallback.h>
#include <android/frameworks/automotive/powerpolicy/ICarPowerPolicyServer.h>
#include <android/frameworks/automotive/powerpolicy/PowerComponent.h>
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <utils/StrongPointer.h>

namespace {

using ::android::BBinder;
using ::android::IBinder;
using ::android::OK;
using ::android::ProcessState;
using ::android::sp;
using ::android::status_t;
using ::android::String16;
using ::android::wp;
using ::android::binder::Status;
using ::android::frameworks::automotive::powerpolicy::CarPowerPolicy;
using ::android::frameworks::automotive::powerpolicy::CarPowerPolicyFilter;
using ::android::frameworks::automotive::powerpolicy::ICarPowerPolicyChangeCallbackDefault;
using ::android::frameworks::automotive::powerpolicy::ICarPowerPolicyServer;
using ::android::frameworks::automotive::powerpolicy::PowerComponent;
using ::testing::_;
using ::testing::Return;

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

    void expectNormalBinder() {
        EXPECT_CALL(*mBinder, linkToDeath(_, nullptr, 0)).WillRepeatedly(Return(OK));
        EXPECT_CALL(*mBinder, unlinkToDeath(_, nullptr, 0, nullptr)).WillRepeatedly(Return(OK));
        EXPECT_CALL(*this, onAsBinder()).WillRepeatedly(Return(mBinder.get()));
    }

   private:
    sp<MockBinder> mBinder;
};

}  // namespace

class PowerPolicyAidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        powerPolicyServer =
            android::waitForDeclaredService<ICarPowerPolicyServer>(String16(GetParam().c_str()));
        ASSERT_NE(powerPolicyServer.get(), nullptr);
    }

    sp<ICarPowerPolicyServer> powerPolicyServer;
};

TEST_P(PowerPolicyAidlTest, TestGetCurrentPowerPolicy) {
    CarPowerPolicy policy;

    Status status = powerPolicyServer->getCurrentPowerPolicy(&policy);

    ASSERT_TRUE(status.isOk() || status.exceptionCode() == Status::EX_ILLEGAL_STATE);
}

TEST_P(PowerPolicyAidlTest, TestGetPowerComponentState) {
    bool state;
    for (const auto componentId : android::enum_range<PowerComponent>()) {
        Status status = powerPolicyServer->getPowerComponentState(componentId, &state);

        ASSERT_TRUE(status.isOk());
    }
}

TEST_P(PowerPolicyAidlTest, TestGetPowerComponentState_invalidComponent) {
    bool state;
    PowerComponent invalidComponent = (PowerComponent)-1;

    Status status = powerPolicyServer->getPowerComponentState(invalidComponent, &state);

    ASSERT_FALSE(status.isOk());
}

TEST_P(PowerPolicyAidlTest, TestRegisterCallback) {
    sp<MockPowerPolicyChangeCallback> callback = new MockPowerPolicyChangeCallback();
    callback->expectNormalBinder();
    CarPowerPolicyFilter filter;
    filter.components.push_back(PowerComponent::AUDIO);

    Status status = powerPolicyServer->registerPowerPolicyChangeCallback(callback, filter);

    ASSERT_TRUE(status.isOk());

    status = powerPolicyServer->unregisterPowerPolicyChangeCallback(callback.get());

    ASSERT_TRUE(status.isOk());
}

TEST_P(PowerPolicyAidlTest, TestRegisterCallback_doubleRegistering) {
    sp<MockPowerPolicyChangeCallback> callback = new MockPowerPolicyChangeCallback();
    callback->expectNormalBinder();
    CarPowerPolicyFilter filter;
    filter.components.push_back(PowerComponent::AUDIO);

    Status status = powerPolicyServer->registerPowerPolicyChangeCallback(callback, filter);

    ASSERT_TRUE(status.isOk());

    status = powerPolicyServer->registerPowerPolicyChangeCallback(callback, filter);

    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
}

TEST_P(PowerPolicyAidlTest, TestUnegisterNotRegisteredCallback) {
    sp<MockPowerPolicyChangeCallback> callback = new MockPowerPolicyChangeCallback();

    Status status = powerPolicyServer->unregisterPowerPolicyChangeCallback(callback);

    ASSERT_FALSE(status.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PowerPolicyAidlTest);
INSTANTIATE_TEST_SUITE_P(
    CarPowerPolicyServer, PowerPolicyAidlTest,
    ::testing::ValuesIn(android::getAidlHalInstanceNames(ICarPowerPolicyServer::descriptor)),
    android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
