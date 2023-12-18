/*
 * Copyright 2020 The Android Open Source Project
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

#include "LooperStub.h"
#include "MockDataProcessor.h"
#include "MockProcDiskStats.h"
#include "MockProcPidStat.h"
#include "MockProcStat.h"
#include "MockUidIoStats.h"
#include "ProcPidDir.h"
#include "ProcPidStat.h"
#include "ProcStat.h"
#include "UidIoStats.h"
#include "WatchdogPerfService.h"

#include <WatchdogProperties.sysprop.h>
#include <android-base/file.h>
#include <gmock/gmock.h>

#include <future>  // NOLINT(build/c++11)
#include <queue>
#include <string>
#include <vector>

namespace android {
namespace automotive {
namespace watchdog {

using ::android::sp;
using ::android::String16;
using ::android::wp;
using ::android::automotive::watchdog::internal::PowerCycle;
using ::android::automotive::watchdog::testing::LooperStub;
using ::android::base::Error;
using ::android::base::Result;
using ::testing::_;
using ::testing::DefaultValue;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::UnorderedElementsAreArray;

constexpr std::chrono::seconds kTestBoottimeCollectionInterval = 1s;
constexpr std::chrono::seconds kTestPeriodicCollectionInterval = 5s;
constexpr std::chrono::seconds kTestCustomCollectionInterval = 3s;
constexpr std::chrono::seconds kTestCustomCollectionDuration = 11s;
constexpr std::chrono::seconds kTestPeriodicMonitorInterval = 2s;

namespace internal {

class WatchdogPerfServicePeer {
public:
    explicit WatchdogPerfServicePeer(sp<WatchdogPerfService> service) : service(service) {}
    WatchdogPerfServicePeer() = delete;
    ~WatchdogPerfServicePeer() { service->terminate(); }

    void injectFakes() {
        looperStub = sp<LooperStub>::make();
        mockUidIoStats = sp<NiceMock<MockUidIoStats>>::make();
        mockProcDiskStats = sp<NiceMock<MockProcDiskStats>>::make();
        mockProcStat = sp<NiceMock<MockProcStat>>::make();
        mockProcPidStat = sp<NiceMock<MockProcPidStat>>::make();
        mockDataProcessor = sp<StrictMock<MockDataProcessor>>::make();

        {
            Mutex::Autolock lock(service->mMutex);
            service->mHandlerLooper = looperStub;
            service->mUidIoStats = mockUidIoStats;
            service->mProcDiskStats = mockProcDiskStats;
            service->mProcStat = mockProcStat;
            service->mProcPidStat = mockProcPidStat;
        }
        EXPECT_CALL(*mockDataProcessor, init()).Times(1);
        ASSERT_RESULT_OK(service->registerDataProcessor(mockDataProcessor));
    }

    Result<void> start() {
        if (auto ret = service->start(); !ret.ok()) {
            return ret;
        }
        Mutex::Autolock lock(service->mMutex);
        service->mBoottimeCollection.interval = kTestBoottimeCollectionInterval;
        service->mPeriodicCollection.interval = kTestPeriodicCollectionInterval;
        service->mPeriodicMonitor.interval = kTestPeriodicMonitorInterval;
        return {};
    }

    EventType getCurrCollectionEvent() {
        Mutex::Autolock lock(service->mMutex);
        return service->mCurrCollectionEvent;
    }

    std::future<void> joinCollectionThread() {
        return std::async([&]() {
            if (service->mCollectionThread.joinable()) {
                service->mCollectionThread.join();
            }
        });
    }

    void verifyAndClearExpectations() {
        Mock::VerifyAndClearExpectations(mockUidIoStats.get());
        Mock::VerifyAndClearExpectations(mockProcStat.get());
        Mock::VerifyAndClearExpectations(mockProcPidStat.get());
        Mock::VerifyAndClearExpectations(mockDataProcessor.get());
    }

    sp<WatchdogPerfService> service;
    // Below fields are populated only on injectFakes.
    sp<LooperStub> looperStub;
    sp<MockUidIoStats> mockUidIoStats;
    sp<MockProcDiskStats> mockProcDiskStats;
    sp<MockProcStat> mockProcStat;
    sp<MockProcPidStat> mockProcPidStat;
    sp<MockDataProcessor> mockDataProcessor;
};

}  // namespace internal

namespace {

void startPeriodicCollection(internal::WatchdogPerfServicePeer* servicePeer) {
    ASSERT_NO_FATAL_FAILURE(servicePeer->injectFakes());

    ASSERT_RESULT_OK(servicePeer->start());

    EXPECT_CALL(*servicePeer->mockUidIoStats, collect()).Times(2);
    EXPECT_CALL(*servicePeer->mockProcStat, collect()).Times(2);
    EXPECT_CALL(*servicePeer->mockProcPidStat, collect()).Times(2);
    EXPECT_CALL(*servicePeer->mockDataProcessor,
                onBoottimeCollection(_, wp<UidIoStats>(servicePeer->mockUidIoStats),
                                     wp<ProcStat>(servicePeer->mockProcStat),
                                     wp<ProcPidStat>(servicePeer->mockProcPidStat)))
            .Times(2);

    // Make sure the collection event changes from EventType::INIT to
    // EventType::BOOT_TIME_COLLECTION.
    ASSERT_RESULT_OK(servicePeer->looperStub->pollCache());

    // Mark boot complete.
    ASSERT_RESULT_OK(servicePeer->service->onBootFinished());

    // Process |SwitchMessage::END_BOOTTIME_COLLECTION| and switch to periodic collection.
    ASSERT_RESULT_OK(servicePeer->looperStub->pollCache());

    ASSERT_EQ(servicePeer->getCurrCollectionEvent(), EventType::PERIODIC_COLLECTION)
            << "Invalid collection event";

    servicePeer->verifyAndClearExpectations();
}

void skipPeriodicMonitorEvents(internal::WatchdogPerfServicePeer* servicePeer) {
    EXPECT_CALL(*servicePeer->mockDataProcessor, onPeriodicMonitor(_, _, _)).Times(2);
    ASSERT_RESULT_OK(servicePeer->looperStub->pollCache());
    ASSERT_RESULT_OK(servicePeer->looperStub->pollCache());
}

}  // namespace

TEST(WatchdogPerfServiceTest, TestServiceStartAndTerminate) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();
    sp<MockDataProcessor> mockDataProcessor = sp<MockDataProcessor>::make();

    EXPECT_CALL(*mockDataProcessor, init()).Times(1);

    ASSERT_RESULT_OK(service->registerDataProcessor(mockDataProcessor));
    ASSERT_RESULT_OK(service->start());
    ASSERT_TRUE(service->mCollectionThread.joinable()) << "Collection thread not created";
    ASSERT_FALSE(service->start().ok())
            << "No error returned when WatchdogPerfService was started more than once";
    ASSERT_TRUE(sysprop::boottimeCollectionInterval().has_value());
    ASSERT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                      service->mBoottimeCollection.interval)
                      .count(),
              sysprop::boottimeCollectionInterval().value());
    ASSERT_TRUE(sysprop::periodicCollectionInterval().has_value());
    ASSERT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                      service->mPeriodicCollection.interval)
                      .count(),
              sysprop::periodicCollectionInterval().value());

    service->terminate();
    ASSERT_FALSE(service->mCollectionThread.joinable()) << "Collection thread did not terminate";
}

TEST(WatchdogPerfServiceTest, TestValidCollectionSequence) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();

    internal::WatchdogPerfServicePeer servicePeer(service);
    ASSERT_NO_FATAL_FAILURE(servicePeer.injectFakes());

    ASSERT_RESULT_OK(servicePeer.start());

    wp<UidIoStats> uidIoStats(servicePeer.mockUidIoStats);
    wp<IProcDiskStatsInterface> procDiskStats(servicePeer.mockProcDiskStats);
    wp<ProcStat> procStat(servicePeer.mockProcStat);
    wp<ProcPidStat> procPidStat(servicePeer.mockProcPidStat);

    // #1 Boot-time collection
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onBoottimeCollection(_, uidIoStats, procStat, procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), 0)
            << "Boot-time collection didn't start immediately";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::BOOT_TIME_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #2 Boot-time collection
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onBoottimeCollection(_, uidIoStats, procStat, procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), kTestBoottimeCollectionInterval.count())
            << "Subsequent boot-time collection didn't happen at "
            << kTestBoottimeCollectionInterval.count() << " seconds interval";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::BOOT_TIME_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #3 Last boot-time collection
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onBoottimeCollection(_, uidIoStats, procStat, procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(service->onBootFinished());

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), 0)
            << "Last boot-time collection didn't happen immediately after receiving boot complete "
            << "notification";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::PERIODIC_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #4 Periodic monitor
    EXPECT_CALL(*servicePeer.mockProcDiskStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor, onPeriodicMonitor(_, procDiskStats, _)).Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), kTestPeriodicMonitorInterval.count())
            << "First periodic monitor didn't happen at " << kTestPeriodicMonitorInterval.count()
            << " seconds interval";
    servicePeer.verifyAndClearExpectations();

    // #5 Periodic monitor
    EXPECT_CALL(*servicePeer.mockProcDiskStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor, onPeriodicMonitor(_, procDiskStats, _)).Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), kTestPeriodicMonitorInterval.count())
            << "Second periodic monitor didn't happen at " << kTestPeriodicMonitorInterval.count()
            << " seconds interval";
    servicePeer.verifyAndClearExpectations();

    // #6 Periodic collection
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onPeriodicCollection(_, SystemState::NORMAL_MODE, uidIoStats, procStat,
                                     procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), 1)
            << "First periodic collection didn't happen at 1 second interval";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::PERIODIC_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #7 Custom collection
    Vector<String16> args;
    args.push_back(String16(kStartCustomCollectionFlag));
    args.push_back(String16(kIntervalFlag));
    args.push_back(String16(std::to_string(kTestCustomCollectionInterval.count()).c_str()));
    args.push_back(String16(kMaxDurationFlag));
    args.push_back(String16(std::to_string(kTestCustomCollectionDuration.count()).c_str()));

    ASSERT_RESULT_OK(service->onCustomCollection(-1, args));

    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onCustomCollection(_, SystemState::NORMAL_MODE, _, uidIoStats, procStat,
                                   procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), 0)
            << "Custom collection didn't start immediately";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::CUSTOM_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #8 Custom collection
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onCustomCollection(_, SystemState::NORMAL_MODE, _, uidIoStats, procStat,
                                   procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), kTestCustomCollectionInterval.count())
            << "Subsequent custom collection didn't happen at "
            << kTestCustomCollectionInterval.count() << " seconds interval";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::CUSTOM_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #9 End custom collection
    TemporaryFile customDump;
    {
        InSequence s;
        EXPECT_CALL(*servicePeer.mockDataProcessor, onCustomCollectionDump(customDump.fd)).Times(1);
        EXPECT_CALL(*servicePeer.mockDataProcessor, onCustomCollectionDump(-1)).Times(1);
    }

    args.clear();
    args.push_back(String16(kEndCustomCollectionFlag));
    ASSERT_RESULT_OK(service->onCustomCollection(customDump.fd, args));
    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::PERIODIC_COLLECTION)
            << "Invalid collection event";

    // #10 Switch to periodic collection
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onPeriodicCollection(_, SystemState::NORMAL_MODE, uidIoStats, procStat,
                                     procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), 0)
            << "Periodic collection didn't start immediately after ending custom collection";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::PERIODIC_COLLECTION)
            << "Invalid collection event";
    servicePeer.verifyAndClearExpectations();

    // #11 Periodic monitor.
    EXPECT_CALL(*servicePeer.mockProcDiskStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor, onPeriodicMonitor(_, procDiskStats, _)).Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), kTestPeriodicMonitorInterval.count());
    servicePeer.verifyAndClearExpectations();

    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);
}

TEST(WatchdogPerfServiceTest, TestCollectionTerminatesOnZeroEnabledCollectors) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();

    internal::WatchdogPerfServicePeer servicePeer(service);
    ASSERT_NO_FATAL_FAILURE(servicePeer.injectFakes());

    ASSERT_RESULT_OK(servicePeer.start());

    ON_CALL(*servicePeer.mockUidIoStats, enabled()).WillByDefault(Return(false));
    ON_CALL(*servicePeer.mockProcStat, enabled()).WillByDefault(Return(false));
    ON_CALL(*servicePeer.mockProcPidStat, enabled()).WillByDefault(Return(false));

    // Collection should terminate and call data processor's terminate method on error.
    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.joinCollectionThread().wait_for(1s), std::future_status::ready)
            << "Collection thread didn't terminate within 1 second.";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::TERMINATED);
}

TEST(WatchdogPerfServiceTest, TestCollectionTerminatesOnDataCollectorError) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();

    internal::WatchdogPerfServicePeer servicePeer(service);
    ASSERT_NO_FATAL_FAILURE(servicePeer.injectFakes());

    ASSERT_RESULT_OK(servicePeer.start());

    // Inject data collector error.
    Result<void> errorRes = Error() << "Failed to collect data";
    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).WillOnce(Return(errorRes));

    // Collection should terminate and call data processor's terminate method on error.
    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.joinCollectionThread().wait_for(1s), std::future_status::ready)
            << "Collection thread didn't terminate within 1 second.";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::TERMINATED);
}

TEST(WatchdogPerfServiceTest, TestCollectionTerminatesOnDataProcessorError) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();

    internal::WatchdogPerfServicePeer servicePeer(service);
    ASSERT_NO_FATAL_FAILURE(servicePeer.injectFakes());

    EXPECT_CALL(*servicePeer.mockDataProcessor, name()).Times(1);

    ASSERT_RESULT_OK(servicePeer.start());

    // Inject data processor error.
    Result<void> errorRes = Error() << "Failed to process data";
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onBoottimeCollection(_, wp<UidIoStats>(servicePeer.mockUidIoStats),
                                     wp<ProcStat>(servicePeer.mockProcStat),
                                     wp<ProcPidStat>(servicePeer.mockProcPidStat)))
            .WillOnce(Return(errorRes));

    // Collection should terminate and call data processor's terminate method on error.
    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.joinCollectionThread().wait_for(1s), std::future_status::ready)
            << "Collection thread didn't terminate within 1 second.";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::TERMINATED);
}

TEST(WatchdogPerfServiceTest, TestCustomCollection) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();
    internal::WatchdogPerfServicePeer servicePeer(service);

    ASSERT_NO_FATAL_FAILURE(startPeriodicCollection(&servicePeer));

    // Start custom collection with filter packages option.
    Vector<String16> args;
    args.push_back(String16(kStartCustomCollectionFlag));
    args.push_back(String16(kIntervalFlag));
    args.push_back(String16(std::to_string(kTestCustomCollectionInterval.count()).c_str()));
    args.push_back(String16(kMaxDurationFlag));
    args.push_back(String16(std::to_string(kTestCustomCollectionDuration.count()).c_str()));
    args.push_back(String16(kFilterPackagesFlag));
    args.push_back(String16("android.car.cts,system_server"));

    ASSERT_RESULT_OK(service->onCustomCollection(-1, args));

    // Poll until custom collection auto terminates.
    int maxIterations = static_cast<int>(kTestCustomCollectionDuration.count() /
                                         kTestCustomCollectionInterval.count());
    for (int i = 0; i <= maxIterations; ++i) {
        EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
        EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
        EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
        EXPECT_CALL(*servicePeer.mockDataProcessor,
                    onCustomCollection(_, SystemState::NORMAL_MODE,
                                       UnorderedElementsAreArray(
                                               {"android.car.cts", "system_server"}),
                                       wp<UidIoStats>(servicePeer.mockUidIoStats),
                                       wp<ProcStat>(servicePeer.mockProcStat),
                                       wp<ProcPidStat>(servicePeer.mockProcPidStat)))
                .Times(1);

        ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

        int secondsElapsed = (i == 0 ? 0 : kTestCustomCollectionInterval.count());
        ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), secondsElapsed)
                << "Custom collection didn't happen at " << secondsElapsed
                << " seconds interval in iteration " << i;
        ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::CUSTOM_COLLECTION)
                << "Invalid collection event";
        servicePeer.verifyAndClearExpectations();
    }

    EXPECT_CALL(*servicePeer.mockDataProcessor, onCustomCollectionDump(-1)).Times(1);

    // Next looper message was injected during startCustomCollection to end the custom collection
    // after |kTestCustomCollectionDuration|. On processing this message, the custom collection
    // should auto terminate.
    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(),
              kTestCustomCollectionDuration.count() % kTestCustomCollectionInterval.count())
            << "Custom collection did't end after " << kTestCustomCollectionDuration.count()
            << " seconds";
    ASSERT_EQ(servicePeer.getCurrCollectionEvent(), EventType::PERIODIC_COLLECTION)
            << "Invalid collection event";
    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);
}

TEST(WatchdogPerfServiceTest, TestPeriodicMonitorRequestsCollection) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();
    internal::WatchdogPerfServicePeer servicePeer(service);

    ASSERT_NO_FATAL_FAILURE(startPeriodicCollection(&servicePeer));

    wp<UidIoStats> uidIoStats(servicePeer.mockUidIoStats);
    wp<IProcDiskStatsInterface> procDiskStats(servicePeer.mockProcDiskStats);
    wp<ProcStat> procStat(servicePeer.mockProcStat);
    wp<ProcPidStat> procPidStat(servicePeer.mockProcPidStat);

    // Periodic monitor issuing an alert to start new collection.
    EXPECT_CALL(*servicePeer.mockProcDiskStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor, onPeriodicMonitor(_, procDiskStats, _))
            .WillOnce([&](auto, auto, const auto& alertHandler) -> Result<void> {
                alertHandler();
                return {};
            });

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), kTestPeriodicMonitorInterval.count())
            << "First periodic monitor didn't happen at " << kTestPeriodicMonitorInterval.count()
            << " seconds interval";
    servicePeer.verifyAndClearExpectations();

    EXPECT_CALL(*servicePeer.mockUidIoStats, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockProcPidStat, collect()).Times(1);
    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onPeriodicCollection(_, SystemState::NORMAL_MODE, uidIoStats, procStat,
                                     procPidStat))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    ASSERT_EQ(servicePeer.looperStub->numSecondsElapsed(), 0)
            << "First periodic collection didn't happen immediately after the alert";

    servicePeer.verifyAndClearExpectations();

    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);
}

TEST(WatchdogPerfServiceTest, TestSystemStateSwitch) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();
    internal::WatchdogPerfServicePeer servicePeer(service);

    ASSERT_NO_FATAL_FAILURE(startPeriodicCollection(&servicePeer));
    ASSERT_NO_FATAL_FAILURE(skipPeriodicMonitorEvents(&servicePeer));

    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onPeriodicCollection(_, SystemState::NORMAL_MODE, _, _, _))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    servicePeer.verifyAndClearExpectations();

    ASSERT_NO_FATAL_FAILURE(skipPeriodicMonitorEvents(&servicePeer));

    service->setSystemState(SystemState::GARAGE_MODE);

    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onPeriodicCollection(_, SystemState::GARAGE_MODE, _, _, _))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    servicePeer.verifyAndClearExpectations();

    ASSERT_NO_FATAL_FAILURE(skipPeriodicMonitorEvents(&servicePeer));

    service->setSystemState(SystemState::NORMAL_MODE);

    EXPECT_CALL(*servicePeer.mockDataProcessor,
                onPeriodicCollection(_, SystemState::NORMAL_MODE, _, _, _))
            .Times(1);

    ASSERT_RESULT_OK(servicePeer.looperStub->pollCache());

    servicePeer.verifyAndClearExpectations();

    EXPECT_CALL(*servicePeer.mockDataProcessor, terminate()).Times(1);
}

TEST(WatchdogPerfServiceTest, TestHandlesInvalidDumpArguments) {
    sp<WatchdogPerfService> service = sp<WatchdogPerfService>::make();
    Vector<String16> args;
    args.push_back(String16(kStartCustomCollectionFlag));
    args.push_back(String16("Invalid flag"));
    args.push_back(String16("Invalid value"));
    ASSERT_FALSE(service->onCustomCollection(-1, args).ok());

    args.clear();
    args.push_back(String16(kStartCustomCollectionFlag));
    args.push_back(String16(kIntervalFlag));
    args.push_back(String16("Invalid interval"));
    ASSERT_FALSE(service->onCustomCollection(-1, args).ok());

    args.clear();
    args.push_back(String16(kStartCustomCollectionFlag));
    args.push_back(String16(kMaxDurationFlag));
    args.push_back(String16("Invalid duration"));
    ASSERT_FALSE(service->onCustomCollection(-1, args).ok());

    args.clear();
    args.push_back(String16(kEndCustomCollectionFlag));
    args.push_back(String16(kMaxDurationFlag));
    args.push_back(String16(std::to_string(kTestCustomCollectionDuration.count()).c_str()));
    ASSERT_FALSE(service->onCustomCollection(-1, args).ok());

    args.clear();
    args.push_back(String16("Invalid flag"));
    ASSERT_FALSE(service->onCustomCollection(-1, args).ok());
    service->terminate();
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
