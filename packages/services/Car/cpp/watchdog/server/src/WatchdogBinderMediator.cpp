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

#include "WatchdogBinderMediator.h"

#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <binder/IServiceManager.h>
#include <log/log.h>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::defaultServiceManager;
using ::android::IBinder;
using ::android::sp;
using ::android::String16;
using ::android::base::Error;
using ::android::base::Join;
using ::android::base::ParseUint;
using ::android::base::Result;
using ::android::base::Split;
using ::android::base::StringAppendF;
using ::android::base::StringPrintf;
using ::android::base::WriteStringToFd;
using ::android::binder::Status;

using AddServiceFunction =
        std::function<android::base::Result<void>(const char*,
                                                  const android::sp<android::IBinder>&)>;

namespace {

constexpr const char* kHelpFlag = "--help";
constexpr const char* kHelpShortFlag = "-h";
constexpr const char* kHelpText =
        "CarWatchdog daemon dumpsys help page:\n"
        "Format: dumpsys android.automotive.watchdog.ICarWatchdog/default [options]\n\n"
        "%s or %s: Displays this help text.\n"
        "When no options are specified, carwatchdog report is generated.\n";
constexpr const char* kCarWatchdogServerInterface =
        "android.automotive.watchdog.ICarWatchdog/default";
constexpr const char* kCarWatchdogInternalServerInterface = "carwatchdogd_system";
constexpr const char* kNullCarWatchdogClientError =
        "Must provide a non-null car watchdog client instance";

Status fromExceptionCode(const int32_t exceptionCode, const std::string& message) {
    ALOGW("%s", message.c_str());
    return Status::fromExceptionCode(exceptionCode, message.c_str());
}

Result<void> addToServiceManager(const char* serviceName, sp<IBinder> service) {
    status_t status = defaultServiceManager()->addService(String16(serviceName), service);
    if (status != OK) {
        return Error(status) << "Failed to add '" << serviceName << "' to ServiceManager";
    }
    return {};
}

}  // namespace

WatchdogBinderMediator::WatchdogBinderMediator(
        const android::sp<WatchdogProcessService>& watchdogProcessService,
        const android::sp<WatchdogPerfServiceInterface>& watchdogPerfService,
        const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper,
        const AddServiceFunction& addServiceHandler) :
      mWatchdogProcessService(watchdogProcessService),
      mWatchdogPerfService(watchdogPerfService),
      mAddServiceHandler(addServiceHandler) {
    if (mAddServiceHandler == nullptr) {
        mAddServiceHandler = &addToServiceManager;
    }
    if (watchdogServiceHelper != nullptr) {
        mIoOveruseMonitor = sp<IoOveruseMonitor>::make(watchdogServiceHelper);
        mWatchdogInternalHandler =
                sp<WatchdogInternalHandler>::make(this, watchdogServiceHelper,
                                                  mWatchdogProcessService, mWatchdogPerfService,
                                                  mIoOveruseMonitor);
    }
}

Result<void> WatchdogBinderMediator::init() {
    if (mWatchdogProcessService == nullptr || mWatchdogPerfService == nullptr ||
        mIoOveruseMonitor == nullptr || mWatchdogInternalHandler == nullptr) {
        std::string serviceList;
        if (mWatchdogProcessService == nullptr) {
            StringAppendF(&serviceList, "%s%s", (!serviceList.empty() ? ", " : ""),
                          "Watchdog process service");
        }
        if (mWatchdogPerfService == nullptr) {
            StringAppendF(&serviceList, "%s%s", (!serviceList.empty() ? ", " : ""),
                          "Watchdog performance service");
        }
        if (mIoOveruseMonitor == nullptr) {
            StringAppendF(&serviceList, "%s%s", (!serviceList.empty() ? ", " : ""),
                          "I/O overuse monitor service");
        }
        if (mWatchdogInternalHandler == nullptr) {
            StringAppendF(&serviceList, "%s%s", (!serviceList.empty() ? ", " : ""),
                          "Watchdog internal handler");
        }
        return Error(INVALID_OPERATION)
                << serviceList << " must be initialized with non-null instance";
    }
    if (const auto result = mAddServiceHandler(kCarWatchdogServerInterface, this); !result.ok()) {
        return result;
    }
    if (const auto result =
                mAddServiceHandler(kCarWatchdogInternalServerInterface, mWatchdogInternalHandler);
        !result.ok()) {
        return result;
    }
    return {};
}

status_t WatchdogBinderMediator::dump(int fd, const Vector<String16>& args) {
    int numArgs = args.size();
    if (numArgs == 0) {
        return dumpServices(fd, args);
    }
    if (numArgs == 1 && (args[0] == String16(kHelpFlag) || args[0] == String16(kHelpShortFlag))) {
        return dumpHelpText(fd, "");
    }
    if (args[0] == String16(kStartCustomCollectionFlag) ||
        args[0] == String16(kEndCustomCollectionFlag)) {
        if (auto result = mWatchdogPerfService->onCustomCollection(fd, args); !result.ok()) {
            std::string mode = args[0] == String16(kStartCustomCollectionFlag) ? "start" : "end";
            std::string errorMsg = StringPrintf("Failed to %s custom I/O perf collection: %s",
                                                mode.c_str(), result.error().message().c_str());
            if (result.error().code() == BAD_VALUE) {
                dumpHelpText(fd, errorMsg);
            } else {
                ALOGW("%s", errorMsg.c_str());
            }
            return result.error().code();
        }
        return OK;
    }
    if (numArgs == 2 && args[0] == String16(kResetResourceOveruseStatsFlag)) {
        std::string value = std::string(String8(args[1]));
        std::vector<std::string> packageNames = Split(value, ",");
        if (value.empty() || packageNames.empty()) {
            dumpHelpText(fd,
                         StringPrintf("Must provide valid package names: [%s]\n", value.c_str()));
            return BAD_VALUE;
        }
        if (auto result = mIoOveruseMonitor->resetIoOveruseStats(packageNames); !result.ok()) {
            ALOGW("Failed to reset stats for packages: [%s]", value.c_str());
            return FAILED_TRANSACTION;
        }
        return OK;
    }
    dumpHelpText(fd,
                 StringPrintf("Invalid carwatchdog dumpsys options: [%s]\n",
                              Join(args, " ").c_str()));
    return dumpServices(fd, args);
}

status_t WatchdogBinderMediator::dumpServices(int fd, const Vector<String16>& args) {
    if (auto result = mWatchdogProcessService->dump(fd, args); !result.ok()) {
        ALOGW("Failed to dump carwatchdog process service: %s", result.error().message().c_str());
        return result.error().code();
    }
    if (auto result = mWatchdogPerfService->onDump(fd); !result.ok()) {
        ALOGW("Failed to dump carwatchdog perf service: %s", result.error().message().c_str());
        return result.error().code();
    }
    return OK;
}

status_t WatchdogBinderMediator::dumpHelpText(const int fd, const std::string& errorMsg) {
    if (!errorMsg.empty()) {
        ALOGW("Error: %s", errorMsg.c_str());
        if (!WriteStringToFd(StringPrintf("Error: %s\n\n", errorMsg.c_str()), fd)) {
            ALOGW("Failed to write error message to fd");
            return FAILED_TRANSACTION;
        }
    }
    if (!WriteStringToFd(StringPrintf(kHelpText, kHelpFlag, kHelpShortFlag), fd) ||
        !mWatchdogPerfService->dumpHelpText(fd) || !mIoOveruseMonitor->dumpHelpText(fd)) {
        ALOGW("Failed to write help text to fd");
        return FAILED_TRANSACTION;
    }
    return OK;
}

Status WatchdogBinderMediator::registerClient(const sp<ICarWatchdogClient>& client,
                                              TimeoutLength timeout) {
    if (client == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogClientError);
    }
    return mWatchdogProcessService->registerClient(client, timeout);
}

Status WatchdogBinderMediator::unregisterClient(const sp<ICarWatchdogClient>& client) {
    if (client == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogClientError);
    }
    return mWatchdogProcessService->unregisterClient(client);
}

Status WatchdogBinderMediator::tellClientAlive(const sp<ICarWatchdogClient>& client,
                                               int32_t sessionId) {
    if (client == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogClientError);
    }
    return mWatchdogProcessService->tellClientAlive(client, sessionId);
}

Status WatchdogBinderMediator::addResourceOveruseListener(
        const std::vector<ResourceType>& resourceTypes,
        const sp<IResourceOveruseListener>& listener) {
    if (listener == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Must provide a non-null resource overuse listener");
    }
    if (resourceTypes.size() != 1 || resourceTypes[0] != ResourceType::IO) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Must provide exactly one I/O resource type");
    }
    /*
     * When more resource types are added, implement a new module to manage listeners for all
     * resources.
     */
    if (const auto result = mIoOveruseMonitor->addIoOveruseListener(listener); !result.ok()) {
        return fromExceptionCode(result.error().code(),
                                 StringPrintf("Failed to register resource overuse listener: %s ",
                                              result.error().message().c_str()));
    }
    return Status::ok();
}

Status WatchdogBinderMediator::removeResourceOveruseListener(
        const sp<IResourceOveruseListener>& listener) {
    if (listener == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Must provide a non-null resource overuse listener");
    }
    if (const auto result = mIoOveruseMonitor->removeIoOveruseListener(listener); !result.ok()) {
        return fromExceptionCode(result.error().code(),
                                 StringPrintf("Failed to unregister resource overuse listener: %s",
                                              result.error().message().c_str()));
    }
    return Status::ok();
}

Status WatchdogBinderMediator::getResourceOveruseStats(
        const std::vector<ResourceType>& resourceTypes,
        std::vector<ResourceOveruseStats>* resourceOveruseStats) {
    if (resourceOveruseStats == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Must provide a non-null resource overuse stats parcelable");
    }
    if (resourceTypes.size() != 1 || resourceTypes[0] != ResourceType::IO) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Must provide exactly one I/O resource type");
    }
    IoOveruseStats ioOveruseStats;
    if (const auto result = mIoOveruseMonitor->getIoOveruseStats(&ioOveruseStats); !result.ok()) {
        return fromExceptionCode(result.error().code(),
                                 StringPrintf("Failed to get resource overuse stats: %s",
                                              result.error().message().c_str()));
    }
    ResourceOveruseStats stats;
    stats.set<ResourceOveruseStats::ioOveruseStats>(std::move(ioOveruseStats));
    resourceOveruseStats->emplace_back(std::move(stats));
    return Status::ok();
}

Status WatchdogBinderMediator::registerMediator(const sp<ICarWatchdogClient>& /*mediator*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION,
                             "Deprecated method registerMediator");
}

Status WatchdogBinderMediator::unregisterMediator(const sp<ICarWatchdogClient>& /*mediator*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION,
                             "Deprecated method unregisterMediator");
}

Status WatchdogBinderMediator::registerMonitor(const sp<ICarWatchdogMonitor>& /*monitor*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION, "Deprecated method registerMonitor");
}

Status WatchdogBinderMediator::unregisterMonitor(const sp<ICarWatchdogMonitor>& /*monitor*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION,
                             "Deprecated method unregisterMonitor");
}

Status WatchdogBinderMediator::tellMediatorAlive(
        const sp<ICarWatchdogClient>& /*mediator*/,
        const std::vector<int32_t>& /*clientsNotResponding*/, int32_t /*sessionId*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION,
                             "Deprecated method tellMediatorAlive");
}

Status WatchdogBinderMediator::tellDumpFinished(const sp<ICarWatchdogMonitor>& /*monitor*/,
                                                int32_t /*pid*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION,
                             "Deprecated method tellDumpFinished");
}

Status WatchdogBinderMediator::notifySystemStateChange(StateType /*type*/, int32_t /*arg1*/,
                                                       int32_t /*arg2*/) {
    return fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION,
                             "Deprecated method notifySystemStateChange");
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
