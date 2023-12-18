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

#include "install/fuse_install.h"

#include <dirent.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>

#include "bootloader_message/bootloader_message.h"
#include "fuse_provider.h"
#include "fuse_sideload.h"
#include "install/install.h"
#include "recovery_utils/roots.h"
#include "otautil/sysutil.h"

static constexpr const char* SDCARD_ROOT = "/sdcard";
// How long (in seconds) we wait for the fuse-provided package file to
// appear, before timing out.
static constexpr int SDCARD_INSTALL_TIMEOUT = 10;

// Set the BCB to reboot back into recovery (it won't resume the install from
// sdcard though).
static void SetSdcardUpdateBootloaderMessage() {
  std::vector<std::string> options;
  std::string err;
  if (!update_bootloader_message(options, &err)) {
    LOG(ERROR) << "Failed to set BCB message: " << err;
  }
}

// Returns the selected filename, or an empty string.
static std::string BrowseDirectory(const std::string& path, Device* device, RecoveryUI* ui) {
  ensure_path_mounted(path);

  std::unique_ptr<DIR, decltype(&closedir)> d(opendir(path.c_str()), closedir);
  if (!d) {
    PLOG(ERROR) << "error opening " << path;
    return "";
  }

  std::vector<std::string> dirs;
  std::vector<std::string> entries{ "../" };  // "../" is always the first entry.

  dirent* de;
  while ((de = readdir(d.get())) != nullptr) {
    std::string name(de->d_name);

    if (de->d_type == DT_DIR) {
      // Skip "." and ".." entries.
      if (name == "." || name == "..") continue;
      dirs.push_back(name + "/");
    } else if (de->d_type == DT_REG && (android::base::EndsWithIgnoreCase(name, ".zip") ||
                                        android::base::EndsWithIgnoreCase(name, ".map"))) {
      entries.push_back(name);
    }
  }

  std::sort(dirs.begin(), dirs.end());
  std::sort(entries.begin(), entries.end());

  // Append dirs to the entries list.
  entries.insert(entries.end(), dirs.begin(), dirs.end());

  std::vector<std::string> headers{ "Choose a package to install:", path };

  size_t chosen_item = 0;
  while (true) {
    chosen_item = ui->ShowMenu(
        headers, entries, chosen_item, true,
        std::bind(&Device::HandleMenuKey, device, std::placeholders::_1, std::placeholders::_2));

    // Return if WaitKey() was interrupted.
    if (chosen_item == static_cast<size_t>(RecoveryUI::KeyError::INTERRUPTED)) {
      return "";
    }

    const std::string& item = entries[chosen_item];
    if (chosen_item == 0) {
      // Go up but continue browsing (if the caller is BrowseDirectory).
      return "";
    }

    std::string new_path = path + "/" + item;
    if (new_path.back() == '/') {
      // Recurse down into a subdirectory.
      new_path.pop_back();
      std::string result = BrowseDirectory(new_path, device, ui);
      if (!result.empty()) return result;
    } else {
      // Selected a zip file: return the path to the caller.
      return new_path;
    }
  }

  // Unreachable.
}

// Returns the selected block path, or an empty string.
static std::string BrowseBlock(const std::string& path, Device* device, RecoveryUI* ui) {
  std::unique_ptr<DIR, decltype(&closedir)> d(opendir(path.c_str()), closedir);
  if (!d) {
    PLOG(ERROR) << "error opening " << path;
    return "";
  }
  bool isnand = access((path + "/nand0").c_str(), F_OK) == 0;

  std::vector<std::string> blocks;
  std::vector<std::string> entries{ "../" };  // "../" is always the first entry.

  dirent* de;
  while ((de = readdir(d.get())) != nullptr) {
    std::string name(de->d_name);

    if (de->d_type == DT_BLK
        && !android::base::StartsWithIgnoreCase(name, isnand ? "nand0" : "mmcblk0")
        && !android::base::StartsWithIgnoreCase(name, "dm-")
        && !android::base::StartsWithIgnoreCase(name, "loop")
        && !android::base::StartsWithIgnoreCase(name, "ram")
        && !android::base::StartsWithIgnoreCase(name, "zram")) {
      blocks.push_back(name);
    }
  }

  std::sort(blocks.begin(), blocks.end(), std::greater());

  // Append dirs to the entries list.
  entries.insert(entries.end(), blocks.begin(), blocks.end());

  std::vector<std::string> headers{ "Choose a block device to mount:", path };

  size_t chosen_item = 0;
  while (true) {
    chosen_item = ui->ShowMenu(
        headers, entries, chosen_item, true,
        std::bind(&Device::HandleMenuKey, device, std::placeholders::_1, std::placeholders::_2));

    // Return if WaitKey() was interrupted.
    if (chosen_item == static_cast<size_t>(RecoveryUI::KeyError::INTERRUPTED)) {
      return "";
    }

    const std::string& item = entries[chosen_item];
    if (chosen_item == 0) {
      // Go up but continue browsing (if the caller is BrowseDirectory).
      return "";
    }

    std::string new_path = path + "/" + item;
    return new_path;
  }

  // Unreachable.
}

static bool StartInstallPackageFuse(std::string_view path) {
  if (path.empty()) {
    return false;
  }

  constexpr auto FUSE_BLOCK_SIZE = 65536;
  bool is_block_map = android::base::ConsumePrefix(&path, "@");
  auto fuse_data_provider =
      is_block_map ? FuseBlockDataProvider::CreateFromBlockMap(std::string(path), FUSE_BLOCK_SIZE)
                   : FuseFileDataProvider::CreateFromFile(std::string(path), FUSE_BLOCK_SIZE);

  if (!fuse_data_provider || !fuse_data_provider->Valid()) {
    LOG(ERROR) << "Failed to create fuse data provider.";
    return false;
  }

  if (android::base::StartsWith(path, SDCARD_ROOT)) {
    // The installation process expects to find the sdcard unmounted. Unmount it with MNT_DETACH so
    // that our open file continues to work but new references see it as unmounted.
    umount2(SDCARD_ROOT, MNT_DETACH);
  }

  return run_fuse_sideload(std::move(fuse_data_provider)) == 0;
}

InstallResult InstallWithFuseFromPath(std::string_view path, RecoveryUI* ui) {
  // We used to use fuse in a thread as opposed to a process. Since accessing
  // through fuse involves going from kernel to userspace to kernel, it leads
  // to deadlock when a page fault occurs. (Bug: 26313124)
  pid_t child;
  if ((child = fork()) == 0) {
    bool status = StartInstallPackageFuse(path);

    _exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
  }

  // FUSE_SIDELOAD_HOST_PATHNAME will start to exist once the fuse in child process is ready.
  InstallResult result = INSTALL_ERROR;
  int status;
  bool waited = false;
  for (int i = 0; i < SDCARD_INSTALL_TIMEOUT; ++i) {
    if (waitpid(child, &status, WNOHANG) == -1) {
      result = INSTALL_ERROR;
      waited = true;
      break;
    }

    struct stat sb;
    if (stat(FUSE_SIDELOAD_HOST_PATHNAME, &sb) == -1) {
      if (errno == ENOENT && i < SDCARD_INSTALL_TIMEOUT - 1) {
        sleep(1);
        continue;
      } else {
        LOG(ERROR) << "Timed out waiting for the fuse-provided package.";
        result = INSTALL_ERROR;
        kill(child, SIGKILL);
        break;
      }
    }
    auto package =
        Package::CreateFilePackage(FUSE_SIDELOAD_HOST_PATHNAME,
                                   std::bind(&RecoveryUI::SetProgress, ui, std::placeholders::_1));
    result =
        InstallPackage(package.get(), FUSE_SIDELOAD_HOST_PATHNAME, false, 0 /* retry_count */, ui);
    break;
  }

  if (!waited) {
    // Calling stat() on this magic filename signals the fuse
    // filesystem to shut down.
    struct stat sb;
    stat(FUSE_SIDELOAD_HOST_EXIT_PATHNAME, &sb);

    waitpid(child, &status, 0);
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LOG(ERROR) << "Error exit from the fuse process: " << WEXITSTATUS(status);
  }

  return result;
}

int SupportNtfs(const std::string& block_device) {
  int status;
  pid_t pid = fork();

  if (pid == -1) {
    LOG(ERROR) << "Failed to fork child process";
    return -1;
  }

  if (pid == 0) {
    std::vector<std::string> ntfs_commands = {
      "/system/bin/ntfs-3g",
      "-o",
      "uid=0,gid=0,fmask=0177,dmask=077,big_writes,async,noatime",
      block_device,
      SDCARD_ROOT,
    };
    auto exec_args = StringVectorToNullTerminatedArray(ntfs_commands);
    execv(exec_args[0], exec_args.data());
    LOG(ERROR) << "exec in SupportNtfs";
    _exit(EXIT_FAILURE);
  }

  if (waitpid(pid, &status, 0) == -1) {
    LOG(ERROR) << "waitpid in SupportNtfs";
    return -errno;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LOG(ERROR) << "Error exit from the fuse process: " << WEXITSTATUS(status);
    return -1;
  }

  return 0;
}

int SupportExfat(const std::string& block_device) {
  int status;
  pid_t pid = fork();

  if (pid == -1) {
    LOG(ERROR) << "Failed to fork child process";
    return -1;
  }

  if (pid == 0) {
    std::vector<std::string> exfat_commands = {
      "/system/bin/mount.exfat",
      "-o",
      "uid=0,gid=0,fmask=0177,dmask=077",
      block_device,
      SDCARD_ROOT,
    };
    auto exec_args = StringVectorToNullTerminatedArray(exfat_commands);
    execv(exec_args[0], exec_args.data());
    LOG(ERROR) << "exec in SupportExfat";
    _exit(EXIT_FAILURE);
  }

  if (waitpid(pid, &status, 0) == -1) {
    LOG(ERROR) << "waitpid in SupportExfat";
    return -errno;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LOG(ERROR) << "Error exit from the fuse process: " << WEXITSTATUS(status);
    return -1;
  }

  return 0;
}

InstallResult ApplyFromSdcard(Device* device) {
  auto ui = device->GetUI();
  std::string blk_device = BrowseBlock("/dev/block", device, ui);
  if (blk_device.empty()) {
    LOG(ERROR) << "\n-- No block device selected.\n";
    return INSTALL_ERROR;
  }
  ui->Print("\n-- Select block device %s.\n", blk_device.c_str());
  std::string filesystems;
  if (!android::base::ReadFileToString("/proc/filesystems", &filesystems)) {
    LOG(ERROR) << "Error reading file: /proc/filesystems";
    return INSTALL_ERROR;
  }

  static const std::vector<std::string> supported_fs{"ext4", "vfat", "f2fs", "ntfs", "exfat"};
  int ret = -1;
  for (auto fs_type : supported_fs) {
    if (filesystems.find("\t" + fs_type + "\n") == std::string::npos) {
      LOG(INFO) << "unknown fs_type " << fs_type;
      continue;
    }
    ret = mount(blk_device.c_str(), SDCARD_ROOT,
        fs_type.c_str(), 0, "");
    if (ret == 0) {
      ui->Print("Mount type=%s success.\n", fs_type.c_str());
      break;
    }
    ui->Print("Mount type=%s ret=%d errno=%d %s.\n", fs_type.c_str(), ret, errno, strerror(errno));
  }
  if (ret != 0) {
    if (SupportNtfs(blk_device) != 0) {
      LOG(ERROR) << "\n-- Mount Ntfs " << blk_device << " to " << SDCARD_ROOT << " failed. try Exfat\n";
      if (SupportExfat(blk_device) != 0) {
        LOG(ERROR) << "\n-- Mount Exfat " << blk_device << " to " << SDCARD_ROOT << " failed.\n";
        return INSTALL_ERROR;
      }
    }
  }

  std::string path = BrowseDirectory(SDCARD_ROOT, device, ui);
  if (path.empty()) {
    LOG(ERROR) << "\n-- No package file selected.\n";
    ensure_path_unmounted(SDCARD_ROOT);
    return INSTALL_ERROR;
  }

  // Hint the install function to read from a block map file.
  if (android::base::EndsWithIgnoreCase(path, ".map")) {
    path = "@" + path;
  }

  ui->Print("\n-- Install %s ...\n", path.c_str());
  SetSdcardUpdateBootloaderMessage();

  auto result = InstallWithFuseFromPath(path, ui);
  ensure_path_unmounted(SDCARD_ROOT);
  return result;
}
