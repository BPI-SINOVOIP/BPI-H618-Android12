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

#include <gtest/gtest.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread>

#include "app_test_base.h"
#include "chpp/app.h"
#include "chpp/clients/discovery.h"
#include "chpp/clients/loopback.h"
#include "chpp/clients/timesync.h"
#include "chpp/log.h"
#include "chpp/transport.h"

/*
 * Test suite for the CHPP Loopback client/service
 */
namespace chpp {
namespace {

TEST_F(AppTestBase, SimpleStartStop) {
  // Simple test to make sure start/stop work threads work without crashing
  ASSERT_TRUE(mClientTransportContext.linkParams.linkEstablished);
  ASSERT_TRUE(mServiceTransportContext.linkParams.linkEstablished);
}

TEST_F(AppTestBase, TransportLayerLoopback) {
  // This tests the more limited transport-layer-looopback. In contrast,
  // the regular application-layer loopback test provides a more thorough test
  // and test results.
  constexpr size_t kTestLen = CHPP_TRANSPORT_TX_MTU_BYTES;
  uint8_t buf[kTestLen];
  for (size_t i = 0; i < kTestLen; i++) {
    buf[i] = (uint8_t)(i + 100);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CHPP_LOGI("Starting transport-layer loopback test (max buffer = %zu)...",
            kTestLen);

  EXPECT_EQ(CHPP_APP_ERROR_NONE,
            chppRunTransportLoopback(mClientAppContext.transportContext, buf,
                                     kTestLen));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_EQ(CHPP_APP_ERROR_NONE,
            mClientAppContext.transportContext->loopbackResult);

  EXPECT_EQ(
      CHPP_APP_ERROR_NONE,
      chppRunTransportLoopback(mClientAppContext.transportContext, buf, 100));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_EQ(CHPP_APP_ERROR_NONE,
            mClientAppContext.transportContext->loopbackResult);

  EXPECT_EQ(
      CHPP_APP_ERROR_NONE,
      chppRunTransportLoopback(mClientAppContext.transportContext, buf, 1));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_EQ(CHPP_APP_ERROR_NONE,
            mClientAppContext.transportContext->loopbackResult);

  EXPECT_EQ(
      CHPP_APP_ERROR_INVALID_LENGTH,
      chppRunTransportLoopback(mClientAppContext.transportContext, buf, 0));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_EQ(CHPP_APP_ERROR_INVALID_LENGTH,
            mClientAppContext.transportContext->loopbackResult);
}

TEST_F(AppTestBase, SimpleLoopback) {
  constexpr size_t kTestLen =
      CHPP_TRANSPORT_TX_MTU_BYTES - CHPP_LOOPBACK_HEADER_LEN;
  uint8_t buf[kTestLen];
  for (size_t i = 0; i < kTestLen; i++) {
    buf[i] = (uint8_t)(i + 100);
  }

  CHPP_LOGI(
      "Starting loopback test without fragmentation (max buffer = %zu)...",
      kTestLen);

  struct ChppLoopbackTestResult result;

  result = chppRunLoopbackTest(&mClientAppContext, buf, kTestLen);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 10);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 1);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 0);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_INVALID_LENGTH);
}

TEST_F(AppTestBase, FragmentedLoopback) {
  constexpr size_t kTestLen = UINT16_MAX;
  uint8_t buf[kTestLen];
  for (size_t i = 0; i < kTestLen; i++) {
    buf[i] = (uint8_t)(
        (i % 251) + 64);  // Arbitrary data. A modulus of 251, a prime number,
                          // reduces the chance of alignment with the MTU.
  }

  CHPP_LOGI("Starting loopback test with fragmentation (max buffer = %zu)...",
            kTestLen);

  struct ChppLoopbackTestResult result;

  result = chppRunLoopbackTest(&mClientAppContext, buf, kTestLen);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 50000);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(
      &mClientAppContext, buf,
      CHPP_TRANSPORT_TX_MTU_BYTES - CHPP_LOOPBACK_HEADER_LEN + 1);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);
}

TEST_F(AppTestBase, Timesync) {
  constexpr uint64_t kMaxRtt = 2 * CHPP_NSEC_PER_MSEC;    // in ms
  constexpr int64_t kMaxOffset = 1 * CHPP_NSEC_PER_MSEC;  // in ms

  CHPP_LOGI("Starting timesync test...");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(chppTimesyncMeasureOffset(&mClientAppContext));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(chppTimesyncGetResult(&mClientAppContext)->error,
            CHPP_APP_ERROR_NONE);

  EXPECT_LT(chppTimesyncGetResult(&mClientAppContext)->rttNs, kMaxRtt);
  EXPECT_NE(chppTimesyncGetResult(&mClientAppContext)->rttNs, 0);

  EXPECT_LT(chppTimesyncGetResult(&mClientAppContext)->offsetNs, kMaxOffset);
  EXPECT_GT(chppTimesyncGetResult(&mClientAppContext)->offsetNs, -kMaxOffset);
  EXPECT_NE(chppTimesyncGetResult(&mClientAppContext)->offsetNs, 0);
}

TEST_F(AppTestBase, DiscoveryMatched) {
  constexpr uint64_t kTimeoutMs = 5000;
  EXPECT_TRUE(chppWaitForDiscoveryComplete(&mClientAppContext, kTimeoutMs));
  EXPECT_TRUE(chppAreAllClientsMatched(&mClientAppContext));
}

}  // namespace
}  // namespace chpp
