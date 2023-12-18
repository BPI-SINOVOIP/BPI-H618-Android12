/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef INPUTEVENTS_H
#define INPUTEVENTS_H
#include <linux/input.h>  // KEY_MAX

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <androidfw/AssetManager.h>
#include <utils/Thread.h>
#include <binder/IBinder.h>

#include <media/mediaplayer.h>

namespace android {

class InputEvents : virtual public RefBase
{
public:
    InputEvents(sp<MediaPlayer> player);
    ~InputEvents();

    bool Init();
    bool HasThreeButtons() const;

private:
    int key_last_down;              // under key_queue_mutex
    sp<MediaPlayer> mPlayer;
    float volume;

    std::thread input_thread_;
    std::thread device_thread_;
    std::atomic<bool> input_thread_stopped_{ false };

    void OnKeyDetected(int key_code);
    int OnInputEvent(int fd, uint32_t epevents);
    void ProcessKey(int key_code, int updown);
};

// ---------------------------------------------------------------------------

} // namespace android

//
// Input events.
//

struct input_event;

using ev_callback = std::function<int(int fd, uint32_t epevents)>;
using ev_set_key_callback = std::function<int(int code, int value)>;

int ev_init(ev_callback input_cb, bool allow_touch_inputs = false);
void ev_exit();
int ev_add_fd(android::base::unique_fd&& fd, ev_callback cb);
void ev_iterate_available_keys(const std::function<void(int)>& f);
void ev_iterate_touch_inputs(const std::function<void(int)>& action);
int ev_sync_key_state(const ev_set_key_callback& set_key_cb);
int ev_create_inotify(void);
int ev_listen(void);
void ev_set_callback(ev_callback cb);
static int ifd, wfd;
static ev_callback input_cb_save;

// 'timeout' has the same semantics as poll(2).
//    0 : don't block
//  < 0 : block forever
//  > 0 : block for 'timeout' milliseconds
int ev_wait(int timeout);

int ev_get_input(int fd, uint32_t epevents, input_event* ev);
void ev_dispatch();
int ev_get_epollfd();

#endif // INPUTEVENTS_H