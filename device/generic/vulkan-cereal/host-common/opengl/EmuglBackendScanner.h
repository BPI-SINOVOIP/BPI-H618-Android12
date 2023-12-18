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

#pragma once

#include <string>
#include <vector>

namespace android {
namespace opengl {

class EmuglBackendScanner {
public:
// Scan |execDir| for sub-directories named like <lib>/gles_<name> where
// <lib> is either 'lib' or 'lib64' depending on current host bitness,
// and <name> is an arbitrary names. Returns a vector of matching <name>
// strings, which will be empty on error.
// If |programBitness| is 0, the this will use the current program
// bitness, otherwise the value must be either 32 or 64.
    static std::vector<std::string> scanDir(const char* execDir,
                                            int programBitness = 0);
};

}  // namespace opengl
}  // namespace android
