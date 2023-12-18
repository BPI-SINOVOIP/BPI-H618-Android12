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

#include "watchdog.h"

#include <chrono>
#include <functional>

#include <systemd/sd-daemon.h>

namespace android::automotive::agl::utils {

SystemdWatchdog::SystemdWatchdog() : mThread(std::bind(&SystemdWatchdog::WatchdogThread, this)) {}

void SystemdWatchdog::WatchdogThread() {
    uint64_t usec = 0;
    int r = sd_watchdog_enabled(0, &usec);
    if (r < 0) {
        fprintf(stderr, "watchdog error: %d\n", r);
        return;
    }
    // function returned, but watchdog not applicable
    if (r == 0) return;
    if (usec == 0) {
        fprintf(stderr, "watchdog interval of 0 does not make sense!\n");
        return;
    }

    usec = (2 * usec) / 3;  // give us breathing room here
    if (usec == 0) usec = 1;

    sd_notify(0, "READY=1");

    std::chrono::duration<uint64_t, std::micro> mInterval{usec};
    while (true) {
        std::this_thread::sleep_for(mInterval);
        if (IsHealthy())
            sd_notify(0, "WATCHDOG=1");
        else
            sd_notify(0, "WATCHDOG=trigger");
    }
}

}  // namespace android::automotive::agl::utils
