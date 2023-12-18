/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <string>
#include <string_view>

namespace android {
namespace derive_classpath {

constexpr std::string_view kGeneratedClasspathExportsFilepath = "/data/system/environ/classpath";

bool GenerateClasspathExports(std::string_view output_path = kGeneratedClasspathExportsFilepath);

// This must only be used in tests.
bool GenerateClasspathExports(const std::string& globPatternPrefix,
                              std::string_view output_path = kGeneratedClasspathExportsFilepath);

}  // namespace derive_classpath
}  // namespace android
