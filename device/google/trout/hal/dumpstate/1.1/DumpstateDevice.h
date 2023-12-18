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

#include <android/hardware/dumpstate/1.1/IDumpstateDevice.h>

#include <automotive/filesystem>
#include <functional>

#include <grpc++/grpc++.h>

#include "DumpstateServer.grpc.pb.h"
#include "DumpstateServer.pb.h"

namespace android::hardware::dumpstate::V1_1::implementation {

namespace fs = android::hardware::automotive::filesystem;

class DumpstateDevice : public IDumpstateDevice {
  public:
    explicit DumpstateDevice(const std::string& addr);

    // Methods from ::android::hardware::dumpstate::V1_0::IDumpstateDevice follow.
    Return<void> dumpstateBoard(const hidl_handle& h) override;

    // Methods from ::android::hardware::dumpstate::V1_1::IDumpstateDevice follow.
    Return<DumpstateStatus> dumpstateBoard_1_1(const hidl_handle& h, const DumpstateMode mode,
                                               const uint64_t timeoutMillis) override;
    Return<void> setVerboseLoggingEnabled(const bool enable) override;
    Return<bool> getVerboseLoggingEnabled() override;

    bool isHealthy();

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options);

  private:
    bool dumpRemoteLogs(::grpc::ClientReaderInterface<dumpstate_proto::DumpstateBuffer>* reader,
                        const fs::path& dumpPath);

    bool dumpHelperSystem(int textFd, int binFd);

    void debugDumpServices(std::function<void(std::string)> f);

    std::vector<std::string> getAvailableServices();

    std::string mServiceAddr;
    std::shared_ptr<::grpc::Channel> mGrpcChannel;
    std::unique_ptr<dumpstate_proto::DumpstateServer::Stub> mGrpcStub;
};

sp<DumpstateDevice> makeVirtualizationDumpstateDevice(const std::string& addr);

}  // namespace android::hardware::dumpstate::V1_1::implementation
