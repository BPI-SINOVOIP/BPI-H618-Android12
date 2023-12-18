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

#include <filesystem>
#include <fstream>

#include <asm-generic/errno-base.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sched.h>
#include <sys/mount.h>

#include <android-base/errors.h>
#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/result.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <android/apex/ApexInfo.h>
#include <android/apex/ApexSessionInfo.h>
#include <binder/IServiceManager.h>
#include <selinux/android.h>

#include "apex_file.h"
#include "session_state.pb.h"

#include "com_android_apex.h"

using android::base::Error;
using android::base::Result;
using apex::proto::SessionState;

namespace android {
namespace apex {
namespace testing {

using ::testing::AllOf;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::Field;
using ::testing::Property;

template <typename T>
inline ::testing::AssertionResult IsOk(const android::base::Result<T>& result) {
  if (result.ok()) {
    return ::testing::AssertionSuccess() << " is Ok";
  } else {
    return ::testing::AssertionFailure() << " failed with " << result.error();
  }
}

inline ::testing::AssertionResult IsOk(const android::binder::Status& status) {
  if (status.isOk()) {
    return ::testing::AssertionSuccess() << " is Ok";
  } else {
    return ::testing::AssertionFailure()
           << " failed with " << status.exceptionMessage().c_str();
  }
}

MATCHER_P(SessionInfoEq, other, "") {
  return ExplainMatchResult(
      AllOf(
          Field("sessionId", &ApexSessionInfo::sessionId, Eq(other.sessionId)),
          Field("isUnknown", &ApexSessionInfo::isUnknown, Eq(other.isUnknown)),
          Field("isVerified", &ApexSessionInfo::isVerified,
                Eq(other.isVerified)),
          Field("isStaged", &ApexSessionInfo::isStaged, Eq(other.isStaged)),
          Field("isActivated", &ApexSessionInfo::isActivated,
                Eq(other.isActivated)),
          Field("isRevertInProgress", &ApexSessionInfo::isRevertInProgress,
                Eq(other.isRevertInProgress)),
          Field("isActivationFailed", &ApexSessionInfo::isActivationFailed,
                Eq(other.isActivationFailed)),
          Field("isSuccess", &ApexSessionInfo::isSuccess, Eq(other.isSuccess)),
          Field("isReverted", &ApexSessionInfo::isReverted,
                Eq(other.isReverted)),
          Field("isRevertFailed", &ApexSessionInfo::isRevertFailed,
                Eq(other.isRevertFailed))),
      arg, result_listener);
}

MATCHER_P(ApexInfoEq, other, "") {
  return ExplainMatchResult(
      AllOf(Field("moduleName", &ApexInfo::moduleName, Eq(other.moduleName)),
            Field("modulePath", &ApexInfo::modulePath, Eq(other.modulePath)),
            Field("preinstalledModulePath", &ApexInfo::preinstalledModulePath,
                  Eq(other.preinstalledModulePath)),
            Field("versionCode", &ApexInfo::versionCode, Eq(other.versionCode)),
            Field("isFactory", &ApexInfo::isFactory, Eq(other.isFactory)),
            Field("isActive", &ApexInfo::isActive, Eq(other.isActive))),
      arg, result_listener);
}

MATCHER_P(ApexFileEq, other, "") {
  return ExplainMatchResult(
      AllOf(Property("path", &ApexFile::GetPath, Eq(other.get().GetPath())),
            Property("image_offset", &ApexFile::GetImageOffset,
                     Eq(other.get().GetImageOffset())),
            Property("image_size", &ApexFile::GetImageSize,
                     Eq(other.get().GetImageSize())),
            Property("fs_type", &ApexFile::GetFsType,
                     Eq(other.get().GetFsType())),
            Property("public_key", &ApexFile::GetBundledPublicKey,
                     Eq(other.get().GetBundledPublicKey())),
            Property("is_compressed", &ApexFile::IsCompressed,
                     Eq(other.get().IsCompressed()))),
      arg, result_listener);
}

inline ApexSessionInfo CreateSessionInfo(int session_id) {
  ApexSessionInfo info;
  info.sessionId = session_id;
  info.isUnknown = false;
  info.isVerified = false;
  info.isStaged = false;
  info.isActivated = false;
  info.isRevertInProgress = false;
  info.isActivationFailed = false;
  info.isSuccess = false;
  info.isReverted = false;
  info.isRevertFailed = false;
  return info;
}

}  // namespace testing

// Must be in apex::android namespace, otherwise gtest won't be able to find it.
inline void PrintTo(const ApexSessionInfo& session, std::ostream* os) {
  *os << "apex_session: {\n";
  *os << "  sessionId : " << session.sessionId << "\n";
  *os << "  isUnknown : " << session.isUnknown << "\n";
  *os << "  isVerified : " << session.isVerified << "\n";
  *os << "  isStaged : " << session.isStaged << "\n";
  *os << "  isActivated : " << session.isActivated << "\n";
  *os << "  isActivationFailed : " << session.isActivationFailed << "\n";
  *os << "  isSuccess : " << session.isSuccess << "\n";
  *os << "  isReverted : " << session.isReverted << "\n";
  *os << "  isRevertFailed : " << session.isRevertFailed << "\n";
  *os << "}";
}

inline void PrintTo(const ApexInfo& apex, std::ostream* os) {
  *os << "apex_info: {\n";
  *os << "  moduleName : " << apex.moduleName << "\n";
  *os << "  modulePath : " << apex.modulePath << "\n";
  *os << "  preinstalledModulePath : " << apex.preinstalledModulePath << "\n";
  *os << "  versionCode : " << apex.versionCode << "\n";
  *os << "  isFactory : " << apex.isFactory << "\n";
  *os << "  isActive : " << apex.isActive << "\n";
  *os << "}";
}

inline Result<bool> CompareFiles(const std::string& filename1,
                                 const std::string& filename2) {
  std::ifstream file1(filename1, std::ios::binary);
  std::ifstream file2(filename2, std::ios::binary);

  if (file1.bad() || file2.bad()) {
    return Error() << "Could not open one of the file";
  }

  std::istreambuf_iterator<char> begin1(file1);
  std::istreambuf_iterator<char> begin2(file2);

  return std::equal(begin1, std::istreambuf_iterator<char>(), begin2);
}

inline android::base::Result<std::string> GetCurrentMountNamespace() {
  std::string result;
  if (!android::base::Readlink("/proc/self/ns/mnt", &result)) {
    return android::base::ErrnoError() << "Failed to read /proc/self/ns/mnt";
  }
  return result;
}

// A helper class to switch back to the original mount namespace of a process
// upon exiting current scope.
class MountNamespaceRestorer final {
 public:
  explicit MountNamespaceRestorer() {
    original_namespace_.reset(open("/proc/self/ns/mnt", O_RDONLY | O_CLOEXEC));
    if (original_namespace_.get() < 0) {
      PLOG(ERROR) << "Failed to open /proc/self/ns/mnt";
    }
  }

  ~MountNamespaceRestorer() {
    if (original_namespace_.get() != -1) {
      if (setns(original_namespace_.get(), CLONE_NEWNS) == -1) {
        PLOG(ERROR) << "Failed to switch back to " << original_namespace_.get();
      }
    }
  }

 private:
  android::base::unique_fd original_namespace_;
  DISALLOW_COPY_AND_ASSIGN(MountNamespaceRestorer);
};

inline std::vector<std::string> GetApexMounts() {
  std::vector<std::string> apex_mounts;
  std::string mount_info;
  if (!android::base::ReadFileToString("/proc/self/mountinfo", &mount_info)) {
    return apex_mounts;
  }
  for (const auto& line : android::base::Split(mount_info, "\n")) {
    std::vector<std::string> tokens = android::base::Split(line, " ");
    // line format:
    // mnt_id parent_mnt_id major:minor source target option propagation_type
    // ex) 33 260:19 / /apex rw,nosuid,nodev -
    if (tokens.size() >= 7 && android::base::StartsWith(tokens[4], "/apex/")) {
      apex_mounts.push_back(tokens[4]);
    }
  }
  return apex_mounts;
}

// Sets up a test environment for unit testing logic around mounting/unmounting
// apexes. For examples of usage see apexd_test.cpp
inline android::base::Result<void> SetUpApexTestEnvironment() {
  using android::base::ErrnoError;

  // 1. Switch to new mount namespace.
  if (unshare(CLONE_NEWNS) != 0) {
    return ErrnoError() << "Failed to unshare";
  }

  // 2. Make everything private, so that changes don't propagate.
  if (mount(nullptr, "/", nullptr, MS_PRIVATE | MS_REC, nullptr) == -1) {
    return ErrnoError() << "Failed to mount / as private";
  }

  // 3. Unmount all apexes. This needs to happen in two phases:
  // Note: unlike regular unmount flow in apexd, we don't destroy dm and loop
  // devices, since that would've propagated outside of the test environment.
  std::vector<std::string> apex_mounts = GetApexMounts();

  // 3a. First unmount all bind mounds (without @version_code).
  for (const auto& mount : apex_mounts) {
    if (mount.find('@') == std::string::npos) {
      if (umount2(mount.c_str(), 0) != 0) {
        return ErrnoError() << "Failed to unmount " << mount;
      }
    }
  }

  // 3.b Now unmount versioned mounts.
  for (const auto& mount : apex_mounts) {
    if (mount.find('@') != std::string::npos) {
      if (umount2(mount.c_str(), 0) != 0) {
        return ErrnoError() << "Failed to unmount " << mount;
      }
    }
  }

  static constexpr const char* kApexMountForTest = "/mnt/scratch/apex";

  // Clean up in case previous test left directory behind.
  if (access(kApexMountForTest, F_OK) == 0) {
    if (umount2(kApexMountForTest, MNT_FORCE | UMOUNT_NOFOLLOW) != 0) {
      PLOG(WARNING) << "Failed to unmount " << kApexMountForTest;
    }
    if (rmdir(kApexMountForTest) != 0) {
      return ErrnoError() << "Failed to rmdir " << kApexMountForTest;
    }
  }

  // 4. Create an empty tmpfs that will substitute /apex in tests.
  if (mkdir(kApexMountForTest, 0755) != 0) {
    return ErrnoError() << "Failed to mkdir " << kApexMountForTest;
  }

  if (mount("tmpfs", kApexMountForTest, "tmpfs", 0, nullptr) == -1) {
    return ErrnoError() << "Failed to mount " << kApexMountForTest;
  }

  // 5. Overlay it  over /apex via bind mount.
  if (mount(kApexMountForTest, "/apex", nullptr, MS_BIND, nullptr) == -1) {
    return ErrnoError() << "Failed to bind mount " << kApexMountForTest
                        << " over /apex";
  }

  // Just in case, run restorecon -R on /apex.
  if (selinux_android_restorecon("/apex", SELINUX_ANDROID_RESTORECON_RECURSE) <
      0) {
    return android::base::ErrnoError() << "Failed to restorecon /apex";
  }

  return {};
}

}  // namespace apex
}  // namespace android

namespace com {
namespace android {
namespace apex {

namespace testing {
MATCHER_P(ApexInfoXmlEq, other, "") {
  using ::testing::AllOf;
  using ::testing::Eq;
  using ::testing::ExplainMatchResult;
  using ::testing::Field;
  using ::testing::Property;

  return ExplainMatchResult(
      AllOf(
          Property("moduleName", &ApexInfo::getModuleName,
                   Eq(other.getModuleName())),
          Property("modulePath", &ApexInfo::getModulePath,
                   Eq(other.getModulePath())),
          Property("preinstalledModulePath",
                   &ApexInfo::getPreinstalledModulePath,
                   Eq(other.getPreinstalledModulePath())),
          Property("versionCode", &ApexInfo::getVersionCode,
                   Eq(other.getVersionCode())),
          Property("isFactory", &ApexInfo::getIsFactory,
                   Eq(other.getIsFactory())),
          Property("isActive", &ApexInfo::getIsActive, Eq(other.getIsActive())),
          Property("lastUpdateMillis", &ApexInfo::getLastUpdateMillis,
                   Eq(other.getLastUpdateMillis()))),
      arg, result_listener);
}

}  // namespace testing

// Must be in com::android::apex namespace for gtest to pick it up.
inline void PrintTo(const ApexInfo& apex, std::ostream* os) {
  *os << "apex_info: {\n";
  *os << "  moduleName : " << apex.getModuleName() << "\n";
  *os << "  modulePath : " << apex.getModulePath() << "\n";
  *os << "  preinstalledModulePath : " << apex.getPreinstalledModulePath()
      << "\n";
  *os << "  versionCode : " << apex.getVersionCode() << "\n";
  *os << "  isFactory : " << apex.getIsFactory() << "\n";
  *os << "  isActive : " << apex.getIsActive() << "\n";
  *os << "}";
}

}  // namespace apex
}  // namespace android
}  // namespace com
