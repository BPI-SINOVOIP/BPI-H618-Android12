/*
 * Copyright (C) 2016 The Android Open Source Project
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
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BOOTCTL_MSG_OFFSET       2048
#define BOOTCTL_MSG_SIZE         (sizeof(struct bootloader_control))
#define BOOTCTL_DEFAULT_TRIES    6
#define BOOTCTL_DEFAULT_PRIORITY 15
#define CMD_BUF_SIZE             128

typedef enum {
	BOOTCTL_V1_0,
	BOOTCTL_V1_1,
	BOOTCTL_V1_2
} BootCtlVersion;

typedef enum {
	EX_OK,
	EX_SOFTWARE,
	EX_USAGE,
} CMD_STATUS;

typedef enum {
	CANCELLED,
	MERGING,
	NONE,
	SNAPSHOTTED,
	UNKNOWN,
	NOVALUE,
} MergeStatus;

typedef uint32_t Slot;

typedef struct {
	/*
	 * (*init)() perform any initialization tasks needed for the HAL.
	 * This is called only once.
	 */
	void (*init)(void);

	/*
	 * (*getNumberSlots)() returns the number of available slots.
	 * For instance, a system with a single set of partitions would return
	 * 1, a system with A/B would return 2, A/B/C -> 3...
	 */
	unsigned (*getNumberSlots)(void);

	/*
	 * (*getCurrentSlot)() returns the value letting the system know
	 * whether the current slot is A or B. The meaning of A and B is
	 * left up to the implementer. It is assumed that if the current slot
	 * is A, then the block devices underlying B can be accessed directly
	 * without any risk of corruption.
	 * The returned value is always guaranteed to be strictly less than the
	 * value returned by getNumberSlots. Slots start at 0 and
	 * finish at getNumberSlots() - 1
	 */
	unsigned (*getCurrentSlot)(void);

	/*
	 * (*markBootSuccessful)() marks the current slot
	 * as having booted successfully
	 *
	 * Returns 0 on success, -errno on error.
	 */
	int (*markBootSuccessful)(void);

	/*
	 * (*setActiveBootSlot)() marks the slot passed in parameter as
	 * the active boot slot (see getCurrentSlot for an explanation
	 * of the "slot" parameter). This overrides any previous call to
	 * setSlotAsUnbootable.
	 * Returns 0 on success, -errno on error.
	 */
	int (*setActiveBootSlot)(unsigned slot);

	/*
	 * (*setSlotAsUnbootable)() marks the slot passed in parameter as
	 * an unbootable. This can be used while updating the contents of the slot's
	 * partitions, so that the system will not attempt to boot a known bad set up.
	 * Returns 0 on success, -errno on error.
	 */
	int (*setSlotAsUnbootable)(unsigned slot);

	/*
	 * (*isSlotBootable)() returns if the slot passed in parameter is
	 * bootable. Note that slots can be made unbootable by both the
	 * bootloader and by the OS using setSlotAsUnbootable.
	 * Returns 1 if the slot is bootable, 0 if it's not, and -errno on
	 * error.
	 */
	int (*isSlotBootable)(unsigned slot);

	/*
	 * (*getSuffix)() returns the string suffix used by partitions that
	 * correspond to the slot number passed in parameter. The returned string
	 * is expected to be statically allocated and not need to be freed.
	 * Returns NULL if slot does not match an existing slot.
	 */
	const char* (*getSuffix)(unsigned slot);

	/*
	 * (*isSlotMarkedSucessful)() returns if the slot passed in parameter has
	 * been marked as successful using markBootSuccessful.
	 * Returns 1 if the slot has been marked as successful, 0 if it's
	 * not the case, and -errno on error.
	 */
	int (*isSlotMarkedSuccessful)(unsigned slot);

	void* reserved[30];
} bootctl_t_v1_0;

typedef struct  {
	int (*setSnapshotMergeStatus)(MergeStatus status);
	MergeStatus (*getSnapshotMergeStatus)(void);
} bootctl_t_v1_1;

typedef struct  {
	/**
	 * Returns the active slot to boot into on the next boot. If
	 * setActiveBootSlot() has been called, the getter function should return
	 * the same slot as the one provided in the last setActiveBootSlot() call.
	 */
	unsigned (*getActiveBootSlot)(void);
} bootctl_t_v1_2;

struct slot_metadata {
	// Slot priority with 15 meaning highest priority, 1 lowest
	// priority and 0 the slot is unbootable.
	uint8_t priority : 4;
	// Number of times left attempting to boot this slot.
	uint8_t tries_remaining : 3;
	// 1 if this slot has booted successfully, 0 otherwise.
	uint8_t successful_boot : 1;
	// 1 if this slot is corrupted from a dm-verity corruption, 0
	// otherwise.
	uint8_t verity_corrupted : 1;
	// Reserved for further use.
	uint8_t reserved : 7;
} __attribute__((packed));

struct bootloader_control {
	// NUL terminated active slot suffix, a or b
	char slot_suffix[4];
	// Bootloader Control AB magic number (see BOOT_CTRL_MAGIC), 0x42414342
	uint32_t magic;
	// Version of struct being used (see BOOT_CTRL_VERSION), current 1
	uint8_t version;
	// Number of slots being managed.
	uint8_t nb_slot : 3;
	// Number of times left attempting to boot recovery.
	uint8_t recovery_tries_remaining : 3;
	// Status of any pending snapshot merge of dynamic partitions.
	uint8_t merge_status : 3;
	// Ensure 4-bytes alignment for slot_info field.
	uint8_t reserved0[1];
	// Per-slot information.  Up to 4 slots.
	struct slot_metadata slot_info[4];
	// Reserved for further use.
	uint8_t reserved1[8];
	// CRC32 of all 28 bytes preceding this field (little endian
	// format).
	uint32_t crc32_le;
} __attribute__((packed));

int run_cmd(const char *cmd, char *out, uint32_t *len)
{
	FILE *fp = NULL;
	uint32_t size;
	char data[CMD_BUF_SIZE] = {'0'};
	*len = 0;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		printf("popen error!\n");
		return -1;
	}

	while ((size = fread(data, 1, CMD_BUF_SIZE, fp)) > 0) {
		memcpy(out, data, size);
		out += size;
		*len += size;
	}
	pclose(fp);
	return 0;
}

static const unsigned int crc32tab[] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
	0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
	0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
	0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
	0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
	0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
	0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
	0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
	0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
	0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
	0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
	0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
	0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
	0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
	0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
	0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
	0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
	0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
	0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
	0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
	0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
	0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
	0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
	0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
	0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
	0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
	0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
	0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
	0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
	0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
	0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
	0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
	0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
	0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
	0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
	0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
	0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
	0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
	0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
	0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
	0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
	0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
	0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
	0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
	0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
	0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
	0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
	0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
	0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
	0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
	0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

static uint32_t crc32(const uint8_t *buf, uint32_t size)
{
	unsigned int i, crc;
	crc = 0xFFFFFFFF;
	for (i = 0; i < size; i++)
		crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
	return crc^0xFFFFFFFF;
}

static char *get_bootloader_message_blk_device(void)
{
	const char *devlist[] = {
		"/dev/mmcblk0",
		"/dev/nand0",
		"/dev/block/mmcblk0",
		"/dev/block/nand0"};

	struct stat buf;
	char cmd[CMD_BUF_SIZE], result[CMD_BUF_SIZE];
	uint32_t len;
	int ret;
	static char devpath[CMD_BUF_SIZE];

	for (uint32_t i = 0; i < sizeof(devlist) / sizeof(devlist[0]); i++) {
		if (stat(devlist[i], &buf) == 0) {
			sprintf(cmd, "busybox fdisk -l %s | awk '/ misc$/{print $1}'", devlist[i]);
			ret = run_cmd(cmd, result, &len);
			if (ret == 0 && len > 0) {
				sprintf(devpath, "%sp%lu", devlist[i], strtoul(result, NULL, 10));
				return devpath;
			}
		}
	}

	return NULL;
}

static int read_bootctl_message(struct bootloader_control *bc)
{
	char *devpath = get_bootloader_message_blk_device();
	int fd;
	uint8_t buffer[BOOTCTL_MSG_SIZE];
	int len = 0;

	if (devpath == NULL)
		return -1;

	fd = open(devpath, O_RDONLY);
	if (fd < 0) {
		printf("error!\n");
		return -1;
	}

	lseek(fd, BOOTCTL_MSG_OFFSET, SEEK_SET);
	len = read(fd, buffer, BOOTCTL_MSG_SIZE);
	if (len != BOOTCTL_MSG_SIZE) {
		printf("read misc error: %d\n", len);
		close(fd);
		return -1;
	}

	close(fd);
	memcpy(bc, buffer, BOOTCTL_MSG_SIZE);
	return 0;
}

static int write_bootctl_message(struct bootloader_control *bc)
{
	char *devpath = get_bootloader_message_blk_device();
	int fd;
	uint8_t *buffer = (uint8_t *)bc;
	int len = 0;

	if (devpath == NULL)
		return -1;

	fd = open(devpath, O_WRONLY | O_SYNC);
	if (fd < 0) {
		printf("error!\n");
		return -1;
	}

	lseek(fd, BOOTCTL_MSG_OFFSET, SEEK_SET);

	bc->crc32_le = crc32(buffer, BOOTCTL_MSG_SIZE - 4);

	len = write(fd, buffer, BOOTCTL_MSG_SIZE);
	if (len != BOOTCTL_MSG_SIZE) {
		printf("write misc error: %d\n", len);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

void module_init(void)
{
}

static unsigned module_getNumberSlots(void)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);

	if (ret < 0)
		return 0;

	return bc.nb_slot;
}

static unsigned module_getCurrentSlot(void)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);

	if (ret < 0)
		return 0;

	if (bc.slot_suffix[0] >= 'a' && bc.slot_suffix[0] <= 'z')
		return  bc.slot_suffix[0] - 'a';

	return 0;
}

static int module_setActiveBootSlot(unsigned slot)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);
	int slot_num = bc.nb_slot;
	uint32_t i = 0;

	if (ret < 0)
		return ret;

	bc.slot_info[slot].tries_remaining = BOOTCTL_DEFAULT_TRIES;
	bc.slot_info[slot].priority = BOOTCTL_DEFAULT_PRIORITY;

	for (i = 0; i < slot_num; i++) {
		if (i != slot && bc.slot_info[i].priority >= BOOTCTL_DEFAULT_PRIORITY)
			bc.slot_info[i].priority = BOOTCTL_DEFAULT_PRIORITY - 1;
	}

	return write_bootctl_message(&bc);
}

static int module_markBootSuccessful(void)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);
	uint32_t slot = bc.slot_suffix[0] - 'a';

	if (ret < 0)
		return ret;

	bc.slot_info[slot].successful_boot = 1;
	bc.slot_info[slot].tries_remaining = 1;

	return write_bootctl_message(&bc);
}

static int module_setSlotAsUnbootable(unsigned slot)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);

	if (ret < 0)
		return ret;

	bc.slot_info[slot].successful_boot = 0;
	bc.slot_info[slot].tries_remaining = 0;

	return write_bootctl_message(&bc);
}

static int module_isSlotBootable(unsigned slot)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);

	if (ret < 0)
		return ret;

	return bc.slot_info[slot].tries_remaining != 0;
}

static const char *module_getSuffix(unsigned slot)
{
	static char suffix[3] = {'_', 0, 0};
	suffix[1] = slot + 'a';
	return suffix;
}

static int module_isSlotMarkedSuccessful(unsigned slot)
{
	struct bootloader_control bc;
	int ret = read_bootctl_message(&bc);

	if (ret < 0)
		return -1;

	return bc.slot_info[slot].successful_boot ? 0 : -1;
}

static int module_setSnapshotMergeStatus(MergeStatus status)
{
	return 0;
}

static MergeStatus module_getSnapshotMergeStatus(void)
{
	return 0;
}

static unsigned module_getActiveBootSlot(void)
{
	struct bootloader_control bc;
	uint32_t slot_num, slot, priority, i;
	int ret = read_bootctl_message(&bc);

	if (ret)
		return 0;

	slot_num = bc.nb_slot;

	// Use the current slot by default.
	slot = bc.slot_suffix[0] - 'a';
	priority = bc.slot_info[slot].priority;

	// Find the slot with the highest priority.
	for (i = 0; i < slot_num; i++) {
		if (bc.slot_info[i].priority > priority) {
			priority = bc.slot_info[i].priority;
			slot = i;
		}
	}

	return slot;
}

static bootctl_t_v1_0 bootctl_impl_v1_0 = {
	.getNumberSlots = module_getNumberSlots,
	.getCurrentSlot = module_getCurrentSlot,
	.markBootSuccessful = module_markBootSuccessful,
	.setActiveBootSlot = module_setActiveBootSlot,
	.setSlotAsUnbootable = module_setSlotAsUnbootable,
	.setSlotAsUnbootable = module_setSlotAsUnbootable,
	.isSlotBootable = module_isSlotBootable,
	.getSuffix = module_getSuffix,
	.isSlotMarkedSuccessful = module_isSlotMarkedSuccessful,
};

static bootctl_t_v1_1 bootctl_impl_v1_1 = {
	.setSnapshotMergeStatus = module_setSnapshotMergeStatus,
	.getSnapshotMergeStatus = module_getSnapshotMergeStatus,
};

static bootctl_t_v1_2 bootctl_impl_v1_2 = {
	.getActiveBootSlot = module_getActiveBootSlot,
};

static void usage(FILE* where, BootCtlVersion bootVersion, int argc, char* argv[]) {
	fprintf(where,
			"%s - command-line wrapper for the boot HAL.\n"
			"\n"
			"Usage:\n"
			"  %s COMMAND\n"
			"\n"
			"Commands:\n"
			"  hal-info                       - Show info about boot_control HAL used.\n"
			"  get-number-slots               - Prints number of slots.\n"
			"  get-current-slot               - Prints currently running SLOT.\n"
			"  mark-boot-successful           - Mark current slot as GOOD.\n"
			"  get-active-boot-slot           - Prints the SLOT to load on next boot.\n"
			"  set-active-boot-slot SLOT      - On next boot, load and execute SLOT.\n"
			"  set-slot-as-unbootable SLOT    - Mark SLOT as invalid.\n"
			"  is-slot-bootable SLOT          - Returns 0 only if SLOT is bootable.\n"
			"  is-slot-marked-successful SLOT - Returns 0 only if SLOT is marked GOOD.\n"
			"  get-suffix SLOT                - Prints suffix for SLOT.\n",
			argv[0], argv[0]);
	if (bootVersion >= BOOTCTL_V1_1) {
		fprintf(where,
			"  set-snapshot-merge-status STAT - Sets whether a snapshot-merge of any dynamic\n"
			"                                   partition is in progress. Valid STAT values\n"
			"                                   are: none, unknown, snapshotted, merging,\n"
			"                                   or cancelled.\n"
			"  get-snapshot-merge-status      - Prints the current snapshot-merge status.\n");
	}
	fprintf(where,
			"\n"
			"SLOT parameter is the zero-based slot-number.\n");
}

static int do_hal_info(const bootctl_t_v1_0 *module) {
	fprintf(stdout, "HAL Version: %s\n", "v1.0");
	return EX_OK;
}

static int do_get_number_slots(bootctl_t_v1_0 *module) {
	uint32_t numSlots = module->getNumberSlots();
	fprintf(stdout, "%u\n", numSlots);
	return EX_OK;
}

static int do_get_current_slot(bootctl_t_v1_0 *module) {
	Slot curSlot = module->getCurrentSlot();
	fprintf(stdout, "%u\n", curSlot);
	return EX_OK;
}

static int handle_return(const int ret, const char *errStr) {
	if (ret != 0) {
		fprintf(stderr, errStr);
		return EX_SOFTWARE;
	}
	return EX_OK;
}

static int do_mark_boot_successful(bootctl_t_v1_0 *module) {
	int ret = module->markBootSuccessful();
	return handle_return(ret, "Error marking as having booted successfully\n");
}

static int do_get_active_boot_slot(bootctl_t_v1_2 *module) {
	uint32_t slot = module->getActiveBootSlot();
	fprintf(stdout, "%u\n", slot);
	return EX_OK;
}

static int do_set_active_boot_slot(bootctl_t_v1_0 *module, Slot slot_number) {
	int ret = module->setActiveBootSlot(slot_number);
	return handle_return(ret, "Error setting active boot slot: %s\n");
}

static int do_set_slot_as_unbootable(bootctl_t_v1_0 *module, Slot slot_number) {
	int ret = module->setSlotAsUnbootable(slot_number);
	return handle_return(ret, "Error setting slot as unbootable: %s\n");
}

static int do_is_slot_bootable(bootctl_t_v1_0 *module, Slot slot_number) {
	int ret = module->isSlotBootable(slot_number);
	if (ret)
		fprintf(stdout, "slot %u is bootable\n", slot_number);
	return handle_return(!ret, "Error calling isSlotBootable(): %s\n");
}

static int do_is_slot_marked_successful(bootctl_t_v1_0 *module, Slot slot_number) {
	int ret = module->isSlotMarkedSuccessful(slot_number);
	if (ret == 0)
		fprintf(stdout, "slot %u is marked successful\n", slot_number);
	return handle_return(ret, "Error calling isSlotMarkedSuccessful(): %s\n");
}

static MergeStatus stringToMergeStatus(const char *status)
{
	if (strncmp(status, "cancelled", 8) == 0)
		return CANCELLED;

	if (strncmp(status, "merging", 7) == 0)
		return MERGING;

	if (strncmp(status, "none", 4) == 0)
		return NONE;

	if (strncmp(status, "snapshotted", 11) == 0)
		return SNAPSHOTTED;

	if (strncmp(status, "unknown", 7) == 0)
		return UNKNOWN;

	return NOVALUE;
}

static int do_set_snapshot_merge_status(bootctl_t_v1_1 *module, BootCtlVersion bootVersion, int argc, char* argv[])
{
	if (argc != 3) {
		usage(stderr, bootVersion, argc, argv);
		return -1;
	}

	MergeStatus status = stringToMergeStatus(argv[2]);
	if (status == NOVALUE) {
		usage(stderr, bootVersion, argc, argv);
		return -1;
	}

	if (!module->setSnapshotMergeStatus(status)) {
		return EX_SOFTWARE;
	}
	return EX_OK;
}

static const char *mergeStatusTostring(MergeStatus state)
{
	switch (state) {
		case CANCELLED:
			return "cancelled";
		case MERGING:
			return "merging";
		case NONE:
			return "none";
		case SNAPSHOTTED:
			return "snapshotted";
		case UNKNOWN:
			return "unknown";
		default:
			return NULL;
	}
}

static int do_get_snapshot_merge_status(bootctl_t_v1_1 *module)
{
	MergeStatus ret = module->getSnapshotMergeStatus();
	fprintf(stdout, "%s\n", mergeStatusTostring(ret));
	return EX_OK;
}

static int do_get_suffix(bootctl_t_v1_0 *module, Slot slot_number)
{
	const char *ret = module->getSuffix(slot_number);
	if (ret == NULL) {
		fprintf(stderr, "Error calling getSuffix()\n");
		return EX_SOFTWARE;
	}
	printf("%s\n", ret);
	return EX_OK;
}

static uint32_t parse_slot(BootCtlVersion bootVersion, int pos, int argc, char* argv[])
{
	if (pos > argc - 1) {
		usage(stderr, bootVersion, argc, argv);
		return -1;
	}
	errno = 0;
	uint64_t ret = strtoul(argv[pos], NULL, 10);
	if (errno != 0 || ret > UINT_MAX) {
		usage(stderr, bootVersion, argc, argv);
		return -1;
	}
	return (uint32_t)ret;
}

int main(int argc, char* argv[])
{
	bootctl_t_v1_0 *v1_0_module = &bootctl_impl_v1_0;
	bootctl_t_v1_1 *v1_1_module = &bootctl_impl_v1_1;
	bootctl_t_v1_2 *v1_2_module = &bootctl_impl_v1_2;

	BootCtlVersion bootVersion = BOOTCTL_V1_0;

	if (argc < 2) {
		usage(stderr, bootVersion, argc, argv);
		return EX_USAGE;
	}

	// Functions present from version 1.0
	if (strcmp(argv[1], "hal-info") == 0) {
		return do_hal_info(v1_0_module);
	} else if (strcmp(argv[1], "get-number-slots") == 0) {
		return do_get_number_slots(v1_0_module);
	} else if (strcmp(argv[1], "get-current-slot") == 0) {
		return do_get_current_slot(v1_0_module);
	} else if (strcmp(argv[1], "mark-boot-successful") == 0) {
		return do_mark_boot_successful(v1_0_module);
	} else if (strcmp(argv[1], "set-active-boot-slot") == 0) {
		return do_set_active_boot_slot(v1_0_module, parse_slot(bootVersion, 2, argc, argv));
	} else if (strcmp(argv[1], "set-slot-as-unbootable") == 0) {
		return do_set_slot_as_unbootable(v1_0_module, parse_slot(bootVersion, 2, argc, argv));
	} else if (strcmp(argv[1], "is-slot-bootable") == 0) {
		return do_is_slot_bootable(v1_0_module, parse_slot(bootVersion, 2, argc, argv));
	} else if (strcmp(argv[1], "is-slot-marked-successful") == 0) {
		return do_is_slot_marked_successful(v1_0_module, parse_slot(bootVersion, 2, argc, argv));
	} else if (strcmp(argv[1], "get-suffix") == 0) {
		return do_get_suffix(v1_0_module, parse_slot(bootVersion, 2, argc, argv));
	}

	// Functions present from version 1.1
	if (strcmp(argv[1], "set-snapshot-merge-status") == 0 ||
		strcmp(argv[1], "get-snapshot-merge-status") == 0) {
		if (v1_1_module == NULL) {
			fprintf(stderr, "Error getting bootctrl v1.1 module.\n");
			return EX_SOFTWARE;
		}
		if (strcmp(argv[1], "set-snapshot-merge-status") == 0) {
			return do_set_snapshot_merge_status(v1_1_module, bootVersion, argc, argv);
		} else if (strcmp(argv[1], "get-snapshot-merge-status") == 0) {
			return do_get_snapshot_merge_status(v1_1_module);
		}
	}

	if (strcmp(argv[1], "get-active-boot-slot") == 0) {
		if (v1_2_module == NULL) {
			fprintf(stderr, "Error getting bootctrl v1.2 module.\n");
			return EX_SOFTWARE;
		}

		return do_get_active_boot_slot(v1_2_module);
	}

	// Parameter not matched, print usage
	usage(stderr, bootVersion, argc, argv);
	return EX_USAGE;
}
