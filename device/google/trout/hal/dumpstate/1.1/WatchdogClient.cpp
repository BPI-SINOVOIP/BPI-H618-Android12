/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "trout.dumpstate@1.1-watchdog"

#include "WatchdogClient.h"

#include <android/binder_manager.h>

using aidl::android::automotive::watchdog::ICarWatchdog;
using aidl::android::automotive::watchdog::TimeoutLength;

namespace {

enum { WHAT_CHECK_ALIVE = 1 };

}  // namespace

namespace android::hardware::dumpstate::V1_1::implementation {

WatchdogClient::WatchdogClient(const sp<Looper>& handlerLooper, DumpstateDevice* ddh)
    : BaseWatchdogClient(handlerLooper), mDumpstateImpl(ddh) {}

bool WatchdogClient::isClientHealthy() const {
    return mDumpstateImpl->isHealthy();
}

}  // namespace android::hardware::dumpstate::V1_1::implementation
