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

#include "XmlServiceSupplier.h"
#include "ServiceDescriptor.h"

using dumpstate::hal::configuration::V1_0::Service;

static std::optional<ServiceDescriptor> serviceFromXml(const Service& svc) {
    if (svc.hasName() && svc.hasCommand()) {
        return ServiceDescriptor{svc.getName(), svc.getCommand()};
    }
    return std::nullopt;
}

std::optional<XmlServiceSupplier> XmlServiceSupplier::fromFile(const std::string& path) {
    if (auto cfg = dumpstate::hal::configuration::V1_0::readFile(path.c_str())) {
        return XmlServiceSupplier{*cfg};
    }
    return std::nullopt;
}

std::optional<XmlServiceSupplier> XmlServiceSupplier::fromBuffer(const std::string& buffer) {
    if (auto cfg = dumpstate::hal::configuration::V1_0::readBuffer(buffer)) {
        return XmlServiceSupplier{*cfg};
    }
    return std::nullopt;
}

XmlServiceSupplier::XmlServiceSupplier(const DumpstateHalConfiguration& cfg) {
    // TODO(egranata): perform semantic validation before constructing

    if (cfg.hasSystemLogs()) {
        auto sl = cfg.getFirstSystemLogs();
        if (sl->hasService()) {
            auto xsvc = sl->getFirstService();
            if (auto svc = serviceFromXml(*xsvc)) {
                mSystemLogs = *svc;
            }
        }
    }

    if (cfg.hasServices()) {
        auto svcs = cfg.getFirstServices();
        for (const auto& xsvc : svcs->getService()) {
            if (auto svc = serviceFromXml(xsvc)) {
                mServices.push_back(*svc);
            }
        }
    }
}

std::optional<ServiceDescriptor> XmlServiceSupplier::GetSystemLogsService() const {
    return mSystemLogs;
}

std::vector<ServiceDescriptor> XmlServiceSupplier::GetServices() const {
    return mServices;
}
