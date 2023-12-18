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

#include "PowerStateListener.h"

#include <chrono>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#include <android-base/logging.h>

#include "Utils.h"

namespace android::hardware::automotive::vehicle::V2_0::impl {

static bool ForwardSocketToFile(int sockfd, const std::string& filePath) {
    char buffer[1024] = {0};
    auto readlen = read(sockfd, buffer, sizeof(buffer));
    if (readlen < 0) {
        LOG(ERROR) << __func__ << ": read error: " << strerror(errno);
        return false;
    } else if (readlen > 0) {
        auto tempFilePath = filePath + ".XXXXXX";
        auto tempFileFd = mkstemp(const_cast<char*>(tempFilePath.c_str()));
        LOG(INFO) << "write to temp file " << tempFilePath;

        if (tempFileFd < 0) {
            LOG(ERROR) << __func__ << ": failed to create temp file " << tempFilePath << ": "
                       << strerror(errno);
            return false;
        }

        auto writelen = write(tempFileFd, buffer, readlen);
        if (writelen < 0) {
            LOG(ERROR) << __func__ << ": write error to temp file " << tempFilePath << ": "
                       << strerror(errno);
            return false;
        } else if (writelen != readlen) {
            LOG(ERROR) << __func__ << ": failed to write the entire buffer to the temp file, "
                       << "buffer: " << buffer << ", length: " << readlen
                       << "bytes written: " << writelen;
        }

        close(tempFileFd);
        LOG(INFO) << "move " << tempFilePath << " to " << filePath;
        rename(tempFilePath.c_str(), filePath.c_str());
    }
    return true;
}

PowerStateListener::PowerStateListener(const std::string& socketPath,
                                       const std::string& powerStateMarkerFilePath)
    : mSocketPath(socketPath), mPowerStateMarkerFilePath(powerStateMarkerFilePath) {}

void PowerStateListener::Listen() {
    using std::literals::chrono_literals::operator""s;

    // Newly created files are not accessible by other users
    umask(0077);

    int socketfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (socketfd < 0) {
        LOG(ERROR) << __func__ << ": failed to create UNIX socket: " << strerror(errno);
        return;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (mSocketPath.length() >= sizeof(addr.sun_path)) {
        LOG(ERROR) << __func__ << ": socket file path " << mSocketPath << " is longer than limit "
                   << sizeof(addr.sun_path);
        return;
    }
    std::strncpy(addr.sun_path, mSocketPath.c_str(), mSocketPath.length());

    unlink(mSocketPath.c_str());
    if (bind(socketfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG(ERROR) << __func__ << ": failed to bind the address " << mSocketPath
                   << " to the socket: " << strerror(errno);
        return;
    }

    if (listen(socketfd, 1) < 0) {
        LOG(ERROR) << __func__ << ": failed to listen on the socket " << mSocketPath << ": "
                   << strerror(errno);
        return;
    }

    constexpr auto kSocketCheckPeriod = 1s;

    while (!mShuttingDownFlag.load()) {
        if (!WaitForReadWithTimeout(socketfd, kSocketCheckPeriod)) {
            continue;
        }

        socklen_t socklen = sizeof(addr);
        int fd = accept(socketfd, reinterpret_cast<sockaddr*>(&addr), &socklen);

        if (fd == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                PLOG(ERROR) << __func__ << ": failed to accept, path: " << mSocketPath;
            }
            continue;
        }

        if (!ForwardSocketToFile(fd, mPowerStateMarkerFilePath)) {
            LOG(ERROR) << __func__ << ": failed to forward power state, "
                       << "path: " << mPowerStateMarkerFilePath;
            continue;
        }

        close(fd);
    }
}

void PowerStateListener::Stop() {
    mShuttingDownFlag.store(true);
}

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
