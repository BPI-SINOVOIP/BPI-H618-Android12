/**
 * Copyright (c) 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "carwatchdogd"
#define DEBUG false  // STOPSHIP if true.

#include "WatchdogProcessService.h"

#include "WatchdogServiceHelper.h"

#include <android-base/chrono_utils.h>
#include <android-base/file.h>
#include <android-base/macros.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android/automotive/watchdog/BnCarWatchdogClient.h>
#include <android/automotive/watchdog/internal/BnCarWatchdogMonitor.h>
#include <android/automotive/watchdog/internal/BnCarWatchdogServiceForSystem.h>
#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/SystemClock.h>

#include <utility>

namespace android {
namespace automotive {
namespace watchdog {

namespace aawi = ::android::automotive::watchdog::internal;

using aawi::BnCarWatchdogServiceForSystem;
using aawi::ICarWatchdogServiceForSystem;
using ::android::IBinder;
using ::android::sp;
using ::android::String16;
using ::android::base::Error;
using ::android::base::GetProperty;
using ::android::base::ReadFileToString;
using ::android::base::Result;
using ::android::base::StringAppendF;
using ::android::base::StringPrintf;
using ::android::base::Trim;
using ::android::base::WriteStringToFd;
using ::android::binder::Status;
using ::android::hardware::hidl_vec;
using ::android::hardware::interfacesEqual;
using ::android::hardware::Return;
using ::android::hardware::automotive::vehicle::V2_0::IVehicle;
using ::android::hardware::automotive::vehicle::V2_0::ProcessTerminationReason;
using ::android::hardware::automotive::vehicle::V2_0::StatusCode;
using ::android::hardware::automotive::vehicle::V2_0::SubscribeFlags;
using ::android::hardware::automotive::vehicle::V2_0::SubscribeOptions;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropConfig;
using ::android::hardware::automotive::vehicle::V2_0::VehicleProperty;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyStatus;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValue;
using ::android::hidl::base::V1_0::IBase;

namespace {

const std::vector<TimeoutLength> kTimeouts = {TimeoutLength::TIMEOUT_CRITICAL,
                                              TimeoutLength::TIMEOUT_MODERATE,
                                              TimeoutLength::TIMEOUT_NORMAL};

// TimeoutLength is also used as a message ID. Other message IDs should start next to
// TimeoutLength::TIMEOUT_NORMAL.
const int32_t MSG_VHAL_WATCHDOG_ALIVE = static_cast<int>(TimeoutLength::TIMEOUT_NORMAL) + 1;
const int32_t MSG_VHAL_HEALTH_CHECK = MSG_VHAL_WATCHDOG_ALIVE + 1;

// VHAL sends heart beat every 3s. Car watchdog checks if there is the latest heart beat from VHAL
// with 1s marginal time.
constexpr std::chrono::nanoseconds kVhalHealthCheckDelayNs = 4s;
constexpr int64_t kVhalHeartBeatIntervalMs = 3000;

constexpr const char kServiceName[] = "WatchdogProcessService";
constexpr const char kVhalInterfaceName[] = "android.hardware.automotive.vehicle@2.0::IVehicle";

std::chrono::nanoseconds timeoutToDurationNs(const TimeoutLength& timeout) {
    switch (timeout) {
        case TimeoutLength::TIMEOUT_CRITICAL:
            return 3s;  // 3s and no buffer time.
        case TimeoutLength::TIMEOUT_MODERATE:
            return 6s;  // 5s + 1s as buffer time.
        case TimeoutLength::TIMEOUT_NORMAL:
            return 12s;  // 10s + 2s as buffer time.
    }
}

std::string pidArrayToString(const std::vector<int32_t>& pids) {
    size_t size = pids.size();
    if (size == 0) {
        return "";
    }
    std::string buffer;
    StringAppendF(&buffer, "%d", pids[0]);
    for (int i = 1; i < size; i++) {
        int pid = pids[i];
        StringAppendF(&buffer, ", %d", pid);
    }
    return buffer;
}

bool isSystemShuttingDown() {
    std::string sysPowerCtl;
    std::istringstream tokenStream(GetProperty("sys.powerctl", ""));
    std::getline(tokenStream, sysPowerCtl, ',');
    return sysPowerCtl == "reboot" || sysPowerCtl == "shutdown";
}

}  // namespace

WatchdogProcessService::WatchdogProcessService(const sp<Looper>& handlerLooper) :
      mHandlerLooper(handlerLooper),
      mIsEnabled(true),
      mLastSessionId(0),
      mServiceStarted(false),
      mVhalService(nullptr) {
    mMessageHandler = sp<MessageHandlerImpl>::make(this);
    mBinderDeathRecipient = sp<BinderDeathRecipient>::make(this);
    mHidlDeathRecipient = sp<HidlDeathRecipient>::make(this);
    mPropertyChangeListener = sp<PropertyChangeListener>::make(this);
    for (const auto& timeout : kTimeouts) {
        mClients.insert(std::make_pair(timeout, std::vector<ClientInfo>()));
        mPingedClients.insert(std::make_pair(timeout, PingedClientMap()));
    }
}
Result<void> WatchdogProcessService::registerWatchdogServiceHelper(
        const sp<IWatchdogServiceHelper>& helper) {
    if (helper == nullptr) {
        return Error() << "Must provide a non-null watchdog service helper instance";
    }
    Mutex::Autolock lock(mMutex);
    mWatchdogServiceHelper = helper;
    return {};
}

Status WatchdogProcessService::registerClient(const sp<ICarWatchdogClient>& client,
                                              TimeoutLength timeout) {
    pid_t callingPid = IPCThreadState::self()->getCallingPid();
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    ClientInfo clientInfo(client, callingPid, callingUid);

    Mutex::Autolock lock(mMutex);
    return registerClientLocked(clientInfo, timeout);
}

Status WatchdogProcessService::unregisterClient(const sp<ICarWatchdogClient>& client) {
    Mutex::Autolock lock(mMutex);
    sp<IBinder> binder = BnCarWatchdogClient::asBinder(client);
    // kTimeouts is declared as global static constant to cover all kinds of timeout (CRITICAL,
    // MODERATE, NORMAL).
    return unregisterClientLocked(kTimeouts, binder, ClientType::Regular);
}

Status WatchdogProcessService::registerCarWatchdogService(const sp<IBinder>& binder) {
    pid_t callingPid = IPCThreadState::self()->getCallingPid();
    uid_t callingUid = IPCThreadState::self()->getCallingUid();

    Mutex::Autolock lock(mMutex);
    if (mWatchdogServiceHelper == nullptr) {
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE,
                                         "Watchdog service helper instance is null");
    }
    ClientInfo clientInfo(mWatchdogServiceHelper, binder, callingPid, callingUid);
    return registerClientLocked(clientInfo, TimeoutLength::TIMEOUT_CRITICAL);
}

void WatchdogProcessService::unregisterCarWatchdogService(const sp<IBinder>& binder) {
    Mutex::Autolock lock(mMutex);

    std::vector<TimeoutLength> timeouts = {TimeoutLength::TIMEOUT_CRITICAL};
    unregisterClientLocked(timeouts, binder, ClientType::Service);
}

Status WatchdogProcessService::registerMonitor(const sp<aawi::ICarWatchdogMonitor>& monitor) {
    Mutex::Autolock lock(mMutex);
    sp<IBinder> binder = aawi::BnCarWatchdogMonitor::asBinder(monitor);
    if (mMonitor != nullptr && binder == aawi::BnCarWatchdogMonitor::asBinder(mMonitor)) {
        return Status::ok();
    }
    status_t ret = binder->linkToDeath(mBinderDeathRecipient);
    if (ret != OK) {
        ALOGW("Failed to register the monitor as it is dead.");
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "The monitor is dead.");
    }
    mMonitor = monitor;
    if (DEBUG) {
        ALOGD("Car watchdog monitor is registered");
    }
    return Status::ok();
}

Status WatchdogProcessService::unregisterMonitor(const sp<aawi::ICarWatchdogMonitor>& monitor) {
    Mutex::Autolock lock(mMutex);
    sp<IBinder> curBinder = aawi::BnCarWatchdogMonitor::asBinder(mMonitor);
    sp<IBinder> newBinder = aawi::BnCarWatchdogMonitor::asBinder(monitor);
    if (curBinder != newBinder) {
        ALOGW("Failed to unregister the monitor as it has not been registered.");
        return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                         "The monitor has not been registered.");
    }
    curBinder->unlinkToDeath(mBinderDeathRecipient);
    mMonitor = nullptr;
    if (DEBUG) {
        ALOGD("Car watchdog monitor is unregistered");
    }
    return Status::ok();
}

Status WatchdogProcessService::tellClientAlive(const sp<ICarWatchdogClient>& client,
                                               int32_t sessionId) {
    Mutex::Autolock lock(mMutex);
    return tellClientAliveLocked(BnCarWatchdogClient::asBinder(client), sessionId);
}

Status WatchdogProcessService::tellCarWatchdogServiceAlive(
        const sp<ICarWatchdogServiceForSystem>& service,
        const std::vector<int32_t>& clientsNotResponding, int32_t sessionId) {
    Status status;
    {
        Mutex::Autolock lock(mMutex);
        if (DEBUG) {
            std::string buffer;
            int size = clientsNotResponding.size();
            if (size != 0) {
                StringAppendF(&buffer, "%d", clientsNotResponding[0]);
                for (int i = 1; i < clientsNotResponding.size(); i++) {
                    StringAppendF(&buffer, ", %d", clientsNotResponding[i]);
                }
                ALOGD("CarWatchdogService(session: %d) responded with non-responding clients: %s",
                      sessionId, buffer.c_str());
            }
        }
        status = tellClientAliveLocked(BnCarWatchdogServiceForSystem::asBinder(service), sessionId);
    }
    if (status.isOk()) {
        dumpAndKillAllProcesses(clientsNotResponding, true);
    }
    return status;
}

Status WatchdogProcessService::tellDumpFinished(const sp<aawi::ICarWatchdogMonitor>& monitor,
                                                int32_t pid) {
    Mutex::Autolock lock(mMutex);
    if (mMonitor == nullptr || monitor == nullptr ||
        aawi::BnCarWatchdogMonitor::asBinder(monitor) !=
                aawi::BnCarWatchdogMonitor::asBinder(mMonitor)) {
        return Status::
                fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                  "The monitor is not registered or an invalid monitor is given");
    }
    ALOGI("Process(pid: %d) has been dumped and killed", pid);
    return Status::ok();
}

void WatchdogProcessService::setEnabled(bool isEnabled) {
    Mutex::Autolock lock(mMutex);
    if (mIsEnabled != isEnabled) {
        ALOGI("%s is %s", kServiceName, isEnabled ? "enabled" : "disabled");
    }
    mIsEnabled = isEnabled;
    if (mIsEnabled) {
        for (const auto& timeout : kTimeouts) {
            startHealthCheckingLocked(timeout);
        }
    }
}

Status WatchdogProcessService::notifyUserStateChange(userid_t userId, aawi::UserState state) {
    std::string buffer;
    Mutex::Autolock lock(mMutex);
    switch (state) {
        case aawi::UserState::USER_STATE_STARTED:
            mStoppedUserIds.erase(userId);
            buffer = StringPrintf("user(%d) is started", userId);
            break;
        case aawi::UserState::USER_STATE_STOPPED:
            mStoppedUserIds.insert(userId);
            buffer = StringPrintf("user(%d) is stopped", userId);
            break;
        default:
            ALOGW("Unsupported user state: %d", state);
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, "Unsupported user state");
    }
    ALOGI("Received user state change: %s", buffer.c_str());
    return Status::ok();
}

Result<void> WatchdogProcessService::dump(int fd, const Vector<String16>& /*args*/) {
    Mutex::Autolock lock(mMutex);
    const char* indent = "  ";
    const char* doubleIndent = "    ";
    std::string buffer;
    WriteStringToFd("CAR WATCHDOG PROCESS SERVICE\n", fd);
    WriteStringToFd(StringPrintf("%s%s enabled: %s\n", indent, kServiceName,
                                 mIsEnabled ? "true" : "false"),
                    fd);
    WriteStringToFd(StringPrintf("%sRegistered clients\n", indent), fd);
    int count = 1;
    for (const auto& timeout : kTimeouts) {
        std::vector<ClientInfo>& clients = mClients[timeout];
        for (auto it = clients.begin(); it != clients.end(); it++, count++) {
            WriteStringToFd(StringPrintf("%sClient #%d: %s\n", doubleIndent, count,
                                         it->toString().c_str()),
                            fd);
        }
    }
    WriteStringToFd(StringPrintf("%sMonitor registered: %s\n", indent,
                                 mMonitor == nullptr ? "false" : "true"),
                    fd);
    WriteStringToFd(StringPrintf("%sisSystemShuttingDown: %s\n", indent,
                                 isSystemShuttingDown() ? "true" : "false"),
                    fd);
    buffer = "none";
    bool first = true;
    for (const auto& userId : mStoppedUserIds) {
        if (first) {
            buffer = StringPrintf("%d", userId);
            first = false;
        } else {
            StringAppendF(&buffer, ", %d", userId);
        }
    }
    WriteStringToFd(StringPrintf("%sStopped users: %s\n", indent, buffer.c_str()), fd);
    return {};
}

void WatchdogProcessService::doHealthCheck(int what) {
    mHandlerLooper->removeMessages(mMessageHandler, what);
    if (Mutex::Autolock lock(mMutex); !mIsEnabled) {
        return;
    }
    const TimeoutLength timeout = static_cast<TimeoutLength>(what);
    dumpAndKillClientsIfNotResponding(timeout);

    /* Generates a temporary/local vector containing clients.
     * Using a local copy may send unnecessary ping messages to clients after they are unregistered.
     * Clients should be able to handle them.
     */
    std::vector<ClientInfo> clientsToCheck;
    PingedClientMap& pingedClients = mPingedClients[timeout];
    {
        Mutex::Autolock lock(mMutex);
        pingedClients.clear();
        clientsToCheck = mClients[timeout];
        for (auto& clientInfo : clientsToCheck) {
            if (mStoppedUserIds.count(clientInfo.userId) > 0) {
                continue;
            }
            int sessionId = getNewSessionId();
            clientInfo.sessionId = sessionId;
            pingedClients.insert(std::make_pair(sessionId, clientInfo));
        }
    }

    for (const auto& clientInfo : clientsToCheck) {
        Status status = clientInfo.checkIfAlive(timeout);
        if (!status.isOk()) {
            ALOGW("Sending a ping message to client(pid: %d) failed: %s", clientInfo.pid,
                  status.exceptionMessage().c_str());
            {
                Mutex::Autolock lock(mMutex);
                pingedClients.erase(clientInfo.sessionId);
            }
        }
    }
    // Though the size of pingedClients is a more specific measure, clientsToCheck is used as a
    // conservative approach.
    if (clientsToCheck.size() > 0) {
        auto durationNs = timeoutToDurationNs(timeout);
        mHandlerLooper->sendMessageDelayed(durationNs.count(), mMessageHandler, Message(what));
    }
}

Result<void> WatchdogProcessService::start() {
    if (mServiceStarted) {
        return Error(INVALID_OPERATION) << "Cannot start process monitoring more than once";
    }
    mServiceStarted = true;
    reportWatchdogAliveToVhal();
    return {};
}

void WatchdogProcessService::terminate() {
    Mutex::Autolock lock(mMutex);
    for (const auto& timeout : kTimeouts) {
        std::vector<ClientInfo>& clients = mClients[timeout];
        for (auto it = clients.begin(); it != clients.end();) {
            it->unlinkToDeath(mBinderDeathRecipient);
            it = clients.erase(it);
        }
    }
    mWatchdogServiceHelper.clear();
    if (mMonitor != nullptr) {
        sp<IBinder> binder = aawi::BnCarWatchdogMonitor::asBinder(mMonitor);
        binder->unlinkToDeath(mBinderDeathRecipient);
    }
    if (mVhalService != nullptr) {
        mVhalService->unlinkToDeath(mHidlDeathRecipient);
    }
    mServiceStarted = false;
}

Status WatchdogProcessService::registerClientLocked(const ClientInfo& clientInfo,
                                                    TimeoutLength timeout) {
    if (findClientAndProcessLocked(kTimeouts, clientInfo, nullptr)) {
        ALOGW("Failed to register (%s) as it is already registered.",
              clientInfo.toString().c_str());
        return Status::ok();
    }
    status_t status = clientInfo.linkToDeath(mBinderDeathRecipient);
    if (status != OK) {
        ALOGW("Failed to register (%s) as it is dead", clientInfo.toString().c_str());
        std::string errorStr = StringPrintf("(%s) is dead", clientInfo.toString().c_str());
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, errorStr.c_str());
    }
    std::vector<ClientInfo>& clients = mClients[timeout];
    clients.emplace_back(clientInfo);

    // If the client array becomes non-empty, start health checking.
    if (clients.size() == 1) {
        startHealthCheckingLocked(timeout);
    }
    if (DEBUG) {
        ALOGD("Car watchdog client (%s, timeout = %d) is registered", clientInfo.toString().c_str(),
              timeout);
    }
    return Status::ok();
}

Status WatchdogProcessService::unregisterClientLocked(const std::vector<TimeoutLength>& timeouts,
                                                      sp<IBinder> binder, ClientType clientType) {
    const char* clientName = clientType == ClientType::Regular ? "client" : "watchdog service";
    bool result = findClientAndProcessLocked(timeouts, binder,
                                             [&](std::vector<ClientInfo>& clients,
                                                 std::vector<ClientInfo>::const_iterator it) {
                                                 it->unlinkToDeath(mBinderDeathRecipient);
                                                 clients.erase(it);
                                             });
    if (!result) {
        std::string errorStr = StringPrintf("The %s has not been registered", clientName);
        const char* errorCause = errorStr.c_str();
        ALOGW("Failed to unregister the %s: %s", clientName, errorCause);
        return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, errorCause);
    }
    if (DEBUG) {
        ALOGD("Car watchdog %s is unregistered", clientName);
    }
    return Status::ok();
}

Status WatchdogProcessService::tellClientAliveLocked(const sp<IBinder>& binder, int32_t sessionId) {
    for (const auto& timeout : kTimeouts) {
        PingedClientMap& clients = mPingedClients[timeout];
        PingedClientMap::const_iterator it = clients.find(sessionId);
        if (it == clients.cend() || !it->second.matchesBinder(binder)) {
            continue;
        }
        clients.erase(it);
        return Status::ok();
    }
    return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT,
                                     "The client is not registered or the session ID is not found");
}

bool WatchdogProcessService::findClientAndProcessLocked(const std::vector<TimeoutLength> timeouts,
                                                        const ClientInfo& clientInfo,
                                                        const Processor& processor) {
    for (const auto& timeout : timeouts) {
        std::vector<ClientInfo>& clients = mClients[timeout];
        for (auto it = clients.begin(); it != clients.end(); it++) {
            if (std::as_const(*it) != clientInfo) {
                continue;
            }
            if (processor != nullptr) {
                processor(clients, it);
            }
            return true;
        }
    }
    return false;
}

bool WatchdogProcessService::findClientAndProcessLocked(const std::vector<TimeoutLength> timeouts,
                                                        const sp<IBinder> binder,
                                                        const Processor& processor) {
    for (const auto& timeout : timeouts) {
        std::vector<ClientInfo>& clients = mClients[timeout];
        for (auto it = clients.begin(); it != clients.end(); it++) {
            if (!it->matchesBinder(binder)) {
                continue;
            }
            if (processor != nullptr) {
                processor(clients, it);
            }
            return true;
        }
    }
    return false;
}

Result<void> WatchdogProcessService::startHealthCheckingLocked(TimeoutLength timeout) {
    PingedClientMap& clients = mPingedClients[timeout];
    clients.clear();
    int what = static_cast<int>(timeout);
    auto durationNs = timeoutToDurationNs(timeout);
    mHandlerLooper->sendMessageDelayed(durationNs.count(), mMessageHandler, Message(what));
    return {};
}

Result<void> WatchdogProcessService::dumpAndKillClientsIfNotResponding(TimeoutLength timeout) {
    std::vector<int32_t> processIds;
    std::vector<const ClientInfo*> clientsToNotify;
    {
        Mutex::Autolock lock(mMutex);
        PingedClientMap& clients = mPingedClients[timeout];
        for (PingedClientMap::const_iterator it = clients.cbegin(); it != clients.cend(); it++) {
            pid_t pid = -1;
            userid_t userId = -1;
            std::vector<TimeoutLength> timeouts = {timeout};
            findClientAndProcessLocked(timeouts, it->second,
                                       [&](std::vector<ClientInfo>& cachedClients,
                                           std::vector<ClientInfo>::const_iterator
                                                   cachedClientsIt) {
                                           pid = cachedClientsIt->pid;
                                           userId = cachedClientsIt->userId;
                                           cachedClients.erase(cachedClientsIt);
                                       });
            if (pid != -1 && mStoppedUserIds.count(userId) == 0) {
                clientsToNotify.emplace_back(&it->second);
                processIds.push_back(pid);
            }
        }
    }
    for (const ClientInfo*& clientInfo : clientsToNotify) {
        clientInfo->prepareProcessTermination();
    }
    return dumpAndKillAllProcesses(processIds, true);
}

Result<void> WatchdogProcessService::dumpAndKillAllProcesses(
        const std::vector<int32_t>& processesNotResponding, bool reportToVhal) {
    size_t size = processesNotResponding.size();
    if (size == 0) {
        return {};
    }
    std::string pidString = pidArrayToString(processesNotResponding);
    sp<aawi::ICarWatchdogMonitor> monitor;
    {
        Mutex::Autolock lock(mMutex);
        if (mMonitor == nullptr) {
            std::string errorMsg =
                    StringPrintf("Failed to dump and kill processes(pid = %s): Monitor is not set",
                                 pidString.c_str());
            ALOGW("%s", errorMsg.c_str());
            return Error() << errorMsg;
        }
        monitor = mMonitor;
    }
    if (isSystemShuttingDown()) {
        ALOGI("Skip dumping and killing processes(%s): The system is shutting down",
              pidString.c_str());
        return {};
    }
    if (reportToVhal) {
        reportTerminatedProcessToVhal(processesNotResponding);
    }
    monitor->onClientsNotResponding(processesNotResponding);
    if (DEBUG) {
        ALOGD("Dumping and killing processes is requested: %s", pidString.c_str());
    }
    return {};
}

// Handle when car watchdog clients die.
void WatchdogProcessService::handleBinderDeath(const wp<IBinder>& who) {
    Mutex::Autolock lock(mMutex);
    IBinder* binder = who.unsafe_get();
    // Check if dead binder is monitor.
    sp<IBinder> monitor = aawi::BnCarWatchdogMonitor::asBinder(mMonitor);
    if (monitor == binder) {
        mMonitor = nullptr;
        ALOGW("The monitor has died.");
        return;
    }
    findClientAndProcessLocked(kTimeouts, binder,
                               [&](std::vector<ClientInfo>& clients,
                                   std::vector<ClientInfo>::const_iterator it) {
                                   ALOGW("Client(pid: %d) died", it->pid);
                                   clients.erase(it);
                               });
}

// Handle when VHAL dies.
void WatchdogProcessService::handleHidlDeath(const wp<IBase>& who) {
    Mutex::Autolock lock(mMutex);
    if (!interfacesEqual(mVhalService, who.promote())) {
        return;
    }
    ALOGW("VHAL has died.");
    mVhalService->unlinkToDeath(mHidlDeathRecipient);
    mVhalService = nullptr;
}

void WatchdogProcessService::reportWatchdogAliveToVhal() {
    if (mNotSupportedVhalProperties.count(VehicleProperty::WATCHDOG_ALIVE) > 0) {
        ALOGW("VHAL doesn't support WATCHDOG_ALIVE. Car watchdog will not update WATCHDOG_ALIVE.");
        return;
    }
    int64_t systemUptime = uptimeMillis();
    VehiclePropValue propValue{
            .prop = static_cast<int32_t>(VehicleProperty::WATCHDOG_ALIVE),
            .status = VehiclePropertyStatus::AVAILABLE,
            .value = {.int64Values = {systemUptime}},
    };
    const auto& ret = updateVhal(propValue);
    if (!ret.ok()) {
        ALOGW("Failed to update WATCHDOG_ALIVE VHAL property. Will try again in 3s");
    }
    // Update VHAL with the interval of TIMEOUT_CRITICAL(3s).
    auto durationNs = timeoutToDurationNs(TimeoutLength::TIMEOUT_CRITICAL);
    mHandlerLooper->removeMessages(mMessageHandler, MSG_VHAL_WATCHDOG_ALIVE);
    mHandlerLooper->sendMessageDelayed(durationNs.count(), mMessageHandler,
                                       Message(MSG_VHAL_WATCHDOG_ALIVE));
}

void WatchdogProcessService::reportTerminatedProcessToVhal(
        const std::vector<int32_t>& processesNotResponding) {
    if (mNotSupportedVhalProperties.count(VehicleProperty::WATCHDOG_TERMINATED_PROCESS) > 0) {
        ALOGW("VHAL doesn't support WATCHDOG_TERMINATED_PROCESS. Terminated process is not "
              "reported to VHAL.");
        return;
    }
    for (auto&& pid : processesNotResponding) {
        const auto& retCmdLine = readProcCmdLine(pid);
        if (!retCmdLine.ok()) {
            ALOGW("Failed to get process command line for pid(%d): %s", pid,
                  retCmdLine.error().message().c_str());
            continue;
        }
        std::string procCmdLine = retCmdLine.value();
        VehiclePropValue propValue{
                .prop = static_cast<int32_t>(VehicleProperty::WATCHDOG_TERMINATED_PROCESS),
                .status = VehiclePropertyStatus::AVAILABLE,
                .value = {
                         .int32Values = {static_cast<int32_t>(
                                 ProcessTerminationReason::NOT_RESPONDING)},
                         .stringValue = procCmdLine,
                },
        };
        const auto& retUpdate = updateVhal(propValue);
        if (!retUpdate.ok()) {
            ALOGW("Failed to update WATCHDOG_TERMINATED_PROCESS VHAL property(command line: %s)",
                  procCmdLine.c_str());
        }
    }
}

Result<void> WatchdogProcessService::updateVhal(const VehiclePropValue& value) {
    Mutex::Autolock lock(mMutex);
    const auto& connectRet = connectToVhalLocked();
    if (!connectRet.ok()) {
        std::string errorMsg = "VHAL is not connected: " + connectRet.error().message();
        ALOGW("%s", errorMsg.c_str());
        return Error() << errorMsg;
    }
    if (mNotSupportedVhalProperties.count(static_cast<VehicleProperty>(value.prop)) > 0) {
        std::string errorMsg = StringPrintf("VHAL doesn't support property(id: %d)", value.prop);
        ALOGW("%s", errorMsg.c_str());
        return Error() << errorMsg;
    }
    const auto& updateRet = mVhalService->set(value);
    if (updateRet.isOk() && updateRet == StatusCode::OK) {
        return {};
    }
    return Error() << "Failed to set propValue(" << value.prop << ") to VHAL";
}

Result<std::string> WatchdogProcessService::readProcCmdLine(int32_t pid) {
    std::string cmdLinePath = StringPrintf("/proc/%d/cmdline", pid);
    std::string procCmdLine;
    if (ReadFileToString(cmdLinePath, &procCmdLine)) {
        std::replace(procCmdLine.begin(), procCmdLine.end(), '\0', ' ');
        procCmdLine = Trim(procCmdLine);
        return procCmdLine;
    }
    return Error() << "Failed to read " << cmdLinePath;
}

Result<void> WatchdogProcessService::connectToVhalLocked() {
    if (mVhalService.get() != nullptr) {
        return {};
    }
    mVhalService = IVehicle::tryGetService();
    if (mVhalService.get() == nullptr) {
        return Error() << "Failed to connect to VHAL.";
    }
    mVhalService->linkToDeath(mHidlDeathRecipient, /*cookie=*/0);
    queryVhalPropertiesLocked();
    subscribeToVhalHeartBeatLocked();
    ALOGI("Successfully connected to VHAL.");
    return {};
}

void WatchdogProcessService::queryVhalPropertiesLocked() {
    mNotSupportedVhalProperties.clear();
    std::vector<VehicleProperty> propIds = {VehicleProperty::WATCHDOG_ALIVE,
                                            VehicleProperty::WATCHDOG_TERMINATED_PROCESS,
                                            VehicleProperty::VHAL_HEARTBEAT};
    for (const auto& propId : propIds) {
        if (!isVhalPropertySupportedLocked(propId)) {
            mNotSupportedVhalProperties.insert(propId);
        }
    }
}

bool WatchdogProcessService::isVhalPropertySupportedLocked(VehicleProperty propId) {
    StatusCode status;
    hidl_vec<int32_t> props = {static_cast<int32_t>(propId)};
    mVhalService->getPropConfigs(props,
                                 [&status](StatusCode s,
                                           hidl_vec<VehiclePropConfig> /*propConfigs*/) {
                                     status = s;
                                 });
    return status == StatusCode::OK;
}

void WatchdogProcessService::subscribeToVhalHeartBeatLocked() {
    if (mNotSupportedVhalProperties.count(VehicleProperty::VHAL_HEARTBEAT) > 0) {
        ALOGW("VHAL doesn't support VHAL_HEARTBEAT. Checking VHAL health is disabled.");
        return;
    }

    mVhalHeartBeat = {
            .eventTime = 0,
            .value = 0,
    };

    SubscribeOptions reqVhalProperties[] = {
            {.propId = static_cast<int32_t>(VehicleProperty::VHAL_HEARTBEAT),
             .flags = SubscribeFlags::EVENTS_FROM_CAR},
    };
    hidl_vec<SubscribeOptions> options;
    options.setToExternal(reqVhalProperties, arraysize(reqVhalProperties));
    StatusCode status = mVhalService->subscribe(mPropertyChangeListener, options);
    if (status != StatusCode::OK) {
        ALOGW("Failed to subscribe to VHAL_HEARTBEAT. Checking VHAL health is disabled.");
        return;
    }
    mHandlerLooper->sendMessageDelayed(kVhalHealthCheckDelayNs.count(), mMessageHandler,
                                       Message(MSG_VHAL_HEALTH_CHECK));
}

int32_t WatchdogProcessService::getNewSessionId() {
    // Make sure that session id is always positive number.
    if (++mLastSessionId <= 0) {
        mLastSessionId = 1;
    }
    return mLastSessionId;
}

void WatchdogProcessService::updateVhalHeartBeat(int64_t value) {
    bool wrongHeartBeat;
    {
        Mutex::Autolock lock(mMutex);
        wrongHeartBeat = value <= mVhalHeartBeat.value;
        mVhalHeartBeat.eventTime = uptimeMillis();
        mVhalHeartBeat.value = value;
    }
    if (wrongHeartBeat) {
        ALOGW("VHAL updated heart beat with a wrong value. Terminating VHAL...");
        terminateVhal();
        return;
    }
    mHandlerLooper->sendMessageDelayed(kVhalHealthCheckDelayNs.count(), mMessageHandler,
                                       Message(MSG_VHAL_HEALTH_CHECK));
}

void WatchdogProcessService::checkVhalHealth() {
    int64_t lastEventTime;
    int64_t currentUptime = uptimeMillis();
    {
        Mutex::Autolock lock(mMutex);
        lastEventTime = mVhalHeartBeat.eventTime;
    }
    if (currentUptime > lastEventTime + kVhalHeartBeatIntervalMs) {
        ALOGW("VHAL failed to update heart beat within timeout. Terminating VHAL...");
        terminateVhal();
    }
}

void WatchdogProcessService::terminateVhal() {
    using ::android::hidl::manager::V1_0::IServiceManager;

    std::vector<int32_t> processIds;
    sp<IServiceManager> manager = IServiceManager::getService();
    Return<void> ret = manager->debugDump([&](auto& hals) {
        for (const auto& info : hals) {
            if (info.pid == static_cast<int>(IServiceManager::PidConstant::NO_PID)) {
                continue;
            }
            if (info.interfaceName == kVhalInterfaceName) {
                processIds.push_back(info.pid);
                break;
            }
        }
    });

    if (!ret.isOk()) {
        ALOGE("Failed to terminate VHAL: could not get VHAL process id");
        return;
    } else if (processIds.empty()) {
        ALOGE("Failed to terminate VHAL: VHAL is not running");
        return;
    }
    dumpAndKillAllProcesses(processIds, false);
}

std::string WatchdogProcessService::ClientInfo::toString() const {
    std::string buffer;
    StringAppendF(&buffer, "pid = %d, userId = %d, type = %s", pid, userId,
                  type == ClientType::Regular ? "regular" : "watchdog service");
    return buffer;
}

sp<IBinder> WatchdogProcessService::ClientInfo::getBinder() const {
    if (type == ClientType::Regular) {
        return BnCarWatchdogClient::asBinder(client);
    }
    return watchdogServiceBinder;
}

status_t WatchdogProcessService::ClientInfo::linkToDeath(
        const sp<IBinder::DeathRecipient>& recipient) const {
    if (type == ClientType::Regular) {
        return BnCarWatchdogClient::asBinder(client)->linkToDeath(recipient);
    }
    // WatchdogServiceHelper is the binder death recipient for watchdog service, ergo
    // skip this step.
    return OK;
}

status_t WatchdogProcessService::ClientInfo::unlinkToDeath(
        const wp<IBinder::DeathRecipient>& recipient) const {
    if (type == ClientType::Regular) {
        return BnCarWatchdogClient::asBinder(client)->unlinkToDeath(recipient);
    }
    // WatchdogServiceHelper is the binder death recipient for watchdog service, ergo
    // skip this step.
    return OK;
}

Status WatchdogProcessService::ClientInfo::checkIfAlive(TimeoutLength timeout) const {
    if (type == ClientType::Regular) {
        return client->checkIfAlive(sessionId, timeout);
    }
    return watchdogServiceHelper->checkIfAlive(watchdogServiceBinder, sessionId, timeout);
}

Status WatchdogProcessService::ClientInfo::prepareProcessTermination() const {
    if (type == ClientType::Regular) {
        return client->prepareProcessTermination();
    }
    return watchdogServiceHelper->prepareProcessTermination(watchdogServiceBinder);
}

WatchdogProcessService::BinderDeathRecipient::BinderDeathRecipient(
        const sp<WatchdogProcessService>& service) :
      mService(service) {}

void WatchdogProcessService::BinderDeathRecipient::binderDied(const wp<IBinder>& who) {
    mService->handleBinderDeath(who);
}

WatchdogProcessService::HidlDeathRecipient::HidlDeathRecipient(
        const sp<WatchdogProcessService>& service) :
      mService(service) {}

void WatchdogProcessService::HidlDeathRecipient::serviceDied(uint64_t /*cookie*/,
                                                             const wp<IBase>& who) {
    mService->handleHidlDeath(who);
}

WatchdogProcessService::PropertyChangeListener::PropertyChangeListener(
        const sp<WatchdogProcessService>& service) :
      mService(service) {}

Return<void> WatchdogProcessService::PropertyChangeListener::onPropertyEvent(
        const hidl_vec<VehiclePropValue>& propValues) {
    for (const auto& value : propValues) {
        if (value.prop == static_cast<int32_t>(VehicleProperty::VHAL_HEARTBEAT)) {
            mService->updateVhalHeartBeat(value.value.int64Values[0]);
            break;
        }
    }
    return Return<void>();
}

Return<void> WatchdogProcessService::PropertyChangeListener::onPropertySet(
        const VehiclePropValue& /*propValue*/) {
    return Return<void>();
}

Return<void> WatchdogProcessService::PropertyChangeListener::onPropertySetError(
        StatusCode /*status*/, int32_t /*propId*/, int32_t /*areaId*/) {
    return Return<void>();
}

WatchdogProcessService::MessageHandlerImpl::MessageHandlerImpl(
        const sp<WatchdogProcessService>& service) :
      mService(service) {}

void WatchdogProcessService::MessageHandlerImpl::handleMessage(const Message& message) {
    switch (message.what) {
        case static_cast<int>(TimeoutLength::TIMEOUT_CRITICAL):
        case static_cast<int>(TimeoutLength::TIMEOUT_MODERATE):
        case static_cast<int>(TimeoutLength::TIMEOUT_NORMAL):
            mService->doHealthCheck(message.what);
            break;
        case MSG_VHAL_WATCHDOG_ALIVE:
            mService->reportWatchdogAliveToVhal();
            break;
        case MSG_VHAL_HEALTH_CHECK:
            mService->checkVhalHealth();
            break;
        default:
            ALOGW("Unknown message: %d", message.what);
    }
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
