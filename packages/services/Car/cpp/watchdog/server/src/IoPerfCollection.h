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

#ifndef CPP_WATCHDOG_SERVER_SRC_IOPERFCOLLECTION_H_
#define CPP_WATCHDOG_SERVER_SRC_IOPERFCOLLECTION_H_

#include "PackageInfoResolver.h"
#include "ProcDiskStats.h"
#include "ProcPidStat.h"
#include "ProcStat.h"
#include "UidIoStats.h"
#include "WatchdogPerfService.h"

#include <android-base/result.h>
#include <cutils/multiuser.h>
#include <gtest/gtest_prod.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include <ctime>
#include <string>
#include <unordered_set>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

// Number of periodic collection perf data snapshots to cache in memory.
const int32_t kDefaultPeriodicCollectionBufferSize = 180;
constexpr const char* kEmptyCollectionMessage = "No collection recorded\n";

// Performance data collected from the `/proc/uid_io/stats` file.
struct UidIoPerfData {
    struct Stats {
        userid_t userId = 0;
        std::string packageName;
        int64_t bytes[UID_STATES];
        int64_t fsync[UID_STATES];
    };
    std::vector<Stats> topNReads = {};
    std::vector<Stats> topNWrites = {};
    int64_t total[METRIC_TYPES][UID_STATES] = {{0}};
};

std::string toString(const UidIoPerfData& perfData);

// Performance data collected from the `/proc/stats` file.
struct SystemIoPerfData {
    uint64_t cpuIoWaitTime = 0;
    uint64_t totalCpuTime = 0;
    uint32_t ioBlockedProcessesCnt = 0;
    uint32_t totalProcessesCnt = 0;
};

std::string toString(const SystemIoPerfData& perfData);

// Performance data collected from the `/proc/[pid]/stat` and `/proc/[pid]/task/[tid]/stat` files.
struct ProcessIoPerfData {
    struct UidStats {
        userid_t userId = 0;
        std::string packageName;
        uint64_t count = 0;
        struct ProcessStats {
            std::string comm = "";
            uint64_t count = 0;
        };
        std::vector<ProcessStats> topNProcesses = {};
    };
    std::vector<UidStats> topNIoBlockedUids = {};
    // Total # of tasks owned by each UID in |topNIoBlockedUids|.
    std::vector<uint64_t> topNIoBlockedUidsTotalTaskCnt = {};
    std::vector<UidStats> topNMajorFaultUids = {};
    uint64_t totalMajorFaults = 0;
    // Percentage of increase/decrease in the major page faults since last collection.
    double majorFaultsPercentChange = 0.0;
};

std::string toString(const ProcessIoPerfData& data);

struct IoPerfRecord {
    time_t time;  // Collection time.
    UidIoPerfData uidIoPerfData;
    SystemIoPerfData systemIoPerfData;
    ProcessIoPerfData processIoPerfData;
};

std::string toString(const IoPerfRecord& record);

struct CollectionInfo {
    size_t maxCacheSize = 0;            // Maximum cache size for the collection.
    std::vector<IoPerfRecord> records;  // Cache of collected performance records.
};

std::string toString(const CollectionInfo& collectionInfo);

// Forward declaration for testing use only.
namespace internal {

class IoPerfCollectionPeer;

}  // namespace internal

// IoPerfCollection implements the I/O performance data collection module.
class IoPerfCollection : public IDataProcessorInterface {
public:
    IoPerfCollection() :
          mTopNStatsPerCategory(0),
          mTopNStatsPerSubcategory(0),
          mPackageInfoResolver(PackageInfoResolver::getInstance()),
          mBoottimeCollection({}),
          mPeriodicCollection({}),
          mCustomCollection({}),
          mLastMajorFaults(0) {}

    ~IoPerfCollection() { terminate(); }

    std::string name() { return "IoPerfCollection"; }

    // Implements IDataProcessorInterface.
    android::base::Result<void> onBoottimeCollection(time_t time,
                                                     const android::wp<UidIoStats>& uidIoStats,
                                                     const android::wp<ProcStat>& procStat,
                                                     const android::wp<ProcPidStat>& procPidStat);

    android::base::Result<void> onPeriodicCollection(time_t time, SystemState systemState,
                                                     const android::wp<UidIoStats>& uidIoStats,
                                                     const android::wp<ProcStat>& procStat,
                                                     const android::wp<ProcPidStat>& procPidStat);

    android::base::Result<void> onCustomCollection(
            time_t time, SystemState systemState,
            const std::unordered_set<std::string>& filterPackages,
            const android::wp<UidIoStats>& uidIoStats, const android::wp<ProcStat>& procStat,
            const android::wp<ProcPidStat>& procPidStat);

    android::base::Result<void> onPeriodicMonitor(
            [[maybe_unused]] time_t time,
            [[maybe_unused]] const android::wp<IProcDiskStatsInterface>& procDiskStats,
            [[maybe_unused]] const std::function<void()>& alertHandler) {
        // No monitoring done here as this DataProcessor only collects I/O performance records.
        return {};
    }

    android::base::Result<void> onDump(int fd);

    android::base::Result<void> onCustomCollectionDump(int fd);

protected:
    android::base::Result<void> init();

    // Clears in-memory cache.
    void terminate();

private:
    // Processes the collected data.
    android::base::Result<void> processLocked(time_t time,
                                              const std::unordered_set<std::string>& filterPackages,
                                              const android::wp<UidIoStats>& uidIoStats,
                                              const android::wp<ProcStat>& procStat,
                                              const android::wp<ProcPidStat>& procPidStat,
                                              CollectionInfo* collectionInfo);

    // Processes performance data from the `/proc/uid_io/stats` file.
    void processUidIoPerfData(const std::unordered_set<std::string>& filterPackages,
                              const android::wp<UidIoStats>& uidIoStats,
                              UidIoPerfData* uidIoPerfData) const;

    // Processes performance data from the `/proc/stats` file.
    void processSystemIoPerfData(const android::wp<ProcStat>& procStat,
                                 SystemIoPerfData* systemIoPerfData) const;

    // Processes performance data from the `/proc/[pid]/stat` and `/proc/[pid]/task/[tid]/stat`
    // files.
    void processProcessIoPerfDataLocked(const std::unordered_set<std::string>& filterPackages,
                                        const android::wp<ProcPidStat>& procPidStat,
                                        ProcessIoPerfData* processIoPerfData);

    // Top N per-UID stats per category.
    int mTopNStatsPerCategory;

    // Top N per-process stats per subcategory.
    int mTopNStatsPerSubcategory;

    // Local IPackageInfoResolver instance. Useful to mock in tests.
    sp<IPackageInfoResolver> mPackageInfoResolver;

    // Makes sure only one collection is running at any given time.
    Mutex mMutex;

    // Info for the boot-time collection event. The cache is persisted until system shutdown/reboot.
    CollectionInfo mBoottimeCollection GUARDED_BY(mMutex);

    // Info for the periodic collection event. The cache size is limited by
    // |ro.carwatchdog.periodic_collection_buffer_size|.
    CollectionInfo mPeriodicCollection GUARDED_BY(mMutex);

    // Info for the custom collection event. The info is cleared at the end of every custom
    // collection.
    CollectionInfo mCustomCollection GUARDED_BY(mMutex);

    // Major faults delta from last collection. Useful when calculating the percentage change in
    // major faults since last collection.
    uint64_t mLastMajorFaults GUARDED_BY(mMutex);

    friend class WatchdogPerfService;

    // For unit tests.
    friend class internal::IoPerfCollectionPeer;
    FRIEND_TEST(IoPerfCollectionTest, TestUidIoStatsGreaterThanTopNStatsLimit);
    FRIEND_TEST(IoPerfCollectionTest, TestUidIOStatsLessThanTopNStatsLimit);
    FRIEND_TEST(IoPerfCollectionTest, TestProcessSystemIoPerfData);
    FRIEND_TEST(IoPerfCollectionTest, TestProcPidContentsGreaterThanTopNStatsLimit);
    FRIEND_TEST(IoPerfCollectionTest, TestProcPidContentsLessThanTopNStatsLimit);
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_SRC_IOPERFCOLLECTION_H_
