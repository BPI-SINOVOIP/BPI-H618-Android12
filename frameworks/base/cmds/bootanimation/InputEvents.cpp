/*
 * Copyright (C) 2014 The Android Open Source Project
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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include "InputEvents.h"

namespace android {

InputEvents::InputEvents(sp<MediaPlayer> player)
    :   key_last_down(-1),
        mPlayer(player),
        volume(1.0)
{
}

InputEvents::~InputEvents() {
    ev_exit();
    input_thread_stopped_ = true;
    if (input_thread_.joinable()) {
        input_thread_.join();
    }
}

bool InputEvents::Init() {
    if (ev_init(std::bind(&InputEvents::OnInputEvent, this, std::placeholders::_1, std::placeholders::_2), false)) {
        ALOGE("ev_init fail");
        return false;
    }

    // Create a separate thread that handles input events.
    input_thread_ = std::thread([this]() {
        while (!this->input_thread_stopped_) {
            if (!ev_wait(500)) {
                ev_dispatch();
            }
        }
    });

    if (ev_listen()) {
        ALOGE("Failed to listen device event");
        return false;
    }

    return true;
}

int InputEvents::OnInputEvent(int fd, uint32_t epevents) {
    struct input_event ev;
    if (ev_get_input(fd, epevents, &ev) == -1) {
        return -1;
    }

    //ALOGD("ev.type = %d, ev.code = %d, ev.value = %d", ev.type, ev.code, ev.value);

    if (ev.type == EV_SYN) {
        return 0;
    }

    if (ev.type == EV_KEY && ev.code <= KEY_MAX) {
        ProcessKey(ev.code, ev.value);
    }
    return 0;
}

void InputEvents::ProcessKey(int key_code, int updown) {
    bool register_key = false;

    if (updown) {
        key_last_down = key_code;
    } else {
        if (key_last_down == key_code) {
            register_key = true;
        }
        key_last_down = -1;
    }

    if (register_key) {
        if (key_code == KEY_P) {
            if (volume >= 0) {
                ALOGD("KEY_P down");
                volume -= 0.1;
                mPlayer->setVolume(volume, volume);
            } else {
                mPlayer->setVolume(0, 0);
            }
        } else if (key_code == KEY_O) {
            if (volume < 1.1) {
                ALOGD("KEY_O up");
                volume += 0.1;
                mPlayer->setVolume(volume, volume);
            } else {
                mPlayer->setVolume(1.0, 1.0);
            }
        }
    }
}


// ---------------------------------------------------------------------------

}; // namespace android