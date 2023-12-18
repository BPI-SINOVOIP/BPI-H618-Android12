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

#include "WatchdogInternalHandler.h"

#include "WatchdogBinderMediator.h"

#include <android/automotive/watchdog/internal/BootPhase.h>
#include <android/automotive/watchdog/internal/GarageMode.h>
#include <android/automotive/watchdog/internal/PowerCycle.h>
#include <android/automotive/watchdog/internal/UserState.h>
#include <binder/IPCThreadState.h>
#include <private/android_filesystem_config.h>

namespace android {
namespace automotive {
namespace watchdog {

namespace aawi = ::android::automotive::watchdog::internal;

using aawi::ComponentType;
using aawi::GarageMode;
using aawi::ICarWatchdogServiceForSystem;
using aawi::PackageResourceOveruseAction;
using aawi::PowerCycle;
using aawi::ResourceOveruseConfiguration;
using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

namespace {

constexpr const char* kNullCarWatchdogServiceError =
        "Must provide a non-null car watchdog service instance";
constexpr const char* kNullCarWatchdogMonitorError =
        "Must provide a non-null car watchdog monitor instance";

Status checkSystemUser() {
    if (IPCThreadState::self()->getCallingUid() != AID_SYSTEM) {
        return Status::fromExceptionCode(Status::EX_SECURITY,
                                         "Calling process does not have proper privilege");
    }
    return Status::ok();
}

Status fromExceptionCode(int32_t exceptionCode, std::string message) {
    ALOGW("%s", message.c_str());
    return Status::fromExceptionCode(exceptionCode, message.c_str());
}

}  // namespace

status_t WatchdogInternalHandler::dump(int fd, const Vector<String16>& args) {
    return mBinderMediator->dump(fd, args);
}

void WatchdogInternalHandler::checkAndRegisterIoOveruseMonitor() {
    if (mIoOveruseMonitor->isInitialized()) {
        return;
    }
    if (const auto result = mWatchdogPerfService->registerDataProcessor(mIoOveruseMonitor);
        !result.ok()) {
        ALOGE("Failed to register I/O overuse monitor to watchdog performance service: %s",
              result.error().message().c_str());
    }
    return;
}

Status WatchdogInternalHandler::registerCarWatchdogService(
        const sp<ICarWatchdogServiceForSystem>& service) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (service == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogServiceError);
    }
    /*
     * I/O overuse monitor reads from system, vendor, and data partitions during initialization.
     * When CarService is running these partitions are available to read, thus register the I/O
     * overuse monitor on processing the request to register CarService.
     */
    checkAndRegisterIoOveruseMonitor();
    return mWatchdogServiceHelper->registerService(service);
}

Status WatchdogInternalHandler::unregisterCarWatchdogService(
        const sp<ICarWatchdogServiceForSystem>& service) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (service == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogServiceError);
    }
    return mWatchdogServiceHelper->unregisterService(service);
}

Status WatchdogInternalHandler::registerMonitor(const sp<aawi::ICarWatchdogMonitor>& monitor) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (monitor == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogMonitorError);
    }
    return mWatchdogProcessService->registerMonitor(monitor);
}

Status WatchdogInternalHandler::unregisterMonitor(const sp<aawi::ICarWatchdogMonitor>& monitor) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (monitor == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogMonitorError);
    }
    return mWatchdogProcessService->unregisterMonitor(monitor);
}

Status WatchdogInternalHandler::tellCarWatchdogServiceAlive(
        const android::sp<ICarWatchdogServiceForSystem>& service,
        const std::vector<int32_t>& clientsNotResponding, int32_t sessionId) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (service == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogServiceError);
    }
    return mWatchdogProcessService->tellCarWatchdogServiceAlive(service, clientsNotResponding,
                                                                sessionId);
}
Status WatchdogInternalHandler::tellDumpFinished(
        const android::sp<aawi::ICarWatchdogMonitor>& monitor, int32_t pid) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (monitor == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, kNullCarWatchdogMonitorError);
    }
    return mWatchdogProcessService->tellDumpFinished(monitor, pid);
}

Status WatchdogInternalHandler::notifySystemStateChange(aawi::StateType type, int32_t arg1,
                                                        int32_t arg2) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    switch (type) {
        case aawi::StateType::POWER_CYCLE: {
            PowerCycle powerCycle = static_cast<PowerCycle>(static_cast<uint32_t>(arg1));
            if (powerCycle >= PowerCycle::NUM_POWER_CYLES) {
                return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                         StringPrintf("Invalid power cycle %d", powerCycle));
            }
            return handlePowerCycleChange(powerCycle);
        }
        case aawi::StateType::GARAGE_MODE: {
            GarageMode garageMode = static_cast<GarageMode>(static_cast<uint32_t>(arg1));
            mWatchdogPerfService->setSystemState(garageMode == GarageMode::GARAGE_MODE_OFF
                                                         ? SystemState::NORMAL_MODE
                                                         : SystemState::GARAGE_MODE);
            return Status::ok();
        }
        case aawi::StateType::USER_STATE: {
            userid_t userId = static_cast<userid_t>(arg1);
            aawi::UserState userState = static_cast<aawi::UserState>(static_cast<uint32_t>(arg2));
            if (userState >= aawi::UserState::NUM_USER_STATES) {
                return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                         StringPrintf("Invalid user state %d", userState));
            }
            return mWatchdogProcessService->notifyUserStateChange(userId, userState);
        }
        case aawi::StateType::BOOT_PHASE: {
            aawi::BootPhase phase = static_cast<aawi::BootPhase>(static_cast<uint32_t>(arg1));
            if (phase >= aawi::BootPhase::BOOT_COMPLETED) {
                if (const auto result = mWatchdogPerfService->onBootFinished(); !result.ok()) {
                    return fromExceptionCode(result.error().code(), result.error().message());
                }
            }
            return Status::ok();
        }
    }
    return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                             StringPrintf("Invalid state change type %d", type));
}

Status WatchdogInternalHandler::handlePowerCycleChange(PowerCycle powerCycle) {
    switch (powerCycle) {
        case PowerCycle::POWER_CYCLE_SHUTDOWN_PREPARE:
            ALOGI("Received SHUTDOWN_PREPARE power cycle");
            mWatchdogProcessService->setEnabled(/*isEnabled=*/false);
            // TODO(b/189508862): Upload resource overuse stats on shutdown prepare.
            break;
        case PowerCycle::POWER_CYCLE_SHUTDOWN_ENTER:
            ALOGI("Received SHUTDOWN_ENTER power cycle");
            mWatchdogProcessService->setEnabled(/*isEnabled=*/false);
            break;
        case PowerCycle::POWER_CYCLE_RESUME:
            ALOGI("Received RESUME power cycle");
            mWatchdogProcessService->setEnabled(/*isEnabled=*/true);
            break;
        default:
            ALOGW("Unsupported power cycle: %d", powerCycle);
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                             "Unsupported power cycle");
    }
    return Status::ok();
}

Status WatchdogInternalHandler::updateResourceOveruseConfigurations(
        const std::vector<ResourceOveruseConfiguration>& configs) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    // Maybe retry registring I/O overuse monitor if failed to initialize previously.
    checkAndRegisterIoOveruseMonitor();
    if (const auto result = mIoOveruseMonitor->updateResourceOveruseConfigurations(configs);
        !result.ok()) {
        return fromExceptionCode(result.error().code(), result.error().message());
    }
    return Status::ok();
}

Status WatchdogInternalHandler::getResourceOveruseConfigurations(
        std::vector<ResourceOveruseConfiguration>* configs) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    // Maybe retry registring I/O overuse monitor if failed to initialize previously.
    checkAndRegisterIoOveruseMonitor();
    if (const auto result = mIoOveruseMonitor->getResourceOveruseConfigurations(configs);
        !result.ok()) {
        return fromExceptionCode(result.error().code(), result.error().message());
    }
    return Status::ok();
}

Status WatchdogInternalHandler::actionTakenOnResourceOveruse(
        const std::vector<PackageResourceOveruseAction>& actions) {
    Status status = checkSystemUser();
    if (!status.isOk()) {
        return status;
    }
    if (const auto result = mIoOveruseMonitor->actionTakenOnIoOveruse(actions); !result.ok()) {
        return fromExceptionCode(result.error().code(), result.error().message());
    }
    return Status::ok();
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
