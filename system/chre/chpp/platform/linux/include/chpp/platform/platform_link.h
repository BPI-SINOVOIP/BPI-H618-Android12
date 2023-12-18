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

#ifndef CHPP_PLATFORM_LINK_H_
#define CHPP_PLATFORM_LINK_H_

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "chpp/mutex.h"
#include "chpp/notifier.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHPP_PLATFORM_LINK_TX_MTU_BYTES 1280
#define CHPP_PLATFORM_LINK_RX_MTU_BYTES 1280

#define CHPP_PLATFORM_TRANSPORT_TIMEOUT_MS 1000

// Forward declaration
struct ChppTransportState;

struct ChppPlatformLinkParameters {
  //! Indicates that the link to the remote endpoint has been established.
  //! This simulates the establishment of the physical link, so
  //! chppPlatformLinkSend() will fail if this field is set to false.
  bool linkEstablished;

  //! A pointer to the transport context of the remote endpoint.
  struct ChppTransportState *remoteTransportContext;

  //! A thread to use when sending data to the remote endpoint asynchronously.
  pthread_t linkSendThread;

  //! The notifier for linkSendThread.
  struct ChppNotifier notifier;

  //! The mutex to protect buf/bufLen.
  struct ChppMutex mutex;

  //! The temporary buffer to use to send data to the remote endpoint.
  uint8_t buf[CHPP_PLATFORM_LINK_TX_MTU_BYTES];
  size_t bufLen;

  //! The string name of the linkSendThread.
  const char *linkThreadName;

  //! The string name of the CHPP work thread.
  const char *workThreadName;
};

#ifdef __cplusplus
}
#endif

#endif  // CHPP_PLATFORM_LINK_H_
