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

#include "chpp/services.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"
#include "chpp/log.h"
#include "chpp/memory.h"
#ifdef CHPP_SERVICE_ENABLED_GNSS
#include "chpp/services/gnss.h"
#endif
#ifdef CHPP_SERVICE_ENABLED_WIFI
#include "chpp/services/wifi.h"
#endif
#ifdef CHPP_SERVICE_ENABLED_WWAN
#include "chpp/services/wwan.h"
#endif
#include "chpp/time.h"
#include "chpp/transport.h"

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterCommonServices(struct ChppAppState *context) {
  UNUSED_VAR(context);

#ifdef CHPP_SERVICE_ENABLED_WWAN
  if (context->clientServiceSet.wwanService) {
    chppRegisterWwanService(context);
  }
#endif

#ifdef CHPP_SERVICE_ENABLED_WIFI
  if (context->clientServiceSet.wifiService) {
    chppRegisterWifiService(context);
  }
#endif

#ifdef CHPP_SERVICE_ENABLED_GNSS
  if (context->clientServiceSet.gnssService) {
    chppRegisterGnssService(context);
  }
#endif
}

void chppDeregisterCommonServices(struct ChppAppState *context) {
  UNUSED_VAR(context);

#ifdef CHPP_SERVICE_ENABLED_WWAN
  if (context->clientServiceSet.wwanService) {
    chppDeregisterWwanService(context);
  }
#endif

#ifdef CHPP_SERVICE_ENABLED_WIFI
  if (context->clientServiceSet.wifiService) {
    chppDeregisterWifiService(context);
  }
#endif

#ifdef CHPP_SERVICE_ENABLED_GNSS
  if (context->clientServiceSet.gnssService) {
    chppDeregisterGnssService(context);
  }
#endif
}

uint8_t chppRegisterService(struct ChppAppState *appContext,
                            void *serviceContext,
                            const struct ChppService *newService) {
  CHPP_NOT_NULL(newService);

  if (appContext->registeredServiceCount >= CHPP_MAX_REGISTERED_SERVICES) {
    CHPP_LOGE("Max services registered: # %" PRIu8,
              appContext->registeredServiceCount);
    return CHPP_HANDLE_NONE;

  } else {
    appContext->registeredServices[appContext->registeredServiceCount] =
        newService;
    appContext->registeredServiceContexts[appContext->registeredServiceCount] =
        serviceContext;

    char uuidText[CHPP_SERVICE_UUID_STRING_LEN];
    chppUuidToStr(newService->descriptor.uuid, uuidText);
    CHPP_LOGD("Registered service # %" PRIu8
              " on handle %d"
              " with name=%s, UUID=%s, version=%" PRIu8 ".%" PRIu8 ".%" PRIu16
              ", min_len=%" PRIuSIZE " ",
              appContext->registeredServiceCount,
              CHPP_SERVICE_HANDLE_OF_INDEX(appContext->registeredServiceCount),
              newService->descriptor.name, uuidText,
              newService->descriptor.version.major,
              newService->descriptor.version.minor,
              newService->descriptor.version.patch, newService->minLength);

    return CHPP_SERVICE_HANDLE_OF_INDEX(appContext->registeredServiceCount++);
  }
}

struct ChppAppHeader *chppAllocServiceNotification(size_t len) {
  CHPP_ASSERT(len >= sizeof(struct ChppAppHeader));

  struct ChppAppHeader *result = chppMalloc(len);
  if (result) {
    result->handle = CHPP_HANDLE_NONE;
    result->type = CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION;
    result->transaction = 0;
    result->error = CHPP_APP_ERROR_NONE;
    result->command = CHPP_APP_COMMAND_NONE;
  }
  return result;
}

struct ChppAppHeader *chppAllocServiceResponse(
    const struct ChppAppHeader *requestHeader, size_t len) {
  CHPP_ASSERT(len >= sizeof(struct ChppAppHeader));

  struct ChppAppHeader *result = chppMalloc(len);
  if (result) {
    *result = *requestHeader;
    result->type = CHPP_MESSAGE_TYPE_SERVICE_RESPONSE;
    result->error = CHPP_APP_ERROR_NONE;
  }
  return result;
}

void chppServiceTimestampRequest(struct ChppRequestResponseState *rRState,
                                 struct ChppAppHeader *requestHeader) {
  if (rRState->responseTimeNs == CHPP_TIME_NONE &&
      rRState->requestTimeNs != CHPP_TIME_NONE) {
    CHPP_LOGE("Duplicate request rx at t=%" PRIu64,
              rRState->requestTimeNs / CHPP_NSEC_PER_MSEC);
  }
  rRState->requestTimeNs = chppGetCurrentTimeNs();
  rRState->responseTimeNs = CHPP_TIME_NONE;
  rRState->transaction = requestHeader->transaction;
}

void chppServiceTimestampResponse(struct ChppRequestResponseState *rRState) {
  uint64_t previousResponseTime = rRState->responseTimeNs;
  rRState->responseTimeNs = chppGetCurrentTimeNs();

  if (rRState->requestTimeNs == CHPP_TIME_NONE) {
    CHPP_LOGE("Tx response w/ no req t=%" PRIu64,
              rRState->responseTimeNs / CHPP_NSEC_PER_MSEC);

  } else if (previousResponseTime != CHPP_TIME_NONE) {
    CHPP_LOGW("Resend response t=%" PRIu64 " for request at t=%" PRIu64,
              rRState->responseTimeNs / CHPP_NSEC_PER_MSEC,
              rRState->responseTimeNs / CHPP_NSEC_PER_MSEC);

  } else {
    CHPP_LOGD("Sending initial response at t=%" PRIu64
              " for request at t=%" PRIu64 " (RTT=%" PRIu64 ")",
              rRState->responseTimeNs / CHPP_NSEC_PER_MSEC,
              rRState->responseTimeNs / CHPP_NSEC_PER_MSEC,
              (rRState->responseTimeNs - rRState->requestTimeNs) /
                  CHPP_NSEC_PER_MSEC);
  }
}

bool chppSendTimestampedResponseOrFail(struct ChppServiceState *serviceState,
                                       struct ChppRequestResponseState *rRState,
                                       void *buf, size_t len) {
  chppServiceTimestampResponse(rRState);
  return chppEnqueueTxDatagramOrFail(serviceState->appContext->transportContext,
                                     buf, len);
}
