/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <cutils/properties.h>
// AW:Added for BOOTEVENT
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "StartPropertySetThread.h"

namespace android {

StartPropertySetThread::StartPropertySetThread(bool timestampPropertyValue):
        Thread(false), mTimestampPropertyValue(timestampPropertyValue) {}

status_t StartPropertySetThread::Start() {
    return run("SurfaceFlinger::StartPropertySetThread", PRIORITY_NORMAL);
}

bool StartPropertySetThread::threadLoop() {
    // Set property service.sf.present_timestamp, consumer need check its readiness
    property_set(kTimestampProperty, mTimestampPropertyValue ? "1" : "0");
    // Clear BootAnimation exit flag
    property_set("service.bootanim.exit", "0");
    property_set("service.bootanim.progress", "0");
    // Start BootAnimation if not started
    property_set("ctl.start", "bootanim");
    // Exit immediately
    return false;
}

// AW:Added for BOOTEVENT
void StartPropertySetThread::addBootEvent(int enable) {
    int fd;
    char buf[64];

    if (!property_get_bool("persist.sys.bootevent", false)) {
        return;
    }

    fd = open("/proc/bootevent", O_RDWR);
    if (fd == -1) return;

    if (1 == enable) {
        strcpy(buf,"BOOT_Animation:START");
        if (fd > 0) {
            write(fd, buf, 32);
            close(fd);
        }
    } else {
        strcpy(buf, "BOOT_Animation:END");
        if (fd > 0) {
            write(fd, buf, 32);
            //write(fd, "0", 1);
            close(fd);
        }
    }
}
} // namespace android
