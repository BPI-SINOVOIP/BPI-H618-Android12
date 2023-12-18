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

#include <dirent.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <modprobe/modprobe.h>

#include "android-base/logging.h"
#include "android-base/strings.h"

using namespace android::base;

static constexpr const char MODULE_BASE_DIR[] = "/lib/modules";

bool LoadKernelModules() {
    struct utsname uts;
    if (uname(&uts)) {
        LOG(ERROR) << "Failed to get kernel version.";
        return false;
    }
    int major, minor;
    if (sscanf(uts.release, "%d.%d", &major, &minor) != 2) {
        LOG(ERROR) << "Failed to parse kernel version " << uts.release;
        return false;
    }

    std::unique_ptr<DIR, decltype(&closedir)> base_dir(opendir(MODULE_BASE_DIR), closedir);
    if (!base_dir) {
        LOG(ERROR) << "Unable to open /lib/modules, skipping module loading.";
        return false;
    }
    dirent* entry;
    std::vector<std::string> module_dirs;
    while ((entry = readdir(base_dir.get()))) {
        if (entry->d_type != DT_DIR) {
            continue;
        }
        int dir_major, dir_minor;
        if (sscanf(entry->d_name, "%d.%d", &dir_major, &dir_minor) != 2 || dir_major != major ||
            dir_minor != minor) {
            continue;
        }
        module_dirs.emplace_back(entry->d_name);
    }

    // Sort the directories so they are iterated over during module loading
    // in a consistent order. Alphabetical sorting is fine here because the
    // kernel version at the beginning of the directory name must match the
    // current kernel version, so the sort only applies to a label that
    // follows the kernel version, for example /lib/modules/5.4 vs.
    // /lib/modules/5.4-gki.
    std::sort(module_dirs.begin(), module_dirs.end());

    for (const auto& module_dir : module_dirs) {
        std::string dir_path(MODULE_BASE_DIR);
        dir_path.append("/");
        dir_path.append(module_dir);
        Modprobe m({dir_path});
        bool retval = m.LoadListedModules();
        int modules_loaded = m.GetModuleCount();
        if (modules_loaded > 0) {
            return retval;
        }
    }

    Modprobe m({MODULE_BASE_DIR});
    bool retval = m.LoadListedModules();
    int modules_loaded = m.GetModuleCount();
    if (modules_loaded > 0) {
        return retval;
    }

    return true;
}

int main(int argc, const char* argv[]) {
    SetLogger(StderrLogger);

    LOG(INFO) << "Guest VM init process";
    LOG(INFO) << "Command line: " << Join(std::vector(argv, argv + argc), " ");

    if (clearenv() != EXIT_SUCCESS) {
        PLOG(ERROR) << "clearenv";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Loading kernel modules...";
    if (!LoadKernelModules()) {
        LOG(ERROR) << "LoadKernelModules failed";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Executing test binary " << argv[1] << "...";
    execv(argv[1], (char**)(argv + 1));

    PLOG(ERROR) << "execv";
    return EXIT_FAILURE;
}
