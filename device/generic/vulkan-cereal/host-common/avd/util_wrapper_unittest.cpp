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

#include "android/avd/util.h"
#include "android/utils/path.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

using namespace android::base;

TEST(AvdUtilWrapper, emulator_path_getSdkRoot) {
  TestSystem sys("", 32, "/myhome");
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk/platform-tools"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk/platforms"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk/tools"));
  ASSERT_TRUE(sys.pathIsDir("/tmp/Sdk"));

  sys.setLauncherDirectory(".");
  sys.setCurrentDirectory("/tmp/Sdk/tools");

  char* sdk = path_getSdkRoot();
  EXPECT_TRUE(sdk != nullptr);
  EXPECT_TRUE(path_is_absolute(sdk));
  free(sdk);
}


