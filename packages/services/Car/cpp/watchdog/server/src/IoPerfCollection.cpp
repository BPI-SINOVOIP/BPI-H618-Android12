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

#define LOG_TAG "carwatchdogd"

#include "IoPerfCollection.h"

#include <WatchdogProperties.sysprop.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <log/log.h>

#include <inttypes.h>

#include <iomanip>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::wp;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StringAppendF;
using ::android::base::StringPrintf;
using ::android::base::WriteStringToFd;

namespace {

const int32_t kDefaultTopNStatsPerCategory = 10;
const int32_t kDefaultTopNStatsPerSubcategory = 5;

double percentage(uint64_t numer, uint64_t denom) {
    return denom == 0 ? 0.0 : (static_cast<double>(numer) / static_cast<double>(denom)) * 100.0;
}

struct UidProcessStats {
    struct ProcessInfo {
        std::string comm = "";
        uint64_t count = 0;
    };
    uint64_t uid = 0;
    uint32_t ioBlockedTasksCnt = 0;
    uint32_t totalTasksCnt = 0;
    uint64_t majorFaults = 0;
    std::vector<ProcessInfo> topNIoBlockedProcesses = {};
    std::vector<ProcessInfo> topNMajorFaultProcesses = {};
};

std::unique_ptr<std::unordered_map<uid_t, UidProcessStats>> getUidProcessStats(
        const std::vector<ProcessStats>& processStats, int topNStatsPerSubCategory) {
    std::unique_ptr<std::unordered_map<uid_t, UidProcessStats>> uidProcessStats(
            new std::unordered_map<uid_t, UidProcessStats>());
    for (const auto& stats : processStats) {
        if (stats.uid < 0) {
            continue;
        }
        uid_t uid = static_cast<uid_t>(stats.uid);
        if (uidProcessStats->find(uid) == uidProcessStats->end()) {
            (*uidProcessStats)[uid] = UidProcessStats{
                    .uid = uid,
                    .topNIoBlockedProcesses = std::vector<
                            UidProcessStats::ProcessInfo>(topNStatsPerSubCategory,
                                                          UidProcessStats::ProcessInfo{}),
                    .topNMajorFaultProcesses = std::vector<
                            UidProcessStats::ProcessInfo>(topNStatsPerSubCategory,
                                                          UidProcessStats::ProcessInfo{}),
            };
        }
        auto& curUidProcessStats = (*uidProcessStats)[uid];
        // Top-level process stats has the aggregated major page faults count and this should be
        // persistent across thread creation/termination. Thus use the value from this field.
        curUidProcessStats.majorFaults += stats.process.majorFaults;
        curUidProcessStats.totalTasksCnt += stats.threads.size();
        // The process state is the same as the main thread state. Thus to avoid double counting
        // ignore the process state.
        uint32_t ioBlockedTasksCnt = 0;
        for (const auto& threadStat : stats.threads) {
            ioBlockedTasksCnt += threadStat.second.state == "D" ? 1 : 0;
        }
        curUidProcessStats.ioBlockedTasksCnt += ioBlockedTasksCnt;
        for (auto it = curUidProcessStats.topNIoBlockedProcesses.begin();
             it != curUidProcessStats.topNIoBlockedProcesses.end(); ++it) {
            if (it->count < ioBlockedTasksCnt) {
                curUidProcessStats.topNIoBlockedProcesses
                        .emplace(it,
                                 UidProcessStats::ProcessInfo{
                                         .comm = stats.process.comm,
                                         .count = ioBlockedTasksCnt,
                                 });
                curUidProcessStats.topNIoBlockedProcesses.pop_back();
                break;
            }
        }
        for (auto it = curUidProcessStats.topNMajorFaultProcesses.begin();
             it != curUidProcessStats.topNMajorFaultProcesses.end(); ++it) {
            if (it->count < stats.process.majorFaults) {
                curUidProcessStats.topNMajorFaultProcesses
                        .emplace(it,
                                 UidProcessStats::ProcessInfo{
                                         .comm = stats.process.comm,
                                         .count = stats.process.majorFaults,
                                 });
                curUidProcessStats.topNMajorFaultProcesses.pop_back();
                break;
            }
        }
    }
    return uidProcessStats;
}

Result<void> checkDataCollectors(const wp<UidIoStats>& uidIoStats, const wp<ProcStat>& procStat,
                                 const wp<ProcPidStat>& procPidStat) {
    if (uidIoStats != nullptr && procStat != nullptr && procPidStat != nullptr) {
        return {};
    }
    std::string error;
    if (uidIoStats == nullptr) {
        error = "Per-UID I/O stats collector must not be empty";
    }
    if (procStat == nullptr) {
        StringAppendF(&error, "%s%s", error.empty() ? "" : ", ",
                      "Proc stats collector must not be empty");
    }
    if (procPidStat == nullptr) {
        StringAppendF(&error, "%s%s", error.empty() ? "" : ", ",
                      "Per-process stats collector must not be empty");
    }

    return Error() << "Invalid data collectors: " << error;
}

}  // namespace

std::string toString(const UidIoPerfData& data) {
    std::string buffer;
    if (data.topNReads.size() > 0) {
        StringAppendF(&buffer, "\nTop N Reads:\n%s\n", std::string(12, '-').c_str());
        StringAppendF(&buffer,
                      "Android User ID, Package Name, Foreground Bytes, Foreground Bytes %%, "
                      "Foreground Fsync, Foreground Fsync %%, Background Bytes, "
                      "Background Bytes %%, Background Fsync, Background Fsync %%\n");
    }
    for (const auto& stat : data.topNReads) {
        StringAppendF(&buffer, "%" PRIu32 ", %s", stat.userId, stat.packageName.c_str());
        for (int i = 0; i < UID_STATES; ++i) {
            StringAppendF(&buffer, ", %" PRIi64 ", %.2f%%, %" PRIi64 ", %.2f%%", stat.bytes[i],
                          percentage(stat.bytes[i], data.total[READ_BYTES][i]), stat.fsync[i],
                          percentage(stat.fsync[i], data.total[FSYNC_COUNT][i]));
        }
        StringAppendF(&buffer, "\n");
    }
    if (data.topNWrites.size() > 0) {
        StringAppendF(&buffer, "\nTop N Writes:\n%s\n", std::string(13, '-').c_str());
        StringAppendF(&buffer,
                      "Android User ID, Package Name, Foreground Bytes, Foreground Bytes %%, "
                      "Foreground Fsync, Foreground Fsync %%, Background Bytes, "
                      "Background Bytes %%, Background Fsync, Background Fsync %%\n");
    }
    for (const auto& stat : data.topNWrites) {
        StringAppendF(&buffer, "%" PRIu32 ", %s", stat.userId, stat.packageName.c_str());
        for (int i = 0; i < UID_STATES; ++i) {
            StringAppendF(&buffer, ", %" PRIi64 ", %.2f%%, %" PRIi64 ", %.2f%%", stat.bytes[i],
                          percentage(stat.bytes[i], data.total[WRITE_BYTES][i]), stat.fsync[i],
                          percentage(stat.fsync[i], data.total[FSYNC_COUNT][i]));
        }
        StringAppendF(&buffer, "\n");
    }
    return buffer;
}

std::string toString(const SystemIoPerfData& data) {
    std::string buffer;
    StringAppendF(&buffer, "CPU I/O wait time/percent: %" PRIu64 " / %.2f%%\n", data.cpuIoWaitTime,
                  percentage(data.cpuIoWaitTime, data.totalCpuTime));
    StringAppendF(&buffer, "Number of I/O blocked processes/percent: %" PRIu32 " / %.2f%%\n",
                  data.ioBlockedProcessesCnt,
                  percentage(data.ioBlockedProcessesCnt, data.totalProcessesCnt));
    return buffer;
}

std::string toString(const ProcessIoPerfData& data) {
    std::string buffer;
    StringAppendF(&buffer, "Number of major page faults since last collection: %" PRIu64 "\n",
                  data.totalMajorFaults);
    StringAppendF(&buffer,
                  "Percentage of change in major page faults since last collection: %.2f%%\n",
                  data.majorFaultsPercentChange);
    if (data.topNMajorFaultUids.size() > 0) {
        StringAppendF(&buffer, "\nTop N major page faults:\n%s\n", std::string(24, '-').c_str());
        StringAppendF(&buffer,
                      "Android User ID, Package Name, Number of major page faults, "
                      "Percentage of total major page faults\n");
        StringAppendF(&buffer,
                      "\tCommand, Number of major page faults, Percentage of UID's major page "
                      "faults\n");
    }
    for (const auto& uidStats : data.topNMajorFaultUids) {
        StringAppendF(&buffer, "%" PRIu32 ", %s, %" PRIu64 ", %.2f%%\n", uidStats.userId,
                      uidStats.packageName.c_str(), uidStats.count,
                      percentage(uidStats.count, data.totalMajorFaults));
        for (const auto& procStats : uidStats.topNProcesses) {
            StringAppendF(&buffer, "\t%s, %" PRIu64 ", %.2f%%\n", procStats.comm.c_str(),
                          procStats.count, percentage(procStats.count, uidStats.count));
        }
    }
    if (data.topNIoBlockedUids.size() > 0) {
        StringAppendF(&buffer, "\nTop N I/O waiting UIDs:\n%s\n", std::string(23, '-').c_str());
        StringAppendF(&buffer,
                      "Android User ID, Package Name, Number of owned tasks waiting for I/O, "
                      "Percentage of owned tasks waiting for I/O\n");
        StringAppendF(&buffer,
                      "\tCommand, Number of I/O waiting tasks, Percentage of UID's tasks waiting "
                      "for I/O\n");
    }
    for (size_t i = 0; i < data.topNIoBlockedUids.size(); ++i) {
        const auto& uidStats = data.topNIoBlockedUids[i];
        StringAppendF(&buffer, "%" PRIu32 ", %s, %" PRIu64 ", %.2f%%\n", uidStats.userId,
                      uidStats.packageName.c_str(), uidStats.count,
                      percentage(uidStats.count, data.topNIoBlockedUidsTotalTaskCnt[i]));
        for (const auto& procStats : uidStats.topNProcesses) {
            StringAppendF(&buffer, "\t%s, %" PRIu64 ", %.2f%%\n", procStats.comm.c_str(),
                          procStats.count, percentage(procStats.count, uidStats.count));
        }
    }
    return buffer;
}

std::string toString(const IoPerfRecord& record) {
    std::string buffer;
    StringAppendF(&buffer, "%s%s%s", toString(record.systemIoPerfData).c_str(),
                  toString(record.processIoPerfData).c_str(),
                  toString(record.uidIoPerfData).c_str());
    return buffer;
}

std::string toString(const CollectionInfo& collectionInfo) {
    if (collectionInfo.records.empty()) {
        return kEmptyCollectionMessage;
    }
    std::string buffer;
    double duration =
            difftime(collectionInfo.records.back().time, collectionInfo.records.front().time);
    StringAppendF(&buffer, "Collection duration: %.f seconds\nNumber of collections: %zu\n",
                  duration, collectionInfo.records.size());

    for (size_t i = 0; i < collectionInfo.records.size(); ++i) {
        const auto& record = collectionInfo.records[i];
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&record.time), "%c %Z");
        StringAppendF(&buffer, "\nCollection %zu: <%s>\n%s\n%s", i, timestamp.str().c_str(),
                      std::string(45, '=').c_str(), toString(record).c_str());
    }
    return buffer;
}

Result<void> IoPerfCollection::init() {
    Mutex::Autolock lock(mMutex);
    if (mTopNStatsPerCategory != 0 || mTopNStatsPerSubcategory != 0) {
        return Error() << "Cannot initialize " << name() << " more than once";
    }
    mTopNStatsPerCategory = static_cast<int>(
            sysprop::topNStatsPerCategory().value_or(kDefaultTopNStatsPerCategory));
    mTopNStatsPerSubcategory = static_cast<int>(
            sysprop::topNStatsPerSubcategory().value_or(kDefaultTopNStatsPerSubcategory));
    size_t periodicCollectionBufferSize = static_cast<size_t>(
            sysprop::periodicCollectionBufferSize().value_or(kDefaultPeriodicCollectionBufferSize));
    mBoottimeCollection = {
            .maxCacheSize = std::numeric_limits<std::size_t>::max(),
            .records = {},
    };
    mPeriodicCollection = {
            .maxCacheSize = periodicCollectionBufferSize,
            .records = {},
    };
    mCustomCollection = {
            .maxCacheSize = std::numeric_limits<std::size_t>::max(),
            .records = {},
    };
    return {};
}

void IoPerfCollection::terminate() {
    Mutex::Autolock lock(mMutex);

    ALOGW("Terminating %s", name().c_str());

    mBoottimeCollection.records.clear();
    mBoottimeCollection = {};

    mPeriodicCollection.records.clear();
    mPeriodicCollection = {};

    mCustomCollection.records.clear();
    mCustomCollection = {};
}

Result<void> IoPerfCollection::onDump(int fd) {
    Mutex::Autolock lock(mMutex);
    if (!WriteStringToFd(StringPrintf("%s\nBoot-time I/O performance report:\n%s\n",
                                      std::string(75, '-').c_str(), std::string(33, '=').c_str()),
                         fd) ||
        !WriteStringToFd(toString(mBoottimeCollection), fd) ||
        !WriteStringToFd(StringPrintf("%s\nLast N minutes I/O performance report:\n%s\n",
                                      std::string(75, '-').c_str(), std::string(38, '=').c_str()),
                         fd) ||
        !WriteStringToFd(toString(mPeriodicCollection), fd)) {
        return Error(FAILED_TRANSACTION)
                << "Failed to dump the boot-time and periodic collection reports.";
    }
    return {};
}

Result<void> IoPerfCollection::onCustomCollectionDump(int fd) {
    if (fd == -1) {
        // Custom collection ends so clear the cache.
        mCustomCollection.records.clear();
        mCustomCollection = {
                .maxCacheSize = std::numeric_limits<std::size_t>::max(),
                .records = {},
        };
        return {};
    }

    if (!WriteStringToFd(StringPrintf("%s\nCustom I/O performance data report:\n%s\n",
                                      std::string(75, '-').c_str(), std::string(75, '-').c_str()),
                         fd) ||
        !WriteStringToFd(toString(mCustomCollection), fd)) {
        return Error(FAILED_TRANSACTION) << "Failed to write custom I/O collection report.";
    }

    return {};
}

Result<void> IoPerfCollection::onBoottimeCollection(time_t time, const wp<UidIoStats>& uidIoStats,
                                                    const wp<ProcStat>& procStat,
                                                    const wp<ProcPidStat>& procPidStat) {
    auto result = checkDataCollectors(uidIoStats, procStat, procPidStat);
    if (!result.ok()) {
        return result;
    }
    Mutex::Autolock lock(mMutex);
    return processLocked(time, std::unordered_set<std::string>(), uidIoStats, procStat, procPidStat,
                         &mBoottimeCollection);
}

Result<void> IoPerfCollection::onPeriodicCollection(time_t time,
                                                    [[maybe_unused]] SystemState systemState,
                                                    const wp<UidIoStats>& uidIoStats,
                                                    const wp<ProcStat>& procStat,
                                                    const wp<ProcPidStat>& procPidStat) {
    auto result = checkDataCollectors(uidIoStats, procStat, procPidStat);
    if (!result.ok()) {
        return result;
    }
    Mutex::Autolock lock(mMutex);
    return processLocked(time, std::unordered_set<std::string>(), uidIoStats, procStat, procPidStat,
                         &mPeriodicCollection);
}

Result<void> IoPerfCollection::onCustomCollection(
        time_t time, [[maybe_unused]] SystemState systemState,
        const std::unordered_set<std::string>& filterPackages, const wp<UidIoStats>& uidIoStats,
        const wp<ProcStat>& procStat, const wp<ProcPidStat>& procPidStat) {
    auto result = checkDataCollectors(uidIoStats, procStat, procPidStat);
    if (!result.ok()) {
        return result;
    }
    Mutex::Autolock lock(mMutex);
    return processLocked(time, filterPackages, uidIoStats, procStat, procPidStat,
                         &mCustomCollection);
}

Result<void> IoPerfCollection::processLocked(time_t time,
                                             const std::unordered_set<std::string>& filterPackages,
                                             const wp<UidIoStats>& uidIoStats,
                                             const wp<ProcStat>& procStat,
                                             const wp<ProcPidStat>& procPidStat,
                                             CollectionInfo* collectionInfo) {
    if (collectionInfo->maxCacheSize == 0) {
        return Error() << "Maximum cache size cannot be 0";
    }
    IoPerfRecord record{
            .time = time,
    };
    processSystemIoPerfData(procStat, &record.systemIoPerfData);
    processProcessIoPerfDataLocked(filterPackages, procPidStat, &record.processIoPerfData);
    processUidIoPerfData(filterPackages, uidIoStats, &record.uidIoPerfData);
    if (collectionInfo->records.size() > collectionInfo->maxCacheSize) {
        collectionInfo->records.erase(collectionInfo->records.begin());  // Erase the oldest record.
    }
    collectionInfo->records.emplace_back(record);
    return {};
}

void IoPerfCollection::processUidIoPerfData(const std::unordered_set<std::string>& filterPackages,
                                            const wp<UidIoStats>& uidIoStats,
                                            UidIoPerfData* uidIoPerfData) const {
    const std::unordered_map<uid_t, UidIoUsage>& usages = uidIoStats.promote()->deltaStats();

    // Fetch only the top N reads and writes from the usage records.
    UidIoUsage tempUsage = {};
    std::vector<const UidIoUsage*> topNReads(mTopNStatsPerCategory, &tempUsage);
    std::vector<const UidIoUsage*> topNWrites(mTopNStatsPerCategory, &tempUsage);
    std::vector<uid_t> uids;

    for (const auto& uIt : usages) {
        const UidIoUsage& curUsage = uIt.second;
        uids.push_back(curUsage.uid);
        uidIoPerfData->total[READ_BYTES][FOREGROUND] +=
                curUsage.ios.metrics[READ_BYTES][FOREGROUND];
        uidIoPerfData->total[READ_BYTES][BACKGROUND] +=
                curUsage.ios.metrics[READ_BYTES][BACKGROUND];
        uidIoPerfData->total[WRITE_BYTES][FOREGROUND] +=
                curUsage.ios.metrics[WRITE_BYTES][FOREGROUND];
        uidIoPerfData->total[WRITE_BYTES][BACKGROUND] +=
                curUsage.ios.metrics[WRITE_BYTES][BACKGROUND];
        uidIoPerfData->total[FSYNC_COUNT][FOREGROUND] +=
                curUsage.ios.metrics[FSYNC_COUNT][FOREGROUND];
        uidIoPerfData->total[FSYNC_COUNT][BACKGROUND] +=
                curUsage.ios.metrics[FSYNC_COUNT][BACKGROUND];

        for (auto it = topNReads.begin(); it != topNReads.end(); ++it) {
            const UidIoUsage* curRead = *it;
            if (curRead->ios.sumReadBytes() < curUsage.ios.sumReadBytes()) {
                topNReads.emplace(it, &curUsage);
                if (filterPackages.empty()) {
                    topNReads.pop_back();
                }
                break;
            }
        }
        for (auto it = topNWrites.begin(); it != topNWrites.end(); ++it) {
            const UidIoUsage* curWrite = *it;
            if (curWrite->ios.sumWriteBytes() < curUsage.ios.sumWriteBytes()) {
                topNWrites.emplace(it, &curUsage);
                if (filterPackages.empty()) {
                    topNWrites.pop_back();
                }
                break;
            }
        }
    }

    const auto& uidToPackageNameMapping = mPackageInfoResolver->getPackageNamesForUids(uids);

    // Convert the top N I/O usage to UidIoPerfData.
    for (const auto& usage : topNReads) {
        if (usage->ios.isZero()) {
            // End of non-zero usage records. This case occurs when the number of UIDs with active
            // I/O operations is < |ro.carwatchdog.top_n_stats_per_category|.
            break;
        }
        UidIoPerfData::Stats stats = {
                .userId = multiuser_get_user_id(usage->uid),
                .packageName = std::to_string(usage->uid),
                .bytes = {usage->ios.metrics[READ_BYTES][FOREGROUND],
                          usage->ios.metrics[READ_BYTES][BACKGROUND]},
                .fsync = {usage->ios.metrics[FSYNC_COUNT][FOREGROUND],
                          usage->ios.metrics[FSYNC_COUNT][BACKGROUND]},
        };
        if (uidToPackageNameMapping.find(usage->uid) != uidToPackageNameMapping.end()) {
            stats.packageName = uidToPackageNameMapping.at(usage->uid);
        }
        if (!filterPackages.empty() &&
            filterPackages.find(stats.packageName) == filterPackages.end()) {
            continue;
        }
        uidIoPerfData->topNReads.emplace_back(stats);
    }

    for (const auto& usage : topNWrites) {
        if (usage->ios.isZero()) {
            // End of non-zero usage records. This case occurs when the number of UIDs with active
            // I/O operations is < |ro.carwatchdog.top_n_stats_per_category|.
            break;
        }
        UidIoPerfData::Stats stats = {
                .userId = multiuser_get_user_id(usage->uid),
                .packageName = std::to_string(usage->uid),
                .bytes = {usage->ios.metrics[WRITE_BYTES][FOREGROUND],
                          usage->ios.metrics[WRITE_BYTES][BACKGROUND]},
                .fsync = {usage->ios.metrics[FSYNC_COUNT][FOREGROUND],
                          usage->ios.metrics[FSYNC_COUNT][BACKGROUND]},
        };
        if (uidToPackageNameMapping.find(usage->uid) != uidToPackageNameMapping.end()) {
            stats.packageName = uidToPackageNameMapping.at(usage->uid);
        }
        if (!filterPackages.empty() &&
            filterPackages.find(stats.packageName) == filterPackages.end()) {
            continue;
        }
        uidIoPerfData->topNWrites.emplace_back(stats);
    }
}

void IoPerfCollection::processSystemIoPerfData(const wp<ProcStat>& procStat,
                                               SystemIoPerfData* systemIoPerfData) const {
    const ProcStatInfo& procStatInfo = procStat.promote()->deltaStats();
    systemIoPerfData->cpuIoWaitTime = procStatInfo.cpuStats.ioWaitTime;
    systemIoPerfData->totalCpuTime = procStatInfo.totalCpuTime();
    systemIoPerfData->ioBlockedProcessesCnt = procStatInfo.ioBlockedProcessesCnt;
    systemIoPerfData->totalProcessesCnt = procStatInfo.totalProcessesCnt();
}

void IoPerfCollection::processProcessIoPerfDataLocked(
        const std::unordered_set<std::string>& filterPackages, const wp<ProcPidStat>& procPidStat,
        ProcessIoPerfData* processIoPerfData) {
    const std::vector<ProcessStats>& processStats = procPidStat.promote()->deltaStats();

    const auto& uidProcessStats = getUidProcessStats(processStats, mTopNStatsPerSubcategory);
    std::vector<uid_t> uids;
    // Fetch only the top N I/O blocked UIDs and UIDs with most major page faults.
    UidProcessStats temp = {};
    std::vector<const UidProcessStats*> topNIoBlockedUids(mTopNStatsPerCategory, &temp);
    std::vector<const UidProcessStats*> topNMajorFaultUids(mTopNStatsPerCategory, &temp);
    processIoPerfData->totalMajorFaults = 0;
    for (const auto& it : *uidProcessStats) {
        const UidProcessStats& curStats = it.second;
        uids.push_back(curStats.uid);
        processIoPerfData->totalMajorFaults += curStats.majorFaults;
        for (auto it = topNIoBlockedUids.begin(); it != topNIoBlockedUids.end(); ++it) {
            const UidProcessStats* topStats = *it;
            if (topStats->ioBlockedTasksCnt < curStats.ioBlockedTasksCnt) {
                topNIoBlockedUids.emplace(it, &curStats);
                if (filterPackages.empty()) {
                    topNIoBlockedUids.pop_back();
                }
                break;
            }
        }
        for (auto it = topNMajorFaultUids.begin(); it != topNMajorFaultUids.end(); ++it) {
            const UidProcessStats* topStats = *it;
            if (topStats->majorFaults < curStats.majorFaults) {
                topNMajorFaultUids.emplace(it, &curStats);
                if (filterPackages.empty()) {
                    topNMajorFaultUids.pop_back();
                }
                break;
            }
        }
    }

    const auto& uidToPackageNameMapping = mPackageInfoResolver->getPackageNamesForUids(uids);

    // Convert the top N uid process stats to ProcessIoPerfData.
    for (const auto& it : topNIoBlockedUids) {
        if (it->ioBlockedTasksCnt == 0) {
            // End of non-zero elements. This case occurs when the number of UIDs with I/O blocked
            // processes is < |ro.carwatchdog.top_n_stats_per_category|.
            break;
        }
        ProcessIoPerfData::UidStats stats = {
                .userId = multiuser_get_user_id(it->uid),
                .packageName = std::to_string(it->uid),
                .count = it->ioBlockedTasksCnt,
        };
        if (uidToPackageNameMapping.find(it->uid) != uidToPackageNameMapping.end()) {
            stats.packageName = uidToPackageNameMapping.at(it->uid);
        }
        if (!filterPackages.empty() &&
            filterPackages.find(stats.packageName) == filterPackages.end()) {
            continue;
        }
        for (const auto& pIt : it->topNIoBlockedProcesses) {
            if (pIt.count == 0) {
                break;
            }
            stats.topNProcesses.emplace_back(
                    ProcessIoPerfData::UidStats::ProcessStats{pIt.comm, pIt.count});
        }
        processIoPerfData->topNIoBlockedUids.emplace_back(stats);
        processIoPerfData->topNIoBlockedUidsTotalTaskCnt.emplace_back(it->totalTasksCnt);
    }
    for (const auto& it : topNMajorFaultUids) {
        if (it->majorFaults == 0) {
            // End of non-zero elements. This case occurs when the number of UIDs with major faults
            // is < |ro.carwatchdog.top_n_stats_per_category|.
            break;
        }
        ProcessIoPerfData::UidStats stats = {
                .userId = multiuser_get_user_id(it->uid),
                .packageName = std::to_string(it->uid),
                .count = it->majorFaults,
        };
        if (uidToPackageNameMapping.find(it->uid) != uidToPackageNameMapping.end()) {
            stats.packageName = uidToPackageNameMapping.at(it->uid);
        }
        if (!filterPackages.empty() &&
            filterPackages.find(stats.packageName) == filterPackages.end()) {
            continue;
        }
        for (const auto& pIt : it->topNMajorFaultProcesses) {
            if (pIt.count == 0) {
                break;
            }
            stats.topNProcesses.emplace_back(
                    ProcessIoPerfData::UidStats::ProcessStats{pIt.comm, pIt.count});
        }
        processIoPerfData->topNMajorFaultUids.emplace_back(stats);
    }
    if (mLastMajorFaults == 0) {
        processIoPerfData->majorFaultsPercentChange = 0;
    } else {
        int64_t increase = processIoPerfData->totalMajorFaults - mLastMajorFaults;
        processIoPerfData->majorFaultsPercentChange =
                (static_cast<double>(increase) / static_cast<double>(mLastMajorFaults)) * 100.0;
    }
    mLastMajorFaults = processIoPerfData->totalMajorFaults;
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
