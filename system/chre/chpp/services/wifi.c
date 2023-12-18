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

#include "chpp/services/wifi.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/common/standard_uuids.h"
#include "chpp/common/wifi.h"
#include "chpp/common/wifi_types.h"
#include "chpp/common/wifi_utils.h"
#include "chpp/log.h"
#include "chpp/macros.h"
#include "chpp/services.h"
#include "chre/pal/wifi.h"

/************************************************
 *  Prototypes
 ***********************************************/

static enum ChppAppErrorCode chppDispatchWifiRequest(void *serviceContext,
                                                     uint8_t *buf, size_t len);
static void chppWifiServiceNotifyReset(void *serviceContext);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this service
 */
static const struct ChppService kWifiServiceConfig = {
    .descriptor.uuid = CHPP_UUID_WIFI_STANDARD,

    // Human-readable name
    .descriptor.name = "WiFi",

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Notifies service if CHPP is reset
    .resetNotifierFunctionPtr = &chppWifiServiceNotifyReset,

    // Client request dispatch function pointer
    .requestDispatchFunctionPtr = &chppDispatchWifiRequest,

    // Client notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the WiFi service and its Request/Response
 * (RR) functionality.
 */
struct ChppWifiServiceState {
  struct ChppServiceState service;   // WiFi service state
  const struct chrePalWifiApi *api;  // WiFi PAL API

  // Based on chre/pal/wifi.h and chrePalWifiApi
  struct ChppRequestResponseState open;             // Service init state
  struct ChppRequestResponseState close;            // Service deinit state
  struct ChppRequestResponseState getCapabilities;  // Get Capabilities state
  struct ChppRequestResponseState
      configureScanMonitorAsync;  // Configure scan monitor state
  struct ChppRequestResponseState requestScanAsync;     // Request scan state
  struct ChppRequestResponseState requestRangingAsync;  // Request ranging state
};

// Note: The CHRE PAL API only allows for one definition - see comment in WWAN
// service for details.
// Note: There is no notion of a cookie in the CHRE WiFi API so we need to use
// the global service state (gWifiServiceContext) directly in all callbacks.
struct ChppWifiServiceState gWifiServiceContext;

/************************************************
 *  Prototypes
 ***********************************************/

static enum ChppAppErrorCode chppWifiServiceOpen(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader);
static enum ChppAppErrorCode chppWifiServiceClose(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader);
static enum ChppAppErrorCode chppWifiServiceGetCapabilities(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader);
static enum ChppAppErrorCode chppWifiServiceConfigureScanMonitorAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);
static enum ChppAppErrorCode chppWifiServiceRequestScanAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);
static enum ChppAppErrorCode chppWifiServiceRequestRangingAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);

static void chppWifiServiceScanMonitorStatusChangeCallback(bool enabled,
                                                           uint8_t errorCode);
static void chppWifiServiceScanResponseCallback(bool pending,
                                                uint8_t errorCode);
static void chppWifiServiceScanEventCallback(struct chreWifiScanEvent *event);
static void chppWifiServiceRangingEventCallback(
    uint8_t errorCode, struct chreWifiRangingEvent *event);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a client request from the transport layer that is determined to be
 * for the WiFi service. If the result of the dispatch is an error, this
 * function responds to the client with the same error.
 *
 * This function is called from the app layer using its function pointer given
 * during service registration.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppDispatchWifiRequest(void *serviceContext,
                                                     uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct ChppWifiServiceState *wifiServiceContext =
      (struct ChppWifiServiceState *)serviceContext;
  struct ChppRequestResponseState *rRState = NULL;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;
  bool dispatched = true;

  switch (rxHeader->command) {
    case CHPP_WIFI_OPEN: {
      rRState = &wifiServiceContext->open;
      chppServiceTimestampRequest(rRState, rxHeader);
      error = chppWifiServiceOpen(wifiServiceContext, rxHeader);
      break;
    }

    case CHPP_WIFI_CLOSE: {
      rRState = &wifiServiceContext->close;
      chppServiceTimestampRequest(rRState, rxHeader);
      error = chppWifiServiceClose(wifiServiceContext, rxHeader);
      break;
    }

    case CHPP_WIFI_GET_CAPABILITIES: {
      rRState = &wifiServiceContext->getCapabilities;
      chppServiceTimestampRequest(rRState, rxHeader);
      error = chppWifiServiceGetCapabilities(wifiServiceContext, rxHeader);
      break;
    }

    case CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC: {
      rRState = &wifiServiceContext->configureScanMonitorAsync;
      chppServiceTimestampRequest(rRState, rxHeader);
      error = chppWifiServiceConfigureScanMonitorAsync(wifiServiceContext,
                                                       rxHeader, buf, len);
      break;
    }

    case CHPP_WIFI_REQUEST_SCAN_ASYNC: {
      rRState = &wifiServiceContext->requestScanAsync;
      chppServiceTimestampRequest(rRState, rxHeader);
      error = chppWifiServiceRequestScanAsync(wifiServiceContext, rxHeader, buf,
                                              len);
      break;
    }

    case CHPP_WIFI_REQUEST_RANGING_ASYNC: {
      rRState = &wifiServiceContext->requestRangingAsync;
      chppServiceTimestampRequest(rRState, rxHeader);
      error = chppWifiServiceRequestRangingAsync(wifiServiceContext, rxHeader,
                                                 buf, len);
      break;
    }

    default: {
      dispatched = false;
      error = CHPP_APP_ERROR_INVALID_COMMAND;
      break;
    }
  }

  if (dispatched == true && error != CHPP_APP_ERROR_NONE) {
    // Request was dispatched but an error was returned. Close out
    // chppServiceTimestampRequest()
    chppServiceTimestampResponse(rRState);
  }

  return error;
}

/**
 * Initializes the WiFi service upon an open request from the client and
 * responds to the client with the result.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppWifiServiceOpen(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader) {
  static const struct chrePalWifiCallbacks palCallbacks = {
      .scanMonitorStatusChangeCallback =
          chppWifiServiceScanMonitorStatusChangeCallback,
      .scanResponseCallback = chppWifiServiceScanResponseCallback,
      .scanEventCallback = chppWifiServiceScanEventCallback,
      .rangingEventCallback = chppWifiServiceRangingEventCallback,
  };

  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (wifiServiceContext->service.openState == CHPP_OPEN_STATE_OPENED) {
    CHPP_LOGE("WiFi service already open");
    CHPP_DEBUG_ASSERT(false);
    error = CHPP_APP_ERROR_INVALID_COMMAND;

  } else if (!wifiServiceContext->api->open(
                 wifiServiceContext->service.appContext->systemApi,
                 &palCallbacks)) {
    CHPP_LOGE("WiFi PAL open failed");
    CHPP_DEBUG_ASSERT(false);
    error = CHPP_APP_ERROR_BEYOND_CHPP;

  } else {
    CHPP_LOGI("WiFi service opened");
    wifiServiceContext->service.openState = CHPP_OPEN_STATE_OPENED;

    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    size_t responseLen = sizeof(*response);

    if (response == NULL) {
      CHPP_LOG_OOM();
      error = CHPP_APP_ERROR_OOM;
    } else {
      chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                        &wifiServiceContext->open, response,
                                        responseLen);
    }
  }

  return error;
}

/**
 * Deinitializes the WiFi service.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppWifiServiceClose(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  wifiServiceContext->api->close();
  wifiServiceContext->service.openState = CHPP_OPEN_STATE_CLOSED;

  CHPP_LOGI("WiFi service closed");

  struct ChppAppHeader *response =
      chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
  size_t responseLen = sizeof(*response);

  if (response == NULL) {
    CHPP_LOG_OOM();
    error = CHPP_APP_ERROR_OOM;
  } else {
    chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                      &wifiServiceContext->close, response,
                                      responseLen);
  }
  return error;
}

/**
 * Notifies the service of an incoming reset.
 *
 * @param serviceContext Maintains status for each service instance.
 */
static void chppWifiServiceNotifyReset(void *serviceContext) {
  struct ChppWifiServiceState *wifiServiceContext =
      (struct ChppWifiServiceState *)serviceContext;

  if (wifiServiceContext->service.openState != CHPP_OPEN_STATE_OPENED) {
    CHPP_LOGW("WiFi service reset but wasn't open");
  } else {
    CHPP_LOGI("WiFi service reset. Closing");
    wifiServiceContext->service.openState = CHPP_OPEN_STATE_CLOSED;
    wifiServiceContext->api->close();
  }
}

/**
 * Retrieves a set of flags indicating the WiFi features supported by the
 * current implementation.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppWifiServiceGetCapabilities(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  struct ChppWifiGetCapabilitiesResponse *response =
      chppAllocServiceResponseFixed(requestHeader,
                                    struct ChppWifiGetCapabilitiesResponse);
  size_t responseLen = sizeof(*response);

  if (response == NULL) {
    CHPP_LOG_OOM();
    error = CHPP_APP_ERROR_OOM;
  } else {
    response->params.capabilities = wifiServiceContext->api->getCapabilities();

    CHPP_LOGD("chppWifiServiceGetCapabilities returning 0x%" PRIx32
              ", %" PRIuSIZE " bytes",
              response->params.capabilities, responseLen);
    chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                      &wifiServiceContext->getCapabilities,
                                      response, responseLen);
  }

  return error;
}

/**
 * Configures whether scanEventCallback receives unsolicited scan results, i.e.
 * the results of scans not performed at the request of CHRE.
 *
 * This function returns an error code synchronously.
 * A subsequent call to chppWifiServiceScanMonitorStatusChangeCallback() will be
 * used to communicate the result of this request (as a service response).
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppWifiServiceConfigureScanMonitorAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  UNUSED_VAR(requestHeader);
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (len < sizeof(bool)) {
    error = CHPP_APP_ERROR_INVALID_ARG;
  } else {
    bool *enable = (bool *)buf;
    if (!wifiServiceContext->api->configureScanMonitor(*enable)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  return error;
}

/**
 * Request that the WiFi chipset perform a scan, or deliver results from its
 * cache if the parameters allow for it.
 *
 * This function returns an error code synchronously.
 * A subsequent call to chppWifiServiceScanResponseCallback() will be used to
 * communicate the result of this request (as a service response).
 * A subsequent call to chppWifiServiceScanEventCallback() will be used to
 * communicate the scan results (as a service notification).
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppWifiServiceRequestScanAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  UNUSED_VAR(requestHeader);
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  struct chreWifiScanParams *chre =
      chppWifiScanParamsToChre((struct ChppWifiScanParams *)buf, len);

  if (chre == NULL) {
    CHPP_LOGE(
        "WifiServiceRequestScanAsync CHPP -> CHRE conversion failed. Input "
        "len=%" PRIuSIZE,
        len);
    error = CHPP_APP_ERROR_INVALID_ARG;

  } else {
    if (!wifiServiceContext->api->requestScan(chre)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }

    if (chre->frequencyListLen > 0) {
      void *frequencyList = CHPP_CONST_CAST_POINTER(chre->frequencyList);
      CHPP_FREE_AND_NULLIFY(frequencyList);
    }
    if (chre->ssidListLen > 0) {
      void *ssidList = CHPP_CONST_CAST_POINTER(chre->ssidList);
      CHPP_FREE_AND_NULLIFY(ssidList);
    }
    CHPP_FREE_AND_NULLIFY(chre);
  }

  return error;
}

/**
 * Request that the WiFi chipset perform RTT ranging against a set of access
 * points specified in params.
 *
 * This function returns an error code synchronously.
 * A subsequent call to chppWifiServiceRangingEventCallback() will be used to
 * communicate the ranging results (as a service notification).
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppWifiServiceRequestRangingAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  UNUSED_VAR(requestHeader);
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  struct chreWifiRangingParams *chre =
      chppWifiRangingParamsToChre((struct ChppWifiRangingParams *)buf, len);

  if (chre == NULL) {
    CHPP_LOGE(
        "WifiServiceRequestRangingAsync CHPP -> CHRE conversion failed. Input "
        "len=%" PRIuSIZE,
        len);
    error = CHPP_APP_ERROR_INVALID_ARG;

  } else {
    if (!wifiServiceContext->api->requestRanging(chre)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;

    } else {
      struct ChppAppHeader *response =
          chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
      size_t responseLen = sizeof(*response);

      if (response == NULL) {
        CHPP_LOG_OOM();
        error = CHPP_APP_ERROR_OOM;
      } else {
        chppSendTimestampedResponseOrFail(
            &wifiServiceContext->service,
            &wifiServiceContext->requestRangingAsync, response, responseLen);
      }
    }

    if (chre->targetListLen > 0) {
      void *targetList = CHPP_CONST_CAST_POINTER(chre->targetList);
      CHPP_FREE_AND_NULLIFY(targetList);
    }
    CHPP_FREE_AND_NULLIFY(chre);
  }

  return error;
}

/**
 * PAL callback with the result of changes to the scan monitor registration
 * status requested via configureScanMonitor.
 *
 * @param enabled true if the scan monitor is currently active
 * @param errorCode An error code from enum chreError
 */
static void chppWifiServiceScanMonitorStatusChangeCallback(bool enabled,
                                                           uint8_t errorCode) {
  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = gWifiServiceContext.service.handle,
      .transaction = gWifiServiceContext.configureScanMonitorAsync.transaction,
      .command = CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC,
  };

  struct ChppWifiConfigureScanMonitorAsyncResponse *response =
      chppAllocServiceResponseFixed(
          &requestHeader, struct ChppWifiConfigureScanMonitorAsyncResponse);
  size_t responseLen = sizeof(*response);

  if (response == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    response->params.enabled = enabled;
    response->params.errorCode = errorCode;

    chppSendTimestampedResponseOrFail(
        &gWifiServiceContext.service,
        &gWifiServiceContext.configureScanMonitorAsync, response, responseLen);
  }
}

/**
 * PAL callback with the result of a requestScan.
 *
 * @param pending true if the request was successful.
 * @param errorCode An error code from enum chreError.
 */
static void chppWifiServiceScanResponseCallback(bool pending,
                                                uint8_t errorCode) {
  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = gWifiServiceContext.service.handle,
      .transaction = gWifiServiceContext.requestScanAsync.transaction,
      .command = CHPP_WIFI_REQUEST_SCAN_ASYNC,
  };

  struct ChppWifiRequestScanResponse *response = chppAllocServiceResponseFixed(
      &requestHeader, struct ChppWifiRequestScanResponse);
  size_t responseLen = sizeof(*response);

  if (response == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    response->params.pending = pending;
    response->params.errorCode = errorCode;

    chppSendTimestampedResponseOrFail(&gWifiServiceContext.service,
                                      &gWifiServiceContext.requestScanAsync,
                                      response, responseLen);
  }
}

/**
 * PAL callback with WiFi scan results.
 *
 * @param event Scan result data.
 */
static void chppWifiServiceScanEventCallback(struct chreWifiScanEvent *event) {
  // Craft response per parser script
  struct ChppWifiScanEventWithHeader *notification = NULL;
  size_t notificationLen = 0;

  CHPP_DEBUG_ASSERT(chppCheckWifiScanEventNotification(event));

  if (!chppWifiScanEventFromChre(event, &notification, &notificationLen)) {
    CHPP_LOGE("ScanEvent conversion failed (OOM?). ID=%" PRIu8,
              gWifiServiceContext.requestScanAsync.transaction);

    notification = chppMalloc(sizeof(struct ChppAppHeader));
    if (notification == NULL) {
      CHPP_LOG_OOM();
    } else {
      notificationLen = sizeof(struct ChppAppHeader);
    }
  }

  if (notification != NULL) {
    notification->header.handle = gWifiServiceContext.service.handle;
    notification->header.type = CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION;
    notification->header.transaction =
        gWifiServiceContext.requestScanAsync.transaction;
    notification->header.error =
        (notificationLen > sizeof(struct ChppAppHeader))
            ? CHPP_APP_ERROR_NONE
            : CHPP_APP_ERROR_CONVERSION_FAILED;
    notification->header.command = CHPP_WIFI_REQUEST_SCAN_ASYNC;

    chppEnqueueTxDatagramOrFail(
        gWifiServiceContext.service.appContext->transportContext, notification,
        notificationLen);
  }

  gWifiServiceContext.api->releaseScanEvent(event);
}

/**
 * PAL callback with RTT ranging results from the WiFi module.
 *
 * @param errorCode An error code from enum chreError.
 * @param event Ranging data.
 */
static void chppWifiServiceRangingEventCallback(
    uint8_t errorCode, struct chreWifiRangingEvent *event) {
  struct ChppWifiRangingEventWithHeader *notification = NULL;
  size_t notificationLen = 0;

  if (!chppWifiRangingEventFromChre(event, &notification, &notificationLen)) {
    CHPP_LOGE("RangingEvent conversion failed (OOM?) ID=%" PRIu8,
              gWifiServiceContext.requestRangingAsync.transaction);

    notification = chppMalloc(sizeof(struct ChppAppHeader));
    if (notification == NULL) {
      CHPP_LOG_OOM();
    } else {
      notificationLen = sizeof(struct ChppAppHeader);
    }
  }

  if (notification != NULL) {
    notification->header.handle = gWifiServiceContext.service.handle;
    notification->header.type = CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION;
    notification->header.transaction =
        gWifiServiceContext.requestRangingAsync.transaction;
    notification->header.error =
        (notificationLen > sizeof(struct ChppAppHeader))
            ? CHPP_APP_ERROR_NONE
            : CHPP_APP_ERROR_CONVERSION_FAILED;
    notification->header.command = CHPP_WIFI_REQUEST_RANGING_ASYNC;

    if (errorCode != CHRE_ERROR_NONE) {
      notification->header.error = CHPP_APP_ERROR_BEYOND_CHPP;
      notificationLen = MIN(notificationLen, sizeof(struct ChppAppHeader));
    }

    chppEnqueueTxDatagramOrFail(
        gWifiServiceContext.service.appContext->transportContext, notification,
        notificationLen);
  }

  gWifiServiceContext.api->releaseRangingEvent(event);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWifiService(struct ChppAppState *appContext) {
  gWifiServiceContext.api = chrePalWifiGetApi(CHRE_PAL_WIFI_API_V1_2);

  chppCheckWifiScanEventNotificationReset();

  if (gWifiServiceContext.api == NULL) {
    CHPP_LOGE(
        "WiFi PAL API version not compatible with CHPP. Cannot register WiFi "
        "service");
    CHPP_DEBUG_ASSERT(false);

  } else {
    gWifiServiceContext.service.appContext = appContext;
    gWifiServiceContext.service.handle = chppRegisterService(
        appContext, (void *)&gWifiServiceContext, &kWifiServiceConfig);
    CHPP_DEBUG_ASSERT(gWifiServiceContext.service.handle);
  }
}

void chppDeregisterWifiService(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}
