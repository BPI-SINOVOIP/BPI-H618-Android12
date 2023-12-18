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

#include "chpp/clients/loopback.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/app.h"
#include "chpp/clients.h"
#include "chpp/log.h"
#include "chpp/memory.h"
#include "chpp/transport.h"

#include "chpp/clients/discovery.h"

/************************************************
 *  Prototypes
 ***********************************************/

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Structure to maintain state for the loopback client and its Request/Response
 * (RR) functionality.
 */
struct ChppLoopbackClientState {
  struct ChppClientState client;                    // Loopback client state
  struct ChppRequestResponseState runLoopbackTest;  // Loopback test state

  struct ChppLoopbackTestResult testResult;  // Last test result
  const uint8_t *loopbackData;               // Pointer to loopback data
};

/************************************************
 *  Public Functions
 ***********************************************/

void chppLoopbackClientInit(struct ChppAppState *context) {
  CHPP_LOGD("Loopback client init");

  context->loopbackClientContext =
      chppMalloc(sizeof(struct ChppLoopbackClientState));
  CHPP_NOT_NULL(context->loopbackClientContext);
  memset(context->loopbackClientContext, 0,
         sizeof(struct ChppLoopbackClientState));

  context->loopbackClientContext->client.appContext = context;
  chppClientInit(&context->loopbackClientContext->client, CHPP_HANDLE_LOOPBACK);
  context->loopbackClientContext->testResult.error = CHPP_APP_ERROR_NONE;
  context->loopbackClientContext->client.openState = CHPP_OPEN_STATE_OPENED;
}

void chppLoopbackClientDeinit(struct ChppAppState *context) {
  CHPP_LOGD("Loopback client deinit");

  CHPP_NOT_NULL(context);
  CHPP_NOT_NULL(context->loopbackClientContext);

  chppClientDeinit(&context->loopbackClientContext->client);
  CHPP_FREE_AND_NULLIFY(context->loopbackClientContext);
}

bool chppDispatchLoopbackServiceResponse(struct ChppAppState *context,
                                         const uint8_t *response, size_t len) {
  CHPP_LOGD("Loopback client dispatch service response");

  CHPP_ASSERT(len >= CHPP_LOOPBACK_HEADER_LEN);
  CHPP_NOT_NULL(response);
  CHPP_NOT_NULL(context->loopbackClientContext);
  CHPP_NOT_NULL(context->loopbackClientContext->loopbackData);

  CHPP_ASSERT(chppClientTimestampResponse(
      &context->loopbackClientContext->client,
      &context->loopbackClientContext->runLoopbackTest,
      (const struct ChppAppHeader *)response));

  context->loopbackClientContext->testResult.error = CHPP_APP_ERROR_NONE;
  context->loopbackClientContext->testResult.responseLen = len;
  context->loopbackClientContext->testResult.firstError = len;
  context->loopbackClientContext->testResult.byteErrors = 0;
  context->loopbackClientContext->testResult.rttNs =
      context->loopbackClientContext->runLoopbackTest.responseTimeNs -
      context->loopbackClientContext->runLoopbackTest.requestTimeNs;

  if (context->loopbackClientContext->testResult.requestLen !=
      context->loopbackClientContext->testResult.responseLen) {
    context->loopbackClientContext->testResult.error =
        CHPP_APP_ERROR_INVALID_LENGTH;
    context->loopbackClientContext->testResult.firstError =
        MIN(context->loopbackClientContext->testResult.requestLen,
            context->loopbackClientContext->testResult.responseLen);
  }

  for (size_t loc = CHPP_LOOPBACK_HEADER_LEN;
       loc < MIN(context->loopbackClientContext->testResult.requestLen,
                 context->loopbackClientContext->testResult.responseLen);
       loc++) {
    if (context->loopbackClientContext
            ->loopbackData[loc - CHPP_LOOPBACK_HEADER_LEN] != response[loc]) {
      context->loopbackClientContext->testResult.error =
          CHPP_APP_ERROR_UNSPECIFIED;
      context->loopbackClientContext->testResult.firstError =
          MIN(context->loopbackClientContext->testResult.firstError,
              loc - CHPP_LOOPBACK_HEADER_LEN);
      context->loopbackClientContext->testResult.byteErrors++;
    }
  }

  CHPP_LOGI("Loopback client RX response. Error code=0x%" PRIx16
            ", response len=%" PRIuSIZE ", request len=%" PRIuSIZE
            ", first err=%" PRIuSIZE ", total err=%" PRIuSIZE,
            context->loopbackClientContext->testResult.error,
            context->loopbackClientContext->testResult.responseLen,
            context->loopbackClientContext->testResult.requestLen,
            context->loopbackClientContext->testResult.firstError,
            context->loopbackClientContext->testResult.byteErrors);

  // Notify waiting (synchronous) client
  chppMutexLock(&context->loopbackClientContext->client.responseMutex);
  context->loopbackClientContext->client.responseReady = true;
  chppConditionVariableSignal(
      &context->loopbackClientContext->client.responseCondVar);
  chppMutexUnlock(&context->loopbackClientContext->client.responseMutex);

  return true;
}

struct ChppLoopbackTestResult chppRunLoopbackTest(struct ChppAppState *context,
                                                  const uint8_t *buf,
                                                  size_t len) {
  CHPP_LOGI("Loopback test. payload len=%" PRIuSIZE ", request len=%" PRIuSIZE,
            len, len + CHPP_LOOPBACK_HEADER_LEN);

  if (!chppWaitForDiscoveryComplete(context, 0 /* timeoutMs */)) {
    static const struct ChppLoopbackTestResult result = {
        .error = CHPP_APP_ERROR_NOT_READY,
    };
    return result;
  }

  CHPP_NOT_NULL(context->loopbackClientContext);
  if (context->loopbackClientContext->testResult.error ==
      CHPP_APP_ERROR_BLOCKED) {
    CHPP_LOGE("Loopback test cannot be run while another is in progress");
    CHPP_DEBUG_ASSERT(false);

  } else {
    context->loopbackClientContext->testResult.error = CHPP_APP_ERROR_BLOCKED;
    context->loopbackClientContext->testResult.requestLen =
        len + CHPP_LOOPBACK_HEADER_LEN;
    context->loopbackClientContext->testResult.responseLen = 0;
    context->loopbackClientContext->testResult.firstError = 0;
    context->loopbackClientContext->testResult.byteErrors = 0;
    context->loopbackClientContext->testResult.rttNs = 0;
    context->loopbackClientContext->runLoopbackTest.requestTimeNs =
        CHPP_TIME_NONE;
    context->loopbackClientContext->runLoopbackTest.responseTimeNs =
        CHPP_TIME_NONE;

    if (len == 0) {
      CHPP_LOGE("Loopback payload too short (0)");
      context->loopbackClientContext->testResult.error =
          CHPP_APP_ERROR_INVALID_LENGTH;

    } else {
      uint8_t *loopbackRequest = (uint8_t *)chppAllocClientRequest(
          &context->loopbackClientContext->client,
          context->loopbackClientContext->testResult.requestLen);

      if (loopbackRequest == NULL) {
        // OOM
        context->loopbackClientContext->testResult.requestLen = 0;
        context->loopbackClientContext->testResult.error = CHPP_APP_ERROR_OOM;
        CHPP_LOG_OOM();

      } else {
        context->loopbackClientContext->loopbackData = buf;
        memcpy(&loopbackRequest[CHPP_LOOPBACK_HEADER_LEN], buf, len);

        if (!chppSendTimestampedRequestAndWaitTimeout(
                &context->loopbackClientContext->client,
                &context->loopbackClientContext->runLoopbackTest,
                loopbackRequest,
                context->loopbackClientContext->testResult.requestLen,
                CHPP_NSEC_PER_SEC /* 1s */)) {
          context->loopbackClientContext->testResult.error =
              CHPP_APP_ERROR_UNSPECIFIED;
        }  // else {context->loopbackClientContext->testResult is now populated}
      }
    }
  }

  return context->loopbackClientContext->testResult;
}
