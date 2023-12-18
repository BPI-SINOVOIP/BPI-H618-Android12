/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <selinux/selinux.h>

#include <logwrap/logwrap.h>

#include "Ntfs.h"
#include "Utils.h"
#include "VoldUtil.h"

#define USE_FS_NTFS 0

using android::base::StringPrintf;

namespace android {
namespace vold {
namespace ntfs {

static const char* kMkfsPath = "/system/bin/mkntfs";
static const char* kFsckPath = "/system/bin/ntfs-3g.probe";
static const char* kMntPath = "/system/bin/ntfs-3g";

bool IsSupported() {
    return access(kMkfsPath, X_OK) == 0 && access(kFsckPath, X_OK) == 0 &&
#if USE_FS_NTFS
           IsFilesystemSupported("ntfs");
#else
           access(kMntPath, X_OK) == 0;
#endif
}

status_t Check(const std::string& source) {
    int pass = 1;
    int rc = 0;
    do {
        std::vector<std::string> cmd;
        cmd.push_back(kFsckPath);
        cmd.push_back("--readwrite");
        cmd.push_back(source);

        // Ntfs devices are currently always untrusted
        rc = ForkExecvp(cmd, nullptr, sFsckUntrustedContext);

        if (rc < 0) {
            LOG(ERROR) << "Filesystem check failed due to logwrap error";
            errno = EIO;
            return -1;
        }

        switch(rc) {
        case 0:
            LOG(INFO) << "Filesystem check completed OK";
            return 0;

        case 2:
            LOG(ERROR) << "Filesystem check failed (not a NTFS filesystem)";
            errno = ENODATA;
            return -1;

        case 4:
            if (pass++ <= 3) {
                LOG(WARNING) << "Filesystem modified - rechecking (pass " << pass << ")";
                continue;
            }
            LOG(ERROR) << "Failing check after too many rechecks";
            errno = EIO;
            return -1;

        case 15:
            LOG(ERROR) << "The disk contains an unclean file system (0, 0).";
            return 0;

        default:
            LOG(ERROR) << "Filesystem check failed (unknown exit code " << rc << ")";
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}


status_t Mount(const std::string& source, const std::string& target, int ownerUid, int ownerGid,
               int permMask) {
    int mountFlags = MS_NODEV | MS_NOSUID | MS_DIRSYNC | MS_NOATIME | MS_NOEXEC;
    auto mountData = android::base::StringPrintf(
            "uid=%d,gid=%d,fmask=%o,dmask=%o,big_writes,async,noatime",
            ownerUid, ownerGid, permMask, permMask);

#if USE_FS_NTFS
    if (mount(source.c_str(), target.c_str(), "ntfs", mountFlags, mountData.c_str()) == 0) {
        return 0;
    }
#else
    std::vector<std::string> cmd;
    cmd.push_back(kMntPath);
    cmd.push_back("-o");
    cmd.push_back(mountData);
    cmd.push_back(source);
    cmd.push_back(target);

    if (ForkExecvp(cmd) == 0)
        return 0;
#endif

    PLOG(ERROR) << "Mount failed; attempting read-only";
    mountFlags |= MS_RDONLY;
    if (mount(source.c_str(), target.c_str(), "ntfs", mountFlags, mountData.c_str()) == 0) {
        return 0;
    }

    return -1;
}

status_t Format(const std::string& source) {
    std::vector<std::string> cmd;
    cmd.push_back(kMkfsPath);
    cmd.push_back(source);

    int rc = ForkExecvp(cmd);
    if (rc == 0) {
        LOG(INFO) << "Format OK";
        return 0;
    } else {
        LOG(ERROR) << "Format failed (code " << rc << ")";
        errno = EIO;
        return -1;
    }
    return 0;
}

}  // namespace ntfs
}  // namespace vold
}  // namespace android
