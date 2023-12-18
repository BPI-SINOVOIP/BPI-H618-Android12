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

#pragma once

#include <android-base/properties.h>
#include <android/api-level.h>

namespace android {
namespace modules {
namespace sdklevel {

// Checks if the codename is a matching or higher version than the device's
// codename.
static bool IsAtLeastPreReleaseCodename(const std::string &codename) {
  const std::string &deviceCodename =
      android::base::GetProperty("ro.build.version.codename", "");
  return "REL" != deviceCodename && deviceCodename.compare(codename) >= 0;
}

// Checks if the device is running on release version of Android R or newer.
static inline bool IsAtLeastR() { return android_get_device_api_level() >= 30; }

// Checks if the device is running on a pre-release version of Android S or a
// release version of Android S or newer.
static inline bool IsAtLeastS() { return android_get_device_api_level() >= 31; }

// Checks if the device is running on a pre-release version of Android T or a
// release version of Android T or newer.
static inline bool IsAtLeastT() { return IsAtLeastPreReleaseCodename("T"); }

} // namespace utils
} // namespace modules
} // namespace android
