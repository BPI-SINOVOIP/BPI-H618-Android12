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

#include "chpp/clients/timesync.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/app.h"
#include "chpp/clients.h"
#include "chpp/common/timesync.h"
#include "chpp/log.h"
#include "chpp/memory.h"
#include "chpp/time.h"
#include "chpp/transport.h"

#include "chpp/clients/discovery.h"

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Structure to maintain state for the Timesync client and its Request/Response
 * (RR) functionality.
 */
struct ChppTimesyncClientState {
  struct ChppClientState client;                  // Timesync client state
  struct ChppRequestResponseState measureOffset;  // Request response state

  struct ChppTimesyncResult timesyncResult;  // Result of measureOffset
};

/************************************************
 *  Public Functions
 ***********************************************/

void chppTimesyncClientInit(struct ChppAppState *context) {
  CHPP_LOGD("Timesync client init");

  context->timesyncClientContext =
      chppMalloc(sizeof(struct ChppTimesyncClientState));
  CHPP_NOT_NULL(context->timesyncClientContext);
  memset(context->timesyncClientContext, 0,
         sizeof(struct ChppTimesyncClientState));

  context->timesyncClientContext->client.appContext = context;
  context->timesyncClientContext->timesyncResult.error = CHPP_APP_ERROR_NONE;

  chppClientInit(&context->timesyncClientContext->client, CHPP_HANDLE_TIMESYNC);
  context->timesyncClientContext->timesyncResult.error =
      CHPP_APP_ERROR_UNSPECIFIED;
  context->timesyncClientContext->client.openState = CHPP_OPEN_STATE_OPENED;
}

void chppTimesyncClientDeinit(struct ChppAppState *context) {
  CHPP_LOGD("Timesync client deinit");

  CHPP_NOT_NULL(context->timesyncClientContext);
  chppClientDeinit(&context->timesyncClientContext->client);
  CHPP_FREE_AND_NULLIFY(context->timesyncClientContext);
}

void chppTimesyncClientReset(struct ChppAppState *context) {
  CHPP_LOGD("Timesync client reset");

  CHPP_NOT_NULL(context->timesyncClientContext);

  context->timesyncClientContext->timesyncResult.error = CHPP_APP_ERROR_NONE;
  context->timesyncClientContext->timesyncResult.offsetNs = 0;
  context->timesyncClientContext->timesyncResult.rttNs = 0;
  context->timesyncClientContext->timesyncResult.measurementTimeNs = 0;
}

bool chppDispatchTimesyncServiceResponse(struct ChppAppState *context,
                                         const uint8_t *buf, size_t len) {
  CHPP_LOGD("Timesync client dispatch service response");

  CHPP_NOT_NULL(context->timesyncClientContext);
  CHPP_NOT_NULL(buf);

  if (len < sizeof(struct ChppTimesyncResponse)) {
    CHPP_LOGE("Timesync resp short len=%" PRIuSIZE, len);
    context->timesyncClientContext->timesyncResult.error =
        CHPP_APP_ERROR_INVALID_LENGTH;
    return false;
  }

  const struct ChppTimesyncResponse *response =
      (const struct ChppTimesyncResponse *)buf;
  if (chppClientTimestampResponse(
          &context->timesyncClientContext->client,
          &context->timesyncClientContext->measureOffset, &response->header)) {
    context->timesyncClientContext->timesyncResult.rttNs =
        context->timesyncClientContext->measureOffset.responseTimeNs -
        context->timesyncClientContext->measureOffset.requestTimeNs;
    int64_t offsetNs =
        (int64_t)(response->timeNs -
                  context->timesyncClientContext->measureOffset.responseTimeNs);
    int64_t offsetChangeNs =
        offsetNs - context->timesyncClientContext->timesyncResult.offsetNs;

    int64_t clippedOffsetChangeNs = offsetChangeNs;
    if (context->timesyncClientContext->timesyncResult.offsetNs != 0) {
      clippedOffsetChangeNs = MIN(clippedOffsetChangeNs,
                                  (int64_t)CHPP_CLIENT_TIMESYNC_MAX_CHANGE_NS);
      clippedOffsetChangeNs = MAX(clippedOffsetChangeNs,
                                  -(int64_t)CHPP_CLIENT_TIMESYNC_MAX_CHANGE_NS);
    }

    context->timesyncClientContext->timesyncResult.offsetNs +=
        clippedOffsetChangeNs;

    if (offsetChangeNs != clippedOffsetChangeNs) {
      CHPP_LOGW("Drift=%" PRId64 " clipped to %" PRId64 " at t=%" PRIu64,
                offsetChangeNs / (int64_t)CHPP_NSEC_PER_MSEC,
                clippedOffsetChangeNs / (int64_t)CHPP_NSEC_PER_MSEC,
                context->timesyncClientContext->measureOffset.responseTimeNs /
                    CHPP_NSEC_PER_MSEC);
    } else {
      context->timesyncClientContext->timesyncResult.measurementTimeNs =
          context->timesyncClientContext->measureOffset.responseTimeNs;
    }

    context->timesyncClientContext->timesyncResult.error = CHPP_APP_ERROR_NONE;

    CHPP_LOGI("Timesync RTT=%" PRIu64 " correction=%" PRId64 " offset=%" PRId64
              " t=%" PRIu64,
              context->timesyncClientContext->timesyncResult.rttNs /
                  CHPP_NSEC_PER_MSEC,
              clippedOffsetChangeNs / (int64_t)CHPP_NSEC_PER_MSEC,
              offsetNs / (int64_t)CHPP_NSEC_PER_MSEC,
              context->timesyncClientContext->timesyncResult.measurementTimeNs /
                  CHPP_NSEC_PER_MSEC);
  }

  return true;
}

bool chppTimesyncMeasureOffset(struct ChppAppState *context) {
  bool result = false;
  CHPP_LOGI("Measuring timesync t=%" PRIu64,
            chppGetCurrentTimeNs() / CHPP_NSEC_PER_MSEC);

  CHPP_NOT_NULL(context->timesyncClientContext);

  context->timesyncClientContext->timesyncResult.error =
      CHPP_APP_ERROR_BUSY;  // A measurement is in progress

  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &context->timesyncClientContext->client, CHPP_TIMESYNC_COMMAND_GETTIME);
  size_t requestLen = sizeof(*request);

  if (request == NULL) {
    context->timesyncClientContext->timesyncResult.error = CHPP_APP_ERROR_OOM;
    CHPP_LOG_OOM();

  } else if (!chppSendTimestampedRequestOrFail(
                 &context->timesyncClientContext->client,
                 &context->timesyncClientContext->measureOffset, request,
                 requestLen, CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE)) {
    context->timesyncClientContext->timesyncResult.error =
        CHPP_APP_ERROR_UNSPECIFIED;

  } else {
    result = true;
  }

  return result;
}

int64_t chppTimesyncGetOffset(struct ChppAppState *context,
                              uint64_t maxTimesyncAgeNs) {
  bool timesyncNeverDone =
      (context->timesyncClientContext->timesyncResult.offsetNs == 0);
  bool timesyncIsStale =
      (chppGetCurrentTimeNs() -
           context->timesyncClientContext->timesyncResult.measurementTimeNs >
       maxTimesyncAgeNs);

  if (timesyncNeverDone || timesyncIsStale) {
    chppTimesyncMeasureOffset(context);
  } else {
    CHPP_LOGD("No need to timesync at t~=%" PRIu64 "offset=%" PRId64,
              chppGetCurrentTimeNs() / CHPP_NSEC_PER_MSEC,
              context->timesyncClientContext->timesyncResult.offsetNs /
                  (int64_t)CHPP_NSEC_PER_MSEC);
  }

  return context->timesyncClientContext->timesyncResult.offsetNs;
}

const struct ChppTimesyncResult *chppTimesyncGetResult(
    struct ChppAppState *context) {
  return &context->timesyncClientContext->timesyncResult;
}
