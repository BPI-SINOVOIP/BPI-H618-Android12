/*
**********************************************************************************************************************
*											        eGon
*						                     the Embedded System
*									       boot pack sub-system
*
*						  Copyright(C), 2006-2017, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : part.c
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include "sunxi_mbr.h"
#include "script.h"
#include "types.h"
#include "file.h"
#include "part.h"
#include "dos_part.h"
#include "crc.h"

char IMG_NAME[MAX_PATH];

int get_partition_file(char *part_name, char *file_name)
{
	int part_handle;
	int  value[8], ret;
	char fullname[260];
	char filename[32];

	part_handle = script_parser_fetch_partition();
	if (part_handle <= 0) {
		return -1;
	}
	memset(value, 0, 8 * sizeof(int));
	if (!script_parser_fetch_mainkey_sub("name", part_handle, value)) {
		if (!strcmp((char *)value, part_name)) {
			memset(fullname, 0, 260);
			ret = script_parser_fetch_mainkey_sub(
			    "downloadfile", part_handle, (int *)fullname);
			if (!ret) {
				memset(filename, 0, 32);
				get_file_name(fullname, filename);
				memcpy(file_name, filename, sizeof(filename));
				printf("part_name=%s, downloadfile=%s\n",
				       part_name, file_name);
				return 0;
			}
		}
	}

	return -1;
}

int get_img_name(char *img_name)
{
	strncpy(img_name, IMG_NAME, sizeof(IMG_NAME));
	return 0;
}

int set_img_name(char *img_name)
{
	memset(IMG_NAME, 0, sizeof(IMG_NAME));
	strncpy(IMG_NAME, img_name, sizeof(IMG_NAME));
	return 0;
}

int write_normal_part2img(FILE *img_file, sunxi_mbr_t *mbr_info, int mbr_size)
{
	char filename[320];
	char filepath[320];
	int i, ret;
	unsigned crc32_total;
	off_t fill_size;
	off_t src_size;
	off_t empty_part_fill_size = 0;

	FILE  *src_file = NULL;

	crc32_total = calc_crc32((void *)&(mbr_info->version), (sizeof(sunxi_mbr_t) - 4));
	if (crc32_total != mbr_info->crc32) {
		printf("mbr crr err,src=%u,dst=%u\n", mbr_info->crc32,
		       crc32_total);
		return -1;
	}

	for (i = 0; i < mbr_info->PartCount; i++) {
		memset(filename, 0, sizeof(filename));
		printf("mbr_info->array[%d].name=%s\n", i, mbr_info->array[i].name);
		// printf("mbr_info->array[%d].addrlo=%x\n", i, mbr_info->array[i].addrlo);

		fill_size = (((off_t)(mbr_info->array[i].lenhi) << 32) |
			     mbr_info->array[i].lenlo) *
			    512;
		memset(filepath, 0, sizeof(filepath));
		ret = get_partition_file(mbr_info->array[i].name, filename);
		if (!ret) {
			if (!strncmp("UDISK", mbr_info->array[i].name,
				     sizeof("UDISK"))) {
				continue;
			} else {
				if (!strncmp("system", mbr_info->array[i].name,
					     sizeof("system"))) {
					memset(filename, 0, sizeof(filename));
					memcpy(filename, "system_ext4.fex",
					       sizeof("system_ext4.fex"));
				} else if (!strncmp("vendor", mbr_info->array[i].name,
							sizeof("vendor"))) {
					memset(filename, 0, sizeof(filename));
					memcpy(filename, "vendor_ext4.fex",
							sizeof("vendor_ext4.fex"));
				} else if (!strncmp("sysrecovery",
						    mbr_info->array[i].name,
						    sizeof("sysrecovery"))) {
					get_img_name(filename);
				}

				GetFullPath(filepath, filename);
				printf("src=%s\n", filepath);
				src_file = fopen(filepath, "rb");
				if (!src_file) {
					printf("unable to open %s file\n",
					       filepath);
					return -1;
				}

				if (empty_part_fill_size) {
					fill_file_data(img_file, empty_part_fill_size);
					empty_part_fill_size = 0;
				}

				if (!append_file_data(src_file, &src_size,
						      img_file)) {
					printf("line=%d, src_size=%zu, "
					       "fill_size=%zu\n",
					       __LINE__, src_size, fill_size);
					fill_size = fill_size - src_size;
					fill_file_data(img_file, fill_size);
				}
				fclose(src_file);
			}
		} else {
			printf("line=%d, fill_size=%zu\n", __LINE__, fill_size);
			empty_part_fill_size += fill_size;
		}
	}

	return 0;
}

int write_mbr_file(char *mbr_name, FILE *img_file, int mbr_fill_size)
{
	int ret = 0;
	off_t mbr_size;
	FILE *mbr_file = NULL;
	off_t fill_size = mbr_fill_size * 512;

	mbr_file = fopen(mbr_name, "rb");
	if (!mbr_file) {
		printf("can't open %s file\n", mbr_name);
		goto _err_out;
	}

	ret = append_file_data(mbr_file, &mbr_size, img_file);
	if (!ret) {
		printf("line=%d, mbr_size=%zu,fill_size=%zu\n", __LINE__, mbr_size, fill_size);
		fill_size = fill_size - mbr_size;
		fill_file_data(img_file, fill_size);
	} else {
		printf("write mbr %s file failed\n", mbr_name);
		goto _err_out;
	}

_err_out:
	if (mbr_file)
		fclose(mbr_file);

	return ret;
}

int sunxi_sprite_size(void)
{
	return EMMC_CAPACITY * (1024 / 512);
}

int write_standar_mbr_file(sunxi_mbr_t *mbr_info, FILE *img_file)
{
	mbr_stand *mbrst;
	char mbr_bufst[512];
	sunxi_mbr_t *mbr = mbr_info;
	int i, sectors = 0, unusd_sectors = 0;

	for (i = 1; i < mbr->PartCount - 1; i++) {
		memset(mbr_bufst, 0, sizeof(mbr_bufst));
		mbrst = (mbr_stand *)mbr_bufst;

		sectors += mbr->array[i].lenlo;

		mbrst->part_info[0].part_type = 0x83;
		mbrst->part_info[0].start_sectorl =
		    ((mbr->array[i].addrlo - i + 20 * 1024 * 1024 / 512) &
		     0x0000ffff) >>
		    0;
		mbrst->part_info[0].start_sectorh =
		    ((mbr->array[i].addrlo - i + 20 * 1024 * 1024 / 512) &
		     0xffff0000) >>
		    16;
		mbrst->part_info[0].total_sectorsl =
		    (mbr->array[i].lenlo & 0x0000ffff) >> 0;
		mbrst->part_info[0].total_sectorsh =
		    (mbr->array[i].lenlo & 0xffff0000) >> 16;

		if (i != mbr->PartCount - 2) {
			mbrst->part_info[1].part_type = 0x05;
			mbrst->part_info[1].start_sectorl = i;
			mbrst->part_info[1].start_sectorh = 0;
			mbrst->part_info[1].total_sectorsl =
			    (mbr->array[i].lenlo & 0x0000ffff) >> 0;
			mbrst->part_info[1].total_sectorsh =
			    (mbr->array[i].lenlo & 0xffff0000) >> 16;
		}

		mbrst->end_flag = 0xAA55;
		if (write_data2file(img_file, i * 512, mbr_bufst, 512)) {
			printf("write standard mbr %d failed\n", i);

			return -1;
		}
	}
	memset(mbr_bufst, 0, 512);
	mbrst = (mbr_stand *)mbr_bufst;

	unusd_sectors = sunxi_sprite_size() - 20 * 1024 * 1024/512 - sectors;
	mbrst->part_info[0].indicator = 0x80;
	mbrst->part_info[0].part_type = 0x0B;
	mbrst->part_info[0].start_sectorl  = ((mbr->array[mbr->PartCount-1].addrlo + 20 * 1024 * 1024/512 ) & 0x0000ffff) >> 0;
	mbrst->part_info[0].start_sectorh  = ((mbr->array[mbr->PartCount-1].addrlo + 20 * 1024 * 1024/512 ) & 0xffff0000) >> 16;
	mbrst->part_info[0].total_sectorsl = ( unusd_sectors & 0x0000ffff) >> 0;
	mbrst->part_info[0].total_sectorsh = ( unusd_sectors & 0xffff0000) >> 16;

	mbrst->part_info[1].part_type = 0x06;
	mbrst->part_info[1].start_sectorl  = ((mbr->array[0].addrlo + 20 * 1024 * 1024/512) & 0x0000ffff) >> 0;
	mbrst->part_info[1].start_sectorh  = ((mbr->array[0].addrlo + 20 * 1024 * 1024/512) & 0xffff0000) >> 16;
	mbrst->part_info[1].total_sectorsl = (mbr->array[0].lenlo  & 0x0000ffff) >> 0;
	mbrst->part_info[1].total_sectorsh = (mbr->array[0].lenlo  & 0xffff0000) >> 16;

	mbrst->part_info[2].part_type = 0x05;
	mbrst->part_info[2].start_sectorl  = 1;
	mbrst->part_info[2].start_sectorh  = 0;
	mbrst->part_info[2].total_sectorsl = (sectors & 0x0000ffff) >> 0;
	mbrst->part_info[2].total_sectorsh = (sectors & 0xffff0000) >> 16;

	mbrst->end_flag = 0xAA55;
	if (write_data2file(img_file, 0, mbr_bufst, 512)) {
		printf("write standard mbr 0 failed\n");

		return -1;
	}

	return 0;
}

int write_boot_part(char *boot0_name, char *boot1_name, FILE *img_file)
{
	int ret;
	FILE  *boot0_file = NULL;
	FILE  *boot1_file = NULL;
	off_t boot0_size = 0;
	off_t boot1_size = 0;

	boot0_file = fopen(boot0_name, "rb");
	if (!boot0_file) {
		printf("unable to open %s file\n", boot0_name);
		ret = -1;
		goto _err_out;
	}

	ret = write_file_data(boot0_file, &boot0_size, img_file, (16 * 512));
	if (ret) {
		printf("write boot0 err\n");
		goto _err_out;
	}

	ret = write_file_data(boot0_file, &boot0_size, img_file, (256 * 512));
	if (ret) {
		printf("write boot0 bak err\n");
		goto _err_out;
	}

	boot1_file = fopen(boot1_name, "rb");
	if (!boot1_file) {
		printf("unable to open %s file\n", boot1_name);
		ret = -1;
		goto _err_out;
	}

	ret = write_file_data(boot1_file, &boot1_size, img_file, (24576 * 512));
	if (ret) {
		printf("write boot1 bak err\n");
		goto _err_out;
	}

	ret = write_file_data(boot1_file, &boot1_size, img_file, (32800 * 512));
	if (ret) {
		printf("write boot1 bak err\n");
		goto _err_out;
	}

_err_out:
	if (boot0_file) {
		fclose(boot0_file);
		boot0_file = NULL;
	}

	if (boot1_file) {
		fclose(boot1_file);
		boot1_file = NULL;
	}

	return ret;
}

int write_gpt_file(char *gpt_name, FILE *img_file)
{
	int ret;
	FILE  *gpt_file = NULL;
	off_t gpt_size = 0;

	gpt_file = fopen(gpt_name, "rb");
	if (!gpt_file) {
		printf("unable to open %s file\n", gpt_name);
		ret = -1;
		goto _err_out;
	}

	ret = write_file_data(gpt_file, &gpt_size, img_file, (0 * 512));
	if (ret) {
		printf("write gpt err\n");
		goto _err_out;
	}

	ret = write_file_data(gpt_file, &gpt_size, img_file, (40960 * 512));
	if (ret) {
		printf("write gpt bak err\n");
		goto _err_out;
	}

_err_out:
	if (gpt_file) {
		fclose(gpt_file);
		gpt_file = NULL;
	}

	return ret;

}
