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

#include "chpp/clients/wwan.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"
#include "chpp/clients/discovery.h"
#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
#include "chpp/clients/timesync.h"
#endif
#include "chpp/common/standard_uuids.h"
#include "chpp/common/wwan.h"
#include "chpp/common/wwan_types.h"
#include "chpp/log.h"
#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chre/pal/wwan.h"

#ifndef CHPP_WWAN_DISCOVERY_TIMEOUT_MS
#define CHPP_WWAN_DISCOVERY_TIMEOUT_MS CHPP_DISCOVERY_DEFAULT_TIMEOUT_MS
#endif

#ifndef CHPP_WWAN_MAX_TIMESYNC_AGE_NS
#define CHPP_WWAN_MAX_TIMESYNC_AGE_NS CHPP_TIMESYNC_DEFAULT_MAX_AGE_NS
#endif

/************************************************
 *  Prototypes
 ***********************************************/

static enum ChppAppErrorCode chppDispatchWwanResponse(void *clientContext,
                                                      uint8_t *buf, size_t len);
static bool chppWwanClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion);
static void chppWwanClientDeinit(void *clientContext);
static void chppWwanClientNotifyReset(void *clientContext);
static void chppWwanClientNotifyMatch(void *clientContext);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Structure to maintain state for the WWAN client and its Request/Response
 * (RR) functionality.
 */
struct ChppWwanClientState {
  struct ChppClientState client;     // WWAN client state
  const struct chrePalWwanApi *api;  // WWAN PAL API

  struct ChppRequestResponseState rRState[CHPP_WWAN_CLIENT_REQUEST_MAX + 1];

  uint32_t capabilities;  // Cached GetCapabilities result
};

// Note: This global definition of gWwanClientContext supports only one
// instance of the CHPP WWAN client at a time.
struct ChppWwanClientState gWwanClientContext;
static const struct chrePalSystemApi *gSystemApi;
static const struct chrePalWwanCallbacks *gCallbacks;

/**
 * Configuration parameters for this client
 */
static const struct ChppClient kWwanClientConfig = {
    .descriptor.uuid = CHPP_UUID_WWAN_STANDARD,

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Notifies client if CHPP is reset
    .resetNotifierFunctionPtr = &chppWwanClientNotifyReset,

    // Notifies client if they are matched to a service
    .matchNotifierFunctionPtr = &chppWwanClientNotifyMatch,

    // Service response dispatch function pointer
    .responseDispatchFunctionPtr = &chppDispatchWwanResponse,

    // Service notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

    // Service response dispatch function pointer
    .initFunctionPtr = &chppWwanClientInit,

    // Service notification dispatch function pointer
    .deinitFunctionPtr = &chppWwanClientDeinit,

    // Number of request-response states in the rRStates array.
    .rRStateCount = ARRAY_SIZE(gWwanClientContext.rRState),

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppWwanClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWwanCallbacks *callbacks);
static void chppWwanClientClose(void);
static uint32_t chppWwanClientGetCapabilities(void);
static bool chppWwanClientGetCellInfoAsync(void);
static void chppWwanClientReleaseCellInfoResult(
    struct chreWwanCellInfoResult *result);

static void chppWwanCloseResult(struct ChppWwanClientState *clientContext,
                                uint8_t *buf, size_t len);
static void chppWwanGetCapabilitiesResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len);
static void chppWwanGetCellInfoAsyncResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a service response from the transport layer that is determined to
 * be for the WWAN client.
 *
 * This function is called from the app layer using its function pointer given
 * during client registration.
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppDispatchWwanResponse(void *clientContext,
                                                      uint8_t *buf,
                                                      size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (rxHeader->command > CHPP_WWAN_CLIENT_REQUEST_MAX) {
    error = CHPP_APP_ERROR_INVALID_COMMAND;

  } else if (!chppClientTimestampResponse(
                 &wwanClientContext->client,
                 &wwanClientContext->rRState[rxHeader->command], rxHeader)) {
    error = CHPP_APP_ERROR_UNEXPECTED_RESPONSE;

  } else {
    switch (rxHeader->command) {
      case CHPP_WWAN_OPEN: {
        chppClientProcessOpenResponse(&wwanClientContext->client, buf, len);
        break;
      }

      case CHPP_WWAN_CLOSE: {
        chppWwanCloseResult(wwanClientContext, buf, len);
        break;
      }

      case CHPP_WWAN_GET_CAPABILITIES: {
        chppWwanGetCapabilitiesResult(wwanClientContext, buf, len);
        break;
      }

      case CHPP_WWAN_GET_CELLINFO_ASYNC: {
        chppWwanGetCellInfoAsyncResult(wwanClientContext, buf, len);
        break;
      }

      default: {
        error = CHPP_APP_ERROR_INVALID_COMMAND;
        break;
      }
    }
  }

  return error;
}

/**
 * Initializes the client and provides its handle number and the version of the
 * matched service when/if it the client is matched with a service during
 * discovery.
 *
 * @param clientContext Maintains status for each client instance.
 * @param handle Handle number for this client.
 * @param serviceVersion Version of the matched service.
 *
 * @return True if client is compatible and successfully initialized.
 */
static bool chppWwanClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion) {
  UNUSED_VAR(serviceVersion);

  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  chppClientInit(&wwanClientContext->client, handle);

  return true;
}

/**
 * Deinitializes the client.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWwanClientDeinit(void *clientContext) {
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  chppClientDeinit(&wwanClientContext->client);
}

/**
 * Notifies the client of an incoming reset.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWwanClientNotifyReset(void *clientContext) {
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;

  chppClientCloseOpenRequests(&wwanClientContext->client, &kWwanClientConfig,
                              false /* clearOnly */);

  if (wwanClientContext->client.openState != CHPP_OPEN_STATE_OPENED &&
      !wwanClientContext->client.pseudoOpen) {
    CHPP_LOGW("WWAN client reset but wasn't open");
  } else {
    CHPP_LOGI("WWAN client reopening from state=%" PRIu8,
              wwanClientContext->client.openState);
    chppClientSendOpenRequest(&wwanClientContext->client,
                              &wwanClientContext->rRState[CHPP_WWAN_OPEN],
                              CHPP_WWAN_OPEN,
                              /*blocking=*/false);
  }
}

/**
 * Notifies the client of being matched to a service.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWwanClientNotifyMatch(void *clientContext) {
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;

  if (wwanClientContext->client.pseudoOpen) {
    CHPP_LOGD("Pseudo-open WWAN client opening");
    chppClientSendOpenRequest(&wwanClientContext->client,
                              &wwanClientContext->rRState[CHPP_WWAN_OPEN],
                              CHPP_WWAN_OPEN,
                              /*blocking=*/false);
  }
}

/**
 * Handles the service response for the close client request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanCloseResult(struct ChppWwanClientState *clientContext,
                                uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/**
 * Handles the service response for the get capabilities client request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanGetCapabilitiesResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len) {
  if (len < sizeof(struct ChppWwanGetCapabilitiesResponse)) {
    struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
    CHPP_LOGE("GetCapabilities resp. too short. err=%" PRIu8, rxHeader->error);

  } else {
    struct ChppWwanGetCapabilitiesParameters *result =
        &((struct ChppWwanGetCapabilitiesResponse *)buf)->params;

    CHPP_LOGD("chppWwanGetCapabilitiesResult received capabilities=0x%" PRIx32,
              result->capabilities);

#ifdef CHPP_WWAN_DEFAULT_CAPABILITIES
    CHPP_ASSERT_LOG((result->capabilities == CHPP_WWAN_DEFAULT_CAPABILITIES),
                    "Unexpected capability 0x%" PRIx32 " != 0x%" PRIx32,
                    result->capabilities, CHPP_WWAN_DEFAULT_CAPABILITIES);
#endif

    clientContext->capabilities = result->capabilities;
  }
}

/**
 * Handles the service response for the asynchronous get cell info client
 * request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanGetCellInfoAsyncResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);
  CHPP_LOGD("chppWwanGetCellInfoAsyncResult received data len=%" PRIuSIZE, len);

  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct chreWwanCellInfoResult *chre = NULL;
  uint8_t errorCode = CHRE_ERROR;

  if (len == sizeof(struct ChppAppHeader)) {
    // Short response length indicates an error
    CHPP_LOGE("GetCellInfo resp. too short. err=%" PRIu8, rxHeader->error);

    if (rxHeader->error == CHPP_APP_ERROR_NONE) {
      errorCode = CHPP_APP_ERROR_INVALID_LENGTH;
    } else {
      errorCode = chppAppErrorToChreError(rxHeader->error);
    }

  } else {
    buf += sizeof(struct ChppAppHeader);
    len -= sizeof(struct ChppAppHeader);
    chre =
        chppWwanCellInfoResultToChre((struct ChppWwanCellInfoResult *)buf, len);

    if (chre == NULL) {
      CHPP_LOGE("Cell info conversion failed len=%" PRIuSIZE " err=%" PRIu8,
                len, rxHeader->error);
    }
  }

  if (chre == NULL) {
    chre = chppMalloc(sizeof(struct chreWwanCellInfoResult));
    if (chre == NULL) {
      CHPP_LOG_OOM();
    } else {
      chre->version = CHRE_WWAN_CELL_INFO_RESULT_VERSION;
      chre->errorCode = errorCode;
      chre->cellInfoCount = 0;
      chre->reserved = 0;
      chre->cookie = 0;
      chre->cells = NULL;
    }

  } else {
#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
    int64_t offset = chppTimesyncGetOffset(gWwanClientContext.client.appContext,
                                           CHPP_WWAN_MAX_TIMESYNC_AGE_NS);
    for (uint8_t i = 0; i < chre->cellInfoCount; i++) {
      uint64_t *timeStamp =
          (uint64_t *)(CHPP_CONST_CAST_POINTER(&chre->cells[i].timeStamp));
      *timeStamp -= (uint64_t)offset;
    }
#endif
  }

  if (chre != NULL) {
    gCallbacks->cellInfoResultCallback(chre);
  }
}

/**
 * Initializes the WWAN client upon an open request from CHRE and responds with
 * the result.
 *
 * @param systemApi CHRE system function pointers.
 * @param callbacks CHRE entry points.
 *
 * @return True if successful. False otherwise.
 */
static bool chppWwanClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWwanCallbacks *callbacks) {
  CHPP_DEBUG_ASSERT(systemApi != NULL);
  CHPP_DEBUG_ASSERT(callbacks != NULL);

  bool result = false;
  gSystemApi = systemApi;
  gCallbacks = callbacks;

  CHPP_LOGD("WWAN client opening");

  // Wait for discovery to complete for "open" call to succeed
  if (chppWaitForDiscoveryComplete(gWwanClientContext.client.appContext,
                                   CHPP_WWAN_DISCOVERY_TIMEOUT_MS)) {
    result = chppClientSendOpenRequest(
        &gWwanClientContext.client, &gWwanClientContext.rRState[CHPP_WWAN_OPEN],
        CHPP_WWAN_OPEN,
        /*blocking=*/true);
  }

#ifdef CHPP_WWAN_CLIENT_OPEN_ALWAYS_SUCCESS
  chppClientPseudoOpen(&gWwanClientContext.client);
  result = true;
#endif

  return result;
}

/**
 * Deinitializes the WWAN client.
 */
static void chppWwanClientClose(void) {
  // Remote
  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWwanClientContext.client, CHPP_WWAN_CLOSE);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else if (chppSendTimestampedRequestAndWait(
                 &gWwanClientContext.client,
                 &gWwanClientContext.rRState[CHPP_WWAN_CLOSE], request,
                 sizeof(*request))) {
    gWwanClientContext.client.openState = CHPP_OPEN_STATE_CLOSED;
    gWwanClientContext.capabilities = CHRE_WWAN_CAPABILITIES_NONE;
    chppClientCloseOpenRequests(&gWwanClientContext.client, &kWwanClientConfig,
                                true /* clearOnly */);
  }
}

/**
 * Retrieves a set of flags indicating the WWAN features supported by the
 * current implementation.
 *
 * @return Capabilities flags.
 */
static uint32_t chppWwanClientGetCapabilities(void) {
#ifdef CHPP_WWAN_DEFAULT_CAPABILITIES
  uint32_t capabilities = CHPP_WWAN_DEFAULT_CAPABILITIES;
#else
  uint32_t capabilities = CHRE_WWAN_CAPABILITIES_NONE;
#endif

  if (gWwanClientContext.capabilities != CHRE_WWAN_CAPABILITIES_NONE) {
    // Result already cached
    capabilities = gWwanClientContext.capabilities;

  } else {
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWwanClientContext.client, CHPP_WWAN_GET_CAPABILITIES);

    if (request == NULL) {
      CHPP_LOG_OOM();
    } else {
      if (chppSendTimestampedRequestAndWait(
              &gWwanClientContext.client,
              &gWwanClientContext.rRState[CHPP_WWAN_GET_CAPABILITIES], request,
              sizeof(*request))) {
        // Success. gWwanClientContext.capabilities is now populated
        capabilities = gWwanClientContext.capabilities;
      }
    }
  }

  return capabilities;
}

/**
 * Query information about the current serving cell and its neighbors. This does
 * not perform a network scan, but should return state from the current network
 * registration data stored in the cellular modem.
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWwanClientGetCellInfoAsync(void) {
  bool result = false;

  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWwanClientContext.client, CHPP_WWAN_GET_CELLINFO_ASYNC);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else {
    result = chppSendTimestampedRequestOrFail(
        &gWwanClientContext.client,
        &gWwanClientContext.rRState[CHPP_WWAN_GET_CELLINFO_ASYNC], request,
        sizeof(*request), CHPP_CLIENT_REQUEST_TIMEOUT_DEFAULT);
  }

  return result;
}

/**
 * Releases the memory held for the GetCellInfoAsync result.
 */
static void chppWwanClientReleaseCellInfoResult(
    struct chreWwanCellInfoResult *result) {
  if (result->cellInfoCount > 0) {
    void *cells = CHPP_CONST_CAST_POINTER(result->cells);
    CHPP_FREE_AND_NULLIFY(cells);
  }

  CHPP_FREE_AND_NULLIFY(result);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWwanClient(struct ChppAppState *appContext) {
  chppRegisterClient(appContext, (void *)&gWwanClientContext,
                     &gWwanClientContext.client, gWwanClientContext.rRState,
                     &kWwanClientConfig);
}

void chppDeregisterWwanClient(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}

struct ChppClientState *getChppWwanClientState(void) {
  return &gWwanClientContext.client;
}

#ifdef CHPP_CLIENT_ENABLED_WWAN

#ifdef CHPP_CLIENT_ENABLED_CHRE_WWAN
const struct chrePalWwanApi *chrePalWwanGetApi(uint32_t requestedApiVersion) {
#else
const struct chrePalWwanApi *chppPalWwanGetApi(uint32_t requestedApiVersion) {
#endif

  static const struct chrePalWwanApi api = {
      .moduleVersion = CHPP_PAL_WWAN_API_VERSION,
      .open = chppWwanClientOpen,
      .close = chppWwanClientClose,
      .getCapabilities = chppWwanClientGetCapabilities,
      .requestCellInfo = chppWwanClientGetCellInfoAsync,
      .releaseCellInfoResult = chppWwanClientReleaseCellInfoResult,
  };

  CHPP_STATIC_ASSERT(
      CHRE_PAL_WWAN_API_CURRENT_VERSION == CHPP_PAL_WWAN_API_VERSION,
      "A newer CHRE PAL API version is available. Please update.");

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(api.moduleVersion,
                                        requestedApiVersion)) {
    return NULL;
  } else {
    return &api;
  }
}

#endif
