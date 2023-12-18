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

#include "clients_test.h"

#include <gtest/gtest.h>

#include <string.h>
#include <thread>

#include "chpp/app.h"
#include "chpp/clients.h"
#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chpp/platform/utils.h"
#include "chpp/services.h"
#include "chpp/time.h"
#include "chpp/transport.h"
#include "chre/pal/wwan.h"

class ClientsTest : public testing::Test {
 protected:
  void SetUp() override {
    chppClearTotalAllocBytes();
    memset(&mTransportContext.linkParams, 0,
           sizeof(mTransportContext.linkParams));
    mTransportContext.linkParams.linkEstablished = true;
    chppTransportInit(&mTransportContext, &mAppContext);
    chppAppInit(&mAppContext, &mTransportContext);
    mClientState =
        (struct ChppClientState *)mAppContext.registeredClientContexts[0];
    chppClientInit(mClientState, CHPP_HANDLE_NEGOTIATED_RANGE_START);

    mTransportContext.resetState = CHPP_RESET_STATE_NONE;
  }

  void TearDown() override {
    chppAppDeinit(&mAppContext);
    chppTransportDeinit(&mTransportContext);

    EXPECT_EQ(chppGetTotalAllocBytes(), 0);
  }

  struct ChppTransportState mTransportContext;
  struct ChppAppState mAppContext;
  struct ChppClient mClient;
  struct ChppClientState *mClientState;
  struct ChppRequestResponseState mRRState;
};

void getClientRRStateInputCheck(struct ChppClientState *clientState,
                                struct ChppAppHeader *header) {
  ASSERT_TRUE(clientState != NULL);
  uint8_t clientIdx = clientState->index;

  ASSERT_TRUE(clientState->appContext != NULL);
  ASSERT_TRUE(clientState->appContext->registeredClients != NULL);
  ASSERT_TRUE(clientState->appContext->registeredClients[clientIdx] != NULL);
  ASSERT_TRUE(
      clientState->appContext->registeredClientStates[clientIdx]->rRStates !=
      NULL);
  ASSERT_LT(
      header->command,
      clientState->appContext->registeredClients[clientIdx]->rRStateCount);
}

struct ChppRequestResponseState *getClientRRState(
    struct ChppClientState *clientState, struct ChppAppHeader *header) {
  getClientRRStateInputCheck(clientState, header);

  uint8_t clientIdx = clientState->index;
  return &(clientState->appContext->registeredClientStates[clientIdx]
               ->rRStates[header->command]);
}

void isTimeoutAsExpected(uint64_t timeoutTimeNs, uint64_t expectedTimeNs) {
  uint64_t kJitterNs = 10 * CHPP_NSEC_PER_MSEC;

  if (expectedTimeNs == CHPP_TIME_MAX) {
    EXPECT_EQ(timeoutTimeNs, expectedTimeNs);
  } else {
    EXPECT_GE(timeoutTimeNs, expectedTimeNs);
    EXPECT_LE(timeoutTimeNs, expectedTimeNs + kJitterNs);
  }
}

void registerAndValidateRequestForTimeout(struct ChppClientState *clientState,
                                          struct ChppAppHeader *header,
                                          uint64_t timeoutNs,
                                          uint64_t expectedTimeNs) {
  struct ChppRequestResponseState *rRState =
      getClientRRState(clientState, header);
  chppClientTimestampRequest(clientState, rRState, header, timeoutNs);

  isTimeoutAsExpected(clientState->appContext->nextRequestTimeoutNs,
                      expectedTimeNs);
}

void registerAndValidateResponseForTimeout(struct ChppClientState *clientState,
                                           const struct ChppAppHeader *header,
                                           uint64_t expectedTimeNs) {
  ASSERT_FALSE(clientState == NULL);
  uint8_t clientIdx = clientState->index;

  ASSERT_FALSE(clientState->appContext == NULL);
  ASSERT_FALSE(clientState->appContext->registeredClients == NULL);
  ASSERT_FALSE(clientState->appContext->registeredClients[clientIdx] == NULL);
  ASSERT_FALSE(
      clientState->appContext->registeredClientStates[clientIdx]->rRStates ==
      NULL);
  ASSERT_LT(
      header->command,
      clientState->appContext->registeredClients[clientIdx]->rRStateCount);

  struct ChppRequestResponseState *rRState =
      &(clientState->appContext->registeredClientStates[clientIdx]
            ->rRStates[header->command]);
  chppClientTimestampResponse(clientState, rRState, header);

  isTimeoutAsExpected(clientState->appContext->nextRequestTimeoutNs,
                      expectedTimeNs);
}

void validateTimeoutResponse(const struct ChppAppHeader *request,
                             const struct ChppAppHeader *response) {
  ASSERT_TRUE(request != NULL);
  ASSERT_TRUE(response != NULL);

  EXPECT_EQ(response->handle, request->handle);
  EXPECT_EQ(response->type, CHPP_MESSAGE_TYPE_SERVICE_RESPONSE);
  EXPECT_EQ(response->transaction, request->transaction);
  EXPECT_EQ(response->error, CHPP_APP_ERROR_TIMEOUT);
  EXPECT_EQ(response->command, request->command);
}

TEST_F(ClientsTest, RequestResponseTimestampValid) {
  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(mClientState, 0 /* command */);
  chppClientTimestampRequest(mClientState, &mRRState, reqHeader,
                             CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE);

  struct ChppAppHeader *respHeader =
      chppAllocServiceResponse(reqHeader, sizeof(*reqHeader));
  ASSERT_TRUE(chppClientTimestampResponse(mClientState, &mRRState, respHeader));

  chppFree(reqHeader);
  chppFree(respHeader);
}

TEST_F(ClientsTest, RequestResponseTimestampDuplicate) {
  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(mClientState, 0 /* command */);
  chppClientTimestampRequest(mClientState, &mRRState, reqHeader,
                             CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE);

  struct ChppAppHeader *respHeader =
      chppAllocServiceResponse(reqHeader, sizeof(*reqHeader));
  ASSERT_TRUE(chppClientTimestampResponse(mClientState, &mRRState, respHeader));
  ASSERT_FALSE(
      chppClientTimestampResponse(mClientState, &mRRState, respHeader));

  chppFree(reqHeader);
  chppFree(respHeader);
}

TEST_F(ClientsTest, RequestResponseTimestampInvalidId) {
  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(mClientState, 0 /* command */);
  chppClientTimestampRequest(mClientState, &mRRState, reqHeader,
                             CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE);

  struct ChppAppHeader *newReqHeader =
      chppAllocClientRequestCommand(mClientState, 0 /* command */);
  struct ChppAppHeader *respHeader =
      chppAllocServiceResponse(newReqHeader, sizeof(*reqHeader));
  ASSERT_FALSE(
      chppClientTimestampResponse(mClientState, &mRRState, respHeader));

  chppFree(reqHeader);
  chppFree(newReqHeader);
  chppFree(respHeader);
}

TEST_F(ClientsTest, RequestTimeoutAddRemoveSingle) {
  EXPECT_EQ(mAppContext.nextRequestTimeoutNs, CHPP_TIME_MAX);

  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(mClientState, 1 /* command */);

  uint64_t time = chppGetCurrentTimeNs();
  uint64_t timeout = 1000 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader, timeout,
                                       time + timeout);

  EXPECT_TRUE(
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext) == NULL);

  registerAndValidateResponseForTimeout(mClientState, reqHeader, CHPP_TIME_MAX);

  chppFree(reqHeader);
}

TEST_F(ClientsTest, RequestTimeoutAddRemoveMultiple) {
  struct ChppAppHeader *reqHeader1 =
      chppAllocClientRequestCommand(mClientState, 0 /* command */);
  struct ChppAppHeader *reqHeader2 =
      chppAllocClientRequestCommand(mClientState, 1 /* command */);
  struct ChppAppHeader *reqHeader3 =
      chppAllocClientRequestCommand(mClientState, 2 /* command */);

  EXPECT_EQ(mAppContext.nextRequestTimeoutNs, CHPP_TIME_MAX);

  uint64_t time1 = chppGetCurrentTimeNs();
  uint64_t timeout1 = 2000 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader1, timeout1,
                                       time1 + timeout1);

  uint64_t time2 = chppGetCurrentTimeNs();
  uint64_t timeout2 = 4000 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader2, timeout2,
                                       time1 + timeout1);

  uint64_t time3 = chppGetCurrentTimeNs();
  uint64_t timeout3 = 3000 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader3, timeout3,
                                       time1 + timeout1);

  registerAndValidateResponseForTimeout(mClientState, reqHeader1,
                                        time3 + timeout3);

  EXPECT_TRUE(
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext) == NULL);

  uint64_t time4 = chppGetCurrentTimeNs();
  uint64_t timeout4 = 1000 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader1, timeout4,
                                       time4 + timeout4);

  registerAndValidateResponseForTimeout(mClientState, reqHeader1,
                                        time3 + timeout3);

  registerAndValidateResponseForTimeout(mClientState, reqHeader3,
                                        time2 + timeout2);

  registerAndValidateResponseForTimeout(mClientState, reqHeader2,
                                        CHPP_TIME_MAX);

  EXPECT_TRUE(
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext) == NULL);

  chppFree(reqHeader1);
  chppFree(reqHeader2);
  chppFree(reqHeader3);
}

TEST_F(ClientsTest, DuplicateRequestTimeoutResponse) {
  EXPECT_EQ(mAppContext.nextRequestTimeoutNs, CHPP_TIME_MAX);

  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(mClientState, 1 /* command */);

  uint64_t time1 = chppGetCurrentTimeNs();
  uint64_t timeout1 = 200 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader, timeout1,
                                       time1 + timeout1);

  std::this_thread::sleep_for(std::chrono::nanoseconds(timeout1 / 2));

  uint64_t time2 = chppGetCurrentTimeNs();
  uint64_t timeout2 = 200 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader, timeout2,
                                       time2 + timeout2);

  std::this_thread::sleep_for(
      std::chrono::nanoseconds(timeout1 + time1 - chppGetCurrentTimeNs()));
  // First request would have timed out but superseded by second request
  ASSERT_GT(mAppContext.nextRequestTimeoutNs, chppGetCurrentTimeNs());

  std::this_thread::sleep_for(
      std::chrono::nanoseconds(timeout2 + time2 - chppGetCurrentTimeNs()));
  // Second request should have timed out
  ASSERT_LT(mAppContext.nextRequestTimeoutNs, chppGetCurrentTimeNs());

  struct ChppAppHeader *response =
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext);
  validateTimeoutResponse(reqHeader, response);
  if (response != NULL) {
    chppFree(response);
  }

  registerAndValidateResponseForTimeout(mClientState, reqHeader, CHPP_TIME_MAX);
  EXPECT_TRUE(
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext) == NULL);

  chppFree(reqHeader);
}

TEST_F(ClientsTest, RequestTimeoutResponse) {
  EXPECT_EQ(mAppContext.nextRequestTimeoutNs, CHPP_TIME_MAX);

  struct ChppAppHeader *reqHeader1 =
      chppAllocClientRequestCommand(mClientState, 1 /* command */);
  struct ChppAppHeader *reqHeader2 =
      chppAllocClientRequestCommand(mClientState, 2 /* command */);

  uint64_t time1 = chppGetCurrentTimeNs();
  uint64_t timeout1 = 200 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader1, timeout1,
                                       time1 + timeout1);

  std::this_thread::sleep_for(std::chrono::nanoseconds(timeout1));
  ASSERT_LT(mAppContext.nextRequestTimeoutNs, chppGetCurrentTimeNs());

  struct ChppAppHeader *response =
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext);
  validateTimeoutResponse(reqHeader1, response);
  if (response != NULL) {
    chppFree(response);
  }

  registerAndValidateResponseForTimeout(mClientState, reqHeader1,
                                        CHPP_TIME_MAX);
  EXPECT_TRUE(
      chppTransportGetClientRequestTimeoutResponse(&mTransportContext) == NULL);

  uint64_t time2 = chppGetCurrentTimeNs();
  uint64_t timeout2 = 200 * CHPP_NSEC_PER_MSEC;
  registerAndValidateRequestForTimeout(mClientState, reqHeader2, timeout2,
                                       time2 + timeout2);

  std::this_thread::sleep_for(std::chrono::nanoseconds(timeout2));
  ASSERT_LT(mAppContext.nextRequestTimeoutNs, chppGetCurrentTimeNs());

  response = chppTransportGetClientRequestTimeoutResponse(&mTransportContext);
  validateTimeoutResponse(reqHeader2, response);
  if (response != NULL) {
    chppFree(response);
  }

  chppFree(reqHeader1);
  chppFree(reqHeader2);
}
