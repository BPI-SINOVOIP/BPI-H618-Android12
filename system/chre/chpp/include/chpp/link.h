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

/*
 * Implementation Notes:
 * Each platform must supply a platform-specific platform_link.h to provide the
 * definitions and a platform-specific link.c to provide the implementation for
 * the definitions in this file.
 * The platform must also initialize the ChppPlatformLinkParameters for each
 * link (context.linkParams).
 */

#ifndef CHPP_LINK_H_
#define CHPP_LINK_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error codes used by the link layer.
 */
enum ChppLinkErrorCode {
  //! No error - data queued to be sent asynchronously
  CHPP_LINK_ERROR_NONE_QUEUED = 0,

  //! No error - data successfully sent
  CHPP_LINK_ERROR_NONE_SENT = 1,

  //! Timeout
  CHPP_LINK_ERROR_TIMEOUT = 2,

  //! Busy
  CHPP_LINK_ERROR_BUSY = 3,

  //! Out of memory
  CHPP_LINK_ERROR_OOM = 4,

  //! Link not established
  CHPP_LINK_ERROR_NO_LINK = 5,

  //! Unspecified failure
  CHPP_LINK_ERROR_UNSPECIFIED = 255
};

/*
 * Platform-specific struct with link details / parameters.
 */
struct ChppPlatformLinkParameters;

/**
 * Platform-specific function to initialize the link layer.
 *
 * @param params Platform-specific struct with link details / parameters.
 */
void chppPlatformLinkInit(struct ChppPlatformLinkParameters *params);

/**
 * Platform-specific function to deinitialize the link layer (e.g. clean exit).
 *
 * @param params Platform-specific struct with link details / parameters.
 */
void chppPlatformLinkDeinit(struct ChppPlatformLinkParameters *params);

/*
 * Platform-specific function to send Tx data over to the link layer.
 *
 * @param params Platform-specific struct with link details / parameters.
 * @param buf Data to be sent.
 * @param len Length of the data to be sent in bytes.
 *
 * @return CHPP_LINK_ERROR_NONE_SENT if the platform implementation for this
 * function is synchronous, i.e. it is done with buf and len once the function
 * returns. A return value of CHPP_LINK_ERROR_NONE_QUEUED indicates that this
 * function is implemented asynchronously. In this case, it is up to the
 * platform implementation to call chppLinkSendDoneCb() after processing the
 * contents of buf and len. Otherwise, an error code is returned per enum
 * ChppLinkErrorCode.
 */
enum ChppLinkErrorCode chppPlatformLinkSend(
    struct ChppPlatformLinkParameters *params, uint8_t *buf, size_t len);

/**
 * Platform-specific function to perform a task from the main CHPP transport
 * work thread. The task can be specified by the signal argument, which is
 * triggered by previous call[s] to chppWorkThreadSignalFromLink(). An example
 * of the type of work that can be performed is processing RX data from the
 * physical layer.
 *
 * @param params Platform-specific struct with link details / parameters.
 * @param signal The signal that describes the work to be performed. Only bits
 * specified by CHPP_TRANSPORT_SIGNAL_PLATFORM_MASK can be set.
 */
void chppPlatformLinkDoWork(struct ChppPlatformLinkParameters *params,
                            uint32_t signal);

/*
 * Platform-specific function to reset a non-synchronous link, where the link
 * implementation is responsible for calling chppLinkSendDoneCb() after
 * processing the contents of buf and len. For such links, a reset called before
 * chppLinkSendDoneCb() indicates to the link to abort sending out buf, and that
 * the contents of buf and len will become invalid.
 *
 * @param params Platform-specific struct with link details / parameters.
 */
void chppPlatformLinkReset(struct ChppPlatformLinkParameters *params);

#ifdef __cplusplus
}
#endif

#include "chpp/platform/platform_link.h"

#endif  // CHPP_LINK_H_
