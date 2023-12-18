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

#include <sys/socket.h>

// Needs to be included after sys/socket.h
#include <linux/vm_sockets.h>

#include <iostream>

#include "android-base/file.h"
#include "android-base/logging.h"
#include "android-base/parseint.h"
#include "android-base/unique_fd.h"

using namespace android::base;

int main(int argc, const char *argv[]) {
    SetLogger(StderrLogger);

    unsigned int cid, port;
    if (argc != 4 || !ParseUint(argv[1], &cid) || !ParseUint(argv[2], &port)) {
        LOG(ERROR) << "Usage: " << argv[0] << " <cid> <port> <msg>";
        return EXIT_FAILURE;
    }
    std::string msg(argv[3]);

    unique_fd fd(TEMP_FAILURE_RETRY(socket(AF_VSOCK, SOCK_STREAM, 0)));
    if (fd < 0) {
        PLOG(ERROR) << "socket";
        return EXIT_FAILURE;
    }

    struct sockaddr_vm sa = (struct sockaddr_vm){
            .svm_family = AF_VSOCK,
            .svm_port = port,
            .svm_cid = cid,
    };

    LOG(INFO) << "Connecting to CID " << cid << " on port " << port << "...";
    int ret = TEMP_FAILURE_RETRY(connect(fd, (struct sockaddr *)&sa, sizeof(sa)));
    if (ret < 0) {
        PLOG(ERROR) << "connect";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Sending message to server...";
    if (!WriteStringToFd(msg, fd)) {
        PLOG(ERROR) << "WriteStringToFd";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Exiting...";
    return EXIT_SUCCESS;
}
