/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/binder_process.h>
#include <hidl/HidlTransportSupport.h>

#include <vhal_v2_0/EmulatedVehicleConnector.h>
#include <vhal_v2_0/EmulatedVehicleHal.h>
#include <vhal_v2_0/VehicleHalManager.h>

#include "GrpcVehicleClient.h"
#include "Utils.h"

using android::OK;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::automotive::vehicle::V2_0::VehicleHalManager;
using android::hardware::automotive::vehicle::V2_0::VehiclePropertyStore;

int main(int argc, char* argv[]) {
    namespace vhal_impl = android::hardware::automotive::vehicle::V2_0::impl;

    auto serverInfo = vhal_impl::VirtualizedVhalServerInfo::fromRoPropertyStore();
    CHECK(serverInfo.has_value()) << "Invalid server CID/port combination";
    LOG(INFO) << "Connecting to vsock server at " << serverInfo->vsock.str();

    auto store = std::make_unique<VehiclePropertyStore>();
    auto connector = vhal_impl::makeGrpcVehicleClient(serverInfo->getServerUri());
    auto hal = std::make_unique<vhal_impl::EmulatedVehicleHal>(store.get(), connector.get());
    auto emulator = std::make_unique<vhal_impl::VehicleEmulator>(hal.get());
    auto service = std::make_unique<VehicleHalManager>(hal.get());

    configureRpcThreadpool(4, true /* callerWillJoin */);

    LOG(INFO) << "Registering as service...";
    status_t status = service->registerAsService();

    if (status != OK) {
        LOG(ERROR) << "Unable to register vehicle service (" << status << ")";
        return 1;
    }

    LOG(INFO) << "Ready";
    joinRpcThreadpool();

    // We don't ever actually expect to return, so return an error if we do get here
    return 1;
}
