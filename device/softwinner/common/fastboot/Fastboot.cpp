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

#include "Fastboot.h"
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <android/hardware/boot/1.1/IBootControl.h>
#include <cutils/android_reboot.h>
#include <ext4_utils/ext4_utils.h>
#include <ext4_utils/wipe.h>
#include <secure_storage.h>
#include <unordered_map>
#include <sys/ioctl.h>
#include <linux/fs.h>

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

using ::android::hardware::boot::V1_1::IBootControl;
using ::android::hardware::boot::V1_1::MergeStatus;

using ::android::base::ParseInt;
using ::android::base::ReadFileToString;
using ::android::base::ReadFullyAtOffset;
using ::android::base::Split;
using ::android::base::StringPrintf;
using ::android::base::unique_fd;

enum LockStatus {
    EMPTY,
    UNLOCKED,
    LOCKED,
};
static android::sp<IBootControl> boot_control;
static LockStatus lock_status;
static bool is_secure;

static bool GetSecure() {
    std::string cmdline;
    // Return lock status true if unable to read kernel command line.
    if (!ReadFileToString("/proc/cmdline", &cmdline)) {
        return false;
    }
    return cmdline.find("androidboot.verifiedbootstate") != std::string::npos;
}

static int64_t get_file_size(int fd) {
    struct stat buf;
    int ret = fstat(fd, &buf);
    if (ret) return 0;

    int64_t computed_size = 0;
    if (S_ISREG(buf.st_mode)) {
        computed_size = buf.st_size;
    } else if (S_ISBLK(buf.st_mode)) {
        uint64_t block_device_size = get_block_device_size(fd);
        computed_size = block_device_size;
    }

    LOG(INFO) << "Read file size " << computed_size;
    return computed_size;
}

static int64_t get_frp_size() {
    std::unique_ptr<DIR, decltype(&closedir)>dir(opendir("/sys/class/block"), closedir);
    if (!dir) return 0;

    int size = 0;
    dirent* dp;
    while ((dp = readdir(dir.get())) != nullptr) {
        if (dp->d_name[0] == '.') continue;

        std::string uevent;
        auto dir = StringPrintf("/sys/class/block/%s/", dp->d_name);
        ReadFileToString(dir + "uevent", &uevent);
        auto args = Split(uevent, "\n");
        if (std::find(args.begin(), args.end(), "PARTNAME=frp") != args.end()) {
            std::string psize;
            ReadFileToString(dir + "size", &psize);
            if (ParseInt(psize.c_str(), &size, 0))
                size *= 512;
            break;
        }
    }

    return size;
}

static bool OemUnlockEnabled() {
    unique_fd fd(TEMP_FAILURE_RETRY(open(FRP_DEV_PATH.c_str(), O_RDONLY | O_CLOEXEC | O_BINARY)));
    int64_t offset = get_file_size(fd) - 1;
    LOG(INFO) << "Read oem unlock flag offset " << offset;
    int flag;
    ReadFullyAtOffset(fd, &flag, 1, offset);
    LOG(INFO) << "Read oem unlock flag is " << flag;
    return (flag == 1);
}

// init
Fastboot::Fastboot() {
    boot_control = IBootControl::getService();
    is_secure = GetSecure();
    if (!sunxi_secure_storage_init()) {
        char buffer[32] = {0};
        int len;
        sunxi_secure_object_read("fastboot_status_flag", buffer, 32, &len);
        sunxi_secure_storage_exit(0);
        if (!strcmp(buffer, "unlocked")) {
            lock_status = UNLOCKED;
        } else if (!strcmp(buffer, "locked")) {
            lock_status = LOCKED;
        } else {
            lock_status = EMPTY;
        }
    }
}

// Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
Return<void> Fastboot::getPartitionType(const hidl_string& /* partitionName */,
                                        getPartitionType_cb _hidl_cb) {
    _hidl_cb(FileSystemType::RAW, {Status::SUCCESS, ""});
    return Void();
}

static bool EarseUserdata() {
    if (boot_control != nullptr) {
        auto merge_status = boot_control->getSnapshotMergeStatus();
        if (merge_status == MergeStatus::SNAPSHOTTED || merge_status == MergeStatus::MERGING) {
            return false;
        }
    }

    unique_fd fd(TEMP_FAILURE_RETRY(open(USERDATA_DEV_PATH.c_str(), O_WRONLY | O_CLOEXEC | O_BINARY)));
    if (fd < 0)
        fd.reset(TEMP_FAILURE_RETRY(open(UDISK_DEV_PATH.c_str(), O_WRONLY | O_CLOEXEC | O_BINARY)));
    if (fd >= 0) {
        wipe_block_device(fd, get_file_size(fd));
    } else {
        LOG(ERROR) << __FUNCTION__ << "(): " << USERDATA_DEV_PATH << " open failed!";
        return false;
    }

    return true;
}

Result DoOemUnlock(const std::vector<std::string>& /*args*/) {
    if (!is_secure) {
        return { Status::SUCCESS, "The system is normal."};
    }
    if (lock_status == UNLOCKED) {
        return { Status::SUCCESS, "Your device is already unlocked."};
    }

    if (!OemUnlockEnabled()) {
        return { Status::FAILURE_UNKNOWN, "Oem unlock ability is 0. Permission denied for this command!" };
    }

    // write secure storage
    if (sunxi_secure_storage_init()) {
        return { Status::FAILURE_UNKNOWN, "Secure storage init error!" };
    }
    if (sunxi_secure_object_write("fastboot_status_flag", "unlocked", strlen("unlocked"))) {
        return { Status::FAILURE_UNKNOWN, "Save fastboot_status_flag flag to secure storage failed!" };
    }
    if (sunxi_secure_object_write("device_unlock", "unlock", strlen("unlock"))) {
        return { Status::FAILURE_UNKNOWN, "Save device_unlock flag to secure storage failed!" };
    }
    sunxi_secure_storage_exit(0);

    if (!EarseUserdata()) {
        return { Status::FAILURE_UNKNOWN, "Earse userdata failed!" };
    }
    lock_status = UNLOCKED;
    return { Status::SUCCESS, "Unlock device successfully!"};
}

Result DoOemLock(const std::vector<std::string>& /*args*/) {
    if (!is_secure) {
        return { Status::SUCCESS, "The system is normal."};
    }
    if (lock_status == LOCKED) {
        return { Status::SUCCESS, "Your device is already locked."};
    }

    // write secure storage
    if (sunxi_secure_storage_init()) {
        return { Status::FAILURE_UNKNOWN, "Secure storage init error!" };
    }
    if (sunxi_secure_object_write("fastboot_status_flag", "locked", strlen("locked"))) {
        return { Status::FAILURE_UNKNOWN, "Save fastboot_status_flag flag to secure storage failed!" };
    }
    sunxi_secure_storage_exit(0);


    if (!EarseUserdata()) {
        return { Status::FAILURE_UNKNOWN, "Earse userdata failed!" };
    }
    lock_status = LOCKED;
    return { Status::SUCCESS, "Lock device successfully!"};
}

Result DoOemEfex(const std::vector<std::string>& /*args*/) {
    if (!android::base::SetProperty(ANDROID_RB_PROPERTY, "reboot,efex")) {
        return { Status::FAILURE_UNKNOWN, "Reboot failed!" };
    }

    while (true) pause();
    return { Status::SUCCESS, "Reboot to efex!"};
}

Return<void> Fastboot::doOemCommand(const hidl_string& oemCmd, doOemCommand_cb _hidl_cb) {
    const std::unordered_map<std::string, std::function<Result(const std::vector<std::string>&)>> kOEMCmdMap = {
        {OEM_UNLOCK, DoOemUnlock},
        {OEM_LOCK, DoOemLock},
        {OEM_EFEX, DoOemEfex},
    };

    auto args = Split(oemCmd, " ");
    if (args.size() < 2) {
        _hidl_cb({ Status::INVALID_ARGUMENT, "Invalid OEM command" });
        return Void();
    }

    // args[0] will be "oem", args[1] will be the command name
    auto cmd_handler = kOEMCmdMap.find(args[1]);
    if (cmd_handler != kOEMCmdMap.end()) {
        _hidl_cb(cmd_handler->second(std::vector<std::string>(args.begin() + 2, args.end())));
    } else {
        _hidl_cb({ Status::FAILURE_UNKNOWN, "Unknown OEM command" });
    }

    return Void();
}

Return<void> Fastboot::getVariant(getVariant_cb _hidl_cb) {
    _hidl_cb("NA", {Status::SUCCESS, ""});
    return Void();
}

Return<void> Fastboot::getOffModeChargeState(getOffModeChargeState_cb _hidl_cb) {
    _hidl_cb(false, {Status::SUCCESS, ""});
    return Void();
}

Return<void> Fastboot::getBatteryVoltageFlashingThreshold(
        getBatteryVoltageFlashingThreshold_cb _hidl_cb) {
    _hidl_cb(0, {Status::SUCCESS, ""});
    return Void();
}

extern "C" IFastboot* HIDL_FETCH_IFastboot(const char* /* name */) {
    return new Fastboot();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android
