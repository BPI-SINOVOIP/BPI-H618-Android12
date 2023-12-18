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

#define LOG_TAG "cartelemetryd_sample"

#include <aidl/android/frameworks/automotive/telemetry/ICarTelemetry.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/binder_manager.h>
#include <utils/SystemClock.h>

using ::aidl::android::frameworks::automotive::telemetry::CarData;
using ::aidl::android::frameworks::automotive::telemetry::ICarTelemetry;
using ::android::base::StringPrintf;

int main(int argc, char* argv[]) {
    const auto started_at_millis = android::elapsedRealtime();

    // The name of the service is described in
    // https://source.android.com/devices/architecture/aidl/aidl-hals#instance-names
    const std::string instance = StringPrintf("%s/default", ICarTelemetry::descriptor);
    LOG(INFO) << "Obtaining: " << instance;
    std::shared_ptr<ICarTelemetry> service = ICarTelemetry::fromBinder(
            ndk::SpAIBinder(AServiceManager_getService(instance.c_str())));
    if (!service) {
        LOG(ERROR) << "ICarTelemetry service not found, may be still initializing?";
        return 1;
    }

    LOG(INFO) << "Building a CarData message, delta_since_start: "
              << android::elapsedRealtime() - started_at_millis << " millis";

    // Build a CarData message
    // TODO(b/174608802): set a correct data ID and content
    CarData msg;
    msg.id = 101;
    msg.content = {1, 0, 1, 0};

    LOG(INFO) << "Sending the car data, delta_since_start: "
              << android::elapsedRealtime() - started_at_millis << " millis";

    // Send the data
    ndk::ScopedAStatus writeStatus = service->write({msg});

    if (!writeStatus.isOk()) {
        LOG(WARNING) << "Failed to write to the service: " << writeStatus.getMessage();
    }

    // Note: On a device the delta_since_start was between 1ms to 4ms
    //      (service side was not fully implemented yet during the test).
    LOG(INFO) << "Finished, delta_since_start: " << android::elapsedRealtime() - started_at_millis
              << " millis";

    return 0;
}
