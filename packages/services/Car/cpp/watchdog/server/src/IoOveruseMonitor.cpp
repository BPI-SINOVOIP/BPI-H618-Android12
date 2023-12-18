/*
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
#define DEBUG false  // STOPSHIP if true.

#include "IoOveruseMonitor.h"

#include "PackageInfoResolver.h"

#include <WatchdogProperties.sysprop.h>
#include <android-base/file.h>
#include <android/automotive/watchdog/internal/PackageIdentifier.h>
#include <android/automotive/watchdog/internal/UidType.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <cutils/multiuser.h>
#include <log/log.h>
#include <processgroup/sched_policy.h>

#include <pthread.h>

#include <limits>
#include <thread>  // NOLINT(build/c++11)

namespace android {
namespace automotive {
namespace watchdog {

using ::android::IPCThreadState;
using ::android::sp;
using ::android::automotive::watchdog::internal::ComponentType;
using ::android::automotive::watchdog::internal::IoOveruseConfiguration;
using ::android::automotive::watchdog::internal::PackageIdentifier;
using ::android::automotive::watchdog::internal::PackageInfo;
using ::android::automotive::watchdog::internal::PackageIoOveruseStats;
using ::android::automotive::watchdog::internal::PackageResourceOveruseAction;
using ::android::automotive::watchdog::internal::ResourceOveruseConfiguration;
using ::android::automotive::watchdog::internal::UidType;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::WriteStringToFd;
using ::android::binder::Status;

// Minimum written bytes to sync the stats with the Watchdog service.
constexpr int64_t kMinSyncWrittenBytes = 100 * 1024;
// Minimum percentage of threshold to warn killable applications.
constexpr double kDefaultIoOveruseWarnPercentage = 80;
// Maximum numer of system-wide stats (from periodic monitoring) to cache.
constexpr size_t kMaxPeriodicMonitorBufferSize = 1000;
constexpr const char* kHelpText =
        "\n%s dump options:\n"
        "%s <package name>, <package name>,...: Reset resource overuse stats for the given package "
        "names. Value for this flag is a comma-separated value containing package names.\n";

namespace {

std::string uniquePackageIdStr(const PackageIdentifier& id) {
    return StringPrintf("%s:%" PRId32, id.name.c_str(), multiuser_get_user_id(id.uid));
}

PerStateBytes diff(const PerStateBytes& lhs, const PerStateBytes& rhs) {
    const auto sub = [](const int64_t& l, const int64_t& r) -> int64_t {
        return l >= r ? (l - r) : 0;
    };
    PerStateBytes result;
    result.foregroundBytes = sub(lhs.foregroundBytes, rhs.foregroundBytes);
    result.backgroundBytes = sub(lhs.backgroundBytes, rhs.backgroundBytes);
    result.garageModeBytes = sub(lhs.garageModeBytes, rhs.garageModeBytes);
    return result;
}

std::tuple<int64_t, int64_t> calculateStartAndDuration(struct tm currentTm) {
    // The stats are stored per-day so the start time is always the beginning of the day.
    auto startTm = currentTm;
    startTm.tm_sec = 0;
    startTm.tm_min = 0;
    startTm.tm_hour = 0;

    int64_t startTime = static_cast<int64_t>(mktime(&startTm));
    int64_t currentEpochSeconds = static_cast<int64_t>(mktime(&currentTm));
    return std::make_tuple(startTime, currentEpochSeconds - startTime);
}

int64_t totalPerStateBytes(PerStateBytes perStateBytes) {
    const auto sum = [](const int64_t& l, const int64_t& r) -> int64_t {
        return std::numeric_limits<int64_t>::max() - l > r ? (l + r)
                                                           : std::numeric_limits<int64_t>::max();
    };
    return sum(perStateBytes.foregroundBytes,
               sum(perStateBytes.backgroundBytes, perStateBytes.garageModeBytes));
}

}  // namespace

std::tuple<int64_t, int64_t> calculateStartAndDuration(const time_t& currentTime) {
    struct tm currentGmt;
    gmtime_r(&currentTime, &currentGmt);
    return calculateStartAndDuration(currentGmt);
}

IoOveruseMonitor::IoOveruseMonitor(
        const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper) :
      mMinSyncWrittenBytes(kMinSyncWrittenBytes),
      mWatchdogServiceHelper(watchdogServiceHelper),
      mSystemWideWrittenBytes({}),
      mPeriodicMonitorBufferSize(0),
      mLastSystemWideIoMonitorTime(0),
      mUserPackageDailyIoUsageById({}),
      mIoOveruseWarnPercentage(0),
      mLastUserPackageIoMonitorTime(0),
      mOveruseListenersByUid({}),
      mBinderDeathRecipient(sp<BinderDeathRecipient>::make(this)) {}

Result<void> IoOveruseMonitor::init() {
    std::unique_lock writeLock(mRwMutex);
    if (isInitializedLocked()) {
        return Error() << "Cannot initialize " << name() << " more than once";
    }
    mPeriodicMonitorBufferSize = static_cast<size_t>(
            sysprop::periodicMonitorBufferSize().value_or(kDefaultPeriodicMonitorBufferSize));
    if (mPeriodicMonitorBufferSize == 0 ||
        mPeriodicMonitorBufferSize > kMaxPeriodicMonitorBufferSize) {
        return Error() << "Periodic monitor buffer size cannot be zero or above "
                       << kDefaultPeriodicMonitorBufferSize << ". Received "
                       << mPeriodicMonitorBufferSize;
    }
    mIoOveruseWarnPercentage = static_cast<double>(
            sysprop::ioOveruseWarnPercentage().value_or(kDefaultIoOveruseWarnPercentage));
    mIoOveruseConfigs = sp<IoOveruseConfigs>::make();
    mPackageInfoResolver = PackageInfoResolver::getInstance();
    mPackageInfoResolver->setPackageConfigurations(mIoOveruseConfigs->vendorPackagePrefixes(),
                                                   mIoOveruseConfigs->packagesToAppCategories());
    if (DEBUG) {
        ALOGD("Initialized %s data processor", name().c_str());
    }
    return {};
}

void IoOveruseMonitor::terminate() {
    std::unique_lock writeLock(mRwMutex);

    ALOGW("Terminating %s", name().c_str());
    mWatchdogServiceHelper.clear();
    mIoOveruseConfigs.clear();
    mSystemWideWrittenBytes.clear();
    mUserPackageDailyIoUsageById.clear();
    for (const auto& [uid, listener] : mOveruseListenersByUid) {
        BnResourceOveruseListener::asBinder(listener)->unlinkToDeath(mBinderDeathRecipient);
    }
    mBinderDeathRecipient.clear();
    mOveruseListenersByUid.clear();
    if (DEBUG) {
        ALOGD("Terminated %s data processor", name().c_str());
    }
    return;
}

Result<void> IoOveruseMonitor::onPeriodicCollection(
        time_t time, SystemState systemState, const android::wp<UidIoStats>& uidIoStats,
        [[maybe_unused]] const android::wp<ProcStat>& procStat,
        [[maybe_unused]] const android::wp<ProcPidStat>& procPidStat) {
    if (uidIoStats == nullptr) {
        return Error() << "Per-UID I/O stats collector must not be null";
    }

    std::unique_lock writeLock(mRwMutex);
    struct tm prevGmt, curGmt;
    gmtime_r(&mLastUserPackageIoMonitorTime, &prevGmt);
    gmtime_r(&time, &curGmt);
    if (prevGmt.tm_yday != curGmt.tm_yday || prevGmt.tm_year != curGmt.tm_year) {
        /*
         * Date changed so reset the daily I/O usage cache. CarWatchdogService automatically handles
         * date change on |CarWatchdogService.latestIoOveruseStats| call.
         */
        mUserPackageDailyIoUsageById.clear();
    }
    mLastUserPackageIoMonitorTime = time;
    const auto [startTime, durationInSeconds] = calculateStartAndDuration(curGmt);

    auto perUidIoUsage = uidIoStats.promote()->deltaStats();
    /*
     * TODO(b/185849350): Maybe move the packageInfo fetching logic into UidIoStats module.
     *  This will also help avoid fetching package names in IoPerfCollection module.
     */
    std::vector<uid_t> seenUids;
    for (auto it = perUidIoUsage.begin(); it != perUidIoUsage.end();) {
        /*
         * UidIoStats::deltaStats returns entries with zero write bytes because other metrics
         * in these entries are non-zero.
         */
        if (it->second.ios.sumWriteBytes() == 0) {
            it = perUidIoUsage.erase(it);
            continue;
        }
        seenUids.push_back(it->first);
        ++it;
    }
    if (perUidIoUsage.empty()) {
        return {};
    }
    const auto packageInfosByUid = mPackageInfoResolver->getPackageInfosForUids(seenUids);
    std::unordered_map<uid_t, IoOveruseStats> overusingNativeStats;
    bool isGarageModeActive = systemState == SystemState::GARAGE_MODE;
    for (const auto& [uid, uidIoStats] : perUidIoUsage) {
        const auto& packageInfo = packageInfosByUid.find(uid);
        if (packageInfo == packageInfosByUid.end()) {
            continue;
        }
        UserPackageIoUsage curUsage(packageInfo->second, uidIoStats.ios, isGarageModeActive);
        UserPackageIoUsage* dailyIoUsage;
        if (auto cachedUsage = mUserPackageDailyIoUsageById.find(curUsage.id());
            cachedUsage != mUserPackageDailyIoUsageById.end()) {
            cachedUsage->second += curUsage;
            dailyIoUsage = &cachedUsage->second;
        } else {
            const auto& [it, wasInserted] = mUserPackageDailyIoUsageById.insert(
                    std::pair(curUsage.id(), std::move(curUsage)));
            dailyIoUsage = &it->second;
        }

        const auto threshold = mIoOveruseConfigs->fetchThreshold(dailyIoUsage->packageInfo);

        PackageIoOveruseStats stats;
        stats.uid = uid;
        stats.shouldNotify = false;
        stats.ioOveruseStats.startTime = startTime;
        stats.ioOveruseStats.durationInSeconds = durationInSeconds;
        stats.ioOveruseStats.writtenBytes = dailyIoUsage->writtenBytes;
        stats.ioOveruseStats.totalOveruses = dailyIoUsage->totalOveruses;
        stats.ioOveruseStats.remainingWriteBytes =
                diff(threshold, diff(dailyIoUsage->writtenBytes, dailyIoUsage->forgivenWriteBytes));
        stats.ioOveruseStats.killableOnOveruse =
                mIoOveruseConfigs->isSafeToKill(dailyIoUsage->packageInfo);

        const auto& remainingWriteBytes = stats.ioOveruseStats.remainingWriteBytes;
        const auto exceedsWarnThreshold = [&](double remaining, double threshold) {
            if (threshold == 0) {
                return true;
            }
            double usedPercent = (100 - (remaining / threshold) * 100);
            return usedPercent > mIoOveruseWarnPercentage;
        };
        bool shouldSyncWatchdogService =
                (totalPerStateBytes(dailyIoUsage->writtenBytes) -
                 dailyIoUsage->lastSyncedWrittenBytes) >= mMinSyncWrittenBytes;
        if (remainingWriteBytes.foregroundBytes == 0 || remainingWriteBytes.backgroundBytes == 0 ||
            remainingWriteBytes.garageModeBytes == 0) {
            stats.ioOveruseStats.totalOveruses = ++dailyIoUsage->totalOveruses;
            /*
             * Reset counters as the package may be disabled/killed by the watchdog service.
             * NOTE: If this logic is updated, update watchdog service side logic as well.
             */
            dailyIoUsage->forgivenWriteBytes = dailyIoUsage->writtenBytes;
            dailyIoUsage->isPackageWarned = false;
            /*
             * Send notifications for native service I/O overuses as well because system listeners
             * need to be notified of all I/O overuses.
             */
            stats.shouldNotify = true;
            if (dailyIoUsage->packageInfo.uidType == UidType::NATIVE) {
                overusingNativeStats[uid] = stats.ioOveruseStats;
            }
            shouldSyncWatchdogService = true;
        } else if (dailyIoUsage->packageInfo.uidType != UidType::NATIVE &&
                   stats.ioOveruseStats.killableOnOveruse && !dailyIoUsage->isPackageWarned &&
                   (exceedsWarnThreshold(remainingWriteBytes.foregroundBytes,
                                         threshold.foregroundBytes) ||
                    exceedsWarnThreshold(remainingWriteBytes.backgroundBytes,
                                         threshold.backgroundBytes) ||
                    exceedsWarnThreshold(remainingWriteBytes.garageModeBytes,
                                         threshold.garageModeBytes))) {
            /*
             * No need to warn native services or applications that won't be killed on I/O overuse
             * as they will be sent a notification when they exceed their daily threshold.
             */
            stats.shouldNotify = true;
            // Avoid duplicate warning before the daily threshold exceeded notification is sent.
            dailyIoUsage->isPackageWarned = true;
            shouldSyncWatchdogService = true;
        }
        if (shouldSyncWatchdogService) {
            dailyIoUsage->lastSyncedWrittenBytes = totalPerStateBytes(dailyIoUsage->writtenBytes);
            mLatestIoOveruseStats.emplace_back(std::move(stats));
        }
    }
    if (!overusingNativeStats.empty()) {
        notifyNativePackagesLocked(overusingNativeStats);
    }
    if (mLatestIoOveruseStats.empty()) {
        return {};
    }
    if (const auto status = mWatchdogServiceHelper->latestIoOveruseStats(mLatestIoOveruseStats);
        !status.isOk()) {
        // Don't clear the cache as it can be pushed again on the next collection.
        ALOGW("Failed to push the latest I/O overuse stats to watchdog service: %s",
              status.toString8().c_str());
    } else {
        mLatestIoOveruseStats.clear();
        if (DEBUG) {
            ALOGD("Pushed latest I/O overuse stats to watchdog service");
        }
    }
    return {};
}

Result<void> IoOveruseMonitor::onCustomCollection(
        time_t time, SystemState systemState,
        [[maybe_unused]] const std::unordered_set<std::string>& filterPackages,
        const android::wp<UidIoStats>& uidIoStats, const android::wp<ProcStat>& procStat,
        const android::wp<ProcPidStat>& procPidStat) {
    // Nothing special for custom collection.
    return onPeriodicCollection(time, systemState, uidIoStats, procStat, procPidStat);
}

Result<void> IoOveruseMonitor::onPeriodicMonitor(
        time_t time, const android::wp<IProcDiskStatsInterface>& procDiskStats,
        const std::function<void()>& alertHandler) {
    if (procDiskStats == nullptr) {
        return Error() << "Proc disk stats collector must not be null";
    }

    std::unique_lock writeLock(mRwMutex);
    if (mLastSystemWideIoMonitorTime == 0) {
        /*
         * Do not record the first disk stats as it reflects the aggregated disks stats since the
         * system boot up and is not in sync with the polling period. This will lead to spurious
         * I/O overuse alerting.
         */
        mLastSystemWideIoMonitorTime = time;
        return {};
    }
    const auto diskStats = procDiskStats.promote()->deltaSystemWideDiskStats();
    mSystemWideWrittenBytes.push_back(
            {.pollDurationInSecs = difftime(time, mLastSystemWideIoMonitorTime),
             .bytesInKib = diskStats.numKibWritten});
    for (const auto& threshold : mIoOveruseConfigs->systemWideAlertThresholds()) {
        int64_t accountedWrittenKib = 0;
        double accountedDurationInSecs = 0;
        size_t accountedPolls = 0;
        for (auto rit = mSystemWideWrittenBytes.rbegin(); rit != mSystemWideWrittenBytes.rend();
             ++rit) {
            accountedWrittenKib += rit->bytesInKib;
            accountedDurationInSecs += rit->pollDurationInSecs;
            ++accountedPolls;
            if (accountedDurationInSecs >= threshold.durationInSeconds) {
                break;
            }
        }
        // Heuristic to handle spurious alerting when the buffer is partially filled.
        if (const size_t bufferSize = mSystemWideWrittenBytes.size();
            accountedPolls == bufferSize && bufferSize < mPeriodicMonitorBufferSize + 1 &&
            threshold.durationInSeconds > accountedDurationInSecs) {
            continue;
        }
        const double thresholdKbps = threshold.writtenBytesPerSecond / 1024.0;
        if (const auto kbps = accountedWrittenKib / accountedDurationInSecs;
            kbps >= thresholdKbps) {
            alertHandler();
            break;
        }
    }
    if (mSystemWideWrittenBytes.size() > mPeriodicMonitorBufferSize) {
        mSystemWideWrittenBytes.erase(mSystemWideWrittenBytes.begin());  // Erase the oldest entry.
    }
    mLastSystemWideIoMonitorTime = time;
    return {};
}

Result<void> IoOveruseMonitor::onDump([[maybe_unused]] int fd) {
    // TODO(b/183436216): Dump the list of killed/disabled packages. Dump the list of packages that
    //  exceed xx% of their threshold.
    return {};
}

bool IoOveruseMonitor::dumpHelpText(int fd) {
    return WriteStringToFd(StringPrintf(kHelpText, name().c_str(), kResetResourceOveruseStatsFlag),
                           fd);
}

void IoOveruseMonitor::notifyNativePackagesLocked(
        const std::unordered_map<uid_t, IoOveruseStats>& statsByUid) {
    for (const auto& [uid, ioOveruseStats] : statsByUid) {
        IResourceOveruseListener* listener;
        if (const auto it = mOveruseListenersByUid.find(uid); it == mOveruseListenersByUid.end()) {
            continue;
        } else {
            listener = it->second.get();
        }
        ResourceOveruseStats stats;
        stats.set<ResourceOveruseStats::ioOveruseStats>(ioOveruseStats);
        listener->onOveruse(stats);
    }
    if (DEBUG) {
        ALOGD("Notified native packages on I/O overuse");
    }
    // TODO(b/184310189): Upload I/O overuse metrics for native packages.
}

Result<void> IoOveruseMonitor::updateResourceOveruseConfigurations(
        const std::vector<ResourceOveruseConfiguration>& configs) {
    std::unique_lock writeLock(mRwMutex);
    if (!isInitializedLocked()) {
        return Error(Status::EX_ILLEGAL_STATE) << name() << " is not initialized";
    }
    if (const auto result = mIoOveruseConfigs->update(configs); !result.ok()) {
        return result;
    }
    std::thread writeToDiskThread([&]() {
        if (set_sched_policy(0, SP_BACKGROUND) != 0) {
            ALOGW("Failed to set background scheduling priority for writing resource overuse "
                  "configs to disk");
        }
        if (int result = pthread_setname_np(pthread_self(), "ResOveruseCfgWr"); result != 0) {
            ALOGE("Failed to set thread name to 'ResOveruseCfgWr'");
        }
        std::unique_lock writeLock(mRwMutex);
        if (const auto result = mIoOveruseConfigs->writeToDisk(); !result.ok()) {
            ALOGE("Failed to write resource overuse configs to disk: %s",
                  result.error().message().c_str());
        }
    });
    writeToDiskThread.detach();
    return {};
}

Result<void> IoOveruseMonitor::getResourceOveruseConfigurations(
        std::vector<ResourceOveruseConfiguration>* configs) {
    std::shared_lock readLock(mRwMutex);
    if (!isInitializedLocked()) {
        return Error(Status::EX_ILLEGAL_STATE) << name() << " is not initialized";
    }
    mIoOveruseConfigs->get(configs);
    return {};
}

Result<void> IoOveruseMonitor::actionTakenOnIoOveruse(
        [[maybe_unused]] const std::vector<PackageResourceOveruseAction>& actions) {
    // TODO(b/184310189): Upload metrics.
    if (DEBUG) {
        ALOGD("Recorded action taken on I/O overuse");
    }
    return {};
}

Result<void> IoOveruseMonitor::addIoOveruseListener(const sp<IResourceOveruseListener>& listener) {
    pid_t callingPid = IPCThreadState::self()->getCallingPid();
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    std::unique_lock writeLock(mRwMutex);
    auto binder = BnResourceOveruseListener::asBinder(listener);
    if (findListenerAndProcessLocked(binder, nullptr)) {
        ALOGW("Failed to register the I/O overuse listener (pid: %d, uid: %d) as it is already "
              "registered",
              callingPid, callingUid);
        return {};
    }
    if (const auto status = binder->linkToDeath(mBinderDeathRecipient); status != OK) {
        return Error(Status::EX_ILLEGAL_STATE)
                << "(pid " << callingPid << ", uid: " << callingUid << ") is dead";
    }
    mOveruseListenersByUid[callingUid] = listener;
    if (DEBUG) {
        ALOGD("Added I/O overuse listener for uid: %d", callingUid);
    }
    return {};
}

Result<void> IoOveruseMonitor::removeIoOveruseListener(
        const sp<IResourceOveruseListener>& listener) {
    std::unique_lock writeLock(mRwMutex);
    const auto processor = [&](ListenersByUidMap& listeners, ListenersByUidMap::const_iterator it) {
        auto binder = BnResourceOveruseListener::asBinder(it->second);
        binder->unlinkToDeath(mBinderDeathRecipient);
        listeners.erase(it);
    };
    if (const auto binder = BnResourceOveruseListener::asBinder(listener);
        !findListenerAndProcessLocked(binder, processor)) {
        return Error(Status::EX_ILLEGAL_ARGUMENT) << "Listener is not previously registered";
    }
    if (DEBUG) {
        ALOGD("Removed I/O overuse listener for uid: %d", IPCThreadState::self()->getCallingUid());
    }
    return {};
}

Result<void> IoOveruseMonitor::getIoOveruseStats(IoOveruseStats* ioOveruseStats) {
    if (!isInitialized()) {
        return Error(Status::EX_ILLEGAL_STATE) << "I/O overuse monitor is not initialized";
    }
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    const auto packageInfosByUid = mPackageInfoResolver->getPackageInfosForUids({callingUid});
    const PackageInfo* packageInfo;
    if (const auto it = packageInfosByUid.find(callingUid); it == packageInfosByUid.end()) {
        return Error(Status::EX_ILLEGAL_ARGUMENT)
                << "Package information not available for calling UID(" << callingUid << ")";
    } else {
        packageInfo = &it->second;
    }
    std::shared_lock readLock(mRwMutex);
    const UserPackageIoUsage* dailyIoUsage;
    if (const auto it = mUserPackageDailyIoUsageById.find(
                uniquePackageIdStr(packageInfo->packageIdentifier));
        it == mUserPackageDailyIoUsageById.end()) {
        return Error(Status::EX_ILLEGAL_ARGUMENT)
                << "Calling UID " << callingUid << " doesn't have I/O overuse stats";
    } else {
        dailyIoUsage = &it->second;
    }
    ioOveruseStats->killableOnOveruse = mIoOveruseConfigs->isSafeToKill(*packageInfo);
    const auto thresholdBytes = mIoOveruseConfigs->fetchThreshold(*packageInfo);
    ioOveruseStats->remainingWriteBytes =
            diff(thresholdBytes,
                 diff(dailyIoUsage->writtenBytes, dailyIoUsage->forgivenWriteBytes));
    ioOveruseStats->totalOveruses = dailyIoUsage->totalOveruses;
    ioOveruseStats->writtenBytes = dailyIoUsage->writtenBytes;
    const auto [startTime, durationInSeconds] =
            calculateStartAndDuration(mLastUserPackageIoMonitorTime);
    ioOveruseStats->startTime = startTime;
    ioOveruseStats->durationInSeconds = durationInSeconds;
    if (DEBUG) {
        ALOGD("Returning I/O overuse stats for uid: %d", callingUid);
    }
    return {};
}

Result<void> IoOveruseMonitor::resetIoOveruseStats(const std::vector<std::string>& packageNames) {
    if (const auto status = mWatchdogServiceHelper->resetResourceOveruseStats(packageNames);
        !status.isOk()) {
        return Error() << "Failed to reset stats in watchdog service: " << status.toString8();
    }
    std::unordered_set<std::string> uniquePackageNames;
    std::copy(packageNames.begin(), packageNames.end(),
              std::inserter(uniquePackageNames, uniquePackageNames.end()));
    for (auto& [key, usage] : mUserPackageDailyIoUsageById) {
        if (uniquePackageNames.find(usage.packageInfo.packageIdentifier.name) !=
            uniquePackageNames.end()) {
            usage.resetStats();
        }
    }
    return {};
}

void IoOveruseMonitor::handleBinderDeath(const wp<IBinder>& who) {
    std::unique_lock writeLock(mRwMutex);
    IBinder* binder = who.unsafe_get();
    findListenerAndProcessLocked(binder,
                                 [&](ListenersByUidMap& listeners,
                                     ListenersByUidMap::const_iterator it) {
                                     ALOGW("Resource overuse notification handler died for uid(%d)",
                                           it->first);
                                     listeners.erase(it);
                                 });
}

bool IoOveruseMonitor::findListenerAndProcessLocked(const sp<IBinder>& binder,
                                                    const Processor& processor) {
    for (auto it = mOveruseListenersByUid.begin(); it != mOveruseListenersByUid.end(); ++it) {
        if (BnResourceOveruseListener::asBinder(it->second) != binder) {
            continue;
        }
        if (processor != nullptr) {
            processor(mOveruseListenersByUid, it);
        }
        return true;
    }
    return false;
}

IoOveruseMonitor::UserPackageIoUsage::UserPackageIoUsage(const PackageInfo& pkgInfo,
                                                         const IoUsage& ioUsage,
                                                         const bool isGarageModeActive) {
    packageInfo = pkgInfo;
    if (isGarageModeActive) {
        writtenBytes.garageModeBytes = ioUsage.sumWriteBytes();
    } else {
        writtenBytes.foregroundBytes = ioUsage.metrics[WRITE_BYTES][FOREGROUND];
        writtenBytes.backgroundBytes = ioUsage.metrics[WRITE_BYTES][BACKGROUND];
    }
}

IoOveruseMonitor::UserPackageIoUsage& IoOveruseMonitor::UserPackageIoUsage::operator+=(
        const UserPackageIoUsage& r) {
    if (id() == r.id()) {
        packageInfo = r.packageInfo;
    }
    const auto sum = [](const int64_t& l, const int64_t& r) -> int64_t {
        return (std::numeric_limits<int64_t>::max() - l) > r ? (l + r)
                                                             : std::numeric_limits<int64_t>::max();
    };
    writtenBytes.foregroundBytes =
            sum(writtenBytes.foregroundBytes, r.writtenBytes.foregroundBytes);
    writtenBytes.backgroundBytes =
            sum(writtenBytes.backgroundBytes, r.writtenBytes.backgroundBytes);
    writtenBytes.garageModeBytes =
            sum(writtenBytes.garageModeBytes, r.writtenBytes.garageModeBytes);

    return *this;
}

const std::string IoOveruseMonitor::UserPackageIoUsage::id() const {
    return uniquePackageIdStr(packageInfo.packageIdentifier);
}

void IoOveruseMonitor::UserPackageIoUsage::resetStats() {
    writtenBytes = {};
    forgivenWriteBytes = {};
    totalOveruses = 0;
    isPackageWarned = false;
    lastSyncedWrittenBytes = 0;
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
