/*
 * Copyright 2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parsedouble.h>
#include <android-base/stringprintf.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/fs.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cutils/properties.h>

#include <android/hardware/boot/1.1/IBootControl.h>
#include <hardware/hardware.h>
#include <map>

#include "otautil/sysutil.h"
#include "otautil/error_code.h"
#include "updater/mounts.h"
#include "bootinfo.h"
#include "BurnBoot.h"
#include "BurnNandBoot.h"
#include "BurnSdBoot.h"

#include "UpdateCustom.h"

using android::sp;
using android::hardware::boot::V1_1::IBootControl;

static uint32_t abSlot = -1;

bool executeBurn(ZipArchiveHandle zip, const char* path, BufferExtractCookie* cookie) {
    ZipEntry entry;
    if (FindEntry(zip, path, &entry) != 0) {
        LOG(ERROR) << "Path: failed";
        return false;
    }
    cookie->len = entry.uncompressed_length;
    cookie->buffer = (unsigned char *)malloc(cookie->len);
    int32_t ret = ExtractToMemory(zip, &entry, cookie->buffer, cookie->len);
    if (ret) {
        LOG(ERROR) << "ExtractToMemory: failed";
        return false;
    }
    return true;
}

static int executeUbootUpdate(DeviceBurn burnFunc, ZipArchiveHandle zip, const char* path) {
    BufferExtractCookie* cookie =
        reinterpret_cast<BufferExtractCookie *>(malloc(sizeof(BufferExtractCookie)));
    if (!executeBurn(zip, path, cookie)) {
        return -1;
    }
    int ret = burnFunc(cookie);
    free(cookie);
    return ret;
}

static int executeUbootUpdateNew(DeviceBurnNew burnFunc, ZipArchiveHandle zip,
    const char* path, bool updateBoot) {
    BufferExtractCookie* cookie =
        reinterpret_cast<BufferExtractCookie *>(malloc(sizeof(BufferExtractCookie)));
    if (!executeBurn(zip, path, cookie)) {
        return -1;  
    }
    int ret = burnFunc(cookie, abSlot, updateBoot);
    free(cookie);
    return ret;
}

static int executePartitionUpdate(DeviceBurnCustom burnFunc, ZipArchiveHandle zip,
    const char* path, std::uint32_t targetSlot, std::string partition) {
    BufferExtractCookie* cookie =
        reinterpret_cast<BufferExtractCookie *>(malloc(sizeof(BufferExtractCookie)));
    if (!executeBurn(zip, path, cookie)) {
        return -1;
    }
    int ret = burnFunc(cookie, targetSlot, partition);
    free(cookie);
    return ret;
}

int updateUboot(ZipArchiveHandle zip) {
    const char *boot0_fex;
    const char *uboot_fex;
    int flash_type = getFlashType();
    if (flash_type == FLASH_TYPE_UNKNOW)
        return -1;
    int secure = check_soc_is_secure();
    if (secure) {
        boot0_fex = TOC0_FEX;
        uboot_fex = TOC1_FEX;
    } else {
        if (flash_type == FLASH_TYPE_NAND) {
            boot0_fex = BOOT0_NAND_FEX;
        } else {
            boot0_fex = BOOT0_EMMC_FEX;
        }
        uboot_fex = UBOOT_FEX;
    }

    std::string const& filepath_boot0 = std::string(boot0_fex);
    std::string const& filepath_uboot = std::string(uboot_fex);
    LOG(ERROR) << "filepath_uboot: " << filepath_uboot.c_str();
    LOG(ERROR) << "filepath_boot0: " << filepath_boot0.c_str();

    int ret;
    if (flash_type == FLASH_TYPE_NAND) {
        ret = executeUbootUpdateNew(burnNandBoot, zip, filepath_uboot.c_str(), true) &
            executeUbootUpdateNew(burnNandBoot, zip, filepath_boot0.c_str(), false);
    } else {
        ret = executeUbootUpdate(
        burnSdBoot0, zip, filepath_boot0.c_str()) > 0 &&
        executeUbootUpdate(
        burnSdUboot, zip, filepath_uboot.c_str()) > 0 ? 0 : -1;
    }

    return ret;
}

/** the params of updateCustomPartition
 * zip: the xxx.zip(ota update package location)
 * targetSlot: the target slot, we use 0,1,2,3 to distinguish
 * partition_fex: the fex file of the partition which one you want to update
 * burnpartition_func: the function of custom you want to execute
**/
int updateCustomPartition(ZipArchiveHandle zip, std::uint32_t targetSlot, std::string partition, std::string partition_fex, DeviceBurnCustom burnpartition_func) {
    LOG(INFO) << "partition_fex: " << partition_fex;
    std::string const& filepath_partition = partition_fex;
    return executePartitionUpdate(burnpartition_func, zip, filepath_partition.c_str(), targetSlot, partition);
}

/** the params of UpdateExec
 * filepath: the xxx.zip(ota update package location)
 * targetSlot: the target slot, we use 0,1,2,3 to distinguish
**/
bool UpdateExec(char *filepath, uint32_t targetSlot) {

  // Check the ota update package is exist
    if(filepath != NULL) {
        LOG(INFO) << "get ota package path " << filepath;
    } else {
        LOG(ERROR) << "do not get ota package path ";
        return false;
    }
    abSlot = targetSlot;
    // open zip
    MemMapping map;
    if (!map.MapFile(filepath)) {
        LOG(ERROR) << "failed to map file";
    }

    ZipArchiveHandle zip;
    int err = OpenArchiveFromMemory(map.addr, map.length, filepath, &zip);
    if (err != 0) {
        LOG(ERROR) << "Can't open " << filepath << " : " << ErrorCodeString(err);
        CloseArchive(zip);
        return false;
    }
    LOG(INFO) << "Path: " << filepath;


    // Include the old update boot content
    int ret = updateUboot(zip);
    if (ret != 0) {
        LOG(ERROR) << "update uboot failed!";
        CloseArchive(zip);
        return false;
    }

    // Update partition
    // all partition that can be found in dev/block/by-name also can use
    std::map<std::string, std::string> partition;
    partition["env"] = "env.fex";
    partition["bootloader"] = "boot-resource.fex";

    for (std::map<std::string, std::string>::iterator iter = partition.begin(); iter != partition.end(); iter++) {
        ret = updateCustomPartition(zip, targetSlot, iter->first, iter->second, burnPartition);
        if (ret <= 0) {
            LOG(ERROR) << "update failed!";
            CloseArchive(zip);
            return false;
        }
    }
    CloseArchive(zip);
    return true;
}
