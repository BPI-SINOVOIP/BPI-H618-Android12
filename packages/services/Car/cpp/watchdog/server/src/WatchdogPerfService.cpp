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

#include "WatchdogPerfService.h"

#include <WatchdogProperties.sysprop.h>
#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <log/log.h>
#include <processgroup/sched_policy.h>

#include <pthread.h>

#include <iterator>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::sp;
using ::android::String16;
using ::android::String8;
using ::android::automotive::watchdog::internal::PowerCycle;
using ::android::base::Error;
using ::android::base::Join;
using ::android::base::ParseUint;
using ::android::base::Result;
using ::android::base::Split;
using ::android::base::StringAppendF;
using ::android::base::StringPrintf;
using ::android::base::WriteStringToFd;

namespace {

// Minimum required collection interval between subsequent collections.
const std::chrono::nanoseconds kMinEventInterval = 1s;
const std::chrono::seconds kDefaultBoottimeCollectionInterval = 1s;
const std::chrono::seconds kDefaultPeriodicCollectionInterval = 20s;
const std::chrono::seconds kDefaultPeriodicMonitorInterval = 5s;
const std::chrono::nanoseconds kCustomCollectionInterval = 10s;
const std::chrono::nanoseconds kCustomCollectionDuration = 30min;

constexpr const char* kServiceName = "WatchdogPerfService";
static const std::string kDumpMajorDelimiter = std::string(100, '-') + "\n";  // NOLINT
constexpr const char* kHelpText =
        "\n%s dump options:\n"
        "%s: Starts custom performance data collection. Customize the collection behavior with "
        "the following optional arguments:\n"
        "\t%s <seconds>: Modifies the collection interval. Default behavior is to collect once "
        "every %lld seconds.\n"
        "\t%s <seconds>: Modifies the maximum collection duration. Default behavior is to collect "
        "until %ld minutes before automatically stopping the custom collection and discarding "
        "the collected data.\n"
        "\t%s <package name>,<package name>,...: Comma-separated value containing package names. "
        "When provided, the results are filtered only to the provided package names. Default "
        "behavior is to list the results for the top N packages.\n"
        "%s: Stops custom performance data collection and generates a dump of "
        "the collection report.\n\n"
        "When no options are specified, the carwatchdog report contains the performance data "
        "collected during boot-time and over the last few minutes before the report generation.\n";

Result<std::chrono::seconds> parseSecondsFlag(const Vector<String16>& args, size_t pos) {
    if (args.size() <= pos) {
        return Error() << "Value not provided";
    }
    uint64_t value;
    if (std::string strValue = std::string(String8(args[pos]).string());
        !ParseUint(strValue, &value)) {
        return Error() << "Invalid value " << strValue << ", must be an integer";
    }
    return std::chrono::seconds(value);
}

constexpr const char* toString(std::variant<EventType, SwitchMessage> what) {
    return std::visit(
            [&](const auto& v) -> const char* {
                switch (static_cast<int>(v)) {
                    case EventType::INIT:
                        return "INIT";
                    case EventType::TERMINATED:
                        return "TERMINATED";
                    case EventType::BOOT_TIME_COLLECTION:
                        return "BOOT_TIME_COLLECTION";
                    case EventType::PERIODIC_COLLECTION:
                        return "PERIODIC_COLLECTION";
                    case EventType::CUSTOM_COLLECTION:
                        return "CUSTOM_COLLECTION";
                    case EventType::PERIODIC_MONITOR:
                        return "PERIODIC_MONITOR";
                    case EventType::LAST_EVENT:
                        return "LAST_EVENT";
                    case SwitchMessage::END_BOOTTIME_COLLECTION:
                        return "END_BOOTTIME_COLLECTION";
                    case SwitchMessage::END_CUSTOM_COLLECTION:
                        return "END_CUSTOM_COLLECTION";
                    default:
                        return "INVALID_EVENT_OR_SWITCH_MESSAGE";
                }
            },
            what);
}

constexpr const char* toString(SystemState systemState) {
    switch (systemState) {
        case SystemState::NORMAL_MODE:
            return "NORMAL_MODE";
        case SystemState::GARAGE_MODE:
            return "GARAGE_MODE";
        default:
            return "UNKNOWN MODE";
    }
}

}  // namespace

std::string WatchdogPerfService::EventMetadata::toString() const {
    std::string buffer;
    const auto intervalInSecs = std::chrono::duration_cast<std::chrono::seconds>(interval).count();
    StringAppendF(&buffer, "Event interval: %lld second%s\n", intervalInSecs,
                  ((intervalInSecs > 1) ? "s" : ""));
    if (!filterPackages.empty()) {
        std::vector<std::string> packages(filterPackages.begin(), filterPackages.end());
        StringAppendF(&buffer, "Filtered results to packages: %s\n", Join(packages, ", ").c_str());
    }
    return buffer;
}

Result<void> WatchdogPerfService::registerDataProcessor(sp<IDataProcessorInterface> processor) {
    if (processor == nullptr) {
        return Error() << "Must provide a valid data processor";
    }
    if (const auto result = processor->init(); !result.ok()) {
        return Error() << "Failed to initialize " << processor->name().c_str() << ": "
                       << result.error().message();
    }
    Mutex::Autolock lock(mMutex);
    mDataProcessors.push_back(processor);
    if (DEBUG) {
        ALOGD("Successfully registered %s to %s", processor->name().c_str(), kServiceName);
    }
    return {};
}

Result<void> WatchdogPerfService::start() {
    {
        Mutex::Autolock lock(mMutex);
        if (mCurrCollectionEvent != EventType::INIT || mCollectionThread.joinable()) {
            return Error(INVALID_OPERATION) << "Cannot start " << kServiceName << " more than once";
        }
        std::chrono::nanoseconds boottimeCollectionInterval =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::seconds(sysprop::boottimeCollectionInterval().value_or(
                                kDefaultBoottimeCollectionInterval.count())));
        std::chrono::nanoseconds periodicCollectionInterval =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::seconds(sysprop::periodicCollectionInterval().value_or(
                                kDefaultPeriodicCollectionInterval.count())));
        std::chrono::nanoseconds periodicMonitorInterval =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::seconds(sysprop::periodicMonitorInterval().value_or(
                                kDefaultPeriodicMonitorInterval.count())));
        mBoottimeCollection = {
                .eventType = EventType::BOOT_TIME_COLLECTION,
                .interval = boottimeCollectionInterval,
                .lastUptime = 0,
        };
        mPeriodicCollection = {
                .eventType = EventType::PERIODIC_COLLECTION,
                .interval = periodicCollectionInterval,
                .lastUptime = 0,
        };
        mPeriodicMonitor = {
                .eventType = EventType::PERIODIC_MONITOR,
                .interval = periodicMonitorInterval,
                .lastUptime = 0,
        };
        if (mDataProcessors.empty()) {
            ALOGE("Terminating %s: No data processor is registered", kServiceName);
            mCurrCollectionEvent = EventType::TERMINATED;
            return Error() << "No data processor is registered";
        }
    }

    mCollectionThread = std::thread([&]() {
        {
            Mutex::Autolock lock(mMutex);
            if (EventType expected = EventType::INIT; mCurrCollectionEvent != expected) {
                ALOGE("Skipping performance data collection as the current collection event "
                      "%s != %s",
                      toString(mCurrCollectionEvent), toString(expected));
                return;
            }
            mCurrCollectionEvent = EventType::BOOT_TIME_COLLECTION;
            mBoottimeCollection.lastUptime = mHandlerLooper->now();
            mHandlerLooper->setLooper(Looper::prepare(/*opts=*/0));
            mHandlerLooper->sendMessage(this, EventType::BOOT_TIME_COLLECTION);
        }
        if (set_sched_policy(0, SP_BACKGROUND) != 0) {
            ALOGW("Failed to set background scheduling priority to %s thread", kServiceName);
        }
        if (int result = pthread_setname_np(pthread_self(), "WatchdogPerfSvc"); result != 0) {
            ALOGE("Failed to set %s thread name: %d", kServiceName, result);
        }
        ALOGI("Starting %s performance data collection", toString(mCurrCollectionEvent));
        bool isCollectionActive = true;
        /*
         * Loop until the collection is not active -- performance collection runs on this thread in
         * a handler.
         */
        while (isCollectionActive) {
            mHandlerLooper->pollAll(/*timeoutMillis=*/-1);
            Mutex::Autolock lock(mMutex);
            isCollectionActive = mCurrCollectionEvent != EventType::TERMINATED;
        }
    });
    return {};
}

void WatchdogPerfService::terminate() {
    {
        Mutex::Autolock lock(mMutex);
        if (mCurrCollectionEvent == EventType::TERMINATED) {
            ALOGE("%s was terminated already", kServiceName);
            return;
        }
        ALOGE("Terminating %s as carwatchdog is terminating", kServiceName);
        if (mCurrCollectionEvent != EventType::INIT) {
            /*
             * Looper runs only after EventType::TNIT has completed so remove looper messages
             * and wake the looper only when the current collection has changed from INIT.
             */
            mHandlerLooper->removeMessages(this);
            mHandlerLooper->wake();
        }
        for (const auto& processor : mDataProcessors) {
            processor->terminate();
        }
        mCurrCollectionEvent = EventType::TERMINATED;
    }
    if (mCollectionThread.joinable()) {
        mCollectionThread.join();
        if (DEBUG) {
            ALOGD("%s collection thread terminated", kServiceName);
        }
    }
}

void WatchdogPerfService::setSystemState(SystemState systemState) {
    Mutex::Autolock lock(mMutex);
    if (mSystemState != systemState) {
        ALOGI("%s switching from %s to %s", kServiceName, toString(mSystemState),
              toString(systemState));
    }
    mSystemState = systemState;
}

Result<void> WatchdogPerfService::onBootFinished() {
    Mutex::Autolock lock(mMutex);
    if (EventType expected = EventType::BOOT_TIME_COLLECTION; mCurrCollectionEvent != expected) {
        /*
         * This case happens when either the WatchdogPerfService has prematurely terminated before
         * boot complete notification is received or multiple boot complete notifications are
         * received. In either case don't return error as this will lead to runtime exception and
         * cause system to boot loop.
         */
        ALOGE("Current performance data collection event %s != %s", toString(mCurrCollectionEvent),
              toString(expected));
        return {};
    }
    mBoottimeCollection.lastUptime = mHandlerLooper->now();
    mHandlerLooper->removeMessages(this);
    mHandlerLooper->sendMessage(this, SwitchMessage::END_BOOTTIME_COLLECTION);
    if (DEBUG) {
        ALOGD("Boot-time event finished");
    }
    return {};
}

Result<void> WatchdogPerfService::onCustomCollection(int fd, const Vector<String16>& args) {
    if (args.empty()) {
        return Error(BAD_VALUE) << "No custom collection dump arguments";
    }

    if (args[0] == String16(kStartCustomCollectionFlag)) {
        if (args.size() > 7) {
            return Error(BAD_VALUE) << "Number of arguments to start custom performance data "
                                    << "collection cannot exceed 7";
        }
        std::chrono::nanoseconds interval = kCustomCollectionInterval;
        std::chrono::nanoseconds maxDuration = kCustomCollectionDuration;
        std::unordered_set<std::string> filterPackages;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == String16(kIntervalFlag)) {
                const auto& result = parseSecondsFlag(args, i + 1);
                if (!result.ok()) {
                    return Error(BAD_VALUE)
                            << "Failed to parse " << kIntervalFlag << ": " << result.error();
                }
                interval = std::chrono::duration_cast<std::chrono::nanoseconds>(*result);
                ++i;
                continue;
            }
            if (args[i] == String16(kMaxDurationFlag)) {
                const auto& result = parseSecondsFlag(args, i + 1);
                if (!result.ok()) {
                    return Error(BAD_VALUE)
                            << "Failed to parse " << kMaxDurationFlag << ": " << result.error();
                }
                maxDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(*result);
                ++i;
                continue;
            }
            if (args[i] == String16(kFilterPackagesFlag)) {
                if (args.size() < i + 1) {
                    return Error(BAD_VALUE)
                            << "Must provide value for '" << kFilterPackagesFlag << "' flag";
                }
                std::vector<std::string> packages =
                        Split(std::string(String8(args[i + 1]).string()), ",");
                std::copy(packages.begin(), packages.end(),
                          std::inserter(filterPackages, filterPackages.end()));
                ++i;
                continue;
            }
            ALOGW("Unknown flag %s provided to start custom performance data collection",
                  String8(args[i]).string());
            return Error(BAD_VALUE) << "Unknown flag " << String8(args[i]).string()
                                    << " provided to start custom performance data collection";
        }
        if (const auto& result = startCustomCollection(interval, maxDuration, filterPackages);
            !result.ok()) {
            WriteStringToFd(result.error().message(), fd);
            return result;
        }
        return {};
    }

    if (args[0] == String16(kEndCustomCollectionFlag)) {
        if (args.size() != 1) {
            ALOGW("Number of arguments to stop custom performance data collection cannot exceed 1. "
                  "Stopping the data collection.");
            WriteStringToFd("Number of arguments to stop custom performance data collection "
                            "cannot exceed 1. Stopping the data collection.",
                            fd);
        }
        return endCustomCollection(fd);
    }

    return Error(BAD_VALUE) << "Custom perf collection dump arguments start neither with "
                            << kStartCustomCollectionFlag << " nor with "
                            << kEndCustomCollectionFlag << " flags";
}

Result<void> WatchdogPerfService::onDump(int fd) {
    Mutex::Autolock lock(mMutex);
    if (mCurrCollectionEvent == EventType::TERMINATED) {
        ALOGW("%s not active. Dumping cached data", kServiceName);
        if (!WriteStringToFd(StringPrintf("%s not active. Dumping cached data.", kServiceName),
                             fd)) {
            return Error(FAILED_TRANSACTION) << "Failed to write " << kServiceName << " status";
        }
    }

    if (const auto& result = dumpCollectorsStatusLocked(fd); !result.ok()) {
        return Error(FAILED_TRANSACTION) << result.error();
    }

    if (!WriteStringToFd(StringPrintf("\n%s%s report:\n%sBoot-time collection information:\n%s\n",
                                      kDumpMajorDelimiter.c_str(), kServiceName,
                                      kDumpMajorDelimiter.c_str(), std::string(33, '=').c_str()),
                         fd) ||
        !WriteStringToFd(mBoottimeCollection.toString(), fd) ||
        !WriteStringToFd(StringPrintf("\nPeriodic collection information:\n%s\n",
                                      std::string(32, '=').c_str()),
                         fd) ||
        !WriteStringToFd(mPeriodicCollection.toString(), fd)) {
        return Error(FAILED_TRANSACTION)
                << "Failed to dump the boot-time and periodic collection reports.";
    }

    for (const auto& processor : mDataProcessors) {
        if (const auto result = processor->onDump(fd); !result.ok()) {
            return result;
        }
    }

    WriteStringToFd(kDumpMajorDelimiter, fd);
    return {};
}

bool WatchdogPerfService::dumpHelpText(int fd) {
    return WriteStringToFd(StringPrintf(kHelpText, kServiceName, kStartCustomCollectionFlag,
                                        kIntervalFlag,
                                        std::chrono::duration_cast<std::chrono::seconds>(
                                                kCustomCollectionInterval)
                                                .count(),
                                        kMaxDurationFlag,
                                        std::chrono::duration_cast<std::chrono::minutes>(
                                                kCustomCollectionDuration)
                                                .count(),
                                        kFilterPackagesFlag, kEndCustomCollectionFlag),
                           fd);
}

Result<void> WatchdogPerfService::dumpCollectorsStatusLocked(int fd) {
    if (!mUidIoStats->enabled() &&
        !WriteStringToFd(StringPrintf("UidIoStats collector failed to access the file %s",
                                      mUidIoStats->filePath().c_str()),
                         fd)) {
        return Error() << "Failed to write UidIoStats collector status";
    }
    if (!mProcStat->enabled() &&
        !WriteStringToFd(StringPrintf("ProcStat collector failed to access the file %s",
                                      mProcStat->filePath().c_str()),
                         fd)) {
        return Error() << "Failed to write ProcStat collector status";
    }
    if (!mProcPidStat->enabled() &&
        !WriteStringToFd(StringPrintf("ProcPidStat collector failed to access the directory %s",
                                      mProcPidStat->dirPath().c_str()),
                         fd)) {
        return Error() << "Failed to write ProcPidStat collector status";
    }
    return {};
}

Result<void> WatchdogPerfService::startCustomCollection(
        std::chrono::nanoseconds interval, std::chrono::nanoseconds maxDuration,
        const std::unordered_set<std::string>& filterPackages) {
    if (interval < kMinEventInterval || maxDuration < kMinEventInterval) {
        return Error(INVALID_OPERATION)
                << "Collection interval and maximum duration must be >= "
                << std::chrono::duration_cast<std::chrono::milliseconds>(kMinEventInterval).count()
                << " milliseconds.";
    }
    Mutex::Autolock lock(mMutex);
    if (EventType expected = EventType::PERIODIC_COLLECTION; mCurrCollectionEvent != expected) {
        return Error(INVALID_OPERATION)
                << "Cannot start a custom collection when the current collection event "
                << toString(mCurrCollectionEvent) << " != " << toString(expected)
                << " collection event";
    }

    mCustomCollection = {
            .eventType = EventType::CUSTOM_COLLECTION,
            .interval = interval,
            .lastUptime = mHandlerLooper->now(),
            .filterPackages = filterPackages,
    };

    mHandlerLooper->removeMessages(this);
    nsecs_t uptime = mHandlerLooper->now() + maxDuration.count();
    mHandlerLooper->sendMessageAtTime(uptime, this, SwitchMessage::END_CUSTOM_COLLECTION);
    mCurrCollectionEvent = EventType::CUSTOM_COLLECTION;
    mHandlerLooper->sendMessage(this, EventType::CUSTOM_COLLECTION);
    ALOGI("Starting %s performance data collection", toString(mCurrCollectionEvent));
    return {};
}

Result<void> WatchdogPerfService::endCustomCollection(int fd) {
    Mutex::Autolock lock(mMutex);
    if (mCurrCollectionEvent != EventType::CUSTOM_COLLECTION) {
        return Error(INVALID_OPERATION) << "No custom collection is running";
    }

    mHandlerLooper->removeMessages(this);
    mHandlerLooper->sendMessage(this, SwitchMessage::END_CUSTOM_COLLECTION);

    if (const auto result = dumpCollectorsStatusLocked(fd); !result.ok()) {
        return Error(FAILED_TRANSACTION) << result.error();
    }

    if (!WriteStringToFd(StringPrintf("%sPerformance data report for custom collection:\n%s",
                                      kDumpMajorDelimiter.c_str(), kDumpMajorDelimiter.c_str()),
                         fd) ||
        !WriteStringToFd(mCustomCollection.toString(), fd)) {
        return Error(FAILED_TRANSACTION) << "Failed to write custom collection report.";
    }

    for (const auto& processor : mDataProcessors) {
        if (const auto result = processor->onCustomCollectionDump(fd); !result.ok()) {
            return Error() << processor->name() << " failed on " << toString(mCurrCollectionEvent)
                           << " collection: " << result.error();
        }
    }

    if (DEBUG) {
        ALOGD("Custom event finished");
    }
    WriteStringToFd(kDumpMajorDelimiter, fd);
    return {};
}

void WatchdogPerfService::handleMessage(const Message& message) {
    Result<void> result;

    auto switchToPeriodicLocked = [&](bool startNow) {
        mHandlerLooper->removeMessages(this);
        mCurrCollectionEvent = EventType::PERIODIC_COLLECTION;
        mPeriodicCollection.lastUptime = mHandlerLooper->now();
        if (startNow) {
            mHandlerLooper->sendMessage(this, EventType::PERIODIC_COLLECTION);
        } else {
            mPeriodicCollection.lastUptime += mPeriodicCollection.interval.count();
            mHandlerLooper->sendMessageAtTime(mPeriodicCollection.lastUptime, this,
                                              EventType::PERIODIC_COLLECTION);
        }
        mPeriodicMonitor.lastUptime = mHandlerLooper->now() + mPeriodicMonitor.interval.count();
        mHandlerLooper->sendMessageAtTime(mPeriodicMonitor.lastUptime, this,
                                          EventType::PERIODIC_MONITOR);
        ALOGI("Switching to %s and %s", toString(mCurrCollectionEvent),
              toString(EventType::PERIODIC_MONITOR));
    };

    switch (message.what) {
        case static_cast<int>(EventType::BOOT_TIME_COLLECTION):
            result = processCollectionEvent(&mBoottimeCollection);
            break;
        case static_cast<int>(SwitchMessage::END_BOOTTIME_COLLECTION):
            if (result = processCollectionEvent(&mBoottimeCollection); result.ok()) {
                Mutex::Autolock lock(mMutex);
                switchToPeriodicLocked(/*startNow=*/false);
            }
            break;
        case static_cast<int>(EventType::PERIODIC_COLLECTION):
            result = processCollectionEvent(&mPeriodicCollection);
            break;
        case static_cast<int>(EventType::CUSTOM_COLLECTION):
            result = processCollectionEvent(&mCustomCollection);
            break;
        case static_cast<int>(EventType::PERIODIC_MONITOR):
            result = processMonitorEvent(&mPeriodicMonitor);
            break;
        case static_cast<int>(SwitchMessage::END_CUSTOM_COLLECTION): {
            Mutex::Autolock lock(mMutex);
            if (EventType expected = EventType::CUSTOM_COLLECTION;
                mCurrCollectionEvent != expected) {
                ALOGW("Skipping END_CUSTOM_COLLECTION message as the current collection %s != %s",
                      toString(mCurrCollectionEvent), toString(expected));
                return;
            }
            mCustomCollection = {};
            for (const auto& processor : mDataProcessors) {
                /*
                 * Clear custom collection cache on the data processors when the custom collection
                 * ends.
                 */
                processor->onCustomCollectionDump(-1);
            }
            switchToPeriodicLocked(/*startNow=*/true);
            return;
        }
        default:
            result = Error() << "Unknown message: " << message.what;
    }

    if (!result.ok()) {
        Mutex::Autolock lock(mMutex);
        ALOGE("Terminating %s: %s", kServiceName, result.error().message().c_str());
        /*
         * DO NOT CALL terminate() as it tries to join the collection thread but this code is
         * executed on the collection thread. Thus it will result in a deadlock.
         */
        mCurrCollectionEvent = EventType::TERMINATED;
        mHandlerLooper->removeMessages(this);
        mHandlerLooper->wake();
    }
}

Result<void> WatchdogPerfService::processCollectionEvent(
        WatchdogPerfService::EventMetadata* metadata) {
    Mutex::Autolock lock(mMutex);
    /*
     * Messages sent to the looper are intrinsically racy such that a message from the previous
     * collection event may land in the looper after the current collection has already begun. Thus
     * verify the current collection event before starting the collection.
     */
    if (mCurrCollectionEvent != metadata->eventType) {
        ALOGW("Skipping %s event on collection event %s", toString(metadata->eventType),
              toString(mCurrCollectionEvent));
        return {};
    }
    if (DEBUG) {
        ALOGD("Processing %s collection event", toString(metadata->eventType));
    }
    if (metadata->interval < kMinEventInterval) {
        return Error()
                << "Collection interval of "
                << std::chrono::duration_cast<std::chrono::seconds>(metadata->interval).count()
                << " seconds for " << toString(metadata->eventType)
                << " collection cannot be less than "
                << std::chrono::duration_cast<std::chrono::seconds>(kMinEventInterval).count()
                << " seconds";
    }
    if (const auto result = collectLocked(metadata); !result.ok()) {
        return Error() << toString(metadata->eventType) << " collection failed: " << result.error();
    }
    metadata->lastUptime += metadata->interval.count();
    mHandlerLooper->sendMessageAtTime(metadata->lastUptime, this, metadata->eventType);
    return {};
}

Result<void> WatchdogPerfService::collectLocked(WatchdogPerfService::EventMetadata* metadata) {
    if (!mUidIoStats->enabled() && !mProcStat->enabled() && !mProcPidStat->enabled()) {
        return Error() << "No collectors enabled";
    }

    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    if (mUidIoStats->enabled()) {
        if (const auto result = mUidIoStats->collect(); !result.ok()) {
            return Error() << "Failed to collect per-uid I/O usage: " << result.error();
        }
    }

    if (mProcStat->enabled()) {
        if (const auto result = mProcStat->collect(); !result.ok()) {
            return Error() << "Failed to collect proc stats: " << result.error();
        }
    }

    if (mProcPidStat->enabled()) {
        if (const auto result = mProcPidStat->collect(); !result.ok()) {
            return Error() << "Failed to collect process stats: " << result.error();
        }
    }

    for (const auto& processor : mDataProcessors) {
        Result<void> result;
        switch (mCurrCollectionEvent) {
            case EventType::BOOT_TIME_COLLECTION:
                result = processor->onBoottimeCollection(now, mUidIoStats, mProcStat, mProcPidStat);
                break;
            case EventType::PERIODIC_COLLECTION:
                result = processor->onPeriodicCollection(now, mSystemState, mUidIoStats, mProcStat,
                                                         mProcPidStat);
                break;
            case EventType::CUSTOM_COLLECTION:
                result = processor->onCustomCollection(now, mSystemState, metadata->filterPackages,
                                                       mUidIoStats, mProcStat, mProcPidStat);
                break;
            default:
                result = Error() << "Invalid collection event " << toString(mCurrCollectionEvent);
        }
        if (!result.ok()) {
            return Error() << processor->name() << " failed on " << toString(mCurrCollectionEvent)
                           << " collection: " << result.error();
        }
    }

    return {};
}

Result<void> WatchdogPerfService::processMonitorEvent(
        WatchdogPerfService::EventMetadata* metadata) {
    if (metadata->eventType != static_cast<int>(EventType::PERIODIC_MONITOR)) {
        return Error() << "Invalid monitor event " << toString(metadata->eventType);
    }
    if (DEBUG) {
        ALOGD("Processing %s monitor event", toString(metadata->eventType));
    }
    if (metadata->interval < kMinEventInterval) {
        return Error()
                << "Monitor interval of "
                << std::chrono::duration_cast<std::chrono::seconds>(metadata->interval).count()
                << " seconds for " << toString(metadata->eventType) << " event cannot be less than "
                << std::chrono::duration_cast<std::chrono::seconds>(kMinEventInterval).count()
                << " seconds";
    }
    Mutex::Autolock lock(mMutex);
    if (!mProcDiskStats->enabled()) {
        return Error() << "Cannot access proc disk stats for monitoring";
    }
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (const auto result = mProcDiskStats->collect(); !result.ok()) {
        return Error() << "Failed to collect disk stats: " << result.error();
    }
    auto* currCollectionMetadata = currCollectionMetadataLocked();
    if (currCollectionMetadata == nullptr) {
        return Error() << "No metadata available for current collection event: "
                       << toString(mCurrCollectionEvent);
    }
    bool requestedCollection = false;
    const auto requestCollection = [&]() mutable {
        if (requestedCollection) {
            return;
        }
        const nsecs_t prevUptime =
                currCollectionMetadata->lastUptime - currCollectionMetadata->interval.count();
        nsecs_t uptime = mHandlerLooper->now();
        if (const auto delta = std::abs(uptime - prevUptime); delta < kMinEventInterval.count()) {
            return;
        }
        currCollectionMetadata->lastUptime = uptime;
        mHandlerLooper->removeMessages(this, currCollectionMetadata->eventType);
        mHandlerLooper->sendMessage(this, currCollectionMetadata->eventType);
        requestedCollection = true;
    };
    for (const auto& processor : mDataProcessors) {
        if (const auto result =
                    processor->onPeriodicMonitor(now, mProcDiskStats, requestCollection);
            !result.ok()) {
            return Error() << processor->name() << " failed on " << toString(metadata->eventType)
                           << ": " << result.error();
        }
    }
    metadata->lastUptime += metadata->interval.count();
    if (metadata->lastUptime == currCollectionMetadata->lastUptime) {
        /*
         * If the |PERIODIC_MONITOR| and  *_COLLECTION events overlap, skip the |PERIODIC_MONITOR|
         * event.
         */
        metadata->lastUptime += metadata->interval.count();
    }
    mHandlerLooper->sendMessageAtTime(metadata->lastUptime, this, metadata->eventType);
    return {};
}

WatchdogPerfService::EventMetadata* WatchdogPerfService::currCollectionMetadataLocked() {
    switch (mCurrCollectionEvent) {
        case EventType::BOOT_TIME_COLLECTION:
            return &mBoottimeCollection;
        case EventType::PERIODIC_COLLECTION:
            return &mPeriodicCollection;
        case EventType::CUSTOM_COLLECTION:
            return &mCustomCollection;
        default:
            return nullptr;
    }
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
