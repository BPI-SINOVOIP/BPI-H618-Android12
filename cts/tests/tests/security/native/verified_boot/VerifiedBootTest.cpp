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

#include <string>

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <fs_mgr.h>
#include <fstab/fstab.h>
#include <gtest/gtest.h>

#include "utils.h"

// The relevant Android API levels
constexpr auto S_API_LEVEL = 31;

static int getFirstApiLevel() {
  int level = android::base::GetIntProperty("ro.product.first_api_level", 0);
  if (level == 0) {
    level = android::base::GetIntProperty("ro.build.version.sdk", 0);
  }
  if (level == 0) {
    ADD_FAILURE() << "Failed to determine first API level";
  }
  return level;
}

// As required by CDD, verified boot MUST use verification algorithms as strong
// as current recommendations from NIST for hashing algorithms (SHA-256).
// https://source.android.com/compatibility/11/android-11-cdd#9_10_device_integrity
TEST(VerifiedBootTest, avbHashtreeNotUsingSha1) {
  int first_api_level = getFirstApiLevel();
  GTEST_LOG_(INFO) << "First API level is " << first_api_level;
  if (first_api_level < S_API_LEVEL) {
    GTEST_LOG_(INFO)
        << "Exempt from avb hash tree test due to old starting API level";
    return;
  }

  // This feature name check only applies to devices that first shipped with
  // SC or later. The check above already screens out pre-S devices.
  if(!deviceSupportsFeature("android.hardware.security.model.compatible")) {
      GTEST_SKIP()
          << "Skipping test: FEATURE_SECURITY_MODEL_COMPATIBLE missing.";
    return;
  }
  android::fs_mgr::Fstab fstab;
  ASSERT_TRUE(ReadDefaultFstab(&fstab)) << "Failed to read default fstab";

  for (const auto& entry : fstab) {
    if (!entry.fs_mgr_flags.verify && !entry.fs_mgr_flags.avb) {
      continue;
    }

    if (android::base::EqualsIgnoreCase(entry.fs_type, "emmc")) {
      GTEST_LOG_(INFO) << entry.mount_point << " has emmc fs_type, skipping"
          << " hashtree algorithm verification";
      continue;
    }

    GTEST_LOG_(ERROR) << "partition enabled verity " << entry.mount_point;

    // The verity sysprop use "system" as the partition name in the system as
    // root case.
    std::string partition = entry.mount_point == "/"
                                ? "system"
                                : android::base::Basename(entry.mount_point);

    std::string alg_prop_name = "partition." + partition + ".verified.hash_alg";
    std::string hash_alg = android::base::GetProperty(alg_prop_name, "");
    ASSERT_FALSE(hash_alg.empty());
    ASSERT_FALSE(android::base::StartsWithIgnoreCase(hash_alg, "sha1"));
  }
}
