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

#ifndef LOG_TAG
#define LOG_TAG "bpfloader"
#endif

#include <arpa/inet.h>
#include <dirent.h>
#include <elf.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/bpf.h>
#include <linux/unistd.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <libbpf_android.h>
#include <log/log.h>
#include <netdutils/Misc.h>
#include <netdutils/Slice.h>
#include "bpf/BpfUtils.h"

using android::base::EndsWith;
using std::string;

struct {
    const char* const dir;
    const char* const prefix;
} locations[] = {
        // Tethering mainline module
        {
                .dir = "/apex/com.android.tethering/etc/bpf/",
                .prefix = "tethering/",
        },
        // Core operating system
        {
                .dir = "/system/etc/bpf/",
                .prefix = "",
        },
};

int loadAllElfObjects(const char* const progDir, const char* const prefix) {
    int retVal = 0;
    DIR* dir;
    struct dirent* ent;

    if ((dir = opendir(progDir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string s = ent->d_name;
            if (!EndsWith(s, ".o")) continue;

            string progPath(progDir);
            progPath += s;

            bool critical;
            int ret = android::bpf::loadProg(progPath.c_str(), &critical, prefix);
            if (ret) {
                if (critical) retVal = ret;
                ALOGE("Failed to load object: %s, ret: %s", progPath.c_str(), std::strerror(-ret));
            } else {
                ALOGI("Loaded object: %s", progPath.c_str());
            }
        }
        closedir(dir);
    }
    return retVal;
}

void createSysFsBpfSubDir(const char* const prefix) {
    if (*prefix) {
        mode_t prevUmask = umask(0);

        string s = "/sys/fs/bpf/";
        s += prefix;

        errno = 0;
        int ret = mkdir(s.c_str(), S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO);
        if (ret && errno != EEXIST) {
            ALOGW("Failed to create directory: %s, ret: %s", s.c_str(), std::strerror(errno));
        }

        umask(prevUmask);
    }
}

int main() {
    // Load all ELF objects, create programs and maps, and pin them
    for (const auto location : locations) {
        createSysFsBpfSubDir(location.prefix);
        if (loadAllElfObjects(location.dir, location.prefix) != 0) {
            ALOGE("=== CRITICAL FAILURE LOADING BPF PROGRAMS FROM %s ===", location.dir);
            ALOGE("If this triggers reliably, you're probably missing kernel options or patches.");
            ALOGE("If this triggers randomly, you might be hitting some memory allocation "
                  "problems or startup script race.");
            ALOGE("--- DO NOT EXPECT SYSTEM TO BOOT SUCCESSFULLY ---");
            sleep(20);
            return 2;
        }
    }

    if (android::base::SetProperty("bpf.progs_loaded", "1") == false) {
        ALOGE("Failed to set bpf.progs_loaded property");
        return 1;
    }

    return 0;
}
