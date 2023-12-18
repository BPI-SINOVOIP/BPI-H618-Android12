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

#include "chpp/link.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/log.h"
#include "chpp/macros.h"
#include "chpp/transport.h"

// The set of signals to use for the linkSendThread.
#define SIGNAL_EXIT UINT32_C(1 << 0)
#define SIGNAL_DATA UINT32_C(1 << 1)

/**
 * This thread is used to "send" TX data to the remote endpoint. The remote
 * endpoint is defined by the ChppTransportState pointer, so a loopback link
 * with a single CHPP instance can be supported.
 */
static void *linkSendThread(void *arg) {
  struct ChppPlatformLinkParameters *params =
      (struct ChppPlatformLinkParameters *)arg;
  while (true) {
    uint32_t signal = chppNotifierTimedWait(&params->notifier, CHPP_TIME_MAX);

    if (signal & SIGNAL_EXIT) {
      break;
    }
    if (signal & SIGNAL_DATA) {
      enum ChppLinkErrorCode error;

      chppMutexLock(&params->mutex);

      if (params->remoteTransportContext == NULL) {
        CHPP_LOGW("remoteTransportContext is NULL");
        error = CHPP_LINK_ERROR_NONE_SENT;

      } else if (!params->linkEstablished) {
        CHPP_LOGE("No (fake) link");
        error = CHPP_LINK_ERROR_NO_LINK;

      } else if (!chppRxDataCb(params->remoteTransportContext, params->buf,
                               params->bufLen)) {
        CHPP_LOGW("chppRxDataCb return state!=preamble (packet incomplete)");
        error = CHPP_LINK_ERROR_NONE_SENT;

      } else {
        error = CHPP_LINK_ERROR_NONE_SENT;
      }

      params->bufLen = 0;
      chppLinkSendDoneCb(params, error);

      chppMutexUnlock(&params->mutex);
    }
  }

  return NULL;
}

void chppPlatformLinkInit(struct ChppPlatformLinkParameters *params) {
  params->bufLen = 0;
  chppMutexInit(&params->mutex);
  chppNotifierInit(&params->notifier);
  pthread_create(&params->linkSendThread, NULL /* attr */, linkSendThread,
                 params);
  if (params->linkThreadName != NULL) {
    pthread_setname_np(params->linkSendThread, params->linkThreadName);
  }
}

void chppPlatformLinkDeinit(struct ChppPlatformLinkParameters *params) {
  params->bufLen = 0;
  chppNotifierSignal(&params->notifier, SIGNAL_EXIT);
  pthread_join(params->linkSendThread, NULL /* retval */);
  chppNotifierDeinit(&params->notifier);
  chppMutexDeinit(&params->mutex);
}

enum ChppLinkErrorCode chppPlatformLinkSend(
    struct ChppPlatformLinkParameters *params, uint8_t *buf, size_t len) {
  bool success = false;
  chppMutexLock(&params->mutex);
  if (params->bufLen != 0) {
    CHPP_LOGE("Failed to send data - link layer busy");
  } else {
    success = true;
    memcpy(params->buf, buf, len);
    params->bufLen = len;
  }
  chppMutexUnlock(&params->mutex);

  if (success) {
    chppNotifierSignal(&params->notifier, SIGNAL_DATA);
  }

  return success ? CHPP_LINK_ERROR_NONE_QUEUED : CHPP_LINK_ERROR_BUSY;
}

void chppPlatformLinkDoWork(struct ChppPlatformLinkParameters *params,
                            uint32_t signal) {
  UNUSED_VAR(params);
  UNUSED_VAR(signal);
}

void chppPlatformLinkReset(struct ChppPlatformLinkParameters *params) {
  chppPlatformLinkDeinit(params);
  chppPlatformLinkInit(params);
}
