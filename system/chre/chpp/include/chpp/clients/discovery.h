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

#ifndef CHPP_CLIENT_DISCOVERY_H_
#define CHPP_CLIENT_DISCOVERY_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public functions
 ***********************************************/

/**
 * CHPP discovery state initialization that should be called on CHPP startup. It
 * may be called during transient internal resets, but is a no-op.
 */
void chppDiscoveryInit(struct ChppAppState *context);

/**
 * CHPP discovery state de-initialization that should be called during shutdown.
 */
void chppDiscoveryDeinit(struct ChppAppState *context);

/**
 * A method that can be invoked to block until the CHPP discovery sequence
 * completes. This can be useful to wait until CHPP client invocations can
 * succeed.
 *
 * @param context The non-null pointer to the ChppAppState of this instance.
 * @param timeoutMs The timeout in milliseconds.
 *
 * @return False if timed out waiting for discovery completion.
 */
bool chppWaitForDiscoveryComplete(struct ChppAppState *context,
                                  uint64_t timeoutMs);

/**
 * Dispatches an Rx Datagram from the transport layer that is determined to be
 * for the CHPP Discovery Client.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input (request) datagram. Cannot be null.
 * @param len Length of input data in bytes.
 */
bool chppDispatchDiscoveryServiceResponse(struct ChppAppState *context,
                                          const uint8_t *buf, size_t len);

/**
 * Initiates a CHPP service discovery from the client side, in order to send a
 * CHPP_DISCOVERY_COMMAND_DISCOVER_ALL client request to a server. It is
 * expected that this function be called upon initialization, after sending or
 * receiving a reset-ack.
 *
 * @param context Maintains status for each app layer instance.
 */
void chppInitiateDiscovery(struct ChppAppState *context);

/**
 * Checks if all discovery clients have been matched with a remote service.
 *
 * @param context Maintains status for each app layer instance.
 *
 * @return true if all registered clients have been matched by a discovered
 * service.
 */
bool chppAreAllClientsMatched(struct ChppAppState *context);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_CLIENT_DISCOVERY_H_
