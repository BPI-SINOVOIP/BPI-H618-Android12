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

#include "install/wipe_data.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <functional>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "install/snapshot_utils.h"
#include "otautil/dirutil.h"
#include "recovery_ui/ui.h"
#include "recovery_utils/logging.h"
#include "recovery_utils/roots.h"

#include <errno.h>
#include "otautil/sysutil.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>

constexpr const char* CACHE_ROOT = "/cache";
constexpr const char* DATA_ROOT = "/data";
constexpr const char* METADATA_ROOT = "/metadata";
constexpr const char* RESERVE0_ROOT = "/dev/block/by-name/Reserve0";

static bool EraseVolume(const char* volume, RecoveryUI* ui, bool convert_fbe) {
  bool is_cache = (strcmp(volume, CACHE_ROOT) == 0);
  bool is_data = (strcmp(volume, DATA_ROOT) == 0);

  std::vector<saved_log_file> log_files;
  if (is_cache) {
    // If we're reformatting /cache, we load any past logs (i.e. "/cache/recovery/last_*") and the
    // current log ("/cache/recovery/log") into memory, so we can restore them after the reformat.
    log_files = ReadLogFilesToMemory();
  }

  ui->Print("Formatting %s...\n", volume);

  ensure_path_unmounted(volume);

  int result;
  if (is_data && convert_fbe) {
    constexpr const char* CONVERT_FBE_DIR = "/tmp/convert_fbe";
    constexpr const char* CONVERT_FBE_FILE = "/tmp/convert_fbe/convert_fbe";
    // Create convert_fbe breadcrumb file to signal init to convert to file based encryption, not
    // full disk encryption.
    if (mkdir(CONVERT_FBE_DIR, 0700) != 0) {
      PLOG(ERROR) << "Failed to mkdir " << CONVERT_FBE_DIR;
      return false;
    }
    FILE* f = fopen(CONVERT_FBE_FILE, "wbe");
    if (!f) {
      PLOG(ERROR) << "Failed to convert to file encryption";
      return false;
    }
    fclose(f);
    result = format_volume(volume, CONVERT_FBE_DIR);
    remove(CONVERT_FBE_FILE);
    rmdir(CONVERT_FBE_DIR);
  } else {
    result = format_volume(volume);
  }

  if (is_cache) {
    RestoreLogFilesAfterFormat(log_files);
  }

  return (result == 0);
}

static int exec_cmd(const std::vector<std::string>& args) {
  CHECK(!args.empty());
  auto argv = StringVectorToNullTerminatedArray(args);

  pid_t child;
  if ((child = fork()) == 0) {
    execv(argv[0], argv.data());
    _exit(EXIT_FAILURE);
  }

  int status;
  waitpid(child, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    printf("aw_tag %s failed with status %d\n ",argv[0], WEXITSTATUS(status));
  }
  return WEXITSTATUS(status);
}

static int ensure_umount_Reserve0(){
  int ret = umount("/Reserve0");
  if (ret < 0) {
    printf("aw_tag Failed to umount %s ,errorno = %s\n",RESERVE0_ROOT,strerror(errno));
  }
  return ret;
}

bool WipeReserve0(RecoveryUI* ui, const std::function<bool()>& confirm_func) {
  ui->Print("aw_tag start WipeReserve0.\n");
  if (confirm_func && !confirm_func()) {
    return false;
  }

  errno = 0;
  if (access("/system/bin/newfs_msdos", X_OK)) {
    ui->Print("aw_tag open newfs_msdos errorno = %s\n", strerror(errno));
    return false;
  }

  ensure_umount_Reserve0();

  std::vector<std::string> vfat_args = {
    "/system/bin/newfs_msdos", "-F", "16", "-c" , "4", RESERVE0_ROOT
  };
  if (exec_cmd(vfat_args) != 0) {
    ui->Print("aw_tag format %s failed\n",RESERVE0_ROOT);
    return false;
  }
  return true;
}

bool WipeCache(RecoveryUI* ui, const std::function<bool()>& confirm_func) {
  bool has_cache = volume_for_mount_point("/cache") != nullptr;
  if (!has_cache) {
    ui->Print("No /cache partition found.\n");
    return false;
  }

  if (confirm_func && !confirm_func()) {
    return false;
  }

  ui->Print("\n-- Wiping cache...\n");
  ui->SetBackground(RecoveryUI::ERASING);
  ui->SetProgressType(RecoveryUI::INDETERMINATE);

  bool success = EraseVolume("/cache", ui, false);
  ui->Print("Cache wipe %s.\n", success ? "complete" : "failed");
  return success;
}

bool WipeData(Device* device, bool convert_fbe) {
  RecoveryUI* ui = device->GetUI();
  ui->Print("\n-- Wiping data...\n");
  ui->SetBackground(RecoveryUI::ERASING);
  ui->SetProgressType(RecoveryUI::INDETERMINATE);

  if (!FinishPendingSnapshotMerges(device)) {
    ui->Print("Unable to check update status or complete merge, cannot wipe partitions.\n");
    return false;
  }

  bool success = device->PreWipeData();
  if (success) {
    success &= EraseVolume(DATA_ROOT, ui, convert_fbe);
    bool has_cache = volume_for_mount_point("/cache") != nullptr;
    if (has_cache) {
      success &= EraseVolume(CACHE_ROOT, ui, false);
    }
    if (volume_for_mount_point(METADATA_ROOT) != nullptr) {
      success &= EraseVolume(METADATA_ROOT, ui, false);
    }
  }
  if (success) {
    success &= device->PostWipeData();
  }
  ui->Print("Data wipe %s.\n", success ? "complete" : "failed");
  return success;
}
