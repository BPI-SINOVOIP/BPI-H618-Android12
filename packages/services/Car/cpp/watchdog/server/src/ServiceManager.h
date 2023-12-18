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

#ifndef CPP_WATCHDOG_SERVER_SRC_SERVICEMANAGER_H_
#define CPP_WATCHDOG_SERVER_SRC_SERVICEMANAGER_H_

#include "IoOveruseMonitor.h"
#include "WatchdogBinderMediator.h"
#include "WatchdogPerfService.h"
#include "WatchdogProcessService.h"
#include "WatchdogServiceHelper.h"

#include <android-base/result.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

namespace android {
namespace automotive {
namespace watchdog {

class ServiceManager {
public:
    static android::base::Result<void> startServices(const android::sp<Looper>& looper);
    static android::base::Result<void> startBinderMediator();
    static void terminateServices();

private:
    static android::base::Result<void> startProcessAnrMonitor(const android::sp<Looper>& looper);
    static android::base::Result<void> startPerfService();

    static android::sp<WatchdogProcessService> sWatchdogProcessService;
    static android::sp<WatchdogPerfServiceInterface> sWatchdogPerfService;
    static android::sp<WatchdogBinderMediator> sWatchdogBinderMediator;
    static android::sp<IWatchdogServiceHelper> sWatchdogServiceHelper;
};

}  // namespace watchdog
}  // namespace automotive
}  // namespace android

#endif  // CPP_WATCHDOG_SERVER_SRC_SERVICEMANAGER_H_
