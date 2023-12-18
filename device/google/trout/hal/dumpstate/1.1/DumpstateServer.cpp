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

#include "DumpstateServer.h"

#include <iostream>

DumpstateServer::DumpstateServer(const ServiceSupplier& services) {
    mSystemLogsService = services.GetSystemLogsService();
    for (auto svc : services.GetServices()) {
        mServices.emplace(svc.name(), svc);
    }

    services.dump(std::cerr);
}

ServiceDescriptor::Error DumpstateServer::GetSystemLogs(ServiceDescriptor::OutputConsumer* out) {
    if (mSystemLogsService)
        return mSystemLogsService->GetOutput(out);
    else
        return "system logs missing";
}

std::vector<std::string> DumpstateServer::GetAvailableServices() {
    std::vector<std::string> ret;

    for (auto& svc : mServices) {
        if (svc.second.IsAvailable()) ret.push_back(svc.first);
    }

    return ret;
}

ServiceDescriptor::Error DumpstateServer::GetServiceLogs(const std::string& svc,
                                                         ServiceDescriptor::OutputConsumer* out) {
    auto iter = mServices.find(svc);
    if (iter == mServices.end()) {
        return "Bad service name: " + svc;
    }

    return iter->second.GetOutput(out);
}
