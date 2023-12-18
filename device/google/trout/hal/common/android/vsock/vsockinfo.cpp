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

#include "vsockinfo.h"

#ifdef __BIONIC__
#include <sstream>

#include <android-base/logging.h>
#include <cutils/properties.h>

namespace android::hardware::automotive::utils {

namespace {
std::optional<unsigned> getNumberFromProperty(const char* key) {
    auto value = property_get_int64(key, -1);
    if ((value <= 0) || (value > UINT_MAX)) {
        LOG(WARNING) << key << " is missing or out of bounds";
        return std::nullopt;
    }

    return static_cast<unsigned int>(value);
}

std::optional<unsigned> getNumberFromProperties(const PropertyList& arr) {
    for (const auto& key : arr) {
        auto val = getNumberFromProperty(key.c_str());
        if (val) return val;
    }

    return std::nullopt;
}
}  // namespace

std::optional<VsockConnectionInfo> VsockConnectionInfo::fromRoPropertyStore(
        const PropertyList& cid_props, const PropertyList& port_props) {
    const auto cid = getNumberFromProperties(cid_props);
    const auto port = getNumberFromProperties(port_props);

    if (cid && port) {
        return {{*cid, *port}};
    } else {
        return {};
    }
}

std::string VsockConnectionInfo::str() const {
    std::stringstream ss;

    ss << "vsock:" << cid << ":" << port;
    return ss.str();
}

}  // namespace android::hardware::automotive::utils
#endif  // __BIONIC__
