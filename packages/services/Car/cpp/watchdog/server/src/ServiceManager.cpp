/*
 * Copyright (c) 2020 The Android Open Source Project
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

#include "ServiceManager.h"

#include "IoPerfCollection.h"
#include "PackageInfoResolver.h"

namespace android {
namespace automotive {
namespace watchdog {

using ::android::sp;
using ::android::base::Error;
using ::android::base::Result;

sp<WatchdogProcessService> ServiceManager::sWatchdogProcessService = nullptr;
sp<WatchdogPerfServiceInterface> ServiceManager::sWatchdogPerfService = nullptr;
sp<WatchdogBinderMediator> ServiceManager::sWatchdogBinderMediator = nullptr;
sp<IWatchdogServiceHelper> ServiceManager::sWatchdogServiceHelper = nullptr;

Result<void> ServiceManager::startServices(const sp<Looper>& looper) {
    if (sWatchdogBinderMediator != nullptr || sWatchdogServiceHelper != nullptr ||
        sWatchdogProcessService != nullptr || sWatchdogPerfService != nullptr) {
        return Error(INVALID_OPERATION) << "Cannot start services more than once";
    }
    /*
     * PackageInfoResolver must be initialized first time on the main thread before starting any
     * other thread as the getInstance method isn't thread safe. Thus initialize PackageInfoResolver
     * by calling the getInstance method before starting other service as they may access
     * PackageInfoResolver's instance during initialization.
     */
    sp<IPackageInfoResolver> packageInfoResolver = PackageInfoResolver::getInstance();
    if (const auto result = startProcessAnrMonitor(looper); !result.ok()) {
        return result;
    }
    if (const auto result = startPerfService(); !result.ok()) {
        return result;
    }
    sWatchdogServiceHelper = sp<WatchdogServiceHelper>::make();
    if (const auto result = sWatchdogServiceHelper->init(sWatchdogProcessService); !result.ok()) {
        return Error() << "Failed to initialize watchdog service helper: " << result.error();
    }
    if (const auto result = packageInfoResolver->initWatchdogServiceHelper(sWatchdogServiceHelper);
        !result.ok()) {
        return Error() << "Failed to initialize package name resolver: " << result.error();
    }
    return {};
}

void ServiceManager::terminateServices() {
    if (sWatchdogProcessService != nullptr) {
        sWatchdogProcessService->terminate();
        sWatchdogProcessService.clear();
    }
    if (sWatchdogPerfService != nullptr) {
        sWatchdogPerfService->terminate();
        sWatchdogPerfService.clear();
    }
    if (sWatchdogBinderMediator != nullptr) {
        sWatchdogBinderMediator->terminate();
        sWatchdogBinderMediator.clear();
    }
    if (sWatchdogServiceHelper != nullptr) {
        sWatchdogServiceHelper->terminate();
        sWatchdogServiceHelper.clear();
    }
    PackageInfoResolver::terminate();
}

Result<void> ServiceManager::startProcessAnrMonitor(const sp<Looper>& looper) {
    sp<WatchdogProcessService> service = sp<WatchdogProcessService>::make(looper);
    if (const auto result = service->start(); !result.ok()) {
        return Error(result.error().code())
                << "Failed to start watchdog process monitoring: " << result.error();
    }
    sWatchdogProcessService = service;
    return {};
}

Result<void> ServiceManager::startPerfService() {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();
    if (const auto result = service->registerDataProcessor(sp<IoPerfCollection>::make());
        !result.ok()) {
        return Error() << "Failed to register I/O perf collection: " << result.error();
    }
    if (const auto result = service->start(); !result.ok()) {
        return Error(result.error().code())
                << "Failed to start watchdog performance service: " << result.error();
    }
    sWatchdogPerfService = service;
    return {};
}

Result<void> ServiceManager::startBinderMediator() {
    sWatchdogBinderMediator =
            sp<WatchdogBinderMediator>::make(sWatchdogProcessService, sWatchdogPerfService,
                                             sWatchdogServiceHelper);
    if (const auto result = sWatchdogBinderMediator->init(); !result.ok()) {
        return Error(result.error().code())
                << "Failed to initialize watchdog binder mediator: " << result.error();
    }
    return {};
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
