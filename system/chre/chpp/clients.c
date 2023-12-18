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

#include "chpp/clients.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"
#ifdef CHPP_CLIENT_ENABLED_DISCOVERY
#include "chpp/clients/discovery.h"
#endif
#ifdef CHPP_CLIENT_ENABLED_GNSS
#include "chpp/clients/gnss.h"
#endif
#ifdef CHPP_CLIENT_ENABLED_LOOPBACK
#include "chpp/clients/loopback.h"
#endif
#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
#include "chpp/clients/timesync.h"
#endif
#ifdef CHPP_CLIENT_ENABLED_WIFI
#include "chpp/clients/wifi.h"
#endif
#ifdef CHPP_CLIENT_ENABLED_WWAN
#include "chpp/clients/wwan.h"
#endif
#include "chpp/log.h"
#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chpp/time.h"
#include "chpp/transport.h"

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppIsClientApiReady(struct ChppClientState *clientState);
ChppClientDeinitFunction *chppGetClientDeinitFunction(
    struct ChppAppState *context, uint8_t index);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Determines whether a client is ready to accept commands via its API (i.e. is
 * initialized and opened). If the client is in the process of reopening, it
 * will wait for the client to reopen.
 *
 * @param clientState State of the client sending the client request.
 *
 * @return Indicates whetherthe client is ready.
 */
static bool chppIsClientApiReady(struct ChppClientState *clientState) {
  bool result = false;

  if (clientState->initialized) {
    switch (clientState->openState) {
      case (CHPP_OPEN_STATE_CLOSED):
      case (CHPP_OPEN_STATE_WAITING_TO_OPEN): {
        // result remains false
        break;
      }

      case (CHPP_OPEN_STATE_OPENED): {
        result = true;
        break;
      }

      case (CHPP_OPEN_STATE_OPENING): {
        // Allow the open request to go through
        clientState->openState = CHPP_OPEN_STATE_WAITING_TO_OPEN;
        result = true;
        break;
      }
    }
  }

  if (!result) {
    CHPP_LOGE("Client not ready (everInit=%d, init=%d, open=%" PRIu8 ")",
              clientState->everInitialized, clientState->initialized,
              clientState->openState);
  }
  return result;
}

/**
 * Returns the deinitialization function pointer of a particular negotiated
 * client.
 *
 * @param context Maintains status for each app layer instance.
 * @param index Index of the registered client.
 *
 * @return Pointer to the match notification function.
 */
ChppClientDeinitFunction *chppGetClientDeinitFunction(
    struct ChppAppState *context, uint8_t index) {
  return context->registeredClients[index]->deinitFunctionPtr;
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterCommonClients(struct ChppAppState *context) {
  UNUSED_VAR(context);
  CHPP_LOGD("Registering Clients");

#ifdef CHPP_CLIENT_ENABLED_WWAN
  if (context->clientServiceSet.wwanClient) {
    chppRegisterWwanClient(context);
  }
#endif

#ifdef CHPP_CLIENT_ENABLED_WIFI
  if (context->clientServiceSet.wifiClient) {
    chppRegisterWifiClient(context);
  }
#endif

#ifdef CHPP_CLIENT_ENABLED_GNSS
  if (context->clientServiceSet.gnssClient) {
    chppRegisterGnssClient(context);
  }
#endif
}

void chppDeregisterCommonClients(struct ChppAppState *context) {
  UNUSED_VAR(context);
  CHPP_LOGD("Deregistering Clients");

#ifdef CHPP_CLIENT_ENABLED_WWAN
  if (context->clientServiceSet.wwanClient) {
    chppDeregisterWwanClient(context);
  }
#endif

#ifdef CHPP_CLIENT_ENABLED_WIFI
  if (context->clientServiceSet.wifiClient) {
    chppDeregisterWifiClient(context);
  }
#endif

#ifdef CHPP_CLIENT_ENABLED_GNSS
  if (context->clientServiceSet.gnssClient) {
    chppDeregisterGnssClient(context);
  }
#endif
}

void chppRegisterClient(struct ChppAppState *appContext, void *clientContext,
                        struct ChppClientState *clientState,
                        struct ChppRequestResponseState *rRStates,
                        const struct ChppClient *newClient) {
  CHPP_NOT_NULL(newClient);

  if (appContext->registeredClientCount >= CHPP_MAX_REGISTERED_CLIENTS) {
    CHPP_LOGE("Max clients registered: %" PRIu8,
              appContext->registeredClientCount);

  } else {
    clientState->appContext = appContext;
    clientState->rRStates = rRStates;
    clientState->index = appContext->registeredClientCount;

    appContext->registeredClientContexts[appContext->registeredClientCount] =
        clientContext;
    appContext->registeredClientStates[appContext->registeredClientCount] =
        clientState;
    appContext->registeredClients[appContext->registeredClientCount] =
        newClient;

    char uuidText[CHPP_SERVICE_UUID_STRING_LEN];
    chppUuidToStr(newClient->descriptor.uuid, uuidText);
    CHPP_LOGD("Client # %" PRIu8 " UUID=%s, version=%" PRIu8 ".%" PRIu8
              ".%" PRIu16 ", min_len=%" PRIuSIZE,
              appContext->registeredClientCount, uuidText,
              newClient->descriptor.version.major,
              newClient->descriptor.version.minor,
              newClient->descriptor.version.patch, newClient->minLength);

    appContext->registeredClientCount++;
  }
}

void chppInitBasicClients(struct ChppAppState *context) {
  UNUSED_VAR(context);
  CHPP_LOGD("Initializing basic clients");

#ifdef CHPP_CLIENT_ENABLED_LOOPBACK
  if (context->clientServiceSet.loopbackClient) {
    chppLoopbackClientInit(context);
  }
#endif

#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
  chppTimesyncClientInit(context);
#endif

#ifdef CHPP_CLIENT_ENABLED_DISCOVERY
  chppDiscoveryInit(context);
#endif
}

void chppClientInit(struct ChppClientState *clientState, uint8_t handle) {
  CHPP_ASSERT_LOG(!clientState->initialized,
                  "Client H#%" PRIu8 " already initialized", handle);

  if (!clientState->everInitialized) {
    clientState->handle = handle;
    chppMutexInit(&clientState->responseMutex);
    chppConditionVariableInit(&clientState->responseCondVar);
    clientState->everInitialized = true;
  }

  clientState->initialized = true;
}

void chppClientDeinit(struct ChppClientState *clientState) {
  CHPP_ASSERT_LOG(clientState->initialized,
                  "Client H#%" PRIu8 " already deinitialized",
                  clientState->handle);

  clientState->initialized = false;
}

void chppDeinitBasicClients(struct ChppAppState *context) {
  UNUSED_VAR(context);
  CHPP_LOGD("Deinitializing basic clients");

#ifdef CHPP_CLIENT_ENABLED_LOOPBACK
  if (context->clientServiceSet.loopbackClient) {
    chppLoopbackClientDeinit(context);
  }
#endif

#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
  chppTimesyncClientDeinit(context);
#endif

#ifdef CHPP_CLIENT_ENABLED_DISCOVERY
  chppDiscoveryDeinit(context);
#endif
}

void chppDeinitMatchedClients(struct ChppAppState *context) {
  CHPP_LOGD("Deinitializing matched clients");

  for (uint8_t i = 0; i < context->discoveredServiceCount; i++) {
    uint8_t clientIndex = context->clientIndexOfServiceIndex[i];
    if (clientIndex != CHPP_CLIENT_INDEX_NONE) {
      // Discovered service has a matched client
      ChppClientDeinitFunction *clientDeinitFunction =
          chppGetClientDeinitFunction(context, clientIndex);

      CHPP_LOGD("Client #%" PRIu8 " (H#%d) deinit fp found=%d", clientIndex,
                CHPP_SERVICE_HANDLE_OF_INDEX(i),
                (clientDeinitFunction != NULL));

      if (clientDeinitFunction != NULL) {
        clientDeinitFunction(context->registeredClientContexts[clientIndex]);
      }
    }
  }
}

struct ChppAppHeader *chppAllocClientRequest(
    struct ChppClientState *clientState, size_t len) {
  CHPP_ASSERT(len >= CHPP_APP_MIN_LEN_HEADER_WITH_TRANSACTION);

  struct ChppAppHeader *result = chppMalloc(len);
  if (result != NULL) {
    result->handle = clientState->handle;
    result->type = CHPP_MESSAGE_TYPE_CLIENT_REQUEST;
    result->transaction = clientState->transaction;
    result->error = CHPP_APP_ERROR_NONE;
    result->command = CHPP_APP_COMMAND_NONE;

    clientState->transaction++;
  }
  return result;
}

struct ChppAppHeader *chppAllocClientRequestCommand(
    struct ChppClientState *clientState, uint16_t command) {
  struct ChppAppHeader *result =
      chppAllocClientRequest(clientState, sizeof(struct ChppAppHeader));

  if (result != NULL) {
    result->command = command;
  }
  return result;
}

void chppClientTimestampRequest(struct ChppClientState *clientState,
                                struct ChppRequestResponseState *rRState,
                                struct ChppAppHeader *requestHeader,
                                uint64_t timeoutNs) {
  if (rRState->requestState == CHPP_REQUEST_STATE_REQUEST_SENT) {
    CHPP_LOGE("Dupe req ID=%" PRIu8 " existing ID=%" PRIu8 " from t=%" PRIu64,
              requestHeader->transaction, rRState->transaction,
              rRState->requestTimeNs / CHPP_NSEC_PER_MSEC);

    // Clear a possible pending timeout from the previous request
    rRState->responseTimeNs = CHPP_TIME_MAX;
    chppClientRecalculateNextTimeout(clientState->appContext);
  }

  rRState->requestTimeNs = chppGetCurrentTimeNs();
  rRState->requestState = CHPP_REQUEST_STATE_REQUEST_SENT;
  rRState->transaction = requestHeader->transaction;

  if (timeoutNs == CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE) {
    rRState->responseTimeNs = CHPP_TIME_MAX;

  } else {
    rRState->responseTimeNs = timeoutNs + rRState->requestTimeNs;

    clientState->appContext->nextRequestTimeoutNs = MIN(
        clientState->appContext->nextRequestTimeoutNs, rRState->responseTimeNs);
  }

  CHPP_LOGD("Timestamp req ID=%" PRIu8 " at t=%" PRIu64 " timeout=%" PRIu64
            " (requested=%" PRIu64 "), next timeout=%" PRIu64,
            rRState->transaction, rRState->requestTimeNs / CHPP_NSEC_PER_MSEC,
            rRState->responseTimeNs / CHPP_NSEC_PER_MSEC,
            timeoutNs / CHPP_NSEC_PER_MSEC,
            clientState->appContext->nextRequestTimeoutNs / CHPP_NSEC_PER_MSEC);
}

bool chppClientTimestampResponse(struct ChppClientState *clientState,
                                 struct ChppRequestResponseState *rRState,
                                 const struct ChppAppHeader *responseHeader) {
  bool success = false;
  uint64_t responseTime = chppGetCurrentTimeNs();

  switch (rRState->requestState) {
    case CHPP_REQUEST_STATE_NONE: {
      CHPP_LOGE("Resp with no req t=%" PRIu64,
                responseTime / CHPP_NSEC_PER_MSEC);
      break;
    }

    case CHPP_REQUEST_STATE_RESPONSE_RCV: {
      CHPP_LOGE("Extra resp at t=%" PRIu64 " for req t=%" PRIu64,
                responseTime / CHPP_NSEC_PER_MSEC,
                rRState->requestTimeNs / CHPP_NSEC_PER_MSEC);
      break;
    }

    case CHPP_REQUEST_STATE_RESPONSE_TIMEOUT: {
      CHPP_LOGE("Late resp at t=%" PRIu64 " for req t=%" PRIu64,
                responseTime / CHPP_NSEC_PER_MSEC,
                rRState->requestTimeNs / CHPP_NSEC_PER_MSEC);
      break;
    }

    case CHPP_REQUEST_STATE_REQUEST_SENT: {
      if (responseHeader->transaction != rRState->transaction) {
        CHPP_LOGE("Invalid resp ID=%" PRIu8 " at t=%" PRIu64
                  " expected=%" PRIu8,
                  responseHeader->transaction,
                  responseTime / CHPP_NSEC_PER_MSEC, rRState->transaction);
      } else {
        rRState->requestState = (responseTime > rRState->responseTimeNs)
                                    ? CHPP_REQUEST_STATE_RESPONSE_TIMEOUT
                                    : CHPP_REQUEST_STATE_RESPONSE_RCV;
        success = true;

        CHPP_LOGD(
            "Timestamp resp ID=%" PRIu8 " req t=%" PRIu64 " resp t=%" PRIu64
            " timeout t=%" PRIu64 " (RTT=%" PRIu64 ", timeout = %s)",
            rRState->transaction, rRState->requestTimeNs / CHPP_NSEC_PER_MSEC,
            responseTime / CHPP_NSEC_PER_MSEC,
            rRState->responseTimeNs / CHPP_NSEC_PER_MSEC,
            (responseTime - rRState->requestTimeNs) / CHPP_NSEC_PER_MSEC,
            (responseTime > rRState->responseTimeNs) ? "yes" : "no");
      }
      break;
    }

    default: {
      CHPP_DEBUG_ASSERT(false);
    }
  }

  if (success) {
    if (rRState->responseTimeNs ==
        clientState->appContext->nextRequestTimeoutNs) {
      // This was the next upcoming timeout
      chppClientRecalculateNextTimeout(clientState->appContext);
    }
    rRState->responseTimeNs = responseTime;
  }
  return success;
}

bool chppSendTimestampedRequestOrFail(struct ChppClientState *clientState,
                                      struct ChppRequestResponseState *rRState,
                                      void *buf, size_t len,
                                      uint64_t timeoutNs) {
  CHPP_ASSERT(len >= CHPP_APP_MIN_LEN_HEADER_WITH_TRANSACTION);
  if (!chppIsClientApiReady(clientState)) {
    CHPP_FREE_AND_NULLIFY(buf);
    return false;
  }

  chppClientTimestampRequest(clientState, rRState, buf, timeoutNs);
  clientState->responseReady = false;

  bool success = chppEnqueueTxDatagramOrFail(
      clientState->appContext->transportContext, buf, len);

  // Failure to enqueue a TX datagram means that a request was known to be not
  // transmitted. We explicitly set requestState to be in the NONE state, so
  // that unintended app layer timeouts do not occur.
  if (!success) {
    rRState->requestState = CHPP_REQUEST_STATE_NONE;
  }

  return success;
}

bool chppSendTimestampedRequestAndWait(struct ChppClientState *clientState,
                                       struct ChppRequestResponseState *rRState,
                                       void *buf, size_t len) {
  return chppSendTimestampedRequestAndWaitTimeout(
      clientState, rRState, buf, len, CHPP_CLIENT_REQUEST_TIMEOUT_DEFAULT);
}

bool chppSendTimestampedRequestAndWaitTimeout(
    struct ChppClientState *clientState,
    struct ChppRequestResponseState *rRState, void *buf, size_t len,
    uint64_t timeoutNs) {
  bool result = chppSendTimestampedRequestOrFail(
      clientState, rRState, buf, len, CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE);

  if (result) {
    chppMutexLock(&clientState->responseMutex);

    while (result && !clientState->responseReady) {
      result = chppConditionVariableTimedWait(&clientState->responseCondVar,
                                              &clientState->responseMutex,
                                              timeoutNs);
    }
    if (!clientState->responseReady) {
      rRState->requestState = CHPP_REQUEST_STATE_RESPONSE_TIMEOUT;
      CHPP_LOGE("Response timeout after %" PRIu64 " ms",
                timeoutNs / CHPP_NSEC_PER_MSEC);
      result = false;
    }

    chppMutexUnlock(&clientState->responseMutex);
  }

  return result;
}

void chppClientPseudoOpen(struct ChppClientState *clientState) {
  clientState->pseudoOpen = true;
}

bool chppClientSendOpenRequest(struct ChppClientState *clientState,
                               struct ChppRequestResponseState *openRRState,
                               uint16_t openCommand, bool blocking) {
  bool result = false;
  uint8_t priorState = clientState->openState;

#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
  chppTimesyncMeasureOffset(clientState->appContext);
#endif

  struct ChppAppHeader *request =
      chppAllocClientRequestCommand(clientState, openCommand);

  if (request == NULL) {
    CHPP_LOG_OOM();

  } else {
    clientState->openState = CHPP_OPEN_STATE_OPENING;

    if (blocking) {
      CHPP_LOGD("Opening service - blocking");
      result = chppSendTimestampedRequestAndWait(clientState, openRRState,
                                                 request, sizeof(*request));
    } else {
      CHPP_LOGD("Opening service - non-blocking");
      result = chppSendTimestampedRequestOrFail(
          clientState, openRRState, request, sizeof(*request),
          CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE);
    }

    if (!result) {
      CHPP_LOGE("Service open fail from state=%" PRIu8 " psudo=%d blocking=%d",
                priorState, clientState->pseudoOpen, blocking);
      clientState->openState = CHPP_OPEN_STATE_CLOSED;

    } else if (blocking) {
      result = (clientState->openState == CHPP_OPEN_STATE_OPENED);
    }
  }

  return result;
}

void chppClientProcessOpenResponse(struct ChppClientState *clientState,
                                   uint8_t *buf, size_t len) {
  UNUSED_VAR(len);  // Necessary depending on assert macro below
  // Assert condition already guaranteed by chppAppProcessRxDatagram() but
  // checking again since this is a public function
  CHPP_ASSERT(len >= sizeof(struct ChppAppHeader));

  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  if (rxHeader->error != CHPP_APP_ERROR_NONE) {
    CHPP_LOGE("Service open failed at service");
    clientState->openState = CHPP_OPEN_STATE_CLOSED;
  } else {
    CHPP_LOGI("Service open succeeded at service");
    clientState->openState = CHPP_OPEN_STATE_OPENED;
  }
}

void chppClientRecalculateNextTimeout(struct ChppAppState *context) {
  context->nextRequestTimeoutNs = CHPP_TIME_MAX;

  for (uint8_t clientIdx = 0; clientIdx < context->registeredClientCount;
       clientIdx++) {
    for (uint16_t cmdIdx = 0;
         cmdIdx < context->registeredClients[clientIdx]->rRStateCount;
         cmdIdx++) {
      struct ChppRequestResponseState *rRState =
          &context->registeredClientStates[clientIdx]->rRStates[cmdIdx];

      if (rRState->requestState == CHPP_REQUEST_STATE_REQUEST_SENT) {
        context->nextRequestTimeoutNs =
            MIN(context->nextRequestTimeoutNs, rRState->responseTimeNs);
      }
    }
  }

  CHPP_LOGD("nextReqTimeout=%" PRIu64,
            context->nextRequestTimeoutNs / CHPP_NSEC_PER_MSEC);
}

void chppClientCloseOpenRequests(struct ChppClientState *clientState,
                                 const struct ChppClient *client,
                                 bool clearOnly) {
  bool recalcNeeded = false;

  for (uint16_t cmdIdx = 0; cmdIdx < client->rRStateCount; cmdIdx++) {
    if (clientState->rRStates[cmdIdx].requestState ==
        CHPP_REQUEST_STATE_REQUEST_SENT) {
      recalcNeeded = true;

      CHPP_LOGE("Closing open req #%" PRIu16 " clear %d", cmdIdx, clearOnly);

      if (clearOnly) {
        clientState->rRStates[cmdIdx].requestState =
            CHPP_REQUEST_STATE_RESPONSE_TIMEOUT;
      } else {
        struct ChppAppHeader *response =
            chppMalloc(sizeof(struct ChppAppHeader));
        if (response == NULL) {
          CHPP_LOG_OOM();
        } else {
          response->handle = clientState->handle;
          response->type = CHPP_MESSAGE_TYPE_SERVICE_RESPONSE;
          response->transaction = clientState->rRStates[cmdIdx].transaction;
          response->error = CHPP_APP_ERROR_TIMEOUT;
          response->command = cmdIdx;

          chppAppProcessRxDatagram(clientState->appContext, (uint8_t *)response,
                                   sizeof(struct ChppAppHeader));
        }
      }
    }
  }

  if (recalcNeeded) {
    chppClientRecalculateNextTimeout(clientState->appContext);
  }
}
