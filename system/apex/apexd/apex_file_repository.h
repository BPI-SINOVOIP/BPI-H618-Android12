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

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "apex_constants.h"
#include "apex_file.h"

#include <android-base/result.h>

namespace android {
namespace apex {

using ApexFileRef = std::reference_wrapper<const android::apex::ApexFile>;

// This class serves as a ApexFile repository for all apexes on device. It also
// provides information about the ApexFiles it hosts, such as which are
// pre-installed and which are data. Such information can be used, for example,
// to verify validity of an apex before trying to mount it.
//
// It's expected to have a single instance of this class in a process that
// mounts apexes (e.g. apexd, otapreopt_chroot).
class ApexFileRepository final {
 public:
  // c-tor and d-tor are exposed for testing.
  explicit ApexFileRepository(
      const std::string& decompression_dir = kApexDecompressedDir)
      : decompression_dir_(decompression_dir){};

  ~ApexFileRepository() {
    pre_installed_store_.clear();
    data_store_.clear();
  };

  // Returns a singletone instance of this class.
  static ApexFileRepository& GetInstance();

  // Populate instance by collecting pre-installed apex files from the given
  // |prebuilt_dirs|.
  // Note: this call is **not thread safe** and is expected to be performed in a
  // single thread during initialization of apexd. After initialization is
  // finished, all queries to the instance are thread safe.
  android::base::Result<void> AddPreInstalledApex(
      const std::vector<std::string>& prebuilt_dirs);

  // Populate instance by collecting data apex files from the given |data_dir|.
  // Note: this call is **not thread safe** and is expected to be performed in a
  // single thread during initialization of apexd. After initialization is
  // finished, all queries to the instance are thread safe.
  android::base::Result<void> AddDataApex(const std::string& data_dir);

  // Returns trusted public key for an apex with the given |name|.
  android::base::Result<const std::string> GetPublicKey(
      const std::string& name) const;

  // Returns path to the pre-installed version of an apex with the given |name|.
  android::base::Result<const std::string> GetPreinstalledPath(
      const std::string& name) const;

  // Returns path to the data version of an apex with the given |name|.
  android::base::Result<const std::string> GetDataPath(
      const std::string& name) const;

  // Checks whether there is a pre-installed version of an apex with the given
  // |name|.
  bool HasPreInstalledVersion(const std::string& name) const;

  // Checks whether there is a data version of an apex with the given |name|.
  bool HasDataVersion(const std::string& name) const;

  // Checks if given |apex| is pre-installed.
  bool IsPreInstalledApex(const ApexFile& apex) const;

  // Checks if given |apex| is decompressed from a pre-installed APEX
  bool IsDecompressedApex(const ApexFile& apex) const;

  // Returns reference to all pre-installed APEX on device
  std::vector<ApexFileRef> GetPreInstalledApexFiles() const;

  // Returns reference to all data APEX on device
  std::vector<ApexFileRef> GetDataApexFiles() const;

  // Group all ApexFiles on device by their package name
  std::unordered_map<std::string, std::vector<ApexFileRef>> AllApexFilesByName()
      const;

  // Returns a pre-installed version of apex with the given name. Caller is
  // expected to check if there is a pre-installed apex with the given name
  // using |HasPreinstalledVersion| function.
  ApexFileRef GetPreInstalledApex(const std::string& name) const;
  // Returns a data version of apex with the given name. Caller is
  // expected to check if there is a data apex with the given name
  // using |HasDataVersion| function.
  ApexFileRef GetDataApex(const std::string& name) const;

  // Clears ApexFileRepostiry.
  // Only use in tests.
  void Reset(const std::string& decompression_dir = kApexDecompressedDir) {
    pre_installed_store_.clear();
    data_store_.clear();
    decompression_dir_ = decompression_dir;
  }

 private:
  // Non-copyable && non-moveable.
  ApexFileRepository(const ApexFileRepository&) = delete;
  ApexFileRepository& operator=(const ApexFileRepository&) = delete;
  ApexFileRepository& operator=(ApexFileRepository&&) = delete;
  ApexFileRepository(ApexFileRepository&&) = delete;

  // Scans apexes in the given directory and adds collected data into
  // |pre_installed_store_|.
  android::base::Result<void> ScanBuiltInDir(const std::string& dir);

  std::unordered_map<std::string, ApexFile> pre_installed_store_, data_store_;
  // Decompression directory which will be used to determine if apex is
  // decompressed or not
  std::string decompression_dir_;
};

}  // namespace apex
}  // namespace android
