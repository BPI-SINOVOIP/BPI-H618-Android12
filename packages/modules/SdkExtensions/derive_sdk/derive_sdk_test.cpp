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

#define LOG_TAG "derive_sdk_test"

#include "derive_sdk.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-modules-utils/sdk_level.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <cstdlib>

#include "packages/modules/common/proto/sdk.pb.h"

class DeriveSdkTest : public ::testing::Test {
 protected:
  void TearDown() override { android::derivesdk::SetSdkLevels("/apex"); }

  const std::string dir() { return std::string(dir_.path); }

  const std::string EtcDir(const std::string& apex) {
    return dir() + "/" + apex + "/etc";
  }

  void AddExtensionVersion(
      const int version,
      const std::unordered_map<SdkModule, int>& requirements) {
    ExtensionVersion* sdk = db_.add_versions();
    sdk->set_version(version);
    for (auto pair : requirements) {
      ExtensionVersion_ModuleRequirement* req = sdk->add_requirements();
      req->set_module(pair.first);
      req->mutable_version()->set_version(pair.second);
    }
    WriteProto(db_, EtcDir("com.android.sdkext") + "/extensions_db.pb");

    android::derivesdk::SetSdkLevels(dir());
  }

  void SetApexVersion(const std::string apex, int version) {
    SdkVersion sdk_version;
    sdk_version.set_version(version);
    WriteProto(sdk_version, EtcDir(apex) + "/sdkinfo.pb");

    android::derivesdk::SetSdkLevels(dir());
  }

  void WriteProto(const google::protobuf::MessageLite& proto,
                  const std::string& path) {
    std::string buf;
    proto.SerializeToString(&buf);
    std::string cmd("mkdir -p " + path.substr(0, path.find_last_of('/')));
    ASSERT_EQ(0, system(cmd.c_str()));
    ASSERT_TRUE(android::base::WriteStringToFile(buf, path, true));
  }

  void EXPECT_R(int n) {
    int R = android::base::GetIntProperty("build.version.extensions.r", -1);
    EXPECT_EQ(R, n);
  }

  void EXPECT_S(int n) {
    int S = android::base::GetIntProperty("build.version.extensions.s", -1);
    // Only expect the S extension level to be set on S+ devices.
    EXPECT_EQ(S, android::modules::sdklevel::IsAtLeastS() ? n : -1);
  }

  ExtensionDatabase db_;
  TemporaryDir dir_;
};

TEST_F(DeriveSdkTest, CurrentSystemImageValue) {
  EXPECT_R(1);
  EXPECT_S(1);
}

TEST_F(DeriveSdkTest, OneDessert_OneVersion_OneApex) {
  AddExtensionVersion(3, {{SdkModule::SDK_EXTENSIONS, 2}});
  EXPECT_S(3);

  SetApexVersion("com.android.sdkext", 3);

  EXPECT_R(3);
  EXPECT_S(3);
}

TEST_F(DeriveSdkTest, OneDessert_OneVersion_TwoApexes) {
  AddExtensionVersion(5, {
                             {SdkModule::MEDIA, 5},
                             {SdkModule::SDK_EXTENSIONS, 2},
                         });
  EXPECT_R(0);
  EXPECT_S(5);

  SetApexVersion("com.android.sdkext", 2);
  EXPECT_R(0);
  SetApexVersion("com.android.media", 5);
  EXPECT_R(5);
}

TEST_F(DeriveSdkTest, OneDessert_ManyVersions) {
  AddExtensionVersion(1, {
                             {SdkModule::MEDIA, 1},
                         });
  AddExtensionVersion(2, {
                             {SdkModule::MEDIA, 1},
                             {SdkModule::MEDIA_PROVIDER, 2},
                             {SdkModule::SDK_EXTENSIONS, 2},
                         });
  AddExtensionVersion(3, {
                             {SdkModule::MEDIA, 3},
                             {SdkModule::MEDIA_PROVIDER, 2},
                             {SdkModule::SDK_EXTENSIONS, 3},
                         });
  EXPECT_R(0);
  EXPECT_S(3);

  SetApexVersion("com.android.media", 1);
  EXPECT_R(1);

  SetApexVersion("com.android.mediaprovider", 2);
  EXPECT_R(1);
  SetApexVersion("com.android.sdkext", 2);
  EXPECT_R(2);

  SetApexVersion("com.android.media", 3);
  EXPECT_R(2);
  SetApexVersion("com.android.sdkext", 3);
  EXPECT_R(3);
}

TEST_F(DeriveSdkTest, TwoDesserts_ManyVersions) {
  AddExtensionVersion(1, {
                             {SdkModule::TETHERING, 1},
                         });
  AddExtensionVersion(2, {
                             {SdkModule::ART, 2},
                             {SdkModule::TETHERING, 1},
                         });
  AddExtensionVersion(3, {
                             {SdkModule::ART, 3},
                             {SdkModule::MEDIA, 3},
                             {SdkModule::TETHERING, 1},
                         });
  EXPECT_R(0);
  EXPECT_S(1);

  SetApexVersion("com.android.tethering", 1);
  EXPECT_R(2);
  EXPECT_S(1);

  SetApexVersion("com.android.art", 2);
  EXPECT_R(2);
  EXPECT_S(2);

  SetApexVersion("com.android.media", 3);
  EXPECT_R(3);
  EXPECT_S(2);
  SetApexVersion("com.android.art", 3);
  EXPECT_R(3);
  EXPECT_S(3);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
