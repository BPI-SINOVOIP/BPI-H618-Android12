/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <fstream>
#include <string>

#include <android-base/properties.h>
#include <android/api-level.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android {
namespace kernel {

class KernelVersionTest : public ::testing::Test {
 protected:
  const std::string arch_;
  const int first_api_level_;
  const bool should_run_compiler_test_;
  const bool should_run_linker_test_;
  std::string version_;
  KernelVersionTest()
      : arch_(android::base::GetProperty("ro.bionic.arch", "")),
        first_api_level_(std::stoi(
            android::base::GetProperty("ro.product.first_api_level", "0"))),
        should_run_compiler_test_(
            first_api_level_ >= __ANDROID_API_R__ ||
            (arch_ == "arm64" && first_api_level_ >= __ANDROID_API_Q__)),
        should_run_linker_test_(first_api_level_ >= __ANDROID_API_S__) {
    std::ifstream proc_version("/proc/version");
    std::getline(proc_version, version_);
  }
};

TEST_F(KernelVersionTest, IsntGCC) {
  if (!should_run_compiler_test_) return;
  const std::string needle = "gcc version";
  ASSERT_THAT(version_, ::testing::Not(::testing::HasSubstr(needle)));
}

TEST_F(KernelVersionTest, IsClang) {
  if (!should_run_compiler_test_) return;
  const std::string needle = "clang version";
  ASSERT_THAT(version_, ::testing::HasSubstr(needle));
}

TEST_F(KernelVersionTest, IsntBFD) {
  if (!should_run_linker_test_) return;
  const std::string needle = "GNU ld";
  ASSERT_THAT(version_, ::testing::Not(::testing::HasSubstr(needle)));
  ASSERT_THAT(version_, ::testing::Not(::testing::HasSubstr("GNU Binutils")));
  ASSERT_THAT(version_, ::testing::Not(::testing::HasSubstr("binutils")));
}

TEST_F(KernelVersionTest, IsntGold) {
  if (!should_run_linker_test_) return;
  const std::string needle = "GNU gold";
  ASSERT_THAT(version_, ::testing::Not(::testing::HasSubstr(needle)));
}

TEST_F(KernelVersionTest, IsLLD) {
  if (!should_run_linker_test_) return;
  const std::string needle = "LLD";
  ASSERT_THAT(version_, ::testing::HasSubstr(needle));
}

}  // namespace kernel
}  // namespace android
