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

#ifndef CHPP_CLIENTS_H_
#define CHPP_CLIENTS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"
#include "chpp/condition_variable.h"
#include "chpp/macros.h"
#include "chpp/mutex.h"
#include "chre_api/chre/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public Definitions
 ***********************************************/

/**
 * Uses chppAllocClientRequest() to allocate a client request message of a
 * specific type and its corresponding length.
 *
 * @param clientState State variable of the client.
 * @param type Type of response.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocClientRequestFixed(clientState, type) \
  (type *)chppAllocClientRequest(clientState, sizeof(type))

/**
 * Uses chppAllocClientRequest() to allocate a variable-length client request
 * message of a specific type.
 *
 * @param clientState State variable of the client.
 * @param type Type of response which includes an arrayed member.
 * @param count number of items in the array of arrayField.
 * @param arrayField The arrayed member field.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocClientRequestTypedArray(clientState, type, count, arrayField) \
  (type *)chppAllocClientRequest(                                              \
      clientState, sizeof(type) + (count)*sizeof_member(type, arrayField[0]))

/**
 * Maintains the basic state of a client.
 * This is expected to be included once in the (context) status variable of
 * each client.
 */
struct ChppClientState {
  struct ChppAppState *appContext;  // Pointer to app layer context
  struct ChppRequestResponseState
      *rRStates;        // Pointer to array of request-response states, if any
  uint8_t index;        // Index of this client
  uint8_t handle;       // Handle number for this client
  uint8_t transaction;  // Next Transaction ID to be used

  uint8_t openState;         // As defined in enum ChppOpenState
  bool pseudoOpen : 1;       // Client to be opened upon a reset
  bool initialized : 1;      // Is initialized
  bool everInitialized : 1;  // Synchronization primitives initialized

  bool responseReady : 1;  // For sync. request/responses
  struct ChppMutex responseMutex;
  struct ChppConditionVariable responseCondVar;
};

#ifdef CHPP_CLIENT_ENABLED_CHRE_WWAN
#define CHPP_CLIENT_ENABLED_WWAN
#endif

#ifdef CHPP_CLIENT_ENABLED_CHRE_WIFI
#define CHPP_CLIENT_ENABLED_WIFI
#endif

#ifdef CHPP_CLIENT_ENABLED_CHRE_GNSS
#define CHPP_CLIENT_ENABLED_GNSS
#endif

#if defined(CHPP_CLIENT_ENABLED_LOOPBACK) ||                                  \
    defined(CHPP_CLIENT_ENABLED_TIMESYNC) ||                                  \
    defined(CHPP_CLIENT_ENABLED_DISCOVERY) ||                                 \
    defined(CHPP_CLIENT_ENABLED_WWAN) || defined(CHPP_CLIENT_ENABLED_WIFI) || \
    defined(CHPP_CLIENT_ENABLED_GNSS)
#define CHPP_CLIENT_ENABLED
#endif

#define CHPP_CLIENT_REQUEST_TIMEOUT_INFINITE CHPP_TIME_MAX

#ifndef CHPP_CLIENT_REQUEST_TIMEOUT_DEFAULT
#define CHPP_CLIENT_REQUEST_TIMEOUT_DEFAULT CHRE_ASYNC_RESULT_TIMEOUT_NS
#endif

// Default timeout for discovery completion.
#ifndef CHPP_DISCOVERY_DEFAULT_TIMEOUT_MS
#define CHPP_DISCOVERY_DEFAULT_TIMEOUT_MS UINT64_C(10000)  // 10s
#endif

/************************************************
 *  Public functions
 ***********************************************/

/**
 * Registers common clients with the CHPP app layer. These clients are enabled
 * by CHPP_CLIENT_ENABLED_xxxx definitions. This function is automatically
 * called by chppAppInit().
 *
 * @param context Maintains status for each app layer instance.
 */
void chppRegisterCommonClients(struct ChppAppState *context);

/**
 * Deregisters common clients with the CHPP app layer. These clients are enabled
 * by CHPP_CLIENT_ENABLED_xxxx definitions. This function is automatically
 * called by chppAppDeinit().
 *
 * @param context Maintains status for each app layer instance.
 */
void chppDeregisterCommonClients(struct ChppAppState *context);

/**
 * Registers a new client on CHPP. This function is to be called by the
 * platform initialization code for every non-common client available on a
 * server (if any), i.e. except those that are registered through
 * chppRegisterCommonClients().
 *
 * Registered clients are matched with discovered services during discovery.
 * When a match succeeds, the client's initialization function (pointer) is
 * called, assigning them their handle number.
 *
 * Note that the maximum number of clients that can be registered on a platform
 * can specified as CHPP_MAX_REGISTERED_CLIENTS by the initialization code.
 * Otherwise, a default value will be used.
 *
 * @param appContext Maintains status for each app layer instance.
 * @param clientContext Maintains status for each client instance.
 * @param clientState State variable of the client.
 * @param rRStates Pointer to array of request-response states, if any.
 * @param newClient The client to be registered on this platform.
 */
void chppRegisterClient(struct ChppAppState *appContext, void *clientContext,
                        struct ChppClientState *clientState,
                        struct ChppRequestResponseState *rRStates,
                        const struct ChppClient *newClient);

/**
 * Initializes basic CHPP clients.
 *
 * @param context Maintains status for each app layer instance.
 */
void chppInitBasicClients(struct ChppAppState *context);

/**
 * Initializes a client. This function must be called when a client is matched
 * with a service during discovery to provides its handle number.
 *
 * @param clientState State variable of the client.
 * @param handle Handle number for this client.
 */
void chppClientInit(struct ChppClientState *clientState, uint8_t handle);

/**
 * Deinitializes a client.
 *
 * @param clientState State variable of the client.
 */
void chppClientDeinit(struct ChppClientState *clientState);

/**
 * Deinitializes basic clients.
 *
 * @param context Maintains status for each app layer instance.
 */
void chppDeinitBasicClients(struct ChppAppState *context);

/**
 * Deinitializes all matched clients.
 *
 * @param context Maintains status for each app layer instance.
 */
void chppDeinitMatchedClients(struct ChppAppState *context);

/**
 * Allocates a client request message of a specified length, populating the
 * (app layer) client request header, including the sequence ID. The
 * next-sequence ID stored in the client state variable is subsequently
 * incremented.
 *
 * It is expected that for most use cases, the chppAllocClientRequestFixed()
 * or chppAllocClientRequestTypedArray() macros shall be used rather than
 * calling this function directly.
 *
 * @param clientState State variable of the client.
 * @param len Length of the response message (including header) in bytes. Note
 * that the specified length must be at least equal to the lendth of the app
 * layer header.
 *
 * @return Pointer to allocated memory
 */
struct ChppAppHeader *chppAllocClientRequest(
    struct ChppClientState *clientState, size_t len);

/**
 * Uses chppAllocClientRequest() to allocate a specific client request command
 * without any additional payload.
 *
 * @param clientState State variable of the client.
 * @param command Type of response.
 *
 * @return Pointer to allocated memory
 */
struct ChppAppHeader *chppAllocClientRequestCommand(
    struct ChppClientState *clientState, uint16_t command);

/**
 * This function shall be called for all outgoing client requests in order to
 * A) Timestamp them, and
 * B) Save their Transaction ID
 * as part of the request/response's ChppRequestResponseState struct.
 *
 * This function prints an error message if a duplicate request is sent
 * while outstanding request is still pending without a response.
 *
 * @param clientState State of the client sending the client request.
 * @param transactionId The transaction ID to use when loading the app.
 * @param rRState Maintains the basic state for each request/response
 * functionality of a client.
 * @param requestHeader Client request header.
 */
void chppClientTimestampRequest(struct ChppClientState *clientState,
                                struct ChppRequestResponseState *rRState,
                                struct ChppAppHeader *requestHeader,
                                uint64_t timeoutNs);

/**
 * This function shall be called for incoming responses to a client request in
 * order to
 * A) Verify the correct transaction ID
 * B) Timestamp them, and
 * C) Mark them as fulfilled
 * D) TODO: check for timeout
 *
 * This function prints an error message if a response is received without an
 * outstanding request.
 *
 * @param clientState State of the client sending the client request.
 * @param rRState Maintains the basic state for each request/response
 * functionality of a client.
 * @param requestHeader Client request header.
 *
 * @return false if there is an error. True otherwise.
 */
bool chppClientTimestampResponse(struct ChppClientState *clientState,
                                 struct ChppRequestResponseState *rRState,
                                 const struct ChppAppHeader *responseHeader);

/**
 * Timestamps a client request using chppClientTimestampResponse() and enqueues
 * it using chppEnqueueTxDatagramOrFail().
 *
 * Refer to their respective documentation for details.
 *
 * Note that the ownership of buf is taken from the caller when this method is
 * invoked.
 *
 * @param clientState State of the client sending the client request.
 * @param rRState Maintains the basic state for each request/response
 * functionality of a client.
 * @param buf Datagram payload allocated through chppMalloc. Cannot be null.
 * @param len Datagram length in bytes.
 * @param timeoutNs Time in nanoseconds before a timeout response is generated.
 * Zero means no timeout response.
 *
 * @return True informs the sender that the datagram was successfully enqueued.
 * False informs the sender that the queue was full and the payload discarded.
 */
bool chppSendTimestampedRequestOrFail(struct ChppClientState *clientState,
                                      struct ChppRequestResponseState *rRState,
                                      void *buf, size_t len,
                                      uint64_t timeoutNs);

/**
 * Similar to chppSendTimestampedRequestOrFail() but blocks execution until a
 * response is received. Used for synchronous requests.
 *
 * In order to use this function, clientState->responseNotifier must have been
 * initialized using chppNotifierInit() upon initialization of the client.
 *
 * @param clientState State of the client sending the client request.
 * @param rRState Maintains the basic state for each request/response
 * functionality of a client.
 * @param buf Datagram payload allocated through chppMalloc. Cannot be null.
 * @param len Datagram length in bytes.
 *
 * @return True informs the sender that the datagram was successfully enqueued.
 * False informs the sender that the payload was discarded because either the
 * queue was full, or the request timed out.
 */
bool chppSendTimestampedRequestAndWait(struct ChppClientState *clientState,
                                       struct ChppRequestResponseState *rRState,
                                       void *buf, size_t len);

/**
 * Same as chppSendTimestampedRequestAndWait() but with a specified timeout.
 */
bool chppSendTimestampedRequestAndWaitTimeout(
    struct ChppClientState *clientState,
    struct ChppRequestResponseState *rRState, void *buf, size_t len,
    uint64_t timeoutNs);

/**
 * Markes a closed client as pseudo-open, so that it would be opened upon a
 * reset.
 *
 * @param clientState State variable of the client.
 */
void chppClientPseudoOpen(struct ChppClientState *clientState);

/**
 * Sends a client request for the open command in a blocking or non-blocking
 * manner.
 * A non-blocking open is used to for reopening a service after a reset or for
 * opening a pseudo-open service.
 *
 * @param clientState State variable of the client.
 * @param openRRState Request/response state for the open command.
 * @param openCommand Open command to be sent.
 * @param blocking Indicates a blocking (vs. non-blocking) open request.
 *
 * @return Indicates success or failure.
 */
bool chppClientSendOpenRequest(struct ChppClientState *clientState,
                               struct ChppRequestResponseState *openRRState,
                               uint16_t openCommand, bool blocking);

/**
 * Processes a service response for the open command.
 *
 * @param clientState State variable of the client.
 */
void chppClientProcessOpenResponse(struct ChppClientState *clientState,
                                   uint8_t *buf, size_t len);

/**
 * Recalculates the next upcoming client request timeout time.
 *
 * @param context Maintains status for each app layer instance.
 */
void chppClientRecalculateNextTimeout(struct ChppAppState *context);

/**
 * Closes any remaining open requests for a given client by sending a timeout.
 * This function is used when a client is reset.
 *
 * @param clientState State variable of the client.
 * @param client The client for whech to clear out open requests.
 * @param clearOnly If true, indicates that a timeout response shouldn't be
 *     sent to the client. This must only be set if the requests are being
 *     cleared as part of the client closing.
 */
void chppClientCloseOpenRequests(struct ChppClientState *clientState,
                                 const struct ChppClient *client,
                                 bool clearOnly);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_CLIENTS_H_
