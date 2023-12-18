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

#ifndef CPP_WATCHDOG_SERVER_SRC_WATCHDOGPERFSERVICE_H_
#define CPP_WATCHDOG_SERVER_SRC_WATCHDOGPERFSERVICE_H_

#include "LooperWrapper.h"
#include "ProcDiskStats.h"
#include "ProcPidStat.h"
#include "ProcStat.h"
#include "UidIoStats.h"

#include <android-base/chrono_utils.h>
#include <android-base/result.h>
#include <android/automotive/watchdog/internal/PowerCycle.h>
#include <cutils/multiuser.h>
#include <gtest/gtest_prod.h>
#include <utils/Errors.h>
#include <utils/Looper.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <utils/Vector.h>

#include <time.h>

#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <unordered_set>

namespace android {
namespace automotive {
namespace watchdog {

// Forward declaration for testing use only.
namespace internal {

class WatchdogPerfServicePeer;

}  // namespace internal

constexpr const char* kStartCustomCollectionFlag = "--start_perf";
constexpr const char* kEndCustomCollectionFlag = "--stop_perf";
constexpr const char* kIntervalFlag = "--interval";
constexpr const char* kMaxDurationFlag = "--max_duration";
constexpr const char* kFilterPackagesFlag = "--filter_packages";

enum SystemState {
    NORMAL_MODE = 0,
    GARAGE_MODE = 1,
};

/*
 * DataProcessor defines methods that must be implemented in order to process the data collected
 * by |WatchdogPerfService|.
 */
class IDataProcessorInterface : public android::RefBase {
public:
    IDataProcessorInterface() {}
    virtual ~IDataProcessorInterface() {}
    // Returns the name of the data processor.
    virtual std::string name() = 0;
    // Callback to initialize the data processor.
    virtual android::base::Result<void> init() = 0;
    // Callback to terminate the data processor.
    virtual void terminate() = 0;
    // Callback to process the data collected during boot-time.
    virtual android::base::Result<void> onBoottimeCollection(
            time_t time, const android::wp<UidIoStats>& uidIoStats,
            const android::wp<ProcStat>& procStat, const android::wp<ProcPidStat>& procPidStat) = 0;
    // Callback to process the data collected periodically post boot complete.
    virtual android::base::Result<void> onPeriodicCollection(
            time_t time, SystemState systemState, const android::wp<UidIoStats>& uidIoStats,
            const android::wp<ProcStat>& procStat, const android::wp<ProcPidStat>& procPidStat) = 0;
    /*
     * Callback to process the data collected on custom collection and filter the results only to
     * the specified |filterPackages|.
     */
    virtual android::base::Result<void> onCustomCollection(
            time_t time, SystemState systemState,
            const std::unordered_set<std::string>& filterPackages,
            const android::wp<UidIoStats>& uidIoStats, const android::wp<ProcStat>& procStat,
            const android::wp<ProcPidStat>& procPidStat) = 0;
    /*
     * Callback to periodically monitor the collected data and trigger the given |alertHandler|
     * on detecting resource overuse.
     */
    virtual android::base::Result<void> onPeriodicMonitor(
            time_t time, const android::wp<IProcDiskStatsInterface>& procDiskStats,
            const std::function<void()>& alertHandler) = 0;
    // Callback to dump the boot-time collected and periodically collected data.
    virtual android::base::Result<void> onDump(int fd) = 0;
    /*
     * Callback to dump the custom collected data. When fd == -1, clear the custom collection cache.
     */
    virtual android::base::Result<void> onCustomCollectionDump(int fd) = 0;
};

enum EventType {
    // WatchdogPerfService's state.
    INIT = 0,
    TERMINATED,

    // Collection events.
    BOOT_TIME_COLLECTION,
    PERIODIC_COLLECTION,
    CUSTOM_COLLECTION,

    // Monitor event.
    PERIODIC_MONITOR,

    LAST_EVENT,
};

enum SwitchMessage {
    /*
     * On receiving this message, collect the last boot-time record and start periodic collection
     * and monitor.
     */
    END_BOOTTIME_COLLECTION = EventType::LAST_EVENT + 1,

    /*
     * On receiving this message, ends custom collection, discard collected data and start periodic
     * collection and monitor.
     */
    END_CUSTOM_COLLECTION,
};

/*
 * WatchdogPerfServiceInterface collects performance data during boot-time and periodically post
 * boot complete. It exposes APIs that the main thread and binder service can call to start a
 * collection, switch the collection type, and generate collection dumps.
 */
class WatchdogPerfServiceInterface : public MessageHandler {
public:
    // Register a data processor to process the data collected by |WatchdogPerfService|.
    virtual android::base::Result<void> registerDataProcessor(
            android::sp<IDataProcessorInterface> processor) = 0;
    /*
     * Starts the boot-time collection in the looper handler on a new thread and returns
     * immediately. Must be called only once. Otherwise, returns an error.
     */
    virtual android::base::Result<void> start() = 0;
    // Terminates the collection thread and returns.
    virtual void terminate() = 0;
    // Sets the system state.
    virtual void setSystemState(SystemState systemState) = 0;
    // Ends the boot-time collection by switching to periodic collection and returns immediately.
    virtual android::base::Result<void> onBootFinished() = 0;
    /*
     * Depending on the arguments, it either:
     * 1. Starts a custom collection.
     * 2. Or ends the current custom collection and dumps the collected data.
     * Returns any error observed during the dump generation.
     */
    virtual android::base::Result<void> onCustomCollection(
            int fd, const Vector<android::String16>& args) = 0;
    // Generates a dump from the boot-time and periodic collection events.
    virtual android::base::Result<void> onDump(int fd) = 0;
    // Dumps the help text.
    virtual bool dumpHelpText(int fd) = 0;
};

class WatchdogPerfService final : public WatchdogPerfServiceInterface {
public:
    WatchdogPerfService() :
          mHandlerLooper(android::sp<LooperWrapper>::make()),
          mSystemState(NORMAL_MODE),
          mBoottimeCollection({}),
          mPeriodicCollection({}),
          mCustomCollection({}),
          mPeriodicMonitor({}),
          mCurrCollectionEvent(EventType::INIT),
          mUidIoStats(android::sp<UidIoStats>::make()),
          mProcStat(android::sp<ProcStat>::make()),
          mProcPidStat(android::sp<ProcPidStat>::make()),
          mProcDiskStats(android::sp<ProcDiskStats>::make()),
          mDataProcessors({}) {}

    ~WatchdogPerfService() { terminate(); }

    android::base::Result<void> registerDataProcessor(
            android::sp<IDataProcessorInterface> processor) override;

    android::base::Result<void> start() override;

    void terminate() override;

    void setSystemState(SystemState systemState) override;

    android::base::Result<void> onBootFinished() override;

    android::base::Result<void> onCustomCollection(int fd,
                                                   const Vector<android::String16>& args) override;

    android::base::Result<void> onDump(int fd) override;

    bool dumpHelpText(int fd) override;

private:
    struct EventMetadata {
        // Collection or monitor event.
        EventType eventType = EventType::LAST_EVENT;
        // Interval between subsequent events.
        std::chrono::nanoseconds interval = 0ns;
        // Used to calculate the uptime for next event.
        nsecs_t lastUptime = 0;
        // Filter the results only to the specified packages.
        std::unordered_set<std::string> filterPackages;

        std::string toString() const;
    };

    // Dumps the collectors' status when they are disabled.
    android::base::Result<void> dumpCollectorsStatusLocked(int fd);

    /*
     * Starts a custom collection on the looper handler, temporarily stops the periodic collection
     * (won't discard the collected data), and returns immediately. Returns any error observed
     * during this process.
     * The custom collection happens once every |interval| seconds. When the |maxDuration| is
     * reached, the looper receives a message to end the collection, discards the collected data,
     * and starts the periodic collection. This is needed to ensure the custom collection doesn't
     * run forever when a subsequent |endCustomCollection| call is not received.
     * When |kFilterPackagesFlag| value specified, the results are filtered only to the specified
     * package names.
     */
    android::base::Result<void> startCustomCollection(
            std::chrono::nanoseconds interval, std::chrono::nanoseconds maxDuration,
            const std::unordered_set<std::string>& filterPackages);

    /*
     * Ends the current custom collection, generates a dump, sends a looper message to start the
     * periodic collection, and returns immediately. Returns an error when there is no custom
     * collection running or when a dump couldn't be generated from the custom collection.
     */
    android::base::Result<void> endCustomCollection(int fd);

    // Handles the messages received by the lopper.
    void handleMessage(const Message& message) override;

    // Processes the collection events received by |handleMessage|.
    android::base::Result<void> processCollectionEvent(EventMetadata* metadata);

    // Collects/processes the performance data for the current collection event.
    android::base::Result<void> collectLocked(EventMetadata* metadata);

    // Processes the monitor events received by |handleMessage|.
    android::base::Result<void> processMonitorEvent(EventMetadata* metadata);

    /*
     * Returns the metadata for the current collection based on |mCurrCollectionEvent|. Returns
     * nullptr on invalid collection event.
     */
    EventMetadata* currCollectionMetadataLocked();

    // Thread on which the actual collection happens.
    std::thread mCollectionThread;

    // Makes sure only one collection is running at any given time.
    Mutex mMutex;

    // Handler lopper to execute different collection events on the collection thread.
    android::sp<LooperWrapper> mHandlerLooper GUARDED_BY(mMutex);

    // Current system state.
    SystemState mSystemState GUARDED_BY(mMutex);

    // Info for the |CollectionEvent::BOOT_TIME| collection event.
    EventMetadata mBoottimeCollection GUARDED_BY(mMutex);

    // Info for the |CollectionEvent::PERIODIC| collection event.
    EventMetadata mPeriodicCollection GUARDED_BY(mMutex);

    /*
     * Info for the |CollectionEvent::CUSTOM| collection event. The info is cleared at the end of
     * every custom collection.
     */
    EventMetadata mCustomCollection GUARDED_BY(mMutex);

    // Info for the |EventType::PERIODIC| monitor event.
    EventMetadata mPeriodicMonitor GUARDED_BY(mMutex);

    /*
     * Tracks either the WatchdogPerfService's state or current collection event. Updated on
     * |start|, |onBootComplete|, |startCustomCollection|, |endCustomCollection|, and |terminate|.
     */
    EventType mCurrCollectionEvent GUARDED_BY(mMutex);

    // Collector/parser for `/proc/uid_io/stats`.
    android::sp<UidIoStats> mUidIoStats GUARDED_BY(mMutex);

    // Collector/parser for `/proc/stat`.
    android::sp<ProcStat> mProcStat GUARDED_BY(mMutex);

    // Collector/parser for `/proc/PID/*` stat files.
    android::sp<ProcPidStat> mProcPidStat GUARDED_BY(mMutex);

    // Collector/parser for `/proc/diskstats` file.
    android::sp<IProcDiskStatsInterface> mProcDiskStats GUARDED_BY(mMutex);

    // Data processors for the collected performance data.
    std::vector<android::sp<IDataProcessorInterface>> mDataProcessors GUARDED_BY(mMutex);

    // For unit tests.
    friend class internal::WatchdogPerfServicePeer;
    FRIEND_TEST(WatchdogPerfServiceTest, TestServiceStartAndTerminate);
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  //  CPP_WATCHDOG_SERVER_SRC_WATCHDOGPERFSERVICE_H_
