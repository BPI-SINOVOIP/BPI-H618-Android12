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

#include "IcuRegistration.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <unicode/udata.h>
#include <unicode/utypes.h>

#ifndef __ANDROID__
static int PriorityToLevel(char priority) {
  // Priority is just the array index of priority in kPriorities.
  static const char* kPriorities = "VDIWEF";
  static const int kLogSuppress = sizeof(kPriorities);
  const char* matching_priority = strchr(kPriorities, toupper(priority));
  return (matching_priority != nullptr) ? matching_priority - kPriorities : kLogSuppress;
}

static int GetHostLogLevel() {
  const char* log_tags = getenv("ANDROID_LOG_TAGS");
  if (log_tags == nullptr) {
    return 0;
  }
  // Find the wildcard prefix if present in ANDROID_LOG_TAGS.
  static constexpr const char kLogWildcardPrefix[] = "*:";
  static constexpr size_t kLogWildcardPrefixLength = sizeof(kLogWildcardPrefix) - 1;
  const char* wildcard_start = strstr(log_tags, kLogWildcardPrefix);
  if (wildcard_start == nullptr) {
    return 0;
  }
  // Priority is based on the character after the wildcard prefix.
  char priority = *(wildcard_start + kLogWildcardPrefixLength);
  return PriorityToLevel(priority);
}

bool AIcuHostShouldLog(char priority) {
  static int g_LogLevel = GetHostLogLevel();
  return PriorityToLevel(priority) >= g_LogLevel;
}
#endif  // __ANDROID__

namespace androidicuinit {
namespace impl {

#ifndef __ANDROID__
// http://b/171371690 Avoid dependency on liblog and libbase on host
// Simplified version of android::base::unique_fd for host.
class simple_unique_fd final {
 public:
  simple_unique_fd(int fd) {
    reset(fd);
  }
  ~simple_unique_fd() {
    reset();
  }
  int get() {
    return fd_;
  }

 private:
  int fd_ = -1;
  void reset(int new_fd = -1) {
    if (fd_ != -1) {
      close(fd_);
    }
    fd_ = new_fd;
  }
  // Disable copy constructor and assignment operator
  simple_unique_fd(const simple_unique_fd&) = delete;
  void operator=(const simple_unique_fd&) = delete;

};

  // A copy of TEMP_FAILURE_RETRY from android-base/macros.h
  // bionic and glibc both have TEMP_FAILURE_RETRY, but eg Mac OS' libc doesn't.
  #ifndef TEMP_FAILURE_RETRY
    #define TEMP_FAILURE_RETRY(exp)            \
      ({                                       \
        decltype(exp) _rc;                     \
        do {                                   \
          _rc = (exp);                         \
        } while (_rc == -1 && errno == EINTR); \
        _rc;                                   \
      })
  #endif
#endif // #ifndef __ANDROID__

// http://b/171371690 Avoid dependency on liblog and libbase on host
#ifdef __ANDROID__
  typedef android::base::unique_fd aicu_unique_fd;
#else
  typedef simple_unique_fd aicu_unique_fd;
#endif

// Map in ICU data at the path, returning null to print error if it failed.
std::unique_ptr<IcuDataMap> IcuDataMap::Create(const std::string& path) {
  std::unique_ptr<IcuDataMap> map(new IcuDataMap(path));

  if (!map->TryMap()) {
    // madvise or ICU could fail but mmap still succeeds.
    // Destructor will take care of cleaning up a partial init.
    return nullptr;
  }

  return map;
}

// Unmap the ICU data.
IcuDataMap::~IcuDataMap() { TryUnmap(); }

bool IcuDataMap::TryMap() {
  // Open the file and get its length.
  aicu_unique_fd fd(TEMP_FAILURE_RETRY(open(path_.c_str(), O_RDONLY)));

  if (fd.get() == -1) {
    AICU_LOGE("Couldn't open '%s': %s", path_.c_str(), strerror(errno));
    return false;
  }

  struct stat sb;
  if (fstat(fd.get(), &sb) == -1) {
    AICU_LOGE("Couldn't stat '%s': %s", path_.c_str(), strerror(errno));
    return false;
  }

  data_length_ = sb.st_size;

  // Map it.
  data_ =
      mmap(NULL, data_length_, PROT_READ, MAP_SHARED, fd.get(), 0 /* offset */);
  if (data_ == MAP_FAILED) {
    AICU_LOGE("Couldn't mmap '%s': %s", path_.c_str(), strerror(errno));
    return false;
  }

  // Tell the kernel that accesses are likely to be random rather than
  // sequential.
  if (madvise(data_, data_length_, MADV_RANDOM) == -1) {
    AICU_LOGE("Couldn't madvise(MADV_RANDOM) '%s': %s", path_.c_str(),
          strerror(errno));
    return false;
  }

  UErrorCode status = U_ZERO_ERROR;

  // Tell ICU to use our memory-mapped data.
  udata_setCommonData(data_, &status);
  if (status != U_ZERO_ERROR) {
    AICU_LOGE("Couldn't initialize ICU (udata_setCommonData): %s (%s)",
          u_errorName(status), path_.c_str());
    return false;
  }

  return true;
}

bool IcuDataMap::TryUnmap() {
  // Don't need to do opposite of udata_setCommonData,
  // u_cleanup (performed in IcuRegistration::~IcuRegistration()) takes care of
  // it.

  // Don't need to opposite of madvise, munmap will take care of it.

  if (data_ != nullptr && data_ != MAP_FAILED) {
    if (munmap(data_, data_length_) == -1) {
      AICU_LOGE("Couldn't munmap '%s': %s", path_.c_str(), strerror(errno));
      return false;
    }
  }

  // Don't need to close the file, it was closed automatically during TryMap.
  return true;
}

}  // namespace impl

// A pointer to the instance used by Register and Deregister. Since this code
// is currently included in a static library this doesn't prevent duplicate
// initialization calls.
static std::unique_ptr<IcuRegistration> gIcuRegistration;

void IcuRegistration::Register() {
  CHECK(gIcuRegistration.get() == nullptr);

  gIcuRegistration.reset(new IcuRegistration());
}

void IcuRegistration::Deregister() {
  gIcuRegistration.reset();
}

// Init ICU, configuring it and loading the data files.
IcuRegistration::IcuRegistration() {
  // Note: This logic below should match the logic for ICU4J in
  // TimeZoneDataFiles.java in libcore/ to ensure consistent behavior between
  // ICU4C and ICU4J.

  // Check the timezone /data override file exists from the "Time zone update
  // via APK" feature.
  // https://source.android.com/devices/tech/config/timezone-rules
  // If it does, map it first so we use its data in preference to later ones.
  std::string dataPath = getDataTimeZonePath();
  if (pathExists(dataPath)) {
    AICU_LOGD("Time zone override file found: %s", dataPath.c_str());
    icu_datamap_from_data_ = impl::IcuDataMap::Create(dataPath);
    if (icu_datamap_from_data_ == nullptr) {
      AICU_LOGW(
          "TZ override /data file %s exists but could not be loaded. Skipping.",
          dataPath.c_str());
    }
  } else {
    AICU_LOGV("No timezone override /data file found: %s", dataPath.c_str());
  }

  // Check the timezone override file exists from a mounted APEX file.
  // If it does, map it next so we use its data in preference to later ones.
  std::string tzModulePath = getTimeZoneModulePath();
  if (pathExists(tzModulePath)) {
    AICU_LOGD("Time zone APEX ICU file found: %s", tzModulePath.c_str());
    icu_datamap_from_tz_module_ = impl::IcuDataMap::Create(tzModulePath);
    if (icu_datamap_from_tz_module_ == nullptr) {
      AICU_LOGW(
          "TZ module override file %s exists but could not be loaded. "
          "Skipping.",
          tzModulePath.c_str());
    }
  } else {
    AICU_LOGV("No time zone module override file found: %s", tzModulePath.c_str());
  }

  // Use the ICU data files that shipped with the i18n module for everything
  // else.
  std::string i18nModulePath = getI18nModulePath();
  icu_datamap_from_i18n_module_ = impl::IcuDataMap::Create(i18nModulePath);
  if (icu_datamap_from_i18n_module_ == nullptr) {
    // IcuDataMap::Create() will log on error so there is no need to log here.
    abort();
  }
  AICU_LOGD("I18n APEX ICU file found: %s", i18nModulePath.c_str());
}

// De-init ICU, unloading the data files. Do the opposite of the above function.
IcuRegistration::~IcuRegistration() {
  // Unmap ICU data files.
  icu_datamap_from_i18n_module_.reset();
  icu_datamap_from_tz_module_.reset();
  icu_datamap_from_data_.reset();
}

bool IcuRegistration::pathExists(const std::string& path) {
  struct stat sb;
  return stat(path.c_str(), &sb) == 0;
}

// Returns a string containing the expected path of the (optional) /data tz data
// file
std::string IcuRegistration::getDataTimeZonePath() {
  const char* dataPathPrefix = getenv("ANDROID_DATA");
  if (dataPathPrefix == NULL) {
    AICU_LOGE("ANDROID_DATA environment variable not set");
    abort();
  }
  std::string dataPath;
  dataPath = dataPathPrefix;
  dataPath += "/misc/zoneinfo/current/icu/icu_tzdata.dat";

  return dataPath;
}

// Returns a string containing the expected path of the (optional) /apex tz
// module data file
std::string IcuRegistration::getTimeZoneModulePath() {
  const char* tzdataModulePathPrefix = getenv("ANDROID_TZDATA_ROOT");
  if (tzdataModulePathPrefix == NULL) {
    AICU_LOGE("ANDROID_TZDATA_ROOT environment variable not set");
    abort();
  }

  std::string tzdataModulePath;
  tzdataModulePath = tzdataModulePathPrefix;
  tzdataModulePath += "/etc/icu/icu_tzdata.dat";
  return tzdataModulePath;
}

std::string IcuRegistration::getI18nModulePath() {
  const char* i18nModulePathPrefix = getenv("ANDROID_I18N_ROOT");
  if (i18nModulePathPrefix == NULL) {
    AICU_LOGE("ANDROID_I18N_ROOT environment variable not set");
    abort();
  }

  std::string i18nModulePath;
  i18nModulePath = i18nModulePathPrefix;
  i18nModulePath += "/etc/icu/" U_ICUDATA_NAME ".dat";
  return i18nModulePath;
}

}  // namespace androidicuinit

void android_icu_register() {
  androidicuinit::IcuRegistration::Register();
}

void android_icu_deregister() {
  androidicuinit::IcuRegistration::Deregister();
}

bool android_icu_is_registered() {
  return androidicuinit::gIcuRegistration.get() != nullptr;
}
