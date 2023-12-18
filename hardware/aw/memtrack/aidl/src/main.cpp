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

#include "Memtrack.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <log/log.h>

using aidl::android::hardware::memtrack::Memtrack;

int main() {
    int ret = 0;
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Memtrack> memtrack = ndk::SharedRefBase::make<Memtrack>();
    if (ret < 0) {
        ALOGE("hal Memtrack init failed\n");
        return -1;
    }

    ret = memtrack->init();
    if (ret < 0) {
        ALOGE("hal Memtrack init failed!\n");
    }

    ALOGD("hal Memtrack init succeed!\n\n");
    const std::string instance = std::string() + Memtrack::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(memtrack->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    memtrack->exit();

    return EXIT_FAILURE;  // Unreachable
}
