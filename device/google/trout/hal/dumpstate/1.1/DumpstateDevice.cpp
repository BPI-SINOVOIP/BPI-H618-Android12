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

#include "DumpstateDevice.h"

#include <DumpstateUtil.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>

#include <fstream>
#include <string>

using android::os::dumpstate::CommandOptions;
using android::os::dumpstate::DumpFileToFd;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::literals::chrono_literals::operator""s;

namespace fs = android::hardware::automotive::filesystem;

static constexpr const char* VENDOR_VERBOSE_LOGGING_ENABLED_PROPERTY =
        "persist.vendor.verbose_logging_enabled";

static constexpr const char* VENDOR_HELPER_SYSTEM_LOG_LOC_PROPERTY =
        "ro.vendor.helpersystem.log_loc";

namespace android::hardware::dumpstate::V1_1::implementation {

static std::shared_ptr<::grpc::ChannelCredentials> getChannelCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureChannelCredentials();
}

static void dumpDirAsText(int textFd, const fs::path& dirToDump) {
    for (const auto& fileEntry : fs::recursive_directory_iterator(dirToDump)) {
        if (!fileEntry.is_regular_file()) {
            continue;
        }

        DumpFileToFd(textFd, "Helper System Log", fileEntry.path());
    }
}

static void tryDumpDirAsTar(int textFd, int binFd, const fs::path& dirToDump) {
    if (!fs::is_directory(dirToDump)) {
        LOG(ERROR) << "'" << dirToDump << "'"
                   << " is not a valid directory to dump";
        return;
    }

    if (binFd < 0) {
        LOG(WARNING) << "No binary dumped file, fallback to text mode";
        return dumpDirAsText(textFd, dirToDump);
    }

    TemporaryFile tempTarFile;
    constexpr auto kTarTimeout = 20s;

    RunCommandToFd(
            textFd, "TAR LOG", {"/vendor/bin/tar", "cvf", tempTarFile.path, dirToDump.c_str()},
            CommandOptions::WithTimeout(duration_cast<seconds>(kTarTimeout).count()).Build());

    std::vector<uint8_t> buffer(65536);
    while (true) {
        ssize_t bytes_read = TEMP_FAILURE_RETRY(read(tempTarFile.fd, buffer.data(), buffer.size()));

        if (bytes_read == 0) {
            break;
        } else if (bytes_read < 0) {
            PLOG(DEBUG) << "Error reading temporary tar file(" << tempTarFile.path << ")";
            break;
        }

        ssize_t result = TEMP_FAILURE_RETRY(write(binFd, buffer.data(), bytes_read));

        if (result != bytes_read) {
            LOG(DEBUG) << "Failed to write " << bytes_read
                       << " bytes, actually written: " << result;
            break;
        }
    }
}

bool DumpstateDevice::dumpRemoteLogs(
        ::grpc::ClientReaderInterface<dumpstate_proto::DumpstateBuffer>* grpcReader,
        const fs::path& dumpPath) {
    dumpstate_proto::DumpstateBuffer logStreamBuffer;
    std::fstream logFile(dumpPath, std::fstream::out | std::fstream::binary);

    if (!logFile.is_open()) {
        LOG(ERROR) << "Failed to open file " << dumpPath;
        return false;
    }

    while (grpcReader->Read(&logStreamBuffer)) {
        const auto& writeBuffer = logStreamBuffer.buffer();
        logFile.write(writeBuffer.c_str(), writeBuffer.size());
    }
    auto grpcStatus = grpcReader->Finish();
    if (!grpcStatus.ok()) {
        LOG(ERROR) << __func__ << ": GRPC GetCommandOutput Failed: " << grpcStatus.error_message();
        return false;
    }

    return true;
}

bool DumpstateDevice::dumpHelperSystem(int textFd, int binFd) {
    std::string helperSystemLogDir =
            android::base::GetProperty(VENDOR_HELPER_SYSTEM_LOG_LOC_PROPERTY, "");

    if (helperSystemLogDir.empty()) {
        LOG(ERROR) << "Helper system log location '" << VENDOR_HELPER_SYSTEM_LOG_LOC_PROPERTY
                   << "' not set";
        return false;
    }

    std::error_code error;

    auto helperSysLogPath = fs::path(helperSystemLogDir);
    if (!fs::create_directories(helperSysLogPath, error)) {
        LOG(ERROR) << "Failed to create the dumping log directory " << helperSystemLogDir << ": "
                   << error;
        return false;
    }

    if (!fs::is_directory(helperSysLogPath)) {
        LOG(ERROR) << helperSystemLogDir << " is not a directory";
        return false;
    }

    if (!isHealthy()) {
        LOG(ERROR) << "Failed to connect to the dumpstate server";
        return false;
    }

    // When start dumping, we always return success to keep dumped logs
    // even if some of them are failed

    {
        // Dumping system logs
        ::grpc::ClientContext context;
        auto reader = mGrpcStub->GetSystemLogs(&context, ::google::protobuf::Empty());
        dumpRemoteLogs(reader.get(), helperSysLogPath / "system_log");
    }

    // Request for service list every time to allow the service list to change on the server side.
    // Also the getAvailableServices() may fail and return an empty list (e.g., failure on the
    // server side), and it should not affect the future queries
    const auto availableServices = getAvailableServices();

    // Dumping service logs
    for (const auto& service : availableServices) {
        ::grpc::ClientContext context;
        dumpstate_proto::ServiceLogRequest request;
        request.set_service_name(service);
        auto reader = mGrpcStub->GetServiceLogs(&context, request);
        dumpRemoteLogs(reader.get(), helperSysLogPath / service);
    }

    tryDumpDirAsTar(textFd, binFd, helperSystemLogDir);

    if (fs::remove_all(helperSysLogPath, error) == static_cast<std::uintmax_t>(-1)) {
        LOG(ERROR) << "Failed to clear the dumping log directory " << helperSystemLogDir << ": "
                   << error;
    }
    return true;
}

bool DumpstateDevice::isHealthy() {
    // Check that we can get services back from the remote end
    // This check will not work if the server actually works but is
    // not exporting any services. This seems like a corner case
    // but it's worth pointing out.
    return (getAvailableServices().size() > 0);
}

std::vector<std::string> DumpstateDevice::getAvailableServices() {
    ::grpc::ClientContext context;
    dumpstate_proto::ServiceNameList servicesProto;
    auto grpc_status =
            mGrpcStub->GetAvailableServices(&context, ::google::protobuf::Empty(), &servicesProto);
    if (!grpc_status.ok()) {
        LOG(ERROR) << "Failed to get available services from the server: "
                   << grpc_status.error_message();
        return {};
    }

    std::vector<std::string> services;
    for (auto& service : servicesProto.service_names()) {
        services.emplace_back(service);
    }
    return services;
}

DumpstateDevice::DumpstateDevice(const std::string& addr)
    : mServiceAddr(addr),
      mGrpcChannel(::grpc::CreateChannel(mServiceAddr, getChannelCredentials())),
      mGrpcStub(dumpstate_proto::DumpstateServer::NewStub(mGrpcChannel)) {}

// Methods from ::android::hardware::dumpstate::V1_0::IDumpstateDevice follow.
Return<void> DumpstateDevice::dumpstateBoard(const hidl_handle& handle) {
    // Ignore return value, just return an empty status.
    dumpstateBoard_1_1(handle, DumpstateMode::DEFAULT, 30 * 1000 /* timeoutMillis */);
    return Void();
}

// Methods from ::android::hardware::dumpstate::V1_1::IDumpstateDevice follow.
Return<DumpstateStatus> DumpstateDevice::dumpstateBoard_1_1(const hidl_handle& handle,
                                                            const DumpstateMode /* mode */,
                                                            const uint64_t /* timeoutMillis */) {
    if (handle == nullptr || handle->numFds < 1) {
        LOG(ERROR) << "No FDs";
        return DumpstateStatus::ILLEGAL_ARGUMENT;
    }

    const int textFd = handle->data[0];
    const int binFd = handle->numFds >= 2 ? handle->data[1] : -1;

    if (!dumpHelperSystem(textFd, binFd)) {
        return DumpstateStatus::DEVICE_LOGGING_NOT_ENABLED;
    }

    return DumpstateStatus::OK;
}

Return<void> DumpstateDevice::setVerboseLoggingEnabled(const bool enable) {
    android::base::SetProperty(VENDOR_VERBOSE_LOGGING_ENABLED_PROPERTY, enable ? "true" : "false");
    return Void();
}

Return<bool> DumpstateDevice::getVerboseLoggingEnabled() {
    return android::base::GetBoolProperty(VENDOR_VERBOSE_LOGGING_ENABLED_PROPERTY, false);
}

Return<void> DumpstateDevice::debug(const hidl_handle& h, const hidl_vec<hidl_string>& options) {
    if (h.getNativeHandle() == nullptr || h->numFds == 0) {
        LOG(ERROR) << "Invalid FD passed to debug() function";
        return Void();
    }

    const int fd = h->data[0];
    auto pf = [fd](std::string s) -> void { dprintf(fd, "%s\n", s.c_str()); };
    debugDumpServices(pf);

    return Void();
}

void DumpstateDevice::debugDumpServices(std::function<void(std::string)> f) {
    f("Available services for Dumpstate:");
    for (const auto& svc : getAvailableServices()) {
        f("  " + svc);
    }
}

sp<DumpstateDevice> makeVirtualizationDumpstateDevice(const std::string& addr) {
    return new DumpstateDevice(addr);
}

}  // namespace android::hardware::dumpstate::V1_1::implementation
