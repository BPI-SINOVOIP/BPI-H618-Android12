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

#include "WatchdogServiceHelper.h"

#include "ServiceManager.h"

#include <android/automotive/watchdog/internal/BnCarWatchdogServiceForSystem.h>

namespace android {
namespace automotive {
namespace watchdog {

namespace aawi = ::android::automotive::watchdog::internal;

using aawi::BnCarWatchdogServiceForSystem;
using aawi::ICarWatchdogServiceForSystem;
using aawi::PackageInfo;
using aawi::PackageIoOveruseStats;
using ::android::IBinder;
using ::android::sp;
using ::android::wp;
using ::android::base::Error;
using ::android::base::Result;
using ::android::binder::Status;

namespace {

Status fromExceptionCode(int32_t exceptionCode, std::string message) {
    ALOGW("%s.", message.c_str());
    return Status::fromExceptionCode(exceptionCode, message.c_str());
}

}  // namespace

Result<void> WatchdogServiceHelper::init(const sp<WatchdogProcessService>& watchdogProcessService) {
    if (watchdogProcessService == nullptr) {
        return Error() << "Must provide a non-null watchdog process service instance";
    }
    mWatchdogProcessService = watchdogProcessService;
    return mWatchdogProcessService->registerWatchdogServiceHelper(this);
}

WatchdogServiceHelper::~WatchdogServiceHelper() {
    terminate();
}

Status WatchdogServiceHelper::registerService(
        const android::sp<ICarWatchdogServiceForSystem>& service) {
    std::unique_lock writeLock(mRWMutex);
    if (mWatchdogProcessService == nullptr) {
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE,
                                         "Must initialize watchdog service helper before "
                                         "registering car watchdog service");
    }
    sp<IBinder> curBinder = BnCarWatchdogServiceForSystem::asBinder(mService);
    sp<IBinder> newBinder = BnCarWatchdogServiceForSystem::asBinder(service);
    if (mService != nullptr && curBinder == newBinder) {
        return Status::ok();
    }
    if (status_t ret = newBinder->linkToDeath(this); ret != OK) {
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE,
                                         "Failed to register car watchdog service as it is dead");
    }
    unregisterServiceLocked();
    if (Status status = mWatchdogProcessService->registerCarWatchdogService(newBinder);
        !status.isOk()) {
        newBinder->unlinkToDeath(this);
        return status;
    }
    mService = service;
    return Status::ok();
}

Status WatchdogServiceHelper::unregisterService(const sp<ICarWatchdogServiceForSystem>& service) {
    std::unique_lock writeLock(mRWMutex);
    if (sp<IBinder> binder = BnCarWatchdogServiceForSystem::asBinder(service);
        binder != BnCarWatchdogServiceForSystem::asBinder(mService)) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Failed to unregister car watchdog service as it is not "
                                 "registered");
    }
    unregisterServiceLocked();
    return Status::ok();
}

void WatchdogServiceHelper::binderDied(const wp<android::IBinder>& who) {
    std::unique_lock writeLock(mRWMutex);
    sp<IBinder> curBinder = BnCarWatchdogServiceForSystem::asBinder(mService);
    if (IBinder* diedBinder = who.unsafe_get(); curBinder == nullptr || diedBinder != curBinder) {
        return;
    }
    ALOGW("Car watchdog service had died.");
    mService.clear();
    mWatchdogProcessService->unregisterCarWatchdogService(curBinder);
}

void WatchdogServiceHelper::terminate() {
    std::unique_lock writeLock(mRWMutex);
    unregisterServiceLocked();
    mWatchdogProcessService.clear();
}

Status WatchdogServiceHelper::checkIfAlive(const wp<IBinder>& who, int32_t sessionId,
                                           TimeoutLength timeout) const {
    sp<ICarWatchdogServiceForSystem> service;
    if (std::shared_lock readLock(mRWMutex); mService == nullptr ||
        who.unsafe_get() != BnCarWatchdogServiceForSystem::asBinder(mService)) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Dropping checkIfAlive request as the given car watchdog "
                                 "service "
                                 "binder isn't registered");
    } else {
        service = mService;
    }
    return service->checkIfAlive(sessionId, static_cast<aawi::TimeoutLength>(timeout));
}

Status WatchdogServiceHelper::prepareProcessTermination(const wp<IBinder>& who) {
    sp<ICarWatchdogServiceForSystem> service;
    if (std::shared_lock readLock(mRWMutex); mService == nullptr ||
        who.unsafe_get() != BnCarWatchdogServiceForSystem::asBinder(mService)) {
        return fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                 "Dropping prepareProcessTermination request as the given "
                                 "car watchdog service binder isn't registered");
    } else {
        service = mService;
    }
    Status status = service->prepareProcessTermination();
    if (status.isOk()) {
        std::unique_lock writeLock(mRWMutex);
        /*
         * prepareTermination callback is called when CarWatchdogService isn't responding, which
         * indicates the CarWatchdogService is stuck, terminating, or restarting.
         *
         * When CarWatchdogService is terminating, it will issue an unregisterService call.
         * If the unregisterService is executed after the previous |readLock| is released and the
         * before current |writeLock| is acquired, the |mService| will be updated to null. Then it
         * won't match |service|.
         *
         * When CarWatchdogService is restarting, it will issue an registerService call. When the
         * registerService is executed between after the previous |readLock| is released and before
         * the current |writeLock| is acquired, the |mService| will be overwritten. This will lead
         * to unregistering the new CarWatchdogService.
         *
         * To avoid this race condition, check mService before proceeding with unregistering the
         * CarWatchdogService.
         */
        if (mService == service) {
            unregisterServiceLocked();
        }
    }
    return status;
}

void WatchdogServiceHelper::unregisterServiceLocked() {
    if (mService == nullptr) return;
    sp<IBinder> binder = BnCarWatchdogServiceForSystem::asBinder(mService);
    binder->unlinkToDeath(this);
    mService.clear();
    mWatchdogProcessService->unregisterCarWatchdogService(binder);
}

Status WatchdogServiceHelper::getPackageInfosForUids(
        const std::vector<int32_t>& uids, const std::vector<std::string>& vendorPackagePrefixes,
        std::vector<PackageInfo>* packageInfos) {
    sp<ICarWatchdogServiceForSystem> service;
    if (std::shared_lock readLock(mRWMutex); mService == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_STATE, "Watchdog service is not initialized");
    } else {
        service = mService;
    }
    /*
     * The expected number of vendor package prefixes is in the order of 10s. Thus the overhead of
     * forwarding these in each get call is very low.
     */
    return service->getPackageInfosForUids(uids, vendorPackagePrefixes, packageInfos);
}

Status WatchdogServiceHelper::latestIoOveruseStats(
        const std::vector<PackageIoOveruseStats>& packageIoOveruseStats) {
    sp<ICarWatchdogServiceForSystem> service;
    if (std::shared_lock readLock(mRWMutex); mService == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_STATE, "Watchdog service is not initialized");
    } else {
        service = mService;
    }
    return service->latestIoOveruseStats(packageIoOveruseStats);
}

Status WatchdogServiceHelper::resetResourceOveruseStats(
        const std::vector<std::string>& packageNames) {
    sp<ICarWatchdogServiceForSystem> service;
    if (std::shared_lock readLock(mRWMutex); mService == nullptr) {
        return fromExceptionCode(Status::EX_ILLEGAL_STATE, "Watchdog service is not initialized");
    } else {
        service = mService;
    }
    return service->resetResourceOveruseStats(packageNames);
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
