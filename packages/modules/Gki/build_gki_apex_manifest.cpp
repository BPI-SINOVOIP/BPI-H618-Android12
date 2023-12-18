//
// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <sysexits.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <gflags/gflags.h>
#include <json/json.h>
#include <kver/kernel_release.h>
#include <kver/kmi_version.h>
#include <kver/utils.h>

using android::kver::GetFactoryApexVersion;
using android::kver::KernelRelease;
using android::kver::KmiVersion;

static constexpr uint64_t UNSTABLE_GENERATION = UINT64_MAX;

namespace {

int CheckKmi(const KernelRelease& kernel_release, const KmiVersion& kmi_version) {
  const auto& actual_kmi_version = kernel_release.kmi_version();
  if (actual_kmi_version == kmi_version) {
    return EX_OK;
  }
  if (kmi_version.generation() == UNSTABLE_GENERATION &&
      kmi_version.version() == actual_kmi_version.version() &&
      kmi_version.patch_level() == actual_kmi_version.patch_level() &&
      kmi_version.android_release() == actual_kmi_version.android_release()) {
    LOG(WARNING) << "Actual KMI version " << actual_kmi_version.string()
                 << " matches unstable KMI version";
    return EX_OK;
  }
  LOG(ERROR) << "KMI version does not match. Actual: " << actual_kmi_version.string()
             << ", expected: " << kmi_version.string();
  return EX_SOFTWARE;
}

int WriteApexManifest(const std::string& apex_name, Json::UInt64 apex_version,
                      const std::string& out_file) {
  Json::Value root;
  root["name"] = apex_name;
  root["version"] = apex_version;
  root["preInstallHook"] = "bin/com.android.gki.preinstall";
  Json::StreamWriterBuilder factory;
  std::string json_string = Json::writeString(factory, root);
  if (!android::base::WriteStringToFile(json_string, out_file)) {
    PLOG(ERROR) << "Cannot write to " << out_file;
    return EX_SOFTWARE;
  }
  return EX_OK;
}

}  // namespace

DEFINE_string(kernel_release_file, "",
              "Input file that contains a kernel release string parsed from the boot image. "
              "Exactly one of --kernel_release_file or --factory must be set.");
DEFINE_bool(factory, false,
            "Set to true for factory APEX package. Exactly one of --kernel_release_file or "
            "--factory must be set.");
DEFINE_string(kmi_version, "", "Declared KMI version for this APEX.");
DEFINE_string(apex_manifest, "", "Output APEX manifest JSON file.");
DEFINE_uint64(apex_version, GetFactoryApexVersion(),
              "Override APEX version in APEX manifest. Use factory APEX version if unspecified.");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_kmi_version.empty()) {
    LOG(ERROR) << "--kmi_version must be set.";
    return EX_SOFTWARE;
  }
  std::string_view kmi_version_sv(FLAGS_kmi_version);
  std::string kmi_version_string;
  if (android::base::ConsumeSuffix(&kmi_version_sv, "unstable")) {
    kmi_version_string = std::string(kmi_version_sv) + std::to_string(UNSTABLE_GENERATION);
  } else {
    kmi_version_string = kmi_version_sv;
  }
  auto kmi_version = KmiVersion::Parse(kmi_version_string);
  if (!kmi_version.has_value()) {
    LOG(ERROR) << "--kmi_version is not a valid KMI version.";
    return EX_SOFTWARE;
  }

  if (FLAGS_factory + (!FLAGS_kernel_release_file.empty()) != 1) {
    LOG(ERROR) << "Exactly one of --kernel_release_file or --factory must be set.";
    return EX_SOFTWARE;
  }

  if (!FLAGS_kernel_release_file.empty()) {
    std::string kernel_release_string;
    if (!android::base::ReadFileToString(FLAGS_kernel_release_file, &kernel_release_string)) {
      PLOG(ERROR) << "Cannot read " << FLAGS_kernel_release_file;
      return EX_SOFTWARE;
    }
    auto kernel_release = KernelRelease::Parse(kernel_release_string, true /* allow_suffix */);
    if (!kernel_release.has_value()) {
      LOG(ERROR) << kernel_release_string << " is not a valid GKI kernel release string";
      return EX_SOFTWARE;
    }
    int res = CheckKmi(*kernel_release, *kmi_version);
    if (res != EX_OK) return res;
  }

  std::string apex_name = GetApexName(*kmi_version);
  uint64_t apex_version = FLAGS_apex_version;

  if (FLAGS_apex_manifest.empty()) {
    LOG(WARNING) << "Skip writing APEX manifest because --apex_manifest is not set.";
  } else {
    int res = WriteApexManifest(apex_name, apex_version, FLAGS_apex_manifest);
    if (res != EX_OK) return res;
  }

  return EX_OK;
}
