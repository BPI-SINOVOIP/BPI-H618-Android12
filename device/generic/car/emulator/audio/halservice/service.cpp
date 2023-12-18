/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "audiohalservice"

#include "AudioControl.h"
#include "PowerPolicyClient.h"

#include <string>
#include <vector>

#include <log/log.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>

#include <android/hardware/audio/6.0/IDevicesFactory.h>
#include <android/hardware/audio/effect/6.0/IEffectsFactory.h>

using namespace android::hardware;
using android::OK;
using aidl::android::hardware::automotive::audiocontrol::AudioControl;
using aidl::android::hardware::automotive::audiocontrol::PowerPolicyClient;

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;

using android::hardware::audio::effect::V6_0::IEffectsFactory;
using android::hardware::audio::V6_0::IDevicesFactory;
using android::hardware::registerPassthroughServiceImplementation;

int main(int /* argc */, char* /* argv */ []) {
    // Setup HIDL Audio HAL
    configureRpcThreadpool(16, false /*callerWillJoin*/);
    android::status_t status;
    status = registerPassthroughServiceImplementation<IDevicesFactory>();
    LOG_ALWAYS_FATAL_IF(status != OK, "Error while registering audio service: %d", status);
    status = registerPassthroughServiceImplementation<IEffectsFactory>();
    LOG_ALWAYS_FATAL_IF(status != OK, "Error while registering audio effects service: %d", status);

    // Setup AudioControl HAL
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<AudioControl> audioControl = ::ndk::SharedRefBase::make<AudioControl>();

    const std::string instance = std::string() + AudioControl::descriptor + "/default";
    binder_status_t aidlStatus =
            AServiceManager_addService(audioControl->asBinder().get(), instance.c_str());
    CHECK(aidlStatus == STATUS_OK);

    PowerPolicyClient powerPolicyClient = PowerPolicyClient(audioControl);
    powerPolicyClient.init();

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
