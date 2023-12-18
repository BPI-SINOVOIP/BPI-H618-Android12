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

#ifndef CPP_WATCHDOG_SERVER_SRC_WATCHDOGBINDERMEDIATOR_H_
#define CPP_WATCHDOG_SERVER_SRC_WATCHDOGBINDERMEDIATOR_H_

#include "IoOveruseMonitor.h"
#include "WatchdogInternalHandler.h"
#include "WatchdogPerfService.h"
#include "WatchdogProcessService.h"
#include "WatchdogServiceHelper.h"

#include <android-base/result.h>
#include <android/automotive/watchdog/BnCarWatchdog.h>
#include <android/automotive/watchdog/BnResourceOveruseListener.h>
#include <android/automotive/watchdog/ResourceOveruseStats.h>
#include <android/automotive/watchdog/ResourceType.h>
#include <android/automotive/watchdog/StateType.h>
#include <binder/IBinder.h>
#include <binder/Status.h>
#include <gtest/gtest_prod.h>
#include <utils/Errors.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <utils/Vector.h>

#include <functional>

namespace android {
namespace automotive {
namespace watchdog {

class ServiceManager;

// Forward declaration for testing use only.
namespace internal {

class WatchdogBinderMediatorPeer;

}  // namespace internal

// WatchdogBinderMediator implements the public carwatchdog binder APIs such that it forwards
// the calls either to process ANR or performance services.
class WatchdogBinderMediator : public BnCarWatchdog {
public:
    WatchdogBinderMediator(const android::sp<WatchdogProcessService>& watchdogProcessService,
                           const android::sp<WatchdogPerfServiceInterface>& watchdogPerfService,
                           const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper,
                           const std::function<android::base::Result<void>(
                                   const char*, const android::sp<android::IBinder>&)>&
                                   addServiceHandler = nullptr);
    ~WatchdogBinderMediator() { terminate(); }

    // Implements ICarWatchdog.aidl APIs.
    status_t dump(int fd, const Vector<android::String16>& args) override;
    android::binder::Status registerClient(const android::sp<ICarWatchdogClient>& client,
                                           TimeoutLength timeout) override;
    android::binder::Status unregisterClient(
            const android::sp<ICarWatchdogClient>& client) override;
    android::binder::Status tellClientAlive(const android::sp<ICarWatchdogClient>& client,
                                            int32_t sessionId) override;
    android::binder::Status addResourceOveruseListener(
            const std::vector<ResourceType>& resourceTypes,
            const android::sp<IResourceOveruseListener>& listener);
    android::binder::Status removeResourceOveruseListener(
            const android::sp<IResourceOveruseListener>& listener);
    android::binder::Status getResourceOveruseStats(
            const std::vector<ResourceType>& resourceTypes,
            std::vector<ResourceOveruseStats>* resourceOveruseStats);

    // Deprecated APIs.
    android::binder::Status registerMediator(
            const android::sp<ICarWatchdogClient>& mediator) override;
    android::binder::Status unregisterMediator(
            const android::sp<ICarWatchdogClient>& mediator) override;
    android::binder::Status registerMonitor(
            const android::sp<ICarWatchdogMonitor>& monitor) override;
    android::binder::Status unregisterMonitor(
            const android::sp<ICarWatchdogMonitor>& monitor) override;
    android::binder::Status tellMediatorAlive(const android::sp<ICarWatchdogClient>& mediator,
                                              const std::vector<int32_t>& clientsNotResponding,
                                              int32_t sessionId) override;
    android::binder::Status tellDumpFinished(const android::sp<ICarWatchdogMonitor>& monitor,
                                             int32_t pid) override;
    android::binder::Status notifySystemStateChange(StateType type, int32_t arg1,
                                                    int32_t arg2) override;

protected:
    android::base::Result<void> init();

    void terminate() {
        mWatchdogProcessService.clear();
        mWatchdogPerfService.clear();
        mIoOveruseMonitor.clear();
        if (mWatchdogInternalHandler != nullptr) {
            mWatchdogInternalHandler->terminate();
            mWatchdogInternalHandler.clear();
        }
    }

private:
    status_t dumpServices(int fd, const Vector<String16>& args);
    status_t dumpHelpText(const int fd, const std::string& errorMsg);

    android::sp<WatchdogProcessService> mWatchdogProcessService;
    android::sp<WatchdogPerfServiceInterface> mWatchdogPerfService;
    android::sp<IIoOveruseMonitor> mIoOveruseMonitor;
    android::sp<WatchdogInternalHandler> mWatchdogInternalHandler;

    // Used by tests to stub the call to IServiceManager.
    std::function<android::base::Result<void>(const char*, const android::sp<android::IBinder>&)>
            mAddServiceHandler;

    friend class ServiceManager;

    // For unit tests.
    friend class internal::WatchdogBinderMediatorPeer;
    FRIEND_TEST(WatchdogBinderMediatorTest, TestInit);
    FRIEND_TEST(WatchdogBinderMediatorTest, TestErrorOnInitWithNullServiceInstances);
    FRIEND_TEST(WatchdogBinderMediatorTest, TestTerminate);
    FRIEND_TEST(WatchdogBinderMediatorTest, TestHandlesEmptyDumpArgs);
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  // CPP_WATCHDOG_SERVER_SRC_WATCHDOGBINDERMEDIATOR_H_
