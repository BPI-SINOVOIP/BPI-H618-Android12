/*
 * Copyright 2020 The Android Open Source Project
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

#include "MockIoOveruseMonitor.h"
#include "MockResourceOveruseListener.h"
#include "MockWatchdogPerfService.h"
#include "MockWatchdogProcessService.h"
#include "MockWatchdogServiceHelper.h"
#include "WatchdogBinderMediator.h"

#include <android-base/stringprintf.h>
#include <binder/IBinder.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/String16.h>

#include <errno.h>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::sp;
using ::android::String16;
using ::android::base::Result;
using ::android::base::StringAppendF;
using ::android::binder::Status;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::UnorderedElementsAreArray;

namespace {

const std::function<android::base::Result<void>(const char*, const android::sp<android::IBinder>&)>
        kAddServiceFunctionStub =
                [](const char*, const android::sp<android::IBinder>&) -> Result<void> {
    return Result<void>{};
};

class MockICarWatchdogClient : public ICarWatchdogClient {
public:
    MOCK_METHOD(Status, checkIfAlive, (int32_t sessionId, TimeoutLength timeout), (override));
    MOCK_METHOD(Status, prepareProcessTermination, (), (override));
    MOCK_METHOD(IBinder*, onAsBinder, (), (override));
    MOCK_METHOD(int32_t, getInterfaceVersion, (), (override));
    MOCK_METHOD(std::string, getInterfaceHash, (), (override));
};

std::string toString(const std::vector<ResourceOveruseStats>& resourceOveruseStats) {
    std::string buffer;
    for (const auto& stats : resourceOveruseStats) {
        StringAppendF(&buffer, "%s\n", stats.toString().c_str());
    }
    return buffer;
}

}  // namespace

namespace internal {

class WatchdogBinderMediatorPeer {
public:
    explicit WatchdogBinderMediatorPeer(const sp<WatchdogBinderMediator>& mediator) :
          mMediator(mediator) {}
    ~WatchdogBinderMediatorPeer() { mMediator.clear(); }

    Result<void> init(const sp<IIoOveruseMonitor>& ioOveruseMonitor) {
        mMediator->mIoOveruseMonitor = ioOveruseMonitor;
        return mMediator->init();
    }

private:
    sp<WatchdogBinderMediator> mMediator;
};

}  // namespace internal

class WatchdogBinderMediatorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        mMockWatchdogProcessService = sp<MockWatchdogProcessService>::make();
        mMockWatchdogPerfService = sp<MockWatchdogPerfService>::make();
        mWatchdogBinderMediator =
                sp<WatchdogBinderMediator>::make(mMockWatchdogProcessService,
                                                 mMockWatchdogPerfService,
                                                 sp<MockWatchdogServiceHelper>::make(),
                                                 kAddServiceFunctionStub);
        internal::WatchdogBinderMediatorPeer mediatorPeer(mWatchdogBinderMediator);
        mMockIoOveruseMonitor = sp<MockIoOveruseMonitor>::make();
        ASSERT_RESULT_OK(mediatorPeer.init(mMockIoOveruseMonitor));
    }
    virtual void TearDown() {
        mMockWatchdogProcessService.clear();
        mMockWatchdogPerfService.clear();
        mMockIoOveruseMonitor.clear();
        mWatchdogBinderMediator.clear();
    }

    sp<MockWatchdogProcessService> mMockWatchdogProcessService;
    sp<MockWatchdogPerfService> mMockWatchdogPerfService;
    sp<MockIoOveruseMonitor> mMockIoOveruseMonitor;
    sp<WatchdogBinderMediator> mWatchdogBinderMediator;
};

TEST_F(WatchdogBinderMediatorTest, TestInit) {
    sp<WatchdogBinderMediator> mediator =
            sp<WatchdogBinderMediator>::make(sp<MockWatchdogProcessService>::make(),
                                             sp<MockWatchdogPerfService>::make(),
                                             sp<MockWatchdogServiceHelper>::make(),
                                             kAddServiceFunctionStub);

    ASSERT_RESULT_OK(mediator->init());

    ASSERT_NE(mediator->mWatchdogProcessService, nullptr);
    ASSERT_NE(mediator->mWatchdogPerfService, nullptr);
    ASSERT_NE(mediator->mIoOveruseMonitor, nullptr);
    ASSERT_NE(mediator->mWatchdogInternalHandler, nullptr);
}

TEST_F(WatchdogBinderMediatorTest, TestErrorOnInitWithNullServiceInstances) {
    auto mockWatchdogProcessService = sp<MockWatchdogProcessService>::make();
    auto mockWatchdogPerfservice = sp<MockWatchdogPerfService>::make();
    auto mockWatchdogServiceHelper = sp<MockWatchdogServiceHelper>::make();
    sp<WatchdogBinderMediator> mediator =
            sp<WatchdogBinderMediator>::make(nullptr, mockWatchdogPerfservice,
                                             mockWatchdogServiceHelper, kAddServiceFunctionStub);

    EXPECT_FALSE(mediator->init().ok()) << "No error returned on nullptr watchdog process service";
    mediator.clear();

    mediator = sp<WatchdogBinderMediator>::make(mockWatchdogProcessService, nullptr,
                                                mockWatchdogServiceHelper, kAddServiceFunctionStub);

    EXPECT_FALSE(mediator->init().ok()) << "No error returned on nullptr watchdog perf service";
    mediator.clear();

    mediator = sp<WatchdogBinderMediator>::make(mockWatchdogProcessService, mockWatchdogPerfservice,
                                                nullptr, kAddServiceFunctionStub);

    EXPECT_FALSE(mediator->init().ok()) << "No error returned on nullptr watchdog service helper";
    mediator.clear();

    mediator = sp<WatchdogBinderMediator>::make(nullptr, nullptr, nullptr, kAddServiceFunctionStub);

    EXPECT_FALSE(mediator->init().ok()) << "No error returned on null services";
    mediator.clear();
}

TEST_F(WatchdogBinderMediatorTest, TestTerminate) {
    EXPECT_NE(mWatchdogBinderMediator->mWatchdogProcessService, nullptr);
    EXPECT_NE(mWatchdogBinderMediator->mWatchdogPerfService, nullptr);
    EXPECT_NE(mWatchdogBinderMediator->mIoOveruseMonitor, nullptr);
    EXPECT_NE(mWatchdogBinderMediator->mWatchdogInternalHandler, nullptr);

    mWatchdogBinderMediator->terminate();

    EXPECT_EQ(mWatchdogBinderMediator->mWatchdogProcessService, nullptr);
    EXPECT_EQ(mWatchdogBinderMediator->mWatchdogPerfService, nullptr);
    EXPECT_EQ(mWatchdogBinderMediator->mIoOveruseMonitor, nullptr);
    EXPECT_EQ(mWatchdogBinderMediator->mWatchdogInternalHandler, nullptr);
}

TEST_F(WatchdogBinderMediatorTest, TestDumpWithEmptyArgs) {
    EXPECT_CALL(*mMockWatchdogProcessService, dump(-1, _)).WillOnce(Return(Result<void>()));
    EXPECT_CALL(*mMockWatchdogPerfService, onDump(-1)).WillOnce(Return(Result<void>()));
    mWatchdogBinderMediator->dump(-1, Vector<String16>());
}

TEST_F(WatchdogBinderMediatorTest, TestDumpWithStartCustomPerfCollection) {
    EXPECT_CALL(*mMockWatchdogPerfService, onCustomCollection(-1, _))
            .WillOnce(Return(Result<void>()));

    Vector<String16> args;
    args.push_back(String16(kStartCustomCollectionFlag));
    ASSERT_EQ(mWatchdogBinderMediator->dump(-1, args), OK);
}

TEST_F(WatchdogBinderMediatorTest, TestDumpWithStopCustomPerfCollection) {
    EXPECT_CALL(*mMockWatchdogPerfService, onCustomCollection(-1, _))
            .WillOnce(Return(Result<void>()));

    Vector<String16> args;
    args.push_back(String16(kEndCustomCollectionFlag));
    ASSERT_EQ(mWatchdogBinderMediator->dump(-1, args), OK);
}

TEST_F(WatchdogBinderMediatorTest, TestDumpWithResetResourceOveruseStats) {
    std::vector<std::string> actualPackages;
    EXPECT_CALL(*mMockIoOveruseMonitor, resetIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualPackages), Return(Result<void>())));

    Vector<String16> args;
    args.push_back(String16(kResetResourceOveruseStatsFlag));
    args.push_back(String16("packageA,packageB"));
    ASSERT_EQ(mWatchdogBinderMediator->dump(-1, args), OK);
    ASSERT_EQ(actualPackages, std::vector<std::string>({"packageA", "packageB"}));
}

TEST_F(WatchdogBinderMediatorTest, TestFailsDumpWithInvalidResetResourceOveruseStatsArg) {
    EXPECT_CALL(*mMockIoOveruseMonitor, resetIoOveruseStats(_)).Times(0);

    Vector<String16> args;
    args.push_back(String16(kResetResourceOveruseStatsFlag));
    args.push_back(String16(""));
    ASSERT_EQ(mWatchdogBinderMediator->dump(-1, args), BAD_VALUE);
}

TEST_F(WatchdogBinderMediatorTest, TestDumpWithInvalidDumpArgs) {
    Vector<String16> args;
    args.push_back(String16("--invalid_option"));
    int nullFd = open("/dev/null", O_RDONLY);
    EXPECT_CALL(*mMockWatchdogProcessService, dump(nullFd, _)).WillOnce(Return(Result<void>()));
    EXPECT_CALL(*mMockWatchdogPerfService, onDump(nullFd)).WillOnce(Return(Result<void>()));

    EXPECT_EQ(mWatchdogBinderMediator->dump(nullFd, args), OK) << "Error returned on invalid args";
    close(nullFd);
}

TEST_F(WatchdogBinderMediatorTest, TestRegisterClient) {
    sp<ICarWatchdogClient> client = sp<MockICarWatchdogClient>::make();
    TimeoutLength timeout = TimeoutLength::TIMEOUT_MODERATE;
    EXPECT_CALL(*mMockWatchdogProcessService, registerClient(client, timeout))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogBinderMediator->registerClient(client, timeout);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogBinderMediatorTest, TestUnregisterClient) {
    sp<ICarWatchdogClient> client = sp<MockICarWatchdogClient>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, unregisterClient(client))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogBinderMediator->unregisterClient(client);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogBinderMediatorTest, TestTellClientAlive) {
    sp<ICarWatchdogClient> client = sp<MockICarWatchdogClient>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, tellClientAlive(client, 456))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogBinderMediator->tellClientAlive(client, 456);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogBinderMediatorTest, TestAddResourceOveruseListener) {
    sp<IResourceOveruseListener> listener = sp<MockResourceOveruseListener>::make();
    EXPECT_CALL(*mMockIoOveruseMonitor, addIoOveruseListener(listener))
            .WillOnce(Return(Result<void>{}));

    Status status =
            mWatchdogBinderMediator->addResourceOveruseListener({ResourceType::IO}, listener);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogBinderMediatorTest, TestErrorsAddResourceOveruseListenerOnInvalidArgs) {
    sp<IResourceOveruseListener> listener = sp<MockResourceOveruseListener>::make();
    EXPECT_CALL(*mMockIoOveruseMonitor, addIoOveruseListener(listener)).Times(0);

    ASSERT_FALSE(mWatchdogBinderMediator->addResourceOveruseListener({}, listener).isOk())
            << "Should fail on empty resource types";

    ASSERT_FALSE(
            mWatchdogBinderMediator->addResourceOveruseListener({ResourceType::IO}, nullptr).isOk())
            << "Should fail on null listener";
}

TEST_F(WatchdogBinderMediatorTest, TestRemoveResourceOveruseListener) {
    sp<IResourceOveruseListener> listener = sp<MockResourceOveruseListener>::make();
    EXPECT_CALL(*mMockIoOveruseMonitor, removeIoOveruseListener(listener))
            .WillOnce(Return(Result<void>{}));

    Status status = mWatchdogBinderMediator->removeResourceOveruseListener(listener);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogBinderMediatorTest, TestGetResourceOveruseStats) {
    IoOveruseStats ioOveruseStats;
    ioOveruseStats.killableOnOveruse = true;
    ioOveruseStats.startTime = 99898;
    ioOveruseStats.durationInSeconds = 12345;
    ioOveruseStats.totalOveruses = 3;
    std::vector<ResourceOveruseStats> expected;
    ResourceOveruseStats stats;
    stats.set<ResourceOveruseStats::ioOveruseStats>(ioOveruseStats);
    expected.emplace_back(std::move(stats));

    EXPECT_CALL(*mMockIoOveruseMonitor, getIoOveruseStats(_))
            .WillOnce(DoAll(SetArgPointee<0>(ioOveruseStats), Return(Result<void>{})));

    std::vector<ResourceOveruseStats> actual;
    Status status = mWatchdogBinderMediator->getResourceOveruseStats({ResourceType::IO}, &actual);
    ASSERT_TRUE(status.isOk()) << status;
    EXPECT_THAT(actual, UnorderedElementsAreArray(expected))
            << "Expected: " << toString(expected) << "\nActual: " << toString(actual);
}

TEST_F(WatchdogBinderMediatorTest, TestErrorsGetResourceOveruseStatsOnInvalidArgs) {
    sp<IResourceOveruseListener> listener = sp<MockResourceOveruseListener>::make();
    EXPECT_CALL(*mMockIoOveruseMonitor, getIoOveruseStats(_)).Times(0);

    std::vector<ResourceOveruseStats> actual;
    ASSERT_FALSE(mWatchdogBinderMediator->getResourceOveruseStats({}, &actual).isOk())
            << "Should fail on empty resource types";

    ASSERT_FALSE(
            mWatchdogBinderMediator->getResourceOveruseStats({ResourceType::IO}, nullptr).isOk())
            << "Should fail on null listener";
}

TEST_F(WatchdogBinderMediatorTest, TestRegisterMediator) {
    Status status = mWatchdogBinderMediator->registerMediator(nullptr);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

TEST_F(WatchdogBinderMediatorTest, TestUnregisterMediator) {
    Status status = mWatchdogBinderMediator->unregisterMediator(nullptr);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

TEST_F(WatchdogBinderMediatorTest, TestRegisterMonitor) {
    Status status = mWatchdogBinderMediator->registerMonitor(nullptr);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

TEST_F(WatchdogBinderMediatorTest, TestUnregisterMonitor) {
    Status status = mWatchdogBinderMediator->unregisterMonitor(nullptr);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

TEST_F(WatchdogBinderMediatorTest, TestTellMediatorAlive) {
    Status status = mWatchdogBinderMediator->tellMediatorAlive(nullptr, {}, 0);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

TEST_F(WatchdogBinderMediatorTest, TestTellDumpFinished) {
    Status status = mWatchdogBinderMediator->tellDumpFinished(nullptr, 0);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

TEST_F(WatchdogBinderMediatorTest, TestNotifySystemStateChange) {
    Status status = mWatchdogBinderMediator->notifySystemStateChange(StateType::POWER_CYCLE, 0, 0);
    ASSERT_EQ(status.exceptionCode(), Status::EX_UNSUPPORTED_OPERATION);
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
