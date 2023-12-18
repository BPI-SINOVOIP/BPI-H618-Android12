/*
 * Copyright 2021 The Android Open Source Project
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

#include "IoOveruseMonitor.h"
#include "MockIoOveruseConfigs.h"
#include "MockPackageInfoResolver.h"
#include "MockProcDiskStats.h"
#include "MockResourceOveruseListener.h"
#include "MockUidIoStats.h"
#include "MockWatchdogServiceHelper.h"

#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

#include <functional>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::IPCThreadState;
using ::android::RefBase;
using ::android::sp;
using ::android::automotive::watchdog::internal::IoOveruseAlertThreshold;
using ::android::automotive::watchdog::internal::PackageIdentifier;
using ::android::automotive::watchdog::internal::PackageInfo;
using ::android::automotive::watchdog::internal::PackageIoOveruseStats;
using ::android::automotive::watchdog::internal::ResourceOveruseConfiguration;
using ::android::automotive::watchdog::internal::UidType;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StringAppendF;
using ::android::binder::Status;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::UnorderedElementsAreArray;

namespace {

constexpr size_t kTestMonitorBufferSize = 3;
constexpr int64_t KTestMinSyncWrittenBytes = 5'000;
constexpr double kTestIoOveruseWarnPercentage = 80;
constexpr std::chrono::seconds kTestMonitorInterval = 5s;
constexpr std::chrono::seconds kMaxWaitSeconds = 5s;

IoOveruseAlertThreshold toIoOveruseAlertThreshold(const int64_t durationInSeconds,
                                                  const int64_t writtenBytesPerSecond) {
    IoOveruseAlertThreshold threshold;
    threshold.durationInSeconds = durationInSeconds;
    threshold.writtenBytesPerSecond = writtenBytesPerSecond;
    return threshold;
}

PackageIdentifier constructPackageIdentifier(const char* packageName, const int32_t uid) {
    PackageIdentifier packageIdentifier;
    packageIdentifier.name = packageName;
    packageIdentifier.uid = uid;
    return packageIdentifier;
}

PackageInfo constructPackageInfo(const char* packageName, const int32_t uid,
                                 const UidType uidType) {
    PackageInfo packageInfo;
    packageInfo.packageIdentifier = constructPackageIdentifier(packageName, uid);
    packageInfo.uidType = uidType;
    return packageInfo;
}

PerStateBytes constructPerStateBytes(const int64_t fgBytes, const int64_t bgBytes,
                                     const int64_t gmBytes) {
    PerStateBytes perStateBytes;
    perStateBytes.foregroundBytes = fgBytes;
    perStateBytes.backgroundBytes = bgBytes;
    perStateBytes.garageModeBytes = gmBytes;
    return perStateBytes;
}

IoOveruseStats constructIoOveruseStats(const bool isKillable, const PerStateBytes& remaining,
                                       const PerStateBytes& written, const int totalOveruses,
                                       const int64_t startTime, const int64_t durationInSeconds) {
    IoOveruseStats stats;
    stats.killableOnOveruse = isKillable;
    stats.remainingWriteBytes = remaining;
    stats.startTime = startTime;
    stats.durationInSeconds = durationInSeconds;
    stats.writtenBytes = written;
    stats.totalOveruses = totalOveruses;

    return stats;
}

ResourceOveruseStats constructResourceOveruseStats(IoOveruseStats ioOveruseStats) {
    ResourceOveruseStats stats;
    stats.set<ResourceOveruseStats::ioOveruseStats>(ioOveruseStats);
    return stats;
}

PackageIoOveruseStats constructPackageIoOveruseStats(
        const int32_t uid, const bool shouldNotify, const bool isKillable,
        const PerStateBytes& remaining, const PerStateBytes& written, const int totalOveruses,
        const int64_t startTime, const int64_t durationInSeconds) {
    PackageIoOveruseStats stats;
    stats.uid = uid;
    stats.shouldNotify = shouldNotify;
    stats.ioOveruseStats = constructIoOveruseStats(isKillable, remaining, written, totalOveruses,
                                                   startTime, durationInSeconds);

    return stats;
}

class ScopedChangeCallingUid : public RefBase {
public:
    explicit ScopedChangeCallingUid(uid_t uid) {
        mCallingUid = IPCThreadState::self()->getCallingUid();
        mCallingPid = IPCThreadState::self()->getCallingPid();
        if (mCallingUid == uid) {
            return;
        }
        mChangedUid = uid;
        int64_t token = ((int64_t)mChangedUid << 32) | mCallingPid;
        IPCThreadState::self()->restoreCallingIdentity(token);
    }
    ~ScopedChangeCallingUid() {
        if (mCallingUid == mChangedUid) {
            return;
        }
        int64_t token = ((int64_t)mCallingUid << 32) | mCallingPid;
        IPCThreadState::self()->restoreCallingIdentity(token);
    }

private:
    uid_t mCallingUid;
    uid_t mChangedUid;
    pid_t mCallingPid;
};

std::string toString(const std::vector<PackageIoOveruseStats>& ioOveruseStats) {
    if (ioOveruseStats.empty()) {
        return "empty";
    }
    std::string buffer;
    for (const auto& stats : ioOveruseStats) {
        StringAppendF(&buffer, "%s\n", stats.toString().c_str());
    }
    return buffer;
}

}  // namespace

namespace internal {

class IoOveruseMonitorPeer : public RefBase {
public:
    explicit IoOveruseMonitorPeer(const sp<IoOveruseMonitor>& ioOveruseMonitor) :
          mIoOveruseMonitor(ioOveruseMonitor) {}

    Result<void> init(const sp<IIoOveruseConfigs>& ioOveruseConfigs,
                      const sp<IPackageInfoResolver>& packageInfoResolver) {
        if (const auto result = mIoOveruseMonitor->init(); !result.ok()) {
            return result;
        }
        mIoOveruseMonitor->mMinSyncWrittenBytes = KTestMinSyncWrittenBytes;
        mIoOveruseMonitor->mPeriodicMonitorBufferSize = kTestMonitorBufferSize;
        mIoOveruseMonitor->mIoOveruseWarnPercentage = kTestIoOveruseWarnPercentage;
        mIoOveruseMonitor->mIoOveruseConfigs = ioOveruseConfigs;
        mIoOveruseMonitor->mPackageInfoResolver = packageInfoResolver;
        return {};
    }

private:
    sp<IoOveruseMonitor> mIoOveruseMonitor;
};

}  // namespace internal

class IoOveruseMonitorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        mMockWatchdogServiceHelper = sp<MockWatchdogServiceHelper>::make();
        mMockIoOveruseConfigs = sp<MockIoOveruseConfigs>::make();
        mMockPackageInfoResolver = sp<MockPackageInfoResolver>::make();
        mIoOveruseMonitor = sp<IoOveruseMonitor>::make(mMockWatchdogServiceHelper);
        mIoOveruseMonitorPeer = sp<internal::IoOveruseMonitorPeer>::make(mIoOveruseMonitor);
        mIoOveruseMonitorPeer->init(mMockIoOveruseConfigs, mMockPackageInfoResolver);
    }

    virtual void TearDown() {
        mMockWatchdogServiceHelper.clear();
        mMockIoOveruseConfigs.clear();
        mMockPackageInfoResolver.clear();
        mIoOveruseMonitor.clear();
        mIoOveruseMonitorPeer.clear();
    }

    void setUpPackagesAndConfigurations() {
        std::unordered_map<uid_t, PackageInfo> packageInfoMapping =
                {{1001000,
                  constructPackageInfo(
                          /*packageName=*/"system.daemon", /*uid=*/1001000, UidType::NATIVE)},
                 {1112345,
                  constructPackageInfo(
                          /*packageName=*/"com.android.google.package", /*uid=*/1112345,
                          UidType::APPLICATION)},
                 {1113999,
                  constructPackageInfo(
                          /*packageName=*/"com.android.google.package", /*uid=*/1113999,
                          UidType::APPLICATION)},
                 {1212345,
                  constructPackageInfo(
                          /*packageName=*/"com.android.google.package", /*uid=*/1212345,
                          UidType::APPLICATION)},
                 {1312345,
                  constructPackageInfo(
                          /*packageName=*/"com.android.google.package", /*uid=*/1312345,
                          UidType::APPLICATION)}};
        ON_CALL(*mMockPackageInfoResolver, getPackageInfosForUids(_))
                .WillByDefault(Return(packageInfoMapping));
        mMockIoOveruseConfigs->injectPackageConfigs({
                {"system.daemon",
                 {constructPerStateBytes(/*fgBytes=*/80'000, /*bgBytes=*/40'000,
                                         /*gmBytes=*/100'000),
                  /*isSafeToKill=*/false}},
                {"com.android.google.package",
                 {constructPerStateBytes(/*fgBytes=*/70'000, /*bgBytes=*/30'000,
                                         /*gmBytes=*/100'000),
                  /*isSafeToKill=*/true}},
        });
    }

    void executeAsUid(uid_t uid, std::function<void()> func) {
        sp<ScopedChangeCallingUid> scopedChangeCallingUid = sp<ScopedChangeCallingUid>::make(uid);
        ASSERT_NO_FATAL_FAILURE(func());
    }

    sp<MockWatchdogServiceHelper> mMockWatchdogServiceHelper;
    sp<MockIoOveruseConfigs> mMockIoOveruseConfigs;
    sp<MockPackageInfoResolver> mMockPackageInfoResolver;
    sp<IoOveruseMonitor> mIoOveruseMonitor;
    sp<internal::IoOveruseMonitorPeer> mIoOveruseMonitorPeer;
};

TEST_F(IoOveruseMonitorTest, TestOnPeriodicCollection) {
    setUpPackagesAndConfigurations();
    sp<MockResourceOveruseListener> mockResourceOveruseListener =
            sp<MockResourceOveruseListener>::make();
    ASSERT_NO_FATAL_FAILURE(executeAsUid(1001000, [&]() {
        ASSERT_RESULT_OK(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener));
    }));

    /*
     * Package "system.daemon" (UID: 1001000) exceeds warn threshold percentage of 80% but no
     * warning is issued as it is a native UID.
     */
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/70'000, /*bgWrBytes=*/20'000, 0, 0)},
             {1112345, IoUsage(0, 0, /*fgWrBytes=*/35'000, /*bgWrBytes=*/15'000, 0, 0)},
             {1212345, IoUsage(0, 0, /*fgWrBytes=*/70'000, /*bgWrBytes=*/20'000, 0, 0)}});

    std::vector<PackageIoOveruseStats> actualIoOveruseStats;
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualIoOveruseStats), Return(Status::ok())));

    time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto [startTime, durationInSeconds] = calculateStartAndDuration(currentTime);

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::NORMAL_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    std::vector<PackageIoOveruseStats> expectedIoOveruseStats =
            {constructPackageIoOveruseStats(/*uid*=*/1001000, /*shouldNotify=*/false,
                                            /*isKillable=*/false, /*remaining=*/
                                            constructPerStateBytes(10'000, 20'000, 100'000),
                                            /*written=*/constructPerStateBytes(70'000, 20'000, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             constructPackageIoOveruseStats(/*uid*=*/1112345, /*shouldNotify=*/false,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(35'000, 15'000, 100'000),
                                            /*written=*/constructPerStateBytes(35'000, 15'000, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             // Exceeds threshold.
             constructPackageIoOveruseStats(/*uid*=*/1212345, /*shouldNotify=*/true,
                                            /*isKillable=*/true,
                                            /*remaining=*/
                                            constructPerStateBytes(0, 10'000, 100'000),
                                            /*written=*/constructPerStateBytes(70'000, 20'000, 0),
                                            /*totalOveruses=*/1, startTime, durationInSeconds)};
    EXPECT_THAT(actualIoOveruseStats, UnorderedElementsAreArray(expectedIoOveruseStats))
            << "Expected: " << toString(expectedIoOveruseStats)
            << "\nActual: " << toString(actualIoOveruseStats);

    ResourceOveruseStats actualOverusingNativeStats;
    // Package "com.android.google.package" for user 11 changed uid from 1112345 to 1113999.
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/30'000, /*bgWrBytes=*/0, 0, 0)},
             {1113999, IoUsage(0, 0, /*fgWrBytes=*/25'000, /*bgWrBytes=*/10'000, 0, 0)},
             {1212345, IoUsage(0, 0, /*fgWrBytes=*/20'000, /*bgWrBytes=*/30'000, 0, 0)}});
    actualIoOveruseStats.clear();
    EXPECT_CALL(*mockResourceOveruseListener, onOveruse(_))
            .WillOnce(DoAll(SaveArg<0>(&actualOverusingNativeStats), Return(Status::ok())));
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualIoOveruseStats), Return(Status::ok())));

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::NORMAL_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    const auto expectedOverusingNativeStats = constructResourceOveruseStats(
            constructIoOveruseStats(/*isKillable=*/false,
                                    /*remaining=*/constructPerStateBytes(0, 20'000, 100'000),
                                    /*written=*/constructPerStateBytes(100'000, 20'000, 0),
                                    /*totalOveruses=*/1, startTime, durationInSeconds));
    EXPECT_THAT(actualOverusingNativeStats, expectedOverusingNativeStats)
            << "Expected: " << expectedOverusingNativeStats.toString()
            << "\nActual: " << actualOverusingNativeStats.toString();

    expectedIoOveruseStats =
            {constructPackageIoOveruseStats(/*uid*=*/1001000, /*shouldNotify=*/true,
                                            /*isKillable=*/false, /*remaining=*/
                                            constructPerStateBytes(0, 20'000, 100'000),
                                            /*written=*/constructPerStateBytes(100'000, 20'000, 0),
                                            /*totalOveruses=*/1, startTime, durationInSeconds),
             // Exceeds warn threshold percentage.
             constructPackageIoOveruseStats(/*uid*=*/1113999, /*shouldNotify=*/true,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(10'000, 5'000, 100'000),
                                            /*written=*/constructPerStateBytes(60'000, 25'000, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             /*
              * Exceeds threshold.
              * The package was forgiven on previous overuse so the remaining bytes should only
              * reflect the bytes written after the forgiven bytes.
              */
             constructPackageIoOveruseStats(/*uid*=*/1212345, /*shouldNotify=*/true,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(50'000, 0, 100'000),
                                            /*written=*/constructPerStateBytes(90'000, 50'000, 0),
                                            /*totalOveruses=*/2, startTime, durationInSeconds)};
    EXPECT_THAT(actualIoOveruseStats, UnorderedElementsAreArray(expectedIoOveruseStats))
            << "Expected: " << toString(expectedIoOveruseStats)
            << "\nActual: " << toString(actualIoOveruseStats);

    /*
     * Current date changed so the daily I/O usage stats should be reset and the latest I/O overuse
     * stats should not aggregate with the previous day's stats.
     */
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/78'000, /*bgWrBytes=*/38'000, 0, 0)},
             {1113999, IoUsage(0, 0, /*fgWrBytes=*/55'000, /*bgWrBytes=*/23'000, 0, 0)},
             {1212345, IoUsage(0, 0, /*fgWrBytes=*/55'000, /*bgWrBytes=*/23'000, 0, 0)}});
    actualIoOveruseStats.clear();
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualIoOveruseStats), Return(Status::ok())));

    currentTime += (24 * 60 * 60);  // Change collection time to next day.
    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::NORMAL_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    const auto [nextDayStartTime, nextDayDuration] = calculateStartAndDuration(currentTime);
    expectedIoOveruseStats =
            {constructPackageIoOveruseStats(/*uid*=*/1001000, /*shouldNotify=*/false,
                                            /*isKillable=*/false, /*remaining=*/
                                            constructPerStateBytes(2'000, 2'000, 100'000),
                                            /*written=*/constructPerStateBytes(78'000, 38'000, 0),
                                            /*totalOveruses=*/0, nextDayStartTime, nextDayDuration),
             constructPackageIoOveruseStats(/*uid*=*/1113999, /*shouldNotify=*/false,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(15'000, 7'000, 100'000),
                                            /*written=*/constructPerStateBytes(55'000, 23'000, 0),
                                            /*totalOveruses=*/0, nextDayStartTime, nextDayDuration),
             constructPackageIoOveruseStats(/*uid*=*/1212345, /*shouldNotify=*/false,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(15'000, 7'000, 100'000),
                                            /*written=*/constructPerStateBytes(55'000, 23'000, 0),
                                            /*totalOveruses=*/0, nextDayStartTime,
                                            nextDayDuration)};

    EXPECT_THAT(actualIoOveruseStats, UnorderedElementsAreArray(expectedIoOveruseStats))
            << "Expected: " << toString(expectedIoOveruseStats)
            << "\nActual: " << toString(actualIoOveruseStats);
}

TEST_F(IoOveruseMonitorTest, TestOnPeriodicCollectionWithGarageMode) {
    setUpPackagesAndConfigurations();
    sp<MockResourceOveruseListener> mockResourceOveruseListener =
            sp<MockResourceOveruseListener>::make();
    ASSERT_NO_FATAL_FAILURE(executeAsUid(1001000, [&]() {
        ASSERT_RESULT_OK(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener));
    }));

    /*
     * Package "system.daemon" (UID: 1001000) exceeds warn threshold percentage of 80% but no
     * warning is issued as it is a native UID.
     */
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/70'000, /*bgWrBytes=*/60'000, 0, 0)},
             {1112345, IoUsage(0, 0, /*fgWrBytes=*/35'000, /*bgWrBytes=*/15'000, 0, 0)},
             {1212345, IoUsage(0, 0, /*fgWrBytes=*/90'000, /*bgWrBytes=*/20'000, 0, 0)}});

    ResourceOveruseStats actualOverusingNativeStats;
    EXPECT_CALL(*mockResourceOveruseListener, onOveruse(_))
            .WillOnce(DoAll(SaveArg<0>(&actualOverusingNativeStats), Return(Status::ok())));
    std::vector<PackageIoOveruseStats> actualIoOveruseStats;
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualIoOveruseStats), Return(Status::ok())));

    time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto [startTime, durationInSeconds] = calculateStartAndDuration(currentTime);

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::GARAGE_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    const auto expectedOverusingNativeStats = constructResourceOveruseStats(
            constructIoOveruseStats(/*isKillable=*/false,
                                    /*remaining=*/constructPerStateBytes(80'000, 40'000, 0),
                                    /*written=*/constructPerStateBytes(0, 0, 130'000),
                                    /*totalOveruses=*/1, startTime, durationInSeconds));
    EXPECT_THAT(actualOverusingNativeStats, expectedOverusingNativeStats)
            << "Expected: " << expectedOverusingNativeStats.toString()
            << "\nActual: " << actualOverusingNativeStats.toString();

    const std::vector<PackageIoOveruseStats> expectedIoOveruseStats =
            {constructPackageIoOveruseStats(/*uid*=*/1001000, /*shouldNotify=*/true,
                                            /*isKillable=*/false, /*remaining=*/
                                            constructPerStateBytes(80'000, 40'000, 0),
                                            /*written=*/constructPerStateBytes(0, 0, 130'000),
                                            /*totalOveruses=*/1, startTime, durationInSeconds),
             constructPackageIoOveruseStats(/*uid*=*/1112345, /*shouldNotify=*/false,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(70'000, 30'000, 50'000),
                                            /*written=*/constructPerStateBytes(0, 0, 50'000),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             // Exceeds threshold.
             constructPackageIoOveruseStats(/*uid*=*/1212345, /*shouldNotify=*/true,
                                            /*isKillable=*/true,
                                            /*remaining=*/
                                            constructPerStateBytes(70'000, 30'000, 0),
                                            /*written=*/constructPerStateBytes(0, 0, 110'000),
                                            /*totalOveruses=*/1, startTime, durationInSeconds)};
    EXPECT_THAT(actualIoOveruseStats, UnorderedElementsAreArray(expectedIoOveruseStats))
            << "Expected: " << toString(expectedIoOveruseStats)
            << "\nActual: " << toString(actualIoOveruseStats);
}

TEST_F(IoOveruseMonitorTest, TestOnPeriodicCollectionWithZeroWriteBytes) {
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(10, 0, /*fgWrBytes=*/0, /*bgWrBytes=*/0, 1, 0)},
             {1112345, IoUsage(0, 20, /*fgWrBytes=*/0, /*bgWrBytes=*/0, 0, 0)},
             {1212345, IoUsage(0, 00, /*fgWrBytes=*/0, /*bgWrBytes=*/0, 0, 1)}});

    EXPECT_CALL(*mMockPackageInfoResolver, getPackageInfosForUids(_)).Times(0);
    EXPECT_CALL(*mMockIoOveruseConfigs, fetchThreshold(_)).Times(0);
    EXPECT_CALL(*mMockIoOveruseConfigs, isSafeToKill(_)).Times(0);
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_)).Times(0);

    ASSERT_RESULT_OK(
            mIoOveruseMonitor->onPeriodicCollection(std::chrono::system_clock::to_time_t(
                                                            std::chrono::system_clock::now()),
                                                    SystemState::NORMAL_MODE, mockUidIoStats,
                                                    nullptr, nullptr));
}

TEST_F(IoOveruseMonitorTest, TestOnPeriodicCollectionWithSmallWrittenBytes) {
    setUpPackagesAndConfigurations();
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    /*
     * UID 1212345 current written bytes < |KTestMinSyncWrittenBytes| so the UID's stats are not
     * synced.
     */
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(10, 0, /*fgWrBytes=*/59'200, /*bgWrBytes=*/0, 1, 0)},
             {1112345, IoUsage(0, 20, /*fgWrBytes=*/0, /*bgWrBytes=*/25'200, 0, 0)},
             {1212345, IoUsage(0, 00, /*fgWrBytes=*/300, /*bgWrBytes=*/600, 0, 1)},
             {1312345, IoUsage(0, 00, /*fgWrBytes=*/51'200, /*bgWrBytes=*/0, 0, 1)}});

    std::vector<PackageIoOveruseStats> actualIoOveruseStats;
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualIoOveruseStats), Return(Status::ok())));

    time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto [startTime, durationInSeconds] = calculateStartAndDuration(currentTime);

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::NORMAL_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    std::vector<PackageIoOveruseStats> expectedIoOveruseStats =
            {constructPackageIoOveruseStats(/*uid*=*/1001000, /*shouldNotify=*/false,
                                            /*isKillable=*/false, /*remaining=*/
                                            constructPerStateBytes(20'800, 40'000, 100'000),
                                            /*written=*/
                                            constructPerStateBytes(59'200, 0, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             constructPackageIoOveruseStats(/*uid*=*/1112345, /*shouldNotify=*/true,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(70'000, 4'800, 100'000),
                                            /*written=*/constructPerStateBytes(0, 25'200, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             constructPackageIoOveruseStats(/*uid*=*/1312345, /*shouldNotify=*/false,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(18'800, 30'000, 100'000),
                                            /*written=*/constructPerStateBytes(51'200, 0, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds)};

    EXPECT_THAT(actualIoOveruseStats, UnorderedElementsAreArray(expectedIoOveruseStats))
            << "Expected: " << toString(expectedIoOveruseStats)
            << "\nActual: " << toString(actualIoOveruseStats);

    actualIoOveruseStats.clear();
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_))
            .WillOnce(DoAll(SaveArg<0>(&actualIoOveruseStats), Return(Status::ok())));

    /*
     * UID 1001000 current written bytes is < |kTestMinSyncWrittenBytes| but exceeds warn threshold
     * but not killable so the UID's stats are not synced.
     * UID 1112345 current written bytes is < |kTestMinSyncWrittenBytes| but exceeds threshold so
     * the UID's stats are synced.
     * UID 1212345 current written bytes is < |kTestMinSyncWrittenBytes| but total written bytes
     * since last synced > |kTestMinSyncWrittenBytes| so the UID's stats are synced.
     * UID 1312345 current written bytes is < |kTestMinSyncWrittenBytes| but exceeds warn threshold
     * and killable so the UID's stat are synced.
     */
    mockUidIoStats->expectDeltaStats(
            {{1001000,
              IoUsage(10, 0, /*fgWrBytes=*/KTestMinSyncWrittenBytes - 100, /*bgWrBytes=*/0, 1, 0)},
             {1112345,
              IoUsage(0, 20, /*fgWrBytes=*/0, /*bgWrBytes=*/KTestMinSyncWrittenBytes - 100, 0, 0)},
             {1212345,
              IoUsage(0, 00, /*fgWrBytes=*/KTestMinSyncWrittenBytes - 300, /*bgWrBytes=*/0, 0, 1)},
             {1312345,
              IoUsage(0, 00, /*fgWrBytes=*/KTestMinSyncWrittenBytes - 100, /*bgWrBytes=*/0, 0,
                      1)}});

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::NORMAL_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    expectedIoOveruseStats =
            {constructPackageIoOveruseStats(/*uid*=*/1112345, /*shouldNotify=*/true,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(70'000, 0, 100'000),
                                            /*written=*/constructPerStateBytes(0, 30'100, 0),
                                            /*totalOveruses=*/1, startTime, durationInSeconds),
             constructPackageIoOveruseStats(/*uid*=*/1212345, /*shouldNotify=*/false,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(65'000, 29'400, 100'000),
                                            /*written=*/constructPerStateBytes(5'000, 600, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds),
             constructPackageIoOveruseStats(/*uid*=*/1312345, /*shouldNotify=*/true,
                                            /*isKillable=*/true, /*remaining=*/
                                            constructPerStateBytes(13'900, 30'000, 100'000),
                                            /*written=*/constructPerStateBytes(56'100, 0, 0),
                                            /*totalOveruses=*/0, startTime, durationInSeconds)};
    EXPECT_THAT(actualIoOveruseStats, UnorderedElementsAreArray(expectedIoOveruseStats))
            << "Expected: " << toString(expectedIoOveruseStats)
            << "\nActual: " << toString(actualIoOveruseStats);
}

TEST_F(IoOveruseMonitorTest, TestOnPeriodicCollectionWithNoPackageInfo) {
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/70'000, /*bgWrBytes=*/20'000, 0, 0)},
             {1112345, IoUsage(0, 0, /*fgWrBytes=*/35'000, /*bgWrBytes=*/15'000, 0, 0)},
             {1212345, IoUsage(0, 0, /*fgWrBytes=*/70'000, /*bgWrBytes=*/20'000, 0, 0)}});

    ON_CALL(*mMockPackageInfoResolver, getPackageInfosForUids(_))
            .WillByDefault(Return(std::unordered_map<uid_t, PackageInfo>{}));

    EXPECT_CALL(*mMockIoOveruseConfigs, fetchThreshold(_)).Times(0);
    EXPECT_CALL(*mMockIoOveruseConfigs, isSafeToKill(_)).Times(0);
    EXPECT_CALL(*mMockWatchdogServiceHelper, latestIoOveruseStats(_)).Times(0);

    ASSERT_RESULT_OK(
            mIoOveruseMonitor->onPeriodicCollection(std::chrono::system_clock::to_time_t(
                                                            std::chrono::system_clock::now()),
                                                    SystemState::NORMAL_MODE, mockUidIoStats,
                                                    nullptr, nullptr));
}

TEST_F(IoOveruseMonitorTest, TestOnPeriodicMonitor) {
    IIoOveruseConfigs::IoOveruseAlertThresholdSet alertThresholds =
            {toIoOveruseAlertThreshold(
                     /*durationInSeconds=*/10, /*writtenBytesPerSecond=*/15'360),
             toIoOveruseAlertThreshold(
                     /*durationInSeconds=*/17, /*writtenBytesPerSecond=*/10'240),
             toIoOveruseAlertThreshold(
                     /*durationInSeconds=*/23, /*writtenBytesPerSecond=*/7'168)};
    ON_CALL(*mMockIoOveruseConfigs, systemWideAlertThresholds())
            .WillByDefault(ReturnRef(alertThresholds));

    time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto nextCollectionTime = [&]() -> time_t {
        time += kTestMonitorInterval.count();
        return time;
    };
    bool isAlertReceived = false;
    const auto alertHandler = [&]() { isAlertReceived = true; };

    // 1st polling is ignored
    sp<MockProcDiskStats> mockProcDiskStats = sp<MockProcDiskStats>::make();
    EXPECT_CALL(*mockProcDiskStats, deltaSystemWideDiskStats()).Times(0);

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicMonitor(nextCollectionTime(), mockProcDiskStats,
                                                          alertHandler));
    EXPECT_FALSE(isAlertReceived) << "Triggered spurious alert because first polling is ignored";

    // 2nd polling - guarded by the heuristic to handle spurious alerting on partially filled buffer
    EXPECT_CALL(*mockProcDiskStats, deltaSystemWideDiskStats()).WillOnce([]() -> DiskStats {
        return DiskStats{.numKibWritten = 70};
    });

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicMonitor(nextCollectionTime(), mockProcDiskStats,
                                                          alertHandler));
    EXPECT_FALSE(isAlertReceived) << "Triggered spurious alert when not exceeding the threshold";

    // 3rd polling exceeds first threshold
    EXPECT_CALL(*mockProcDiskStats, deltaSystemWideDiskStats()).WillOnce([]() -> DiskStats {
        return DiskStats{.numKibWritten = 90};
    });

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicMonitor(nextCollectionTime(), mockProcDiskStats,
                                                          alertHandler));
    EXPECT_TRUE(isAlertReceived) << "Failed to trigger alert when exceeding the threshold";

    isAlertReceived = false;

    // 4th polling - guarded by the heuristic to handle spurious alerting on partially filled buffer
    EXPECT_CALL(*mockProcDiskStats, deltaSystemWideDiskStats()).WillOnce([]() -> DiskStats {
        return DiskStats{.numKibWritten = 10};
    });

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicMonitor(nextCollectionTime(), mockProcDiskStats,
                                                          alertHandler));
    EXPECT_FALSE(isAlertReceived) << "Triggered spurious alert when not exceeding the threshold";

    // 5th polling exceeds second threshold
    EXPECT_CALL(*mockProcDiskStats, deltaSystemWideDiskStats()).WillOnce([]() -> DiskStats {
        return DiskStats{.numKibWritten = 80};
    });

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicMonitor(nextCollectionTime(), mockProcDiskStats,
                                                          alertHandler));
    EXPECT_TRUE(isAlertReceived) << "Failed to trigger alert when exceeding the threshold";

    isAlertReceived = false;

    // 6th polling exceeds third threshold
    EXPECT_CALL(*mockProcDiskStats, deltaSystemWideDiskStats()).WillOnce([]() -> DiskStats {
        return DiskStats{.numKibWritten = 10};
    });

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicMonitor(nextCollectionTime(), mockProcDiskStats,
                                                          alertHandler));
    EXPECT_TRUE(isAlertReceived) << "Failed to trigger alert when exceeding the threshold";
}

TEST_F(IoOveruseMonitorTest, TestRegisterResourceOveruseListener) {
    sp<MockResourceOveruseListener> mockResourceOveruseListener =
            sp<MockResourceOveruseListener>::make();

    ASSERT_RESULT_OK(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener));

    ASSERT_RESULT_OK(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener));
}

TEST_F(IoOveruseMonitorTest, TestErrorsRegisterResourceOveruseListenerOnLinkToDeathError) {
    sp<MockResourceOveruseListener> mockResourceOveruseListener =
            sp<MockResourceOveruseListener>::make();

    mockResourceOveruseListener->injectLinkToDeathFailure();

    ASSERT_FALSE(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener).ok());
}

TEST_F(IoOveruseMonitorTest, TestUnaddIoOveruseListener) {
    sp<MockResourceOveruseListener> mockResourceOveruseListener =
            sp<MockResourceOveruseListener>::make();

    ASSERT_RESULT_OK(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener));

    ASSERT_RESULT_OK(mIoOveruseMonitor->removeIoOveruseListener(mockResourceOveruseListener));

    ASSERT_FALSE(mIoOveruseMonitor->removeIoOveruseListener(mockResourceOveruseListener).ok())
            << "Should error on duplicate unregister";
}

TEST_F(IoOveruseMonitorTest, TestUnaddIoOveruseListenerOnUnlinkToDeathError) {
    sp<MockResourceOveruseListener> mockResourceOveruseListener =
            sp<MockResourceOveruseListener>::make();

    ASSERT_RESULT_OK(mIoOveruseMonitor->addIoOveruseListener(mockResourceOveruseListener));

    mockResourceOveruseListener->injectUnlinkToDeathFailure();

    ASSERT_RESULT_OK(mIoOveruseMonitor->removeIoOveruseListener(mockResourceOveruseListener));
}

TEST_F(IoOveruseMonitorTest, TestGetIoOveruseStats) {
    setUpPackagesAndConfigurations();
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/90'000, /*bgWrBytes=*/20'000, 0, 0)}});

    time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto [startTime, durationInSeconds] = calculateStartAndDuration(currentTime);

    ASSERT_RESULT_OK(mIoOveruseMonitor->onPeriodicCollection(currentTime, SystemState::NORMAL_MODE,
                                                             mockUidIoStats, nullptr, nullptr));

    const auto expected =
            constructIoOveruseStats(/*isKillable=*/false,
                                    /*remaining=*/
                                    constructPerStateBytes(80'000, 40'000, 100'000),
                                    /*written=*/
                                    constructPerStateBytes(90'000, 20'000, 0),
                                    /*totalOveruses=*/1, startTime, durationInSeconds);
    IoOveruseStats actual;
    ASSERT_NO_FATAL_FAILURE(executeAsUid(1001000, [&]() {
        ASSERT_RESULT_OK(mIoOveruseMonitor->getIoOveruseStats(&actual));
    }));
    EXPECT_THAT(actual, expected) << "Expected: " << expected.toString()
                                  << "\nActual: " << actual.toString();
}

TEST_F(IoOveruseMonitorTest, TestResetIoOveruseStats) {
    setUpPackagesAndConfigurations();
    sp<MockUidIoStats> mockUidIoStats = sp<MockUidIoStats>::make();
    mockUidIoStats->expectDeltaStats(
            {{1001000, IoUsage(0, 0, /*fgWrBytes=*/90'000, /*bgWrBytes=*/20'000, 0, 0)}});

    ASSERT_RESULT_OK(
            mIoOveruseMonitor->onPeriodicCollection(std::chrono::system_clock::to_time_t(
                                                            std::chrono::system_clock::now()),
                                                    SystemState::NORMAL_MODE, mockUidIoStats,
                                                    nullptr, nullptr));

    IoOveruseStats actual;
    ASSERT_NO_FATAL_FAILURE(executeAsUid(1001000, [&]() {
        ASSERT_RESULT_OK(mIoOveruseMonitor->getIoOveruseStats(&actual));
    }));

    EXPECT_NE(actual.totalOveruses, 0);
    EXPECT_NE(actual.writtenBytes.foregroundBytes, 0);
    EXPECT_NE(actual.writtenBytes.backgroundBytes, 0);

    std::vector<std::string> packageNames = {"system.daemon"};
    EXPECT_CALL(*mMockWatchdogServiceHelper, resetResourceOveruseStats(packageNames))
            .WillOnce(Return(Status::ok()));

    ASSERT_RESULT_OK(mIoOveruseMonitor->resetIoOveruseStats(packageNames));

    ASSERT_NO_FATAL_FAILURE(executeAsUid(1001000, [&]() {
        ASSERT_RESULT_OK(mIoOveruseMonitor->getIoOveruseStats(&actual));
    }));

    EXPECT_EQ(actual.totalOveruses, 0);
    EXPECT_EQ(actual.writtenBytes.foregroundBytes, 0);
    EXPECT_EQ(actual.writtenBytes.backgroundBytes, 0);
}

TEST_F(IoOveruseMonitorTest, TestErrorsResetIoOveruseStatsOnWatchdogServiceHelperError) {
    std::vector<std::string> packageNames = {"system.daemon"};
    EXPECT_CALL(*mMockWatchdogServiceHelper, resetResourceOveruseStats(packageNames))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE)));

    ASSERT_FALSE(mIoOveruseMonitor->resetIoOveruseStats(packageNames).ok())
            << "Must return error when WatchdogServiceHelper fails to reset stats";
}

TEST_F(IoOveruseMonitorTest, TestErrorsGetIoOveruseStatsOnNoStats) {
    ON_CALL(*mMockPackageInfoResolver, getPackageInfosForUids(_))
            .WillByDefault([]() -> std::unordered_map<uid_t, PackageInfo> {
                return {{1001000,
                         constructPackageInfo(/*packageName=*/"system.daemon", /*uid=*/1001000,
                                              UidType::NATIVE)}};
            });
    IoOveruseStats actual;
    ASSERT_NO_FATAL_FAILURE(executeAsUid(1001000, [&]() {
        ASSERT_FALSE(mIoOveruseMonitor->getIoOveruseStats(&actual).ok())
                << "Should fail on missing I/O overuse stats";
    }));

    ASSERT_NO_FATAL_FAILURE(executeAsUid(1102001, [&]() {
        ASSERT_FALSE(mIoOveruseMonitor->getIoOveruseStats(&actual).ok())
                << "Should fail on missing package information";
    }));
}

TEST_F(IoOveruseMonitorTest, TestUpdateResourceOveruseConfigurations) {
    EXPECT_CALL(*mMockIoOveruseConfigs, update(_)).WillOnce(Return(Result<void>{}));
    EXPECT_CALL(*mMockIoOveruseConfigs, writeToDisk()).WillOnce(Return(Result<void>{}));

    ASSERT_RESULT_OK(mIoOveruseMonitor->updateResourceOveruseConfigurations({}));
}

TEST_F(IoOveruseMonitorTest, TestFailsUpdateResourceOveruseConfigurations) {
    EXPECT_CALL(*mMockIoOveruseConfigs, update(_))
            .WillOnce([&]([[maybe_unused]] const std::vector<ResourceOveruseConfiguration>& configs)
                              -> Result<void> { return Error() << "Failed to update"; });
    EXPECT_CALL(*mMockIoOveruseConfigs, writeToDisk()).Times(0);

    ASSERT_FALSE(mIoOveruseMonitor->updateResourceOveruseConfigurations({}).ok());
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
