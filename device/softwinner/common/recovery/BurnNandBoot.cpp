/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <android-base/stringprintf.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "Utils.h"
#include "BurnNandBoot.h"

#define NAND_BLKBURNBOOT0 		_IO('v',127)
#define NAND_BLKBURNUBOOT 		_IO('v',128)
#define DEVNODE_PATH_NAND   "/dev/block/by-name/bootloader"

using namespace std;
static int targetSlot = -1;
static char path[256];

void clearPageCache(){
    FILE *fp = fopen("/proc/sys/vm/drop_caches", "w");
    const char *num = "1";
    if (fp == NULL) {
        printf("clearPageCache: open drop_caches fail\n");
        return;
    }
    fwrite(num, sizeof(char), 1, fp);
    fclose(fp);
}

int burnNandBoot(BufferExtractCookie *cookie, uint32_t slot, bool updateUboot) {
    targetSlot = slot;
    switch(slot) {
        case 0: sprintf(path, "%s%s", DEVNODE_PATH_NAND, "_a");break;
        case 1: sprintf(path, "%s%s", DEVNODE_PATH_NAND, "_b");break;
        default: sprintf(path, "%s", DEVNODE_PATH_NAND);break;
    }

    int ret;
    if (updateUboot) {
        printf("burnboot uboot\n");
        ret = burnNandUboot(cookie);
    } else {
        printf("burnboot boot0\n");
        ret = burnNandBoot0(cookie);
    }
    return ret;
}

int burnNandBoot0(BufferExtractCookie *cookie) {

	if (checkBoot0Sum(cookie)) {
		printf("burnboot: wrong boot0 binary file!\n");
		return -1;
	}

    //const char* path = getPartitionName(targetSlot);
	int fd = open(path, O_RDWR);
	if (fd == -1) {
		printf("burnboot: open device node failed ! errno is %d : %s\n", errno, strerror(errno));
		return -1;
	}

    clearPageCache();

	int ret = ioctl(fd, NAND_BLKBURNBOOT0, (unsigned long)cookie);

	if (ret) {
		printf("burnboot: burnNandBoot0 failed ! errno is %d : %s\n", errno, strerror(errno));
	} else {
		printf("burnboot: burnNandBoot0 succeed!\n");
	}
	close(fd);
	return ret;
}

int burnNandUboot(BufferExtractCookie *cookie) {

	if (checkUbootSum(cookie) && checkBoot1Sum(cookie)) {
		printf("burnboot: wrong uboot binary file!\n");
		return -1;
	}

    //const char* path = getPartitionName(targetSlot);
	int fd = open(path, O_RDWR);
	if (fd == -1) {
		printf("burnboot: open device node failed ! errno is %d : %s\n", errno, strerror(errno));
		return -1;
	}

    clearPageCache();

	int ret = ioctl(fd,NAND_BLKBURNUBOOT, (unsigned long)cookie);

	if (ret) {
		printf("burnboot: burnNandUboot failed ! errno is %d : %s\n", errno, strerror(errno));
	} else {
		printf("burnboot: burnNandUboot succeed!\n");
	}
	close(fd);
	return ret;
}
