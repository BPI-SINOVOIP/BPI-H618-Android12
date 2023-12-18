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

#ifndef CHPP_APP_H_
#define CHPP_APP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/condition_variable.h"
#include "chpp/macros.h"
#include "chpp/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public Definitions
 ***********************************************/

/**
 * Maximum number of services that can be registered by CHPP (not including
 * predefined services), if not defined by the build system.
 */
#ifndef CHPP_MAX_REGISTERED_SERVICES
#define CHPP_MAX_REGISTERED_SERVICES 1
#endif

/**
 * Maximum number of clients that can be registered by CHPP (not including
 * predefined clients), if not defined by the build system.
 */
#ifndef CHPP_MAX_REGISTERED_CLIENTS
#define CHPP_MAX_REGISTERED_CLIENTS 1
#endif

/**
 * Maximum number of services that can be discovered by CHPP (not including
 * predefined services), if not defined by the build system.
 */
#ifndef CHPP_MAX_DISCOVERED_SERVICES
#define CHPP_MAX_DISCOVERED_SERVICES \
  MAX(CHPP_MAX_REGISTERED_SERVICES, CHPP_MAX_REGISTERED_CLIENTS)
#endif

/**
 * Default value for reserved fields.
 */
#define CHPP_RESERVED 0

/**
 * Client index number when there is no matching client
 */
#define CHPP_CLIENT_INDEX_NONE 0xff

/**
 * App layer command at initialization.
 */
#define CHPP_APP_COMMAND_NONE 0

/**
 * Handle Numbers in ChppAppHeader
 */
enum ChppHandleNumber {
  //! Handleless communication
  CHPP_HANDLE_NONE = 0x00,

  //! Loopback Service
  CHPP_HANDLE_LOOPBACK = 0x01,

  //! Time Service
  CHPP_HANDLE_TIMESYNC = 0x02,

  //! Discovery Service
  CHPP_HANDLE_DISCOVERY = 0x0F,

  //! Negotiated Services (starting from this offset)
  CHPP_HANDLE_NEGOTIATED_RANGE_START = 0x10,
};

/**
 * Message Types as used in ChppAppHeader
 */
#define CHPP_APP_MASK_MESSAGE_TYPE LEAST_SIGNIFICANT_NIBBLE
#define CHPP_APP_GET_MESSAGE_TYPE(value) \
  ((enum ChppMessageType)(               \
      (value)&CHPP_APP_MASK_MESSAGE_TYPE))  // TODO: Consider checking if this
                                            // maps into a valid enum
enum ChppMessageType {
  //! Request from client. Needs response from service.
  CHPP_MESSAGE_TYPE_CLIENT_REQUEST = 0,

  //! Response from service (with the same Command and Transaction ID as the
  //! client request).
  CHPP_MESSAGE_TYPE_SERVICE_RESPONSE = 1,

  //! Notification from client. Service shall not respond.
  CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION = 2,

  //! Notification from service. Client shall not respond.
  CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION = 3,
};

/**
 * Error codes used by the app layer / clients / services.
 */
enum ChppAppErrorCode {
  //! Success (no error)
  CHPP_APP_ERROR_NONE = 0,
  //! Invalid command
  CHPP_APP_ERROR_INVALID_COMMAND = 1,
  //! Invalid argument(s)
  CHPP_APP_ERROR_INVALID_ARG = 2,
  //! Busy
  CHPP_APP_ERROR_BUSY = 3,
  //! Out of memory
  CHPP_APP_ERROR_OOM = 4,
  //! Feature not supported
  CHPP_APP_ERROR_UNSUPPORTED = 5,
  //! Timeout
  CHPP_APP_ERROR_TIMEOUT = 6,
  //! Functionality disabled (e.g. per user configuration)
  CHPP_APP_ERROR_DISABLED = 7,
  //! Rate limit exceeded (try again later)
  CHPP_APP_ERROR_RATELIMITED = 8,
  //! Function in use / blocked by another entity (e.g. the AP)
  CHPP_APP_ERROR_BLOCKED = 9,
  //! Invalid length
  CHPP_APP_ERROR_INVALID_LENGTH = 10,
  //! CHPP Not Ready
  CHPP_APP_ERROR_NOT_READY = 11,
  //! Error outside of CHPP (e.g. PAL API)
  CHPP_APP_ERROR_BEYOND_CHPP = 12,
  //! Response not matching a pending request
  CHPP_APP_ERROR_UNEXPECTED_RESPONSE = 13,
  //! Conversion failed
  CHPP_APP_ERROR_CONVERSION_FAILED = 14,
  //! Unspecified failure
  CHPP_APP_ERROR_UNSPECIFIED = 255
};

/**
 * Open status for clients / services.
 */
enum ChppOpenState {
  CHPP_OPEN_STATE_CLOSED = 0,           // Closed
  CHPP_OPEN_STATE_OPENING = 1,          // Enables the open request to pass
  CHPP_OPEN_STATE_WAITING_TO_OPEN = 2,  // Waiting for open response
  CHPP_OPEN_STATE_OPENED = 3,           // Opened
};

/**
 * CHPP Application Layer header
 */
CHPP_PACKED_START
struct ChppAppHeader {
  //! Service Handle
  uint8_t handle;

  //! Most significant nibble (MSN): Reserved
  //! Least significant nibble (LSN): Message Type from enum ChppMessageType
  uint8_t type;

  //! Transaction ID
  uint8_t transaction;

  //! Error if any, from enum ChppAppErrorCode
  uint8_t error;

  //! Command
  uint16_t command;

} CHPP_PACKED_ATTR;
CHPP_PACKED_END

//! Minimum length of a header that includes upto the transaction ID
#define CHPP_APP_MIN_LEN_HEADER_WITH_TRANSACTION (3 * sizeof(uint8_t))

/**
 * Function type that dispatches incoming datagrams for any client or service
 */
typedef enum ChppAppErrorCode(ChppDispatchFunction)(void *context, uint8_t *buf,
                                                    size_t len);

/**
 * Function type that initializes a client and assigns it its handle number
 */
typedef bool(ChppClientInitFunction)(void *context, uint8_t handle,
                                     struct ChppVersion serviceVersion);

/**
 * Function type that deinitializes a client.
 */
typedef void(ChppClientDeinitFunction)(void *context);

/**
 * Function type that dispatches a reset notification to any client or service
 */
typedef void(ChppNotifierFunction)(void *context);

/**
 * Length of a service UUID and its human-readable printed form in bytes
 */
#define CHPP_SERVICE_UUID_LEN 16
#define CHPP_SERVICE_UUID_STRING_LEN (16 * 2 + 4 + 1)

/**
 * Length of a version number, in bytes (major + minor + revision), per CHPP
 * spec.
 */
#define CHPP_SERVICE_VERSION_LEN (1 + 1 + 2)

/**
 * Maximum length of a human-readable service name, per CHPP spec.
 * (15 ASCII characters + null)
 */
#define CHPP_SERVICE_NAME_MAX_LEN (15 + 1)

/**
 * CHPP definition of a service descriptor as sent over the wire.
 */
CHPP_PACKED_START
struct ChppServiceDescriptor {
  //! UUID of the service.
  //! Must be generated according to RFC 4122, UUID version 4 (random).
  uint8_t uuid[CHPP_SERVICE_UUID_LEN];

  //! Human-readable name of the service for debugging.
  char name[CHPP_SERVICE_NAME_MAX_LEN];

  //! Version of the service.
  struct ChppVersion version;
} CHPP_PACKED_ATTR;
CHPP_PACKED_END

/**
 * CHPP definition of a service as supported on a server.
 */
struct ChppService {
  //! Service Descriptor as sent over the wire.
  struct ChppServiceDescriptor descriptor;

  //! Pointer to the function that is used to notify the service if CHPP is
  //! reset.
  ChppNotifierFunction *resetNotifierFunctionPtr;

  //! Pointer to the function that dispatches incoming client requests for the
  //! service.
  ChppDispatchFunction *requestDispatchFunctionPtr;

  //! Pointer to the function that dispatches incoming client notifications for
  //! the service.
  ChppDispatchFunction *notificationDispatchFunctionPtr;

  //! Minimum valid length of datagrams for the service.
  size_t minLength;
};

/**
 * CHPP definition of a client descriptor.
 */
struct ChppClientDescriptor {
  //! UUID of the client.
  //! Must be generated according to RFC 4122, UUID version 4 (random).
  uint8_t uuid[CHPP_SERVICE_UUID_LEN];

  //! Version of the client.
  struct ChppVersion version;
};

/**
 * CHPP definition of a client.
 */
struct ChppClient {
  //! Client descriptor.
  struct ChppClientDescriptor descriptor;

  //! Pointer to the function that is used to notify the client if CHPP is
  //! reset.
  ChppNotifierFunction *resetNotifierFunctionPtr;

  //! Pointer to the function that is used to notify the client if CHPP is
  //! matched to a service.
  ChppNotifierFunction *matchNotifierFunctionPtr;

  //! Pointer to the function that dispatches incoming service responses for the
  //! client.
  //! Service responses are only dispatched to clients that have been opened or
  //! are in the process of being (re)opened. @see ChppOpenState
  ChppDispatchFunction *responseDispatchFunctionPtr;

  //! Pointer to the function that dispatches incoming service notifications for
  //! the client.
  //! Service notifications are only dispatched to clients that have been
  //! opened. @see ChppOpenState
  ChppDispatchFunction *notificationDispatchFunctionPtr;

  //! Pointer to the function that initializes the client (after it is matched
  //! with a service at discovery) and assigns it its handle number.
  ChppClientInitFunction *initFunctionPtr;

  //! Pointer to the function that deinitializes the client.
  ChppClientDeinitFunction *deinitFunctionPtr;

  //! Number of request-response states in the rRStates array. This is a
  //! uint16_t to match the uint16_t command in struct ChppAppHeader.
  uint16_t rRStateCount;

  //! Minimum valid length of datagrams for the service.
  size_t minLength;
};

/**
 * Request status for clients.
 */
enum ChppRequestState {
  CHPP_REQUEST_STATE_NONE = 0,              // No request sent ever
  CHPP_REQUEST_STATE_REQUEST_SENT = 1,      // Sent but no response yet
  CHPP_REQUEST_STATE_RESPONSE_RCV = 2,      // Sent and response received
  CHPP_REQUEST_STATE_RESPONSE_TIMEOUT = 3,  // Timeout. Responded as need be
};

/**
 * Maintains the basic state for each request/response functionality of a
 * client or service.
 * Any number of these may be included in the (context) status variable of a
 * client or service (one per every every request/response functionality).
 */
struct ChppRequestResponseState {
  uint64_t requestTimeNs;  // Time of the last request
  uint64_t
      responseTimeNs;  // If requestState is CHPP_REQUEST_STATE_REQUEST_SENT,
                       // indicates the timeout time for the request
                       // If requestState is CHPP_REQUEST_STATE_RESPONSE_RCV,
                       // indicates when the response was received

  uint8_t requestState;  // From enum ChppRequestState
  uint8_t transaction;   // Transaction ID for the last request/response
};

/**
 * Enabled clients and services.
 */
struct ChppClientServiceSet {
  bool wifiService : 1;
  bool gnssService : 1;
  bool wwanService : 1;
  bool wifiClient : 1;
  bool gnssClient : 1;
  bool wwanClient : 1;
  bool loopbackClient : 1;
};

struct ChppLoopbackClientState;
struct ChppTimesyncClientState;

struct ChppAppState {
  struct ChppTransportState *transportContext;  // Pointing to transport context

  const struct chrePalSystemApi *systemApi;  // Pointing to the PAL system APIs

  uint8_t registeredServiceCount;  // Number of services currently registered

  const struct ChppService *registeredServices[CHPP_MAX_REGISTERED_SERVICES];

  void *registeredServiceContexts[CHPP_MAX_REGISTERED_SERVICES];

  uint8_t registeredClientCount;  // Number of clients currently registered

  const struct ChppClient *registeredClients[CHPP_MAX_REGISTERED_CLIENTS];

  const struct ChppClientState
      *registeredClientStates[CHPP_MAX_REGISTERED_CLIENTS];

  void *registeredClientContexts[CHPP_MAX_REGISTERED_CLIENTS];

  uint64_t nextRequestTimeoutNs;

  uint8_t
      clientIndexOfServiceIndex[CHPP_MAX_DISCOVERED_SERVICES];  // Lookup table

  struct ChppClientServiceSet clientServiceSet;  // Enabled client/services

  // Pointers to the contexts of basic clients, which are allocated if and when
  // they are initialized
  struct ChppLoopbackClientState *loopbackClientContext;
  struct ChppTimesyncClientState *timesyncClientContext;

  // For discovery clients
  bool isDiscoveryClientEverInitialized;
  bool isDiscoveryClientInitialized;
  bool isDiscoveryComplete;

  // The number of clients that matched a service during discovery.
  uint8_t matchedClientCount;

  // The number of services that were found during discovery.
  uint8_t discoveredServiceCount;

  struct ChppMutex discoveryMutex;
  struct ChppConditionVariable discoveryCv;
};

#define CHPP_SERVICE_INDEX_OF_HANDLE(handle) \
  ((handle)-CHPP_HANDLE_NEGOTIATED_RANGE_START)

#define CHPP_SERVICE_HANDLE_OF_INDEX(index) \
  ((index) + CHPP_HANDLE_NEGOTIATED_RANGE_START)

/************************************************
 *  Public functions
 ***********************************************/

/**
 * Initializes the CHPP app layer state stored in the parameter appContext.
 * It is necessary to initialize state for each app layer instance on
 * every platform.
 *
 * @param appContext Maintains status for each app layer instance.
 * @param transportContext The transport layer status struct associated with
 * this app layer instance.
 */
void chppAppInit(struct ChppAppState *appContext,
                 struct ChppTransportState *transportContext);

/**
 * Same as chppAppInit(), but specifies the client/service endpoints to be
 * enabled.
 *
 * @param appContext Maintains status for each app layer instance.
 * @param transportContext The transport layer status struct associated with
 * this app layer instance.
 * @param clientServiceSet Bitmap specifying the client/service endpoints to be
 * enabled.
 */
void chppAppInitWithClientServiceSet(
    struct ChppAppState *appContext,
    struct ChppTransportState *transportContext,
    struct ChppClientServiceSet clientServiceSet);

/**
 * Deinitializes the CHPP app layer for e.g. clean shutdown.
 *
 * @param appContext A non-null pointer to ChppAppState initialized previously
 * in chppAppInit().
 */
void chppAppDeinit(struct ChppAppState *appContext);

/**
 * Processes an Rx Datagram from the transport layer.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppAppProcessRxDatagram(struct ChppAppState *context, uint8_t *buf,
                              size_t len);

/**
 * Used by the transport layer to notify the app layer of a reset during
 * operation. This function is called after the transport layer has sent a reset
 * or reset-ack packet.
 * In turn, this function notifies clients and services to allow them to reset
 * or recover state as necessary.
 *
 * @param context Maintains status for each app layer instance.
 */
void chppAppProcessReset(struct ChppAppState *context);

/**
 * Convert UUID to a human-readable, null-terminated string.
 *
 * @param uuid Input UUID
 * @param strOut Output null-terminated string
 */
void chppUuidToStr(const uint8_t uuid[CHPP_SERVICE_UUID_LEN],
                   char strOut[CHPP_SERVICE_UUID_STRING_LEN]);

/**
 * Maps a CHPP app layer error to a CHRE error.
 *
 * @param chppError CHPP app layer error (from enum ChppAppErrorCode).
 *
 * @return CHRE error (from enum chreError).
 */
uint8_t chppAppErrorToChreError(uint8_t error);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_APP_H_
