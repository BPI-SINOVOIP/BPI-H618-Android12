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

#ifndef IORAP_UTILS_PROPERTY_H_
#define IORAP_UTILS_PROPERTY_H_

#include <android-base/properties.h>
#include <server_configurable_flags/get_flags.h>

namespace iorap::common {

constexpr const char* ph_namespace = "runtime_native_boot";

inline bool IsTracingEnabled(const std::string& default_value) {
  return server_configurable_flags::GetServerConfigurableFlag(
      ph_namespace,
      "iorap_perfetto_enable",
      ::android::base::GetProperty("iorapd.perfetto.enable", default_value)) == "true";
}

inline bool IsReadAheadEnabled(const std::string& default_value) {
  return server_configurable_flags::GetServerConfigurableFlag(
      ph_namespace,
      "iorap_readahead_enable",
      ::android::base::GetProperty("iorapd.readahead.enable", default_value)) == "true";
}

inline bool ExcludeDexFiles(bool default_value) {
  return ::android::base::GetBoolProperty("iorapd.exclude_dex_files", default_value);
}

}  // namespace iorap::common

#endif  // IORAP_UTILS_PROPERTY_H_
