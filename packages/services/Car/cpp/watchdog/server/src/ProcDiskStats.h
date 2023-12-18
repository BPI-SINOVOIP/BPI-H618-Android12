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

#ifndef CPP_WATCHDOG_SERVER_SRC_PROCDISKSTATS_H_
#define CPP_WATCHDOG_SERVER_SRC_PROCDISKSTATS_H_

#include <android-base/result.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include <string>
#include <unordered_set>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

constexpr const char* kProcDiskStatsPath = "/proc/diskstats";

inline constexpr bool recordStatsForDevice(const std::string& deviceName) {
    for (const auto& prefix : {"zram", "ram"}) {
        if (android::base::StartsWith(deviceName, prefix)) {
            return false;
        }
    }
    return true;
}

// Struct that represents the stats from |kUidIoStatsPath|.
struct DiskStats {
    int major = 0;
    int minor = 0;
    std::string deviceName;
    uint64_t numReadsCompleted = 0;
    uint64_t numReadsMerged = 0;
    uint64_t numKibRead = 0;
    uint64_t readTimeInMs = 0;
    uint64_t numWritesCompleted = 0;
    uint64_t numWritesMerged = 0;
    uint64_t numKibWritten = 0;
    uint64_t writeTimeInMs = 0;
    uint64_t totalIoTimeInMs = 0;
    uint64_t weightedTotalIoTimeInMs = 0;
    uint64_t numFlushCompleted = 0;
    uint64_t flushTimeInMs = 0;

    DiskStats& operator-=(const DiskStats& rhs);
    DiskStats& operator+=(const DiskStats& rhs);
    struct HashByPartition {
        size_t operator()(const DiskStats& stats) const;
    };
    struct EqualByPartition {
        bool operator()(const DiskStats& lhs, const DiskStats& rhs) const;
    };
};

/*
 * Contains methods that should be implemented by the /proc/diskstats reader or any mock reader
 * used in tests.
 */
class IProcDiskStatsInterface : public android::RefBase {
public:
    using PerPartitionDiskStats = ::std::unordered_set<DiskStats, DiskStats::HashByPartition,
                                                       DiskStats::EqualByPartition>;

    // Collects the system-wide block devices statistics.
    virtual android::base::Result<void> collect() = 0;

    // Returns the latest per-disk stats.
    virtual PerPartitionDiskStats latestPerPartitionDiskStats() const = 0;

    // Returns the aggregated delta stats since the last before collection.
    virtual DiskStats deltaSystemWideDiskStats() const = 0;

    // Returns true when the proc diskstats file is accessible. Otherwise, returns false.
    virtual bool enabled() const = 0;

    // Path to the disk stats file.
    virtual std::string filePath() const = 0;
};

class ProcDiskStats : public IProcDiskStatsInterface {
public:
    explicit ProcDiskStats(const std::string& path = kProcDiskStatsPath) :
          kEnabled(!access(path.c_str(), R_OK)),
          kPath(path) {}

    ~ProcDiskStats() {}

    android::base::Result<void> collect();

    PerPartitionDiskStats latestPerPartitionDiskStats() const {
        Mutex::Autolock lock(mMutex);
        return mLatestPerPartitionDiskStats;
    }

    DiskStats deltaSystemWideDiskStats() const {
        Mutex::Autolock lock(mMutex);
        return mDeltaSystemWideDiskStats;
    }

    bool enabled() const { return kEnabled; }

    std::string filePath() const { return kPath; }

private:
    // Makes sure only one collection is running at any given time.
    mutable Mutex mMutex;

    // Delta of per-UID I/O usage since last before collection.
    DiskStats mDeltaSystemWideDiskStats GUARDED_BY(mMutex);

    /*
     * Latest per-disk stats from the file at |kPath|. Per-disk stats is required for calculating
     * per-disk delta since last collection. Because the stats reported in |kPath| may overflow,
     * storing the stats per-disk helps to deal with this issue.
     */
    PerPartitionDiskStats mLatestPerPartitionDiskStats GUARDED_BY(mMutex);

    // True if |kPath| is accessible.
    const bool kEnabled;

    // Path to disk stats file.
    const std::string kPath;
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_SRC_PROCDISKSTATS_H_
