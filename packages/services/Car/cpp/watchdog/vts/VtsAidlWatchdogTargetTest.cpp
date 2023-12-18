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
#include <android-base/chrono_utils.h>
#include <android-base/file.h>
#include <android/automotive/watchdog/BnCarWatchdogClient.h>
#include <android/automotive/watchdog/ICarWatchdog.h>
#include <android/automotive/watchdog/IResourceOveruseListener.h>
#include <android/automotive/watchdog/IoOveruseStats.h>
#include <android/automotive/watchdog/PerStateBytes.h>
#include <android/automotive/watchdog/ResourceOveruseStats.h>
#include <android/automotive/watchdog/ResourceType.h>
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/Status.h>
#include <gmock/gmock.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/StrongPointer.h>

#include <unistd.h>

#include <future>  // NOLINT(build/c++11)

using ::android::BBinder;
using ::android::Condition;
using ::android::IBinder;
using ::android::Mutex;
using ::android::OK;
using ::android::ProcessState;
using ::android::sp;
using ::android::status_t;
using ::android::String16;
using ::android::wp;
using ::android::automotive::watchdog::BnCarWatchdogClient;
using ::android::automotive::watchdog::ICarWatchdog;
using ::android::automotive::watchdog::IoOveruseStats;
using ::android::automotive::watchdog::IResourceOveruseListenerDefault;
using ::android::automotive::watchdog::PerStateBytes;
using ::android::automotive::watchdog::ResourceOveruseStats;
using ::android::automotive::watchdog::ResourceType;
using ::android::automotive::watchdog::TimeoutLength;
using ::android::base::WriteStringToFile;
using ::android::binder::Status;
using ::testing::_;
using ::testing::InitGoogleTest;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::TestWithParam;
using ::testing::ValuesIn;

namespace {

constexpr std::chrono::nanoseconds kMaxWatchdogPingWaitTimeNs = 6s;

class MockCarWatchdogClient : public BnCarWatchdogClient {
public:
    MockCarWatchdogClient() {}

    MOCK_METHOD(Status, checkIfAlive, (int32_t, TimeoutLength), (override));
    MOCK_METHOD(Status, prepareProcessTermination, (), (override));

    void expectCheckIfAlive() {
        EXPECT_CALL(*this, checkIfAlive(_, _))
                .WillOnce(Invoke([&](int32_t sessionId, TimeoutLength timeoutLength) -> Status {
                    Mutex::Autolock lock(mMutex);
                    mSessionId = sessionId;
                    mTimeoutLength = timeoutLength;
                    mCond.signal();
                    return Status::ok();
                }));
    }

    void waitCheckIfAlive(TimeoutLength expectedTimeoutLength, int32_t* actualSessionId) {
        Mutex::Autolock lock(mMutex);
        ASSERT_THAT(mCond.waitRelative(mMutex, kMaxWatchdogPingWaitTimeNs.count()), OK);
        ASSERT_THAT(mTimeoutLength, expectedTimeoutLength);
        *actualSessionId = mSessionId;
    }

private:
    Mutex mMutex;
    Condition mCond GUARDED_BY(mMutex);
    int32_t mSessionId GUARDED_BY(mMutex);
    TimeoutLength mTimeoutLength GUARDED_BY(mMutex);
};

class MockBinder : public BBinder {
public:
    MOCK_METHOD(status_t, linkToDeath, (const sp<DeathRecipient>&, void*, uint32_t), (override));
    MOCK_METHOD(status_t, unlinkToDeath,
                (const wp<DeathRecipient>&, void*, uint32_t, wp<DeathRecipient>* outRecipient),
                (override));
};

class MockResourceOveruseListener : public IResourceOveruseListenerDefault {
public:
    MockResourceOveruseListener() : mMockBinder(sp<MockBinder>::make()) {}

    MOCK_METHOD(IBinder*, onAsBinder, (), (override));

    void expectNormalBinder() {
        EXPECT_CALL(*mMockBinder, linkToDeath(_, nullptr, 0)).WillRepeatedly(Return(OK));
        EXPECT_CALL(*mMockBinder, unlinkToDeath(_, nullptr, 0, nullptr)).WillRepeatedly(Return(OK));
        EXPECT_CALL(*this, onAsBinder()).WillRepeatedly(Return(mMockBinder.get()));
    }

private:
    sp<MockBinder> mMockBinder;
};

}  // namespace

class WatchdogAidlTest : public TestWithParam<std::string> {
public:
    virtual void SetUp() {
        watchdogServer =
                android::waitForDeclaredService<ICarWatchdog>(String16(GetParam().c_str()));
        ASSERT_NE(watchdogServer.get(), nullptr);
    }

    sp<ICarWatchdog> watchdogServer;
};

TEST_P(WatchdogAidlTest, TestWatchdogClient) {
    sp<MockCarWatchdogClient> mockClient = sp<MockCarWatchdogClient>::make();
    mockClient->expectCheckIfAlive();
    Status status = watchdogServer->registerClient(mockClient, TimeoutLength::TIMEOUT_CRITICAL);
    ASSERT_TRUE(status.isOk()) << "Failed to register client: " << status.toString8();
    int32_t sessionId;
    ASSERT_NO_FATAL_FAILURE(
            mockClient->waitCheckIfAlive(TimeoutLength::TIMEOUT_CRITICAL, &sessionId));
    status = watchdogServer->tellClientAlive(mockClient, sessionId);
    ASSERT_TRUE(status.isOk()) << "Failed to tell client alive: " << status.toString8();
    status = watchdogServer->unregisterClient(mockClient);
    ASSERT_TRUE(status.isOk()) << "Failed to unregister client: " << status.toString8();
}

TEST_P(WatchdogAidlTest, TestFailsRegisterClientWithNullptrClient) {
    ASSERT_FALSE(watchdogServer->registerClient(nullptr, TimeoutLength::TIMEOUT_CRITICAL).isOk())
            << "Should fail to register null client";
}

TEST_P(WatchdogAidlTest, TestFailsToTellClientAliveForNotRegisteredClient) {
    sp<MockCarWatchdogClient> mockClient = sp<MockCarWatchdogClient>::make();
    ASSERT_FALSE(watchdogServer->tellClientAlive(mockClient, 0).isOk())
            << "Should fail tell client alive for not registered client";
}

TEST_P(WatchdogAidlTest, TestFailsToUnRegisterNotRegisteredClient) {
    sp<MockCarWatchdogClient> mockClient = sp<MockCarWatchdogClient>::make();
    ASSERT_FALSE(watchdogServer->unregisterClient(mockClient).isOk())
            << "Should fail to unregister not registered client";
}

TEST_P(WatchdogAidlTest, TestResourceOveruseListener) {
    sp<MockResourceOveruseListener> mockListener = sp<MockResourceOveruseListener>::make();
    mockListener->expectNormalBinder();
    Status status = watchdogServer->addResourceOveruseListener({ResourceType::IO}, mockListener);
    ASSERT_TRUE(status.isOk()) << "Failed to add resource overuse listener: " << status.toString8();
    status = watchdogServer->removeResourceOveruseListener(mockListener);
    ASSERT_TRUE(status.isOk()) << "Failed to remove resource overuse listener: "
                               << status.toString8();
}

TEST_P(WatchdogAidlTest, TestFailsAddResourceOveruseListenerWithNoResourceType) {
    sp<MockResourceOveruseListener> mockListener = sp<MockResourceOveruseListener>::make();
    mockListener->expectNormalBinder();
    ASSERT_FALSE(watchdogServer->addResourceOveruseListener({}, mockListener).isOk())
            << "Should fail to add resource overuse listener with no resource type";
}

TEST_P(WatchdogAidlTest, TestFailsAddResourceOveruseListenerWithNullptrListener) {
    ASSERT_FALSE(watchdogServer->addResourceOveruseListener({ResourceType::IO}, nullptr).isOk())
            << "Should fail to add null resource overuse listener";
}

TEST_P(WatchdogAidlTest, TestFailsToRemoveNotAddedResourceOveruseListener) {
    sp<MockResourceOveruseListener> mockListener = sp<MockResourceOveruseListener>::make();
    mockListener->expectNormalBinder();
    ASSERT_FALSE(watchdogServer->removeResourceOveruseListener(mockListener).isOk())
            << "Should fail to remote listener that is not added";
}

/*
 * getResourceOveruseStats AIDL method is not tested as it requires writing to disk and waiting
 * until the watchdog server has read I/O stats. The waiting duration depends on the watchdog
 * server's performance data collection frequency, which varies between 20 - 60 seconds depending
 * on the build type. The core implementation is tested in ATS with the help of custom performance
 * data collection, which requires dumpsys access and this is not available to VTS. Thus skipping
 * this test in VTS.
 */

TEST_P(WatchdogAidlTest, TestFailsGetResourceOveruseStatsWithNoResourceTypes) {
    std::vector<ResourceOveruseStats> resourceOveruseStats;
    ASSERT_FALSE(watchdogServer->getResourceOveruseStats({}, &resourceOveruseStats).isOk())
            << "Should fail to fetch resource overuse stats with no resource types";
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WatchdogAidlTest);
INSTANTIATE_TEST_SUITE_P(CarWatchdogServer, WatchdogAidlTest,
                         ValuesIn(android::getAidlHalInstanceNames(ICarWatchdog::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
