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

#ifndef CPP_WATCHDOG_SERVER_SRC_WATCHDOGINTERNALHANDLER_H_
#define CPP_WATCHDOG_SERVER_SRC_WATCHDOGINTERNALHANDLER_H_

#include "IoOveruseMonitor.h"
#include "WatchdogPerfService.h"
#include "WatchdogProcessService.h"
#include "WatchdogServiceHelper.h"

#include <android/automotive/watchdog/internal/BnCarWatchdog.h>
#include <android/automotive/watchdog/internal/ComponentType.h>
#include <android/automotive/watchdog/internal/ICarWatchdogMonitor.h>
#include <android/automotive/watchdog/internal/ICarWatchdogServiceForSystem.h>
#include <android/automotive/watchdog/internal/PackageResourceOveruseAction.h>
#include <android/automotive/watchdog/internal/ResourceOveruseConfiguration.h>
#include <android/automotive/watchdog/internal/StateType.h>
#include <binder/Status.h>
#include <gtest/gtest_prod.h>
#include <utils/Errors.h>
#include <utils/String16.h>
#include <utils/Vector.h>

namespace android {
namespace automotive {
namespace watchdog {

class WatchdogBinderMediator;

class WatchdogInternalHandler : public android::automotive::watchdog::internal::BnCarWatchdog {
public:
    explicit WatchdogInternalHandler(
            const android::sp<WatchdogBinderMediator>& binderMediator,
            const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper,
            const android::sp<WatchdogProcessService>& watchdogProcessService,
            const android::sp<WatchdogPerfServiceInterface>& watchdogPerfService,
            const android::sp<IIoOveruseMonitor>& ioOveruseMonitor) :
          mBinderMediator(binderMediator),
          mWatchdogServiceHelper(watchdogServiceHelper),
          mWatchdogProcessService(watchdogProcessService),
          mWatchdogPerfService(watchdogPerfService),
          mIoOveruseMonitor(ioOveruseMonitor) {}
    ~WatchdogInternalHandler() { terminate(); }

    status_t dump(int fd, const Vector<android::String16>& args) override;
    android::binder::Status registerCarWatchdogService(
            const android::sp<
                    android::automotive::watchdog::internal::ICarWatchdogServiceForSystem>& service)
            override;
    android::binder::Status unregisterCarWatchdogService(
            const android::sp<
                    android::automotive::watchdog::internal::ICarWatchdogServiceForSystem>& service)
            override;
    android::binder::Status registerMonitor(
            const android::sp<android::automotive::watchdog::internal::ICarWatchdogMonitor>&
                    monitor) override;
    android::binder::Status unregisterMonitor(
            const android::sp<android::automotive::watchdog::internal::ICarWatchdogMonitor>&
                    monitor) override;
    android::binder::Status tellCarWatchdogServiceAlive(
            const android::sp<
                    android::automotive::watchdog::internal::ICarWatchdogServiceForSystem>& service,
            const std::vector<int32_t>& clientsNotResponding, int32_t sessionId) override;
    android::binder::Status tellDumpFinished(
            const android::sp<android::automotive::watchdog::internal::ICarWatchdogMonitor>&
                    monitor,
            int32_t pid) override;
    android::binder::Status notifySystemStateChange(
            android::automotive::watchdog::internal::StateType type, int32_t arg1,
            int32_t arg2) override;
    android::binder::Status updateResourceOveruseConfigurations(
            const std::vector<
                    android::automotive::watchdog::internal::ResourceOveruseConfiguration>& configs)
            override;
    android::binder::Status getResourceOveruseConfigurations(
            std::vector<android::automotive::watchdog::internal::ResourceOveruseConfiguration>*
                    configs) override;
    android::binder::Status actionTakenOnResourceOveruse(
            const std::vector<
                    android::automotive::watchdog::internal::PackageResourceOveruseAction>&
                    actions);

protected:
    void terminate() {
        mBinderMediator.clear();
        mWatchdogServiceHelper.clear();
        mWatchdogProcessService.clear();
        mWatchdogPerfService.clear();
        mIoOveruseMonitor.clear();
    }

private:
    void checkAndRegisterIoOveruseMonitor();

    android::binder::Status handlePowerCycleChange(
            android::automotive::watchdog::internal::PowerCycle powerCycle);

    android::sp<WatchdogBinderMediator> mBinderMediator;
    android::sp<IWatchdogServiceHelper> mWatchdogServiceHelper;
    android::sp<WatchdogProcessService> mWatchdogProcessService;
    android::sp<WatchdogPerfServiceInterface> mWatchdogPerfService;
    android::sp<IIoOveruseMonitor> mIoOveruseMonitor;

    friend class WatchdogBinderMediator;

    FRIEND_TEST(WatchdogInternalHandlerTest, TestTerminate);
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  // CPP_WATCHDOG_SERVER_SRC_WATCHDOGINTERNALHANDLER_H_
