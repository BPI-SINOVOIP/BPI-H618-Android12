/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <chrono>
#include <fstream>
#include <thread>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <android-base/file.h>
#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <gtest/gtest.h>

#include "GrpcVehicleClient.h"
#include "GrpcVehicleServer.h"
#include "Utils.h"
#include "vhal_v2_0/VehicleUtils.h"

namespace android::hardware::automotive::vehicle::V2_0::impl {

class GrpcServerTest : public ::testing::Test {
  public:
    GrpcServerTest()
        : mPowerStateSocketForTest(std::string(mTestTempDir.path) +
                                   std::string("/power_state_socket_for_test_") +
                                   std::to_string(getpid())),
          mServerInfo({{GetTestCID(), 12345},
                       mPowerStateMarkerFileForTest.path,
                       mPowerStateSocketForTest}) {}

    void SetUp() override {
        mGrpcServer = makeGrpcVehicleServer(mServerInfo);

        ASSERT_TRUE(GetGrpcServer() != nullptr);
        GetGrpcServer()->Start();
    }

    void TearDown() override {
        GetGrpcServer()->Stop().Wait();
        mGrpcServer.reset();
    }

    static unsigned GetTestCID();

    GrpcVehicleServer* GetGrpcServer() const { return mGrpcServer.get(); }

    std::string GetGrpcServerUri() const { return mServerInfo.getServerUri(); }

    std::string GetPowerStateSocketPath() const { return mPowerStateSocketForTest; }

    std::string GetPowerStateMarkerFilePath() const {
        return std::string(mPowerStateMarkerFileForTest.path);
    }

    void SendDummyValueFromServer();

    void ExpectActivePropValueStreamNum(unsigned expected);

    void WriteToPowerStateSocket(const std::string& val);

    std::string ReadFromPowerStateMarkerFile() const;

  private:
    TemporaryDir mTestTempDir;
    TemporaryFile mPowerStateMarkerFileForTest;

    std::string mPowerStateSocketForTest{};

    GrpcVehicleServerPtr mGrpcServer{nullptr};
    VirtualizedVhalServerInfo mServerInfo;
};

unsigned GrpcServerTest::GetTestCID() {
    // TODO(chenhaosjtuacm): find a way to get the local CID
    return 1000;
}

void GrpcServerTest::SendDummyValueFromServer() {
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::INVALID);
    GetGrpcServer()->onPropertyValueFromCar(value, false);
}

void GrpcServerTest::ExpectActivePropValueStreamNum(unsigned expected) {
    // Force the server to refresh streams
    SendDummyValueFromServer();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(GetGrpcServer()->NumOfActivePropertyValueStream(), expected);
}

void GrpcServerTest::WriteToPowerStateSocket(const std::string& val) {
    int power_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(power_socket_fd, 0);

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::string socket_path = GetPowerStateSocketPath();
    std::strncpy(addr.sun_path, socket_path.c_str(), socket_path.length() + 1);

    sync();

    ASSERT_EQ(connect(power_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)), 0);
    EXPECT_EQ(write(power_socket_fd, val.c_str(), val.length()),
              static_cast<ssize_t>(val.length()));
    close(power_socket_fd);
}

std::string GrpcServerTest::ReadFromPowerStateMarkerFile() const {
    std::ifstream stream(GetPowerStateMarkerFilePath());
    std::string val;
    stream >> val;
    return val;
}

TEST_F(GrpcServerTest, PropertyValueStreamTest) {
    ExpectActivePropValueStreamNum(0);
    {
        auto client1 = makeGrpcVehicleClient(GetGrpcServerUri());
        ExpectActivePropValueStreamNum(1);
        {
            auto client2 = makeGrpcVehicleClient(GetGrpcServerUri());
            ExpectActivePropValueStreamNum(2);
        }
        ExpectActivePropValueStreamNum(1);
    }
    ExpectActivePropValueStreamNum(0);
}

TEST_F(GrpcServerTest, PowerStateListenerTest) {
    {
        std::string power_state_str = "ok";
        WriteToPowerStateSocket(power_state_str);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        EXPECT_EQ(ReadFromPowerStateMarkerFile(), power_state_str);
    }

    {
        std::string power_state_str = "shutdown";
        WriteToPowerStateSocket(power_state_str);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        EXPECT_EQ(ReadFromPowerStateMarkerFile(), power_state_str);
    }
}

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
