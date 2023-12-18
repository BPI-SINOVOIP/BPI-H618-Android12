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

#include "MockIoOveruseMonitor.h"
#include "MockWatchdogPerfService.h"
#include "MockWatchdogProcessService.h"
#include "MockWatchdogServiceHelper.h"
#include "WatchdogBinderMediator.h"
#include "WatchdogInternalHandler.h"
#include "WatchdogServiceHelper.h"

#include <android-base/result.h>
#include <android/automotive/watchdog/internal/BootPhase.h>
#include <android/automotive/watchdog/internal/GarageMode.h>
#include <android/automotive/watchdog/internal/PowerCycle.h>
#include <android/automotive/watchdog/internal/UserState.h>
#include <binder/IBinder.h>
#include <binder/IPCThreadState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <private/android_filesystem_config.h>
#include <utils/RefBase.h>

#include <errno.h>

namespace android {
namespace automotive {
namespace watchdog {

namespace aawi = ::android::automotive::watchdog::internal;

using aawi::GarageMode;
using aawi::ICarWatchdogServiceForSystem;
using aawi::ICarWatchdogServiceForSystemDefault;
using aawi::PackageResourceOveruseAction;
using aawi::PowerCycle;
using aawi::ResourceOveruseConfiguration;
using ::android::sp;
using ::android::String16;
using ::android::base::Result;
using ::android::binder::Status;
using ::testing::_;
using ::testing::Pointer;
using ::testing::Return;

namespace {

class MockWatchdogBinderMediator : public WatchdogBinderMediator {
public:
    MockWatchdogBinderMediator(const android::sp<WatchdogProcessService>& watchdogProcessService,
                               const android::sp<WatchdogPerfServiceInterface>& watchdogPerfService,
                               const android::sp<IWatchdogServiceHelper>& watchdogServiceHelper) :
          WatchdogBinderMediator(watchdogProcessService, watchdogPerfService, watchdogServiceHelper,
                                 [](const char*, const android::sp<android::IBinder>&)
                                         -> Result<void> { return Result<void>{}; }) {}
    ~MockWatchdogBinderMediator() {}

    MOCK_METHOD(status_t, dump, (int fd, const Vector<android::String16>& args), (override));
};

class ScopedChangeCallingUid : public RefBase {
public:
    explicit ScopedChangeCallingUid(uid_t uid) {
        mCallingUid = IPCThreadState::self()->getCallingUid();
        mCallingPid = IPCThreadState::self()->getCallingPid();
        if (mCallingUid == uid) {
            return;
        }
        mChangedUid = uid;
        int64_t token = ((int64_t)mChangedUid << 32) | mCallingPid;
        IPCThreadState::self()->restoreCallingIdentity(token);
    }
    ~ScopedChangeCallingUid() {
        if (mCallingUid == mChangedUid) {
            return;
        }
        int64_t token = ((int64_t)mCallingUid << 32) | mCallingPid;
        IPCThreadState::self()->restoreCallingIdentity(token);
    }

private:
    uid_t mCallingUid;
    uid_t mChangedUid;
    pid_t mCallingPid;
};

}  // namespace

class WatchdogInternalHandlerTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        mMockWatchdogProcessService = sp<MockWatchdogProcessService>::make();
        mMockWatchdogPerfService = sp<MockWatchdogPerfService>::make();
        mMockWatchdogServiceHelper = sp<MockWatchdogServiceHelper>::make();
        mMockIoOveruseMonitor = sp<MockIoOveruseMonitor>::make();
        mMockWatchdogBinderMediator =
                sp<MockWatchdogBinderMediator>::make(mMockWatchdogProcessService,
                                                     mMockWatchdogPerfService,
                                                     mMockWatchdogServiceHelper);
        mWatchdogInternalHandler =
                sp<WatchdogInternalHandler>::make(mMockWatchdogBinderMediator,
                                                  mMockWatchdogServiceHelper,
                                                  mMockWatchdogProcessService,
                                                  mMockWatchdogPerfService, mMockIoOveruseMonitor);
    }
    virtual void TearDown() {
        mMockWatchdogBinderMediator.clear();
        mMockWatchdogServiceHelper.clear();
        mMockWatchdogProcessService.clear();
        mMockWatchdogPerfService.clear();
        mMockIoOveruseMonitor.clear();
        mWatchdogInternalHandler.clear();
        mScopedChangeCallingUid.clear();
    }

    // Sets calling UID to imitate System's process.
    void setSystemCallingUid() {
        mScopedChangeCallingUid = sp<ScopedChangeCallingUid>::make(AID_SYSTEM);
    }

    sp<MockWatchdogBinderMediator> mMockWatchdogBinderMediator;
    sp<MockWatchdogServiceHelper> mMockWatchdogServiceHelper;
    sp<MockWatchdogProcessService> mMockWatchdogProcessService;
    sp<MockWatchdogPerfService> mMockWatchdogPerfService;
    sp<MockIoOveruseMonitor> mMockIoOveruseMonitor;
    sp<WatchdogInternalHandler> mWatchdogInternalHandler;
    sp<ScopedChangeCallingUid> mScopedChangeCallingUid;
};

TEST_F(WatchdogInternalHandlerTest, TestTerminate) {
    ASSERT_NE(mWatchdogInternalHandler->mBinderMediator, nullptr);
    ASSERT_NE(mWatchdogInternalHandler->mWatchdogServiceHelper, nullptr);
    ASSERT_NE(mWatchdogInternalHandler->mWatchdogProcessService, nullptr);
    ASSERT_NE(mWatchdogInternalHandler->mWatchdogPerfService, nullptr);
    ASSERT_NE(mWatchdogInternalHandler->mIoOveruseMonitor, nullptr);

    mWatchdogInternalHandler->terminate();

    ASSERT_EQ(mWatchdogInternalHandler->mBinderMediator, nullptr);
    ASSERT_EQ(mWatchdogInternalHandler->mWatchdogServiceHelper, nullptr);
    ASSERT_EQ(mWatchdogInternalHandler->mWatchdogProcessService, nullptr);
    ASSERT_EQ(mWatchdogInternalHandler->mWatchdogPerfService, nullptr);
    ASSERT_EQ(mWatchdogInternalHandler->mIoOveruseMonitor, nullptr);
}

TEST_F(WatchdogInternalHandlerTest, TestDump) {
    EXPECT_CALL(*mMockWatchdogBinderMediator, dump(-1, _)).WillOnce(Return(OK));
    ASSERT_EQ(mWatchdogInternalHandler->dump(-1, Vector<String16>()), OK);
}

TEST_F(WatchdogInternalHandlerTest, TestRegisterCarWatchdogService) {
    setSystemCallingUid();
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    EXPECT_CALL(*mMockWatchdogServiceHelper, registerService(service))
            .WillOnce(Return(Status::ok()));
    EXPECT_CALL(*mMockWatchdogServiceHelper, registerService(service))
            .WillOnce(Return(Status::ok()));

    Status status = mWatchdogInternalHandler->registerCarWatchdogService(service);

    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnRegisterCarWatchdogServiceWithNonSystemCallingUid) {
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    EXPECT_CALL(*mMockWatchdogServiceHelper, registerService(_)).Times(0);

    Status status = mWatchdogInternalHandler->registerCarWatchdogService(service);

    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest,
       TestErrorOnRegisterCarWatchdogServiceWithWatchdogServiceHelperError) {
    setSystemCallingUid();
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    EXPECT_CALL(*mMockWatchdogServiceHelper, registerService(service))
            .WillOnce(Return(Status::fromExceptionCode(Status::EX_ILLEGAL_STATE, "Illegal state")));

    Status status = mWatchdogInternalHandler->registerCarWatchdogService(service);

    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestUnregisterCarWatchdogService) {
    setSystemCallingUid();
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    EXPECT_CALL(*mMockWatchdogServiceHelper, unregisterService(service))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogInternalHandler->unregisterCarWatchdogService(service);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest,
       TestErrorOnUnregisterCarWatchdogServiceWithNonSystemCallingUid) {
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    EXPECT_CALL(*mMockWatchdogServiceHelper, unregisterService(service)).Times(0);
    Status status = mWatchdogInternalHandler->unregisterCarWatchdogService(service);
    ASSERT_FALSE(status.isOk()) << status;
}
TEST_F(WatchdogInternalHandlerTest,
       TestErrorOnUnregisterCarWatchdogServiceWithWatchdogServiceHelperError) {
    setSystemCallingUid();
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    EXPECT_CALL(*mMockWatchdogServiceHelper, unregisterService(service))
            .WillOnce(Return(
                    Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, "Illegal argument")));
    Status status = mWatchdogInternalHandler->unregisterCarWatchdogService(service);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestRegisterMonitor) {
    setSystemCallingUid();
    sp<aawi::ICarWatchdogMonitor> monitor = sp<aawi::ICarWatchdogMonitorDefault>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, registerMonitor(monitor))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogInternalHandler->registerMonitor(monitor);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnRegisterMonitorWithNonSystemCallingUid) {
    sp<aawi::ICarWatchdogMonitor> monitor = sp<aawi::ICarWatchdogMonitorDefault>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, registerMonitor(monitor)).Times(0);
    Status status = mWatchdogInternalHandler->registerMonitor(monitor);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestUnregisterMonitor) {
    setSystemCallingUid();
    sp<aawi::ICarWatchdogMonitor> monitor = sp<aawi::ICarWatchdogMonitorDefault>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, unregisterMonitor(monitor))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogInternalHandler->unregisterMonitor(monitor);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnUnregisterMonitorWithNonSystemCallingUid) {
    sp<aawi::ICarWatchdogMonitor> monitor = sp<aawi::ICarWatchdogMonitorDefault>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, unregisterMonitor(monitor)).Times(0);
    Status status = mWatchdogInternalHandler->unregisterMonitor(monitor);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestCarWatchdogServiceAlive) {
    setSystemCallingUid();
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    std::vector clientsNotResponding = {123};
    EXPECT_CALL(*mMockWatchdogProcessService,
                tellCarWatchdogServiceAlive(service, clientsNotResponding, 456))
            .WillOnce(Return(Status::ok()));
    Status status =
            mWatchdogInternalHandler->tellCarWatchdogServiceAlive(service, clientsNotResponding,
                                                                  456);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnCarWatchdogServiceWithNonSystemCallingUid) {
    sp<ICarWatchdogServiceForSystem> service = sp<ICarWatchdogServiceForSystemDefault>::make();
    std::vector clientsNotResponding = {123};
    EXPECT_CALL(*mMockWatchdogProcessService, tellCarWatchdogServiceAlive(_, _, _)).Times(0);
    Status status =
            mWatchdogInternalHandler->tellCarWatchdogServiceAlive(service, clientsNotResponding,
                                                                  456);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestTellDumpFinished) {
    setSystemCallingUid();
    sp<aawi::ICarWatchdogMonitor> monitor = sp<aawi::ICarWatchdogMonitorDefault>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, tellDumpFinished(monitor, 456))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogInternalHandler->tellDumpFinished(monitor, 456);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnTellDumpFinishedWithNonSystemCallingUid) {
    sp<aawi::ICarWatchdogMonitor> monitor = sp<aawi::ICarWatchdogMonitorDefault>::make();
    EXPECT_CALL(*mMockWatchdogProcessService, tellDumpFinished(_, _)).Times(0);
    Status status = mWatchdogInternalHandler->tellDumpFinished(monitor, 456);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyPowerCycleChangeToShutdownPrepare) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockWatchdogProcessService, setEnabled(/*isEnabled=*/false)).Times(1);
    Status status =
            mWatchdogInternalHandler
                    ->notifySystemStateChange(aawi::StateType::POWER_CYCLE,
                                              static_cast<int32_t>(
                                                      PowerCycle::POWER_CYCLE_SHUTDOWN_PREPARE),
                                              -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyPowerCycleChangeToShutdownEnter) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockWatchdogProcessService, setEnabled(/*isEnabled=*/false)).Times(1);
    Status status =
            mWatchdogInternalHandler
                    ->notifySystemStateChange(aawi::StateType::POWER_CYCLE,
                                              static_cast<int32_t>(
                                                      PowerCycle::POWER_CYCLE_SHUTDOWN_ENTER),
                                              -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyPowerCycleChangeToResume) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockWatchdogProcessService, setEnabled(/*isEnabled=*/true)).Times(1);
    EXPECT_CALL(*mMockWatchdogPerfService, setSystemState(SystemState::NORMAL_MODE)).Times(1);
    Status status =
            mWatchdogInternalHandler
                    ->notifySystemStateChange(aawi::StateType::POWER_CYCLE,
                                              static_cast<int32_t>(PowerCycle::POWER_CYCLE_RESUME),
                                              -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnNotifyPowerCycleChangeWithInvalidArgs) {
    EXPECT_CALL(*mMockWatchdogProcessService, setEnabled(_)).Times(0);
    EXPECT_CALL(*mMockWatchdogPerfService, setSystemState(_)).Times(0);
    aawi::StateType type = aawi::StateType::POWER_CYCLE;

    Status status = mWatchdogInternalHandler->notifySystemStateChange(type, -1, -1);
    ASSERT_FALSE(status.isOk()) << status;

    status = mWatchdogInternalHandler->notifySystemStateChange(type, 3000, -1);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyGarageModeOn) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockWatchdogPerfService, setSystemState(SystemState::GARAGE_MODE)).Times(1);
    Status status =
            mWatchdogInternalHandler->notifySystemStateChange(aawi::StateType::GARAGE_MODE,
                                                              static_cast<int32_t>(
                                                                      GarageMode::GARAGE_MODE_ON),
                                                              -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyGarageModeOff) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockWatchdogPerfService, setSystemState(SystemState::NORMAL_MODE)).Times(1);
    Status status =
            mWatchdogInternalHandler->notifySystemStateChange(aawi::StateType::GARAGE_MODE,
                                                              static_cast<int32_t>(
                                                                      GarageMode::GARAGE_MODE_OFF),
                                                              -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyUserStateChange) {
    setSystemCallingUid();
    aawi::StateType type = aawi::StateType::USER_STATE;
    EXPECT_CALL(*mMockWatchdogProcessService,
                notifyUserStateChange(234567, aawi::UserState::USER_STATE_STOPPED))
            .WillOnce(Return(Status::ok()));
    Status status = mWatchdogInternalHandler
                            ->notifySystemStateChange(type, 234567,
                                                      static_cast<int32_t>(
                                                              aawi::UserState::USER_STATE_STOPPED));
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnNotifyUserStateChangeWithInvalidArgs) {
    EXPECT_CALL(*mMockWatchdogProcessService, notifyUserStateChange(_, _)).Times(0);
    aawi::StateType type = aawi::StateType::USER_STATE;

    Status status = mWatchdogInternalHandler->notifySystemStateChange(type, 234567, -1);
    ASSERT_FALSE(status.isOk()) << status;

    status = mWatchdogInternalHandler->notifySystemStateChange(type, 234567, 3000);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyBootPhaseChange) {
    setSystemCallingUid();
    aawi::StateType type = aawi::StateType::BOOT_PHASE;
    EXPECT_CALL(*mMockWatchdogPerfService, onBootFinished()).WillOnce(Return(Result<void>()));
    Status status =
            mWatchdogInternalHandler
                    ->notifySystemStateChange(type,
                                              static_cast<int32_t>(aawi::BootPhase::BOOT_COMPLETED),
                                              -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestNotifyBootPhaseChangeWithNonBootCompletedPhase) {
    setSystemCallingUid();
    aawi::StateType type = aawi::StateType::BOOT_PHASE;
    EXPECT_CALL(*mMockWatchdogPerfService, onBootFinished()).Times(0);
    Status status = mWatchdogInternalHandler->notifySystemStateChange(type, 0, -1);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestErrorOnNotifySystemStateChangeWithNonSystemCallingUid) {
    aawi::StateType type = aawi::StateType::POWER_CYCLE;
    EXPECT_CALL(*mMockWatchdogProcessService, setEnabled(_)).Times(0);
    EXPECT_CALL(*mMockWatchdogPerfService, setSystemState(_)).Times(0);
    Status status =
            mWatchdogInternalHandler
                    ->notifySystemStateChange(type,
                                              static_cast<int32_t>(
                                                      PowerCycle::POWER_CYCLE_SHUTDOWN_PREPARE),
                                              -1);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestUpdateResourceOveruseConfigurations) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockIoOveruseMonitor, updateResourceOveruseConfigurations(_))
            .WillOnce(Return(Result<void>()));
    Status status = mWatchdogInternalHandler->updateResourceOveruseConfigurations(
            std::vector<ResourceOveruseConfiguration>{});
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest,
       TestErrorOnUpdateResourceOveruseConfigurationsWithNonSystemCallingUid) {
    EXPECT_CALL(*mMockIoOveruseMonitor, updateResourceOveruseConfigurations(_)).Times(0);
    Status status = mWatchdogInternalHandler->updateResourceOveruseConfigurations(
            std::vector<ResourceOveruseConfiguration>{});
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestGetResourceOveruseConfigurations) {
    setSystemCallingUid();
    std::vector<ResourceOveruseConfiguration> configs;
    EXPECT_CALL(*mMockIoOveruseMonitor, getResourceOveruseConfigurations(Pointer(&configs)))
            .WillOnce(Return(Result<void>()));
    Status status = mWatchdogInternalHandler->getResourceOveruseConfigurations(&configs);
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest,
       TestErrorOnGetResourceOveruseConfigurationsWithNonSystemCallingUid) {
    EXPECT_CALL(*mMockIoOveruseMonitor, getResourceOveruseConfigurations(_)).Times(0);
    std::vector<ResourceOveruseConfiguration> configs;
    Status status = mWatchdogInternalHandler->getResourceOveruseConfigurations(&configs);
    ASSERT_FALSE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest, TestActionTakenOnResourceOveruse) {
    setSystemCallingUid();
    EXPECT_CALL(*mMockIoOveruseMonitor, actionTakenOnIoOveruse(_)).WillOnce(Return(Result<void>()));
    Status status = mWatchdogInternalHandler->actionTakenOnResourceOveruse(
            std::vector<PackageResourceOveruseAction>{});
    ASSERT_TRUE(status.isOk()) << status;
}

TEST_F(WatchdogInternalHandlerTest,
       TestErrorOnActionTakenOnResourceOveruseWithNonSystemCallingUid) {
    EXPECT_CALL(*mMockIoOveruseMonitor, actionTakenOnIoOveruse(_)).Times(0);
    Status status = mWatchdogInternalHandler->actionTakenOnResourceOveruse(
            std::vector<PackageResourceOveruseAction>{});
    ASSERT_FALSE(status.isOk()) << status;
}

}  // namespace watchdog
}  // namespace automotive
}  // namespace android
