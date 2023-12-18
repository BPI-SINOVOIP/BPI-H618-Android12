/*
 * Copyright (C) 2021 The Android Open Source Project
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
#define LOG_TAG "apex_shared_libraries_test"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/scopeguard.h>
#include <android-base/strings.h>
#include <dlfcn.h>
#include <fstab/fstab.h>
#include <gtest/gtest.h>
#include <link.h>

#include <filesystem>
#include <fstream>
#include <string>

using android::base::GetBoolProperty;
using android::base::Split;
using android::base::StartsWith;
using android::fs_mgr::Fstab;
using android::fs_mgr::ReadFstabFromFile;

namespace fs = std::filesystem;

static constexpr const char* kApexRoot = "/apex";
static constexpr const char* kApexSharedLibsRoot = "/apex/sharedlibs";

TEST(apex_shared_libraries, symlink_libraries_loadable) {
  if (!GetBoolProperty("ro.apex.updatable", false)) {
    GTEST_SKIP() << "Skipping test because device doesn't support APEX";
  }

  Fstab fstab;
  ASSERT_TRUE(ReadFstabFromFile("/proc/mounts", &fstab));

  // Traverse mount points to identify apexs.
  for (auto& entry : fstab) {
    if (fs::path(entry.mount_point).parent_path() != kApexRoot) {
      continue;
    }

    // Focus on "active" apexs.
    if (entry.mount_point.find('@') != std::string::npos) {
      continue;
    }
    std::string dev_file = fs::path(entry.blk_device).filename();

    // Filter out any mount irrelevant (e.g. tmpfs)
    if (!StartsWith(dev_file, "loop") && !StartsWith(dev_file, "dm-")) {
      continue;
    }

#if !defined(__LP64__)
    auto lib = fs::path(entry.mount_point) / "lib";
#else   // !__LP64__
    auto lib = fs::path(entry.mount_point) / "lib64";
#endif  // !__LP64__
    if (!fs::is_directory(lib)) {
      continue;
    }
    for (auto& p : fs::directory_iterator(lib)) {
      std::error_code ec;
      if (!fs::is_symlink(p, ec)) {
        continue;
      }

      // We are only checking libraries pointing at a location inside
      // /apex/sharedlibs.
      auto target = fs::read_symlink(p.path(), ec);
      if (ec || !StartsWith(target.string(), kApexSharedLibsRoot)) {
        continue;
      }

      // Symlink validity check.
      auto dest = fs::canonical(p.path(), ec);
      EXPECT_FALSE(ec) << "Failed to resolve " << p.path() << " (symlink to "
                       << target << "): " << ec;
      if (ec) {
        continue;
      }

      // Library loading validity check.
      dlerror();  // Clear any pending errors.
      void* handle = dlopen(p.path().c_str(), RTLD_NOW);
      EXPECT_TRUE(handle != nullptr) << dlerror();
      if (handle == nullptr) {
        continue;
      }
      auto guard = android::base::make_scope_guard([&]() { dlclose(handle); });

      // Check that library is loaded and pointing to the realpath of the
      // library.
      auto dl_callback = [](dl_phdr_info* info, size_t /* size */, void* data) {
        auto dest = *static_cast<fs::path*>(data);
        if (info->dlpi_name == nullptr) {
          // This is linker imposing as libdl.so - skip it
          return 0;
        }
        int j;
        for (j = 0; j < info->dlpi_phnum; j++) {
          void* addr = (void*)(info->dlpi_addr + info->dlpi_phdr[j].p_vaddr);
          Dl_info dl_info;
          int rc = dladdr(addr, &dl_info);
          if (rc == 0) {
            continue;
          }
          if (dl_info.dli_fname) {
            auto libpath = fs::path(dl_info.dli_fname);
            if (libpath == dest) {
              // Library found!
              return 1;
            }
          }
        }

        return 0;
      };
      bool found = (dl_iterate_phdr(dl_callback, &dest) == 1);
      EXPECT_TRUE(found) << "Error verifying library symlink " << p.path()
                         << " which points to " << target
                         << " which resolves to file " << dest;
      if (found) {
        LOG(INFO) << "Verified that " << p.path()
                  << " correctly loads as library " << dest;
      }
    }
  }
}
