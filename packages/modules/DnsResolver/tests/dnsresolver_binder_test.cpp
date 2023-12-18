/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <netdb.h>

#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include <aidl/android/net/IDnsResolver.h>
#include <android-base/file.h>
#include <android-base/format.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <netdutils/Stopwatch.h>

#include "dns_metrics_listener/base_metrics_listener.h"
#include "dns_metrics_listener/test_metrics.h"
#include "unsolicited_listener/unsolicited_event_listener.h"

#include "ResolverStats.h"
#include "dns_responder.h"
#include "dns_responder_client_ndk.h"

using aidl::android::net::IDnsResolver;
using aidl::android::net::ResolverHostsParcel;
using aidl::android::net::ResolverOptionsParcel;
using aidl::android::net::ResolverParamsParcel;
using aidl::android::net::metrics::INetdEventListener;
using android::base::ReadFdToString;
using android::base::StringPrintf;
using android::base::unique_fd;
using android::net::ResolverStats;
using android::net::metrics::TestOnDnsEvent;
using android::net::resolv::aidl::UnsolicitedEventListener;
using android::netdutils::Stopwatch;

// TODO: make this dynamic and stop depending on implementation details.
// Sync from TEST_NETID in dns_responder_client.cpp as resolv_integration_test.cpp does.
constexpr int TEST_NETID = 30;

namespace {

std::vector<std::string> dumpService(ndk::SpAIBinder binder) {
    unique_fd localFd, remoteFd;
    bool success = Pipe(&localFd, &remoteFd);
    EXPECT_TRUE(success) << "Failed to open pipe for dumping: " << strerror(errno);
    if (!success) return {};

    // dump() blocks until another thread has consumed all its output.
    std::thread dumpThread = std::thread([binder, remoteFd{std::move(remoteFd)}]() {
        EXPECT_EQ(STATUS_OK, AIBinder_dump(binder.get(), remoteFd, nullptr, 0));
    });

    std::string dumpContent;

    EXPECT_TRUE(ReadFdToString(localFd.get(), &dumpContent))
            << "Error during dump: " << strerror(errno);
    dumpThread.join();

    std::stringstream dumpStream(std::move(dumpContent));
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(dumpStream, line)) {
        lines.push_back(std::move(line));
    }

    return lines;
}

}  // namespace

class DnsResolverBinderTest : public ::testing::Test {
  public:
    DnsResolverBinderTest() {
        ndk::SpAIBinder resolvBinder = ndk::SpAIBinder(AServiceManager_getService("dnsresolver"));

        mDnsResolver = IDnsResolver::fromBinder(resolvBinder);
        // This could happen when the test isn't running as root, or if netd isn't running.
        assert(nullptr != mDnsResolver.get());
        // Create cache for test
        mDnsResolver->createNetworkCache(TEST_NETID);
    }

    ~DnsResolverBinderTest() {
        expectLog();
        // Destroy cache for test
        mDnsResolver->destroyNetworkCache(TEST_NETID);
    }

  protected:
    void expectLog() {
        ndk::SpAIBinder netdBinder = ndk::SpAIBinder(AServiceManager_getService("netd"));
        // This could happen when the test isn't running as root, or if netd isn't running.
        assert(nullptr != netdBinder.get());
        // Send the service dump request to netd.
        std::vector<std::string> lines = dumpService(netdBinder);

        // Basic regexp to match dump output lines. Matches the beginning and end of the line, and
        // puts the output of the command itself into the first match group.
        // Example: "      11-05 00:23:39.481 myCommand(args) <2.02ms>".
        // Accept any number of the leading space.
        const std::basic_regex lineRegex(
                "^\\s*[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}[.][0-9]{3} "
                "(.*)"
                " <[0-9]+[.][0-9]{2}ms>$");

        // For each element of testdata, check that the expected output appears in the dump output.
        // If not, fail the test and use hintRegex to print similar lines to assist in debugging.
        for (const auto& td : mExpectedLogData) {
            const bool found =
                    std::any_of(lines.begin(), lines.end(), [&](const std::string& line) {
                        std::smatch match;
                        if (!std::regex_match(line, match, lineRegex)) return false;
                        if (match.size() != 2) return false;

                        // The binder_to_string format is changed from S that will add "(null)" to
                        // the log on method's argument if binder object is null. But Q and R don't
                        // have this format in log. So to make register null listener tests are
                        // compatible from all version, just remove the "(null)" argument from
                        // output logs if existed.
                        const std::string output = android::base::StringReplace(
                                match[1].str(), "(null)", "", /*all=*/true);
                        return output == td.output;
                    });
            EXPECT_TRUE(found) << "Didn't find line '" << td.output << "' in dumpsys output.";
            if (found) continue;
            std::cerr << "Similar lines" << std::endl;
            for (const auto& line : lines) {
                if (std::regex_search(line, std::basic_regex(td.hintRegex))) {
                    std::cerr << line << std::endl;
                }
            }
        }

        // The log output is different between R and S, either one is fine for the
        // test to avoid test compatible issue.
        // TODO: Remove after S.
        for (const auto& td : mExpectedLogDataWithPacel) {
            const bool found =
                    std::any_of(lines.begin(), lines.end(), [&](const std::string& line) {
                        std::smatch match;
                        if (!std::regex_match(line, match, lineRegex)) return false;
                        return (match.size() == 2) && ((match[1].str() == td.withPacel.output) ||
                                                       (match[1].str() == td.withoutPacel.output));
                    });
            EXPECT_TRUE(found) << fmt::format("Didn't find line '{}' or '{}' in dumpsys output.",
                                              td.withPacel.output, td.withoutPacel.output);
            if (found) continue;
            std::cerr << "Similar lines" << std::endl;
            for (const auto& line : lines) {
                if (std::regex_search(line, std::basic_regex(td.withPacel.hintRegex))) {
                    std::cerr << line << std::endl;
                }
                if (std::regex_search(line, std::basic_regex(td.withoutPacel.hintRegex))) {
                    std::cerr << line << std::endl;
                }
            }
        }
    }

    struct LogData {
        // Expected contents of the dump command.
        const std::string output;
        // A regex that might be helpful in matching relevant lines in the output.
        // Used to make it easier to add test cases for this code.
        const std::string hintRegex;
    };

    // TODO: Remove this struct and below toString methods after S.
    struct PossibleLogData {
        LogData withPacel;
        LogData withoutPacel;
    };

    std::string toString(const std::vector<ResolverHostsParcel>& parms) {
        std::string o;
        const size_t size = parms.size();
        for (size_t i = 0; i < size; ++i) {
            o.append(fmt::format("ResolverHostsParcel{{ipAddr: {}, hostName: {}}}", parms[i].ipAddr,
                                 parms[i].hostName));
            if (i + 1 < size) o.append(", ");
        }
        return o;
    }

    std::string toString(const std::optional<ResolverOptionsParcel>& parms) {
        if (!parms.has_value()) return "(null)";
        return fmt::format("ResolverOptionsParcel{{hosts: [{}], tcMode: {}, enforceDnsUid: {}}}",
                           toString(parms->hosts), parms->tcMode, parms->enforceDnsUid);
    }

    std::string toString(const ResolverParamsParcel& parms) {
        return fmt::format(
                "ResolverParamsParcel{{netId: {}, sampleValiditySeconds: {}, successThreshold: {}, "
                "minSamples: {}, "
                "maxSamples: {}, baseTimeoutMsec: {}, retryCount: {}, "
                "servers: [{}], domains: [{}], "
                "tlsName: {}, tlsServers: [{}], "
                "tlsFingerprints: [{}], "
                "caCertificate: {}, tlsConnectTimeoutMs: {}, "
                "resolverOptions: {}, transportTypes: [{}]}}",
                parms.netId, parms.sampleValiditySeconds, parms.successThreshold, parms.minSamples,
                parms.maxSamples, parms.baseTimeoutMsec, parms.retryCount,
                fmt::join(parms.servers, ", "), fmt::join(parms.domains, ", "), parms.tlsName,
                fmt::join(parms.tlsServers, ", "), fmt::join(parms.tlsFingerprints, ", "),
                android::base::StringReplace(parms.caCertificate, "\n", "\\n", true),
                parms.tlsConnectTimeoutMs, toString(parms.resolverOptions),
                fmt::join(parms.transportTypes, ", "));
    }

    PossibleLogData toSetResolverConfigurationLogData(const ResolverParamsParcel& parms,
                                                      int returnCode = 0) {
        std::string outputWithParcel = "setResolverConfiguration(" + toString(parms) + ")";
        std::string hintRegexWithParcel = fmt::format("setResolverConfiguration.*{}", parms.netId);

        std::string outputWithoutParcel = "setResolverConfiguration()";
        std::string hintRegexWithoutParcel = "setResolverConfiguration";
        if (returnCode != 0) {
            outputWithParcel.append(fmt::format(" -> ServiceSpecificException({}, \"{}\")",
                                                returnCode, strerror(returnCode)));
            hintRegexWithParcel.append(fmt::format(".*{}", returnCode));
            outputWithoutParcel.append(fmt::format(" -> ServiceSpecificException({}, \"{}\")",
                                                   returnCode, strerror(returnCode)));
            hintRegexWithoutParcel.append(fmt::format(".*{}", returnCode));
        }
        return {{std::move(outputWithParcel), std::move(hintRegexWithParcel)},
                {std::move(outputWithoutParcel), std::move(hintRegexWithoutParcel)}};
    }

    std::shared_ptr<aidl::android::net::IDnsResolver> mDnsResolver;
    std::vector<LogData> mExpectedLogData;
    std::vector<PossibleLogData> mExpectedLogDataWithPacel;
};

class TimedOperation : public Stopwatch {
  public:
    explicit TimedOperation(const std::string& name) : mName(name) {}
    virtual ~TimedOperation() {
        std::cerr << "    " << mName << ": " << timeTakenUs() << "us" << std::endl;
    }

  private:
    std::string mName;
};

TEST_F(DnsResolverBinderTest, IsAlive) {
    TimedOperation t("isAlive RPC");
    bool isAlive = false;
    mDnsResolver->isAlive(&isAlive);
    ASSERT_TRUE(isAlive);
}

TEST_F(DnsResolverBinderTest, RegisterEventListener_NullListener) {
    ::ndk::ScopedAStatus status = mDnsResolver->registerEventListener(nullptr);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(EINVAL, status.getServiceSpecificError());
    mExpectedLogData.push_back(
            {"registerEventListener() -> ServiceSpecificException(22, \"Invalid argument\")",
             "registerEventListener.*22"});
}

TEST_F(DnsResolverBinderTest, RegisterEventListener_DuplicateSubscription) {
    class FakeListener : public android::net::metrics::BaseMetricsListener {};

    // Expect to subscribe successfully.
    std::shared_ptr<FakeListener> fakeListener = ndk::SharedRefBase::make<FakeListener>();
    ::ndk::ScopedAStatus status = mDnsResolver->registerEventListener(fakeListener);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    mExpectedLogData.push_back({"registerEventListener()", "registerEventListener.*"});

    // Expect to subscribe failed with registered listener instance.
    status = mDnsResolver->registerEventListener(fakeListener);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(EEXIST, status.getServiceSpecificError());
    mExpectedLogData.push_back(
            {"registerEventListener() -> ServiceSpecificException(17, \"File exists\")",
             "registerEventListener.*17"});
}

TEST_F(DnsResolverBinderTest, RegisterUnsolicitedEventListener_NullListener) {
    ::ndk::ScopedAStatus status = mDnsResolver->registerUnsolicitedEventListener(nullptr);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(EINVAL, status.getServiceSpecificError());
    mExpectedLogData.push_back(
            {"registerUnsolicitedEventListener() -> ServiceSpecificException(22, \"Invalid "
             "argument\")",
             "registerUnsolicitedEventListener.*22"});
}

TEST_F(DnsResolverBinderTest, RegisterUnsolicitedEventListener_DuplicateSubscription) {
    // Expect to subscribe successfully.
    std::shared_ptr<UnsolicitedEventListener> listener =
            ndk::SharedRefBase::make<UnsolicitedEventListener>(TEST_NETID);
    ::ndk::ScopedAStatus status = mDnsResolver->registerUnsolicitedEventListener(listener);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    mExpectedLogData.push_back(
            {"registerUnsolicitedEventListener()", "registerUnsolicitedEventListener.*"});

    // Expect to subscribe failed with registered listener instance.
    status = mDnsResolver->registerUnsolicitedEventListener(listener);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(EEXIST, status.getServiceSpecificError());
    mExpectedLogData.push_back(
            {"registerUnsolicitedEventListener() -> ServiceSpecificException(17, \"File exists\")",
             "registerUnsolicitedEventListener.*17"});
}

// TODO: Move this test to resolv_integration_test.cpp
TEST_F(DnsResolverBinderTest, RegisterEventListener_onDnsEvent) {
    // The test configs are used to trigger expected events. The expected results are defined in
    // expectedResults.
    static const struct TestConfig {
        std::string hostname;
        int returnCode;
    } testConfigs[] = {
            {"hi", 0 /*success*/},
            {"nonexistent", EAI_NODATA},
    };

    // The expected results define expected event content for test verification.
    static const std::vector<TestOnDnsEvent::TestResult> expectedResults = {
            {TEST_NETID, INetdEventListener::EVENT_GETADDRINFO, 0 /*success*/, 1, "hi", "1.2.3.4"},
            {TEST_NETID, INetdEventListener::EVENT_GETADDRINFO, EAI_NODATA, 0, "nonexistent", ""},
    };

    // Start the Binder thread pool.
    // TODO: Consider doing this once if there has another event listener unit test.
    ABinderProcess_startThreadPool();

    // Setup network.
    // TODO: Setup device configuration and DNS responser server as resolver test does.
    // Currently, leave DNS related configuration in this test because only it needs DNS
    // client-server testing environment.
    DnsResponderClient dnsClient;
    dnsClient.SetUp();

    // Setup DNS responder server.
    constexpr char listen_addr[] = "127.0.0.3";
    constexpr char listen_srv[] = "53";
    test::DNSResponder dns(listen_addr, listen_srv, ns_rcode::ns_r_servfail);
    dns.addMapping("hi.example.com.", ns_type::ns_t_a, "1.2.3.4");
    ASSERT_TRUE(dns.startServer());

    // Setup DNS configuration.
    const std::vector<std::string> test_servers = {listen_addr};
    std::vector<std::string> test_domains = {"example.com"};
    std::vector<int> test_params = {300 /*sample_validity*/, 25 /*success_threshold*/,
                                    8 /*min_samples*/, 8 /*max_samples*/};

    ASSERT_TRUE(dnsClient.SetResolversForNetwork(test_servers, test_domains, test_params));
    dns.clearQueries();

    // Register event listener.
    std::shared_ptr<TestOnDnsEvent> testOnDnsEvent =
            ndk::SharedRefBase::make<TestOnDnsEvent>(expectedResults);
    ::ndk::ScopedAStatus status = mDnsResolver->registerEventListener(testOnDnsEvent);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    mExpectedLogData.push_back({"registerEventListener()", "registerEventListener.*"});

    // DNS queries.
    // Once all expected events of expectedResults are received by the listener, the unit test will
    // be notified. Otherwise, notified with a timeout expired failure.
    auto& cv = testOnDnsEvent->getCv();
    auto& cvMutex = testOnDnsEvent->getCvMutex();
    {
        std::unique_lock lock(cvMutex);

        for (const auto& config : testConfigs) {
            SCOPED_TRACE(config.hostname);

            addrinfo* result = nullptr;
            addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_DGRAM};
            int status = getaddrinfo(config.hostname.c_str(), nullptr, &hints, &result);
            EXPECT_EQ(config.returnCode, status);

            if (result) freeaddrinfo(result);
        }

        // Wait for receiving expected events.
        EXPECT_EQ(std::cv_status::no_timeout, cv.wait_for(lock, std::chrono::seconds(2)));
    }

    // Verify that all testcases are passed.
    EXPECT_TRUE(testOnDnsEvent->isVerified());

    dnsClient.TearDown();
}

// TODO: Need to test more than one server cases.
TEST_F(DnsResolverBinderTest, SetResolverConfiguration_Tls) {
    const std::vector<std::string> LOCALLY_ASSIGNED_DNS{"8.8.8.8", "2001:4860:4860::8888"};
    static const std::vector<std::string> valid_v4_addr = {"192.0.2.1"};
    static const std::vector<std::string> valid_v6_addr = {"2001:db8::2"};
    static const std::vector<std::string> invalid_v4_addr = {"192.0.*.5"};
    static const std::vector<std::string> invalid_v6_addr = {"2001:dg8::6"};
    constexpr char valid_tls_name[] = "example.com";
    std::vector<int> test_params = {300, 25, 8, 8};
    // We enumerate valid and invalid v4/v6 address, and several different TLS names
    // to be the input data and verify the binder status.
    static const struct TestData {
        const std::vector<std::string> servers;
        const std::string tlsName;
        const int expectedReturnCode;
    } kTlsTestData[] = {
            {valid_v4_addr, valid_tls_name, 0},
            {valid_v4_addr, "host.com", 0},
            {valid_v4_addr, "@@@@", 0},
            {valid_v4_addr, "", 0},
            {valid_v6_addr, valid_tls_name, 0},
            {valid_v6_addr, "host.com", 0},
            {valid_v6_addr, "@@@@", 0},
            {valid_v6_addr, "", 0},
            {invalid_v4_addr, valid_tls_name, EINVAL},
            {invalid_v4_addr, "host.com", EINVAL},
            {invalid_v4_addr, "@@@@", EINVAL},
            {invalid_v4_addr, "", EINVAL},
            {invalid_v6_addr, valid_tls_name, EINVAL},
            {invalid_v6_addr, "host.com", EINVAL},
            {invalid_v6_addr, "@@@@", EINVAL},
            {invalid_v6_addr, "", EINVAL},
            {{}, "", 0},
            {{""}, "", EINVAL},
    };

    for (size_t i = 0; i < std::size(kTlsTestData); i++) {
        const auto& td = kTlsTestData[i];

        const auto resolverParams = DnsResponderClient::makeResolverParamsParcel(
                TEST_NETID, test_params, LOCALLY_ASSIGNED_DNS, {}, td.tlsName, td.servers);
        ::ndk::ScopedAStatus status = mDnsResolver->setResolverConfiguration(resolverParams);

        if (td.expectedReturnCode == 0) {
            SCOPED_TRACE(StringPrintf("test case %zu should have passed", i));
            SCOPED_TRACE(status.getMessage());
            EXPECT_EQ(0, status.getServiceSpecificError());
            mExpectedLogDataWithPacel.push_back(toSetResolverConfigurationLogData(resolverParams));
        } else {
            SCOPED_TRACE(StringPrintf("test case %zu should have failed", i));
            EXPECT_EQ(EX_SERVICE_SPECIFIC, status.getExceptionCode());
            EXPECT_EQ(td.expectedReturnCode, status.getServiceSpecificError());
            mExpectedLogDataWithPacel.push_back(
                    toSetResolverConfigurationLogData(resolverParams, td.expectedReturnCode));
        }
    }
}

TEST_F(DnsResolverBinderTest, SetResolverConfiguration_TransportTypes) {
    using ::testing::HasSubstr;
    auto resolverParams = DnsResponderClient::GetDefaultResolverParamsParcel();
    resolverParams.transportTypes = {IDnsResolver::TRANSPORT_WIFI, IDnsResolver::TRANSPORT_VPN};
    ::ndk::ScopedAStatus status = mDnsResolver->setResolverConfiguration(resolverParams);
    EXPECT_TRUE(status.isOk()) << status.getMessage();
    mExpectedLogDataWithPacel.push_back(toSetResolverConfigurationLogData(resolverParams));
    // TODO: Find a way to fix a potential deadlock here if it's larger than pipe buffer
    // size(65535).
    android::base::unique_fd writeFd, readFd;
    EXPECT_TRUE(Pipe(&readFd, &writeFd));
    EXPECT_EQ(mDnsResolver->dump(writeFd.get(), nullptr, 0), 0);
    writeFd.reset();
    std::string str;
    ASSERT_TRUE(ReadFdToString(readFd, &str)) << strerror(errno);
    EXPECT_THAT(str, HasSubstr("WIFI_VPN"));
}

TEST_F(DnsResolverBinderTest, SetResolverConfiguration_TransportTypes_Default) {
    using ::testing::HasSubstr;
    auto resolverParams = DnsResponderClient::GetDefaultResolverParamsParcel();
    ::ndk::ScopedAStatus status = mDnsResolver->setResolverConfiguration(resolverParams);
    EXPECT_TRUE(status.isOk()) << status.getMessage();
    mExpectedLogDataWithPacel.push_back(toSetResolverConfigurationLogData(resolverParams));
    android::base::unique_fd writeFd, readFd;
    EXPECT_TRUE(Pipe(&readFd, &writeFd));
    EXPECT_EQ(mDnsResolver->dump(writeFd.get(), nullptr, 0), 0);
    writeFd.reset();
    std::string str;
    ASSERT_TRUE(ReadFdToString(readFd, &str)) << strerror(errno);
    EXPECT_THAT(str, HasSubstr("UNKNOWN"));
}

TEST_F(DnsResolverBinderTest, GetResolverInfo) {
    std::vector<std::string> servers = {"127.0.0.1", "127.0.0.2"};
    std::vector<std::string> domains = {"example.com"};
    std::vector<int> testParams = {
            300,     // sample validity in seconds
            25,      // success threshod in percent
            8,   8,  // {MIN,MAX}_SAMPLES
            100,     // BASE_TIMEOUT_MSEC
            3,       // retry count
    };
    const auto resolverParams = DnsResponderClient::makeResolverParamsParcel(
            TEST_NETID, testParams, servers, domains, "", {});
    ::ndk::ScopedAStatus status = mDnsResolver->setResolverConfiguration(resolverParams);
    EXPECT_TRUE(status.isOk()) << status.getMessage();
    mExpectedLogDataWithPacel.push_back(toSetResolverConfigurationLogData(resolverParams));

    std::vector<std::string> res_servers;
    std::vector<std::string> res_domains;
    std::vector<std::string> res_tls_servers;
    std::vector<int32_t> params32;
    std::vector<int32_t> stats32;
    std::vector<int32_t> wait_for_pending_req_timeout_count32{0};
    status = mDnsResolver->getResolverInfo(TEST_NETID, &res_servers, &res_domains, &res_tls_servers,
                                           &params32, &stats32,
                                           &wait_for_pending_req_timeout_count32);

    EXPECT_TRUE(status.isOk()) << status.getMessage();
    EXPECT_EQ(servers.size(), res_servers.size());
    EXPECT_EQ(domains.size(), res_domains.size());
    EXPECT_EQ(0U, res_tls_servers.size());
    ASSERT_EQ(static_cast<size_t>(IDnsResolver::RESOLVER_PARAMS_COUNT), testParams.size());
    EXPECT_EQ(testParams[IDnsResolver::RESOLVER_PARAMS_SAMPLE_VALIDITY],
              params32[IDnsResolver::RESOLVER_PARAMS_SAMPLE_VALIDITY]);
    EXPECT_EQ(testParams[IDnsResolver::RESOLVER_PARAMS_SUCCESS_THRESHOLD],
              params32[IDnsResolver::RESOLVER_PARAMS_SUCCESS_THRESHOLD]);
    EXPECT_EQ(testParams[IDnsResolver::RESOLVER_PARAMS_MIN_SAMPLES],
              params32[IDnsResolver::RESOLVER_PARAMS_MIN_SAMPLES]);
    EXPECT_EQ(testParams[IDnsResolver::RESOLVER_PARAMS_MAX_SAMPLES],
              params32[IDnsResolver::RESOLVER_PARAMS_MAX_SAMPLES]);
    EXPECT_EQ(testParams[IDnsResolver::RESOLVER_PARAMS_BASE_TIMEOUT_MSEC],
              params32[IDnsResolver::RESOLVER_PARAMS_BASE_TIMEOUT_MSEC]);
    EXPECT_EQ(testParams[IDnsResolver::RESOLVER_PARAMS_RETRY_COUNT],
              params32[IDnsResolver::RESOLVER_PARAMS_RETRY_COUNT]);

    std::vector<ResolverStats> stats;
    ResolverStats::decodeAll(stats32, &stats);

    EXPECT_EQ(servers.size(), stats.size());

    EXPECT_THAT(res_servers, testing::UnorderedElementsAreArray(servers));
    EXPECT_THAT(res_domains, testing::UnorderedElementsAreArray(domains));
}

TEST_F(DnsResolverBinderTest, CreateDestroyNetworkCache) {
    // Must not be the same as TEST_NETID
    const int ANOTHER_TEST_NETID = TEST_NETID + 1;

    // Create a new network cache.
    EXPECT_TRUE(mDnsResolver->createNetworkCache(ANOTHER_TEST_NETID).isOk());
    mExpectedLogData.push_back({"createNetworkCache(31)", "createNetworkCache.*31"});

    // create it again, expect a EEXIST.
    EXPECT_EQ(EEXIST,
              mDnsResolver->createNetworkCache(ANOTHER_TEST_NETID).getServiceSpecificError());
    mExpectedLogData.push_back(
            {"createNetworkCache(31) -> ServiceSpecificException(17, \"File exists\")",
             "createNetworkCache.*31.*17"});

    // destroy it.
    EXPECT_TRUE(mDnsResolver->destroyNetworkCache(ANOTHER_TEST_NETID).isOk());
    mExpectedLogData.push_back({"destroyNetworkCache(31)", "destroyNetworkCache.*31"});

    // re-create it
    EXPECT_TRUE(mDnsResolver->createNetworkCache(ANOTHER_TEST_NETID).isOk());
    mExpectedLogData.push_back({"createNetworkCache(31)", "createNetworkCache.*31"});

    // destroy it.
    EXPECT_TRUE(mDnsResolver->destroyNetworkCache(ANOTHER_TEST_NETID).isOk());
    mExpectedLogData.push_back({"destroyNetworkCache(31)", "destroyNetworkCache.*31"});

    // re-destroy it
    EXPECT_TRUE(mDnsResolver->destroyNetworkCache(ANOTHER_TEST_NETID).isOk());
    mExpectedLogData.push_back({"destroyNetworkCache(31)", "destroyNetworkCache.*31"});
}

TEST_F(DnsResolverBinderTest, FlushNetworkCache) {
    SKIP_IF_REMOTE_VERSION_LESS_THAN(mDnsResolver.get(), 4);
    // cache has beed created in DnsResolverBinderTest constructor
    EXPECT_TRUE(mDnsResolver->flushNetworkCache(TEST_NETID).isOk());
    mExpectedLogData.push_back({"flushNetworkCache(30)", "destroyNetworkCache.*30"});
    EXPECT_EQ(ENONET, mDnsResolver->flushNetworkCache(-1).getServiceSpecificError());
    mExpectedLogData.push_back(
            {"flushNetworkCache(-1) -> ServiceSpecificException(64, \"Machine is not on the "
             "network\")",
             "flushNetworkCache.*-1.*64"});
}

TEST_F(DnsResolverBinderTest, setLogSeverity) {
    // Expect fail
    EXPECT_EQ(EINVAL, mDnsResolver->setLogSeverity(-1).getServiceSpecificError());
    mExpectedLogData.push_back(
            {"setLogSeverity(-1) -> ServiceSpecificException(22, \"Invalid argument\")",
             "flushNetworkCache.*-1.*22"});

    // Test set different log level
    EXPECT_TRUE(mDnsResolver->setLogSeverity(IDnsResolver::DNS_RESOLVER_LOG_VERBOSE).isOk());
    mExpectedLogData.push_back({"setLogSeverity(0)", "setLogSeverity.*0"});

    EXPECT_TRUE(mDnsResolver->setLogSeverity(IDnsResolver::DNS_RESOLVER_LOG_DEBUG).isOk());
    mExpectedLogData.push_back({"setLogSeverity(1)", "setLogSeverity.*1"});

    EXPECT_TRUE(mDnsResolver->setLogSeverity(IDnsResolver::DNS_RESOLVER_LOG_INFO).isOk());
    mExpectedLogData.push_back({"setLogSeverity(2)", "setLogSeverity.*2"});

    EXPECT_TRUE(mDnsResolver->setLogSeverity(IDnsResolver::DNS_RESOLVER_LOG_WARNING).isOk());
    mExpectedLogData.push_back({"setLogSeverity(3)", "setLogSeverity.*3"});

    EXPECT_TRUE(mDnsResolver->setLogSeverity(IDnsResolver::DNS_RESOLVER_LOG_ERROR).isOk());
    mExpectedLogData.push_back({"setLogSeverity(4)", "setLogSeverity.*4"});

    // Set back to default
    EXPECT_TRUE(mDnsResolver->setLogSeverity(IDnsResolver::DNS_RESOLVER_LOG_WARNING).isOk());
    mExpectedLogData.push_back({"setLogSeverity(3)", "setLogSeverity.*3"});
}

TEST_F(DnsResolverBinderTest, SetResolverOptions) {
    SKIP_IF_REMOTE_VERSION_LESS_THAN(mDnsResolver.get(), 9);
    ResolverOptionsParcel options;
    options.tcMode = 1;
    options.enforceDnsUid = true;
    EXPECT_TRUE(mDnsResolver->setResolverOptions(TEST_NETID, options).isOk());
    mExpectedLogData.push_back(
            {"setResolverOptions(30, " + toString(options) + ")", "setResolverOptions.*30"});
    EXPECT_EQ(ENONET, mDnsResolver->setResolverOptions(-1, options).getServiceSpecificError());
    mExpectedLogData.push_back({"setResolverOptions(-1, " + toString(options) +
                                        ") -> ServiceSpecificException(64, \"Machine is not on the "
                                        "network\")",
                                "setResolverOptions.*-1.*64"});
}
