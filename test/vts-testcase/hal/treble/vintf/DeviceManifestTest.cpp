/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "DeviceManifestTest.h"

#include <android-base/properties.h>
#include <android-base/result.h>
#include <libvts_vintf_test_common/common.h>
#include <vintf/VintfObject.h>

#include "SingleManifestTest.h"

namespace android {
namespace vintf {
namespace testing {

void DeviceManifestTest::SetUp() {
  VtsTrebleVintfTestBase::SetUp();

  vendor_manifest_ = VintfObject::GetDeviceHalManifest();
  ASSERT_NE(vendor_manifest_, nullptr)
      << "Failed to get vendor HAL manifest." << endl;
}

// Tests that Shipping FCM Version in the device manifest is at least the
// minimum Shipping FCM Version as required by Board API level.
TEST_F(DeviceManifestTest, ShippingFcmVersion) {
  uint64_t board_api_level = GetBoardApiLevel();
  Level shipping_fcm_version = VintfObject::GetDeviceHalManifest()->level();
  auto res = TestTargetFcmVersion(shipping_fcm_version, board_api_level);
  ASSERT_RESULT_OK(res);
}

TEST_F(DeviceManifestTest, KernelFcmVersion) {
  Level shipping_fcm_version = VintfObject::GetDeviceHalManifest()->level();

  if (shipping_fcm_version == Level::UNSPECIFIED ||
      shipping_fcm_version < Level::R) {
    GTEST_SKIP() << "Kernel FCM version not enforced on target FCM version "
                 << shipping_fcm_version;
  }
  std::string error;
  Level kernel_fcm_version = VintfObject::GetInstance()->getKernelLevel(&error);
  ASSERT_NE(Level::UNSPECIFIED, kernel_fcm_version)
      << "Kernel FCM version must be specified for target FCM version '"
      << shipping_fcm_version << "': " << error;
  ASSERT_GE(kernel_fcm_version, shipping_fcm_version)
      << "Kernel FCM version " << kernel_fcm_version
      << " must be greater or equal to target FCM version "
      << shipping_fcm_version;
}

// Tests that deprecated HALs are not in the manifest, unless a higher,
// non-deprecated minor version is in the manifest.
TEST_F(DeviceManifestTest, NoDeprecatedHalsOnManifest) {
  string error;
  EXPECT_EQ(android::vintf::NO_DEPRECATED_HALS,
            VintfObject::GetInstance()->checkDeprecation(
                HidlInterfaceMetadata::all(), &error))
      << error;
}

// Tests that devices launching R support mapper@4.0.  Go devices are exempt
// from this requirement, so we use this test to enforce instead of the
// compatibility matrix.
TEST_F(DeviceManifestTest, GrallocHalVersionCompatibility) {
  Level shipping_fcm_version = VintfObject::GetDeviceHalManifest()->level();
  bool is_go_device =
      android::base::GetBoolProperty("ro.config.low_ram", false);
  if (shipping_fcm_version == Level::UNSPECIFIED ||
      shipping_fcm_version < Level::R || is_go_device) {
    GTEST_SKIP() << "Gralloc4 is only required on launching R devices";
  }

  ASSERT_TRUE(vendor_manifest_->hasHidlInstance(
      "android.hardware.graphics.mapper", {4, 0}, "IMapper", "default"));
  ASSERT_FALSE(vendor_manifest_->hasHidlInstance(
      "android.hardware.graphics.mapper", {2, 0}, "IMapper", "default"));
  ASSERT_FALSE(vendor_manifest_->hasHidlInstance(
      "android.hardware.graphics.mapper", {2, 1}, "IMapper", "default"));
}

static std::vector<HalManifestPtr> GetTestManifests() {
  return {
      VintfObject::GetDeviceHalManifest(),
  };
}

INSTANTIATE_TEST_CASE_P(DeviceManifest, SingleManifestTest,
                        ::testing::ValuesIn(GetTestManifests()));

}  // namespace testing
}  // namespace vintf
}  // namespace android
