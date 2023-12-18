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

#define LOG_TAG "apexd"

#include "apexd_loop.h"

#include <mutex>

#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/loop.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <unistd.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "apexd_utils.h"
#include "string_log.h"

using android::base::Basename;
using android::base::ErrnoError;
using android::base::Error;
using android::base::GetBoolProperty;
using android::base::ParseUint;
using android::base::Result;
using android::base::StartsWith;
using android::base::StringPrintf;
using android::base::unique_fd;

#ifndef LOOP_CONFIGURE
// These can be removed whenever we pull in the Linux v5.8 UAPI headers
struct loop_config {
  __u32 fd;
  __u32 block_size;
  struct loop_info64 info;
  __u64 __reserved[8];
};
#define LOOP_CONFIGURE 0x4C0A
#endif

namespace android {
namespace apex {
namespace loop {

static constexpr const char* kApexLoopIdPrefix = "apex:";

// 128 kB read-ahead, which we currently use for /system as well
static constexpr const char* kReadAheadKb = "128";

// TODO(b/122059364): Even though the kernel has created the loop
// device, we still depend on ueventd to run to actually create the
// device node in userspace. To solve this properly we should listen on
// the netlink socket for uevents, or use inotify. For now, this will
// have to do.
static constexpr size_t kLoopDeviceRetryAttempts = 3u;

void LoopbackDeviceUniqueFd::MaybeCloseBad() {
  if (device_fd.get() != -1) {
    // Disassociate any files.
    if (ioctl(device_fd.get(), LOOP_CLR_FD) == -1) {
      PLOG(ERROR) << "Unable to clear fd for loopback device";
    }
  }
}

Result<void> ConfigureReadAhead(const std::string& device_path) {
  CHECK(StartsWith(device_path, "/dev/"));
  std::string device_name = Basename(device_path);

  std::string sysfs_device =
      StringPrintf("/sys/block/%s/queue/read_ahead_kb", device_name.c_str());
  unique_fd sysfs_fd(open(sysfs_device.c_str(), O_RDWR | O_CLOEXEC));
  if (sysfs_fd.get() == -1) {
    return ErrnoError() << "Failed to open " << sysfs_device;
  }

  int ret = TEMP_FAILURE_RETRY(
      write(sysfs_fd.get(), kReadAheadKb, strlen(kReadAheadKb) + 1));
  if (ret < 0) {
    return ErrnoError() << "Failed to write to " << sysfs_device;
  }

  return {};
}

Result<void> PreAllocateLoopDevices(size_t num) {
  Result<void> loop_ready = WaitForFile("/dev/loop-control", 20s);
  if (!loop_ready.ok()) {
    return loop_ready;
  }
  unique_fd ctl_fd(
      TEMP_FAILURE_RETRY(open("/dev/loop-control", O_RDWR | O_CLOEXEC)));
  if (ctl_fd.get() == -1) {
    return ErrnoError() << "Failed to open loop-control";
  }

  bool found = false;
  size_t start_id = 0;
  constexpr const char* kLoopPrefix = "loop";
  WalkDir("/dev/block", [&](const std::filesystem::directory_entry& entry) {
    std::string devname = entry.path().filename().string();
    if (StartsWith(devname, kLoopPrefix)) {
      size_t id;
      auto parse_ok = ParseUint(
          devname.substr(std::char_traits<char>::length(kLoopPrefix)), &id);
      if (parse_ok && id > start_id) {
        start_id = id;
        found = true;
      }
    }
  });
  if (found) ++start_id;

  // Assumption: loop device ID [0..num) is valid.
  // This is because pre-allocation happens during bootstrap.
  // Anyway Kernel pre-allocated loop devices
  // as many as CONFIG_BLK_DEV_LOOP_MIN_COUNT,
  // Within the amount of kernel-pre-allocation,
  // LOOP_CTL_ADD will fail with EEXIST
  for (size_t id = start_id; id < num + start_id; ++id) {
    int ret = ioctl(ctl_fd.get(), LOOP_CTL_ADD, id);
    if (ret < 0 && errno != EEXIST) {
      return ErrnoError() << "Failed LOOP_CTL_ADD";
    }
  }

  // Don't wait until the dev nodes are actually created, which
  // will delay the boot. By simply returing here, the creation of the dev
  // nodes will be done in parallel with other boot processes, and we
  // just optimistally hope that they are all created when we actually
  // access them for activating APEXes. If the dev nodes are not ready
  // even then, we wait 50ms and warning message will be printed (see below
  // CreateLoopDevice()).
  LOG(INFO) << "Pre-allocated " << num << " loopback devices";
  return {};
}

Result<void> ConfigureLoopDevice(const int device_fd, const std::string& target,
                                 const int32_t image_offset,
                                 const size_t image_size) {
  static bool use_loop_configure;
  static std::once_flag once_flag;
  std::call_once(once_flag, [&]() {
    // LOOP_CONFIGURE is a new ioctl in Linux 5.8 (and backported in Android
    // common) that allows atomically configuring a loop device. It is a lot
    // faster than the traditional LOOP_SET_FD/LOOP_SET_STATUS64 combo, but
    // it may not be available on updating devices, so try once before
    // deciding.
    struct loop_config config;
    memset(&config, 0, sizeof(config));
    config.fd = -1;
    if (ioctl(device_fd, LOOP_CONFIGURE, &config) == -1 && errno == EBADF) {
      // If the IOCTL exists, it will fail with EBADF for the -1 fd
      use_loop_configure = true;
    }
  });

  /*
   * Using O_DIRECT will tell the kernel that we want to use Direct I/O
   * on the underlying file, which we want to do to avoid double caching.
   * Note that Direct I/O won't be enabled immediately, because the block
   * size of the underlying block device may not match the default loop
   * device block size (512); when we call LOOP_SET_BLOCK_SIZE below, the
   * kernel driver will automatically enable Direct I/O when it sees that
   * condition is now met.
   */
  unique_fd target_fd(open(target.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECT));
  if (target_fd.get() == -1) {
    struct statfs stbuf;
    int saved_errno = errno;
    // let's give another try with buffered I/O for EROFS and squashfs
    if (statfs(target.c_str(), &stbuf) != 0 ||
        (stbuf.f_type != EROFS_SUPER_MAGIC_V1 &&
         stbuf.f_type != SQUASHFS_MAGIC &&
         stbuf.f_type != OVERLAYFS_SUPER_MAGIC)) {
      return Error(saved_errno) << "Failed to open " << target;
    }
    LOG(WARNING) << "Fallback to buffered I/O for " << target;
    target_fd.reset(open(target.c_str(), O_RDONLY | O_CLOEXEC));
    if (target_fd.get() == -1) {
      return ErrnoError() << "Failed to open " << target;
    }
  }

  struct loop_info64 li;
  memset(&li, 0, sizeof(li));
  strlcpy((char*)li.lo_crypt_name, kApexLoopIdPrefix, LO_NAME_SIZE);
  li.lo_offset = image_offset;
  li.lo_sizelimit = image_size;
  // Automatically free loop device on last close.
  li.lo_flags |= LO_FLAGS_AUTOCLEAR;

  if (use_loop_configure) {
    struct loop_config config;
    memset(&config, 0, sizeof(config));
    li.lo_flags |= LO_FLAGS_DIRECT_IO;
    config.fd = target_fd.get();
    config.info = li;
    config.block_size = 4096;

    if (ioctl(device_fd, LOOP_CONFIGURE, &config) == -1) {
      return ErrnoError() << "Failed to LOOP_CONFIGURE";
    }

    return {};
  } else {
    if (ioctl(device_fd, LOOP_SET_FD, target_fd.get()) == -1) {
      return ErrnoError() << "Failed to LOOP_SET_FD";
    }

    if (ioctl(device_fd, LOOP_SET_STATUS64, &li) == -1) {
      return ErrnoError() << "Failed to LOOP_SET_STATUS64";
    }

    if (ioctl(device_fd, BLKFLSBUF, 0) == -1) {
      // This works around a kernel bug where the following happens.
      // 1) The device runs with a value of loop.max_part > 0
      // 2) As part of LOOP_SET_FD above, we do a partition scan, which loads
      //    the first 2 pages of the underlying file into the buffer cache
      // 3) When we then change the offset with LOOP_SET_STATUS64, those pages
      //    are not invalidated from the cache.
      // 4) When we try to mount an ext4 filesystem on the loop device, the ext4
      //    code will try to find a superblock by reading 4k at offset 0; but,
      //    because we still have the old pages at offset 0 lying in the cache,
      //    those pages will be returned directly. However, those pages contain
      //    the data at offset 0 in the underlying file, not at the offset that
      //    we configured
      // 5) the ext4 driver fails to find a superblock in the (wrong) data, and
      //    fails to mount the filesystem.
      //
      // To work around this, explicitly flush the block device, which will
      // flush the buffer cache and make sure we actually read the data at the
      // correct offset.
      return ErrnoError() << "Failed to flush buffers on the loop device";
    }

    // Direct-IO requires the loop device to have the same block size as the
    // underlying filesystem.
    if (ioctl(device_fd, LOOP_SET_BLOCK_SIZE, 4096) == -1) {
      PLOG(WARNING) << "Failed to LOOP_SET_BLOCK_SIZE";
    }
  }
  return {};
}

Result<LoopbackDeviceUniqueFd> WaitForDevice(int num) {
  std::string opened_device;
  const std::vector<std::string> candidate_devices = {
      StringPrintf("/dev/block/loop%d", num),
      StringPrintf("/dev/loop%d", num),
  };

  // apexd-bootstrap runs in parallel with ueventd to optimize boot time. In
  // rare cases apexd would try attempt to mount an apex before ueventd created
  // a loop device for it. To work around this we keep polling for loop device
  // to be created until ueventd's cold boot sequence is done.
  // See comment on kLoopDeviceRetryAttempts.
  unique_fd sysfs_fd;
  bool cold_boot_done = GetBoolProperty("ro.cold_boot_done", false);
  for (size_t i = 0; i != kLoopDeviceRetryAttempts; ++i) {
    if (!cold_boot_done) {
      cold_boot_done = GetBoolProperty("ro.cold_boot_done", false);
    }
    for (const auto& device : candidate_devices) {
      sysfs_fd.reset(open(device.c_str(), O_RDWR | O_CLOEXEC));
      if (sysfs_fd.get() != -1) {
        return LoopbackDeviceUniqueFd(std::move(sysfs_fd), device);
      }
    }
    PLOG(WARNING) << "Loopback device " << num << " not ready. Waiting 50ms...";
    usleep(50000);
    if (!cold_boot_done) {
      // ueventd hasn't finished cold boot yet, keep trying.
      i = 0;
    }
  }

  return Error() << "Faled to open loopback device " << num;
}

Result<LoopbackDeviceUniqueFd> CreateLoopDevice(const std::string& target,
                                                const int32_t image_offset,
                                                const size_t image_size) {
  unique_fd ctl_fd(open("/dev/loop-control", O_RDWR | O_CLOEXEC));
  if (ctl_fd.get() == -1) {
    return ErrnoError() << "Failed to open loop-control";
  }

  static std::mutex mlock;
  std::lock_guard lock(mlock);
  int num = ioctl(ctl_fd.get(), LOOP_CTL_GET_FREE);
  if (num == -1) {
    return ErrnoError() << "Failed LOOP_CTL_GET_FREE";
  }

  Result<LoopbackDeviceUniqueFd> loop_device = WaitForDevice(num);
  if (!loop_device.ok()) {
    return loop_device.error();
  }
  CHECK_NE(loop_device->device_fd.get(), -1);

  Result<void> configureStatus = ConfigureLoopDevice(
      loop_device->device_fd.get(), target, image_offset, image_size);
  if (!configureStatus.ok()) {
    return configureStatus.error();
  }

  Result<void> read_ahead_status = ConfigureReadAhead(loop_device->name);
  if (!read_ahead_status.ok()) {
    return read_ahead_status.error();
  }

  return loop_device;
}

void DestroyLoopDevice(const std::string& path, const DestroyLoopFn& extra) {
  unique_fd fd(open(path.c_str(), O_RDWR | O_CLOEXEC));
  if (fd.get() == -1) {
    if (errno != ENOENT) {
      PLOG(WARNING) << "Failed to open " << path;
    }
    return;
  }

  struct loop_info64 li;
  if (ioctl(fd.get(), LOOP_GET_STATUS64, &li) < 0) {
    if (errno != ENXIO) {
      PLOG(WARNING) << "Failed to LOOP_GET_STATUS64 " << path;
    }
    return;
  }

  auto id = std::string((char*)li.lo_crypt_name);
  if (StartsWith(id, kApexLoopIdPrefix)) {
    extra(path, id);

    if (ioctl(fd.get(), LOOP_CLR_FD, 0) < 0) {
      PLOG(WARNING) << "Failed to LOOP_CLR_FD " << path;
    }
  }
}

}  // namespace loop
}  // namespace apex
}  // namespace android
