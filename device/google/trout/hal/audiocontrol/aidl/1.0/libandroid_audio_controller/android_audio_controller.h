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

#pragma once

#include <stdbool.h>

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    aafc_audio_usage_t audio_usage;
    aafc_zone_id_t zone_id;
    bool allow_duck;
    bool is_transient;
    bool is_exclusive;
} aafc_audio_focus_request_t;

/* Initialize the audio focus controller before using, return 0 when success.
 * Otherwise return value will be -ERROR_CODE
 *
 * This should be called before starting sending any audio focus requests
 * (mandatory and the only recommended usage),
 * or it may be called to update the address when the caller is absolutely
 * sure that there are no existing sessions or any concurrent focus requests
 * in this process (NOT RECOMMENDED because it is error-prone). Its behavior
 * is undefined if running concurrently with other requests or active sessions
 * in the same process. */
int aafc_init_audio_focus_controller(const char* audio_control_server_addr);

/* Acquire audio focus from Android AudioControl HAL.
 * This call will return immediately with a globally unique
 * session ID. Return AAFC_SESSION_ID_INVALID when error.
 */
aafc_session_id_t aafc_acquire_audio_focus(aafc_audio_focus_request_t);

/* Release the audio focus of the specified session.
 * This call will return immediately. Invalid session ID will be ignored.
 */
void aafc_release_audio_focus(aafc_session_id_t session_id);

#ifdef __cplusplus
}  // extern "C"
#endif
