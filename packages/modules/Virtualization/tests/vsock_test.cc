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
#include <unistd.h>

// Needs to be included after sys/socket.h
#include <linux/vm_sockets.h>

#include <iostream>
#include <optional>

#include "android-base/file.h"
#include "android-base/logging.h"
#include "android-base/parseint.h"
#include "android-base/unique_fd.h"
#include "virt/VirtualizationTest.h"

using namespace android::base;
using namespace android::os;

namespace virt {

static constexpr int kGuestPort = 45678;
static constexpr const char kVmConfigPath[] = "/data/local/tmp/virt-test/vsock_config.json";
static constexpr const char kTestMessage[] = "HelloWorld";

TEST_F(VirtualizationTest, TestVsock) {
    binder::Status status;

    unique_fd server_fd(TEMP_FAILURE_RETRY(socket(AF_VSOCK, SOCK_STREAM, 0)));
    ASSERT_GE(server_fd, 0) << strerror(errno);

    struct sockaddr_vm server_sa = (struct sockaddr_vm){
            .svm_family = AF_VSOCK,
            .svm_port = kGuestPort,
            .svm_cid = VMADDR_CID_ANY,
    };

    int ret = TEMP_FAILURE_RETRY(bind(server_fd, (struct sockaddr *)&server_sa, sizeof(server_sa)));
    ASSERT_EQ(ret, 0) << strerror(errno);

    LOG(INFO) << "Listening on port " << kGuestPort << "...";
    ret = TEMP_FAILURE_RETRY(listen(server_fd, 1));
    ASSERT_EQ(ret, 0) << strerror(errno);

    sp<IVirtualMachine> vm;
    unique_fd vm_config_fd(open(kVmConfigPath, O_RDONLY | O_CLOEXEC));
    status =
            mVirtManager->startVm(ParcelFileDescriptor(std::move(vm_config_fd)), std::nullopt, &vm);
    ASSERT_TRUE(status.isOk()) << "Error starting VM: " << status;

    int32_t cid;
    status = vm->getCid(&cid);
    ASSERT_TRUE(status.isOk()) << "Error getting CID: " << status;
    LOG(INFO) << "VM starting with CID " << cid;

    LOG(INFO) << "Accepting connection...";
    struct sockaddr_vm client_sa;
    socklen_t client_sa_len = sizeof(client_sa);
    unique_fd client_fd(
            TEMP_FAILURE_RETRY(accept(server_fd, (struct sockaddr *)&client_sa, &client_sa_len)));
    ASSERT_GE(client_fd, 0) << strerror(errno);
    LOG(INFO) << "Connection from CID " << client_sa.svm_cid << " on port " << client_sa.svm_port;

    LOG(INFO) << "Reading message from the client...";
    std::string msg;
    ASSERT_TRUE(ReadFdToString(client_fd, &msg));

    LOG(INFO) << "Received message: " << msg;
    ASSERT_EQ(msg, kTestMessage);
}

} // namespace virt
