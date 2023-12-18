/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "resolv"

#include "DnsTlsDispatcher.h"

#include <netdutils/Stopwatch.h>

#include "DnsTlsSocketFactory.h"
#include "Experiments.h"
#include "PrivateDnsConfiguration.h"
#include "resolv_cache.h"
#include "resolv_private.h"
#include "stats.pb.h"

#include <android-base/logging.h>

namespace android {
namespace net {

using android::netdutils::IPSockAddr;
using android::netdutils::Stopwatch;
using netdutils::Slice;

// static
std::mutex DnsTlsDispatcher::sLock;

DnsTlsDispatcher::DnsTlsDispatcher() {
    mFactory.reset(new DnsTlsSocketFactory());
}

DnsTlsDispatcher& DnsTlsDispatcher::getInstance() {
    static DnsTlsDispatcher instance;
    return instance;
}

std::list<DnsTlsServer> DnsTlsDispatcher::getOrderedAndUsableServerList(
        const std::list<DnsTlsServer>& tlsServers, unsigned netId, unsigned mark) {
    // Our preferred DnsTlsServer order is:
    //     1) reuse existing IPv6 connections
    //     2) reuse existing IPv4 connections
    //     3) establish new IPv6 connections
    //     4) establish new IPv4 connections
    std::list<DnsTlsServer> existing6;
    std::list<DnsTlsServer> existing4;
    std::list<DnsTlsServer> new6;
    std::list<DnsTlsServer> new4;

    // Pull out any servers for which we might have existing connections and
    // place them at the from the list of servers to try.
    {
        std::lock_guard guard(sLock);

        for (const auto& tlsServer : tlsServers) {
            const Key key = std::make_pair(mark, tlsServer);
            if (const Transport* xport = getTransport(key); xport != nullptr) {
                // DoT revalidation specific feature.
                if (!xport->usable()) {
                    // Don't use this xport. It will be removed after timeout
                    // (IDLE_TIMEOUT minutes).
                    LOG(VERBOSE) << "Skip using DoT server " << tlsServer.toIpString() << " on "
                               << netId;
                    continue;
                }

                switch (tlsServer.ss.ss_family) {
                    case AF_INET:
                        existing4.push_back(tlsServer);
                        break;
                    case AF_INET6:
                        existing6.push_back(tlsServer);
                        break;
                }
            } else {
                switch (tlsServer.ss.ss_family) {
                    case AF_INET:
                        new4.push_back(tlsServer);
                        break;
                    case AF_INET6:
                        new6.push_back(tlsServer);
                        break;
                }
            }
        }
    }

    auto& out = existing6;
    out.splice(out.cend(), existing4);
    out.splice(out.cend(), new6);
    out.splice(out.cend(), new4);
    return out;
}

DnsTlsTransport::Response DnsTlsDispatcher::query(const std::list<DnsTlsServer>& tlsServers,
                                                  res_state statp, const Slice query,
                                                  const Slice ans, int* resplen) {
    const std::list<DnsTlsServer> servers(
            getOrderedAndUsableServerList(tlsServers, statp->netid, statp->_mark));

    if (servers.empty()) LOG(WARNING) << "No usable DnsTlsServers";

    DnsTlsTransport::Response code = DnsTlsTransport::Response::internal_error;
    int serverCount = 0;
    for (const auto& server : servers) {
        DnsQueryEvent* dnsQueryEvent =
                statp->event->mutable_dns_query_events()->add_dns_query_event();

        bool connectTriggered = false;
        Stopwatch queryStopwatch;
        code = this->query(server, statp->netid, statp->_mark, query, ans, resplen,
                           &connectTriggered);

        dnsQueryEvent->set_latency_micros(saturate_cast<int32_t>(queryStopwatch.timeTakenUs()));
        dnsQueryEvent->set_dns_server_index(serverCount++);
        dnsQueryEvent->set_ip_version(ipFamilyToIPVersion(server.ss.ss_family));
        dnsQueryEvent->set_protocol(PROTO_DOT);
        dnsQueryEvent->set_type(getQueryType(query.base(), query.size()));
        dnsQueryEvent->set_connected(connectTriggered);

        switch (code) {
            // These response codes are valid responses and not expected to
            // change if another server is queried.
            case DnsTlsTransport::Response::success:
                dnsQueryEvent->set_rcode(
                        static_cast<NsRcode>(reinterpret_cast<HEADER*>(ans.base())->rcode));
                resolv_stats_add(statp->netid, IPSockAddr::toIPSockAddr(server.ss), dnsQueryEvent);
                return code;
            case DnsTlsTransport::Response::limit_error:
                dnsQueryEvent->set_rcode(NS_R_INTERNAL_ERROR);
                resolv_stats_add(statp->netid, IPSockAddr::toIPSockAddr(server.ss), dnsQueryEvent);
                return code;
            // These response codes might differ when trying other servers, so
            // keep iterating to see if we can get a different (better) result.
            case DnsTlsTransport::Response::network_error:
                // Sync from res_tls_send in res_send.cpp
                dnsQueryEvent->set_rcode(NS_R_TIMEOUT);
                resolv_stats_add(statp->netid, IPSockAddr::toIPSockAddr(server.ss), dnsQueryEvent);
                break;
            case DnsTlsTransport::Response::internal_error:
                dnsQueryEvent->set_rcode(NS_R_INTERNAL_ERROR);
                resolv_stats_add(statp->netid, IPSockAddr::toIPSockAddr(server.ss), dnsQueryEvent);
                break;
            // No "default" statement.
        }
    }

    return code;
}

DnsTlsTransport::Response DnsTlsDispatcher::query(const DnsTlsServer& server, unsigned netId,
                                                  unsigned mark, const Slice query, const Slice ans,
                                                  int* resplen, bool* connectTriggered) {
    // TODO: This can cause the resolver to create multiple connections to the same DoT server
    // merely due to different mark, such as the bit explicitlySelected unset.
    // See if we can save them and just create one connection for one DoT server.
    const Key key = std::make_pair(mark, server);
    Transport* xport;
    {
        std::lock_guard guard(sLock);
        if (xport = getTransport(key); xport == nullptr) {
            xport = addTransport(server, mark, netId);
        }
        ++xport->useCount;
    }

    // Don't call this function and hold sLock at the same time because of the following reason:
    // TLS handshake requires a lock which is also needed by this function, if the handshake gets
    // stuck, this function also gets blocked.
    const int connectCounter = xport->transport.getConnectCounter();

    const auto& result = queryInternal(*xport, query);
    *connectTriggered = (xport->transport.getConnectCounter() > connectCounter);

    DnsTlsTransport::Response code = result.code;
    if (code == DnsTlsTransport::Response::success) {
        if (result.response.size() > ans.size()) {
            LOG(VERBOSE) << "Response too large: " << result.response.size() << " > " << ans.size();
            code = DnsTlsTransport::Response::limit_error;
        } else {
            LOG(VERBOSE) << "Got response successfully";
            *resplen = result.response.size();
            netdutils::copy(ans, netdutils::makeSlice(result.response));
        }
    } else {
        LOG(VERBOSE) << "Query failed: " << (unsigned int)code;
    }

    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard guard(sLock);
        --xport->useCount;
        xport->lastUsed = now;

        // DoT revalidation specific feature.
        if (xport->checkRevalidationNecessary(code)) {
            // Even if the revalidation passes, it doesn't guarantee that DoT queries
            // to the xport can stop failing because revalidation creates a new connection
            // to probe while the xport still uses an existing connection. So far, there isn't
            // a feasible way to force the xport to disconnect the connection. If the case
            // happens, the xport will be marked as unusable and DoT queries won't be sent to
            // it anymore. Eventually, after IDLE_TIMEOUT, the xport will be destroyed, and
            // a new xport will be created.
            const auto result = PrivateDnsConfiguration::getInstance().requestValidation(
                    netId, PrivateDnsConfiguration::ServerIdentity{server}, mark);
            LOG(WARNING) << "Requested validation for " << server.toIpString() << " with mark 0x"
                         << std::hex << mark << ", "
                         << (result.ok() ? "succeeded" : "failed: " + result.error().message());
        }

        cleanup(now);
    }
    return code;
}

void DnsTlsDispatcher::forceCleanup(unsigned netId) {
    std::lock_guard guard(sLock);
    forceCleanupLocked(netId);
}

DnsTlsTransport::Result DnsTlsDispatcher::queryInternal(Transport& xport,
                                                        const netdutils::Slice query) {
    LOG(VERBOSE) << "Sending query of length " << query.size();

    // If dot_async_handshake is not set, the call might block in some cases; otherwise,
    // the call should return very soon.
    auto res = xport.transport.query(query);
    LOG(VERBOSE) << "Awaiting response";

    if (xport.timeout().count() == -1) {
        // Infinite timeout.
        return res.get();
    }

    const auto status = res.wait_for(xport.timeout());
    if (status == std::future_status::timeout) {
        // TODO(b/186613628): notify the Transport to remove this query.
        LOG(WARNING) << "DoT query timed out after " << xport.timeout().count() << " ms";
        return DnsTlsTransport::Result{
                .code = DnsTlsTransport::Response::network_error,
                .response = {},
        };
    }

    return res.get();
}

// This timeout effectively controls how long to keep SSL session tickets.
static constexpr std::chrono::minutes IDLE_TIMEOUT(5);
void DnsTlsDispatcher::cleanup(std::chrono::time_point<std::chrono::steady_clock> now) {
    // To avoid scanning mStore after every query, return early if a cleanup has been
    // performed recently.
    if (now - mLastCleanup < IDLE_TIMEOUT) {
        return;
    }
    for (auto it = mStore.begin(); it != mStore.end();) {
        auto& s = it->second;
        if (s->useCount == 0 && now - s->lastUsed > IDLE_TIMEOUT) {
            it = mStore.erase(it);
        } else {
            ++it;
        }
    }
    mLastCleanup = now;
}

// TODO: unify forceCleanupLocked() and cleanup().
void DnsTlsDispatcher::forceCleanupLocked(unsigned netId) {
    for (auto it = mStore.begin(); it != mStore.end();) {
        auto& s = it->second;
        if (s->useCount == 0 && s->mNetId == netId) {
            it = mStore.erase(it);
        } else {
            ++it;
        }
    }
}

DnsTlsDispatcher::Transport* DnsTlsDispatcher::addTransport(const DnsTlsServer& server,
                                                            unsigned mark, unsigned netId) {
    const Key key = std::make_pair(mark, server);
    Transport* ret = getTransport(key);
    if (ret != nullptr) return ret;

    const Experiments* const instance = Experiments::getInstance();
    int triggerThr =
            instance->getFlag("dot_revalidation_threshold", Transport::kDotRevalidationThreshold);
    int unusableThr = instance->getFlag("dot_xport_unusable_threshold",
                                        Transport::kDotXportUnusableThreshold);
    int queryTimeout = instance->getFlag("dot_query_timeout_ms", Transport::kDotQueryTimeoutMs);

    // Check and adjust the parameters if they are improperly set.
    bool revalidationEnabled = false;
    const bool isForOpportunisticMode = server.name.empty();
    if (triggerThr > 0 && unusableThr > 0 && isForOpportunisticMode) {
        revalidationEnabled = true;
    } else {
        triggerThr = -1;
        unusableThr = -1;
    }
    if (queryTimeout < 0) {
        queryTimeout = -1;
    } else if (queryTimeout < 1000) {
        queryTimeout = 1000;
    }

    ret = new Transport(server, mark, netId, mFactory.get(), revalidationEnabled, triggerThr,
                        unusableThr, queryTimeout);
    LOG(VERBOSE) << "Transport is initialized with { " << triggerThr << ", " << unusableThr << ", "
               << queryTimeout << "ms }"
               << " for server { " << server.toIpString() << "/" << server.name << " }";

    mStore[key].reset(ret);

    return ret;
}

DnsTlsDispatcher::Transport* DnsTlsDispatcher::getTransport(const Key& key) {
    auto it = mStore.find(key);
    return (it == mStore.end() ? nullptr : it->second.get());
}

bool DnsTlsDispatcher::Transport::checkRevalidationNecessary(DnsTlsTransport::Response code) {
    if (!revalidationEnabled) return false;

    if (code == DnsTlsTransport::Response::network_error) {
        continuousfailureCount++;
    } else {
        continuousfailureCount = 0;
    }

    // triggerThreshold must be greater than 0 because the value of revalidationEnabled is true.
    if (usable() && continuousfailureCount == triggerThreshold) {
        return true;
    }
    return false;
}

bool DnsTlsDispatcher::Transport::usable() const {
    if (!revalidationEnabled) return true;

    return continuousfailureCount < unusableThreshold;
}

}  // end of namespace net
}  // end of namespace android
