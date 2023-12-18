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

#include "IoPerfCollection.h"
#include "MockPackageInfoResolver.h"
#include "MockProcPidStat.h"
#include "MockProcStat.h"
#include "MockUidIoStats.h"
#include "MockWatchdogServiceHelper.h"
#include "PackageInfoResolver.h"

#include <WatchdogProperties.sysprop.h>
#include <android-base/file.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::sp;
using ::android::base::Error;
using ::android::base::ReadFdToString;
using ::android::base::Result;
using ::testing::_;
using ::testing::Return;

namespace {

bool isEqual(const UidIoPerfData& lhs, const UidIoPerfData& rhs) {
    if (lhs.topNReads.size() != rhs.topNReads.size() ||
        lhs.topNWrites.size() != rhs.topNWrites.size()) {
        return false;
    }
    for (int i = 0; i < METRIC_TYPES; ++i) {
        for (int j = 0; j < UID_STATES; ++j) {
            if (lhs.total[i][j] != rhs.total[i][j]) {
                return false;
            }
        }
    }
    auto comp = [&](const UidIoPerfData::Stats& l, const UidIoPerfData::Stats& r) -> bool {
        bool isEqual = l.userId == r.userId && l.packageName == r.packageName;
        for (int i = 0; i < UID_STATES; ++i) {
            isEqual &= l.bytes[i] == r.bytes[i] && l.fsync[i] == r.fsync[i];
        }
        return isEqual;
    };
    return lhs.topNReads.size() == rhs.topNReads.size() &&
            std::equal(lhs.topNReads.begin(), lhs.topNReads.end(), rhs.topNReads.begin(), comp) &&
            lhs.topNWrites.size() == rhs.topNWrites.size() &&
            std::equal(lhs.topNWrites.begin(), lhs.topNWrites.end(), rhs.topNWrites.begin(), comp);
}

bool isEqual(const SystemIoPerfData& lhs, const SystemIoPerfData& rhs) {
    return lhs.cpuIoWaitTime == rhs.cpuIoWaitTime && lhs.totalCpuTime == rhs.totalCpuTime &&
            lhs.ioBlockedProcessesCnt == rhs.ioBlockedProcessesCnt &&
            lhs.totalProcessesCnt == rhs.totalProcessesCnt;
}

bool isEqual(const ProcessIoPerfData& lhs, const ProcessIoPerfData& rhs) {
    if (lhs.topNIoBlockedUids.size() != rhs.topNIoBlockedUids.size() ||
        lhs.topNMajorFaultUids.size() != rhs.topNMajorFaultUids.size() ||
        lhs.totalMajorFaults != rhs.totalMajorFaults ||
        lhs.majorFaultsPercentChange != rhs.majorFaultsPercentChange) {
        return false;
    }
    auto comp = [&](const ProcessIoPerfData::UidStats& l,
                    const ProcessIoPerfData::UidStats& r) -> bool {
        auto comp = [&](const ProcessIoPerfData::UidStats::ProcessStats& l,
                        const ProcessIoPerfData::UidStats::ProcessStats& r) -> bool {
            return l.comm == r.comm && l.count == r.count;
        };
        return l.userId == r.userId && l.packageName == r.packageName && l.count == r.count &&
                l.topNProcesses.size() == r.topNProcesses.size() &&
                std::equal(l.topNProcesses.begin(), l.topNProcesses.end(), r.topNProcesses.begin(),
                           comp);
    };
    return lhs.topNIoBlockedUids.size() == lhs.topNIoBlockedUids.size() &&
            std::equal(lhs.topNIoBlockedUids.begin(), lhs.topNIoBlockedUids.end(),
                       rhs.topNIoBlockedUids.begin(), comp) &&
            lhs.topNIoBlockedUidsTotalTaskCnt.size() == rhs.topNIoBlockedUidsTotalTaskCnt.size() &&
            std::equal(lhs.topNIoBlockedUidsTotalTaskCnt.begin(),
                       lhs.topNIoBlockedUidsTotalTaskCnt.end(),
                       rhs.topNIoBlockedUidsTotalTaskCnt.begin()) &&
            lhs.topNMajorFaultUids.size() == rhs.topNMajorFaultUids.size() &&
            std::equal(lhs.topNMajorFaultUids.begin(), lhs.topNMajorFaultUids.end(),
                       rhs.topNMajorFaultUids.begin(), comp);
}

bool isEqual(const IoPerfRecord& lhs, const IoPerfRecord& rhs) {
    return isEqual(lhs.uidIoPerfData, rhs.uidIoPerfData) &&
            isEqual(lhs.systemIoPerfData, rhs.systemIoPerfData) &&
            isEqual(lhs.processIoPerfData, rhs.processIoPerfData);
}

int countOccurrences(std::string str, std::string subStr) {
    size_t pos = 0;
    int occurrences = 0;
    while ((pos = str.find(subStr, pos)) != std::string::npos) {
        ++occurrences;
        pos += subStr.length();
    }
    return occurrences;
}

}  // namespace

namespace internal {

class IoPerfCollectionPeer {
public:
    explicit IoPerfCollectionPeer(sp<IoPerfCollection> collector) :
          mCollector(collector),
          mMockPackageInfoResolver(new MockPackageInfoResolver()) {
        mCollector->mPackageInfoResolver = mMockPackageInfoResolver;
    }

    IoPerfCollectionPeer() = delete;
    ~IoPerfCollectionPeer() {
        mCollector->terminate();
        mCollector.clear();
        mMockPackageInfoResolver.clear();
    }

    Result<void> init() { return mCollector->init(); }

    void setTopNStatsPerCategory(int value) { mCollector->mTopNStatsPerCategory = value; }

    void setTopNStatsPerSubcategory(int value) { mCollector->mTopNStatsPerSubcategory = value; }

    void injectUidToPackageNameMapping(std::unordered_map<uid_t, std::string> mapping) {
        EXPECT_CALL(*mMockPackageInfoResolver, getPackageNamesForUids(_))
                .WillRepeatedly(Return(mapping));
    }

    const CollectionInfo& getBoottimeCollectionInfo() {
        Mutex::Autolock lock(mCollector->mMutex);
        return mCollector->mBoottimeCollection;
    }

    const CollectionInfo& getPeriodicCollectionInfo() {
        Mutex::Autolock lock(mCollector->mMutex);
        return mCollector->mPeriodicCollection;
    }

    const CollectionInfo& getCustomCollectionInfo() {
        Mutex::Autolock lock(mCollector->mMutex);
        return mCollector->mCustomCollection;
    }

private:
    sp<IoPerfCollection> mCollector;
    sp<MockPackageInfoResolver> mMockPackageInfoResolver;
};

}  // namespace internal

TEST(IoPerfCollectionTest, TestBoottimeCollection) {
    sp<MockUidIoStats> mockUidIoStats = new MockUidIoStats();
    sp<MockProcStat> mockProcStat = new MockProcStat();
    sp<MockProcPidStat> mockProcPidStat = new MockProcPidStat();

    sp<IoPerfCollection> collector = new IoPerfCollection();
    internal::IoPerfCollectionPeer collectorPeer(collector);

    ASSERT_RESULT_OK(collectorPeer.init());

    const std::unordered_map<uid_t, UidIoUsage> uidIoUsages({
            {1009, {.uid = 1009, .ios = {0, 14000, 0, 16000, 0, 100}}},
    });
    const ProcStatInfo procStatInfo{
            /*stats=*/{2900, 7900, 4900, 8900, /*ioWaitTime=*/5900, 6966, 7980, 0, 0, 2930},
            /*runnableCnt=*/100,
            /*ioBlockedCnt=*/57,
    };
    const std::vector<ProcessStats> processStats({
            {.tgid = 100,
             .uid = 1009,
             .process = {100, "disk I/O", "D", 1, 11000, 1, 234},
             .threads = {{100, {100, "mount", "D", 1, 11000, 1, 234}}}},
    });

    EXPECT_CALL(*mockUidIoStats, deltaStats()).WillOnce(Return(uidIoUsages));
    EXPECT_CALL(*mockProcStat, deltaStats()).WillOnce(Return(procStatInfo));
    EXPECT_CALL(*mockProcPidStat, deltaStats()).WillOnce(Return(processStats));

    const IoPerfRecord expected = {
            .uidIoPerfData = {.topNReads = {{0, "mount", {0, 14000}, {0, 100}}},
                              .topNWrites = {{0, "mount", {0, 16000}, {0, 100}}},
                              .total = {{0, 14000}, {0, 16000}, {0, 100}}},
            .systemIoPerfData = {5900, 48376, 57, 157},
            .processIoPerfData =
                    {.topNIoBlockedUids = {{0, "mount", 1, {{"disk I/O", 1}}}},
                     .topNIoBlockedUidsTotalTaskCnt = {1},
                     .topNMajorFaultUids = {{0, "mount", 11000, {{"disk I/O", 11000}}}},
                     .totalMajorFaults = 11000,
                     .majorFaultsPercentChange = 0},
    };
    collectorPeer.injectUidToPackageNameMapping({{1009, "mount"}});

    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ASSERT_RESULT_OK(
            collector->onBoottimeCollection(now, mockUidIoStats, mockProcStat, mockProcPidStat));

    const CollectionInfo& collectionInfo = collectorPeer.getBoottimeCollectionInfo();

    ASSERT_EQ(collectionInfo.maxCacheSize, std::numeric_limits<std::size_t>::max());
    ASSERT_EQ(collectionInfo.records.size(), 1);
    ASSERT_TRUE(isEqual(collectionInfo.records[0], expected))
            << "Boottime collection record doesn't match.\nExpected:\n"
            << toString(expected) << "\nActual:\n"
            << toString(collectionInfo.records[0]);

    TemporaryFile dump;
    ASSERT_RESULT_OK(collector->onDump(dump.fd));

    lseek(dump.fd, 0, SEEK_SET);
    std::string dumpContents;
    ASSERT_TRUE(ReadFdToString(dump.fd, &dumpContents));
    ASSERT_FALSE(dumpContents.empty());

    ASSERT_EQ(countOccurrences(dumpContents, kEmptyCollectionMessage), 1)
            << "Only periodic collection should be not collected. Dump contents: " << dumpContents;
}

TEST(IoPerfCollectionTest, TestPeriodicCollection) {
    sp<MockUidIoStats> mockUidIoStats = new MockUidIoStats();
    sp<MockProcStat> mockProcStat = new MockProcStat();
    sp<MockProcPidStat> mockProcPidStat = new MockProcPidStat();

    sp<IoPerfCollection> collector = new IoPerfCollection();
    internal::IoPerfCollectionPeer collectorPeer(collector);

    ASSERT_RESULT_OK(collectorPeer.init());

    const std::unordered_map<uid_t, UidIoUsage> uidIoUsages({
            {1009, {.uid = 1009, .ios = {0, 14000, 0, 16000, 0, 100}}},
    });
    const ProcStatInfo procStatInfo{
            /*stats=*/{2900, 7900, 4900, 8900, /*ioWaitTime=*/5900, 6966, 7980, 0, 0, 2930},
            /*runnableCnt=*/100,
            /*ioBlockedCnt=*/57,
    };
    const std::vector<ProcessStats> processStats({
            {.tgid = 100,
             .uid = 1009,
             .process = {100, "disk I/O", "D", 1, 11000, 1, 234},
             .threads = {{100, {100, "mount", "D", 1, 11000, 1, 234}}}},
    });

    EXPECT_CALL(*mockUidIoStats, deltaStats()).WillOnce(Return(uidIoUsages));
    EXPECT_CALL(*mockProcStat, deltaStats()).WillOnce(Return(procStatInfo));
    EXPECT_CALL(*mockProcPidStat, deltaStats()).WillOnce(Return(processStats));

    const IoPerfRecord expected = {
            .uidIoPerfData = {.topNReads = {{0, "mount", {0, 14000}, {0, 100}}},
                              .topNWrites = {{0, "mount", {0, 16000}, {0, 100}}},
                              .total = {{0, 14000}, {0, 16000}, {0, 100}}},
            .systemIoPerfData = {5900, 48376, 57, 157},
            .processIoPerfData =
                    {.topNIoBlockedUids = {{0, "mount", 1, {{"disk I/O", 1}}}},
                     .topNIoBlockedUidsTotalTaskCnt = {1},
                     .topNMajorFaultUids = {{0, "mount", 11000, {{"disk I/O", 11000}}}},
                     .totalMajorFaults = 11000,
                     .majorFaultsPercentChange = 0},
    };

    collectorPeer.injectUidToPackageNameMapping({{1009, "mount"}});

    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ASSERT_RESULT_OK(collector->onPeriodicCollection(now, SystemState::NORMAL_MODE, mockUidIoStats,
                                                     mockProcStat, mockProcPidStat));

    const CollectionInfo& collectionInfo = collectorPeer.getPeriodicCollectionInfo();

    ASSERT_EQ(collectionInfo.maxCacheSize,
              static_cast<size_t>(sysprop::periodicCollectionBufferSize().value_or(
                      kDefaultPeriodicCollectionBufferSize)));
    ASSERT_EQ(collectionInfo.records.size(), 1);
    ASSERT_TRUE(isEqual(collectionInfo.records[0], expected))
            << "Periodic collection record doesn't match.\nExpected:\n"
            << toString(expected) << "\nActual:\n"
            << toString(collectionInfo.records[0]);

    TemporaryFile dump;
    ASSERT_RESULT_OK(collector->onDump(dump.fd));

    lseek(dump.fd, 0, SEEK_SET);
    std::string dumpContents;
    ASSERT_TRUE(ReadFdToString(dump.fd, &dumpContents));
    ASSERT_FALSE(dumpContents.empty());

    ASSERT_EQ(countOccurrences(dumpContents, kEmptyCollectionMessage), 1)
            << "Only boot-time collection should be not collected. Dump contents: " << dumpContents;
}

TEST(IoPerfCollectionTest, TestCustomCollection) {
    sp<MockUidIoStats> mockUidIoStats = new MockUidIoStats();
    sp<MockProcStat> mockProcStat = new MockProcStat();
    sp<MockProcPidStat> mockProcPidStat = new MockProcPidStat();

    sp<IoPerfCollection> collector = new IoPerfCollection();
    internal::IoPerfCollectionPeer collectorPeer(collector);

    ASSERT_RESULT_OK(collectorPeer.init());

    // Filter by package name should ignore this limit.
    collectorPeer.setTopNStatsPerCategory(1);

    const std::unordered_map<uid_t, UidIoUsage> uidIoUsages({
            {1009, {.uid = 1009, .ios = {0, 14000, 0, 16000, 0, 100}}},
            {2001, {.uid = 2001, .ios = {0, 3400, 0, 6700, 0, 200}}},
            {3456, {.uid = 3456, .ios = {0, 4200, 0, 5600, 0, 300}}},
    });
    const ProcStatInfo procStatInfo{
            /*stats=*/{2900, 7900, 4900, 8900, /*ioWaitTime=*/5900, 6966, 7980, 0, 0, 2930},
            /*runnableCnt=*/100,
            /*ioBlockedCnt=*/57,
    };
    const std::vector<ProcessStats> processStats({
            {.tgid = 100,
             .uid = 1009,
             .process = {100, "cts_test", "D", 1, 50900, 2, 234},
             .threads = {{100, {100, "cts_test", "D", 1, 50900, 1, 234}},
                         {200, {200, "cts_test_2", "D", 1, 0, 1, 290}}}},
            {.tgid = 1000,
             .uid = 2001,
             .process = {1000, "system_server", "D", 1, 1234, 1, 345},
             .threads = {{1000, {1000, "system_server", "D", 1, 1234, 1, 345}}}},
            {.tgid = 4000,
             .uid = 3456,
             .process = {4000, "random_process", "D", 1, 3456, 1, 890},
             .threads = {{4000, {4000, "random_process", "D", 1, 50900, 1, 890}}}},
    });

    EXPECT_CALL(*mockUidIoStats, deltaStats()).WillOnce(Return(uidIoUsages));
    EXPECT_CALL(*mockProcStat, deltaStats()).WillOnce(Return(procStatInfo));
    EXPECT_CALL(*mockProcPidStat, deltaStats()).WillOnce(Return(processStats));
    const IoPerfRecord expected = {
            .uidIoPerfData = {.topNReads = {{.userId = 0,
                                             .packageName = "android.car.cts",
                                             .bytes = {0, 14000},
                                             .fsync = {0, 100}},
                                            {.userId = 0,
                                             .packageName = "system_server",
                                             .bytes = {0, 3400},
                                             .fsync = {0, 200}}},
                              .topNWrites = {{.userId = 0,
                                              .packageName = "android.car.cts",
                                              .bytes = {0, 16000},
                                              .fsync = {0, 100}},
                                             {.userId = 0,
                                              .packageName = "system_server",
                                              .bytes = {0, 6700},
                                              .fsync = {0, 200}}},
                              .total = {{0, 21600}, {0, 28300}, {0, 600}}},
            .systemIoPerfData = {.cpuIoWaitTime = 5900,
                                 .totalCpuTime = 48376,
                                 .ioBlockedProcessesCnt = 57,
                                 .totalProcessesCnt = 157},
            .processIoPerfData =
                    {.topNIoBlockedUids = {{0, "android.car.cts", 2, {{"cts_test", 2}}},
                                           {0, "system_server", 1, {{"system_server", 1}}}},
                     .topNIoBlockedUidsTotalTaskCnt = {2, 1},
                     .topNMajorFaultUids = {{0, "android.car.cts", 50900, {{"cts_test", 50900}}},
                                            {0, "system_server", 1234, {{"system_server", 1234}}}},
                     .totalMajorFaults = 55590,
                     .majorFaultsPercentChange = 0},
    };
    collectorPeer.injectUidToPackageNameMapping({
            {1009, "android.car.cts"},
            {2001, "system_server"},
            {3456, "random_process"},
    });

    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ASSERT_RESULT_OK(collector->onCustomCollection(now, SystemState::NORMAL_MODE,
                                                   {"android.car.cts", "system_server"},
                                                   mockUidIoStats, mockProcStat, mockProcPidStat));

    const CollectionInfo& collectionInfo = collectorPeer.getCustomCollectionInfo();

    EXPECT_EQ(collectionInfo.maxCacheSize, std::numeric_limits<std::size_t>::max());
    ASSERT_EQ(collectionInfo.records.size(), 1);
    ASSERT_TRUE(isEqual(collectionInfo.records[0], expected))
            << "Custom collection record doesn't match.\nExpected:\n"
            << toString(expected) << "\nActual:\n"
            << toString(collectionInfo.records[0]);

    TemporaryFile customDump;
    ASSERT_RESULT_OK(collector->onCustomCollectionDump(customDump.fd));

    lseek(customDump.fd, 0, SEEK_SET);
    std::string customDumpContents;
    ASSERT_TRUE(ReadFdToString(customDump.fd, &customDumpContents));
    ASSERT_FALSE(customDumpContents.empty());
    ASSERT_EQ(countOccurrences(customDumpContents, kEmptyCollectionMessage), 0)
            << "Custom collection should be reported. Dump contents: " << customDumpContents;

    // Should clear the cache.
    ASSERT_RESULT_OK(collector->onCustomCollectionDump(-1));

    const CollectionInfo& emptyCollectionInfo = collectorPeer.getCustomCollectionInfo();
    EXPECT_TRUE(emptyCollectionInfo.records.empty());
    EXPECT_EQ(emptyCollectionInfo.maxCacheSize, std::numeric_limits<std::size_t>::max());
}

TEST(IoPerfCollectionTest, TestUidIoStatsGreaterThanTopNStatsLimit) {
    std::unordered_map<uid_t, UidIoUsage> uidIoUsages({
            {1001234, {.uid = 1001234, .ios = {3000, 0, 500, 0, 20, 0}}},
            {1005678, {.uid = 1005678, .ios = {30, 100, 50, 200, 45, 60}}},
            {1009, {.uid = 1009, .ios = {0, 20000, 0, 30000, 0, 300}}},
            {1001000, {.uid = 1001000, .ios = {2000, 200, 1000, 100, 50, 10}}},
    });
    sp<MockUidIoStats> mockUidIoStats = new MockUidIoStats();
    EXPECT_CALL(*mockUidIoStats, deltaStats()).WillOnce(Return(uidIoUsages));

    struct UidIoPerfData expectedUidIoPerfData = {
            .topNReads = {{.userId = 0,  // uid: 1009
                           .packageName = "mount",
                           .bytes = {0, 20000},
                           .fsync = {0, 300}},
                          {.userId = 10,  // uid: 1001234
                           .packageName = "1001234",
                           .bytes = {3000, 0},
                           .fsync = {20, 0}}},
            .topNWrites = {{.userId = 0,  // uid: 1009
                            .packageName = "mount",
                            .bytes = {0, 30000},
                            .fsync = {0, 300}},
                           {.userId = 10,  // uid: 1001000
                            .packageName = "shared:android.uid.system",
                            .bytes = {1000, 100},
                            .fsync = {50, 10}}},
            .total = {{5030, 20300}, {1550, 30300}, {115, 370}},
    };

    IoPerfCollection collector;
    collector.mTopNStatsPerCategory = 2;

    sp<MockPackageInfoResolver> mockPackageInfoResolver = new MockPackageInfoResolver();
    collector.mPackageInfoResolver = mockPackageInfoResolver;
    EXPECT_CALL(*mockPackageInfoResolver, getPackageNamesForUids(_))
            .WillRepeatedly(Return<std::unordered_map<uid_t, std::string>>(
                    {{1009, "mount"}, {1001000, "shared:android.uid.system"}}));

    struct UidIoPerfData actualUidIoPerfData = {};
    collector.processUidIoPerfData({}, mockUidIoStats, &actualUidIoPerfData);

    EXPECT_TRUE(isEqual(expectedUidIoPerfData, actualUidIoPerfData))
        << "First snapshot doesn't match.\nExpected:\n"
        << toString(expectedUidIoPerfData) << "\nActual:\n"
        << toString(actualUidIoPerfData);

    uidIoUsages = {
            {1001234, {.uid = 1001234, .ios = {4000, 0, 450, 0, 25, 0}}},
            {1005678, {.uid = 1005678, .ios = {10, 900, 0, 400, 5, 10}}},
            {1003456, {.uid = 1003456, .ios = {200, 0, 300, 0, 50, 0}}},
            {1001000, {.uid = 1001000, .ios = {0, 0, 0, 0, 0, 0}}},
    };
    EXPECT_CALL(*mockUidIoStats, deltaStats()).WillOnce(Return(uidIoUsages));

    expectedUidIoPerfData = {
            .topNReads = {{.userId = 10,  // uid: 1001234
                           .packageName = "1001234",
                           .bytes = {4000, 0},
                           .fsync = {25, 0}},
                          {.userId = 10,  // uid: 1005678
                           .packageName = "1005678",
                           .bytes = {10, 900},
                           .fsync = {5, 10}}},
            .topNWrites = {{.userId = 10,  // uid: 1001234
                            .packageName = "1001234",
                            .bytes = {450, 0},
                            .fsync = {25, 0}},
                           {.userId = 10,  // uid: 1005678
                            .packageName = "1005678",
                            .bytes = {0, 400},
                            .fsync = {5, 10}}},
            .total = {{4210, 900}, {750, 400}, {80, 10}},
    };
    actualUidIoPerfData = {};
    collector.processUidIoPerfData({}, mockUidIoStats, &actualUidIoPerfData);

    EXPECT_TRUE(isEqual(expectedUidIoPerfData, actualUidIoPerfData))
        << "Second snapshot doesn't match.\nExpected:\n"
        << toString(expectedUidIoPerfData) << "\nActual:\n"
        << toString(actualUidIoPerfData);
}

TEST(IoPerfCollectionTest, TestUidIOStatsLessThanTopNStatsLimit) {
    const std::unordered_map<uid_t, UidIoUsage> uidIoUsages(
            {{1001234, {.uid = 1001234, .ios = {3000, 0, 500, 0, 20, 0}}}});

    const struct UidIoPerfData expectedUidIoPerfData = {
            .topNReads = {{.userId = 10,
                           .packageName = "1001234",
                           .bytes = {3000, 0},
                           .fsync = {20, 0}}},
            .topNWrites =
                    {{.userId = 10, .packageName = "1001234", .bytes = {500, 0}, .fsync = {20, 0}}},
            .total = {{3000, 0}, {500, 0}, {20, 0}},
    };

    sp<MockUidIoStats> mockUidIoStats = new MockUidIoStats();
    EXPECT_CALL(*mockUidIoStats, deltaStats()).WillOnce(Return(uidIoUsages));

    IoPerfCollection collector;
    collector.mTopNStatsPerCategory = 10;

    struct UidIoPerfData actualUidIoPerfData = {};
    collector.processUidIoPerfData({}, mockUidIoStats, &actualUidIoPerfData);

    EXPECT_TRUE(isEqual(expectedUidIoPerfData, actualUidIoPerfData))
        << "Collected data doesn't match.\nExpected:\n"
        << toString(expectedUidIoPerfData) << "\nActual:\n"
        << toString(actualUidIoPerfData);
}

TEST(IoPerfCollectionTest, TestProcessSystemIoPerfData) {
    const ProcStatInfo procStatInfo(
            /*stats=*/{6200, 5700, 1700, 3100, 1100, 5200, 3900, 0, 0, 0},
            /*runnableCnt=*/17,
            /*ioBlockedCnt=*/5);
    struct SystemIoPerfData expectedSystemIoPerfData = {
            .cpuIoWaitTime = 1100,
            .totalCpuTime = 26900,
            .ioBlockedProcessesCnt = 5,
            .totalProcessesCnt = 22,
    };

    sp<MockProcStat> mockProcStat = new MockProcStat();
    EXPECT_CALL(*mockProcStat, deltaStats()).WillOnce(Return(procStatInfo));

    IoPerfCollection collector;
    struct SystemIoPerfData actualSystemIoPerfData = {};
    collector.processSystemIoPerfData(mockProcStat, &actualSystemIoPerfData);

    EXPECT_TRUE(isEqual(expectedSystemIoPerfData, actualSystemIoPerfData))
            << "Expected:\n"
            << toString(expectedSystemIoPerfData) << "\nActual:\n"
            << toString(actualSystemIoPerfData);
}

TEST(IoPerfCollectionTest, TestProcPidContentsGreaterThanTopNStatsLimit) {
    const std::vector<ProcessStats> firstProcessStats({
            {.tgid = 1,
             .uid = 0,
             .process = {1, "init", "S", 0, 220, 2, 0},
             .threads = {{1, {1, "init", "S", 0, 200, 2, 0}},
                         {453, {453, "init", "S", 0, 20, 2, 275}}}},
            {.tgid = 2456,
             .uid = 1001000,
             .process = {2456, "system_server", "R", 1, 6000, 3, 1000},
             .threads = {{2456, {2456, "system_server", "R", 1, 1000, 3, 1000}},
                         {3456, {3456, "system_server", "S", 1, 3000, 3, 2300}},
                         {4789, {4789, "system_server", "D", 1, 2000, 3, 4500}}}},
            {.tgid = 7890,
             .uid = 1001000,
             .process = {7890, "logd", "D", 1, 15000, 3, 2345},
             .threads = {{7890, {7890, "logd", "D", 1, 10000, 3, 2345}},
                         {8978, {8978, "logd", "D", 1, 1000, 3, 2500}},
                         {12890, {12890, "logd", "D", 1, 500, 3, 2900}}}},
            {.tgid = 18902,
             .uid = 1009,
             .process = {18902, "disk I/O", "D", 1, 45678, 3, 897654},
             .threads = {{18902, {18902, "disk I/O", "D", 1, 30000, 3, 897654}},
                         {21345, {21345, "disk I/O", "D", 1, 15000, 3, 904000}},
                         {32452, {32452, "disk I/O", "D", 1, 678, 3, 1007000}}}},
            {.tgid = 28900,
             .uid = 1001234,
             .process = {28900, "tombstoned", "D", 1, 89765, 1, 2345671},
             .threads = {{28900, {28900, "tombstoned", "D", 1, 89765, 1, 2345671}}}},
    });
    sp<MockProcPidStat> mockProcPidStat = new MockProcPidStat();
    EXPECT_CALL(*mockProcPidStat, deltaStats()).WillOnce(Return(firstProcessStats));

    struct ProcessIoPerfData expectedProcessIoPerfData = {
            .topNIoBlockedUids = {{.userId = 10,  // uid: 1001000
                                   .packageName = "shared:android.uid.system",
                                   .count = 4,
                                   .topNProcesses = {{"logd", 3}, {"system_server", 1}}},
                                  {.userId = 0,
                                   .packageName = "mount",
                                   .count = 3,
                                   .topNProcesses = {{"disk I/O", 3}}}},
            .topNIoBlockedUidsTotalTaskCnt = {6, 3},
            .topNMajorFaultUids = {{.userId = 10,  // uid: 1001234
                                    .packageName = "1001234",
                                    .count = 89765,
                                    .topNProcesses = {{"tombstoned", 89765}}},
                                   {.userId = 0,  // uid: 1009
                                    .packageName = "mount",
                                    .count = 45678,
                                    .topNProcesses = {{"disk I/O", 45678}}}},
            .totalMajorFaults = 156663,
            .majorFaultsPercentChange = 0.0,
    };

    IoPerfCollection collector;
    collector.mTopNStatsPerCategory = 2;
    collector.mTopNStatsPerSubcategory = 2;

    sp<MockPackageInfoResolver> mockPackageInfoResolver = new MockPackageInfoResolver();
    collector.mPackageInfoResolver = mockPackageInfoResolver;
    EXPECT_CALL(*mockPackageInfoResolver, getPackageNamesForUids(_))
            .WillRepeatedly(Return<std::unordered_map<uid_t, std::string>>(
                    {{0, "root"}, {1009, "mount"}, {1001000, "shared:android.uid.system"}}));

    struct ProcessIoPerfData actualProcessIoPerfData = {};
    collector.processProcessIoPerfDataLocked({}, mockProcPidStat, &actualProcessIoPerfData);

    EXPECT_TRUE(isEqual(expectedProcessIoPerfData, actualProcessIoPerfData))
            << "First snapshot doesn't match.\nExpected:\n"
            << toString(expectedProcessIoPerfData) << "\nActual:\n"
            << toString(actualProcessIoPerfData);

    const std::vector<ProcessStats> secondProcessStats({
            {.tgid = 1,
             .uid = 0,
             .process = {1, "init", "S", 0, 660, 2, 0},
             .threads = {{1, {1, "init", "S", 0, 600, 2, 0}},
                         {453, {453, "init", "S", 0, 60, 2, 275}}}},
            {.tgid = 2546,
             .uid = 1001000,
             .process = {2546, "system_server", "R", 1, 12000, 3, 1000},
             .threads = {{2456, {2456, "system_server", "R", 1, 2000, 3, 1000}},
                         {3456, {3456, "system_server", "S", 1, 6000, 3, 2300}},
                         {4789, {4789, "system_server", "D", 1, 4000, 3, 4500}}}},
    });
    EXPECT_CALL(*mockProcPidStat, deltaStats()).WillOnce(Return(secondProcessStats));
    expectedProcessIoPerfData = {
            .topNIoBlockedUids = {{.userId = 10,  // uid: 1001000
                                   .packageName = "shared:android.uid.system",
                                   .count = 1,
                                   .topNProcesses = {{"system_server", 1}}}},
            .topNIoBlockedUidsTotalTaskCnt = {3},
            .topNMajorFaultUids = {{.userId = 10,  // uid: 1001000
                                    .packageName = "shared:android.uid.system",
                                    .count = 12000,
                                    .topNProcesses = {{"system_server", 12000}}},
                                   {.userId = 0,  // uid: 0
                                    .packageName = "root",
                                    .count = 660,
                                    .topNProcesses = {{"init", 660}}}},
            .totalMajorFaults = 12660,
            .majorFaultsPercentChange = ((12660.0 - 156663.0) / 156663.0) * 100,
    };

    actualProcessIoPerfData = {};
    collector.processProcessIoPerfDataLocked({}, mockProcPidStat, &actualProcessIoPerfData);

    EXPECT_TRUE(isEqual(expectedProcessIoPerfData, actualProcessIoPerfData))
            << "Second snapshot doesn't match.\nExpected:\n"
            << toString(expectedProcessIoPerfData) << "\nActual:\n"
            << toString(actualProcessIoPerfData);
}

TEST(IoPerfCollectionTest, TestProcPidContentsLessThanTopNStatsLimit) {
    const std::vector<ProcessStats> processStats({
            {.tgid = 1,
             .uid = 0,
             .process = {1, "init", "S", 0, 880, 2, 0},
             .threads = {{1, {1, "init", "S", 0, 800, 2, 0}},
                         {453, {453, "init", "S", 0, 80, 2, 275}}}},
    });
    sp<MockProcPidStat> mockProcPidStat = new MockProcPidStat();
    EXPECT_CALL(*mockProcPidStat, deltaStats()).WillOnce(Return(processStats));

    struct ProcessIoPerfData expectedProcessIoPerfData = {
            .topNMajorFaultUids = {{.userId = 0,  // uid: 0
                                    .packageName = "root",
                                    .count = 880,
                                    .topNProcesses = {{"init", 880}}}},
            .totalMajorFaults = 880,
            .majorFaultsPercentChange = 0.0,
    };

    IoPerfCollection collector;
    collector.mTopNStatsPerCategory = 5;
    collector.mTopNStatsPerSubcategory = 3;

    sp<MockPackageInfoResolver> mockPackageInfoResolver = new MockPackageInfoResolver();
    collector.mPackageInfoResolver = mockPackageInfoResolver;
    EXPECT_CALL(*mockPackageInfoResolver, getPackageNamesForUids(_))
            .WillRepeatedly(Return<std::unordered_map<uid_t, std::string>>({{0, "root"}}));

    struct ProcessIoPerfData actualProcessIoPerfData = {};
    collector.processProcessIoPerfDataLocked({}, mockProcPidStat, &actualProcessIoPerfData);

    EXPECT_TRUE(isEqual(expectedProcessIoPerfData, actualProcessIoPerfData))
            << "proc pid contents don't match.\nExpected:\n"
            << toString(expectedProcessIoPerfData) << "\nActual:\n"
            << toString(actualProcessIoPerfData);
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
