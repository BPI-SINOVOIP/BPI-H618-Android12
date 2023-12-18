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

#pragma once

#include "ServiceDescriptor.h"
#include "ServiceSupplier.h"

#include <optional>
#include <unordered_map>

class DumpstateServer {
  public:
    explicit DumpstateServer(const ServiceSupplier& services);

    ServiceDescriptor::Error GetSystemLogs(ServiceDescriptor::OutputConsumer* out);

    std::vector<std::string> GetAvailableServices();

    ServiceDescriptor::Error GetServiceLogs(const std::string& svc,
                                            ServiceDescriptor::OutputConsumer* out);

  private:
    std::optional<ServiceDescriptor> mSystemLogsService;
    std::unordered_map<std::string, ServiceDescriptor> mServices;
};
