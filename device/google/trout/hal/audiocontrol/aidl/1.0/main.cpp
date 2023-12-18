/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "AudioControl.h"
#include "vsockinfo.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::automotive::audiocontrol::AudioControl;
using android::hardware::automotive::utils::VsockConnectionInfo;

int main() {
    const auto si = VsockConnectionInfo::fromRoPropertyStore(
            {
                    "ro.boot.vendor.audiocontrol.server.cid",
                    "ro.vendor.audiocontrol.server.cid",
            },
            {
                    "ro.boot.vendor.audiocontrol.server.port",
                    "ro.vendor.audiocontrol.server.port",
            });

    if (!si) {
        LOG(ERROR) << "failed to get server connection cid/port; audio control server disabled.";
    } else {
        LOG(INFO) << "Creating audio control server at " << si->str();
    }

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    // Create an instance of our service class
    std::shared_ptr<AudioControl> audioControl = ndk::SharedRefBase::make<AudioControl>(si ? si->str() : "");

    const std::string instance = std::string() + AudioControl::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(audioControl->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
