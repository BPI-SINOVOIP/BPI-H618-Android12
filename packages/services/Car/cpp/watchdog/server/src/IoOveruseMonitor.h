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

#ifndef CPP_WATCHDOG_SERVER_SRC_IOOVERUSEMONITOR_H_
#define CPP_WATCHDOG_SERVER_SRC_IOOVERUSEMONITOR_H_

#include "IoOveruseConfigs.h"
#include "PackageInfoResolver.h"
#include "ProcPidStat.h"
#include "ProcStat.h"
#include "UidIoStats.h"
#include "WatchdogPerfService.h"

#include <android-base/result.h>
#include <android-base/stringprintf.h>
#include <android/automotive/watchdog/BnResourceOveruseListener.h>
#include <android/automotive/watchdog/PerStateBytes.h>
#include <android/automotive/watchdog/internal/ComponentType.h>
#include <android/automotive/watchdog/internal/IoOveruseConfiguration.h>
#include <android/automotive/watchdog/internal/PackageInfo.h>
#include <android/automotive/watchdog/internal/PackageIoOveruseStats.h>
#include <android/automotive/watchdog/internal/PackageResourceOveruseAction.h>
#include <utils/Mutex.h>

#include <time.h>

#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

// Number of periodically monitored stats to cache in memory.
constexpr int32_t kDefaultPeriodicMonitorBufferSize = 360;
// Dumpsys flags.
constexpr const char* kResetResourceOveruseStatsFlag = "--reset_resource_overuse_stats";

// Forward declaration for testing use only.
namespace internal {

class IoOveruseMonitorPeer;

}  // namespace internal

// Used only in tests.
std::tuple<int64_t, int64_t> calculateStartAndDuration(const time_t& currentTime);

/*
 * IIoOveruseMonitor interface defines the methods that the I/O overuse monitoring module
 * should implement.
 */
class IIoOveruseMonitor : virtual public IDataProcessorInterface {
public:
    // Returns whether or not the monitor is initialized.
    virtual bool isInitialized() = 0;

    // Dumps the help text.
    virtual bool dumpHelpText(int fd) = 0;

    // Below API is from internal/ICarWatchdog.aidl. Please refer to the AIDL for description.
    virtual android::base::Result<void> updateResourceOveruseConfigurations(
            const std::vector<
                    android::automotive::watchdog::internal::ResourceOveruseConfiguration>&
                    configs) = 0;
    virtual android::base::Result<void> getResourceOveruseConfigurations(
            std::vector<android::automotive::watchdog::internal::ResourceOveruseConfiguration>*
                    configs) = 0;
    virtual android::base::Result<void> actionTakenOnIoOveruse(
            const std::vector<
                    android::automotive::watchdog::internal::PackageResourceOveruseAction>&
                    actions) = 0;

    // Below methods support APIs from ICarWatchdog.aidl. Please refer to the AIDL for description.
    virtual android::base::Result<void> addIoOveruseListener(
            const sp<IResourceOveruseListener>& listener) = 0;

    virtual android::base::Result<void> removeIoOveruseListener(
            const sp<IResourceOveruseListener>& listener) = 0;

    virtual android::base::Result<void> getIoOveruseStats(IoOveruseStats* ioOveruseStats) = 0;

    virtual android::base::Result<void> resetIoOveruseStats(
            const std::vector<std::string>& packageNames) = 0;
};

class IoOveruseMonitor final : public IIoOveruseMonitor {
public:
    explicit IoOveruseMonitor(const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper);

    ~IoOveruseMonitor() { terminate(); }

    bool isInitialized() {
        std::shared_lock readLock(mRwMutex);
        return isInitializedLocked();
    }

    // Below methods implement IDataProcessorInterface.
    std::string name() { return "IoOveruseMonitor"; }
    friend std::ostream& operator<<(std::ostream& os, const IoOveruseMonitor& monitor);
    android::base::Result<void> onBoottimeCollection(
            time_t /*time*/, const android::wp<UidIoStats>& /*uidIoStats*/,
            const android::wp<ProcStat>& /*procStat*/,
            const android::wp<ProcPidStat>& /*procPidStat*/) {
        // No I/O overuse monitoring during boot-time.
        return {};
    }

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
            time_t time, const android::wp<IProcDiskStatsInterface>& procDiskStats,
            const std::function<void()>& alertHandler);

    android::base::Result<void> onDump(int fd);

    bool dumpHelpText(int fd);

    android::base::Result<void> onCustomCollectionDump(int /*fd*/) {
        // No special processing for custom collection. Thus no custom collection dump.
        return {};
    }

    // Below methods implement AIDL interfaces.
    android::base::Result<void> updateResourceOveruseConfigurations(
            const std::vector<
                    android::automotive::watchdog::internal::ResourceOveruseConfiguration>&
                    configs);

    android::base::Result<void> getResourceOveruseConfigurations(
            std::vector<android::automotive::watchdog::internal::ResourceOveruseConfiguration>*
                    configs);

    android::base::Result<void> actionTakenOnIoOveruse(
            const std::vector<
                    android::automotive::watchdog::internal::PackageResourceOveruseAction>&
                    actions);

    android::base::Result<void> addIoOveruseListener(const sp<IResourceOveruseListener>& listener);

    android::base::Result<void> removeIoOveruseListener(
            const sp<IResourceOveruseListener>& listener);

    android::base::Result<void> getIoOveruseStats(IoOveruseStats* ioOveruseStats);

    android::base::Result<void> resetIoOveruseStats(const std::vector<std::string>& packageName);

protected:
    android::base::Result<void> init();

    void terminate();

private:
    struct WrittenBytesSnapshot {
        double pollDurationInSecs;
        uint64_t bytesInKib;
    };

    struct UserPackageIoUsage {
        UserPackageIoUsage(const android::automotive::watchdog::internal::PackageInfo& packageInfo,
                           const IoUsage& IoUsage, const bool isGarageModeActive);
        android::automotive::watchdog::internal::PackageInfo packageInfo = {};
        PerStateBytes writtenBytes = {};
        PerStateBytes forgivenWriteBytes = {};
        int totalOveruses = 0;
        bool isPackageWarned = false;
        uint64_t lastSyncedWrittenBytes = 0;

        UserPackageIoUsage& operator+=(const UserPackageIoUsage& r);

        const std::string id() const;
        void resetStats();
    };

    class BinderDeathRecipient final : public android::IBinder::DeathRecipient {
    public:
        explicit BinderDeathRecipient(const android::sp<IoOveruseMonitor>& service) :
              mService(service) {}

        void binderDied(const android::wp<android::IBinder>& who) override {
            mService->handleBinderDeath(who);
        }

    private:
        android::sp<IoOveruseMonitor> mService;
    };

private:
    bool isInitializedLocked() { return mIoOveruseConfigs != nullptr; }

    void notifyNativePackagesLocked(const std::unordered_map<uid_t, IoOveruseStats>& statsByUid);

    void handleBinderDeath(const wp<IBinder>& who);

    using ListenersByUidMap = std::unordered_map<uid_t, android::sp<IResourceOveruseListener>>;
    using Processor = std::function<void(ListenersByUidMap&, ListenersByUidMap::const_iterator)>;
    bool findListenerAndProcessLocked(const sp<IBinder>& binder, const Processor& processor);

    /*
     * Writes in-memory configs to disk asynchronously if configs are not written after latest
     * update.
     */
    void writeConfigsToDiskAsyncLocked();

    // Local IPackageInfoResolver instance. Useful to mock in tests.
    sp<IPackageInfoResolver> mPackageInfoResolver;
    // Minimum written bytes to sync the stats with the Watchdog service.
    double mMinSyncWrittenBytes;
    android::sp<IWatchdogServiceHelper> mWatchdogServiceHelper;

    // Makes sure only one collection is running at any given time.
    mutable std::shared_mutex mRwMutex;

    // Summary of configs available for all the components and system-wide overuse alert thresholds.
    sp<IIoOveruseConfigs> mIoOveruseConfigs GUARDED_BY(mRwMutex);

    /*
     * Delta of system-wide written kib across all disks from the last |mPeriodicMonitorBufferSize|
     * polls along with the polling duration.
     */
    std::vector<WrittenBytesSnapshot> mSystemWideWrittenBytes GUARDED_BY(mRwMutex);
    size_t mPeriodicMonitorBufferSize GUARDED_BY(mRwMutex);
    time_t mLastSystemWideIoMonitorTime GUARDED_BY(mRwMutex);

    // Cache of per user package I/O usage. Key is a unique ID with the format `packageName:userId`.
    std::unordered_map<std::string, UserPackageIoUsage> mUserPackageDailyIoUsageById
            GUARDED_BY(mRwMutex);
    double mIoOveruseWarnPercentage GUARDED_BY(mRwMutex);
    time_t mLastUserPackageIoMonitorTime GUARDED_BY(mRwMutex);
    std::vector<android::automotive::watchdog::internal::PackageIoOveruseStats>
            mLatestIoOveruseStats;

    ListenersByUidMap mOveruseListenersByUid GUARDED_BY(mRwMutex);
    android::sp<BinderDeathRecipient> mBinderDeathRecipient;

    friend class WatchdogPerfService;

    // For unit tests.
    friend class internal::IoOveruseMonitorPeer;
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_SRC_IOOVERUSEMONITOR_H_
