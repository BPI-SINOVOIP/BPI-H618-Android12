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

#define LOG_TAG "apexd"

#include "apex_file_repository.h"

#include <unordered_map>

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/result.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "apex_constants.h"
#include "apex_file.h"
#include "apexd_utils.h"

using android::base::EndsWith;
using android::base::Error;
using android::base::GetProperty;
using android::base::Result;

namespace android {
namespace apex {

Result<void> ApexFileRepository::ScanBuiltInDir(const std::string& dir) {
  LOG(INFO) << "Scanning " << dir << " for pre-installed ApexFiles";
  if (access(dir.c_str(), F_OK) != 0 && errno == ENOENT) {
    LOG(WARNING) << dir << " does not exist. Skipping";
    return {};
  }

  Result<std::vector<std::string>> all_apex_files = FindFilesBySuffix(
      dir, {kApexPackageSuffix, kCompressedApexPackageSuffix});
  if (!all_apex_files.ok()) {
    return all_apex_files.error();
  }

  // TODO(b/179248390): scan parallelly if possible
  for (const auto& file : *all_apex_files) {
    LOG(INFO) << "Found pre-installed APEX " << file;
    Result<ApexFile> apex_file = ApexFile::Open(file);
    if (!apex_file.ok()) {
      return Error() << "Failed to open " << file << " : " << apex_file.error();
    }

    const std::string& name = apex_file->GetManifest().name();
    auto it = pre_installed_store_.find(name);
    if (it == pre_installed_store_.end()) {
      pre_installed_store_.emplace(name, std::move(*apex_file));
    } else if (it->second.GetPath() != apex_file->GetPath()) {
      auto level = base::FATAL;
      // On some development (non-REL) builds the VNDK apex could be in /vendor.
      // When testing CTS-on-GSI on these builds, there would be two VNDK apexes
      // in the system, one in /system and one in /vendor.
      static constexpr char kVndkApexModuleNamePrefix[] = "com.android.vndk.";
      static constexpr char kPlatformVersionCodenameProperty[] =
          "ro.build.version.codename";
      if (android::base::StartsWith(name, kVndkApexModuleNamePrefix) &&
          GetProperty(kPlatformVersionCodenameProperty, "REL") != "REL") {
        level = android::base::INFO;
      }
      LOG(level) << "Found two apex packages " << it->second.GetPath()
                 << " and " << apex_file->GetPath()
                 << " with the same module name " << name;
    } else if (it->second.GetBundledPublicKey() !=
               apex_file->GetBundledPublicKey()) {
      LOG(FATAL) << "Public key of apex package " << it->second.GetPath()
                 << " (" << name << ") has unexpectedly changed";
    }
  }
  return {};
}

ApexFileRepository& ApexFileRepository::GetInstance() {
  static ApexFileRepository instance;
  return instance;
}

android::base::Result<void> ApexFileRepository::AddPreInstalledApex(
    const std::vector<std::string>& prebuilt_dirs) {
  for (const auto& dir : prebuilt_dirs) {
    if (auto result = ScanBuiltInDir(dir); !result.ok()) {
      return result.error();
    }
  }
  return {};
}

// TODO(b/179497746): AddDataApex should not concern with filtering out invalid
//   apex.
Result<void> ApexFileRepository::AddDataApex(const std::string& data_dir) {
  LOG(INFO) << "Scanning " << data_dir << " for data ApexFiles";
  if (access(data_dir.c_str(), F_OK) != 0 && errno == ENOENT) {
    LOG(WARNING) << data_dir << " does not exist. Skipping";
    return {};
  }

  Result<std::vector<std::string>> active_apex =
      FindFilesBySuffix(data_dir, {kApexPackageSuffix});
  if (!active_apex.ok()) {
    return active_apex.error();
  }

  // TODO(b/179248390): scan parallelly if possible
  for (const auto& file : *active_apex) {
    LOG(INFO) << "Found updated apex " << file;
    Result<ApexFile> apex_file = ApexFile::Open(file);
    if (!apex_file.ok()) {
      LOG(ERROR) << "Failed to open " << file << " : " << apex_file.error();
      continue;
    }

    const std::string& name = apex_file->GetManifest().name();
    if (!HasPreInstalledVersion(name)) {
      LOG(ERROR) << "Skipping " << file << " : no preisntalled apex";
      // Ignore data apex without corresponding pre-installed apex
      continue;
    }
    auto pre_installed_public_key = GetPublicKey(name);
    if (!pre_installed_public_key.ok() ||
        apex_file->GetBundledPublicKey() != *pre_installed_public_key) {
      // Ignore data apex if public key doesn't match with pre-installed apex
      LOG(ERROR) << "Skipping " << file
                 << " : public key doesn't match pre-installed one";
      continue;
    }

    if (EndsWith(apex_file->GetPath(), kDecompressedApexPackageSuffix)) {
      LOG(WARNING) << "Skipping " << file
                   << " : Non-decompressed APEX should not have "
                   << kDecompressedApexPackageSuffix << " suffix";
      continue;
    }

    auto it = data_store_.find(name);
    if (it == data_store_.end()) {
      data_store_.emplace(name, std::move(*apex_file));
      continue;
    }

    const auto& existing_version = it->second.GetManifest().version();
    const auto new_version = apex_file->GetManifest().version();
    // If multiple data apexs are preset, select the one with highest version
    bool prioritize_higher_version = new_version > existing_version;
    // For same version, non-decompressed apex gets priority
    if (prioritize_higher_version) {
      it->second = std::move(*apex_file);
    }
  }
  return {};
}

// TODO(b/179497746): remove this method when we add api for fetching ApexFile
//  by name
Result<const std::string> ApexFileRepository::GetPublicKey(
    const std::string& name) const {
  auto it = pre_installed_store_.find(name);
  if (it == pre_installed_store_.end()) {
    return Error() << "No preinstalled apex found for package " << name;
  }
  return it->second.GetBundledPublicKey();
}

// TODO(b/179497746): remove this method when we add api for fetching ApexFile
//  by name
Result<const std::string> ApexFileRepository::GetPreinstalledPath(
    const std::string& name) const {
  auto it = pre_installed_store_.find(name);
  if (it == pre_installed_store_.end()) {
    return Error() << "No preinstalled data found for package " << name;
  }
  return it->second.GetPath();
}

// TODO(b/179497746): remove this method when we add api for fetching ApexFile
//  by name
Result<const std::string> ApexFileRepository::GetDataPath(
    const std::string& name) const {
  auto it = data_store_.find(name);
  if (it == data_store_.end()) {
    return Error() << "No data apex found for package " << name;
  }
  return it->second.GetPath();
}

bool ApexFileRepository::HasPreInstalledVersion(const std::string& name) const {
  return pre_installed_store_.find(name) != pre_installed_store_.end();
}

bool ApexFileRepository::HasDataVersion(const std::string& name) const {
  return data_store_.find(name) != data_store_.end();
}

// ApexFile is considered a decompressed APEX if it is located in decompression
// dir
bool ApexFileRepository::IsDecompressedApex(const ApexFile& apex) const {
  return apex.GetPath().starts_with(decompression_dir_);
}

bool ApexFileRepository::IsPreInstalledApex(const ApexFile& apex) const {
  auto it = pre_installed_store_.find(apex.GetManifest().name());
  if (it == pre_installed_store_.end()) {
    return false;
  }
  return it->second.GetPath() == apex.GetPath() || IsDecompressedApex(apex);
}

std::vector<ApexFileRef> ApexFileRepository::GetPreInstalledApexFiles() const {
  std::vector<ApexFileRef> result;
  for (const auto& it : pre_installed_store_) {
    result.emplace_back(std::cref(it.second));
  }
  return std::move(result);
}

std::vector<ApexFileRef> ApexFileRepository::GetDataApexFiles() const {
  std::vector<ApexFileRef> result;
  for (const auto& it : data_store_) {
    result.emplace_back(std::cref(it.second));
  }
  return std::move(result);
}

// Group pre-installed APEX and data APEX by name
std::unordered_map<std::string, std::vector<ApexFileRef>>
ApexFileRepository::AllApexFilesByName() const {
  // Collect all apex files
  std::vector<ApexFileRef> all_apex_files;
  auto pre_installed_apexs = GetPreInstalledApexFiles();
  auto data_apexs = GetDataApexFiles();
  std::move(pre_installed_apexs.begin(), pre_installed_apexs.end(),
            std::back_inserter(all_apex_files));
  std::move(data_apexs.begin(), data_apexs.end(),
            std::back_inserter(all_apex_files));

  // Group them by name
  std::unordered_map<std::string, std::vector<ApexFileRef>> result;
  for (const auto& apex_file_ref : all_apex_files) {
    const ApexFile& apex_file = apex_file_ref.get();
    const std::string& package_name = apex_file.GetManifest().name();
    if (result.find(package_name) == result.end()) {
      result[package_name] = std::vector<ApexFileRef>{};
    }
    result[package_name].emplace_back(apex_file_ref);
  }

  return std::move(result);
}

ApexFileRef ApexFileRepository::GetDataApex(const std::string& name) const {
  auto it = data_store_.find(name);
  CHECK(it != data_store_.end());
  return std::cref(it->second);
}

ApexFileRef ApexFileRepository::GetPreInstalledApex(
    const std::string& name) const {
  auto it = pre_installed_store_.find(name);
  CHECK(it != pre_installed_store_.end());
  return std::cref(it->second);
}

}  // namespace apex
}  // namespace android
