/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define LOG_TAG "BpfCompatTest"

#include <fstream>

#include <gtest/gtest.h>

#include "libbpf_android.h"

using namespace android::bpf;

namespace android {

void doBpfStructSizeTest(const char *elfPath) {
  std::ifstream elfFile(elfPath, std::ios::in | std::ios::binary);
  ASSERT_TRUE(elfFile.is_open());

  EXPECT_EQ(48, readSectionUint("size_of_bpf_map_def", elfFile, 0));
  EXPECT_EQ(28, readSectionUint("size_of_bpf_prog_def", elfFile, 0));
}

TEST(BpfTest, bpfStructSizeTest) {
  doBpfStructSizeTest("/system/etc/bpf/netd.o");
  doBpfStructSizeTest("/system/etc/bpf/clatd.o");
}

}  // namespace android
