/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <android-base/macros.h>
#include <android-base/unique_fd.h>


//
// Input events.
//

struct input_event;

using ev_callback = std::function<int(int fd, uint32_t epevents)>;
using ev_set_key_callback = std::function<int(int code, int value)>;

int ev_init(ev_callback input_cb, bool allow_touchpad);
void ev_exit();
int ev_add_fd(android::base::unique_fd&& fd, ev_callback cb);
void ev_iterate_available_keys(const std::function<void(int)>& f);
void ev_iterate_touch_inputs(const std::function<void(int)>& action);
int ev_sync_key_state(const ev_set_key_callback& set_key_cb);

// 'timeout' has the same semantics as poll(2).
//    0 : don't block
//  < 0 : block forever
//  > 0 : block for 'timeout' milliseconds
int ev_wait(int timeout);

int ev_get_input(int fd, uint32_t epevents, input_event* ev);
void ev_dispatch();
int ev_get_epollfd();

