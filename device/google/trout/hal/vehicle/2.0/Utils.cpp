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

#include "Utils.h"

#ifdef __BIONIC__
#include <cutils/properties.h>
#endif  // __BIONIC__

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <array>
#include <climits>
#include <iostream>
#include <sstream>

using std::cerr;
using std::endl;

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace impl {

#ifdef __BIONIC__
std::string VirtualizedVhalServerInfo::getServerUri() const {
    return vsock.str();
}
#else
std::string VirtualizedVhalServerInfo::getServerUri() const {
    std::stringstream ss;

    ss << "vsock:" << vsock.cid << ":" << vsock.port;
    return ss.str();
}
#endif

bool WaitForReadWithTimeout(int fd, struct timeval&& timeout) {
    fd_set read_fd_set;
    FD_ZERO(&read_fd_set);
    FD_SET(fd, &read_fd_set);
    auto ready = select(FD_SETSIZE, &read_fd_set, nullptr, nullptr, &timeout);

    if (ready < 0) {
        cerr << __func__ << ": fd: " << fd << ", errno: " << errno << ", " << strerror(errno)
             << endl;
        return false;
    }
    return ready > 0;
}

static std::optional<unsigned> parseUnsignedIntFromString(const char* optarg, const char* name) {
    auto v = strtoul(optarg, nullptr, 0);
    if (((v == ULONG_MAX) && (errno == ERANGE)) || (v > UINT_MAX)) {
        cerr << name << " value is out of range: " << optarg << endl;
    } else if (v != 0) {
        return v;
    } else {
        cerr << name << " value is invalid or missing: " << optarg << endl;
    }

    return std::nullopt;
}

std::optional<VirtualizedVhalServerInfo> VirtualizedVhalServerInfo::fromCommandLine(
        int argc, char* argv[], std::string* error) {
    // TODO(egranata): move command-line parsing into vsockinfo
    std::optional<unsigned int> cid;
    std::optional<unsigned int> port;
    std::optional<std::string> powerStateMarkerFilePath;
    std::optional<std::string> powerStateSocketPath;

    // unique values to identify the options
    constexpr int OPT_VHAL_SERVER_CID = 1001;
    constexpr int OPT_VHAL_SERVER_PORT_NUMBER = 1002;
    constexpr int OPT_VHAL_SERVER_POWER_STATE_FILE = 1003;
    constexpr int OPT_VHAL_SERVER_POWER_STATE_SOCKET = 1004;

    struct option longOptions[] = {
            {"server_cid", 1, 0, OPT_VHAL_SERVER_CID},
            {"server_port", 1, 0, OPT_VHAL_SERVER_PORT_NUMBER},
            {"power_state_file", 1, 0, OPT_VHAL_SERVER_POWER_STATE_FILE},
            {"power_state_socket", 1, 0, OPT_VHAL_SERVER_POWER_STATE_SOCKET},
            {},
    };

    int optValue;
    while ((optValue = getopt_long_only(argc, argv, ":", longOptions, 0)) != -1) {
        switch (optValue) {
            case OPT_VHAL_SERVER_CID:
                cid = parseUnsignedIntFromString(optarg, "cid");
                break;
            case OPT_VHAL_SERVER_PORT_NUMBER:
                port = parseUnsignedIntFromString(optarg, "port");
                break;
            case OPT_VHAL_SERVER_POWER_STATE_FILE:
                powerStateMarkerFilePath = std::string(optarg);
                break;
            case OPT_VHAL_SERVER_POWER_STATE_SOCKET:
                powerStateSocketPath = std::string(optarg);
                break;
            default:
                // ignore other options
                break;
        }
    }

    if (!cid.has_value() && error) {
        *error += "Missing server CID. ";
    }
    if (!port.has_value() && error) {
        *error += "Missing server port number. ";
    }
    if (!powerStateMarkerFilePath.has_value() && error) {
        *error += "Missing power state marker file path. ";
    }
    if (!powerStateSocketPath.has_value() && error) {
        *error += "Missing power state socket path. ";
    }

    if (cid && port && powerStateMarkerFilePath && powerStateSocketPath) {
        return VirtualizedVhalServerInfo{
                {*cid, *port}, *powerStateMarkerFilePath, *powerStateSocketPath};
    }
    return std::nullopt;
}

#ifdef __BIONIC__
std::optional<VirtualizedVhalServerInfo> VirtualizedVhalServerInfo::fromRoPropertyStore() {
    auto vsock = android::hardware::automotive::utils::VsockConnectionInfo::fromRoPropertyStore(
            {
                    "ro.boot.vendor.vehiclehal.server.cid",
                    "ro.vendor.vehiclehal.server.cid",
            },
            {
                    "ro.boot.vendor.vehiclehal.server.port",
                    "ro.vendor.vehiclehal.server.port",
            });

    if (vsock) {
        return VirtualizedVhalServerInfo{*vsock, "", ""};
    }
    return std::nullopt;
}
#endif  // __BIONIC__

}  // namespace impl
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
