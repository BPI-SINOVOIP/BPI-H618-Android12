/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <list>
#include <map>
#include <mutex>
#include <vector>

#include <android-base/result.h>
#include <android-base/thread_annotations.h>
#include <netdutils/BackoffSequence.h>
#include <netdutils/DumpWriter.h>
#include <netdutils/InternetAddresses.h>

#include "DnsTlsServer.h"
#include "LockedQueue.h"
#include "PrivateDnsValidationObserver.h"

namespace android {
namespace net {

// TODO: decouple the dependency of DnsTlsServer.
struct PrivateDnsStatus {
    PrivateDnsMode mode;

    // TODO: change the type to std::vector<DnsTlsServer>.
    std::map<DnsTlsServer, Validation, AddressComparator> serversMap;

    std::list<DnsTlsServer> validatedServers() const {
        std::list<DnsTlsServer> servers;

        for (const auto& pair : serversMap) {
            if (pair.second == Validation::success) {
                servers.push_back(pair.first);
            }
        }
        return servers;
    }
};

class PrivateDnsConfiguration {
  public:
    struct ServerIdentity {
        const netdutils::IPSockAddr sockaddr;
        const std::string provider;

        explicit ServerIdentity(const IPrivateDnsServer& server)
            : sockaddr(server.addr()), provider(server.provider()) {}

        bool operator<(const ServerIdentity& other) const {
            return std::tie(sockaddr, provider) < std::tie(other.sockaddr, other.provider);
        }
        bool operator==(const ServerIdentity& other) const {
            return std::tie(sockaddr, provider) == std::tie(other.sockaddr, other.provider);
        }
    };

    // The only instance of PrivateDnsConfiguration.
    static PrivateDnsConfiguration& getInstance() {
        static PrivateDnsConfiguration instance;
        return instance;
    }

    int set(int32_t netId, uint32_t mark, const std::vector<std::string>& servers,
            const std::string& name, const std::string& caCert) EXCLUDES(mPrivateDnsLock);

    PrivateDnsStatus getStatus(unsigned netId) const EXCLUDES(mPrivateDnsLock);

    void clear(unsigned netId) EXCLUDES(mPrivateDnsLock);

    // Request the server to be revalidated on a connection tagged with |mark|.
    // Returns a Result to indicate if the request is accepted.
    base::Result<void> requestValidation(unsigned netId, const ServerIdentity& identity,
                                         uint32_t mark) EXCLUDES(mPrivateDnsLock);

    void setObserver(PrivateDnsValidationObserver* observer);

    void dump(netdutils::DumpWriter& dw) const;

  private:
    typedef std::map<ServerIdentity, std::unique_ptr<IPrivateDnsServer>> PrivateDnsTracker;

    PrivateDnsConfiguration() = default;

    // Launchs a thread to run the validation for |server| on the network |netId|.
    // |isRevalidation| is true if this call is due to a revalidation request.
    void startValidation(const ServerIdentity& identity, unsigned netId, bool isRevalidation)
            REQUIRES(mPrivateDnsLock);

    bool recordPrivateDnsValidation(const ServerIdentity& identity, unsigned netId, bool success,
                                    bool isRevalidation) EXCLUDES(mPrivateDnsLock);

    void sendPrivateDnsValidationEvent(const ServerIdentity& identity, unsigned netId, bool success)
            REQUIRES(mPrivateDnsLock);

    // Decide if a validation for |server| is needed. Note that servers that have failed
    // multiple validation attempts but for which there is still a validating
    // thread running are marked as being Validation::in_process.
    bool needsValidation(const IPrivateDnsServer& server) const REQUIRES(mPrivateDnsLock);

    void updateServerState(const ServerIdentity& identity, Validation state, uint32_t netId)
            REQUIRES(mPrivateDnsLock);

    // For testing.
    base::Result<IPrivateDnsServer*> getPrivateDns(const ServerIdentity& identity, unsigned netId)
            EXCLUDES(mPrivateDnsLock);

    base::Result<IPrivateDnsServer*> getPrivateDnsLocked(const ServerIdentity& identity,
                                                         unsigned netId) REQUIRES(mPrivateDnsLock);

    mutable std::mutex mPrivateDnsLock;
    std::map<unsigned, PrivateDnsMode> mPrivateDnsModes GUARDED_BY(mPrivateDnsLock);

    // Contains all servers for a network, along with their current validation status.
    // In case a server is removed due to a configuration change, it remains in this map,
    // but is marked inactive.
    // Any pending validation threads will continue running because we have no way to cancel them.
    std::map<unsigned, PrivateDnsTracker> mPrivateDnsTransports GUARDED_BY(mPrivateDnsLock);

    void notifyValidationStateUpdate(const netdutils::IPSockAddr& sockaddr, Validation validation,
                                     uint32_t netId) const REQUIRES(mPrivateDnsLock);

    // TODO: fix the reentrancy problem.
    PrivateDnsValidationObserver* mObserver GUARDED_BY(mPrivateDnsLock);

    friend class PrivateDnsConfigurationTest;

    // It's not const because PrivateDnsConfigurationTest needs to override it.
    // TODO: make it const by dependency injection.
    netdutils::BackoffSequence<>::Builder mBackoffBuilder =
            netdutils::BackoffSequence<>::Builder()
                    .withInitialRetransmissionTime(std::chrono::seconds(60))
                    .withMaximumRetransmissionTime(std::chrono::seconds(3600));

    struct RecordEntry {
        RecordEntry(uint32_t netId, const ServerIdentity& identity, Validation state)
            : netId(netId), serverIdentity(identity), state(state) {}

        const uint32_t netId;
        const ServerIdentity serverIdentity;
        const Validation state;
        const std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    };

    LockedRingBuffer<RecordEntry> mPrivateDnsLog{100};
};

}  // namespace net
}  // namespace android
