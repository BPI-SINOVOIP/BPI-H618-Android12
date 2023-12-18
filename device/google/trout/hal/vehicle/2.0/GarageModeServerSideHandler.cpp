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

#include "GarageModeServerSideHandler.h"

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <thread>

#include <errno.h>
#include <sys/inotify.h>

#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "Utils.h"
#include "vhal_v2_0/VehicleUtils.h"

namespace android::hardware::automotive::vehicle::V2_0::impl {

using std::chrono::duration_cast;
using std::chrono::steady_clock;
using std::literals::chrono_literals::operator""s;

class GarageModeServerSideHandlerImpl : public GarageModeServerSideHandler {
  public:
    GarageModeServerSideHandlerImpl(IVehicleServer* vehicleServer,
                                    VehiclePropValuePool* vehicleObjectPool,
                                    const std::string& powerStateMarkerFilePath)
        : mVehicleServer(vehicleServer),
          mValueObjectPool(vehicleObjectPool),
          mPowerStateMarkerPath(powerStateMarkerFilePath) {
        mThreads.emplace_back(std::bind(&GarageModeServerSideHandlerImpl::PowerStateWatcher, this));
        mThreads.emplace_back(
                std::bind(&GarageModeServerSideHandlerImpl::HeartbeatTimeoutWatcher, this));
    }

    ~GarageModeServerSideHandlerImpl() {
        mShuttingDownFlag.store(true);
        mHeartbeatCV.notify_all();
        for (auto& thread : mThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void HandleHeartbeat() override;

  private:
    void HeartbeatTimeoutWatcher();

    void PowerStateWatcher();

    void HandleNewPowerState();

    recyclable_ptr<VehiclePropValue> CreateApPowerStateReq(VehicleApPowerStateReq state,
                                                           int32_t param);

    IVehicleServer* const mVehicleServer;
    VehiclePropValuePool* const mValueObjectPool;

    // TODO(chenhaosjtuacm): use std::filesystem when toolchain >= gcc8 is available
    const std::string mPowerStateMarkerPath;

    std::atomic<bool> mSystemShuttingDownPrepareFlag{false};
    std::atomic<bool> mShuttingDownFlag{false};
    std::atomic<steady_clock::time_point> mLastHeartbeatTime{};
    std::vector<std::thread> mThreads;
    std::condition_variable mHeartbeatCV;
    std::mutex mHeartbeatMutex;
};

void GarageModeServerSideHandlerImpl::HandleHeartbeat() {
    LOG(DEBUG) << __func__ << ": received heartbeat from the client";
    mLastHeartbeatTime.store(steady_clock::now());
}

void GarageModeServerSideHandlerImpl::HeartbeatTimeoutWatcher() {
    constexpr auto kHeartbeatTimeout = duration_cast<steady_clock::duration>(5s);
    constexpr auto kHeartbeatCheckPeriod = 1s;
    while (!mShuttingDownFlag.load()) {
        if (!mSystemShuttingDownPrepareFlag.load()) {
            std::unique_lock<std::mutex> heartbeatLock(mHeartbeatMutex);
            mHeartbeatCV.wait(heartbeatLock, [this]() {
                return mSystemShuttingDownPrepareFlag.load() || mShuttingDownFlag.load();
            });

            // Reset mLastHeartbeatTime everytime after entering shutdown state
            HandleHeartbeat();
        }
        auto timeSinceLastHeartbeat = steady_clock::now() - mLastHeartbeatTime.load();
        if (timeSinceLastHeartbeat > kHeartbeatTimeout) {
            LOG(ERROR) << __func__ << ": heartbeat timeout!";
            // TODO(chenhaosjtuacm): Shutdown AGL
            break;
        }
        std::this_thread::sleep_for(kHeartbeatCheckPeriod);
    }
}

void GarageModeServerSideHandlerImpl::PowerStateWatcher() {
    constexpr auto kFileStatusCheckPeriod = 1s;

    bool log_marker_file_not_exists_message_once = false;
    bool log_marker_file_no_access_message_once = false;
    auto call_once = [](bool* once_flag, auto&& func) {
        if (!*once_flag) {
            *once_flag = true;
            func();
        }
    };

    while (access(mPowerStateMarkerPath.c_str(), F_OK | R_OK) < 0) {
        if (errno == ENOENT) {
            call_once(&log_marker_file_not_exists_message_once, [this]() {
                LOG(ERROR) << __func__ << ": marker file " << mPowerStateMarkerPath
                           << " has not been created yet.";
            });
        } else {
            call_once(&log_marker_file_no_access_message_once, [this]() {
                LOG(ERROR) << __func__ << ": no read access to marker file "
                           << mPowerStateMarkerPath;
            });
        }
        std::this_thread::sleep_for(kFileStatusCheckPeriod);
    }

    int inotifyFd = inotify_init();
    if (inotifyFd < 0) {
        LOG(ERROR) << __func__ << ": failed to open inotify instance: " << strerror(errno);
        return;
    }

    alignas(alignof(struct inotify_event)) char inotifyEventBuffer[4096] = {0};
    [[maybe_unused]] struct inotify_event& inotifyEvent =
            *reinterpret_cast<struct inotify_event*>(inotifyEventBuffer);

    HandleNewPowerState();
    while (!mShuttingDownFlag.load()) {
        int watchDescriptor =
                inotify_add_watch(inotifyFd, mPowerStateMarkerPath.c_str(), IN_MODIFY);
        if (watchDescriptor < 0) {
            LOG(ERROR) << __func__ << ": failed to watch file " << mPowerStateMarkerPath << " : "
                       << strerror(errno);
            return;
        }

        if (!WaitForReadWithTimeout(inotifyFd, kFileStatusCheckPeriod)) {
            continue;
        }

        auto eventReadLen = read(inotifyFd, inotifyEventBuffer, sizeof(inotifyEventBuffer));
        if (eventReadLen < 0) {
            LOG(ERROR) << __func__ << "failed to read the inotify event: " << strerror(errno);
            return;
        }
        if (eventReadLen < static_cast<ssize_t>(sizeof(struct inotify_event))) {
            LOG(ERROR) << __func__ << ":  failed to read the full event, min event size: "
                       << sizeof(struct inotify_event) << ", read size: " << eventReadLen;
            return;
        }
        HandleNewPowerState();
    }
}

void GarageModeServerSideHandlerImpl::HandleNewPowerState() {
    std::ifstream markerFileStream(mPowerStateMarkerPath);
    std::string powerStateString;

    markerFileStream >> powerStateString;
    LOG(INFO) << __func__ << ": set power state to " << powerStateString;

    if (powerStateString == "shutdown") {
        mVehicleServer->onPropertyValueFromCar(
                *CreateApPowerStateReq(VehicleApPowerStateReq::SHUTDOWN_PREPARE,
                                       toInt(VehicleApPowerStateShutdownParam::CAN_SLEEP)),
                true);
        mSystemShuttingDownPrepareFlag.store(true);
        mHeartbeatCV.notify_all();
    } else if (powerStateString == "on") {
        if (mSystemShuttingDownPrepareFlag.load()) {
            mVehicleServer->onPropertyValueFromCar(
                    *CreateApPowerStateReq(VehicleApPowerStateReq::CANCEL_SHUTDOWN, 0), true);
            mSystemShuttingDownPrepareFlag.store(false);
        } else {
            LOG(INFO) << __func__ << ": not in the shutdown state, nothing changed";
        }
    } else {
        LOG(ERROR) << __func__ << ": unknown power state: " << powerStateString;
    }
}

recyclable_ptr<VehiclePropValue> GarageModeServerSideHandlerImpl::CreateApPowerStateReq(
        VehicleApPowerStateReq state, int32_t param) {
    auto req = mValueObjectPool->obtain(VehiclePropertyType::INT32_VEC, 2);
    req->prop = toInt(VehicleProperty::AP_POWER_STATE_REQ);
    req->areaId = 0;
    req->timestamp = elapsedRealtimeNano();
    req->status = VehiclePropertyStatus::AVAILABLE;
    req->value.int32Values[0] = toInt(state);
    req->value.int32Values[1] = param;
    return req;
}

std::unique_ptr<GarageModeServerSideHandler> makeGarageModeServerSideHandler(
        IVehicleServer* vehicleServer, VehiclePropValuePool* valueObjectPool,
        const std::string& powerStateMarkerFilePath) {
    return std::make_unique<GarageModeServerSideHandlerImpl>(vehicleServer, valueObjectPool,
                                                             powerStateMarkerFilePath);
}

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
