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

// This is a sample reader client for ICarTelemetryInternal.
// TODO(b/186017953): remove this client when CarTelemetryService is implemented.
//
// adb remount  # make sure run "adb disable-verity" before remounting
// adb push $ANDROID_PRODUCT_OUT/system/bin/android.automotive.telemetryd-sampleinternalclient
// /system/bin/
//
// adb shell /system/bin/android.automotive.telemetryd-sampleinternalclient

#define LOG_TAG "cartelemetryd_sampleint"

#include <aidl/android/automotive/telemetry/internal/BnCarDataListener.h>
#include <aidl/android/automotive/telemetry/internal/CarDataInternal.h>
#include <aidl/android/automotive/telemetry/internal/ICarTelemetryInternal.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using ::aidl::android::automotive::telemetry::internal::BnCarDataListener;
using ::aidl::android::automotive::telemetry::internal::CarDataInternal;
using ::aidl::android::automotive::telemetry::internal::ICarDataListener;
using ::aidl::android::automotive::telemetry::internal::ICarTelemetryInternal;
using ::android::base::StringPrintf;

class CarDataListenerImpl : public BnCarDataListener {
public:
    ::ndk::ScopedAStatus onCarDataReceived(
            const std::vector<CarDataInternal>& in_dataList) override;
};

::ndk::ScopedAStatus CarDataListenerImpl::onCarDataReceived(
        const std::vector<CarDataInternal>& dataList) {
    LOG(INFO) << "Received data size = " << dataList.size();
    for (const auto data : dataList) {
        LOG(INFO) << "data.id = " << data.id;
    }
    return ::ndk::ScopedAStatus::ok();
}

int main(int argc, char* argv[]) {
    // The name of the service is described in
    // https://source.android.com/devices/architecture/aidl/aidl-hals#instance-names
    const std::string instance = StringPrintf("%s/default", ICarTelemetryInternal::descriptor);
    LOG(INFO) << "Obtaining: " << instance;
    std::shared_ptr<ICarTelemetryInternal> service = ICarTelemetryInternal::fromBinder(
            ndk::SpAIBinder(AServiceManager_getService(instance.c_str())));
    if (!service) {
        LOG(FATAL) << "ICarTelemetryInternal service not found, may be still initializing?";
    }

    LOG(INFO) << "Setting the listener";
    std::shared_ptr<CarDataListenerImpl> listener = ndk::SharedRefBase::make<CarDataListenerImpl>();
    auto status = service->setListener(listener);
    if (!status.isOk()) {
        LOG(FATAL) << "Failed to set the listener";
    }

    ::ABinderProcess_startThreadPool();
    ::ABinderProcess_joinThreadPool();
    return 1;  // not reachable
}
