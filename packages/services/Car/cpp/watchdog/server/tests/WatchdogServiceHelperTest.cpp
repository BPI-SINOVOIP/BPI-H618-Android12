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

#include "MockCarWatchdogServiceForSystem.h"
#include "MockWatchdogProcessService.h"
#include "WatchdogServiceHelper.h"

#include <binder/IBinder.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/RefBase.h>

namespace android {
namespace automotive {
namespace watchdog {

namespace aawi = ::android::automotive::watchdog::internal;

using aawi::ApplicationCategoryType;
using aawi::ComponentType;
using aawi::ICarWatchdogServiceForSystem;
using aawi::PackageInfo;
using aawi::PackageIoOveruseStats;
using aawi::UidType;
using ::android::IBinder;
using ::android::RefBase;
using ::android::sp;
using ::android::base::Error;
using ::android::base::Result;
using ::android::binder::Status;
using ::testing::_;
using ::testing::DoAll;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::UnorderedElementsAreArray;

namespace internal {

class WatchdogServiceHelperPeer : public RefBase {
public:
    explicit WatchdogServiceHelperPeer(const sp<WatchdogServiceHelper>& helper) : mHelper(helper) {}
    ~WatchdogServiceHelperPeer() { mHelper.clear(); }

    Result<void> init(const android::sp<WatchdogProcessService>& watchdogProcessService) {
        return mHelper->init(watchdogProcessService);
    }

    const sp<ICarWatchdogServiceForSystem> getCarWatchdogServiceForSystem() {
        return mHelper->mService;
    }

private:
    sp<WatchdogServiceHelper> mHelper;
};

}  // namespace internal

namespace {

PackageInfo constructPackageInfo(const char* packageName, int32_t uid, UidType uidType,
                                 ComponentType componentType,
                                 ApplicationCategoryType appCategoryType) {
    PackageInfo packageInfo;
    packageInfo.packageIdentifier.name = packageName;
    packageInfo.packageIdentifier.uid = uid;
    packageInfo.uidType = uidType;
    packageInfo.componentType = componentType;
    packageInfo.appCategoryType = appCategoryType;
    return packageInfo;
}

}  // namespace

class WatchdogServiceHelperTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        mMockWatchdogProcessService = new MockWatchdogProcessService();
        mWatchdogServiceHelper = new WatchdogServiceHelper();
        mWatchdogServiceHelperPeer =
                new internal::WatchdogServiceHelperPeer(mWatchdogServiceHelper);
        mMockCarWatchdogServiceForSystem = new MockCarWatchdogServiceForSystem();
        mMockCarWatchdogServiceForSystemBinder = mMockCarWatchdogServiceForSystem->getBinder();

        EXPECT_CALL(*mMockWatchdogProcessService, registerWatchdogServiceHelper(_))
                .WillOnce(Return(Result<void>()));
        auto result = mWatchdogServiceHelperPeer->init(mMockWatchdogProcessService);
        ASSERT_RESULT_OK(result);
    }

    virtual void TearDown() {
        if (mWatchdogServiceHelperPeer->getCarWatchdogServiceForSystem() != nullptr) {
            EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder,
                        unlinkToDeath(_, nullptr, 0, nullptr))
                    .WillOnce(Return(OK));
            EXPECT_CALL(*mMockWatchdogProcessService, unregisterCarWatchdogService(_)).Times(1);
        }
        mWatchdogServiceHelper.clear();

        mMockWatchdogProcessService.clear();
        mMockCarWatchdogServiceForSystem.clear();
        mMockCarWatchdogServiceForSystemBinder.clear();
        mWatchdogServiceHelperPeer.clear();
    }

    void registerCarWatchdogService() {
        EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, linkToDeath(_, nullptr, 0))
                .WillOnce(Return(OK));
        EXPECT_CALL(*mMockWatchdogProcessService, registerCarWatchdogService(_))
                .WillOnce(Return(Status::ok()));

        Status status = mWatchdogServiceHelper->registerService(mMockCarWatchdogServiceForSystem);
        ASSERT_TRUE(status.isOk()) << status;
        ASSERT_NE(mWatchdogServiceHelperPeer->getCarWatchdogServiceForSystem(), nullptr);
    }

    sp<WatchdogServiceHelper> mWatchdogServiceHelper;
    sp<MockWatchdogProcessService> mMockWatchdogProcessService;
    sp<MockCarWatchdogServiceForSystem> mMockCarWatchdogServiceForSystem;
    sp<MockBinder> mMockCarWatchdogServiceForSystemBinder;
    sp<internal::WatchdogServiceHelperPeer> mWatchdogServiceHelperPeer;
};

TEST_F(WatchdogServiceHelperTest, TestInit) {
    sp<WatchdogServiceHelper> helper(new WatchdogServiceHelper());
    sp<MockWatchdogProcessService> mockWatchdogProcessService(new MockWatchdogProcessService());

    EXPECT_CALL(*mockWatchdogProcessService, registerWatchdogServiceHelper(_))
            .WillOnce(Return(Result<void>()));

    ASSERT_RESULT_OK(helper->init(mockWatchdogProcessService));
}

TEST_F(WatchdogServiceHelperTest, TestErrorOnInitWithErrorFromWatchdogProcessServiceRegistration) {
    sp<WatchdogServiceHelper> helper(new WatchdogServiceHelper());
    sp<MockWatchdogProcessService> mockWatchdogProcessService(new MockWatchdogProcessService());

    EXPECT_CALL(*mockWatchdogProcessService, registerWatchdogServiceHelper(_))
            .WillOnce([](const sp<IWatchdogServiceHelper>&) -> Result<void> {
                return Error() << "Failed to register";
            });

    auto result = helper->init(nullptr);

    ASSERT_FALSE(result.ok()) << "Watchdog service helper init should fail on error from "
                              << "watchdog process service registration error";
}

TEST_F(WatchdogServiceHelperTest, TestErrorOnInitWithNullWatchdogProcessServiceInstance) {
    sp<WatchdogServiceHelper> helper(new WatchdogServiceHelper());

    auto result = helper->init(nullptr);

    ASSERT_FALSE(result.ok())
            << "Watchdog service helper init should fail on null watchdog process service instance";
}

TEST_F(WatchdogServiceHelperTest, TestTerminate) {
    registerCarWatchdogService();
    EXPECT_CALL(*(mMockCarWatchdogServiceForSystem->getBinder()),
                unlinkToDeath(_, nullptr, 0, nullptr))
            .WillOnce(Return(OK));

    mWatchdogServiceHelper->terminate();

    ASSERT_EQ(mWatchdogServiceHelper->mService, nullptr);
}

TEST_F(WatchdogServiceHelperTest, TestRegisterService) {
    sp<IBinder> binder = static_cast<sp<IBinder>>(mMockCarWatchdogServiceForSystemBinder);
    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, linkToDeath(_, nullptr, 0))
            .WillOnce(Return(OK));
    EXPECT_CALL(*mMockWatchdogProcessService, registerCarWatchdogService(binder))
            .WillOnce(Return(Status::ok()));

    Status status = mWatchdogServiceHelper->registerService(mMockCarWatchdogServiceForSystem);
    ASSERT_TRUE(status.isOk()) << status;
    ASSERT_NE(mWatchdogServiceHelperPeer->getCarWatchdogServiceForSystem(), nullptr);

    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, linkToDeath(_, nullptr, 0)).Times(0);
    EXPECT_CALL(*mMockWatchdogProcessService, registerCarWatchdogService(_)).Times(0);

    status = mWatchdogServiceHelper->registerService(mMockCarWatchdogServiceForSystem);
    ASSERT_TRUE(status.isOk()) << status;
    ASSERT_NE(mWatchdogServiceHelperPeer->getCarWatchdogServiceForSystem(), nullptr);
}

TEST_F(WatchdogServiceHelperTest, TestErrorOnRegisterServiceWithBinderDied) {
    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, linkToDeath(_, nullptr, 0))
            .WillOnce(Return(DEAD_OBJECT));
    EXPECT_CALL(*mMockWatchdogProcessService, registerCarWatchdogService(_)).Times(0);

    ASSERT_FALSE(mWatchdogServiceHelper->registerService(mMockCarWatchdogServiceForSystem).isOk())
            << "Failed to return error on register service with dead binder";
}

TEST_F(WatchdogServiceHelperTest, TestErrorOnRegisterServiceWithWatchdogProcessServiceError) {
    sp<IBinder> binder = static_cast<sp<IBinder>>(mMockCarWatchdogServiceForSystemBinder);
    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, linkToDeath(_, nullptr, 0))
            .WillOnce(Return(OK));
    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, unlinkToDeath(_, nullptr, 0, nullptr))
            .WillOnce(Return(OK));
    EXPECT_CALL(*mMockWatchdogProcessService, registerCarWatchdogService(binder))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE)));

    ASSERT_FALSE(mWatchdogServiceHelper->registerService(mMockCarWatchdogServiceForSystem).isOk())
            << "Failed to return error on error from watchdog process service";
}

TEST_F(WatchdogServiceHelperTest, TestUnregisterService) {
    registerCarWatchdogService();
    sp<IBinder> binder = static_cast<sp<IBinder>>(mMockCarWatchdogServiceForSystemBinder);
    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, unlinkToDeath(_, nullptr, 0, nullptr))
            .WillOnce(Return(OK));
    EXPECT_CALL(*mMockWatchdogProcessService, unregisterCarWatchdogService(binder)).Times(1);

    Status status = mWatchdogServiceHelper->unregisterService(mMockCarWatchdogServiceForSystem);
    ASSERT_TRUE(status.isOk()) << status;
    ASSERT_EQ(mWatchdogServiceHelperPeer->getCarWatchdogServiceForSystem(), nullptr);

    EXPECT_CALL(*mMockCarWatchdogServiceForSystemBinder, unlinkToDeath(_, nullptr, 0, nullptr))
            .Times(0);
    EXPECT_CALL(*mMockWatchdogProcessService, unregisterCarWatchdogService(_)).Times(0);

    status = mWatchdogServiceHelper->unregisterService(mMockCarWatchdogServiceForSystem);
    ASSERT_FALSE(status.isOk()) << "Unregistering an unregistered service should return an error: "
                                << status;
}

TEST_F(WatchdogServiceHelperTest, TestCheckIfAlive) {
    registerCarWatchdogService();
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem,
                checkIfAlive(0, aawi::TimeoutLength::TIMEOUT_CRITICAL))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogServiceHelper->checkIfAlive(mMockCarWatchdogServiceForSystemBinder, 0,
                                                         TimeoutLength::TIMEOUT_CRITICAL);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorOnCheckIfAliveWithNotRegisteredCarWatchdogServiceBinder) {
    registerCarWatchdogService();
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, checkIfAlive(_, _)).Times(0);
    Status status = mWatchdogServiceHelper->checkIfAlive(new MockBinder(), 0,
                                                         TimeoutLength::TIMEOUT_CRITICAL);
    ASSERT_FALSE(status.isOk()) << "checkIfAlive should fail when the given car watchdog service "
                                   "binder is not registered with the helper";
}

TEST_F(WatchdogServiceHelperTest, TestErrorOnCheckIfAliveWithNoCarWatchdogServiceRegistered) {
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, checkIfAlive(_, _)).Times(0);
    Status status = mWatchdogServiceHelper->checkIfAlive(mMockCarWatchdogServiceForSystemBinder, 0,
                                                         TimeoutLength::TIMEOUT_CRITICAL);
    ASSERT_FALSE(status.isOk())
            << "checkIfAlive should fail when no car watchdog service registered with the helper";
}

TEST_F(WatchdogServiceHelperTest, TestErrorOnCheckIfAliveWithErrorStatusFromCarWatchdogService) {
    registerCarWatchdogService();
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem,
                checkIfAlive(0, aawi::TimeoutLength::TIMEOUT_CRITICAL))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "Illegal state")));
    Status status = mWatchdogServiceHelper->checkIfAlive(mMockCarWatchdogServiceForSystemBinder, 0,
                                                         TimeoutLength::TIMEOUT_CRITICAL);
    ASSERT_FALSE(status.isOk())
            << "checkIfAlive should fail when car watchdog service API returns error";
}

TEST_F(WatchdogServiceHelperTest, TestPrepareProcessTermination) {
    registerCarWatchdogService();
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, prepareProcessTermination())
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogServiceHelper->prepareProcessTermination(
            mMockCarWatchdogServiceForSystemBinder);
    ASSERT_EQ(mWatchdogServiceHelperPeer->getCarWatchdogServiceForSystem(), nullptr);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorOnPrepareProcessTerminationWithNotRegisteredCarWatchdogServiceBinder) {
    registerCarWatchdogService();
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, prepareProcessTermination()).Times(0);
    Status status = mWatchdogServiceHelper->prepareProcessTermination(new MockBinder());
    ASSERT_FALSE(status.isOk()) << "prepareProcessTermination should fail when the given car "
                                   "watchdog service binder is not registered with the helper";
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorOnPrepareProcessTerminationWithNoCarWatchdogServiceRegistered) {
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, prepareProcessTermination()).Times(0);
    Status status = mWatchdogServiceHelper->prepareProcessTermination(
            mMockCarWatchdogServiceForSystemBinder);
    ASSERT_FALSE(status.isOk()) << "prepareProcessTermination should fail when no car watchdog "
                                   "service registered with the helper";
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorOnPrepareProcessTerminationWithErrorStatusFromCarWatchdogService) {
    registerCarWatchdogService();

    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, prepareProcessTermination())
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "Illegal state")));

    Status status = mWatchdogServiceHelper->prepareProcessTermination(
            mMockCarWatchdogServiceForSystemBinder);

    ASSERT_FALSE(status.isOk())
            << "prepareProcessTermination should fail when car watchdog service API returns error";
}

TEST_F(WatchdogServiceHelperTest, TestGetPackageInfosForUids) {
    std::vector<int32_t> uids = {1000};
    std::vector<std::string> prefixesStr = {"vendor.package"};
    std::vector<PackageInfo> expectedPackageInfo{
            constructPackageInfo("vendor.package.A", 120000, UidType::NATIVE, ComponentType::VENDOR,
                                 ApplicationCategoryType::OTHERS),
            constructPackageInfo("third_party.package.B", 130000, UidType::APPLICATION,
                                 ComponentType::THIRD_PARTY, ApplicationCategoryType::OTHERS),
    };
    std::vector<PackageInfo> actualPackageInfo;

    registerCarWatchdogService();

    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, getPackageInfosForUids(uids, prefixesStr, _))
            .WillOnce(DoAll(SetArgPointee<2>(expectedPackageInfo), Return(Status::ok())));

    Status status =
            mWatchdogServiceHelper->getPackageInfosForUids(uids, prefixesStr, &actualPackageInfo);

    ASSERT_TRUE(status.isOk()) << status;
    EXPECT_THAT(actualPackageInfo, UnorderedElementsAreArray(expectedPackageInfo));
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorOnGetPackageInfosForUidsWithNoCarWatchdogServiceRegistered) {
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, getPackageInfosForUids(_, _, _)).Times(0);

    std::vector<int32_t> uids;
    std::vector<std::string> prefixes;
    std::vector<PackageInfo> actualPackageInfo;
    Status status =
            mWatchdogServiceHelper->getPackageInfosForUids(uids, prefixes, &actualPackageInfo);

    ASSERT_FALSE(status.isOk()) << "getPackageInfosForUids should fail when no "
                                   "car watchdog service registered with the helper";
    EXPECT_THAT(actualPackageInfo, IsEmpty());
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorOnGetPackageInfosForUidsWithErrorStatusFromCarWatchdogService) {
    registerCarWatchdogService();
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, getPackageInfosForUids(_, _, _))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "Illegal state")));

    std::vector<int32_t> uids;
    std::vector<std::string> prefixes;
    std::vector<PackageInfo> actualPackageInfo;
    Status status =
            mWatchdogServiceHelper->getPackageInfosForUids(uids, prefixes, &actualPackageInfo);

    ASSERT_FALSE(status.isOk()) << "getPackageInfosForUids should fail when car watchdog "
                                   "service API returns error";
    ASSERT_TRUE(actualPackageInfo.empty());
}

TEST_F(WatchdogServiceHelperTest, TestLatestIoOveruseStats) {
    PackageIoOveruseStats stats;
    stats.uid = 101000;
    stats.ioOveruseStats.killableOnOveruse = true;
    stats.ioOveruseStats.startTime = 99898;
    stats.ioOveruseStats.durationInSeconds = 12345;
    stats.ioOveruseStats.totalOveruses = 10;
    stats.shouldNotify = true;
    std::vector<PackageIoOveruseStats> expectedIoOveruseStats = {stats};

    registerCarWatchdogService();

    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, latestIoOveruseStats(expectedIoOveruseStats))
            .WillOnce(Return(Status::ok()));

    Status status = mWatchdogServiceHelper->latestIoOveruseStats(expectedIoOveruseStats);

    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorsOnLatestIoOveruseStatsWithNoCarWatchdogServiceRegistered) {
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, latestIoOveruseStats(_)).Times(0);

    Status status = mWatchdogServiceHelper->latestIoOveruseStats({});

    ASSERT_FALSE(status.isOk()) << "latetstIoOveruseStats should fail when no "
                                   "car watchdog service registered with the helper";
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorsOnLatestIoOveruseStatsWithErrorStatusFromCarWatchdogService) {
    registerCarWatchdogService();

    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, latestIoOveruseStats(_))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "Illegal state")));

    Status status = mWatchdogServiceHelper->latestIoOveruseStats({});

    ASSERT_FALSE(status.isOk()) << "latetstIoOveruseStats should fail when car watchdog "
                                   "service API returns error";
}

TEST_F(WatchdogServiceHelperTest, TestResetResourceOveruseStats) {
    registerCarWatchdogService();

    std::vector<std::string> packageNames = {"system.daemon"};
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, resetResourceOveruseStats(packageNames))
            .WillOnce(Return(Status::ok()));

    Status status = mWatchdogServiceHelper->resetResourceOveruseStats(packageNames);

    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorsOnResetResourceOveruseStatsWithNoCarWatchdogServiceRegistered) {
    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, resetResourceOveruseStats(_)).Times(0);

    Status status = mWatchdogServiceHelper->resetResourceOveruseStats({});

    ASSERT_FALSE(status.isOk()) << "resetResourceOveruseStats should fail when no "
                                   "car watchdog service registered with the helper";
}

TEST_F(WatchdogServiceHelperTest,
       TestErrorsOnResetResourceOveruseStatsWithErrorStatusFromCarWatchdogService) {
    registerCarWatchdogService();

    EXPECT_CALL(*mMockCarWatchdogServiceForSystem, resetResourceOveruseStats(_))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "Illegal state")));

    Status status = mWatchdogServiceHelper->resetResourceOveruseStats({});

    ASSERT_FALSE(status.isOk()) << "resetResourceOveruseStats should fail when car watchdog "
                                   "service API returns error";
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
