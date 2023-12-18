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
#include "utils.h"

#include <sstream>

#include "parse_string.h"

namespace android::vintf::details {

bool canConvertToFqInstance(const std::string& package, const Version& version,
                            const std::string& interface, const std::string& instance,
                            HalFormat format, std::string* appendedError) {
    if (FqInstance::from(package, version.majorVer, version.minorVer, interface, instance)
            .has_value()) {
        return true;
    }
    if (appendedError == nullptr) {
        return false;
    }

    // Attempt to construct a good error message.
    std::stringstream ss;
    ss << "Invalid instance: '";
    if (format == HalFormat::AIDL) {
        ss << toAidlFqnameString(package, interface, instance) << " (@" << version.minorVer << ")";
    } else {
        ss << toFQNameString(package, version, interface, instance);
    }
    ss << "'. ";

    // Attempt to guess the source of error.
    bool foundError = false;
    if (!FqInstance::from(package).has_value()) {
        ss << "Package '" << package
           << "' should have the format [a-zA-Z_][a-zA-Z0-9_]*(\\.[a-zA-Z_][a-zA-Z0-9_]*)*";
        foundError = true;
    }

    std::optional<FqInstance> convertedInterface = FqInstance::from(interface);
    if (!convertedInterface.has_value() || !convertedInterface->hasInterface()) {
        ss << "Interface '" << interface << "' should have the format I[a-zA-Z0-9_]*";
        foundError = true;
    }

    if (!foundError) {
        ss << "Unknown error.";
    }
    ss << "\n";

    *appendedError += ss.str();
    return false;
}

}  // namespace android::vintf::details
