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

#ifndef CHPP_SERVICES_H_
#define CHPP_SERVICES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"
#include "chpp/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public Definitions
 ***********************************************/

#if defined(CHPP_SERVICE_ENABLED_WWAN) || \
    defined(CHPP_SERVICE_ENABLED_WIFI) || defined(CHPP_SERVICE_ENABLED_GNSS)
#define CHPP_SERVICE_ENABLED
#endif

/**
 * Uses chppAllocServiceNotification() to allocate a variable-length response
 * message of a specific type.
 *
 * @param type Type of notification which includes an arrayed member.
 * @param count number of items in the array of arrayField.
 * @param arrayField The arrayed member field.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocServiceNotificationTypedArray(type, count, arrayField) \
  (type *)chppAllocServiceNotification(                                 \
      sizeof(type) + (count)*sizeof_member(type, arrayField[0]))

/**
 * Uses chppAllocServiceNotification() to allocate a response message of a
 * specific type and its corresponding length.
 *
 * @param type Type of notification.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocServiceNotificationFixed(type) \
  (type *)chppAllocServiceNotification(sizeof(type))

/**
 * Uses chppAllocServiceResponse() to allocate a variable-length response
 * message of a specific type.
 *
 * @param requestHeader client request header, as per
 * chppAllocServiceResponse().
 * @param type Type of response which includes an arrayed member.
 * @param count number of items in the array of arrayField.
 * @param arrayField The arrayed member field.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocServiceResponseTypedArray(requestHeader, type, count, \
                                           arrayField)                 \
  (type *)chppAllocServiceResponse(                                    \
      requestHeader,                                                   \
      sizeof(type) + (count)*sizeof_member(type, arrayField[0]))

/**
 * Uses chppAllocServiceResponse() to allocate a response message of a specific
 * type and its corresponding length.
 *
 * @param requestHeader client request header, as per
 * chppAllocServiceResponse().
 * @param type Type of response.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocServiceResponseFixed(requestHeader, type) \
  (type *)chppAllocServiceResponse(requestHeader, sizeof(type))

/**
 * Maintains the basic state of a service.
 * This is expected to be included once in the (context) status variable of each
 * service.
 */
struct ChppServiceState {
  struct ChppAppState *appContext;  // Pointer to app layer context
  uint8_t handle;                   // Handle number for this service

  uint8_t openState;  // As defined in enum ChppOpenState
};

/************************************************
 *  Public functions
 ***********************************************/

/**
 * Registers common services with the CHPP app layer. These services are enabled
 * by CHPP_SERVICE_ENABLED_xxxx definitions. This function is automatically
 * called by chppAppInit().
 *
 * @param context Maintains status for each app layer instance.
 */
void chppRegisterCommonServices(struct ChppAppState *context);

/**
 * Deregisters common services with the CHPP app layer. These services are
 * enabled by CHPP_SERVICE_ENABLED_xxxx definitions. This function is
 * automatically called by chppAppInit().
 *
 * @param context Maintains status for each app layer instance.
 */
void chppDeregisterCommonServices(struct ChppAppState *context);

/**
 * Registers a new service on CHPP. This function is to be called by the
 * platform initialization code for every non-common service available on a
 * server (if any), i.e. except those that are registered through
 * chppRegisterCommonServices().
 *
 * Note that the maximum number of services that can be registered on a platform
 * can specified as CHPP_MAX_REGISTERED_SERVICES by the initialization code.
 * Otherwise, a default value will be used.
 *
 * @param appContext Maintains status for each app layer instance.
 * @param serviceContext Maintains status for each service instance.
 * @param newService The service to be registered on this platform.
 *
 * @return Handle number of the registered service.
 */
uint8_t chppRegisterService(struct ChppAppState *appContext,
                            void *serviceContext,
                            const struct ChppService *newService);

/**
 * Allocates a service notification of a specified length.
 *
 * It is expected that for most use cases, the
 * chppAllocServiceNotificationFixed() or
 * chppAllocServiceNotificationTypedArray() macros shall be used rather than
 * calling this function directly.
 *
 * @param len Length of the notification (including header) in bytes. Note
 * that the specified length must be at least equal to the lendth of the app
 * layer header.
 *
 * @return Pointer to allocated memory
 */
struct ChppAppHeader *chppAllocServiceNotification(size_t len);

/**
 * Allocates a service response message of a specified length, populating the
 * (app layer) service response header accorging to the provided client request
 * (app layer) header.
 *
 * It is expected that for most use cases, the chppAllocServiceResponseFixed()
 * or chppAllocServiceResponseTypedArray() macros shall be used rather than
 * calling this function directly.
 *
 * @param requestHeader Client request header.
 * @param len Length of the response message (including header) in bytes. Note
 * that the specified length must be at least equal to the lendth of the app
 * layer header.
 *
 * @return Pointer to allocated memory
 */
struct ChppAppHeader *chppAllocServiceResponse(
    const struct ChppAppHeader *requestHeader, size_t len);

/**
 * This function shall be called for all incoming client requests in order to
 * A) Timestamp them, and
 * B) Save their Transaction ID
 * as part of the request/response's ChppRequestResponseState struct.
 *
 * This function prints an error message if a duplicate request is received
 * while outstanding request is still pending without a response.
 *
 * @param rRState Maintains the basic state for each request/response
 * functionality of a service.
 * @param requestHeader Client request header.
 */
void chppServiceTimestampRequest(struct ChppRequestResponseState *rRState,
                                 struct ChppAppHeader *requestHeader);

/**
 * This function shall be called for the final service response to a client
 * request in order to
 * A) Timestamp them, and
 * B) Mark them as fulfilled
 * part of the request/response's ChppRequestResponseState struct.
 *
 * This function prints an error message if a response is attempted without an
 * outstanding request.
 *
 * For most responses, it is expected that chppSendTimestampedResponseOrFail()
 * shall be used to both timestamp and send the response in one shot.
 *
 * @param rRState Maintains the basic state for each request/response
 * functionality of a service.
 */
void chppServiceTimestampResponse(struct ChppRequestResponseState *rRState);

/**
 * Timestamps a service response using chppServiceTimestampResponse() and
 * enqueues it using chppEnqueueTxDatagramOrFail().
 *
 * Refer to their respective documentation for details.
 *
 * @param serviceState State of the service sending the response service.
 * @param rRState Maintains the basic state for each request/response
 * functionality of a service.
 * @param buf Datagram payload allocated through chppMalloc. Cannot be null.
 * @param len Datagram length in bytes.
 *
 * @return True informs the sender that the datagram was successfully enqueued.
 * False informs the sender that the queue was full and the payload discarded.
 */
bool chppSendTimestampedResponseOrFail(struct ChppServiceState *serviceState,
                                       struct ChppRequestResponseState *rRState,
                                       void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_SERVICES_H_
