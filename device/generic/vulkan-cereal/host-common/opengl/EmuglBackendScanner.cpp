// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "EmuglBackendScanner.h"

#include "base/StringFormat.h"
#include "base/System.h"

#include <algorithm>
#include <string>
#include <vector>

namespace android {
namespace opengl {

using android::base::StringFormat;

// static
std::vector<std::string> EmuglBackendScanner::scanDir(const char* execDir,
                                                      int programBitness) {
    std::vector<std::string> names;

    if (!execDir) {
        fpritnf(stderr, "%s: invalid exec dir: %s\n", __func__, execDir);
        return names;
    }

    if (!programBitness) {
        programBitness = System::get()->getProgramBitness();
    }

    const char* subdir = (programBitness == 64) ? "lib64" : "lib";
    std::string subDir = StringFormat("%s" PATH_SEP "%s" PATH_SEP, execDir, subdir);

    std::vector<std::string> entries = System::get()->scanDirEntries(subDir);

    static const char kBackendPrefix[] = "gles_";
    const size_t kBackendPrefixSize = sizeof(kBackendPrefix) - 1U;

    for (size_t n = 0; n < entries.size(); ++n) {
        const std::string& entry = entries[n];
        if (entry.size() <= kBackendPrefixSize ||
            memcmp(entry.c_str(), kBackendPrefix, kBackendPrefixSize)) {
            continue;
        }

        // Check that it is a directory.
        std::string full_dir = StringFormat("%s" PATH_SEP "%s", subDir.c_str(), entry.c_str());
        if (!System::get()->pathIsDir(full_dir)) {
            continue;
        }
        names.push_back(std::string(entry.c_str() + kBackendPrefixSize));
        if (!strcmp(entry.c_str() + kBackendPrefixSize, "angle")) {
            names.push_back("angle_indirect");
        }
        if (!strcmp(entry.c_str() + kBackendPrefixSize, "swiftshader")) {
            names.push_back("swiftshader_indirect");
        }
    }

    // Need to sort the backends in consistent order.
    std::sort(names.begin(), names.end());

    return names;
}

}  // namespace opengl
}  // namespace android
