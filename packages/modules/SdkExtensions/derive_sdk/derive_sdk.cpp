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

#define LOG_TAG "derive_sdk"

#include "derive_sdk.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-modules-utils/sdk_level.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "packages/modules/common/proto/sdk.pb.h"

namespace android {
namespace derivesdk {

static const std::unordered_map<std::string, SdkModule> kApexNameToModule = {
    {"com.android.art", SdkModule::ART},
    {"com.android.conscrypt", SdkModule::CONSCRYPT},
    {"com.android.ipsec", SdkModule::IPSEC},
    {"com.android.media", SdkModule::MEDIA},
    {"com.android.mediaprovider", SdkModule::MEDIA_PROVIDER},
    {"com.android.permission", SdkModule::PERMISSIONS},
    {"com.android.scheduling", SdkModule::SCHEDULING},
    {"com.android.sdkext", SdkModule::SDK_EXTENSIONS},
    {"com.android.os.statsd", SdkModule::STATSD},
    {"com.android.tethering", SdkModule::TETHERING},
};

static const std::unordered_set<SdkModule> kRModules = {
    SdkModule::CONSCRYPT,   SdkModule::IPSEC,          SdkModule::MEDIA,  SdkModule::MEDIA_PROVIDER,
    SdkModule::PERMISSIONS, SdkModule::SDK_EXTENSIONS, SdkModule::STATSD, SdkModule::TETHERING,
};

static const std::unordered_set<SdkModule> kSModules = {SdkModule::ART, SdkModule::SCHEDULING};

bool ReadDatabase(const std::string& db_path, ExtensionDatabase& db) {
  std::string contents;
  if (!android::base::ReadFileToString(db_path, &contents, true)) {
    PLOG(ERROR) << "failed to read " << db_path << ": ";
    return false;
  }
  if (!db.ParseFromString(contents)) {
    LOG(ERROR) << "failed to parse " << db_path;
    return false;
  }
  return true;
}

bool VersionRequirementsMet(
    const ExtensionVersion& ext_version,
    const std::unordered_set<SdkModule>& relevant_modules,
    const std::unordered_map<SdkModule, int>& module_versions) {
  for (const auto& requirement : ext_version.requirements()) {
    // Only requirements on modules relevant for this extension matter.
    if (relevant_modules.find(requirement.module()) == relevant_modules.end())
      continue;

    auto version = module_versions.find(requirement.module());
    if (version == module_versions.end()) {
      LOG(DEBUG) << "Not version " << ext_version.version() << ": Module "
                 << requirement.module() << " is missing";
      return false;
    }
    if (version->second < requirement.version().version()) {
      LOG(DEBUG) << "Not version " << ext_version.version() << ": Module "
                 << requirement.module() << " version (" << version->second
                 << ") too low. Needed " << requirement.version().version();
      return false;
    }
  }
  return true;
}

int GetSdkLevel(const ExtensionDatabase& db,
                const std::unordered_set<SdkModule>& relevant_modules,
                const std::unordered_map<SdkModule, int>& module_versions) {
  int max = 0;

  for (const auto& ext_version : db.versions()) {
    if (ext_version.version() > max &&
        VersionRequirementsMet(ext_version, relevant_modules,
                               module_versions)) {
      max = ext_version.version();
    }
  }
  return max;
}

bool SetSdkLevels(const std::string& mountpath) {
  ExtensionDatabase db;
  if (!ReadDatabase(mountpath + "/com.android.sdkext/etc/extensions_db.pb", db)) {
    LOG(ERROR) << "Failed to read database";
    return false;
  }
  std::unique_ptr<DIR, decltype(&closedir)> apex(opendir(mountpath.c_str()),
                                                 closedir);
  if (!apex) {
    LOG(ERROR) << "Could not read " + mountpath;
    return false;
  }
  struct dirent* de;
  std::unordered_map<SdkModule, int> versions;
  while ((de = readdir(apex.get()))) {
    std::string name = de->d_name;
    if (name[0] == '.' || name.find('@') != std::string::npos) {
      // Skip <name>@<ver> dirs, as they are bind-mounted to <name>
      continue;
    }
    std::string path = mountpath + "/" + name + "/etc/sdkinfo.pb";
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
      continue;
    }
    auto module_itr = kApexNameToModule.find(name);
    if (module_itr == kApexNameToModule.end()) {
      LOG(WARNING) << "Found sdkinfo in unexpected apex " << name;
      continue;
    }
    std::string contents;
    if (!android::base::ReadFileToString(path, &contents, true)) {
      LOG(ERROR) << "failed to read " << path;
      continue;
    }
    SdkVersion sdk_version;
    if (!sdk_version.ParseFromString(contents)) {
      LOG(ERROR) << "failed to parse " << path;
      continue;
    }
    SdkModule module = module_itr->second;
    LOG(INFO) << "Read version " << sdk_version.version() << " from " << module;
    versions[module] = sdk_version.version();
  }

  int version_R = GetSdkLevel(db, kRModules, versions);
  LOG(INFO) << "R extension version is " << version_R;

  if (!android::base::SetProperty("build.version.extensions.r",
                                  std::to_string(version_R))) {
    LOG(ERROR) << "failed to set r sdk_info prop";
    return false;
  }
  if (android::modules::sdklevel::IsAtLeastS()) {
    int version_S = GetSdkLevel(db, kSModules, versions);
    LOG(INFO) << "S extension version is " << version_S;
    if (!android::base::SetProperty("build.version.extensions.s",
                                    std::to_string(version_S))) {
      LOG(ERROR) << "failed to set s sdk_info prop";
      return false;
    }
  }

  return true;
}

}  // namespace derivesdk
}  // namespace android
